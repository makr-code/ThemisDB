# Gewährleistungs- und Wartungsvertrag - ThemisDB Enterprise Edition

**Stand:** April 2026  
**Version:** 1.0  
**Kategorie:** 🏢 Enterprise Support & Maintenance

---

## Präambel

Dieser Gewährleistungs- und Wartungsvertrag ("Wartungsvertrag") ergänzt den Softwarevertrag für die ThemisDB Enterprise Edition und regelt die Wartungs-, Support- und Gewährleistungsleistungen zwischen:

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

## § 1 Vertragsgegenstand und Leistungsumfang

### 1.1 Gegenstand
Dieser Vertrag regelt die Wartung, den Support und die Gewährleistungsleistungen für die ThemisDB Enterprise Edition gemäß dem zugrunde liegenden Softwarelizenzvertrag.

### 1.2 Wartungsleistungen
Der Dienstleister erbringt folgende Wartungsleistungen:

**Software-Maintenance:**
- Bereitstellung von Patches (Bugfixes)
- Bereitstellung von Security-Updates
- Bereitstellung von Minor-Releases
- Bereitstellung von Major-Upgrades (im ersten Jahr)

**Preventive Maintenance:**
- Proaktive Überwachung bekannter Issues
- Benachrichtigung über kritische Sicherheitslücken
- Empfehlungen für Performance-Optimierungen
- Quartalsweise Release-Notes und Best Practices

**Corrective Maintenance:**
- Fehleranalyse und Diagnose
- Erstellung von Workarounds
- Entwicklung und Bereitstellung von Hotfixes
- Regression Testing nach Bugfixes

### 1.3 Support-Leistungen
**Technical Support:**
- 24/7 Telefon- und E-Mail-Support
- Ticket-System mit Tracking
- Remote-Assistance (Screen Sharing, SSH)
- Knowledge Base und FAQ-Zugang

**Technical Account Manager (TAM):**
- Dedizierter Ansprechpartner (ab 50 Nodes)
- Quartalsweise Review-Meetings
- Proaktive Eskalation bei kritischen Issues
- Koordination mit Engineering-Team

**Advisory Services:**
- Architectural Guidance
- Performance Tuning Recommendations
- Security Best Practices
- Upgrade Planning

---

## § 2 Gewährleistung

### 2.1 Umfang der Gewährleistung
Der Dienstleister gewährleistet, dass die Software:
- Die in der aktuellen Dokumentation beschriebenen Funktionen erfüllt
- Frei von reproduzierbaren, die Nutzung wesentlich beeinträchtigenden Fehlern ist
- Den aktuellen Sicherheitsstandards entspricht (OWASP Top 10, CWE/SANS Top 25)
- Mit den dokumentierten Plattformen und Abhängigkeiten kompatibel ist

### 2.2 Fehlerkategorien und Definitionen

**Kritischer Fehler (P0/P1):**
- Produktionssystem ist nicht verfügbar
- Datenverlust oder Datenkorruption
- Sicherheitslücke (Critical/High Severity)
- Kernfunktion ist nicht nutzbar

**Schwerer Fehler (P2):**
- Wesentliche Funktionseinschränkung
- Starke Performance-Degradation (> 50%)
- Workaround vorhanden aber aufwändig
- Auswirkung auf Endbenutzer

**Mittlerer Fehler (P3):**
- Nebenfunktion beeinträchtigt
- Workaround einfach umsetzbar
- Keine direkten Endbenutzer-Auswirkungen
- Dokumentationsfehler (wesentlich)

**Geringer Fehler (P4):**
- Kosmetische Probleme
- Dokumentationsfehler (unwesentlich)
- Feature-Requests
- Performance-Optimierungswünsche

### 2.3 Gewährleistungsfristen
- **Erstgewährleistung:** 12 Monate ab Lieferung/Installation
- **Erweiterung durch Wartungsvertrag:** Solange Wartungsvertrag aktiv
- **Sicherheitslücken:** Keine zeitliche Begrenzung (Responsible Disclosure)

### 2.4 Mängelbehebung - Response und Resolution Times

| Priorität | Beschreibung | Reaktionszeit | Workaround-Ziel | Fix-Bereitstellung |
|-----------|--------------|---------------|-----------------|-------------------|
| **P0 - Kritisch** | System down | 4 Stunden (Business Hours) | 24 Stunden | 5 Werktage |
| **P1 - Hoch** | Kernfunktion stark beeinträchtigt | 1 Werktag | 3 Werktage | 2 Wochen |
| **P2 - Mittel** | Nebenfunktion beeinträchtigt | 2 Werktage | 2 Wochen | 1 Monat |
| **P3 - Niedrig** | Geringe Auswirkung | 5 Werktage | Best Effort | Nächstes Minor Release |
| **P4 - Kosmetisch** | Keine Auswirkung | 5 Werktage | N/A | Nächstes Major Release |

**Reaktionszeit:** Erste Rückmeldung durch Support-Team  
**Workaround-Ziel:** Temporäre Lösung zur Fortsetzung des Betriebs  
**Fix-Bereitstellung:** Permanente Lösung als Patch oder Update

### 2.5 Eskalationsprozess
**Level 1 (L1) - Support Team:**
- Erste Anlaufstelle
- Sammlung von Diagnosedaten
- Bekannte Issues und Workarounds
- Eskalation nach 1 Tag (P0) bzw. 3 Tagen (P1)

**Level 2 (L2) - Senior Engineers:**
- Detaillierte Fehleranalyse
- Root-Cause-Analyse
- Entwicklung von Workarounds
- Eskalation nach 3 Tagen (P0) bzw. 1 Woche (P1)

**Level 3 (L3) - Development Team:**
- Code-Analyse und Debugging
- Patch-Development
- Regression Testing
- Eskalation nach 1 Woche (P0) zum Management

**Management Escalation:**
- VP Engineering (auf Kundenwunsch)
- C-Level bei anhaltenden kritischen Issues
- Direkte Kommunikation bei Business-Impact

### 2.6 Ausschluss der Gewährleistung
Gewährleistung entfällt bei:
- Unsachgemäßer Installation oder Konfiguration
- Einsatz mit nicht unterstützten Plattformen oder Versionen
- Modifikation der Software durch Kunde oder Dritte
- Nichtbeachtung der Systemanforderungen
- Einsatz in nicht dokumentierten Umgebungen
- Force Majeure (siehe § 13)

---

## § 3 Software-Updates und Upgrades

### 3.1 Arten von Updates
**Patch-Releases (z.B. v1.3.4 → v1.3.5):**
- Bugfixes
- Security-Patches
- Performance-Verbesserungen
- Kostenfrei, solange Wartungsvertrag aktiv

**Minor-Releases (z.B. v1.3.x → v1.4.x):**
- Neue Features
- API-Erweiterungen
- Deprecated Features
- Kostenfrei, solange Wartungsvertrag aktiv

**Major-Releases (z.B. v1.x → v2.x):**
- Breaking Changes möglich
- Architektur-Änderungen
- Neue Module
- Kostenfrei im ersten Jahr, danach optional

### 3.2 Update-Bereitstellung
**Frequency:**
- Security Patches: Nach Bedarf (innerhalb 48h nach Disclosure)
- Patch Releases: Monatlich (bei Bedarf)
- Minor Releases: Quartalsweise
- Major Releases: Jährlich

**Delivery:**
- Download-Portal (HTTPS, 2FA-geschützt)
- Docker Registry (Private)
- E-Mail-Benachrichtigung bei neuen Releases
- RSS-Feed für Release-Notes

**Release Notes:**
- Detaillierte Changelog
- Breaking Changes hervorgehoben
- Upgrade Instructions
- Known Issues

### 3.3 Update-Prozess
**Kunde ist verantwortlich für:**
- Durchführung von Backups vor Updates
- Testing in Staging-Umgebung
- Ausführung des Update-Prozesses
- Validierung nach Update

**Dienstleister bietet:**
- Update-Skripte und Tools
- Dokumentierte Upgrade-Pfade
- Rollback-Prozeduren
- Remote-Assistance (optional, kostenpflichtig)

### 3.4 Backwards Compatibility
- APIs: Mindestens 2 Major-Versionen
- Datenformat: Forward-kompatibel (neue Version liest alte Daten)
- Configuration: Automatische Migration von Konfigurationsdateien
- Deprecation Policy: Mindestens 12 Monate Ankündigung

### 3.5 Long-Term Support (LTS)
**LTS-Releases:**
- Jede zweite Major-Version
- 5 Jahre Support (2 Jahre Mainstream + 3 Jahre Extended)
- Nur Security und Critical Bugfixes (keine neuen Features)
- Empfohlen für stabile Produktionsumgebungen

**Extended Support (Jahre 3-5):**
- Gegen Aufpreis (50% der Standard-Wartungsgebühr)
- Nur Critical und Security Fixes
- Mindestens 6 Monate Vorlauf für Buchung

---

## § 4 Service Level Agreement (SLA)

### 4.1 Support-Verfügbarkeit
**Business Hours Support:**
- Verfügbarkeit: Mo-Fr, 9-17 Uhr (Ortszeit)
- Telefon-Hotline (Deutsch und Englisch)
- E-Mail Support (support@themisdb.com)
- Ticket-System (Online-Portal)

**Verfügbarkeit der Support-Kanäle:**
- 99.5% pro Quartal
- Geplante Wartungsfenster: Max. 4 Stunden/Quartal (angekündigt)

### 4.2 Response Times (siehe § 2.4)
Reaktionszeiten gemäß Priorität:
- P0: 4 Stunden (Business Hours)
- P1: 1 Werktag
- P2: 2 Werktage
- P3: 5 Werktage
- P4: 1 Woche

### 4.3 Resolution Times (Zielwerte)
**Workaround-Bereitstellung:**
- P0: 24 Stunden
- P1: 3 Werktage
- P2: 2 Wochen
- P3: Best Effort

**Permanente Lösung:**
- P0: 5 Werktage (Hotfix)
- P1: 2 Wochen (Patch)
- P2: 1 Monat (nächstes Release)
- P3: Nächstes Minor Release
- P4: Nächstes Major Release

### 4.4 SLA-Gutschriften
Bei Unterschreitung der Reaktionszeiten:

| SLA-Verletzung | Gutschrift (% der Quartalsgebühr) |
|----------------|-----------------------------------|
| P0 Response > 8h | 10% |
| P0 Workaround > 48h | 15% |
| P1 Response > 2 Werktage | 5% |
| P1 Workaround > 1 Woche | 10% |
| Support-Verfügbarkeit < 99.5% | 10% pro 0.1% Unterschreitung |

**Maximum:** 50% der Quartals-Wartungsgebühr  
**Geltendmachung:** Innerhalb 30 Tage nach Quartalsende  
**Auszahlung:** Als Gutschrift auf nächste Rechnung

### 4.5 Ausnahmen von SLA
SLA-Verpflichtungen entfallen bei:
- Wartungsfenster (angekündigt)
- Force Majeure
- Kundenseitige Netzwerk-/Infrastruktur-Probleme
- Nicht unterstützte Konfigurationen
- Veraltete Versionen (> 2 Major-Versionen alt)

---

## § 5 Technical Account Manager (TAM)

### 5.1 TAM-Service (ab 50 Nodes)
**Leistungen:**
- Dedizierter Ansprechpartner
- Direkte Telefonnummer und E-Mail
- Proaktive Überwachung der Kundenumgebung
- Eskalationsmanagement

**Meetings:**
- Monatliche Status-Calls (30 Minuten)
- Quartalsweise Business Reviews (2 Stunden)
- On-Demand Meetings bei Bedarf
- Jährliches Strategic Planning Meeting

**Deliverables:**
- Quartalsweise Health Check Reports
- Jährliche Roadmap-Alignment
- Custom Best Practices Guide
- Incident Post-Mortem Reports

### 5.2 Proaktive Services
**Monitoring & Alerting:**
- Überwachung von Release Notes auf Relevanz für Kunden
- Benachrichtigung über kritische Security Advisories
- Warnung vor End-of-Life (EOL) von Versionen
- Hinweise auf Performance-Optimierungen

**Advisory:**
- Architektur-Reviews (jährlich)
- Upgrade-Planung (vor Major-Releases)
- Capacity-Planning (bei Wachstum)
- Disaster-Recovery-Planning

### 5.3 Verfügbarkeit des TAM
- Business Hours: Mo-Fr, 9-17 Uhr (Ortszeit Kunde)
- Erreichbarkeit: E-Mail, Telefon, Slack/Teams
- Backup-TAM: Bei Urlaub/Krankheit
- Eskalationspfad: VP Customer Success

---

## § 6 Wartungs- und Support-Gebühren

### 6.1 Jährliche Wartungsgebühr
Die Wartungsgebühr beträgt 20% der aktuellen Jahres-Lizenzgebühr.

**Beispiel:**
- Lizenzgebühr: € 50.000 p.a.
- Wartungsgebühr: € 10.000 p.a. (20%)

### 6.2 Erste Jahr
Die Wartungsgebühr ist im ersten Jahr der Lizenz inkludiert (keine zusätzlichen Kosten).

### 6.3 Folge-Jahre
Ab dem zweiten Jahr:
- Automatische Verlängerung mit Lizenzvertrag
- Jährliche Zahlung (Vorauszahlung)
- Kündigung nur zusammen mit Lizenzvertrag

### 6.4 Preisanpassungen
- Jährliche Anpassung entsprechend der Lizenzgebühr
- Bei Verbraucherpreisindex-Anpassung: Gleiche %-Satz
- Ankündigung: Mindestens 3 Monate vor Verlängerung

### 6.5 Multi-Year Discount
Bei mehrjähriger Vorauszahlung:
- 2 Jahre: 5% Rabatt
- 3 Jahre: 10% Rabatt
- 5 Jahre: 15% Rabatt

### 6.6 Erweiterte Support-Optionen (Optional)
**Premium Support (+ 10% Aufpreis):**
- TAM auch für < 50 Nodes
- Priorisierte Bearbeitung (P1 → P0 Response Time)
- Monatliche Health Checks
- Zugang zu Beta-Features

**Extended Support für EOL-Versionen (+ 50% Aufpreis):**
- Support für Versionen außerhalb Mainstream-Support
- Nur Security- und Critical-Bugfixes
- Mindestbuchung: 12 Monate

---

## § 7 Pflichten des Kunden

### 7.1 Mitwirkungspflichten
Der Kunde ist verpflichtet:
- Fehler unverzüglich und detailliert zu melden
- Reproduktionsschritte bereitzustellen
- Diagnosedaten (Logs, Konfigurationen) zur Verfügung zu stellen
- Staging-Umgebung für Testung von Fixes bereitzustellen
- Updates zeitnah zu installieren (innerhalb 90 Tage)

### 7.2 Umgebungsanforderungen
- Software auf unterstützten Plattformen betreiben
- Systemanforderungen einhalten (CPU, RAM, Storage)
- Netzwerk-Konnektivität für Remote-Support
- Backup-Strategie implementieren

### 7.3 Informationspflicht
- Änderungen an der Infrastruktur mitteilen (vor Durchführung)
- Sicherheitsvorfälle unverzüglich melden
- Kontaktdaten aktuell halten
- Geplante Wartungsfenster kommunizieren

### 7.4 Eskalationskontakte
Bereitstellung von:
- Technischer Ansprechpartner (Primary + Backup)
- Management-Eskalationskontakt
- 24/7 Rufbereitschaft für P0-Incidents
- E-Mail-Verteiler für Statusupdates

### 7.5 Testing und Abnahme
- Testung von Patches in Staging-Umgebung
- Abnahme von Fixes innerhalb 5 Werktagen
- Feedback zu neuen Releases
- Teilnahme an Beta-Programmen (optional)

---

## § 8 Remote-Support und Fernwartung

### 8.1 Remote-Zugriff
Für effektive Fehlerdiagnose kann Remote-Zugriff erforderlich sein:
- **Screen Sharing:** Via Teams, Zoom oder WebEx
- **SSH-Zugang:** Temporär, auf Anfrage (mit Kunden-Freigabe)
- **VPN-Zugang:** Für komplexe Debugging-Sessions
- **Log-Upload:** Automatisiert (verschlüsselt, DSGVO-konform)

### 8.2 Voraussetzungen
- Kunde muss Remote-Zugang explizit genehmigen (pro Session)
- Alle Aktivitäten werden protokolliert (Audit-Log)
- Zugang wird nach Session sofort widerrufen
- NDA-Vereinbarung abdeckt Remote-Zugriff

### 8.3 Datenschutz
- Keine Speicherung von Produktionsdaten durch Dienstleister
- Logs werden anonymisiert (Entfernung von PII)
- Datenübertragung verschlüsselt (TLS 1.3)
- Löschung nach Abschluss des Support-Falls (max. 90 Tage)

### 8.4 Sicherheit
- Zugriff nur durch autorisierte Mitarbeiter
- Multi-Factor Authentication (MFA)
- Zeitlich begrenzte Credentials
- Prinzip der geringsten Rechte (Least Privilege)

---

## § 9 Schulung und Dokumentation

### 9.1 Dokumentations-Zugang
Im Wartungsvertrag enthalten:
- Online-Dokumentation (immer aktuellste Version)
- API-Referenz
- Best Practices Guides
- Troubleshooting Guides
- Video-Tutorials

### 9.2 Schulungsangebote (Optional)
**Basic Training (3 Tage):**
- Architektur-Überblick
- Installation und Konfiguration
- Grundlegende Administration
- Preis: € 2.500 (bis 5 Teilnehmer)

**Advanced Training (5 Tage):**
- Performance-Tuning
- High-Availability Setup
- Disaster Recovery
- Monitoring und Alerting
- Preis: € 4.500 (bis 5 Teilnehmer)

**Custom Training:**
- Auf Kundenbedürfnisse zugeschnitten
- On-Site oder Remote
- Preis: € 1.500/Tag + Reisekosten

### 9.3 Webinare und Workshops
- Quartalsweise kostenfreie Webinare (1 Stunde)
- Themen: Neue Features, Best Practices, Use Cases
- Aufzeichnungen verfügbar im Kundenportal

### 9.4 Knowledge Base
- Self-Service Portal mit Suchfunktion
- FAQs zu häufigen Problemen
- Schritt-für-Schritt-Anleitungen
- Community-Forum (optional)

---

## § 10 Gewährleistungserweiterungen (Optional)

### 10.1 Extended Warranty (Verlängerte Gewährleistung)
- Verlängerung der Gewährleistung auf 36 Monate
- Preis: + 10% der Wartungsgebühr
- Abdeckung aller Fehler (auch Low-Priority)
- Rückwirkende Buchung nicht möglich

### 10.2 Performance Guarantee
- Garantie für spezifische Performance-Metriken
- Basierend auf Benchmark-Tests
- Bei Unterschreitung: Kostenloses Performance-Tuning (40h)
- Preis: + 15% der Wartungsgebühr

### 10.3 Zero-Downtime-Guarantee
- Garantie für Rolling Updates ohne Ausfallzeit
- Voraussetzung: HA-Setup (min. 3 Nodes)
- Bei Downtime: SLA-Gutschriften verdoppelt
- Preis: + 20% der Wartungsgebühr

### 10.4 Priority Bugfixing
- P1-Bugs werden als P0 behandelt
- P2-Bugs werden als P1 behandelt
- Keine Auswirkung auf P0 (bereits höchste Priorität)
- Preis: + 25% der Wartungsgebühr

---

## § 11 Berichterstattung und Metriken

### 11.1 Quartalsberichte
Der Dienstleister erstellt quartalsweise Berichte:
- Anzahl und Art der Support-Tickets
- Durchschnittliche Response- und Resolution-Times
- Anzahl und Kategorisierung der Bugs
- Übersicht über bereitgestellte Updates
- SLA-Compliance-Report

### 11.2 Incident Post-Mortems
Bei P0/P1-Incidents:
- Detaillierte Root-Cause-Analyse
- Timeline der Ereignisse
- Lessons Learned
- Präventive Maßnahmen
- Bereitstellung innerhalb 5 Werktagen

### 11.3 Jährliche Reviews
- Umfassende Analyse der Support-Aktivitäten
- Trend-Analyse (Häufige Probleme, Verbesserungen)
- Recommendations für Optimierungen
- Präsentation durch TAM

### 11.4 Metriken-Dashboard
- Online-Portal mit Echtzeit-Statistiken
- Offene Tickets, Status, Priorität
- SLA-Compliance in Prozent
- Update-Status (aktuellste Version, pending updates)

---

## § 12 Kündigung und Vertragsende

### 12.1 Laufzeit
- Parallel zum Softwarelizenzvertrag
- Automatische Verlängerung mit Lizenzvertrag
- Keine separate Kündigungsoption

### 12.2 Folgen der Kündigung
**Bei Vertragsende:**
- Einstellung aller Support-Leistungen
- Kein Zugang zu Updates und Patches
- Löschung des Zugangs zum Kundenportal
- Grace Period: 30 Tage (kostenpflichtig verlängerbar)

**Ausnahmen:**
- Kritische Sicherheitslücken werden auch nach Vertragsende gepatcht (Best Effort, 6 Monate)
- Dokumentation bleibt zugänglich (als PDF-Download)

### 12.3 Downgrade zu Community Support
- Umstieg auf Community-Forum (kostenfrei)
- Keine garantierten Response-Times
- Keine Gewährleistung
- Keine Updates (nur Open-Source-Releases)

### 12.4 Reaktivierung
- Reaktivierung innerhalb 12 Monate möglich
- Nachzahlung der ausgesetzten Wartungsgebühren
- Keine Rückwirkende Bearbeitung von Tickets

---

## § 13 Haftung und Versicherung

### 13.1 Haftungsbegrenzung
- Wie im Hauptvertrag (Softwarelizenzvertrag) definiert
- Zusätzlich: Haftung für fehlerhafte Wartungsleistungen begrenzt auf Wartungsgebühr des laufenden Jahres

### 13.2 Professional Indemnity Insurance
Der Dienstleister unterhält:
- Berufshaftpflichtversicherung (€ 5M Deckung)
- Nachweis auf Anfrage
- Deckung umfasst fahrlässige Fehler bei Wartung

### 13.3 Keine Haftung für
- Kundenseitige Fehlkonfigurationen
- Nicht rechtzeitig installierte Updates
- Modifikationen durch Dritte
- Force Majeure

---

## § 14 Datenschutz und Vertraulichkeit

### 14.1 Diagnosedaten
- Logs und Konfigurationen können PII enthalten
- Kunde gibt Zustimmung zur Verarbeitung für Support-Zwecke
- Löschung nach Abschluss des Support-Falls (max. 90 Tage)
- DSGVO-konform (DPA ist Bestandteil)

### 14.2 Vertraulichkeit
- Beide Parteien behandeln Support-Fälle vertraulich
- Keine Weitergabe an Dritte ohne Zustimmung
- Ausnahme: Subunternehmer (unter NDA)

### 14.3 Anonymisierte Statistiken
- Dienstleister darf anonymisierte Statistiken nutzen
- Für interne Analysen und Produktverbesserung
- Keine Rückschlüsse auf einzelne Kunden möglich

---

## § 15 Änderungen und Ergänzungen

### 15.1 Vertragsänderungen
- Schriftform erforderlich
- Beidseitige Zustimmung
- Keine mündlichen Nebenabreden

### 15.2 Einseitige Anpassungen (durch Dienstleister)
Dienstleister kann einseitig ändern (mit 3 Monaten Vorlauf):
- Support-Kanäle (z.B. neue Chat-Plattform)
- Dokumentations-Plattform
- Ticket-System

Nicht einseitig änderbar:
- SLA-Garantien
- Reaktions-/Resolution-Times
- Preise (siehe § 6.4)

### 15.3 Gesetzesänderungen
- Bei relevanten Gesetzesänderungen (z.B. DSGVO-Updates)
- Anpassung zur Compliance (kostenneutral)
- Benachrichtigung des Kunden

---

## § 16 Schlussbestimmungen

### 16.1 Anwendbares Recht
- Recht der Bundesrepublik Deutschland
- Unter Ausschluss des UN-Kaufrechts (CISG)

### 16.2 Gerichtsstand
- Ausschließlicher Gerichtsstand: Sitz des Dienstleisters
- Bei Verbrauchern: Gesetzlicher Gerichtsstand

### 16.3 Salvatorische Klausel
- Unwirksamkeit einzelner Klauseln berührt Vertrag nicht
- Ersetzung durch rechtskonforme, ähnliche Regelung

### 16.4 Gesamtvereinbarung
- Dieser Vertrag ergänzt den Softwarelizenzvertrag
- Bei Widersprüchen: Wartungsvertrag geht vor (nur für Wartungsthemen)

### 16.5 Sprache
- Vertragssprache: Deutsch
- Support-Sprachen: Deutsch, Englisch

---

## Anlagen

1. **Anlage 1:** Support-Kontakte (Telefon, E-Mail, Ticket-System)
2. **Anlage 2:** Fehlerklassifizierung (Detailliert)
3. **Anlage 3:** Systemanforderungen (Unterstützte Plattformen)
4. **Anlage 4:** Eskalationspfade und Kontakte
5. **Anlage 5:** Data Processing Agreement (DSGVO)
6. **Anlage 6:** Remote-Support Richtlinien

---

## Unterschriften

**Dienstleister:**  
ThemisDB GmbH

_______________________  
Ort, Datum

_______________________  
Unterschrift, Name

**Kunde:**  
[Firmenname]

_______________________  
Ort, Datum

_______________________  
Unterschrift, Name, Funktion

---

**Vertragsnummer:** MAINT-ENT-2026-[XXXX]  
**Erstellt am:** [Datum]  
**Version:** 1.0  
**Gültigkeit:** Parallel zum Softwarelizenzvertrag
