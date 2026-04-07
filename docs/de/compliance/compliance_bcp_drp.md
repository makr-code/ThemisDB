# Business Continuity & Disaster Recovery Plan (BCP/DRP) - ThemisDB

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🔒 Compliance  
**Klassifizierung:** Vertraulich  
**Basis:** BSI C5 (SIM-05/06), ISO 22301, ISO 27001 (A.17), NIS2 Art. 21(2)(c)

---

## 📑 Inhaltsverzeichnis

- [Executive Summary](#-executive-summary)
- [Zielsetzung](#1-zielsetzung-und-geltungsbereich)
- [Business Impact](#2-business-impact-analysis-bia)

---

## 📋 Executive Summary

Dieses Dokument definiert die Strategien und Verfahren zur Aufrechterhaltung des Geschäftsbetriebs (Business Continuity) und zur Wiederherstellung nach Katastrophen (Disaster Recovery) für ThemisDB-Systeme.

### Dokumentinformationen

| Feld | Wert |
|------|------|
| **Verantwortlicher** | [Organisation eintragen] |
| **BC-Manager** | [Name/Kontakt eintragen] |
| **DR-Manager** | [Name/Kontakt eintragen] |
| **System** | ThemisDB Multi-Model Database |
| **Erstellt** | November 2025 |
| **Nächstes Review** | [Datum + 12 Monate] |

---

## 1. Zielsetzung und Geltungsbereich

### 1.1 Ziele

- Minimierung von Ausfallzeiten und Datenverlust
- Sicherstellung der Geschäftskontinuität
- Schnelle Wiederherstellung kritischer Systeme
- Einhaltung regulatorischer Anforderungen (NIS2, KRITIS)

### 1.2 Geltungsbereich

| Komponente | Abgedeckt | Priorität |
|------------|-----------|-----------|
| ThemisDB Core Server | ✅ | Kritisch |
| RocksDB Storage Engine | ✅ | Kritisch |
| Audit-Logging System | ✅ | Hoch |
| Admin Tools (WPF) | ⚠️ | Mittel |
| Client SDKs | ⚠️ | Niedrig |
| Dokumentation | ⚠️ | Niedrig |

---

## 2. Business Impact Analysis (BIA)

### 2.1 Kritische Geschäftsprozesse

| Prozess | Beschreibung | RTO | RPO | Priorität |
|---------|--------------|-----|-----|-----------|
| Datenbankoperationen | CRUD, Queries, Transaktionen | 1h | 5min | P1 |
| Authentifizierung | Login, Token-Validierung | 30min | 0min | P1 |
| Audit-Logging | Security Events | 2h | 0min | P1 |
| Backup/Recovery | Checkpoints, WAL | 4h | 15min | P2 |
| Monitoring | Prometheus Metrics | 4h | 1h | P2 |
| Admin-Operationen | Key Rotation, Config | 8h | 1h | P3 |

### 2.2 Recovery Time Objective (RTO)

| Tier | RTO | Beschreibung |
|------|-----|--------------|
| **Tier 1** | < 1 Stunde | Kritische Systeme (DB, Auth) |
| **Tier 2** | < 4 Stunden | Wichtige Systeme (Backup, Monitoring) |
| **Tier 3** | < 8 Stunden | Unterstützende Systeme (Admin Tools) |
| **Tier 4** | < 24 Stunden | Nicht-kritische Systeme (Docs) |

### 2.3 Recovery Point Objective (RPO)

| Tier | RPO | Datenverlust-Toleranz |
|------|-----|----------------------|
| **Tier 1** | 0-5 Minuten | Transaktionsdaten |
| **Tier 2** | 15-30 Minuten | Konfiguration |
| **Tier 3** | 1-4 Stunden | Logs, Metrics |
| **Tier 4** | 24 Stunden | Dokumentation |

### 2.4 Maximum Tolerable Downtime (MTD)

| System | MTD | Auswirkung bei Überschreitung |
|--------|-----|-------------------------------|
| ThemisDB Core | 4 Stunden | Geschäftsunterbrechung |
| Audit-System | 8 Stunden | Compliance-Verstoß |
| Backup-System | 24 Stunden | Erhöhtes Risiko |

---

## 3. Risikobewertung

### 3.1 Bedrohungsszenarien

| Szenario | Wahrscheinlichkeit | Auswirkung | Risiko |
|----------|-------------------|------------|--------|
| Hardware-Ausfall (Server) | Mittel | Hoch | Hoch |
| Hardware-Ausfall (Storage) | Niedrig | Sehr Hoch | Hoch |
| Stromausfall | Mittel | Mittel | Mittel |
| Netzwerkausfall | Mittel | Hoch | Hoch |
| Cyberangriff (Ransomware) | Mittel | Sehr Hoch | Hoch |
| Cyberangriff (DDoS) | Hoch | Mittel | Hoch |
| Naturkatastrophe | Niedrig | Sehr Hoch | Mittel |
| Menschlicher Fehler | Hoch | Mittel | Hoch |
| Software-Bug | Mittel | Mittel | Mittel |
| Datacenter-Ausfall | Niedrig | Sehr Hoch | Mittel |

### 3.2 Risikomatrix

```
                    AUSWIRKUNG
              Niedrig  Mittel  Hoch  S.Hoch
           ┌────────┬────────┬──────┬──────┐
     Hoch  │ Mittel │  Hoch  │ Hoch │ Krit │
W.keit     ├────────┼────────┼──────┼──────┤
    Mittel │ Niedrig│ Mittel │ Hoch │ Hoch │
           ├────────┼────────┼──────┼──────┤
   Niedrig │ Niedrig│ Niedrig│Mittel│Mittel│
           └────────┴────────┴──────┴──────┘
```

---

## 4. Business Continuity Strategien

### 4.1 Präventive Maßnahmen

| Maßnahme | Beschreibung | Status |
|----------|--------------|--------|
| **Redundante Hardware** | RAID, Multi-Disk | ⚠️ Deployment-abhängig |
| **Regelmäßige Backups** | RocksDB Checkpoints | ✅ Implementiert |
| **WAL-Archivierung** | Point-in-Time Recovery | ✅ Implementiert |
| **Monitoring & Alerting** | Prometheus, Alertmanager | ✅ Implementiert |
| **Automatische Failover** | Cluster-Modus | 📋 Geplant (Sharding) |
| **Geografische Redundanz** | Multi-Region | ⚠️ Deployment-abhängig |

### 4.2 Backup-Strategie

#### Backup-Typen

| Typ | Frequenz | Retention | Speicherort |
|-----|----------|-----------|-------------|
| **Continuous WAL** | Kontinuierlich | 7 Tage | Lokal + Remote |
| **Checkpoints** | Alle 6 Stunden | 30 Tage | Lokal + Remote |
| **Full Backup** | Täglich | 90 Tage | Remote (verschlüsselt) |
| **Archiv** | Monatlich | 7 Jahre | Cold Storage |

#### Backup-Kommandos

```bash
# Manuelles Checkpoint erstellen
curl -X POST http://localhost:8765/admin/backup?path=/backup/checkpoint_$(date +%Y%m%d_%H%M%S)

# WAL-Archivierung verifizieren
ls -la data/wal/

# Backup-Integrität prüfen
./themis_server --verify-backup /backup/checkpoint_YYYYMMDD
```

### 4.3 Hochverfügbarkeits-Konfiguration

```yaml
# Empfohlene Produktionskonfiguration
high_availability:
  replication:
    enabled: true
    mode: sync  # async für höhere Performance
    replicas: 2
  
  failover:
    automatic: true
    timeout_seconds: 30
    health_check_interval: 5
  
  load_balancing:
    algorithm: round_robin
    health_endpoint: /health
```

---

## 5. Disaster Recovery Verfahren

### 5.1 Wiederherstellungsphasen

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Phase 1   │───▶│   Phase 2   │───▶│   Phase 3   │───▶│   Phase 4   │
│ Aktivierung │    │ Eskalation  │    │ Recovery    │    │ Rückkehr    │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
     0-15min          15-60min          1-4 Stunden       Nach Recovery
```

### 5.2 Phase 1: Aktivierung (0-15 Minuten)

#### Checkliste

- [ ] Incident bestätigen und klassifizieren
- [ ] BC/DR-Team benachrichtigen
- [ ] Erste Schadensbewertung durchführen
- [ ] Entscheidung: BC-Plan aktivieren?

#### Aktivierungskriterien

| Kriterium | Schwellenwert | Aktion |
|-----------|--------------|--------|
| Systemausfall | > 30 Minuten | BC aktivieren |
| Datenverlust | > RPO | DR aktivieren |
| Mehrere Systeme betroffen | > 2 | Eskalieren |
| Sicherheitsvorfall | Jeder | IRP + BC |

### 5.3 Phase 2: Eskalation (15-60 Minuten)

#### Eskalationsmatrix

| Stufe | Trigger | Benachrichtigung |
|-------|---------|------------------|
| **L1** | System langsam/instabil | Operations Team |
| **L2** | System ausgefallen | + BC-Manager |
| **L3** | Mehrere Systeme | + Management |
| **L4** | Kritischer Datenverlust | + Legal, Behörden |

#### Kommunikationsplan

| Stakeholder | Kanal | Frist |
|-------------|-------|-------|
| IT-Operations | Slack/Teams | Sofort |
| Management | E-Mail + Telefon | 30 Minuten |
| Kunden | E-Mail + Status-Page | 1 Stunde |
| Behörden (NIS2) | Offiziell | 24 Stunden |

### 5.4 Phase 3: Recovery (1-4 Stunden)

#### Szenario A: Hardware-Ausfall

```bash
# 1. Backup-Server aktivieren
ssh backup-server "systemctl start themis"

# 2. DNS/Load Balancer umschalten
./scripts/failover.sh --target backup-server

# 3. Daten von letztem Checkpoint wiederherstellen
./themis_server --restore /backup/latest_checkpoint

# 4. WAL-Replay für Point-in-Time Recovery
./themis_server --replay-wal --until "2025-01-15 14:30:00"

# 5. Integrität verifizieren
curl http://localhost:8765/health
```

#### Szenario B: Datenbeschädigung

```bash
# 1. Service stoppen
systemctl stop themis

# 2. Beschädigte Daten sichern (Forensik)
mv data/ data_corrupted_$(date +%Y%m%d)/

# 3. Letztes gutes Backup identifizieren
ls -la /backup/checkpoints/ | tail -10

# 4. Backup wiederherstellen
./themis_server --restore /backup/checkpoint_YYYYMMDD

# 5. Service starten und verifizieren
systemctl start themis
./scripts/verify_data_integrity.sh
```

#### Szenario C: Ransomware/Cyberangriff

```bash
# 1. SOFORT: Netzwerk isolieren
iptables -P INPUT DROP
iptables -P OUTPUT DROP

# 2. Incident Response Team aktivieren
# (Siehe INCIDENT_RESPONSE_PLAN.md)

# 3. Forensische Kopie erstellen
dd if=/dev/sda of=/forensic/disk_image.img

# 4. Saubere Infrastruktur aufbauen
./scripts/deploy_clean_environment.sh

# 5. Backup aus isoliertem Storage wiederherstellen
./themis_server --restore /offline_backup/verified_checkpoint
```

### 5.5 Phase 4: Rückkehr zum Normalbetrieb

#### Checkliste

- [ ] Alle Systeme funktionieren normal
- [ ] Datenintegrität verifiziert
- [ ] Performance-Baseline erreicht
- [ ] Monitoring-Alerts konfiguriert
- [ ] Stakeholder informiert
- [ ] Lessons Learned dokumentiert
- [ ] BC/DR-Plan aktualisiert (falls nötig)

#### Verifikationsschritte

```bash
# 1. Health Check
curl -s http://localhost:8765/health | jq .

# 2. Performance-Test
./benchmarks/bench_sanity

# 3. Audit-Log prüfen
tail -100 data/logs/audit.jsonl | jq .

# 4. Metrics verifizieren
curl -s http://localhost:8765/metrics | grep themis_
```

---

## 6. Rollen und Verantwortlichkeiten

### 6.1 BC/DR-Team

| Rolle | Verantwortlichkeiten | Kontakt |
|-------|---------------------|---------|
| **BC-Manager** | Gesamtkoordination, Entscheidungen | [TBD] |
| **DR-Manager** | Technische Wiederherstellung | [TBD] |
| **Operations Lead** | Systemadministration | [TBD] |
| **Communications Lead** | Stakeholder-Kommunikation | [TBD] |
| **Security Lead** | Sicherheitsbewertung | [TBD] |

### 6.2 Bereitschaftsplan

| Tag | Primär On-Call | Sekundär On-Call |
|-----|----------------|------------------|
| Mo-Fr | Operations | DR-Manager |
| Sa-So | BC-Manager | Operations |
| Feiertage | Rotierend | Rotierend |

---

## 7. Kommunikation

### 7.1 Interne Kommunikation

```
                    ┌─────────────┐
                    │ BC-Manager  │
                    └──────┬──────┘
           ┌───────────────┼───────────────┐
           ▼               ▼               ▼
    ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
    │ Operations  │ │  Security   │ │   Comms     │
    └─────────────┘ └─────────────┘ └─────────────┘
           │               │               │
           ▼               ▼               ▼
    ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
    │ Tech Teams  │ │  Forensik   │ │  Kunden     │
    └─────────────┘ └─────────────┘ └─────────────┘
```

### 7.2 Kommunikationsvorlagen

#### Interne Benachrichtigung

```
BETREFF: [BC/DR] ThemisDB - [Incident-Typ] - [Status]

Status: [Aktiv | In Bearbeitung | Gelöst]
Beginn: [Zeitpunkt]
Betroffene Systeme: [Liste]
Geschätzte Wiederherstellungszeit: [ETA]

Aktuelle Maßnahmen:
- [Maßnahme 1]
- [Maßnahme 2]

Nächstes Update: [Zeitpunkt]
```

#### Kunden-Benachrichtigung

```
BETREFF: Service-Unterbrechung - ThemisDB

Liebe Kunden,

wir erleben derzeit eine [Beschreibung der Störung].

Status: [Aktiv | In Bearbeitung | Gelöst]
Beginn: [Zeitpunkt]
Voraussichtliche Wiederherstellung: [ETA]

Wir arbeiten mit Hochdruck an der Lösung und halten Sie 
auf dem Laufenden.

Status-Updates: [Status-Page URL]

Mit freundlichen Grüßen
[Team]
```

---

## 8. Tests und Übungen

### 8.1 Testplan

| Test-Typ | Frequenz | Beschreibung |
|----------|----------|--------------|
| **Backup-Verifikation** | Wöchentlich | Automatische Integritätsprüfung |
| **Restore-Test** | Monatlich | Wiederherstellung in Test-Umgebung |
| **Failover-Test** | Vierteljährlich | Umschaltung auf Backup-System |
| **Tabletop-Übung** | Halbjährlich | Simulation mit BC/DR-Team |
| **Full DR-Test** | Jährlich | Vollständige Wiederherstellung |

### 8.2 Testprotokoll-Vorlage

| Feld | Wert |
|------|------|
| **Test-Datum** | [Datum] |
| **Test-Typ** | [Backup/Restore/Failover/Full DR] |
| **Durchgeführt von** | [Name] |
| **Szenario** | [Beschreibung] |
| **RTO erreicht?** | [Ja/Nein - Ziel: X Stunden, Tatsächlich: Y Stunden] |
| **RPO erreicht?** | [Ja/Nein - Ziel: X Minuten, Tatsächlich: Y Minuten] |
| **Probleme** | [Beschreibung] |
| **Lessons Learned** | [Erkenntnisse] |
| **Verbesserungen** | [Empfehlungen] |

### 8.3 Letzte Tests

| Datum | Test-Typ | Ergebnis | Notizen |
|-------|----------|----------|---------|
| [TBD] | Backup-Verifikation | [Pending] | - |
| [TBD] | Restore-Test | [Pending] | - |
| [TBD] | Failover-Test | [Pending] | - |

---

## 9. Wartung und Aktualisierung

### 9.1 Review-Zyklus

| Aktivität | Frequenz | Verantwortlich |
|-----------|----------|----------------|
| BIA-Update | Jährlich | BC-Manager |
| Risikobewertung | Halbjährlich | Security Lead |
| Kontaktlisten | Vierteljährlich | Operations |
| Backup-Verifizierung | Wöchentlich | Automatisch |
| DR-Tests | Gemäß Testplan | DR-Manager |
| Plan-Review | Nach jedem Incident | BC-Manager |

### 9.2 Änderungsmanagement

| Änderung | Genehmigung durch |
|----------|-------------------|
| Kontaktdaten | BC-Manager |
| RTO/RPO-Ziele | Management |
| Backup-Strategie | DR-Manager + Security |
| Neue Systeme | BC-Manager |
| Major Revision | Management |

---

## 10. Compliance-Mapping

### 10.1 Regulatorische Anforderungen

| Standard | Anforderung | Status |
|----------|-------------|--------|
| **BSI C5** | SIM-05, SIM-06, SIM-07 | ✅ |
| **ISO 22301** | Business Continuity | ✅ |
| **ISO 27001** | A.17 Business Continuity | ✅ |
| **NIS2** | Art. 21(2)(c) | ✅ |
| **KRITIS** | §8a BSIG | ✅ |

### 10.2 Audit-Nachweise

| Nachweis | Dokument |
|----------|----------|
| BCP/DRP-Plan | Dieses Dokument |
| Backup-Logs | `data/logs/backup.log` |
| Test-Protokolle | `docs/reports/dr_tests/` |
| Incident-Reports | `docs/reports/incidents/` |

---

## 11. Anhänge

### A. Kontaktliste (Vorlage)

| Rolle | Name | Telefon | E-Mail | Notfall |
|-------|------|---------|--------|---------|
| BC-Manager | [Name] | [Tel] | [Email] | [Mobil] |
| DR-Manager | [Name] | [Tel] | [Email] | [Mobil] |
| Operations | [Name] | [Tel] | [Email] | [Mobil] |
| Security | [Name] | [Tel] | [Email] | [Mobil] |
| Management | [Name] | [Tel] | [Email] | [Mobil] |
| Externe IT | [Firma] | [Tel] | [Email] | [24/7] |

### B. Kritische Systeme

| System | Standort | IP/Hostname | Credentials |
|--------|----------|-------------|-------------|
| Primary DB | DC1 | db-primary.internal | Vault: db/primary |
| Backup DB | DC2 | db-backup.internal | Vault: db/backup |
| Backup Storage | S3/Cloud | s3://themis-backup | Vault: s3/backup |
| Monitoring | DC1 | prometheus.internal | Vault: mon/admin |

### C. Referenzdokumente

| Dokument | Pfad |
|----------|------|
| Incident Response Plan | `docs/security/INCIDENT_RESPONSE_PLAN.md` |
| Security Audit Report | `docs/reports/SECURITY_AUDIT_REPORT.md` |
| Backup Documentation | `docs/operations_runbook.md` |
| Deployment Guide | `docs/guides/deployment.md` |

---

## 12. Unterschriften

| Rolle | Name | Datum | Unterschrift |
|-------|------|-------|--------------|
| **BC-Manager** | [Name] | [Datum] | _____________ |
| **DR-Manager** | [Name] | [Datum] | _____________ |
| **IT-Leitung** | [Name] | [Datum] | _____________ |
| **Geschäftsführung** | [Name] | [Datum] | _____________ |

---

**Letzte Aktualisierung:** November 2025  
**Dokumentverantwortlicher:** ThemisDB BC/DR Team  
**Nächstes Review:** [Datum + 12 Monate]
