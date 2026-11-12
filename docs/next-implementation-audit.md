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
