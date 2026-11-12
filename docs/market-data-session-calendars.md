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

## Magazyn i wersjonowanie

Kalendarze są częścią modelu domenowego oraz migawki i partii repozytorium JSON. Kluczem jest para `calendar_id + version`. Opublikowanej wersji nie można zmienić ani usunąć przez zapis partii lub podmianę migawki. Zmiana harmonogramu wymaga nowego, większego numeru wersji, a starsze wersje pozostają dostępne do odtworzenia analiz. Dokumenty schema v1 bez pola `session_calendars` są odczytywane jako pusta lista.

Wyniki regresji sesyjnej zachowują `calendar_id` i `calendar_version`, dzięki czemu pochodzenie obliczenia wskazuje dokładny harmonogram.

## Odpowiedzialność za kalendarz

Moduł nie zgaduje świąt, godzin sesji ani reguł czasu letniego. Dostawca kalendarza musi przekazać konkretne terminy w UTC po rozstrzygnięciu lokalnej strefy czasowej i wyjątków rynkowych. Zmiana czasu letniego powinna być odzwierciedlona w terminach UTC wygenerowanych dla każdej sesji. Wersja kalendarza powinna być stabilna i audytowalna.

Pole `timezone` przechowuje identyfikator strefy użyty do przygotowania slotów; znaczniki czasu pozostają w UTC. Walidacja składni nie potwierdza obecności strefy w lokalnej bazie IANA. Wersja v1 jest harmonogramem slotów, a nie generatorem reguł cyklicznych. Nie ma jeszcze gotowych kalendarzy giełdowych dla poszczególnych instrumentów ani pobierania oficjalnych świąt. Brak kalendarza nie uprawnia do automatycznego uznania szeregu za kompletny.

## Ważne ograniczenie

Kalendarz musi definiować dokładne oczekiwane czasy obserwacji zgodne z czasami zwracanymi przez adapter źródłowy. Dla dziennych danych referencyjnych może to być jeden slot na dzień publikacji. Dla danych minutowych harmonogram powinien zawierać wszystkie oczekiwane sloty danej sesji. Rozwiązanie nie interpoluje brakujących wartości.


## Generator IANA TZ/DST

Moduł `market_data_calendar` generuje sloty z lokalnych reguł tygodniowych przy użyciu bazy stref ICU. Zakres dat jest półotwarty `[start_date, end_date_exclusive)`. Można wskazać ISO dni tygodnia, wiele godzin lokalnych i jawne daty wyłączeń. `AmbiguousLocalTimePolicy` rozstrzyga powtórzoną godzinę jesienną; `NonexistentLocalTimePolicy` obsługuje godzinę pominiętą wiosną. Domyślne ustawienia odrzucają oba przypadki, aby harmonogram nie był generowany z ukrytym założeniem. Wartości wynikowe są zapisywane jako UTC i mogą zostać przekazane do `upsert_session_calendar()`.
