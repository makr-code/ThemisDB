# Softwarevertrag - ThemisDB Hyperscaler Edition

**Stand:** April 2026  
**Version:** 1.0  
**Kategorie:** 🌐 Hyperscaler Legal

---

## Präambel

Dieser Softwarevertrag ("Vertrag") regelt die Nutzung der ThemisDB Hyperscaler Edition für großskalige, unternehmenskritische Deployments zwischen:

**Lizenzgeber:**  
ThemisDB GmbH  
[Musterstraße 123]  
[12345 Musterstadt]  
Handelsregister: HRB [XXXXX]  
Geschäftsführer: [Name]

**und**

**Lizenznehmer:**  
[Firmenname des Kunden]  
[Adresse des Kunden]  
[PLZ, Ort]  
Vertreten durch: [Name, Funktion]

---

## § 1 Vertragsgegenstand

### 1.1 Software
Der Lizenzgeber überlässt dem Lizenznehmer die ThemisDB Hyperscaler Edition, eine für hyperscale-Deployments optimierte Multi-Model-Datenbank mit unbegrenzter Skalierbarkeit und Enterprise-Grade-Features.

### 1.2 Lizenzumfang
Die Hyperscaler Edition umfasst alle Features der Enterprise Edition plus:

**Erweiterte Skalierung:**
- Unbegrenzte Anzahl an Nodes
- Unbegrenzte Anzahl an Shards pro Node
- Multi-Region Deployment Support
- Cross-Datacenter Replication
- Global Distribution mit Geo-Routing

**Orchestrierung & Automation:**
- Kubernetes Operator (vollständig zertifiziert)
- Auto-Scaling (horizontal und vertikal)
- Self-Healing Capabilities
- Automated Failover & Recovery
- Dynamic Rebalancing

**Performance & Optimization:**
- Advanced Multi-GPU Support (GPU-Cluster)
- NUMA-Aware Memory Management
- Custom Storage Engines (auf Anfrage)
- Hardware-Accelerated Encryption
- Zero-Copy Networking (RDMA, DPDK)

**Management & Observability:**
- Multi-Tenant Management Console
- Advanced Analytics Dashboard
- Predictive Alerting (AI-basiert)
- Cost Optimization Tools
- Capacity Planning Suite

### 1.3 Professional Services (Inkludiert)
- Dedicated Solutions Architect (full-time)
- Architecture Design Review
- Performance Engineering
- Disaster Recovery Planning
- Quarterly Business Reviews

### 1.4 Source Code Access
- Vollständiger Zugriff auf Enterprise-Module (unter NDA)
- Zugriff auf interne APIs
- Commit-Rechte für kritische Patches
- Early Access zu neuen Features (Beta-Programm)

---

## § 2 Lizenzmodell und Lizenzmetriken

### 2.1 Unbegrenzte Skalierung
Die Hyperscaler Edition hat keine Node-Limitierung:
- Unbegrenzte Anzahl an Production-Nodes
- Unbegrenzte Anzahl an Dev/Test/Staging-Environments
- Weltweite Deployments ohne geografische Einschränkungen

### 2.2 Deployment-Szenarien
Alle Deployment-Szenarien sind abgedeckt:
- **On-Premises:** Eigene Rechenzentren
- **Private Cloud:** AWS, Azure, GCP, Oracle Cloud
- **Hybrid Cloud:** Multi-Cloud-Strategien
- **Edge Computing:** CDN-nahe Deployments
- **Sovereign Cloud:** Nationale Cloud-Infrastrukturen

### 2.3 Managed Service Provider (MSP) Option
Der Kunde kann ThemisDB als Managed Service anbieten:
- Zu beliebig vielen Endkunden
- White-Label oder Co-Branding
- Separate MSP-Addendum erforderlich
- Revenue-Share Model oder Fixed Fee

### 2.4 OEM/Embedding Rechte
Embedding in eigene Produkte möglich:
- Redistribution als Teil größerer Lösungen
- API-Reselling
- Separate OEM-Lizenz erforderlich

---

## § 3 Lizenzgebühren und Zahlungsbedingungen

### 3.1 Lizenzgebühr (Subscription-Modell)
Die Lizenzgebühr wird individuell verhandelt basierend auf:
- Geplante Anzahl Nodes (Initial + Wachstum)
- Geografische Distribution (Single-Region vs. Multi-Region)
- Redundanz-Level (RAID-Konfiguration)
- GPU-Nutzung (Anzahl GPUs)
- Support-Level (Standard vs. Premium)

**Beispiel-Preismodelle:**
- **Per-Node Pricing:** € [XXX] pro Node/Monat
- **Capacity-Based Pricing:** € [XXX] pro TB/Monat
- **Flat-Rate Enterprise:** € [XXX.XXX] p.a. (unbegrenzt)
- **Hybrid Model:** Kombination aus den obigen

### 3.2 Volume Discounts
Rabattstaffeln bei größeren Deployments:
- 101-500 Nodes: 30% Rabatt
- 501-1000 Nodes: 40% Rabatt
- 1001-5000 Nodes: 50% Rabatt
- > 5000 Nodes: Individuelles Pricing

### 3.3 Multi-Year Discounts
- 2 Jahre Vorauszahlung: 15% Rabatt
- 3 Jahre Vorauszahlung: 25% Rabatt
- 5 Jahre Vorauszahlung: 35% Rabatt

### 3.4 Support & Professional Services
**Support-Gebühr:**
- Premium Support (24/7 + Dedicated Team): 25% der Lizenzgebühr
- Im ersten Jahr inkludiert
- Ab zweitem Jahr optional abwählbar (Downgrade zu Standard Support)

**Professional Services Credits:**
- Jährlich inkludiert: 200 Stunden (€ 50.000 Wert)
- Verwendbar für Custom Development, Training, Consulting
- Übertrag ins Folgejahr: 50% der nicht genutzten Stunden

### 3.5 Zahlungsbedingungen
- Zahlungsziel: 30 Tage netto (bei Jahresabrechnung)
- Alternativ: Monatliche Abrechnung (Zahlungsziel 14 Tage)
- Akzeptierte Zahlungsmethoden: Überweisung, SEPA-Lastschrift
- Rechnungsstellung: Ende des Abrechnungszeitraums

### 3.6 True-Up Mechanismus
Bei dynamischer Skalierung:
- Quartalsweise Überprüfung der tatsächlichen Node-Nutzung
- Nachberechnung bei Überschreitung um > 20%
- Gutschrift bei dauerhafter Unterschreitung um > 30%
- True-Up Window: 30 Tage nach Quartalsende

---

## § 4 Vertragslaufzeit und Kündigung

### 4.1 Laufzeit
- Erstlaufzeit: 36 Monate (Mindestlaufzeit)
- Automatische Verlängerung um jeweils 12 Monate
- Empfohlen: 5-Jahres-Vertrag (höchste Rabatte)

### 4.2 Ordentliche Kündigung
- Kündigungsfrist: 6 Monate zum Vertragsende
- Schriftform erforderlich (inkl. qualifizierte E-Mail-Signatur)
- Keine Rückerstattung bereits gezahlter Gebühren
- Datenexport-Unterstützung (90 Tage nach Vertragsende)

### 4.3 Außerordentliche Kündigung
Kündigungsgründe:
- **Lizenzgeber:** Zahlungsverzug > 60 Tage, schwere Vertragsverletzung
- **Lizenznehmer:** Wesentliche SLA-Unterschreitungen (> 3 Monate), Insolvenz Lizenzgeber
- **Beide:** Force Majeure > 6 Monate

### 4.4 Folgen der Kündigung
- **Grace Period:** 90 Tage für Datenmigration
- **Feature Downgrade:** Nach Grace Period auf Community Edition
- **Support-Ende:** Mit Vertragsende (außer Extended Support gebucht)
- **Daten-Löschung:** Kunde ist selbst verantwortlich
- **Zertifikat-Widerruf:** Nach Vertragsende

### 4.5 Downgrade Option
Downgrade zu Enterprise Edition möglich:
- Mit 6 Monaten Vorlauf
- Node-Limitierung greift zum Downgrade-Datum
- Erstattung anteiliger Differenz (pro-rata)
- Hyperscaler-spezifische Features werden deaktiviert

---

## § 5 Nutzungsrechte und Beschränkungen

### 5.1 Gewährte Rechte
Der Lizenznehmer erhält ein:
- Nicht-exklusives
- Übertragbares (innerhalb des Konzerns, siehe § 5.5)
- Weltweites
- Zeitlich auf Vertragsdauer beschränktes
Nutzungsrecht an der Software

### 5.2 Erweiterte Nutzungsrechte (Hyperscaler)
Zusätzlich zur Enterprise Edition:
- **Managed Service Provision:** Bereitstellung als DBaaS
- **Multi-Tenancy:** Isolation verschiedener Kunden auf gleicher Infrastruktur
- **API Reselling:** Anbieten der ThemisDB-API als Service
- **White-Labeling:** Eigenes Branding (mit Akkreditierung)
- **Custom Extensions:** Entwicklung proprietärer Plugins

### 5.3 Eingeschränkte Nutzung
Nicht gestattet ohne separates Addendum:
- **OEM Distribution:** Weiterverkauf als eigenständiges Produkt
- **Competitive Use:** Nutzung zum Aufbau konkurrierender DB-Produkte
- **Benchmarking-Publikation:** Ohne schriftliche Genehmigung
- **Reverse Engineering:** Der Core Engine (Enterprise-Module erlaubt unter NDA)

### 5.4 Source Code Modifikationen
Der Kunde darf:
- Enterprise-Module anpassen (unter NDA)
- Eigene Module entwickeln (Plugin-Interface)
- Patches für Bugfixes erstellen
- Upstream-Contributions einreichen (CLA erforderlich)

Nicht gestattet:
- Forking der Core Engine
- Entfernung von Lizenz-Checks
- Redistribution modifizierter Binaries

### 5.5 Konzern- und Sublizenzierung
**Konzernklausel:**
- Weitergabe innerhalb des Konzerns (§ 18 AktG) kostenfrei
- Registrierung der Tochtergesellschaften erforderlich
- Node-Limits gelten konzernweit

**Sublizenzierung:**
- An Partner oder Kunden (bei MSP-Modell)
- Mit schriftlicher Genehmigung
- Kunde haftet für Vertragseinhaltung durch Sublizenznehmer

---

## § 6 Lieferung und Bereitstellung

### 6.1 Lieferumfang
Der Lizenzgeber stellt bereit:
- Alle Enterprise-Module + Hyperscaler-Extensions
- Kubernetes Operator (Helm Charts)
- Infrastructure-as-Code Templates (Terraform, Ansible)
- Container Images (Docker, Podman)
- VM Images (OVA, QCOW2, VHD)
- Bare-Metal Installers (ISO)

### 6.2 Lieferweg
- **Enterprise Download Portal:** HTTPS mit 2FA
- **Private Container Registry:** Harbor oder GitLab Registry
- **Artifact Repository:** Nexus oder Artifactory
- **Direktlieferung:** Verschlüsselte USB-Drives (auf Anfrage)

### 6.3 Installation & Deployment Support
**Inkludierte Services:**
- Remote Installation (bis zu 3 Deployments)
- Architecture Review (vor Go-Live)
- Performance Tuning (Post-Deployment)
- Runbook Erstellung

**On-Site Services (optional):**
- Vor-Ort-Installation durch Deployment-Team
- Training und Knowledge Transfer
- Kosten: € 2.500/Tag + Reisekosten

### 6.4 Lieferfrist
- Standard-Lieferung: Sofort (Download)
- Customized Build: 10 Werktage
- On-Site Deployment: 30 Tage (nach Terminvereinbarung)

---

## § 7 Updates, Upgrades und Roadmap

### 7.1 Continuous Updates
Im Hyperscaler-Support enthalten:
- **Patch-Releases:** Wöchentlich (Security) oder bei Bedarf
- **Minor-Releases:** Monatlich (Features)
- **Major-Releases:** Halbjährlich (neue Versionen)

### 7.2 Release Channels
Der Kunde kann wählen:
- **Stable Channel:** Production-ready, getestet (empfohlen)
- **Fast Channel:** Early Access, neue Features (Staging)
- **LTS Channel:** Long-Term Support, nur Security Updates

### 7.3 Zero-Downtime Updates
- Rolling Updates über Kubernetes
- Blue-Green Deployments
- Canary Releases
- Automated Rollback bei Fehlern

### 7.4 Roadmap-Einfluss
Hyperscaler-Kunden erhalten:
- Zugang zu interner Roadmap
- Priorisierung von Feature-Requests (Top 3 p.a.)
- Early Access zu Beta-Features
- Design-Partner-Status für neue Module

### 7.5 Long-Term Support (LTS)
- LTS-Versionen: 5 Jahre Support
- Erscheinungsrhythmus: Alle 2 Jahre
- Nur Security- und Critical-Bugfixes (keine neuen Features)
- Kostenfrei im Rahmen des Hauptvertrags

---

## § 8 Gewährleistung und Haftung

### 8.1 Erweiterte Gewährleistung (Hyperscaler)
Der Lizenzgeber garantiert zusätzlich:
- **Zero-Data-Loss:** Bei HA-Setup (RAID5+)
- **Performance-Garantie:** Min. 95% der Benchmark-Werte
- **Skalierbarkeits-Garantie:** Linear bis 1000 Nodes
- **Kompatibilitäts-Garantie:** Backwards Compatibility (2 Major-Versionen)

### 8.2 Gewährleistungsfrist
- 24 Monate ab Lieferung
- Verlängerung auf 60 Monate bei LTS-Versionen
- Kein Zeitlimit bei Sicherheitslücken (Responsible Disclosure)

### 8.3 Accelerated Bugfixing (SLA)
| Priorität | Reaktionszeit | Workaround | Fix-Bereitstellung |
|-----------|---------------|------------|-------------------|
| **P0 - Kritisch** | 2 Stunden | 12 Stunden | 3 Werktage |
| **P1 - Hoch** | 4 Stunden | 1 Werktag | 1 Woche |
| **P2 - Mittel** | 1 Werktag | 3 Werktage | 2 Wochen |
| **P3 - Niedrig** | 2 Werktage | Best Effort | 1 Monat |

### 8.4 Erweiterte Haftung (Hyperscaler)
Im Gegensatz zur Enterprise Edition:
- **Vermögensschäden:** Haftung bis zu 200% der Jahres-Lizenzgebühr
- **Datenverlust:** Haftung bis € 1.000.000 (bei erwiesener grober Fahrlässigkeit)
- **Betriebsunterbrechung:** Bis zu 50% der Jahres-Lizenzgebühr

**Cyber-Versicherung:**
- Lizenzgeber unterhält Cyber-Versicherung (€ 10M Deckung)
- Deckung umfasst auch Kundenschäden
- Nachweis auf Anfrage

### 8.5 Service Credits
Bei SLA-Verletzungen (siehe § 9):
- Automatische Gutschriften
- Wahlweise: Gelderstattung oder Service-Credits
- Service Credits verwendbar für Professional Services

### 8.6 Indemnification (Freistellung)
**Schutzrechte:**
- Lizenzgeber stellt Kunde vollständig frei bei IP-Klagen
- Übernahme aller Rechtskosten
- Vergleichsverhandlungen in Abstimmung mit Kunde

**Datenschutz:**
- Freistellung bei DSGVO-Verstößen durch Software-Defekte
- Nicht bei Fehlkonfiguration durch Kunden

---

## § 9 Service Level Agreement (SLA) - Hyperscaler

### 9.1 Infrastructure Uptime
**Software-Garantie:**
- 99.99% Verfügbarkeit (entspricht ~52 Minuten Downtime/Jahr)
- Messung: Server-Verfügbarkeit, nicht Netzwerk
- Ausnahmen: Geplante Wartung (max. 4h/Jahr, angekündigt 14 Tage vorher)

**Multi-Region Setup:**
- 99.999% Verfügbarkeit ("Five-Nines") bei >= 3 Regionen
- Entspricht ~5 Minuten Downtime/Jahr

### 9.2 Support-Verfügbarkeit
**24/7/365 Premium Support:**
- Telefon, E-Mail, Ticket-System, Chat
- Response-Times wie in § 8.3
- Dedicated Slack/Teams Channel
- Direktkontakt zu Entwicklungs-Team bei P0

### 9.3 Dedicated Support Team
Hyperscaler-Kunden erhalten:
- **Technical Account Manager (TAM):** 1 dedizierter TAM
- **Solutions Architect:** Zugriff auf Architektur-Team
- **Customer Success Manager (CSM):** Quartalsweise Business Reviews
- **Escalation Manager:** Direktkontakt zu VP Engineering

### 9.4 Proactive Support
- **Health Checks:** Quartalsweise automatisierte Checks
- **Performance Monitoring:** Alerting bei Anomalien
- **Security Scanning:** Monatliche Vulnerability Scans
- **Capacity Planning:** Wachstumsprognosen und Empfehlungen

### 9.5 Response und Resolution Times

| Severity | Description | Response | Workaround | Resolution |
|----------|-------------|----------|------------|-----------|
| **P0 - Site Down** | Complete service outage | 2 hours | 12 hours | 3 days |
| **P1 - Critical** | Core functionality impaired | 4 hours | 1 day | 1 week |
| **P2 - Major** | Significant impact | 1 day | 3 days | 2 weeks |
| **P3 - Minor** | Limited impact | 2 days | 1 week | 1 month |
| **P4 - Cosmetic** | No business impact | 1 business day | Best effort | Next release |

### 9.6 SLA Credits (Detailed)

**Verfügbarkeits-Credits:**
| Verfügbarkeit | Gutschrift (% der Monatsgebühr) |
|---------------|--------------------------------|
| 99.99% - 99.95% | 10% |
| 99.95% - 99.90% | 25% |
| 99.90% - 99.00% | 50% |
| < 99.00% | 100% |

**Support-SLA-Credits:**
| SLA-Verletzung | Gutschrift |
|----------------|-----------|
| P0 Response > 4h | 5% |
| P0 Resolution > 1 Woche | 15% |
| P1 Response > 8h | 2% |
| P1 Resolution > 2 Wochen | 10% |

**Maximum Credits:** 100% der Quartalslizenzgebühr

### 9.7 Performance Guarantees
- **Read Latency:** p99 < 2ms (bei korrekter Konfiguration)
- **Write Latency:** p99 < 10ms (bei korrekter Konfiguration)
- **Throughput:** Min. 1M ops/s pro 10-Node-Cluster
- **Skalierung:** Linear bis 1000 Nodes (Benchmark-verifiziert)

Unterschreitungen:
- Bei reproduzierbarem Performance-Defekt
- Gemeinsames Performance-Tuning (kostenfrei)
- Falls nicht behebbar: Anteilige Rückerstattung

---

## § 10 Professional Services & Custom Development

### 10.1 Inkludierte Services (200h p.a.)
**Architecture & Consulting:**
- Reference Architecture Design
- Disaster Recovery Planning
- Security Architecture Review
- Performance Engineering
- Capacity Planning

**Training & Enablement:**
- Advanced Admin Training (5 Tage)
- Developer Bootcamp (3 Tage)
- Operations Workshop (2 Tage)
- Bis zu 20 Teilnehmer

**Migration Support:**
- Migration Planning & Strategy
- Data Migration Support (bis 10 TB)
- Application Refactoring Guidance
- Post-Migration Optimization

### 10.2 Zusätzliche Services (gegen Aufpreis)
**Stundensätze:**
- Junior Engineer: € 150/h
- Senior Engineer: € 250/h
- Principal Architect: € 350/h
- CTO Consulting: € 500/h

**Pakete:**
- **Extended Migration** (> 10 TB): € 25.000
- **Custom Module Development:** Ab € 50.000
- **Dedicated DevOps Engineer:** € 180k p.a.
- **On-Site Resident Engineer:** € 220k p.a.

### 10.3 Custom Development
**Feature Development:**
- Prioritized Feature Requests (Top 3)
- Custom Modules/Plugins
- Integration mit proprietären Systemen
- Performance-Optimierungen

**Intellectual Property:**
- Generischer Code: Bleibt bei Lizenzgeber (wird Open Source)
- Kundenspezifischer Code: Gehört dem Kunden
- Shared IP: Co-Development Agreement erforderlich

### 10.4 Proof of Concept (PoC)
- 60 Tage (doppelt so lang wie Enterprise)
- Unbegrenzte Nodes und Shards
- Voller Support (P1/P2 nur Business Hours)
- Übergang zu Produktivlizenz ohne Neuinstallation

---

## § 11 Datenschutz, Compliance und Zertifizierungen

### 11.1 DSGVO & Datenschutz
- **Data Processing Agreement (DPA):** Inkludiert
- **Standard Contractual Clauses (SCC):** Bei EU-Nicht-EU-Transfer
- **Data Residency:** Konfigurierbar (EU, US, APAC, etc.)
- **Right to be Forgotten:** Automatisierte Löschfunktionen
- **Data Portability:** Export in standardisierten Formaten

### 11.2 Compliance-Zertifizierungen
Der Lizenzgeber verpflichtet sich:
- **ISO 27001:** Informationssicherheits-Management
- **ISO 9001:** Qualitätsmanagement
- **SOC 2 Type II:** Security, Availability, Confidentiality
- **TISAX:** Automotive-Sicherheit (auf Anfrage)

Kunden-Audits:
- Jährlich 1x kostenfrei (vor Ort oder remote)
- Zusätzliche Audits: € 5.000/Tag

### 11.3 Branchenspezifische Compliance
**Finanzsektor:**
- PCI DSS (Payment Card Industry)
- Basel III (Bankenregulierung)
- MiFID II (Märkte für Finanzinstrumente)

**Gesundheitswesen:**
- HIPAA (Health Insurance Portability)
- HITECH Act
- FDA 21 CFR Part 11 (elektronische Aufzeichnungen)

**Public Sector:**
- BSI C5 (Cloud Computing Compliance)
- FedRAMP (US Federal Risk Authorization)
- Sovereign Cloud Ready

### 11.4 Verschlüsselung & Key Management
- **At-Rest Encryption:** AES-256 (Standard)
- **In-Transit Encryption:** TLS 1.3
- **End-to-End Encryption:** Optional (mit Performance-Impact)
- **Key Management:** 
  - Built-in KMS (Key Management Service)
  - Integration mit AWS KMS, Azure Key Vault, HashiCorp Vault
  - HSM-Support (Hardware Security Module)

### 11.5 Audit Logging & Forensics
- **Comprehensive Audit Logs:** Alle Zugriffe und Änderungen
- **Tamper-Proof Logging:** Kryptografisch gesichert
- **SIEM Integration:** Splunk, QRadar, ArcSight
- **Retention:** Mindestens 7 Jahre (konfigurierbar)
- **Forensic Mode:** Read-only Snapshot für Untersuchungen

### 11.6 Telemetrie und Diagnostics (Opt-In)
Der Kunde kann optional aktivieren:
- **Performance Metrics:** Anonymisiert
- **Usage Analytics:** Aggregiert (keine personenbezogenen Daten)
- **Crash Dumps:** Nur bei P0/P1-Incidents (zeitlich begrenzt)

Alle Telemetrie-Daten:
- Verschlüsselt übertragen
- Ausschließlich in EU-Rechenzentren gespeichert
- Löschung auf Kundenwunsch innerhalb 24h

---

## § 12 Schutzrechte, Open Source und IP

### 12.1 Intellectual Property
- Alle Rechte an Core Engine und Enterprise-Modulen beim Lizenzgeber
- Hyperscaler-spezifische Extensions: Lizenzgeber
- Kundenspezifische Anpassungen: Kunde (siehe § 10.3)

### 12.2 Open Source Strategie
**Community Edition (Core):**
- MIT License mit Government Clause
- Öffentlich auf GitHub
- Community Contributions willkommen (CLA erforderlich)

**Enterprise & Hyperscaler Module:**
- Closed Source (kompilierte Binaries)
- Source Access unter NDA für Hyperscaler-Kunden
- Keine GPL/AGPL-Abhängigkeiten

**Third-Party Dependencies:**
- Vollständige Liste in ATTRIBUTIONS.md
- Alle Lizenzen: MIT, Apache 2.0, BSD (permissive)
- Keine Copyleft-Lizenzen in Enterprise-Komponenten

### 12.3 Patente und Schutzrechte
**Patent-Schutz:**
- Lizenzgeber hält mehrere Patente (US, EU) auf:
  - Hybrid Vector/Graph-Traversal Algorithmus
  - RAID-like Data Sharding Mechanism
  - GPU-Accelerated MVCC Implementation

**Patent-Grant:**
- Kunde erhält nicht-exklusive Patent-Lizenz
- Gilt für Vertragsdauer
- Defensive Patent-Klausel (keine Klagen gegen Lizenzgeber)

### 12.4 Trademark und Branding
**Verwendung der Marke "ThemisDB":**
- Erlaubt in Marketingmaterialien ("Powered by ThemisDB")
- Erlaubt in technischer Dokumentation
- White-Label möglich (mit Akkreditierung im Footer)

Nicht erlaubt:
- Täuschende Verwendung (z.B. "ThemisDB by [Kunde]")
- Markenanmeldungen mit "Themis" (konkurrierende Märkte)

### 12.5 Confidential Information
**Definition:**
- Source Code der Enterprise-Module
- Interne APIs und Protokolle
- Sicherheitslücken (vor Patch-Release)
- Pricing und Geschäftsbedingungen (NDA)

**Vertraulichkeitspflicht:**
- Dauer: 10 Jahre nach Vertragsende
- Ausnahmen: Public Domain, rechtliche Verpflichtung
- Strafen bei Verstoß: Vertragsstrafe bis € 500.000

---

## § 13 Disaster Recovery und Business Continuity

### 13.1 Disaster Recovery Support
**Inkludierte DR-Services:**
- DR-Plan Review (jährlich)
- RPO/RTO Target Definition
- Backup-Strategie Beratung
- Disaster Recovery Testing (jährlich 1x)

**Recovery Objectives:**
- **RPO (Recovery Point Objective):** < 1 Minute (bei RAID5+)
- **RTO (Recovery Time Objective):** < 15 Minuten (bei HA-Setup)

### 13.2 Backup und Restore
**Automated Backups:**
- Continuous Backups (CDP - Continuous Data Protection)
- Point-in-Time Recovery (PITR) bis zu 30 Tage zurück
- Cross-Region Backup Replication
- Encrypted Backups (AES-256)

**Backup Storage:**
- Kostenfrei: Bis zu 2x Datenbankgröße
- Zusätzlicher Storage: € 0.10/GB/Monat
- Retention: Konfigurierbar (7 Tage bis 7 Jahre)

### 13.3 High Availability (HA)
**Multi-Availability-Zone:**
- Synchronous Replication innerhalb einer Region
- Automatic Failover < 30 Sekunden
- Zero Data Loss (bei >= 3 Nodes)

**Multi-Region:**
- Asynchronous Replication zwischen Regionen
- Configurable Replication Lag (100ms - 5s)
- Manual or Automatic Failover

**Konflikt-Resolution:**
- Last-Write-Wins (LWW)
- CRDTs für spezielle Datentypen
- Custom Conflict Resolvers (Plugins)

### 13.4 Chaos Engineering Support
- Support für Chaos Monkey, Gremlin, Chaos Toolkit
- Failure Injection Testing (Lizenzgeber-begleitet)
- Game Days mit Solutions Architect (quartalsweise)

---

## § 14 Migration und Lock-In Prevention

### 14.1 Data Portability
Der Lizenzgeber garantiert:
- **Export-Formate:** JSON, CSV, Parquet, Avro, Protocol Buffers
- **Full Database Export:** Kompletter Datenbank-Dump
- **Incremental Exports:** Delta-Exports seit letztem Export
- **No Lock-In:** Kein proprietäres Format

### 14.2 API-Stabilität
- **API Compatibility:** Mindestens 2 Major-Versionen
- **Deprecation Policy:** Min. 12 Monate Ankündigung
- **Compatibility Layer:** Für Legacy-APIs (falls möglich)

### 14.3 Migration Support (Out-Migration)
Falls der Kunde zu anderer Lösung wechseln möchte:
- 90 Tage Extended Grace Period
- Migrations-Beratung (20h kostenfrei)
- Export-Skripte für gängige Zielsysteme (PostgreSQL, MongoDB, etc.)
- Daten-Validierung Post-Migration

### 14.4 Re-Import Guarantee
- Daten-Re-Import innerhalb 12 Monate kostenfrei
- Support bei Rückmigration (falls neues System nicht passt)

---

## § 15 Höhere Gewalt und Business Continuity

### 15.1 Force Majeure
Erweiterte Definition für Hyperscaler:
- Naturkatastrophen (Erdbeben, Fluten, etc.)
- Kriege, Terrorismus, Cyberangriffe (staatlich)
- Pandemien (wie COVID-19)
- Stromausfälle (> 24h, großflächig)
- Internet-Backbone-Ausfälle
- Regierungsbeschlüsse (Lockdowns, Export-Verbote)
- Halbleiter-Knappheit (GPUs, CPUs)

### 15.2 Business Continuity Plan
**Lizenzgeber-seitig:**
- Backup-Rechenzentrum für Development
- Redundante Team-Standorte (EU + USA + APAC)
- Cloud-basierte CI/CD (Multi-Cloud)
- Notfall-Support über Satellitentelefon

**Kunde-seitig:**
- Multi-Region Deployment empfohlen
- Disaster Recovery Testing (jährlich)
- Incident Response Playbooks

### 15.3 Kriegs- und Terrorklausel
Bei bewaffneten Konflikten:
- Notfall-Lizenz für Ausweich-Standorte
- Temporäre Exceeding Node-Limits (ohne Nachberechnung)
- Priorisierung bei Hardware-Engpässen

---

## § 16 Vertragliche Besonderheiten (Hyperscaler)

### 16.1 Strategic Partnership
Hyperscaler-Kunden werden Partner:
- Gemeinsame Case Studies und Whitepapers
- Speaking Engagements auf Konferenzen
- Logo-Platzierung auf Webseite (opt-in)
- Executive Briefings (CTO-Level)

### 16.2 Product Advisory Board
Teilnahme am Hyperscaler Advisory Board:
- Quartalsweise Meetings
- Roadmap-Einfluss
- Early Access zu Beta-Features
- Design-Partner für neue Module

### 16.3 Co-Innovation
Gemeinsame Entwicklung:
- Joint Development Agreements
- Shared IP (nach Vereinbarung)
- Co-Marketing Opportunities
- Integration Partnerships (z.B. mit Cloud-Providern)

### 16.4 Preferred Partner Benefits
- Fast-Track Support (Bypass L1)
- Dedicated Slack Channel mit Engineering
- Einladung zu Engineering Summits
- Rabatte auf zusätzliche Services (15%)

---

## § 17 Zahlungsausfall und Eskalation

### 17.1 Zahlungsverzug
Bei Zahlungsverzug:
- **30 Tage:** Zahlungserinnerung (kostenfrei)
- **60 Tage:** Mahnung mit Verzugszinsen (5% über Basiszins)
- **90 Tage:** Androhung der Lizenz-Sperrung
- **120 Tage:** Lizenz-Sperrung + außerordentliche Kündigung

### 17.2 Sicherheitsleistung
Bei Kunden mit erhöhtem Risiko:
- Bank-Garantie oder Letter of Credit
- Parent Company Guarantee
- Prepayment für 1 Jahr

### 17.3 Inkasso
Bei erfolgloser Mahnung:
- Übergabe an Inkasso-Dienstleister
- Kunde trägt alle Inkasso-Kosten
- Eintrag in Wirtschaftsauskunfteien

---

## § 18 Schlussbestimmungen

### 18.1 Gerichtsstand und anwendbares Recht
- **Gerichtsstand:** Hamburg (bei B2B)
- **Anwendbares Recht:** Deutsches Recht
- **Ausschluss:** UN-Kaufrecht (CISG)
- **Internationale Streitigkeiten:** ICC Arbitration (International Chamber of Commerce)

### 18.2 Schiedsgerichtsklausel
Bei Streitigkeiten > € 500.000:
- Obligatorisches Schiedsverfahren (ICC Rules)
- Sitz des Schiedsgerichts: Zürich, Schweiz
- Sprache: Deutsch oder Englisch (nach Wahl des Klägers)
- 3 Schiedsrichter (je 1 von Parteien benannt, 1 gemeinsam)

### 18.3 Salvatorische Klausel
- Unwirksamkeit einzelner Klauseln berührt Vertrag nicht
- Ersetzung durch gesetzliche Regelung oder ähnliche wirksame Klausel

### 18.4 Vertragsänderungen
- Schriftform erforderlich (inkl. qualifizierte E-Signatur)
- Addenda und Amendments müssen beidseitig signiert werden
- Keine mündlichen Nebenabreden

### 18.5 Vertragssprache
- Deutsch (diese Version)
- Englische Übersetzung verfügbar (bei Widerspruch: deutsch maßgeblich)

### 18.6 Gesamtvereinbarung
- Dieser Vertrag ersetzt alle vorherigen Vereinbarungen
- Alle Addenda sind integraler Bestandteil
- Keine weiteren Verpflichtungen außer schriftlich vereinbart

---

## § 19 Besondere Konditionen

### 19.1 Academic und Non-Profit
- 75% Rabatt für Universitäten und Forschungseinrichtungen
- 50% Rabatt für Non-Profit-Organisationen
- Nur für nicht-kommerzielle Nutzung

### 19.2 Government und Public Sector
- Spezielle Konditionen für Behörden
- Souveränitätsklausel (kein Cloud-Zwang)
- Multi-Year Budgeting möglich (3-5 Jahre)

### 19.3 Startup-Programm (Accelerated Growth)
- 90% Rabatt im ersten Jahr (bis 100 Nodes)
- 75% Rabatt im zweiten Jahr
- 50% Rabatt im dritten Jahr
- Voraussetzung: < 50 Mitarbeiter, < € 10M Funding

### 19.4 Cloud Service Provider (CSP) Partnership
Für AWS, Azure, GCP, Oracle Cloud:
- Revenue-Share-Modell (15% für Lizenzgeber)
- Co-Selling Agreement
- Marketplace-Listing (kostenfrei)
- Joint Go-to-Market

---

## Anlagen

1. **Anlage 1:** Lizenzschlüssel und Aktivierungsinformationen
2. **Anlage 2:** Systemanforderungen (Hyperscaler-spezifisch)
3. **Anlage 3:** Feature-Matrix (Enterprise vs. Hyperscaler)
4. **Anlage 4:** Service Level Agreement (Detailliert)
5. **Anlage 5:** Data Processing Agreement (DSGVO/GDPR)
6. **Anlage 6:** Professional Services Katalog
7. **Anlage 7:** Security Whitepaper
8. **Anlage 8:** Architecture Reference Designs
9. **Anlage 9:** Disaster Recovery Playbook
10. **Anlage 10:** Compliance-Zertifikate (ISO, SOC2, etc.)
11. **Anlage 11:** Non-Disclosure Agreement (NDA)
12. **Anlage 12:** Managed Service Provider (MSP) Addendum
13. **Anlage 13:** OEM/Reseller Addendum

---

## Unterschriften

**Lizenzgeber:**  
ThemisDB GmbH

_______________________  
Ort, Datum

_______________________  
Unterschrift (Geschäftsführer)

_______________________  
Name, Funktion

**Lizenznehmer:**  
[Firmenname]

_______________________  
Ort, Datum

_______________________  
Unterschrift

_______________________  
Name, Funktion

---

**Vertragsnummer:** HYP-2026-[XXXX]  
**Erstellt am:** [Datum]  
**Version:** 1.0  
**Gültigkeit:** [Startdatum] bis [Enddatum + Verlängerungen]

---

**Siegel/Stempel (optional):**

Lizenzgeber: _________________    Lizenznehmer: _________________
