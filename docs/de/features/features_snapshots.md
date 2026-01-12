# Named Snapshots (Semantische Tagging)

**Version:** 1.4.0+  
**Status:** ✅ Produktionsreif  
**Kategorie:** MVCC, Disaster Recovery, Compliance

---

## Überblick

Named Snapshots ist eine Git-ähnliche Funktion für das MVCC-System (Multi-Version Concurrency Control) von ThemisDB, die semantisches Tagging wichtiger Datenbankzustände ermöglicht. Dieses Feature bietet benannte, persistente Markierungen für kritische Datenbankzustände und macht Disaster Recovery, Compliance-Audits und Schema-Migrationen sicherer und besser verwaltbar.

### Hauptvorteile

- 🔄 **Disaster Recovery**: Benannte Wiederherstellungspunkte vor kritischen Operationen
- 📋 **Compliance**: Audit-bereite Snapshots für regulatorische Anforderungen
- 🚀 **DevOps**: Sichere Deployment-Checkpoints mit einfachem Rollback
- 🧪 **Testing**: Wiederholbare Testzustände mit semantischen Namen
- 🔧 **Schema-Migrationen**: Benannte Rollback-Punkte für Datenbankschema-Änderungen

---

## Kernkonzepte

### Was ist ein Named Snapshot?

Ein Named Snapshot ist ein semantischer Tag, der einen bestimmten Punkt in der Transaktionshistorie der Datenbank markiert. Jeder Snapshot erfasst:

- **Tag-Name**: Ein eindeutiger, menschenlesbarer Bezeichner (z.B. `vor_q1_2026_migration`)
- **Sequenznummer**: Die MVCC-Sequenznummer zum Snapshot-Zeitpunkt
- **Zeitstempel**: Präziser Moment der Snapshot-Erstellung
- **Beschreibung**: Optionaler menschenlesbarer Kontext
- **Ersteller**: Benutzer oder System, das den Snapshot erstellt hat

### Speichermodell

Snapshots werden persistent in RocksDB mit folgendem Format gespeichert:

```
Schlüssel: tags:{tag_name}
Wert: {
  "tag_name": "vor_migration_2026_q1",
  "sequence_number": 12345,
  "timestamp_ms": 1736629200000,
  "description": "Snapshot vor Q1 2026 Schema-Migration",
  "created_by": "admin"
}
```

---

## REST API Referenz

Alle Snapshot-Endpunkte befinden sich unter dem `/api/v1/snapshots/` Namespace.

### Snapshot Erstellen

**Endpunkt:** `POST /api/v1/snapshots/tags`

Erstellt einen neuen benannten Snapshot im aktuellen Datenbankzustand.

**Request Body:**
```json
{
  "tag_name": "vor_migration_2026_q1",
  "description": "Snapshot vor Q1 2026 Schema-Migration",
  "created_by": "admin"
}
```

**Response (201 Created):**
```json
{
  "tag_name": "vor_migration_2026_q1",
  "sequence_number": 12345,
  "timestamp_ms": 1736629200000,
  "description": "Snapshot vor Q1 2026 Schema-Migration",
  "created_by": "admin"
}
```

**Beispiel:**
```bash
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "vor_migration_2026_q1",
    "description": "Snapshot vor Q1 2026 Schema-Migration",
    "created_by": "admin"
  }'
```

### Snapshots Auflisten

**Endpunkt:** `GET /api/v1/snapshots/tags`

Gibt alle Snapshots zurück, sortiert nach Zeitstempel (neueste zuerst).

**Response (200 OK):**
```json
{
  "tags": [
    {
      "tag_name": "vor_migration_2026_q1",
      "sequence_number": 12345,
      "timestamp_ms": 1736629200000,
      "description": "Snapshot vor Q1 2026 Schema-Migration",
      "created_by": "admin"
    },
    {
      "tag_name": "quartalsbackup_2025_q4",
      "sequence_number": 10000,
      "timestamp_ms": 1704067200000,
      "description": "Q4 2025 Quartalsbackup",
      "created_by": "system"
    }
  ],
  "total": 2
}
```

**Beispiel:**
```bash
curl http://localhost:8765/api/v1/snapshots/tags
```

### Spezifischen Snapshot Abrufen

**Endpunkt:** `GET /api/v1/snapshots/tags/:name`

Ruft einen bestimmten Snapshot anhand seines Tag-Namens ab.

**Response (200 OK):**
```json
{
  "tag_name": "vor_migration_2026_q1",
  "sequence_number": 12345,
  "timestamp_ms": 1736629200000,
  "description": "Snapshot vor Q1 2026 Schema-Migration",
  "created_by": "admin"
}
```

**Beispiel:**
```bash
curl http://localhost:8765/api/v1/snapshots/tags/vor_migration_2026_q1
```

### Snapshot Löschen

**Endpunkt:** `DELETE /api/v1/snapshots/tags/:name`

Löscht einen Snapshot-Tag.

**Response (200 OK):**
```json
{
  "message": "Tag 'vor_migration_2026_q1' erfolgreich gelöscht"
}
```

**Beispiel:**
```bash
curl -X DELETE http://localhost:8765/api/v1/snapshots/tags/vor_migration_2026_q1
```

### Statistiken Abrufen

**Endpunkt:** `GET /api/v1/snapshots/stats`

Gibt Statistiken über alle Snapshots zurück.

**Response (200 OK):**
```json
{
  "total_tags": 5,
  "oldest_sequence": 100,
  "newest_sequence": 12345,
  "oldest_timestamp_ms": 1704067200000,
  "newest_timestamp_ms": 1736629200000
}
```

**Beispiel:**
```bash
curl http://localhost:8765/api/v1/snapshots/stats
```

---

## Anwendungsbeispiele

### Disaster Recovery Szenario

```bash
# 1. Snapshot vor kritischer Operation erstellen
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "vor_datenimport_2026_01_12",
    "description": "Vor Import von 1M Kundendatensätzen",
    "created_by": "daten_team"
  }'

# 2. Kritische Operation durchführen
# ... Daten importieren ...

# 3. Bei Problemen kann dieser Snapshot für
#    Wiederherstellungsoperationen referenziert werden
#    (zukünftige PITR-Funktion wird dies nutzen)
```

### Compliance-Auditing

```bash
# Quartalsweise Compliance-Snapshots erstellen
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "compliance_q1_2026",
    "description": "Q1 2026 Compliance-Snapshot für Audit",
    "created_by": "compliance_system"
  }'

# Alle Compliance-Snapshots auflisten
curl http://localhost:8765/api/v1/snapshots/tags | jq '.tags[] | select(.tag_name | startswith("compliance_"))'
```

### Schema-Migrations-Workflow

```bash
# 1. Pre-Migrations-Snapshot erstellen
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "vor_schema_v2_0",
    "description": "Vor Upgrade auf Schema v2.0",
    "created_by": "migrations_script"
  }'

# 2. Migration durchführen
# ... Schema-Änderungen anwenden ...

# 3. Post-Migrations-Snapshot erstellen
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "nach_schema_v2_0",
    "description": "Nach Upgrade auf Schema v2.0",
    "created_by": "migrations_script"
  }'

# 4. Beide Snapshots verifizieren
curl http://localhost:8765/api/v1/snapshots/tags | jq '.tags[] | select(.tag_name | contains("schema_v2_0"))'
```

---

## Konfiguration

### Named Snapshots Aktivieren

Named Snapshots erfordert die Aktivierung der CDC-Funktion (Change Data Capture):

```cpp
// In der HttpServer-Konfiguration
HttpServer::Config config;
config.feature_cdc = true;  // Erforderlich für Snapshots
```

Oder über Umgebungsvariable:
```bash
THEMIS_ENABLE_CDC=true
```

### Tag-Namen-Validierung

Tag-Namen müssen folgende Regeln einhalten:

- **Erlaubte Zeichen**: Alphanumerisch (a-z, A-Z, 0-9), Bindestriche (-), Unterstriche (_)
- **Maximale Länge**: 128 Zeichen
- **Minimale Länge**: 1 Zeichen
- **Eindeutigkeit**: Tag-Namen müssen eindeutig sein

**Gültige Beispiele:**
- `vor_migration_2026_q1`
- `quartalsbackup-2025-Q4`
- `test_zustand_001`

**Ungültige Beispiele:**
- `vor migration` (Leerzeichen nicht erlaubt)
- `backup@2026` (Sonderzeichen nicht erlaubt)
- `` (leerer String)

---

## Performance-Charakteristiken

### Operations-Komplexität

| Operation | Zeitkomplexität | Hinweise |
|-----------|----------------|----------|
| Tag Erstellen | O(1) | Direkte RocksDB-Put-Operation |
| Tag Abrufen | O(1) | Direkte RocksDB-Get-Operation |
| Tags Auflisten | O(n) | Sequenzieller Scan, in-memory sortiert |
| Tag Löschen | O(1) | Direkte RocksDB-Delete-Operation |
| Statistiken Abrufen | O(n) | Sequenzieller Scan für Statistiken |

### Ressourcennutzung

- **Speicher**: ~200-500 Bytes pro Snapshot (abhängig von Beschreibungslänge)
- **RAM**: Minimal (kein Caching, direkter RocksDB-Zugriff)
- **CPU**: Vernachlässigbar für typische Workloads (<1000 Snapshots)

### Skalierbarkeit

- Mit 1000+ Snapshots ohne Performance-Degradierung getestet
- List-Operation bleibt schnell (<50ms) mit 1000 Snapshots
- Thread-sicher ohne Lock-Contention unter normaler Last

---

## Best Practices

### Namenskonventionen

Verwenden Sie konsistente, beschreibende Namens-Patterns:

```
# Datumsbasiert
JJJJ_MM_TT_beschreibung
vor_JJJJ_MM_TT_ereignis

# Ereignisbasiert
vor_operation_beschreibung
nach_operation_beschreibung

# Compliance
compliance_periode_kennung
audit_JJJJ_QX
```

### Snapshot-Lebenszyklus

1. **Erstellen**: Vor kritischen Operationen
2. **Dokumentieren**: Aussagekräftige Beschreibungen hinzufügen
3. **Verifizieren**: Snapshot-Existenz nach Erstellung prüfen
4. **Aufräumen**: Alte Snapshots löschen, wenn nicht mehr benötigt
5. **Audit**: Compliance-Snapshots gemäß Richtlinien pflegen

### Integration mit CI/CD

```bash
#!/bin/bash
# Beispiel Deployment-Script

# Pre-Deployment-Snapshot erstellen
SNAPSHOT_NAME="vor_deploy_$(date +%Y%m%d_%H%M%S)"
curl -X POST http://db.example.com/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d "{
    \"tag_name\": \"$SNAPSHOT_NAME\",
    \"description\": \"Pre-Deployment-Snapshot für Build $BUILD_ID\",
    \"created_by\": \"ci_cd_pipeline\"
  }"

# Anwendung deployen
./deploy.sh

# Deployment verifizieren
if ./verify.sh; then
  echo "Deployment erfolgreich"
else
  echo "Deployment fehlgeschlagen - Rollback zu $SNAPSHOT_NAME empfohlen"
  exit 1
fi
```

---

## Fehlerbehandlung

### Häufige Fehler

**400 Bad Request**
- Ungültiges Tag-Namensformat
- Leerer Tag-Name
- Beschreibung zu lang (>1024 Zeichen)
- Fehlende Pflichtfelder

**404 Not Found**
- Tag existiert nicht (GET/DELETE-Operationen)

**409 Conflict**
- Tag-Name existiert bereits (CREATE-Operation)

**503 Service Unavailable**
- CDC-Funktion nicht aktiviert
- Datenbank nicht bereit

### Fehler-Response-Format

```json
{
  "error": "Tag 'ungültig@name' enthält ungültige Zeichen (verwenden Sie nur alphanumerische Zeichen, Bindestriche, Unterstriche)"
}
```

---

## Sicherheitsüberlegungen

### Zugriffskontrolle

Snapshot-Operationen sollten auf autorisierte Benutzer beschränkt sein:

- Erstellen: Erfordert `snapshot:write` Berechtigung
- Auflisten/Abrufen: Erfordert `snapshot:read` Berechtigung
- Löschen: Erfordert `snapshot:delete` Berechtigung
- Statistiken: Erfordert `snapshot:read` Berechtigung

### Audit-Protokollierung

Alle Snapshot-Operationen werden im Audit-Trail protokolliert:
- Wer hat Snapshots erstellt/gelöscht
- Wann sind Operationen erfolgt
- Welche Änderungen wurden vorgenommen

### Datenschutz

- Snapshots enthalten nur Metadaten, keine tatsächlichen Daten
- Tag-Namen und Beschreibungen sollten keine sensiblen Informationen enthalten
- Befolgen Sie die Datenklassifizierungsrichtlinien Ihrer Organisation

---

## Einschränkungen

### Aktuelle Einschränkungen

1. **Keine automatische Retention-Policy**: Manuelle Bereinigung erforderlich
2. **Keine Restore-Funktionalität**: Point-in-Time Recovery (PITR) für Phase 3 geplant
3. **Keine Snapshot-Validierung**: Keine Verifizierung der Snapshot-Integrität
4. **Keine Kompression**: Metadaten als Plain-JSON gespeichert

### Zukünftige Erweiterungen (Geplant)

- **Phase 2**: Diff-API - Vergleich von Datenbankzuständen zwischen Snapshots
- **Phase 3**: Point-in-Time Recovery - Wiederherstellung zu jedem Snapshot
- **Zukunft**: Automatische Retention-Policies mit konfigurierbaren Regeln
- **Zukunft**: Snapshot-Export/Import für Backup-Portabilität

---

## Fehlerbehebung

### Snapshot Nicht Erstellt

**Symptom**: POST-Request gibt Fehler zurück

**Lösungen**:
1. CDC aktiviert prüfen: `config.feature_cdc = true`
2. Tag-Name folgt Validierungsregeln verifizieren
3. Beschreibungslänge prüfen (<1024 Zeichen)
4. Eindeutigen Tag-Namen sicherstellen

### Kann Snapshots Nicht Auflisten

**Symptom**: GET-Request gibt leer oder Fehler zurück

**Lösungen**:
1. CDC-Funktion aktiviert verifizieren
2. Datenbank läuft und ist erreichbar prüfen
3. Keine RocksDB-Korruption verifizieren

### Performance-Degradierung

**Symptom**: Langsame List-Operationen

**Lösungen**:
1. Snapshot-Anzahl prüfen (>10000 kann langsam sein)
2. Retention-Policy implementieren erwägen
3. Ungenutzte alte Snapshots löschen

---

## Verwandte Features

- **Change Data Capture (CDC)**: Grundlage für Snapshots
- **Changefeed**: Transaktionshistorien-Tracking
- **Temporal Graphs**: Zeitbasierte Graph-Abfragen
- **Audit Logging**: Compliance und Sicherheits-Tracking

---

## Referenzen

- [Implementierungsplan](../../research/IMPLEMENTATION_PLAN_GIT_FEATURES.md)
- [Git-ähnliche Features Research](../../research/GIT_LIKE_FEATURES_FOR_MVCC.md)
- [MVCC-Architektur](../../architecture/architecture_mvcc.md)
- [Changefeed-Dokumentation](features_cdc.md)

---

## Support

Für Probleme oder Fragen:
- GitHub Issues: [ThemisDB Issues](https://github.com/makr-code/ThemisDB/issues)
- Dokumentation: [ThemisDB Docs](https://github.com/makr-code/ThemisDB/tree/main/docs)

---

**Letzte Aktualisierung**: 2026-01-12  
**Version**: 1.4.0+
