# Audyt i integracja: Linear Regression Channel — LonesomeTheBlue

## 1. Cel

Drugi skrypt wnosi do Monitora Rynków trzy niezależne kanały regresji (domyślnie długości 377, 233 i 144), poziomy Fibonacciego wewnątrz kanału, wykrywanie wybicia poza kanał oraz opcję pozostawiania oznaczeń wybitych kanałów. Jest to odrębna metoda od wskaźnika DGT. Nie wolno utożsamiać ich parametrów ani wyników.

Niniejszy dokument określa audyt źródła Pine Script v4 i sposób połączenia obu metod w jednym module analitycznym. To specyfikacja i audyt kodu, a nie potwierdzenie istnienia działającego silnika obliczeniowego lub wykresu.

## 2. Co robi drugi wskaźnik

Dla każdej z trzech długości `len`, `len2`, `len3` wyznacza:
- średnią źródła z okna;
- nachylenie jako różnicę `linreg(src, len, 0) - linreg(src, len, 1)`;
- wyraz wolny i wartość końcową regresji;
- odchylenie RMS reszt;
- trzy linie kanału: dolną, środkową i górną, z szerokością `dev × devlen`;
- cztery poziomy Fibonacciego 0,236 / 0,382 / 0,618 / 0,786 wewnątrz kanału;
- stan wybicia: 0 dla wybicia dołem przy dodatnim nachyleniu, 2 dla wybicia górą przy ujemnym nachyleniu, -1 w pozostałych przypadkach.

Domyślne długości to 377, 233 i 144, a mnożnik odchylenia `devlen` wynosi 2,41.

## 3. Matematyka i zgodność indeksowania

Nachylenie:
`b = linreg(src, n, 0) - linreg(src, n, 1)`.

Wyraz wolny dla najstarszej obserwacji:
`a = mean(src, n) - b × (n-1)/2`.

Wartość dopasowana na pozycji `j` liczonej od najstarszej świecy:
`ŷ(j) = a + b × j`, dla `j = 0 ... n-1`.

Ponieważ w Pine `src[x]` oznacza x świec wstecz, gdzie x=0 jest świecą najnowszą, dopasowana wartość dla obserwacji `src[x]` wynosi:
`ŷ_x = a + b × (n-1-x)`.

### P1 — błąd o jedną świecę w obliczaniu odchylenia

W pętli oryginału użyto:
`src[x] - (b × (n-x) + a)`.

Powinno być:
`src[x] - (b × (n-1-x) + a)`.

Obecna formuła porównuje cenę każdej świecy z linią przesuniętą o jedno nachylenie. Powoduje to błędne reszty, a zatem błędną szerokość kanału i położenie wszystkich poziomów opartych na `dev`. W implementacji Monitora należy poprawić indeksowanie i udostępnić osobny tryb „odtworzenie oryginalnego błędu” wyłącznie wtedy, gdy potrzebna będzie reprodukcja historycznego wyglądu wskaźnika. Domyślnie używać poprawnej formuły.

### P1 — alertcondition otrzymuje kod liczbowy zamiast jawnego warunku logicznego

`outofchannel` przyjmuje wartości -1, 0 albo 2. Wywołanie `alertcondition(outofchannel, ...)` jest niejednoznaczne i w zależności od reguł typowania Pine może być odrzucone lub interpretowane niezgodnie z intencją. Poprawny warunek to jawne:
`outofchannel == 0 or outofchannel == 2`.
Dodatkowo należy rozdzielić alert wybicia dołem i górą, a alert potwierdzać po zamknięciu świecy. W Monitorze każde zdarzenie otrzymuje osobny typ, kierunek i identyfikator świecy.

### P1 — logika wybicia jest kierunkowa, a nie symetryczna

Kod zgłasza wybicie dolną granicą wyłącznie przy dodatnim nachyleniu oraz górną granicą wyłącznie przy ujemnym nachyleniu. Nie zgłasza wybicia górą w trendzie wzrostowym ani dołem w trendzie spadkowym. To może być zamierzony filtr „wyjścia przeciw trendowi”, ale nie jest ogólnym wykrywaniem wybicia z kanału. W Monitorze należy zachować dwa osobne tryby:
- `counter_trend_only` — dokładna intencja autora: dół przy wzroście, góra przy spadku;
- `any_boundary_break` — dowolne zamknięcie poza górną lub dolną granicą.
Nie wolno prezentować pierwszego trybu jako wykrywania wszystkich wybić.

### P2 — logika pozostawiania wybitych linii i zarządzanie obiektami

Skrypt tworzy i usuwa obiekty linii na kolejnych świecach. W gałęzi wybicia zachowuje poprzednią linię, koloruje ją na niebiesko i zastępuje identyfikator nową linią. Jest to mechanizm wizualnego śladu, który może kumulować obiekty aż do limitu `max_lines_count=300`. W Monitorze nie należy kopiować obiektowego modelu Pine. Należy przechowywać zdarzenie wybicia jako rekord danych i rysować jego odcinek z ograniczoną retencją.

### P2 — tablice i inicjalizacja kolorów

`array.new_color(2)` tworzy tablicę z dwoma elementami, po czym `unshift` dodaje kolejne dwa. Tablica ma cztery elementy, a w pokazanym kodzie jest nieużywana, ponieważ wybór koloru kierunku jest zakomentowany. To martwy kod do usunięcia podczas portowania.

### P2 — powielony kod trzech kanałów

`get_channel`, `get_channel2` i `get_channel3` są kopiami tej samej funkcji. W Monitorze należy użyć jednej funkcji obliczeniowej przyjmującej długość i konfigurację. Zapobiega to rozbieżnościom między kanałami i upraszcza testy.

### P2 — identyczne tytuły alertów

Wszystkie trzy `alertcondition` używają tytułu `Channel Broken`. W nowym module alerty powinny identyfikować kanał (377/233/144), kierunek wybicia i rodzaj reguły.

### P2 — odchylenie RMS a kanał DGT

Drugi skrypt liczy:
`dev = sqrt(SSE/n)`.

DGT liczy:
`SD_DGT = sqrt(SSE/(n-1))`.

Nie są to identyczne kanały. Dla tego samego SSE kanał DGT będzie szerszy. Nie należy ujednolicać mianownika, jeśli celem jest wierne odtworzenie każdego wskaźnika. Wspólny silnik ma udostępniać metody `RMS_n`, `DGT_n_minus_1` oraz `Regression_SE_n_minus_2` jako jawne warianty.

### P3 — poziomy Fibonacciego

Poziomy 0,236 / 0,382 / 0,618 / 0,786 są interpolowane liniowo między dolną i górną granicą kanału. Są poziomami proporcji szerokości kanału, a nie klasycznymi poziomami Fibonacciego wyznaczanymi z odrębnego ruchu cenowego. Interfejs powinien nazywać je „poziomami Fibo wewnątrz kanału”.

### P3 — brak niezależnych ustawień części parametrów

Kod definiuje osobne szerokości i style linii dla trzech kanałów, ale w części wywołań szerokość jest przekazywana z `widt` zamiast `widt2` lub `widt3`. Widać to przy rysowaniu kanału 2 i 3: `width = widt`. Należy używać odpowiednio `widt2` i `widt3`. Ponadto wejścia `line_av*`, `line_fi*`, `line_dev*` są typowane jako style linii, ale port docelowy powinien używać własnego enumu prezentacyjnego.

### P3 — sygnał wybicia oparty na bieżącej cenie

`close` na aktywnej świecy zmienia się w trakcie jej formowania. Wybicie może zniknąć przed zamknięciem. Domyślnie Monitor ma generować zdarzenie dopiero po zamknięciu świecy; tryb intrabar musi być opcjonalny i oznaczony jako tymczasowy.

## 4. Integracja z DGT — wspólny silnik, osobne profile

Nie tworzyć jednego „uśrednionego wskaźnika”. Oba źródła korzystają z regresji liniowej, ale różnią się:
- szerokością kanału: DGT `SSE/(n-1)`, LonesomeTheBlue `SSE/n`;
- konfiguracją kanałów: DGT dwa kanały z poziomami SD, drugi może mieć tryb rozszerzenia; LonesomeTheBlue trzy równoległe długości;
- dodatkowymi poziomami: LonesomeTheBlue ma poziomy Fibo wewnątrz kanału;
- zdarzeniami: DGT przecięcia linii i dodatki wolumenowo-ATR; drugi skrypt ma kierunkowy stan wybicia kanału;
- logiką HTF: DGT przeskalowuje długość okna; drugi skrypt nie implementuje HTF.

### Wspólne komponenty obliczeniowe

1. `linear_regression(source, length)` — zwraca slope, intercept, fitted values i SSE.
2. `residual_width(SSE, n, method)` — wybór RMS / DGT / regresyjny SE.
3. `channel_levels(midline, width, multiplier)` — granice i poziomy odchyleń.
4. `channel_fibonacci(lower, upper, ratios)` — 0,236 / 0,382 / 0,618 / 0,786.
5. `breakout_detector(close, upper, lower, slope, mode)` — kierunkowe lub symetryczne wykrywanie wybicia.
6. `timeframe_aggregator(OHLCV, target_interval)` — prawdziwe świece HTF.
7. `event_store` — historia przecięć i wybić z parametrami i znacznikiem czasu.

### Profile wskaźników w interfejsie

- **DGT:** dwa kanały, poziomy ±1/±2/±3 SD, krzywa, pasma ATR/SD, nachylenie, wolumen i zakres świecy względem ATR.
- **LonesomeTheBlue:** trzy kanały (domyślnie 377/233/144), mnożnik 2,41, poziomy Fibo w kanale, wykrywanie wybić i ślad historyczny.
- **Porównanie:** nakładanie profili na ten sam instrument i interwał, z widocznym oznaczeniem metod szerokości kanału i czasu danych.

## 5. Model danych wyniku

Każdy kanał powinien zwracać:
- `instrument`, `provider`, `timeframe`, `source`, `bar_timestamp`, `is_closed`;
- `length`, `slope`, `intercept`, `start_value`, `end_value`;
- `sse`, `deviation_method`, `deviation`, `multiplier`;
- `lower`, `mid`, `upper`;
- `fibonacci_levels` (opcjonalne);
- `breakout_mode`, `breakout_direction`, `breakout_confirmed`;
- `calculation_version` i status jakości danych.

Wyniki obu profili muszą zawierać jawny identyfikator metody, by można było odtworzyć obliczenia historyczne po zmianie algorytmu.

## 6. Testy akceptacyjne

1. Seria stała: slope=0, SSE=0, dev=0.
2. Seria liniowa: dokładny slope i SSE=0.
3. Seria liniowa z szumem: RMS obliczony jako `sqrt(SSE/n)`.
4. Porównanie DGT i RMS: dla tego samego SSE kanał DGT jest szerszy niż RMS, jeśli n>1.
5. Test indeksowania: reszta dla `src[x]` używa `n-1-x`; porównanie z błędną formułą `n-x` musi wykazać przesunięcie.
6. Test poziomów Fibo: poziom 0,236 leży 23,6% drogi od dolnej do górnej granicy.
7. Wybicie przeciw trendowi: dodatni slope i close poniżej dolnej granicy; ujemny slope i close powyżej górnej granicy.
8. Wybicie dowolne: test wszystkich czterech kombinacji strony kanału i kierunku trendu.
9. Test przejścia przez granicę i zamknięcia świecy; brak wielokrotnych alertów dla jednego zdarzenia.
10. Trzy długości kanałów działają przez tę samą funkcję, ale zachowują niezależne parametry.
11. Test limitu obiektów/retencji zdarzeń — liczba zapisanych wybić nie powoduje niekontrolowanego wzrostu zasobów.
12. Test identycznych danych wejściowych dla profili DGT i LonesomeTheBlue; różnice są zgodne z metodą odchylenia i odmiennymi regułami wybicia.

## 7. Decyzje wdrożeniowe

- Nie kopiować błędu indeksowania z oryginalnego skryptu do domyślnego trybu.
- Zachować tryb reprodukcji oryginalnego wyglądu tylko jako opcję diagnostyczną, wyłączoną domyślnie.
- Nie utożsamiać kanału RMS z kanałem DGT.
- Zachować trzy osobne długości i parametry wizualne.
- Domyślnie wykrywać wybicie po zamknięciu świecy.
- Udostępnić tryb „przeciw trendowi” zgodny z autorem oraz symetryczny tryb dowolnego wybicia.
- Nie generować rekomendacji kupna/sprzedaży wyłącznie na podstawie wybicia.
- Najpierw testy obliczeniowe i porównanie z TradingView, potem wizualizacja, alerty i zapis historii.

## 8. Status

Audyt drugiego skryptu i projekt integracji z profilem DGT są dokumentacją techniczną. Wersja pluginu nie zawiera jeszcze uruchomionego silnika OHLCV, wykresu ani trwałego rejestru sygnałów. Do tego potrzebne są źródła danych, backend obliczeniowy i testy porównawcze.


## Aktualizacja implementacyjna — 2026-11-12

W repozytorium dodano połączony wskaźnik Pine Script v6: [`tradingview/monitor_combined_linear_regression.pine`](../tradingview/monitor_combined_linear_regression.pine) oraz instrukcję [`tradingview/README.md`](../tradingview/README.md). Implementacja zachowuje odrębne mianowniki i ustawienia profili. Kod został zapisany i ponownie odczytany z gałęzi `main`; nie został jednak skompilowany w TradingView Pine Editor. Audyt nadal opisuje także szerszą docelową integrację backendową, która pozostaje osobnym zadaniem.
