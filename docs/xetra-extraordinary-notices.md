# Komunikaty nadzwyczajnych sesji Xetra

## Cel

Utrzymywać skrócone, wydłużone lub odwołane sesje jako osobne, audytowalne komunikaty źródłowe. Bazowy kalendarz świąt i standardowych sesji pozostaje niezmienny.

## Model

### `VenueSessionNotice`

- `notice_id`: stabilny identyfikator komunikatu;
- `revision`: rosnąca rewizja komunikatu;
- `calendar_id` i `venue`: rynek, którego dotyczy komunikat;
- `effective_date`: lokalna data sesji;
- `source_object_id`: obowiązkowa referencja do niezmiennego obiektu archiwum SHA-256;
- `notice_kind`: `Informational`, `SessionOverride` albo `Cancellation`;
- `day_status`: wymagany dla nadpisania sesji, np. `Trading`, `Closed` lub `SettlementOnly`;
- `product_sessions`: komplet jawnie opublikowanych przedziałów godzinowych dla wskazanych grup produktów;
- `attribution`, `source_uri`, `retrieved_at` i status licencji w metadanych pochodzenia.

Komunikat jest niezmienny. Korekta publikuje nową rewizję. Ta sama rewizja nie może zmienić daty, obiektu źródłowego ani godzin.

## Reguły bezpieczeństwa i interpretacji

1. `source_object_id` musi wskazywać istniejący, integralny obiekt archiwum.
2. `calendar_id` i `venue` muszą odpowiadać istniejącemu kalendarzowi.
3. `effective_date` musi mieścić się w półotwartym zakresie kalendarza.
4. Komunikat informacyjny nie zmienia sesji.
5. Nadpisanie sesji wymaga jawnego statusu dnia i kompletnych godzin dla wszystkich wskazanych grup produktów; nie wolno uzupełniać godzin przez domysł.
6. Anulowanie wskazuje rewizję komunikatu, którą unieważnia; nie usuwa historii.
7. Konflikt dwóch aktywnych komunikatów dla tego samego rynku, daty i grupy produktów wymaga jawnego rozstrzygnięcia nową rewizją, a nie kolejnością odczytu plików.
8. Funkcja efektywnego dnia tworzy wynikową kopię dnia bazowego i nakłada zatwierdzone komunikaty. Nie modyfikuje `VenueSessionCalendar`.
9. Brak komunikatu dla daty oznaczonej `requires_official_notice` pozostawia bazowe dane bez zmian i sygnalizuje nierozstrzygnięty status.
10. Nie publikować godzin 30 grudnia ani innej sesji skróconej bez oficjalnego komunikatu zawierającego te godziny.

## Plan implementacji

- 17A: model, JSON, walidacja referencji i niezmienność rewizji.
- 17B: parser oficjalnego komunikatu; początkowo wspiera wyłącznie jednoznaczny tekst HTML z datą, rynkiem, statusem i godzinami.
- 17C: efektywny dzień sesyjny i rozstrzyganie konfliktów/anulowań.
- 17D: fixture offline i testy regresji. Live HTTP pozostaje poza zwykłym CI.

## Kryteria akceptacji

- brak zmiany bazowego kalendarza przy publikacji komunikatu;
- odrzucanie komunikatów z brakującym obiektem źródłowym, datą poza zakresem lub niejednoznacznymi godzinami;
- idempotentny import identycznego komunikatu;
- nowa rewizja zachowuje poprzednią historię;
- anulowanie nie usuwa archiwum ani poprzedniej rewizji;
- konflikty sesji są jawne i deterministyczne;
- pełny CI przechodzi dla konkretnego SHA.

## Stan implementacji 17A

Wprowadzono `VenueSessionNotice`, kolekcję `venue_session_notices` w migawce i partii, serializację JSON schema v1, `upsert_venue_session_notice` oraz walidację referencji do obiektu źródłowego, rynku, kalendarza, daty, rewizji, sesji i anulowań. Testy 17A przeszły w CI [43323528084](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43323528084), CTest 9/9.


## Stan implementacji 17B

Parser `parse_xetra_session_notice_html` przyjmuje jawne pola HTML: `Venue`, `Effective date`, `Notice kind`, opcjonalnie `Day status`, `Cancels` oraz tabelę `Product Group / Opens / Closes`. Obsługuje komunikaty informacyjne, nadpisania i anulowania. `XetraCalendarSourceAdapter::import_notice` pobiera komunikat z zatwierdzonego hosta HTTPS, archiwizuje dokładne bajty jako obiekt SHA-256 i zapisuje obiekt oraz komunikat w jednej partii. Powtórny import identycznej rewizji jest idempotentny; zmiana bajtów lub interpretacji dla tej samej rewizji jest odrzucana. Testy używają wyłącznie syntetycznych fixture offline. CI [43324267708](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43324267708) zakończył się sukcesem dla parsera i importera; Etap 17B jest zamknięty.


## Stan implementacji 17C

Dodano `resolve_effective_venue_calendar_day(snapshot, calendar_id, date)`. Resolver wybiera najnowszą rewizję kalendarza, wybiera najnowszą rewizję każdego komunikatu, uwzględnia anulowania i nakłada najwyżej jedno aktywne nadpisanie na kopię bazowego dnia. Komunikaty informacyjne pozostawiają harmonogram bez zmian. Wiele aktywnych nadpisań powoduje jawny konflikt. `requires_official_notice_unresolved` informuje, czy baza nadal oczekuje na oficjalne rozstrzygnięcie. Testy resolvera są dodane; CI aktualnego SHA pozostaje w toku.


## Stan implementacji 17C–17D

Resolver efektywnego dnia i raport nierozstrzygniętych dat są wdrożone. Testy potwierdzają pozostawienie bazowego harmonogramu przez komunikat informacyjny, nałożenie jawnego skrócenia, cofnięcie override po anulowaniu, zachowanie nowszej rewizji przy anulowaniu starszej oraz jawne odrzucenie konfliktujących nadpisań. Raport osieroconych obiektów uwzględnia referencje zarówno kalendarzy, jak i komunikatów. CI dla resolvera i raportów: [43324719408](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43324719408), [43324853676](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43324853676), [43325061044](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43325061044) — sukces. Dodatkowy test dwóch anulowań tej samej rewizji jest w toku.


## Wynik Etapu 17D

Testy regresji potwierdzają dokładne anulowanie wskazanej rewizji, pozostawienie nowszej rewizji aktywnej, odrzucanie wielu anulowań tej samej rewizji, konflikt kilku aktywnych nadpisań, przywrócenie bazowego harmonogramu i raport nierozstrzygniętych dat. CI [43325240256](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43325240256) — success. Etapy 17A–17D są zakończone dla syntetycznych fixture offline. Rzeczywisty format aktualnego okólnika giełdy nie został jeszcze pobrany ani zweryfikowany; to osobny następny etap.


## Etap 18A — fazy sesji w ogłoszeniu PDF

Wprowadzono `VenueSessionPhaseEventKind` i `VenueSessionPhaseEvent`, aby zachować kamienie milowe sesji, których nie da się opisać jednym przedziałem otwarcia/zamknięcia. Model obejmuje start pre-trading, retail early, aukcję otwarcia, aukcję śróddzienną, aukcję zamknięcia, najwcześniejszy moment ustalenia ceny, koniec Trade-at-Close, koniec usług off-book oraz koniec post-trading. `parse_xetra_session_notice_text` czyta tekst wydobyty z PDF, sprawdza MIC XETR, datę i kompletność dziewięciu faz oraz konwertuje AM/PM na lokalny czas 24-godzinny. Import i archiwizacja surowego PDF nie są jeszcze zaimplementowane — to Etap 18B.


## Etap 18B–18C — PDF i wersjonowanie interpretacji

Importer `import_pdf_notice` pobiera PDF z zatwierdzonego hosta, wydobywa tekst przez `pdftotext`, archiwizuje dokładne bajty jako obiekt SHA-256 i publikuje komunikat z fazami. CI [43327239674](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43327239674) potwierdził ścieżkę na syntetycznym PDF. Dodano też `parser_version` i `publish_notice_revision_from_archive`, które tworzy kolejną rewizję z już zarchiwizowanego PDF bez kolejnego HTTP. Testy tej ostatniej ścieżki oczekują na CI.


## Wynik Etapu 18C

`VenueSessionNotice` przechowuje teraz `parser_version`. `publish_notice_revision_from_archive` odczytuje archiwalny PDF, weryfikuje SHA-256 i długość, wydobywa tekst ponownie i publikuje nową rewizję bez HTTP. Kolejne rewizje współdzielą ten sam niezmienny obiekt źródłowy. Testy sprawdzają również idempotentny replay. CI [43327718619](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43327718619) — success. Etapy 18A–18C są zakończone w zakresie offline. Oryginalny PDF giełdy nie został jeszcze zapisany jako fixture; jego weryfikacja to Etap 19.


## Parser v2 — oryginalny układ PDF

Oryginalne ogłoszenie z 30 grudnia 2026 r. umieszcza godziny aukcji i Trade-at-Close w sekcji 2 (rynek Xetra), natomiast koniec usług TES OTC/LIS i Xetra EnLight w osobnej sekcji 3, podsekcji b) Xetra. Trade-at-Close jest rozdzielony między dwa wiersze tekstu. Parser `xetra-pdf-text-notice-v2` rozdziela te sekcje, łączy kontynuację wierszy tylko dla właściwego zdarzenia i wymaga MIC XETR w obu miejscach. Oficjalny PDF: https://cashmarket.deutsche-boerse.com/resource/blob/5673886/22764993c4b1617c5f465186066344e6/data/Final%20trading%20session%202026.pdf. Testowy fixture jest transkrypcją tekstu, a nie oryginalnymi bajtami PDF.
