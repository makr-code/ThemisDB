# ThemisDB — EU AI Act Compliance & Risk Assessment

**Datum:** 2026-08-08  
**Version:** 1.0  
**Scope:** ThemisDB v2.4.0-rc1 – AI/LLM-Komponenten und automatisierte Entscheidungssysteme  
**Standard:** Verordnung (EU) 2024/1689 (EU AI Act) — Risiko-Klassifizierung und Transparenzanforderungen

> **Hinweis**: Das EU AI Act gilt ab 2026 für High-Risk-Systeme. Diese Compliance-Bewertung dokumentiert ThemisDB's Vorbereitung auf die Anforderungen.

---

## 1. Überblick: KI-Komponenten in ThemisDB

ThemisDB integriert folgende KI/LLM-Funktionen, die unter das EU AI Act fallen können:

| Komponente | Modul | Funktion | Risk-Level (Preliminary) |
|------------|-------|---------|-------------------------|
| **LLM Integration** | `src/llm/`, `src/rag/` | Textgenerierung, Semantic Search, RAG-Pipeline | 🟡 Medium-High |
| **Graph Reasoning** | `src/graph/` | Constraint-Solving, Path-Finding, Inference | 🟡 Medium |
| **Vector Search & ANN** | `src/index/`, `src/search/` | Semantic Similarity Matching, Nearest-Neighbor Retrieval | 🟠 Medium |
| **Automated Anomaly Detection** | `src/observability/`, `src/analytics/` | Pattern Detection, Threshold Violations | 🟡 Medium |
| **Query Optimization via ML** | `src/query/`, `src/performance/` | Adaptive Query Planning, Workload Prediction | 🟢 Low-Medium |
| **Governance & Policy Enforcement** | `src/governance/` | Automated Access Control, Data Masking Decisions | 🔴 High (wenn automatisiert) |
| **AI Decision Auditing** | `src/security/`, `src/utils/` | Audit Trails, Impact Logging | 🟢 Low (Mitigations) |

---

## 2. EU AI Act Risk Classification

Gemäß EU AI Act §4 werden KI-Systeme klassifiziert als:

### 2.1 **Verbotene KI-Praktiken (Art. 5-6)** ❌

| Kategorie | Implementierung in ThemisDB | Status |
|-----------|---------------------------|--------|
| Subliminal Manipulation | Nicht vorhanden | ✅ **NICHT ANWENDBAR** |
| Social Credit Scoring | Nicht vorhanden | ✅ **NICHT ANWENDBAR** |
| Real-time Biometric Identification (außer Law Enforcement) | Nicht vorhanden | ✅ **NICHT ANWENDBAR** |
| Diskriminierung basierend auf Kategorien | **Governance-Module mit Masking** | 🟡 **MITIGIERT** (siehe §4.3) |

**Fazit:** ThemisDB enthält keine verbotenen KI-Praktiken.

### 2.2 **High-Risk KI-Systeme (Art. 6-52)** 🔴

High-Risk-Klassifizierung gemäß EU AI Act Annex III trifft auf folgende Szenarien zu:

#### Szenario A: **Automatisierte Biometrische Identifikation**
- **Anwendbar?** ❌ Nein — ThemisDB ist eine Datenbank, keine Gesichtserkennungs-/Biometrieplatform

#### Szenario B: **Kritische Infrastruktur & Cyberüberwachung**
- **Anwendbar?** 🟡 **TEILWEISE** — Wenn ThemisDB zur Netzwerküberwachung oder Anomalieerkennung verwendet wird
- **Mitigation:** Transparenzanforderungen (§4.3), Audit Trails

#### Szenario C: **Bildung & Berufsausbildung**
- **Anwendbar?** ❌ Nein — ThemisDB stellt keine Learning Analytics bereit
- **Ausnahme:** Falls Custom-Plugins für Edu-Analytics verwendet werden → dann High-Risk

#### Szenario D: **Beschäftigung & Arbeitsmarkt**
- **Anwendbar?** 🟡 **OPTIONAL** — Wenn LLM-Integration für Kandidaten-Scoring genutzt wird
- **Beispiel:** Automatisierte Recruiting-Queries via LLM
- **Mitigation:** Explizite Opt-in, Transparenzberichte

#### Szenario E: **Strafrecht & Polizei**
- **Anwendbar?** 🟡 **OPTIONAL** — Wenn für Predictive Policing/Risk Assessment genutzt wird
- **Mitigation:** Audit Trails, Transparenz (§4.3), Menschliche Aufsicht

#### Szenario F: **Ziviles Rechtssystem & Verwaltung**
- **Anwendbar?** 🟡 **OPTIONAL** — Wenn für automatisierte Verwaltungsentscheidungen genutzt wird
- **Mitigation:** Logging, Audit Trails, Recht auf Erklärung

#### Szenario G: **Kreditvergabe & Versicherung**
- **Anwendbar?** 🟡 **OPTIONAL** — Wenn Credit/Insurance-Scoring via Query/LLM erfolgt
- **Mitigation:** Explizite Consent, Audit Trails, Erklärbarkeit

#### Szenario H: **Sicherheit essentieller Infrastruktur**
- **Anwendbar?** 🟡 **OPTIONAL** — Wenn für kritische Systeme (Energie, Verkehr) verwendet
- **Mitigation:** High-Assurance Testing, Audit Trails

### 2.3 **Limited-Risk KI-Systeme (Art. 50-51)** 🟡

Transparenzanforderungen für Chatbots, LLM-Integration:

| Anforderung | ThemisDB-Status | Mitigationen |
|-------------|-----------------|--------------|
| Disclosure of AI use | 🟡 **TEILWEISE** | `src/utils/ai_decision_auditing.cpp` logs KI-Einsatz |
| Fact-Checking & Falsifiability | 🟡 **TEILWEISE** | RAG integriert mit Knowledge-Bases |
| Human Oversight | 🟠 **UNVOLLSTÄNDIG** | Manueller Review erforderlich per Deployment |
| Explainability | 🟡 **TEILWEISE** | Scoring-Gründe dokumentierbar |

---

## 3. Gap-Analyse: ThemisDB vs. EU AI Act Anforderungen

### 3.1 **Anforderung: High-Risk AISMA (Art. 6-52)**

| Anforderung | Artikel | ThemisDB-Status | Maßnahmen |
|-------------|---------|-----------------|-----------|
| Risk Assessment | 19 | 🟡 **TEILWEISE** | `src/governance/AI_ML_IMPACT_ASSESSMENT.md` vorhanden, aber nicht EU AI Act strukturiert |
| Quality & Robustness | 15 | 🟢 **JA** | Testing (Wave 6-9), Sanitizer-Evidenz |
| Human Oversight | 26 | 🟡 **OPTIONAL** | Deployment-dependent |
| Transparency & Documentation | 13, 28-29 | 🟠 **UNVOLLSTÄNDIG** | Modellkarten fehlen, Audit-Trails vorhanden |
| Record Keeping | 12 | 🟢 **JA** | Audit-Logger integriert |
| Cybersecurity & Robustness Monitoring | 15, 24 | 🟢 **JA** | CodeQL, Sanitizer, Pentest-Evidence |

### 3.2 **Anforderung: Transparency für Limited-Risk (Art. 50-51)**

| Anforderung | ThemisDB-Status | Maßnahmen |
|-------------|-----------------|-----------|
| AI-Einsatz offenbart | 🟡 **TEILWEISE** | Log-Markierung: `[AI_DECISION]` in Audit-Logs |
| Konfidenzwert dokumentiert | 🟡 **TEILWEISE** | RAG-Ranking-Scores vorhanden |
| Input-Output Transparenz | 🟡 **OPTIONAL** | Deployment-dependent |
| Erklärbarkeit für Nutzer | 🟠 **UNVOLLSTÄNDIG** | Requires Custom Implementation |

### 3.3 **Anforderung: Governance & Compliance** 

| Anforderung | ThemisDB-Status | Maßnahmen |
|-------------|-----------------|-----------|
| Compliance Policy | 🟡 **TEILWEISE** | `docs/de/compliance/` vorhanden |
| Risikomanagement | 🟢 **JA** | `docs/security/RISK_MANAGEMENT_FRAMEWORK.md` |
| Incident Response | 🟢 **JA** | Security Runbooks vorhanden |
| Audit-Dokumentation | 🟢 **JA** | Zentral in `audit/` |

---

## 4. Implementierungsstatus pro KI-Modul

### 4.1 **LLM-Modul (`src/llm/`)**

**Risk Level:** 🔴 **High** (Wenn automatisierte Entscheidungen treffen)

| Anforderung | Status | Evidenz |
|-------------|--------|---------|
| Input Validation | ✅ | Injection Detection: `prompt_engineering/` |
| Output Monitoring | 🟡 | Token-Limits, aber keine Fakten-Prüfung |
| Audit Logging | ✅ | `utils/ai_decision_auditing.cpp` |
| Fallback-Strategie | 🟡 | Teilweise vorhanden; See LLM Failover Patterns |
| Version Control | ✅ | Model Manifest Versioning |
| Testing | ✅ | Tests in `tests/llm/` (Wave 8+) |

**Gaps:**
- [ ] Model Card / Transparenz-Dokumentation
- [ ] Explainability Report (Warum wählte LLM diese Antwort?)
- [ ] Fact-Checking Integration (Validierung gegen Knowledge-Base)
- [ ] EU AI Act Compliance Checklist im Deployment

---

### 4.2 **RAG-Modul (`src/rag/`)**

**Risk Level:** 🟡 **Medium** (Retrievale ist meist deterministic)

| Anforderung | Status | Evidenz |
|-------------|--------|---------|
| Source Attribution | ✅ | Retrieval mit Source-IDs |
| Hallucination Prevention | 🟡 | Knowledge-Base-begrenzt, aber LLM kann noch halluzinieren |
| Audit Logging | ✅ | Retrieved Docs + Ranking Scores |
| Transparency | 🟡 | Scores sichtbar, aber nicht standardisiert |
| Testing | ✅ | Hybrid-Retrieval Tests |

**Gaps:**
- [ ] Consistency Checks (Redundante Retrieval-Queries)
- [ ] Confidence Scoring dokumentiert
- [ ] Human-Review-Workflows

---

### 4.3 **Graph Reasoning (`src/graph/`)**

**Risk Level:** 🟡 **Medium** (Meist deterministic, aber komplexe Inferenzen)

| Anforderung | Status | Evidenz |
|-------------|--------|---------|
| Rule Transparency | ✅ | Graph-Constraints dokumentiert |
| Audit Trails | ✅ | Path/Reasoning-Logs |
| Determinism | ✅ | Constraint-Solving ist deterministisch |
| Testing | ✅ | 31 Test-Dateien |

**Gaps:**
- [ ] Explainability (Warum wurde Constraint X angewandt?)
- [ ] Conflict Resolution Docs

---

### 4.4 **Governance-Modul (`src/governance/`)**

**Risk Level:** 🔴 **High** (Automatisierte Access-Control Entscheidungen)

| Anforderung | Status | Evidenz |
|-------------|--------|---------|
| Policy Definition | ✅ | OPA-Integration vorhanden |
| Audit Logging | ✅ | Alle Decisions geloggt |
| Override/Appeal | 🟡 | Manueller Override möglich |
| Transparency | 🟡 | Log-vorhanden, aber nicht benutzerfreundlich |
| Testing | ✅ | Compliance Tests vorhanden |

**Gaps:**
- [ ] User-facing Transparency (Warum wurde Zugriff verweigert?)
- [ ] Appeal Process dokumentiert
- [ ] Bias-Audits für Policy-Regeln

---

## 5. Maßnahmen zur EU AI Act Compliance (Roadmap)

### Priority 1: **Immediately Required** (for High-Risk Deployments)

- [ ] **Audit Trail Compliance** — Alle KI-Entscheidungen mit Timestamp, Input, Output, Confidence logging (bereits teilweise vorhanden)
  - Status: 🟢 Code: `src/security/ai_snapshot_cleanup.h`, `src/utils/ai_decision_auditing.cpp`
  - Roadmap: Formale Integration in Release v2.5.0

- [ ] **Model Card & Transparency Report** — Dokumentation von Modellen, Trainings-Daten, Limitations
  - Status: 🔴 NICHT VORHANDEN
  - Roadmap: Q3 2026 — Add `docs/model-cards/` für alle LLM-Integrationen

- [ ] **Human Oversight & Appeal** — Dokumentierte Prozesse für High-Risk-Systeme
  - Status: 🟡 TEILWEISE
  - Roadmap: Q3 2026 — Document in Governance Module

### Priority 2: **Should Have** (for Certified Deployments)

- [ ] **Explainability & Interpretability** — Tools zur Erklärung von KI-Entscheidungen
  - Status: 🟡 TEILWEISE (Audit-Logs vorhanden)
  - Roadmap: Q4 2026 — Add LIME/SHAP Integration

- [ ] **Bias & Fairness Audits** — Regelmäßige Überprüfung auf Diskriminierung
  - Status: 🟠 UNVOLLSTÄNDIG
  - Roadmap: Q4 2026 — Add Fairness Test Suite

- [ ] **Data Governance & Provenance** — Dokumentation von Trainingsdaten
  - Status: 🟢 TEILWEISE (Metadata module)
  - Roadmap: Q4 2026 — Enhance for LLM Training Data

### Priority 3: **Nice to Have** (for Excellence)

- [ ] **Continuous Monitoring Dashboard** — Real-time KI-Performance Tracking
  - Status: 🟡 TEILWEISE (Observability vorhanden)
  - Roadmap: Q1 2027

- [ ] **Automated Fairness Tests** — CI/CD Gate für Bias-Detection
  - Status: 🔴 NICHT VORHANDEN
  - Roadmap: Q1 2027

---

## 6. Evidence Bundle

Folgende Artefakte dokumentieren die EU AI Act Compliance:

### Sicherheit & Testing
- `docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md` — Sanitizer-Tests für KI-Module
- `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md` — Penetration Testing
- `tests/llm/` — LLM-spezifische Tests (Wave 8+)
- `benchmarks/` — Performance-Charakterisierung

### Audit & Logging
- `src/utils/ai_decision_auditing.cpp` — KI-Entscheidungs-Logging
- `src/security/audit_logger.cpp` — Zentrale Audit-Log Infrastruktur
- `audit/` — Zentrale Audit-Reports

### Governance & Policy
- `docs/de/compliance/compliance_full_checklist.md` — Compliance-Status
- `src/governance/AI_ML_IMPACT_ASSESSMENT.md` — Impact Assessment
- `docs/security/RISK_MANAGEMENT_FRAMEWORK.md` — Risikomanagement

### Dokumentation
- `docs/de/compliance/` — Compliance-Framework
- `ROADMAP.md` — Feature-Roadmap mit Phasen
- `src/*/ROADMAP.md` — Modul-spezifische Roadmaps

---

## 7. Deployment Checklist für EU AI Act Compliance

Vor dem Produktiveinsatz von ThemisDB mit High-Risk KI-Features (LLM, Governance):

- [ ] **Risk Classification** — Klassifizieren Sie Ihren Use-Case (Verboten/High-Risk/Limited/Minimal)
- [ ] **Audit Trail Setup** — Aktivieren Sie AI Decision Logging
- [ ] **Transparency Documentation** — Veröffentlichen Sie Model Cards & Impact Assessments
- [ ] **Human Oversight** — Definieren Sie Approval-Workflows für kritische Entscheidungen
- [ ] **Appeal Process** — Dokumentieren Sie, wie Nutzer Entscheidungen anfechten können
- [ ] **Testing** — Führen Sie Fairness/Bias Tests durch
- [ ] **Compliance Review** — Externe Audit vor Release
- [ ] **Monitoring** — Kontinuierliche Performance & Fairness Überwachung

---

## 8. Regulatorische Konformität Zusammenfassung

| Standard | Konformität | Evidence | Verantwortlich |
|----------|-------------|----------|----------------|
| **EU AI Act** | 🟡 **Conditional** (High-Risk Scenarios) | This Document + Evidence Bundle | Security/Governance |
| **GDPR** | ✅ **Compliant** | `docs/de/compliance/compliance_dpia.md` | Legal/Security |
| **BSI C5** | ✅ **Compliant** | `audit/BSI_C5_2026_THEMISDB_AUDIT.md` | Security |
| **ISO 27001** | ✅ **Compliant** | `docs/de/compliance/compliance_full_checklist.md` | Security |
| **NIS2** | ✅ **Compliant** | `docs/de/compliance/compliance_bcp_drp.md` | Security |

---

## 9. Kontakt & Fragen

Für Fragen zur EU AI Act Compliance:
- **Governance Module Owner**: siehe `src/governance/CODEOWNERS`
- **Security Lead**: siehe `SECURITY.md`
- **Compliance Officer**: siehe `docs/de/compliance/README.md`

---

**Dokument-Versionsgeschichte:**
- v1.0 (2026-08-08): Initiale Compliance-Bewertung basierend auf v2.4.0-rc1
