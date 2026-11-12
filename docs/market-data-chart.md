# Model danych wykresów — Etap 5

## Zakres

Moduł `monitor::market` dostarcza niezależny od GUI model wykresów liniowych, obszarowych i świecowych. Jest to kontrakt danych dla przyszłych klientów Linux Desktop, WebUI, TUI i Android; nie jest jeszcze rendererem graficznym.

## Zasady

- Wykres punktowy zachowuje wartości, czasy, jakość, status okresu i identyfikator źródłowy.
- Punkty są sortowane deterministycznie według UTC i identyfikatora.
- Każda seria zawiera dokładnie jeden instrument, dostawcę, rodzaj ceny i częstotliwość źródłową.
- Wykres świecowy przyjmuje jawny wynik `aggregate_ohlc`; nie oblicza własnej, rozbieżnej agregacji.
- Luki pozostają osobnymi zakresami czasu. Konsument wykresu nie może zastępować ich zerem ani łączyć linii przez lukę.
- Limity punktów i świec chronią pamięć oraz czas renderowania.
- Moduł nie zależy od biblioteki GUI ani sieci.

## Ograniczenia

- Brak gotowego renderera, interakcji, zoomu, kursora i osi; to zakres klientów.
- Brak zapisu wykresów lub świec w repozytorium JSON.
- Wykres punktowy nie interpoluje i nie uzupełnia brakujących obserwacji.
