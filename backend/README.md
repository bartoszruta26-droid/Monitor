# Historyczny prototyp backendu — zamrożony

Katalog zawiera wcześniejszy prototyp warstwy danych. Nie jest on częścią aktywnej architektury ani procesu budowania i testowania. Nowy kod należy rozwijać w C++20.

## Docelowa architektura

Monitor ma posiadać odrębny serwer wymiany danych, który komunikuje się z wieloma backendami/adaptorami oraz czterema klientami: Linux GUI, WebUI, TUI i Android App.

Serwer udostępnia wersjonowane API HTTP/REST z JSON oraz kanał WebSocket dla zdarzeń i aktualizacji. Backend dostawcy nie zapisuje bezpośrednio do magazynu ani do bazy innego modułu — przekazuje znormalizowane dane do usług serwera.

## Trwały magazyn danych

- Domyślnie: pliki JSON w katalogu danych skonfigurowanym przez użytkownika.
- Opcjonalnie: MariaDB, wybierana przez konfigurację.
- Oba warianty mają implementować ten sam kontrakt repozytorium.
- Klienci nie łączą się bezpośrednio z MariaDB.
- Hasła i tokeny są dostarczane przez bezpieczne zmienne środowiskowe lub magazyn sekretów.

SQLite i PostgreSQL nie są obecnie założonymi magazynami docelowymi.

## Status migracji

- Aktywny rdzeń: `../native`.
- Budowanie i testy: `../scripts/build-native.sh`, `../scripts/test-native.sh`.
- Serwer wymiany danych: planowany, jeszcze niezaimplementowany.
- Adaptery NBP, Frankfurter i World Bank w C++: planowane.
- Magazyn JSON: planowany.
- Adapter MariaDB: planowany.
- Linux GUI, WebUI, TUI i Android App: planowane.

Nie rozwijaj nowych funkcji w tym historycznym katalogu. Pliki prototypu można usunąć dopiero po przeniesieniu i zweryfikowaniu wszystkich potrzebnych funkcji w implementacji C++.


## Potwierdzony status po audycie 2026-11-12

- Kod `backend/market_data/` zawiera historyczny prototyp Python: adaptery REST/ NB P, import NBP/Frankfurter/World Bank, API FastAPI i testy pytest.
- Prototyp można uruchomić niezależnie, ale nie jest częścią aktywnej aplikacji C++20 ani bieżącego workflow CI.
- Prototyp używa SQLite jako domyślnego magazynu i ma opcjonalną obsługę PostgreSQL. Nie implementuje docelowego magazynu JSON ani MariaDB.
- Testy Python pozostają w repozytorium jako materiał historyczny i nie są uruchamiane w aktywnym CI. Nie należy ich traktować jako kryterium ukończenia aktywnych modułów.
- Opis adapterów w `docs/open-data-providers.md` dotyczy wyłącznie tego prototypu.
