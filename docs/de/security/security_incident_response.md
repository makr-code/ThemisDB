# ThemisDB – Incident Response Plan (IRP)

**Version:** v1.3.0  
**Stand:** 6. April 2026  
**Klassifizierung:** Intern / VS-NfD  
**Typ:** Sicherheits-Notfallplan nach BSI IT-Grundschutz & NIST CSF  
**Kategorie:** 🔒 Security

---

## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [🚨 Incident-Klassifizierung](#-incident-klassifizierung)
- [👥 Incident Response Team (IRT)](#-incident-response-team-irt)
- [📊 Incident Response Phasen](#-incident-response-phasen)

---

## 📋 Übersicht

Dieser Incident Response Plan definiert die Prozesse und Verantwortlichkeiten für die Erkennung, Analyse, Eindämmung und Behebung von Sicherheitsvorfällen in ThemisDB-Umgebungen.

### Geltungsbereich

- ThemisDB Server (Core Database Engine)
- ThemisDB Admin Tools (WPF Suite)
- Client SDKs (JavaScript, Python, Rust, Go, Java, C#)
- Zugehörige Infrastruktur (Container, CI/CD, Dokumentation)

### Referenzen

- **BSI IT-Grundschutz:** DER.2.1 Behandlung von Sicherheitsvorfällen
- **NIST CSF 2.0:** RS (Respond) & RC (Recover) Functions
- **NIS2:** Artikel 23 (Meldepflichten)
- **ISO 27001:2022:** A.5.24-A.5.28 (Incident Management)

---

## 🚨 Incident-Klassifizierung

### Schweregrade

| Stufe | Bezeichnung | Beschreibung | Reaktionszeit |
|-------|-------------|--------------|---------------|
| **P1** | Kritisch | Datenverlust, aktiver Angriff, Systemausfall | < 1 Stunde |
| **P2** | Hoch | Sicherheitslücke, partielle Beeinträchtigung | < 4 Stunden |
| **P3** | Mittel | Verdächtige Aktivität, Performance-Problem | < 24 Stunden |
| **P4** | Niedrig | Policy-Verstoß, Konfigurationsfehler | < 72 Stunden |

### Incident-Kategorien

| Kategorie | Beispiele | Typische Schwere |
|-----------|-----------|------------------|
| **DATA_BREACH** | Unautorisierter Datenzugriff, Datenexfiltration | P1 |
| **MALWARE** | Ransomware, Trojaner, Cryptominer | P1-P2 |
| **UNAUTHORIZED_ACCESS** | Brute-Force, Credential Stuffing | P1-P2 |
| **DOS_ATTACK** | DDoS, Resource Exhaustion | P2 |
| **VULNERABILITY** | Zero-Day, CVE-Exploit | P1-P3 |
| **CONFIGURATION_ERROR** | Fehlkonfiguration, Exposure | P2-P3 |
| **INSIDER_THREAT** | Missbrauch von Privilegien | P1-P2 |
| **PHYSICAL_SECURITY** | Unbefugter Zugang zu Hardware | P2-P3 |
| **SUPPLY_CHAIN** | Kompromittierte Abhängigkeit | P1-P2 |
| **POLICY_VIOLATION** | Verstoß gegen Sicherheitsrichtlinien | P3-P4 |

---

## 👥 Incident Response Team (IRT)

### Rollen und Verantwortlichkeiten

| Rolle | Verantwortlichkeiten | Kontakt |
|-------|---------------------|---------|
| **Incident Commander (IC)** | Gesamtkoordination, Entscheidungen, Kommunikation | [TBD] |
| **Security Lead** | Technische Analyse, Forensik, Eindämmung | [TBD] |
| **Operations Lead** | Systemwiederherstellung, Monitoring | [TBD] |
| **Communications Lead** | Stakeholder-Kommunikation, Dokumentation | [TBD] |
| **Legal/Compliance** | Rechtliche Bewertung, Meldepflichten | [TBD] |

### Eskalationsmatrix

```
P1 (Kritisch):  IC + Security Lead + Operations Lead + Management
P2 (Hoch):      IC + Security Lead + Operations Lead
P3 (Mittel):    Security Lead + Operations Lead
P4 (Niedrig):   Security Lead
```

### Kontaktliste (Vorlage)

| Rolle | Name | Telefon | E-Mail | Bereitschaft |
|-------|------|---------|--------|--------------|
| Incident Commander | [Name] | [Tel] | [Email] | 24/7 |
| Security Lead | [Name] | [Tel] | [Email] | 24/7 |
| Operations Lead | [Name] | [Tel] | [Email] | 24/7 |
| Communications Lead | [Name] | [Tel] | [Email] | Geschäftszeiten |
| Legal/Compliance | [Name] | [Tel] | [Email] | Geschäftszeiten |
| Externer Forensik-Partner | [Firma] | [Tel] | [Email] | On-Call |

---

## 📊 Incident Response Phasen

### Phase 1: Vorbereitung (Preparation)

#### Präventive Maßnahmen

- [x] Logging & Monitoring aktiviert (Prometheus, Audit-Logs)
- [x] RBAC implementiert (4-stufige Hierarchie)
- [x] Encryption at-rest und in-transit (AES-256-GCM, TLS 1.3)
- [x] Backup-Strategie (RocksDB Checkpoints, WAL)
- [x] Vulnerability Disclosure Policy (SECURITY.md)
- [ ] Regelmäßige Security-Schulungen
- [ ] Tabletop-Übungen (jährlich)

#### Werkzeuge und Ressourcen

| Werkzeug | Zweck | Verfügbarkeit |
|----------|-------|---------------|
| Audit-Logs | Event-Analyse | `data/logs/audit.jsonl` |
| Prometheus Metrics | Performance-Monitoring | `/metrics` Endpoint |
| RocksDB Checkpoints | Backup/Recovery | `POST /admin/backup` |
| Gitleaks | Secret Scanning | CI/CD |
| clang-tidy | Static Analysis | CI/CD |
| security-scan.ps1 | Vulnerability Scan | Lokal |

### Phase 2: Erkennung (Detection)

#### Erkennungsmethoden

| Methode | Beschreibung | Verantwortlich |
|---------|--------------|----------------|
| **Monitoring-Alerts** | Prometheus Alertmanager, Anomalie-Erkennung | Operations |
| **Audit-Log-Analyse** | Verdächtige Events (UNAUTHORIZED_ACCESS, LOGIN_FAILED) | Security |
| **Vulnerability Scanning** | Regelmäßige Scans (security-scan.ps1) | Security |
| **User Reports** | Meldungen via SECURITY.md | Security |
| **Threat Intelligence** | CVE-Monitoring, Security Advisories | Security |

#### Indikatoren für Sicherheitsvorfälle (IoC)

| Indikator | Event-Typ | Schwellenwert |
|-----------|-----------|---------------|
| Fehlgeschlagene Logins | `LOGIN_FAILED` | > 10 in 5 Min |
| Privilege Escalation | `PRIVILEGE_ESCALATION_ATTEMPT` | Jedes Auftreten |
| Ungewöhnliche Datenexporte | `DATA_EXPORT` | Außerhalb Geschäftszeiten |
| Konfigurationsänderungen | `CONFIG_MODIFIED` | Unangekündigt |
| Hohe Fehlerrate | `themis_errors_total` | > 5% |
| Ungewöhnlicher Traffic | `themis_requests_total` | > 200% Normal |

#### Alarm-Eskalation

```
Automatische Alerts → Security On-Call → Incident Classification → IRT Aktivierung
```

### Phase 3: Analyse (Analysis)

#### Erste Bewertung (Triage)

1. **Bestätigung:** Handelt es sich um einen echten Sicherheitsvorfall?
2. **Klassifizierung:** Schweregrad (P1-P4) und Kategorie bestimmen
3. **Scope:** Betroffene Systeme, Daten, Benutzer identifizieren
4. **Timeline:** Zeitraum des Vorfalls bestimmen

#### Analyse-Checkliste

- [ ] Audit-Logs sichern (`data/logs/audit.jsonl`)
- [ ] System-Snapshots erstellen (RocksDB Checkpoint)
- [ ] Netzwerk-Logs sammeln (falls verfügbar)
- [ ] Betroffene Entitäten identifizieren
- [ ] Angriffsvektoren analysieren
- [ ] Indicator of Compromise (IoC) dokumentieren
- [ ] Timeline rekonstruieren

#### Forensik-Kommandos

```bash
# Audit-Logs sichern
cp data/logs/audit.jsonl /secure/backup/audit_$(date +%Y%m%d_%H%M%S).jsonl

# RocksDB Checkpoint erstellen
curl -X POST http://localhost:8765/admin/backup?path=/secure/backup/incident_$(date +%Y%m%d)

# Letzte 1000 Audit-Events analysieren
tail -1000 data/logs/audit.jsonl | jq '.action, .timestamp, .user_id'

# Fehlgeschlagene Logins suchen
grep "LOGIN_FAILED" data/logs/audit.jsonl | jq '.'

# Privilege Escalation suchen
grep "PRIVILEGE_ESCALATION" data/logs/audit.jsonl | jq '.'
```

### Phase 4: Eindämmung (Containment)

#### Kurzfristige Eindämmung

| Maßnahme | Kommando/Aktion | Zuständig |
|----------|-----------------|-----------|
| **Benutzer sperren** | RBAC: Rolle entziehen | Security |
| **IP blockieren** | Firewall/Reverse Proxy | Operations |
| **Service isolieren** | Container stoppen/isolieren | Operations |
| **Netzwerk segmentieren** | VLAN-Isolation | Network |

#### Langfristige Eindämmung

| Maßnahme | Beschreibung | Zuständig |
|----------|--------------|-----------|
| **Credentials rotieren** | Alle betroffenen Keys/Tokens | Security |
| **Patches einspielen** | Vulnerability Fixes | Operations |
| **Konfiguration härten** | Security Hardening | Security |
| **Monitoring verstärken** | Zusätzliche Alerts | Operations |

#### Eindämmungs-Checkliste

- [ ] Betroffene Benutzerkonten gesperrt
- [ ] Betroffene API-Tokens widerrufen
- [ ] Firewall-Regeln aktualisiert
- [ ] Verdächtige IPs blockiert
- [ ] Encryption Keys rotiert (falls kompromittiert)
- [ ] Backup verifiziert und gesichert

### Phase 5: Beseitigung (Eradication)

#### Maßnahmen

1. **Root Cause Analysis:** Ursache identifizieren
2. **Malware-Entfernung:** Schädliche Komponenten entfernen
3. **Vulnerability-Patches:** Sicherheitslücken schließen
4. **Konfigurationskorrekturen:** Fehlkonfigurationen beheben
5. **Systemhärtung:** Zusätzliche Sicherheitsmaßnahmen

#### Verifikation

- [ ] Keine weiteren IoCs gefunden
- [ ] Vulnerability Scans clean
- [ ] Penetrationstest bestanden (falls erforderlich)
- [ ] Code Review abgeschlossen (falls Code-Änderungen)

### Phase 6: Wiederherstellung (Recovery)

#### Wiederherstellungsprozess

1. **Backup-Validierung:** Integrität der Backups prüfen
2. **Schrittweise Wiederherstellung:** Dienste nacheinander starten
3. **Monitoring:** Erhöhte Überwachung während Recovery
4. **Benutzer-Benachrichtigung:** Stakeholder informieren

#### Recovery-Checkliste

- [ ] RocksDB-Backup verifiziert
- [ ] Datenbank wiederhergestellt
- [ ] Dienste gestartet
- [ ] Health Checks bestanden (`GET /health`)
- [ ] Performance-Baseline erreicht
- [ ] Monitoring-Alerts konfiguriert
- [ ] Benutzer informiert

#### Recovery-Kommandos

```bash
# Backup-Integrität prüfen
./themis_server --verify-backup /secure/backup/incident_YYYYMMDD

# Dienst starten mit erhöhtem Logging
./themis_server --config config.json --log-level DEBUG

# Health Check
curl http://localhost:8765/health

# Metrics verifizieren
curl http://localhost:8765/metrics | grep themis_
```

### Phase 7: Nachbereitung (Lessons Learned)

#### Post-Incident Review

| Aspekt | Fragen |
|--------|--------|
| **Erkennung** | Wurde der Vorfall rechtzeitig erkannt? |
| **Reaktion** | Waren die Reaktionszeiten angemessen? |
| **Eindämmung** | War die Eindämmung effektiv? |
| **Kommunikation** | War die Kommunikation klar und zeitnah? |
| **Werkzeuge** | Waren die Werkzeuge ausreichend? |
| **Prozesse** | Wo können Prozesse verbessert werden? |

#### Dokumentation

- [ ] Incident Report erstellt
- [ ] Timeline dokumentiert
- [ ] Root Cause dokumentiert
- [ ] Maßnahmen dokumentiert
- [ ] Lessons Learned Meeting durchgeführt
- [ ] IRP aktualisiert (falls nötig)

---

## 📝 Meldepflichten

### Interne Meldung

| Schwere | Meldung an | Frist |
|---------|------------|-------|
| P1 | Management, IRT, Legal | Sofort |
| P2 | IRT, Management | < 4 Stunden |
| P3 | IRT | < 24 Stunden |
| P4 | Security Lead | < 72 Stunden |

### Externe Meldung (NIS2 / DSGVO)

| Ereignis | Behörde | Frist |
|----------|---------|-------|
| Erheblicher Sicherheitsvorfall | ENISA / nationale CSIRT | 24 Stunden (Frühwarnung) |
| Sicherheitsvorfall (NIS2) | Nationale Behörde | 72 Stunden (Meldung) |
| Datenschutzverletzung (DSGVO) | Datenschutzbehörde | 72 Stunden |
| Betroffenen-Benachrichtigung | Betroffene Personen | "Unverzüglich" |
| Abschlussbericht | Behörde | 1 Monat |

### Meldevorlage (Template)

```markdown
# Incident Report

**Incident-ID:** INC-YYYY-NNNN
**Datum/Uhrzeit Entdeckung:** [Timestamp]
**Datum/Uhrzeit Vorfall:** [Timestamp]
**Schweregrad:** P1/P2/P3/P4
**Kategorie:** [DATA_BREACH, MALWARE, etc.]
**Status:** [Offen, In Bearbeitung, Geschlossen]

## Zusammenfassung
[Kurze Beschreibung des Vorfalls]

## Betroffene Systeme
- [System 1]
- [System 2]

## Betroffene Daten
- [Datentyp, Umfang, Klassifizierung]

## Timeline
| Zeitpunkt | Ereignis |
|-----------|----------|
| [Zeit] | [Ereignis] |

## Maßnahmen
- [x] [Maßnahme 1]
- [ ] [Maßnahme 2]

## Root Cause
[Beschreibung der Ursache]

## Lessons Learned
[Erkenntnisse und Verbesserungen]

## Anhänge
- [Log-Auszüge]
- [Screenshots]
- [Analyse-Berichte]
```

---

## 📊 Metriken und KPIs

### Incident Response Metriken

| Metrik | Ziel | Messung |
|--------|------|---------|
| **MTTD** (Mean Time to Detect) | < 1 Stunde | Zeit bis Erkennung |
| **MTTR** (Mean Time to Respond) | < 4 Stunden | Zeit bis Reaktion |
| **MTTC** (Mean Time to Contain) | < 8 Stunden | Zeit bis Eindämmung |
| **MTTR** (Mean Time to Recover) | < 24 Stunden | Zeit bis Wiederherstellung |

### Berichterstattung

- **Wöchentlich:** Security Incident Summary
- **Monatlich:** Incident Trends, KPI-Dashboard
- **Quarterly:** Management Report, Audit-Vorbereitung
- **Jährlich:** IRP Review, Tabletop-Übung

---

## 🔄 Wartung des IRP

### Review-Zyklus

| Aktivität | Frequenz | Verantwortlich |
|-----------|----------|----------------|
| IRP Review | Jährlich | Security Lead |
| Kontaktlisten aktualisieren | Vierteljährlich | Operations |
| Tabletop-Übungen | Halbjährlich | IRT |
| Tool-Validierung | Vierteljährlich | Security |
| Lessons Learned Integration | Nach jedem Incident | Security Lead |

### Versionierung

| Version | Datum | Autor | Änderungen |
|---------|-------|-------|------------|
| 1.0 | November 2025 | ThemisDB Team | Erstversion |

---

## 📚 Anhänge

### A. Kommunikationsvorlagen

#### Interne Benachrichtigung (P1/P2)

```
BETREFF: [DRINGEND] Sicherheitsvorfall - [Kurzbeschreibung]

Liebe Kolleginnen und Kollegen,

wir haben einen Sicherheitsvorfall der Kategorie [P1/P2] festgestellt.

**Was ist passiert:**
[Kurze, nicht-technische Beschreibung]

**Was wir tun:**
[Aktuelle Maßnahmen]

**Was Sie tun sollten:**
[Anweisungen für Mitarbeiter]

**Nächste Updates:**
[Wann weitere Informationen kommen]

Bei Fragen wenden Sie sich an: [Kontakt]

Das Incident Response Team
```

#### Externe Benachrichtigung (Kunden)

```
BETREFF: Wichtige Sicherheitsmitteilung - [Kurzbeschreibung]

Sehr geehrte Kundinnen und Kunden,

wir möchten Sie über einen Sicherheitsvorfall informieren, der [Beschreibung].

**Was ist passiert:**
[Klare, verständliche Beschreibung]

**Ihre Daten:**
[Was war betroffen / nicht betroffen]

**Unsere Maßnahmen:**
[Was wir unternommen haben]

**Empfohlene Maßnahmen:**
[Was Kunden tun sollten]

**Kontakt:**
[Support-Kontakt für Rückfragen]

Mit freundlichen Grüßen
[Unterschrift]
```

### B. Referenzdokumente

- [Security Policy](../SECURITY.md)
- [Audit Logging](../features/audit_logging.md)
- [Threat Model](threat_model.md)
- [Hardening Guide](hardening_guide.md)
- [Key Management](key_management.md)
- [Full Audit Checklist](../FULL_AUDIT_CHECKLIST.md)

---

**Letzte Aktualisierung:** November 2025  
**Dokumentverantwortlicher:** ThemisDB Security Team  
**Nächstes Review:** [Datum + 12 Monate]
