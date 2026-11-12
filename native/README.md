# Natywny rdzeń Monitor

Biblioteka `monitor_core` jest wspólną warstwą domenową i obliczeniową dla Linux GUI, WebUI (przez API serwera), TUI i Android App. Jest rozwijana w C++20.

## Zasady zależności

Rdzeń nie zależy od Qt, Android SDK, serwera HTTP, JSON-owego magazynu ani MariaDB. Warstwy zewnętrzne korzystają z publicznych interfejsów rdzenia. Nie wolno powielać obliczeń w poszczególnych interfejsach.

## Aktualny zakres

- regresja liniowa metodą najmniejszych kwadratów;
- suma kwadratów reszt (SSE);
- szerokość DGT: `sqrt(SSE / (n - 1))`;
- szerokość LonesomeTheBlue: `sqrt(SSE / n)`;
- kanał z niezależnym mnożnikiem;
- program konsolowy i testy jednostkowe.

## Budowanie i testowanie

Wymagania: CMake 3.20+, kompilator C++20 i Bash.

Z katalogu głównego:

```bash
bash scripts/build-native.sh
bash scripts/test-native.sh
```

## Docelowy podział modułów

- `core` — modele domenowe, walidacja i obliczenia;
- `server` — API, autoryzacja, synchronizacja i komunikacja backendów;
- `storage-json` — domyślny zapis plikowy;
- `storage-mariadb` — opcjonalny magazyn MariaDB;
- `adapters` — dostawcy danych zewnętrznych;
- `clients/linux-gui` — Qt 6;
- `clients/tui` — terminal C++;
- `clients/webui` — aplikacja przeglądarkowa;
- `clients/android` — Qt 6 albo Kotlin/JNI.

Powyższy podział opisuje architekturę docelową. Poza aktualnym silnikiem regresji moduły nie są jeszcze zaimplementowane.

## Kontrakt magazynu

JSON jest domyślnym wariantem przechowywania danych. MariaDB jest opcjonalnym adapterem wybieranym w konfiguracji. Oba warianty mają implementować ten sam kontrakt repozytorium, wspólną walidację, wersjonowanie danych i obsługę błędów.

Serwer wymiany danych jest jedynym komponentem uprawnionym do zapisu w magazynie serwerowym. Klienci korzystają z wersjonowanego API i nie łączą się bezpośrednio z MariaDB.
