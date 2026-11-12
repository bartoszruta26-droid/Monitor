# Rejestr etapów prac — Monitor

**Repozytorium:** `bartoszruta26-droid/Monitor`  
**Gałąź:** `main`  
**Ostatnia weryfikacja:** 2026-11-12 23:4x Europe/Warsaw
**Zasada prowadzenia:** status etapu zmienia się na „zakończony” dopiero po sprawdzeniu kodu, testów i CI dla konkretnego SHA. Ten plik jest bieżącym rejestrem wykonania; szczegóły techniczne pozostają w dokumentach etapów.

## Stan repozytorium przy audycie

- HEAD `main` przed zapisem tego wpisu: `f24805a0669ffd5ac2a756f03e18dbaf53edd08b`.
- Najnowszy potwierdzony zielony CI obejmujący kod i testy importera NBP: [CI 43311044110](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311044110), commit `ae9ccf2`; Configure, Build i Test przeszły.
- CI dla zmiany czasu odbioru per odpowiedź: [43310994641](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43310994641) — sukces.
- CI dla dokumentacji Xetra i rejestru: [43311190718](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43311190718) było w toku w chwili zapisu; nie zmienia ono kodu produkcyjnego.
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
| 11 | Kalendarze źródłowe i instrumentowe (NBP, metale, giełdy) | W toku — generator okien publikacji NBP | Generator okien A/B/C przeszedł CI 43303169211 (8/8). Generator okien A/B/C, świąt ustawowych i automatycznego stosowania kalendarza świąt przeszedł CI 43304025796 (8/8). Etap 11 nadal w toku: model obserwowanych publikacji NBP jest zaimplementowany i przetestowany; importer NBP ma już podział zakresu 93-dniowego i obsługę 404 z testami offline (CI bieżącej zmiany w toku); pozostaje implementacja parsera/wersjonowanego rejestru oficjalnego kalendarza venue (Xetra). |
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
3. Dodać wersjonowany rejestr źródłowego dokumentu kalendarza giełdowego (najpierw Xetra): URL, data publikacji, zakres obowiązywania, strefa IANA, skrót SHA-256, licencja/atrybucja.
4. Dodać importer Xetra dla jawnie wybranego oficjalnego formatu i utrzymywać skrócone sesje, dni handlowe w święta, zamknięcia i wyjątki jako osobne reguły.
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