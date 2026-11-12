# Rejestr etapów prac — Monitor

**Repozytorium:** `bartoszruta26-droid/Monitor`  
**Gałąź:** `main`  
**Ostatnia weryfikacja:** 2026-11-12  
**Zasada prowadzenia:** status etapu zmienia się na „zakończony” dopiero po sprawdzeniu kodu, testów i CI dla konkretnego SHA. Ten plik jest bieżącym rejestrem wykonania; szczegóły techniczne pozostają w dokumentach etapów.

## Stan repozytorium przy audycie

- HEAD `main`: `95c53dc2a6a8519119480836d45eab189e69b801` (pierwsza implementacja reguł NBP; poprawka testu oczekuje na CI).
- Ostatni workflow: [CI 43302839736](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43302839736), kompilacja nieudana w pliku testowym z powodu braku `#include <algorithm>`; moduł kalendarza skompilował się poprawnie, CTest pominięty.
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
| 9 | Trwały magazyn i wersjonowanie kalendarzy sesyjnych | Zakończony | Commit `df867c6efaa7f8cebad74db3291a3c0dde12f67e`; CI [43264555372](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43264555372) sukces, kompilacja i CTest 7/7. Pochodzenie regresji zawiera `calendar_id` i `calendar_version`. |
| 10 | Generator harmonogramów z IANA TZ/DST i reguł sesji | Zakończony | Commit `da61302dc4de43308228e6f60e82e187bef566df`; CI [43301335250](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43301335250) sukces: konfiguracja, kompilacja i CTest 8/8. Testy obejmują DST wiosenny i jesienny, polityki czasu, weekendy, wyłączenia, błędne strefy i zakres dat. |
| 11 | Kalendarze źródłowe i instrumentowe (NBP, metale, giełdy) | W toku — generator okien publikacji NBP | Generator okien A/B/C przeszedł CI 43303169211 (8/8). Dodano teraz funkcję obliczania polskich świąt ustawowych wraz z ruchomymi datami; oczekuje na CI. |
| 12 | Wspólne fixture i porównanie C++/Pine | Do wykonania | Pine Editor niezweryfikowany; brak potwierdzonej kompilacji TradingView |
| 13 | Serwer API, autoryzacja, REST/JSON i WebSocket | Do wykonania | Brak aktywnego modułu serwera |
| 14 | Klienci Linux GUI, WebUI, TUI i Android | Do wykonania | Brak aktywnych modułów klientów |
| 15 | Opcjonalny adapter MariaDB | Do wykonania | Brak implementacji |

## Ostatnie potwierdzone commity

- `d2bc1364986bee6f4474bfce9ad2549e60c90f97` — jawna kontrola kompletności HTF względem harmonogramu sesji.
- `c561ac86cf7d1231aed6ef40d47e8ff2f36de763` — walidacja pochodzenia wykresów i silnik regresji kroczącej.
- `8b7264e08fe946824304bea190dd76322166c8be` — wcześniejszy etap regresji HTF.

## Ostatnia aktualizacja rejestru — commit `72d7ce5a13e082b21752821d653550ad7a1f7299`

- Rejestr oraz audyt zostały zapisane na `main`.
- CI dla tego commitu: [workflow 43261502952](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43261502952) — sukces.
- Kroki: konfiguracja, kompilacja i testy zakończone powodzeniem.
- CTest: 7/7; 100% testów zaliczonych; czas testów 0,09 s.
- Commit zawiera dokumentację i rejestr, nie zmienia logiki aplikacji.

## Weryfikacja etapu 10 — historia

- Commit `cedee367f5528abef5d956b9f560167a5b86aaf0`, CI 43265633802: konfiguracja i kompilacja sukces; CTest 7/8. Test generatora nie przeszedł przez błędnie ustawiony koniec zakresu półotwartego (fixture wykluczał 3 kwietnia). Poprawka w kolejnym commicie.

## Weryfikacja etapu 9 — końcowa

- Commit `df867c6efaa7f8cebad74db3291a3c0dde12f67e` — CI 43264555372 zakończony sukcesem; konfiguracja, kompilacja i testy zakończone powodzeniem.
- Ochrona niezmienności obejmuje zapis partii i `replace(snapshot)`; testy potwierdzają odrzucenie edycji i usunięcia wersji.
- Starszy dokument schema v1 bez kolekcji kalendarzy jest odczytywany jako pusta lista.

## Dziennik weryfikacji etapu 9

- Commit `68377875` — CI nieudany na etapie kompilacji: zduplikowana definicja `MarketSessionCalendar` w nagłówku regresji. Błąd został zidentyfikowany w logach GitHub Actions.
- Commit `201ede21` — poprawiono definicję i dodano pochodzenie regresji, ale CI nadal wykrył tę samą definicję pozostawioną w nagłówku.
- Commit `9b122aec3a26eff0bc7b516118cfb722770ae723` — usunięto duplikat; CI dla tego SHA oczekuje na wynik.

## Ostatnia aktualizacja — etap 9

- Commit: `68377875d9d8a60b4bfbbf8cabfe08b989af541d`.
- Zmieniono model domenowy, repozytorium JSON i testy magazynu.
- CI dla tego commitu oczekuje na uruchomienie/wynik; etap nie jest jeszcze oznaczony jako zakończony.
- Starszy dokument schema v1 bez `session_calendars` pozostaje obsługiwany.

## Bieżący etap roboczy

### Etap 10 — generator harmonogramów IANA TZ/DST

Zakres:
1. Generować sloty lokalne według jawnych dni tygodnia i godzin w przedziale dat.
2. Rozpoznawać wyłącznie strefy IANA z bazy ICU.
3. Wymagać jawnej polityki dla czasu powtórzonego jesienią; domyślnie odrzucać niejednoznaczność.
4. Wymagać jawnej polityki dla czasu nieistniejącego wiosną; domyślnie odrzucać, opcjonalnie przesuwać do pierwszego poprawnego czasu.
5. Obsługiwać daty wyłączone, walidację i deterministyczne sortowanie UTC.

Kryteria zakończenia: testy DST, weekendów, dat wyłączonych, błędnych stref, kolizji i zakresów; pełny CI dla konkretnego SHA.

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


## 22. Etap 10 — druga diagnoza CI i poprawka ICU

- Commit `6845c071890a762fb7e9fe00fef220a31134af7e`, CI [43265958349](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43265958349): konfiguracja i kompilacja zakończone sukcesem; CTest 7/8.
- Test `monitor-market-data-calendar-tests` przerwał proces wyjątkiem ICU `U_ILLEGAL_ARGUMENT_ERROR` przy konwersji lokalnego czasu na UTC. Nie był to błąd kompilacji ani zakresu dat.
- Przyczyna: kalendarz ICU był ustawiany jako nielenientny, mimo że generator korzysta z jawnych opcji `UCAL_WALLTIME_NEXT_VALID` do rozstrzygania nieistniejących czasów. Tryb lenient jest wymagany do zastosowania polityki ICU; rygorystyczne odrzucanie pozostaje realizowane przez odczyt pól po konwersji i porównanie z żądanym czasem lokalnym.
- Poprawka: w `market_data_calendar.cpp` włączono tryb lenient z komentarzem wyjaśniającym. Dla `NonexistentLocalTimePolicy::Reject` nadal porównywane są rok, miesiąc, dzień, godzina, minuta i sekunda; rozbieżność powoduje `invalid_argument`. Polityka `NextValidTime` akceptuje pierwszą poprawną chwilę po luce DST.
- Status etapu 10 pozostaje „w toku” do czasu zielonego CI dla nowego SHA.


## 23. Etap 10 — zamknięcie po poprawce ICU

- Commit: `da61302dc4de43308228e6f60e82e187bef566df`.
- GitHub Actions: [CI 43301335250](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43301335250) — sukces.
- Konfiguracja CMake: sukces; kompilacja C++20/ICU: sukces; CTest: **8/8, 100%** (0,59 s).
- `monitor-market-data-calendar-tests` przeszedł po zmianie trybu ICU na lenient z zachowaniem jawnej walidacji odrzucania nieistniejącego czasu.
- Etap 10 zamknięty. Kolejny jest Etap 11 — kalendarze źródłowe i instrumentowe, z oddzieleniem terminów publikacji od sesji rynkowych.

## 24. Etap 11 — rozpoznanie źródeł kalendarzy

- NBP publikuje osobne reguły terminów dla tabel A/B/C; oficjalny harmonogram podaje okna publikacji, a nie precyzyjny czas obserwacji każdego rekordu. API udostępnia daty publikacji i daty notowania dla tabeli C oraz zwraca 404 przy braku danych.
- NBP gold API dostarcza datę publikacji, ale dokumentacja API nie deklaruje godziny publikacji. Nie należy dopisywać fikcyjnej godziny do obserwacji.
- World Bank Pink Sheet jest miesięcznym źródłem danych; nie wolno przypisywać mu dziennego kalendarza sesji.
- XAUS udostępnia orientacyjny spot z cache 60 s i serię intraday co 5 minut z ograniczoną retencją; nie jest to kalendarz giełdowy ani kwotowanie wykonywalne.
- Dla giełd należy importować oficjalny kalendarz konkretnego operatora i rynku (np. Xetra), uwzględniając dni handlu w święta, skrócone sesje i wersje kalendarza.
- Szczegóły, źródła i kryteria implementacji: `docs/market-calendar-sources.md`.


## 25. Etap 11 — implementacja okien publikacji NBP

- Dodano `NbpPublicationWindow` i `NbpPublicationTable`; okno przechowuje lokalną datę, strefę `Europe/Warsaw` i przedział godzin, nie timestamp obserwacji.
- `generate_nbp_publication_windows()` tworzy A/C w dni robocze oraz B w środy. Gdy środa jest na jawnej liście dni wolnych, B jest przenoszona na poprzedni dzień roboczy.
- Oficjalne przedziały: A/B 11:45–12:15, C 07:45–08:15. Źródło: https://nbp.pl/statystyka-i-sprawozdawczosc/kursy/informacja-o-terminach-publikacji-kursow-walut/.
- Lista świąt jest parametrem wejściowym; nie zaszyto ani nie zgadnięto polskiego kalendarza świąt.
- Fixture: 11 listopada 2026 r. jako środa wolna; B przesunięta na 10 listopada, A/C pomijają święto. Testy obejmują zwykłą środę, błędny zakres i duplikaty świąt.
- Status pozostaje w toku do pełnego CI.


## 26. Etap 11 — korekta po pierwszym CI NBP

- Commit `95c53dc2a6a8519119480836d45eab189e69b801`, CI [43302839736](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43302839736): konfiguracja zakończona sukcesem; moduł `market_data_calendar.cpp` skompilował się; kompilacja testu `test_market_data_calendar.cpp` nie powiodła się, ponieważ test używa `std::any_of` bez `#include <algorithm>`. CTest nie został uruchomiony.
- Poprawka: jawnie dołączono `<algorithm>` w teście. Logika produkcyjna generatora nie wymagała zmiany.
- Etap 11 pozostaje w toku do pełnej kompilacji i zielonego CTest.


## 27. Etap 11 — generator świąt ustawowych RP

- CI [43303169211](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43303169211) dla commitu `4f27d8e8550f4eafab70e3ed34c054e098053d8b`: konfiguracja, kompilacja i CTest 8/8 zakończone sukcesem. Pierwsza próba `43302839736` nie przeszła kompilacji testu z powodu brakującego include; poprawiono to w następnym commicie.
- Dodano `polish_statutory_holidays(year)`: święta stałe oraz Wielkanoc, Poniedziałek Wielkanocny, Zielone Świątki i Boże Ciało. Daty ruchome wyliczane są algorytmem gregoriańskim Meeusa/Jonesa/Butchera.
- Podstawa prawna: ustawa o dniach wolnych od pracy, tekst jednolity Dz.U. 2025 poz. 296; Wigilia 24 grudnia obowiązuje od 1 lutego 2025 r. Źródło: https://api.sejm.gov.pl/eli/acts/DU/2025/296/text.pdf.
- Testy dla 2025/2026 weryfikują daty ruchome, 24 grudnia i odrzucenie roku 0. Nowa funkcja oczekuje na CI.
