<div align="center">

# MONITOR RYNKÓW

### Natywna platforma analizy danych rynkowych, regresji i portfela aktywów

**C++20 · Bash · Linux Desktop · Android · TradingView Pine Script**

</div>

---

## 1. Cel projektu

Monitor Rynków ma być wieloplatformowym systemem do gromadzenia, porządkowania, wizualizacji i analizy danych finansowych. Obejmuje waluty, złoto, srebro, inne metale i aktywa wyceniane indywidualnie. Projekt łączy:

1. kontrolowane źródła danych i zachowanie ich pochodzenia;
2. analizę techniczną, regresję liniową i kanały odchyleń;
3. ewidencję portfela, wycenę, ekspozycję i ryzyko;
4. wykresy, alerty, raporty i narzędzia decyzyjne.

Podstawową zasadą jest rozdzielenie danych źródłowych, danych przeliczonych, estymacji i prognoz. Każda wartość powinna mieć określone źródło, datę, jednostkę, walutę i metodę obliczenia.

## 2. Kierunek technologiczny — bez Pythona

Aktywny rozwój odbywa się w **C++20** oraz **Bash**. Python nie jest wymagany do budowania, testowania ani uruchamiania aktywnego rdzenia projektu. Historyczny prototyp backendu w katalogu `backend/` jest zamrożony i nie jest częścią bieżącego procesu CI.

### Docelowa architektura

| Warstwa | Technologia | Rola |
|---|---|---|
| Rdzeń obliczeniowy | C++20 | regresja, kanały, statystyki, portfel |
| Linux Desktop | Qt 6 / C++ | pełny interfejs graficzny |
| Android | C++20 + JNI lub Qt 6 | współdzielenie rdzenia i interfejs mobilny |
| Usługa danych | C++20 | API, adaptery dostawców i harmonogram |
| Magazyn lokalny | SQLite | praca offline i lokalna baza |
| Magazyn serwerowy | PostgreSQL | synchronizacja i praca wieloużytkownikowa |
| Komunikacja HTTP | libcurl | pobieranie danych z dostawców |
| Format danych | JSON | konfiguracja, wymiana danych i API |
| Budowanie i automatyzacja | CMake + Bash | kompilacja, testy, pakowanie i administracja |
| Wskaźnik TradingView | Pine Script v6 | niezależna integracja z wykresami TradingView |

Celem jest jeden wspólny rdzeń C++ używany przez różne interfejsy. Nie należy powielać matematyki regresji osobno w aplikacji desktopowej i mobilnej.

### Zasady architektoniczne

- Rdzeń analityczny nie może zależeć od Qt, Android SDK, bazy danych ani interfejsu.
- Interfejs komunikuje się z rdzeniem przez stabilne nagłówki C++ i — w razie potrzeby — cienką warstwę JNI.
- Adaptery dostawców, magazyn danych, API i interfejs użytkownika pozostają osobnymi modułami.
- Aplikacja mobilna powinna móc wykonywać podstawowe obliczenia i odczytywać lokalne dane bez połączenia z serwerem.
- Synchronizacja z usługą serwerową nie może blokować interfejsu.
- Sekrety i tokeny nigdy nie mogą być zapisane w repozytorium ani w plikach konfiguracyjnych użytkownika w postaci jawnej.

## 3. Aktualny stan implementacji

<badge color="success">Dostępne w repozytorium</badge>

- Natywna biblioteka C++20 `monitor_core`.
- Regresja liniowa metodą najmniejszych kwadratów.
- Obliczanie sumy kwadratów reszt (SSE).
- Dwie odrębne definicje szerokości kanału: DGT i LonesomeTheBlue.
- Kanał regresji z niezależnym mnożnikiem.
- Program konsolowy `monitor-regression`.
- Testy jednostkowe rdzenia bez zewnętrznego frameworka.
- Skrypty Bash do kompilacji i testów.
- CI GitHub Actions budujące i testujące kod C++ na Linux.

**Jeszcze nieukończone:** natywne adaptery danych NBP/Frankfurter/World Bank, magazyn SQLite/PostgreSQL w C++, usługa HTTP C++, interfejs Qt Desktop, aplikacja Android, synchronizacja i moduł portfela. Nie należy przedstawiać tych elementów jako gotowych.

## 4. Budowanie i testowanie

### Wymagania

- Linux;
- CMake 3.20 lub nowszy;
- kompilator C++20, np. GCC 10+ albo Clang 12+;
- Bash.

Na Debianie/Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build
```

### Kompilacja i testy

Z katalogu głównego repozytorium:

```bash
git clone https://github.com/bartoszruta26-droid/Monitor.git
cd Monitor
bash scripts/build-native.sh
```

Tryb testowy:

```bash
bash scripts/test-native.sh
```

Ręcznie przez CMake:

```bash
cmake -S native -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --parallel
ctest --test-dir build/native --output-on-failure
```

Uruchomienie przykładowego obliczenia:

```bash
./build/native/monitor-regression 10 11 12 13 15 14 16
```

Program wypisuje nachylenie, wartość regresji na końcu okna, SSE, szerokość DGT oraz szerokość RMS LonesomeTheBlue.

## 5. Silnik regresji i kanałów

Implementacja znajduje się w:

- `native/include/monitor/regression.hpp`;
- `native/src/regression.cpp`;
- `native/tests/test_regression.cpp`.

Dane wejściowe są przekazywane chronologicznie — od najstarszej do najnowszej obserwacji. Wartość regresji zwracana jako `fitted_last` odpowiada końcowi analizowanego okna.

### Wspólna regresja

Dla obserwacji `y[i]`, gdzie indeks czasu wynosi od 0 do n−1, silnik wyznacza współczynnik kierunkowy, wyraz wolny, wartości dopasowane i sumę kwadratów reszt:

`SSE = Σ(y[i] − (intercept + slope · i))²`

Obliczenia SSE są wykonywane przez bezpośrednie sumowanie reszt, aby ograniczyć utratę dokładności przy odejmowaniu dużych, zbliżonych wartości.

### Profil DGT

`SD_DGT = sqrt(SSE / (n − 1))`

Mianownik `n−1` jest zachowany zgodnie z audytowaną metodologią DGT.

### Profil LonesomeTheBlue

`RMS_LTB = sqrt(SSE / n)`

Mianownik `n` jest zachowany zgodnie z audytowaną metodologią LonesomeTheBlue. Nie należy ujednolicać tych mianowników — zmieniłoby to szerokość kanałów.

### Walidacja danych

Rdzeń odrzuca:
- mniej niż dwie obserwacje;
- wartości NaN i nieskończone;
- ujemny lub nieskończony mnożnik kanału.

Dalsze testy powinny obejmować porównania z oryginalnymi skryptami na tych samych instrumentach, interwałach, źródłach ceny i oknach danych.

## 6. Wskaźnik TradingView

Połączony wskaźnik Pine Script v6 znajduje się w:

[`tradingview/monitor_combined_linear_regression.pine`](tradingview/monitor_combined_linear_regression.pine)

Dokumentacja: [`tradingview/README.md`](tradingview/README.md).

Wskaźnik zachowuje odrębne profile DGT i LonesomeTheBlue. Pine Script jest przeznaczony do uruchamiania wewnątrz TradingView. Nie jest częścią natywnej biblioteki C++ i nie zastępuje jej.

**Status weryfikacji TradingView:** statyczny przegląd kodu i testy matematyczne na danych syntetycznych nie zastępują kompilacji w Pine Editor ani porównania wartości na rzeczywistych wykresach. Integrację należy uznać za zweryfikowaną dopiero po wykonaniu obu testów.

## 7. Źródła danych i model obserwacji

Planowane adaptery:

### NBP
- kursy tabel A i B jako oficjalne kursy średnie;
- tabela C jako kurs kupna i sprzedaży;
- środek bid/ask z tabeli C musi być oznaczony jako wartość pochodna;
- cena referencyjna złota za gram w PLN.

API: https://api.nbp.pl/

### Frankfurter
- kursy referencyjne rynku;
- jawna waluta bazowa i kwotowana;
- możliwość wyboru dostawcy, jeśli API udostępnia taki parametr.

API: https://api.frankfurter.dev/

### World Bank Pink Sheet
- miesięczne ceny złota i srebra;
- USD za uncję trojańską;
- częstotliwość miesięczna, bez sztucznego tworzenia świec dziennych.

Źródło: https://www.worldbank.org/en/research/commodity-markets

### Wymagane metadane

Każda obserwacja powinna zachować dostawcę, symbol, datę, walutę, jednostkę, częstotliwość, rodzaj ceny, czas pobrania i status jakości. Dane miesięczne nie mogą być przedstawiane jako dzienne OHLC. Wartości pochodne muszą być oznaczone.

## 8. Docelowy magazyn danych

Dane będą rozdzielone według semantyki:

- `fx_rates` — kursy walut, bid/ask i rodzaj kursu;
- `metal_prices` — ceny metali wraz z jednostką i częstotliwością;
- `candles` — rzeczywiste dane OHLCV;
- `valuation_observations` — wyceny punktowe aktywów, np. bursztynu i kamieni;
- `sources` — konfiguracja dostawców i historia importów;
- `portfolio_positions` oraz `portfolio_transactions` — stan portfela i historia transakcji;
- `analysis_runs` — wersja algorytmu, parametry i wyniki obliczeń.

SQLite będzie obsługiwać lokalny tryb pracy. PostgreSQL będzie przeznaczony dla usługi serwerowej i synchronizacji.

## 9. Plan rozwoju

### Etap A — natywny rdzeń C++
- regresja, SSE i kanały;
- testy numeryczne;
- profile DGT i LonesomeTheBlue;
- wersjonowanie algorytmów.

### Etap B — dane i trwały zapis
- adaptery NBP, Frankfurter i World Bank w C++;
- libcurl dla HTTPS;
- parser JSON i XLSX;
- SQLite;
- walidacja, deduplikacja i historia importu.

### Etap C — API i synchronizacja
- usługa HTTP w C++;
- uwierzytelnianie i uprawnienia;
- PostgreSQL;
- harmonogram pobierania i ponawianie błędów;
- synchronizacja klient–serwer.

### Etap D — Linux Desktop
- Qt 6;
- wykresy interaktywne;
- ustawienia instrumentów i źródeł;
- portfel, raporty i alerty.

### Etap E — Android
- współdzielenie `monitor_core`;
- warstwa JNI lub Qt 6;
- lokalna baza SQLite;
- wykresy dotykowe i powiadomienia;
- bezpieczna synchronizacja z usługą.

### Etap F — model statystyczny i portfel
- centyle rozkładu reszt;
- scenariusze i horyzonty;
- testy kroczące poza próbą;
- kalibracja prawdopodobieństw;
- wycena faktycznego portfela i analiza ryzyka.

Każdy etap wymaga testów oraz aktualizacji dokumentacji. Funkcje planowane nie mogą być oznaczane jako ukończone.

## 10. Odpowiedzialne przedstawianie wyników

Regresja opisuje dopasowanie do danych z wybranego okna. Nie gwarantuje przyszłego kierunku ceny. Pozycja w centylu rozkładu reszt nie jest automatycznie prawdopodobieństwem zysku. Model przyszłych scenariuszy musi być walidowany poza próbą, a jego niepewność pokazywana użytkownikowi.

Monitor ma rozdzielać:
1. pozycję bieżącej ceny w historycznym rozkładzie;
2. prawdopodobieństwo scenariusza w określonym horyzoncie;
3. potencjalny wynik kwotowy dla rzeczywistego portfela.

Brak wystarczających danych powinien skutkować komunikatem o braku wiarygodnej estymacji, a nie pozornie precyzyjnym wynikiem.

## 11. Dokumentacja techniczna

- [Natywny rdzeń C++](native/README.md)
- [Wskaźnik TradingView](tradingview/README.md)
- [Audyt metodologii DGT](docs/dgt-linear-regression-audit.md)
- [Audyt metodologii LonesomeTheBlue](docs/lonesometheblue-linear-regression-audit.md)
- [Dostawcy danych — notatki i ograniczenia](docs/open-data-providers.md)
- [Status migracji warstwy danych](backend/README.md)

## 12. Licencja i warunki wykorzystania

Repozytorium zawiera plik LICENSE. W przypadku integracji kodu lub danych dostawców zewnętrznych należy dodatkowo przestrzegać ich licencji, regulaminów, limitów i zasad atrybucji.
