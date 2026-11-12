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

To wzór docelowy, a nie deklaracja implementacji. Serwer i oba adaptery magazynu pozostają do zaimplementowania.

## Status

Obecnie w repozytorium działa natywny rdzeń regresji C++20, program CLI, testy i CI. Serwer, magazyny JSON/MariaDB, adaptery, API i interfejsy pozostają do implementacji.
