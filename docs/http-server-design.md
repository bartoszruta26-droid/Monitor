# Projekt serwera HTTP/REST/JSON — Monitor FX/XAU

## Status dokumentu
Wybór technologii i kontrakt pierwszej iteracji Etapu 27. Dokument nie oznacza, że serwer HTTP jest już zaimplementowany.

## 1. Wybór biblioteki

**Boost.Beast + Boost.Asio (C++20)**.

Uzasadnienie:
- biblioteka C++ do HTTP/1.1 i WebSocket, oparta na Asio;
- Beast jest biblioteką nagłówkową, więc nie wymaga osobnej biblioteki binarnej Beast;
- pozwala użyć jednego modelu żądania/odpowiedzi dla routera, testów i późniejszego transportu;
- do obsługi błędów Boost.System można włączyć tryb nagłówkowy przez `BOOST_ERROR_CODE_HEADER_ONLY`, aby nie dodawać niepotrzebnej zależności linkera;
- pakiet systemowy Ubuntu/Debian: `libboost-dev`; CI instaluje jawnie tę zależność;
- licencja Boost Software License 1.0 jest licencją permissive i musi być odnotowana w zestawieniu zależności.

Wersja minimalna do ustalenia w CMake: Boost 1.74. Moduł serwera ma być osobnym celem CMake i nie może być zależnością `monitor_core`. Rdzeń domenowy pozostaje bez zależności HTTP.

## 2. Granice bezpieczeństwa pierwszej iteracji

- Domyślny adres nasłuchiwania: `127.0.0.1`; nie używać `0.0.0.0` jako wartości domyślnej.
- Serwer odrzuca konfigurację zdalnego interfejsu, jeśli nie ma jawnie skonfigurowanego transportu TLS i mechanizmu uwierzytelniania.
- Token API jest pobierany wyłącznie ze zmiennej środowiskowej `MONITOR_API_TOKEN`; nie zapisuje się go w repozytorium, pliku konfiguracyjnym ani logach.
- Wszystkie endpointy danych portfela wymagają nagłówka `Authorization: Bearer ...`. Porównanie tokenów musi być stałoczasowe.
- Endpoint zdrowia ujawnia wyłącznie status procesu i wersję API, bez ścieżek plików, konfiguracji, sekretów i danych portfela.
- Brak tokenu przy uruchomieniu produkcyjnego serwera powoduje odmowę startu, a nie tryb anonimowy.
- Pierwsza wersja nie udostępnia zdalnego nasłuchiwania. TLS i zdalne wdrożenie wymagają osobnego testowanego etapu.
- Nie logować wartości nagłówka Authorization, tokenów, pełnych ścieżek prywatnych ani danych wrażliwych.

## 3. Kontrakt endpointów v1

### `GET /healthz`
Publiczny endpoint diagnostyczny. Odpowiedź zawiera tylko `status` i `api_version`.

### `GET /api/v1/portfolio`
Wymaga Bearer tokenu. Zwraca migawkę księgi portfela: wersję schematu, rewizję, walutę raportowania i zapisane transakcje wraz z audytem kwotowania. Nie zwraca ścieżki pliku repozytorium ani nie wylicza bieżącej wartości rynkowej; wycena wymaga osobnego kontraktu z migawką danych rynkowych.

### `GET /api/v1/portfolio/history`
Wymaga Bearer tokenu. Parametry:
- `asset_code` — opcjonalny kod waluty albo `XAU`;
- `fx_pair` — opcjonalna para `BASE/QUOTE`;
- `source_id` i `operation_id` — opcjonalne filtry;
- `executed_from` i `executed_to` — opcjonalne inkluzywne czasy Unix;
- `page_size` — liczba 1–500, domyślnie 100;
- `cursor` — nieprzezroczysty kursor API v1 z Etapu 26.

Router mapuje parametry na `PortfolioHistoryPageRequest` i wywołuje `get_portfolio_history_page()`. Nie implementuje własnej paginacji ani filtrów. Odpowiedź zachowuje rewizję migawki, `total_matching`, wpisy, dowody cenowe, audyt wykonania FX i `next_cursor`.

### Metody i ścieżki poza kontraktem
- Metody inne niż GET otrzymują 405.
- Nieznana ścieżka otrzymuje 404.
- Nie dodawać endpointów zapisu, modyfikacji transakcji ani portfela w pierwszej iteracji.
- WebSocket, MariaDB, konta użytkowników i role nie są częścią pierwszego serwera REST.

## 4. Format błędów

Jednolity JSON:
```json
{
  "error": {
    "code": "invalid_argument",
    "message": "Nieprawidłowy parametr żądania.",
    "request_id": "..."
  }
}
```

Mapowanie:
- 400 — niepoprawne parametry, format kursora lub zakres czasu;
- 401 — brak lub niepoprawny token;
- 404 — nieznana ścieżka;
- 405 — metoda niedozwolona;
- 413 — żądanie przekracza limit;
- 429 — przekroczony limit żądań, jeśli limiter zostanie włączony;
- 500 — błąd wewnętrzny, bez szczegółów implementacyjnych w odpowiedzi.

Odpowiedzi mają `Content-Type: application/json; charset=utf-8`, jawny limit rozmiaru i wyłączone cache dla danych portfela.

## 5. Wymagania implementacyjne i testy

1. Router ma być czystą warstwą mapującą HTTP na istniejące usługi domenowe.
2. Serializacja ma być jawna i stabilna; nie serializować obiektów domenowych przez przypadkowe dumpowanie pamięci ani ujawniać prywatnych pól.
3. Ograniczyć rozmiar nagłówków, body, długość URI, liczbę parametrów i timeouty.
4. Obsłużyć błędne kodowanie URL, powtórzone parametry, brakujące wartości i niepoprawne liczby bez wyjątku poza granicę routera.
5. Testować 200/400/401/404/405/413/500, brak tokenu, błędny token, poprawny token, nieprawidłowy kursor, filtry i kontynuację stron.
6. Test integracyjny ma używać tymczasowego repozytorium JSON i danych FX/XAU, bez połączeń do prawdziwych dostawców.
7. Test uruchomienia procesu ma potwierdzać domyślne wiązanie loopback i odmowę startu przy braku tokenu.
8. CI musi instalować zależności jawnie i budować/testować serwer w oddzielnym celu CMake.
9. Etap nie jest ukończony, dopóki CI dla dokładnego SHA nie przejdzie w całości.

## 6. Granica domenowa

Produkt obejmuje wymianę walut FX oraz zakup, sprzedaż, wymianę i wycenę złota XAU. Serwer nie może wprowadzać obsługi akcji, brokerów akcyjnych, zleceń giełdowych ani kalendarzy sesji giełdowych.


## 7. Proces serwera — pierwsza implementacja

Dodano cel wykonywalny `monitor-http-server` oparty na routerze z sekcji 3. Konfiguracja środowiskowa:
- `MONITOR_API_TOKEN` — wymagany token Bearer; bez niego proces odmawia startu;
- `MONITOR_PORTFOLIO_FILE` — wymagany plik repozytorium portfela JSON;
- `MONITOR_PORT` — opcjonalny port TCP 1–65535, domyślnie 8080.

Proces nasłuchuje wyłącznie na `127.0.0.1`, obsługuje SIGINT/SIGTERM, ogranicza nagłówki do 16 KiB, body do 64 KiB i stosuje 15-sekundowy termin operacji sieciowych. Nie udostępnia opcji bind na interfejs zewnętrzny. Jest to lokalny transport HTTP bez TLS, a nie serwer gotowy do publicznego wdrożenia. Przed jakimkolwiek dostępem zdalnym wymagane są TLS, właściwe zarządzanie sekretami, testy bezpieczeństwa i przegląd wdrożenia.
