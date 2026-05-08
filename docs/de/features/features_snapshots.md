# Named Snapshots - Semantisches Tagging für MVCC

**Version:** 1.4.0  
**Kategorie:** 🏷️ Transaktionsverwaltung  
**Status:** ✅ Implementiert (Phase 1)

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

Named Snapshots bieten Git-ähnliches semantisches Tagging für ThemisDB's MVCC-System. Erstellen Sie aussagekräftige Labels für Datenbankzustände, um Audit-Trails, Deployment-Checkpoints und Point-in-Time Recovery zu ermöglichen.

### Kernfunktionen

- **Semantische Tags**: Beschriften Sie Datenbankzustände mit bedeutungsvollen Namen (z.B. `v1.0.0`, `pre-migration`, `release-2024-01-12`)
- **Persistente Speicherung**: Tags in RocksDB gespeichert, überleben Datenbank-Neustarts
- **Tag-basierter Diff**: Berechnen Sie Unterschiede zwischen getaggten Zuständen (`GET /api/v1/diff?from_tag=v1.0&to_tag=v2.0`)
- **Audit-Trail**: Verfolgen Sie, wer Tags erstellt hat und wann
- **Flexible Sortierung**: Listen Sie Tags nach Zeitstempel, Sequenz oder Name

### Anwendungsfälle

- **Deployment-Checkpoints**: Taggen Sie den Datenbankzustand vor Deployments
- **Audit/Compliance**: Erstellen Sie benannte Snapshots für regulatorische Anforderungen
- **Testing**: Markieren Sie bekannt gute Zustände für Test-Rollback
- **Release-Management**: Taggen Sie jede Release für Versionsverfolgung
- **Debugging**: Erstellen Sie Snapshots vor/nach Vorfällen

---

## Features

### ✅ Implementiert

- Tag CRUD-Operationen (Erstellen, Lesen, Auflisten, Löschen)
- Persistente Speicherung in RocksDB
- Tag-Validierung (alphanumerisch, Bindestriche, Unterstriche, Punkte, 1-128 Zeichen)
- Sortierung nach Zeitstempel, Sequenz oder Name (aufsteigend/absteigend)
- Paginierung
- Statistik-API
- Integration mit Diff API für Tag-basierten Diff
- REST API Endpunkte

### 🔜 Zukünftige Erweiterungen

- Tag-Metadaten (benutzerdefinierte Schlüssel-Wert-Paare)
- Tag-Ablauf/TTL
- Tag-Kategorien/Namespaces
- Bulk-Tag-Operationen

---

## API-Referenz

### Endpunkte

#### POST /api/v1/snapshots/tags

Erstellen Sie einen neuen Snapshot-Tag.

**Request Body:**

```json
{
  "tag_name": "v1.0.0",
  "description": "Release 1.0 - Initiales Production-Release",
  "created_by": "admin"
}
```

**Parameter:**
- `tag_name` (erforderlich): Eindeutiger Tag-Identifier (1-128 Zeichen, alphanumerisch, `-`, `_`, `.`)
- `description` (erforderlich): Lesbare Beschreibung
- `created_by` (optional): Benutzer/Service, der den Tag erstellt hat (Standard: "system")

**Response (201 Created):**

```json
{
  "tag_name": "v1.0.0",
  "sequence_number": 12345,
  "timestamp_ms": 1736657231000,
  "description": "Release 1.0 - Initiales Production-Release",
  "created_by": "admin"
}
```

**Fehler-Antworten:**
- `400 Bad Request`: Ungültiger Tag-Name oder fehlende Felder
- `409 Conflict`: Tag existiert bereits

---

#### GET /api/v1/snapshots/tags

Listen Sie alle Snapshot-Tags auf.

**Query-Parameter:**
- `limit` (optional): Maximale Anzahl zurückzugebender Tags (Standard: 0 = alle)
- `sort_by` (optional): Sortierfeld - `timestamp` (Standard), `sequence`, `name`
- `ascending` (optional): Sortierrichtung - `true` oder `false` (Standard: `false`)

**Response (200 OK):**

```json
[
  {
    "tag_name": "v2.0.0",
    "sequence_number": 50000,
    "timestamp_ms": 1736744231000,
    "description": "Release 2.0",
    "created_by": "admin"
  },
  {
    "tag_name": "v1.0.0",
    "sequence_number": 12345,
    "timestamp_ms": 1736657231000,
    "description": "Release 1.0",
    "created_by": "admin"
  }
]
```

---

#### GET /api/v1/snapshots/tags/:name

Rufen Sie einen spezifischen Snapshot-Tag ab.

**Response (200 OK):**

```json
{
  "tag_name": "v1.0.0",
  "sequence_number": 12345,
  "timestamp_ms": 1736657231000,
  "description": "Release 1.0 - Initiales Production-Release",
  "created_by": "admin"
}
```

**Fehler-Antwort:**
- `404 Not Found`: Tag existiert nicht

---

#### DELETE /api/v1/snapshots/tags/:name

Löschen Sie einen Snapshot-Tag.

**Response (200 OK):**

```json
{
  "status": "success",
  "message": "Tag 'v1.0.0' deleted successfully"
}
```

**Fehler-Antwort:**
- `404 Not Found`: Tag existiert nicht

---

#### GET /api/v1/snapshots/stats

Rufen Sie Snapshot-Statistiken ab.

**Response (200 OK):**

```json
{
  "total_snapshots": 5,
  "oldest_timestamp_ms": 1736657231000,
  "newest_timestamp_ms": 1736744231000,
  "oldest_sequence": 12345,
  "newest_sequence": 50000
}
```

---

## Verwendungsbeispiele

### Beispiel 1: Release-Snapshot erstellen

```bash
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "v1.0.0",
    "description": "Release 1.0 - Initiales Production-Release",
    "created_by": "admin"
  }'
```

### Beispiel 2: Pre-Deployment-Snapshot erstellen

```bash
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "pre-deploy-2024-01-12",
    "description": "Zustand vor Deployment am 2024-01-12",
    "created_by": "deploy-bot"
  }'
```

### Beispiel 3: Alle Snapshots auflisten (Neueste zuerst)

```bash
curl http://localhost:8765/api/v1/snapshots/tags?sort_by=timestamp&ascending=false
```

### Beispiel 4: Letzte 10 Snapshots auflisten

```bash
curl http://localhost:8765/api/v1/snapshots/tags?limit=10&sort_by=timestamp&ascending=false
```

### Beispiel 5: Spezifischen Snapshot abrufen

```bash
curl http://localhost:8765/api/v1/snapshots/tags/v1.0.0
```

### Beispiel 6: Alten Snapshot löschen

```bash
curl -X DELETE http://localhost:8765/api/v1/snapshots/tags/old-snapshot
```

### Beispiel 7: Statistiken abrufen

```bash
curl http://localhost:8765/api/v1/snapshots/stats
```

### Beispiel 8: Tag-basierter Diff (Integration mit Diff API)

```bash
# Änderungen zwischen zwei getaggten Versionen vergleichen
curl "http://localhost:8765/api/v1/diff?from_tag=v1.0&to_tag=v2.0"
```

### Beispiel 9: Verwendung mit Python

```python
import requests
import json

# Snapshot erstellen
def create_snapshot(tag_name, description, created_by="system"):
    url = "http://localhost:8765/api/v1/snapshots/tags"
    data = {
        "tag_name": tag_name,
        "description": description,
        "created_by": created_by
    }
    response = requests.post(url, json=data)
    return response.json()

# Release-Snapshot erstellen
snapshot = create_snapshot(
    tag_name="v1.0.0",
    description="Release 1.0",
    created_by="admin"
)
print(f"Snapshot erstellt: {snapshot['tag_name']} bei Sequenz {snapshot['sequence_number']}")

# Alle Snapshots auflisten
def list_snapshots(limit=0, sort_by="timestamp", ascending=False):
    url = "http://localhost:8765/api/v1/snapshots/tags"
    params = {
        "limit": limit,
        "sort_by": sort_by,
        "ascending": str(ascending).lower()
    }
    response = requests.get(url, params=params)
    return response.json()

# Letzte 5 Snapshots abrufen
snapshots = list_snapshots(limit=5)
for snap in snapshots:
    print(f"- {snap['tag_name']}: {snap['description']}")

# Versionen vergleichen
def diff_between_tags(from_tag, to_tag):
    url = "http://localhost:8765/api/v1/diff"
    params = {
        "from_tag": from_tag,
        "to_tag": to_tag
    }
    response = requests.get(url, params=params)
    return response.json()

# Unterschiede zwischen Releases abrufen
diff = diff_between_tags("v1.0.0", "v2.0.0")
print(f"Änderungen: {diff['stats']['total_changes']}")
print(f"  Hinzugefügt: {diff['stats']['added_count']}")
print(f"  Geändert: {diff['stats']['modified_count']}")
print(f"  Gelöscht: {diff['stats']['deleted_count']}")
```

---

## Integration

### Mit Diff API

Snapshots integrieren sich nahtlos mit der Diff API, um Tag-basierte Diffs zu ermöglichen:

```bash
# Datenbankzustände zwischen zwei Tags vergleichen
GET /api/v1/diff?from_tag=v1.0&to_tag=v2.0

# Mit Filtern
GET /api/v1/diff?from_tag=v1.0&to_tag=v2.0&table=users&limit=100
```

### Deployment-Workflow-Beispiel

```bash
# 1. Snapshot vor Deployment erstellen
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "pre-deploy-$(date +%Y%m%d-%H%M%S)",
    "description": "Pre-Deployment-Snapshot",
    "created_by": "ci-cd"
  }'

# 2. Deployment durchführen
./deploy.sh

# 3. Snapshot nach Deployment erstellen
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "post-deploy-$(date +%Y%m%d-%H%M%S)",
    "description": "Post-Deployment-Snapshot",
    "created_by": "ci-cd"
  }'

# 4. Änderungen überprüfen
curl "http://localhost:8765/api/v1/diff?from_tag=pre-deploy-*&to_tag=post-deploy-*"
```

---

## Performance

### Benchmarks

| Operation | Ziel | Tatsächlich | Datensatz | Hinweise |
|-----------|------|-------------|-----------|----------|
| Tag erstellen | <1ms | ~0.5ms | N/A | Einzelne Tag-Erstellung |
| Tag abrufen | <0.5ms | ~0.2ms | N/A | Einzelner Tag-Abruf |
| Tags auflisten (10) | <5ms | ~2ms | 10 Tags | Alle Tags auflisten |
| Tags auflisten (100) | <10ms | ~8ms | 100 Tags | Alle Tags auflisten |
| Tags auflisten (1000) | <50ms | ~40ms | 1000 Tags | Alle Tags auflisten |
| Tag löschen | <1ms | ~0.5ms | N/A | Einzelne Tag-Löschung |
| Tag existiert | <0.1ms | ~0.05ms | N/A | Existenzprüfung |
| Statistiken abrufen | <5ms | ~3ms | 100 Tags | Statistiken berechnen |

---

## Best Practices

### Tag-Namenskonventionen

```bash
# ✅ Gut: Klare, semantische Namen
v1.0.0
v2.1.3-beta
release-2024-01-12
pre-migration-users
post-rollback-incident-123

# ❌ Schlecht: Unklare, generische Namen
tag1
snapshot
backup
test
```

---

## Siehe auch

- [Diff API Dokumentation](./features_diff.md)
- [MVCC Architektur](../architecture/architecture_mvcc.md)
- [Changefeed Dokumentation](../cdc/changefeed.md)
- [Point-in-Time Recovery](./features_pitr.md)
- [Git-ähnliche Features Forschung](../../research/GIT_LIKE_FEATURES_FOR_MVCC.md)

---

**Erstellt:** 2026-01-12  
**Zuletzt aktualisiert:** 2026-01-12  
**Version:** 1.0  
**Status:** Produktionsreif ✅
