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

Parser `parse_xetra_session_notice_html` przyjmuje jawne pola HTML: `Venue`, `Effective date`, `Notice kind`, opcjonalnie `Day status`, `Cancels` oraz tabelę `Product Group / Opens / Closes`. Obsługuje komunikaty informacyjne, nadpisania i anulowania. `XetraCalendarSourceAdapter::import_notice` pobiera komunikat z zatwierdzonego hosta HTTPS, archiwizuje dokładne bajty jako obiekt SHA-256 i zapisuje obiekt oraz komunikat w jednej partii. Powtórny import identycznej rewizji jest idempotentny; zmiana bajtów lub interpretacji dla tej samej rewizji jest odrzucana. Testy używają wyłącznie syntetycznych fixture offline.
