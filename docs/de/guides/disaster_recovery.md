# Disaster Recovery Leitfaden - ThemisDB

**Version:** 1.0  
**Datum:** Januar 2026  
**Status:** ✅ Produktionsbereit  
**Zielgruppe:** Datenbankadministratoren, DevOps Engineers, SREs

---

## Inhaltsverzeichnis

- [Überblick](#überblick)
- [Schnellreferenz](#schnellreferenz)
- [Wiederherstellungsszenarien](#wiederherstellungsszenarien)
- [Schritt-für-Schritt-Anleitungen](#schritt-für-schritt-anleitungen)
- [Best Practices](#best-practices)
- [Fehlerbehebung](#fehlerbehebung)

---

## Überblick

Dieser Leitfaden bietet umfassende Verfahren zur Wiederherstellung von ThemisDB aus verschiedenen Katastrophenszenarien mithilfe von Point-in-Time Recovery (PITR).

### Wann diesen Leitfaden verwenden

Verwenden Sie diesen Leitfaden, wenn Sie:
- **Datenverlust beheben** oder versehentliche Löschungen rückgängig machen müssen
- **Fehlgeschlagene Schema-Migrationen** oder Deployments zurückrollen müssen
- **Auf einen bekannten guten Zustand wiederherstellen** müssen nach Vorfällen
- **Disaster Recovery Verfahren testen** (DR-Übungen)
- **Compliance-Anforderungen** zur Datenwiederherstellung erfüllen müssen

### Voraussetzungen

- ThemisDB v1.5.0+ mit aktiviertem PITR
- Admin-Zugang zur ThemisDB REST API oder CLI
- Named Snapshots (Tags) an kritischen Punkten erstellt
- Changefeed aktiviert und konfiguriert
- Gültige Backup-Strategie vorhanden

---

## Schnellreferenz

### Kritische Befehle

```bash
# Snapshot vor kritischer Operation erstellen
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "before_migration_2026_01",
    "description": "Vor Q1 2026 Schema-Migration",
    "created_by": "admin"
  }'

# Wiederherstellung vorschauen (Dry-Run)
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -H "Content-Type: application/json" \
  -d '{
    "restore_type": "tag",
    "target": "before_migration_2026_01"
  }'

# Wiederherstellung ausführen
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "restore_type": "tag",
    "target": "before_migration_2026_01",
    "dry_run": false,
    "create_backup": true
  }'

# Wiederherstellungsfortschritt prüfen
curl http://localhost:8080/api/v1/restore/progress
```

---

## Wiederherstellungsszenarien

### Szenario 1: Versehentliche Datenlöschung

**Symptome:**
- Kritische Daten fehlen in Tabellen
- Benutzer melden verlorene Datensätze
- Audit-Logs zeigen DELETE-Operationen

**Lösung:** [Point-in-Time Recovery zum letzten bekannten guten Zustand](#pitr-zum-letzten-bekannten-guten-zustand)

---

### Szenario 2: Fehlgeschlagene Schema-Migration

**Symptome:**
- Anwendungsfehler nach Migration
- Schema-Inkompatibilitätsprobleme
- Fehlgeschlagene Foreign-Key-Constraints

**Lösung:** [Rollback zum Pre-Migration Snapshot](#rollback-zum-pre-migration-snapshot)

---

### Szenario 3: Datenkorruption

**Symptome:**
- Datenbank meldet Korruptionsfehler
- Inkonsistente Abfrageergebnisse
- RocksDB Kompaktierungsfehler

**Lösung:** [Selektive Tabellenwiederherstellung](#selektive-tabellenwiederherstellung)

---

## Schritt-für-Schritt-Anleitungen

### PITR zum letzten bekannten guten Zustand

**Anwendungsfall:** Wiederherstellung nach aktueller Datenkorruption oder Löschung

**Schritte:**

1. **Vorfallzeit identifizieren**
   ```bash
   # Changefeed auf aktuelle Events prüfen
   curl http://localhost:8080/api/v1/changefeed/events?limit=100
   ```

2. **Geeigneten Wiederherstellungspunkt finden**
   ```bash
   # Verfügbare Tags auflisten
   curl http://localhost:8080/api/v1/snapshots/tags
   
   # Oder Zeitstempel verwenden (1 Stunde zurück)
   TIMESTAMP=$(($(date +%s) - 3600))000  # Unix-Zeitstempel in ms
   ```

3. **Wiederherstellung vorschauen**
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/preview \
     -H "Content-Type: application/json" \
     -d "{
       \"restore_type\": \"timestamp\",
       \"target\": \"$TIMESTAMP\"
     }"
   ```

4. **Preview-Ausgabe prüfen**
   - `events_to_replay` Anzahl prüfen
   - `affected_tables` Liste verifizieren
   - `estimated_duration_sec` überprüfen
   - `estimated_size_bytes` bestätigen

5. **Wiederherstellung ausführen**
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/pitr \
     -H "Content-Type: application/json" \
     -d "{
       \"restore_type\": \"timestamp\",
       \"target\": \"$TIMESTAMP\",
       \"dry_run\": false,
       \"create_backup\": true,
       \"backup_tag\": \"emergency_backup_$(date +%Y%m%d_%H%M%S)\"
     }"
   ```

6. **Fortschritt überwachen**
   ```bash
   # Fortschritt alle 10 Sekunden prüfen
   while true; do
     curl http://localhost:8080/api/v1/restore/progress
     sleep 10
   done
   ```

7. **Wiederherstellung verifizieren**
   ```bash
   # Kritische Tabellen prüfen
   curl http://localhost:8080/api/v1/query \
     -H "Content-Type: application/json" \
     -d '{"query": "SELECT COUNT(*) FROM critical_table"}'
   ```

**Erwartete Dauer:** 10-60 Minuten

---

### Rollback zum Pre-Migration Snapshot

**Anwendungsfall:** Fehlgeschlagene Schema- oder Datenmigration rückgängig machen

**Schritte:**

1. **Anwendungstraffic stoppen** (falls möglich)
   ```bash
   # Anwendung in Wartungsmodus versetzen
   kubectl scale deployment/app --replicas=0
   ```

2. **Migrations-Tag identifizieren**
   ```bash
   # Tags sollten Namenskonvention folgen: before_migration_YYYYMMDD
   curl http://localhost:8080/api/v1/snapshots/tags/before_migration_20260115
   ```

3. **Rollback vorschauen**
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/preview \
     -H "Content-Type: application/json" \
     -d '{
       "restore_type": "tag",
       "target": "before_migration_20260115"
     }'
   ```

4. **Rollback ausführen**
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/pitr \
     -H "Content-Type: application/json" \
     -d '{
       "restore_type": "tag",
       "target": "before_migration_20260115",
       "dry_run": false,
       "create_backup": true,
       "abort_on_first_error": true
     }'
   ```

5. **Schema-Zustand verifizieren**
   ```bash
   # Tabellen-Schemas prüfen
   curl http://localhost:8080/api/v1/schema/tables
   ```

6. **Anwendungstraffic fortsetzen**
   ```bash
   kubectl scale deployment/app --replicas=3
   ```

**Erwartete Dauer:** 5-30 Minuten

---

### Selektive Tabellenwiederherstellung

**Anwendungsfall:** Spezifische Tabellen wiederherstellen ohne die gesamte Datenbank zu beeinflussen

**Schritte:**

1. **Betroffene Tabellen identifizieren**
   ```bash
   # Tabellen auflisten
   curl http://localhost:8080/api/v1/schema/tables
   ```

2. **Selektive Wiederherstellung vorschauen**
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/preview \
     -H "Content-Type: application/json" \
     -d '{
       "restore_type": "tag",
       "target": "before_incident",
       "tables": ["users", "orders"]
     }'
   ```

3. **Selektive Wiederherstellung ausführen**
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/pitr \
     -H "Content-Type: application/json" \
     -d '{
       "restore_type": "tag",
       "target": "before_incident",
       "dry_run": false,
       "create_backup": true,
       "tables": ["users", "orders"]
     }'
   ```

4. **Nur betroffene Tabellen verifizieren**
   ```bash
   # Wiederhergestellte Tabellen prüfen
   curl http://localhost:8080/api/v1/query \
     -H "Content-Type: application/json" \
     -d '{"query": "SELECT COUNT(*) FROM users"}'
   ```

**Erwartete Dauer:** 5-20 Minuten

---

## Best Practices

### Vor Katastrophen

#### 1. Proaktive Snapshot-Erstellung

Snapshots an kritischen Punkten erstellen:

```bash
# Vor Deployments
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d '{"tag_name": "before_deploy_v1.2.0", "description": "..."}'

# Vor Schema-Migrationen
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d '{"tag_name": "before_migration_YYYYMMDD", "description": "..."}'
```

#### 2. Snapshot-Namenskonvention

Konsistente Benennung befolgen:
- `before_deploy_<version>` - Pre-Deployment Snapshots
- `before_migration_<YYYYMMDD>` - Pre-Migration Snapshots
- `daily_backup_<YYYYMMDD>` - Regelmäßige Backups
- `emergency_<YYYYMMDD_HHMMSS>` - Notfall-Snapshots

#### 3. Regelmäßige DR-Übungen

Wiederherstellungsverfahren vierteljährlich testen

#### 4. Changefeed-Aufbewahrung

Angemessene Aufbewahrung konfigurieren:

```yaml
# config.yaml
changefeed:
  retention_days: 90  # 90 Tage Events behalten
  max_events: 10000000  # Ereignisse limitieren
```

### Während Katastrophen

#### 1. Ruhe bewahren und Verfahren befolgen

- **Nicht in Panik geraten** - Diesem Leitfaden Schritt für Schritt folgen
- **Kommunizieren** - Stakeholder sofort benachrichtigen
- **Dokumentieren** - Alle durchgeführten Aktionen aufzeichnen
- **Verifizieren** - Immer vor der Ausführung vorschauen

#### 2. Dry-Run zuerst verwenden

Immer mit `dry_run: true` testen

#### 3. Notfall-Backup erstellen

Automatisches Backup niemals überspringen:

```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -d '{
    "restore_type": "tag",
    "target": "before_incident",
    "create_backup": true,  # Immer Backup erstellen!
    "backup_tag": "emergency_backup_20260115_143000"
  }'
```

### Nach der Wiederherstellung

#### 1. Datenintegrität verifizieren

Validierungsabfragen ausführen

#### 2. Vorfall dokumentieren

Vorfallsbericht erstellen

#### 3. Alte Snapshots aufräumen

Notfall-Snapshots nach Verifizierung entfernen

---

## Fehlerbehebung

### Problem: Wiederherstellung dauert zu lange

**Lösungen:**
1. `estimated_duration_sec` in Preview prüfen
2. Selektive Tabellenwiederherstellung verwenden, falls möglich
3. Events mit `max_events_to_replay` limitieren

---

### Problem: Wiederherstellung schlägt mit Fehler fehl

**Lösungen:**
1. Fehlermeldung in Antwort prüfen
2. Verifizieren, dass Ziel-Sequence/Tag existiert
3. Ausreichend Festplattenspeicher sicherstellen
4. RocksDB-Logs prüfen

---

### Problem: Wiederherstellungspunkt nicht gefunden

**Lösungen:**
1. Alle verfügbaren Snapshots auflisten
2. Changefeed verwenden, um passende Sequence zu finden
3. Wiederherstellung zum nächstgelegenen verfügbaren Punkt erwägen

---

## Anhang

### A. Wiederherstellungstypen-Vergleich

| Typ | Anwendungsfall | Beispiel | Vorteile | Nachteile |
|-----|----------------|----------|----------|-----------|
| **Sequence** | Präzise Wiederherstellung | `12345` | Exakter Punkt | Sequence-Nummer muss bekannt sein |
| **Tag** | Benannte Checkpoints | `before_deploy_v1.0` | Menschenlesbar | Tags müssen proaktiv erstellt werden |
| **Timestamp** | Zeitbasierte Wiederherstellung | `1705045200000` | Intuitiv | Weniger präzise |

### B. Wiederherstellungsoptionen-Referenz

```json
{
  "restore_type": "sequence|tag|timestamp",
  "target": "...",
  "dry_run": false,
  "create_backup": true,
  "abort_on_first_error": true,
  "tables": [],
  "max_events_to_replay": 0,
  "backup_tag": "before_pitr_restore"
}
```

### C. Fortschrittsphasen

1. `not_started` - Wiederherstellung noch nicht initiiert
2. `validating` - Wiederherstellungsparameter validieren
3. `creating_backup` - Automatisches Backup erstellen
4. `replaying_events` - Events rückwärts abspielen
5. `committing` - Änderungen committen
6. `completed` - Wiederherstellung erfolgreich abgeschlossen
7. `failed` - Wiederherstellung mit Fehler fehlgeschlagen
8. `rolled_back` - Wiederherstellung wegen Fehler zurückgerollt

---

## Fazit

Dieser Leitfaden bietet umfassende Verfahren zur Disaster Recovery mit ThemisDB's PITR-Funktionen. Regelmäßige Übung, proaktive Snapshot-Erstellung und Befolgung der Best Practices gewährleisten erfolgreiche Wiederherstellung bei Bedarf.

**Denken Sie daran:**
- Immer vorschauen vor der Ausführung
- Immer Backups erstellen
- Immer nach der Wiederherstellung verifizieren
- Immer Vorfälle dokumentieren
- Immer aus Vorfällen lernen

---

**Dokumentenkontrolle:**
- **Version:** 1.0
- **Letzte Aktualisierung:** Januar 2026
- **Nächste Überprüfung:** April 2026
- **Eigentümer:** ThemisDB Operations Team
