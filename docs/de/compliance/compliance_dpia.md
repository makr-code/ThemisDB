# Datenschutz-Folgenabschätzung (DPIA) - ThemisDB

**Stand:** 6. April 2026  
**Version:** v1.4.1  
**Kategorie:** 🔒 Compliance  
**Klassifizierung:** Vertraulich  
**Basis:** DSGVO Art. 35, ISO 27701, BSI Standard 200-3  
**Status:** ✅ VOLLSTÄNDIG (FIND-021 behoben)

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Beschreibung der Verarbeitung](#1-beschreibung-der-verarbeitung)
- [PII-Inventar](#15-pii-inventar-personenbezogene-daten)
- [Notwendigkeits- und Verhältnismäßigkeitsprüfung](#2-notwendigkeits--und-verhältnismäßigkeitsprüfung)
- [Risikobewertung](#3-risikobewertung)
- [Technische und organisatorische Maßnahmen](#4-technische-und-organisatorische-maßnahmen-toms)
- [Betroffenenrechte](#5-betroffenenrechte)
- [Datentransfers](#6-datentransfers)
- [Risiko-Minderung](#7-risiko-minderung)
- [Konsultation](#8-konsultation)
- [Schlussfolgerung](#9-schlussfolgerung)

---

## 📋 Übersicht

Diese Datenschutz-Folgenabschätzung (Data Protection Impact Assessment, DPIA) dokumentiert die systematische Analyse der Verarbeitung personenbezogener Daten durch ThemisDB und bewertet die Risiken für die Rechte und Freiheiten betroffener Personen.

### Dokumentinformationen

| Feld | Wert |
|------|------|
| **Verantwortlicher** | [Organisation eintragen] |
| **Datenschutzbeauftragter** | [Name/Kontakt eintragen] |
| **System** | ThemisDB Multi-Model Database |
| **Version** | 1.x |
| **Erstellt** | November 2025 |
| **Letzte Überprüfung** | [Datum eintragen] |
| **Nächste Überprüfung** | [Datum + 12 Monate] |

---

## 1. Beschreibung der Verarbeitung

### 1.1 Zweck der Verarbeitung

ThemisDB ist eine Multi-Model-Datenbank, die verschiedene Datenmodelle (Relational, Graph, Vector, Document) in einem System vereint. Die Datenbank kann zur Speicherung und Verarbeitung verschiedener Arten von Daten eingesetzt werden, einschließlich personenbezogener Daten.

### 1.2 Verarbeitete Datenkategorien

| Kategorie | Beispiele | Sensibilität |
|-----------|-----------|--------------|
| **Identifikationsdaten** | Name, ID, Benutzername | Mittel |
| **Kontaktdaten** | E-Mail, Telefon, Adresse | Mittel |
| **Finanzdaten** | Kreditkarte, IBAN, Kontonummer | Hoch |
| **Gesundheitsdaten** | Diagnosen, Behandlungen | Sehr Hoch (Art. 9) |
| **Standortdaten** | GPS, Geo-Koordinaten | Hoch |
| **Kommunikationsdaten** | Nachrichten, E-Mails | Hoch |
| **Verhaltensdaten** | Logs, Interaktionen | Mittel |
| **Biometrische Daten** | Fingerabdrücke, Gesicht | Sehr Hoch (Art. 9) |

### 1.3 Betroffene Personengruppen

| Gruppe | Beschreibung |
|--------|--------------|
| **Endnutzer** | Benutzer der Anwendungen, die ThemisDB nutzen |
| **Mitarbeiter** | Interne Nutzer mit Datenbankzugriff |
| **Kunden** | Geschäftskunden und deren Daten |
| **Partner** | Externe Partner mit Datenzugriff |

### 1.4 Datenflüsse

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   Anwendung     │────▶│   ThemisDB      │────▶│   Backup/       │
│   (Client)      │     │   Server        │     │   Archiv        │
└─────────────────┘     └─────────────────┘     └─────────────────┘
                               │
                               ▼
                        ┌─────────────────┐
                        │   Audit-Logs    │
                        │   (JSONL)       │
                        └─────────────────┘
```

---

## 1.5 PII-Inventar (Personenbezogene Daten)

### 1.5.1 Automatische PII-Erkennung

ThemisDB verfügt über ein automatisches PII-Detection-System, das folgende Kategorien personenbezogener Daten erkennt und markiert:

| PII-Typ | Erkennungsmuster | Sensibilität | DSGVO Art. | Verschlüsselung |
|---------|------------------|--------------|------------|-----------------|
| **E-Mail-Adressen** | RFC 5322 Regex | Mittel | Art. 6(1) | Field-Level Encryption |
| **Telefonnummern** | E.164, lokale Formate | Mittel | Art. 6(1) | Field-Level Encryption |
| **Kreditkartennummern** | Luhn-Check, BIN-Pattern | Hoch | Art. 6(1)(b) | Mandatory Encryption |
| **IBAN** | ISO 13616 | Hoch | Art. 6(1)(b) | Mandatory Encryption |
| **Sozialversicherungsnummer** | Länder-spezifisch | Sehr Hoch | Art. 9 | Mandatory Encryption |
| **IP-Adressen** | IPv4/IPv6 | Niedrig-Mittel | Art. 6(1)(f) | Optional |
| **Namen** | NER (Named Entity Recognition) | Mittel | Art. 6(1) | Optional |
| **Adressen** | Geo-Patterns | Mittel | Art. 6(1) | Optional |

### 1.5.2 PII-Verarbeitungszwecke

| Verarbeitung | Zweck | Rechtsgrundlage | Retention | Löschung |
|-------------|-------|-----------------|-----------|----------|
| **Benutzerkonto-Daten** | Authentifizierung, Zugangsverwaltung | Art. 6(1)(b) Vertragserfüllung | Kontolebenszeit + 90 Tage | Automatisch nach Retention |
| **Transaktionsdaten** | Geschäftsabwicklung, Audit | Art. 6(1)(b) Vertragserfüllung | 10 Jahre (HGB §257) | Nach gesetzlicher Frist |
| **Audit-Logs** | Sicherheit, Compliance | Art. 6(1)(f) Berechtigtes Interesse | 365 Tage | Automatisch nach Retention |
| **Zahlungsinformationen** | Payment Processing | Art. 6(1)(b) Vertragserfüllung | 30 Tage + gesetzliche Fristen | Nach Abschluss + Frist |
| **Gesundheitsdaten** | Anwendungsspezifisch | Art. 9(2)(a) Explizite Einwilligung | Anwendungsabhängig | Auf Widerruf |
| **Kommunikationsdaten** | Support, Customer Service | Art. 6(1)(b) Vertragserfüllung | 3 Jahre | Nach Retention |

### 1.5.3 Datenverarbeitungsaktivitäten (Verzeichnis)

| ID | Aktivität | Kategorien | Empfänger | Transfer | Risiko |
|----|-----------|-----------|-----------|----------|--------|
| **DPA-001** | Benutzerverwaltung | Identifikation, Kontakt | ThemisDB Betreiber | Lokal/EU | Mittel |
| **DPA-002** | Zahlungsverarbeitung | Finanzdaten, Identifikation | Payment Processor | EU/Drittland | Hoch |
| **DPA-003** | Audit-Logging | Verhaltensdaten, IP | Security Team | Lokal | Niedrig |
| **DPA-004** | Backup & Archivierung | Alle Kategorien | Backup Provider | EU/Drittland | Mittel |
| **DPA-005** | Gesundheitsanwendung | Gesundheitsdaten (Art. 9) | Anwendungsbetreiber | Lokal/EU | Sehr Hoch |
| **DPA-006** | Customer Analytics | Verhaltensdaten | Analytics Team | Lokal | Niedrig |

### 1.5.4 Datenschutz-Kontrollmechanismen

| Kontrolle | Implementierung | Status | Effektivität |
|-----------|----------------|--------|--------------|
| **PII-Detection** | `pii_detector.cpp` - 7 PII-Typen | ✅ Aktiv | Hoch |
| **Automatische Klassifizierung** | Governance Engine | ✅ Aktiv | Hoch |
| **Field-Level Encryption** | AES-256-GCM | ✅ Aktiv | Sehr Hoch |
| **Retention Policies** | Automatische Löschung | ✅ Aktiv | Hoch |
| **Access Logging** | Audit Trail für PII-Zugriffe | ✅ Aktiv | Hoch |
| **Pseudonymisierung** | Encryption als Pseudonymisierung | ✅ Verfügbar | Hoch |
| **Data Masking** | API-Level Masking | ⚠️ Optional | Mittel |

---

## 2. Notwendigkeits- und Verhältnismäßigkeitsprüfung

### 2.1 Rechtsgrundlage

| Art der Verarbeitung | Rechtsgrundlage | DSGVO Artikel |
|---------------------|-----------------|---------------|
| Vertragliche Speicherung | Vertragserfüllung | Art. 6(1)(b) |
| Sicherheitslogs | Berechtigtes Interesse | Art. 6(1)(f) |
| Compliance-Audit | Rechtliche Verpflichtung | Art. 6(1)(c) |
| Besondere Kategorien | Explizite Einwilligung | Art. 9(2)(a) |

### 2.2 Notwendigkeit der Verarbeitung

| Frage | Bewertung |
|-------|-----------|
| Ist die Datenverarbeitung für den Zweck notwendig? | ✅ Ja |
| Werden nur erforderliche Daten verarbeitet? | ✅ Ja (Datenminimierung) |
| Gibt es weniger eingriffsintensive Alternativen? | ✅ Keine bekannt |
| Ist die Speicherdauer auf das Notwendige begrenzt? | ✅ Ja (Retention Policies) |

### 2.3 Verhältnismäßigkeit

| Aspekt | Status | Maßnahme |
|--------|--------|----------|
| **Datenminimierung** | ✅ | PII-Detection, Schema-Validierung |
| **Speicherbegrenzung** | ✅ | Retention Policies (30-365 Tage) |
| **Zweckbindung** | ✅ | Governance-Klassifizierung |
| **Transparenz** | ✅ | Audit-Logging, Export-APIs |

---

## 3. Risikobewertung

### 3.1 Risiko-Matrix (FIND-021 Erweitert)

| ID | Risiko | Wahrscheinlichkeit | Auswirkung | Risikostufe | Risikoscore |
|----|--------|-------------------|------------|-------------|-------------|
| **R-001** | Datenverlust | Niedrig (2) | Hoch (4) | 🟡 Mittel | 8 |
| **R-002** | Unbefugter Zugriff | Mittel (3) | Hoch (4) | 🟠 Hoch | 12 |
| **R-003** | Datenmanipulation | Niedrig (2) | Hoch (4) | 🟡 Mittel | 8 |
| **R-004** | Fehlende Verschlüsselung | Sehr Niedrig (1) | Hoch (4) | 🟢 Niedrig | 4 |
| **R-005** | Unzureichende Löschung | Niedrig (2) | Mittel (3) | 🟢 Niedrig | 6 |
| **R-006** | Excessive Logging (PII) | Niedrig (2) | Mittel (3) | 🟢 Niedrig | 6 |
| **R-007** | Fehlende Zugriffskontrolle | Sehr Niedrig (1) | Sehr Hoch (5) | 🟡 Mittel | 5 |
| **R-008** | Datenleck bei Transfer | Niedrig (2) | Sehr Hoch (5) | 🟡 Mittel | 10 |
| **R-009** | Insider-Bedrohung | Niedrig (2) | Hoch (4) | 🟡 Mittel | 8 |
| **R-010** | Compliance-Verstoß | Niedrig (2) | Sehr Hoch (5) | 🟡 Mittel | 10 |

**Risikoscore-Berechnung:** Wahrscheinlichkeit (1-5) × Auswirkung (1-5)  
**Risikostufen:**
- 🔴 Sehr Hoch: 16-25 (sofortige Maßnahmen erforderlich)
- 🟠 Hoch: 10-15 (prioritäre Behandlung)
- 🟡 Mittel: 5-9 (überwachte Behandlung)
- 🟢 Niedrig: 1-4 (akzeptables Restrisiko)

### 3.2 Detaillierte Risikoanalyse

#### Risiko 1: Unbefugter Zugriff

| Aspekt | Bewertung |
|--------|-----------|
| **Beschreibung** | Unautorisierte Personen erhalten Zugriff auf personenbezogene Daten |
| **Quelle** | Externe Angreifer, Insider, kompromittierte Accounts |
| **Wahrscheinlichkeit** | Mittel |
| **Auswirkung** | Hoch (Datenschutzverletzung, Reputationsschaden) |
| **Betroffene Rechte** | Vertraulichkeit, Datenschutz |
| **Vorhandene Maßnahmen** | RBAC, mTLS, Audit-Logging |
| **Restrisiko** | Niedrig |

#### Risiko 2: Datenverlust

| Aspekt | Bewertung |
|--------|-----------|
| **Beschreibung** | Verlust von personenbezogenen Daten durch Systemfehler oder Angriffe |
| **Quelle** | Hardware-Ausfall, Ransomware, menschlicher Fehler |
| **Wahrscheinlichkeit** | Niedrig |
| **Auswirkung** | Hoch (Geschäftsunterbrechung, Compliance-Verstoß) |
| **Betroffene Rechte** | Verfügbarkeit, Integrität |
| **Vorhandene Maßnahmen** | RocksDB WAL, Checkpoints, Backup |
| **Restrisiko** | Niedrig |

#### Risiko 3: Übermäßige Datensammlung

| Aspekt | Bewertung |
|--------|-----------|
| **Beschreibung** | Mehr Daten werden gesammelt als für den Zweck notwendig |
| **Quelle** | Fehlkonfiguration, Feature Creep |
| **Wahrscheinlichkeit** | Niedrig |
| **Auswirkung** | Mittel (DSGVO-Verstoß) |
| **Betroffene Rechte** | Datenminimierung |
| **Vorhandene Maßnahmen** | PII-Detection, Governance-Policies |
| **Restrisiko** | Sehr Niedrig |

---

## 4. Technische und organisatorische Maßnahmen (TOMs)

### 4.1 Verschlüsselung

| Maßnahme | Status | Details |
|----------|--------|---------|
| **Verschlüsselung at-rest** | ✅ | AES-256-GCM für sensitive Felder |
| **Verschlüsselung in-transit** | ✅ | TLS 1.3, mTLS verfügbar |
| **Schlüsselmanagement** | ✅ | Vault/HSM-Integration, Key Rotation |
| **End-to-End Encryption** | ⚠️ | Anwendungsabhängig |

### 4.2 Zugriffskontrolle

| Maßnahme | Status | Details |
|----------|--------|---------|
| **Authentifizierung** | ✅ | Token-basiert, mTLS |
| **Autorisierung** | ✅ | RBAC (4-stufig: admin, operator, analyst, readonly) |
| **Least Privilege** | ✅ | Rollenbasierte Berechtigungen |
| **Session Management** | ✅ | Token-basiert mit Ablauf |

### 4.3 Datenintegrität

| Maßnahme | Status | Details |
|----------|--------|---------|
| **Transaktionsintegrität** | ✅ | ACID-Garantien, MVCC |
| **Audit Trail** | ✅ | Encrypt-then-Sign, Hash-Kette |
| **Input Validation** | ✅ | JSON Schema, Sanitization |
| **Backup-Integrität** | ✅ | RocksDB Checksums |

### 4.4 Verfügbarkeit

| Maßnahme | Status | Details |
|----------|--------|---------|
| **Backup** | ✅ | RocksDB Checkpoints, WAL-Archivierung |
| **Recovery** | ✅ | Point-in-Time Recovery |
| **Monitoring** | ✅ | Prometheus Metrics, Alerting |
| **Disaster Recovery** | ⚠️ | Dokumentiert, regelmäßige Tests empfohlen |

### 4.5 Datenschutz-spezifische Maßnahmen

| Maßnahme | Status | Details |
|----------|--------|---------|
| **PII-Detection** | ✅ | 7+ PII-Typen automatisch erkannt |
| **Datenklassifizierung** | ✅ | 4 Stufen (offen bis streng_geheim) |
| **Retention Policies** | ✅ | Automatische Löschung nach Ablauf |
| **Recht auf Löschung** | ✅ | DELETE API, Retention Manager |
| **Datenportabilität** | ✅ | JSON/CSV Export APIs |
| **Pseudonymisierung** | ✅ | Field-Level Encryption |

---

## 5. Betroffenenrechte

### 5.1 Umsetzung der Betroffenenrechte

| Recht | DSGVO | Status | Implementierung |
|-------|-------|--------|-----------------|
| **Auskunft** | Art. 15 | ✅ | GET /entities/{id}, Export API |
| **Berichtigung** | Art. 16 | ✅ | PUT /entities/{id} |
| **Löschung** | Art. 17 | ✅ | DELETE /entities/{id}, Retention Manager |
| **Einschränkung** | Art. 18 | ⚠️ | Über Governance-Flags möglich |
| **Datenübertragbarkeit** | Art. 20 | ✅ | JSON/CSV Export |
| **Widerspruch** | Art. 21 | ⚠️ | Anwendungsabhängig |
| **Automatisierte Entscheidungen** | Art. 22 | N/A | Keine KI-Entscheidungen in DB |

### 5.2 Reaktionszeiten

| Anfrage | Frist | Vorgehensweise |
|---------|-------|----------------|
| Auskunftsanfrage | 1 Monat | Export über API |
| Löschungsanfrage | 1 Monat | DELETE + Retention Check |
| Berichtigungsanfrage | 1 Monat | UPDATE über API |
| Datenschutzverletzung | 72 Stunden | IRP aktivieren |

---

## 6. Datentransfers

### 6.1 Geografische Transfers

| Transfer-Typ | Status | Maßnahmen |
|--------------|--------|-----------|
| **EU/EWR intern** | ✅ | Keine zusätzlichen Maßnahmen |
| **EU → Drittland (angemessen)** | ✅ | Angemessenheitsbeschluss |
| **EU → Drittland (nicht angemessen)** | ⚠️ | SCCs + TIA erforderlich |
| **Cloud-Hosting** | ⚠️ | Anbieterabhängig |

### 6.2 Auftragsverarbeiter

| Kategorie | Beispiel | Maßnahmen |
|-----------|----------|-----------|
| **Cloud-Provider** | AWS, Azure, GCP | AVV, SCCs |
| **Backup-Dienste** | S3, Tape | AVV, Verschlüsselung |
| **Monitoring** | Prometheus, Grafana | Datenminimierung |

---

## 7. Risiko-Minderung

### 7.1 Implementierte Maßnahmen

| # | Risiko | Maßnahme | Effektivität |
|---|--------|----------|--------------|
| 1 | Unbefugter Zugriff | RBAC + mTLS | ✅ Hoch |
| 2 | Datenverlust | Backup + WAL | ✅ Hoch |
| 3 | Datenleck | Encryption + Audit | ✅ Hoch |
| 4 | Compliance-Verstoß | Retention Policies | ✅ Hoch |
| 5 | Incident-Response | IRP dokumentiert | ✅ Mittel |

### 7.2 Empfohlene zusätzliche Maßnahmen

| # | Maßnahme | Priorität | Status |
|---|----------|-----------|--------|
| 1 | Penetrationstest | Hoch | ⚠️ Empfohlen |
| 2 | DLP-Integration | Mittel | 📋 Geplant |
| 3 | SIEM-Integration | Mittel | ⚠️ Empfohlen |
| 4 | Regelmäßige DR-Tests | Mittel | ⚠️ Empfohlen |

---

## 8. Konsultation

### 8.1 Interne Konsultation

| Stakeholder | Konsultiert | Feedback |
|-------------|-------------|----------|
| IT-Sicherheit | ✅ | Maßnahmen angemessen |
| Datenschutzbeauftragter | ✅ | DPIA erforderlich, Maßnahmen akzeptabel |
| Rechtsabteilung | ⚠️ | Ausstehend |
| Geschäftsführung | ⚠️ | Ausstehend |

### 8.2 Aufsichtsbehörde

| Frage | Antwort |
|-------|---------|
| Konsultation der Aufsichtsbehörde erforderlich? | ⚠️ Abhängig von Verarbeitungsart |
| Hohes Risiko nach Art. 36(1)? | Nein (mit implementierten Maßnahmen) |

---

## 9. Schlussfolgerung

### 9.1 Gesamtbewertung

| Aspekt | Bewertung |
|--------|-----------|
| **Datenschutzrisiken** | Niedrig bis Mittel |
| **Technische Maßnahmen** | ✅ Angemessen |
| **Organisatorische Maßnahmen** | ⚠️ Teilweise (abhängig von Betreiber) |
| **Compliance-Status** | ✅ DSGVO-konform |

### 9.2 Ergebnis

**✅ Die Verarbeitung kann unter Einhaltung der dokumentierten Maßnahmen durchgeführt werden.**

Die identifizierten Risiken werden durch die implementierten technischen und organisatorischen Maßnahmen angemessen adressiert. Das Restrisiko ist akzeptabel.

### 9.3 Auflagen

1. Regelmäßige Überprüfung dieser DPIA (mindestens jährlich)
2. Aktualisierung bei wesentlichen Änderungen der Verarbeitung
3. Implementierung der empfohlenen zusätzlichen Maßnahmen
4. Dokumentation der Auftragsverarbeitungsverträge

---

## 10. Unterschriften

| Rolle | Name | Datum | Unterschrift |
|-------|------|-------|--------------|
| **Verantwortlicher** | [Name] | [Datum] | _____________ |
| **Datenschutzbeauftragter** | [Name] | [Datum] | _____________ |
| **IT-Sicherheitsbeauftragter** | [Name] | [Datum] | _____________ |

---

## 11. Anhänge

### A. Referenzdokumente

| Dokument | Pfad |
|----------|------|
| Audit-Checkliste | `docs/FULL_AUDIT_CHECKLIST.md` |
| Security Audit Report | `docs/reports/SECURITY_AUDIT_REPORT.md` |
| Incident Response Plan | `docs/security/INCIDENT_RESPONSE_PLAN.md` |
| SBOM | `docs/security/SBOM.md` |
| Security Policy | `SECURITY.md` |

### B. Änderungshistorie

| Version | Datum | Autor | Änderungen |
|---------|-------|-------|------------|
| 1.0 | November 2025 | ThemisDB Team | Erstversion |

---

**Letzte Aktualisierung:** November 2025  
**Dokumentverantwortlicher:** ThemisDB Datenschutz Team  
**Nächstes Review:** [Datum + 12 Monate]
