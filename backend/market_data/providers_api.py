from fastapi import APIRouter, Header, HTTPException, Query
from pydantic import BaseModel, Field
import os
from . import open_providers as p

router=APIRouter(prefix="/providers",tags=["Open market data"])

class NBPImport(BaseModel):
    start:str
    end:str
    tables:list[str]=Field(default_factory=lambda:["A","B","C"])
    include_gold:bool=True

class FrankfurterImport(BaseModel):
    start:str
    end:str
    base:str="EUR"
    quotes:list[str]=Field(default_factory=lambda:["USD","PLN","GBP","CHF"])
    provider:str|None=None

class WorldBankImport(BaseModel):
    start:str|None=None
    end:str|None=None

def _admin(token):
    expected=os.getenv("ADMIN_API_TOKEN")
    if not expected: raise HTTPException(503,"ADMIN_API_TOKEN nie jest skonfigurowany.")
    if token!=expected: raise HTTPException(401,"Nieprawidłowy token administracyjny.")

@router.on_event("startup")
def startup():
    p.init_provider_tables()

@router.post("/nbp/import")
async def nbp_import(body:NBPImport,x_admin_token:str|None=Header(default=None)):
    _admin(x_admin_token)
    try: return await p.import_nbp(body.start,body.end,tuple(body.tables),body.include_gold)
    except Exception as exc: raise HTTPException(502,f"Import NBP nie powiódł się: {type(exc).__name__}: {exc}")

@router.post("/frankfurter/import")
async def frankfurter_import(body:FrankfurterImport,x_admin_token:str|None=Header(default=None)):
    _admin(x_admin_token)
    try: return await p.import_frankfurter(body.start,body.end,body.base,tuple(body.quotes),body.provider)
    except Exception as exc: raise HTTPException(502,f"Import Frankfurter nie powiódł się: {type(exc).__name__}: {exc}")

@router.post("/world-bank/import")
async def world_bank_import(body:WorldBankImport,x_admin_token:str|None=Header(default=None)):
    _admin(x_admin_token)
    try: return await p.import_world_bank(body.start,body.end)
    except Exception as exc: raise HTTPException(502,f"Import World Bank nie powiódł się: {type(exc).__name__}: {exc}")

@router.get("/fx-rates")
def fx_rates(base:str,quote:str,start:str|None=None,end:str|None=None,
             limit:int=Query(5000,ge=1,le=50000)):
    try: return p.get_fx(base,quote,start,end,limit)
    except Exception as exc: raise HTTPException(500,f"Błąd odczytu kursów: {type(exc).__name__}")

@router.get("/metal-prices")
def metal_prices(symbol:str=Query(pattern="^(XAU|XAG)$"),start:str|None=None,end:str|None=None,
                 limit:int=Query(5000,ge=1,le=50000)):
    try: return p.get_metals(symbol,start,end,limit)
    except Exception as exc: raise HTTPException(500,f"Błąd odczytu metali: {type(exc).__name__}")
