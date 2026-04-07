# ThemisDB - Business Continuity Plan (BCP) & Disaster Recovery Plan (DRP)

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🔒 Compliance  
**Klassifizierung:** Vertraulich  
**BSI C5 Referenz:** SIM-05, SIM-06, SIM-07  
**ISO 22301 Konformität:** Ja

---

## 📑 Inhaltsverzeichnis

- [Einleitung](#1-einleitung)
- [Business Impact Analysis](#2-business-impact-analysis-bia)
- [Recovery Strategien](#recovery-strategien)

---

## 1. Einleitung

### 1.1 Zweck

Dieser Business Continuity Plan (BCP) und Disaster Recovery Plan (DRP) definiert die Maßnahmen zur Aufrechterhaltung und Wiederherstellung des ThemisDB-Betriebs bei Störungen, Ausfällen oder Katastrophen.

### 1.2 Geltungsbereich

- ThemisDB-Datenbanksysteme
- Zugehörige Infrastruktur (Server, Storage, Netzwerk)
- Backup-Systeme
- Monitoring-Systeme
- Dokumentation und Konfiguration

### 1.3 Definitionen

| Begriff | Definition |
|---------|------------|
| **RTO** | Recovery Time Objective - Maximale tolerierbare Ausfallzeit |
| **RPO** | Recovery Point Objective - Maximaler tolerierbarer Datenverlust |
| **MTPD** | Maximum Tolerable Period of Disruption |
| **BIA** | Business Impact Analysis |

---

## 2. Business Impact Analysis (BIA)

### 2.1 Kritische Geschäftsprozesse

| Prozess | Kritikalität | RTO | RPO | MTPD |
|---------|--------------|-----|-----|------|
| Datenbankzugriff (CRUD) | Kritisch | 1h | 5min | 4h |
| Backup-Erstellung | Hoch | 4h | 24h | 24h |
| Monitoring/Alerting | Hoch | 2h | 15min | 8h |
| Admin-Zugang | Mittel | 4h | 1h | 24h |
| Reporting | Niedrig | 24h | 4h | 72h |

### 2.2 Risikobewertung

| Risiko | Wahrscheinlichkeit | Auswirkung | Risikostufe |
|--------|-------------------|------------|-------------|
| Hardware-Ausfall | Mittel | Hoch | Hoch |
| Software-Fehler | Mittel | Mittel | Mittel |
| Cyberangriff | Niedrig | Kritisch | Hoch |
| Naturkatastrophe | Sehr niedrig | Kritisch | Mittel |
| Stromausfall | Niedrig | Hoch | Mittel |
| Menschliches Versagen | Mittel | Mittel | Mittel |
| Datenkorruption | Niedrig | Hoch | Mittel |

---

## 3. Recovery-Strategien

### 3.1 Backup-Strategie

#### 3.1.1 Backup-Typen

| Typ | Häufigkeit | Aufbewahrung | Methode |
|-----|------------|--------------|---------|
| **Full Backup** | Wöchentlich (Sonntag 02:00) | 4 Wochen | RocksDB Checkpoint |
| **Incremental** | Täglich (02:00) | 7 Tage | WAL-Archivierung |
| **Continuous** | Laufend | 24 Stunden | WAL Streaming |

#### 3.1.2 Backup-Skripte

```bash
# Full Backup (Linux)
./scripts/backup-incremental.sh --full --target /backup/themisdb/full

# Incremental Backup
./scripts/backup-incremental.sh --incremental --target /backup/themisdb/incremental

# Windows PowerShell
.\scripts\backup-incremental.ps1 -Type Full -Target D:\Backup\ThemisDB
```

#### 3.1.3 Backup-Verschlüsselung

- AES-256-GCM für Backup-Dateien
- Schlüssel in HSM oder Vault gespeichert
- Separate Schlüssel für Backup (nicht DB-Schlüssel)

### 3.2 Standby-Strategie

| Strategie | Beschreibung | RTO | Kosten |
|-----------|--------------|-----|--------|
| **Cold Standby** | Backup-Restore auf Abruf | 4-8h | Niedrig |
| **Warm Standby** | Vorkonfigurierter Server | 1-2h | Mittel |
| **Hot Standby** | Leader-Follower Replication | < 15min | Hoch |
| **Multi-Master** | Aktiv-Aktiv Cluster | < 1min | Sehr hoch |

### 3.3 Geo-Redundanz

| Konfiguration | Primär | Sekundär | Replikation |
|---------------|--------|----------|-------------|
| **Single Site** | DC1 | - | Keine |
| **Multi-Site Async** | DC1 | DC2 | Async (< 1min) |
| **Multi-Site Sync** | DC1 | DC2 | Sync |
| **Multi-Region** | EU | US | Async |

### 3.4 RAID-like Redundanz (Distributed Sharding)

ThemisDB unterstützt RAID-ähnliche Redundanzstrategien auf Datenbankebene. Diese bieten automatische Datenredundanz und Failover-Fähigkeiten.

#### 3.4.1 Verfügbare Redundanz-Modi

| Modus | Beschreibung | Speichereffizienz | RTO | RPO | Min. Shards |
|-------|--------------|-------------------|-----|-----|-------------|
| **NONE** | Nur Sharding, keine Redundanz | 100% | Backup-abhängig | Backup-abhängig | 1 |
| **MIRROR** | Vollständige Spiegelung (RAID-1) | 50% | < 1 min | 0 | 2 |
| **STRIPE** | Daten-Striping für Throughput (RAID-0) | 100% | Backup-abhängig | Backup-abhängig | 2 |
| **STRIPE_MIRROR** | Striping + Mirror (RAID-10) | 50% | < 1 min | 0 | 4 |
| **PARITY** | Erasure Coding (RAID-5/6) | k/(k+m) | < 5 min | 0 | 3+ |
| **GEO_MIRROR** | Geo-verteilte Replikation | 50-33% | < 5 min | < 1 min | 2+ DCs |

#### 3.4.2 Empfohlene Konfigurationen nach Kritikalität

| Kritikalität | Empfohlener Modus | Begründung |
|--------------|-------------------|------------|
| **Mission Critical** | GEO_MIRROR + STRIPE_MIRROR | Maximale Verfügbarkeit, DC-Ausfallschutz |
| **Business Critical** | STRIPE_MIRROR | Hohe Verfügbarkeit, gute Performance |
| **Standard** | MIRROR | Einfache Redundanz, gutes Kosten/Nutzen |
| **Non-Critical** | PARITY | Speichereffizient, akzeptable Recovery-Zeit |
| **Development** | NONE | Keine Produktionsdaten |

#### 3.4.3 Konfigurationsbeispiel

```yaml
# config/storage_redundancy.yaml
redundancy:
  default_mode: MIRROR
  
  collections:
    # Mission-critical Kundendaten
    customers:
      mode: STRIPE_MIRROR
      min_replicas: 4
      sync_mode: synchronous
      
    # Standard-Transaktionsdaten
    transactions:
      mode: MIRROR
      min_replicas: 2
      sync_mode: semi-synchronous
      
    # Logs und Analytics
    analytics:
      mode: PARITY
      data_shards: 4
      parity_shards: 2
      sync_mode: asynchronous

  # Granulare Blob-Level Redundanz
  blob_level:
    sst_files:
      mode: MIRROR
      priority: high
    wal_files:
      mode: STRIPE_MIRROR
      priority: critical
    index_files:
      mode: MIRROR
      priority: high
    blob_files:
      mode: PARITY
      priority: medium
```

#### 3.4.4 Recovery-Zeiten nach Modus

| Szenario | MIRROR | STRIPE_MIRROR | PARITY | GEO_MIRROR |
|----------|--------|---------------|--------|------------|
| Single Shard Failure | < 1 min | < 1 min | 2-5 min | < 1 min |
| Dual Shard Failure | Backup req. | < 2 min | 5-10 min* | < 2 min |
| DC Failure | N/A | Backup req. | Backup req. | < 5 min |
| Full Cluster Loss | Backup req. | Backup req. | Backup req. | Backup req. |

*bei RAID-6 Konfiguration (2 Parity-Shards)

#### 3.5.1 Leader-Follower Replication

```yaml
# config/replication.yaml
replication:
  mode: leader-follower
  sync_mode: semi-synchronous  # sync | semi-sync | async
  
  leader:
    node_id: themisdb-node-1
    
  followers:
    - node_id: themisdb-node-2
      priority: 100  # Höher = bevorzugt für Promotion
      lag_threshold_sec: 30
    - node_id: themisdb-node-3
      priority: 50
      lag_threshold_sec: 60
      
  failover:
    automatic: true
    health_check_interval_sec: 10
    failure_threshold: 3
    promotion_delay_sec: 30
```

| Sync-Modus | RPO | Performance Impact | Anwendung |
|------------|-----|-------------------|-----------|
| `synchronous` | 0 | Hoch (Latenz +50-100%) | Finanzielle Daten |
| `semi-synchronous` | < 1 sec | Mittel (Latenz +10-30%) | Standard |
| `asynchronous` | < 1 min | Niedrig | Logs, Analytics |

#### 3.5.2 Multi-Master Replication

```yaml
replication:
  mode: multi-master
  
  conflict_resolution:
    strategy: last-write-wins  # lww | vector-clock | custom
    
  nodes:
    - node_id: themisdb-eu
      datacenter: eu-west-1
      region: EU
    - node_id: themisdb-us
      datacenter: us-east-1
      region: US
      
  vector_clocks:
    enabled: true
    sync_interval_ms: 100
```

---

## 4. Recovery-Prozeduren

### 4.1 Point-in-Time Recovery (PITR)

#### 4.1.1 Voraussetzungen

- Vollständiges Backup vorhanden
- WAL-Archive seit Backup verfügbar
- Ziel-Zeitpunkt bekannt

#### 4.1.2 Prozedur

```bash
# 1. ThemisDB stoppen
systemctl stop themisdb

# 2. Datenverzeichnis sichern
mv /var/lib/themisdb /var/lib/themisdb.corrupted

# 3. Letztes Full Backup wiederherstellen
./scripts/restore.sh --source /backup/themisdb/full/latest --target /var/lib/themisdb

# 4. WAL-Archive anwenden bis Zielzeitpunkt
./scripts/restore.sh --apply-wal --until "2025-12-02T14:30:00Z"

# 5. Konsistenz prüfen
./scripts/verify-consistency.sh /var/lib/themisdb

# 6. ThemisDB starten
systemctl start themisdb

# 7. Funktionstest
curl http://localhost:8765/health
```

#### 4.1.3 Erwartete Dauer

| Datenmenge | Full Restore | WAL Apply | Gesamt |
|------------|--------------|-----------|--------|
| 10 GB | 5 min | 2 min | 7 min |
| 100 GB | 30 min | 10 min | 40 min |
| 1 TB | 3 h | 45 min | 4 h |

### 4.2 Failover zu Standby

#### 4.2.1 Automatischer Failover

```yaml
# config/replication.yaml
replication:
  mode: leader-follower
  failover:
    automatic: true
    health_check_interval_sec: 10
    failure_threshold: 3
    promotion_delay_sec: 30
```

#### 4.2.2 Manueller Failover

```bash
# 1. Standby-Status prüfen
themisctl replication status

# 2. Failover initiieren
themisctl replication failover --target standby-node-1

# 3. DNS/Load-Balancer aktualisieren
# (Deployment-spezifisch)

# 4. Alte Primary als Standby neu konfigurieren
themisctl replication demote --node old-primary
```

### 4.3 Bare-Metal Recovery

#### 4.3.1 Voraussetzungen

- Betriebssystem-Image oder Installationsmedien
- Backup der ThemisDB-Daten
- Backup der Konfiguration
- Dokumentation der Netzwerkkonfiguration

#### 4.3.2 Prozedur

1. **Hardware bereitstellen** (0-4h abhängig von Verfügbarkeit)
2. **OS installieren** (~30min)
3. **ThemisDB installieren** (~15min)
4. **Konfiguration wiederherstellen** (~15min)
5. **Daten wiederherstellen** (abhängig von Datenmenge)
6. **Netzwerk konfigurieren** (~15min)
7. **Funktionstest** (~15min)

### 4.4 RAID-Redundanz Recovery

#### 4.4.1 Single Shard Failure (MIRROR/STRIPE_MIRROR)

**Automatischer Recovery (empfohlen):**
```yaml
# Die Redundanz-Engine erkennt den Ausfall automatisch
# und leitet Anfragen an den Mirror um

# Status prüfen:
themisctl redundancy status --cluster production

# Erwartete Ausgabe:
# Cluster: production
# Mode: MIRROR
# Healthy Shards: 3/4
# Degraded: shard-2 (node themisdb-node-2)
# Recovery: IN_PROGRESS (45%)
```

**Manueller Recovery:**
```bash
# 1. Ausgefallenen Shard identifizieren
themisctl shard list --status failed

# 2. Neuen Shard provisionieren
themisctl shard add --node themisdb-node-5 --role mirror

# 3. Resync starten
themisctl redundancy resync --shard shard-2 --source shard-2-mirror

# 4. Status überwachen
themisctl redundancy watch --shard shard-2
```

#### 4.4.2 PARITY (Erasure Coding) Recovery

```bash
# 1. Degradierten Shard identifizieren
themisctl parity status

# 2. Recovery initiieren (automatische Rekonstruktion)
themisctl parity rebuild --shard shard-3

# 3. Fortschritt überwachen
# ACHTUNG: Während Rebuild ist das System weiterhin verfügbar,
# aber mit erhöhter Latenz und ohne weitere Fehlertoleranz

themisctl parity watch
# Output:
# Rebuilding shard-3 from parity...
# Progress: 67% | ETA: 12 min | Throughput: 450 MB/s
```

#### 4.4.3 GEO_MIRROR Recovery (Datacenter Failure)

**Szenario: Primäres Datacenter ausgefallen**

```bash
# 1. DC-Status prüfen
themisctl dc status
# Output:
# DC eu-west-1: UNREACHABLE
# DC us-east-1: HEALTHY

# 2. Failover zum sekundären DC
themisctl dc failover --target us-east-1 --force

# 3. DNS/Load-Balancer aktualisieren
# (Automatisch wenn CloudFlare/Route53 Health Checks konfiguriert)

# 4. Nach DC-Recovery: Resync
themisctl dc resync --source us-east-1 --target eu-west-1

# 5. Failback (optional)
themisctl dc failback --primary eu-west-1
```

#### 4.4.4 Multi-Master Conflict Resolution

**Bei Split-Brain Recovery:**
```bash
# 1. Konflikt-Status prüfen
themisctl conflicts list

# 2. Automatische Resolution (LWW)
themisctl conflicts resolve --strategy lww --dry-run
themisctl conflicts resolve --strategy lww

# 3. Manuelle Resolution (falls nötig)
themisctl conflicts resolve --key "customer:12345" --winner node-eu

# 4. Konsistenz verifizieren
themisctl consistency check --full
```

### 4.5 Streaming Protocol Recovery

#### 4.5.1 Backpressure-Wiederaufnahme nach Überlast

```bash
# 1. Backpressure-Status prüfen
themisctl streaming status
# Output:
# Node themisdb-node-3: BACKPRESSURE_ACTIVE
# Pending: 45,000 ops | Buffer: 89% full
# Deferred since: 2025-12-02T14:30:00Z

# 2. Kapazität erhöhen oder Last reduzieren
themisctl streaming resume --node themisdb-node-3

# 3. WAL-Replay für verpasste Änderungen
themisctl wal replay --node themisdb-node-3 --from "2025-12-02T14:30:00Z"

# 4. Catch-up Status
themisctl streaming catch-up --node themisdb-node-3
# Output:
# Catching up... 12,000 ops remaining | ETA: 2 min
```

---

## 5. Backup-Test-Prozeduren

### 5.1 Regelmäßige Tests

| Test | Häufigkeit | Verantwortlich | Dokumentation |
|------|------------|----------------|---------------|
| Backup-Integrität | Täglich | Automatisiert | Log-Datei |
| Restore-Test (Subset) | Wöchentlich | DBA | Test-Protokoll |
| Full Restore-Test | Monatlich | DBA | Vollständiger Bericht |
| Failover-Test | Quartalsweise | Operations | Failover-Protokoll |
| DR-Übung | Jährlich | alle | DR-Bericht |
| **RAID Degradation Test** | Monatlich | Operations | RAID-Test-Protokoll |
| **Shard Failure Simulation** | Quartalsweise | Operations | Sharding-Test-Protokoll |
| **DC Failover Test** | Halbjährlich | Operations | DC-Failover-Protokoll |
| **Multi-Master Conflict Test** | Quartalsweise | DBA | Conflict-Resolution-Protokoll |

### 5.2 Backup-Integritätsprüfung

```bash
#!/bin/bash
# scripts/verify-backup.sh

BACKUP_PATH="$1"
REPORT_PATH="/var/log/themisdb/backup-verification.log"

echo "=== Backup Verification $(date) ===" >> "$REPORT_PATH"

# 1. Prüfsumme verifizieren
if sha256sum -c "$BACKUP_PATH/checksums.sha256"; then
    echo "✓ Checksums valid" >> "$REPORT_PATH"
else
    echo "✗ Checksum mismatch!" >> "$REPORT_PATH"
    exit 1
fi

# 2. Verschlüsselung prüfen
if openssl enc -d -aes-256-gcm -in "$BACKUP_PATH/data.enc" -out /dev/null -pass file:/etc/themisdb/backup.key 2>/dev/null; then
    echo "✓ Encryption valid" >> "$REPORT_PATH"
else
    echo "✗ Decryption failed!" >> "$REPORT_PATH"
    exit 1
fi

# 3. Metadaten prüfen
if [ -f "$BACKUP_PATH/manifest.json" ]; then
    echo "✓ Manifest exists" >> "$REPORT_PATH"
else
    echo "✗ Manifest missing!" >> "$REPORT_PATH"
    exit 1
fi

echo "=== Verification Complete ===" >> "$REPORT_PATH"
exit 0
```

### 5.3 Restore-Test-Protokoll

```markdown
# Restore-Test-Protokoll

**Datum:** [YYYY-MM-DD]
**Tester:** [Name]
**Backup-Datum:** [YYYY-MM-DD HH:MM]
**Backup-Typ:** [Full/Incremental/PITR]

## Testumgebung
- Server: [Hostname/IP]
- OS: [OS Version]
- ThemisDB Version: [Version]

## Durchführung

| Schritt | Erwartet | Tatsächlich | Status |
|---------|----------|-------------|--------|
| Backup lokalisiert | Vorhanden | | ☐ |
| Checksums gültig | OK | | ☐ |
| Restore gestartet | Ohne Fehler | | ☐ |
| Restore abgeschlossen | Ohne Fehler | | ☐ |
| Datenbank startet | OK | | ☐ |
| Health Check | 200 OK | | ☐ |
| Stichproben-Query | Erwartete Daten | | ☐ |
| Performance akzeptabel | < 2x normal | | ☐ |

## Ergebnis
- [ ] Bestanden
- [ ] Bestanden mit Anmerkungen
- [ ] Nicht bestanden

## Dauer
- Restore: [HH:MM]
- Verifizierung: [HH:MM]
- Gesamt: [HH:MM]

## Anmerkungen
[Freitext]

## Unterschrift
Tester: _________________ Datum: _________
Reviewer: _________________ Datum: _________
```

---

## 6. Kommunikationsplan

### 6.1 Eskalationsmatrix

| Stufe | Situation | Benachrichtigen | Innerhalb |
|-------|-----------|-----------------|-----------|
| 1 | Minor (Service degraded) | On-Call DBA | 15 min |
| 2 | Major (Service down) | Team Lead + On-Call | 30 min |
| 3 | Kritisch (Datenverlust möglich) | Management + Team | 1 h |
| 4 | Katastrophe (Multi-System) | C-Level + alle Teams | Sofort |

### 6.2 Kontaktliste

| Rolle | Name | Telefon | E-Mail | Ersatz |
|-------|------|---------|--------|--------|
| On-Call DBA | [Name] | [Tel] | [Email] | [Name2] |
| Team Lead | [Name] | [Tel] | [Email] | [Name2] |
| Security Lead | [Name] | [Tel] | [Email] | [Name2] |
| Management | [Name] | [Tel] | [Email] | [Name2] |

### 6.3 Status-Kommunikation

- **Intern:** Slack/Teams Channel: #themisdb-incidents
- **Extern:** Status-Page (falls vorhanden)
- **Updates:** Alle 30 Minuten während Incident

---

## 7. Rollen und Verantwortlichkeiten

| Rolle | Verantwortlichkeiten |
|-------|---------------------|
| **Incident Commander** | Gesamtkoordination, Entscheidungen |
| **DBA/Operations** | Technische Wiederherstellung |
| **Security Lead** | Sicherheitsbewertung |
| **Communications** | Interne/externe Kommunikation |
| **Documentation** | Incident-Protokollierung |

---

## 8. Training und Übungen

### 8.1 Trainingsplan

| Training | Zielgruppe | Häufigkeit | Dauer |
|----------|------------|------------|-------|
| BCP-Übersicht | alle | Jährlich | 1h |
| Restore-Prozeduren | DBA/Ops | Quartalsweise | 2h |
| Failover-Prozeduren | DBA/Ops | Halbjährlich | 4h |
| DR-Übung | alle | Jährlich | 1 Tag |

### 8.2 Übungstypen

| Typ | Beschreibung | Aufwand |
|-----|--------------|---------|
| **Tabletop** | Theoretische Durchsprache | Niedrig |
| **Walkthrough** | Schrittweise Prozedur-Review | Mittel |
| **Simulation** | Test in Testumgebung | Mittel |
| **Full-Scale** | Test in Produktion (kontrolliert) | Hoch |

---

## 9. Wartung und Updates

### 9.1 Dokumentations-Review

Diese Dokumentation wird überprüft:
- Nach jedem Incident
- Nach signifikanten Infrastrukturänderungen
- Mindestens jährlich

### 9.2 Änderungshistorie

| Version | Datum | Autor | Änderungen |
|---------|-------|-------|------------|
| 1.0 | Dezember 2025 | ThemisDB Team | Erstversion |
| 1.1 | Dezember 2025 | ThemisDB Team | RAID-Sharding, Replication, Streaming Protocol Recovery hinzugefügt |

---

## 10. Anhänge

### A. Checklisten

#### A.1 Incident-Start-Checkliste

- [ ] Incident Commander benannt
- [ ] War Room / Channel eingerichtet
- [ ] Erste Einschätzung dokumentiert
- [ ] Stakeholder informiert
- [ ] Backup-Status geprüft

#### A.2 Post-Incident-Checkliste

- [ ] Dienste wiederhergestellt
- [ ] Datenintegrität verifiziert
- [ ] Alle Systeme überwacht
- [ ] Incident-Report erstellt
- [ ] Lessons Learned Meeting geplant

### B. Referenzen

- `docs/security/INCIDENT_RESPONSE_PLAN.md`
- `docs/guides/deployment.md`
- `scripts/backup-incremental.sh`
- `scripts/restore.sh`
- `docs/sharding/RAID_REDUNDANCY_ARCHITECTURE.md`
- `docs/sharding/SHARDING_UNIFIED_DOCUMENTATION.md`
- `include/replication/replication_manager.h`
- `include/sharding/redundancy_strategy.h`

### C. RAID-Redundanz Test-Protokoll

```markdown
# RAID-Redundanz Test-Protokoll

**Datum:** [YYYY-MM-DD]
**Tester:** [Name]
**Redundanz-Modus:** [MIRROR/STRIPE_MIRROR/PARITY/GEO_MIRROR]
**Cluster:** [Cluster-Name]

## Testumgebung
- Nodes: [Anzahl]
- Shards: [Anzahl]
- Datenvolumen: [GB]
- Redundanz-Faktor: [N]

## Durchführung: Simulierter Shard-Ausfall

| Schritt | Erwartet | Tatsächlich | Status |
|---------|----------|-------------|--------|
| Shard deaktiviert | System meldet Degradation | | ☐ |
| Failover erfolgt | < 1 min für MIRROR | | ☐ |
| Queries weiterhin möglich | Ja, evtl. erhöhte Latenz | | ☐ |
| Monitoring-Alert | Alert ausgelöst | | ☐ |
| Recovery gestartet | Automatisch oder manuell | | ☐ |
| Resync abgeschlossen | Daten konsistent | | ☐ |
| System wieder healthy | Alle Shards aktiv | | ☐ |

## Metriken

| Metrik | Wert |
|--------|------|
| Time to Detection | [Sekunden] |
| Time to Failover | [Sekunden] |
| Query Latency (während Degradation) | [ms] |
| Resync Duration | [Minuten] |
| Resync Throughput | [MB/s] |

## Ergebnis
- [ ] Bestanden
- [ ] Bestanden mit Anmerkungen
- [ ] Nicht bestanden

## Anmerkungen
[Freitext]

## Unterschrift
Tester: _________________ Datum: _________
Reviewer: _________________ Datum: _________
```

---

**Letzte Aktualisierung:** Dezember 2025  
**Nächstes Review:** Juni 2026  
**Dokumentverantwortlicher:** ThemisDB Operations Team
