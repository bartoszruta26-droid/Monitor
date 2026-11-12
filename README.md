<div align="center">

# MONITOR RYNKÓW

### Wieloplatformowa platforma danych rynkowych, analizy i zarządzania portfelem

**C++20 · Bash · Linux GUI · WebUI · TUI · Android · JSON · MariaDB**

</div>

---

## 1. Cel i zakres systemu

Monitor Rynków jest jedną platformą, która gromadzi, wymienia, przechowuje, analizuje i prezentuje dane rynkowe oraz dane portfela. Obsługuje waluty, złoto, srebro, inne metale i aktywa wyceniane indywidualnie.

System ma cztery równorzędne interfejsy użytkownika:

1. **Linux GUI** — pełna aplikacja graficzna dla systemów Linux;
2. **WebUI** — interfejs dostępny przez przeglądarkę;
3. **TUI** — interfejs terminalowy;
4. **Android App** — aplikacja mobilna.

Wszystkie interfejsy korzystają z tego samego modelu danych, reguł biznesowych i silnika analitycznego. Wymianę danych pomiędzy usługami, adapterami źródeł i klientami zapewnia odrębny **serwer wymiany danych**.

## 2. Architektura docelowa

System składa się z sześciu logicznych warstw:

| Warstwa | Odpowiedzialność | Docelowa technologia |
|---|---|---|
| Wspólny rdzeń | modele danych, walidacja, obliczenia, reguły biznesowe | C++20 |
| Serwer wymiany danych | komunikacja między backendami i klientami, API, synchronizacja, autoryzacja | C++20, HTTP/REST, JSON, WebSocket |
| Adaptery backendów | pobieranie i wysyłanie danych do dostawców oraz usług zewnętrznych | C++20, libcurl |
| Magazyn trwały | zapis danych i konfiguracji | JSON jako tryb domyślny; MariaDB jako opcja |
| Interfejsy | prezentacja, sterowanie i obsługa użytkownika | Qt 6/C++, WebUI, C++ TUI, Android |
| Narzędzia rozwojowe | konfiguracja kompilacji, testy, pakowanie i administracja | CMake, Bash, GitHub Actions |

### 2.1. Serwer wymiany danych — centralny element komunikacji

Serwer wymiany danych jest niezależną usługą. Nie jest tym samym co magazyn danych i nie może być utożsamiany z jednym z adapterów dostawców.

Jego zadania:

- udostępnianie wspólnego API dla Linux GUI, WebUI, TUI i Android App;
- wymiana danych pomiędzy wieloma backendami i adapterami;
- normalizacja komunikatów do wspólnego modelu danych;
- obsługa importu, eksportu, synchronizacji i kontroli wersji danych;
- przekazywanie zdarzeń i aktualizacji klientom;
- uwierzytelnianie klientów i backendów oraz kontrola uprawnień;
- rejestrowanie operacji, błędów i pochodzenia danych;
- kontrola konfliktów przy równoczesnej modyfikacji danych.

Podstawowym interfejsem komunikacji będzie HTTP/REST z komunikatami JSON. WebSocket będzie używany tam, gdzie potrzebne są aktualizacje w czasie rzeczywistym. Kontrakt API powinien być wersjonowany i opisany w OpenAPI.

Serwer ma obsługiwać wielu dostawców i wiele instancji backendów. Każdy backend komunikuje się przez jawnie zdefiniowany adapter i nie zapisuje bezpośrednio do magazynu należącego do innego modułu. Zapis przechodzi przez warstwę usług i repozytoriów serwera.

### 2.2. Magazyn danych — JSON domyślnie, MariaDB opcjonalnie

Aplikacja ma obsługiwać dwa wymienne warianty trwałego zapisu:

**Tryb domyślny: pliki JSON**
- działa bez instalowania i konfigurowania serwera bazy danych;
- przechowuje dane w katalogu wskazanym w konfiguracji;
- wykorzystuje wersjonowany format dokumentów;
- powinien zapewniać zapis atomowy, kopie bezpieczeństwa, walidację i odzyskiwanie po przerwanym zapisie;
- powinien obsługiwać blokady lub kontrolę wersji przy równoczesnym dostępie.

**Tryb opcjonalny: MariaDB**
- wybierany przez konfigurację;
- przeznaczony do większej liczby użytkowników, backendów i równoległych zapisów;
- korzysta z migracji schematu i transakcji;
- połączenia i dane dostępowe są przechowywane poza repozytorium, najlepiej w zmiennych środowiskowych lub bezpiecznym magazynie sekretów.

Oba warianty muszą implementować ten sam interfejs repozytorium i zapewniać równoważne operacje domenowe. Zmiana magazynu nie może wymagać modyfikacji kodu interfejsów ani silnika analitycznego.

Nie zakłada się SQLite ani PostgreSQL jako domyślnych magazynów. Mogą zostać dodane w przyszłości jako osobne adaptery, jeśli pojawi się uzasadniona potrzeba.

### 2.3. Wspólny rdzeń C++

Rdzeń C++20 jest niezależny od interfejsu, serwera i konkretnego magazynu. Obejmuje:
- modele instrumentów, notowań, świec, portfeli i transakcji;
- walidację jednostek, walut, dat i jakości danych;
- regresję, kanały, statystyki i modele ryzyka;
- reguły wyceny oraz obliczenia portfelowe;
- wspólne typy błędów i wyników.

Nie wolno implementować tej samej logiki analitycznej odrębnie w Linux GUI, WebUI, TUI i Androidzie.

## 3. Cztery interfejsy użytkownika

### 3.1. Linux GUI

Pełna aplikacja graficzna na Linux. Zalecany Qt 6 i C++. Interfejs powinien obsługiwać wykresy, tabele, konfigurację źródeł, instrumenty, portfel, alerty, raporty i ustawienia serwera. Może pracować z lokalnym serwerem albo łączyć się ze zdalną instancją.

### 3.2. WebUI

Interfejs przeglądarkowy udostępniany przez serwer. Zalecany HTML, CSS i TypeScript. WebUI korzysta z tego samego wersjonowanego API co pozostałe klienty. Nie może zawierać niezależnej implementacji obliczeń finansowych.

### 3.3. TUI

Interfejs terminalowy do administracji, diagnostyki, monitorowania danych i podstawowej obsługi portfela. Zalecany C++ z biblioteką terminalową, np. FTXUI lub ncurses. TUI korzysta z API serwera albo z lokalnej usługi — nie omija warstwy uprawnień i walidacji.

### 3.4. Android App

Aplikacja mobilna z dostępem do tych samych funkcji domenowych. Możliwe warianty:
- Qt 6 i C++;
- natywny interfejs Android w Kotlinie z biblioteką C++ udostępnioną przez JNI.

Wybór wariantu nie może prowadzić do duplikowania logiki rdzenia. Aplikacja ma obsługiwać bezpieczne logowanie, konfigurację serwera, synchronizację, wykresy, alerty i lokalną pamięć podręczną. Tryb offline powinien jasno rozróżniać dane zsynchronizowane od oczekujących na wysłanie.

## 4. Zasady komunikacji i synchronizacji

- Serwer wymiany danych jest punktem komunikacji między klientami i backendami.
- Wszystkie komunikaty mają wersjonowany schemat JSON.
- Każdy rekord powinien mieć stabilny identyfikator, źródło, czas utworzenia i aktualizacji oraz informację o wersji.
- Dane rynkowe zachowują czas obserwacji, czas pobrania, dostawcę, symbol, jednostkę, walutę i częstotliwość.
- Import musi być idempotentny: ponowne przesłanie tego samego rekordu nie może tworzyć niekontrolowanych duplikatów.
- Konflikty synchronizacji wymagają jawnej reguły rozstrzygania; nie wolno bezwarunkowo nadpisywać nowszych danych starszymi.
- Operacje zapisu powinny być rejestrowane i możliwe do audytu.
- Klienci nie łączą się bezpośrednio z MariaDB. Dostęp do magazynu zapewnia wyłącznie serwer.
- W trybie JSON serwer zarządza zapisem plików; wiele procesów nie może równolegle modyfikować tych samych dokumentów bez blokad lub kontroli wersji.
- Komunikacja zdalna wymaga TLS. Sekrety nie mogą trafiać do repozytorium.

## 5. Kierunek technologiczny — bez Pythona

Aktywny rozwój odbywa się w C++20, Bash oraz językach niezbędnych dla interfejsów (TypeScript/JavaScript dla WebUI, Kotlin wyłącznie jeśli zostanie wybrany natywny Android). Python nie jest wymagany i nie może być wprowadzany do aktywnego procesu budowania, testowania ani uruchamiania.

| Obszar | Technologia |
|---|---|
| Rdzeń, serwer i adaptery | C++20 |
| Budowanie | CMake |
| Automatyzacja | Bash |
| Linux GUI | Qt 6 / C++ |
| WebUI | HTML, CSS, TypeScript |
| TUI | C++ / FTXUI lub ncurses |
| Android | Qt 6/C++ albo Kotlin + JNI |
| API | HTTP/REST, JSON, WebSocket |
| Magazyn domyślny | pliki JSON |
| Magazyn opcjonalny | MariaDB |
| HTTP i integracje | libcurl |
| Testy | CTest oraz testy C++ |
| CI | GitHub Actions |

## 6. Aktualny stan implementacji

**Dostępne w repozytorium:**
- biblioteka C++20 `monitor_core`;
- regresja liniowa, SSE i kanały DGT/LonesomeTheBlue;
- program konsolowy `monitor-regression`;
- testy jednostkowe rdzenia;
- skrypty Bash do budowania i testowania;
- GitHub Actions budujące i testujące rdzeń C++ na Linux;
- wskaźnik Pine Script v6 dla TradingView.

**Kierunek architektoniczny opisany, ale jeszcze niezaimplementowany:**
- serwer wymiany danych i jego API;
- komunikacja pomiędzy wieloma backendami;
- magazyn JSON jako domyślny adapter trwałego zapisu;
- adapter MariaDB;
- autoryzacja, synchronizacja i kontrola konfliktów;
- Linux GUI, WebUI, TUI i Android App;
- adaptery dostawców danych w C++.

Nie należy przedstawiać tych elementów jako działających, dopóki nie zostaną zaimplementowane i przetestowane.

## 7. Budowanie i testowanie obecnego rdzenia

Wymagania: Linux, CMake 3.20+, kompilator C++20 i Bash.

Na Debianie/Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build
```

Kompilacja i testy:

```bash
git clone https://github.com/bartoszruta26-droid/Monitor.git
cd Monitor
bash scripts/build-native.sh
```

Osobny przebieg testów:

```bash
bash scripts/test-native.sh
```

Ręcznie:

```bash
cmake -S native -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --parallel
ctest --test-dir build/native --output-on-failure
```

Przykładowe obliczenie:

```bash
./build/native/monitor-regression 10 11 12 13 15 14 16
```

## 8. Silnik regresji i kanałów

Kod znajduje się w:
- `native/include/monitor/regression.hpp`;
- `native/src/regression.cpp`;
- `native/tests/test_regression.cpp`.

Dane wejściowe są uporządkowane chronologicznie od najstarszej do najnowszej obserwacji. Wartość dopasowana `fitted_last` odpowiada końcowi analizowanego okna.

Suma kwadratów reszt:

`SSE = Σ(y[i] − (intercept + slope · i))²`

DGT zachowuje szerokość:

`SD_DGT = sqrt(SSE / (n − 1))`

LonesomeTheBlue zachowuje:

`RMS_LTB = sqrt(SSE / n)`

Mianowniki nie mogą być ujednolicone, ponieważ zmieniłoby to szerokość kanałów. Testy rdzenia nie zastępują porównania z oryginalnymi wskaźnikami na wykresach TradingView.

## 9. Źródła danych i adaptery backendów

Planowane adaptery C++:

### NBP
- tabele A i B — kursy średnie;
- tabela C — kurs kupna i sprzedaży;
- wyliczony środek bid/ask musi być oznaczony jako wartość pochodna;
- referencyjna cena złota za gram w PLN.

API: https://api.nbp.pl/

### Frankfurter
- kursy referencyjne;
- jawna waluta bazowa i kwotowana;
- zachowanie daty i źródła obserwacji.

API: https://api.frankfurter.dev/

### World Bank Pink Sheet
- miesięczne ceny złota i srebra;
- USD za uncję trojańską;
- częstotliwość miesięczna, bez sztucznego tworzenia świec dziennych.

Źródło: https://www.worldbank.org/en/research/commodity-markets

Każdy adapter normalizuje odpowiedź dostawcy do wspólnego modelu. Dane źródłowe i przeliczone pozostają rozróżnialne. Dane miesięczne nie mogą być prezentowane jako dzienne OHLC.

## 10. Model danych i domeny

Docelowe obszary danych:
- instrumenty i symbole;
- kursy walut, ceny metali i inne wyceny;
- rzeczywiste świece OHLCV;
- źródła i konfiguracje adapterów;
- historia pobrań, importów i synchronizacji;
- portfele, pozycje i transakcje;
- wyniki analiz, wersje algorytmów i parametry;
- użytkownicy, role, uprawnienia i sesje;
- zdarzenia audytowe i alerty.

Wspólny model danych musi zachowywać jednostki, waluty, strefy czasowe, częstotliwości, pochodzenie i jakość obserwacji.

## 11. Konfiguracja magazynu

Przykładowy plik konfiguracyjny będzie wskazywał wybrany adapter:

```json
{
  "storage": {
    "driver": "json",
    "json": {
      "directory": "./data",
      "atomic_writes": true
    },
    "mariadb": {
      "host": "127.0.0.1",
      "port": 3306,
      "database": "monitor",
      "username": "monitor",
      "password_env": "MONITOR_DB_PASSWORD"
    }
  },
  "server": {
    "host": "127.0.0.1",
    "port": 8080,
    "api_prefix": "/api/v1"
  }
}
```

To jest docelowy przykład konfiguracji, a nie deklaracja, że oba adaptery są już zaimplementowane. Hasło nie jest wpisywane do pliku JSON — aplikacja odczytuje je ze zmiennej środowiskowej. Wdrożenie produkcyjne musi dodatkowo ograniczać dostęp do pliku konfiguracyjnego i katalogu danych.

## 12. Plan realizacji

### Etap A — wspólny rdzeń C++
- modele domenowe i typy danych;
- regresja, kanały i statystyka;
- walidacja i testy numeryczne.

### Etap B — magazyn i konfiguracja
- interfejs repozytorium;
- JSON jako domyślny magazyn;
- atomowy zapis, blokady, kopie bezpieczeństwa i migracje formatu;
- MariaDB jako opcjonalny adapter;
- testy zgodności obu magazynów.

### Etap C — serwer wymiany danych
- serwer HTTP C++;
- wersjonowane API REST/JSON;
- WebSocket dla aktualizacji;
- rejestracja backendów i klientów;
- uwierzytelnianie, role i uprawnienia;
- synchronizacja, idempotencja, konflikty i audyt.

### Etap D — adaptery backendów
- NBP, Frankfurter i World Bank;
- wspólny kontrakt adaptera;
- harmonogram, ponawianie błędów, limity i monitoring;
- testy z odpowiedziami wzorcowymi.

### Etap E — Linux GUI i TUI
- Qt 6 dla aplikacji graficznej;
- C++ i FTXUI/ncurses dla terminala;
- wspólny klient API i wspólny rdzeń.

### Etap F — WebUI
- responsywny interfejs przeglądarkowy;
- wersjonowany klient API;
- wykresy, tabele, konfiguracja, portfel i alerty.

### Etap G — Android
- współdzielenie rdzenia C++;
- JNI lub Qt 6;
- bezpieczna konfiguracja serwera i synchronizacja;
- tryb offline i powiadomienia.

### Etap H — portfel i analityka
- wycena pozycji;
- ekspozycja walutowa i ryzyko;
- centyle rozkładu reszt;
- scenariusze i horyzonty;
- testy kroczące poza próbą i kalibracja prawdopodobieństw.

Każdy etap wymaga testów i aktualizacji dokumentacji. Status „ukończony” można nadać dopiero po implementacji, testach i udokumentowaniu ograniczeń.

## 13. Odpowiedzialne przedstawianie wyników

Regresja opisuje dopasowanie do danych z wybranego okna, a nie gwarantowany kierunek przyszłej ceny. Pozycja w centylu rozkładu reszt nie jest automatycznie prawdopodobieństwem zysku. Prognozy muszą być walidowane poza próbą, a ich niepewność przedstawiana użytkownikowi.

System powinien odróżniać:
1. pozycję bieżącej ceny w historycznym rozkładzie;
2. prawdopodobieństwo scenariusza w określonym horyzoncie;
3. potencjalny wynik kwotowy dla rzeczywistego portfela.

Brak wystarczających danych powinien skutkować komunikatem o braku wiarygodnej estymacji.

## 14. Dokumentacja

- [Natywny rdzeń C++](native/README.md)
- [Wskaźnik TradingView](tradingview/README.md)
- [Audyt metodologii DGT](docs/dgt-linear-regression-audit.md)
- [Audyt metodologii LonesomeTheBlue](docs/lonesometheblue-linear-regression-audit.md)
- [Dostawcy danych](docs/open-data-providers.md)
- [Status historycznego backendu](backend/README.md)
- [Założenia architektury i wymiany danych](docs/architecture.md)

## 15. Licencja

Repozytorium zawiera plik LICENSE. Integracje z dostawcami zewnętrznymi muszą przestrzegać ich licencji, regulaminów, limitów i zasad atrybucji.
