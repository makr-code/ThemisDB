# Softwarevertrag - ThemisDB Enterprise Edition

**Stand:** April 2026  
**Version:** 1.0  
**Kategorie:** 🏢 Enterprise Legal

---

## Präambel

Dieser Softwarevertrag ("Vertrag") regelt die Nutzung der ThemisDB Enterprise Edition zwischen:

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
Der Lizenzgeber überlässt dem Lizenznehmer die ThemisDB Enterprise Edition, eine hochperformante Multi-Model-Datenbank mit nativer KI/LLM-Integration, zur vertragsgemäßen Nutzung.

### 1.2 Lizenzumfang
Die Enterprise Edition umfasst:
- Kern-Datenbank-Engine mit allen Community-Features
- 7 Enterprise-Module als separate DLLs/Shared Libraries:
  1. Sharding-Modul (horizontale Skalierung, 4-100 Nodes)
  2. GPU-Beschleunigung (CUDA, Vulkan, HIP, DirectX)
  3. Analytics-Modul (OLAP, CEP, Apache Arrow)
  4. Replikation (Leader-Follower, Multi-Master, CRDTs)
  5. Security-Modul (RBAC, HSM, Field-Level Encryption)
  6. Management-Modul (Multi-Tenancy, Rate Limiting, Admin Tools)
  7. Content-Processing (PDF, Video, Audio, Geo, CAD, Image)

### 1.3 Dokumentation
Der Lizenznehmer erhält Zugang zu:
- Technischer Dokumentation
- API-Referenz
- Best Practices Guides
- Architektur-Dokumentation
- Migration Guides

### 1.4 Source Code
Enterprise-Module werden als kompilierte Binaries ausgeliefert. Quellcode-Zugang ist unter separatem NDA verfügbar.

---

## § 2 Lizenzmodell und Lizenzmetriken

### 2.1 Node-basierte Lizenzierung
Die Lizenz berechtigt zur Installation auf der vertraglich vereinbarten Anzahl von Nodes:
- **Standard:** 4-100 Nodes
- **Erweitert:** Durch Zukauf erweiterbar

### 2.2 Deployment-Szenarien
Folgende Deployment-Szenarien sind abgedeckt:
- **On-Premises:** Eigene Rechenzentren
- **Hosted:** Dedicated Server bei Hosting-Providern
- **Private Cloud:** Eigene Cloud-Infrastruktur
- **Hybrid:** Kombination aus den obigen

### 2.3 Entwicklungs- und Testumgebungen
- Unbegrenzte Anzahl von Entwicklungs- und Testinstanzen
- Keine zusätzlichen Kosten für Staging-Umgebungen
- Production-Limitierungen gelten nicht für Test-Systeme

### 2.4 Disaster Recovery
- Hot-Standby-Systeme zählen nicht zur Node-Lizenz
- Cold-Standby-Systeme sind kostenfrei
- Failover-Szenarien sind abgedeckt

---

## § 3 Lizenzgebühren und Zahlungsbedingungen

### 3.1 Lizenzgebühr
Die jährliche Lizenzgebühr beträgt:
- **Basis-Lizenz (4-20 Nodes):** [€ XX.XXX] p.a.
- **Erweiterte Lizenz (21-50 Nodes):** [€ XX.XXX] p.a.
- **Premium-Lizenz (51-100 Nodes):** [€ XX.XXX] p.a.

### 3.2 Staffelpreise
Bei größeren Deployments gelten folgende Rabattstaffeln:
- 11-25 Nodes: 10% Rabatt
- 26-50 Nodes: 15% Rabatt
- 51-75 Nodes: 20% Rabatt
- 76-100 Nodes: 25% Rabatt

### 3.3 Support-Gebühr
Die jährliche Support-Gebühr (24/7 + TAM) beträgt:
- 20% der Lizenzgebühr
- Im ersten Jahr inkludiert

### 3.4 Zahlungsbedingungen
- Zahlungsziel: 30 Tage netto
- Jährliche Vorauszahlung
- Bei mehrjährigen Verträgen: 5% Rabatt (2 Jahre), 10% Rabatt (3 Jahre)

### 3.5 Preisanpassungen
- Jährliche Anpassung basierend auf Verbraucherpreisindex (max. 5%)
- Ankündigung mindestens 3 Monate vor Vertragsende
- Kunde hat Sonderkündigungsrecht bei Erhöhung > 5%

---

## § 4 Vertragslaufzeit und Kündigung

### 4.1 Laufzeit
- Erstlaufzeit: 12 Monate ab Vertragsunterzeichnung
- Automatische Verlängerung um jeweils 12 Monate
- Mindestlaufzeit: 12 Monate

### 4.2 Ordentliche Kündigung
- Kündigungsfrist: 3 Monate zum Vertragsende
- Schriftform (inkl. E-Mail mit qualifizierter Signatur)
- Keine Rückerstattung bereits gezahlter Gebühren

### 4.3 Außerordentliche Kündigung
Beide Parteien können aus wichtigem Grund außerordentlich kündigen, insbesondere bei:
- Zahlungsverzug > 30 Tage (Lizenzgeber)
- Wesentliche Vertragsverletzung (beide Parteien)
- Insolvenz der Gegenpartei (beide Parteien)

### 4.4 Folgen der Kündigung
- Nutzungsrecht erlischt mit Vertragsende
- 30-tägige Grace Period für Datenmigration
- Enterprise-Module werden nach Grace Period deaktiviert
- Community Edition bleibt nutzbar (MIT-Lizenz)

---

## § 5 Nutzungsrechte und Beschränkungen

### 5.1 Gewährte Rechte
Der Lizenznehmer erhält ein:
- Nicht-exklusives
- Nicht-übertragbares (außer § 5.4)
- Weltweites
- Zeitlich auf Vertragsdauer beschränktes
Nutzungsrecht an der Software

### 5.2 Zulässige Nutzung
- Produktive Nutzung in eigenen Geschäftsprozessen
- Einbindung in eigene Anwendungen (ohne Weitervertrieb der DB)
- Anpassung und Konfiguration für eigene Zwecke
- Erstellung von Backups und Kopien für DR-Zwecke

### 5.3 Unzulässige Nutzung
Nicht gestattet sind:
- Weiterverkauf oder Vermietung der Software
- Bereitstellung als Managed Service (ohne Managed Service Addendum)
- Reverse Engineering der Enterprise-Module
- Umgehung von Lizenz-Checks oder Node-Limitierungen
- Entfernung von Copyright-Hinweisen

### 5.4 Übertragung
Eine Übertragung der Lizenz ist möglich:
- Bei Unternehmensübernahmen (Asset Deal)
- Bei Fusionen (Share Deal)
- Mit schriftlicher Zustimmung des Lizenzgebers
- Gegen Bearbeitungsgebühr (€ 500)

### 5.5 Konzernklausel
Innerhalb eines Konzerns (i.S.d. § 18 AktG) darf die Software weitergegeben werden:
- Ohne zusätzliche Lizenzgebühren
- Unter Einhaltung der Node-Limits
- Mit Mitteilung an den Lizenzgeber

---

## § 6 Lieferung und Installation

### 6.1 Lieferumfang
Der Lizenzgeber stellt bereit:
- Enterprise-Module als Binaries (DLL/SO)
- Digitale Lizenzschlüssel
- Installations-Skripte und Tools
- Dokumentation (PDF + Online)

### 6.2 Lieferweg
- Download-Portal (HTTPS)
- Docker Registry (Private)
- Helm Chart Repository
- GitHub Release (für berechtigte Kunden)

### 6.3 Installation
- Self-Service durch Kunden
- Remote-Assistance möglich (kostenpflichtig)
- On-Site Installation möglich (siehe § 10)

### 6.4 Lieferfrist
- Sofort nach Zahlungseingang oder
- Innerhalb von 5 Werktagen nach Vertragsabschluss

---

## § 7 Updates und Upgrades

### 7.1 Maintenance-Updates
Im Support-Zeitraum erhält der Kunde kostenfrei:
- **Patch-Releases** (z.B. v1.3.4 → v1.3.5)
  - Security Fixes
  - Bug Fixes
  - Performance-Verbesserungen
- **Minor-Releases** (z.B. v1.3.x → v1.4.x)
  - Neue Features
  - API-Erweiterungen
  - Stabilitätsverbesserungen

### 7.2 Major-Upgrades
Major-Releases (z.B. v1.x → v2.x):
- Im ersten Jahr kostenfrei
- Danach optional gegen Aufpreis (25% der Lizenzgebühr)
- Mit Migrationssupport

### 7.3 Update-Prozess
- Automatische Benachrichtigung über neue Releases
- Download über Kundenportal
- Rollback-Garantie innerhalb 48h
- Zero-Downtime-Updates (bei HA-Setup)

### 7.4 End-of-Life Policy
- Mainstream Support: 24 Monate nach Release
- Extended Support: Weitere 12 Monate (kostenpflichtig)
- EOL-Ankündigung: Mindestens 12 Monate im Voraus

---

## § 8 Gewährleistung und Haftung

### 8.1 Funktionsgarantie
Der Lizenzgeber gewährleistet, dass die Software:
- Die in der Dokumentation beschriebenen Funktionen bereitstellt
- Frei von reproduzierbaren Fehlern ist, die die Nutzung wesentlich beeinträchtigen
- Den gültigen Sicherheitsstandards entspricht (OWASP, CWE)

### 8.2 Gewährleistungsfrist
- 12 Monate ab Lieferung
- Bei Updates: Neuberechnung ab Update-Zeitpunkt
- Verlängerung durch Support-Vertrag

### 8.3 Mängelbeseitigung
Bei Mängeln hat der Lizenzgeber die Wahl zwischen:
- **Nachbesserung:** Bug-Fix oder Patch innerhalb angemessener Frist
- **Ersatzlieferung:** Alternative Version ohne den Mangel
- **Workaround:** Temporäre Lösung bis zur finalen Behebung

Fristen für Mängelbeseitigung:
- **Kritisch** (System nicht nutzbar): 4 Stunden (Reaktion), 24 Stunden (Workaround)
- **Hoch** (Kernfunktion beeinträchtigt): 8 Stunden (Reaktion), 5 Werktage (Fix)
- **Mittel** (Nebenfunction betroffen): 2 Werktage (Reaktion), 30 Tage (Fix)
- **Niedrig** (Kosmetisch): Best Effort

### 8.4 Haftungsbegrenzung
Die Haftung des Lizenzgebers ist beschränkt auf:
- **Bei Vorsatz und grober Fahrlässigkeit:** Unbegrenzt
- **Bei leichter Fahrlässigkeit und Verletzung wesentlicher Vertragspflichten:**
  - Personenschäden: Unbegrenzt
  - Sachschäden: Vorhersehbarer, vertragstypischer Schaden
  - Vermögensschäden: Begrenzt auf die im jeweiligen Vertragsjahr gezahlte Lizenzgebühr
- **Bei leichter Fahrlässigkeit und Verletzung nicht wesentlicher Vertragspflichten:** Ausgeschlossen

### 8.5 Ausschluss von Gewährleistung
Gewährleistung entfällt bei:
- Unsachgemäßer Bedienung
- Modifikation der Software durch Dritte
- Einsatz mit inkompatibler Hard-/Software
- Nichtbeachtung der Systemanforderungen

### 8.6 Haftungsausschluss Dritter
Für Schäden durch eingebundene Open-Source-Komponenten haftet der Lizenzgeber nur bei Vorsatz.

---

## § 9 Service Level Agreement (SLA)

### 9.1 Verfügbarkeit
Der Lizenzgeber garantiert eine Verfügbarkeit der Support-Kanäle von:
- **99.5% pro Quartal** (entspricht ~43h Downtime/Jahr)

Nicht eingerechnet werden:
- Geplante Wartungsfenster (angekündigt 48h vorher)
- Force Majeure
- Probleme in der Kundennetzwerk-Infrastruktur

### 9.2 Support-Reaktionszeiten

| Priorität | Beschreibung | Reaktionszeit | Lösungszeit (Ziel) |
|-----------|--------------|---------------|-------------------|
| **P1 - Kritisch** | Produktionssystem vollständig ausgefallen | 4 Stunden (Business Hours) | 24 Stunden (Workaround) |
| **P2 - Hoch** | Kernfunktion stark beeinträchtigt | 1 Werktag | 3 Werktage (Workaround) |
| **P3 - Mittel** | Funktionseinschränkung | 2 Werktage | 2 Wochen |
| **P4 - Niedrig** | Frage, Feature-Request | 5 Werktage | Best Effort |

### 9.3 Support-Kanäle
- **24/7 Telefon-Hotline:** Für P1/P2-Fälle
- **E-Mail Support:** Alle Prioritäten, support@themisdb.com
- **Ticket-System:** Web-Portal mit Tracking
- **Remote-Assistance:** Screen Sharing, SSH (nach Vereinbarung)

### 9.4 Technical Account Manager (TAM)
Inkludiert ab 50 Nodes:
- Dedizierter Ansprechpartner
- Quartalsweise Review-Meetings
- Proaktive Monitoring-Alerts
- Architectural Advisory

### 9.5 Eskalationspfad
1. **L1 Support:** Support-Team (Reaktionszeit gemäß SLA)
2. **L2 Support:** Senior Engineers (Eskalation nach 24h bei P1)
3. **L3 Support:** Entwicklungs-Team (Eskalation nach 3 Tagen bei P1)
4. **Management Escalation:** CTO (auf Kundenwunsch)

### 9.6 SLA-Gutschriften
Bei Unterschreitung der Verfügbarkeit:
- 99.5% - 99.0%: 10% Gutschrift (nächste Rechnung)
- 99.0% - 98.0%: 25% Gutschrift
- < 98.0%: 50% Gutschrift

---

## § 10 Professional Services (Optional)

### 10.1 Onboarding & Training
**Paket "Quick Start"** (3 Tage, Remote):
- Architektur-Review
- Installation & Konfiguration
- Basis-Training (5 Teilnehmer)
- Preis: € 4.500

**Paket "Enterprise Onboarding"** (5 Tage, On-Site):
- Quick Start Umfang +
- Performance-Tuning
- High-Availability Setup
- Erweiteres Training (10 Teilnehmer)
- Preis: € 9.500

### 10.2 Migrations-Services
**Paket "Standard Migration"**:
- Migrations-Konzept
- Daten-Migration (bis 500 GB)
- Testing & Validation
- Preis: € 6.500

**Paket "Enterprise Migration"**:
- Standard Migration Umfang +
- Daten-Migration (bis 5 TB)
- Zero-Downtime Migration
- Rollback-Plan
- Preis: € 15.000

### 10.3 Custom Development
- Stundensatz: € 180 (Developer)
- Stundensatz: € 250 (Senior Architect)
- Mindestabnahme: 20 Stunden
- Bereitstellung: Innerhalb 4 Wochen

### 10.4 Health Check
**Jährlicher Health Check** (1 Tag):
- Performance-Analyse
- Security-Audit
- Best-Practices Review
- Optimierungs-Empfehlungen
- Preis: € 2.500

---

## § 11 Datenschutz und Vertraulichkeit

### 11.1 DSGVO-Konformität
- Software ist DSGVO-ready
- Data Processing Agreement (DPA) verfügbar
- EU-DSGVO Compliance bei Cloud-Bereitstellung

### 11.2 Vertraulichkeit
Beide Parteien verpflichten sich:
- Geschäftsgeheimnisse vertraulich zu behandeln
- Nur zur Vertragserfüllung zu verwenden
- Weitergabe an Dritte nur mit Zustimmung
- Verpflichtung gilt 5 Jahre nach Vertragsende

### 11.3 Telemetrie und Monitoring (Opt-In)
Der Kunde kann optional aktivieren:
- Anonymisierte Performance-Metriken
- Crash Reports
- Feature-Usage Analytics
Daten werden ausschließlich zur Produktverbesserung verwendet.

### 11.4 Audit-Rechte
Der Lizenzgeber darf jährlich (mit 30 Tagen Vorlauf):
- Einhaltung der Lizenzbedingungen prüfen
- Node-Count verifizieren
- Durch externen Wirtschaftsprüfer (unter NDA)

---

## § 12 Schutzrechte und Rechte Dritter

### 12.1 Eigentum
Alle Rechte an der Software und Dokumentation verbleiben beim Lizenzgeber.

### 12.2 Open-Source-Komponenten
Die Software nutzt Open-Source-Komponenten (siehe ATTRIBUTIONS.md):
- Core Engine: MIT License
- Dependencies: MIT, Apache 2.0, BSD
- Keine GPL/AGPL-Komponenten in Enterprise-Modulen

### 12.3 Schutzrechtsverletzungen
Bei Ansprüchen Dritter wegen Schutzrechtsverletzungen:
- Freistellung des Kunden durch Lizenzgeber
- Alternative: Ersatzlieferung rechtfreie Version
- Alternative: Modifikation zur Rechtfreiheit
- Ultima Ratio: Rücknahme gegen Erstattung

Ausgeschlossen bei:
- Kundenseitige Modifikationen
- Kombination mit Dritt-Software
- Nutzung nach Kenntnis der Verletzung

---

## § 13 Höhere Gewalt

### 13.1 Definition
Höhere Gewalt umfasst:
- Naturkatastrophen
- Kriege, Terrorismus
- Pandemien
- Stromausfälle, Netzwerkausfälle (großflächig)
- Cyberangriffe auf kritische Infrastruktur
- Gesetzesänderungen

### 13.2 Folgen
- Aussetzung der Leistungspflicht für Dauer der höheren Gewalt
- Keine Vertragsstrafen
- Verlängerung der Fristen entsprechend
- Außerordentliches Kündigungsrecht ab 3 Monate Dauer

---

## § 14 Schlussbestimmungen

### 14.1 Gerichtsstand
- Ausschließlicher Gerichtsstand: Sitz des Lizenzgebers
- Bei Verbrauchern: Gesetzlicher Gerichtsstand

### 14.2 Anwendbares Recht
- Recht der Bundesrepublik Deutschland
- Unter Ausschluss des UN-Kaufrechts (CISG)

### 14.3 Salvatorische Klausel
- Unwirksamkeit einzelner Klauseln berührt Vertrag nicht
- Ersetzung durch wirksame Regelung mit ähnlichem Zweck

### 14.4 Schriftform
- Änderungen bedürfen der Schriftform
- E-Mail mit qualifizierter elektronischer Signatur ausreichend
- Keine mündlichen Nebenabreden

### 14.5 Vertragssprache
- Vertragssprache: Deutsch
- Bei Übersetzungen: Deutsche Fassung maßgeblich

### 14.6 Überschriften
- Überschriften dienen nur der Orientierung
- Keine Auslegungsrelevanz

---

## § 15 Besondere Vereinbarungen

### 15.1 Proof of Concept (PoC)
- 30 Tage kostenlose PoC-Lizenz
- Alle Enterprise-Features aktiviert
- Limitierung: 10 Nodes, 1 TB Daten
- Übergang in Produktivlizenz ohne Neuinstallation

### 15.2 Academic Licensing
- 50% Rabatt für Universitäten und Forschungseinrichtungen
- Nur für nicht-kommerzielle Forschung
- Publikationen müssen ThemisDB als Tool nennen

### 15.3 Startup-Programm
- 75% Rabatt im ersten Jahr für Startups
- 50% Rabatt im zweiten Jahr
- 25% Rabatt im dritten Jahr
- Voraussetzung: < 50 Mitarbeiter, < € 5M Jahresumsatz

---

## Anlagen

1. **Anlage 1:** Lizenzschlüssel und Aktivierungsinformationen
2. **Anlage 2:** Systemanforderungen und unterstützte Plattformen
3. **Anlage 3:** Liste der Enterprise-Module und Features
4. **Anlage 4:** Service Level Agreement (Detailliert)
5. **Anlage 5:** Data Processing Agreement (DSGVO)
6. **Anlage 6:** Professional Services Katalog

---

## Unterschriften

**Lizenzgeber:**  
ThemisDB GmbH

_______________________  
Ort, Datum

_______________________  
Unterschrift, Name

**Lizenznehmer:**  
[Firmenname]

_______________________  
Ort, Datum

_______________________  
Unterschrift, Name, Funktion

---

**Vertragsnummer:** ENT-2026-[XXXX]  
**Erstellt am:** [Datum]  
**Version:** 1.0
