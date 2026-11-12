# Magazyn JSON C++20 — Etap 3

## Zakres
JsonMarketDataRepository jest pierwszym magazynem trwałym dla wspólnego modelu danych rynkowych. Jest odrębną warstwą infrastruktury: adaptery dostawców nie zapisują bezpośrednio plików, a interfejsy użytkownika nie znają formatu JSON.

## Format dokumentu
Jeden dokument monitor.market_data przechowuje:
- schema_version — wersję schematu całego dokumentu (obecnie 1);
- revision — monotoniczny numer zapisu;
- updated_at — czas ostatniego zatwierdzonego zapisu UTC;
- instrumenty, dostawców, bieżące notowania i metadane szeregów;
- observation_history — poprzednie wersje notowań zastąpione korektą.

Daty są zapisane jako ISO-8601 (YYYY-MM-DD), znaczniki czasu jako UTC ISO-8601, a enumy jako stabilne nazwy tekstowe. Identyfikatory, pochodzenie, jednostki, rodzaj ceny, częstotliwość, status jakości i identyfikatory rekordów dostawcy są zachowane. Wersja schematu dokumentu jest niezależna od wersji pojedynczego MarketObservation.

## Operacje
- load() — odczyt i pełna walidacja; brak pliku oznacza pustą migawkę, pusty/uszkodzony plik jest błędem.
- replace(snapshot) — atomowa zamiana całej migawki.
- upsert_batch() — atomowe przyjęcie całego wyniku adaptera w jednej rewizji;
- upsert_instrument/provider/observation/series_metadata() — operacje idempotentne z kontrolą powiązań.
- restore_backup() — jawne odtworzenie ostatniej kopii; magazyn nie maskuje samoczynnie uszkodzeń.

## Integralność i współbieżność
- zapis do pliku tymczasowego w tym samym katalogu, fsync pliku, rename atomowy i fsync katalogu;
- blokada międzyprocesowa flock na pliku .lock, obejmująca odczyt-modyfikację-zapis;
- pliki tworzone z uprawnieniami 0600, odczyt bez podążania za dowiązaniami symbolicznymi;
- kopia .bak poprzedniego poprawnego dokumentu przed zatwierdzeniem nowego;
- limit rozmiaru dokumentu i walidacja limitu po serializacji;
- nieznane wersje schematu są odrzucane, bez cichego obniżania lub zgadywania migracji.

## Deduplikacja i korekty
Klucz notowania to (instrument_id, provider_id, observed_at, price_kind, frequency). Ponowne pobranie tego samego payloadu aktualizuje received_at, nie tworząc korekty. Zmieniona wartość lub inne pole semantyczne przenosi poprzednią wersję do observation_history, a nowa staje się bieżąca. Starszy odczyt nie nadpisuje nowszego.

## Zależności i ograniczenia
- C++20, nlohmann-json, POSIX flock/fsync/rename (pierwszy backend plikowy jest przeznaczony dla Linux/POSIX).
- Kontrakt MarketDataRepository jest niezależny od magazynu; adapter MariaDB i serwer wymiany danych nie są jeszcze zaimplementowane.
- Migracja dokumentu do przyszłej wersji wymaga jawnej funkcji migracyjnej i testów fixture; wersje nieznane są bezpiecznie odrzucane.
- Jedna migawka JSON upraszcza atomowość i spójność relacji, ale może stać się duża. Paginacja/partycjonowanie należy rozważyć dopiero po pomiarach, zachowując kontrakt repozytorium.
- Historyczny backend/market_data/storage.py nie jest częścią aktywnego C++ build/test path; korzysta z SQLite/PostgreSQL i nie zastępuje tego magazynu JSON.

## Integracja z adapterami
MarketDataBatch odwzorowuje kolekcje AdapterResult: instrumenty, dostawców, notowania i metadane. Warstwa wywołująca przekazuje wynik adaptera do upsert_batch(), co zatwierdza cały import jednym zapisem i jedną rewizją. Adaptery nie otrzymują zależności od plikowego magazynu.

## Testy
native/tests/test_market_data_repository.cpp sprawdza round-trip, walidację, rewizje, deduplikację, historię korekt, kopie zapasowe, jawne odtwarzanie, odrzucanie nieznanej wersji schematu oraz współbieżne zapisy procesów. Testy nie korzystają z sieci. Partia adaptera jest przyjmowana jako jedna transakcja plikowa, więc błąd walidacji nie pozostawia częściowo zapisanych instrumentów lub notowań.


## Kalendarze sesyjne venue — rozszerzenie zgodne z v1

Dokument JSON może opcjonalnie zawierać tablicę `venue_calendars`. Pole jest rozszerzeniem kompatybilnym wstecz: istniejące dokumenty schema v1 bez tej tablicy są odczytywane jako pusta kolekcja. Nie zmieniono numeru schematu głównego, a dotychczasowe `session_calendars` zachowują dotychczasową semantykę.

Każdy element `venue_calendars` przechowuje:
- `source`: identyfikator dokumentu źródłowego, venue, wersję, zakres półotwarty, strefę IANA, URL, opcjonalny czas pobrania UTC, opcjonalny SHA-256 dokładnych bajtów, atrybucję i status licencji;
- `days`: pełną, uporządkowaną listę dat z osobnym statusem `closed`, `trading` lub `settlement_only`;
- `product_sessions`: grupę produktu i lokalne godziny otwarcia/zamknięcia;
- `requires_official_notice`: sygnał, że przed użyciem daty należy sprawdzić komunikat operatora.

Walidacja wymaga pełnego pokrycia zakresu źródła, kolejnych dat bez luk i duplikatów, poprawnych godzin oraz co najmniej jednej sesji dla dnia handlowego. Kalendarz nie może mieć sesji w dniu zamknięcia ani rozliczeniowym.

Wersje są niezmienne. Powtórne zapisanie identycznego `document_id + version` jest idempotentne; zmiana zawartości pod tym samym kluczem, usunięcie wersji przez `replace` lub zapis numeru nie większego od najnowszego jest odrzucany. Korekta źródła wymaga nowej wersji, a poprzednia pozostaje w migawce.

Testy obejmują round-trip 365-dniowego kalendarza Xetra, metadane źródła, ponowne zapisanie, konflikt edycji, dopisanie wersji 2, próbę usunięcia/zmiany przez `replace` oraz odczyt starszego dokumentu schema v1 bez pola `venue_calendars`.
