# ThemisDB – Change Management Policy

**Version:** 1.0  
**Stand:** 6. April 2026  
**Klassifizierung:** Intern  
**Basis:** ISO 27001 (A.12.1.2), ITIL v4, BSI C5 (OPS-03)

---

## 📋 Übersicht

Diese Change Management Policy definiert die Prozesse zur kontrollierten Einführung von Änderungen an ThemisDB-Systemen, um Risiken zu minimieren, Stabilität zu gewährleisten und Compliance-Anforderungen zu erfüllen.

---

## 1. Grundsätze

### 1.1 Ziele

| Ziel | Beschreibung |
|------|--------------|
| **Stabilität** | Änderungen dürfen den Betrieb nicht gefährden |
| **Nachvollziehbarkeit** | Alle Änderungen sind dokumentiert und auditierbar |
| **Minimierung von Risiken** | Risiken werden vor Implementierung bewertet |
| **Effizienz** | Schnelle Umsetzung wichtiger Änderungen |
| **Compliance** | Einhaltung regulatorischer Anforderungen |

### 1.2 Anwendungsbereich

| Bereich | Abgedeckt |
|---------|-----------|
| Produktions-Datenbank | ✅ |
| Staging/Test-Umgebungen | ✅ (vereinfacht) |
| Infrastruktur | ✅ |
| Konfiguration | ✅ |
| Code-Änderungen | ✅ |
| Dokumentation | ⚠️ Vereinfacht |

---

## 2. Änderungskategorien

### 2.1 Klassifizierung

| Kategorie | Beschreibung | Genehmigung | Vorlaufzeit |
|-----------|--------------|-------------|-------------|
| **Standard** | Wiederholbare, risikoarme Änderungen | Pre-approved | < 24h |
| **Normal** | Geplante Änderungen mit bekanntem Risiko | CAB | 5 Werktage |
| **Major** | Signifikante Änderungen mit hohem Risiko | CAB + Management | 10 Werktage |
| **Emergency** | Kritische Fixes bei Incidents | Emergency CAB | Sofort |

### 2.2 Beispiele je Kategorie

| Kategorie | Beispiele |
|-----------|-----------|
| **Standard** | Patch-Updates, Konfiguration (Minor), Log-Rotation |
| **Normal** | Feature-Releases, Schema-Änderungen, Dependency-Updates |
| **Major** | Major Versions, Architektur-Änderungen, Migration |
| **Emergency** | Sicherheits-Patches, Hotfixes, Incident-Behebung |

---

## 3. Change-Prozess

### 3.1 Prozessübersicht

```
┌───────────────┐
│  1. Request   │
│   erstellen   │
└───────┬───────┘
        ▼
┌───────────────┐
│  2. Bewertung │
│  & Priorisierung│
└───────┬───────┘
        ▼
┌───────────────┐     ┌───────────────┐
│ 3. Genehmigung├─No──▶│   Abgelehnt   │
│     (CAB)     │     └───────────────┘
└───────┬───────┘
        │Yes
        ▼
┌───────────────┐
│  4. Planung   │
│ & Scheduling  │
└───────┬───────┘
        ▼
┌───────────────┐
│ 5. Implementierung│
│    & Test     │
└───────┬───────┘
        ▼
┌───────────────┐     ┌───────────────┐
│  6. Review    ├─Fail─▶│   Rollback    │
│               │     └───────────────┘
└───────┬───────┘
        │Success
        ▼
┌───────────────┐
│  7. Abschluss │
│   & Doku      │
└───────────────┘
```

### 3.2 Phase 1: Request

| Feld | Erforderlich | Beschreibung |
|------|:------------:|--------------|
| Titel | ✅ | Kurzbeschreibung der Änderung |
| Beschreibung | ✅ | Detaillierte Änderungsbeschreibung |
| Begründung | ✅ | Warum ist die Änderung notwendig? |
| Betroffene Systeme | ✅ | Liste der betroffenen Komponenten |
| Risikobewertung | ✅ | Impact und Wahrscheinlichkeit |
| Rollback-Plan | ✅ | Wie wird die Änderung zurückgesetzt? |
| Test-Plan | ✅ | Wie wird die Änderung validiert? |
| Zeitfenster | ✅ | Geplanter Implementierungszeitraum |
| Antragsteller | ✅ | Wer beantragt die Änderung? |
| Implementierer | ⚠️ | Wer führt die Änderung durch? |

### 3.3 Phase 2: Bewertung

#### Risiko-Matrix für Änderungen

| Impact \ Likelihood | Niedrig | Mittel | Hoch |
|---------------------|:-------:|:------:|:----:|
| **Hoch** | Normal | Major | Major |
| **Mittel** | Standard | Normal | Major |
| **Niedrig** | Standard | Standard | Normal |

#### Bewertungskriterien

| Kriterium | Niedrig | Mittel | Hoch |
|-----------|---------|--------|------|
| **Betroffene Nutzer** | < 10 | 10-100 | > 100 |
| **Downtime** | 0 | < 1h | > 1h |
| **Datenverlust-Risiko** | Nein | Möglich | Wahrscheinlich |
| **Rollback-Komplexität** | Einfach | Mäßig | Komplex |
| **Abhängigkeiten** | Keine | Wenige | Viele |

### 3.4 Phase 3: Genehmigung

#### Change Advisory Board (CAB)

| Rolle | Stimmberechtigt | Erforderlich für |
|-------|:---------------:|------------------|
| **IT-Leitung** | ✅ | Major |
| **Development Lead** | ✅ | Alle |
| **Operations Lead** | ✅ | Alle |
| **Security** | ✅ | Security-relevant |
| **QA** | ⚠️ | Beratend |
| **Business Owner** | ⚠️ | Bei Business Impact |

#### Emergency CAB (E-CAB)

Für Emergency Changes:
- Mindestens 2 Personen erforderlich
- Telefonische Genehmigung erlaubt
- Dokumentation innerhalb 24h nachreichen
- Retrospektive Genehmigung durch volles CAB

### 3.5 Phase 4: Planung

| Aspekt | Anforderung |
|--------|-------------|
| **Zeitfenster** | Außerhalb Geschäftszeiten (Standard: So 02:00-06:00) |
| **Kommunikation** | Stakeholder 48h vorher informieren |
| **Backup** | Vor jeder Major/Normal Change |
| **Ressourcen** | Team-Verfügbarkeit sicherstellen |
| **Checkliste** | Pre-Implementation Checklist abarbeiten |

### 3.6 Phase 5: Implementierung

#### Pre-Implementation Checklist

- [ ] Backup erstellt und verifiziert
- [ ] Rollback-Prozedur getestet
- [ ] Monitoring aktiv
- [ ] Team erreichbar
- [ ] Kommunikation erfolgt
- [ ] Test-Umgebung validiert

#### Implementierungs-Regeln

| Regel | Beschreibung |
|-------|--------------|
| **Vier-Augen-Prinzip** | Mindestens 2 Personen für Major Changes |
| **Schrittweise** | Inkrementelle Implementierung wenn möglich |
| **Monitoring** | Aktive Überwachung während Implementierung |
| **Dokumentation** | Jeden Schritt protokollieren |

### 3.7 Phase 6: Review

#### Post-Implementation Checklist

- [ ] Alle Änderungen erfolgreich angewandt
- [ ] Systeme funktionieren wie erwartet
- [ ] Monitoring zeigt keine Anomalien
- [ ] Performance ist akzeptabel
- [ ] Keine unerwarteten Fehler in Logs
- [ ] Stakeholder bestätigen Funktionalität

#### Rollback-Trigger

| Trigger | Aktion |
|---------|--------|
| Kritischer Fehler | Sofortiger Rollback |
| Performance-Degradation > 50% | Rollback oder Hotfix |
| Datenverlust | Sofortiger Rollback + Incident |
| Security-Vulnerability | Rollback + Security-Incident |

### 3.8 Phase 7: Abschluss

| Aktivität | Frist |
|-----------|-------|
| Dokumentation finalisieren | < 24h |
| Stakeholder informieren | < 24h |
| Lessons Learned (bei Problemen) | < 1 Woche |
| Change Record schließen | < 24h |

---

## 4. Change-Typen im Detail

### 4.1 Code-Änderungen

```
Feature Branch ──▶ Pull Request ──▶ Code Review ──▶ Tests ──▶ Merge ──▶ Deploy
                                        │
                                        ▼
                                   Security Review
                                   (bei kritischen Änderungen)
```

| Schritt | Anforderung |
|---------|-------------|
| **Pull Request** | Beschreibung, Tests, Breaking Changes |
| **Code Review** | Mindestens 1 Reviewer (2 für kritisch) |
| **Tests** | Alle Tests müssen bestehen |
| **Security Review** | Bei Auth, Crypto, Input-Handling |
| **Deployment** | Staging vor Production |

### 4.2 Infrastruktur-Änderungen

| Änderung | Kategorie | Zusätzliche Anforderungen |
|----------|-----------|---------------------------|
| OS-Patches | Standard | Staging-Test |
| Netzwerk-Config | Normal | CAB-Genehmigung |
| Hardware-Tausch | Major | Downtime-Fenster |
| Cloud-Migration | Major | Ausführlicher Plan |

### 4.3 Konfigurations-Änderungen

| Änderung | Kategorie | Anforderung |
|----------|-----------|-------------|
| Logging-Level | Standard | Pre-approved |
| Ressourcen-Limits | Normal | CAB |
| Security-Parameter | Normal+ | Security-Review |
| Encryption-Settings | Major | Security + CAB |

### 4.4 Datenbank-Änderungen

| Änderung | Kategorie | Besonderheiten |
|----------|-----------|----------------|
| Index hinzufügen | Standard | Online möglich |
| Schema erweitern (additiv) | Normal | Migration Script |
| Schema ändern (breaking) | Major | Migration + Downtime |
| Daten-Migration | Major | Backup + Validierung |

---

## 5. Notfall-Änderungen (Emergency)

### 5.1 Kriterien

Emergency Changes sind nur erlaubt bei:
- Aktiver Security-Incident
- Systemausfall in Produktion
- Kritischer Datenverlust droht
- Compliance-Verletzung

### 5.2 Verfahren

1. **Mündliche Genehmigung** von E-CAB (min. 2 Personen)
2. **Sofortige Implementierung** mit Monitoring
3. **Dokumentation** innerhalb 24h
4. **Retrospektive CAB-Genehmigung** in nächster Sitzung
5. **Root Cause Analysis** innerhalb 1 Woche

### 5.3 Dokumentation

| Feld | Erforderlich |
|------|:------------:|
| Incident-Referenz | ✅ |
| Genehmiger (Name, Zeit) | ✅ |
| Implementierer | ✅ |
| Durchgeführte Änderungen | ✅ |
| Ergebnis | ✅ |
| Follow-up-Aktionen | ✅ |

---

## 6. Kommunikation

### 6.1 Kommunikationsplan

| Wann | Was | Wer | An wen |
|------|-----|-----|--------|
| -5 Tage | Change-Ankündigung | Change Owner | Stakeholder |
| -48h | Reminder | Operations | Betroffene Nutzer |
| -1h | Start-Benachrichtigung | Operations | Team |
| Nach Abschluss | Abschluss-Meldung | Change Owner | Alle |
| Bei Problemen | Status-Updates | Operations | Management |

### 6.2 Vorlagen

#### Change-Ankündigung

```
BETREFF: [Geplante Wartung] ThemisDB - [Beschreibung]

Datum: [Datum]
Zeit: [Beginn] - [Ende] (geschätzt)
Betroffene Systeme: [Liste]
Auswirkungen: [Beschreibung]

Beschreibung:
[Was wird geändert]

Bei Fragen wenden Sie sich an: [Kontakt]
```

---

## 7. Metriken und Reporting

### 7.1 KPIs

| KPI | Ziel | Messung |
|-----|------|---------|
| **Change Success Rate** | > 95% | Erfolgreiche / Gesamt |
| **Emergency Change Rate** | < 10% | Emergency / Gesamt |
| **Rollback Rate** | < 5% | Rollbacks / Gesamt |
| **Mean Time to Implement** | < 5 Tage | Request bis Abschluss |
| **CAB Approval Time** | < 3 Tage | Request bis Genehmigung |

### 7.2 Reporting

| Report | Frequenz | Empfänger |
|--------|----------|-----------|
| Change Summary | Wöchentlich | Operations |
| Change Metrics | Monatlich | Management |
| Failed Changes | Ad-hoc | CAB |
| Trend Analysis | Vierteljährlich | IT-Leitung |

---

## 8. Rollen und Verantwortlichkeiten

| Rolle | Verantwortlichkeiten |
|-------|---------------------|
| **Change Owner** | Antrag, Koordination, Abschluss |
| **CAB** | Bewertung, Genehmigung, Überwachung |
| **Change Manager** | Prozess-Owner, Reporting, Verbesserung |
| **Implementierer** | Technische Umsetzung |
| **Operations** | Monitoring, Kommunikation |
| **Security** | Security-Review, Genehmigung |

---

## 9. Compliance-Mapping

| Standard | Anforderung | Erfüllt |
|----------|-------------|:-------:|
| **ISO 27001** | A.12.1.2 Change Management | ✅ |
| **BSI C5** | OPS-03, OPS-04 | ✅ |
| **SOC 2** | CC8.1 Change Management | ✅ |
| **ITIL v4** | Change Enablement | ✅ |

---

## 10. Anhänge

### A. Change Request Template

```yaml
change_request:
  id: CR-YYYY-NNNN
  title: "[Kurztitel]"
  category: [Standard|Normal|Major|Emergency]
  
  requester:
    name: "[Name]"
    date: "[Datum]"
    
  description:
    summary: "[Zusammenfassung]"
    detail: "[Detailbeschreibung]"
    justification: "[Begründung]"
    
  impact:
    systems: ["System1", "System2"]
    users: [Anzahl]
    downtime: "[Geschätzte Downtime]"
    
  risk:
    level: [Low|Medium|High]
    mitigation: "[Risikominderung]"
    
  rollback:
    plan: "[Rollback-Schritte]"
    time: "[Geschätzte Zeit]"
    
  schedule:
    preferred_date: "[Datum]"
    window: "[Zeitfenster]"
    
  approval:
    cab_date: "[Datum]"
    approvers: ["Name1", "Name2"]
    decision: [Approved|Rejected|Deferred]
    
  implementation:
    date: "[Datum]"
    implementer: "[Name]"
    result: [Success|Failed|Rollback]
    
  closure:
    date: "[Datum]"
    lessons_learned: "[Falls zutreffend]"
```

### B. Referenzen

| Dokument | Pfad |
|----------|------|
| Incident Response Plan | `docs/security/INCIDENT_RESPONSE_PLAN.md` |
| BCP/DRP | `docs/compliance/BCP_DRP.md` |
| Deployment Guide | `docs/guides/deployment.md` |
| Risk Register | `docs/compliance/RISK_REGISTER.md` |

---

**Letzte Aktualisierung:** November 2025  
**Dokumentverantwortlicher:** ThemisDB Operations  
**Nächstes Review:** [Datum + 12 Monate]
