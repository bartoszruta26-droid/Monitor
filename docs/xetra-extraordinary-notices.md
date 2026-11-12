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