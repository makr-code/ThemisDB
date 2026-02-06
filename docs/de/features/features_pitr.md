# Point-in-Time Recovery (PITR) für ThemisDB

**Version:** 1.0  
**Datum:** 12. Januar 2026  
**Status:** ✅ Implementiert  
**Kategorie:** Features

---

## Inhaltsverzeichnis

- [Überblick](#überblick)
- [Hauptfunktionen](#hauptfunktionen)
- [Architektur](#architektur)
- [Nutzungsleitfaden](#nutzungsleitfaden)
- [Übersicht](#übersicht)
- [Hauptfunktionen](#hauptfunktionen)
- [Architektur](#architektur)
- [Verwendungsleitfaden](#verwendungsleitfaden)
  - [Named Snapshots](#named-snapshots)
  - [Point-in-Time Recovery](#point-in-time-recovery)
  - [Dry-Run-Vorschau](#dry-run-vorschau)
- [API-Referenz](#api-referenz)
- [Best Practices](#best-practices)
- [Fehlerbehebung](#fehlerbehebung)

---

## Überblick

Point-in-Time Recovery (PITR) ist ein Git-ähnliches Feature für ThemisDB's MVCC-System, das Datenbankadministratoren ermöglicht:

- **Named Snapshots erstellen** (semantische Tags) zu wichtigen Zeitpunkten
- **Datenbank wiederherstellen** zu jedem vorherigen Zustand via Sequence, Tag oder Timestamp
- **Wiederherstellungsoperationen vorschauen** vor der Anwendung
## Übersicht

Point-in-Time Recovery (PITR) ist eine Git-ähnliche Funktion für das MVCC-System von ThemisDB, die es Datenbankadministratoren ermöglicht:

- **Benannte Snapshots erstellen** (semantische Tags) zu wichtigen Zeitpunkten
- **Datenbank wiederherstellen** zu jedem vorherigen Zustand nach Sequenz, Tag oder Zeitstempel
- **Wiederherstellungsoperationen in der Vorschau anzeigen** vor der Anwendung
- **Selektiv wiederherstellen** bestimmte Tabellen

### Warum PITR?

PITR bietet kritische Funktionen für:

- **Disaster Recovery**: Wiederherstellung nach Datenkorruption oder versehentlichen Löschungen
- **Schema Migration Rollback**: Rückgängigmachen fehlgeschlagener Schema-Änderungen
- **Compliance**: Historische Audit-Punkte beibehalten
- **Schema-Migrations-Rollback**: Rückgängigmachen fehlgeschlagener Schemaänderungen
- **Compliance**: Pflege historischer Audit-Punkte
- **Testing**: Wiederherstellung zu bekannten guten Zuständen
- **DevOps**: Sichere Punkte vor Deployments

---

## Hauptfunktionen

### 1. Named Snapshots (Semantisches Tagging)

Unveränderliche Snapshots mit aussagekräftigen Namen erstellen:

```cpp
// Snapshot vor kritischer Operation erstellen
snapshot_mgr.createTag("before_migration", "Vor Q1 2026 Schema-Migration");
Erstellen Sie unveränderliche Snapshots mit aussagekräftigen Namen:

```cpp
// Snapshot vor kritischer Operation erstellen
snapshot_mgr.createTag("before_migration", "Before Q1 2026 schema migration");

// Alle Snapshots auflisten
auto snapshots = snapshot_mgr.listTags(true); // Nach Zeit sortieren

// Spezifischen Snapshot abrufen
// Bestimmten Snapshot abrufen
auto snapshot = snapshot_mgr.getTag("before_migration");

// Alte Snapshots löschen
snapshot_mgr.deleteTag("old_snapshot");
```

**Features:**
- ✅ Persistente Speicherung in RocksDB "tags" Column Family
- ✅ Validierung von Tag-Namen (Kleinbuchstaben, alphanumerisch, Bindestriche, Unterstriche)
- ✅ Beschreibung bis 500 Zeichen
- ✅ Automatische Timestamp- und Sequence-Nummer-Erfassung
- ✅ Benutzer-Attribution
**Funktionen:**
- ✅ Persistente Speicherung in RocksDB "tags" Column Family
- ✅ Validierung von Tag-Namen (Kleinbuchstaben, alphanumerisch, Bindestriche, Unterstriche)
- ✅ Beschreibung bis zu 500 Zeichen
- ✅ Automatische Zeitstempel- und Sequenznummernerfassung
- ✅ Benutzerzuordnung

### 2. Point-in-Time Recovery

Datenbank zu jedem vorherigen Zustand wiederherstellen:

```cpp
// Zu benanntem Snapshot wiederherstellen
pitr_mgr.restoreToTag("before_migration");

// Zu spezifischer Sequence-Nummer wiederherstellen
pitr_mgr.restoreToSequence(12345);

// Zu spezifischem Timestamp wiederherstellen
pitr_mgr.restoreToTimestamp(1705045200000); // Unix-Timestamp in ms
```

**Features:**
- ✅ Wiederherstellung via Sequence-Nummer, Tag-Name oder Timestamp
// Zu bestimmter Sequenznummer wiederherstellen
pitr_mgr.restoreToSequence(12345);

// Zu bestimmtem Zeitstempel wiederherstellen
pitr_mgr.restoreToTimestamp(1705045200000); // Unix-Zeitstempel in ms
```

**Funktionen:**
- ✅ Wiederherstellung nach Sequenznummer, Tag-Name oder Zeitstempel
- ✅ Automatisches Backup vor Wiederherstellung
- ✅ Rollback bei Fehlern
- ✅ Fortschrittsverfolgung
- ✅ Selektive Tabellenwiederherstellung

### 3. Sicherheitsfunktionen

Mehrere Schutzebenen:

```cpp
PITRManager::RestoreOptions options;
options.dry_run = true;              // Nur Vorschau, nicht anwenden
options.create_backup = true;        // Auto-Backup vor Wiederherstellung
options.abort_on_first_error = true; // Bei erstem Fehler stoppen
options.tables = {"users", "orders"}; // Selektive Wiederherstellung
options.max_events_to_replay = 10000; // Events limitieren

// Wiederherstellung vorschauen
auto preview = pitr_mgr.previewRestore(target_sequence, options);
std::cout << "Wiederherzustellende Events: " << preview.events_to_replay << "\n";
std::cout << "Betroffene Tabellen: " << preview.affected_tables.size() << "\n";
std::cout << "Geschätzte Dauer: " << preview.estimated_duration_sec << "s\n";

// Wiederherstellung anwenden wenn Preview gut aussieht
options.max_events_to_replay = 10000; // Events begrenzen

// Wiederherstellung in der Vorschau anzeigen
auto preview = pitr_mgr.previewRestore(target_sequence, options);
std::cout << "Events to replay: " << preview.events_to_replay << "\n";
std::cout << "Affected tables: " << preview.affected_tables.size() << "\n";
std::cout << "Estimated duration: " << preview.estimated_duration_sec << "s\n";

// Wiederherstellung anwenden, wenn Vorschau gut aussieht
auto status = pitr_mgr.restoreToSequence(target_sequence, options);
```

---

## Architektur

### Komponenten

```
┌──────────────────────────────────────────┐
│         SnapshotManager                   │
│  - createTag(name, description)          │
│  - getTag(name)                          │
│  - listTags()                            │
│  - deleteTag(name)                       │
└──────────────┬───────────────────────────┘
               │
               │ Verwendet
               │
┌──────────────▼───────────────────────────┐
│          PITRManager                      │
│  - restoreToSequence(seq)                │
│  - restoreToTag(name)                    │
│  - restoreToTimestamp(ts)                │
│  - previewRestore(seq)                   │
└──────────────┬───────────────────────────┘
               │
      ┌────────┴────────┐
      │                 │
┌─────▼─────┐    ┌──────▼──────┐
│Changefeed │    │ RocksDB     │
│           │    │ TransactionDB│
└───────────┘    └─────────────┘
```

### Datenfluss

1. **Snapshot-Erstellung**:
   - Aktuelle Changefeed-Sequence erfassen
   - Aktuelle Changefeed-Sequenz erfassen
   - Tag-Metadaten in "tags" Column Family speichern
   - In RocksDB persistieren

2. **Point-in-Time Recovery**:
   - Ziel-Sequence abrufen (von Tag/Timestamp)
   - Auto-Backup Snapshot erstellen
   - Changefeed-Events rückwärts abspielen
   - Umgekehrte Operationen anwenden (PUT→DELETE)
   - Committen oder Rollback
   - Zielsequenz abrufen (von Tag/Zeitstempel)
   - Auto-Backup-Snapshot erstellen
   - Changefeed-Events rückwärts abspielen
   - Umkehroperationen anwenden (PUT→DELETE)
   - Commit oder Rollback

### Speicherung

**Tags Column Family**:
```
Key: tag:{tag_name}
Value: {
  "tag_name": "before_migration",
  "sequence_number": 12345,
  "timestamp_ms": 1705045200000,
  "description": "Vor Q1 2026 Schema-Migration",
  "description": "Before Q1 2026 schema migration",
  "created_by": "admin"
}
```

---

## Nutzungsleitfaden

### REST API Beispiele

#### Snapshot erstellen

```bash
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "before_deploy_v2.0",
    "description": "Vor Deployment Version 2.0",
    "created_by": "admin"
  }'
## Verwendungsleitfaden

### Named Snapshots

#### Snapshot erstellen

```cpp
#include "transaction/snapshot_manager.h"

// Initialisieren
SnapshotManager snapshot_mgr(db, tags_cf, changefeed);

// Snapshot erstellen
auto status = snapshot_mgr.createTag(
    "quarterly_backup_2026_q1",
    "Q1 2026 quarterly backup for compliance",
    "admin"
);

if (!status.ok) {
    std::cerr << "Failed to create snapshot: " << status.message << "\n";
}
```

#### Snapshots auflisten

```bash
curl http://localhost:8080/api/v1/snapshots/tags
```

#### Zu Tag wiederherstellen

```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "restore_type": "tag",
    "target": "before_deploy_v2.0",
    "dry_run": false,
    "create_backup": true
  }'
```

#### Wiederherstellung vorschauen

```bash
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -H "Content-Type: application/json" \
  -d '{
    "restore_type": "sequence",
    "target": "12345"
  }'
```

#### Fortschritt abrufen

```bash
curl http://localhost:8080/api/v1/restore/progress
```cpp
// Alle Snapshots nach Zeit sortiert auflisten (neueste zuerst)
auto snapshots = snapshot_mgr.listTags(true);

for (const auto& snap : snapshots) {
    std::cout << "Tag: " << snap.tag_name << "\n";
    std::cout << "  Sequence: " << snap.sequence_number << "\n";
    std::cout << "  Time: " << snap.timestamp_ms << "\n";
    std::cout << "  Description: " << snap.description << "\n";
    std::cout << "  Created by: " << snap.created_by << "\n\n";
}

// Stattdessen nach Name sortieren
auto snapshots_by_name = snapshot_mgr.listTags(false);
```

#### Snapshot-Statistiken abrufen

```cpp
auto stats = snapshot_mgr.getStats();

std::cout << "Total snapshots: " << stats.total_snapshots << "\n";
std::cout << "Storage size: " << stats.total_size_bytes << " bytes\n";
std::cout << "Oldest: " << stats.oldest_timestamp_ms << "\n";
std::cout << "Newest: " << stats.newest_timestamp_ms << "\n";
```

### Point-in-Time Recovery

#### Zu benanntem Snapshot wiederherstellen

```cpp
#include "storage/pitr_manager.h"

// Initialisieren
PITRManager pitr_mgr(db_wrapper, changefeed, snapshot_mgr);

// Wiederherstellungsoptionen konfigurieren
PITRManager::RestoreOptions options;
options.create_backup = true;       // Auto-Backup vor Wiederherstellung
options.backup_tag = "before_restore_2026_01_12";

// Wiederherstellen
auto status = pitr_mgr.restoreToTag("before_migration", options);

if (status.ok) {
    std::cout << "Restore completed successfully\n";
    if (status.progress.has_value()) {
        std::cout << "Events processed: " << status.progress->events_processed << "\n";
        std::cout << "Duration: " << status.progress->getElapsedMs() << "ms\n";
    }
} else {
    std::cerr << "Restore failed: " << status.message << "\n";
}
```

#### Zu bestimmter Sequenz wiederherstellen

```cpp
uint64_t target_sequence = 12000;

PITRManager::RestoreOptions options;
options.create_backup = true;

auto status = pitr_mgr.restoreToSequence(target_sequence, options);
```

#### Zu Zeitstempel wiederherstellen

```cpp
// Wiederherstellen zu 10. Januar 2026, 12:00:00 UTC
int64_t timestamp_ms = 1704888000000;

PITRManager::RestoreOptions options;
options.create_backup = true;

auto status = pitr_mgr.restoreToTimestamp(timestamp_ms, options);
```

### Dry-Run-Vorschau

Immer Vorschau vor Wiederherstellung anzeigen:

```cpp
uint64_t target_sequence = 10000;

PITRManager::RestoreOptions options;
options.dry_run = true; // Nur Vorschau

auto preview = pitr_mgr.previewRestore(target_sequence, options);

std::cout << "Restore Preview:\n";
std::cout << "  Target sequence: " << preview.target_sequence << "\n";
std::cout << "  Current sequence: " << preview.current_sequence << "\n";
std::cout << "  Events to replay: " << preview.events_to_replay << "\n";
std::cout << "  Affected tables: ";
for (const auto& table : preview.affected_tables) {
    std::cout << table << " ";
}
std::cout << "\n";
std::cout << "  Sample keys (first 10):\n";
for (size_t i = 0; i < std::min(preview.affected_keys.size(), size_t(10)); ++i) {
    std::cout << "    - " << preview.affected_keys[i] << "\n";
}
std::cout << "  Estimated duration: " << preview.estimated_duration_sec << "s\n";
std::cout << "  Estimated size: " << preview.estimated_size_bytes << " bytes\n";

// Mit Benutzer bestätigen
std::cout << "Proceed? (yes/no): ";
std::string confirm;
std::cin >> confirm;

if (confirm == "yes") {
    options.dry_run = false;
    auto status = pitr_mgr.restoreToSequence(target_sequence, options);
}
```

### Selektive Wiederherstellung

Nur bestimmte Tabellen wiederherstellen:

```cpp
PITRManager::RestoreOptions options;
options.tables = {"users", "orders"}; // Nur diese Tabellen wiederherstellen
options.create_backup = true;

auto status = pitr_mgr.restoreToTag("before_migration", options);
```

### Fortschrittsverfolgung

Langwierige Wiederherstellungsoperationen überwachen:

```cpp
// Wiederherstellung im Hintergrund starten (würde Threading benötigen)
PITRManager::RestoreOptions options;
auto status = pitr_mgr.restoreToSequence(1000, options);

// Fortschritt prüfen
auto progress = pitr_mgr.getProgress();
if (progress.has_value()) {
    std::cout << "Phase: " << static_cast<int>(progress->phase) << "\n";
    std::cout << "Progress: " << progress->getProgressPercent() << "%\n";
    std::cout << "Events processed: " << progress->events_processed 
              << "/" << progress->total_events << "\n";
    std::cout << "Current table: " << progress->current_table << "\n";
    std::cout << "Elapsed: " << progress->getElapsedMs() << "ms\n";
}
```

---

## API-Referenz

### PITRManager API

**Restore-Optionen:**

```cpp
struct RestoreOptions {
    bool dry_run = false;                // Nur Vorschau
    bool create_backup = true;           // Auto-Backup
    bool abort_on_first_error = true;    // Bei Fehler stoppen
    std::vector<std::string> tables;     // Leer = alle Tabellen
    uint64_t max_events_to_replay = 0;   // 0 = unbegrenzt
    std::string backup_tag = "before_pitr_restore";
};
```

**Hauptmethoden:**

```cpp
// Zu Sequence wiederherstellen
Status restoreToSequence(uint64_t target_sequence, const RestoreOptions& options);

// Zu Tag wiederherstellen
Status restoreToTag(const std::string& tag_name, const RestoreOptions& options);

// Zu Timestamp wiederherstellen
Status restoreToTimestamp(int64_t timestamp_ms, const RestoreOptions& options);

// Wiederherstellung vorschauen
RestorePreview previewRestore(uint64_t target_sequence, const RestoreOptions& options);

// Fortschritt abrufen
std::optional<RestoreProgress> getProgress() const;
```

### REST API Endpunkte

| Endpunkt | Methode | Beschreibung |
|----------|---------|--------------|
| `/api/v1/snapshots/tags` | POST | Snapshot erstellen |
| `/api/v1/snapshots/tags` | GET | Snapshots auflisten |
| `/api/v1/snapshots/tags/:name` | GET | Snapshot abrufen |
| `/api/v1/snapshots/tags/:name` | DELETE | Snapshot löschen |
| `/api/v1/restore/pitr` | POST | Wiederherstellung ausführen |
| `/api/v1/restore/preview` | POST | Wiederherstellung vorschauen |
| `/api/v1/restore/progress` | GET | Fortschritt abrufen |
### SnapshotManager

#### createTag
```cpp
Status createTag(const std::string& tag_name,
                const std::string& description,
                const std::string& created_by = "system");
```
Erstellt einen neuen Snapshot-Tag.

**Parameter:**
- `tag_name`: Eindeutige Kennung (Kleinbuchstaben, alphanumerisch, Bindestriche, Unterstriche, 1-64 Zeichen)
- `description`: Lesbare Beschreibung (max 500 Zeichen)
- `created_by`: Benutzerkennung (optional)

**Rückgabe:** Status, der Erfolg oder Fehler anzeigt

**Fehler:**
- `ALREADY_EXISTS`: Tag-Name existiert bereits
- `INVALID_NAME`: Tag-Name enthält ungültige Zeichen
- `INVALID_DESCRIPTION`: Beschreibung zu lang

#### getTag
```cpp
std::optional<Snapshot> getTag(const std::string& tag_name) const;
```
Ruft Snapshot-Metadaten nach Tag-Name ab.

**Rückgabe:** Snapshot falls vorhanden, nullopt andernfalls

#### listTags
```cpp
std::vector<Snapshot> listTags(bool sort_by_time = true) const;
```
Listet alle Snapshots auf.

**Parameter:**
- `sort_by_time`: Falls true, nach Zeitstempel sortieren (neueste zuerst), andernfalls nach Name

**Rückgabe:** Vektor aller Snapshots

#### deleteTag
```cpp
Status deleteTag(const std::string& tag_name);
```
Löscht einen Snapshot-Tag.

**Rückgabe:** Status, der Erfolg oder Fehler anzeigt

**Fehler:**
- `NOT_FOUND`: Tag existiert nicht

#### getStats
```cpp
Stats getStats() const;
```
Ruft Statistiken über Snapshots ab.

**Rückgabe:** Stats-Struktur mit Gesamtanzahl, Speichergröße und Zeitstempelbereich

### PITRManager

#### restoreToSequence
```cpp
Status restoreToSequence(uint64_t target_sequence, 
                        const RestoreOptions& options = {});
```
Stellt Datenbank zu bestimmter Sequenznummer wieder her.

**Parameter:**
- `target_sequence`: Zielsequenz zur Wiederherstellung
- `options`: Wiederherstellungsoptionen (dry_run, backup, etc.)

**Rückgabe:** Status mit Fortschrittsinformationen

**Fehler:**
- `INVALID_SEQUENCE`: Zielsequenz ist ungültig oder in der Zukunft
- `BACKUP_FAILED`: Auto-Backup-Erstellung fehlgeschlagen
- `REPLAY_FAILED`: Event-Replay fehlgeschlagen

#### restoreToTag
```cpp
Status restoreToTag(const std::string& tag_name, 
                   const RestoreOptions& options = {});
```
Stellt Datenbank zu benanntem Snapshot-Tag wieder her.

**Parameter:**
- `tag_name`: Tag-Kennung
- `options`: Wiederherstellungsoptionen

**Rückgabe:** Status mit Fortschrittsinformationen

**Fehler:**
- `TAG_NOT_FOUND`: Tag existiert nicht
- (plus alle Fehler von restoreToSequence)

#### restoreToTimestamp
```cpp
Status restoreToTimestamp(int64_t timestamp_ms, 
                         const RestoreOptions& options = {});
```
Stellt Datenbank zu bestimmtem Zeitstempel wieder her.

**Parameter:**
- `timestamp_ms`: Unix-Zeitstempel in Millisekunden
- `options`: Wiederherstellungsoptionen

**Rückgabe:** Status mit Fortschrittsinformationen

**Fehler:**
- `NO_EVENTS_AT_TIME`: Keine Events zur oder vor dem Zeitstempel gefunden
- (plus alle Fehler von restoreToSequence)

#### previewRestore
```cpp
RestorePreview previewRestore(uint64_t target_sequence, 
                             const RestoreOptions& options = {}) const;
```
Zeigt Vorschau einer Wiederherstellungsoperation (Dry-Run).

**Parameter:**
- `target_sequence`: Zielsequenz zur Wiederherstellung
- `options`: Wiederherstellungsoptionen (nur Tabellenfilter wird verwendet)

**Rückgabe:** Vorschau mit geschätzter Auswirkung

---

## Best Practices

### 1. Regelmäßige Snapshots

```bash
# Tägliche automatische Snapshots
0 2 * * * curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d "{\"tag_name\": \"daily_$(date +\%Y\%m\%d)\", \"description\": \"Tägliches Backup\"}"
```

### 2. Vor kritischen Operationen

Immer Snapshot erstellen vor:
- Schema-Migrationen
- Bulk-Datenoperationen
- Großen Deployments
- Produktionsupdates

### 3. Namenskonventionen

Konsistente Benennung verwenden:
- `before_migration_YYYYMMDD`
- `before_deploy_vX.Y.Z`
- `emergency_YYYYMMDD_HHMMSS`
- `daily_YYYYMMDD`

### 4. Immer Dry-Run zuerst

```bash
# Immer zuerst vorschauen
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -d '{"restore_type": "tag", "target": "before_migration"}'

# Dann ausführen wenn OK
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -d '{"restore_type": "tag", "target": "before_migration", "dry_run": false}'
```

### 5. Snapshot-Aufbewahrung

Alte Snapshots regelmäßig aufräumen:
- Tägliche Backups: 30 Tage behalten
- Wöchentliche Backups: 90 Tage behalten
- Monatliche Backups: 1 Jahr behalten
- Migrations-Snapshots: Nach erfolgreicher Migration entfernen
### 1. Immer Backups erstellen

Snapshots vor kritischen Operationen erstellen:

```cpp
// Vor Schema-Migration
snapshot_mgr.createTag("before_migration_2026_01", "Before user schema v2.0");
migrate_schema();

// Vor großem Datenimport
snapshot_mgr.createTag("before_import_2026_01", "Before bulk import");
import_data();
```

### 2. Dry-Run zuerst verwenden

Wiederherstellungsoperationen immer vorher in der Vorschau anzeigen:

```cpp
// Zuerst Vorschau
PITRManager::RestoreOptions preview_opts;
preview_opts.dry_run = true;
auto preview = pitr_mgr.restoreToTag("before_migration", preview_opts);

// Überprüfen und bestätigen
if (preview.events_to_replay < 10000) {
    preview_opts.dry_run = false;
    pitr_mgr.restoreToTag("before_migration", preview_opts);
}
```

### 3. Aufbewahrungsrichtlinien implementieren

Alte Snapshots regelmäßig löschen:

```cpp
auto snapshots = snapshot_mgr.listTags(true); // Nach Zeit sortiert

// Nur letzte 10 Snapshots behalten
if (snapshots.size() > 10) {
    for (size_t i = 10; i < snapshots.size(); ++i) {
        snapshot_mgr.deleteTag(snapshots[i].tag_name);
    }
}
```

### 4. Selektive Wiederherstellung verwenden

Nur betroffene Tabellen wiederherstellen:

```cpp
PITRManager::RestoreOptions options;
options.tables = {"users"}; // Nur Users-Tabelle wiederherstellen
options.create_backup = true;

pitr_mgr.restoreToTag("before_user_migration", options);
```

### 5. Fortschritt überwachen

Fortschritt bei langen Operationen verfolgen:

```cpp
// Prüfen, ob Wiederherstellung läuft
if (pitr_mgr.isRestoreInProgress()) {
    auto progress = pitr_mgr.getProgress();
    if (progress.has_value()) {
        log_progress(progress.value());
    }
}
```

### 6. Namenskonvention für Tags

Konsistente Tag-Benennung verwenden:

```cpp
// Gut:
"before_migration_2026_q1"
"quarterly_backup_2026_01"
"hotfix_rollback_2026_01_12"

// Schlecht:
"Backup1"  // Großbuchstaben nicht erlaubt
"backup 1" // Leerzeichen nicht erlaubt
"backup@1" // Sonderzeichen nicht erlaubt
```

---

## Fehlerbehebung

### Problem: Tag existiert bereits

**Lösung:** Anderen Namen verwenden oder alten Tag löschen

```bash
curl -X DELETE http://localhost:8080/api/v1/snapshots/tags/old_tag
```

### Problem: Wiederherstellung schlägt fehl

**Ursachen:**
- Ziel-Sequence ungültig
- Unzureichender Speicherplatz
- Changefeed-Daten nicht verfügbar

**Lösung:** Logs prüfen und Preview verwenden

### Problem: Langsame Wiederherstellung

**Lösungen:**
- Selektive Tabellenwiederherstellung verwenden
- Events limitieren mit `max_events_to_replay`
- Nähere Ziel-Sequence wählen

---

## Performance-Metriken

### Typische Wiederherstellungszeiten

| Events | Datengröße | Geschätzte Dauer |
|--------|-----------|------------------|
| 1.000 | ~10 MB | 1-5 Sekunden |
| 10.000 | ~100 MB | 10-30 Sekunden |
| 100.000 | ~1 GB | 1-5 Minuten |
| 1.000.000 | ~10 GB | 10-30 Minuten |

*Hinweis: Zeiten variieren je nach Hardware und Datenkomplexität*

### Optimierungstipps

1. **Selektive Wiederherstellung:** Nur benötigte Tabellen wiederherstellen
2. **Nähere Snapshots:** Snapshots häufiger erstellen
3. **Event-Limit:** `max_events_to_replay` setzen
4. **SSD-Speicher:** Schnellerer Speicher verbessert Performance

---

## Zusammenfassung

PITR für ThemisDB bietet:
- ✅ Git-ähnliche Snapshots und Wiederherstellung
- ✅ Mehrere Wiederherstellungsmodi (Sequence, Tag, Timestamp)
- ✅ Sichere Dry-Run Vorschau
- ✅ Automatische Backups
- ✅ Selektive Tabellenwiederherstellung
- ✅ Fortschrittsverfolgung
- ✅ REST API Integration

Für detaillierte Disaster Recovery Prozeduren siehe [Disaster Recovery Leitfaden](../guides/disaster_recovery.md).

---

**Dokumentenkontrolle:**
- **Version:** 1.0
- **Datum:** 12. Januar 2026
- **Status:** Produktionsbereit
- **Autor:** ThemisDB Development Team
### Problem: Tag-Erstellung schlägt fehl mit "Invalid tag name"

**Lösung:** Tag-Namen müssen Kleinbuchstaben, alphanumerisch, Bindestriche und Unterstriche sein:

```cpp
// Ungültig
snapshot_mgr.createTag("Backup_2026", "..."); // Großbuchstaben
snapshot_mgr.createTag("backup 2026", "..."); // Leerzeichen
snapshot_mgr.createTag("1backup", "..."); // Beginnt mit Ziffer

// Gültig
snapshot_mgr.createTag("backup_2026", "...");
snapshot_mgr.createTag("backup-2026", "...");
snapshot_mgr.createTag("b2026", "...");
```

### Problem: Wiederherstellung schlägt fehl mit "Target sequence must be less than current sequence"

**Lösung:** Sie können nicht zur aktuellen oder zukünftigen Sequenz wiederherstellen. Verwenden Sie eine vergangene Sequenz:

```cpp
uint64_t current = changefeed->getLatestSequence();
uint64_t target = current - 100; // Zu 100 Sequenzen zurück wiederherstellen

pitr_mgr.restoreToSequence(target);
```

### Problem: Wiederherstellung ist sehr langsam

**Lösung:** Verwenden Sie selektive Wiederherstellung und begrenzen Sie Events:

```cpp
PITRManager::RestoreOptions options;
options.tables = {"critical_table"}; // Nur eine Tabelle wiederherstellen
options.max_events_to_replay = 10000; // Events begrenzen

pitr_mgr.restoreToSequence(target, options);
```

### Problem: Fehler "Tag not found"

**Lösung:** Verifizieren Sie, dass Tag existiert, bevor Sie wiederherstellen:

```cpp
if (!snapshot_mgr.getTag("my_tag").has_value()) {
    std::cerr << "Tag 'my_tag' does not exist\n";
    
    // Verfügbare Tags auflisten
    auto tags = snapshot_mgr.listTags();
    std::cout << "Available tags:\n";
    for (const auto& tag : tags) {
        std::cout << "  - " << tag.tag_name << "\n";
    }
    return;
}

pitr_mgr.restoreToTag("my_tag");
```

### Problem: Speicherplatz wird knapp

**Lösung:** Aufbewahrungsrichtlinie implementieren und alte Snapshots löschen:

```cpp
// Statistiken abrufen
auto stats = snapshot_mgr.getStats();
std::cout << "Total size: " << stats.total_size_bytes << " bytes\n";

// Alte Snapshots löschen
auto snapshots = snapshot_mgr.listTags(true);
for (size_t i = 20; i < snapshots.size(); ++i) {
    snapshot_mgr.deleteTag(snapshots[i].tag_name);
}
```

---

## Verwandte Dokumentation

- [MVCC Architektur](../architecture/architecture_mvcc.md)
- [Changefeed Dokumentation](../cdc/changefeed.md)
- [Disaster Recovery Leitfaden](../guides/disaster_recovery.md)
- [Git-like Features Research](../research/GIT_LIKE_FEATURES_FOR_MVCC.md)

---

**Versionshistorie:**
- v1.0 (12. Januar 2026): Erste Implementierung
- Phase 3 der Git-ähnlichen Features für MVCC

**Autor:** ThemisDB Development Team  
**Lizenz:** MIT License (Community Edition)
