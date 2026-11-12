import json
import os
from datetime import datetime, timezone
from fastapi import FastAPI, Header, HTTPException, Query
from pydantic import BaseModel, Field
from . import storage
from .adapters import AdapterError, fetch_nbp, fetch_rest_ohlcv, fetch_rest_valuation

app=FastAPI(title="Monitor Rynków — Market Data API",version="0.1.1")

class SourceIn(BaseModel):
    name:str=Field(min_length=1,max_length=120)
    kind:str
    config:dict

def require_admin(token):
    expected=os.getenv("ADMIN_API_TOKEN")
    if not expected:
        raise HTTPException(503,"ADMIN_API_TOKEN nie jest skonfigurowany.")
    if token != expected:
        raise HTTPException(401,"Nieprawidłowy token administracyjny.")

@app.on_event("startup")
def startup():
    storage.init_db()

@app.get("/health")
def health():
    return {"status":"ok","time":datetime.now(timezone.utc).isoformat()}

@app.post("/sources")
def create_source(body:SourceIn,x_admin_token:str|None=Header(default=None)):
    require_admin(x_admin_token)
    if body.kind not in ("nbp_fx","rest_ohlcv","rest_valuation"):
        raise HTTPException(400,"Nieobsługiwany adapter.")
    try:
        source_id=storage.insert_source(body.name,body.kind,body.config)
    except Exception as exc:
        raise HTTPException(400,str(exc))
    return {"id":source_id,"name":body.name,"kind":body.kind,"enabled":True}

@app.get("/sources")
def list_sources():
    rows=storage.rows("SELECT id,name,kind,enabled,created_at FROM sources ORDER BY id")
    return [dict(r) for r in rows]

@app.post("/sources/{source_id}/ingest")
async def ingest(source_id:int,x_admin_token:str|None=Header(default=None)):
    require_admin(x_admin_token)
    found=storage.rows("SELECT * FROM sources WHERE id=?",(source_id,))
    if not found:
        raise HTTPException(404,"Nie znaleziono źródła.")
    source=dict(found[0])
    if not source["enabled"]:
        raise HTTPException(409,"Źródło jest wyłączone.")
    config=json.loads(source["config_json"])
    try:
        if source["kind"]=="nbp_fx": data=await fetch_nbp(config)
        elif source["kind"]=="rest_ohlcv": data=await fetch_rest_ohlcv(config)
        else: data=await fetch_rest_valuation(config)
        for item in data:
            if source["kind"]=="rest_valuation": storage.insert_valuation(source_id,item)
            else: storage.insert_candle(source_id,item)
    except AdapterError as exc:
        raise HTTPException(422,str(exc))
    except Exception as exc:
        raise HTTPException(502,f"Błąd dostawcy danych: {type(exc).__name__}")
    return {"source_id":source_id,"records_received":len(data),"stored_at":datetime.now(timezone.utc).isoformat()}

@app.get("/candles")
def candles(symbol:str,interval:str="1d",start:str|None=None,end:str|None=None,limit:int=Query(1000,ge=1,le=10000)):
    clauses=["symbol=?","interval=?"]; params=[symbol,interval]
    if start: clauses.append("time>=?"); params.append(start)
    if end: clauses.append("time<=?"); params.append(end)
    params.append(limit)
    rows=storage.rows("SELECT * FROM candles WHERE "+" AND ".join(clauses)+" ORDER BY time DESC LIMIT ?",tuple(params))
    return [dict(r) for r in reversed(rows)]

@app.get("/valuations")
def valuations(symbol:str,start:str|None=None,end:str|None=None,limit:int=Query(1000,ge=1,le=10000)):
    clauses=["symbol=?"]; params=[symbol]
    if start: clauses.append("observed_at>=?"); params.append(start)
    if end: clauses.append("observed_at<=?"); params.append(end)
    params.append(limit)
    rows=storage.rows("SELECT * FROM valuation_observations WHERE "+" AND ".join(clauses)+" ORDER BY observed_at DESC LIMIT ?",tuple(params))
    return [dict(r) for r in reversed(rows)]

# Additive router: original source, candle and valuation routes remain available.
from .providers_api import router as open_providers_router
app.include_router(open_providers_router)
