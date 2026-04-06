# Gewährleistungs- und Wartungsvertrag - ThemisDB Hyperscaler Edition

**Stand:** April 2026  
**Version:** 1.0  
**Kategorie:** 🌐 Hyperscaler Support & Maintenance

---

## Präambel

Dieser Gewährleistungs- und Wartungsvertrag ("Premium Wartungsvertrag") ergänzt den Softwarevertrag für die ThemisDB Hyperscaler Edition und regelt erweiterte Wartungs-, Support- und Gewährleistungsleistungen mit höchsten SLAs zwischen:

**Dienstleister:**  
ThemisDB GmbH  
[Musterstraße 123]  
[12345 Musterstadt]  
Handelsregister: HRB [XXXXX]  
Geschäftsführer: [Name]

**und**

**Kunde:**  
[Firmenname des Kunden]  
[Adresse des Kunden]  
[PLZ, Ort]  
Vertreten durch: [Name, Funktion]

---

## § 1 Vertragsgegenstand und Premium-Leistungsumfang

### 1.1 Gegenstand
Dieser Premium-Wartungsvertrag regelt erweiterte Wartungs-, Support- und Gewährleistungsleistungen für hyperscale-Deployments mit Mission-Critical-Anforderungen.

### 1.2 Premium-Wartungsleistungen
**Proactive Maintenance:**
- Kontinuierliches Monitoring der Kundenumgebung (opt-in)
- Predictive Analytics für potenzielle Issues
- Automatische Health Checks (wöchentlich)
- Proaktive Performance-Optimierungsvorschläge
- Frühwarnsystem für EOL-Komponenten

**Accelerated Updates:**
- Early Access zu Patches (48h vor Standard-Release)
- Beta-Programm Zugang für neue Features
- Custom Patches innerhalb 4h (P0-Issues)
- Zero-Downtime Update-Support
- Automated Rollback-Assistance

**Extended Maintenance:**
- 24/7/365 Engineering-Support
- Dedicated Development Resources
- Custom Feature Hotfixes
- Database Migration Assistance
- Architecture Evolution Support

### 1.3 Premium-Support-Leistungen
**Dedicated Support Team:**
- Technical Account Manager (TAM) - fulltime dedicated
- Solutions Architect - on-demand
- Senior Site Reliability Engineer (SRE) - shared
- Customer Success Manager (CSM) - quartalsweise
- Escalation Manager - bei kritischen Issues

**White-Glove Support:**
- Named Contacts (keine Warteschlangen)
- Direct Line zu Engineering (bei P0/P1)
- Prioritized Ticket Queue
- Same-Day Callbacks (garantiert)
- Concierge Service für komplexe Anfragen

**Strategic Advisory:**
- Quartalsweise Architecture Reviews
- Annual Strategic Planning Session
- Roadmap Alignment Workshops
- Cost Optimization Analysis
- Competitive Benchmarking

---

## § 2 Erweiterte Gewährleistung (Hyperscaler)

### 2.1 Umfang der Gewährleistung
Zusätzlich zu Enterprise-Gewährleistung garantiert der Dienstleister:

**Funktionale Garantien:**
- 100% API-Kompatibilität (innerhalb Major-Version)
- Skalierung bis 10.000 Nodes (linear)
- Zero-Data-Loss (bei HA-Setup >= 5 Nodes)
- Sub-Second Failover (bei Multi-AZ Deployment)

**Performance-Garantien:**
- Read Latency: p99 < 1ms (bei korrekter Konfiguration)
- Write Latency: p99 < 5ms (bei korrekter Konfiguration)
- Throughput: >= 10M ops/s pro 100-Node-Cluster
- Query Response Time: p95 < 10ms für Standard-Queries

**Verfügbarkeits-Garantien:**
- 99.9% Uptime (Software-Layer)
- 99.95% bei Multi-Region Setup (>= 3 Regionen)
- Automated Failover < 5 Minuten
- Disaster Recovery RTO < 1 Stunde

### 2.2 Erweiterte Fehlerkategorien

**P0 - Site Down (Mission Critical):**
- Kompletter Ausfall des Production-Systems
- Datenverlust oder kritische Datenkorruption
- Zero-Day Security Vulnerability (CVSS >= 9.0)
- Multi-Region Replication Failure

**P1 - Critical (High Impact):**
- Kernfunktion stark beeinträchtigt (>50% Performance-Loss)
- Single-Region Failure (bei Multi-Region Setup)
- High-Severity Security Vulnerability (CVSS 7.0-8.9)
- Data Consistency Issues

**P2 - Major (Significant Impact):**
- Wesentliche Funktionseinschränkung
- Performance Degradation 25-50%
- Medium-Severity Security Vulnerability (CVSS 4.0-6.9)
- Monitoring/Observability Issues

**P3 - Minor (Limited Impact):**
- Nebenfunktionen beeinträchtigt
- Performance Degradation < 25%
- Low-Severity Security Vulnerability (CVSS < 4.0)
- Non-Critical Bugs

**P4 - Enhancement:**
- Feature Requests
- Documentation Updates
- Optimization Opportunities
- Cosmetic Issues

### 2.3 Accelerated Resolution Times

| Priorität | Reaktionszeit | Workaround | Hot-Fix | Permanent Fix |
|-----------|---------------|------------|---------|---------------|
| **P0 - Site Down** | 2 Stunden | 12 Stunden | 3 Tage | 1 Woche |
| **P1 - Critical** | 4 Stunden | 1 Tag | 1 Woche | 2 Wochen |
| **P2 - Major** | 1 Tag | 3 Tage | 2 Wochen | 1 Monat |
| **P3 - Minor** | 2 Tage | 1 Woche | 1 Monat | 2 Monate |
| **P4 - Enhancement** | 1 Werktag | N/A | N/A | Next Release |

**Definitionen:**
- **Reaktionszeit:** Erste Rückmeldung durch Named Support Engineer
- **Workaround:** Temporäre Lösung, die Betrieb ermöglicht
- **Hot-Fix:** Eilpatch für kritische Issues (vor regulärem Release-Zyklus)
- **Permanent Fix:** Vollständige Lösung im regulären Release

### 2.4 Erweiterter Eskalationsprozess
**Fast-Track Escalation:**
- Bypass von L1-Support bei P0/P1
- Direkter Zugang zu L2/L3 Engineers
- Named Contacts (keine Ticketsystem-Warteschlangen)
- Dedizierte Hotline (separate Nummer)

**Management Escalation Path:**
1. **T+4h (P0):** Engineering Manager informiert
2. **T+1 Tag (P0):** VP Engineering eingebunden
3. **T+3 Tage (P0):** CTO Briefing
4. **T+1 Woche (P0):** CEO Briefing (bei Business-Impact)

**War Room Protocol:**
Bei P0-Incidents:
- Dedicated Slack/Teams Channel
- Live Bridge Call (Business Hours)
- Screen Sharing Session
- Echtzeit-Updates alle 4 Stunden
- Post-Incident Review (Mandatory)

### 2.5 Zero-Data-Loss Guarantee
Bei Einsatz von:
- RAID5+ Konfiguration
- >= 5 Nodes mit Quorum
- Multi-AZ Deployment
- Automated Backups aktiviert

Garantiert der Dienstleister:
- Kein Datenverlust bei Single-Node-Failure
- < 1 Minute RPO bei Zone-Failure
- < 5 Minuten RPO bei Region-Failure (Multi-Region Setup)

Bei Verstoß:
- Sofortige Root-Cause-Analyse (innerhalb 24h)
- Data Recovery Support (kostenfrei)
- Kompensation: 200% der Monatsgebühr

---

## § 3 Premium Software-Updates und Continuous Deployment

### 3.1 Continuous Update Stream
**Release Channels:**
- **Stable:** Production-ready (empfohlen für >90% Deployment)
- **Fast:** Early-Access (2-4 Wochen vor Stable, für Staging)
- **Canary:** Daily Builds (für Dev/Test-Umgebungen)
- **LTS:** Long-Term-Support (5 Jahre, nur Critical Fixes)

**Hyperscaler-Benefit:**
- Zugang zu allen Channels (Enterprise: nur Stable + LTS)
- Möglichkeit eigene Custom Builds zu forken
- Early Access zu Roadmap-Features (3-6 Monate Vorlauf)

### 3.2 Zero-Downtime Update Support
**Rolling Update Assistance:**
- Detaillierte Runbooks für Rolling Updates
- Pre-Update Health Check (Remote-Assistance)
- Monitoring während Update (Echtzeit)
- Automated Rollback bei Fehlern (< 5 Minuten)

**Blue-Green Deployment:**
- Anleitung für Blue-Green-Strategie
- Cutover-Support (Telefonkonferenz)
- Traffic-Switching-Validation
- Rollback-Plan (Pre-Approved)

**Canary Release Support:**
- Canary-Deployment-Scripts
- Monitoring-Dashboard-Setup
- Statistical Analysis (Error Rates)
- Progressive Rollout Guidance

### 3.3 Custom Patching
Bei kritischen Kunden-spezifischen Issues:
- Custom Patches innerhalb 4 Stunden (P0)
- Custom Patches innerhalb 24 Stunden (P1)
- Backporting von Fixes zu älteren Versionen (bis 2 Major-Versionen zurück)
- Private Patches (nicht im Public Release)

### 3.4 Version Pinning und Extended Support
- Möglichkeit, auf spezifischer Version zu bleiben (bis 18 Monate)
- Backporting von Security Fixes (kostenlos)
- Backporting von Critical Bugs (kostenfrei)
- Backporting von Feature-Fixes (gegen Aufpreis)

---

## § 4 Premium Service Level Agreement (SLA)

### 4.1 Infrastructure and Software Uptime
**Software Availability:**
- 99.9% Uptime (Single-Region Deployment)
- 99.95% Uptime (Multi-Region Deployment, >= 3 Regionen)
- Messung: Availability of Database Engine (nicht Netzwerk/Hardware)

**Downtime-Definition:**
- Keine erfolgreichen Read/Write-Operationen möglich
- Für >= 1 Minute (unter 1 Minute zählt nicht)
- Bei >= 50% der Nodes (nicht bei Rolling Updates)

**Ausnahmen:**
- Geplante Wartungsfenster (max. 2h/Quartal, 14 Tage Vorlauf)
- Force Majeure Events
- Kundenseitige Infrastruktur-Ausfälle
- DDoS-Attacken (außerhalb Kontrolle des Dienstleisters)

### 4.2 Support Response Times (Guaranteed)

| Priorität | Reaction | First Response | Update Frequency |
|-----------|----------|----------------|------------------|
| **P0** | 30 Minuten | 2 Stunden | Alle 4 Stunden |
| **P1** | 2 Stunden | 4 Stunden | Alle 8 Stunden |
| **P2** | 4 Stunden | 1 Tag | Täglich |
| **P3** | 1 Tag | 2 Tage | Bei Statusänderung |
| **P4** | 4 Stunden | 1 Werktag | Bei Statusänderung |

**Reaction:** Automatisches Paging des On-Call Engineers  
**First Response:** Erste substanzielle Antwort (nicht Auto-Reply)  
**Update Frequency:** Regelmäßige Status-Updates während Bearbeitung

### 4.3 Resolution Time Guarantees

**P0 - Site Down:**
- Workaround: 12 Stunden (80% der Fälle)
- Hot-Fix: 3 Tage (70% der Fälle)
- Root-Cause-Fix: 1 Woche (60% der Fälle)

**P1 - Critical:**
- Workaround: 1 Tag (75% der Fälle)
- Hot-Fix: 1 Woche (65% der Fälle)
- Root-Cause-Fix: 2 Wochen (60% der Fälle)

**SLA-Penalty bei Verfehlung:**
- Miss by 50%: 25% Service Credit
- Miss by 100%: 50% Service Credit
- Miss by 200%: 100% Service Credit

### 4.4 Premium SLA Credits

**Availability Credits:**
| Uptime | Service Credit (% Monthly Fee) |
|--------|-------------------------------|
| 99.9% - 99.5% | 10% |
| 99.5% - 99.0% | 25% |
| 99.0% - 98.5% | 50% |
| < 98.5% | 100% |

**Multi-Region Availability Credits:**
| Uptime (Multi-Region) | Service Credit |
|-----------------------|----------------|
| 99.95% - 99.90% | 10% |
| 99.90% - 99.50% | 25% |
| 99.50% - 99.00% | 50% |
| < 99.00% | 100% + 1 Month Free |

**Response Time Credits:**
| SLA Breach | Service Credit |
|------------|----------------|
| P0 Response > 4h | 15% |
| P0 Workaround > 24h | 25% |
| P1 Response > 8h | 10% |
| P1 Workaround > 2 Tage | 20% |

**Maximum Credits:** 200% der Monatsgebühr (kann übertragen werden)

### 4.5 Performance Guarantees
**Throughput Guarantees:**
- Minimum: 1M ops/s pro 10-Node-Cluster (Standard-Hardware)
- Target: 10M ops/s pro 100-Node-Cluster
- Scale: Linear bis 10.000 Nodes (gemessen in Benchmarks)

**Latency Guarantees:**
- p50: < 0.5ms (Reads), < 2ms (Writes)
- p95: < 1ms (Reads), < 5ms (Writes)
- p99: < 2ms (Reads), < 10ms (Writes)

Bei Verfehlung (reproduzierbar):
- Kostenloses Performance-Tuning (bis 80h)
- Wenn nicht behebbar: Anteilige Rückerstattung (pro-rata)

---

## § 5 Dedicated Support Team

### 5.1 Technical Account Manager (TAM)
**Full-Time Dedicated TAM:**
- Verfügbarkeit: Mo-Fr, 8-18 Uhr (Timezone des Kunden)
- Direkter Kontakt: Handy, E-Mail, Slack/Teams
- Backup-TAM: Bei Urlaub/Krankheit (nahtloser Übergang)

**Verantwortlichkeiten:**
- Primary Point of Contact für alle Themen
- Koordination mit internen Teams (Engineering, Product, Sales)
- Eskalationsmanagement (proaktiv)
- Quartalsweise Business Reviews (Executive-Level)
- Jährliches Strategic Planning (On-Site möglich)

**Deliverables:**
- Monatlicher Status Report
- Quartalsweiser Health Check Report
- Jährlicher Architecture Review Report
- Custom Best Practices Guide (kundenspezifisch)

### 5.2 Solutions Architect (SA)
**On-Demand Zugang:**
- Abrufbar für Architecture Reviews
- Verfügbar für Design-Sessions
- Unterstützung bei Migration-Projekten
- Capacity Planning Workshops

**Inkludiert:**
- 40 Stunden pro Jahr (mehr gegen Aufpreis)
- Remote oder On-Site (Reisekosten separat)
- Dokumentation der Empfehlungen

### 5.3 Site Reliability Engineer (SRE)
**Shared SRE (Pool von 5 SREs):**
- Verfügbar für Incident Response (P0/P1)
- Runbook Development
- Chaos Engineering Guidance
- Disaster Recovery Drills (jährlich)

**On-Call Rotation:**
- 24/7/365 Erreichbarkeit
- Eskalation bei P0 (automatisch)
- Direkter Zugang zu Database Internals

### 5.4 Customer Success Manager (CSM)
**Quartalsweise Engagements:**
- Business Value Realization
- Adoption & Usage Analytics
- ROI Calculations
- Executive Reporting (C-Level)

**Strategic Alignment:**
- Roadmap Alignment (Kunde ↔ ThemisDB)
- Feature Request Priorisierung
- Expansion Planning (Additional Use Cases)

---

## § 6 Proactive Services und Monitoring

### 6.1 Health Monitoring (Opt-In)
**Continuous Monitoring:**
- Echtzeit-Überwachung von Metriken (Prometheus-basiert)
- Anomaly Detection (ML-basiert)
- Predictive Alerting (24h-48h Vorlauf)
- Capacity Forecasting

**Monitored Metrics:**
- CPU/Memory/Disk Utilization
- Query Latency (p50, p95, p99)
- Throughput (Reads, Writes)
- Replication Lag (bei Multi-Master)
- Connection Pool Saturation
- Index Efficiency
- Cache Hit Ratio

**Alerting:**
- Dienstleister wird automatisch alarmiert (bei Schwellwerten)
- Proaktive Kontaktaufnahme (vor Kunden-Impact)
- Empfehlungen zur Behebung

### 6.2 Proaktive Checks
**Weekly Automated Health Checks:**
- Configuration Validation (Best Practices)
- Security Posture Assessment (CVE-Scanning)
- Performance Baseline Comparison
- Capacity Headroom Analysis

**Quartalsweise Manual Reviews:**
- Deep-Dive Architecture Review (4h Session)
- Security Audit (Checklist-basiert)
- Performance Optimization Workshop
- Disaster Recovery Plan Review

### 6.3 Predictive Maintenance
**ML-basierte Vorhersagen:**
- Disk-Failure-Prognose (basierend auf SMART-Daten)
- Capacity Exhaustion Forecast (3-6 Monate)
- Performance Degradation Trends
- Security Vulnerability Likelihood

**Proaktive Maßnahmen:**
- Empfehlungen vor Problemen
- Automatische Ticket-Erstellung (Low-Priority)
- Eskalation bei kritischen Vorhersagen

---

## § 7 Wartungs- und Support-Gebühren (Hyperscaler)

### 7.1 Premium-Wartungsgebühr
Die Premium-Wartungsgebühr beträgt 25% der Jahres-Lizenzgebühr (vs. 20% bei Enterprise).

**Enthält:**
- Alle Enterprise-Support-Leistungen
- + Dedicated TAM (fulltime)
- + Solutions Architect (40h/Jahr)
- + Shared SRE-Pool
- + CSM (quartalsweise)
- + Proactive Monitoring (Opt-In)
- + 200h Professional Services Credits

### 7.2 Erste Jahr
- Premium-Support ist im ersten Jahr inkludiert (keine Extra-Kosten)
- Alle 200h Professional Services Credits verfügbar

### 7.3 Folge-Jahre
- Automatische Verlängerung mit Lizenzvertrag
- Jährliche Zahlung (Vorauszahlung bevorzugt)
- Monatliche Zahlung möglich (+ 5% Aufpreis)

### 7.4 Multi-Year Commitment
- 2 Jahre: 8% Rabatt
- 3 Jahre: 15% Rabatt
- 5 Jahre: 25% Rabatt

### 7.5 Add-On Services (Optional)
**Dedicated SRE (Full-Time):**
- On-Site oder Remote
- Teil des Kundenteams
- Preis: € 220.000 p.a.

**24/7 On-Call Engineer (Dedicated):**
- Nur für diesen Kunden zuständig
- Reaktionszeit < 5 Minuten (garantiert)
- Preis: € 180.000 p.a.

**Managed Service Upgrade:**
- Dienstleister übernimmt Operations komplett
- Inkl. Patches, Backups, Monitoring, Incident Response
- Preis: 50% der Lizenzgebühr p.a.

---

## § 8 Custom Development und Feature Prioritization

### 8.1 Inkludierte Professional Services (200h)
Verwendbar für:
- Custom Feature Development
- Integration mit proprietären Systemen
- Performance Tuning (Deep-Dive)
- Migration von anderen Datenbanken
- Training (Advanced Topics)
- Architecture Consulting

### 8.2 Feature Prioritization
Hyperscaler-Kunden erhalten:
- **Top 5 Feature Requests** pro Jahr werden priorisiert
- Aufnahme in nächstes Major/Minor Release (wenn technisch möglich)
- Direktes Feedback von Product Management
- Design-Partner-Status für neue Features (Beta-Zugang)

### 8.3 Custom Module Development
**Pricing:**
- Simple Module: € 25.000 - € 50.000
- Complex Module: € 50.000 - € 150.000
- Full New Subsystem: € 150.000+

**Intellectual Property:**
- Generischer Code: Bleibt bei Dienstleister (wird später Open-Source)
- Kundenspezifischer Code: Gehört dem Kunden (mit Source-Code-Zugang)
- Shared IP: Co-Development-Agreement erforderlich

### 8.4 Hotfix Development
Bei kritischen Kunden-spezifischen Bugs:
- Hotfix-Development innerhalb 24h (P0), 3 Tage (P1)
- Kostenlos, solange Wartungsvertrag aktiv
- Private Hotfix (nicht im Public Release, wenn kundenspezifisch)

---

## § 9 Training, Enablement und Knowledge Transfer

### 9.1 Inkludierte Schulungen
**Onboarding Training (5 Tage):**
- Architecture Deep-Dive
- Installation & Configuration (Hyperscaler-Setup)
- High-Availability & Disaster Recovery
- Performance Tuning & Optimization
- Security Best Practices
- Bis zu 10 Teilnehmer

**Advanced Training (3 Tage):**
- Distributed Systems Internals
- Kubernetes Operator Management
- Multi-Region Deployment
- Chaos Engineering
- Bis zu 10 Teilnehmer

### 9.2 Custom Training
- On-Site Training (vor Ort beim Kunden)
- Custom Curriculum (auf Bedürfnisse zugeschnitten)
- Hands-On Labs (mit Kundendaten, anonymisiert)
- Preis: Inkludiert (bis 40h), danach € 2.000/Tag

### 9.3 Knowledge Transfer Sessions
- Monatliche Webinare (exklusiv für Hyperscaler-Kunden)
- Themen: New Features, Best Practices, Case Studies
- Q&A mit Engineering-Team
- Aufzeichnungen verfügbar

### 9.4 Documentation und Runbooks
- Custom Runbooks für Kundenumgebung
- Incident Response Playbooks
- Disaster Recovery Procedures
- Architecture Decision Records (ADRs)

---

## § 10 Disaster Recovery und Business Continuity

### 10.1 Disaster Recovery Support
**Inkludierte DR-Services:**
- Annual DR Plan Review (4h Workshop)
- Disaster Recovery Testing (jährlich, 8h)
- Failover/Failback Assistance (24/7)
- Post-Disaster Root-Cause-Analysis

**Recovery Objectives (Guaranteed):**
- **RPO:** < 1 Minute (Multi-AZ), < 5 Minuten (Multi-Region)
- **RTO:** < 15 Minuten (Automated Failover), < 30 Minuten (Manual)

### 10.2 Business Continuity Plan
**Dienstleister-seitig:**
- Redundante Engineering-Teams (EU, US, APAC)
- Multi-Cloud Development Infrastructure
- 24/7 Follow-the-Sun Support
- Emergency Communication Protocols (Satellite-Phone)

**Kunde Support:**
- DR-Runbook Development
- Tabletop Exercises (quartalsweise)
- Chaos Engineering Workshops
- War-Gaming Sessions (jährlich)

### 10.3 Incident Management
**Major Incident Protocol:**
- War Room (Dedicated Slack Channel + Bridge Call)
- Incident Commander (aus dem SRE-Team)
- Echtzeit-Updates (alle 15 Minuten bei P0)
- Post-Incident Review (innerhalb 48h)
- Lessons-Learned Documentation

---

## § 11 Security, Compliance und Auditing

### 11.1 Security Support
**Proactive Security:**
- Monthly Security Scans (Kunde-Environment, Opt-In)
- Vulnerability Assessments (Quarterly)
- Penetration Testing (Annual, kostenfrei)
- Security Advisories (Immediate Notification)

**Incident Response:**
- 24/7 Security Hotline
- Immediate Patching (< 4h für Critical CVEs)
- Forensic Analysis Support
- Breach Notification Assistance

### 11.2 Compliance Support
**Audit Assistance:**
- Support bei externen Audits (ISO, SOC2, etc.)
- Bereitstellung von Compliance-Dokumentation
- Technical Q&A mit Auditoren
- Inkludiert: 40h/Jahr

**Zertifizierungen:**
- ISO 27001, ISO 9001, SOC 2 Type II
- TISAX (Automotive)
- FedRAMP (US Government, in Vorbereitung)
- BSI C5 (Cloud Computing)

### 11.3 Customer Audits
- Jährlich 2 Audits kostenfrei (vs. 1 bei Enterprise)
- Vor Ort oder Remote
- Zugang zu Source Code (Enterprise-Module, unter NDA)
- Zusätzliche Audits: € 3.000/Tag (vs. € 5.000 bei Enterprise)

---

## § 12 Berichterstattung und Governance

### 12.1 Executive Reporting
**Quartalsweise Executive Summary:**
- Business Value Realization
- System Health & Performance
- Support Activity Summary
- Roadmap Alignment
- Strategic Recommendations

**Jährliches Executive Briefing:**
- C-Level Presentation (on-site möglich)
- ROI Analysis
- Strategic Outlook
- Partnership Review

### 12.2 Technical Reporting
**Monatliche Reports:**
- System Health Scorecard
- Performance Metrics (Trends)
- Support Ticket Analysis
- Incident Summary
- Upcoming Releases

**Post-Incident Reports:**
- Innerhalb 48h nach P0/P1-Resolution
- Root Cause Analysis
- Timeline Reconstruction
- Lessons Learned
- Preventive Measures

### 12.3 Governance Meetings
**Steering Committee (Quartalsweise):**
- Executive Alignment
- Strategic Initiatives Review
- Budget Planning
- Escalation Review
- Partnership Health

**Technical Review Board (Monatlich):**
- Architecture Evolution
- Feature Roadmap
- Technical Debt Management
- Performance Optimization
- Security Posture

---

## § 13 SLA-Ausnahmen und Force Majeure

### 13.1 Ausnahmen von SLAs
SLAs gelten nicht bei:
- Angekündigten Wartungsfenstern (max. 2h/Quartal)
- Kundenseitigen Infrastruktur-Problemen
- Netzwerk-Issues außerhalb der Software-Kontrolle
- Nicht unterstützen Konfigurationen (nach Warnung)
- Veralteten Versionen (> 2 Major-Versionen)

### 13.2 Force Majeure (Erweitert)
- Naturkatastrophen, Kriege, Terrorismus
- Pandemien, staatliche Notstandsmaßnahmen
- Großflächige Infrastruktur-Ausfälle (Internet-Backbones)
- Cyberangriffe auf kritische Infrastruktur (staatlich)
- Halbleiter-/Hardware-Knappheit (globale Supply Chain)

Bei Force Majeure:
- Aussetzung der SLA-Verpflichtungen
- Keine Penalties oder Credits
- Verlängerung der Fristen
- Außerordentliches Kündigungsrecht (ab 6 Monate)

---

## § 14 Kündigung und Transition

### 14.1 Kündigungsbedingungen
- Kündigungsfrist: 6 Monate (wie Hauptvertrag)
- Keine separate Kündigung (nur mit Lizenzvertrag)

### 14.2 Transition Support (bei Vertragsende)
**90-Tage Grace Period:**
- Fortgesetzter Support (reduziert auf Business Hours)
- Zugang zu Updates und Patches
- Dokumentations-Zugang
- Daten-Export-Unterstützung

**Knowledge Transfer:**
- Dokumentation aller Custom-Entwicklungen
- Übergabe von Runbooks
- Training für internes Team (40h, kostenfrei)
- 6 Monate E-Mail-Support (Best Effort, kostenfrei)

### 14.3 Data Migration Assistance
- Export-Skripte für gängige Zielsysteme
- Daten-Validierung
- Consultation (20h kostenfrei)
- On-Site Assistance (optional, € 2.000/Tag)

---

## § 15 Haftung und Versicherung (Hyperscaler)

### 15.1 Erweiterte Haftung
Im Gegensatz zu Enterprise:
- **Vermögensschäden:** Bis 300% der Jahres-Wartungsgebühr (vs. 200%)
- **Datenverlust:** Bis € 2.000.000 (bei grober Fahrlässigkeit)
- **Betriebsunterbrechung:** Bis 100% der Jahres-Wartungsgebühr

### 15.2 Versicherungsschutz
- **Cyber-Insurance:** € 25M Deckung
- **Professional Indemnity:** € 10M Deckung
- **Nachweise:** Auf Anfrage bereitgestellt

### 15.3 Indemnification
- Vollständige Freistellung bei IP-Klagen
- Übernahme aller Rechtskosten (unbegrenzt)
- Proaktive Defense (nicht nur Kostenerstattung)

---

## § 16 Sondervereinbarungen und Add-Ons

### 16.1 Concierge Service
- Dedizierter Concierge für administrative Aufgaben
- Hilfe bei Lizenz-Management, Rechnungen, Verträgen
- Koordination zwischen verschiedenen Teams
- Inkludiert (kein Aufpreis)

### 16.2 Executive Briefing Center Visits
- Einladung zu ThemisDB Headquarters (1x jährlich)
- Meeting mit C-Level und Product Team
- Roadmap Preview (6-12 Monate voraus)
- Reisekosten: 50% übernommen durch Dienstleister

### 16.3 Industry Events
- VIP-Tickets zu ThemisDB User Conference
- Speaking Opportunities (optional)
- Private Networking Events
- Co-Presenting Case Studies

---

## § 17 Schlussbestimmungen

### 17.1 Anwendbares Recht und Gerichtsstand
- Recht der Bundesrepublik Deutschland
- Gerichtsstand: Hamburg (bei B2B)
- Internationale Streitigkeiten > € 1M: ICC Arbitration (Zürich)

### 17.2 Gesamtvereinbarung
- Dieser Vertrag ergänzt den Hyperscaler-Lizenzvertrag
- Bei Widersprüchen: Wartungsvertrag hat Vorrang (nur Wartungsthemen)

### 17.3 Vertraulichkeit
- Beide Parteien behandeln alle Support-Informationen vertraulich
- Verpflichtung gilt 10 Jahre nach Vertragsende (vs. 5 Jahre bei Enterprise)

### 17.4 Änderungen
- Schriftform erforderlich (inkl. qualifizierte E-Signatur)
- Keine mündlichen Nebenabreden

---

## Anlagen

1. **Anlage 1:** Support-Kontakte und Escalation-Matrix (24/7)
2. **Anlage 2:** Named Contacts (TAM, SA, SRE, CSM)
3. **Anlage 3:** Fehlerklassifizierung (Detailliert)
4. **Anlage 4:** Monitoring und Alerting (Opt-In Details)
5. **Anlage 5:** Data Processing Agreement (DSGVO/GDPR)
6. **Anlage 6:** Security and Compliance Framework
7. **Anlage 7:** Disaster Recovery Playbook
8. **Anlage 8:** Professional Services Katalog (200h Credits)
9. **Anlage 9:** Training Curriculum
10. **Anlage 10:** SLA-Metriken und Reporting

---

## Unterschriften

**Dienstleister:**  
ThemisDB GmbH

_______________________  
Ort, Datum

_______________________  
Unterschrift (Geschäftsführer)

_______________________  
Name, Funktion

**Kunde:**  
[Firmenname]

_______________________  
Ort, Datum

_______________________  
Unterschrift

_______________________  
Name, Funktion

---

**Vertragsnummer:** MAINT-HYP-2026-[XXXX]  
**Erstellt am:** [Datum]  
**Version:** 1.0  
**Gültigkeit:** Parallel zum Hyperscaler-Lizenzvertrag

---

## Anhang: Service-Level-Matrix (Übersicht)

| Metrik | Enterprise | Hyperscaler |
|--------|------------|-------------|
| **Support-Verfügbarkeit** | Business Hours | Extended Hours + Named Contacts |
| **Uptime-Garantie** | 99.5% | 99.9% (99.95% Multi-Region) |
| **P0 Reaktionszeit** | 4h | 2h |
| **P0 Workaround** | 24h | 12h |
| **TAM** | Ab 50 Nodes | Dedicated (Fulltime) |
| **Solutions Architect** | Auf Anfrage | 40h/Jahr inkludiert |
| **Professional Services** | - | 200h/Jahr inkludiert |
| **Wartungsgebühr** | 20% Lizenzgebühr | 25% Lizenzgebühr |
| **Max. SLA-Credits** | 50% Quartalsgebühr | 200% Monatsgebühr |

---

**Ende des Dokuments**
