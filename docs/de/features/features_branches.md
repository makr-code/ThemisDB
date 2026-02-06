# Persistente Branches - Git-ähnliches Branch-Management für MVCC

**Version:** 1.5.0  
**Kategorie:** 🌿 Transaktionsverwaltung  
**Status:** ✅ Implementiert (Phase 4 - Optional)

---

## 📑 Inhaltsverzeichnis

- [Überblick](#überblick)
- [Features](#features)
- [API-Referenz](#api-referenz)
- [Verwendungsbeispiele](#verwendungsbeispiele)
- [Integration](#integration)
- [Performance](#performance)
- [Best Practices](#best-practices)

---

## Überblick

Persistente Branches bringen Git-ähnliche Branch-Semantik in ThemisDB's MVCC-System. Erstellen Sie benannte, persistente Branches für parallele Entwicklungsworkflows, Schema-Migrationstests, A/B-Testing-Szenarien und Was-wäre-wenn-Analysen.

### Kernfunktionen

- **Benannte Branches**: Erstellen Sie aussagekräftige Branch-Namen (z.B. `main`, `feature/new-schema`, `release/v2.0`)
- **Parent-Tracking**: Branches verfolgen ihren Parent für Abstammung und Merge-Operationen
- **Branch-Wechsel**: Wechseln Sie zwischen Branches, um in verschiedenen Datenbank-Kontexten zu arbeiten
- **Fast-Forward Merges**: Mergen Sie Branches mit Fast-Forward-Strategie
- **Persistente Speicherung**: Branches in RocksDB gespeichert, überleben Datenbank-Neustarts
- **Integration**: Funktioniert nahtlos mit Snapshots, Diff API und PITR

### Anwendungsfälle

- **Schema-Migrationstests**: Erstellen Sie einen Branch, um Schema-Änderungen zu testen, bevor Sie in main mergen
- **A/B-Testing**: Halten Sie parallele Datenzustände für Experimente
- **Was-wäre-wenn-Analyse**: Erstellen Sie Branches, um hypothetische Szenarien zu erkunden
- **Parallele Entwicklung**: Mehrere Teams arbeiten an isolierten Branches
- **Release-Management**: Erstellen Sie Release-Branches für stabile Versionen

---

## Features

### ✅ Implementiert

- Branch CRUD-Operationen (Erstellen, Lesen, Auflisten, Wechseln, Löschen)
- Persistente Speicherung in RocksDB
- Branch-Validierung (alphanumerisch, Bindestriche, Unterstriche, Schrägstriche, 1-128 Zeichen)
- Parent-Branch-Tracking
- Aktive Branch-Verwaltung
- Sortierung nach Name, Zeitstempel oder aktiv-Status
- Erstellen von Branches aus Tags, Sequenzen oder Zeitstempeln
- Fast-Forward-Merge-Unterstützung
- Statistik-API
- REST API Endpunkte

### 🔜 Zukünftige Erweiterungen

- Three-Way-Merge mit Konflikt-Erkennung
- Cherry-Pick-Operationen
- Branch-Rebase
- Branch-Schutzregeln
- Branch-Berechtigungen/Zugriffskontrolle

---

## API-Referenz

### Endpunkte

#### POST /api/v1/branches

Erstellen Sie einen neuen Branch.

**Request Body:**

```json
{
  "branch_name": "feature/new-schema",
  "parent_branch": "main",
  "description": "Branch zum Testen des neuen User-Schemas",
  "created_by": "alice",
  "from_tag": "v1.0.0",
  "set_active": false
}
```

**Parameter:**
- `branch_name` (erforderlich): Eindeutiger Branch-Identifier (1-128 Zeichen, alphanumerisch, `-`, `_`, `/`)
- `parent_branch` (optional): Parent-Branch-Name (Standard: "main")
- `description` (erforderlich): Lesbare Beschreibung
- `created_by` (optional): Benutzer/Service, der den Branch erstellt hat (Standard: "system")
- `from_tag` (optional): Branch aus einem benannten Snapshot erstellen
- `from_sequence` (optional): Branch aus spezifischer Sequenznummer erstellen
- `from_timestamp` (optional): Branch aus Zeitstempel erstellen
- `set_active` (optional): Branch sofort aktiv machen (Standard: false)

**Response (201 Created):**

```json
{
  "branch_name": "feature/new-schema",
  "parent_branch": "main",
  "creation_sequence": 12345,
  "creation_timestamp_ms": 1736657231000,
  "description": "Branch zum Testen des neuen User-Schemas",
  "created_by": "alice",
  "is_active": false
}
```

**Fehler-Antworten:**
- `400 Bad Request`: Branch existiert bereits, ungültiger Name oder Parent nicht gefunden
- `500 Internal Server Error`: Datenbank-Schreibfehler

---

#### GET /api/v1/branches

Listen Sie alle Branches auf.

**Query-Parameter:**
- `limit` (optional): Maximale Anzahl der zurückzugebenden Branches (0 = alle, Standard: 0)
- `sort_by` (optional): Sortierfeld: "name" (Standard), "timestamp", "active"
- `ascending` (optional): Sortierrichtung: true (Standard) oder false

**Beispiel-Request:**

```bash
GET /api/v1/branches?limit=10&sort_by=timestamp&ascending=false
```

**Response (200 OK):**

```json
[
  {
    "branch_name": "main",
    "parent_branch": "",
    "creation_sequence": 0,
    "creation_timestamp_ms": 1736657200000,
    "description": "Standard-Main-Branch",
    "created_by": "system",
    "is_active": true
  },
  {
    "branch_name": "feature/new-schema",
    "parent_branch": "main",
    "creation_sequence": 12345,
    "creation_timestamp_ms": 1736657231000,
    "description": "Branch zum Testen des neuen User-Schemas",
    "created_by": "alice",
    "is_active": false
  }
]
```

---

#### GET /api/v1/branches/:name

Abrufen spezifischer Branch-Metadaten.

**Beispiel-Request:**

```bash
GET /api/v1/branches/feature/new-schema
```

**Response (200 OK):**

```json
{
  "branch_name": "feature/new-schema",
  "parent_branch": "main",
  "creation_sequence": 12345,
  "creation_timestamp_ms": 1736657231000,
  "description": "Branch zum Testen des neuen User-Schemas",
  "created_by": "alice",
  "is_active": false
}
```

**Fehler-Antworten:**
- `404 Not Found`: Branch existiert nicht

---

#### GET /api/v1/branches/active

Aktuell aktiven Branch abrufen.

**Response (200 OK):**

```json
{
  "active_branch": "main"
}
```

---

#### POST /api/v1/branches/:name/switch

Zu einem anderen Branch wechseln.

**Beispiel-Request:**

```bash
POST /api/v1/branches/feature/new-schema/switch
```

**Response (200 OK):**

```json
{
  "success": true,
  "message": "Zu Branch gewechselt: feature/new-schema",
  "active_branch": "feature/new-schema"
}
```

**Fehler-Antworten:**
- `400 Bad Request`: Branch existiert nicht

---

#### POST /api/v1/branches/merge

Mergen Sie einen Branch in einen anderen.

**Request Body:**

```json
{
  "source_branch": "feature/new-schema",
  "target_branch": "main",
  "fast_forward": true,
  "abort_on_conflict": true
}
```

**Parameter:**
- `source_branch` (erforderlich): Branch, aus dem gemergt wird
- `target_branch` (erforderlich): Branch, in den gemergt wird
- `fast_forward` (optional): Fast-Forward-Merges erlauben (Standard: true)
- `abort_on_conflict` (optional): Bei erstem Konflikt stoppen (Standard: true)
- `merge_strategy` (optional): Name der Merge-Strategie (Standard: "default")

**Response (200 OK - Fast-Forward):**

```json
{
  "success": true,
  "message": "Fast-Forward-Merge abgeschlossen",
  "conflicts": [],
  "merged_sequence": 12345
}
```

**Response (409 Conflict - Non-Fast-Forward):**

```json
{
  "success": false,
  "message": "Non-Fast-Forward-Merge noch nicht implementiert. Verwenden Sie Force-Merge oder rebasen Sie den Source-Branch.",
  "conflicts": [],
  "merged_sequence": 0
}
```

---

#### DELETE /api/v1/branches/:name

Löschen Sie einen Branch.

**Query-Parameter:**
- `force` (optional): Löschen erzwingen, auch wenn nicht gemergt (Standard: false)

**Beispiel-Request:**

```bash
DELETE /api/v1/branches/feature/old-branch?force=true
```

**Response (200 OK):**

```json
{
  "success": true,
  "message": "Branch gelöscht: feature/old-branch"
}
```

**Fehler-Antworten:**
- `400 Bad Request`: Branch ist aktiv, ist Standard-Branch oder nicht vollständig gemergt (ohne force)

---

#### GET /api/v1/branches/stats

Branch-Statistiken abrufen.

**Response (200 OK):**

```json
{
  "total_branches": 5,
  "active_branches": 1,
  "oldest_creation_timestamp_ms": 1736657200000,
  "newest_creation_timestamp_ms": 1736657500000,
  "default_branch": "main"
}
```

---

## Verwendungsbeispiele

### Beispiel 1: Basis-Branch-Workflow

```bash
# Feature-Branch erstellen
curl -X POST http://localhost:8080/api/v1/branches \
  -H "Content-Type: application/json" \
  -d '{
    "branch_name": "feature/user-profiles",
    "parent_branch": "main",
    "description": "User-Profil-Funktionalität hinzufügen",
    "created_by": "alice"
  }'

# Zum Feature-Branch wechseln
curl -X POST http://localhost:8080/api/v1/branches/feature/user-profiles/switch

# ... Änderungen in diesem Branch vornehmen ...

# Zurück zu main wechseln
curl -X POST http://localhost:8080/api/v1/branches/main/switch

# Feature-Branch in main mergen
curl -X POST http://localhost:8080/api/v1/branches/merge \
  -H "Content-Type: application/json" \
  -d '{
    "source_branch": "feature/user-profiles",
    "target_branch": "main"
  }'

# Feature-Branch löschen
curl -X DELETE "http://localhost:8080/api/v1/branches/feature/user-profiles?force=true"
```

### Beispiel 2: Branch aus Tag erstellen

```bash
# Zuerst einen Snapshot-Tag erstellen
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "v1.0.0",
    "description": "Release 1.0"
  }'

# Branch aus diesem Tag erstellen
curl -X POST http://localhost:8080/api/v1/branches \
  -H "Content-Type: application/json" \
  -d '{
    "branch_name": "hotfix/security-patch",
    "parent_branch": "main",
    "description": "Sicherheits-Patch für v1.0",
    "from_tag": "v1.0.0"
  }'
```

### Beispiel 3: Branches auflisten und filtern

```bash
# Alle Branches nach Erstellungszeit sortiert auflisten
curl "http://localhost:8080/api/v1/branches?sort_by=timestamp&ascending=false"

# Nur die 5 neuesten Branches auflisten
curl "http://localhost:8080/api/v1/branches?limit=5&sort_by=timestamp&ascending=false"

# Aktuellen aktiven Branch abrufen
curl http://localhost:8080/api/v1/branches/active
```

### Beispiel 4: Branch-Statistiken

```bash
# Branch-Statistiken abrufen
curl http://localhost:8080/api/v1/branches/stats
```

---

## Integration

### Integration mit Snapshots

Branches können aus Snapshot-Tags erstellt werden:

```cpp
// C++ API Beispiel
BranchManager::CreateBranchOptions options;
options.from_tag = "v1.0.0";

auto branch = branch_manager.createBranch(
    "release/v1.0",
    "main",
    "Release-Branch für v1.0",
    "system",
    options
);
```

### Integration mit Diff API

Vergleichen Sie Änderungen zwischen Branches:

```bash
# Erstellungssequenzen zweier Branches abrufen
curl http://localhost:8080/api/v1/branches/feature/new-schema
# Gibt zurück: {"creation_sequence": 1000, ...}

curl http://localhost:8080/api/v1/branches/main
# Gibt zurück: {"creation_sequence": 500, ...}

# Diff API zum Vergleich verwenden
curl "http://localhost:8080/api/v1/diff?from_sequence=500&to_sequence=1000"
```

### Integration mit PITR

Wiederherstellen zu einem Branch-Erstellungspunkt:

```bash
# Branch-Erstellungssequenz abrufen
curl http://localhost:8080/api/v1/branches/release/v1.0
# Gibt zurück: {"creation_sequence": 12345, ...}

# PITR verwenden, um zu diesem Punkt wiederherzustellen
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "target_sequence": 12345,
    "dry_run": true
  }'
```

---

## Performance

### Benchmarks

Performance-Ziele für Branch-Operationen:

| Operation | Ziel | Typisch |
|-----------|------|---------|
| Branch erstellen | < 5ms | ~2ms |
| Branch abrufen | < 1ms | ~0.5ms |
| Branches auflisten (100) | < 20ms | ~10ms |
| Branch wechseln | < 2ms | ~1ms |
| Branch löschen | < 3ms | ~1.5ms |
| Branch existiert | < 0.5ms | ~0.3ms |

### Performance-Tipps

1. **Branch-Anzahl minimieren**: Halten Sie die Anzahl aktiver Branches angemessen (< 1000)
2. **Paginierung verwenden**: Verwenden Sie beim Auflisten vieler Branches den `limit`-Parameter
3. **Aktiven Branch cachen**: Cachen Sie den aktiven Branch-Namen in Ihrer Anwendung
4. **Fast-Forward-Merges**: Fast-Forward-Merges sind viel schneller als Three-Way-Merges

---

## Best Practices

### Namenskonventionen

Folgen Sie Git-Style-Namenskonventionen:

- **Feature-Branches**: `feature/beschreibung` (z.B. `feature/user-auth`)
- **Bugfix-Branches**: `bugfix/issue-nummer` (z.B. `bugfix/issue-123`)
- **Release-Branches**: `release/version` (z.B. `release/v2.0`)
- **Hotfix-Branches**: `hotfix/beschreibung` (z.B. `hotfix/security-patch`)

### Branch-Lebenszyklus

1. **Erstellen**: Branch aus main oder einem spezifischen Tag erstellen
2. **Entwickeln**: Zum Branch wechseln und Änderungen vornehmen
3. **Testen**: Änderungen im Branch validieren
4. **Mergen**: Zurück zu main mergen, wenn bereit
5. **Löschen**: Branch nach erfolgreichem Merge löschen

### Reservierte Namen

Vermeiden Sie diese reservierten Namen:
- `HEAD`
- `FETCH_HEAD`
- `ORIG_HEAD`

### Branch-Löschung

- **Kann nicht gelöscht werden**: Standard-Branch (`main`)
- **Kann nicht gelöscht werden**: Aktuell aktiver Branch
- **Sicherheit**: Branches müssen vollständig gemergt sein, außer bei Verwendung von `force=true`

### Branch-Merging

Derzeit werden nur Fast-Forward-Merges unterstützt:
- Source-Branch-Erstellungssequenz muss >= Target-Branch-Erstellungssequenz sein
- Voller Three-Way-Merge mit Konfliktauflösung ist für zukünftige Releases geplant

### Integration mit CI/CD

Branches integrieren sich gut mit CI/CD-Workflows:

```yaml
# Beispiel GitHub Actions Workflow
on:
  push:
    branches: [ feature/* ]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - name: Test-Branch erstellen
        run: |
          curl -X POST http://db:8080/api/v1/branches \
            -d '{"branch_name": "test/${{ github.sha }}", "parent_branch": "main"}'
      
      - name: Tests auf Branch ausführen
        run: |
          curl -X POST http://db:8080/api/v1/branches/test/${{ github.sha }}/switch
          # Ihre Tests hier ausführen
```

---

## Fehlerbehebung

### Branch existiert bereits

**Fehler**: `Branch konnte nicht erstellt werden. Branch existiert möglicherweise bereits.`

**Lösung**: Prüfen Sie zuerst, ob der Branch existiert:

```bash
curl http://localhost:8080/api/v1/branches/feature/my-branch
```

### Branch kann nicht gelöscht werden

**Fehler**: `Branch konnte nicht gelöscht werden. Branch ist möglicherweise aktiv oder nicht vollständig gemergt.`

**Lösungen**:
1. Wechseln Sie zuerst zu einem anderen Branch
2. Verwenden Sie `force=true`, um das Löschen zu erzwingen
3. Stellen Sie sicher, dass der Branch nicht der Standard-Branch ist

### Parent-Branch nicht gefunden

**Fehler**: `Branch konnte nicht erstellt werden. Parent-Branch nicht gefunden.`

**Lösung**: Überprüfen Sie, ob der Parent-Branch existiert:

```bash
curl http://localhost:8080/api/v1/branches
```

---

## Siehe auch

- [Named Snapshots](features_snapshots.md) - Semantische Tags für Datenbankzustände erstellen
- [Diff API](features_diff.md) - Änderungen zwischen Branches vergleichen
- [Point-in-Time Recovery](features_pitr.md) - Zu spezifischen Zeitpunkten wiederherstellen
- [Transaction Best Practices](TRANSACTION_BEST_PRACTICES.md) - MVCC-Transaktionsmuster

---

**Letzte Aktualisierung:** 6. Februar 2026  
**Dokumentationsversion:** 1.0
