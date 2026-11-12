"""Official/open market data adapters for NBP, Frankfurter v2 and World Bank Pink Sheet."""
from __future__ import annotations
import io, os, re, json, calendar
from datetime import date, datetime, timedelta, timezone
from urllib.parse import urljoin, urlparse
import httpx
from . import storage

NBP = "https://api.nbp.pl/api"
FRANKFURTER = "https://api.frankfurter.dev/v2"
WORLD_BANK_LANDING = "https://www.worldbank.org/en/research/commodity-markets"
WORLD_BANK_FALLBACK = "https://thedocs.worldbank.org/en/doc/74e8be41ceb20fa0da750cda2f6b9e4e-0050012026/related/CMO-Historical-Data-Monthly.xlsx"

class ProviderError(ValueError): pass

def _date(value):
    return date.fromisoformat(str(value)[:10])

def _chunks_93(start: date, end: date):
    cursor=start
    while cursor <= end:
        stop=min(end,cursor+timedelta(days=92))
        yield cursor,stop
        cursor=stop+timedelta(days=1)

def init_provider_tables():
    """Create provider tables using DDL compatible with SQLite and PostgreSQL."""
    sqlite = storage.DATABASE_URL.startswith("sqlite:///")
    id_column = "INTEGER PRIMARY KEY AUTOINCREMENT" if sqlite else "BIGSERIAL PRIMARY KEY"
    storage.execute(f"""CREATE TABLE IF NOT EXISTS fx_rates (
      id {id_column}, source_id BIGINT NOT NULL,
      provider TEXT NOT NULL, rate_date TEXT NOT NULL, base TEXT NOT NULL,
      quote TEXT NOT NULL, mid DOUBLE PRECISION, bid DOUBLE PRECISION, ask DOUBLE PRECISION,
      rate_kind TEXT NOT NULL, received_at TEXT NOT NULL,
      UNIQUE(provider,rate_date,base,quote,rate_kind)
    )""")
    storage.execute("CREATE INDEX IF NOT EXISTS ix_fx_pair_date ON fx_rates(base,quote,rate_date)")
    storage.execute(f"""CREATE TABLE IF NOT EXISTS metal_prices (
      id {id_column}, source_id BIGINT NOT NULL,
      provider TEXT NOT NULL, symbol TEXT NOT NULL, price_date TEXT NOT NULL,
      price DOUBLE PRECISION NOT NULL, currency TEXT NOT NULL, unit TEXT NOT NULL,
      frequency TEXT NOT NULL, price_kind TEXT NOT NULL, received_at TEXT NOT NULL,
      UNIQUE(provider,symbol,price_date,price_kind)
    )""")
    storage.execute("CREATE INDEX IF NOT EXISTS ix_metal_symbol_date ON metal_prices(symbol,price_date)")

def _insert_fx(source_id, provider, row):
    now=datetime.now(timezone.utc).isoformat()
    storage.execute("""INSERT INTO fx_rates(source_id,provider,rate_date,base,quote,mid,bid,ask,rate_kind,received_at)
      VALUES(?,?,?,?,?,?,?,?,?,?) ON CONFLICT(provider,rate_date,base,quote,rate_kind)
      DO UPDATE SET mid=excluded.mid,bid=excluded.bid,ask=excluded.ask,received_at=excluded.received_at""",
      (source_id,provider,row["date"],row["base"],row["quote"],row.get("mid"),row.get("bid"),
       row.get("ask"),row["rate_kind"],now))

def _insert_metal(source_id, provider, row):
    now=datetime.now(timezone.utc).isoformat()
    storage.execute("""INSERT INTO metal_prices(source_id,provider,symbol,price_date,price,currency,unit,frequency,price_kind,received_at)
      VALUES(?,?,?,?,?,?,?,?,?,?) ON CONFLICT(provider,symbol,price_date,price_kind)
      DO UPDATE SET price=excluded.price,currency=excluded.currency,unit=excluded.unit,
      frequency=excluded.frequency,received_at=excluded.received_at""",
      (source_id,provider,row["symbol"],row["date"],row["price"],row["currency"],row["unit"],
       row["frequency"],row["price_kind"],now))

async def _json_get(client,url,params=None):
    response=await client.get(url,params=params,headers={"Accept":"application/json"})
    if response.status_code==404: return None
    response.raise_for_status()
    return response.json()

async def import_nbp(start: str, end: str, tables=("A","B","C"), include_gold=True):
    """Import historical NBP FX tables and 1g gold reference price; NBP limits a query to 93 days."""
    first,last=_date(start),_date(end)
    if first>last: raise ProviderError("Data początkowa jest późniejsza niż końcowa.")
    source_id=storage.insert_source("NBP Web API history","nbp_history",{"tables":list(tables),"start":start,"end":end})
    init_provider_tables()
    counts={"fx":0,"gold":0}
    async with httpx.AsyncClient(timeout=40,follow_redirects=False) as client:
        for table in tables:
            if table not in ("A","B","C"): raise ProviderError("Dozwolone tabele NBP: A, B, C.")
            for a,b in _chunks_93(first,last):
                url=f"{NBP}/exchangerates/tables/{table}/{a.isoformat()}/{b.isoformat()}/"
                payload=await _json_get(client,url,{"format":"json"})
                if not payload: continue
                for daily in payload:
                    effective=daily.get("effectiveDate") or daily.get("tradingDate")
                    for item in daily.get("rates",[]):
                        common={"date":effective,"base":"PLN","quote":item["code"]}
                        if table in ("A","B"):
                            _insert_fx(source_id,"nbp",dict(common,mid=float(item["mid"]),rate_kind=f"official_mid_table_{table}"))
                        else:
                            bid=float(item["bid"]); ask=float(item["ask"])
                            _insert_fx(source_id,"nbp",dict(common,bid=bid,ask=ask,
                              mid=(bid+ask)/2,rate_kind="table_c_bid_ask_midpoint_derived"))
                        counts["fx"]+=1
        if include_gold:
            for a,b in _chunks_93(first,last):
                url=f"{NBP}/cenyzlota/{a.isoformat()}/{b.isoformat()}/"
                payload=await _json_get(client,url,{"format":"json"})
                for item in (payload or []):
                    # NBP response keys: data, cena; 1 gram, 1000 fineness, PLN.
                    _insert_metal(source_id,"nbp",{"symbol":"XAU","date":item["data"],
                      "price":float(item["cena"]),"currency":"PLN","unit":"gram",
                      "frequency":"1d","price_kind":"official_reference_price_1g"})
                    counts["gold"]+=1
    return {"source_id":source_id,**counts}

async def import_frankfurter(start: str, end: str, base="EUR", quotes=("USD","PLN","GBP","CHF"),
                             provider=None):
    first,last=_date(start),_date(end)
    if first>last: raise ProviderError("Data początkowa jest późniejsza niż końcowa.")
    source_id=storage.insert_source("Frankfurter v2 history","frankfurter_v2",
      {"start":start,"end":end,"base":base,"quotes":list(quotes),"provider":provider})
    init_provider_tables()
    count=0
    async with httpx.AsyncClient(timeout=60,follow_redirects=False) as client:
        for a,b in _chunks_93(first,last):
            params={"base":base.upper(),"quotes":",".join(q.upper() for q in quotes),
                    "from":a.isoformat(),"to":b.isoformat()}
            if provider: params["providers"]=provider
            payload=await _json_get(client,f"{FRANKFURTER}/rates",params)
            for item in (payload or []):
                # v2 rows are date/base/quote/rate; rate is Frankfurter's mid-market reference.
                _insert_fx(source_id,"frankfurter"+(f":{provider}" if provider else ":blend"),{
                  "date":item["date"],"base":item["base"].upper(),"quote":item["quote"].upper(),
                  "mid":float(item["rate"]),"rate_kind":"frankfurter_mid_market"})
                count+=1
    return {"source_id":source_id,"fx":count}

def _month_date(period):
    match=re.fullmatch(r"(\d{4})M(\d{2})",str(period).strip(),re.I)
    if not match: return None
    return f"{match.group(1)}-{match.group(2)}-01"

def parse_world_bank_workbook(content: bytes):
    try:
        from openpyxl import load_workbook
    except ImportError as exc:
        raise ProviderError("Brak openpyxl; zainstaluj zależności backendu.") from exc
    wb=load_workbook(io.BytesIO(content),read_only=True,data_only=True)
    if "Monthly Prices" not in wb.sheetnames:
        raise ProviderError("Arkusz Monthly Prices nie istnieje w pliku World Bank.")
    ws=wb["Monthly Prices"]
    rows=ws.iter_rows(values_only=True)
    header=None; header_idx=None; buffered=[]
    for idx,row in enumerate(rows):
        vals=[str(x).strip() if x is not None else "" for x in row]
        lowered=[v.lower() for v in vals]
        gold=next((i for i,v in enumerate(lowered) if "gold" in v),None)
        silver=next((i for i,v in enumerate(lowered) if "silver" in v),None)
        if gold is not None and silver is not None:
            header=vals; header_idx=idx; break
    if header is None: raise ProviderError("Nie znaleziono kolumn Gold i Silver w arkuszu.")
    # Unit row follows the commodity header; Pink Sheet monthly prices are USD per troy ounce.
    result=[]
    for row in rows:
        if not row: continue
        period=_month_date(row[0])
        if not period: continue
        for symbol,needle in (("XAU","gold"),("XAG","silver")):
            col=next(i for i,v in enumerate(header) if needle in v.lower())
            raw=row[col] if col<len(row) else None
            if raw is None or str(raw).strip() in ("...","—","-"): continue
            try: price=float(raw)
            except (TypeError,ValueError): continue
            result.append({"symbol":symbol,"date":period,"price":price,"currency":"USD",
              "unit":"troy_ounce","frequency":"1mo","price_kind":"world_bank_monthly_average"})
    wb.close()
    if not result: raise ProviderError("Arkusz nie zawiera rozpoznanych miesięcznych cen złota/srebra.")
    return result

async def _world_bank_monthly_url(client):
    override=os.getenv("WORLD_BANK_MONTHLY_XLSX_URL")
    if override: return override
    response=await client.get(WORLD_BANK_LANDING,follow_redirects=False)
    response.raise_for_status()
    html=response.text
    # Find the current workbook URL from the official World Bank landing page; release path rotates.
    matches=re.findall(r"""href=["\']([^"\']*CMO-Historical-Data-Monthly\.xlsx[^"\']*)["\']""",html,re.I)
    for href in matches:
        candidate=urljoin(WORLD_BANK_LANDING,href)
        if urlparse(candidate).hostname=="thedocs.worldbank.org" and candidate.startswith("https://"):
            return candidate
    return WORLD_BANK_FALLBACK

async def import_world_bank(start: str|None=None,end: str|None=None):
    source_id=storage.insert_source("World Bank Pink Sheet monthly","world_bank_pink_sheet",{
      "landing_page":WORLD_BANK_LANDING,"series":["XAU","XAG"],"frequency":"1mo"})
    init_provider_tables()
    async with httpx.AsyncClient(timeout=120,follow_redirects=False) as client:
        url=await _world_bank_monthly_url(client)
        if urlparse(url).hostname!="thedocs.worldbank.org" or not url.startswith("https://"):
            raise ProviderError("Nieprawidłowy host pliku World Bank.")
        response=await client.get(url,follow_redirects=False)
        response.raise_for_status()
        if len(response.content)>30_000_000: raise ProviderError("Plik World Bank przekracza 30 MB.")
        rows=parse_world_bank_workbook(response.content)
    first=_date(start) if start else None
    last=_date(end) if end else None
    count=0
    for row in rows:
        d=_date(row["date"])
        if first and d<first: continue
        if last and d>last: continue
        _insert_metal(source_id,"world_bank_pink_sheet",row); count+=1
    return {"source_id":source_id,"metal_prices":count,"workbook_url":url}

def get_fx(base,quote,start=None,end=None,limit=5000):
    clauses=["base=?","quote=?"]; params=[base.upper(),quote.upper()]
    if start: clauses.append("rate_date>=?"); params.append(start)
    if end: clauses.append("rate_date<=?"); params.append(end)
    params.append(limit)
    return [dict(r) for r in storage.rows("SELECT * FROM fx_rates WHERE "+" AND ".join(clauses)+
      " ORDER BY rate_date LIMIT ?",tuple(params))]

def get_metals(symbol,start=None,end=None,limit=5000):
    clauses=["symbol=?"]; params=[symbol.upper()]
    if start: clauses.append("price_date>=?"); params.append(start)
    if end: clauses.append("price_date<=?"); params.append(end)
    params.append(limit)
    return [dict(r) for r in storage.rows("SELECT * FROM metal_prices WHERE "+" AND ".join(clauses)+
      " ORDER BY price_date LIMIT ?",tuple(params))]
