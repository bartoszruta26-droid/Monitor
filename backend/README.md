# Market Data API

## Wymagania
Python 3.11+. Domyślnie SQLite; PostgreSQL przez `DATABASE_URL`.

## Instalacja i uruchomienie

```bash
cd backend
python -m venv .venv
. .venv/bin/activate
pip install -r requirements.txt
export ADMIN_API_TOKEN='wygeneruj-dlugi-losowy-sekret'
export DATABASE_URL='sqlite:///./market_data.db'
uvicorn market_data.main:app --host 0.0.0.0 --port 8080
```

Na produkcji użyj PostgreSQL, HTTPS reverse proxy, zapory i sekretów środowiskowych. Nie wystawiaj portu administracyjnego publicznie bez dodatkowej warstwy uwierzytelniania.

## API

- `GET /health` — stan usługi.
- `POST /sources` — dodanie źródła (wymaga `X-Admin-Token`).
- `GET /sources` — lista źródeł.
- `POST /sources/{source_id}/ingest` — ręczne uruchomienie pobrania (wymaga tokenu).
- `GET /candles?symbol=XAUUSD&interval=1d&start=...&end=...` — historia świec.
- `GET /valuations?symbol=AMBER&start=...&end=...` — historia wycen.

### Przykład: NBP

```json
{
  "name": "NBP tabela A USD",
  "kind": "nbp_fx",
  "config": {"table": "A", "code": "USD", "base_currency": "PLN"}
}
```

### Przykład: dostawca świec OHLCV

```json
{
  "name": "Mój dostawca XAUUSD",
  "kind": "rest_ohlcv",
  "config": {
    "url": "https://provider.example/api/candles?symbol=XAUUSD&interval=1h",
    "symbol": "XAUUSD",
    "interval": "1h",
    "timestamp_unit": "iso",
    "fields": {
      "items": "data",
      "time": "time", "open": "open", "high": "high",
      "low": "low", "close": "close", "volume": "volume"
    },
    "currency": "USD",
    "unit": "troy_ounce",
    "secret_env": "PROVIDER_API_KEY",
    "auth_header": "Authorization",
    "auth_prefix": "Bearer "
  }
}
```

`secret_env` wskazuje nazwę zmiennej środowiskowej, a nie sam sekret. Wartość sekretu nie jest zapisywana w bazie. Adresy muszą być HTTPS i opcjonalnie ograniczone przez `ALLOWED_SOURCE_HOSTS` (lista hostów oddzielona przecinkami).

### Przykład: wyceny bursztynu/kamieni

`rest_valuation` zapisuje obserwacje punktowe. W `config.fields` mapuj `items`, `time`, `price`, `currency`, `unit`, `grade`, `weight`, `stone_type`/`amber_type` i `certificate`. Dla porównywalności wymagaj tych samych atrybutów jakościowych.

## Ograniczenia

Adapter NBP pobiera dzienne kursy referencyjne. Nie generuje śróddziennych danych rynkowych. Adaptery REST wymagają zgodności dostawcy z mapowaniem i limitami API. Harmonogram automatyczny nie jest uruchamiany w tej minimalnej usłudze — pobieranie inicjuje endpoint ingest; harmonogram można dodać w infrastrukturze wdrożeniowej po poznaniu limitów dostawców.
