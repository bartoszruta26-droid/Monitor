# Natywny rdzeń Monitor

Biblioteka monitor_core jest wspólną warstwą domenową i obliczeniową dla Linux GUI, WebUI (przez API serwera), TUI i Android App. Jest rozwijana w C++20.

## Zasady zależności
Model domenowy i obliczenia pozostają niezależne od Qt, Android SDK, serwera HTTP i konkretnego magazynu. Moduł infrastruktury storage-json korzysta z nlohmann-json i systemowych mechanizmów POSIX; MariaDB będzie osobnym adapterem tego samego kontraktu. Nie wolno powielać obliczeń w poszczególnych interfejsach.

## Aktualny zakres
- wspólny model danych rynkowych v1: instrumenty, dostawcy, notowania i metadane szeregów;
- walidacja walut, jednostek, pochodzenia, jakości, czasu i wersji schematu;
- normalizacja ISO-8601 do UTC;
- konwersje gram/kilogram/uncja trojańska;
- regresja liniowa metodą najmniejszych kwadratów;
- suma kwadratów reszt (SSE);
- szerokość DGT: sqrt(SSE / (n - 1));
- szerokość LonesomeTheBlue: sqrt(SSE / n);
- wspólny interfejs adapterów dostawców i transport HTTP libcurl;
- adaptery NBP A/B/C, Frankfurter v2, World Bank Pink Sheet i XAUS spot;
- testy adapterów z atrapami HTTP, bez połączeń sieciowych;
- kontrakt MarketDataRepository i implementacja JsonMarketDataRepository;
- wersjonowany dokument JSON, rewizje, zapis atomowy, blokady międzyprocesowe i kopie zapasowe;
- atomowe przyjmowanie partii adapterów oraz historia korekt notowań;
- testy magazynu obejmujące round-trip, odzyskiwanie i współbieżność;
- CLI, testy jednostkowe i CTest.

## Budowanie i testowanie
Wymagania: CMake 3.20+, kompilator C++20, Bash, libcurl, nlohmann-json, libarchive i libxml2.
Z katalogu głównego:
bash scripts/build-native.sh
bash scripts/test-native.sh

## Docelowy podział modułów
- core — modele domenowe, walidacja i obliczenia;
- server — API, autoryzacja, synchronizacja i komunikacja;
- storage-json — domyślny zapis plikowy;
- storage-mariadb — opcjonalny magazyn MariaDB;
- adapters — dostawcy danych zewnętrznych;
- clients/linux-gui — Qt 6;
- clients/tui — terminal C++;
- clients/webui — aplikacja przeglądarkowa;
- clients/android — Qt 6 albo Kotlin/JNI.

Powyższy podział opisuje architekturę docelową. Aktywnie zaimplementowane są rdzeń regresji, model danych, adaptery dostawców i pierwszy magazyn JSON. Serwer, MariaDB i klienci pozostają do wykonania.
JSON jest docelowym domyślnym magazynem, a MariaDB opcjonalnym adapterem. Oba mają implementować wspólny kontrakt repozytorium. Serwer będzie jedynym komponentem uprawnionym do zapisu w magazynie serwerowym.
