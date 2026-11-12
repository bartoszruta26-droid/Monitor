# Adaptery dostawców danych C++20 — Etap 2

## Status
<badge color="success">Etap 2 — ukończony i zweryfikowany w CI</badge>

Implementacja Etapu 2 w C++20. Ostatni przebieg CI dla kodu: https://github.com/bartoszruta26-droid/Monitor/actions/runs/43250128831 (kompilacja oraz 3/3 testy CTest). Testy adapterów są offline; rzeczywiste zapytania sieciowe nie zostały uruchomione w tej sesji. Adaptery mapują odpowiedzi dostawców na wspólny model market_data.hpp v1. Nie zapisują danych do plików ani bazy — magazyn JSON pozostaje Etapem 3.

## Architektura
- HttpTransport — interfejs transportu, umożliwia testowanie bez sieci.
- CurlHttpTransport — produkcyjny transport libcurl z weryfikacją TLS, bez przekierowań i wyłącznie HTTPS.
- HttpClient — lista dozwolonych hostów, limit rozmiaru odpowiedzi, timeout, minimalny odstęp żądań, ponawianie błędów transportu, HTTP 429 i wybranych 5xx.
- ProviderAdapter — wspólny kontrakt fetch(start, end).
- AdapterResult — instrumenty, dostawcy i obserwacje do dalszego przetwarzania.
- deduplicate_observations — deduplikacja po dostawcy, instrumencie, czasie obserwacji i rodzaju ceny; zachowuje najnowszą wersję pobraną.

## Dostawcy

### NBP
- Tabele A i B: oficjalne kursy średnie, zapisane jako PriceKind::Average, bez udawania ceny transakcyjnej.
- Tabela C: oddzielne rekordy Bid i Ask; nie tworzymy z nich kursu średniego.
- Tabela C używa tradingDate jako czasu obserwacji i effectiveDate jako daty publikacji.
- Złoto: referencyjna cena 1 g próby 1000 w PLN.
- Zakres dzielony na zapytania nie dłuższe niż 93 dni.
Identyfikatory dostawców: nbp-a, nbp-b, nbp-c, nbp-gold.

### Frankfurter v2
- Historyczne kursy z from i to, walutą bazową oraz walutą kwotowaną zwracaną w każdym rekordzie.
- Domyślnie źródło zbiorcze (frankfurter-blend); opcjonalny filtr dostawcy, np. ecb, tworzy odrębną tożsamość szeregu.
- Wartość jest oznaczona jako PriceKind::Mid i flagą frankfurter_mid_market.
- Kursy są dzienne i referencyjne, a nie transakcyjne.

### World Bank Pink Sheet
- Strona docelowa World Bank jest odczytywana w celu znalezienia aktualnego pliku CMO-Historical-Data-Monthly.xlsx; stosowany jest również znany adres awaryjny.
- XLSX jest odczytywany w C++ przez libarchive i libxml2; parser rozpoznaje arkusz Monthly Prices i kolumny Period, Gold, Silver.
- Złoto i srebro są wyrażone w USD za uncję trojańską, jako miesięczne średnie (Frequency::Monthly, PriceKind::Average).
- Nie są tworzone sztuczne dzienne notowania.
- Zakres dat jest filtrowany po parsowaniu skoroszytu.

### XAUS spot
- Dodatkowy dostawca: https://xaus.com/api/v1/spot?compact=1.
- Pobiera bieżące orientacyjne ceny spot złota i srebra; API opisuje je jako indicative mid-market, a nie wykonywalne kwotowania.
- Zachowuje dokładny as_of z odpowiedzi, a nie tylko datę kalendarzową.
- Odpowiedzi oznaczone jako stale są oznaczane QualityStatus::Suspect i flagą provider_stale_snapshot.
- Endpoint może zwrócić tylko XAU lub XAU/XAG; brak XAG nie unieważnia prawidłowego XAU.

## Zasady błędów i odporności
- HTTPS wyłącznie; hosty ograniczone do znanych domen dostawców.
- Brak automatycznego podążania za przekierowaniami.
- Timeout połączenia i całego żądania.
- Ograniczenie rozmiaru odpowiedzi JSON i XLSX.
- Ponawianie błędów transportu, 429 i 500/502/503/504; obsługa liczbowego Retry-After.
- Błędy 4xx inne niż 404 nie są ponawiane. 404 dla zakresu bez notowań jest traktowane jako brak danych.
- Minimalny odstęp pomiędzy żądaniami na instancję klienta.
- Walidacja każdego znormalizowanego rekordu przez walidator modelu domenowego.
- Deduplikacja przed zwróceniem wyniku.

## Zależności budowania
C++20 i CMake 3.20+, libcurl, nlohmann-json, libarchive i libxml2. GitHub Actions instaluje te biblioteki przez pakiety systemowe. Nie są wymagane żadne zależności Python.

## Testy
native/tests/test_market_data_adapters.cpp używa atrap HttpTransport i wzorcowych odpowiedzi JSON. Testuje NBP A/B/C, rozdzielenie bid/ask i tradingDate/effectiveDate, NBP złoto PLN/g, Frankfurter v2 i ponawianie HTTP 503, odczyt XLSX World Bank z archiwum tworzonego w teście, zachowanie miesięcznej częstotliwości, XAUS XAU/XAG i dokładny znacznik czasu, deduplikację oraz odrzucanie HTTP bez TLS i hostów spoza listy.

Testy nie wymagają dostępu do Internetu. Rzeczywiste zapytania do dostawców powinny być wykonywane oddzielnie jako jawne testy integracyjne.

## Ograniczenia i uwagi
- Adaptery zwracają rekordy domenowe; nie zapisują ich jeszcze trwale.
- World Bank publikuje miesięczne dane w skoroszycie XLSX; parser opiera się na nazwach arkusza i kolumn, a nie na stałych numerach wierszy.
- Adres awaryjny pliku World Bank może z czasem przestać wskazywać aktualne wydanie; preferowane jest wykrywanie odnośnika na stronie dostawcy.
- XAUS jest dodatkowym źródłem orientacyjnych cen spot. Należy zachować jego oznaczenie jako indicative mid-market i nie traktować go jak giełdowej książki zleceń.
- Nie wykonuje się przeliczania walut ani zapisu do magazynu.
