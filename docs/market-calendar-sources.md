# Etap 11 — źródła i semantyka kalendarzy rynkowych

**Status:** rozpoznanie źródeł zakończone; implementacja importerów i modeli w toku.
**Repozytorium:** bartoszruta26-droid/Monitor
**Data przeglądu:** 2026-11-12

## 1. Cel

Etap 11 ma dostarczyć kalendarze, które poprawnie określają oczekiwane obserwacje w analizie HTF. Nie wolno utożsamiać trzech różnych pojęć:
1. **Kalendarz publikacji dostawcy** — kiedy źródło publikuje określony zbiór danych.
2. **Kalendarz sesji rynku/instrumentu** — kiedy konkretny instrument/venue prowadzi handel, z uwzględnieniem przerw, aukcji, skróconych sesji i wyjątków.
3. **Kalendarz obserwowanych publikacji** — daty i czasy faktycznie zwrócone przez dostawcę w określonym zakresie historycznym.

Każdy kalendarz musi mieć identyfikator, wersję, zakres obowiązywania, strefę czasową, źródło, czas pobrania, czas wejścia w życie oraz ślad audytowy. Korekta wymaga nowej wersji; opublikowana wersja pozostaje niezmienna.

## 2. Źródła pierwotne i ograniczenia

### NBP — tabele A, B i C

Oficjalna informacja NBP podaje: tabela A — każdy dzień roboczy, okno aktualizacji 11:45–12:15; tabela B — środa, a gdy środa jest dniem wolnym — poprzedni dzień; tabela C — każdy dzień roboczy, okno aktualizacji 7:45–8:15.

Źródło: https://nbp.pl/statystyka-i-sprawozdawczosc/kursy/informacja-o-terminach-publikacji-kursow-walut/

NBP Web API udostępnia tabele A/B/C i ceny złota; dla tabeli C rozdziela tradingDate od effectiveDate. Zapytania dla dat bez danych zwracają 404. Zakres pojedynczego zapytania historycznego jest ograniczony do 93 dni.

Źródło: https://api.nbp.pl/

**Wnioski implementacyjne:**
- Nie wpisywać godziny 12:00 lub 08:00 jako rzeczywistego czasu obserwacji tylko dlatego, że NBP publikuje w przedziale czasowym.
- Dla dziennych danych NBP zachować datę publikacji/notowania i semantykę istniejącego adaptera; jeśli model potrzebuje technicznego znacznika dnia, jawnie opisać go jako kotwicę daty, nie timestamp publikacji.
- Tabela C musi zachować odrębnie dzień notowania i dzień publikacji.
- Kalendarz historyczny z dat faktycznie zwróconych przez API oznaczać jako observed, nie jako kompletną prognozę przyszłych sesji.
- Reguła tabeli B wymaga listy oficjalnych dni wolnych, by rozstrzygnąć przesunięcie środy; nie implementować tego jako każda środa ani nie zgadywać świąt.
- Ceny złota NBP są dzienną serią dat publikacji; dokumentacja API nie deklaruje dokładnej godziny. Nie przenosić na nie okna tabeli A lub C.

### World Bank — Pink Sheet

Oficjalna strona Commodity Markets udostępnia pliki Monthly Prices i Annual Prices. Pink Sheet jest źródłem miesięcznym dla złota/srebra i innych surowców.

Źródło: https://www.worldbank.org/en/research/commodity-markets.

Utrzymać Frequency::Monthly; nie generować dziennych slotów, godzin sesji ani weekendowych wyłączeń dla danych miesięcznych. Zachować datę/okres miesięczny i rewizje skoroszytu jako pochodzenie danych.

### XAUS — orientacyjne ceny spot

Dokumentacja API podaje, że spot jest odświeżany ciągle, ale cache ma 60 sekund; endpoint intraday udostępnia próbki co 5 minut z retencją 14 dni. Dostawca określa ceny jako indicative mid-market, nie jako wykonywalne kwotowania; status stale musi pozostać widoczny.

Źródło: https://xaus.com/api/.

To feed OTC/spot, a nie kalendarz konkretnej giełdy. W pierwszej iteracji nie wyznaczać świąt giełdowych ani nie zakładać nieprzerwanej dostępności 24/7. Kalendarz może opisywać częstotliwość obserwacji/świeżość i retencję, ale sloty oczekiwane muszą wynikać z udokumentowanego kontraktu dostawcy i realnej polityki pobierania.

### Deutsche Börse — Xetra / Frankfurt

Oficjalny kalendarz określa godziny Xetra i Frankfurt oraz dni bez handlu, a także wskazuje wybrane święta, w których handel jednak się odbywa, i odrębne zasady dla końca roku.

Źródło: https://www.cashmarket.deutsche-boerse.com/cash-en/trading/trading-calendar-and-trading-hours.

Kalendarz giełdowy musi być importowany per venue i rok/wersja. Nie wyprowadzać go z ogólnokrajowej listy świąt. Model docelowy powinien obsługiwać normalne sesje, dni bez handlu, skrócone sesje, aukcje i wyjątki, a także datę publikacji/aktualizacji oficjalnego pliku.

## 3. Proponowany model importu

1. **SourceCalendarDocument** — surowy/źródłowy dokument kalendarza z URL, wersją, datą publikacji, skrótem SHA-256 i metadanymi licencji.
2. **CalendarRuleSet** — reguły powtarzalne, lokalna strefa IANA, dni tygodnia i lokalne przedziały sesji.
3. **CalendarException** — jawne zamknięcie, skrócenie, przesunięcie, dodatkowa sesja lub wyjątek instrumentu.
4. **ObservedPublicationCalendar** — daty/timestampy potwierdzone odpowiedzią dostawcy; nie udaje kalendarza przyszłego.
5. **ExpectedObservationSlot** — oczekiwany punkt/zakres publikacji z rodzajem czasu (data publikacji, data notowania, timestamp sesji) i pochodzeniem reguły.

Obecny MarketSessionCalendar jest listą oczekiwanych znaczników UTC i nie przechowuje jeszcze pełnego semantycznego rodzaju czasu, źródłowego dokumentu ani sesji przedziałowej. Rozszerzenie należy wykonać z zachowaniem zgodności JSON v1 i przez jawne wersjonowanie/migrację — bez reinterpretacji istniejących timestampów.

## 4. Kolejność implementacji

1. Dodać typy domenowe dla rodzaju kalendarza i rodzaju znacznika czasu; nie zmieniać semantyki istniejącego expected_observation_times bez migracji.
2. Dodać importerskie parsery NBP z fixture dla A/B/C i gold API; osobno testować 404, limity 93 dni, tradingDate vs effectiveDate, brak/nowe waluty.
3. Dodać importerski format oficjalnego kalendarza venue (najpierw Xetra jako fixture oparty na opublikowanym pliku/źródle); zachować skrócone sesje i wyjątki świąteczne.
4. Dodać observed-calendar builder z oznaczeniem kompletności i źródła; nie używać go jako przyszłego expected calendar.
5. Dodać testy DST, granic sesji, skróconych dni, świąt z handlem, świąt bez handlu, rewizji kalendarza i migracji JSON.
6. Dopiero po przejściu testów podłączyć kalendarze do kompletności regresji HTF.

## 5. Kryteria akceptacji

- Każdy kalendarz ma typ, venue/provider/instrument, strefę IANA, zakres, wersję i pochodzenie.
- Brak danych, dzień wolny, awaria dostawcy i brakujący instrument są rozróżniane.
- Godziny publikacji jako przedziały nie są zamieniane na sztuczny timestamp.
- Kalendarze giełdowe są niezależne od kalendarzy NBP, OTC spot i miesięcznych danych World Bank.
- Wyjątki oraz skrócone sesje są zachowane.
- Importer ma deterministyczne fixture offline; CI kompiluje i uruchamia wszystkie testy.
- Zmiana kalendarza tworzy nową wersję, bez nadpisania już opublikowanej.

## 6. Otwarte decyzje

- Wybór pierwszego formatu źródłowego Xetra: oficjalny plik roczny vs oficjalna tabela HTML; preferować plik do pobrania z kontrolą SHA-256, jeżeli dostępny.
- Rozstrzygnięcie czy MarketSessionCalendar zostaje rozszerzony, czy dodany zostaje odrębny ProviderPublicationCalendar.
- Semantyka dziennych timestampów NBP w obecnym modelu: kotwica daty UTC kontra lokalna data publikacji; nie zmieniać bez testów migracyjnych.
- Dla XAUS: brak oficjalnego harmonogramu godzin handlu w API; na razie modelować wyłącznie świeżość i obserwowaną częstotliwość, nie kalendarz sesyjny.

## 7. Pierwsza implementacja NBP — okna publikacji

Kod: `NbpPublicationWindow`, `NbpPublicationTable`, `generate_nbp_publication_windows()` w `native/include/monitor/market_data_calendar.hpp` i `native/src/market_data_calendar.cpp`.

- Tabele A i C: dni robocze z wyłączeniem przekazanych świąt; A 11:45–12:15, C 07:45–08:15.
- Tabela B: środy 11:45–12:15; gdy środa jest na liście dni wolnych, publikacja przypada poprzedniego dnia roboczego.
- Wszystkie okna są lokalnymi przedziałami Europe/Warsaw. Nie są przeliczane na `observed_at`; dokładny czas publikacji pochodzi z rzeczywistej odpowiedzi NBP.
- Generator nie zawiera własnej listy świąt. Wywołujący przekazuje listę z wiarygodnego, wersjonowanego źródła.
- Fixture: 2026-11-11 jako środa wolna; B przesunięta na 2026-11-10, A/C pomijają 11 listopada.
