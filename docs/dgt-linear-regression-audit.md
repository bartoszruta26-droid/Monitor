# Audyt: Linear Regression Channel / Curve / Slope by DGT

## 1. Cel i zakres

Audyt dotyczy przekazanego skryptu Pine Script v4 autorstwa DGT. Celem jest przygotowanie wiarygodnej specyfikacji przeniesienia obliczeń do Monitora Rynków, a nie mechaniczne przepisanie kodu. Należy zachować rozróżnienie między zgodnością z oryginałem a poprawkami matematycznymi i wydajnościowymi.

## 2. Architektura funkcjonalna oryginału

1. **Kanał regresji 1** — regresja liniowa źródła (domyślnie `close`) na zadanej liczbie świec, linia bazowa oraz trzy pary poziomów ±SD.
2. **Kanał regresji 2** — drugi zestaw parametrów i poziomów; opcjonalne wydłużenie okna przez mnożnik interwału.
3. **Krzywa regresji** — `linreg(source, length, 0)` rysowana jako bieżąca wartość końcowa regresji kroczącej.
4. **Pasma krzywej** — `stdev(source, length)` albo ATR, mnożone przez 1, 2 i 3.
5. **Nachylenie** — nachylenie regresji ceny zamknięcia wyrażone jako zmiana ceny na świecę.
6. **Wolumen** — średnia SMA wolumenu, wykrywanie skoku wolumenu i kolorowanie świec progami 1,618 oraz 0,618.
7. **Zmienność** — zakres świecy `high-low` porównywany z `ATR × 2,718`.
8. **Alerty** — przecięcia linii bazowych i kanałów, skoki wolumenu oraz wysoka zmienność.

## 3. Matematyka kanału regresji

Dla `n` obserwacji skrypt przypisuje `x_i=i+1` i `y_i=source[i]`, gdzie `i=0` oznacza najnowszą świecę. Wyznacza nachylenie metodą najmniejszych kwadratów:

`b = (n Σ(xy) - Σx Σy) / (n Σ(x²) - (Σx)²)`

Wartość linii dla najnowszej świecy (pozycja `x=1`) jest obliczana jako:

`a1 = średnia(y) - b × średnia(x) + b`

Wartość dopasowana dla najstarszej świecy to `a1 + b × (n-1)`, a dla najnowszej `a1`. Rysowanie od najstarszej do najnowszej świecy jest zgodne z tym porządkiem indeksowania.

Reszty regresji to `e_i = y_i - (a1 + b × i)`, a suma kwadratów reszt `SSE = Σ(e_i²)`.

Oryginalny skrypt definiuje szerokość kanału:

`SD_DGT = sqrt(SSE / (n-1))`

Jest to jawna definicja zgodności z DGT. Nie należy jej bezgłośnie utożsamiać z klasycznym błędem standardowym regresji, dla którego przy dwóch estymowanych parametrach stosuje się zwykle `sqrt(SSE/(n-2))`. Możliwa jest także miara RMS `sqrt(SSE/n)`. W Monitorze Rynków metoda musi być jawnie opisana i konfigurowalna. Dla wariantu `n-2` wymagane jest `n>2`.

### Istotna różnica: krzywa i kanał

Kanał główny używa odchylenia reszt od dopasowanej regresji. Natomiast pasma krzywej używają `stdev(source, length)` albo ATR. `stdev(source, length)` mierzy zmienność cen względem ich średniej kroczącej, a nie rozrzut reszt względem krzywej regresji. Te pasma nie są zatem tą samą miarą co kanał ±SD.

## 4. Ustalenia i problemy

### P1 — wydajność obliczeń odchylenia

`f_calcSlope()` wykonuje pętlę tylko na ostatniej świecy, ale `f_calcDev()` jest wywoływana bez warunku `barstate.islast`. Oznacza to wykonywanie pętli odchylenia na wielu historycznych świecach, mimo że nachylenie i wyraz wolny są tam `na`. Przy długich oknach jest to zbędny koszt obliczeniowy i ryzyko przekroczenia limitów wykonania. W implementacji docelowej obliczenia kanału dla bieżącego widoku należy wykonywać tylko wtedy, gdy są potrzebne; historyczne szeregi do testów należy liczyć w kontrolowanym, wektorowym przebiegu.

### P1 — mnożnik HTF nie pobiera rzeczywistych świec HTF

Oryginał mnoży liczbę świec przez stosunek minut wybranego interwału do interwału wykresu, a następnie oblicza regresję na świecach bieżącego interwału. Przykładowo 50 świec 4H na wykresie 5m zamienia się w okno 2400 świec 5m. To przybliża czasowy zasięg okna, ale nie jest regresją z 50 zamknięć świec 4H. Różnice będą widoczne przy zmienności wewnątrz świecy, lukach, sesjach i innych źródłach danych. W Monitorze Rynków prawdziwy HTF wymaga agregowania danych do świec 1H/4H/1D i wykonywania regresji na tych świecach. Tryb zgodności DGT można zachować jako osobną opcję nazwaną „przeskalowane okno na interwale bieżącym”.

### P1 — limit 5000 świec może być przekroczony po przeskalowaniu

Parametr wejściowy ma maksimum 5000, ale po przemnożeniu przez stosunek interwałów `linregLength2` może być znacznie większy. Przykładowo 50 × 240 / 1 = 12000 świec na wykresie 1m. Ograniczenie `max_bars_back(..., 5000)` nie obsłuży takiego okna. Opcja skrócenia wyświetlania nie zmniejsza długości obliczeń, ponieważ `f_calcSlope()` i `f_calcDev()` nadal otrzymują pełne `linregLength2`.

### P2 — błąd o jedną świecę przy skróceniu drugiego kanału

Gdy aktywne jest skrócenie, `startPrice2` jest liczone jako `intercept2 + slope2 × i_showLast`, podczas gdy linia o długości `i_showLast` powinna zaczynać się od `intercept2 + slope2 × (i_showLast-1)`. Powoduje to przesunięcie wartości początku odcinka o jedno nachylenie.

### P2 — interpretacja odchylenia standardowego

DGT dzieli SSE przez `n-1`. W klasycznym błędzie standardowym regresji typowy mianownik to `n-2`, ponieważ estymowane są dwa parametry. Nie oznacza to automatycznie, że wariant DGT jest bezużyteczny — jest to definicja kanału użyta przez autora. Monitor musi udostępnić dokładny tryb zgodności DGT i odrębny tryb statystyczny, z jasnym opisem. Nie wolno mieszać wyników między trybami bez oznaczenia.

### P2 — alerty intrabar i powtarzalność

Warunek przecięcia używa bieżącego `close`, które na aktywnej świecy zmienia się przed jej zamknięciem. Alert może zatem sygnalizować przecięcie, które zniknie przed zamknięciem świecy. Dla stabilnej historii sygnałów należy domyślnie potwierdzać przecięcie na zamknięciu świecy, przechowywać czas i identyfikator świecy oraz deduplikować alerty. Tryb intrabar powinien być opcjonalny i oznaczony jako wstępny.

### P2 — granica przecięcia

`f_crossingLevel()` używa nierówności ostrych. Dotknięcie poziomu dokładnie na zamknięciu świecy nie jest przecięciem. Docelowa logika powinna mieć jawnie określone zachowanie przy równości i uniknąć wielokrotnego sygnału, gdy cena oscyluje wokół poziomu.

### P2 — ograniczenia wolumenu

`nz(volume)` zamienia brak danych wolumenowych na zero. Dla instrumentów bez wiarygodnego wolumenu może to zniekształcić SMA, progi i interpretację. Monitor powinien przechowywać osobny status jakości wolumenu: dostępny, szacowany albo niedostępny. W przypadku braku wolumenu pomijać sygnały wolumenowe, a nie prezentować zerowe wartości jako prawdziwe.

### P3 — interpretacja filtra zmienności

Warunek `high-low > ATR × 2,718` mierzy wyjątkowo duży zakres pojedynczej świecy względem ATR. Nie mierzy wszystkich form zmienności, np. zmienności zrealizowanej, luk między sesjami ani zmienności w krótszych oknach. Nazwa w interfejsie powinna brzmieć „świeca o podwyższonym zakresie względem ATR”.

### P3 — nachylenie zależy od źródła

Kanały mogą korzystać z `i_linregSrc`, natomiast dodatek nachylenia używa `close`. W interfejsie należy ujawnić tę różnicę albo ujednolicić źródło w nowym trybie. Nachylenie surowe ma jednostkę ceny na świecę, więc nie jest bezpośrednio porównywalne między instrumentami i interwałami. Dodatkowo można prezentować nachylenie procentowe/znormalizowane.

### P3 — wypełnienie wykresu

`fill(pl0, pl3b, color=color.red)` wypełnia obszar między krzywą a dolnym pasmem trzeciego poziomu stałym czerwonym kolorem. Nie jest to symetryczne wypełnienie całego kanału i nie ma osobnego ustawienia koloru. W docelowym wykresie wypełnienie powinno być opcjonalne, symetryczne lub jawnie opisane jako dolna strefa.

### P3 — nieużywany parametr funkcji

Parametr `_lrc` w `f_drawLabel()` nie jest wykorzystywany. Można go usunąć w implementacji portowanej.

## 5. Specyfikacja implementacji w Monitorze Rynków

### 5.1. Dane wejściowe

- instrument i dostawca danych;
- interwał źródłowy oraz strefa czasowa;
- źródło ceny: close/open/hl2/hlc3/ohlc4 lub jawnie wskazana seria;
- długość okna regresji;
- tryb kanału: zgodność DGT (`n-1`), błąd standardowy regresji (`n-2`) lub RMS (`n`);
- wybrane poziomy SD, domyślnie 1, 2 i 3;
- drugi kanał: niezależna długość oraz opcjonalny rzeczywisty interwał 1H/4H/1D;
- krzywa: długość, źródło, pasma SD ceny albo ATR;
- nachylenie: długość i źródło;
- wolumen: okres SMA i progi;
- zmienność: okres ATR i mnożnik.

### 5.2. Wyniki obliczeń

Dla każdej świecy/wyliczenia zwracać co najmniej: nachylenie, wartość linii na najnowszej świecy, wartość na początku okna, SSE, wybraną miarę szerokości, górne/dolne poziomy 1/2/3 SD, krzywą, pasma krzywej, ATR, zakres świecy, średni wolumen i status dostępności wolumenu.

Każdy wynik powinien zawierać: instrument, interwał, źródło danych, czas świecy, czas obliczenia, liczbę obserwacji, metodę odchylenia i informację, czy świeca jest zamknięta.

### 5.3. Interfejs

- karta „Regresja” z dwoma kanałami i oddzielnymi ustawieniami;
- przełącznik „Zgodność z DGT” / „Rzeczywisty HTF”;
- jawny wybór metody SD;
- warstwa wykresu: linia bazowa, poziomy ±1/±2/±3 SD, krzywa i pasma;
- panel liczbowy: nachylenie surowe i znormalizowane, szerokość kanału, odległość ceny od linii i bieżący poziom odchylenia;
- warstwa zdarzeń: przecięcia, skoki wolumenu, zakres świecy względem ATR;
- historia zdarzeń z możliwością odtworzenia parametrów i danych źródłowych.

### 5.4. Zasady alertów

Domyślnie alert po zamknięciu świecy. Każde zdarzenie musi mieć klucz deduplikacyjny: instrument + interwał + świeca + typ zdarzenia + poziom + konfiguracja. Alert intrabar jako osobna opcja. Nie formułować sygnału jako pewnej prognozy ani rekomendacji inwestycyjnej.

## 6. Plan testów akceptacyjnych

1. **Seria stała** — nachylenie 0, SSE 0, szerokość kanału 0.
2. **Seria liniowa rosnąca** — nachylenie zgodne z wartością syntetyczną, reszty i szerokość bliskie 0; kierunek zielony.
3. **Seria liniowa malejąca** — analogicznie, kierunek czerwony.
4. **Seria z pojedynczym odstającym punktem** — dodatnie SSE, poprawna symetria reszt i przewidywalna zmiana szerokości.
5. **Minimalne długości** — `n=2` dla zgodności DGT; odrzucenie lub komunikat dla metody `n-2`; brak dzielenia przez zero.
6. **Różne interwały** — porównanie prawdziwego HTF z przeskalowanym oknem; wyniki nie mogą być błędnie oznaczane jako równoważne.
7. **Granice 5000 świec** — walidacja długości przed rozpoczęciem obliczeń, brak odwołań poza historię.
8. **Skrócony drugi kanał** — test początku odcinka potwierdzający poprawkę `showLast-1`.
9. **Luki i brakujące świece** — jawna polityka agregacji i oznaczanie braków.
10. **Brak wolumenu** — brak sygnałów wolumenowych i komunikat o niedostępności danych.
11. **Alerty** — przecięcie intrabar, cofnięcie przed zamknięciem, potwierdzenie na zamknięciu i deduplikacja.
12. **Wydajność** — profilowanie długich okien, obliczenia tylko dla wymaganych punktów, brak zbędnych pętli na każdej świecy historycznej.
13. **Zgodność DGT** — zestawienie wartości z TradingView dla identycznego instrumentu, dostawcy, interwału, źródła i parametrów; różnice wynikające z feedu lub sesji muszą być raportowane.

## 7. Kolejność realizacji

1. Zbudować niezależny moduł obliczeniowy regresji i testy syntetyczne.
2. Odtworzyć tryb zgodności DGT i porównać go z TradingView.
3. Dodać poprawki wydajnościowe oraz walidację długości okna.
4. Dodać prawdziwą agregację HTF jako osobny tryb.
5. Dodać krzywą, pasma SD/ATR, nachylenie, wolumen i zmienność.
6. Zintegrować wykres, panel parametrów, historię i alerty.
7. Przeprowadzić testy akceptacyjne i dopiero potem udostępnić analizę sygnałów.

## 8. Status

To jest audyt kodu źródłowego i specyfikacja realizacji. Nie oznacza, że silnik regresji, wykresy, baza danych lub alerty zostały już zaimplementowane. Do uruchomienia modułu potrzebne są rzeczywiste dane OHLCV, źródło notowań, silnik obliczeniowy i warstwa trwałego zapisu.


## Aktualizacja implementacyjna — 2026-11-12

W repozytorium dodano połączony wskaźnik Pine Script v6: [`tradingview/monitor_combined_linear_regression.pine`](../tradingview/monitor_combined_linear_regression.pine) oraz instrukcję [`tradingview/README.md`](../tradingview/README.md). Implementacja zachowuje odrębne mianowniki i ustawienia profili. Kod został zapisany i ponownie odczytany z gałęzi `main`; nie został jednak skompilowany w TradingView Pine Editor. Audyt nadal opisuje także szerszą docelową integrację backendową, która pozostaje osobnym zadaniem.
