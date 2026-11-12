# Agregacja szeregów i świece OHLC — Etap 4

## Kontrakt

Moduł `monitor::market::aggregate_ohlc` agreguje dokładnie jeden szereg zdefiniowany przez instrument, dostawcę, rodzaj ceny i częstotliwość źródłową. Zwraca uporządkowane świece oraz jawne luki pomiędzy pierwszym i ostatnim okresem zawierającym dane.

Świeca zawiera:
- OHLC: pierwszą, najwyższą, najniższą i ostatnią zaobserwowaną wartość;
- granice okresu UTC, przy czym koniec jest wyłączny;
- częstotliwość źródłową i docelową;
- liczbę obserwacji i identyfikatory rekordów źródłowych;
- konserwatywnie scalony status okresu, jakość i zbiór flag jakości.

## Reguły

1. Wejście jest sortowane deterministycznie według czasu obserwacji i identyfikatora.
2. Open/close wynikają z chronologicznie pierwszej/ostatniej obserwacji, a high/low z ekstremów rzeczywiście zaobserwowanych wartości.
3. Agregacja może zachować lub obniżyć częstotliwość, nigdy jej zwiększać. Dane Monthly nie mogą stać się Daily.
4. Brak okresu pomiędzy świecami jest zwracany w `gaps`; nie powstaje świeca z zerową ceną.
5. Wszystkie granice są wyznaczane w UTC. Tydzień zaczyna się w poniedziałek, a okresy miesięczne/kwartalne/półroczne/roczne respektują kalendarz gregoriański.
6. Nie łączy się różnych instrumentów, dostawców, rodzajów cen ani częstotliwości źródłowych.
7. Jakość jest scalana zachowawczo: Suspect dominuje nad Unreviewed, a Unreviewed nad Validated. Provisional dominuje nad Partial i Complete.
8. Flagi jakości i identyfikatory źródłowe pozostają dostępne. Wolumen nie jest wyliczany, ponieważ model źródłowy go nie dostarcza.

## Ograniczenia wersji 1

- Brak kalendarzy sesji giełdowych; luka oznacza brak obserwacji w okresie kalendarzowym, nie dowód braku sesji.
- Brak interpolacji, forward-fill, estymacji wolumenu i konwersji jednostek.
- Tick i Irregular nie są obecnie agregowane.
- Nie ma jeszcze trwałego zapisu świec w JSON. Wynik jest modelem obliczeniowym; rozszerzenie magazynu wymaga osobnej, wersjonowanej decyzji schematu.
- Limit `max_periods` ogranicza koszt generowania luk i świec.

## Testy

Testy offline obejmują OHLC, chronologię, granice miesiąca, tygodnie UTC, jawne luki, dane miesięczne, zakaz upsamplingu, propagację jakości, deterministyczność, limit i izolację szeregów.
