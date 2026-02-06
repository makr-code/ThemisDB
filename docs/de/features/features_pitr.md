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
- [API-Referenz](#api-referenz)
- [Best Practices](#best-practices)
- [Fehlerbehebung](#fehlerbehebung)

---

## Überblick

Point-in-Time Recovery (PITR) ist ein Git-ähnliches Feature für ThemisDB's MVCC-System, das Datenbankadministratoren ermöglicht:

- **Named Snapshots erstellen** (semantische Tags) zu wichtigen Zeitpunkten
- **Datenbank wiederherstellen** zu jedem vorherigen Zustand via Sequence, Tag oder Timestamp
- **Wiederherstellungsoperationen vorschauen** vor der Anwendung
- **Selektiv wiederherstellen** bestimmte Tabellen

### Warum PITR?

PITR bietet kritische Funktionen für:

- **Disaster Recovery**: Wiederherstellung nach Datenkorruption oder versehentlichen Löschungen
- **Schema Migration Rollback**: Rückgängigmachen fehlgeschlagener Schema-Änderungen
- **Compliance**: Historische Audit-Punkte beibehalten
- **Testing**: Wiederherstellung zu bekannten guten Zuständen
- **DevOps**: Sichere Punkte vor Deployments

---

## Hauptfunktionen

### 1. Named Snapshots (Semantisches Tagging)

Unveränderliche Snapshots mit aussagekräftigen Namen erstellen:

```cpp
// Snapshot vor kritischer Operation erstellen
snapshot_mgr.createTag("before_migration", "Vor Q1 2026 Schema-Migration");

// Alle Snapshots auflisten
auto snapshots = snapshot_mgr.listTags(true); // Nach Zeit sortieren

// Spezifischen Snapshot abrufen
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
   - Tag-Metadaten in "tags" Column Family speichern
   - In RocksDB persistieren

2. **Point-in-Time Recovery**:
   - Ziel-Sequence abrufen (von Tag/Timestamp)
   - Auto-Backup Snapshot erstellen
   - Changefeed-Events rückwärts abspielen
   - Umgekehrte Operationen anwenden (PUT→DELETE)
   - Committen oder Rollback

### Speicherung

**Tags Column Family**:
```
Key: tag:{tag_name}
Value: {
  "tag_name": "before_migration",
  "sequence_number": 12345,
  "timestamp_ms": 1705045200000,
  "description": "Vor Q1 2026 Schema-Migration",
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
