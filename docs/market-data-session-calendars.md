# Kalendarze sesyjne i kompletność okresów HTF

## Kontrakt

`MarketSessionCalendar` reprezentuje jawny harmonogram oczekiwanych znaczników obserwacji dla jednego instrumentu, dostawcy i częstotliwości źródłowej. Harmonogram jest podawany w UTC, rosnąco i bez duplikatów.

`session_aware_rolling_regression`:
- przypisuje oczekiwane sloty do okresów HTF za pomocą wspólnego agregatora;
- uznaje okres za kompletny wyłącznie wtedy, gdy każda zaplanowana obserwacja istnieje, ma `PeriodStatus::Complete` i `QualityStatus::Validated`;
- przerywa segment przy brakującej, podejrzanej, niezatwierdzonej, częściowej lub prowizorycznej obserwacji;
- pomija dni i okresy bez zaplanowanych sesji, więc weekend ani święto wyłączone z kalendarza nie tworzą sztucznej luki;
- odrzuca obserwacje spoza kalendarza i dane innego instrumentu, dostawcy lub częstotliwości;
- zachowuje źródłowe identyfikatory obserwacji i rzeczywisty czas próbki.

## Odpowiedzialność za kalendarz

Moduł nie zgaduje świąt, godzin sesji ani reguł czasu letniego. Dostawca kalendarza musi przekazać konkretne terminy w UTC po rozstrzygnięciu lokalnej strefy czasowej i wyjątków rynkowych. Zmiana czasu letniego powinna być odzwierciedlona w terminach UTC wygenerowanych dla każdej sesji. Wersja kalendarza powinna być stabilna i audytowalna.

Wersja v1 jest harmonogramem slotów, a nie generatorem reguł cyklicznych. Nie ma jeszcze gotowych kalendarzy giełdowych dla poszczególnych instrumentów ani pobierania oficjalnych świąt. Brak kalendarza nie uprawnia do automatycznego uznania szeregu za kompletny.

## Ważne ograniczenie

Kalendarz musi definiować dokładne oczekiwane czasy obserwacji zgodne z czasami zwracanymi przez adapter źródłowy. Dla dziennych danych referencyjnych może to być jeden slot na dzień publikacji. Dla danych minutowych harmonogram powinien zawierać wszystkie oczekiwane sloty danej sesji. Rozwiązanie nie interpoluje brakujących wartości.
