# Audyt funkcjonalny i plan realizacji — Monitor Rynków

**Status dokumentu:** obowiązujący audyt kierunkowy  
**Data aktualizacji:** 2026-11-12  
**Gałąź docelowa:** `main`  
**Zasada technologiczna:** aktywny rozwój rdzenia, serwera, adapterów i obliczeń w C++20 oraz Bash; bez Pythona w aktywnym procesie budowania, testowania i uruchamiania.

Ten dokument jest punktem odniesienia dla kolejnych prac. Zmiany architektury i kolejności realizacji należy z nim uzgadniać oraz aktualizować go po istotnych decyzjach.



## Obowiązujący zakres produktu — decyzja 2026-11-13

**Priorytetem Monitora są wymiana walut (FX) oraz zakup, sprzedaż i wymiana złota.** Cały rozwój funkcjonalny, modelowanie domenowe, źródła danych, wykresy, analizy, portfel i transakcje należy podporządkować tym dwóm zastosowaniom.

Zakres obejmuje pary walutowe, kursy referencyjne i dostępne kwotowania bid/ask, przeliczanie walut z jawnym spreadem i opłatami, wycenę złota (XAU) w różnych walutach i jednostkach (gram, kilogram, uncja trojańska), ewidencję fizycznego złota oraz rozróżnienie ceny referencyjnej/spot od rzeczywistej ceny kupna lub sprzedaży.

**Poza zakresem są akcje spółek, instrumenty udziałowe, giełdowe zlecenia kupna/sprzedaży akcji, arkusze zleceń, brokerzy akcyjni oraz funkcje i kalendarze sesji giełd papierów wartościowych.** Nie planować, nie rozwijać ani nie testować tych funkcji. W szczególności dalsze prace nad Xetra/Frankfurt, MIC XETR/XFRA, aukcjami giełdowymi i kalendarzami sesji akcji są wyłączone. Istniejący kod i historia Xetra pozostają w repozytorium wyłącznie jako zapis wcześniejszych prac; nie są kryterium ukończenia produktu ani zależnością kolejnych etapów.

Srebro i inne surowce nie są priorytetem bieżącej realizacji. Można zachować istniejące, współdzielone typy lub adaptery bez rozbudowy; nowe prace nad nimi wymagają osobnej decyzji. Nie wolno przez nie opóźniać funkcji FX i złota.

Wskaźniki regresji i Pine/TradingView mogą być rozwijane wyłącznie jako narzędzia analizy szeregów walutowych i złota. Nie stanowią uzasadnienia do przywracania zakresu akcji ani giełdowego obrotu papierami wartościowymi.

## 1. Cel i priorytety produktu

Monitor Rynków ma przede wszystkim obsługiwać wymianę walut oraz zakup, sprzedaż i wymianę złota: pobierać aktualne i historyczne kursy FX oraz ceny XAU, prezentować je na wykresach, analizować metodami DGT/LonesomeTheBlue i prowadzić portfel walutowo-złotowy wraz z kosztami i wynikiem operacji.

Priorytety funkcjonalne, w kolejności:
1. Pobieranie aktualnych i historycznych danych z otwartych źródeł.
2. Trwały zapis danych źródłowych w JSON (domyślnie).
3. Agregacja i wybór okresów: dzienny, tygodniowy, miesięczny, kwartalny, półroczny, roczny.
4. Wykresy cen, świec i agregatów.
5. Integracja profili regresji DGT i LonesomeTheBlue z Pine Script.
6. Ewidencja własnych aktywów oraz operacji kupna, sprzedaży i wymiany.
7. Analiza czterech ćwiartek regresji i kalibrowane prawdopodobieństwo wyniku transakcji.
8. Serwer wymiany danych i integracje Nextcloud/ChatGPT.
9. Cztery klienty: Linux GUI, WebUI, TUI i Android.
10. Opcjonalny magazyn MariaDB i rozszerzenia skalowalności.

Infrastruktura ma wspierać powyższe funkcje, a nie odwracać ich kolejność. Nie należy zaczynać od rozbudowy interfejsów kosztem działającego przepływu danych.

## 2. Audyt obecnego repozytorium

### 2.1. Elementy istniejące

- Rdzeń C++20 z regresją liniową OLS, SSE i szerokością kanału.
- Zachowane metody DGT: `sqrt(SSE/(n-1))` oraz LonesomeTheBlue: `sqrt(SSE/n)`.
- Program CLI i testy jednostkowe regresji.
- Skrypty Bash do budowania i testowania.
- GitHub Actions dla natywnego C++.
- Połączony wskaźnik Pine Script v6 i dokumentacja audytów obu metod.
- Dokumentacja docelowej architektury serwera, JSON i MariaDB.

### 2.2. Elementy niezaimplementowane

- Adaptery pobierania danych rynkowych.
- Magazyn JSON i adapter MariaDB.
- Agregacja okresowa i model świec.
- Serwer REST/WebSocket.
- Wykresy aplikacji i kroczące obliczenia regresji.
- Portfel, transakcje i wycena.
- Model prawdopodobieństwa zysku i testy kroczące.
- Integracja Nextcloud oraz serwer MCP dla ChatGPT.
- Linux GUI, WebUI, TUI i Android App.

Obecny `native/CMakeLists.txt` buduje moduł regresji, CLI i testy. Dokumentacja lub przykładowa konfiguracja nie oznacza, że funkcja jest już działająca. Status należy zmieniać dopiero po implementacji i testach.

## 3. Źródła danych rynkowych

### 3.1. Planowane źródła

| Źródło | Zakres | Ograniczenia i zasady |
|---|---|---|
| NBP API | Tabele A/B — kursy średnie; tabela C — kupno/sprzedaż; złoto PLN/g | Dane publikowane w dniach roboczych; pobieranie historii partiami zgodnie z limitami API |
| Frankfurter API | Referencyjne kursy walut i szeregi historyczne | Nie jest notowaniem transakcyjnym w czasie rzeczywistym; zachować datę i walutę bazową |
| World Bank Pink Sheet | Historyczne ceny złota (XAU) | Dane miesięczne; zachować częstotliwość miesięczną; srebro i inne surowce nie są priorytetem |
| Dostawca spot złota | Orientacyjna cena XAU w czasie zbliżonym do rzeczywistego i historia, jeśli dostępna | Odróżnić mid-market od wykonywalnych cen kupna/sprzedaży; ocenić spread, opóźnienie, jednostkę, licencję i retencję |

Dokumentacja źródeł:
- NBP: https://api.nbp.pl/
- Frankfurter: https://api.frankfurter.dev/
- World Bank Commodity Markets: https://www.worldbank.org/en/research/commodity-markets

Adaptery muszą jawnie oznaczać źródło, częstotliwość i typ ceny. Kurs referencyjny, bid, ask, mid, spot i cena transakcyjna to odrębne pojęcia. Nie wolno wyliczać pozornej ceny kupna/sprzedaży z samego kursu średniego bez oznaczenia jej jako wartości pochodnej.

### 3.2. Model obserwacji

Każda obserwacja powinna przechowywać co najmniej:
- stabilny identyfikator, identyfikator instrumentu i symbol;
- typ aktywa, jednostkę i walutę kwotowania;
- wartość referencyjną oraz dostępne pola bid/ask/mid/open/high/low/close;
- czas obserwacji u dostawcy i czas pobrania;
- nazwę dostawcy, identyfikator źródła i adres/identyfikator serii;
- częstotliwość źródłową, strefę czasową i status zamknięcia okresu;
- jakość, kompletność, ewentualne opóźnienie i wersję schematu.

Dla kruszców należy rozróżniać co najmniej gram, kilogram i uncję trojańską. Konwersje jednostek muszą być jawne i testowane. Nie wolno mieszać ceny za gram z ceną za uncję.

## 4. JSON i agregacja czasowa

JSON jest domyślnym magazynem. MariaDB pozostaje opcjonalnym adapterem tego samego kontraktu repozytorium.

Proponowany podział katalogów:
```text
data/
  market/currencies/
  market/metals/
  market/commodities/
  aggregates/daily/
  aggregates/weekly/
  aggregates/monthly/
  aggregates/quarterly/
  aggregates/half_yearly/
  aggregates/yearly/
  portfolio/
  analytics/
  metadata/
```

Wymagania magazynu:
- wersjonowane schematy i migracje;
- zapis atomowy, kopie bezpieczeństwa i odzyskiwanie po przerwanym zapisie;
- blokady lub kontrola wersji przy równoczesnym dostępie;
- idempotentny import i deduplikacja;
- zachowanie surowych danych źródłowych;
- możliwość odtworzenia agregatów i analiz;
- brak bezpośredniego, niekontrolowanego zapisu z wielu klientów do tego samego pliku.

### 4.1. Okresy

Użytkownik ma móc wybrać: dzienny, tygodniowy, miesięczny, kwartalny, półroczny i roczny.

### 4.2. Metody agregacji

Oddzielnie udostępnić:
- średnią arytmetyczną;
- średnią ważoną, wyłącznie gdy istnieje uzasadniona waga;
- medianę;
- OHLC: open, high, low, close;
- ostatnią dostępną obserwację okresu.

Średnia okresowa nie jest świecą OHLC. Agregaty są odrębnymi rekordami pochodnymi, powiązanymi z zakresem dat i wersją algorytmu. Brak notowania w weekend lub święto nie oznacza ceny równej zero ani uprawnienia do wymyślenia obserwacji.

## 5. Wykresy i integracja Pine Script

Należy zachować dwa odrębne profile metodologiczne:

| Cecha | DGT | LonesomeTheBlue |
|---|---|---|
| Domyślne okna | 200 i 50 | 377, 233 i 144 |
| Szerokość | `sqrt(SSE/(n-1))` | `sqrt(SSE/n)` |
| Poziomy | ±1, ±2, ±3 SD | Fibo 0,236 / 0,382 / 0,618 / 0,786 |
| Dodatki | krzywa, nachylenie, wolumen/ATR | wybicia i ślad wybitych kanałów |
| HTF | osobny tryb, nie utożsamiać skalowania z prawdziwym HTF | osobny tryb, jeśli zostanie dodany |

Nie wolno ujednolicać mianowników. Należy zachować poprawione indeksowanie reszt LonesomeTheBlue. Wartości z Pine Script trzeba porównywać z C++ na identycznym instrumencie, dostawcy, interwale, źródle ceny i zamkniętych świecach.

Silnik wykresów powinien obsługiwać:
- świece i serie cenowe;
- agregaty i średnie okresowe;
- linie regresji i kanały;
- poziomy SD i Fibo;
- oznaczenia wybić i zdarzeń;
- transakcje kupna/sprzedaży/wymiany;
- historię prognoz oraz porównanie prognozy z późniejszym wynikiem.

Obliczenia kroczące powinny tworzyć historię wyników dla każdej świecy, a nie tylko pojedynczy wynik ostatniego okna. Dane i wyniki muszą zawierać parametry, wersję algorytmu i identyfikator źródła.

## 6. Portfel i transakcje

Monitor ma ewidencjonować aktywa użytkownika, ilości, jednostki, ceny nabycia, waluty, daty i miejsca przechowywania. Historia operacji jest niezmienna i pozwala odtworzyć stan portfela na dowolny dzień.

Rodzaje operacji:
- kupno;
- sprzedaż;
- wymiana walut;
- wymiana kruszców;
- przewalutowanie;
- opłaty i prowizje;
- korekta z uzasadnieniem.

Każda transakcja powinna przechowywać ilość oddaną i otrzymaną, aktywa, kurs, cenę jednostkową, walutę rozliczenia, opłaty, czas, kontrahenta/miejsce (jeżeli użytkownik poda), źródło wpisu i identyfikator użytkownika.

System ma obliczać koszt nabycia, średnią cenę nabycia według jawnie wybranej metody rozliczenia, wartość bieżącą, wynik zrealizowany i niezrealizowany, ekspozycję walutową oraz wynik netto. Nie należy automatycznie zakładać metody podatkowej ani księgowej — musi być konfigurowalna i opisana.

### 6.1. Przyszła integracja ewidencji transakcji z inną aplikacją

W przyszłości część funkcji związanych z ręcznym wpisywaniem zakupów, sprzedaży i wymiany aktywów ma zostać udostępniona również w innej aplikacji użytkownika, rozwijanej w osobnym repozytorium GitHub.

Jest to funkcja przyszłościowa, nie zakres bieżącego etapu. Już teraz należy jednak:
- wydzielić model transakcji od interfejsu Monitora;
- zapewnić stabilne, wersjonowane API do tworzenia, odczytu i korekty transakcji;
- przewidzieć autoryzację aplikacji zewnętrznych i uprawnienia ograniczone do konkretnych operacji;
- obsłużyć idempotency keys, aby ponowione żądanie nie dublowało zakupu lub sprzedaży;
- zapisywać źródło/klienta, który wprowadził operację;
- nie uzależniać modelu domenowego od konkretnego repozytorium ani ekranu.

Nie należy teraz implementować drugiej aplikacji ani jej interfejsu. Należy przygotować kontrakt integracyjny, aby późniejsze podłączenie nie wymagało przebudowy portfela.

## 7. Regresja, ćwiartki i prawdopodobieństwo wyniku

Cztery stany regresji należy definiować przez znak nachylenia i znormalizowane odchylenie ceny od linii:
`s = slope`, `r = (price - fitted_price) / residual_width`.

| Ćwiartka | Warunek | Opis |
|---|---|---|
| I | s > 0, r > 0 | Trend rosnący, cena powyżej regresji |
| II | s < 0, r > 0 | Trend malejący, cena powyżej regresji |
| III | s < 0, r < 0 | Trend malejący, cena poniżej regresji |
| IV | s > 0, r < 0 | Trend rosnący, cena poniżej regresji |

To autorska macierz stanów regresji, a nie klasyczne ćwiartki kartezjańskie. Przy zerowym nachyleniu, zerowej szerokości lub niewystarczającej liczbie obserwacji należy zwrócić stan neutralny/brak klasyfikacji.

Położenie ceny w kanale lub historyczny centyl reszt nie jest samo w sobie prawdopodobieństwem zysku. Prawdopodobieństwo należy estymować z historycznych analogicznych sytuacji, osobno dla instrumentu, profilu, okresu, horyzontu i kierunku transakcji.

Wymagane horyzonty konfigurowalne, np. 1, 5, 10, 20 i 60 obserwacji. Model powinien raportować:
- liczbę analogicznych przypadków;
- prawdopodobieństwo dodatniego wyniku brutto i netto;
- oczekiwany wynik, medianę i rozrzut wyników;
- ryzyko i maksymalne niekorzystne odchylenie;
- przedział niepewności;
- datę ostatniej kalibracji, wersję modelu i wyniki walidacji poza próbą.

Testy kroczące muszą wykluczać look-ahead bias i survivorship bias. W obliczeniach wymiany należy uwzględnić rzeczywisty bid/ask, spread, prowizje i inne koszty. Przy zbyt małej próbie system ma pokazać brak wiarygodnej estymacji, a nie sztuczny procent.

## 8. Nextcloud — przyszła aplikacja i warstwa integracyjna

Docelowo Monitor ma być możliwy do udostępnienia jako aplikacja Nextcloud. Nie należy uzależniać rdzenia C++ od kodu Nextcloud ani od wewnętrznych tabel jego bazy.

Zalecany kierunek:
- osobna aplikacja Nextcloud zgodna z publicznym API i cyklem życia aplikacji Nextcloud;
- integracja z plikami przez WebDAV/Files API, a z funkcjami platformy przez OCS API;
- uwierzytelnianie i uprawnienia zgodne z użytkownikami Nextcloud;
- opcjonalny zewnętrzny komponent obliczeniowy/serwer C++ komunikujący się z aplikacją Nextcloud przez wersjonowane API;
- pakowanie, testy zgodności, metadane aplikacji, tłumaczenia i proces wydania zgodny z wymaganiami Nextcloud App Store.

Wariant implementacyjny do oceny na późniejszym etapie:
1. cienka aplikacja Nextcloud (PHP/Vue/JavaScript zgodnie z API platformy) odpowiada za ekran, uprawnienia, ustawienia i integrację z plikami;
2. silnik C++20 pozostaje niezależnym procesem/usługą;
3. komunikacja odbywa się przez HTTPS i wersjonowane API;
4. jeśli potrzebne będzie wdrożenie jako zewnętrznej aplikacji Nextcloud, ocenić AppAPI/ExApp i jego wymagania wdrożeniowe.

Nie używać prywatnych klas Nextcloud ani bezpośrednio modyfikować wewnętrznych tabel jego bazy. Nie zakładać, że aplikacja C++ może być bez zmian spakowana jako standardowa aplikacja PHP Nextcloud.

## 9. Integracja ChatGPT / MCP i przepływ danych

Przyszła aplikacja ChatGPT ma umożliwiać odczytywanie dozwolonych danych, wykonywanie analiz AI i zapisywanie wyników prognoz do wskazanej przestrzeni Nextcloud lub kontrolowanego katalogu JSON.

Docelowy model:
1. ChatGPT komunikuje się z dedykowanym serwerem MCP Monitora przez uwierzytelniony kanał.
2. Serwer MCP pobiera dane przez API Monitora i/lub kontrolowaną integrację Nextcloud.
3. Model AI analizuje dane i wywołuje jawnie udostępnione narzędzia obliczeniowe.
4. Wynik jest zapisywany jako wersjonowany rekord prognozy/analizy w repozytorium Monitora, a następnie eksportowany/synchronizowany do Nextcloud lub plików JSON użytkownika.
5. Android App, TUI, Linux GUI i WebUI odczytują dane oraz prognozy z uzgodnionego źródła synchronizacji.

Narzędzia MCP należy rozdzielić na:
- tylko do odczytu: lista instrumentów, notowania, agregaty, portfel w zakresie uprawnień, historia analiz;
- obliczeniowe: regresja, agregacja, scenariusze, porównanie aktywów, kalkulacja kosztów transakcji;
- zapisujące: utworzenie prognozy, zapis raportu, eksport do wskazanego pliku;
- transakcyjne: ewentualne tworzenie/korekta transakcji, domyślnie wyłączone lub wymagające jawnego potwierdzenia.

ChatGPT nie powinien otrzymywać nieograniczonego dostępu do dysku ani bazy. Dostęp do plików lokalnych tabletu wymaga kontrolowanego pośrednika/klienta z uprawnieniami systemowymi; zdalny serwer ChatGPT nie może samodzielnie odczytywać prywatnej ścieżki plików Androida. Preferowanym kanałem synchronizacji jest Nextcloud WebDAV/Files API lub dedykowane API klienta.

Wymagania bezpieczeństwa:
- OAuth 2.1/PKCE lub mechanizm zgodny z wybraną integracją;
- minimalne zakresy uprawnień i rozdzielenie odczytu od zapisu;
- jawne zatwierdzanie działań zmieniających dane portfela;
- szyfrowanie transmisji i bezpieczne przechowywanie tokenów;
- dziennik audytowy działań AI;
- walidacja danych wejściowych i wyników;
- oznaczanie prognoz jako wygenerowanych przez AI, wraz z modelem, datą, danymi wejściowymi, wersją algorytmu i horyzontem;
- zapis prognoz jako nowych rekordów, bez nadpisywania źródłowych notowań i historii transakcji.

Ważne rozróżnienie: AI może interpretować dane i proponować scenariusze, ale krytyczne obliczenia matematyczne (regresja, agregacja, wycena i wynik transakcji) powinny być wykonywane przez deterministyczny silnik C++ i weryfikowalne testami. AI nie może zastępować tych obliczeń swobodnym szacowaniem.

## 10. Docelowy przepływ danych i źródła prawdy

Docelowy przepływ:
```text
Otwarte źródła rynkowe
        |
        v
Adaptery C++ -> walidacja i normalizacja
        |
        v
Serwer Monitora / API / repozytorium JSON (domyślnie)
        |                       |
        |                       +--> silnik C++: agregacja, regresja,
        |                       |    portfel, scenariusze i walidacja
        |                       |
        +--> Nextcloud (Files/WebDAV/OCS)
        |          |
        |          +--> pliki JSON/raporty/wersjonowane prognozy
        |          |
        |          +--> aplikacja Nextcloud
        |
        +--> serwer MCP dla ChatGPT
                   |
                   +--> odczyt uprawnionych danych
                   +--> obliczenia narzędziami Monitora
                   +--> zapis zatwierdzonych prognoz/raportów

Android / TUI / Linux GUI / WebUI
        |
        +--> API Monitora i/lub kontrolowana synchronizacja Nextcloud
        +--> odczyt wspólnych rekordów i prognoz
```

Należy unikać dwóch niezależnych, równorzędnych źródeł prawdy. Rekomendacja: serwer/repozytorium Monitora pozostaje autorytatywnym źródłem danych domenowych; Nextcloud jest warstwą synchronizacji, plików, udostępniania i integracji. Jeżeli użytkownik wybierze tryb plikowy/offline, synchronizacja musi mieć jawne wersje, identyfikatory, znaczniki czasu, idempotencję i rozwiązywanie konfliktów. Nie wolno dopuszczać do cichego nadpisywania nowszych danych starszym plikiem.

## 11. Cztery interfejsy

- Linux GUI: Qt 6/C++, klient wspólnego API.
- WebUI: TypeScript/Vue lub inny zgodny stos webowy, klient wspólnego API.
- TUI: C++/FTXUI lub ncurses, klient wspólnego API.
- Android: Qt/C++ albo Kotlin/JNI, klient API oraz kontrolowana pamięć podręczna i synchronizacja.

Każdy interfejs ma prezentować te same notowania, agregaty, wyniki regresji, portfel i prognozy. Nie wolno duplikować logiki finansowej w klientach. Tryb offline musi odróżniać dane zsynchronizowane od lokalnych zmian oczekujących na synchronizację.

## 12. Kolejność prac i kryteria akceptacji

### Etap 1 — modele domenowe i adaptery źródeł
- modele instrumentów, notowań, jednostek, źródeł i jakości;
- adapter NBP, Frankfurter i World Bank;
- ocena i wybór źródła cen spot złota (XAU);
- pobieranie aktualnych i historycznych danych, paginacja i limity;
- testy odpowiedzi wzorcowych i błędów sieci.

**Akceptacja:** rzeczywiste odpowiedzi źródeł są poprawnie normalizowane i nie gubią pochodzenia ani jednostek.

### Etap 2 — JSON Storage
- interfejs repozytorium;
- wersjonowany zapis JSON;
- atomowość, kopie, blokady, deduplikacja;
- test odtworzenia danych po restarcie.

**Akceptacja:** pobrane dane pozostają dostępne po zamknięciu i ponownym uruchomieniu aplikacji.

### Etap 3 — agregacja i harmonogram pobierania
- sześć okresów;
- metody średniej, mediany i OHLC;
- harmonogram, retry, limity i status aktualności;
- odtwarzanie agregatów z danych źródłowych.

**Akceptacja:** wyniki okresowe są zgodne z niezależnymi testami referencyjnymi.

### Etap 4 — API serwera
- REST/JSON, OpenAPI i WebSocket;
- endpointy danych, agregatów, źródeł i statusu;
- uwierzytelnianie, uprawnienia, audyt i idempotencja.

**Akceptacja:** klient zewnętrzny może pobrać dane bez bezpośredniego dostępu do magazynu.

### Etap 5 — wykresy
- świece, ceny, agregaty i średnie;
- wybór okresu i zakresu;
- test zgodności wykresu z danymi JSON.

**Akceptacja:** wartości na wykresie odpowiadają rekordom źródłowym i agregatom.

### Etap 6 — integracja regresji
- obliczenia kroczące DGT i LTB;
- poziomy, Fibo, wybicia i HTF;
- porównanie z Pine/TradingView;
- wersjonowanie parametrów i wyników.

**Akceptacja:** porównanie na identycznych danych i ustawieniach mieści się w określonej tolerancji numerycznej.

### Etap 7 — portfel walutowo-złotowy i transakcje
- aktywa i pozycje;
- kupno, sprzedaż, wymiana i opłaty;
- wycena i wynik netto;
- API przygotowane do przyszłej aplikacji w innym repozytorium.

**Akceptacja:** da się odtworzyć stan portfela i wynik każdej operacji na dowolną datę.

### Etap 8 — prawdopodobieństwo i walidacja
- macierz ćwiartek;
- historyczne analogie i horyzonty;
- testy kroczące, kalibracja i koszty;
- przedziały niepewności.

**Akceptacja:** model raportuje wyniki poza próbą i ujawnia liczebność próby; nie przedstawia niezweryfikowanych szacunków jako prawdopodobieństwa.

### Etap 9 — Nextcloud i MCP
- projekt aplikacji Nextcloud;
- integracja WebDAV/Files/OCS i uwierzytelnianie;
- serwer MCP z ograniczonymi narzędziami;
- zapis wersjonowanych analiz i prognoz;
- synchronizacja i rozwiązywanie konfliktów.

**Akceptacja:** ChatGPT może odczytać tylko zatwierdzone dane i zapisać prognozę w dozwolonym miejscu; klienty widzą zatwierdzony wynik po synchronizacji.

### Etap 10 — MariaDB i klienci
- adapter MariaDB;
- testy zgodności JSON/MariaDB;
- Linux GUI, WebUI, TUI i Android;
- pakowanie, testy i dokumentacja.

**Akceptacja:** wszystkie klienty używają wspólnego API i pokazują spójne dane.

## 13. Zasady jakości i bezpieczeństwa

- C++20 i Bash dla aktywnego rdzenia, serwera, adapterów, obliczeń i testów; bez Pythona.
- Każda funkcja ma test jednostkowy i integracyjny odpowiedni do ryzyka.
- Testy obejmują brakujące dane, duplikaty, błędy dostawców, zmiany czasu, jednostki i precyzję dziesiętną.
- Pieniądze i ilości aktywów nie powinny być przechowywane w typach zmiennoprzecinkowych bez przemyślanej reprezentacji; rozważyć typ dziesiętny lub całkowite jednostki najmniejsze.
- Zachować pełną proweniencję danych.
- Nie ujawniać sekretów w konfiguracji, logach ani repozytorium.
- Dostęp do plików i transakcji ograniczać zakresem uprawnień.
- Zmiany portfela i operacje AI zapisujące dane muszą być audytowalne.
- Nie utożsamiać prognozy z gwarancją zysku ani poradą inwestycyjną.

## 14. Dokumentacja referencyjna integracji

- Nextcloud Developer Manual: https://docs.nextcloud.com/server/stable/developer_manual/
- Nextcloud App Development: https://docs.nextcloud.com/server/stable/developer_manual/app_development/intro.html
- Nextcloud OCS API: https://docs.nextcloud.com/server/stable/developer_manual/client_apis/OCS/ocs-api-overview.html
- Nextcloud WebDAV / Files API: https://docs.nextcloud.com/server/stable/developer_manual/client_apis/
- OpenAI Apps SDK: https://developers.openai.com/apps-sdk/
- OpenAI MCP servers: https://developers.openai.com/api/docs/guides/tools-connectors-mcp
- OpenAI MCP authentication: https://developers.openai.com/plugins/build/auth

Wytyczne platform i API należy ponownie sprawdzić przed implementacją, ponieważ mogą się zmieniać.

## 15. Aktualny status tego audytu

Ten dokument zapisuje kierunek i kryteria przyszłych prac. Nie oznacza, że pobieranie danych, magazyn JSON, serwer, Nextcloud, MCP, wykresy, portfel lub model prawdopodobieństwa są już zaimplementowane. Każdy z tych elementów wymaga osobnego wdrożenia i testów.

**Najbliższe zadanie implementacyjne:** Etap 1 — modele domenowe i adaptery źródeł danych rynkowych.
