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


## 8. Weryfikacja pierwszego generatora NBP

Pierwsze CI `43302839736` dla commitu `95c53dc2a6a8519119480836d45eab189e69b801` przeszło konfigurację i kompilację modułu produkcyjnego, ale zatrzymało się na kompilacji testu z powodu braku `#include <algorithm>` dla `std::any_of`. Dodano brakujący nagłówek; oczekuje na ponowne CI. Testy nie zostały uruchomione w nieudanym przebiegu.


## 9. Polskie święta ustawowe — generator dat

Dodano `polish_statutory_holidays(year)` w `market_data_calendar`. Funkcja zwraca 14 świąt ustawowych wymienionych w art. 1 pkt 1 ustawy: 10 dat stałych oraz 4 ruchome (Wielkanoc, Poniedziałek Wielkanocny, Zielone Świątki i Boże Ciało). Niedziele są wyłączane osobno przez regułę dni tygodnia generatora NBP. Wigilia 24 grudnia jest uwzględniana od 2025 r.

Podstawa: tekst jednolity ustawy o dniach wolnych od pracy, Dz.U. 2025 poz. 296, art. 1: https://api.sejm.gov.pl/eli/acts/DU/2025/296/text.pdf. Zmiana wprowadzająca 24 grudnia weszła w życie 1 lutego 2025 r.: https://api.sejm.gov.pl/eli/acts/DU/2024/1965/text.html.

Ograniczenie: generator obejmuje ustawowe dni wolne i zwykłe weekendy. Nie obejmuje dodatkowego dnia wolnego ustanowionego odrębnym rozporządzeniem w szczególnym stanie ani ewentualnych wyjątków operacyjnych NBP; takie wyjątki powinny być przekazywane jako dodatkowe daty wyłączenia.


## 10. Automatyczne święta ustawowe w generatorze NBP

Dodano przeciążenie `generate_nbp_publication_windows(start, end_exclusive)`, które automatycznie wylicza ustawowe dni wolne dla wszystkich lat objętych zakresem, a następnie generuje okna A/B/C. Nie wymaga od wywołującego ręcznego przekazywania listy świąt ustawowych. Wariant z jawną listą pozostaje dostępny dla wyjątkowych dni wolnych ustanowionych rozporządzeniem lub dodatkowymi wyjątkami operacyjnymi.

Fixture potwierdza wyłączenie 11 listopada 2026 r., przesunięcie tabeli B na 10 listopada oraz wyłączenie Wigilii 24 grudnia 2025 r. i przesunięcie tabeli B na 23 grudnia.


## 11. Wynik CI — automatyczne święta w generatorze NBP

Commit `c76bf3dc697e93ad3f69bca82a5180e08c338feb` przeszedł [CI 43304025796](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43304025796): konfiguracja, kompilacja i CTest **8/8**. Generator bez jawnej listy świąt automatycznie stosuje polskie święta ustawowe, w tym 11 listopada oraz Wigilię od 2025 r. Wariant z dodatkową listą nadal pozwala przekazać nadzwyczajne dni wolne.

Pozostałe prace etapu 11: wersjonowany rejestr źródłowych kalendarzy i wyjątków Xetra/Frankfurt oraz model kalendarza obserwowanych publikacji NBP.


## 12. Pierwszy model kalendarza faktycznie zaobserwowanych publikacji

Dodano `ObservedPublication` i `build_observed_publications()` w module kalendarza.
Model grupuje rekordy po dostawcy, dacie publikacji zwróconej przez dostawcę,
rzeczywistym czasie pobrania odpowiedzi i URI źródła. Wiele kursów z jednej
odpowiedzi NBP tworzy jedno zdarzenie publikacji z licznikiem obserwacji.
Tabele NBP A/B/C i złoto pozostają odrębnymi dostawcami.

Ważne rozróżnienie semantyczne:
- `publication_date` jest datą cywilną z `provider_published_on`;
- `received_at` jest rzeczywistym czasem odebrania odpowiedzi przez adapter;
- model nie tworzy godziny publikacji NBP z okna 11:45–12:15 lub 07:45–08:15;
- obserwacje bez daty publikacji dostawcy są pomijane;
- nie zmieniono JSON schema v1 ani znaczenia istniejącego `observed_at`.

Testy offline obejmują grupowanie wielu walut w jedną publikację, rozdzielenie tabeli A i złota, pominięcie rekordów bez daty publikacji oraz odrzucenie braku identyfikatora dostawcy.

## 13. NBP — ograniczenie zakresu zapytania i czas odbioru odpowiedzi

Importer `NbpAdapter` dzieli zakres historyczny na rozłączne fragmenty nieprzekraczające 93 dni kalendarzowych (granice obu końców są włączone). Dla zakresu dłuższego niż 93 dni wykonywane są kolejne zapytania, bez nakładania dat i bez pomijania dnia na granicy fragmentów.

Odpowiedź HTTP 404 dla pojedynczego fragmentu jest interpretowana jako brak danych w tym fragmencie i nie przerywa pobierania pozostałych tabel/zakresów. Pusta tablica JSON oznacza poprawną odpowiedź bez rekordów. Żaden z tych przypadków nie tworzy syntetycznych obserwacji.

`received_at` jest teraz pobierany osobno po otrzymaniu każdej poprawnej odpowiedzi HTTP, zamiast raz na początku całego przebiegu `NbpAdapter::fetch`. Dzięki temu różne fragmenty i tabele zachowują własny rzeczywisty czas odebrania odpowiedzi. `provider_published_on` nadal pochodzi wyłącznie z daty zwróconej przez NBP; dla tabeli C `observed_at` zachowuje `tradingDate`, a `provider_published_on` — `effectiveDate`.

Test offline sprawdza zakres 95-dniowy podzielony na fragmenty 93 + 2 dni, wszystkie cztery endpointy (złoto oraz tabele A/B/C), 404 dla pierwszego fragmentu i pustą odpowiedź 200 dla drugiego. Test nie wykonuje połączeń sieciowych.

## 14. Xetra — potwierdzone źródło pierwotne i format

Weryfikacja źródła 2026-11-12:
- Oficjalna strona operatora: https://www.eurexgroup.com/xetra-en/newsroom/trading-calendar
- Alternatywna oficjalna strona Deutsche Börse Cash Market: https://www.cashmarket.deutsche-boerse.com/cash-en/trading/trading-calendar-and-trading-hours
- Oficjalny plik PDF 2026 (kalendarz dla Xetra i Börse Frankfurt): https://www.cashmarket.deutsche-boerse.com/resource/blob/4580826/3e974decb414d6769cc2bc8e601f6c9a/data/xetra-trading-calendar-2026.pdf

Strona podaje regularne godziny Xetra 09:00–17:30 oraz odrębny harmonogram aukcji. Kalendarz dni bez handlu jest prezentowany dla Frankfurter Wertpapierbörse, a następnie osobno wymieniono dni świąteczne, w których Xetra i Börse Frankfurt jednak prowadzą regularny handel. W 2026 r. są to Wniebowstąpienie (14 maja), Zielone Świątki (25 maja) i Boże Ciało (4 czerwca). Nie wolno więc wyprowadzać kalendarza Xetra z niemieckiej listy świąt państwowych ani traktować każdej pozycji tabeli FWB jako automatycznego zamknięcia bez odczytania wyjątków dla Xetra.

PDF jest kalendarzem rocznym w układzie graficznego kalendarza miesięcznego; nie jest tabelą CSV/XLSX o stabilnym schemacie. Strona HTML zawiera dodatkowo tabelę dni bez handlu i osobne listy dni z handlem w święta. To rozdzielenie jest istotne dla parsera: wyjątki handlowe muszą mieć pierwszeństwo nad ogólną regułą świąteczną. Godziny aukcji i wyjątkowe skrócenia sesji wymagają osobnego źródła/komunikatu, nie mogą być wyprowadzane z samego PDF kalendarza.

Strefa sesji w modelu aplikacji powinna być zapisana jako `Europe/Berlin`, a nie stałe `CET`, aby poprawnie obsłużyć CEST i przejścia DST. Strona podaje godziny w terminologii CET, lecz sesja lokalna musi podążać za lokalną strefą giełdy.

Data publikacji PDF nie została potwierdzona w widocznym dokumencie/stronie, a warunki licencji ponownego rozpowszechniania danych kalendarza nie zostały jednoznacznie określone. Nie zapisujemy więc fikcyjnej daty publikacji, nie deklarujemy licencji open data i nie wyliczamy SHA-256 bez pobrania dokładnych bajtów pliku. Atrybucja powinna wskazywać Deutsche Börse; przed dystrybucją kopii źródłowego PDF należy sprawdzić warunki prawne.

**Decyzja implementacyjna:** pierwsza wersja importera powinna wykorzystywać semantyczną tabelę HTML oficjalnej strony (dni bez handlu + jawne dni handlu w święta) albo jawny, kontrolowany plik źródłowy z potwierdzonym schematem. Nie parsować układu graficznego PDF heurystycznie. Rejestr źródła ma przechowywać URL, zakres roku, strefę `Europe/Berlin`, czas pobrania, SHA-256 rzeczywiście pobranej zawartości, status licencji/atrybucji i numer wersji. Zmiana dokumentu tworzy nową wersję.

## 15. Wersjonowany model dokumentu źródłowego i fixture Xetra 2026

Rdzeń udostępnia `SourceCalendarDocument` z identyfikatorem dokumentu, venue, wersją, półotwartym zakresem dat, strefą IANA, adresem HTTPS, opcjonalnym czasem pobrania, opcjonalnym SHA-256 dokładnych pobranych bajtów, atrybucją i statusem licencji. Walidator odrzuca niepoprawny zakres, brak atrybucji, nie-HTTPS i niepoprawny zapis SHA-256.

Puste `content_sha256` oraz brak `retrieved_at` są dozwolone wyłącznie jako jawny stan niezweryfikowanego źródła; kod nie tworzy sumy kontrolnej z URL ani tekstu po parsowaniu. Status licencji pozostaje `Unknown`, dopóki warunki nie zostaną potwierdzone.

`build_xetra_2026_calendar` tworzy 365-dniowy fixture w strefie `Europe/Berlin`, z odrębnymi statusami `Closed`, `Trading` i `SettlementOnly`. Uwzględnia zamknięcia 1 stycznia, Wielki Piątek, Poniedziałek Wielkanocny, 1 maja i 25 grudnia; 24 i 31 grudnia oznacza jako rozliczeniowe bez handlu. Dla 14 maja, 25 maja i 4 czerwca zapisuje jawne wyjątki handlowe: akcje/ETP do 20:00 oraz produkty strukturyzowane do 22:00. Dzień 30 grudnia jest oznaczony `requires_official_notice`, ponieważ operator zastrzega możliwość odmiennych godzin i publikacji okólnika.

To jest wersjonowany fixture transkrybowany z oficjalnego źródła, a nie jeszcze automatyczny importer HTML/PDF. Wersja 1 nie przypisuje fikcyjnego czasu pobrania, sumy SHA-256 ani statusu licencji. Parser aktualizujący fixture musi w kolejnej iteracji pobrać dokładne bajty oficjalnej strony/dokumentu, policzyć SHA-256, zapisać czas pobrania i wykrywać zmiany semantyczne bez nadpisywania poprzedniej wersji.

### 15.1. SHA-256 surowych bajtów

Rdzeń udostępnia `sha256_hex(std::string_view)`, który oblicza małymi literami SHA-256 z dokładnej sekwencji bajtów przekazanej przez warstwę pobierania. Testy używają standardowych wektorów dla pustego wejścia, `abc` i 1000 znaków `a`. Importer powinien wywoływać tę funkcję przed dekodowaniem/parsowaniem odpowiedzi, aby suma identyfikowała oryginalny dokument, a nie jego znormalizowaną reprezentację tekstową.


## 16. Importer semantycznej tabeli HTML Xetra — pierwsza wersja

Dodano funkcję `parse_xetra_html_calendar` w `native/src/xetra_calendar_importer.cpp`. Korzysta z parsera HTML libxml2, bez wykonywania skryptów i bez dostępu sieciowego. Funkcja przyjmuje bajty dokumentu jako `string_view`; warstwa pobierająca ma obliczyć `sha256_hex` na niezmienionym buforze przed dekodowaniem i przekazać wynik w `SourceCalendarDocument.content_sha256`.

Importer:
- wymaga jawnego dokumentu XETRA, zakresu 2026 i strefy IANA `Europe/Berlin`;
- odczytuje kolumnę 2026 i osiem nazwanych wierszy świąt z semantycznej tabeli HTML;
- odrzuca brakujące/zdublowane wiersze, nieznane lub zmienione daty oraz niekompletną listę trzech świąt z handlem;
- rozpoznaje zarówno zapis miesiąc–dzień, jak i dzień–miesiąc;
- rozróżnia zamknięcie, dzień rozliczeniowy oraz handel w święto z odrębnymi godzinami dla grup produktów;
- oznacza 30 grudnia jako wymagający sprawdzenia oficjalnego okólnika.

Fixture `native/tests/fixtures/xetra_2026_calendar.html` jest lokalnym, syntetycznym odwzorowaniem struktury semantycznej źródła. Testy nie wykonują żądań sieciowych. Nie jest to zrzut bajtów pobrany z Deutsche Börse, dlatego hash fixture służy wyłącznie testowi kontraktu parsera, a nie jako hash oficjalnego dokumentu.

**Ograniczenie zakresu:** pierwsza implementacja obsługuje schemat strony i rok 2026. Nie pobiera HTML, nie archiwizuje automatycznie kolejnych wersji i nie rozstrzyga licencji. Zmiana układu tabeli lub etykiet ma zakończyć import błędem, a nie wygenerować pozornie poprawny kalendarz. Importer nie odczytuje skróconych sesji z okólnika końcoworocznego; pozostają one osobnym źródłem i wymagają jawnej wersji dokumentu.
