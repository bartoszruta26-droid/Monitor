import os
from datetime import date
import pytest
from market_data.open_providers import _chunks_93, _month_date, parse_world_bank_workbook, init_provider_tables, _insert_fx, _insert_metal, get_fx, get_metals
from market_data import storage

def test_nbp_date_windows_never_exceed_93_days():
    spans=list(_chunks_93(date(2024,1,1),date(2024,7,1)))
    assert all((b-a).days <= 92 for a,b in spans)
    assert spans[0][0] == date(2024,1,1)
    assert spans[-1][1] == date(2024,7,1)

def test_world_bank_period_parser():
    assert _month_date("2026M09") == "2026-09-01"
    assert _month_date("not-a-month") is None

def test_separate_fx_and_metal_history(tmp_path, monkeypatch):
    monkeypatch.setattr(storage,"DATABASE_URL",f"sqlite:///{tmp_path/'test.db'}")
    storage.init_db()
    init_provider_tables()
    source=storage.insert_source("test","test",{})
    _insert_fx(source,"frankfurter:blend",{"date":"2026-01-02","base":"EUR","quote":"PLN","mid":4.2,"rate_kind":"frankfurter_mid_market"})
    _insert_metal(source,"world_bank_pink_sheet",{"symbol":"XAG","date":"2026-01-01","price":31.5,"currency":"USD","unit":"troy_ounce","frequency":"1mo","price_kind":"world_bank_monthly_average"})
    fx=get_fx("EUR","PLN")
    metals=get_metals("XAG")
    assert fx[0]["mid"] == 4.2
    assert fx[0]["rate_kind"] == "frankfurter_mid_market"
    assert metals[0]["price"] == 31.5
    assert metals[0]["unit"] == "troy_ounce"
    assert metals[0]["frequency"] == "1mo"


def test_world_bank_workbook_parser_reads_gold_and_silver():
    from io import BytesIO
    from openpyxl import Workbook
    wb=Workbook()
    ws=wb.active
    ws.title="Monthly Prices"
    ws.append(["World Bank Pink Sheet"])
    ws.append([""])
    ws.append([""])
    ws.append([""])
    ws.append(["Period","Gold","Silver"])
    ws.append(["Unit","USD/troy oz","USD/troy oz"])
    ws.append(["2026M08",3400.5,39.25])
    payload=BytesIO()
    wb.save(payload)
    parsed=parse_world_bank_workbook(payload.getvalue())
    assert {x["symbol"] for x in parsed} == {"XAU","XAG"}
    assert next(x for x in parsed if x["symbol"]=="XAU")["price"] == 3400.5
    assert next(x for x in parsed if x["symbol"]=="XAG")["price"] == 39.25

def test_old_and_new_api_routes_are_registered():
    from market_data.main import app
    paths={route.path for route in app.routes}
    assert {"/sources","/candles","/valuations","/providers/nbp/import",
            "/providers/frankfurter/import","/providers/world-bank/import",
            "/providers/fx-rates","/providers/metal-prices"}.issubset(paths)
