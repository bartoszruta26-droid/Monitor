> **Status wdrożenia (audyt 2026-11-12):** ten dokument opisuje funkcjonalność historycznego prototypu Python w katalogu `backend/`, a nie aktywne adaptery C++20. Endpointy `/providers/*` są trasami FastAPI i nie są budowane przez `native/CMakeLists.txt` ani uruchamiane przez workflow GitHub Actions. Magazyn prototypu używa SQLite domyślnie i opcjonalnie PostgreSQL; **nie jest to docelowy magazyn JSON ani MariaDB**. Importy i tabele opisane poniżej należy traktować jako prototyp referencyjny, nie jako ukończony etap aktywnej aplikacji. Nową implementację należy przenieść do C++20 po zatwierdzeniu wspólnego modelu danych.

# Integracja źródeł NBP, Frankfurter i World Bank Pink Sheet (0.1.10)

## NBP
Adapter `nbp_history` importuje tabele A/B (oficjalne kursy średnie `mid`) oraz C (bid/ask i jawnie wyliczony midpoint, nieoficjalny kurs średni). Zakres jest automatycznie dzielony na okna maksymalnie 93-dniowe. Ceny złota NBP trafiają do oddzielnej tabeli `metal_prices` jako PLN/gram, próba 1000, dzienny kurs referencyjny.

## Frankfurter v2
Adapter pobiera szeregi `/v2/rates` z `base`, `quotes`, `from`, `to` oraz opcjonalnym `providers`. `rate` jest przechowywany jako `mid` z `rate_kind=frankfurter_mid_market`. Domyślnie API zwraca zestawienie/blend źródeł; można wskazać konkretnego dostawcę, np. `ecb`, aby utrzymać spójny szereg. API publikuje dzienne kursy referencyjne, nie intraday.

## World Bank Pink Sheet
Adapter pobiera bieżący plik CMO Historical Data Monthly XLSX ze strony Commodity Markets, wykrywa kolumny Gold i Silver w arkuszu `Monthly Prices` i zapisuje osobne szeregi XAU/XAG w USD/uncję trojańską, częstotliwość 1mo. Adres pliku jest odnajdywany na stronie World Bank, ponieważ identyfikator wydania w ścieżce pliku rotuje. Zmienna `WORLD_BANK_MONTHLY_XLSX_URL` może nadpisać URL w kontrolowanym wdrożeniu.

## Magazyn
Migracja dodaje `fx_rates` i `metal_prices`; nie usuwa ani nie zmienia tabel `sources`, `candles`, `valuation_observations`. Import jest idempotentny na kluczach provider/date/pair lub provider/symbol/date/kind. Kursy walut i ceny metali są rozdzielone od świec OHLCV i wycen kamieni/bursztynu.

## Endpointy
- `POST /providers/nbp/import` — `{ "start":"2020-01-01", "end":"2020-12-31", "tables":["A","B","C"], "include_gold":true }`
- `POST /providers/frankfurter/import` — `{ "start":"2020-01-01", "end":"2020-12-31", "base":"EUR", "quotes":["USD","PLN","GBP","CHF"], "provider":"ecb" }`
- `POST /providers/world-bank/import` — `{ "start":"1960-01-01", "end":"2026-12-31" }`; pominięcie dat importuje cały dostępny zakres.
- `GET /providers/fx-rates?base=EUR&quote=PLN`
- `GET /providers/metal-prices?symbol=XAU` lub `XAG`

Importy POST wymagają nagłówka `X-Admin-Token`. Sekrety nie są potrzebne dla tych trzech publicznych źródeł.

## Testy
Testy obejmują okna 93-dniowe NBP, parsowanie miesięcy Pink Sheet i integrację zapisu/odczytu do oddzielnych tabel FX i metali. Testy nie wywołują produkcyjnych API; testy sieciowe wymagają osobnego środowiska integracyjnego i powinny być uruchamiane jawnie.
