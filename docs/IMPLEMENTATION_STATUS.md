# Rejestr etapów prac — Monitor

**Repozytorium:** `bartoszruta26-droid/Monitor`  
**Gałąź:** `main`  
**Ostatnia weryfikacja:** 2026-11-12  
**Zasada prowadzenia:** status etapu zmienia się na „zakończony” dopiero po sprawdzeniu kodu, testów i CI dla konkretnego SHA. Ten plik jest bieżącym rejestrem wykonania; szczegóły techniczne pozostają w dokumentach etapów.

## Stan repozytorium przy audycie

- HEAD `main`: `d2bc1364986bee6f4474bfce9ad2549e60c90f97`.
- Ostatni workflow: [CI 43260529082](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43260529082), SHA zgodny z HEAD, zakończony sukcesem.
- Poprzednie przebiegi obejmują jeden nieudany CI dla wcześniejszego SHA `aa3b7eb`, po którym późniejszy commit przeszedł poprawnie. Nie traktować błędu jako aktualnego, ale zachować go w historii.
- Gałęzie: wyłącznie `main`.
- Otwarte pull requesty: 0.
- Otwarte Issues: 0.
- Aktywny build: C++20, CMake, Bash i CTest. Python nie jest częścią aktywnego procesu budowania/testowania.

## Rejestr etapów

| Etap | Zakres | Status | Dowód / uwagi |
|---|---|---|---|
| 1 | Wspólny model danych rynkowych | Zakończony | `market_data.hpp/.cpp`, testy modelu |
| 2 | Adaptery dostawców NBP, Frankfurter, World Bank, XAUS | Zakończony w zakresie obecnych adapterów | C++ adaptery i testy offline; źródła i licencje wymagają okresowej kontroli |
| 3 | Repozytorium JSON, atomowość, kopie, historia korekt | Zakończony w zakresie podstawowego modelu | `market_data_repository.*`; magazyn jest POSIX/Linux |
| 4 | Agregacja okresów i próbkowane OHLC | Zakończony | `market_data_aggregation.*`; brak upsamplingu |
| 5 | Wspólny model danych wykresów | Zakończony jako model danych | `market_data_chart.*`; renderer i GUI nie istnieją |
| 6 | Regresja krocząca DGT/LTB/n-2 | Zakończony jako silnik liczbowy | `regression.*`; zachowano różne mianowniki metod |
| 7 | Regresja HTF z segmentacją po brakujących okresach | Zakończony v1 | `market_data_regression.*`; okresy kalendarzowe, bez reguł sesji |
| 8 | Jawny kalendarz sesyjny dla kompletności HTF | Zakończony v1 jako funkcja obliczeniowa | `MarketSessionCalendar` i `session_aware_rolling_regression()`; terminy przekazuje wywołujący |
| 9 | Trwały magazyn i wersjonowanie kalendarzy sesyjnych | Do wykonania | **Nie jest jeszcze zaimplementowany w JSON repository.** Poprzednia odpowiedź sugerująca jego wdrożenie była przedwczesna; kod repozytorium nie zawiera jeszcze pola kalendarzy, serializacji ani testów wersji. |
| 10 | Generator harmonogramów z IANA TZ/DST i reguł sesji | Do wykonania | Brak generatora i zależności/strefowych testów |
| 11 | Oficjalne kalendarze instrumentów (NBP, metale, giełdy) | Do wykonania | Brak zweryfikowanych kalendarzy dostawców |
| 12 | Wspólne fixture i porównanie C++/Pine | Do wykonania | Pine Editor niezweryfikowany; brak potwierdzonej kompilacji TradingView |
| 13 | Serwer API, autoryzacja, REST/JSON i WebSocket | Do wykonania | Brak aktywnego modułu serwera |
| 14 | Klienci Linux GUI, WebUI, TUI i Android | Do wykonania | Brak aktywnych modułów klientów |
| 15 | Opcjonalny adapter MariaDB | Do wykonania | Brak implementacji |

## Ostatnie potwierdzone commity

- `d2bc1364986bee6f4474bfce9ad2549e60c90f97` — jawna kontrola kompletności HTF względem harmonogramu sesji.
- `c561ac86cf7d1231aed6ef40d47e8ff2f36de763` — walidacja pochodzenia wykresów i silnik regresji kroczącej.
- `8b7264e08fe946824304bea190dd76322166c8be` — wcześniejszy etap regresji HTF.

## Bieżący etap roboczy

### Etap 9 — trwały, niezmienny magazyn kalendarzy

Zakres:
1. Dodać `MarketSessionCalendar` do wspólnego modelu domenowego.
2. Dodać kolekcję kalendarzy do `MarketDataSnapshot` i `MarketDataBatch`.
3. Dodać serializację/deserializację JSON, walidację referencji do instrumentu i dostawcy, unikalność `calendar_id + version`.
4. Opublikowana wersja kalendarza jest niezmienna; zmiana harmonogramu wymaga nowej, rosnącej wersji.
5. Zachować odczyt istniejących dokumentów schema v1 bez kolekcji kalendarzy. Nie zmieniać schematu dokumentu bez jawnej migracji i fixture.
6. Dodać testy round-trip, wersjonowania, konfliktu tej samej wersji, błędnych referencji, odrzucenia nieregularnej częstotliwości i odczytu starego dokumentu.
7. Podłączyć regresję sesyjną do wersjonowanego kalendarza bez kopiowania ani zgadywania harmonogramu.

Kryteria zakończenia:
- C++20 build bez ostrzeżeń krytycznych;
- wszystkie testy CTest zielone;
- CI zakończony sukcesem dla dokładnego SHA;
- test potwierdza, że nie można nadpisać opublikowanej wersji;
- dokumentacja i ten rejestr wskazują commit i wynik CI.

## Zasady aktualizacji rejestru

Po każdym etapie:
- wpisać status (planowany / w toku / zablokowany / zakończony);
- zapisać commit SHA i link do CI;
- podać liczbę testów i ewentualne niepowodzenia;
- odnotować ograniczenia i następny etap;
- nie oznaczać prac jako ukończonych na podstawie samego commita bez zielonego CI.

Przy każdym kolejnym poleceniu kontynuacji należy najpierw odczytać ten plik i zweryfikować HEAD, gałęzie, PR, Issues oraz ostatni CI.
