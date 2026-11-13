# Rejestr etapów prac — Monitor

**Repozytorium:** `bartoszruta26-droid/Monitor`  
**Gałąź:** `main`  
**Ostatnia weryfikacja:** 2026-11-13 12:22 Europe/Warsaw
**Zasada prowadzenia:** status etapu zmienia się na „zakończony” dopiero po sprawdzeniu kodu, testów i CI dla konkretnego SHA. Ten plik jest bieżącym rejestrem wykonania; szczegóły techniczne pozostają w dokumentach etapów.

## Stan repozytorium przy audycie

- Aktualny `main`: `cebdaba6194d3e8cb4ebe65f6251b5fc6126ae06`.
- Kontynuacja FX/XAU: gałąź `feature/fx-xau-pricing-engine`, PR [#1](https://github.com/bartoszruta26-droid/Monitor/pull/1), HEAD `80bbb65879de8f501bff070a05e6d5f2be9b5fda` (silnik FX/XAU, testy i odpięcie modułu Xetra od aktywnego builda).
- Ostatni zielony CI dla `main`: [CI #414 / 43411904371](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43411904371), SHA `cebdaba6194d3e8cb4ebe65f6251b5fc6126ae06` — Configure/Build/Test success; CTest **9/9**.
- CI PR #1: przebieg #415 dla `be0edbbe5e12be32f5eda8a9ff30d416fc43b060` — success; kolejne przebiegi #416/#417 wykryły pozostałe odwołania testu kalendarza do parsera Xetra. Usunięto wyłącznie Xetra-only blok testowy; przebiegi #418–#420 sprawdzają poprawkę i końcowy SHA. Nie oznaczać nowego etapu jako zakończonego.
- Poprzedni przebieg dla zmiany kodu parsera Xetra 2025: [CI #382 / 43401645335](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43401645335), SHA `e055cfe5d5e61123b073c4fae4932216d0d6c249` — **success**, CTest **9/9**.
- Gałęzie: wyłącznie `main`; otwarte pull requesty: 0; otwarte Issues: 0.
- Poprzedni problem kompilacji testu kalendarza (brak `#include <algorithm>` w starszym SHA) został naprawiony; nie jest stanem bieżącym.
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
| 11 | Kalendarze publikacji i aktualizacji źródeł FX oraz złota (NBP, FX, XAU spot) | W toku — obserwowane publikacje NBP i świeżość źródeł FX/XAU | Generator okien A/B/C przeszedł CI 43303169211 (8/8). Generator okien A/B/C, świąt ustawowych i automatycznego stosowania kalendarza świąt przeszedł CI 43304025796 (8/8). Etap 11 nadal w toku: model obserwowanych publikacji NBP jest zaimplementowany i przetestowany; importer NBP ma już podział zakresu 93-dniowego i obsługę 404 z testami offline (CI bieżącej zmiany w toku); dalsza praca ma dotyczyć wyłącznie publikacji NBP oraz aktualizacji źródeł FX/XAU; kalendarz Xetra jest poza zakresem. |
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
- Dla bieżącego produktu nie importować kalendarzy giełdowych; harmonogramy muszą opisywać publikacje NBP oraz świeżość/aktualizację źródeł FX i złota.
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


## 28. Etap 11 — automatyczne stosowanie świąt ustawowych

- CI [43303677601](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43303677601) dla commitu `b92c02f3250f21ca02a0203ee61600b1e7e47bc4`: konfiguracja, kompilacja i CTest 8/8 zakończone sukcesem.
- Dodano przeciążenie `generate_nbp_publication_windows(start, end_exclusive)`, które samodzielnie zbiera święta ustawowe dla wszystkich lat objętych zakresem i deleguje do wariantu z jawną listą świąt.
- Dodatkowe dni wolne ustanowione nadzwyczajnym rozporządzeniem nadal wymagają przekazania jawnego w wariancie z listą.
- Nowe testy weryfikują automatyczne wyłączenie 11 listopada 2026 r. i Wigilii 2025 r. oraz przesunięcie tabeli B.
- Oczekuje na CI.


## 29. Etap 11 — potwierdzenie automatycznych świąt NBP

- Commit: `c76bf3dc697e93ad3f69bca82a5180e08c338feb`.
- CI: [43304025796](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43304025796) — konfiguracja i kompilacja sukces; CTest **8/8, 100%** (0,13 s).
- Testy potwierdzają automatyczne wyłączenie 11 listopada 2026 r. oraz Wigilii 24 grudnia 2025 r. i przesunięcie tabeli B na poprzedni dzień roboczy.
- Reguły NBP A/B/C i ustawowe święta RP są zaimplementowane w module kalendarza. Etap 11 pozostaje w toku do dodania kalendarza faktycznie zaobserwowanych publikacji i importera oficjalnego kalendarza giełdowego.


## 30. Audyt kontynuacyjny i stan na 2026-11-12 22:18 (Europe/Warsaw)

### Potwierdzony stan GitHub
- Repozytorium: `bartoszruta26-droid/Monitor`, gałąź docelowa `main`.
- HEAD: `5ee3b1c22733ba4b7e589e5c8daa8f439bdb5ce3`.
- Gałęzie: wyłącznie `main`; otwarte PR: 0; otwarte Issues: 0.
- Najnowszy GitHub Actions: [43304376496](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43304376496) — sukces. Kroki Configure, Build i Test zakończone powodzeniem; job `native-linux` zakończony sukcesem.
- Testy adapterów pozostają deterministycznymi testami offline. Zielone CI potwierdza kompilację i CTest, nie rzeczywistą dostępność zewnętrznych usług ani zgodność licencyjną feedów.

### Stan funkcji potwierdzony przeglądem kodu
- Model danych C++20: `native/include/monitor/market_data.hpp`.
- Adaptery dostawców NBP A/B/C i złota, Frankfurter v2, World Bank Pink Sheet i XAUS: `native/src/market_data_adapters.cpp` i testy adapterów.
- Magazyn JSON: `native/src/market_data_repository.cpp`; dokumentacja opisuje wersjonowanie, atomowy zapis, flock, kopie i historię korekt.
- Agregacja próbkowanego OHLC: `native/src/market_data_aggregation.cpp`; nie zwiększa częstotliwości i jawnie zwraca luki.
- Model wykresów: `native/src/market_data_chart.cpp`; to kontrakt danych, nie renderer GUI.
- Regresja krocząca HTF, osobne metody n, n-1 i n-2 oraz tryb kalendarza sesyjnego: `native/src/market_data_regression.cpp`.
- Kalendarz sesyjny IANA/DST, okna publikacji NBP ze świętami ustawowymi RP oraz model obserwowanych publikacji: `native/src/market_data_calendar.cpp`.
- CMake zawiera testy dla rdzenia, modelu danych, adapterów, magazynu, agregacji, wykresów, regresji i kalendarzy.
- Serwer API, autoryzacja, WebSocket, klienci Linux GUI/WebUI/TUI/Android i MariaDB pozostają niezaimplementowane. Pine Script nadal wymaga niezależnego potwierdzenia w TradingView Pine Editor.

### Priorytet następnej pracy — domknięcie Etapu 11
Etap 11 pozostaje w toku. Następna implementacja powinna być rozdzielona na małe, testowalne zmiany:
1. Model faktycznie zaobserwowanych publikacji NBP został dodany jako `ObservedPublication`; `provider_published_on` pozostaje datą, a `received_at` rzeczywistym czasem odbioru odpowiedzi. Nie konstruować sztucznej godziny publikacji z przedziału A/B/C.
2. Import NBP: podział zakresu do 93 dni, obsługa 404/pustej odpowiedzi i czas odbioru mierzony per odpowiedź zostały wdrożone; po zielonym CI uznać ten podzakres za zakończony. Kolejny importer do zrobienia to wersjonowany kalendarz Xetra.
3. Dodać wersjonowany rejestr metadanych publikacji i aktualizacji źródeł FX/XAU; bez dokumentów kalendarzy giełdowych.
4. Dodać kontrolę świeżości i kompletności serii FX oraz XAU z uwzględnieniem rzeczywistej częstotliwości dostawcy.
5. Zachować istniejący JSON schema v1 przez kompatybilny odczyt i jawną migrację przy zmianie formatu; opublikowane wersje kalendarzy pozostają niezmienne.
6. Rozszerzyć testy o fixture offline: 404, brak danych, korekty źródła, zmiana wersji, nieznany format, skrócona sesja, handel w święto, zamknięcie oraz granice DST.
7. Po implementacji uruchomić pełne CI i dopiero wtedy zmienić status Etapu 11.

### Kontrole architektoniczne do kolejnego etapu
- Dodać test przepływu adapter → `upsert_batch` JSON → ponowny `load` → agregacja → wykres/HTF, z zachowaniem identyfikatorów źródła i jakości.
- Dodać test odporności na błędne lub częściowe rekordy: brak instrumentu/dostawcy, powtórzone klucze, niechronologiczne dane, wartości niefinitywne i błędne znaczniki czasu.
- Sprawdzić, czy testy równoległego zapisu obejmują również konflikt równoczesnej korekty tego samego klucza notowania, a nie tylko różne notowania.
- Dodać testy degradacji i odzyskiwania po awarii zapisu atomowego (błąd zapisu pliku tymczasowego, rename, backup i restore).
- Zachować rozróżnienie świecy źródłowej OHLC od OHLC utworzonego z próbek punktowych.

### Ograniczenia weryfikacji
- Kod i historię sprawdzono przez GitHub; w tej sesji nie uruchomiono lokalnego kompilatora ani nie wykonano pobierania z rzeczywistych API NBP/Xetra.
- Status kompilacji jest oparty na najnowszym zielonym CI dla SHA powyżej.
- Nie ma otwartego PR ani dodatkowej gałęzi, z której należałoby teraz scalać zmiany.



## 32. Etap 11 — testy modelu obserwowanych publikacji

- Dodano testy fixture do `monitor-market-data-calendar-tests`: grupowanie wielu obserwacji walutowych z jednej publikacji, rozdzielenie tabeli A i złota, zachowanie rzeczywistego `received_at`, pomijanie rekordu bez `provider_published_on` oraz odrzucenie pustego `provider_id`.
- Kod produkcyjny i dokumentacja modelu są już zapisane w commitach `7177c44` i `e7d4558`; testy dodano osobnym commitem, aby CI wskazywało jednoznacznie weryfikowany zakres.
- Status Etapu 11 pozostaje w toku do zielonego CI dla najnowszego SHA.


## 33. Scalanie prac z main — weryfikacja 2026-11-12 22:23 (Europe/Warsaw)

- Gałąź docelowa: `main`; aktualny HEAD: `fc88d20bf33fed2f9cc97ed49de4375ac2030a25`.
- Audyt gałęzi: istnieje wyłącznie `main`; brak dodatkowych gałęzi.
- Audyt pull requestów: 0 otwartych PR; brak zmian oczekujących na scalenie.
- Audyt commitów: seria zmian modelu obserwowanych publikacji, testów i dokumentacji jest już osiągalna z `main`. Najnowszy commit `fc88d20` ma rodzica `a6085d7`, co potwierdza bezpośrednią ciągłość tej serii na gałęzi.
- CI dla najnowszego HEAD: [43309747099](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43309747099) — sukces; job `native-linux`, Configure, Build i Test: success.
- CI dla testów modelu: [43309741566](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43309741566) — sukces; Configure, Build i Test: success.
- CI dla implementacji modelu: [43309669837](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43309669837) — sukces; Configure, Build i Test: success.
- Starsze nieudane przebiegi dotyczyły wcześniejszych SHA i zostały naprawione kolejnymi commitami; nie są wynikiem aktualnego HEAD.
- Wniosek: wszystkie dostępne prace zostały już scalone/zapisane na `main`; nie było otwartego PR ani odrębnej gałęzi do scalenia. Nie wykonano sztucznego merge commita.
- Etap 11 pozostaje w toku wyłącznie z powodu brakującego importera danych NBP i oficjalnego kalendarza Xetra oraz testów integracyjnych wskazanych powyżej.


## 34. Etap 11 — wzmocnienie importera historycznego NBP

- Commit `3b47002e337b36817c44eb8bfc3c358bc65f399d`: przeniesiono pobieranie `received_at` z początku całego `NbpAdapter::fetch` do obsługi każdej odpowiedzi HTTP. Czas odbioru opisuje teraz konkretną odpowiedź, a nie początek serii zapytań.
- Commit `24fa3ef0f6cc35ece615225e72a5018da9a4c7dc`: dodano fixture testujący zakres 95-dniowy rozdzielony na 93 + 2 dni, wszystkie cztery endpointy NBP, 404 dla pierwszego fragmentu i pustą odpowiedź 200 dla drugiego.
- Commit `ae9ccf21716e6cf5300ca5258c0d3f28998fb063`: dokumentacja wyjaśnia semantykę fragmentów, 404, pustych odpowiedzi, czasu odbioru i rozdzielenia `tradingDate` / `effectiveDate`.
- Workflow [43310994641](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43310994641) dla zmiany czasu odbioru i [43311018764](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311018764) dla testu 93 dni były w toku w chwili zapisu; nie oznaczać podetapu za zakończony przed potwierdzeniem Build i Test.
- Następny zakres: pozyskać oficjalny plik/kalendarz Xetra, potwierdzić format i warunki ponownego wykorzystania, a następnie zaprojektować importer z SHA-256 i niezmiennymi wersjami.


## 35. Etap 11 — potwierdzenie źródła kalendarza Xetra i status CI

- Potwierdzono oficjalną stronę kalendarza Xetra/Deutsche Börse: https://www.eurexgroup.com/xetra-en/newsroom/trading-calendar oraz stronę Cash Market: https://www.cashmarket.deutsche-boerse.com/cash-en/trading/trading-calendar-and-trading-hours.
- Potwierdzono oficjalny plik roczny 2026: https://www.cashmarket.deutsche-boerse.com/resource/blob/4580826/3e974decb414d6769cc2bc8e601f6c9a/data/xetra-trading-calendar-2026.pdf.
- Strona rozdziela dni bez handlu FWB od dni świątecznych, w których Xetra i Börse Frankfurt prowadzą handel. Dla 2026 r. jawne wyjątki handlowe: 14 maja, 25 maja i 4 czerwca. Parser musi zachować wyjątki zamiast dziedziczyć ogólną listę świąt.
- Sesje zapisujemy w strefie IANA `Europe/Berlin`; stały skrót CET nie wystarcza do obsługi CEST/DST.
- PDF jest kalendarzem graficznym, a nie stabilnym plikiem CSV/XLSX. Data publikacji PDF i licencja ponownego rozpowszechniania nie zostały potwierdzone; SHA-256 należy zapisać dopiero po pobraniu dokładnych bajtów. Nie wdrażać heurystycznego parsera PDF.
- Podzakres NBP (93-dniowe fragmenty, 404, puste odpowiedzi i rzeczywisty czas odbioru per odpowiedź) przeszedł CI dla commitu dokumentacyjno-integracyjnego `ae9ccf21716e6cf5300ca5258c0d3f28998fb063`: [43311044110](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311044110), Configure/Build/Test — success.
- CI dla commitu dokumentacji Xetra `f24805a`: [43311190718](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311190718) — w toku w chwili zapisu.
- Kolejny etap implementacyjny: dodać model `SourceCalendarDocument` (venue, zakres roku, IANA zone, URL, retrieved_at, content SHA-256, atrybucja/licencja) i jawne wyjątki sesyjne; następnie parser HTML oficjalnej tabeli Xetra z fixture obejmującym zamknięcia i dni handlu w święta. Skrócone sesje wymagają dodatkowego oficjalnego komunikatu/aukcyjnego harmonogramu.


## 36. Etap 11 — model dokumentu źródłowego i fixture Xetra 2026

- Commit `4eecb9e37f6b0332a425262052e892c9c04bf41f`: dodano `SourceCalendarDocument`, status licencji, pochodzenie, wersję, zakres, strefę IANA, opcjonalny czas pobrania i SHA-256 dokładnych bajtów.
- Commit `25caf23a71e34a3075fe33d2a5fa6579efd89fe7`: dodano walidację metadanych oraz fixture 365 dni Xetra 2026 z oddzielnymi statusami zamknięcia, handlu i rozliczenia.
- Commit `1051dea04d47f8ab04fab157737b259963fdbcb2`: testy pokrywają 365 dat, zamknięcia, dni handlu w święta (14 maja, 25 maja, 4 czerwca), wydłużone godziny dla grup produktów, settlement-only (24 i 31 grudnia), obowiązek sprawdzenia okólnika 30 grudnia oraz odrzucenie niepoprawnego SHA, venue i strefy.
- Commit `a063463480d71ccc60de858bb230182ccdfb3805`: dokumentacja opisuje model i ograniczenia źródła.
- CI dla modelu: [43311507761](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311507761); dla generatora fixture: [43311529064](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311529064); dla testów: [43311572796](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311572796). W chwili aktualizacji wpisu przebiegi były w toku — wynik Configure/Build/Test wymaga potwierdzenia.
- Poprzedni HEAD `2ccca053e2f2018116018a0d0924f9ef3d3a21c1` przeszedł CI [43311241403](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311241403) z sukcesem.
- Ograniczenie: fixture jest transkrypcją oficjalnego kalendarza, nie automatycznym importerem HTML/PDF. `content_sha256` i `retrieved_at` pozostają puste, a status licencji `Unknown`, ponieważ nie pobrano i nie zweryfikowano dokładnych bajtów dokumentu ani warunków ponownego wykorzystania.
- Następny zakres: parser semantycznej tabeli HTML oficjalnego kalendarza z fixture offline, wykrywanie zmian i tworzenie kolejnej wersji bez nadpisywania poprzedniej; osobno importer/obsługa okólników dla skróconych sesji.


## 37. Etap 11 — SHA-256 źródła i audyt testów Xetra

- Commit `769cfa99e785b11e4ef83c24162a5b26629ff784`: dodano własną, niezależną od zewnętrznych bibliotek implementację SHA-256 dla dokładnych bajtów źródła.
- Commit `de2667ae054091dd57d5bba1791af037e7893fd4`: dodano standardowe wektory testowe SHA-256 dla pustego wejścia, `abc` i 1000 znaków `a`.
- Commit `978e267e320e632dfebeca0c913659eac641fd2b1`: dokumentacja nakazuje liczyć hash przed dekodowaniem i parsowaniem źródła.
- Fixture Xetra i walidator metadanych: CI [43311594077](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311594077) — Configure, Build i Test: success.
- Testy kalendarza Xetra: [43311572796](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311572796) — Configure i Build: success; Test nadal w toku w ostatnim odczycie.
- SHA-256: CI [43311709415](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311709415) dla implementacji, [43311678837](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311678837) dla deklaracji i [43311636252](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311636252) dla rejestru były w toku; wymagana kontrola końcowego wyniku.
- Ostatni zielony commit bazowy: `a063463480d71ccc60de858bb230182ccdfb3805`, CI [43311594077](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311594077) zakończone sukcesem.
- Nie zapisano sumy SHA-256 kalendarza Xetra 2026, ponieważ nie pobrano dokładnych bajtów pliku źródłowego. Nie potwierdzono również licencji redystrybucji; status pozostaje `Unknown`.
- Następny etap: parser semantycznej tabeli HTML źródła Xetra z fixture offline, wykrywanie zmian i niezmienne wersje dokumentu. Po nim integracja źródła z repozytorium JSON i testy regresji wersji.


## 38. Etap 11 — parser semantycznego HTML Xetra 2026

- Commit `dc9f814990172f835574afd915e8a8605cddaa08`: zadeklarowano publiczny kontrakt `parse_xetra_html_calendar`.
- Commit `f74adf990ef2ed745c7ceccb9e2d6ff69fcd4deb`: dodano parser HTML oparty o libxml2; bez wykonywania skryptów i bez dostępu sieciowego.
- Commit `f70f5246f36ff63164a0306e9ff6805f4cf2e635`: obsłużono format dat dzień–miesiąc występujący w komunikatach o świętach.
- Commit `34bcbfe40d7b374d385f0d91491679d44f0d28e7`: dołączono importer do biblioteki natywnej. Pierwsze przebiegi po podłączeniu ujawniły brak jawnego nagłówka `<chrono>`; błąd kompilacji został naprawiony w commicie `0aee9f9359d6bd9a888d47d2a94a9e9f3767f329`.
- Commit `df0db6a1db2169e867801fef61ec74c450939d0c`: dodano offline fixture semantycznej tabeli i listy wyjątków.
- Commit `37771666d8362d5d09094301705f4b53913140b6` oraz `849991baaaac2b8b76b240705b8967d99e413555`: testy parsera, błędnych/niekompletnych danych i konfiguracja ścieżki fixture.
- Commit `0aee9f9359d6bd9a888d47d2a94a9e9f3767f329`: poprawiono wiązanie wyjątków z semantycznymi elementami listy; walidowane są etykiety Ascension Day, Whit Monday i Corpus Christi.
- Commit `d72b638eb0462c5ec5b41db76b079fb4238b5aad`: dokumentacja zakresu i ograniczeń importera.
- CI najnowszego HEAD: [43312439068](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43312439068) — Configure, Build i Test: success.
- Wcześniejsze przebiegi po pierwszym podłączeniu źródła zakończyły się błędem kompilacji z powodu braku `<chrono>`; poprawka jest obecna na HEAD, a pełny workflow najnowszego SHA przeszedł.
- Testy parsują syntetyczny fixture offline i weryfikują 365 dni, święta zamknięte, trzy święta handlowe, godziny 20:00/22:00, settlement-only, wymóg okólnika 30 grudnia, surowy SHA-256 i odrzucanie niekompletnej listy wyjątków.
- **Ograniczenia:** importer obsługuje wyłącznie schemat i zakres 2026; nie pobiera HTML, nie archiwizuje automatycznie kolejnych wersji, nie potwierdza licencji i nie parsuje okólników skróconych sesji. Fixture jest syntetyczny, a jego hash nie jest hashem oficjalnej strony.
- Następny etap: pobieranie surowych bajtów przez adapter źródłowy z limitem, timeoutem i walidacją HTTPS; zapis niezmiennych wersji `SourceCalendarDocument` i `VenueSessionCalendar` w JSON repository z zachowaniem zgodności schematu v1; testy round-trip, konfliktu wersji, niezmienności oraz nieudanych pobrań. Dopiero później importer okólników i rozszerzenie kalendarza na kolejne lata.


## 39. Etap 11 — trwały zapis wersjonowanych kalendarzy venue w JSON

- Commit `e2a7b19a0748718bc00ea067cc4658245c5134d6`: rozszerzono kontrakt repozytorium o kolekcję `venue_calendars` i operację `upsert_venue_calendar`.
- Commit `e461086078090e2336109527782a2e0aae74c5e9`: dodano kolekcję do migawki JSON.
- Commit `45ff1abc0a1ba913fa3ff63676ea3f86320ab90e`: dodano serializację/deserializację źródła, statusów dnia, sesji produktowych i metadanych licencji oraz walidację pełnego zakresu i godzin.
- Commit `7aaeede5a4ba9b963753c3ed9a02eac7215d1cff`: dołączono `venue_calendars` do dokumentu JSON; brak pola jest dozwolony przy odczycie starszych dokumentów v1.
- Commit `969445741305ba7e9faf2cf88a197a6d4b3eaded`: dodano reguły niezmienności i rosnących wersji oraz metodę upsert.
- Commit `8a21595feaa8225845c8a8b7028afc25e735dc34`: dodano testy round-trip, replay idempotentny, konflikt edycji, dopisanie v2, blokadę usuwania/zmiany przez `replace` i zgodność starego dokumentu v1.
- CI dla kompletnego kodu i testów na SHA `8a21595feaa8225845c8a8b7028afc25e735dc34`: [43312807643](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43312807643) — Configure, Build i Test: success.
- Wcześniejsze przebiegi po dodaniu samej deklaracji interfejsu lub części serializacji zakończyły się błędem linkowania z powodu niekompletnego etapu implementacji. Implementację metody i testy uzupełniono; pełny CI najnowszego SHA przeszedł.
- Zachowano główny `schema_version=1`: pole `venue_calendars` jest opcjonalnym rozszerzeniem. Dokumenty v1 bez tego pola są odczytywane jako pusta kolekcja; dotychczasowa kolekcja `session_calendars` pozostaje bez zmian.
- Wersja kalendarza jest kluczowana przez `document_id + version`; opublikowane rekordy są niezmienne. Zapis identycznej wersji jest idempotentny, zmiana lub usunięcie jest odrzucane, a korekta wymaga kolejnego numeru wersji.
- Następny etap: adapter pobierania źródła HTML (HTTPS, timeout, limit bajtów, kontrola statusu i typu treści), zapis czasu pobrania i SHA-256 surowych bajtów oraz atomowy zapis nowej wersji kalendarza do JSON. Testy HTTP muszą korzystać z lokalnego serwera/transportu testowego i nie mogą zależeć od sieci zewnętrznej. Okólnik skróconej sesji pozostaje osobnym dokumentem źródłowym.

 
## 40. Etap 12 — adapter HTTPS oficjalnego kalendarza Xetra

- Poprzedni etap 39 i najnowszy wówczas HEAD `84d59481b5db0e1517620b851bc4e358a8acbfd9`: [CI 43312975623](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43312975623) — success.
- `733c22885bd727813a45ba3166f09490a5e57392`: kontrakt `XetraCalendarSourceAdapter`, wykorzystujący istniejący interfejs `HttpTransport`.
- `3aebe5f78d2f28c05fc01554283209fceccc04a0`: pobieranie HTML tylko z zatwierdzonych hostów HTTPS, 15 s timeout, limit 4 MiB, odrzucanie statusów innych niż HTTP 200, wymóg `Content-Type: text/html`, SHA-256 dokładnych bajtów, czas pobrania UTC i wywołanie parsera semantycznego. Warstwa `CurlHttpTransport` ma wyłączone przekierowania i włączoną weryfikację certyfikatu.
- `5df0ec19dc15e1e4c15725c1c3aa2c7a3947e60a`: dołączono adapter do biblioteki.
- `4363d190108070e95b6d790c33d9513cf7f8df3e`: testy na sztucznym transporcie: sukces, skrót, czas, limity, błędny host i schemat, podany z góry hash, 302, 503, niepoprawny/missing Content-Type, za duża odpowiedź, zmieniony HTML, atomowość nieudanego importu, dopisanie wersji 2.
- `2312f0c4584684896161430322564f56538da290`: dołączono test adaptera do CTest.
- **CI:** w trakcie weryfikacji nowego SHA; nie oznaczać etapu jako zaliczonego do czasu zakończenia Configure/Build/Test.
- **Ograniczenia:** nie przeprowadzono pobrania żywej strony; fixture HTML jest syntetyczny. Nie potwierdzono licencji źródła. Obecny importer obsługuje tylko rok 2026, a komunikat o godzinach 30 grudnia wymaga osobnego źródła.
- Następne prace: weryfikacja najnowszego CI i poprawa ewentualnych regresji; rzeczywista walidacja struktury oficjalnej strony w kontrolowanym teście integracyjnym; zachowanie dokładnych bajtów HTML w archiwum źródeł; polityka odświeżania i deduplikacji wersji według SHA-256.


## 41. Etap 13 — archiwizacja surowego HTML i deduplikacja SHA-256

- Weryfikacja poprzedniego etapu 12: HEAD `13fb0e5c233182b774eb3f5fe9b2687dc9aa7fe8`, [CI 43314908943](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43314908943) — Configure, Build i Test: success.
- Commit `87274f12821bb0c65aee3697593cb4d7fd406e8b`: importer zachowuje dokładne bajty HTML w katalogu obok repozytorium JSON: `<plik>.sources/<document_id>/<version>-<sha256>.html`.
- Archiwizacja jest atomowa w obrębie katalogu (plik tymczasowy + rename); plik jest zapisywany przed publikacją metadanych kalendarza. Awaria może pozostawić osierocony obiekt archiwum, ale nie tworzy metadanych wskazujących na brakujące bajty.
- Importer weryfikuje ponownie bajty, gdy ścieżka archiwum już istnieje. Identyczne `document_id + SHA-256` nie publikuje kolejnej wersji kalendarza; zmiana choćby jednego bajtu tworzy nowy skrót i może dopisać kolejną wersję.
- Commit `1f3dfbd8313730e32f8946893f4641ab18fb26a2`: testy sprawdzają bitową zgodność archiwum z fixture, brak commitowania błędnego HTML, deduplikację identycznej treści przy innym numerze wersji oraz utworzenie wersji 2 po zmianie bajtów.
- Commit `7c7e93f45a0873da25ad49a0dabe2920df56b796`: opisano strukturę archiwum i zasady deduplikacji w `docs/market-calendar-sources.md`.
- **CI dla finalnego SHA `a5423c69bf80d46b0658be6d61b06fa6216bb1ea`:** [43315452951](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43315452951) — Configure, Build i Test: success. Etap 13 zakończony.
- Ograniczenia: testy są offline; nie pobierano na żywo oficjalnej strony. Status licencji pozostaje Unknown. Obecny parser nadal dotyczy wyłącznie 2026.
- Następny etap po zielonym CI: rozdzielenie identyfikacji dokumentu źródłowego od wydania kalendarza, rejestrowanie metadanych archiwum i kontrola integralności plików podczas ponownego uruchomienia; następnie rozszerzenie parsera na kolejne lata oraz osobny importer okólników.

- Dodatkowy przegląd etapu 13 wykrył, że strumień wejściowy z `istreambuf_iterator` nie gwarantuje ustawienia `eofbit`; warunek kontroli istniejącego pliku archiwum został poprawiony w `b21d6d8421b2b296e011b077847c8e734851cb42` i dodano test ponownego importu istniejącego pliku w `1148015a16e8d056aef460685cc51ef171411c3a`.
- Ostateczna weryfikacja CI dla kodu po poprawce jest wymagana przed oznaczeniem etapu 13 jako zakończonego.


## 42. Etap 14 — manifest archiwum i kontrola integralności

- Potwierdzono zielony CI etapu 13 dla HEAD `a5423c69bf80d46b0658be6d61b06fa6216bb1ea`, run [43315452951](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43315452951).
- Rozszerzono metadane dokumentu źródłowego o względną ścieżkę archiwum, długość bajtową i typ treści. Pola są opcjonalne, aby zachować odczyt starszych dokumentów JSON schema v1.
- Importer zapisuje metadane po trwałym utworzeniu archiwum i przed publikacją kalendarza.
- Odczyt repozytorium weryfikuje manifest: ścieżkę względną, brak komponentów traversal i symlinków, zwykły plik, długość oraz SHA-256. Brakujące lub uszkodzone archiwum blokuje odczyt migawki.
- Commit `43e1a0718f750def89cbb2dcba689b747816a68b`: dodano testy manifestu długości/MIME/ścieżki, uszkodzenia i braku archiwum, symlinku, traversal oraz odzyskania poprawnego JSON po negatywnych próbach. Zgodność starych dokumentów v1 bez pól manifestu pozostaje obsługiwana warunkowo w dekoderze; osobny fixture migracyjny wymaga jeszcze jawnego testu.
- CI run `43315898299` dla SHA `43e1a0718f750def89cbb2dcba689b747816a68b` wykazał błąd wyłącznie w asercji testu typu MIME: fixture po wcześniejszych przypadkach negatywnych miało `text/html`, a test oczekiwał `text/html; charset=utf-8`. Kompilacja przeszła, 8/9 testów przeszło.
- Commit `afcb85e927765b86917cf2e133a66c4ab8226bdf` poprawił oczekiwaną wartość testu, aby odpowiadała rzeczywistemu nagłówkowi fixture. CI dla poprawionego SHA `43316159732` pozostaje w toku; etap nadal nie jest zakończony.
- Ograniczenie: klucz `document_id + version` nadal wiąże dokument źródłowy z wersją interpretacji kalendarza; osobne wersjonowanie źródła i kalendarza pozostaje dalszą zmianą modelu.
- Status: implementacja w toku; oczekuje na testy i CI dla nowego SHA.


## 43. Audyt ciągłości, etap 14 — kontrola symlink race i zgodność legacy

### Audyt bieżącego repozytorium
- `main` jest jedyną gałęzią; brak otwartych PR i Issues w chwili audytu.
- HEAD przed rozpoczęciem tego podetapu: `1c6519fb1d2d526c729eefadf4a590fe88e93466`; CI [43316262104](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43316262104) — sukces.
- Etap 13 pozostaje zakończony; Etap 14 wymagał jawnego testu pól manifestu pominiętych w starszym JSON v1 oraz ochrony przed podmianą pośredniego katalogu na symlink.

### Zmiany bezpieczeństwa i testy
- Commit `3a00acecfa902a8b341c65bd9ed114bb090b1b3a`: kontrola archiwum zmieniona z sekwencji `symlink_status` + `open` na przechodzenie po ścieżce przez deskryptory katalogów i `openat(O_NOFOLLOW)`. Każdy katalog pośredni jest otwierany z `O_DIRECTORY|O_NOFOLLOW`, a plik końcowy z `O_NOFOLLOW`; eliminuje to wyścig pomiędzy sprawdzeniem ścieżki i otwarciem pliku.
- CI [43317179202](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43317179202) dla tego commitu — sukces (Configure/Build/Test).
- Commit `0869dee951e5846aee602f079e8fb82aa2378b53`: test legacy JSON v1 z usuniętymi polami `archive_relative_path`, `content_length_bytes` i `content_type`, a także test symlinku w katalogu nadrzędnym archiwum. CI [43317206138](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43317206138) — w toku w chwili aktualizacji; Etap 14 pozostaje otwarty do zakończenia tego przebiegu.
- Commit `13cf9433af0e04c019002fef79c481b24f9dfb93`: jawny nagłówek `<iterator>` dla `std::next`. CI [43317220372](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43317220372) — sukces.
- Testy negatywne nadal obejmują traversal, symlink pliku końcowego, brak/uszkodzenie archiwum, niezgodny rozmiar i SHA-256.

### Etap 15 — projekt rozdzielenia źródła i rewizji kalendarza
- Dodano `docs/market-calendar-versioning.md` (commit `8508a670627d2c9d0ed0feb0a8545e10ff0470b3`).
- Dokument definiuje docelowe `SourceArchiveObject` jako niezmienny obiekt treści identyfikowany SHA-256 oraz odrębny `VenueCalendarRevision` jako semantyczną rewizję kalendarza.
- Implementacja rozbita na 15A (model/JSON/walidacja), 15B (archiwum content-addressed i adapter), 15C (oddzielne numerowanie rewizji) i 15D (raport osieroconych obiektów). Nie zmieniać dotychczasowego JSON v1 bez testów migracyjnych.
- CI dokumentacji [43317354117](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43317354117) — w toku w chwili aktualizacji.

### Następne czynności
1. Potwierdzić pełny CI dla `0869dee`; w razie błędu pobrać log i poprawić test.
2. Zaktualizować status Etapu 14 na zakończony dopiero po zielonym Configure/Build/Test dla testu legacy i symlinku katalogu.
3. Rozpocząć Etap 15A: model `SourceArchiveObject`, kolekcja `source_objects`, serializacja, walidacja unikalności SHA-256 i referencji, testy round-trip oraz starego JSON v1.
4. Następnie podłączyć adapter Xetra w Etapie 15B; nie usuwać starych ścieżek ani metadanych, dopóki migracja i testy współistnienia nie będą zakończone.

## 44. Etap 14 zamknięty i Etap 15A — niezmienne obiekty źródłowe

### Etap 14 — końcowa weryfikacja
- Test legacy JSON v1 bez `archive_relative_path`, `content_length_bytes` i `content_type` oraz test dowiązania symbolicznego w katalogu nadrzędnym archiwum przeszły w CI [43317206138](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43317206138) dla SHA `0869dee951e5846aee602f079e8fb82aa2378b53`.
- Wzmocniona implementacja `openat(O_NOFOLLOW)` przeszła CI [43317179202](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43317179202), a poprawka nagłówka `<iterator>` przeszła CI [43317220372](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43317220372).
- Etap 14 uznany za zakończony. Ochrona obejmuje każdy komponent ścieżki, plik regularny, długość, SHA-256, traversal i symlinki; zgodność starszych rekordów v1 jest sprawdzona fixture.

### Etap 15A — model i magazyn obiektów źródłowych
- Commity: `b2e5d0087b28c73b74d3c5e464db07e1ddb278da` (model), `f2796425fb694600ae5bb809f2a1e563fddb6494` (interfejs repozytorium), `638a0fd427d01005b635b5bdc87261d61ae42dc7` (serializacja i walidacja), `21a315d54793870854a664e5ac1dbfd630ca9dae` (integralność archiwum), `f0431cf07b8a01f5909b9285225d86706ae4a3a5` (testy), `25343ea73ce00baeb0837180126cd5de02d4699e` (weryfikacja przed publikacją), `902ae42d45a158452c9e3d0feb4b6db95536c34f` / `2796bf671db2114f3482492bc71e7fe6a5fb3c8c` / `010dcbf0b2e29ba06f3c90b4f7dc106630ac939e` (rozdzielenie metadanych pobrania od metadanych bajtów), `04b0b05f9d351b402149c2b332dc92e82b076183` i `caf72e8c297d4c76fc4bdf89e95c1a31b6aebd2f` (wymuszenie nazwy content-addressed i test), `ede4b0f29cec1db11b89e20394254dfd4780e52b` (końcowa poprawka serializacji).
- Model: `SourceArchiveObject` ma stabilny identyfikator `sha256:<hash>` i przechowuje wyłącznie niezmienne metadane bajtów; `SourceCalendarDocument` przechowuje opcjonalną referencję `source_object_id` oraz pochodzenie pobrania.
- Dodano `source_objects` do `MarketDataSnapshot` i `MarketDataBatch`, serializację JSON, `upsert_source_object`, walidację hash/ID/ścieżki/nazwy pliku, kontrolę referencji, deduplikację, niezmienność i weryfikację pliku przed zapisem oraz przy odczycie.
- Testy potwierdzają, że dwie rewizje kalendarza mogą współdzielić jeden obiekt, a konflikt metadanych, brakująca referencja, usunięcie obiektu, niezgodna nazwa pliku i brak archiwum są odrzucane.
- CI dla końcowego kodu SHA `ede4b0f29cec1db11b89e20394254dfd4780e52b`: [43318104387](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43318104387) — Configure, Build i Test: success; CTest **9/9**.
- Wcześniejsze przebiegi `43317910938`, `43317918775`, `43317926291`, `43317942512`, `43317982706` i `43317990281` zakończyły się niepowodzeniem na przejściowych SHA podczas przenoszenia pól metadanych; logi wskazały pozostawione odwołania do pól usuniętych z modelu obiektu. Poprawiono serializację w `ede4b0f`; pełny CI końcowego SHA przeszedł.
- Status Etapu 15A: zakończony.

### Etap 15B — następny zakres
- Przenieść archiwizację Xetra na ścieżkę `*.sources/objects/<sha256>.<ext>` i podłączyć adapter do `SourceArchiveObject`.
- Deduplikować fizyczny zapis po SHA-256, tak aby ponowne pobranie tych samych bajtów nie tworzyło kolejnego pliku.
- Zachować legacy ścieżki archiwum oraz dotychczasowe pola JSON do czasu jawnej migracji; dodać testy współistnienia i awarii importu.

## 45. Etap 15B — archiwum content-addressed i integracja Xetra

- Commit `ec56dedae6544b9f1a672e0c8ccc5834600ededb`: adapter przeniesiony z archiwum `document_id/version/hash` do `*.sources/objects/<sha256>.html`; nowe kalendarze odwołują się przez `source_object_id`.
- Commity `6945519c05acd6e6068ff944fe965502f6fb2086`, `0321b999e39862dd3f601f5dff216f0f99b9b57b`, `b33aa57012012eb2a8c8401122f5340c7ad71a0c`: testy deduplikacji identycznych bajtów, nowego obiektu po zmianie bajtów, manifestu obiektu i ścieżki traversal.
- Commit `4c194b136644c9e984677cfbf883fa581dd7b279`: usunięto nieużywany sanitizer ścieżki dokumentu, ponieważ ścieżka archiwum jest teraz oparta wyłącznie na SHA-256.
- Commity `7ee99d1f904694ffb4a362130146f022d85e744c` i `4dc878ac01e251cb2dc47126f38ba528e9444800`: importer odrzuca metadane obiektu/archiwum podane przez wywołującego; test potwierdza odrzucenie caller-supplied source object ID.
- Commit `f61f1bd103348ff1c07c0133f22e4a75503ae573`: archiwizacja utwardzona. Plik tymczasowy jest synchronizowany, publikacja odbywa się bez nadpisywania istniejącego obiektu przez `link()`, a katalog archiwum jest synchronizowany. Kolizja istniejącego SHA jest akceptowana tylko przy identycznych bajtach; symlink i różna treść są odrzucane.
- CI dla końcowego SHA `f61f1bd103348ff1c07c0133f22e4a75503ae573`: [43319017765](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43319017765) — Configure/Build/Test: success; CTest **9/9**.
- Wcześniejsze błędy: `43318475559` i `43318569332` wykryły przejściowo niezgodne asercje ścieżki; `43318627432` wykrył pozostawione `.value()` po zmianie pola na `std::string`. Poprawki zostały wprowadzone; późniejsze CI dla SHA `b33aa57`, `4c194b1`, `7ee99d1` i `f61f1bd` przeszły.
- Status Etapu 15B: zakończony.

### Etap 15C — następny zakres
- Oddzielić wersję semantycznej rewizji kalendarza od wersji dokumentu źródłowego; zachować zgodność legacy `document_id + version`.
- Umożliwić ponowną interpretację istniejącego obiektu źródłowego bez ponownego pobrania bajtów.
- Dodać testy: wiele rewizji do jednego obiektu, parser revision bez zmiany źródła, zmiana źródła bez niejawnego nadpisania kalendarza i migracja JSON v1.

## 46. Etap 15C zakończony; Etap 15D — diagnostyka osieroconych obiektów

### Etap 15C — niezależne wersjonowanie źródła i kalendarza
- Model `VenueSessionCalendar` ma niezależne `calendar_id`, `revision` i `parser_version`; `SourceCalendarDocument.version` nadal oznacza wersję dokumentu źródłowego.
- Stare rekordy JSON v1 są materializowane do jawnego klucza kalendarza na podstawie `source.document_id + source.version`, bez zmiany znaczenia ich historycznych danych.
- Dodano publikowanie nowej rewizji z archiwum przez `publish_revision_from_archive`; ponowna interpretacja używa zapisanych bajtów, bez kolejnego żądania HTTP.
- Walidacja wiąże parę `document_id + source.version` z jednym SHA-256, a `calendar_id` z jednym venue. Konflikt źródła lub próba zmiany już opublikowanej rewizji jest odrzucana.
- Testy potwierdzają rewizje 10/11/12 na źródle 1/1/2, ponowne użycie tego samego obiektu, idempotencję, brak dodatkowego HTTP, konflikt bajtów i zachowanie starszego klucza.
- CI [43321518426](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43321518426) dla SHA `eb24c7d24080a6edb2fa486af58d6bbe6d432f87`: Configure/Build/Test — success; CTest 9/9.
- Wcześniejszy CI [43321093552](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43321093552) na przejściowym SHA `d9b05fc` miał 8/9 testów; awaria dotyczyła asercji tożsamości/wersji parsera. Poprawki modelu i parsera w `5ca65b2`, `342d5ec`, `87020fe`, `a8384e2`, `ae4a852` doprowadziły do zielonego CI końcowego SHA.
- Status Etapu 15C: zakończony.

### Etap 15D — raport osieroconych obiektów
- Dodano API `find_orphaned_source_objects(snapshot)`, które wskazuje rekordy `source_objects` bez referencji z `venue_calendars`.
- Raport jest wyłącznie diagnostyczny: nie usuwa obiektów, nie modyfikuje migawki i nie skanuje katalogu w poszukiwaniu nieindeksowanych plików.
- Dodano test: obiekt nieużywany pojawia się w raporcie, a po przypisaniu go do nowej rewizji znika z raportu.
- Commity: `c7eac6b11e34ffa70c2d1051f25de4f81fea68d7` (API), `ee0c2460541f7c2c87fff1e8d4e01060bace13cb` (implementacja), `15d5e2f606d030c01cff9d095e75bea45232feab` (test).
- CI dla aktualnego podetapu 15D jest uruchomiony; status pozostaje otwarty do zielonego Configure/Build/Test dla końcowego SHA.

### Następne czynności
1. Potwierdzić CI dla testu raportu osieroconych obiektów; poprawić ewentualne błędy.
2. Po zielonym CI zamknąć Etap 15D.
3. Rozpocząć Etap 16: dynamiczne lata parsera Xetra; usunąć założenia zakodowane na stałe dla 2026 dopiero po przygotowaniu niezależnych fixture i walidacji dat/układów.
4. Następnie Etap 17: osobny importer nadzwyczajnych komunikatów o skróconych sesjach. Godzin nie wolno wyprowadzać z samego kalendarza świąt ani zgadywać.

## 47. Etap 15D zamknięty; Etap 16 — parser Xetra z zakresem dynamicznego roku

### Etap 15D — wynik
- Raport `find_orphaned_source_objects` został wdrożony i przetestowany. Obiekt bez referencji jest wskazywany, a po przypisaniu do rewizji przestaje być raportowany.
- API jest nieinwazyjne: brak automatycznego usuwania i brak modyfikacji danych.
- CI [43321758225](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43321758225) dla SHA `15d5e2f606d030c01cff9d095e75bea45232feab`: Configure/Build/Test — success; CTest 9/9.
- Status Etapu 15D: zakończony. Cały Etap 15 (15A–15D) jest zakończony.

### Etap 16 — wymagania
- Obecny parser Xetra jest ograniczony do roku 2026: wyrażenia regularne, walidacja zakresu, komunikat o sesjach świątecznych i mapowanie dat są zakodowane dla jednego roku.
- Celem jest pobieranie roku z `valid_from`/`valid_to_exclusive`, parsowanie dat tylko z jednoznacznym rokiem i odrzucanie tabel z brakującym/niejednoznacznym rokiem albo nieobsługiwanym układem.
- Każdy rok otrzyma osobny fixture offline. Testy nie wykonują HTTP. Nie wprowadzać dat ani godzin sesji przez założenie; godziny i wyjątki mają pochodzić z treści oficjalnego komunikatu.
- Funkcję `build_xetra_2026_calendar` pozostawić jako fixture/regułę historyczną, dopóki nie będzie niezależnego źródła danych dla kolejnych lat.

### Następne czynności
1. Zidentyfikować elementy parsera zależne od 2026 i przygotować wspólne funkcje parsowania dat dla roku wynikającego z zakresu dokumentu.
2. Dodać fixture drugiego roku na podstawie jawnie przygotowanego, syntetycznego HTML w testach parsera; oznaczyć fixture jako syntetyczny, nie jako kopię oficjalnej strony.
3. Rozdzielić parsowanie tabeli dni wolnych od interpretacji godzin sesji specjalnych; brak oficjalnego komunikatu godzin oznacza odrzucenie importu albo brak publikacji sesji, a nie zgadywanie.
4. Uruchomić pełny CI i dopiero wtedy oznaczyć Etap 16 jako zakończony.

## 48. Etap 16 — parser Xetra: dynamiczny rok i godziny sesji ze źródła

### Zmiany w toku
- Parser przestał ograniczać daty i zakres do 2026. Rok jest wyprowadzany z `valid_from`; zakres musi obejmować dokładnie pełny rok kalendarzowy.
- Wspólne wyrażenie daty obsługuje zapis miesiąc-dzień-rok i dzień-miesiąc-rok, a każda data jest sprawdzana względem roku zakresu. Daty świąt handlowych nie są już porównywane z zestawem stałych dat 2026.
- Wersja parsera jest stemplowana rokiem (`xetra-html-YYYY-v1`). Daty wyjątkowych komunikatów są odczytywane tylko z komunikatów jawnie obecnych w HTML.
- Regularne i wydłużone godziny sesji są odczytywane z treści źródłowej; brak wymaganej informacji o godzinach powoduje odrzucenie parsera, zamiast użycia domyślnej godziny.
- Fixture testowy zawiera tabelę dwóch lat i dodatkowe dane 2027. Jest jawnie oznaczony jako syntetyczny, a nie kopia oficjalnej strony.
- Commity: `ce82488b01016dd819eb3913640bf836774ece7b` (dynamiczny rok), `163227e4c09abb9d79ea0a385b60a2be6633ad8b` (test 2027), `07caf3971f0dea7180be5c7222cdea06d8b8b4c4` i `e069e8e035f654747334901122e51db8b7dad871` (parsowanie godzin), `acc6a248a8f028daf85244b4227a86237cfd77a6` (fixture godzin), `e8052aa1efba9e5c66da3357ee96b97f28c0fe55` (testy godzin).
- CI [43321994920](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43321994920) dla pierwszego podetapu dynamicznego roku: success. CI dla późniejszych zmian godzin i testów jest w toku.
- Status Etapu 16: w trakcie; nie zamykać przed zielonym CI końcowego SHA i testami błędnego/brakującego roku oraz niekompletnych godzin.

### Kolejne czynności
1. Potwierdzić CI dla `e8052aa` i naprawić ewentualne błędy parsera/testów.
2. Dodać negatywne przypadki: brak roku w dacie tabeli, data z innym rokiem niż wybrany nagłówek oraz brak godzin sesji.
3. Zweryfikować obsługę roku przestępnego i pełnego zakresu 366 dni.
4. Dopiero po tych testach oznaczyć Etap 16 jako zakończony; potem przejść do Etapu 17 dla osobnych komunikatów nadzwyczajnych.

## 49. Etap 16 zamknięty; Etap 17 — komunikaty nadzwyczajnych sesji

### Etap 16 — wynik końcowy
- Parser Xetra wyprowadza rok z pełnego zakresu `valid_from`–`valid_to_exclusive`; odrzuca zakresy nieobejmujące dokładnie jednego roku kalendarzowego.
- Parsowanie dat działa dla wybranej kolumny roku, w obu kolejnościach zapisu; niejednoznaczne komórki, brak jawnego roku i data z innego roku są odrzucane.
- Wspólny parser obsługuje lata 2026, 2027 i przestępny 2028. Fixture jest syntetyczny i jawnie tak oznaczony; nie przedstawia się go jako pobranej strony giełdy.
- Daty świąt z handlem i daty komunikatów o odmiennych godzinach są filtrowane według roku wybranego kalendarza, a nie stałych dat 2026.
- Regularne godziny i godziny wydłużonych sesji świątecznych są odczytywane z treści dokumentu; brak wymaganych danych godzinowych blokuje parsowanie.
- Parser odrzuca brakujące i sprzeczne daty oraz niejednoznaczne kolumny roku.
- CI [43322572803](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43322572803) dla SHA `1b9d44eded1c26d89dc01f9964efa06f6312b961`: Configure/Build/Test — success; wszystkie 9 grup testowych przeszło.
- Wcześniejsze niepowodzenia w trakcie prac: `43322035265`/`43322077386` (brak porównania `CivilDate`), `43322271989` (parser traktował komunikaty innych lat jako błąd wybranego roku). Poprawki: jawne porównanie pól daty, filtrowanie komunikatów według roku i usunięcie ostatnich stałych dat 2026. Końcowy SHA przeszedł.
- Status Etapu 16: zakończony.

### Etap 17 — zakres i zasady
- Nadzwyczajne komunikaty o skróconych lub zmienionych godzinach muszą być osobnymi, wersjonowanymi dokumentami źródłowymi, a nie dopisywanymi na stałe wyjątkami do bazowego kalendarza.
- Każdy komunikat musi być archiwizowany jako niezmienny obiekt SHA-256; rewizja komunikatu jest oddzielna od rewizji kalendarza.
- Zmiana sesji może zostać opublikowana wyłącznie po jednoznacznym odczytaniu daty, rynku, grup produktów i godzin z oficjalnego komunikatu. Brak godzin oznacza brak nadpisania sesji, nie domyślną godzinę.
- Bazowy kalendarz pozostaje niezmienny. Komunikaty są nakładane jako osobna warstwa i mają własne testy konfliktów, anulowania oraz idempotencji.
- Nie wykonywano pobrania aktualnego komunikatu ani nie potwierdzono godzin dla 30 grudnia. Nie wolno ich wyprowadzać z samej flagi `requires_official_notice`.

### Następne czynności
1. Zaprojektować `VenueSessionNotice` i kolekcję `venue_session_notices` z referencją do archiwizowanego obiektu źródłowego.
2. Dodać walidację: rynek, kalendarz, data w zakresie, wersjonowanie, referencja SHA-256, kompletność i niezmienność.
3. Dodać parser komunikatu dla jawnych godzin oraz oddzielny stan informacyjny, gdy komunikat nie zawiera danych pozwalających zmienić sesję.
4. Dodać funkcję obliczającą efektywny dzień sesyjny z bazowego kalendarza i zatwierdzonych komunikatów; bez mutowania kalendarza bazowego.

## 50. Etap 17A — model i magazyn komunikatów sesyjnych (w toku)

- Dodano model `VenueSessionNotice` z osobną rewizją, rynkiem, kalendarzem, datą obowiązywania, referencją `source_object_id`, pochodzeniem, rodzajem komunikatu, opcjonalnym statusem dnia i sesjami produktów.
- Rodzaje: `Informational` (bez zmiany sesji), `SessionOverride` (jawny status i komplet godzin) oraz `Cancellation` (wskazanie konkretnej wcześniejszej rewizji).
- Dodano kolekcję `venue_session_notices` do `MarketDataSnapshot` i `MarketDataBatch`, opcjonalny odczyt JSON v1 oraz API `upsert_venue_session_notice`.
- Walidacja obejmuje referencję do istniejącego obiektu archiwum, rynek/kalendarz, datę w zakresie, unikalność i rosnące rewizje, niezmienność, godziny produktów oraz poprawną referencję anulowania.
- Testy obejmują komunikat informacyjny, jawne nadpisanie godzin, odrzucenie nadpisania bez godzin, atomowy zapis obiektu i anulowania, konflikt rewizji, datę poza zakresem i ochronę historii przed usunięciem.
- Commity: `d2d64e6e4a3c7cb62b23dd1af4f685392734a4b0` (model), `a89e536628a4217ad90baa027d2f2e3aeed7e33a` (kolekcja i API), `10cdba3b5742a4fd58e587f02cd1df1ac674a6b5` / `fa6541d56a45a25025c483de4be4a3544f9bf880` (serializacja i walidacja), `8ea421ed3a9ad848ed7c0e2ad0f07158b34fb9a4` (klucze), `5de0f9ef59c5565c8eb347796bd4ad1bbd3bbef8` (niezmienność/upsert), `839e218c624882686b32a6b9af87c4b1585731c0` (testy), `4f10309c6f1a5e8e3c4a2d89ab476ec8268bce62` (deklaracje funkcji przed walidacją migawki).
- Wcześniejsze CI dla przejściowych commitów wykazały brak implementacji metody wirtualnej i brak deklaracji funkcji walidujących przed ich użyciem. Poprawki są już na `main`; CI dla aktualnego SHA pozostaje w toku.
- Status Etapu 17A: w trakcie — nie zamykać przed pełnym zielonym Configure/Build/Test.

### Następny zakres po 17A
1. Etap 17B: parser komunikatu HTML, który publikuje wyłącznie jawnie podane daty/statusy/godziny i odrzuca niejednoznaczny tekst.
2. Etap 17C: funkcja efektywnego dnia sesyjnego nakładająca aktywne komunikaty na kopię bazowego dnia, bez mutacji kalendarza.
3. Etap 17D: testy anulowania, konfliktów, priorytetów rewizji i idempotencji.

## 51. Etap 17A zamknięty; Etap 17B — parser oficjalnych komunikatów

### Etap 17A — wynik
- `VenueSessionNotice` i kolekcja `venue_session_notices` są zapisane w JSON schema v1 jako pole opcjonalne, z zachowaniem odczytu starszych plików.
- Obsługiwane rodzaje: informacyjny, nadpisanie sesji i anulowanie wskazanej rewizji. Nadpisanie wymaga jawnego statusu dnia i godzin dla grup produktów; komunikat informacyjny nie zmienia sesji.
- Walidacja sprawdza obiekt źródłowy, rynek, identyfikator kalendarza, datę w zakresie, monotoniczne i niezmienne rewizje, poprawność przedziałów godzin oraz istnienie anulowanej rewizji.
- `upsert_venue_session_notice` i zapis atomowy obiektu archiwum wraz z komunikatem zostały dodane.
- Testy obejmują JSON round-trip, replay bez duplikowania rewizji, niemodyfikowalność, override z godzinami, odrzucenie override bez godzin, atomowy zapis anulowania, brak celu anulowania, datę poza zakresem i ochronę historii przed usunięciem.
- CI [43323528084](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43323528084) dla SHA `42d20e52c39748be35214e67707d33ee6726ae44`: Configure/Build/Test — success; CTest 9/9.
- Wcześniejsze niepowodzenia ujawniły brakujące deklaracje funkcji przed `validate_snapshot` oraz błędny identyfikator kalendarza w fixture. Poprawiono deklaracje i test odwołuje się do stabilnego `calendar_id`, nie do zmienionego identyfikatora dokumentu źródłowego.
- Status Etapu 17A: zakończony.

### Etap 17B — parser komunikatów
- Parser będzie przyjmował dokładne bajty HTML komunikatu oraz metadane kalendarza i obiektu archiwum.
- Wspierany będzie wyłącznie jednoznaczny układ: rynek, data lokalna, rodzaj komunikatu, status dnia oraz jawne przedziały godzin dla nazwanych grup produktów.
- Jeżeli komunikat nie zawiera godzin, parser może zwrócić wyłącznie `Informational`; nie tworzy zastępczego harmonogramu.
- Brak roku, niejednoznaczna data, nieznany produkt, konflikt strefy lub niepełny zakres godzin będą odrzucane.
- Fixture będzie syntetyczny i offline; zwykły CI nie będzie pobierał komunikatów na żywo.

### Następne czynności
1. Dodać `parse_xetra_session_notice_html` i testy dla komunikatu informacyjnego oraz jawnego skrócenia sesji.
2. Podłączyć parser do importera archiwizującego dokładne bajty jako `SourceArchiveObject`.
3. Następnie Etap 17C: wyliczanie efektywnego dnia z bazowego kalendarza i aktywnych komunikatów, bez mutacji bazowej rewizji.

## 52. Etap 17B — parser i importer komunikatów Xetra (w toku)

- Dodano `parse_xetra_session_notice_html(html, metadata)` w `native/include/monitor/xetra_calendar_importer.hpp` i `native/src/xetra_calendar_importer.cpp`.
- Parser wymaga jawnych pól `Venue`, `Effective date` (ISO YYYY-MM-DD) i `Notice kind`; rozpoznaje `informational`, `session override` oraz `cancellation`.
- Nadpisanie wymaga jawnego `Day status`; sesje odczytywane są z tabeli `Product Group / Opens / Closes`. Parser odrzuca nieznaną grupę produktu, powtórzoną grupę, nieprawidłową godzinę, odwrócony przedział i nadpisanie Trading bez godzin.
- Komunikat bez jawnych godzin pozostaje informacyjny; parser nie tworzy godzin zastępczych.
- Dodano `XetraCalendarSourceAdapter::import_notice`: HTTPS do zatwierdzonego hosta, limit 4 MiB, kontrola Content-Type, wyliczenie SHA-256, archiwizacja dokładnych bajtów i atomowy zapis obiektu źródłowego wraz z komunikatem.
- Ponowny import identycznej rewizji jest idempotentny; inna treść lub interpretacja dla tego samego `notice_id + revision` jest odrzucana. Przy replay sprawdzane są też długość i Content-Type istniejącego obiektu.
- Dodano trzy syntetyczne fixture offline: override, informational i cancellation oraz testy błędnego rynku, braku godzin, nieznanej grupy, nieprawidłowego czasu, archiwizacji, replay i konfliktu bajtów.
- Commity: `a666a499fc210cce5c58e2442cf82aba2d3243e3` (API parsera), `7e874783136c0a45225977a39fe73ce768a84e00` / `930acb43cec211e3491995f7a835f2407e62b354` / `f0bfb8b90262ca85e71ea0ec3d2b559fb0ac1900` (parser), `2fa44b48c2f698b9750c930403b038142feb9b43`, `5e02428977035851e770cf4f72bf3a7a6a06d2e0`, `6d90a590609cfee75202840f8d052a8e84effcbe` (fixture/testy), `e9d8c6d186f1eaa708170a7a1447f505e406fb57` / `8cf339b6fc745f7f4bc3b7466efaa7c515e458f6` (importer), `82af62a2a100be027a1481ca37c668abd0d4cecd` (idempotencja), `5acafd8f8cbe7659b02b52c9e29a75acd17478a5`, `3fa27b99f19a0b57ecc646769f3874a71cbc6b64`, `d011f19d26030630bfe361e6d5fdc1af6533fbb3`, `ec0048156b5d892daa39596d235e1a71feb88726` (testy), `02892ea449ca62ee3d1436d24e92fb9d8475c588` (kontrola metadanych replay).
- Przejściowe CI ujawniły konflikt nazwy zmiennej testowej `missing_hours_html`; zmieniono ją na `notice_missing_hours_html`. CI końcowego SHA jest w toku.
- Status Etapu 17B: zakończony. CI [43324267708](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43324267708) dla SHA `02892ea449ca62ee3d1436d24e92fb9d8475c588` oraz CI dokumentacji [43324342200](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43324342200) — success.

### Następny etap
Po zielonym CI 17B przejść do 17C: wyliczanie efektywnego dnia kalendarza przez zastosowanie najnowszych aktywnych rewizji komunikatów, anulowań i konfliktów, bez modyfikowania bazowego kalendarza.

## 53. Etap 17C — efektywny dzień sesyjny (w toku)

- Dodano `resolve_effective_venue_calendar_day(snapshot, calendar_id, date)`, wybierający najnowszą rewizję kalendarza i zwracający kopię dnia bazowego z nałożonymi komunikatami.
- Resolver wybiera najnowszą rewizję każdego `notice_id`, pomija rewizje anulowane przez aktywne komunikaty Cancellation i pozostawia komunikaty informacyjne bez wpływu na godziny.
- Jedno aktywne nadpisanie może zmienić status dnia i listę sesji; kilka aktywnych nadpisań dla tego samego dnia powoduje jawny konflikt.
- Flaga `requires_official_notice_unresolved` pozostaje aktywna, dopóki jawne nadpisanie nie rozstrzygnie dnia; anulowanie przywraca bazowy harmonogram i nierozstrzygnięty status.
- Testy dodane dla komunikatu informacyjnego, aktywnego skrócenia sesji, anulowania i konfliktu dwóch nadpisań.
- Commity: `f8e59cd05273d58a86b8955b624098c81051a5bc` (API), `85deadb9dfebc22a5c74bbfa66325069e71ac655` / `c131f7ff6b9877036d7244fa1cab98994d0e6b87` (resolver), `4eb666e92f3002775c5b2ffde1b4cc0f6d6194fa` (testy), `a68475f865755ad38f3b7fecf2d1bff2c7cc0517` (nagłówek string_view).
- CI resolvera [43324719408](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43324719408), raportu nierozstrzygniętych dat [43324853676](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43324853676) i walidacji diagnostyki [43325061044](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43325061044) — success. Etap 17C: zakończony.

### Kolejne czynności
1. Potwierdzić CI dla resolvera i testów; poprawić ewentualne błędy.
2. Dodać testy dla anulowania starszej rewizji, pozostawienia nowszej rewizji aktywnej oraz konfliktu kilku anulowań.
3. Dodać raport nierozstrzygniętych `requires_official_notice` dla całego zakresu kalendarza.
4. Zamknąć Etap 17C dopiero po zielonym CI; następnie zakończyć Etap 17D testami regresji i fixture.

## 55. Korekta raportu osieroconych źródeł po wprowadzeniu komunikatów

- Audyt Etapu 15D po dodaniu `VenueSessionNotice` wykazał, że raport osieroconych obiektów musi uwzględniać także referencje komunikatów, a nie wyłącznie bazowych kalendarzy.
- `find_orphaned_source_objects` rozszerzono o `venue_session_notices`; obiekt wskazany wyłącznie przez komunikat nie jest już fałszywie raportowany jako osierocony.
- Dodano regresję dla obiektu źródłowego anulowania, którego jedyną referencją jest komunikat.
- Commity: `a2e0fe988eea3c89fa83fb704ece92654f38b855` (implementacja), `ed2a6d45457f6fe95c2dcc2e24bce9ad64afd76a` (test).
- CI dla korekty jest w toku; status pozostaje otwarty do zielonego przebiegu.


## 56. Etap 17D — testy regresji i konfliktów komunikatów

- Rozszerzono testy o przypadek, w którym anulowanie dotyczy starszej rewizji, ale nowsza rewizja tego samego komunikatu pozostaje aktywna.
- Dodano test odrzucania dwóch aktywnych anulowań wskazujących tę samą rewizję.
- Zestaw regresji obejmuje już także: informacyjny komunikat bez zmiany godzin, jawne nadpisanie, przywrócenie bazowego harmonogramu po anulowaniu, konflikt dwóch nadpisań, nierozstrzygnięte daty i referencje obiektów źródłowych.
- Commit testów: `8bec0c0eab653bdfeeb40386b0774ee99b9014e9`.
- CI [43325240256](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43325240256) dla SHA `8bec0c0eab653bdfeeb40386b0774ee99b9014e9` — Configure/Build/Test success; testy konfliktów i anulowań przeszły.

### Kryterium zamknięcia Etapu 17
Pełny CI musi potwierdzić parser, importer, magazyn JSON, nakładanie i anulowanie komunikatów oraz raportowanie nierozstrzygniętych dni. Nie wykonywano pobierania rzeczywistych komunikatów na żywo i nie zatwierdzono godzin skróconej sesji 30 grudnia.

## 57. Etap 17D zamknięty — wynik Etapu 17

- Etap 17D: zakończony; testy regresji komunikatów, anulowań i konfliktów przeszły w CI [43325240256](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43325240256).
- CI dokumentacji i trackera [43325289179](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43325289179) — success.
- Etapy 17A–17D są zaimplementowane: model/JSON, parser HTML, archiwizujący importer, resolver efektywnego dnia, raport nierozstrzygniętych dat i regresje konfliktów.
- Audyt repozytorium: wyłącznie gałąź `main`; brak otwartych PR i Issues.
- Ograniczenie wdrożeniowe: parser komunikatów jest sprawdzony na syntetycznych fixture offline. Nie pobrano rzeczywistego okólnika i nie potwierdzono zgodności gramatyki parsera z aktualnym formatem publikacji giełdy. Import na żywo wymaga osobnej walidacji źródła.
- Nie zatwierdzono godzin 30 grudnia ani żadnej innej skróconej sesji bez oficjalnego komunikatu. Kod nie dopisuje takich godzin automatycznie.

### Następne etapy
1. Etap 18: adaptacja parsera do rzeczywistego formatu oficjalnych komunikatów Xetra, po pobraniu i zarchiwizowaniu przykładowego komunikatu; fixture musi zachować dokładne bajty i atrybucję.
2. Dodać możliwość ponownej interpretacji archiwizowanego komunikatu nową rewizją bez kolejnego HTTP, analogicznie do kalendarza.
3. Etap 19: raport audytowy źródeł i sesji (pochodzenie, licencja, SHA-256, nierozstrzygnięte dni, zastosowane/anulowane komunikaty).

## 58. Etap 18A — obsługa faz sesji i tekstu ogłoszenia PDF (w toku)

- Weryfikacja źródeł publicznych wykazała, że komunikaty 049/2026 i 050/2026 odsyłają do osobnego oficjalnego ogłoszenia PDF o końcowej sesji 2026. PDF podaje fazy Xetra: pre-trading 07:00, retail early 08:00, opening auction 08:55, intraday auction 13:00, closing auction 14:00, najwcześniejsza cena zamknięcia 14:05, Trade-at-Close do 14:10, usługi TES OTC/LIS i Xetra EnLight do 14:59 oraz koniec post-trading 15:00 (CET).
- Prosty przedział `opens_at`–`closes_at` nie opisuje tych zdarzeń bez utraty informacji; dodano `VenueSessionPhaseEventKind` i `VenueSessionPhaseEvent` z czasem lokalnym oraz zakresem produktu/usługi.
- `VenueCalendarDay` i `VenueSessionNotice` mają teraz `phase_events`; JSON schema v1 zapisuje nowe pole, a odczyt starych rekordów pozostaje opcjonalny.
- Walidacja dopuszcza handlowy dzień opisany samymi zdarzeniami fazowymi; resolver przenosi fazy na efektywny dzień bez tworzenia fikcyjnej sesji ciągłej.
- Dodano `parse_xetra_session_notice_text` do odczytu tekstu wydobytego z oficjalnego PDF; parser sprawdza MIC XETR, datę, wszystkie dziewięć faz, konwersję AM/PM i zakresy usług.
- Dodano syntetyczny fixture tekstowy z harmonogramem 30 grudnia 2026 i testy daty, MIC, kompletności faz, godzin i zakresów.
- Commity: `84fdcb3bbaa5d71839f1acdbf9b39c64c1756900` (model), `c8c1ff8d6cef9e41916f126b2ea2be4e4cdc8bdc` (enum/serializacja faz), `5a4a2e4133a5cd28a7c2581dae5be90bbbf3556f` (JSON), `82c63d6c2e32af6ef5a157a03215577bc834c6f5` (walidacja i resolver), `885a41182f27546970d109384d4874314922396f` (API tekstowego parsera), `e9a5b767f56ad24be54c0035a133aaf24b9a9af5` / `5886bd697c573cde3ff8c0120722fc183b431062` / `f4dace7581630a6fa8f5af873ddf43de0c0e97c4` (parser), `39edb7ae8736a2313d03702743fae3035d0b3f60` (fixture), `67d241c053031fc998f64ed3f59ab4547f123890` (test), `5e92d67df8551322ae10ed4d15aaa04245835b14` (porównywanie faz przy replay), `767f4b74c84c6749969588418939b743f05b8887` (test JSON i resolvera), `07248ae304307d1328a79e09d1693acf24629130` (lokalna walidacja faz w parserze).
- Wczesne CI ujawniły dwa problemy integracyjne: parser nie może wywoływać prywatnej walidacji repozytorium, a funkcja normalizacji czasu musi używać lokalnego helpera `normalized`. Poprawiono oba; CI dla końcowego SHA jest w toku.
- CI [43327239674](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43327239674) — success; potwierdzono model faz, parser tekstu, `pdftotext` i import PDF na syntetycznym pliku. Etapy 18A i 18B są zakończone w zakresie testów offline. Fixture PDF jest syntetyczny, nie jest oryginalnym plikiem giełdy.

### Następne czynności
1. Potwierdzić CI dla modelu faz i parsera tekstowego; poprawić błędy testów.
2. Etap 18C: ponownie interpretować zarchiwizowany PDF do nowej rewizji komunikatu bez kolejnego żądania HTTP; zapisywać wersję parsera.
3. Następnie potwierdzić parser na rzeczywistym, oficjalnym PDF, zachowując jego dokładne bajty i źródło.

## 60. Etap 18C — ponowna interpretacja archiwalnego PDF (w toku)

- Dodano `parser_version` w `VenueSessionNotice`; nowe parsery zapisują `xetra-html-notice-v1` lub `xetra-pdf-text-notice-v1`, a starsze rekordy bez pola otrzymują wartość kompatybilności `legacy-unversioned`.
- `same_notice_payload` uwzględnia wersję parsera i fazy sesji; niezmienność rewizji obejmuje teraz również zastosowaną wersję interpretatora.
- Dodano `publish_notice_revision_from_archive`: odczytuje i weryfikuje istniejący obiekt PDF, ponownie wydobywa tekst, tworzy nową rewizję komunikatu bez HTTP i ponownie wykorzystuje ten sam obiekt SHA-256.
- Testy sprawdzają brak dodatkowego żądania HTTP, współdzielenie jednego PDF przez rewizje 1 i 2 oraz idempotentny replay nowej rewizji.
- Commity: `parser_version` model i parser, serializacja/walidacja JSON, metoda offline reparse i testy są zapisane w historii `main`.
- CI [43327718619](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43327718619) dla metody offline reparse, parser_version i testów — Configure/Build/Test success. Etap 18C: zakończony.

## 61. Etap 18 — wynik i przejście do weryfikacji źródła pierwotnego

- Etapy 18A–18C zakończone dla implementacji i syntetycznych testów offline: model faz, parser tekstu PDF, import i archiwizacja surowego PDF, wersjonowanie parsera oraz ponowna interpretacja z archiwum bez HTTP.
- CI dla PDF importu i `pdftotext`: [43327239674](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43327239674) — success.
- CI dla parser_version i offline reparse: [43327718619](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43327718619) — success.
- Ograniczenie: harmonogram został odczytany z oficjalnego ogłoszenia publicznego, ale end-to-end fixture PDF jest syntetyczny. Nie zapisano jeszcze oryginalnych bajtów PDF giełdy w repozytorium i nie potwierdzono parsera na oryginalnym pliku.
- Żadna godzina nie jest wyprowadzana z domysłu; fazy 30 grudnia wynikają z oficjalnego ogłoszenia. Zwykłe godziny sesji nie są sztucznie skracane do 14:00.

### Etap 19 — weryfikacja oryginalnego dokumentu
1. Pobrać dokładny PDF wskazany przez oficjalną stronę/okólnik 049/2026 i 050/2026.
2. Zarchiwizować oryginalne bajty, SHA-256, długość, Content-Type, źródłowy URL i datę pobrania.
3. Wydobyć tekst przez `pdftotext` i porównać z dziewięcioma fazami zapisanymi w fixture.
4. Dodać oryginalny PDF jako fixture, jeżeli warunki licencji i repozytorium na to pozwalają; w przeciwnym razie dodać kontrolowany test z hash/metryką i tekstem wyodrębnionym.
5. Dodać raport audytowy: źródło, parser_version, obiekt archiwalny, zastosowane/anulowane komunikaty i daty wymagające dalszego ogłoszenia.

## 62. Etap 19A — dopasowanie parsera do oryginalnego układu PDF (w toku)

- Otworzono bezpośredni, oficjalny PDF: `https://cashmarket.deutsche-boerse.com/resource/blob/5673886/22764993c4b1617c5f465186066344e6/data/Final%20trading%20session%202026.pdf`.
- Oryginalny układ potwierdza, że Trade-at-Close jest rozdzielony między wiersze sekcji 2, a usługi off-book Xetra znajdują się w osobnej sekcji 3, pod literą b). Parser v1 oparty na syntetycznym układzie nie obsługiwał tej struktury.
- Parser zmieniono na `xetra-pdf-text-notice-v2`: rozdziela sekcję głównego rynku Xetra i podsekcję off-book Xetra, potrafi odczytać kontynuację Trade-at-Close między wierszami i nie pobiera godzin z sekcji Frankfurt.
- Dodano transkrypcję tekstową rzeczywistego układu PDF jako fixture testowy oraz test negatywny, który odrzuca MIC XFRA w podsekcji usług off-book.
- Zaktualizowano syntetyczny generator PDF do odzwierciedlenia rzeczywistego układu i podziału wierszy.
- Oryginalny PDF nie został pobrany jako plik binarny do repozytorium; fixture jest transkrypcją tekstu odczytanego z oficjalnego PDF. Nie podano fikcyjnego SHA-256 ani długości oryginalnego pliku.
- Commity: `eeed490b8a44bba7984662b109a40f595c2a9402` (parser v2), `136b121081edf732772499abafb6d4f533466dab` (fixture), `bbed8147fdfe1facec3e41416a9e06e6d52e6f68` (testy), `18d0f9b80d9f71914bbbdc62026b4a4f944102ea` (PDF end-to-end fixture).
- CI aktualnego SHA jest w toku; Etap 19A pozostaje otwarty do zielonego Configure/Build/Test.

### Następne czynności
1. Potwierdzić parser v2 na oryginalnym układzie i syntetycznym PDF w CI.
2. Ustalić, czy warunki licencji pozwalają umieścić oryginalny PDF jako fixture; jeśli nie, zachować wyłącznie tekstową transkrypcję i metadane źródła.
3. Jeśli dostęp do pliku binarnego będzie możliwy, zarchiwizować oryginalne bajty przez `import_pdf_notice`, aby rejestr obiektów zawierał rzeczywisty SHA-256.
4. Dopiero wtedy oznaczyć Etap 19A jako zweryfikowany.

## 63. Audyt ciągłości — Etap 15D i naprawa regresji parsera PDF

### Wynik audytu
- HEAD przed poprawką: `48cf44aaa06434085f66cc65fe4f03b619e0f80c`.
- Etap 15D jest zaimplementowany jako `find_orphaned_source_objects(snapshot)`; raport uwzględnia odwołania zarówno z rewizji kalendarzy venue, jak i komunikatów sesyjnych. Nie usuwa plików ani rekordów.
- Poprzednia korekta 15D wymagała ponownego sprawdzenia CI. Najnowszy pełny przebieg [43328474350](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43328474350) dla SHA `48cf44a` zakończył się niepowodzeniem w teście `monitor-xetra-calendar-source-tests`; kompilacja i pozostałe 8/9 grup testowych przeszły.
- Przyczyna: parser tekstu PDF nie rozpoznał podsekcji usług off-book w tekście zwróconym przez `pdftotext`. Test zakończył się wyjątkiem „Ogłoszenie PDF nie zawiera podsekcji dostępności usług Xetra”.

### Poprawka
- Commit `5b8024d234b95d9078f92f1d4ed1186066d9e5cc`: wykrywanie nagłówków sekcji rynku i usług off-book opiera się na stabilnych tokenach semantycznych, a nie na jednym dokładnym zapisie łącznika/diakrytyki. Pozostawiono obowiązkową kontrolę MIC XETR, więc tolerancja formatowania nie dopuszcza sekcji XFRA.
- Oczekuje na pełny CI dla nowego SHA. Etap 19A pozostaje otwarty do zielonego Configure/Build/Test.

### Kolejne prace
1. Potwierdzić test parsera PDF w CI i dodać fixture wariantów podziału linii/łącznika, jeżeli aktualny przebieg ujawni dalsze różnice ekstrakcji.
2. Po zielonym CI zamknąć 15D w trackerze, jeśli testy raportu obejmują zarówno kalendarze, jak i komunikaty.
3. Etap 19B: dodać audytowy eksport źródeł i efektywnych sesji: identyfikatory i rewizje, URI, data pobrania, atrybucja/licencja, SHA-256, długość/MIME, wersja parsera, referencje komunikatów i dni nierozstrzygnięte. Eksport ma być deterministyczny i tylko do odczytu.
4. Nie oznaczać źródła PDF jako binarnie zarchiwizowanego: w repozytorium znajduje się transkrypcja oficjalnego układu, nie oryginalny plik PDF i nie ma potwierdzonego hash oryginału.


### Uzupełnienie naprawy parsera PDF — 2026-11-13
- CI [43385176559](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43385176559) wykazał, że samo uelastycznienie nagłówka nie rozwiązało problemu: `pdftotext` rozdziela elementy nagłówka/podsekcji na linie, więc detekcja wymagająca wszystkich słów w jednym wierszu nadal zawodziła.
- Commit `2a2f13fbd63ef5fa277b47fe23751657e308b41a` zmienia wykrywanie podsekcji: wyszukiwanie odbywa się na całym znormalizowanym tekście, a Xetra subsection jest wybierana dopiero po nagłówku dostępności. Weryfikacja MIC XETR pozostaje obowiązkowa.
- CI dla tego commitu oraz dla raportu audytowego jest monitorowane; Etap 19A nadal otwarty.


### Uzupełnienie 19A — kontrola MIC i diagnostyka tekstu ekstrakcji
- CI wcześniejszej poprawki nadal wykazał brak wykrycia podsekcji, dlatego parser nie może opierać się na pełnej nazwie prawnej rynku w jednej linii.
- Commit `0746f308c946f805779ccb404c9e32112edf04c9` przełącza wybór podsekcji na etykietę Xetra występującą po sekcji dostępności.
- Commit `c22642023cf8ac3ab2a8e3afe7f50396f7cf8c65` wzmacnia walidację: wymaga jawnego MIC XETR oraz TES OTC w wybranej części tekstu; w razie niepowodzenia komunikat zawiera ograniczony fragment ekstrakcji do diagnozy CI. Test negatywny XFRA nadal musi odrzucać komunikat.
- Wynik CI dla tych commitów jest oczekiwany. Etap 19A nie został zamknięty.


## 64. Etap 19B — deterministyczny raport audytowy źródeł i efektywnych sesji (w toku)

- Dodano publiczne API `build_venue_audit_report_json(const MarketDataSnapshot&)`, zwracające raport `monitor.venue_audit.v1` bez modyfikowania repozytorium ani plików.
- Raport zawiera manifest obiektów SHA-256, pochodzenie i licencje kalendarzy, wersje parserów, pełną historię komunikatów, efektywne dni po zastosowaniu aktywnych nadpisań/anulowań, fazy sesji, daty wymagające oficjalnych komunikatów oraz identyfikatory osieroconych obiektów.
- Kolekcje są sortowane deterministycznie. Raport odzwierciedla wyłącznie zapisaną migawkę; nie wykonuje HTTP ani nie skanuje katalogu archiwum.
- Dodano test JSON parse, kompletności zakresu dni, faz komunikatu, referencji zastosowanego komunikatu, pustego raportu orphan dla używanych źródeł i powtarzalności eksportu tej samej migawki.
- Specyfikacja: `docs/venue-audit-report.md` (commit `0efc312e9aa299504a1ce60ac9e20b62a248e698`).
- Wstępny CI ujawnił brak jawnego include `nlohmann/json.hpp` w teście; dodano go w commicie `a5cfcefd8f3b3b2923b0de6bdd9204173ffa3210`. Nie uznaję Etapu 19B za zakończony do pełnego zielonego CI dla aktualnego HEAD.
- Parser PDF 19A nadal wymaga potwierdzenia: dotychczasowe próby ujawniły różnice ekstrakcji sekcji off-book. Wprowadzono diagnostykę i sprawdzanie jawnego MIC XETR; nie wolno osłabiać kontroli MIC w celu „przepuszczenia” fixture.

### Następne czynności
1. Potwierdzić pełny CI aktualnego HEAD; rozdzielić błędy parsera PDF od testów raportu.
2. Jeżeli parser nadal nie przejdzie, pobrać tekst z syntetycznego PDF w samym CI lub dodać test diagnostyczny ekstrakcji, a następnie naprawić fixture/parser na podstawie rzeczywistego tekstu.
3. Po zielonym CI zamknąć 19B i 19A tylko w zakresie, który rzeczywiście przeszedł. Oryginalny PDF nadal nie jest zarchiwizowany binarnie.


### Wyniki CI — korekty raportu i diagnostyka ekstrakcji PDF
- CI [43385526577](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43385526577) dla SHA `a5cfcef` potwierdził kompilację, ale wykrył dwa błędy testowe:
  1. raport JSON kończył się dosłownymi znakami `\\n`, przez co parser JSON zgłaszał dodatkowe dane po zamknięciu dokumentu;
  2. syntetyczny PDF importowany przez `pdftotext` nie ujawnił oczekiwanej podsekcji Xetra w postaci zakładanej przez parser.
- Commit `48fadaa15cf9ec555a84baf0bd85cf0b5fc6b2fc` poprawia zakończenie raportu rzeczywistym znakiem nowej linii.
- Commit `3749922b6a99f5e031bfa108e93f32c8b5bc335d` dodaje ograniczony fragment tekstu ekstrakcji do błędu brakującej podsekcji. To diagnostyka, nie obejście kontroli rynku; jawny MIC XETR pozostaje wymagany.
- Następny przebieg CI musi potwierdzić raport i dostarczyć dokładny fragment tekstu PDF. Etapy 19A i 19B pozostają otwarte.


### Diagnostyka CI i korekta fixture PDF — 2026-11-13
- CI [43385685546](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43385685546) dla SHA `3749922` pokazał, że tekst wyekstrahowany z syntetycznego PDF kończył się na samym słowie „availability”; generator jednego długiego obiektu tekstowego nie zachowywał dalszych linii w ścieżce testowej Poppler.
- Commit `e3adbe4303052adbc58b8834e53c3d732d192f91` zmienia generator PDF: każda linia otrzymuje osobny obiekt tekstowy i bezwzględną pozycję bazową. Dzięki temu fixture sprawdza import wielowierszowego PDF, a nie zachowanie jednego długiego strumienia tekstowego.
- Ten sam przebieg wykazał zbyt ścisłą asercję raportu: dzień może mieć kilka aktywnych referencji komunikatów. Commit `a461065eefc0443c01bba63fbab9219383af09f4` sprawdza obecność konkretnego komunikatu fazowego zamiast wymagać dokładnie jednej referencji.
- Pełny CI dla obu poprawek pozostaje wymagany.


### Diagnostyka i poprawka wielostronicowego fixture PDF — 2026-11-13
- CI [43386862600](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43386862600) dla SHA `9e61936816854f042b2ef02f59d81b24b94f9e76`: Configure i Build sukces; CTest 8/9. Jedyny błąd pozostał w `monitor-xetra-calendar-source-tests`; tekst po nagłówku dostępności kończył się na `availability`.
- Analiza generatora PDF wykazała błąd numeru obiektu: pierwsza strona wskazywała `/Contents 5 0 R`, czyli obiekt czcionki, zamiast strumienia treści nr 6. Druga strona wskazywała poprawny strumień nr 7. Błąd był w syntetycznym fixture, nie w kontroli MIC ani parserze.
- Commit `1a8fbc2d52f73e582e6dc8c7729ba53e08e9d6da` poprawia odwołanie pierwszej strony na `/Contents 6 0 R`. Nie zmieniono parsera ani walidacji MIC XETR.
- CI dla poprawionego SHA oczekuje na wynik. Etapy 19A i 19B pozostają otwarte; raport JSON przechodził w poprzednich przebiegach, ale końcowy status wymaga pełnego zielonego CI.

### Wynik CI #355/#356 i korekta zasobów czcionki fixture PDF — 2026-11-13
- CI [43388463784](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43388463784) dla SHA `1a8fbc2d52f73e582e6dc8c7729ba53e08e9d6da` oraz [43388475718](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43388475718) dla SHA `5f6c05445de0fa4727d28c4e852dc22d46c9d5b9`: Configure i Build zakończone sukcesem; CTest 8/9. W obu przebiegach jedyną porażką pozostał `monitor-xetra-calendar-source-tests`; ekstrakcja z PDF nadal zawierała tylko `availability`.
- Ponowna kontrola tablicy obiektów wykazała drugi, niezależny błąd fixture: obie strony wskazywały `/F1 6 0 R`, mimo że obiekt 6 jest strumieniem treści pierwszej strony, a obiekt 5 jest czcionką Helvetica. Poprawne odwołania stron to `/F1 5 0 R`, przy zachowaniu `/Contents 6 0 R` i `/Contents 7 0 R`.
- Commit `37859c9bbda44bd131a2257fbf505d25245e668b` poprawia zasób czcionki na obu stronach. Zmiana dotyczy wyłącznie syntetycznego fixture; parser oraz obowiązkowa kontrola MIC XETR i test negatywny XFRA pozostają bez zmian.
- Pełny CI dla SHA poprawki oczekuje na wynik. Etapy 19A i 19B pozostają otwarte; nie uznaje się ich za zakończone przed zielonym Configure/Build/Test dla dokładnego SHA.

### Wynik CI #357/#358 i korekta syntetycznej transkrypcji — 2026-11-13
- CI [43389444968](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43389444968) dla SHA `37859c9bbda44bd131a2257fbf505d25245e668b` oraz [43389479992](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43389479992) dla SHA `a2669ee3fce1ecb5973342d1a1309198d25b2d71`: Configure i Build sukces; CTest 8/9. Jedyny błąd nadal wystąpił w `monitor-xetra-calendar-source-tests`, z komunikatem o braku podsekcji Xetra po nagłówku dostępności.
- Diagnostyka kolejności testu wykazała, że wyjątek występował wcześniej niż import PDF: test najpierw parsuje `xetra_final_session_2026.synthetic.txt`. Fixture pozostawał w układzie parsera v1 — umieszczał ogólną linię TES OTC przed nagłówkiem i kończył się samym `3. Availability`. Parser v2 prawidłowo wymaga osobnej podsekcji off-book Xetra po nagłówku oraz jawnego MIC XETR.
- Commit `1e6270901635ae386c36e8931373530db594555f` aktualizuje wyłącznie transkrypcję syntetyczną: rozdziela sekcje Frankfurt (MIC XFRA) i Xetra (MIC XETR), przenosi TES OTC/LIS do odpowiednich podsekcji i zachowuje Xetra EnLight. Parser i walidacja MIC pozostają bez zmian.
- Poprzednia poprawka generatora PDF w commicie `37859c9bbda44bd131a2257fbf505d25245e668b` pozostaje potrzebna: obie strony wskazują teraz obiekt czcionki 5, a strumienie treści 6 i 7. Nie uznaję jej jednak za potwierdzoną przez CI, dopóki cały test nie przejdzie.
- Pełny CI dla nowego SHA oczekuje na wynik. Etapy 19A i 19B pozostają otwarte.

### Weryfikacja CI, zamknięcie raportu audytowego i kontrola praw do źródła — 2026-11-13
- CI [43389751733](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43389751733) dla SHA `1e6270901635ae386c36e8931373530db594555f`: Configure, Build i Test success; 9/9. Potwierdzono poprawny układ syntetycznej transkrypcji: po nagłówku dostępności występują odrębne podsekcje Frankfurt/XFRA i Xetra/XETR.
- CI [43389769825](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43389769825) dla SHA `718ba882d6c9f269b7bac3db4a0e900be8319939`: pełny sukces.
- Commit `76bb9739004a335778501327f94167f0880d386c` zmienia ekstrakcję z `pdftotext -layout` na `pdftotext -raw`, aby zachować kolejność strumienia tekstowego PDF. CI [43389924304](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43389924304) — Configure/Build/Test success.
- CI [43390011688](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43390011688) dla nowego fixture oraz [43390034838](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43390034838) dla SHA `b9eeed1fde2ad1d0f404523fe4231eb077e32434`: Configure, Build i Test success; 9/9. Najnowszy HEAD `main` to `b9eeed1fde2ad1d0f404523fe4231eb077e32434`; wszystkie trzy etapy CI są zielone.
- Etap 19B — deterministyczny raport audytowy — zamknięty na podstawie zielonego CI dla powyższego HEAD. Testy potwierdzają parsowalność JSON, manifest źródeł, pełną historię komunikatów, efektywne dni i fazy, nierozstrzygnięte daty, deterministyczność oraz rozpoznawanie obiektów używanych wyłącznie przez komunikaty.
- Etap 15D — raport osieroconych obiektów — zamknięty: referencje z kalendarzy i komunikatów są uwzględniane; raport nie usuwa plików ani rekordów.
- Etap 19A pozostaje otwarty. Zielony CI potwierdza parser v2 na niezależnie napisanych fixture tekstowym i PDF, ale nie na oryginalnych bajtach ogłoszenia Deutsche Börse. Nie ma potwierdzonego SHA-256, długości ani MIME oryginalnego PDF w archiwum repozytorium.
- Kontrola praw: oficjalna strona [„Urheberrechte und zulässige Nutzung”](https://www.deutsche-boerse.com/dbg-de/meta/urheberrechte) wskazuje ochronę treści i wymóg uprzedniej pisemnej zgody na kopiowanie, publikowanie lub rozpowszechnianie; wyjątek dotyczy pojedynczej kopii do osobistego, niekomercyjnego użytku. Wobec braku zgody nie należy umieszczać oryginalnego PDF ani pełnej transkrypcji w publicznym repozytorium.
- Zastąpiono pełną transkrypcję krótkim, niezależnie napisanym fixture `xetra_final_session_2026.layout.synthetic.txt`; plik `xetra_final_session_2026.official-extracted.txt` usunięto z bieżącego drzewa. Historia Git nadal zawiera wcześniejszą wersję — nie wykonywano przepisywania historii ani wymuszonego push.
- Kolejny krok 19A: przeprowadzić import rzeczywistego PDF w środowisku, w którym jego przechowywanie jest dozwolone, i zapisać w lokalnym archiwum rzeczywiste bajty oraz SHA-256, MIME, długość i czas pobrania. Do publicznego repozytorium publikować wyłącznie kod, syntetyczne fixture i metadane źródła, chyba że zostanie uzyskana pisemna zgoda.

### Uzupełnienie rejestru po konsolidacji fixture — 2026-11-13
- Doprecyzowanie wyników: CI [43390011688](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43390011688) dla SHA `50d3c8ae9d2d9f9fa0a82ff4013de9a49cf7badd` potwierdził dodanie fixture, ale test nadal wskazywał wtedy poprzedni plik. Właściwe wykonanie testów na `xetra_final_session_2026.layout.synthetic.txt` potwierdza CI [43390024432](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43390024432) dla SHA `fc6a2158c43aa37a68a5975c77fa169900ccfb5b` — Configure/Build/Test success.
- Commit `574c4a7fd3fe7d19dbe8fe68d00bfb472605218c` konsoliduje wszystkie asercje parsera na jednym niezależnie napisanym fixture: dziewięć faz, MIC głównego rynku, datę, kompletność harmonogramu oraz negatywną kontrolę MIC w podsekcji off-book.
- Commit `0d60d4683b864942b9cdfa45924fdac76965f919` usuwa redundantny, starszy fixture tekstowy. Bieżący test używa jednego syntetycznego pliku układu, bez transkrypcji źródłowej.
- CI [43390332363](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43390332363) dla dokładnego SHA `0d60d4683b864942b9cdfa45924fdac76965f919` — Configure, Build i Test success; 9/9. Ten SHA jest zweryfikowanym HEAD `main` przed niniejszym wpisem dokumentacyjnym.
- Etapy 15D i 19B pozostają zamknięte. Etap 19A nadal wymaga kontrolowanego importu oryginalnych bajtów PDF; syntetyczny test nie zastępuje tej weryfikacji.


### Audyt kontynuacyjny — poprawka hosta oficjalnego PDF Xetra — 2026-11-13

- Stan przed poprawką: HEAD `0feb473a03810790c64541ffa8a34a9002f55d7f`; CI [#368 / 43390480137](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43390480137) — Configure, Build i Test: success; CTest **9/9**.
- Przegląd ścieżki `import_pdf_notice` ujawnił rozbieżność w allowliście hostów. Kod dopuszczał `www.cashmarket.deutsche-boerse.com` i `www.eurexgroup.com`, ale odrzucał dokładny host `cashmarket.deutsche-boerse.com` zapisany w referencji oficjalnego PDF w dokumentacji i fixture. Odrzucenie następowało przed wysłaniem żądania HTTP.
- Oficjalna strona okólnika 049/2026 wskazuje Trading Calendar, a strona Trading Calendar publikuje link „Announcement final trading session 2026”; oficjalny dokument jest dostępny pod hostem Deutsche Börse. Referencje: [okólnik 049/2026](https://cashmarket.deutsche-boerse.com/cash-en/Stay-Informed/circulars-newsletters/deutsche-boerse-circulars/Final-trading-session-2026-5668166), [kalendarz sesyjny](https://www.cashmarket.deutsche-boerse.com/cash-en/trading/trading-calendar-and-trading-hours), [oficjalny PDF](https://www.cashmarket.deutsche-boerse.com/resource/blob/5673886/22764993c4b1617c5f465186066344e6/data/Final%20trading%20session%202026.pdf).
- Commit `4608ac02f2892a5a2c12b5ca3b5eceec3eb9a7a7` (`fix: allow official Xetra PDF apex hostname`) dodaje wyłącznie dokładny host `cashmarket.deutsche-boerse.com`. Nie zastosowano dopasowania sufiksowego ani wildcard. Test używa pełnego oficjalnego URL PDF i potwierdza, że podobny host `cashmarket.deutsche-boerse.com.attacker.example` jest odrzucany przed żądaniem HTTP.
- CI [#369 / 43393787470](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43393787470) dla dokładnego SHA `4608ac02f2892a5a2c12b5ca3b5eceec3eb9a7a7`: Configure — success; Build — success; Test — success; CTest **9/9**. Test `monitor-xetra-calendar-source-tests` przeszedł. W logach brak błędów kompilacji i testów; występuje jedynie ostrzeżenie GitHub Actions o wycofywaniu Node.js 20 w używanej akcji checkout.
- Zmiana nie dotyka parsera faz, semantyki sesji ani walidacji MIC. Obowiązkowe MIC XETR w sekcji rynku i podsekcji off-book oraz negatywny test XFRA pozostają bez zmian.
- Etap 19A pozostaje **otwarty**: ten commit naprawia dopuszczenie oficjalnego hosta, ale nie stanowi dowodu zarchiwizowania oryginalnych bajtów PDF. Próba pobrania binarnego PDF do środowiska roboczego w tym przebiegu nie powiodła się; nie zapisano fikcyjnego SHA-256, długości ani MIME. Nie dodano pliku binarnego do publicznego repozytorium.
- Następny krok 19A: w środowisku z dostępnym pobieraniem wykonać import oryginalnego PDF przez `import_pdf_notice`, zapisać rzeczywisty SHA-256, długość, MIME, URL i czas pobrania, a następnie porównać dziewięć faz z wynikiem parsera. Przed publicznym udostępnieniem binarnego dokumentu wymagana jest odrębna ocena warunków licencyjnych.

### Kontrola ciągłości i CI #370 — 2026-11-13 10:12 Europe/Warsaw

- Odczytano rzeczywisty HEAD `main`: `fa462169c094657bfe2c6f5ed3e6b40587939ba8`.
- Gałąź `main` wskazuje dokładnie na ten SHA. Repozytorium ma jedną gałąź roboczą; wyszukiwanie otwartych PR i Issues zwróciło po 0 wyników.
- Najnowszy workflow [Native C++ checks #370](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43393970254) dotyczy dokładnie tego SHA i zakończył się `success`.
- Job `native-linux`: instalacja zależności, Configure, Build i Test — wszystkie kroki `success`. CTest: **9/9**. Nie ma nieudanych jobów ani kroków, więc nie występują logi błędów do diagnozowania.
- Poprawka hosta w kodzie znajduje się w poprzednim commicie `4608ac02f2892a5a2c12b5ca3b5eceec3eb9a7a7`; jej CI [#369](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43393787470) również potwierdził Configure/Build/Test i 9/9. Dokładny host `cashmarket.deutsche-boerse.com` jest dozwolony, host z dopisanym obcym sufiksem jest odrzucany; walidacja MIC XETR i negatywne przypadki XFRA nie zostały zmienione.
- Ponowny audyt Etapu 11 potwierdził w kodzie: generator okien publikacji NBP A/B/C i świąt ustawowych, wyprowadzanie `ObservedPublication` wyłącznie z metadanych rzeczywistych obserwacji, parser semantycznej tabeli HTML Xetra, adapter HTTPS oraz archiwizację dokładnych bajtów źródłowych w repozytorium JSON. Testy sieciowe pozostają poza zwykłym CI; testy CI używają atrap transportu i fixture offline.
- Etap 11 pozostaje **w toku**: bieżący zakres Xetra jest ograniczony do obsługi fixture/układu 2026, a automatyczne pobieranie i weryfikacja kolejnych wersji rocznych oraz pełne kalendarze innych źródeł/instrumentów wymagają osobnych prac. Nie przypisujemy XAUS kalendarza giełdowego ani World Bank miesięcznej częstotliwości dziennej.
- Etap 19A pozostaje **otwarty**: test syntetycznego PDF i poprawka allowlisty hosta nie dowodzą pobrania oraz zarchiwizowania oryginalnych bajtów komunikatu końcoworocznego. Nadal brak rzeczywistego SHA-256, długości i MIME oryginalnego PDF; nie wpisano wartości szacunkowych.
- Następny niezależny etap z audytu `docs/next-implementation-audit.md`: Etap 12 — fixture i porównanie C++/Pine. Pine Script v6 jest zapisany, ale nie ma potwierdzonej kompilacji w Pine Editor. Należy przygotować porównywalny zestaw danych i wyników dla obu profili, a test TradingView pozostawić jawnie oczekującym na wykonanie w Pine Editor.
- Nie wykonano zmian kodu ani scalania w tej kontroli, ponieważ najnowszy CI jest zielony i nie ujawnił konkretnej usterki wymagającej poprawki. Nie należy tworzyć poprawki bez wykazanej przyczyny.
### Etap porównania numerycznego C++/Pine — poprawka stabilności SSE — 2026-11-13

- Weryfikacja CI poprzedniego HEAD `4163dc410333cf109d613ac194efb2b96d23665a`: [Native C++ checks #371](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43399568703) — Configure, Build i Test success; CTest 9/9.
- Audyt `native/src/regression.cpp` potwierdził, że C++ oblicza reszty bezpośrednio z wycentrowanych wartości w `long double`. Pine Script używał natomiast różnicy dużych sum kwadratów ceny, co może tracić precyzję przy dużym poziomie cen i małych resztach.
- Commit `0c7deab3c58d117f37178b54f0ebf2f2cdf5566d` dodaje test niezmienniczości SSE i nachylenia przy dużym przesunięciu poziomu cen (seria około `1e12`). Test porównuje dane po odjęciu tego samego przesunięcia i nie osłabia dotychczasowych asercji.
- Commit `2974e990590d8081a81c6d791d535caedb00b88c` centruje serię Pine względem najstarszej obserwacji okna przed obliczeniem sumy i sumy kwadratów. Zachowuje dotychczasowe definicje mianowników DGT `n-1` i LonesomeTheBlue `n`; nie zmienia sygnałów, długości ani walidacji źródeł.
- CI dla testu C++: [#372](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43400345065), SHA `0c7deab3c58d117f37178b54f0ebf2f2cdf5566d` — w chwili zapisu workflow `in_progress`; zależności instalowane, Configure/Build/Test oczekują. Wynik testu nie jest jeszcze znany.
- CI dla Pine commitu: [#373](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43400349801), SHA `2974e990590d8081a81c6d791d535caedb00b88c` — w chwili zapisu workflow `in_progress`; zależności instalowane, Configure/Build/Test oczekują. Ten workflow buduje C++ i nie kompiluje Pine Script.
- Kompilacja Pine Script w TradingView Pine Editor i porównanie wartości na wykresie nie zostały wykonane; nie deklarujemy ich jako zaliczonych. Po zielonym CI C++ należy przeprowadzić ręczną kompilację w Pine Editor oraz porównanie wartości dla fixture o dużym przesunięciu poziomu cen.
- Korekta planu: `docs/next-implementation-audit.md` definiuje **Etap 12 jako MariaDB i skalowanie**. Porównanie C++/Pine jest odrębnym zadaniem walidacji regresji i nie zmienia numeracji etapów.
- Etap 19A (oryginalny PDF) pozostaje otwarty; etapy 15D i 19B pozostają zamknięte na podstawie wcześniejszych potwierdzonych wyników. Brak zmian w MIC XETR, testach negatywnych XFRA ani parserze Xetra.
### Wyniki weryfikacji Etapu porównania C++/Pine — CI #372–#374 — 2026-11-13

- [CI #372](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43400345065), SHA `0c7deab3c58d117f37178b54f0ebf2f2cdf5566d`: Configure success, Build success, Test success; CTest **9/9**, w tym `monitor-core-tests` z nowym testem niezmienniczości SSE przy przesunięciu `1e12`. Logi nie zawierają błędów.
- [CI #373](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43400349801), SHA `2974e990590d8081a81c6d791d535caedb00b88c`: Configure success, Build success, Test success; CTest **9/9**. Logi nie zawierają błędów. Ten przebieg potwierdza C++ build po zmianie pliku Pine, ale nie kompiluje Pine Script.
- [CI #374](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43400431316), SHA `726fc20d22b10104a55846db5aa3885d01c4b120`: Configure success, Build success, Test success; CTest **9/9**. Jest to zweryfikowany CI dla commitu dokumentującego powyższe zmiany i aktualny HEAD przed następnym wpisem.
- Wszystkie trzy przebiegi zakończyły się `success`; brak nieudanych kroków i brak logów błędów.
- Zmiana C++ to test regresyjny stabilności numerycznej; zmiana Pine centruje ceny względem najstarszej obserwacji okna przed obliczeniem momentów. Nie zmieniono mianowników metod DGT/LonesomeTheBlue, sygnałów ani walidacji MIC XETR.
- Kompilacja i uruchomienie wskaźnika w TradingView Pine Editor pozostają niewykonane. CI C++ nie może być traktowane jako potwierdzenie składni lub zachowania Pine. Porównanie wyników na identycznym zestawie danych nadal wymaga ręcznej weryfikacji w Pine Editor.
- Korekta numeracji: w `docs/next-implementation-audit.md` Etap 12 to MariaDB i skalowanie; prace C++/Pine są osobną walidacją regresji. Etapów planu MariaDB nie rozpoczynano ani nie oznaczano jako ukończonych.
- Etapy 15D i 19B pozostają zamknięte; Etap 19A pozostaje otwarty z powodu braku oryginalnych bajtów PDF i ich zweryfikowanych metadanych.
### Korekta implementacji centrowania Pine — 2026-11-13

- Dodatkowy przegląd kodu po commicie `2974e990590d8081a81c6d791d535caedb00b88c` wykazał, że zastosowanie `math.sum(src - anchor, length)` mieszałoby wartości centrowane względem różnych historycznych kotwic, ponieważ każda historyczna wartość wyrażenia ma kotwicę z własnego baru. Nie uznano tej wersji za gotową do walidacji numerycznej.
- Poprawka `2af1e4c73b81dbd4a12253b3a4b2c4e8e7fa1844` wykonuje jawne sumowanie w pętli, odejmując od każdej próbki `src[i]` tę samą bieżącą kotwicę `src[length - 1]`. Dzięki temu wszystkie obserwacje w oknie są w tym samym układzie odniesienia. Zachowano wzór SSE i dotychczasowe mianowniki.
- CI dla poprzedniego commitu Pine [#373](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43400349801) oraz dokumentacji [#374](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43400431316) zakończył się success, CTest 9/9. Te przebiegi nie kompilują Pine Script i nie potwierdzają semantycznej poprawności wskaźnika.
- Nowy commit Pine wymaga osobnego CI C++ oraz ręcznej kompilacji i porównania w Pine Editor. Nie oznaczać walidacji C++/Pine jako ukończonej przed tymi czynnościami.

### Weryfikacja korekty centrowania Pine — CI #376/#377 — 2026-11-13

- Commit Pine `2af1e4c73b81dbd4a12253b3a4b2c4e8e7fa1844`: [CI #376](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43400653858) — Configure, Build i Test zakończone sukcesem; CTest **9/9**.
- Bieżący HEAD `0c8c021457ef012c12c3508647017104cab8642a`: [CI #377](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43400672756) — Configure, Build i Test zakończone sukcesem; CTest **9/9**. Log potwierdza wszystkie dziewięć zestawów testowych, w tym testy regresji, kalendarzy i źródła Xetra. Brak błędów kompilacji i testów.
- Zmiana Pine używa jednej kotwicy `src[length - 1]` dla wszystkich próbek bieżącego okna i jawnej pętli; usuwa błąd mieszania historycznych układów odniesienia. Nie jest to jednak kompilacja Pine Script: workflow #376/#377 buduje wyłącznie C++.
- Status walidacji: stabilność numeryczna C++ — potwierdzona testami; poprawka Pine — zapisana i objęta CI repozytorium; kompilacja w TradingView Pine Editor oraz porównanie wyników C++/Pine na identycznym fixture — nadal **oczekują na niezależną weryfikację**.
- Nie zamykam walidacji C++/Pine na podstawie zielonego CI C++. Nie zmieniono sygnałów, mianowników DGT/LonesomeTheBlue, walidacji MIC XETR ani testu negatywnego XFRA.
- Następny zakres Etapu 11: rozszerzyć pokrycie wersji rocznych oficjalnego kalendarza Xetra i testy parsera na niezależne lata/zmiany układu; zachować osobne kalendarze źródeł i instrumentów. Etap 11 pozostaje w toku. Etap 19A pozostaje otwarty do potwierdzenia oryginalnych bajtów PDF i ich rzeczywistych metadanych.

### Etap 11 — rozszerzenie kalendarza Xetra o rok 2025 i zmienne wyjątki świąteczne — 2026-11-13

- Ponowna kontrola oficjalnej strony Deutsche Börse wykazała, że liczba świąt z handlem zależy od roku: w 2025 r. operator wymienia także Dzień Jedności Niemiec (3 października), natomiast w 2026 r. lista zawiera trzy podstawowe święta. Źródło: https://www.cashmarket.deutsche-boerse.com/cash-en/trading/trading-calendar-and-trading-hours.
- Commit `7490df23f505f9489ec94a09ee5200719ae53bf0`: parser rozpoznaje teraz dodatkowy, jawnie nazwany wyjątek „Day of German Unity”. Nadal wymaga trzech podstawowych pozycji: Ascension Day, Whit Monday i Corpus Christi. Dodatkowe dni są uwzględniane tylko wtedy, gdy zostały jawnie wymienione dla wybranego roku; nie są wyprowadzane z ogólnej listy świąt państwowych. CI [#380](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43401591156) — Configure/Build/Test success, CTest **9/9**.
- Commit `89cc54343f50b0f49acd98cb37e286bfd34614e3`: syntetyczny fixture parsera rozszerzono o kolumnę 2025 i cztery wymienione przez operatora dat handlowe. Fixture pozostaje jawnie syntetyczny — nie jest kopią bajtową oficjalnej strony. CI [#381](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43401612055) — Configure/Build/Test success, CTest **9/9**.
- Commit `e055cfe5d5e61123b073c4fae4932216d0d6c249`: dodano test 2025 r., który potwierdza zamknięcie w Wielki Piątek oraz handel 3 października z odrębnymi godzinami dla akcji/ETP i produktów strukturyzowanych; dodano też negatywny test braku jednego z trzech podstawowych wyjątków. CI [#382](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43401645335) — Configure/Build/Test success, CTest **9/9**.
- Weryfikacja obejmuje obecnie parser 2025 oraz istniejące przypadki 2026, 2027 i 2028 (w tym rok przestępny 2028). Dane 2025–2028 w testach pochodzą z jawnie syntetycznego fixture; nie stanowią archiwizacji oryginalnego HTML ani potwierdzenia sumy SHA-256 oficjalnej strony.
- Aktualny HEAD po tych zmianach: `e055cfe5d5e61123b073c4fae4932216d0d6c249`; CI dla dokładnego SHA [#382](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43401645335) zakończył się sukcesem, CTest **9/9**.
- Etap 11 pozostaje **w toku**: potrzebne jest archiwizowanie dokładnych bajtów kolejnych wersji oficjalnego źródła, kontrola zmian semantycznych i potwierdzenie warunków ponownego wykorzystania. Nie należy utożsamiać syntetycznego fixture z oryginalnym dokumentem.


### Kontrola ciągłości i CI #383 — 2026-11-13 10:32 Europe/Warsaw

- Odczytany HEAD `main`: `9a947e5a483683e14fce232991af6fa4478a81d4`. Jedyna gałąź to `main`; otwarte PR: 0; otwarte Issues: 0.
- Commit `9a947e5a483683e14fce232991af6fa4478a81d4` (`docs: record Xetra 2025 calendar parser coverage`) przeszedł [Native C++ checks #383](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43401833422): **Configure — success; Build — success; Test — success; CTest 9/9 (100%)**. Wszystkie kroki joba `native-linux` zakończyły się powodzeniem. Logi nie zawierają błędów kompilacji ani testów; jedyne ostrzeżenie dotyczy przyszłego wycofania Node.js 20 w `actions/checkout@v4`.
- Testy #383 obejmują m.in. `monitor-xetra-calendar-source-tests`; wynik 9/9 potwierdza aktualny kod na dokładnym SHA. Commit #383 zmienia wyłącznie dokumentację.
- Ostatnia zmiana kodu parsera Xetra, commit `e055cfe5d5e61123b073c4fae4932216d0d6c249`, przeszła [CI #382](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43401645335), CTest 9/9. Test 2025 potwierdza zamknięcie w Wielki Piątek, handel 3 października z osobnymi godzinami dla akcji/ETP i produktów strukturyzowanych oraz odrzucenie kalendarza bez jednego z trzech wymaganych świąt z handlem.
- Ponowny przegląd `native/src/xetra_calendar_importer.cpp` i `native/tests/test_xetra_calendar_source.cpp` potwierdził, że kontrola MIC pozostaje jawna: główna sekcja i podsekcja off-book wymagają MIC XETR; testy negatywne odrzucają zamianę MIC głównego rynku na XEUR oraz MIC podsekcji off-book na XFRA. Nie zmieniano parsera PDF, walidacji MIC ani tych testów.
- Dodatkowa kontrola oficjalnej strony Deutsche Börse: tabela dni bez handlu obejmuje obecnie lata 2026–2032, w tym 2029, lecz blok jawnych świąt z handlem na tej samej stronie dotyczy wyłącznie 2026. Operator zastrzega też, że przyszły przegląd ma charakter informacyjny i może ulec zmianie. Źródło: https://www.cashmarket.deutsche-boerse.com/cash-en/trading/trading-calendar-and-trading-hours. Nie wolno przenosić wyjątków z 2026 na 2029–2032 ani publikować pełnego kalendarza tych lat bez jawnych danych o sesjach świątecznych. Parser zachowuje zachowanie fail-closed.
- Etap 11 pozostaje **w toku**. Dane 2025–2028 w fixture są syntetycznym odwzorowaniem; nadal brakuje archiwizacji dozwolonych do przechowywania oryginalnych bajtów źródła, ich rzeczywistego SHA-256, długości, MIME i czasu pobrania oraz potwierdzenia warunków ponownego wykorzystania.
- Etap 19A pozostaje **otwarty**: zielony test syntetycznego PDF nie zastępuje importu i archiwizacji oryginalnego PDF komunikatu końcoworocznego. Nie wpisano fikcyjnych metadanych źródła.
- CI nie wykrył konkretnej usterki, więc nie tworzono poprawki kodu ani nie scalano zmian. Żaden etap nie został zamknięty na podstawie samego commita.

### Etap 11 — odporność importerów Xetra na wielkość liter nagłówków HTTP — korekta wyników CI — 2026-11-13

- Audyt wykazał, że importer HTML i PDF wyszukiwał nagłówek `content-type` przez dokładne porównanie nazwy klucza. Nazwy nagłówków HTTP są niewrażliwe na wielkość liter; poprawne odpowiedzi z `Content-Type`, `CONTENT-TYPE` lub zapisem mieszanym mogły zostać błędnie odrzucone.
- Commit `2ec7c28e073b4b40dc4d117b57f126fd3d2c76d5`: [CI #390](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43409267724) — Configure success, Build failure, Test skipped. Kompilator zgłosił dwa odwołania do niezadeklarowanego `type_it` w ścieżce PDF (linie 515 i 532).
- Commit `79bfdc3b900ff2d77daf7105f0df744a8a1f18aa`: [CI #391](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43409271970) — Configure success, Build failure, Test skipped; ten sam błąd `type_it`.
- Commit `0b988b3932e11db98d613a86f18f65b879eb02fa`: [CI #392](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43409381755) — Configure success, Build failure, Test skipped. Log wykazał brak `raw_content_type` w ścieżce komunikatu HTML (linie 448 i 465) oraz pozostawione `type_it` w ścieżce PDF (linie 516 i 533).
- Commit dokumentacyjny `ba579daee2e61e6097418befbd31172e4649f9da`: [CI #393](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43409480577) — Configure success, Build failure, Test skipped; odziedziczone błędy kompilacji z #392.
- Commit `cdb3baff9f550907254ed2397e3061693fb7336c` ujednolicił odczyt nagłówka przez `header_value()` i zachował oryginalny zapis MIME w metadanych. [CI #394](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43409530067): Configure success, Build success, Test failure — 8/9. Jedyny błąd: `reject non-HTML content type`.
- Commit testowy `277c167b8eb7dc95bb7e8683309546eb54d00d42` dodał mieszany zapis `cOnTeNt-TyPe` dla importu komunikatu i PDF. [CI #395](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43409563471): Configure success, Build success, Test failure — 8/9. Przyczyna pozostała w fixture: test nieprawidłowego MIME dopisał klucz `content-type`, nie usuwając wcześniejszego `Content-Type: text/html`; mapa zawierała dwa klucze równoważne bez względu na wielkość liter.
- Commit `e0b81fa7ac1e6db1c6b45773e3727e337f469f48` dodał `transport.response.headers.clear()` przed ustawieniem `application/pdf` w teście odrzucania nie-HTML. Nie usunięto ani nie osłabiono asercji; test sprawdza teraz pojedynczą, jednoznaczną odpowiedź. [CI #396](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43409774439): Configure/Build/Test success; CTest **9/9 (100%)**, 1,97 s.
- Commit dokumentacyjny `0eee4c5b2150a2cf1171a0526087f0eeeea4e0cc`: [CI #397](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43409973301) — Configure/Build/Test success; CTest **9/9 (100%)**, 1,28 s.
- Zweryfikowany kod `header_value()` porównuje nazwy nagłówków bez rozróżniania wielkości liter. Testy obejmują `Content-Type`, `cOnTeNt-TyPe` i `CONTENT-TYPE`; walidacja MIME używa wartości znormalizowanej, a metadane archiwum zachowują oryginalną wartość nagłówka.
- Zmiany #390–#397 nie modyfikują kontroli MIC XETR. W parserze PDF nadal obowiązuje osobne potwierdzenie MIC XETR w głównej sekcji rynku i w podsekcji off-book; testy negatywne XEUR i XFRA pozostają aktywne.
- Usunięto przypadkowy końcowy tekst `NaN` z poprzedniego wpisu. Nie zmieniono statusu etapów na podstawie samego commita.

### Etap 11 i Etap 19A — stan źródeł po kontroli oficjalnej strony — 2026-11-13

- Oficjalna strona [Deutsche Börse — Trading Calendar and Trading Hours](https://www.cashmarket.deutsche-boerse.com/cash-en/trading/trading-calendar-and-trading-hours) podaje regularne godziny Xetra 09:00–17:30 CET i tabelę dni bez handlu dla lat 2026–2032. Gwiazdki przy Christmas Eve i New Year's Eve oznaczają brak handlu przy otwartym rozrachunku, jeżeli dzień przypada w dzień roboczy.
- Lista wyjątków świątecznych z handlem na tej stronie jest jawnie podana dla 2026 r.: 14 maja, 25 maja i 4 czerwca. Dla tych dni akcje/ETP kończą handel o 20:00 CET, a produkty strukturyzowane o 22:00 CET. Nie przenosić wyjątków na lata 2027–2032; operator zastrzega informacyjny charakter przyszłych dat i możliwość zmian.
- Odczytany tekst oficjalnego [PDF „Announcement final trading session 2026”](https://www.cashmarket.deutsche-boerse.com/resource/blob/5673886/22764993c4b1617c5f465186066344e6/data/Final%20trading%20session%202026.pdf) potwierdza osobną sekcję Xetra z MIC XETR, osobną sekcję Frankfurt z MIC XFRA oraz sekcję off-book Xetra z MIC XETR i usługami TES OTC/TES LIS/Xetra EnLight. Tekst udostępniony przez przegląd nie jest oryginalnym strumieniem bajtów PDF.
- Odczytany [Disclaimer Deutsche Börse](https://www.cashmarket.deutsche-boerse.com/cash-en/disclaimer) nie zawierał wyraźnej licencji ani zgody na ponowne rozpowszechnianie dokumentów. Przed umieszczeniem oryginalnych bajtów PDF/HTML w publicznym repozytorium wymagana jest osobna ocena praw do ponownego wykorzystania i atrybucji.
- Etap 11 pozostaje **w toku**: fixture 2025–2028 jest syntetyczny; brakuje archiwizacji dozwolonych oryginalnych bajtów HTML i ich rzeczywistych metadanych (SHA-256, długość, MIME, czas pobrania). Dla lat 2027–2032 nie dopowiadać nieopublikowanych wyjątków świątecznych.
- Etap 19A pozostaje **otwarty**: brak oryginalnych bajtów PDF w archiwum projektu oraz ich zweryfikowanego SHA-256, długości, MIME i czasu pobrania. Tekst PDF i syntetyczny fixture nie zastępują importu oryginalnego pliku.
- Historyczny zakres Xetra/PDF: opisane powyżej pobieranie oryginalnych HTML/PDF, walidacje MIC i dalsze prace nad kalendarzem giełdowym nie są już zadaniami bieżącego produktu. Zachowano je jako zapis poprzednich prac; nie kontynuować ich ani nie uzależniać od nich odbioru funkcji FX i złota.


## Decyzja zakresowa produktu — 2026-11-13

**Obowiązujący zakres:** wymiana walut FX oraz zakup, sprzedaż, wymiana i wycena złota XAU. Wszystkie kolejne zadania i priorytety mają służyć tym funkcjom. Wymagane są: kursy referencyjne i bid/ask, przeliczanie z uwzględnieniem spreadu/opłat, ceny XAU w PLN i innych walutach, gram/kg/uncja trojańska, odróżnienie ceny referencyjnej/spot od rzeczywistej ceny transakcji oraz portfel i historia operacji walutowo-złotowych.

**Poza zakresem:** akcje, udziały, papiery wartościowe, zlecenia maklerskie, arkusze zleceń, brokerzy akcyjni i kalendarze sesji giełd papierów wartościowych. Nie kontynuować prac Xetra/Frankfurt, MIC XETR/XFRA ani aukcji. Wszystkie wpisy Xetra w starszej części rejestru dokumentują wyłącznie prace historyczne; nie są otwartymi zadaniami ani kryterium akceptacji. Nie scalać ani nie rozszerzać funkcji akcyjnych w ramach tego produktu.

**Następny kierunek implementacyjny:** (1) zakończyć i przetestować importy FX/NBP/XAU w zakresie zgodnym z aktualnym modelem; (2) dopracować przeliczanie walut, bid/ask, spread i opłaty; (3) dopracować wycenę i jednostki złota; (4) zbudować portfel i dziennik transakcji walutowo-złotowych; (5) podłączyć wykresy i regresję do serii FX/XAU; (6) dopiero potem rozwijać API i klientów. CI nadal jest wymagane dla każdego SHA, ale zielone testy Xetra nie uzasadniają dalszych prac nad Xetra.

### Utrzymaniowa korekta historycznego testu PDF — 2026-11-13

- Po dodaniu asercji dziewięciu faz importowanego PDF CI wykrywało błędną godzinę `PostTradingEnd`. Diagnostyka w [CI #412](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43411313748) wykazała rzeczywistą wartość: enum kind `7`, czas `14:00:00`, podczas gdy źródło i test wymagają `15:00`.
- Przyczyną była zbyt szeroka część wyrażenia regularnego `.{0,100}end of post-trading`: przy ekstrakcji PDF mogła powiązać etykietę końca post-trading z wcześniejszym czasem `from 2.00 p.m.` zamknięcia aukcji. Poprawka wiąże przechwycony czas bezpośrednio z etykietą przez `\s*end of post-trading`. Nie zmienia walidacji MIC, kolejności pozostałych zdarzeń ani oczekiwanych wartości testu.
- Test zachowuje pełne sprawdzenie dziewięciu faz oraz zakresów usług; po rozdzieleniu asercji wypisuje rzeczywisty rodzaj, czas i zakres zdarzenia przy niezgodności.
- Commit poprawki: `f4e79d01964a6b7ae59f68e88f47b345b4969164`. [Native C++ checks #413](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43411692239) dla dokładnego SHA: Configure — success; Build — success; Test — success; CTest **9/9 (100%)**, 1,21 s.
- Jest to wyłącznie naprawa regresji w istniejącym, historycznym kodzie i teście, aby utrzymać zielone CI bez usuwania testów. Zgodnie z wiążącym `docs/PRODUCT_SCOPE.md` Xetra/Frankfurt, MIC i kalendarze sesji akcji pozostają poza zakresem produktu; nie planować dalszych funkcji Xetra.
- Etap 11 pozostaje w toku dla właściwego zakresu FX/NBP/XAU. Następne prace dotyczą obserwowanych publikacji NBP, aktualności źródeł FX/XAU, poprawności bid/ask i spreadu, wyceny złota w gramach/kg/uncjach oraz portfela walutowo-złotowego. Nie oznaczono żadnego z tych zakresów jako zakończonego.


## 45. Audyt ciągłości i nowy etap FX/XAU — 2026-11-13

### Potwierdzony stan repozytorium
- `main` HEAD: `cebdaba6194d3e8cb4ebe65f6251b5fc6126ae06`.
- Gałąź robocza: `feature/fx-xau-pricing-engine`; PR [#1](https://github.com/bartoszruta26-droid/Monitor/pull/1), bez scalania.
- Ostatni zielony CI dla `main`: [#414](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43411904371), dokładny SHA `cebdaba6194d3e8cb4ebe65f6251b5fc6126ae06`; Configure/Build/Test success, CTest 9/9.
- CI #413 również przeszedł 9/9. CI #412 i #411 zakończyły się błędem w historycznym teście Xetra; #413 przeszedł po korekcie historycznego parsera, lecz zgodnie z zakresem moduł Xetra jest obecnie odpinany od aktywnego builda.
- Jedyna gałąź na `main` przed rozpoczęciem nowego etapu; brak wcześniejszych otwartych PR i Issues.
- Po otwarciu PR #1 workflow został uruchomiony; wymagane jest sprawdzenie wyniku dla końcowego SHA PR, nie tylko dla pierwszego commitu gałęzi.

### Wynik audytu funkcjonalnego
- Istniejący model `Instrument` i `MarketObservation` przechowuje waluty bazowe/kwotowane oraz typy cen Reference, Bid, Ask, Mid, Spot i Transaction.
- Istniejące `convert_quantity()` obsługuje gram, kilogram i uncję trojańską.
- Brakowało wydzielonego mechanizmu wykonania kalkulacji wymiany walut z wyborem bid/ask, konwersją odwrotną, spreadem i jawnymi opłatami.
- Brakowało wyceny złota według masy, próby, ceny czystego kruszcu i opłat.
- Adaptery źródeł oraz testy offline istnieją; testy adapterów nie potwierdzają bieżącej dostępności usług zewnętrznych.
- Serwer API, portfel i dziennik transakcji nadal nie są zaimplementowane.

### Etap 16 — silnik kalkulacji FX i wyceny złota (w toku)
- Dodano `native/include/monitor/market_pricing.hpp` i `native/src/market_pricing.cpp`.
- `convert_currency()`: dla sprzedaży waluty bazowej stosuje bid; dla konwersji odwrotnej stosuje odwrotność ask; wylicza kurs wykonania, mid, spread w punktach bazowych, prowizję procentową, opłatę stałą w walucie wyjściowej, kwotę brutto i netto.
- `value_gold()`: przelicza gram/kg/uncję na gramy, stosuje próbę jako ułamek 0..1, przyjmuje cenę jednego grama czystego złota, pokazuje wartość brutto, opłatę i wartość netto.
- Dodano `native/tests/test_market_pricing.cpp` i test CTest `monitor-market-pricing-tests`. Przypadki obejmują kierunek pary, bid/ask, opłaty, spread, błędne kwotowanie, przekroczenie opłat, próbę i jednostki masy.
- CMake wyłącza historyczne `xetra_calendar_importer.cpp`, `xetra_calendar_source.cpp` i `test_xetra_calendar_source.cpp` z aktywnego builda/testów. Pliki źródłowe i ich historia pozostają w repozytorium, lecz nie są już aktywną zależnością ani kryterium CI.
- Commit/HEAD gałęzi po ostatniej zmianie: `ca033af001649a6fc95bf700b86cf491222cd7d2`.
- PR [#1](https://github.com/bartoszruta26-droid/Monitor/pull/1) oczekuje na pełny CI dla końcowego SHA. Pierwsze próby #416/#417 ujawniły, że test kalendarza nadal linkował funkcje Xetra; usunięto ten blok testowy w commicie `181136d5abb2f045f16795efb206597bc5ace003`. Dodano też ochronę przed przepełnieniem kursu środkowego i odwrotności ask w commicie `6a9e310bb553b7bd672aa9160cb2a22e258e9b62`, z testami granicznymi w `80bbb65879de8f501bff070a05e6d5f2be9b5fda`. CI #418, #419, #420 oraz #421 zakończyły się sukcesem. Dla końcowego SHA PR `ca033af001649a6fc95bf700b86cf491222cd7d2` przebieg [#421](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43416755404) potwierdził Configure/Build/Test success i CTest 9/9. PR pozostaje otwarty i nie został scalony; etap oczekuje na decyzję o scaleniu.
- Nie scalać PR przed zielonym CI i przeglądem diffu.

### Następne etapy po Etapie 16
1. Etap 17 — model transakcji FX/XAU, portfel FIFO, koszt nabycia i wynik zrealizowany/niezrealizowany (**zakończony dla warstwy obliczeniowej**, CI #429: 10/10; ograniczenia opisano w sekcji 46).
2. Etap 18 — trwały zapis/odczyt transakcji i portfela w istniejącym repozytorium, z migracją i zgodnością dotychczasowych plików JSON.
3. Etap 19 — przeliczanie wartości portfela XAU/FX do PLN i innych walut na podstawie źródeł wraz z czasem obserwacji i statusem jakości.
4. Etap 20 — integracja API i klientów dopiero po stabilizacji modelu oraz trwałości danych.
5. Nie rozwijać funkcji akcyjnych ani kalendarzy sesji giełdowych.

### Zasada dalszego monitorowania
Przed każdym następnym etapem sprawdzić bieżący `main`, gałęzie, PR, Issues, dokładny SHA PR oraz ostatni CI. Po każdym etapie dopisać commit, wynik Configure/Build/Test, liczbę testów i ograniczenia do tego pliku. Status „zakończony” nadawać wyłącznie po zielonym CI dla dokładnego SHA.


### Uzupełnienie monitoringu CI PR #1 — 2026-11-13
- [CI #416](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43416419441), SHA `8a986df76b4d1017e71f487071458ad516633c13`: Configure success; Build failure. Test kalendarza NBP nadal zawierał historyczne wywołania `parse_xetra_html_calendar()`, których implementację celowo odłączono od biblioteki.
- [CI #417](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43416489927), SHA `5f9774d0f7d27c153ec5705499adb5d8866e8c7c`: ten sam błąd linkowania; diagnoza potwierdzona w logu joba `native-linux`.
- Poprawka: commit `181136d5abb2f045f16795efb206597bc5ace003` usunął z `test_market_data_calendar.cpp` 97-wierszowy blok testów wyłącznie Xetra. Testy okien publikacji NBP, świąt RP i IANA TZ/DST pozostają aktywne.
- Dodatkowa kontrola numeryczna: commit `6a9e310bb553b7bd672aa9160cb2a22e258e9b62` stabilizuje wyliczanie mid przy dużych wartościach i odrzuca nieskończoną odwrotność ask; commit `80bbb65879de8f501bff070a05e6d5f2be9b5fda` dodaje testy tych granic.
- Przebiegi #418 (SHA `181136d5...`), #419 (SHA `6a9e310...`) i #420 (SHA `80bbb658...`) były w toku przy zapisie. Nie deklarować ich jako sukcesu bez ponownego odczytu GitHub Actions.


### Końcowa weryfikacja etapu 16 — 2026-11-13
- Końcowy SHA PR: `ca033af001649a6fc95bf700b86cf491222cd7d2`.
- [Native C++ checks #421](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43416755404): Configure — success; Build — success; Test — success; CTest **9/9 (100%)**, 1,55 s.
- Test `monitor-market-pricing-tests` jest włączony i przeszedł. Aktywny zestaw nie kompiluje ani nie uruchamia historycznego modułu/testów Xetra.
- Poprawki po nieudanych #416/#417 potwierdzone: odłączenie implementacji Xetra wymagało usunięcia jej historycznych przypadków z ogólnego testu kalendarza. Testy NBP i IANA pozostały zielone.
- PR [#1](https://github.com/bartoszruta26-droid/Monitor/pull/1) ma status open, mergeable=true, merged=false. Nie scalono go bez polecenia; `main` pozostaje na SHA `cebdaba6194d3e8cb4ebe65f6251b5fc6126ae06`.
- Codex Review nie dostarczył recenzji merytorycznej z powodu limitu użycia usługi; przegląd zmian wykonano ręcznie, ale nie należy przedstawiać go jako niezależnej recenzji.
- Następny etap po scaleniu: model transakcji FX/XAU i portfel z kosztem nabycia oraz wynikiem zrealizowanym/niezrealizowanym. Nie rozwijać funkcji akcyjnych ani kalendarzy giełdowych.


## 46. Etap 17 — model transakcji i portfel FX/XAU (2026-11-13)

### Zakres implementacji
- Dodano `native/include/monitor/portfolio.hpp`, `native/src/portfolio.cpp` i `native/tests/test_portfolio.cpp`.
- `PortfolioTransaction` przechowuje identyfikator, kod aktywa (kod waluty ISO 4217 albo XAU), rodzaj aktywa, stronę Buy/Sell, ilość, próbę, cenę jednostkową w walucie raportowania, opłatę w walucie raportowania, czas wykonania UTC (Unix) i identyfikator źródła/oferty.
- Waluty są ewidencjonowane w jednostkach waluty; złoto w gramach brutto i gramach czystego kruszcu po zastosowaniu próby.
- `calculate_portfolio()` wylicza pozycje i koszt nabycia metodą FIFO, przychód netto ze sprzedaży, wynik zrealizowany i niezrealizowany, wartość bieżącą oraz sumę opłat.
- Ceny bieżące są jawnie przekazywane w walucie raportowania: za jednostkę waluty albo za gram czystego złota. Brak ceny dla otwartej pozycji jest błędem, a nie domyślną wyceną.
- Walidacja obejmuje unikalność identyfikatorów transakcji, chronologię w obrębie aktywa, kod waluty, kod XAU, próbę, opłaty, nadmierną sprzedaż, brak ceny i wartości niefinitywne.
- CMake rejestruje `monitor-portfolio-tests`; liczba testów CTest wzrosła z 9 do 10.
- Testy sprawdzają FIFO dla EUR, wycenę złota próby 585, opłaty, wynik zrealizowany/niezrealizowany, zamkniętą całkowicie pozycję i walidację błędnych transakcji.

### Weryfikacja CI
- Commit kodu/testów i rejestracji CMake: `ff24b9bb1c989f1d7aa8134dde5f81c521e04006`.
- [Native C++ checks #429](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43418422394), dokładny SHA `ff24b9bb1c989f1d7aa8134dde5f81c521e04006`: Configure — success; Build — success; Test — success; CTest **10/10 (100%)**, 1,58 s.
- `monitor-market-pricing-tests` i `monitor-portfolio-tests` przechodzą; dotychczasowe testy danych, adapterów, repozytorium, agregacji, wykresów, regresji i kalendarza NBP również pozostają zielone.
- PR [#1](https://github.com/bartoszruta26-droid/Monitor/pull/1) nadal otwarty; nie scalono go. `main` pozostaje na `cebdaba6194d3e8cb4ebe65f6251b5fc6126ae06`.

### Ograniczenia — nie oznaczać jeszcze jako pełnego przepływu transakcyjnego
- To jest deterministyczny model obliczeniowy w pamięci; trwały zapis/odczyt i migracja JSON są kolejnym etapem.
- Ceny bieżące dostarcza wywołujący; moduł nie pobiera samodzielnie notowań ani nie przelicza między walutami. Integracja FX z wyceną portfela nastąpi w kolejnym etapie.
- Każda pozycja jest wyceniana w jednej, jawnej walucie raportowania. Wymiana walut powinna być odwzorowana jako osobne, sparowane zapisy nabycia i zbycia aktywów z tym samym identyfikatorem źródłowej operacji; model nie tworzy automatycznie drugiej nogi wymiany.
- FIFO działa w obrębie pojedynczego kodu aktywa; nie stanowi jeszcze księgowego ani podatkowego systemu rozliczeń. Zasady podatkowe i szczególne metody wyceny wymagają odrębnej decyzji.
- Niezależna automatyczna recenzja kodu nie jest dostępna z powodu limitu usługi review; nie przedstawiać zielonego CI jako substytutu niezależnego przeglądu.

### Kolejne prace
1. Etap 18 — trwała serializacja i repozytorium transakcji/portfela, wersjonowanie schematu i zgodność dotychczasowych JSON.
2. Dodać parowanie obu nóg wymiany FX, aby ewidencja obejmowała zarówno nabytą, jak i oddaną walutę, bez podwójnego naliczania opłat.
3. Etap 19 — pobieranie cen bieżących i przeliczanie XAU/FX do waluty raportowania ze znacznikami czasu i jakością źródeł.
4. Etap 20 — API i klienci.



## 47. Etap 18 — trwałe repozytorium transakcji FX/XAU (2026-11-13)

### Zakres implementacji
- Dodano native/include/monitor/portfolio_repository.hpp, native/src/portfolio_repository.cpp i native/tests/test_portfolio_repository.cpp.
- JsonPortfolioRepository przechowuje osobny dokument monitor.portfolio; nie zmienia istniejących dokumentów monitor.market_data ani ich schematu JSON.
- Dokument ma jawne schema_version=1, rewizję, walutę raportowania i listę transakcji. Enumy są zapisywane jako stabilne nazwy tekstowe.
- Operacje append i replace wymagają oczekiwanej rewizji (optimistic concurrency); konflikt nie nadpisuje nowszych danych.
- Zapis odbywa się przez plik tymczasowy, fsync, atomowe rename, blokadę plikową i opcjonalną kopię poprzedniej wersji .bak.
- Odczyt odrzuca pusty/uszkodzony dokument, nieznaną wersję schematu, niezgodną walutę raportowania, nadmierny rozmiar i nieprawidłowe transakcje. Brak pliku oznacza nowy pusty portfel w skonfigurowanej walucie.
- Walidacja dokumentu odtwarza księgę transakcji i odrzuca duplikaty identyfikatorów, chronologię błędną dla danego aktywa, sprzedaż ponad stan i błędne dane złota/walut.
- Dodano testy round-trip JSON, rewizji i konfliktu, kopii zapasowej, błędnej sprzedaży, chronologii, nieznanej wersji i uszkodzonego pliku.
- CMake rejestruje monitor-portfolio-repository-tests; CTest wzrósł do 11 testów.

### Weryfikacja CI
- Końcowy commit etapu: a0bafc71fcece63dbf6d7b63cd8459b6f0a5cc92.
- [Native C++ checks #435](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43418884964), dokładny SHA a0bafc71fcece63dbf6d7b63cd8459b6f0a5cc92: Configure — success; Build — success; Test — success; CTest **11/11 (100%)**, 1,32 s.
- W tym przebiegu przeszły: testy rdzenia, modelu danych, wyceny FX/XAU, portfela FIFO, repozytorium portfela JSON, adapterów, repozytorium danych rynkowych, agregacji, wykresów, regresji i kalendarza publikacji.
- [CI #434](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43418865528) dla poprzedniego SHA 2e084c988a3f4c322e5868d5acfd7bd33d70993c również przeszedł, ale końcową weryfikacją etapu jest #435 po rejestracji repozytorium w CMake.
- PR [#1](https://github.com/bartoszruta26-droid/Monitor/pull/1) pozostaje otwarty i nie scalony. main nie został zmieniony.

### Ograniczenia i ryzyka do kolejnego przeglądu
- Repozytorium jest osobnym plikiem JSON; nie ma jeszcze automatycznej migracji transakcji ze starszego formatu, ponieważ wcześniejszy schemat portfela nie istniał. Istniejące pliki monitor.market_data pozostają nietknięte.
- Kopia .bak jest tworzona przed podmianą; automatyczne odtwarzanie kopii wymaga jawnej operacji i nie jest wykonywane po cichu.
- Model nadal wymaga podania cen bieżących przez wywołującego; etap integracji źródeł FX/XAU i waluty raportowania jest następny.
- Należy w kolejnym przeglądzie ocenić zachowanie przy równoległych procesach, przerwaniu zapisu, braku miejsca i błędach systemu plików; testy obecne sprawdzają blokady rewizji i atomowy przepływ podstawowy, nie pełną macierz awarii systemowych.

### Następny etap
1. Etap 19 — warstwa wyceny portfela: pobieranie najnowszych obserwacji FX/XAU, kontrola świeżości, statusu jakości i źródła, konwersja do waluty raportowania.
2. Dodać jawne sparowanie obu nóg wymiany FX (waluta oddana i otrzymana) i wspólny identyfikator operacji, z jednorazowym naliczaniem prowizji.
3. Rozważyć odtwarzanie kopii zapasowej wyłącznie jako jawne działanie użytkownika i dodać testy uszkodzenia/odtworzenia.
4. Dopiero po stabilizacji wyceny i trwałości danych przejść do API oraz klientów.


## 48. Etap 19 — wycena portfela z obserwacji FX/XAU (2026-11-13)

### Zakres implementacji
- Dodano native/include/monitor/portfolio_valuation.hpp, native/src/portfolio_valuation.cpp i native/tests/test_portfolio_valuation.cpp.
- Funkcja build_portfolio_price_book() buduje ceny bieżące w walucie raportowania na podstawie MarketDataSnapshot; wynik zawiera pary cen do calculate_portfolio() oraz pełne pochodzenie wyceny.
- Dla walut wycena ma charakter likwidacyjny: sprzedaż waluty bazowej używa bid; wycena odwrotna używa odwrotności ask. Jeśli dostępny jest tylko mid/average/reference, wynik pozostaje oparty na cenie orientacyjnej, a rodzaj źródła jest zachowany w pochodzeniu.
- Obsługiwane są kursy krzyżowe przez graf par walutowych; ścieżka i wszystkie użyte instrumenty, dostawcy, URI, rodzaje cen oraz flagi jakości są zachowywane.
- Złoto jest normalizowane do ceny za gram czystego kruszcu. Cena USD/uncja trojańska jest przeliczana przez 31,1034768 g/uncję i następnie przez ścieżkę FX do waluty raportowania. Źródła PLN/g (NBP) są obsługiwane bezpośrednio.
- Źródła XAU są sortowane według czasu obserwacji; jeśli najnowsze źródło nie ma dostępnej ścieżki walutowej, moduł próbuje następne najświeższe źródło, którego walutę można przeliczyć.
- Do wyceny dopuszczane są tylko obserwacje Validated, nieprzyszłe względem as_of i mieszczące się w max_age_seconds. Dane Suspect, Unreviewed, przyszłe i przeterminowane są pomijane.
- Dodano testy kursu bezpośredniego bid, kursu krzyżowego, odwrotnej pary, XAU USD/uncja do PLN/gram czystego złota, odrzucania podejrzanego bid, świeżości oraz awaryjnego wyboru starszego źródła NBP.

### Weryfikacja CI
- Końcowy SHA etapu: 2f41a677697476a372e450bce2aec5fa39052c9e.
- [Native C++ checks #443](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43419410635), dokładny SHA 2f41a677697476a372e450bce2aec5fa39052c9e: Configure — success; Build — success; Test — success; CTest **12/12 (100%)**, 1,37 s.
- Wcześniejszy przebieg #442 wykrył nieprawidłowe oczekiwanie testu w scenariuszu, w którym podejrzany bid został odrzucony, ale nadal dostępna była starsza, prawidłowa wycena NBP. Poprawiono asercję tak, aby wymagała użycia tej starszej, konwertowalnej ceny NBP; poprawiony przebieg #443 jest zielony.
- PR [#1](https://github.com/bartoszruta26-droid/Monitor/pull/1) pozostaje otwarty i nie scalony. main pozostaje bez zmian.

### Ograniczenia
- Wycena wykorzystuje wyłącznie obserwacje już zapisane w MarketDataSnapshot; sama nie pobiera sieciowo nowych danych ani nie uruchamia adapterów.
- Dla waluty, dla której brak ścieżki do waluty raportowania, nie powstaje cena. Kalkulator portfela powinien zgłosić brak wymaganej ceny otwartej pozycji.
- Mid/average/reference są wycenami orientacyjnymi, nie gwarantowanymi cenami wykonania. Wynik nie jest ofertą brokera ani kantoru.
- Przy wielu możliwych ścieżkach FX wybierana jest najkrótsza ścieżka grafu, z deterministyczną kolejnością krawędzi; dalszy etap powinien rozważyć ranking ścieżek według spójności czasu i jakości.
- Pochodzenie ceny jest zachowane, ale warstwa użytkowa/API nie prezentuje go jeszcze w interfejsie.

### Następny etap
1. Etap 20 — transakcja wymiany FX jako atomowa para zapisów: waluta oddana i otrzymana, wspólny identyfikator operacji i jednokrotne naliczenie opłaty.
2. Połączenie repozytorium portfela z price book w jednym przebiegu wyceny.
3. Dodać jawne odtwarzanie kopii zapasowej portfela i testy odporności na przerwanie zapisu.
4. Dopiero potem API i klienci.
