# Założenia architektury Monitor

## Cel
Jedna platforma rynkowa z jednym modelem domenowym, jednym serwerem wymiany danych i czterema interfejsami: Linux GUI, WebUI, TUI i Android App.

## Diagram logiczny

```text
 NBP / Frankfurter / World Bank / inne źródła
                    |
             Adaptery backendów
                    |
                    v
+------------- Serwer wymiany danych --------------+
| REST/JSON | WebSocket | autoryzacja | synchronizacja |
| normalizacja | walidacja | wersjonowanie | audyt     |
+-------------------------+-----------------------+
                          |
                Interfejs repozytorium
                          |
              +-----------+-----------+
              |                       |
       JSON (domyślnie)         MariaDB (opcjonalnie)

          ^         ^          ^          ^
          |         |          |          |
       Linux GUI  WebUI       TUI      Android App
                   |          |          /
           +--------+----------+---------+
                    |
           Wspólny rdzeń C++
```

Wszystkie klienty komunikują się z serwerem przez wspólny kontrakt API. Rdzeń C++ jest współdzielony tam, gdzie klient może wykonywać obliczenia lokalnie; logika autorytatywna, synchronizacja i zapis danych pozostają po stronie serwera.

## Wymagania serwera

1. Jeden stabilny identyfikator rekordu i wersja schematu.
2. API z prefiksem wersji, np. `/api/v1`.
3. REST/JSON dla żądań i WebSocket dla zdarzeń.
4. Rejestracja backendów i klientów, uwierzytelnianie oraz role.
5. Idempotentny import i kontrola duplikatów.
6. Kontrola wersji i jawne rozwiązywanie konfliktów.
7. Dziennik audytowy z informacją kto/co/kiedy zmienił.
8. TLS dla połączeń zdalnych.
9. Klienci nie łączą się bezpośrednio z MariaDB.
10. Zapis i odczyt przechodzą przez abstrakcję repozytorium.

## Magazyn JSON

JSON jest domyślnym magazynem i powinien działać bez zewnętrznego serwera bazy. Należy stosować wersjonowany schemat, zapis atomowy (plik tymczasowy, synchronizacja i zmiana nazwy), kopie bezpieczeństwa oraz blokady lub kontrolę wersji. Nie wolno dopuścić do niekontrolowanego równoległego zapisu do tego samego pliku.

## Magazyn MariaDB

MariaDB jest opcjonalnym backendem repozytorium. Wybór odbywa się przez konfigurację. Migracje schematu, transakcje, indeksy i ograniczenia integralności muszą zapewnić równoważną semantykę operacji z magazynem JSON. Dane dostępowe nie mogą być zapisane w repozytorium.

## Klienci

- Linux GUI: Qt 6 / C++.
- WebUI: HTML/CSS/TypeScript; korzysta z API serwera.
- TUI: C++ z FTXUI lub ncurses; korzysta z API serwera.
- Android: Qt 6/C++ albo Kotlin/JNI; korzysta z API serwera i może utrzymywać lokalną pamięć podręczną.

## Konfiguracja docelowa

Wzór znajduje się w `config/monitor.example.json`. Domyślny sterownik to `json`. MariaDB jest wybierana przez ustawienie `storage.driver` na `mariadb`. Hasło pobierane jest ze zmiennej środowiskowej.

Adaptery dostawców C++20 są zaimplementowane. Magazyn JSON C++20 jest zaimplementowany i zweryfikowany w CI: wspólny kontrakt repozytorium, atomowy zapis migawki, blokady międzyprocesowe, kopia zapasowa, historia korekt i atomowe przyjmowanie partii danych. MariaDB i serwer wymiany danych pozostają do wykonania.

## Status

Obecnie w repozytorium działa natywny rdzeń C++20: regresja, model danych rynkowych v1, walidacja, testy, CLI, transport HTTP libcurl i adaptery NBP, Frankfurter, World Bank Pink Sheet oraz XAUS. Adaptery są budowane w monitor_core i objęte testami bez dostępu do sieci. Magazyn JSON jest zaimplementowany i zweryfikowany w CI; MariaDB, serwer, API i interfejsy pozostają do implementacji.

Wspólny model danych rynkowych opisano w [docs/market-data-model.md](market-data-model.md), a adaptery w [docs/cpp-provider-adapters.md](cpp-provider-adapters.md). Typy domenowe znajdują się w native/include/monitor/market_data.hpp, a kontrakt adapterów i transport HTTP w native/include/monitor/market_data_adapters.hpp. Adaptery zwracają rekordy domenowe. Warstwa repozytorium udostępnia atomowy zapis partii wyników do JSON; adaptery nie zapisują bezpośrednio plików.


## Integracja z Nextcloud — kierunek przyszły

Monitor ma być przygotowany do późniejszego udostępnienia jako aplikacja Nextcloud. Rdzeń C++20, serwer i model domenowy pozostają niezależne od platformy Nextcloud. Integracja powinna korzystać z publicznych i stabilnych interfejsów platformy: aplikacji Nextcloud, OCS API oraz WebDAV/Files API. Nie należy opierać się na prywatnych klasach ani bezpośrednio modyfikować wewnętrznych tabel bazy Nextcloud.

Zalecany podział:
- cienka aplikacja Nextcloud odpowiada za ekran, ustawienia, uprawnienia i integrację z plikami;
- niezależny silnik/serwer C++20 wykonuje pobieranie danych, agregację, regresję, wycenę i analizę portfela;
- komunikacja odbywa się przez wersjonowane HTTPS API;
- AppAPI/ExApp może zostać ocenione jako wariant wdrożenia zewnętrznego komponentu, jeśli będzie to uzasadnione.

Nextcloud ma pełnić rolę warstwy integracji, synchronizacji, przechowywania i udostępniania plików. Nie należy bez decyzji architektonicznej tworzyć dwóch równorzędnych źródeł prawdy. Domyślnie autorytatywne pozostaje repozytorium Monitora, a Nextcloud synchronizuje zatwierdzone rekordy, raporty i prognozy.

## ChatGPT i Model Context Protocol

Przyszła integracja ChatGPT powinna korzystać z dedykowanego, uwierzytelnionego serwera MCP, który wywołuje API Monitora i kontrolowaną integrację Nextcloud. ChatGPT nie otrzymuje bezpośredniego, nieograniczonego dostępu do dysku ani katalogu danych Androida.

Narzędzia MCP należy rozdzielić na:
- odczyt danych rynkowych, agregatów, analiz i dozwolonego zakresu portfela;
- deterministyczne obliczenia realizowane przez silnik C++;
- zapis prognoz i raportów do repozytorium oraz wskazanego katalogu/obszaru Nextcloud;
- operacje zmieniające portfel, domyślnie wyłączone lub wymagające jawnego potwierdzenia.

AI może interpretować dane i tworzyć scenariusze, ale regresja, agregacja, wycena i kalkulacja wyniku transakcji muszą być wykonywane przez testowalny silnik obliczeniowy, a nie szacowane swobodnie przez model językowy. Każda prognoza musi zawierać czas, źródła, horyzont, wersję algorytmu/modelu i informację o niepewności.

Dostęp lokalny do plików tabletu wymaga kontrolowanego klienta/pośrednika z uprawnieniami systemowymi. Serwer zdalny nie może samodzielnie odczytać prywatnej ścieżki Androida. Preferowany kanał synchronizacji to Nextcloud WebDAV/Files API lub API klienta.

## Integracja transakcji z inną aplikacją — przyszła

Część funkcji ręcznego wprowadzania zakupów, sprzedaży i wymiany aktywów ma w przyszłości zostać udostępniona również w innej aplikacji użytkownika, rozwijanej w osobnym repozytorium. Nie jest to zakres bieżącego etapu, ale model transakcji i API należy od początku projektować niezależnie od interfejsu Monitora.

Wymagane przygotowanie:
- wersjonowany kontrakt tworzenia, odczytu i korekty transakcji;
- stabilne identyfikatory i klucze idempotencji;
- uprawnienia ograniczone do operacji;
- rejestr klienta/źródła wprowadzającego transakcję;
- audyt każdej zmiany i ochrona przed duplikacją.

## Klienci i źródło danych

Android App, TUI, Linux GUI i WebUI mają wyświetlać wspólny, zatwierdzony zestaw danych i prognoz. Mogą korzystać bezpośrednio z API Monitora oraz z kontrolowanej synchronizacji plików Nextcloud. Tryb lokalny/offline musi rozróżniać dane zsynchronizowane od oczekujących na synchronizację oraz obsługiwać wersjonowanie i konflikty.

Szczegółowy audyt, priorytety, model danych i kryteria akceptacji znajdują się w [audytcie i planie realizacji](market-monitor-audit-and-roadmap.md).
