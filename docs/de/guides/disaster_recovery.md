# Disaster Recovery Leitfaden für ThemisDB

**Version:** 1.0  
**Datum:** 6. Februar 2026  
**Kategorie:** Operations Guide  
**Status:** ✅ Produktionsbereit

---

## Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Schnellstart](#schnellstart)
- [Wiederherstellungsszenarien](#wiederherstellungsszenarien)
- [Best Practices](#best-practices)
- [Fehlerbehebung](#fehlerbehebung)
- [Anhang](#anhang)

---

## Übersicht

Dieser Leitfaden bietet umfassende Verfahren zur Wiederherstellung von ThemisDB aus verschiedenen Katastrophenszenarien unter Verwendung des Point-in-Time Recovery (PITR) Systems.

### Wann dieser Leitfaden zu verwenden ist

- **Datenkorruption**: Datenbankkorruption nach fehlgeschlagenen Operationen
- **Versehentliche Löschungen**: Kritische Daten wurden versehentlich entfernt
- **Fehlgeschlagene Migrationen**: Schema- oder Datenmigrationen, die schief gelaufen sind
- **Sicherheitsvorfälle**: Wiederherstellung nach Sicherheitsverletzungen
- **Test/Entwicklung**: Wiederherstellung zu bekannten guten Zuständen

### Hauptfunktionen

Das PITR-System von ThemisDB bietet Git-ähnliche Wiederherstellungsfunktionen:

- ✅ **Benannte Snapshots**: Erstellen Sie semantische Tags an wichtigen Punkten
- ✅ **Zeitreise**: Wiederherstellung zu jeder Sequenz, jedem Tag oder Zeitstempel
- ✅ **Vorschaumodus**: Sehen Sie, was sich ändern wird, bevor Sie wiederherstellen
- ✅ **Selektive Wiederherstellung**: Nur bestimmte Tabellen wiederherstellen
- ✅ **Auto-Backup**: Automatisches Backup vor Wiederherstellungsoperationen

---

## Schnellstart

### 1. Snapshot erstellen (Sicherer Punkt)

Erstellen Sie vor jeder riskanten Operation einen benannten Snapshot:

```bash
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "before_migration",
    "description": "Safe point before schema migration",
    "created_by": "admin"
  }'
```

**Antwort:**
```json
{
  "tag_name": "before_migration",
  "sequence_number": 12345,
  "timestamp_ms": 1707217330417,
  "description": "Safe point before schema migration",
  "created_by": "admin"
}
```

### 2. Wiederherstellungsoperation in der Vorschau anzeigen

Zeigen Sie vor der Wiederherstellung eine Vorschau an, was passieren wird:

```bash
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "tag",
      "value": "before_migration"
    }
  }'
```

**Antwort:**
```json
{
  "target_sequence": 12345,
  "current_sequence": 12567,
  "events_to_replay": 222,
  "affected_tables": ["users", "orders", "products"],
  "affected_keys": ["users:1", "users:2", "orders:100", "..."],
  "estimated_duration_sec": 5,
  "estimated_size_bytes": 524288
}
```

### 3. Wiederherstellung ausführen

Wiederherstellung zum Snapshot:

```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "tag",
      "value": "before_migration"
    },
    "options": {
      "create_backup": true,
      "dry_run": false,
      "backup_tag": "auto_backup_20260206"
    }
  }'
```

**Antwort:**
```json
{
  "ok": true,
  "message": "Restore completed successfully",
  "progress": {
    "phase": 6,
    "events_processed": 222,
    "total_events": 222,
    "progress_percent": 100.0,
    "elapsed_ms": 4832
  }
}
```

---

## Wiederherstellungsszenarien

### Szenario 1: Datenkorruption nach fehlgeschlagenem Deployment

**Problem**: Datenbank korrupt nach Deployment einer neuen Anwendungsversion.

**Lösung**: Wiederherstellung zu Pre-Deployment-Snapshot.

#### Schritt-für-Schritt

1. **Sicheren Snapshot identifizieren**:
```bash
curl http://localhost:8080/api/v1/snapshots/tags
```

2. **Anwendung stoppen** (optional, aber empfohlen):
```bash
systemctl stop myapp
```

3. **Datenbank wiederherstellen**:
```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "tag",
      "value": "pre_deployment_v2.0"
    },
    "options": {
      "create_backup": true,
      "abort_on_first_error": true
    }
  }'
```

4. **Wiederherstellung verifizieren**:
```bash
# Datenbankstatus prüfen
curl http://localhost:8080/api/v1/health

# Spezifische Daten prüfen
curl http://localhost:8080/api/v1/entities/users
```

5. **Anwendung neu starten**:
```bash
systemctl start myapp
```

---

### Szenario 2: Versehentliche Massenlöschung

**Problem**: Kritische Daten versehentlich von Benutzer oder Skript gelöscht.

**Lösung**: Wiederherstellung zu einem Zeitpunkt vor der Löschung.

#### Schritt-für-Schritt

1. **Löschzeitpunkt ermitteln**:
```bash
# Changefeed nach Löschereignissen prüfen
curl "http://localhost:8080/api/v1/changefeed?event_type=DELETE&limit=10"
```

2. **Sicheren Wiederherstellungspunkt finden**:
```bash
# Letzte Snapshots auflisten
curl "http://localhost:8080/api/v1/snapshots/tags?limit=10"
```

3. **Wiederherstellung in Vorschau anzeigen**:
```bash
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "timestamp",
      "value": 1707216000000
    }
  }'
```

4. **Wiederherstellung ausführen**:
```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "timestamp",
      "value": 1707216000000
    },
    "options": {
      "create_backup": true,
      "backup_tag": "before_deletion_recovery"
    }
  }'
```

---

### Szenario 3: Fehlgeschlagene Schemamigration

**Problem**: Schemamigrationsskript fehlgeschlagen, Datenbank im inkonsistenten Zustand.

**Lösung**: Rollback zum Pre-Migration-Snapshot.

#### Schritt-für-Schritt

1. **Migrationsprozess stoppen**:
```bash
# Migrationsskript beenden, falls noch aktiv
pkill -f migration_script.py
```

2. **Zu Pre-Migration-Zustand wiederherstellen**:
```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "tag",
      "value": "pre_schema_migration_v5"
    },
    "options": {
      "create_backup": true,
      "abort_on_first_error": true
    }
  }'
```

3. **Datenbankschema verifizieren**:
```bash
curl http://localhost:8080/api/v1/schema
```

4. **Migrationsskript korrigieren**:
```bash
# Migrationsfehler überprüfen und beheben
vim migration_script.py
```

5. **Migration erneut versuchen** (nach Erstellung eines neuen Snapshots):
```bash
# Neuen sicheren Punkt erstellen
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d '{"tag_name":"pre_migration_retry","description":"Before retry"}'

# Migration ausführen
python migration_script.py
```

---

### Szenario 4: Selektive Tabellenwiederherstellung

**Problem**: Nur eine Tabelle benötigt Wiederherstellung, andere sollen unverändert bleiben.

**Lösung**: Verwenden Sie selektive Wiederherstellung mit Tabellenfilter.

#### Schritt-für-Schritt

1. **Betroffene Tabelle identifizieren**:
```bash
# Angenommen "orders" Tabelle benötigt Wiederherstellung
TABLE="orders"
```

2. **Selektive Wiederherstellung in Vorschau anzeigen**:
```bash
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "tag",
      "value": "hourly_backup_14h00"
    },
    "options": {
      "tables": ["orders"]
    }
  }'
```

3. **Selektive Wiederherstellung ausführen**:
```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "tag",
      "value": "hourly_backup_14h00"
    },
    "options": {
      "create_backup": true,
      "tables": ["orders"],
      "backup_tag": "before_orders_restore"
    }
  }'
```

4. **Verifizieren, dass nur betroffene Tabelle geändert wurde**:
```bash
# Orders Tabelle prüfen
curl http://localhost:8080/api/v1/entities/orders | jq '.count'

# Verifizieren, dass andere Tabellen unverändert sind
curl http://localhost:8080/api/v1/entities/users | jq '.count'
curl http://localhost:8080/api/v1/entities/products | jq '.count'
```

---

### Szenario 5: Vollständige Datenbankwiederherstellung

**Problem**: Vollständiger Datenbankverlust oder katastrophale Korruption.

**Lösung**: Wiederherstellung vom aktuellsten Backup.

#### Schritt-für-Schritt

1. **ThemisDB stoppen**:
```bash
systemctl stop themisdb
```

2. **Datenbankdateien aus Backup wiederherstellen**:
```bash
# RocksDB-Dateien wiederherstellen
rsync -av /backup/themisdb/data/ /var/lib/themisdb/data/
```

3. **ThemisDB starten**:
```bash
systemctl start themisdb
```

4. **Datenbank verifizieren**:
```bash
curl http://localhost:8080/api/v1/health
curl http://localhost:8080/api/v1/snapshots/tags
```

5. **Letzten Snapshot finden und bei Bedarf wiederherstellen**:
```bash
# Alle Tags auflisten
curl http://localhost:8080/api/v1/snapshots/tags

# Bei Bedarf zum letzten bekannten guten Zustand wiederherstellen
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -d '{"target":{"type":"tag","value":"latest_good_state"}}'
```

---

## Best Practices

### 1. Regelmäßige Snapshots

Erstellen Sie Snapshots in regelmäßigen Abständen und vor größeren Operationen:

```bash
# Stündliche Snapshots (via cron)
0 * * * * curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d "{\"tag_name\":\"hourly_$(date +\%Y\%m\%d_\%H00)\",\"description\":\"Hourly backup\"}"

# Vor Deployments
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d '{"tag_name":"pre_deploy_v1.2.3","description":"Before v1.2.3 deployment"}'

# Vor Schemaänderungen
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d '{"tag_name":"pre_schema_change","description":"Before adding users.email column"}'
```

### 2. Immer zuerst Vorschau anzeigen

**Niemals ohne Vorschau wiederherstellen**:

```bash
# Vorschau zeigt, was sich ändern wird
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -d '{"target":{"type":"tag","value":"my_snapshot"}}'

# Ausgabe sorgfältig überprüfen:
# - events_to_replay: Wie viele Änderungen werden rückgängig gemacht?
# - affected_tables: Welche Tabellen ändern sich?
# - estimated_duration_sec: Wie lange wird es dauern?

# Nur fortfahren, wenn die Vorschau korrekt aussieht
```

### 3. Auto-Backup verwenden

Aktivieren Sie immer Auto-Backup für Produktionswiederherstellungen:

```json
{
  "options": {
    "create_backup": true,
    "backup_tag": "before_restore_20260206",
    "abort_on_first_error": true
  }
}
```

### 4. Wiederherstellungsfortschritt überwachen

Überwachen Sie den Fortschritt bei langwierigen Wiederherstellungen:

```bash
# Fortschritt alle paar Sekunden überprüfen
watch -n 2 'curl -s http://localhost:8080/api/v1/restore/progress | jq'

# Oder in einem Skript abfragen
while true; do
  curl -s http://localhost:8080/api/v1/restore/progress | jq '.progress_percent'
  sleep 5
done
```

### 5. Aufbewahrungsrichtlinie

Implementieren Sie eine Snapshot-Aufbewahrungsrichtlinie:

```bash
# Behalten Sie:
# - Letzte 24 stündliche Snapshots
# - Letzte 7 tägliche Snapshots
# - Letzte 4 wöchentliche Snapshots
# - Alle Deployment-Snapshots (unbegrenzt)

# Alte Snapshots löschen (Beispiel)
curl -X DELETE http://localhost:8080/api/v1/snapshots/tags/hourly_20260130_0100
```

### 6. Wiederherstellungsverfahren testen

**Testen Sie regelmäßig die Disaster Recovery**:

```bash
# Wiederherstellung in Nicht-Produktionsumgebung testen
# 1. Test-Snapshot erstellen
# 2. Änderungen vornehmen
# 3. Zu Snapshot wiederherstellen
# 4. Datenintegrität verifizieren

# Vierteljährliche DR-Übungen planen
```

---

## Fehlerbehebung

### Wiederherstellung mit "Invalid Sequence" fehlgeschlagen

**Problem**: Zielsequenz existiert nicht oder liegt in der Zukunft.

**Lösung**:
```bash
# Aktuelle Sequenz prüfen
curl http://localhost:8080/api/v1/changefeed?limit=1 | jq '.sequence'

# Verfügbare Snapshots auflisten
curl http://localhost:8080/api/v1/snapshots/tags

# Gültige Sequenz/Tag verwenden
```

### Wiederherstellung hängt bei hoher Fortschrittsanzeige

**Problem**: Wiederherstellung scheint kurz vor Abschluss eingefroren.

**Lösung**:
```bash
# ThemisDB-Logs prüfen
tail -f /var/log/themisdb/themisdb.log

# Datenbanksperren prüfen
curl http://localhost:8080/api/v1/admin/locks

# Falls wirklich hängen geblieben, möglicherweise ThemisDB neu starten
systemctl restart themisdb
```

### Nicht genügend Speicherplatz

**Problem**: Wiederherstellung schlägt mit Speicherplatzfehler fehl.

**Lösung**:
```bash
# Speichernutzung prüfen
df -h /var/lib/themisdb

# Speicherplatz freigeben
# - Alte Snapshots löschen
# - Temporäre Dateien bereinigen
# - Festplatte bei Bedarf erweitern

# Wiederherstellung erneut versuchen
```

### Tag nicht gefunden

**Problem**: Snapshot-Tag existiert nicht.

**Lösung**:
```bash
# Alle Tags auflisten
curl http://localhost:8080/api/v1/snapshots/tags | jq '.[] | .tag_name'

# Korrekten Tag-Namen verwenden (Groß-/Kleinschreibung beachten!)
```

---

## Anhang

### A. Benennungskonventionen für Snapshots

Empfohlene Benennungsmuster:

- **Stündlich**: `hourly_YYYYMMDD_HH00` (z.B. `hourly_20260206_1400`)
- **Täglich**: `daily_YYYYMMDD` (z.B. `daily_20260206`)
- **Wöchentlich**: `weekly_YYYYWW` (z.B. `weekly_202606`)
- **Deployment**: `pre_deploy_vX.Y.Z` (z.B. `pre_deploy_v1.2.3`)
- **Schema**: `pre_schema_DESCRIPTION` (z.B. `pre_schema_add_email_column`)
- **Vorfall**: `pre_incident_YYYYMMDD_HHMM` (z.B. `pre_incident_20260206_1430`)

### B. API-Kurzreferenz

| Endpoint | Methode | Zweck |
|----------|---------|-------|
| `/api/v1/snapshots/tags` | POST | Snapshot erstellen |
| `/api/v1/snapshots/tags` | GET | Snapshots auflisten |
| `/api/v1/snapshots/tags/:name` | GET | Snapshot abrufen |
| `/api/v1/snapshots/tags/:name` | DELETE | Snapshot löschen |
| `/api/v1/restore/pitr` | POST | Wiederherstellung ausführen |
| `/api/v1/restore/preview` | POST | Wiederherstellung in Vorschau anzeigen |
| `/api/v1/restore/progress` | GET | Wiederherstellungsfortschritt abrufen |

### C. Recovery Time Objectives (RTO)

Geschätzte Wiederherstellungszeiten:

| Datensatzgröße | Geschätztes RTO |
|----------------|-----------------|
| 1K Events | < 1 Sekunde |
| 10K Events | < 10 Sekunden |
| 100K Events | < 2 Minuten |
| 1M Events | < 20 Minuten |
| 10M Events | < 3 Stunden |

*Zeiten sind ungefähre Angaben und hängen von Hardware und Workload ab.*

### D. Notfallkontakte

Pflegen Sie eine Notfallkontaktliste:

- **Datenbankadministrator**: [your-dba@company.com]
- **DevOps Lead**: [devops-lead@company.com]
- **Security Team**: [security@company.com]
- **ThemisDB Support**: [support@themisdb.com]

### E. Zusätzliche Ressourcen

- [Named Snapshots Dokumentation](../features/features_snapshots.md)
- [PITR Feature-Leitfaden](../features/features_pitr.md)
- Backup Best Practices (siehe Englische Dokumentation)
- Sicherheitsrichtlinien (siehe [Security Dokumentation](../security/))

---

**Dokumentversion**: 1.0  
**Zuletzt aktualisiert**: 6. Februar 2026  
**Gepflegt von**: ThemisDB Operations Team  
**Überprüfungszyklus**: Vierteljährlich

---

## Feedback

Für Fragen oder Verbesserungen zu diesem Leitfaden:
- 💬 [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 [Issues melden](https://github.com/makr-code/ThemisDB/issues)
- 📧 Email: [support@themisdb.com](mailto:support@themisdb.com)
