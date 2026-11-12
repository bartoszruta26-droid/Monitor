import json
import os
import sqlite3
from contextlib import contextmanager
from datetime import datetime, timezone
from pathlib import Path

DATABASE_URL = os.getenv("DATABASE_URL", "sqlite:///./market_data.db")

def _connect():
    if DATABASE_URL.startswith("sqlite:///"):
        path = DATABASE_URL.removeprefix("sqlite:///")
        if path != ":memory:":
            Path(path).parent.mkdir(parents=True, exist_ok=True)
        conn = sqlite3.connect(path, check_same_thread=False)
        conn.row_factory = sqlite3.Row
        return conn
    if DATABASE_URL.startswith("postgresql://") or DATABASE_URL.startswith("postgres://"):
        try:
            import psycopg
            from psycopg.rows import dict_row
        except ImportError as exc:
            raise RuntimeError("Dla PostgreSQL zainstaluj psycopg[binary].") from exc
        return psycopg.connect(DATABASE_URL, row_factory=dict_row)
    raise ValueError("Nieobsługiwany DATABASE_URL; użyj sqlite:/// lub postgresql://")

@contextmanager
def connection():
    conn = _connect()
    try:
        yield conn
        conn.commit()
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()

def init_db():
    # Use portable SQL types and parameter placeholders per driver.
    with connection() as c:
        if DATABASE_URL.startswith("sqlite:///"):
            c.executescript("""
            CREATE TABLE IF NOT EXISTS sources (
              id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL,
              kind TEXT NOT NULL, config_json TEXT NOT NULL,
              enabled INTEGER NOT NULL DEFAULT 1, created_at TEXT NOT NULL
            );
            CREATE TABLE IF NOT EXISTS candles (
              id INTEGER PRIMARY KEY AUTOINCREMENT, source_id INTEGER NOT NULL,
              symbol TEXT NOT NULL, interval TEXT NOT NULL, time TEXT NOT NULL,
              open REAL NOT NULL, high REAL NOT NULL, low REAL NOT NULL,
              close REAL NOT NULL, volume REAL, currency TEXT, unit TEXT,
              data_kind TEXT NOT NULL DEFAULT 'ohlcv', quality_json TEXT NOT NULL DEFAULT '{}',
              received_at TEXT NOT NULL, UNIQUE(source_id,symbol,interval,time)
            );
            CREATE INDEX IF NOT EXISTS ix_candles_symbol_time ON candles(symbol,interval,time);
            CREATE TABLE IF NOT EXISTS valuation_observations (
              id INTEGER PRIMARY KEY AUTOINCREMENT, source_id INTEGER NOT NULL,
              symbol TEXT NOT NULL, observed_at TEXT NOT NULL, price REAL NOT NULL,
              currency TEXT, unit TEXT, attributes_json TEXT NOT NULL DEFAULT '{}',
              received_at TEXT NOT NULL, UNIQUE(source_id,symbol,observed_at,attributes_json)
            );
            CREATE INDEX IF NOT EXISTS ix_valuations_symbol_time ON valuation_observations(symbol,observed_at);
            """)
        else:
            with c.cursor() as cur:
                cur.execute("""
                CREATE TABLE IF NOT EXISTS sources (
                  id BIGSERIAL PRIMARY KEY, name TEXT NOT NULL, kind TEXT NOT NULL,
                  config_json TEXT NOT NULL, enabled BOOLEAN NOT NULL DEFAULT TRUE,
                  created_at TEXT NOT NULL
                );
                CREATE TABLE IF NOT EXISTS candles (
                  id BIGSERIAL PRIMARY KEY, source_id BIGINT NOT NULL, symbol TEXT NOT NULL,
                  interval TEXT NOT NULL, time TEXT NOT NULL, open DOUBLE PRECISION NOT NULL,
                  high DOUBLE PRECISION NOT NULL, low DOUBLE PRECISION NOT NULL,
                  close DOUBLE PRECISION NOT NULL, volume DOUBLE PRECISION, currency TEXT,
                  unit TEXT, data_kind TEXT NOT NULL DEFAULT 'ohlcv', quality_json TEXT NOT NULL DEFAULT '{}',
                  received_at TEXT NOT NULL, UNIQUE(source_id,symbol,interval,time)
                );
                CREATE INDEX IF NOT EXISTS ix_candles_symbol_time ON candles(symbol,interval,time);
                CREATE TABLE IF NOT EXISTS valuation_observations (
                  id BIGSERIAL PRIMARY KEY, source_id BIGINT NOT NULL, symbol TEXT NOT NULL,
                  observed_at TEXT NOT NULL, price DOUBLE PRECISION NOT NULL, currency TEXT,
                  unit TEXT, attributes_json TEXT NOT NULL DEFAULT '{}', received_at TEXT NOT NULL,
                  UNIQUE(source_id,symbol,observed_at,attributes_json)
                );
                CREATE INDEX IF NOT EXISTS ix_valuations_symbol_time ON valuation_observations(symbol,observed_at);
                """)

def _execute(c, sql, params=()):
    if DATABASE_URL.startswith("sqlite:///"):
        return c.execute(sql, params)
    with c.cursor() as cur:
        cur.execute(sql.replace("?", "%s"), params)
        return cur

def rows(sql, params=()):
    with connection() as c:
        cur = _execute(c, sql, params)
        return cur.fetchall()

def execute(sql, params=()):
    with connection() as c:
        cur = _execute(c, sql, params)
        return getattr(cur, "lastrowid", None)

def insert_source(name, kind, config):
    now = datetime.now(timezone.utc).isoformat()
    if DATABASE_URL.startswith("sqlite:///"):
        return execute("INSERT INTO sources(name,kind,config_json,enabled,created_at) VALUES(?,?,?,1,?)",
                       (name,kind,json.dumps(config),now))
    with connection() as c:
        with c.cursor() as cur:
            cur.execute("INSERT INTO sources(name,kind,config_json,enabled,created_at) VALUES(%s,%s,%s,TRUE,%s) RETURNING id",
                        (name,kind,json.dumps(config),now))
            return cur.fetchone()["id"]

def insert_candle(source_id, x):
    now = datetime.now(timezone.utc).isoformat()
    sql = """INSERT INTO candles(source_id,symbol,interval,time,open,high,low,close,volume,currency,unit,data_kind,quality_json,received_at)
             VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)
             ON CONFLICT(source_id,symbol,interval,time) DO UPDATE SET
             open=excluded.open,high=excluded.high,low=excluded.low,close=excluded.close,
             volume=excluded.volume,currency=excluded.currency,unit=excluded.unit,
             data_kind=excluded.data_kind,quality_json=excluded.quality_json,received_at=excluded.received_at"""
    params=(source_id,x["symbol"],x["interval"],x["time"],x["open"],x["high"],x["low"],x["close"],
            x.get("volume"),x.get("currency"),x.get("unit"),x.get("data_kind","ohlcv"),
            json.dumps(x.get("quality",{})),now)
    execute(sql,params)

def insert_valuation(source_id, x):
    now = datetime.now(timezone.utc).isoformat()
    sql="""INSERT INTO valuation_observations(source_id,symbol,observed_at,price,currency,unit,attributes_json,received_at)
           VALUES(?,?,?,?,?,?,?,?) ON CONFLICT(source_id,symbol,observed_at,attributes_json) DO UPDATE SET
           price=excluded.price,currency=excluded.currency,unit=excluded.unit,received_at=excluded.received_at"""
    execute(sql,(source_id,x["symbol"],x["observed_at"],x["price"],x.get("currency"),x.get("unit"),
                 json.dumps(x.get("attributes",{}),sort_keys=True),now))
