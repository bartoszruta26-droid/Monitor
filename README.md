<div align="center">

# ✦ MONITOR RYNKÓW ✦
### System gromadzenia danych, analizy regresji i oceny portfela aktywów

**Waluty • Złoto • Srebro • Metale • Dane historyczne • Analiza techniczna • Ryzyko i prawdopodobieństwo**

*Projekt rozwijany w duchu prawdy, sprawiedliwości, odpowiedzialności i miłości Bożej.*

[![Status](https://img.shields.io/badge/status-rozwój%20aktywny-blue)](#-status-projektu)
[![Platform](https://img.shields.io/badge/platform-Python%20%7C%20FastAPI-009688)](#-architektura)
[![Data](https://img.shields.io/badge/data-NBP%20%7C%20Frankfurter%20%7C%20World%20Bank-4169e1)](#-źródła-danych)
[![License](https://img.shields.io/badge/licencja-do%20ustalenia-lightgrey)](#-licencja-i-warunki-wykorzystania)

</div>

---

## ✧ O projekcie

**Monitor Rynków** jest rozwijanym systemem do gromadzenia, porządkowania, wizualizacji i analizy danych rynkowych, ze szczególnym uwzględnieniem aktywów walutowych oraz metali szlachetnych. Jego celem jest zapewnienie jednego, audytowalnego środowiska danych, w którym notowania pochodzące z różnych dostawców są zachowywane wraz z informacją o źródle, czasie, jednostce, walucie, częstotliwości i rodzaju ceny.

Projekt ma łączyć trzy obszary:

1. **Rzetelne dane** — import danych historycznych i bieżących z jawnie wskazanych źródeł, kontrola jakości, wykrywanie luk i zachowanie pochodzenia każdej obserwacji.
2. **Analiza techniczna** — wykresy, regresje liniowe, kanały odchyleń i zdarzenia rynkowe, z oddzielnymi profilami metodologicznymi DGT oraz LonesomeTheBlue.
3. **Analiza portfela** — ewidencja posiadanych aktywów, ich wycena w walucie bazowej, analiza ekspozycji i modelowanie rozkładu możliwych wyników.

Monitor ma pomagać w rozumieniu danych i konsekwencji decyzji. Nie ma zastępować samodzielnej oceny, niepewności rynkowej ani odpowiedzialności właściciela aktywów.

> **Zasada nadrzędna:** żadna liczba nie powinna być przedstawiana bez informacji, skąd pochodzi, czego dotyczy, w jakiej jednostce została wyrażona i jak została obliczona.

## ✧ Wartości i zasady projektowe

Rozwój oprogramowania opiera się na czterech wzajemnie powiązanych zasadach:

- **Prawda** — rozdzielanie danych źródłowych od danych przeliczonych, estymacji, założeń i prognoz. Brak danych nie może być maskowany wartością pozorną.
- **Sprawiedliwość** — jednakowe reguły przetwarzania dla porównywalnych instrumentów, jawne ograniczenia metod i możliwość odtworzenia obliczeń.
- **Odpowiedzialność** — ochrona danych i sekretów, kontrola dostępu, rejestrowanie operacji administracyjnych, testowanie zmian oraz ostrożne komunikowanie ryzyka.
- **Miłość i służba** — tworzenie narzędzia, które wspiera roztropność, uczciwość, pokój i odpowiedzialne gospodarowanie powierzonymi dobrami.

Wartości te mają odzwierciedlenie w praktyce inżynierskiej: jawności metod, dokumentacji, testach, kontroli wersji i unikaniu nieuzasadnionych obietnic.

---

## ✧ Główne cele

### 1. Dane rynkowe

- import historycznych kursów walut;
- import historycznych cen złota i srebra;
- zachowanie danych dziennych i miesięcznych bez sztucznego ujednolicania częstotliwości;
- możliwość dodawania kolejnych dostawców i własnych źródeł;
- wykrywanie luk, duplikatów, opóźnień i niezgodności jednostek;
- ponawianie importu bez wielokrotnego tworzenia tych samych obserwacji.

### 2. Analiza techniczna

- wykresy cen i zmian procentowych;
- szeregi OHLCV tam, gdzie źródło rzeczywiście udostępnia takie dane;
- linie regresji liniowej;
- kanały regresji i pasma odchyleń;
- nachylenie trendu, szerokość kanału i odległość ceny od linii;
- osobne profile DGT i LonesomeTheBlue;
- historia obliczeń i zdarzeń z możliwością odtworzenia parametrów.

### 3. Portfel aktywów

- ewidencja posiadanych walut, złota i srebra;
- ilość, jednostka, data nabycia i cena nabycia;
- wycena według wybranego źródła i daty;
- wartość portfela w walucie bazowej;
- wynik niezrealizowany, koszt nabycia i zmiana wartości;
- udział poszczególnych aktywów oraz koncentracja ekspozycji;
- scenariusze zmiany kursów i cen metali.

### 4. Model prawdopodobieństwa wyniku

Docelowo Monitor będzie szacował prawdopodobieństwo wystąpienia określonych klas zysku, uwzględniając położenie ceny względem linii regresji, rozkład reszt, zmienność, horyzont oraz rzeczywistą strukturę portfela.

Wyniki będą rozdzielały:
- historyczną częstość obserwacji w danym przedziale;
- modelowe prawdopodobieństwo przyszłego scenariusza;
- potencjalny wynik kwotowy dla określonego portfela;
- niepewność estymacji i jakość dostępnych danych.

---

## ✧ Źródła danych

### Narodowy Bank Polski — NBP

Oficjalne źródło kursów walut i cen złota.

- tabele A i B — kursy średnie;
- tabela C — kursy kupna i sprzedaży;
- dla tabeli C środek między bid i ask jest wartością pochodną, a nie oficjalnym kursem średnim;
- cena złota — cena referencyjna za 1 gram złota próby 1000, wyrażona w PLN;
- API: https://api.nbp.pl/

Zakres historyczny i ograniczenia zapytań należy respektować zgodnie z dokumentacją NBP. Import powinien dzielić długie zakresy na dopuszczalne fragmenty i zapisywać datę efektywną notowania.

### Frankfurter

Otwarte API kursów walutowych, udostępniające kursy średnie rynku.

- kurs identyfikowany przez datę, walutę bazową i walutę kwotowaną;
- możliwość wyboru waluty bazowej i listy walut docelowych;
- możliwość wskazania dostawcy danych, jeżeli API udostępnia taki filtr;
- kurs jest zapisywany jako referencyjny kurs średni rynku, a nie kurs transakcyjny konkretnego banku;
- API: https://api.frankfurter.dev/

### World Bank Pink Sheet

Źródło historycznych cen surowców i metali.

- arkusz miesięcznych cen;
- złoto i srebro;
- wartości referencyjne w USD za uncję trojańską;
- częstotliwość miesięczna;
- strona źródłowa: https://www.worldbank.org/en/research/commodity-markets

Plik źródłowy może zmieniać adres lub wersję. Integracja powinna wykrywać aktualny plik na stronie World Bank, zamiast bezwarunkowo polegać na jednym, niezmiennym adresie.

### Zasady wspólnego modelu danych

Dane pochodzące z różnych źródeł nie są automatycznie równoważne. W szczególności:

| Instrument / seria | Źródło | Waluta | Jednostka | Częstotliwość |
|---|---|---|---|---|
| Kursy walut | NBP A/B | PLN jako baza notowania | jednostka waluty | dzienna |
| Kursy walut | NBP C | PLN jako baza notowania | jednostka waluty | dzienna |
| Kursy walut | Frankfurter | wskazana waluta bazowa | jednostka waluty | dzienna |
| Złoto | NBP | PLN | gram | dzienna |
| Złoto | World Bank | USD | uncja trojańska | miesięczna |
| Srebro | World Bank | USD | uncja trojańska | miesięczna |

**Nie wolno** traktować miesięcznej średniej World Bank jako dziennej świecy OHLC. Nie wolno również utożsamiać ceny NBP za gram złota w PLN z ceną World Bank za uncję trojańską w USD bez jawnego przeliczenia jednostki i waluty.

---

## ✧ Architektura

Docelowy przepływ danych:

```text
             DOSTAWCY DANYCH
      NBP       Frankfurter       World Bank
       \             |                /
        \            |               /
         +------ ADAPTERY ------------+
                      |
              NORMALIZACJA
          źródło • data • jednostka
          waluta • częstotliwość
                      |
               WALIDACJA JAKOŚCI
          luki • duplikaty • zakresy
          typ ceny • pochodzenie danych
                      |
                MAGAZYN DANYCH
       +--------------+----------------+
       |              |                |
    fx_rates     metal_prices        candles
       |              |                |
       +--------------+----------------+
                      |
              WARSTWA ANALITYCZNA
           +----------+-----------+
           |                      |
       Profil DGT          LonesomeTheBlue
           |                      |
           +----------+-----------+
                      |
               WYKRESY I ZDARZENIA
                      |
                MODEL PORTFELA
                      |
          WYCENA • SCENARIUSZE • RYZYKO
                      |
             RAPORTY I ALERTY
```

### Warstwy systemu

1. **Adaptery dostawców** — komunikacja HTTP, pobieranie odpowiedzi i obsługa błędów zewnętrznych.
2. **Normalizacja** — wspólny format obserwacji bez utraty oryginalnej semantyki.
3. **Walidacja** — sprawdzanie typów, dat, jednostek, zakresów i kompletności.
4. **Magazyn danych** — trwały zapis historii, metadanych i pochodzenia.
5. **Silnik analityczny** — regresja, kanały, statystyki i profile wskaźników.
6. **Portfel** — aktywa, transakcje, koszty nabycia i wycena.
7. **Interfejs i API** — wykresy, ustawienia, raporty i operacje administracyjne.
8. **Harmonogram** — automatyczne pobieranie, ponawianie błędów i kontrola świeżości danych.

### Technologie

Aktualna warstwa backendu wykorzystuje lub przewiduje:

- Python;
- FastAPI;
- HTTPX;
- SQLite w środowisku lokalnym i testowym;
- PostgreSQL jako docelową bazę dla wdrożenia wieloużytkownikowego lub serwerowego;
- pytest;
- openpyxl do odczytu skoroszytów XLSX;
- harmonogram zadań uruchamiany niezależnie od żądań użytkownika.

Wersje zależności i szczegóły instalacji powinny być utrzymywane w plikach projektu, a nie wprowadzane ręcznie w wielu miejscach.

---

## ✧ Magazyn danych

Dane o różnych semantykach muszą pozostawać rozdzielone.

### `fx_rates`

Przechowuje kursy walut:
- dostawcę;
- datę kursu;
- walutę bazową i kwotowaną;
- kurs średni;
- bid i ask, jeśli występują;
- rodzaj kursu i metodę jego wyznaczenia;
- czas pobrania.

### `metal_prices`

Przechowuje ceny metali:
- dostawcę;
- symbol instrumentu, np. XAU lub XAG;
- datę obserwacji;
- cenę;
- walutę;
- jednostkę;
- częstotliwość;
- rodzaj ceny.

### `candles`

Przechowuje rzeczywiste świece OHLCV, jeśli źródło udostępnia open, high, low, close i ewentualnie volume. Nie należy tworzyć fikcyjnych wartości OHLC z pojedynczego kursu lub średniej miesięcznej.

### `valuation_observations`

Przechowuje obserwacje wycen aktywów, które nie są klasycznymi świecami giełdowymi, np. wyceny bursztynu lub kamieni szlachetnych.

### Pochodzenie i jakość

Każda seria powinna zachować co najmniej:
- identyfikator źródła;
- nazwę dostawcy;
- symbol;
- datę obserwacji;
- walutę i jednostkę;
- częstotliwość;
- rodzaj ceny;
- informację, czy wartość jest źródłowa, czy pochodna;
- czas pobrania;
- status jakości i ewentualne ostrzeżenia.

---

## ✧ Regresja liniowa — dwa profile, wspólna podstawa obliczeń

Monitor ma udostępniać wspólną, testowalną implementację matematyczną, ale zachować odrębność metodologii poszczególnych wskaźników.

### Profil DGT

Profil DGT przewiduje:
- dwa kanały regresji;
- poziomy odchyleń standardowych;
- nachylenie i położenie linii;
- krzywą oraz opcjonalne pasma;
- analizę wolumenu i ATR, jeśli dane rzeczywiście zawierają wolumen i zakres cen;
- możliwość odrębnego ustawienia długości i parametrów kanałów.

W audytowanej metodzie szerokość kanału DGT jest oparta na:

[
SD_{DGT}=sqrt{rac{SSE}{n-1}}
]

gdzie:
- (SSE) — suma kwadratów reszt regresji;
- (n) — liczba obserwacji.

### Profil LonesomeTheBlue

Profil LonesomeTheBlue przewiduje:
- trzy niezależne długości kanałów, domyślnie 377, 233 i 144;
- mnożnik odchylenia domyślnie 2,41;
- poziomy Fibonacciego wewnątrz kanału;
- wykrywanie wybić;
- możliwość zachowania historycznych oznaczeń wybicia.

W audytowanej metodzie szerokość kanału jest oparta na RMS:

[
RMS=sqrt{rac{SSE}{n}}
]

Metoda ta nie jest tożsama z metodą DGT. Nie należy ich ujednolicać kosztem zgodności z oryginalnymi wskaźnikami.

### Wspólne elementy silnika

- obliczanie współczynnika kierunkowego i wyrazu wolnego;
- obliczanie wartości dopasowanych;
- obliczanie reszt i SSE;
- wybór metody szerokości kanału;
- wyznaczanie poziomów kanału;
- agregowanie rzeczywistych interwałów wyższych;
- zapis wersji algorytmu i parametrów.

### Wymagania jakościowe

Każdy wynik regresji powinien zawierać:
- instrument i dostawcę;
- interwał oraz źródło ceny;
- długość okna;
- liczbę obserwacji;
- nachylenie;
- SSE;
- metodę odchylenia;
- szerokość kanału i mnożnik;
- wartości linii i granic;
- czas ostatniej świecy;
- informację o zamknięciu świecy;
- wersję obliczeń i jakość danych.

---

## ✧ Centyle i prawdopodobieństwo zysku

To jeden z docelowych, kluczowych modułów Monitora.

### Założenie

Użytkownik będzie mógł wprowadzić stan posiadanych aktywów, w szczególności:
- ilość każdej waluty;
- ilość złota i srebra;
- jednostkę posiadania;
- walutę bazową portfela;
- cenę i datę nabycia;
- ewentualne koszty transakcyjne.

System będzie łączył tę ewidencję z aktualną i historyczną wyceną oraz wynikami regresji.

### Klasy wyniku

Docelowo interfejs ma pokazywać cztery klasy potencjalnego zysku:

| Klasa | Znaczenie |
|---|---|
| Mały zysk | korzystny wynik o niewielkiej skali |
| Średni zysk | wynik o umiarkowanej skali |
| Duży zysk | wynik wyraźnie korzystny |
| Ekstremalny zysk | wynik należący do rzadkiego, skrajnego zakresu rozkładu |

Granice klas powinny być jawnie definiowane na podstawie wybranego rozkładu i horyzontu. Nie mogą być arbitralnie dopasowywane po obejrzeniu wyniku.

### Centyle względem linii regresji

Dla każdej linii regresji należy wyznaczyć rozkład reszt:

[
e_t=P_t-hat P_t
]

Następnie można obliczać kwantyle rozkładu reszt i określać położenie bieżącej ceny względem linii bazowej.

Centyle nie oznaczają automatycznie prawdopodobieństwa zysku. Przykładowo położenie ceny w skrajnym dolnym centylu może oznaczać nietypowe odchylenie od trendu, ale nie dowodzi, że cena wzrośnie. Wymaga to osobnego modelu przejść, horyzontu prognozy i walidacji.

### Trzy odrębne wielkości w raporcie

1. **Pozycja centylowa** — gdzie bieżąca obserwacja znajduje się względem historycznego rozkładu reszt.
2. **Prawdopodobieństwo scenariusza** — oszacowana częstość lub prawdopodobieństwo osiągnięcia określonego wyniku w zadanym horyzoncie.
3. **Wartość portfelowa scenariusza** — kwotowy wynik dla faktycznie posiadanych ilości aktywów, po uwzględnieniu waluty bazowej i kosztów.

### Walidacja modelu

Model powinien przechodzić:
- testy kroczące poza próbą uczącą;
- porównanie prognoz z faktycznymi wynikami;
- pomiar kalibracji prawdopodobieństw;
- analizę stabilności parametrów;
- testy dla trendu wzrostowego, spadkowego i bocznego;
- testy na okresach wysokiej zmienności;
- analizę wpływu luk i niejednorodnych częstotliwości danych.

Jeśli danych jest za mało, Monitor ma wyświetlić brak wiarygodnej estymacji zamiast tworzyć pozornie precyzyjny procent.

---

## ✧ Import początkowy i harmonogram automatyczny

Import historii jest pierwszym etapem operacyjnym po przygotowaniu i uruchomieniu środowiska serwerowego.

### Import początkowy

1. Sprawdzenie dostępności źródeł.
2. Ustalenie zakresu dat.
3. Pobranie historii z NBP.
4. Pobranie historii z Frankfurter.
5. Pobranie miesięcznych danych World Bank.
6. Normalizacja i walidacja.
7. Zapis do odpowiednich tabel.
8. Raport liczby rekordów, luk, błędów i zakresu dat.
9. Ponowny odczyt z bazy i kontrola spójności.

### Harmonogram

Docelowo harmonogram będzie:
- pobierał nowe dane po udostępnieniu ich przez dostawcę;
- oddzielnie obsługiwał źródła dzienne i miesięczne;
- wykrywał nieudaną synchronizację;
- ponawiał operacje z ograniczoną liczbą prób;
- zapobiegał równoległemu uruchamianiu identycznego importu;
- zapisywał czas ostatniego powodzenia i błędu;
- udostępniał stan synchronizacji w interfejsie.

Nie należy deklarować, że harmonogram działa, dopóki nie zostanie uruchomiony w docelowym środowisku i sprawdzony po restarcie serwera.

---

## ✧ API — dotychczasowy zakres backendu

W wersji 0.1.10 backend zawiera następujące trasy:

| Metoda | Trasa | Przeznaczenie |
|---|---|---|
| GET | `/health` | stan API |
| GET | `/sources` | lista źródeł |
| POST | `/sources` | dodanie źródła |
| POST | `/sources/{source_id}/ingest` | import źródła konfigurowalnego |
| GET | `/candles` | odczyt świec |
| GET | `/valuations` | odczyt wycen |
| POST | `/providers/nbp/import` | import historii NBP |
| POST | `/providers/frankfurter/import` | import historii Frankfurter |
| POST | `/providers/world-bank/import` | import historii World Bank |
| GET | `/providers/fx-rates` | odczyt kursów walut |
| GET | `/providers/metal-prices` | odczyt cen metali |

Operacje importu wymagają nagłówka administracyjnego `X-Admin-Token`, skonfigurowanego po stronie serwera przez `ADMIN_API_TOKEN`.

Dokładne schematy parametrów należy sprawdzać w aktualnej implementacji API. Tabela opisuje istniejący zakres, a nie gwarantuje, że backend został już wdrożony publicznie.

---

## ✧ Bezpieczeństwo i odpowiedzialne użytkowanie

- Token administracyjny przechowywać wyłącznie w zmiennych środowiskowych lub menedżerze sekretów.
- Nie umieszczać haseł, kluczy, tokenów ani danych dostępowych w repozytorium.
- Ograniczyć dostęp administracyjny do zaufanych operatorów.
- Stosować kopie zapasowe bazy danych i testować odtwarzanie.
- Używać HTTPS w środowisku produkcyjnym.
- Rejestrować importy, błędy i zmiany konfiguracji.
- Walidować wszystkie dane wejściowe.
- Ograniczać rozmiar odpowiedzi, czas żądań i częstotliwość pobierania.
- Zachowywać warunki korzystania z danych poszczególnych dostawców.

### Ograniczenia analizy

Dane historyczne nie gwarantują powtórzenia się przeszłych zależności. Regresja liniowa opisuje dopasowanie modelu do określonego szeregu i okna, a nie pewny przyszły kierunek ceny. Prawdopodobieństwa są zależne od przyjętego modelu, horyzontu, jakości danych i stabilności rozkładu.

Monitor ma prezentować założenia, niepewność i ograniczenia obok wyników, a nie ukrywać je w dokumentacji.

---

## ✧ Uruchomienie backendu — środowisko deweloperskie

Poniższe polecenia opisują standardowy sposób uruchomienia warstwy FastAPI po sklonowaniu repozytorium i przygotowaniu katalogu backendu. Należy je dostosować do aktualnej struktury plików projektu.

```bash
git clone https://github.com/bartoszruta26-droid/Monitor.git
cd Monitor

python -m venv .venv
source .venv/bin/activate

python -m pip install --upgrade pip
python -m pip install -r backend/requirements.txt

export ADMIN_API_TOKEN="ustaw-wlasny-sekret"
export DATABASE_URL="sqlite:///./market_data.db"

uvicorn market_data.main:app --app-dir backend --host 127.0.0.1 --port 8000
```

Kontrola dostępności:

```bash
curl http://127.0.0.1:8000/health
```

**Uwaga:** polecenia stanowią instrukcję środowiska deweloperskiego. Nie potwierdzają, że produkcyjny serwer jest już skonfigurowany. Do wdrożenia produkcyjnego należy użyć PostgreSQL, HTTPS, bezpiecznego zarządzania sekretami, kopii zapasowych i procesu nadzorującego usługę.

---

## ✧ Testowanie i kontrola jakości

Każda nowa funkcja powinna być objęta testami jednostkowymi, integracyjnymi i — tam, gdzie jest to możliwe — testami zewnętrznych interfejsów.

Przykładowe uruchomienie:

```bash
cd backend
pytest -q
```

### Minimalny zestaw testów

- poprawność parserów odpowiedzi NBP;
- limity zakresów dat NBP;
- poprawność kursów średnich i bid/ask;
- parser miesięcznego arkusza World Bank;
- parser Frankfurter v2;
- idempotencja importów;
- rozdzielenie kursów walut i cen metali;
- poprawność jednostek i walut;
- zachowanie danych źródłowych;
- obliczenia regresji na szeregach syntetycznych;
- zgodność profili DGT i LonesomeTheBlue z ich metodami;
- testy portfela i wyceny;
- testy harmonogramu, ponawiania i blokady duplikatów;
- testy awarii dostawców oraz braku danych.

Testy z atrapami HTTP nie są tym samym co testy rzeczywistych usług zewnętrznych. Raport testów powinien wskazywać, które testy były lokalne, które używały mocków, a które faktycznie łączyły się z dostawcą.

---

## ✧ Plan rozwoju

### Etap I — repozytorium i fundamenty
- uporządkowanie struktury projektu;
- przeniesienie i weryfikacja backendu;
- konfiguracja środowiska;
- dokumentacja uruchomienia;
- automatyczne testy w CI;
- kontrola sekretów i zależności.

### Etap II — import historii i synchronizacja
- import początkowy NBP;
- import początkowy Frankfurter;
- import miesięcznych danych World Bank;
- raport kompletności historii;
- harmonogram dzienny i miesięczny;
- ponawianie błędów i monitoring świeżości.

### Etap III — wspólna historia i wykresy
- jednolity model instrumentów;
- obsługa rzeczywistych interwałów;
- wykresy cen i zmian;
- jawne oznaczanie źródła, waluty i jednostki;
- kontrola luk i jakości szeregu.

### Etap IV — silniki regresji
- implementacja wspólnej matematyki;
- profil DGT;
- profil LonesomeTheBlue;
- testy syntetyczne i porównawcze;
- zapis wyników i wersji algorytmów.

### Etap V — ewidencja portfela
- aktywa i transakcje;
- ceny nabycia i koszty;
- wycena w walucie bazowej;
- zysk/strata niezrealizowana;
- ekspozycja walutowa i metalowa.

### Etap VI — centyle i prawdopodobieństwo
- rozkłady reszt;
- pozycje centylowe;
- scenariusze i horyzonty;
- model prawdopodobieństwa;
- kwotowa wycena scenariuszy portfela;
- testy kalibracji i prognoz poza próbą.

### Etap VII — interfejs, raporty i alerty
- panele konfiguracyjne;
- porównanie profili regresji;
- raport portfela;
- alerty o nowych danych i zdarzeniach;
- eksport danych i raportów.

### Etap VIII — wdrożenie produkcyjne
- serwer i baza PostgreSQL;
- automatyczny start usług;
- kopie zapasowe;
- monitoring i alerty techniczne;
- procedury aktualizacji i odtwarzania.

---

## ✧ Zasady pracy z repozytorium

1. Zmiany wprowadzać w małych, opisanych commitach.
2. Nie usuwać istniejących źródeł ani adapterów bez analizy zależności.
3. Nie zmieniać semantyki danych po cichu.
4. Każda zmiana schematu bazy wymaga migracji i testu.
5. Każda nowa metoda obliczeniowa wymaga opisu matematycznego i testów.
6. Nie scalać zmian, których testy nie przeszły.
7. Nie umieszczać sekretów w kodzie, README ani historii Git.
8. Aktualizować dokumentację razem z kodem.
9. Oznaczać funkcje jako planowane, zaimplementowane, przetestowane lub wdrożone — zgodnie z rzeczywistym stanem.
10. Zachowywać możliwość odtworzenia wyniku na podstawie danych i parametrów z chwili obliczenia.

### Konwencja statusów

- **Planowane** — opisane, ale jeszcze niezaimplementowane.
- **Zaimplementowane** — kod istnieje.
- **Przetestowane lokalnie** — testy wykonano w środowisku deweloperskim.
- **Zintegrowane** — połączone z pozostałymi modułami.
- **Zweryfikowane zewnętrznie** — wykonano rzeczywiste zapytania do dostawców lub systemów.
- **Wdrożone** — działa w docelowym środowisku.

---

## ✧ Stan projektu

Repozytorium jest miejscem docelowego rozwoju kodu i dokumentacji. Istniejący prywatny plugin Monitor Rynków 0.1.10 zawiera adaptery NBP, Frankfurter i World Bank oraz bazowy backend do zapisu historii. Audyty DGT i LonesomeTheBlue stanowią dokumentację metodologiczną i plan integracji.

**Nie należy utożsamiać stanu pluginu z działającym wdrożeniem serwerowym.** Import początkowy, harmonogram produkcyjny, wykresy podłączone do wspólnej historii, portfel aktywów i model prawdopodobieństwa wymagają dalszej implementacji i weryfikacji.

Aktualny stan realizacji należy śledzić w commitach, testach i dokumentacji repozytorium.

---

## ✧ Dokumentacja i odnośniki

- Repozytorium: https://github.com/bartoszruta26-droid/Monitor
- NBP API: https://api.nbp.pl/
- Frankfurter API: https://api.frankfurter.dev/
- World Bank Commodity Markets: https://www.worldbank.org/en/research/commodity-markets
- FastAPI: https://fastapi.tiangolo.com/
- pytest: https://docs.pytest.org/

## ✧ Licencja i warunki wykorzystania

Licencja projektu wymaga jawnego ustalenia i umieszczenia w repozytorium w pliku `LICENSE`. Do tego czasu nie należy zakładać, że kod jest udostępniony na warunkach konkretnej licencji open source.

Dane zewnętrzne podlegają warunkom właściwych dostawców. Użytkownik projektu powinien sprawdzić warunki ich pobierania, przechowywania, redystrybucji i prezentowania.

---

<div align="center">

### ✦ Niech technologia służy prawdzie, roztropności i dobru. ✦

**Monitor Rynków — dane jawne, obliczenia odtwarzalne, decyzje odpowiedzialne.**

</div>
