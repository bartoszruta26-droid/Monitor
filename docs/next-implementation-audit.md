# Audyt kolejnych etapów prac — Monitor Rynków

**Data audytu:** 2026-11-12  
**Repozytorium:** `bartoszruta26-droid/Monitor`  
**Gałąź docelowa:** `main`  
**Dokument nadrzędny:** [Audyt funkcjonalny i plan realizacji](market-monitor-audit-and-roadmap.md)

## 1. Cel audytu

Dokument porządkuje kolejne prace nad Monitorem Rynków na podstawie założeń projektowych, aktualnego kodu i dokumentacji repozytorium. Określa zależności, kryteria akceptacji, ryzyka i kolejność realizacji. Jest punktem odniesienia dla kolejnych zadań implementacyjnych.

Aktywny rozwój rdzenia, serwera, adapterów i obliczeń prowadzimy w C++20 oraz Bash. Nie wprowadzamy Pythona do aktywnego procesu budowania, testowania ani uruchamiania. Historyczny kod Python może pozostać w repozytorium, jeżeli jest odłączony od tych procesów i nie wpływa na skompilowaną aplikację.

## 2. Wnioski z przeglądu aktualnych plików

### 2.1. Potwierdzony zakres natywnego rdzenia

W repozytorium znajdują się:
- biblioteka C++20 z regresją liniową OLS, SSE i szerokością kanału;
- CLI `monitor-regression`;
- testy jednostkowe regresji;
- skrypty Bash do budowania i testowania;
- konfiguracja CMake i GitHub Actions;
- połączony wskaźnik Pine Script v6 dla profili DGT i LonesomeTheBlue oraz dokumentacja tych metod.

Obecny model `WidthMethod` obejmuje `PopulationRms` (mianownik `n`) i `SampleSd` (mianownik `n-1`). Nie ma osobnego wariantu klasycznego błędu standardowego regresji z mianownikiem `n-2`. Nie wolno jednak zmieniać definicji DGT ani LonesomeTheBlue w celu ujednolicenia metod.

### 2.2. Rozbieżność wymagająca rozstrzygnięcia

Plik [open-data-providers.md](open-data-providers.md) opisuje adaptery NBP, Frankfurter i World Bank, endpointy importu oraz zapis danych FX i metali. Natomiast [market-monitor-audit-and-roadmap.md](market-monitor-audit-and-roadmap.md) określa adaptery i magazyn JSON jako niezaimplementowane, a obecny `native/CMakeLists.txt` wskazuje na budowanie przede wszystkim rdzenia regresji, CLI i jego testów.

Nie wolno uznać adapterów, endpointów ani migracji za gotowe wyłącznie na podstawie dokumentacji. Etap 0 ma zweryfikować ich rzeczywiste pliki źródłowe, włączenie do kompilacji, testy i działanie. Po tej kontroli dokumentacja musi zostać ujednolicona.

### 2.3. Status Pine Script

Wskaźnik Pine v6 jest zapisany w repozytorium, ale dokumentacja wyraźnie wskazuje, że nie został skompilowany i zweryfikowany w TradingView Pine Editor. Weryfikacja w Pine Editor pozostaje osobnym kryterium akceptacji. Wskaźnik TradingView nie jest jeszcze połączony z backendem Monitora.

## 3. Kolejność etapów

### Etap 0 — audyt rzeczywistego stanu repozytorium

**Cel:** uzyskać wiarygodny wykaz tego, co istnieje i działa.

Zadania:
1. Przejrzeć drzewo repozytorium, aktywne moduły, zależności, gałęzie, commity i otwarte PR/Issues.
2. Zweryfikować adaptery NBP, Frankfurter i World Bank: źródła C++, parsery, konfigurację, endpointy, migracje i testy.
3. Sprawdzić, czy magazyn JSON rzeczywiście istnieje, czy jest podłączony do aplikacji i czy zapis/odczyt działa.
4. Sprawdzić, czy istniejące endpointy są kompilowane i uruchamiane oraz czy mają testy integracyjne.
5. Ustalić, czy kod Python jest wyłącznie historyczny i odłączony od aktywnego budowania, testowania oraz uruchamiania.
6. Zweryfikować workflow GitHub Actions, skrypty Bash, kompilację i CTest.
7. Sporządzić wykaz funkcji: gotowe, częściowe, brakujące, zablokowane.

**Kryteria akceptacji:**
- każda deklarowana funkcja ma wskazany plik źródłowy i test albo status „brak”;
- rozbieżności dokumentacji są opisane i rozstrzygnięte;
- repozytorium buduje się w czystym środowisku Linux;
- testy są uruchamiane i ich wynik jest zapisany;
- nie ma niejawnej zależności aktywnego procesu od Pythona.

### Etap 1 — wspólny model danych rynkowych

**Cel:** zdefiniować spójny model dla walut, kruszców, surowców, notowań i źródeł.

Zakres:
- stabilne identyfikatory instrumentów i symboli;
- typ aktywa i jednostka miary;
- waluta bazowa i kwotowana;
- rozróżnienie cen referencyjnych, bid, ask, mid, spot i transakcyjnych;
- czas obserwacji, czas pobrania i strefa czasowa;
- częstotliwość źródłowa i status zamknięcia okresu;
- dostawca, identyfikator serii i pochodzenie danych;
- status jakości, kompletności i opóźnienia;
- wersjonowanie schematu i walidacja.

Dla metali trzeba jawnie obsłużyć gram, kilogram i uncję trojańską. Konwersje muszą być deterministyczne i testowane.

**Kryteria akceptacji:**
- każdy dostawca może zwrócić dane w tym samym modelu bez utraty jednostki, waluty, źródła i czasu;
- błędne jednostki, wartości niefinitywne i niepoprawne daty są odrzucane;
- cena referencyjna nie jest przedstawiana jako cena transakcyjna.

### Etap 2 — adaptery dostawców danych

**Cel:** pobierać aktualne i historyczne dane z otwartych źródeł.

Zakres:
- NBP: tabele A/B, tabela C oraz referencyjna cena złota PLN/g;
- Frankfurter: historyczne kursy referencyjne z jawną walutą bazową i kwotowaną;
- World Bank Pink Sheet: miesięczne ceny złota i srebra w USD/uncję trojańską;
- dodatkowy dostawca dziennych lub częstszych cen spot złota i srebra, wybrany po ocenie wiarygodności, opóźnienia, licencji, historii i jednostek.

Implementacja:
- C++20, libcurl i parsery właściwe dla formatów odpowiedzi;
- limity, paginacja, timeouty i kontrolowane ponawianie żądań;
- walidacja odpowiedzi i obsługa błędów;
- deduplikacja i idempotentny import;
- zachowanie surowej odpowiedzi lub jej kontrolowanego śladu audytowego;
- testy z ustalonymi odpowiedziami wzorcowymi, bez zależności od sieci w testach jednostkowych.

**Kryteria akceptacji:**
- pobieranie i normalizacja działają na danych wzorcowych;
- ponowny import nie tworzy duplikatów;
- błędy sieci i niepoprawne odpowiedzi nie uszkadzają magazynu;
- miesięczne dane World Bank pozostają oznaczone jako miesięczne;
- ceny dzienne i referencyjne są rozróżniane od notowań transakcyjnych.

### Etap 3 — magazyn JSON

**Cel:** zapewnić domyślny trwały zapis bez wymogu instalacji serwera bazy danych.

Zakres:
- wspólny interfejs repozytorium;
- pliki JSON dla instrumentów, notowań, źródeł, agregatów i metadanych;
- wersjonowanie formatu i migracje;
- zapis atomowy i kontrola błędów;
- blokady lub kontrola wersji przy współbieżnym dostępie;
- kopie bezpieczeństwa i odzyskiwanie po przerwanym zapisie;
- deduplikacja i odtwarzanie analiz z danych źródłowych.

Proponowany podział:
```text
data/
  market/
    currencies/
    metals/
    commodities/
  aggregates/
    daily/
    weekly/
    monthly/
    quarterly/
    half_yearly/
    yearly/
  analytics/
    regression/
    forecasts/
  metadata/
```

Nie dopuszczać do niekontrolowanego zapisu z wielu klientów do tego samego pliku. W docelowej architekturze zapis powinien przechodzić przez serwer i warstwę repozytoriów.

**Kryteria akceptacji:**
- importowane dane są dostępne po restarcie;
- zapis jest odporny na przerwanie procesu;
- równoczesne operacje nie powodują utraty danych;
- migracje i odtworzenie kopii są testowane;
- magazyn JSON jest domyślnym, działającym wariantem.

### Etap 4 — agregacja czasowa i świece

**Cel:** udostępnić sześć okresów i jawne metody agregacji.

Okresy: dzienny, tygodniowy, miesięczny, kwartalny, półroczny i roczny.

Metody: średnia arytmetyczna, średnia ważona tylko przy uzasadnionej wadze, mediana, OHLC oraz ostatnia dostępna obserwacja.

Należy zdefiniować granice okresów i kalendarz. Znaczniki czasu przechowywać w UTC, zachowując lokalną datę publikacji dostawcy, gdy jest istotna. Brak notowania w weekend lub święto nie oznacza ceny zero ani prawa do syntetycznego utworzenia obserwacji.

**Kryteria akceptacji:**
- wyniki są deterministyczne i odtwarzalne;
- średnia nie jest mylona ze świecą OHLC;
- brakujące obserwacje są oznaczane, a nie sztucznie uzupełniane;
- testy obejmują granice tygodni, miesięcy, kwartałów, półroczy i lat.

### Etap 5 — wykresy i historia notowań

**Cel:** udostępnić użyteczną prezentację danych.

Zakres:
- wykres liniowy i świecowy;
- wybór instrumentu, waluty i zakresu dat;
- wybór sześciu okresów;
- nakładanie średnich i median;
- porównywanie instrumentów;
- oznaczanie luk i nieaktualnych danych;
- widoczne źródło, rodzaj ceny i czas aktualizacji.

**Kryteria akceptacji:**
- zmiana instrumentu i okresu odświeża wykres zgodnie z modelem danych;
- wykres nie sugeruje dokładności większej niż częstotliwość źródła;
- źródło i rodzaj notowania są widoczne.

### Etap 6 — silnik regresji DGT i LonesomeTheBlue

**Cel:** przenieść profile metodologiczne do wspólnego, testowalnego silnika C++.

Zachować odrębne profile:
- DGT: domyślne okna 200 i 50; szerokość `sqrt(SSE/(n-1))`; poziomy ±1, ±2 i ±3 SD;
- LonesomeTheBlue: okna 377, 233 i 144; szerokość `sqrt(SSE/n)`; mnożnik 2,41; poziomy Fibo 0,236, 0,382, 0,618 i 0,786.

Wymagania:
1. Zachować poprawione indeksowanie reszt LonesomeTheBlue.
2. Dodać jawny wariant `n-2` jako odrębną metodę statystyczną, bez zmiany zgodności DGT/LTB.
3. Obliczać historię kroczącą dla każdej świecy.
4. Dodać prawdziwe HTF na podstawie agregowanych świec; przeskalowane okno bieżącego interwału pozostaje odrębnym trybem.
5. Rozdzielić szerokość kanału reszt od pasm krzywej SD/ATR.
6. Dodać zdarzenia przecięć i wybić z potwierdzaniem na zamknięciu świecy.
7. Porównywać wyniki C++ z Pine/TradingView na identycznych danych, interwale, źródle i parametrach.

**Kryteria akceptacji:**
- testy syntetyczne dla serii stałych, liniowych, zaszumionych i odstających;
- testy graniczne i wydajnościowe;
- porównanie z Pine Script z udokumentowanymi tolerancjami;
- brak look-ahead bias w historii kroczącej.

Wskaźnik Pine Script v6 pozostaje osobną implementacją TradingView do czasu weryfikacji w Pine Editor. Nie należy twierdzić, że został skompilowany, dopóki test ten nie zostanie wykonany.

### Etap 7 — portfel i transakcje

**Cel:** odtworzyć stan aktywów i wynik na dowolny dzień.

Zakres:
- posiadane aktywa, ilości, jednostki i lokalizacje;
- kupno, sprzedaż, wymiana, przewalutowanie;
- opłaty, prowizje i korekty z uzasadnieniem;
- ceny i waluty rozliczenia;
- koszt nabycia i jawnie wybrana metoda rozliczania;
- wycena bieżąca i historyczna;
- wynik zrealizowany i niezrealizowany;
- ekspozycja walutowa.

Historia transakcji powinna być niezmienna. Korekty zapisuje się jako osobne, audytowalne zdarzenia.

**Przygotowanie do innej aplikacji:** model transakcji i wersjonowany kontrakt API muszą być niezależne od interfejsu Monitora. W przyszłości inna aplikacja w odrębnym repozytorium będzie mogła wprowadzać kupno, sprzedaż i wymianę. Już teraz należy przewidzieć autoryzację klienta, zakresy uprawnień, idempotency keys i informację o źródle wpisu. Nie należy w tym etapie implementować tamtego klienta.

**Kryteria akceptacji:**
- stan portfela można odtworzyć na dowolny dzień;
- powtórzone żądanie nie dubluje transakcji;
- każda korekta i operacja ma źródło oraz ślad audytowy;
- wynik uwzględnia opłaty i walutę rozliczenia.

### Etap 8 — cztery stany regresji i prawdopodobieństwo wyniku

**Cel:** dostarczyć statystycznie weryfikowalne scenariusze, a nie pozorną pewność prognozy.

Macierz stanów:
- I: nachylenie dodatnie, cena powyżej regresji;
- II: nachylenie ujemne, cena powyżej regresji;
- III: nachylenie ujemne, cena poniżej regresji;
- IV: nachylenie dodatnie, cena poniżej regresji.

Nie jest to klasyczna interpretacja ćwiartek kartezjańskich, lecz macierz znaków nachylenia i znormalizowanego odchylenia ceny. Dla nachylenia równego zero, zerowej szerokości lub zbyt małej próby należy zwracać stan neutralny/brak klasyfikacji.

Prawdopodobieństwo wyniku należy szacować na podstawie historycznych analogii, oddzielnie dla instrumentu, profilu, okresu, horyzontu i kierunku transakcji. Raportować:
- liczbę analogicznych przypadków;
- prawdopodobieństwo dodatniego wyniku brutto i netto;
- oczekiwany wynik, medianę i rozrzut;
- ryzyko oraz maksymalne niekorzystne odchylenie;
- przedział niepewności;
- wersję modelu i datę kalibracji.

Uwzględnić spread, bid/ask, prowizje i inne koszty. Testy kroczące muszą wykluczać look-ahead bias i survivorship bias. Przy niewystarczającej próbie zwracać brak wiarygodnej estymacji.

**Kryteria akceptacji:**
- walidacja poza próbą;
- kalibracja prawdopodobieństw;
- koszty transakcyjne uwzględnione w wyniku netto;
- brak estymacji przy niewystarczającej próbie;
- pełna odtwarzalność prognozy.

### Etap 9 — serwer wymiany danych i API

**Cel:** zapewnić wspólny i kontrolowany dostęp klientów oraz integracji.

Zakres:
- serwer C++ HTTP/REST;
- wersjonowane API JSON i dokumentacja OpenAPI;
- WebSocket dla zdarzeń i aktualizacji;
- uwierzytelnianie, role i uprawnienia;
- idempotencja i walidacja żądań;
- audyt operacji;
- kontrola konfliktów i synchronizacji;
- dostęp do magazynu wyłącznie przez warstwę repozytoriów.

Minimalna lokalna usługa może powstać wcześniej, jeśli jest konieczna do zarządzania zapisem JSON. Pełny kontrakt API powinien być stabilizowany po ustaleniu modeli domenowych.

**Kryteria akceptacji:**
- klient odczytuje notowania, agregaty, regresję i portfel przez API;
- zapis jest walidowany i autoryzowany;
- testy obejmują błędy, równoczesne żądania i powtórzenia.

### Etap 10 — cztery interfejsy

| Klient | Preferowana technologia | Zakres |
|---|---|---|
| Linux GUI | Qt 6/C++ | pełna aplikacja desktopowa |
| WebUI | TypeScript | klient przeglądarkowy |
| TUI | C++/FTXUI lub ncurses | terminal i administracja |
| Android | Qt/C++ lub Kotlin/JNI | aplikacja mobilna |

Wszystkie klienty korzystają z tego samego API i modelu domenowego. Nie duplikują obliczeń finansowych. Kolejność implementacji poszczególnych klientów należy ustalić po stabilizacji API i wymagań użytkowych.

**Kryteria akceptacji:**
- zgodność danych i wyników pomiędzy klientami;
- wspólne reguły uprawnień i walidacji;
- testy połączenia, błędów sieci i odświeżania danych.

### Etap 11 — Nextcloud i ChatGPT/MCP

**Cel:** umożliwić kontrolowaną synchronizację plików i analizę AI.

Nextcloud:
- cienka aplikacja zgodna z publicznymi API platformy;
- WebDAV/Files API i OCS API;
- synchronizacja raportów, prognoz i dozwolonych plików JSON;
- uwierzytelnianie, wersjonowanie i rozwiązywanie konfliktów.

ChatGPT/MCP:
- uwierzytelniony serwer MCP;
- narzędzia odczytu oddzielone od zapisu;
- narzędzia obliczeniowe wywołujące deterministyczny silnik C++;
- zapis prognoz jako nowych, wersjonowanych rekordów;
- jawna zgoda na operacje zmieniające portfel;
- dziennik audytowy działań AI.

ChatGPT nie otrzymuje nieograniczonego dostępu do dysku tabletu. Dostęp lokalny wymaga kontrolowanego klienta/pośrednika albo synchronizacji przez Nextcloud. Nextcloud nie powinien stać się niekontrolowanym drugim źródłem prawdy.

**Kryteria akceptacji:**
- zakres danych ograniczony uprawnieniami;
- operacje zapisu rozdzielone od odczytu;
- prognozy zawierają model, dane wejściowe, czas, horyzont i wersję algorytmu;
- AI nie zastępuje deterministycznych obliczeń C++.

### Etap 12 — MariaDB i skalowanie

**Cel:** dodać opcjonalny magazyn dla większej skali.

Zakres:
- adapter MariaDB zgodny z tym samym kontraktem repozytorium;
- migracje, indeksy i transakcje;
- kopie zapasowe i odtwarzanie;
- testy zgodności semantycznej z JSON;
- testy obciążeniowe i współbieżności.

**Kryteria akceptacji:**
- zmiana JSON na MariaDB nie wymaga zmiany logiki domenowej ani klientów;
- oba magazyny przechodzą wspólny zestaw testów kontraktowych.

## 4. Zależności i ścieżka krytyczna

Główna kolejność:

```text
Etap 0: audyt repozytorium
  -> Etap 1: model danych
  -> Etap 2: adaptery źródeł
  -> Etap 3: JSON
  -> Etap 4: agregacja
  -> Etap 5: wykresy
  -> Etap 6: regresja krocząca
  -> Etap 7: portfel i transakcje
  -> Etap 8: prawdopodobieństwo i walidacja
  -> Etap 9: API
  -> Etap 10: klienci
  -> Etap 11: Nextcloud/MCP
  -> Etap 12: MariaDB i skalowanie
```

Niektóre prace mogą być prowadzone równolegle po ustaleniu kontraktów, np. przygotowanie wykresów po stabilizacji modelu agregatów. Nie należy jednak implementować niezależnych modeli danych w każdym kliencie.

## 5. Ryzyka i wymagane zabezpieczenia

| Ryzyko | Skutek | Zabezpieczenie |
|---|---|---|
| Rozbieżne modele dostawców | błędne porównania | wspólny model, jawne jednostki i rodzaje cen |
| Dane miesięczne traktowane jako dzienne | fałszywa precyzja | zachowanie częstotliwości źródłowej |
| Współbieżny zapis JSON | utrata lub uszkodzenie danych | zapis atomowy, blokady, wersje |
| Niewłaściwe indeksowanie regresji | błędne kanały | testy syntetyczne i porównanie Pine |
| Look-ahead bias | zawyżone wyniki historyczne | testy kroczące poza próbą |
| Kurs referencyjny traktowany jako transakcyjny | błędna wycena | rozdzielenie mid, bid/ask i ceny transakcji |
| Dwa źródła prawdy | konflikty synchronizacji | jawne źródło autorytatywne i wersjonowanie |
| Nadmierne uprawnienia AI | nieautoryzowane zmiany | MCP z minimalnymi uprawnieniami i potwierdzeniem |

## 6. Standard ukończenia każdego etapu

Etap można oznaczyć jako ukończony dopiero, gdy:
- implementacja znajduje się w repozytorium i jest włączona do właściwego procesu budowania;
- testy jednostkowe i integracyjne przechodzą;
- przypadki błędne, puste i graniczne są obsłużone;
- kompilacja Linux i CTest przechodzą;
- GitHub Actions przechodzi;
- aktywny proces budowania/testowania/uruchamiania nie wymaga Pythona;
- dokumentacja odpowiada rzeczywistemu kodowi;
- ograniczenia i pochodzenie danych są jawne;
- zmiany są zweryfikowane na gałęzi docelowej.

Nie deklarować kompilacji, testów, wdrożenia ani zgodności z TradingView bez rzeczywistego wyniku odpowiedniej kontroli.

## 7. Najbliższy zakres prac

Najbliższe zadanie to **Etap 0**. Należy wykonać przegląd drzewa repozytorium i aktywnych modułów, rozstrzygnąć rozbieżność dotyczącą adapterów i magazynu JSON, zweryfikować CMake/CI/testy oraz zaktualizować dokumentację do stanu faktycznego.

Po zamknięciu Etapu 0 rozpocząć Etap 1 — wspólny model danych rynkowych — a następnie adaptery i JSON. Nie rozpoczynać jeszcze pełnej implementacji czterech interfejsów, Nextcloud/MCP, estymacji prawdopodobieństwa ani MariaDB.

## 8. Dokumenty powiązane

- [Audyt funkcjonalny i plan realizacji](market-monitor-audit-and-roadmap.md)
- [Architektura systemu](architecture.md)
- [Dostawcy danych](open-data-providers.md)
- [Audyt DGT](dgt-linear-regression-audit.md)
- [Audyt LonesomeTheBlue](lonesometheblue-linear-regression-audit.md)
- [Wskaźnik TradingView](../tradingview/README.md)
- [Rdzeń natywny](../native/README.md)


## 9. Wyniki wykonania Etapu 0 — audyt stanu faktycznego (2026-11-12)

### 9.1. Stan repozytorium i zarządzanie zmianami

- Repozytorium publiczne: `bartoszruta26-droid/Monitor`; gałąź domyślna: `main`.
- Drzewo `main` zawiera wyłącznie gałąź aplikacyjną `main` (brak dodatkowych gałęzi do odzyskania).
- W chwili audytu wyszukiwanie GitHub nie zwróciło otwartych PR ani otwartych Issues.
- Drzewo zawiera jeden workflow CI: `.github/workflows/backend-tests.yml`, nazwany „Native C++ checks”.
- Nie ma katalogów ani implementacji czterech klientów, serwera C++ ani docelowych adapterów C++.

### 9.2. Status funkcjonalny — stan rzeczywisty

| Obszar | Status | Uzasadnienie |
|---|---|---|
| Rdzeń regresji C++20 | **Gotowe — zakres podstawowy** | `native/src/regression.cpp`, publiczny nagłówek, CLI i testy są w CMake. |
| CMake i skrypty Bash | **Gotowe — zakres podstawowy** | `native/CMakeLists.txt`, `scripts/build-native.sh`, `scripts/test-native.sh`. |
| GitHub Actions Linux | **Gotowe — zakres podstawowy** | Workflow instaluje CMake/Ninja/G++, konfiguruje, buduje i uruchamia CTest. |
| Adapter NBP | **Częściowe — tylko prototyp Python** | `backend/market_data/open_providers.py` i `adapters.py`; brak adaptera C++ w CMake. |
| Adapter Frankfurter | **Częściowe — tylko prototyp Python** | Import historyczny przez FastAPI; brak adaptera C++ i aktywnego testu integracyjnego. |
| Adapter World Bank Pink Sheet | **Częściowe — tylko prototyp Python** | Parser XLSX i import historyczny; brak adaptera C++ i aktywnego CI dla tego kodu. |
| API `/providers/*` | **Częściowe — tylko prototyp Python** | Trasy w `providers_api.py` są dołączane do FastAPI w `main.py`; nie są częścią aplikacji natywnej. |
| Magazyn SQLite/PostgreSQL | **Częściowe — tylko prototyp Python** | `storage.py` implementuje te dwa warianty; nie jest docelowym magazynem. |
| Magazyn JSON | **Brak** | Nie znaleziono implementacji repozytorium JSON ani testów trwałego zapisu/odczytu. |
| Adapter MariaDB | **Brak** | Nie znaleziono implementacji. |
| Wspólny model instrumentów i notowań C++ | **Brak** | Brak modelu domenowego poza strukturami wyniku regresji. |
| Agregacja okresowa i wykresy | **Brak** | Brak implementacji. |
| Rolling regression i rzeczywiste HTF | **Brak** | Rdzeń liczy jedną serię przekazaną przez wywołującego. |
| Wariant błędu standardowego `n-2` | **Brak** | `WidthMethod` ma wyłącznie `PopulationRms` (`n`) i `SampleSd` (`n-1`). |
| Portfel, transakcje i macierz stanów | **Brak** | Brak modeli i modułów. |
| Serwer C++ REST/JSON/WebSocket | **Brak** | Brak implementacji i konfiguracji budowania. |
| Linux GUI, WebUI, TUI, Android | **Brak** | Brak klientów. |
| Nextcloud i MCP | **Brak** | Brak integracji. |
| Pine Script | **Częściowe** | Plik v6 jest w repozytorium; brak potwierdzenia kompilacji w Pine Editor i porównania wyników z C++. |

### 9.3. Python — rozdzielenie prototypu od aktywnego procesu

W repozytorium pozostał kod Python w `backend/`, w tym FastAPI, adaptery, SQLite/PostgreSQL i pytest. `backend/README.md` określa go jako historyczny prototyp. Aktualny workflow CI nie instaluje Pythona ani zależności Python i nie uruchamia pytest; buduje wyłącznie C++20 przez CMake i CTest. Skrypty `scripts/build-native.sh` i `scripts/test-native.sh` również uruchamiają wyłącznie CMake, kompilator C++ i CTest. W ramach audytu nie usuwano prototypu ani jego testów.

Wniosek: Python nie jest zależnością aktywnego procesu budowania i testowania rdzenia C++20. Historyczny prototyp może pozostać, ale jego endpointów i adapterów nie należy przedstawiać jako funkcji aktywnej aplikacji.

### 9.4. Kompilacja i testy — dowód CI

Dla aktualnego `main` (`ea8f80538e65c0849877ea1aec896eb655b1e7f0`) GitHub Actions uruchomił przebieg **43244544593**, zakończony statusem `success`. Wszystkie kroki `Checkout`, instalacja narzędzi, `Configure`, `Build` i `Test` zakończyły się powodzeniem. Jest to potwierdzenie kompilacji i CTest w czystym środowisku `ubuntu-latest` przez CI. Nie wykonywano osobnej lokalnej kompilacji w tej sesji.

Przebieg historyczny 43237592885 zakończył się niepowodzeniem na wcześniejszym commicie; późniejsze przebiegi, w tym aktualny, są zakończone powodzeniem. Za stan bieżący należy uznawać wynik najnowszego przebiegu, nie historyczną porażkę.

- [Aktualny przebieg CI](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43244544593)
- [Commit aktualnego `main`](https://github.com/bartoszruta26-droid/Monitor/commit/ea8f80538e65c0849877ea1aec896eb655b1e7f0)

### 9.5. Rozstrzygnięcie rozbieżności dokumentacji

`docs/open-data-providers.md` opisuje istniejący kod, ale pomija kluczowy kontekst: jest to prototyp Python, a nie aktywna implementacja C++20. Ponadto magazyn prototypu obsługuje SQLite i PostgreSQL, a nie docelowe JSON i MariaDB. Dokument został uzupełniony o ostrzeżenie statusowe, a `backend/README.md` o potwierdzony zakres prototypu. Nie usuwano istniejących danych ani kodu.

### 9.6. Wnioski techniczne

1. Nie rozpoczynać przenoszenia adapterów przed zdefiniowaniem wspólnego modelu danych i jednostek.
2. Etap 1 powinien dostarczyć typy C++ dla instrumentu, notowania, źródła, jednostki, waluty, częstotliwości, rodzaju ceny, czasu obserwacji i metadanych jakości.
3. W Etapie 2 przenieść adaptery do C++20, korzystając z prototypu Python wyłącznie jako materiału referencyjnego. Nie przenosić automatycznie jego założeń magazynowych.
4. W Etapie 3 zaimplementować JSON jako domyślny magazyn. SQLite/PostgreSQL z prototypu nie zastępują wymogu JSON/MariaDB.
5. Dodać `n-2` jako osobny wariant statystyczny w Etapie 6; nie zmieniać profili DGT (`n-1`) ani LTB (`n`).
6. Rozszerzyć CI o testy kontraktowe i integracyjne dla kolejnych modułów C++, gdy zostaną włączone do CMake. Nie uruchamiać historycznego pytest jako części aktywnego procesu.

### 9.7. Decyzja o zamknięciu Etapu 0

**Etap 0: ukończony w zakresie audytu repozytorium i potwierdzenia CI.** Rozbieżność dokumentacji została rozstrzygnięta, a statusy funkcji zapisane powyżej. Ograniczenie: kompilacja i testy zostały potwierdzone przez GitHub Actions, bez osobnego uruchomienia lokalnego w tej sesji. Etap 0 nie oznacza, że adaptery lub magazyn są gotowe — przeciwnie, ich aktywna implementacja pozostaje do wykonania.

**Następny etap: Etap 1 — wspólny model danych rynkowych w C++20.**


## 10. Etap 1 — wspólny model danych rynkowych (implementacja v1)

### Zakres dostarczony
- Typy C++20 dla instrumentów walutowych, metali szlachetnych i surowców.
- Jednostki walutowe, masy, tony metrycznej, baryłki i MWh; jawna konwersja gram/kg/uncja trojańska.
- Waluty bazowe i kwotowane dla par FX oraz waluta wyceny dla metali/surowców.
- Rodzaje ceny: referencyjna, średnia, bid, ask, mid, spot i transakcyjna.
- Znacznik UTC, normalizacja ISO-8601 z offsetem, czas obserwacji, czas pobrania i lokalna data publikacji dostawcy.
- Częstotliwość, status okresu, kompletność szeregu i status jakości.
- Identyfikator dostawcy, identyfikator rekordu źródłowego, adres źródła i informacja licencyjna.
- Walidacja identyfikatorów, kodów walut, jednostek, chronologii, wartości liczbowych, pochodzenia i wersji schematu.
- Jawne schema_version = 1 w rekordzie notowania.
- Testy C++ dla przypadków poprawnych, błędnych, granicznych, konwersji jednostek i czasu.

### Pliki
- native/include/monitor/market_data.hpp
- native/src/market_data.cpp
- native/tests/test_market_data.cpp
- docs/market-data-model.md

### Decyzje i ograniczenia
- Model nie zależy od sieci, JSON, SQLite, MariaDB ani interfejsu.
- Czas przechowywany jest jako sekundy Unix UTC; parser przyjmuje jawne Z lub ±HH:MM. Ułamki sekund są w wersji 1 obcinane.
- Kurs FX oznacza jednostki waluty kwotowanej za jedną jednostkę waluty bazowej.
- Konwersja automatyczna dotyczy wyłącznie masy: 1 troy oz = 31,1034768 g; 1 kg = 1000 g.
- Brak obserwacji pozostaje luką, nie zerową ceną. Częstotliwość miesięczna nie jest zamieniana na dzienną.
- Model nie implementuje jeszcze adapterów, magazynu, agregacji, świec, portfela ani API.

### Kryteria ukończenia
**Etap 1: ukończony.** Model C++20 został włączony do biblioteki monitor_core i CMake. GitHub Actions na commicie e31c62b50840202264374bc1a590f71a9454146f zakończył się powodzeniem: konfiguracja, kompilacja i oba zestawy CTest przeszły; 2/2 testy zaliczone. [Przebieg CI](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43248161715). Nie wykonano osobnej kompilacji lokalnej w tej sesji; potwierdzeniem jest czyste środowisko GitHub Actions.


### 10.1. Wynik weryfikacji i zamknięcie Etapu 1
- Pierwszy przebieg CI wykrył konflikt nazwy zmiennej z typem std::chrono::day. Błąd został poprawiony przed dalszym rozwojem.
- Dodano walidację nieznanych wartości enum dla klasy aktywa, jednostki, częstotliwości, rodzaju ceny, statusu okresu, jakości i kompletności.
- Dodano testy odrzucania nieznanych wartości enum oraz kontrolę dopuszczania ujemnej ceny wyłącznie dla instrumentu z odpowiednią flagą.
- Commit implementacji i walidacji: e31c62b50840202264374bc1a590f71a9454146f.
- CI: sukces; konfiguracja, budowanie i CTest zakończone powodzeniem; 2/2 testy.
- Etap 1 zamknięty. Następny etap to Etap 2 — adaptery dostawców C++20, zgodne z modelem danych v1. Magazyn JSON pozostaje późniejszym Etapem 3.


## 11. Etap 2 — adaptery dostawców C++20 (realizacja)

### Zakres implementacji
- Wspólny kontrakt ProviderAdapter oraz AdapterResult.
- Abstrakcja HttpTransport i produkcyjny transport CurlHttpTransport.
- HttpClient z listą dozwolonych hostów HTTPS, limitami rozmiaru, timeoutami, minimalnym odstępem żądań i ponawianiem błędów przejściowych.
- Adapter NBP: tabele A/B/C i złoto PLN/g.
- Adapter Frankfurter v2 z możliwością przypięcia dostawcy lub użycia blendu.
- Adapter World Bank Pink Sheet: dynamiczne odnajdywanie miesięcznego XLSX i parser arkusza Monthly Prices.
- Adapter XAUS dla orientacyjnych cen spot XAU/XAG.
- Deduplikacja notowań oraz walidacja wyników wspólnym modelem.
- Testy z atrapami HTTP i wzorcowymi odpowiedziami, bez dostępu do Internetu.

### Pliki
- native/include/monitor/market_data_adapters.hpp
- native/src/market_data_adapters.cpp
- native/tests/test_market_data_adapters.cpp
- docs/cpp-provider-adapters.md

### Zależności
libcurl, nlohmann-json, libarchive i libxml2; zależności instalowane w workflow Linux. Aktywny proces pozostaje C++20/CMake/CTest, bez Pythona.

### Semantyka i ograniczenia
- NBP A/B są Average; NBP C emituje osobne Bid i Ask, bez sztucznego midpointu.
- Tabela C zachowuje tradingDate i effectiveDate oddzielnie.
- Frankfurter jest Mid z flagą źródła zbiorczego lub przypiętego.
- Pink Sheet zachowuje częstotliwość miesięczną i USD/uncję trojańską.
- XAUS jest orientacyjnym mid-market spot, nie ceną transakcyjną; stan stale oznaczany jako podejrzany.
- Brak zapisu JSON — ten pozostaje Etapem 3.

### Kryteria ukończenia
Etap 2 można zamknąć dopiero po kompilacji w CI, przejściu testów adapterów i dotychczasowych testów rdzenia oraz kontroli dokumentacji. Rzeczywiste testy sieciowe są osobne i nie są warunkiem testów jednostkowych.


### 11.1. Wynik weryfikacji i zamknięcie Etapu 2
- Wspólny kontrakt adaptera, abstrakcja transportu i produkcyjny transport libcurl są włączone do biblioteki monitor_core.
- Adapter NBP obsługuje tabele A/B/C oraz złoto PLN/g. A/B są zachowane jako Average; C jako osobne Bid/Ask, z rozdzieleniem tradingDate i effectiveDate.
- Adapter Frankfurter v2 obsługuje serie historyczne oraz opcjonalne przypięcie dostawcy.
- Adapter World Bank Pink Sheet odczytuje miesięczny XLSX w C++ (libarchive + libxml2) i zachowuje częstotliwość Monthly.
- Adapter XAUS obsługuje aktualne orientacyjne notowania spot XAU/XAG, dokładny czas obserwacji oraz kontrolę stanu fresh/stale.
- HttpClient ogranicza hosty i protokół do HTTPS, rozmiar odpowiedzi, czas żądania, tempo zapytań oraz ponawia błędy przejściowe i 429/wybrane 5xx.
- Testy korzystają z atrap transportu i wzorcowych odpowiedzi; nie wykonują połączeń sieciowych.
- Parser XLSX indeksuje elementy archiwum, ogranicza liczbę elementów i rozmiar danych po rozpakowaniu.
- Commit aktualnej implementacji: 7f8bed02ab27a8255f8c2b4e56c8a27319bc57ce.
- GitHub Actions: sukces; konfiguracja CMake, kompilacja i 3/3 testy CTest zakończone powodzeniem. [Przebieg CI](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43250128831).
- Nie wykonano testu pobierania z rzeczywistymi usługami zewnętrznymi w tej sesji. Testy adapterów są deterministyczne i offline.
- Etap 2 zamknięty. Następny etap to Etap 3 — magazyn JSON, wersjonowanie dokumentów, zapis atomowy, blokady, deduplikacja i kopie bezpieczeństwa.


## 12. Etap 3 — magazyn JSON C++20 (implementacja i weryfikacja)

### Zakres dostarczony w kodzie
- Wspólny, niezależny od silnika magazynu kontrakt MarketDataRepository.
- Implementacja JsonMarketDataRepository oraz typy MarketDataSnapshot i MarketDataBatch.
- Wersjonowany dokument monitor.market_data (schema_version = 1), rewizja dokumentu i updated_at.
- Serializacja/deserializacja instrumentów, dostawców, notowań, metadanych szeregów i historii korekt.
- Walidacja referencji, identyfikatorów, unikalności kluczy, enumów, czasu i kompletności szeregu.
- Zapis atomowy przez plik tymczasowy, fsync, rename i fsync katalogu.
- Blokada międzyprocesowa flock obejmująca operacje odczyt-modyfikacja-zapis.
- Kopia .bak poprzedniej poprawnej rewizji i jawne restore_backup(); uszkodzony dokument nie jest automatycznie maskowany.
- Deduplikacja kluczem (instrument, provider, observed_at, price_kind, frequency), aktualizacja received_at dla ponownego pobrania oraz zachowanie poprzedniego payloadu w observation_history przy korekcie.
- upsert_batch() przyjmuje instrumenty, dostawców, notowania i metadane jednym zatwierdzeniem; odrzucenie nieprawidłowej partii nie zmienia pliku.
- Testy obejmują round-trip, korekty, backup, odzyskiwanie, nieznaną wersję dokumentu, pusty uszkodzony plik, współbieżnych piszących i powtórne przyjęcie partii.

### Pliki
- native/include/monitor/market_data_repository.hpp
- native/src/market_data_repository.cpp
- native/tests/test_market_data_repository.cpp
- docs/json-market-data-repository.md

### Ograniczenia
- Pierwszy adapter plikowy korzysta z POSIX flock/fsync/rename i jest przeznaczony dla Linux/POSIX.
- Nie ma jeszcze migracji starych formatów; wersja nieznana jest odrzucana. Migracje będą dodawane jawnie z fixture i testami.
- Magazyn JSON używa jednej migawki dla prostoty atomowości; partycjonowanie wymaga pomiarów i nie należy go wprowadzać przed profilowaniem.
- Historyczny Python storage SQLite/PostgreSQL pozostaje poza aktywnym procesem C++20.
- MariaDB i serwer wymiany danych nie są jeszcze zaimplementowane.

### 12.1. Wynik weryfikacji i zamknięcie Etapu 3
- Poprawiono błędy wykryte w pierwszych przebiegach CI: typy iteratorów, jawne porównanie CivilDate, czasy received_at w fixture oraz zapis końcowego znaku nowej linii podczas odtwarzania kopii.
- Końcowy kod magazynu i testów znajduje się na main; commit kodu: 520cea16677a92cafdc5e86a6754d10c6577709f.
- GitHub Actions: sukces; konfiguracja CMake, kompilacja C++20 i wszystkie cztery zestawy CTest zakończone powodzeniem (4/4). [Końcowy przebieg CI](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43253237765).
- Testy magazynu potwierdzają round-trip, rewizje, deduplikację, historię korekt, backup, jawne odtworzenie z podbiciem rewizji, odrzucanie pustego i nieznanego dokumentu, atomowe przyjęcie partii oraz równoległe zapisy procesów.
- Etap 3 zamknięty w zakresie magazynu JSON dla modelu danych rynkowych.
- Ograniczenia: implementacja POSIX/Linux; brak jeszcze adaptera MariaDB, serwera wymiany danych, migracji starszych wersji dokumentu i testu wieloplatformowego.
- Historyczny backend Python SQLite/PostgreSQL pozostaje poza aktywnym C++20 build/test path.

**Następny etap: Etap 4 — agregacja i normalizacja szeregów danych oraz budowa świec OHLC z zachowaniem częstotliwości źródłowej i brakujących okresów.**


## 13. Etap 4 — agregacja i świece próbkowane OHLC

- Moduł `market_data_aggregation` jest częścią `monitor_core`.
- Obsługuje okresy minutowe, godzinowe, dzienne, tygodniowe, miesięczne, kwartalne, półroczne i roczne; nie zwiększa częstotliwości źródłowej.
- Granice okresów są wyznaczane w UTC; tydzień rozpoczyna się w poniedziałek.
- Zwraca jawne luki, statusy jakości, flagi i identyfikatory obserwacji źródłowych.
- Świece mają semantykę `SampledObservations`; `has_intraperiod_range` jest prawdziwe dopiero przy co najmniej dwóch próbkach. To nie jest równoważne natywnym OHLC dostawcy.
- Wolumen nie jest szacowany; dane miesięczne nie są rozszerzane do dziennych.
- Testy obejmują granice okresów, rok przestępny, luki, deterministyczność, izolację szeregów i zakaz upsamplingu.

## 14. Etap 5 — wspólny model danych wykresów

- Moduł `market_data_chart` udostępnia model serii liniowej, obszarowej i świecowej.
- Zachowuje UTC, wartości, jakość, status okresu, identyfikatory źródeł i jawne luki.
- Wykres świecowy przyjmuje wynik `aggregate_ohlc`, bez drugiej implementacji agregacji.
- Dane są sortowane deterministycznie; serie mieszane są odrzucane.
- Limity punktów i świec ograniczają zużycie pamięci.
- Moduł nie zawiera renderera ani zależności GUI.
- Audyt wykazał, że walidacja świec nie weryfikowała wszystkich identyfikatorów źródłowych ani treści luk. Poprawiono to przez ponowne wyliczenie agregacji z wejścia i porównanie całego wyniku.
- Testy wykrywają zmienione identyfikatory źródeł, sfałszowane luki, nieznany rodzaj wykresu i niezgodność świec.

## 15. Etap 6 — regresja krocząca i wariant n-2

- Dodano `ResidualStandardErrorNMinus2` jako trzeci, odrębny wariant szerokości: `sqrt(SSE / (n - 2))`.
- `SampleSd` (`n-1`, DGT) i `PopulationRms` (`n`, LonesomeTheBlue) zachowują dotychczasowe równania.
- Dodano `rolling_linear_regression`, przesuwającą pełne okno o jedną obserwację i zwracającą indeksy okien.
- Nieznane wartości enum, niepoprawne rozmiary okien i wyniki poza zakresem `double` są odrzucane.
- Wersja 1 operuje na ciągłym segmencie wartości. Segmentację przy brakach oraz kontrolę rzeczywistych kalendarzowych interwałów HTF należy wykonać przed podaniem danych do regresji; okno nie może przekraczać luki.
- Metoda n-2 nie jest automatycznie przedziałem ufności ani prognozy.

## 16. Etap 7 — szeregi czasowe HTF (implementacja v1)

- Dodano `market_data_regression` jako adapter między obserwacjami, agregacją OHLC i regresją.
- Obsługiwane są jawne polityki `FirstObservation` i `LastObservation`; domyślna jest ostatnia rzeczywista obserwacja okresu.
- Każdy wynik zachowuje kalendarzowe granice okna UTC, rzeczywisty czas ostatniej próbki, wartość oraz identyfikatory wszystkich źródłowych obserwacji w oknie.
- Brak całego okresu HTF rozdziela segmenty; okna regresji nie przekraczają luki.
- Zachowano istniejące profile `PopulationRms`, `SampleSd` i `ResidualStandardErrorNMinus2` bez zmian.
- Dodano testy granic ISO tygodnia, lutego przestępnego, granicy roku/miesiąca, luk, duplikatów czasu, polityk wyboru i zakazu upsamplingu.

### Ograniczenia v1
- Segmentacja odbywa się na poziomie całych okresów docelowych, nie sesji giełdowych wewnątrz okresu.
- Nie ma kalendarza świąt, godzin handlu ani reguł instrument-specyficznych.
- Tick i Irregular pozostają nieobsługiwane.
- Weryfikacja porównawcza z Pine/TradingView i podłączenie do wykresów pozostają kolejnymi zadaniami.

### Kryteria dalszego etapu
1. Dodać kalendarze sesyjne i kontrolę kompletności LTF w okresie HTF.
2. Rozszerzyć testy o dzienne, kwartalne, półroczne i roczne okresy oraz testy graniczne dla instrumentów o różnych kalendarzach.
3. Przygotować fixture porównawcze C++/Pine na identycznych danych i parametrach.
4. Dopiero po tych testach udostępnić wspólny kontrakt obliczeń wykresom i klientom GUI.


## 17. Audyt kontrolny i Etap 8 — kalendarz sesyjny oraz kompletność HTF

### Wynik ponownego przeglądu repozytorium (2026-11-12)
- Gałąź main wskazuje na commit 8b7264e08fe946824304bea190dd76322166c8be; ostatni CI dla tego SHA zakończył się powodzeniem (7/7).
- Brak dodatkowych gałęzi, otwartych PR i Issues. Nie ma niezałączonych zmian do scalenia.
- CMake buduje aktywny rdzeń C++20; katalogi serwera i czterech klientów nadal nie istnieją.
- Poprzedni etap HTF rozpoznawał brak całego okresu kalendarzowego, ale nie rozróżniał brakującej sesji od dnia wolnego.

### Etap 8 — jawny kalendarz sesyjny (v1)
- Dodano MarketSessionCalendar z identyfikatorem, instrumentem, dostawcą, częstotliwością źródłową i oczekiwanymi znacznikami UTC.
- Dodano session_aware_rolling_regression. Okres HTF jest używany wyłącznie wtedy, gdy wszystkie zaplanowane sloty istnieją, mają status Complete i jakość Validated.
- Weekend i święto nie tworzą luki, jeśli nie mają oczekiwanych slotów w harmonogramie.
- Brak slotu lub obserwacja Suspect, Unreviewed, Partial albo Provisional przerywa segment.
- Obserwacje spoza harmonogramu, pomieszane szeregi, powtórzone czasy i nieuporządkowany kalendarz są odrzucane.
- Kalendarz dostarcza terminy UTC po rozstrzygnięciu strefy czasowej, DST i wyjątków. Generator reguł, pobieranie świąt i gotowe kalendarze instrumentów pozostają do wykonania.
- Testy obejmują weekendy, święto pominięte w harmonogramie, brak oczekiwanej sesji, podejrzaną jakość, obserwację poza kalendarzem i błędną kolejność terminów.

### Kolejny etap
1. Dodać wersjonowany magazyn kalendarzy i import kalendarzy sesyjnych z plików/adapterów.
2. Zapewnić generator terminów UTC z jawnie nazwanej strefy IANA i reguł sesji.
3. Dodać odrębne kalendarze dla NBP, metali spot i instrumentów giełdowych; nie stosować jednego uniwersalnego kalendarza.
4. Zdefiniować jawne polityki jakości i ewentualne tolerancje brakujących slotów.
5. Przygotować fixture wspólne dla C++ i Pine oraz niezależnie skompilować Pine w TradingView.


## 18. Rejestr wykonania i korekta statusu kalendarzy (2026-11-12)

Bieżący, nadrzędny rejestr etapów znajduje się w [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md). Rejestr wiąże każdy etap z kodem, commitem i CI.

**Ważna korekta:** Etap 8 (kalendarz sesyjny) jest zaimplementowany jako funkcja obliczeniowa `session_aware_rolling_regression()`, ale trwały magazyn i wersjonowanie kalendarzy nie zostały jeszcze wdrożone. Poprzednia odpowiedź opisała ten zakres jako wykonany przed sprawdzeniem aktualnych plików repozytorium. Audyt HEAD `d2bc1364986bee6f4474bfce9ad2549e60c90f97` potwierdził brak kalendarzy w `MarketDataSnapshot`, `MarketDataBatch` i `JsonMarketDataRepository`. Magazyn wersji jest następnym etapem, a nie funkcją gotową.

Ostatni CI dla HEAD `d2bc1364986bee6f4474bfce9ad2549e60c90f97` zakończył się powodzeniem: [workflow 43260529082](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43260529082).


## 20. Etap 9 — wynik końcowy i etap 10

Etap 9 potwierdzony: commit `df867c6efaa7f8cebad74db3291a3c0dde12f67e`, CI 43264555372 — sukces, kompilacja i testy zakończone powodzeniem. Wersje kalendarzy są trwałe, niezmienne i identyfikowane w wynikach regresji.

Etap 10 rozpoczęty: generator terminów z IANA TZ/DST w C++20, wykorzystujący ICU. Polityki czasu nieistniejącego i powtórzonego są jawne; domyślnie oba przypadki są odrzucane. Zakres dat, dni tygodnia i wyłączenia są jawnie podawane. Etap oczekuje na CI.


## 21. Etap 10 — pierwsza weryfikacja

Commit `cedee367f5528abef5d956b9f560167a5b86aaf0`: konfiguracja i kompilacja C++20/ICU przeszły. CTest 7/8; niepowodzenie wynikało z błędnego zakresu dat fixture: `end_date_exclusive` ustawiono na 3 kwietnia, więc generator prawidłowo nie uwzględnił 3 kwietnia. Zakres testu poprawiono na 4 kwietnia. Usunięto również nieużywaną funkcję pomocniczą.


## 22. Etap 10 — diagnoza i korekta integracji ICU (2026-11-12)

CI `43265958349` dla commitu `6845c071890a762fb7e9fe00fef220a31134af7e` potwierdził konfigurację i kompilację, ale test generatora zakończył się wyjątkiem ICU `U_ILLEGAL_ARGUMENT_ERROR`. Analiza logu wskazała konflikt pomiędzy `GregorianCalendar::setLenient(false)` a jawną obsługą `UCAL_WALLTIME_NEXT_VALID`. Poprawka ustawia kalendarz w trybie lenient, aby ICU mogło zastosować reguły wall-time. Odrzucanie nieistniejącego czasu dla polityki `Reject` nadal jest jawne: po konwersji porównywane są wszystkie składowe lokalnej daty i czasu z żądaniem; każda normalizacja jest odrzucana. `NextValidTime` zachowuje zachowanie przesunięcia do pierwszej poprawnej chwili. Etap 10 pozostaje w toku do potwierdzenia pełnego CI dla nowego SHA.
