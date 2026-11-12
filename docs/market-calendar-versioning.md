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
- Obiekt zawiera wyłącznie niezmienne metadane bajtów: skrót, długość, typ treści i ścieżkę archiwum. URI, czas pobrania, atrybucja i status licencji pozostają w metadanych pochodzenia kalendarza, ponieważ mogą różnić się między pobraniami tych samych bajtów.

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
## Status implementacji

### Etap 15A — zakończony

- Dodano `SourceArchiveObject`, którego tożsamość ma postać `sha256:<64 lowercase hex>`. Obiekt przechowuje wyłącznie niezmienne metadane bajtów: skrót, długość, typ treści i względną ścieżkę archiwum. URI, czas pobrania, atrybucja i licencja pozostają w `SourceCalendarDocument`, ponieważ opisują pochodzenie konkretnego pobrania.
- Dodano opcjonalną kolekcję `source_objects` w `MarketDataSnapshot` i `MarketDataBatch` oraz operację `upsert_source_object`. Nowa kolekcja jest opcjonalna przy odczycie JSON v1, więc starsze dokumenty pozostają czytelne.
- `SourceCalendarDocument.source_object_id` jest opcjonalnym odwołaniem do istniejącego obiektu; walidacja wymaga zgodności identyfikatora i SHA-256.
- Obiekty są niezmienne, deduplikowane po SHA-256 i sprawdzane przy zapisie/odczycie. Ten sam hash z innymi metadanymi bajtów jest konfliktem. Nazwa pliku archiwum musi zaczynać się od pełnego SHA-256.
- Testy obejmują round-trip, dwie rewizje kalendarza współdzielące jeden obiekt, konflikt metadanych, brakującą referencję, próbę usunięcia obiektu, błędną nazwę ścieżki oraz brak/uszkodzenie pliku.
- CI dla SHA `ede4b0f29cec1db11b89e20394254dfd4780e52b`: [43318104387](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43318104387) — Configure, Build i Test: success; CTest **9/9**.

### Etap 15B — następny

- Przenieść zapis archiwum Xetra na ścieżkę content-addressed `*.sources/objects/<sha256>.<ext>` i podłączyć importer do `SourceArchiveObject`.
- Dla tego samego SHA ponowne pobranie nie może tworzyć kopii pliku. Różne rewizje kalendarza mogą wskazywać ten sam obiekt.
- Zapisywać obiekt przed publikacją kalendarza; awaria importu nie może pozostawić referencji do nieistniejących bajtów.
- Zachować obsługę dotychczasowych archiwów `document_id/version/hash` jako legacy do czasu jawnej migracji.
### Etap 15B — archiwum content-addressed i integracja adaptera Xetra

- Adapter Xetra zapisuje dokładne bajty do `calendar.json.sources/objects/<sha256>.html`, niezależnie od numeru rewizji kalendarza.
- `SourceArchiveObject` i `VenueSessionCalendar` są publikowane w jednej partii repozytorium. Archiwum jest tworzone przed zapisem metadanych; przerwanie może pozostawić osierocony plik, ale nie referencję do brakujących bajtów.
- Powtórne pobranie identycznych bajtów ponownie wykorzystuje istniejący plik i obiekt. Nowy hash tworzy nowy obiekt; kolejne rewizje mogą wskazywać ten sam identyfikator treści.
- Publikacja pliku jest atomowa i bez nadpisywania istniejącego obiektu: tymczasowy plik jest zapisywany i synchronizowany, następnie publikowany przez `link()` (no-clobber), a katalog jest synchronizowany. Kolizja istniejącego SHA wymaga identyczności bajtów; symlink i konflikt treści są odrzucane.
- Importer sam ustala SHA, czas pobrania, referencję obiektu i metadane archiwum. Wywołujący nie może podać tych pól z góry.
- Legacy ścieżki `document_id/version/hash` i opcjonalne pola manifestu w `SourceCalendarDocument` pozostają obsługiwane dla starszych rekordów.
- CI dla SHA `f61f1bd103348ff1c07c0133f22e4a75503ae573`: [43319017765](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43319017765) — Configure, Build i Test: success; CTest **9/9**.

### Etap 15C — następny

- Rozdzielić `calendar_id + revision` od `document_id` i ewentualnej wersji dokumentu źródłowego. Nie zmieniać klucza legacy `document_id + version` bez migracji.
- Umożliwić nową interpretację/parser revision dla istniejącego `source_object_id` bez ponownego pobierania i kopiowania pliku.
- Wprowadzić migrację zgodną z JSON v1, testy wielokrotnego wskazania jednego źródła oraz reguły konfliktów rewizji semantycznych.
### Etap 15C — oddzielne wersjonowanie źródła i kalendarza

- `VenueSessionCalendar` ma własne `calendar_id` i `revision`; `SourceCalendarDocument.version` opisuje wersję dokumentu źródłowego. Dotychczasowy klucz `document_id + version` pozostaje ścieżką zgodności dla rekordów legacy.
- Odczyt starszego JSON v1 bez `calendar_id` i `revision` materializuje te pola z `source.document_id` i `source.version`. Nowy zapis serializuje jawne pola kalendarza.
- Walidacja wymaga rosnących rewizji w obrębie `calendar_id`, stabilnego przypisania kalendarza do jednego venue oraz niezmienności mapowania `document_id + source version -> SHA-256`.
- `parser_version` jest zapisywany przy interpretacji kalendarza; parser Xetra oznacza obecnie wynik jako `xetra-html-2026-v1`.
- Nowy overload `import(..., calendar_id, calendar_revision)` rozdziela źródłową wersję dokumentu od rewizji semantycznej. Stary overload zachowuje dotychczasowe zachowanie.
- `publish_revision_from_archive(...)` umożliwia ponowne sparsowanie już zarchiwizowanych bajtów bez HTTP. Odczyt obiektu używa `openat(O_NOFOLLOW)` dla każdego komponentu, sprawdza typ pliku, rozmiar i SHA-256.
- Testy obejmują trzy rewizje z tym samym źródłem, ponowną interpretację bez żądania HTTP, parser-version round-trip, migrację legacy, konflikt wersji źródła z innym SHA oraz odrzucenie podmiany już opublikowanej rewizji.
- CI dla SHA `ae4a852fde232cb7108e5161ec396a1f80ea0072`: [43321359514](https://github.com/bartoszruta26-droid/Monitor/actions/runs/43321359514) — Configure, Build i Test: success; CTest **9/9**.

### Etap 16 — następny

- Uogólnić parser Xetra z roku 2026 na zakres `valid_from` / `valid_to_exclusive`; rok nie może być zaszyty w selektorach, etykietach ani oczekiwanych datach.
- Wprowadzić fixture offline dla każdego obsługiwanego roku i walidację niejednoznacznych tabel/lat.
- Nie wyprowadzać dat sesji z samych nazw świąt; wymagane są dane źródłowe i oficjalne komunikaty dla skróconych/wyjątkowych sesji.

## Etap 15D — diagnostyka osieroconych obiektów

`find_orphaned_source_objects(snapshot)` zwraca obiekty `source_objects`, do których nie odwołuje się żadna rewizja kalendarza venue ani komunikat sesyjny. Jest to raport manifestu, a nie procedura sprzątająca: nie usuwa plików, nie zmienia repozytorium i nie wyszukuje plików spoza indeksu JSON. Usuwanie obiektów pozostaje zabronione przez politykę niezmienności; decyzja o retencji wymaga osobnego, audytowalnego procesu.
