# Połączony wskaźnik TradingView — DGT + LonesomeTheBlue

## Pliki

- `monitor_combined_linear_regression.pine` — jeden wskaźnik Pine Script v6.
- `../docs/dgt-linear-regression-audit.md` — audyt algorytmu DGT.
- `../docs/lonesometheblue-linear-regression-audit.md` — audyt algorytmu LonesomeTheBlue.

## Co zostało połączone

Wskaźnik ma jedną deklarację `indicator()` i dwie niezależne sekcje metodologiczne:

### DGT

- Kanał 1: domyślnie 200 świec.
- Kanał 2: domyślnie 50 świec.
- Dla obu kanałów poziomy ±1, ±2 i ±3 szerokości SD.
- Opcjonalne przeskalowanie długości drugiego kanału do wybranego interwału czasowego.
- Krzywa z pasmami opartymi niezależnie na odchyleniu ceny albo ATR.
- Tabela nachylenia.
- Dodatki wolumenowe i filtr zakresu świecy względem ATR.
- Alerty przecięcia regresji, poziomów, skoku wolumenu i wysokiej zmienności.

**Zachowana szerokość DGT:** `sqrt(SSE / (n - 1))`.

### LonesomeTheBlue

- Trzy niezależne kanały: domyślnie 377, 233 i 144 świece.
- Niezależne włączanie, kolor i grubość każdego kanału.
- Mnożnik szerokości domyślnie 2,41.
- Poziomy Fibo 0,236 / 0,382 / 0,618 / 0,786.
- Dwa tryby wybicia: kierunkowy („przeciw trendowi”, zgodny z warunkiem oryginału) oraz każde wybicie poza granicę.
- Oddzielne alerty dla kanałów i kierunków.

**Zachowana szerokość LonesomeTheBlue:** `sqrt(SSE / n)`.

Różne mianowniki są celowe. Nie należy porównywać szerokości kanałów bez uwzględnienia metody.

## Ustawienia

W oknie ustawień TradingView parametry są pogrupowane:

1. **Widoczność** — włączanie profili i tabeli.
2. **DGT — kanał 1** — długość, źródło, poziomy i wygląd.
3. **DGT — kanał 2** — długość, przeskalowanie interwału, poziomy i wygląd.
4. **DGT — krzywa, pasma i nachylenie** — metoda pasm i ATR.
5. **DGT — wolumen i zmienność** — SMA wolumenu, próg skoku, kolorowanie świec i filtr ATR.
6. **LonesomeTheBlue — kanały 1–3** — osobna długość, kolor i grubość każdego kanału.
7. **LonesomeTheBlue — wybicia i Fibo** — mnożnik, poziomy Fibo i sposób wykrywania wybić.
8. **Alerty i potwierdzanie** — potwierdzenie sygnałów na zamknięciu świecy.

## Szczegóły matematyczne i wydajność

Obliczenia używają wbudowanej regresji Pine `ta.linreg()`. Szerokość reszt jest wyznaczana bez pętli po całym oknie:

`SSE = Σ(y²) - (Σy)²/n - slope² · n(n²-1)/12`.

Jest to tożsamość OLS dla regresji liniowej z wyrazem wolnym. Zastosowanie sum kroczących ogranicza koszt obliczeń w porównaniu z wykonywaniem kilku długich pętli na każdej świecy. Wynik SSE jest ograniczany od dołu do zera ze względu na możliwy błąd zmiennoprzecinkowy.

### Ważne różnice względem źródeł

- LonesomeTheBlue: w audytowanym kodzie źródłowym stwierdzono przesunięcie indeksu reszt o jedną świecę. Nowa implementacja stosuje poprawną geometrię regresji i nie odtwarza tego błędu.
- DGT: przeskalowanie drugiego kanału zachowuje ideę oryginału — zmienia liczbę świec bieżącego interwału proporcjonalnie do czasu docelowego. **Nie pobiera rzeczywistych świec HTF** i nie jest równoważne regresji liczonej na osobnym interwale.
- Pasma krzywej DGT oparte na odchyleniu ceny lub ATR są odrębne od szerokości kanału regresji opartej na SSE.
- Sygnały wolumenu są pomijane, gdy instrument nie udostępnia wolumenu.
- Domyślnie sygnały są potwierdzane po zamknięciu świecy. Wyłączenie tej opcji dopuszcza sygnały intrabar, które mogą zniknąć przed zamknięciem.
- Wskaźnik działa na danych wykresu TradingView; nie pobiera danych z backendu Monitora Rynków.

## Instalacja w TradingView

1. Otwórz wykres TradingView.
2. Otwórz Pine Editor.
3. Wklej zawartość pliku `monitor_combined_linear_regression.pine`.
4. Zapisz skrypt jako „Monitor Rynków — DGT + LonesomeTheBlue”.
5. Wybierz „Dodaj do wykresu”.
6. W ustawieniach wskaźnika skonfiguruj profile i kanały.
7. Alerty utwórz z menu alertów TradingView, wybierając odpowiedni warunek wskaźnika.

## Testy akceptacyjne do wykonania w Pine Editor

- Seria stała: nachylenie i szerokość kanału powinny wynosić zero.
- Seria liniowa bez szumu: SSE powinno być bliskie zeru.
- Identyczne dane i długość: DGT powinien mieć szerszą szerokość niż RMS LTB, ponieważ dzieli SSE przez `n-1`, a LTB przez `n`.
- Włączenie i wyłączenie każdego kanału powinno działać niezależnie.
- Długości 377, 233, 144, 200 i 50 powinny działać bez błędu historii.
- Oba tryby wybicia powinny generować odmienne sygnały zgodnie z opisem.
- Przy braku wolumenu alert skoku wolumenu nie powinien być emitowany.

## Status weryfikacji

Plik jest przygotowaną implementacją Pine v6. Repozytorium i API GitHub nie zapewniają kompilatora Pine. Dopóki skrypt nie zostanie wklejony do Pine Editor i dodany do wykresu, **nie oznaczamy go jako skompilowanego ani zweryfikowanego na TradingView**. Weryfikacja w Pine Editor pozostaje obowiązkowym testem akceptacyjnym.

## Atrybucja i prawa

Metody i nazwy profili wskazują autorów oryginalnych koncepcji: DGT (dgtrd) oraz LonesomeTheBlue. Ten plik jest niezależną implementacją integracyjną, nie jest oficjalnym wskaźnikiem tych autorów. Należy zachować oznaczenia autorstwa i sprawdzić warunki licencyjne oryginalnych skryptów przed publiczną redystrybucją ich kodu lub pochodnych. Licencja repozytorium Monitor nie rozszerza automatycznie praw do zewnętrznych materiałów.
