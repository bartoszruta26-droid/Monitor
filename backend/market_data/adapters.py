import os
import ipaddress
import socket
from datetime import datetime, timezone
from urllib.parse import urlparse
import httpx

class AdapterError(ValueError):
    pass

def utc_iso(value, unit="iso"):
    if isinstance(value, (int, float)):
        scale = {"s": 1, "ms": 1000}.get(unit)
        if not scale:
            raise AdapterError("Dla znacznika liczbowego ustaw timestamp_unit na s lub ms.")
        dt = datetime.fromtimestamp(value / scale, tz=timezone.utc)
    else:
        dt = datetime.fromisoformat(str(value).replace("Z","+00:00"))
        if dt.tzinfo is None:
            dt = dt.replace(tzinfo=timezone.utc)
        dt = dt.astimezone(timezone.utc)
    return dt.isoformat()

def validate_source_url(url):
    parsed = urlparse(url)
    if parsed.scheme != "https" or not parsed.hostname:
        raise AdapterError("Źródło musi używać HTTPS i zawierać nazwę hosta.")
    host = parsed.hostname.lower()
    allowed = [x.strip().lower() for x in os.getenv("ALLOWED_SOURCE_HOSTS","").split(",") if x.strip()]
    if allowed and host not in allowed:
        raise AdapterError("Host nie znajduje się na liście ALLOWED_SOURCE_HOSTS.")
    try:
        ip = ipaddress.ip_address(host)
        if not ip.is_global:
            raise AdapterError("Adres prywatny/lokalny jest zablokowany.")
    except ValueError:
        # Resolve host and reject private, loopback, link-local and reserved IPs.
        try:
            infos = socket.getaddrinfo(host, parsed.port or 443, type=socket.SOCK_STREAM)
        except socket.gaierror as exc:
            raise AdapterError("Nie można rozwiązać nazwy hosta.") from exc
        for info in infos:
            addr = ipaddress.ip_address(info[4][0])
            if not addr.is_global:
                raise AdapterError("Host rozwiązuje się do niedozwolonego adresu IP.")
    return url

def _get_path(obj, path):
    cur = obj
    for part in path.split("."):
        if isinstance(cur, list):
            cur = cur[int(part)]
        else:
            cur = cur[part]
    return cur

def _number(v, field):
    try:
        n = float(v)
    except (TypeError, ValueError) as exc:
        raise AdapterError(f"Pole {field} nie jest liczbą.") from exc
    if n != n or abs(n) == float("inf"):
        raise AdapterError(f"Pole {field} ma nieprawidłową wartość.")
    return n

async def fetch_nbp(config):
    table = config.get("table","A")
    code = config["code"].upper()
    if table not in ("A","B"):
        raise AdapterError("NBP obsługuje tabele A lub B dla tego adaptera.")
    url = f"https://api.nbp.pl/api/exchangerates/rates/{table}/{code}/last/90/?format=json"
    async with httpx.AsyncClient(timeout=15, follow_redirects=False) as client:
        response = await client.get(url, headers={"Accept":"application/json"})
        response.raise_for_status()
        payload = response.json()
    symbol = f"{code}{config.get('base_currency','PLN').upper()}"
    out=[]
    for item in payload.get("rates",[]):
        rate=_number(item["mid"],"mid")
        out.append({
            "symbol":symbol,"interval":"1d","time":utc_iso(item["effectiveDate"]),
            "open":rate,"high":rate,"low":rate,"close":rate,"volume":None,
            "currency":config.get("base_currency","PLN").upper(),"unit":"per_currency_unit",
            "data_kind":"reference_rate","quality":{"synthetic_ohlc":True,"source_table":table,
            "rate_no":item.get("no"),"note":"Kurs referencyjny NBP; nie jest transakcyjnym OHLC intraday."}
        })
    return out

async def fetch_rest_ohlcv(config):
    url=validate_source_url(config["url"])
    headers={"Accept":"application/json"}
    secret_env=config.get("secret_env")
    if secret_env:
        secret=os.getenv(secret_env)
        if not secret:
            raise AdapterError(f"Brak zmiennej środowiskowej sekretu: {secret_env}.")
        headers[config.get("auth_header","Authorization")]=config.get("auth_prefix","Bearer ")+secret
    async with httpx.AsyncClient(timeout=float(config.get("timeout_seconds",20)),follow_redirects=False) as client:
        response=await client.get(url,headers=headers)
        response.raise_for_status()
        if len(response.content)>5_000_000:
            raise AdapterError("Odpowiedź przekracza limit 5 MB.")
        payload=response.json()
    f=config.get("fields",{})
    items=_get_path(payload,f.get("items","data"))
    out=[]
    for row in items:
        x={
          "symbol":config["symbol"],"interval":config["interval"],
          "time":utc_iso(_get_path(row,f.get("time","time")),config.get("timestamp_unit","iso")),
          "open":_number(_get_path(row,f.get("open","open")),"open"),
          "high":_number(_get_path(row,f.get("high","high")),"high"),
          "low":_number(_get_path(row,f.get("low","low")),"low"),
          "close":_number(_get_path(row,f.get("close","close")),"close"),
          "volume":None if not f.get("volume") or f["volume"] not in row else _number(_get_path(row,f["volume"]),"volume"),
          "currency":config.get("currency"),"unit":config.get("unit"),"data_kind":"ohlcv","quality":{}
        }
        if x["high"] < max(x["open"],x["close"],x["low"]) or x["low"] > min(x["open"],x["close"],x["high"]):
            raise AdapterError("Niepoprawna świeca OHLC: zakres high/low jest niespójny.")
        if x["volume"] is not None and x["volume"] < 0:
            raise AdapterError("Wolumen nie może być ujemny.")
        out.append(x)
    return out

async def fetch_rest_valuation(config):
    url=validate_source_url(config["url"])
    headers={"Accept":"application/json"}
    secret_env=config.get("secret_env")
    if secret_env:
        secret=os.getenv(secret_env)
        if not secret:
            raise AdapterError(f"Brak zmiennej środowiskowej sekretu: {secret_env}.")
        headers[config.get("auth_header","Authorization")]=config.get("auth_prefix","Bearer ")+secret
    async with httpx.AsyncClient(timeout=float(config.get("timeout_seconds",20)),follow_redirects=False) as client:
        response=await client.get(url,headers=headers)
        response.raise_for_status()
        if len(response.content)>5_000_000:
            raise AdapterError("Odpowiedź przekracza limit 5 MB.")
        payload=response.json()
    f=config.get("fields",{})
    items=_get_path(payload,f.get("items","data"))
    out=[]
    for row in items:
        attrs={}
        for key in ("grade","weight","stone_type","amber_type","certificate","origin","treatment","color","clarity","cut"):
            path=f.get(key)
            if path:
                try: attrs[key]=_get_path(row,path)
                except (KeyError,IndexError,TypeError): pass
        out.append({
          "symbol":config["symbol"],
          "observed_at":utc_iso(_get_path(row,f.get("time","time")),config.get("timestamp_unit","iso")),
          "price":_number(_get_path(row,f.get("price","price")),"price"),
          "currency":config.get("currency"),"unit":config.get("unit"),
          "attributes":attrs
        })
    return out
