# Rozdzielenie dokumentu źródłowego i rewizji kalendarza

## Cel

Rozdzielić tożsamość niezmiennych bajtów źródła od wersji semantycznej kalendarza, która została zinterpretowana z tych bajtów. Numer wersji kalendarza nie może być używany jako numer wersji pliku źródłowego.

## Obecny stan

`SourceCalendarDocument` łączy obecnie:
- tożsamość dokumentu i wersję kalendarza (`document_id`, `version`);
- zakres, venue i strefę interpretacji (`venue`, `valid_from`, `valid_to_exclusive`, `timezone`);
- pochodzenie i dane pobrania (`source_uri`, `retrieved_at`, `content_sha256`);
- dane archiwum (`archive_relative_path`, `content_length_bytes`, `content_type`);
- atrybucję i status licencji.

Archiwum ma ścieżkę opartą o `document_id/version/hash`. Powtórne pobranie tych samych bajtów przy kolejnym numerze wersji może utworzyć drugi plik, mimo że treść źródłowa jest identyczna. Klucz `document_id + version` pełni jednocześnie rolę rewizji dokumentu i kalendarza.

## Model docelowy

### `SourceArchiveObject` — niezmienny obiekt źródłowy

- `source_object_id`: stabilny identyfikator treści, domyślnie `sha256:<hex>`;
- `content_sha256`: SHA-256 dokładnych bajtów pobranego dokumentu;
- `archive_relative_path`: ścieżka względna do obiektu treści;
- `content_length_bytes`;
- `content_type`;
- `source_uri`: adres, spod którego pobrano te bajty;
- `retrieved_at`: czas konkretnego pobrania/zaobserwowania obiektu;
- `attribution` i `license_status`.

Obiekt jest niezmienny. Dwa kalendarze i wiele rewizji mogą wskazywać na ten sam obiekt źródłowy. Deduplikacja treści opiera się na SHA-256, nie na numerze rewizji kalendarza.

### `VenueCalendarRevision` — interpretacja semantyczna

- `calendar_id`: stabilny identyfikator kalendarza venue;
- `revision`: rosnący numer rewizji interpretacji;
- `source_object_id`: referencja do istniejącego `SourceArchiveObject`;
- `venue`, zakres dat i strefa IANA;
- komplet dni i sesji produktowych;
- opcjonalne informacje o parserze/wersji reguł interpretacji.

Rewizja jest niezmienna. Zmiana bajtów źródła nie musi automatycznie zmieniać kalendarza, jeżeli interpretacja semantyczna pozostaje identyczna. Zmiana dni/sesji lub reguł interpretacji wymaga nowej rewizji.

## Zasady migracji

1. Zachować odczyt JSON schema v1 bez wymuszania natychmiastowej migracji.
2. Pola archiwum pozostają opcjonalne dla rekordów historycznych.
3. Wprowadzić osobną kolekcję `source_objects`; nie usuwać ani nie reinterpretować istniejącego `venue_calendars`.
4. Stare rekordy bez `source_object_id` pozostają rekordami legacy. Nie tworzyć fikcyjnych obiektów na podstawie URL ani sparsowanego kalendarza.
5. Nowe importy zapisują obiekt źródłowy przed publikacją rewizji kalendarza.
6. Referencja rewizji do obiektu musi być sprawdzana przy walidacji migawki; brak obiektu lub niezgodność SHA blokuje zapis/odczyt.
7. Deduplikacja obiektów jest globalna w obrębie repozytorium po pełnym SHA-256. Ten sam hash o innym MIME lub rozmiarze jest konfliktem integralności, a nie nowym obiektem.
8. Zmiana parsera bez zmiany źródła może tworzyć nową rewizję kalendarza wskazującą na ten sam obiekt źródłowy.
9. Nie usuwać obiektów źródłowych, dopóki jakakolwiek rewizja lub historia audytowa na nie wskazuje.
10. Zachować dotychczasowy import i API adaptera przez warstwę zgodności do czasu przejścia wszystkich wywołań.

## Plan implementacji

- 15A: dodać model obiektu źródłowego, kolekcję migawki/partii, serializację i walidację referencji; testy round-trip i zgodności v1.
- 15B: przenieść archiwizację do ścieżek content-addressed `*.sources/objects/<sha256>.<ext>` i podłączyć adapter Xetra do obiektu źródłowego; testy współdzielenia obiektu przez rewizje.
- 15C: oddzielić `calendar_id + revision` od `document_id + source version`; dodać migrację i testy zmian parsera bez ponownego pobierania.
- 15D: dodać kontrolę osieroconych obiektów jako raport diagnostyczny; bez automatycznego usuwania.

## Kryteria akceptacji

- Jedna kopia bajtów dla identycznego SHA-256 w obrębie repozytorium.
- Wiele rewizji kalendarza może bezpiecznie wskazywać na jeden obiekt.
- Zmiana parsera może utworzyć nową rewizję bez duplikowania pliku.
- Zmiana bajtów tworzy nowy obiekt i wymaga jawnej interpretacji.
- Każda referencja jest walidowana; błędny SHA, brak pliku, niezgodny rozmiar, traversal i symlink są odrzucane.
- Stare rekordy JSON v1 nadal są czytelne.
- Pełny CI przechodzi dla konkretnego SHA.