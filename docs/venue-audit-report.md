# Raport audytowy kalendarzy venue

## API

Publiczna funkcja:

`build_venue_audit_report_json(const MarketDataSnapshot& snapshot)`

zwraca UTF-8 JSON w wersji `monitor.venue_audit.v1`. Funkcja jest tylko do odczytu: nie modyfikuje migawki, repozytorium ani archiwów. Dla niezmienionej migawki wynik jest deterministyczny.

## Zawartość

- `repository_schema_version`, `repository_revision`, `generated_at`: identyfikacja migawki wejściowej.
- `source_objects`: uporządkowany manifest niezmiennych obiektów, ich identyfikatory SHA-256, względne ścieżki, długości i MIME.
- `calendars`: najnowsza rewizja każdego kalendarza; tożsamość venue, zakres, strefa, wersja źródła, URL, czas pobrania, atrybucja, status licencji, SHA/identyfikator obiektu i wersja parsera.
- `effective_days`: wszystkie dni z najnowszej rewizji po zastosowaniu aktywnych komunikatów; status, sesje produktowe, zdarzenia fazowe, referencje zastosowanych komunikatów i flaga nierozstrzygniętego oficjalnego ogłoszenia.
- `unresolved_official_notice_dates`: daty wymagające oficjalnego komunikatu, których aktywne nadpisanie jeszcze nie rozstrzyga.
- `notices`: pełna historia rewizji komunikatów, w tym anulowania, źródła, parser i licencja.
- `orphaned_source_object_ids`: obiekty bez referencji z kalendarza lub komunikatu. To raport diagnostyczny, nie polecenie usunięcia.

## Zasady interpretacji

1. Raport nie pobiera danych z sieci i nie potwierdza aktualności źródeł.
2. Godziny oraz statusy pochodzą z zapisanych rewizji i komunikatów. Brak komunikatu pozostaje jawnie nierozstrzygnięty.
3. Obiekt źródłowy wskazany przez komunikat jest traktowany jako używany, nawet jeśli nie wskazuje go bazowa rewizja kalendarza.
4. Obiekty osierocone nie są usuwane. Retencja wymaga osobnej, zatwierdzonej procedury.
5. Raport obejmuje wyłącznie obiekty znane migawce JSON; nie skanuje katalogu archiwum w poszukiwaniu niezaindeksowanych plików.

## Testy

Testy repozytorium sprawdzają parsowalność JSON, obecność manifestu i historii komunikatów, pełny zakres efektywnych dni, zastosowanie faz sesji, raport dat nierozstrzygniętych, rozróżnienie osieroconych obiektów oraz identyczność dwóch eksportów tej samej migawki.
