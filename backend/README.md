# Warstwa danych — status migracji

## Kierunek projektu

Docelowa implementacja Monitora jest rozwijana w C++20. Nie należy instalować ani uruchamiać starego prototypu serwerowego jako wymagania projektu. Nowy rdzeń C++ jest niezależny od interfejsu i będzie wspólny dla aplikacji Linux Desktop, Android oraz usługi danych.

## Status

- **Aktywny kod:** `../native` — biblioteka C++20 regresji i kanałów.
- **Budowanie/testy:** `../scripts/build-native.sh` oraz `../scripts/test-native.sh`.
- **Dostawcy danych:** migracja adapterów NBP, Frankfurter i World Bank do C++ jest kolejnym etapem.
- **Baza danych:** docelowo SQLite lokalnie oraz PostgreSQL dla usługi serwerowej.
- **API HTTP:** do odtworzenia w C++ po ustaleniu kontraktu API; dotychczasowe trasy są opisane w głównym README jako historyczny zakres prototypu, nie jako działająca usługa natywna.

## Ważne

Katalog zawiera historyczny prototyp backendu. Nie jest on częścią aktualnego procesu budowania, testowania ani wdrażania. Nie rozwijaj nowych funkcji w tym katalogu. Kod zostanie usunięty po przeniesieniu i zweryfikowaniu równoważnych funkcji w natywnej implementacji C++.

Do czasu zakończenia migracji nie należy deklarować, że natywna usługa API, import danych lub synchronizacja automatyczna są gotowe.
