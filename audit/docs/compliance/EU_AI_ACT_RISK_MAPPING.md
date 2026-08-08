# ThemisDB — EU AI Act Risk Mapping & Classification

**Datum:** 2026-08-08  
**Version:** 1.0  
**Scope:** Detaillierte Risikoklassifizierung aller KI-Komponenten nach EU AI Act Annex III  
**Zielgruppe:** Compliance Officers, System Designers, Deployment Teams

---

## 1. Klassifizierungs-Framework

Gemäß EU AI Act werden KI-Systeme in 4 Risikostufen eingeteilt:

| Risk Level | Anforderungen | Zertifizierung | ThemisDB Module |
|-----------|--------------|----------------|-----------------|
| **VERBOTEN** | Nicht zulässig | N/A | Keine ✅ |
| **HIGH** 🔴 | Strikte Compliance, Audit, Human Oversight | Externe Zertifizierung | Governance (optional), LLM (optional) |
| **LIMITED** 🟡 | Transparenzanforderungen | Self-Assessment | LLM, RAG, Anomaly Detection |
| **MINIMAL/GENERAL** 🟢 | Standard Compliance | Self-Assessment | Index, Storage, Query, Network |

---

## 2. Detailliertes Modul-Mapping

### 2.1 CORE DATABASE MODULES

#### **server** (86.167 LOC, 🟢 **MINIMAL RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| HTTP/gRPC Handler | Request Routing | 🟢 Min | Standardprotokoll, keine KI | Standard Security |
| Rate Limiting | Request Throttling | 🟢 Min | Stateless, regelbasiert | Algorithmic |
| Authentication | Token Verification | 🟢 Min | Kryptographisch | BSI C5 |

**Fazit:** ✅ KONFORM — Keine KI-Komponenten

---

#### **storage** (36.841 LOC, 🟢 **MINIMAL RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| MVCC Engine | Concurrency Control | 🟢 Min | Deterministisch | Formal Verification |
| WAL/Backup | Persistence | 🟢 Min | Regelbasiert | Testing |
| Tiering | Data Movement | 🟡 Limited | Heuristische Entscheidungen (Zugriffsmuster) | Config-Driven |

**Fazit:** ✅ KONFORM — Minimal KI-Einsatz, Config-steuerbar

---

#### **query** (40.861 LOC, 🟡 **LIMITED RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| Optimizer | Query Planning | 🟡 Limited | ML-basierte Kostenvorherrsage (Performance-Tuning) | Explainable, Fallback zu Rules |
| Hybrid Retrieval | Vector + Lexical Search | 🟡 Limited | ANN-Algorithmen (nicht supervised ML) | Transparency Logging |
| Caching | Result Caching | 🟢 Min | Deterministisch | Auditable |

**Fazit:** 🟡 **LIMITED RISK** — Query Optimizer nutzt ML für Performance, nicht für automatisierte Entscheidungen über Daten

**EU AI Act Implikationen:**
- Art. 50-51: Transparenz erforderlich wenn Optimizer *sichtbare* Entscheidungen trifft
- Lösung: Query Plans mit Confidence Scores dokumentieren

---

#### **index** (35.980 LOC, 🟡 **LIMITED RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| Vector ANN (FAISS/HNSW) | Approximate NN Retrieval | 🟡 Limited | ML-Inference (ANN), aber keine Entscheidung | Ranking Transparency |
| Spatial Indexing (R-Tree) | Geographic Queries | 🟢 Min | Algorithmic, keine KI | N/A |
| Adaptive Indexing | Index Selection | 🟡 Limited | Workload-Prediction (ML) | Config + Monitoring |

**Fazit:** 🟡 **LIMITED RISK** — Vector-Operationen sind deterministic post-training

---

#### **transaction** (11.055 LOC, 🟢 **MINIMAL RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| 2PC/3PC Coordinator | Distributed Consensus | 🟢 Min | Regelbasiert (Paxos/Raft) | Formal Verification |
| Isolation Levels | MVCC/SSI | 🟢 Min | Algorithmic | Testing |

**Fazit:** ✅ KONFORM

---

#### **graph** (12.476 LOC, 🟡 **LIMITED RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| Constraint Resolution | Rule-based Reasoning | 🟢 Min | Deterministic CSP | Explainable |
| Path Finding | Shortest Path, etc. | 🟢 Min | Algorithmic (Dijkstra) | N/A |
| Inference Engine | Backward/Forward Chaining | 🟡 Limited | Regelbasiert, aber komplex → Opaque | Logging + Explainability |

**Fazit:** 🟡 **LIMITED RISK** — Inference kann sein *Ergebnis* undurchsichtig sein, aber deterministisch

---

#### **sharding** (56.912 LOC, 🟡 **LIMITED-MEDIUM RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| Placement Strategy | Shard Allocation | 🟡 Limited | Heuristic Load Balancing (Machine Learning) | Auditable Placement |
| Rebalancing | Data Redistribution | 🟡 Limited | Adaptive Heuristics | Config-driven Thresholds |
| Cross-Shard TX | Distributed Coordination | 🟢 Min | Regelbasiert | Audit Logging |

**Fazit:** 🟡 **LIMITED RISK** — Placement nutzt ML für Performance-Optimierung, keine sicherheitskritischen Entscheidungen

---

### 2.2 SECURITY & ACCESS CONTROL MODULES

#### **auth** (17.344 LOC, 🟢 **MINIMAL RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| JWT/OIDC | Token Verification | 🟢 Min | Kryptographisch | BSI C5 |
| MFA | Multi-factor Authentication | 🟢 Min | Regelbasiert | No KI |
| Session Management | Token Lifecycle | 🟢 Min | Regelbasiert | Audit Logging |

**Fazit:** ✅ KONFORM

---

#### **security** (22.917 LOC, 🟡 **LIMITED RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| Encryption | AES-256-GCM | 🟢 Min | Kryptographisch | BSI C5 |
| Key Management | PKCS#11/Vault | 🟢 Min | Regelbasiert | Compliance Audits |
| Threat Detection | Anomaly Detection (ML) | 🔴 **HIGH** | ML-basierte Anomalie-Erkennung | (siehe §2.3) |
| PII Detection | Regex + ML Classifier | 🟡 Limited | Hybrid Approach | Explainability |

**Fazit:** 🟡-🔴 **LIMITED to HIGH** — Threat Detection und PII Detection sind ML-basiert

---

#### **governance** (14.664 LOC, 🔴 **HIGH RISK** wenn auto)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| Policy Enforcement | RBAC + OPA | 🟡 Limited | Regelbasiert, aber Regeln komplex sein können | Explainable Rules, Audit Trail |
| Data Masking | Automatic PII Redaction | 🟡-🔴 Limited-High | ML-basierte PII Detection → Automatisierte Maskierung | (siehe §2.3) |
| Compliance Checking | Policy Violations | 🟡 Limited | Regelbasiert, aber Config-abhängig | Audit Logging |

**Fazit:** 🟡-🔴 **LIMITED to HIGH** — Hängt davon ab, ob automatische Entscheidungen Nutzer beeinflussen

---

### 2.3 AI/LLM & ANALYTICS MODULES

#### **llm** (15.234 LOC, 🔴 **HIGH RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| LLM Integration | Text Generation | 🔴 **HIGH** | Neural Network → Black Box | Model Cards, Audit Trails |
| Prompt Engineering | Injection Prevention | 🟢 Min | Security Control | Pattern Detection |
| Model Versioning | Model Lifecycle | 🟡 Limited | Config, aber Breaking Changes möglich | Version Pinning |
| Fine-tuning (wenn vorhanden) | Custom Model Training | 🔴 **HIGH** | Custom NN | Transparency + Bias Audit |

**Fazit:** 🔴 **HIGH RISK** — LLM-Inference ist prinzipiell eine Black Box

**EU AI Act Art. 6 Hochrisiko-Szenarien:**
- ✅ **NICHT ANWENDBAR**: Echtzeit-Biometrische Identifikation
- 🟡 **TEILWEISE**: Bildung (wenn für Student Profiling genutzt)
- 🟡 **TEILWEISE**: Beschäftigung (wenn für HR-Decisions genutzt)
- 🟡 **TEILWEISE**: Strafrecht (wenn für Risk Assessment genutzt)

**Erforderliche Mitigationen (Art. 13-29):**
- [ ] Risk Assessment (Art. 19)
- [ ] Quality Management System (Art. 17)
- [ ] Testing & Validation (Art. 15)
- [ ] Human Oversight (Art. 26)
- [ ] Transparency & Documentation (Art. 13)
- [ ] Record Keeping (Art. 12)
- [ ] Monitoring nach Deployment (Art. 24)

---

#### **rag** (8.431 LOC, 🟡 **LIMITED RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| Retrieval (Vector + Lexical) | Document Ranking | 🟡 Limited | Hybrid Search (deterministic post-training) | Source Attribution |
| Ranking | Score-based Ordering | 🟡 Limited | Explainable Scoring | Confidence Logging |
| LLM + Retrieval Pipeline | Answer Generation | 🔴 **HIGH** | Kombination von Retrieval + LLM-Generation | (siehe LLM oben) |

**Fazit:** 🟡 **LIMITED RISK** (wenn RAG nur zur Retrieval), aber 🔴 **HIGH** wenn LLM-Part automatisierte Entscheidungen trifft

---

#### **analytics** (13.476 LOC, 🟡 **LIMITED RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| CEP Engine | Complex Event Processing | 🟡 Limited | Regelbasiert (aber komplexe Regeln) | Explainability |
| Time Series Anomaly | Threshold-based Detection | 🟡 Limited | Statistical (σ-Abweichung) | Transparent Thresholds |
| ML Anomaly Detection | Supervised/Unsupervised Models | 🔴 **HIGH** | Neural Networks / Ensemble Methods | (siehe Threat Detection) |

**Fazit:** 🟡-🔴 **LIMITED to HIGH** — Hängt vom verwendeten Algorithmus ab

---

#### **observability** (10.869 LOC, 🟡 **LIMITED RISK**)

| Komponente | Funktion | Risk Level | Begründung | Mitigationen |
|------------|---------|-----------|-----------|--------------|
| Metrics Collection | Aggregation | 🟢 Min | Regelbasiert | N/A |
| Alerting | Threshold Violations | 🟡 Limited | Statistical | Auditable Thresholds |
| Anomaly Detection | Pattern Recognition | 🟡-🔴 Limited-High | ML-based (abhängig von Algorithmus) | Monitoring |

**Fazit:** 🟡 **LIMITED RISK** — Alerts sind typischerweise nicht direkt entscheidungskritisch

---

### 2.4 AUXILIARY/OPTIONAL MODULES

#### **prompt_engineering** (3.842 LOC, 🟢 **MINIMAL RISK**)
- **Funktion:** Injection Detection & Prevention
- **Risk Level:** 🟢 Minimal — Security Control, keine KI
- **Fazit:** ✅ KONFORM

---

#### **ethics_ai** (private plugin, 🔴 **HIGH RISK** wenn aktiviert)
- **Funktion:** Ethics Scoring & Decision Review
- **Risk Level:** 🔴 **HIGH** — Ethik-Engine ist selbst eine KI
- **Erforderlich:** Full Art. 6-52 Compliance wenn used for automated decisions
- **Status:** Wave-1 Private Plugin (gated to Enterprise+)

---

#### **training** (1.056 LOC, 🔴 **HIGH RISK** wenn ML-Models trainiert)
- **Funktion:** Model Training Pipeline
- **Risk Level:** 🔴 **HIGH** — Training selbst ist KI
- **Erforderlich:** Trainingsdaten-Governance, Bias Audits
- **Status:** Scaffold (Phase 1 nur)

---

## 3. Aggregierter Risk Score pro Use-Case

### 3.1 Deployment-Szenario A: "Data Warehouse mit Vector Search"

```
Module:  storage (🟢) + query (🟡) + index (🟡)
Risk:    LIMITED 🟡
Grund:   Vector ANN ist deterministic post-training
Anforderungen: Art. 50-51 (Transparency)
```

### 3.2 Deployment-Szenario B: "RAG mit LLM Answering"

```
Module:  rag (🟡) + llm (🔴) + query (🟡) + security (🟡)
Risk:    HIGH 🔴
Grund:   LLM ist Black Box, getroffen automatisierte Entscheidungen
Anforderungen: Art. 6-52 (Full High-Risk Compliance)
```

### 3.3 Deployment-Szenario C: "Governance & Compliance Engine"

```
Module:  governance (🔴) + security (🟡)
Risk:    HIGH 🔴
Grund:   Automatisierte Access Decisions beeinflussen Nutzer
Anforderungen: Art. 6-52 (Critical Infrastructure / Admin Decisions)
```

### 3.4 Deployment-Szenario D: "Analytics & Monitoring"

```
Module:  analytics (🟡) + observability (🟡)
Risk:    LIMITED 🟡
Grund:   Metriken/Alerts sind Informativ, nicht Entscheidungen
Anforderungen: Art. 50-51 (Transparency optional)
```

---

## 4. Mitigation Strategies pro Risk Level

### 4.1 **HIGH RISK** 🔴 (Art. 6-52)

| Anforderung | Umsetzung in ThemisDB | Status |
|-------------|---------------------|--------|
| **Risk Assessment** (Art. 19) | `src/governance/AI_ML_IMPACT_ASSESSMENT.md` | 🟡 Vorhanden, aber nicht strukturiert |
| **Quality Management** (Art. 17) | Testing (Wave 8+), Benchmarks | ✅ Vorhanden |
| **Testing & Validation** (Art. 15) | Unit + Integration Tests | ✅ Vorhanden |
| **Data Governance** (Art. 10) | Metadata module, Data Lineage | 🟡 Teilweise |
| **Human Oversight** (Art. 26) | Deployment-dependent | 🟠 **MUST CONFIGURE** |
| **Transparency** (Art. 13, 28-29) | Audit Logging | 🟡 Teilweise |
| **Record Keeping** (Art. 12) | Audit Logger | ✅ Vorhanden |
| **Cybersecurity Monitoring** (Art. 24) | CodeQL, Sanitizer, Pentest | ✅ Vorhanden |

**Deployment Checklist für High-Risk:**
- [ ] Externe Risikobeurteilung durchführen
- [ ] Model Cards für alle LLMs erstellen
- [ ] Human Oversight Workflows definieren
- [ ] Audit Trail für alle KI-Entscheidungen aktivieren
- [ ] Explainability Tools (LIME/SHAP) implementieren
- [ ] Fairness/Bias Tests durchführen
- [ ] Monitoring Dashboard konfigurieren
- [ ] Compliance Review vor Produktion

---

### 4.2 **LIMITED RISK** 🟡 (Art. 50-51)

| Anforderung | Umsetzung | Status |
|-------------|-----------|--------|
| **Transparency** | Audit Logs + Confidence Scores | 🟡 Teilweise |
| **Consent** | Optional (deployment-dependent) | 🟠 **MUST CONFIGURE** |
| **Human Intervention** | Optional | 🟠 **MUST CONFIGURE** |

**Deployment Checklist für Limited-Risk:**
- [ ] Transparenzbericht für Nutzer verfassen
- [ ] Confidence Scores / Ranking erklären
- [ ] Opt-out Mechanism bereitstellen
- [ ] Audit Logging aktivieren

---

### 4.3 **MINIMAL/GENERAL RISK** 🟢 (Art. 69)

**Anforderung:** Standard Compliance (GDPR, etc.)

---

## 5. Continuous Compliance Monitoring

### 5.1 **Audit-Cadence**

| Event | Häufigkeit | Verantwortlich |
|-------|-----------|----------------|
| Model Performance Check | Täglich | Ops |
| Bias/Fairness Audit | Monatlich | Data Science |
| Compliance Review | Quarterly | Legal/Security |
| External Audit | Jährlich (vor Release) | External Auditor |

### 5.2 **Red Flags** (Sofortiges Eskalation erforderlich)

- ⚠️ Model Accuracy fällt unter 90% (für High-Risk)
- ⚠️ Falsch-positive Rate > 5% (für Safety-Critical)
- ⚠️ Demographic Parity Gap > 10% (für Fairness)
- ⚠️ Deployment ohne Model Card (für High-Risk)
- ⚠️ Keine Audit Logs für 24h (für alle AI-Systeme)

---

## 6. Regulatory Timeline

| Datum | Anforderung | ThemisDB Status |
|-------|-----------|-----------------|
| 2026-01-10 | EU AI Act in Kraft | ✅ Gültig |
| 2026-09-10 | Transition Period für High-Risk (6 Monate) | 🟡 IN PROGRESS |
| 2027-03-10 | High-Risk Compliance erforderlich | 📅 Target Date |
| 2027-06-10 | Notified Bodies Zertifizierung (optional) | 📅 Optional |

---

## 7. Kontakt & Governance

**Risk Owner:** `src/governance/` CODEOWNERS  
**Compliance Officer:** siehe `docs/de/compliance/README.md`  
**Security Lead:** siehe `SECURITY.md`

---

**Änderungsverlauf:**
- v1.0 (2026-08-08): Initiale Risk Mapping basierend auf v2.4.0-rc1
