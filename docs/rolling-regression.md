# Regresja krocząca i profile szerokości — Etap 6

## Metody szerokości

- `PopulationRms`: `sqrt(SSE / n)`; zachowuje profil LonesomeTheBlue.
- `SampleSd`: `sqrt(SSE / (n - 1))`; zachowuje profil DGT.
- `ResidualStandardErrorNMinus2`: `sqrt(SSE / (n - 2))`; klasyczny estymator odchylenia resztowego regresji liniowej z dwoma estymowanymi parametrami.

Warianty są odrębne. Nie zmienia się domyślnie ani istniejących profili, ani wartości mnożników kanału. Metoda n-2 wymaga co najmniej trzech obserwacji. Nie jest to automatycznie przedział ufności ani prognozy.

## Regresja krocząca

`rolling_linear_regression(values, window_size, method)` oblicza kolejną regresję dla każdego pełnego okna przesuwającego się o jedną obserwację. Wynik wskazuje indeksy początku i końca okna oraz pełny `RegressionResult`.

Wejście musi być jednym ciągłym segmentem chronologicznym o równomiernych odstępach. Warstwa wywołująca musi rozdzielić serię na segmenty przed lukami i nie może traktować brakujących notowań jako zera ani sąsiadujących obserwacji. Wersja 1 przyjmuje już przygotowane wartości; walidacja rzeczywistych kalendarzowych interwałów HTF i segmentacja szeregów rynkowych pozostają następnym zadaniem.

## Bezpieczeństwo numeryczne

- Nieznana wartość `WidthMethod` jest odrzucana.
- Niepoprawne okno jest odrzucane.
- Wyniki, które nie mieszczą się w `double`, zgłaszają `overflow_error`, zamiast zwracać nieskończoność.
- SSE jest sumowane bezpośrednio z reszt, bez odejmowania bliskich dużych sum.

## Testy

Testy sprawdzają wszystkie trzy mianowniki, niezmienność profili DGT/LTB, minimalną liczność n-2, indeksowanie okien, zgodność ostatniego okna z regresją samodzielną, błędne rozmiary okna, nieznany enum i dane niefinitywne.
