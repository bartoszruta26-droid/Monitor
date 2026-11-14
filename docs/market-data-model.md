# Wspólny model danych rynkowych — wersja 1

## Cel i granice
Model danych rynkowych jest niezależnym kontraktem domenowym C++20. Nie zależy od dostawców, HTTP, JSON, SQLite, MariaDB ani interfejsów. Adapter mapuje dane źródłowe do modelu; repozytorium danych rynkowych przechowuje obserwacje, a osobne moduły portfela wykorzystują ten model do wyceny.
Implementacja: native/include/monitor/market_data.hpp oraz native/src/market_data.cpp. Wersja kontraktu: schema_version = 1.

## Zakres domenowy

Model jest używany w pierwszej kolejności dla par walutowych FX oraz złota XAU. Waluty wymagają jawnej orientacji pary (waluta bazowa/kwotowana), a złoto — jednostki masy i waluty wyceny. Zachowujemy rozróżnienie kursu referencyjnego, bid, ask, mid, spot i ceny rzeczywiście wykonanej transakcji. Cena referencyjna NBP/Frankfurter ani orientacyjny spot nie mogą być przedstawiane jako gwarantowana cena, po której użytkownik kupi lub sprzeda walutę albo złoto.

Akcje, udziały, papiery wartościowe i giełdowe instrumenty udziałowe są poza zakresem produktu. Ogólne typy modelu nie stanowią zobowiązania do ich obsługi.

## Instrumenty i ceny
Instrument przechowuje stabilny identyfikator, symbol, nazwę, klasę aktywa, opcjonalną walutę bazową, walutę wyceny, jednostkę ilościową i jawne zezwolenie na ujemne ceny.
Para walutowa ma dwie różne waluty. Kurs EUR/PLN = 4,25 oznacza 4,25 PLN za 1 EUR. Dla metali i surowców waluta bazowa jest pusta, a waluta wyceny określa mianownik ceny: np. XAU w USD za uncję trojańską albo złoto NBP w PLN za gram.
PriceKind rozróżnia Reference, Average, Bid, Ask, Mid, Spot i Transaction. Środek wyliczony z bid/ask nie jest oficjalnym kursem średnim. Kurs referencyjny nie jest ceną transakcyjną.

## Jednostki
Wersja 1 obejmuje CurrencyUnit, Gram, Kilogram, TroyOunce, MetricTon, Barrel i MegawattHour. Konwersje masy są deterministyczne: 1 uncja trojańska = 31,1034768 g, 1 kg = 1000 g. Nie ma automatycznej konwersji walut ani baryłek na masę; wymagają osobnej reguły.

## Czas
UtcTimestamp przechowuje sekundy Unix w UTC. Parser akceptuje ISO-8601 z obowiązkowym Z lub przesunięciem ±HH:MM, normalizuje do UTC i akceptuje część ułamkową, którą w wersji 1 obcina do pełnej sekundy. Czas lokalny bez jawnej strefy jest odrzucany.
MarketObservation przechowuje osobno observed_at (czas notowania), received_at (czas pobrania) i provider_published_on (lokalną datę publikacji dostawcy). Data publikacji nie jest przesuwana do UTC.

## Częstotliwość, kompletność i jakość
Częstotliwość obejmuje tick, minutę, godzinę, dzień, tydzień, miesiąc, kwartał, półrocze, rok i dane nieregularne. Częstotliwość źródłowa pozostaje zachowana; nie tworzymy dziennych obserwacji z danych miesięcznych.
PeriodStatus rozróżnia okres kompletny, częściowy i wstępny. Brakujące okresy są luką w szeregu, a nie zerową ceną. SeriesMetadata przechowuje przedział, częstotliwość, oczekiwaną i otrzymaną liczbę obserwacji oraz kompletność: Complete, Partial, Sparse lub Unknown.
QualityStatus rozróżnia dane niezweryfikowane, zweryfikowane i podejrzane. quality_flags zawiera konkretne uwagi, np. derived_mid, delayed lub provider_revision.

## Dostawca i pochodzenie
Provider przechowuje stabilny identyfikator, nazwę, stronę główną i informację licencyjną. Każde notowanie ma provider_id. Opcjonalne provider_record_id i source_uri pozwalają odtworzyć pochodzenie. Adresy w wersji 1 muszą używać HTTPS. Sekrety nie należą do modelu.

## Walidacja i wersjonowanie
Walidatory odrzucają błędne identyfikatory, kody walut, pary FX z identyczną walutą bazową i kwotowaną, metale bez jednostki masy, nieznane wartości enum (klasa aktywa, jednostka, częstotliwość, rodzaj ceny, status okresu, jakość i kompletność), nieobsługiwane wersje schematu, rozbieżne identyfikatory, NaN i nieskończoności, niedozwolone ceny ujemne, czas pobrania wcześniejszy niż obserwacja, błędne daty i URI oraz sprzeczne liczniki kompletności.
Ujemne ceny są dozwolone wyłącznie po jawnym ustawieniu allow_negative_prices. Nie zakładamy dodatniości dla wszystkich rynków.
MarketObservation zawiera jawny numer schematu. Zmiany niekompatybilne wymagają podniesienia wersji i migracji w przyszłej warstwie magazynu.

## Powiązane moduły portfela FX/XAU
Model rynkowy v1 pozostaje oddzielony od ewidencji transakcji. Moduł portfolio przechowuje zakupy i sprzedaże walut oraz złota, a repozytorium portfolio zapisuje je w osobnym, wersjonowanym dokumencie JSON. Moduł portfolio_valuation buduje ceny w walucie raportowania na podstawie MarketDataSnapshot, stosując bid przy sprzedaży waluty bazowej, odwrotność ask dla par odwrotnych, kursy krzyżowe oraz normalizację złota do grama czystego kruszcu. Zachowuje źródła, czas obserwacji, rodzaje cen i flagi jakości. Szczegóły implementacji i ograniczenia opisuje docs/IMPLEMENTATION_STATUS.md.

## Poza zakresem wersji 1
Model rynku nie implementuje REST/WebSocket, UI, automatycznego harmonogramu pobierania, handlu giełdowego ani kalendarzy sesji papierów wartościowych. Trwały zapis obserwacji rynkowych i portfela realizują odrębne repozytoria JSON. Kalendarze giełdowe i akcje pozostają poza zakresem produktu.
