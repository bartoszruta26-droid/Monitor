# Natywny rdzeń Monitor
Rdzeń obliczeniowy jest rozwijany w C++20 i nie wymaga środowiska Python ani zewnętrznych bibliotek.

## Budowanie i testowanie
Wymagania: CMake 3.20+ oraz kompilator C++20 (GCC 10+, Clang 12+ lub równoważny).
Z katalogu głównego repozytorium uruchom:
```bash
bash scripts/build-native.sh
```

## Zakres
- regresja liniowa metodą najmniejszych kwadratów;
- suma kwadratów reszt (SSE);
- szerokość DGT: sqrt(SSE / (n - 1));
- szerokość LonesomeTheBlue: sqrt(SSE / n);
- kanał z niezależnym mnożnikiem;
- testy jednostkowe bez zewnętrznego frameworka.

Dane wejściowe muszą być uporządkowane chronologicznie od najstarszej do najnowszej. Wynik zwraca wartość dopasowaną dla ostatniej obserwacji okna.

## Interfejsy docelowe
- Linux Desktop: Qt 6 / C++;
- Android: Qt 6 lub natywny interfejs Android z JNI do `monitor_core`;
- usługa danych: C++20, libcurl, SQLite/PostgreSQL;
- Bash: budowanie, testowanie, pakowanie i zadania administracyjne.

Wersje desktopowa, Android i usługa danych są etapami planowanymi, a nie funkcjami ukończonymi.
