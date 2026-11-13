# Zakres produktu Monitor Rynków — decyzja wiążąca

**Data:** 2026-11-13  
**Status:** obowiązująca decyzja produktowa  
**Repozytorium:** `bartoszruta26-droid/Monitor`

## Cel

Monitor służy przede wszystkim do:

1. **Wymiany walut (FX)** — porównywania kursów, przeliczania kwot, uwzględniania kursów bazowych/kwotowanych, bid/ask, spreadu, prowizji i opłat oraz rejestrowania wymiany.
2. **Zakupu, sprzedaży i wymiany złota (XAU)** — wyceny w różnych walutach i jednostkach masy, ewidencji ilości i próby, porównywania ceny referencyjnej/spot z rzeczywistą ofertą kupna/sprzedaży oraz rejestrowania operacji.

## Zakres funkcjonalny

- Źródła FX: NBP (tabele A/B/C), Frankfurter i inne zatwierdzone źródła o jawnej metodologii.
- Źródła złota: NBP (PLN/g), źródła spot XAU oraz historyczne dane miesięczne, jeśli zachowują właściwą częstotliwość.
- Jednostki złota: gram, kilogram, uncja trojańska; jawne i testowane przeliczenia.
- Waluty rozliczeniowe i wycena XAU w PLN oraz innych walutach.
- Rozróżnienie kursu/ceny referencyjnej, średniej, bid, ask, mid, spot i rzeczywistej ceny transakcji.
- Spread, prowizje, opłaty, marża punktu wymiany i inne koszty — bez ukrywania ich w kursie źródłowym.
- Historia i portfel walutowo-złotowy, koszt nabycia, wartość bieżąca, wynik zrealizowany i niezrealizowany.
- Wykresy, agregacja, regresja DGT/LonesomeTheBlue i prognozy dla szeregów FX/XAU, z oznaczeniem częstotliwości, źródła i niepewności.
- Wspólny serwer/API i klienci są warstwą dostępu do powyższych funkcji, a nie osobnym celem wyprzedzającym działający przepływ danych.

## Poza zakresem — nie realizować

- Akcje spółek, udziały i inne giełdowe papiery wartościowe.
- Giełdowe funkcje handlu akcjami, zlecenia, arkusze zleceń, egzekucja zleceń i integracje brokerów akcyjnych.
- Kalendarze sesji giełd papierów wartościowych, aukcje giełdowe i reguły venue dla akcji.
- Importery i dalsza rozbudowa kalendarzy Xetra/Frankfurt, MIC XETR/XFRA oraz komunikatów giełdowych. Kod i dokumentacja tych wcześniejszych prac mogą pozostać w repozytorium jako historia, ale nie są elementem bieżącego produktu, kryterium akceptacji ani zależnością kolejnych etapów.

## Zasady priorytetyzacji

1. Najpierw poprawność importu, aktualność i jakość kursów FX oraz cen XAU.
2. Następnie wymiana walut i złota z prawidłowym bid/ask, spreadem, opłatami i walutą rozliczenia.
3. Następnie portfel, historia operacji, wycena i wynik netto.
4. Następnie wykresy, agregacja i analiza/regresja dla FX/XAU.
5. Następnie API, synchronizacja i interfejsy, które udostępniają te funkcje.
6. Srebro i inne surowce są drugorzędne; zachowanie istniejącej obsługi jest dopuszczalne, ale nie mogą opóźniać zakresu FX/XAU. Nowa rozbudowa wymaga osobnej decyzji.

## Reguły semantyczne i jakościowe

- Kurs EUR/PLN oznacza liczbę PLN za 1 EUR; kierunek każdej pary musi być widoczny.
- Kurs referencyjny lub orientacyjny spot nie jest gwarantowaną ceną wykonania wymiany.
- Cena złota musi zawsze wskazywać jednostkę masy i walutę wyceny.
- Nie wolno wyliczać ceny kupna/sprzedaży bez jawnej metody i oznaczenia jej jako pochodnej.
- Koszty i spread mają być pokazane osobno oraz uwzględnione w wyniku netto.
- Dane źródłowe zachowują dostawcę, czas obserwacji/publikacji/odbioru, częstotliwość, jednostkę i status jakości.
- Nie dopisywać godzin publikacji ani częstotliwości, których źródło nie potwierdza.

## Dokumenty podporządkowane

W razie sprzeczności ten dokument ma pierwszeństwo przed starszymi opisami zakresu. Należy synchronizować z nim:
- `README.md`
- `docs/architecture.md`
- `docs/market-data-model.md`
- `docs/market-monitor-audit-and-roadmap.md`
- `docs/next-implementation-audit.md`
- `docs/market-calendar-sources.md`
- `docs/IMPLEMENTATION_STATUS.md`
