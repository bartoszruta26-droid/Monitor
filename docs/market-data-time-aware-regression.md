# Regresja krocząca na rzeczywistych interwałach HTF

## Cel

Moduł `market_data_regression` łączy obserwacje rynkowe z istniejącą agregacją okresów i silnikiem regresji. Obliczenia pozostają deterministyczne i nie zależą od interfejsu użytkownika.

## Kontrakt

Funkcja `time_aware_rolling_regression` przyjmuje jeden szereg obserwacji, docelową częstotliwość HTF, rozmiar okna, metodę szerokości i politykę wyboru próbki.

- Dozwolone są wyłącznie częstotliwości regularne obsługiwane przez `aggregate_ohlc`.
- Docelowy interwał nie może być krótszy od źródłowego.
- Na jeden okres HTF przypada jedna wartość regresji: pierwsza albo ostatnia rzeczywista obserwacja.
- Domyślną polityką jest `LastObservation`, zgodna z wartością zamknięcia świecy próbkowanej.
- Czas wybranej próbki pozostaje oryginalnym czasem obserwacji; granice okna są granicami kalendarzowymi UTC.
- Każdy wynik zawiera identyfikatory wszystkich obserwacji źródłowych z okresów objętych oknem.
- Brakujący okres HTF rozdziela segmenty. Okno nigdy nie przekracza luki.
- Nie są tworzone ceny, znaczniki czasu ani obserwacje syntetyczne.

## Semantyka luk

Segmentacja jest wykonywana na poziomie docelowego interwału. Brak całego tygodnia w szeregu tygodniowym lub całego miesiąca w szeregu miesięcznym rozdziela segmenty. Brak pojedynczej sesji wewnątrz tygodnia nie jest automatycznie traktowany jako brak całego tygodnia. Moduł nie zakłada kalendarza świąt ani godzin sesji giełdowej.

## Metody statystyczne

Wynik korzysta z istniejącego `WidthMethod`:
- `PopulationRms`: `sqrt(SSE/n)`, profil LonesomeTheBlue;
- `SampleSd`: `sqrt(SSE/(n-1))`, profil DGT;
- `ResidualStandardErrorNMinus2`: `sqrt(SSE/(n-2))`, klasyczny błąd standardowy reszt.

Nie zmieniono definicji żadnego istniejącego profilu. Wariant n-2 wymaga okna co najmniej trzech okresów.

## Ograniczenia

- Tick i Irregular są odrzucane.
- Nie ma jeszcze kalendarza sesji, uwzględniania świąt ani reguł instrument-specyficznych.
- Dla danych dziennych agregowanych do tygodnia ciągłość oznacza obecność sąsiednich tygodni, a nie pełną liczbę sesji w każdym tygodniu.
- Wynik nie jest prognozą ani prawdopodobieństwem transakcji.
