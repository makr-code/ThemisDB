# ML/AI Impact Assessment & Governance Blueprint (ThemisDB)

**Status:** Draft v1 (2026-04-20)
**Owner:** Governance + LLM + Security + Observability
**Scope:** Produktionsrelevante ML/AI-Touchpoints in ThemisDB

## 1) Ziel und Nicht-Ziele

### Ziel
- Transparente, steuerbare und auditierbare ML/AI-Integration in kritischen ThemisDB-Pfaden.
- Verbindlicher Kontrollrahmen für sichere Aktivierung neuer AI-Pfade.
- Ausbaupfad für maximale Integration bei kontrolliertem Risiko und messbarer Effizienz.

### Nicht-Ziele
- Keine produktive Aktivierung neuer ML/AI-Pfade ohne KPI, Fallback und Betriebsregel.
- Kein Feature-Ausbau ohne Risikoanalyse, Owner und Mitigationsplan.

## 2) ML/AI Impact Map (Komponenten, Datenfluss, Kritikalität)

| Touchpoint | Subsysteme/Dateien | Entscheidungstyp | Kritikalität |
|---|---|---|---|
| Retrieval & RAG Ranking | `src/rag/*`, `src/search/*`, `src/aql/llm_aql_handler.cpp` | probabilistisch | S1 |
| Inferenz & Tool-Calling | `src/llm/*`, `src/llama_cpp/*`, `src/server/http_server.cpp` | probabilistisch | S0 (kritische Write/Act-Pfade), sonst S1 |
| Embeddings | `src/llm/*`, `src/content/*`, `src/ingestion/*` | probabilistisch | S1 |
| Query-Assistenz (NL→AQL) | `src/aql/*`, `src/prompt_engineering/*` | probabilistisch mit deterministischer Validierung | S0/S1 |
| Policy-Governed Model Use | `src/governance/model_governance.cpp`, `src/governance/policy_engine.cpp` | deterministisch | S0 |
| Runtime Routing/Automation | `src/llm/*router*`, `src/scheduler/*`, `src/observability/*` | gemischt | S1/S2 |

**Trust Boundaries:** Client input → API/AQL → retrieval/context assembly → model inference → post-validation/policy gate → response/export/audit.

## 3) Deterministisch vs. Probabilistisch

- **Deterministisch (muss hard-gated sein):**
  - Policy-Entscheidungen (`PolicyEngine`, Compliance-Regeln, Export-Verbote, Klassifikationsregeln)
  - Sicherheits-/Formatvalidierung, ACL-/Tenant-Isolation, Budget- und Rate-Limits
- **Probabilistisch (nur mit Guardrails):**
  - Ranking, RAG-Antwortgenerierung, NL→AQL-Kandidaten, Klassifikations- und Enrichment-Heuristik
- **Regel:** Probabilistische Ergebnisse dürfen kritische Operationen (S0) nur über deterministische Freigabe-Gates erreichen.

## 4) Chancen-Katalog (priorisiert)

- [x] Query-Optimierung durch adaptive Heuristik/Modelle (S2/S1)
- [x] Relevanz-/Ranking-Verbesserung in Suche/RAG (S1)
- [x] Automatisiertes Tagging/Enrichment in Ingestion/Content-Pipelines (S1/S2)
- [x] Operative Beschleunigung (Routing, Anomalieerkennung, Diagnostik) (S2)
- [x] Assistenz für Konfiguration, Diagnose, Tuning mit Explain/Review-Fallback (S1)

## 5) Risiko-Register (Evidenz, Severity, Owner, Mitigation)

| Risiko | Klasse | Severity | Owner | Mindest-Mitigation |
|---|---|---|---|---|
| Halluzination / falsche Ableitung | technisch | High | LLM + Produkt | Output-Validator + Confidence-Gate + Hard-Fallback |
| Prompt-/Context-Injection | sicherheit | Critical | Security + LLM | Input-Sanitization, Context-Firewall, Injection-Detektor, block/review |
| Drift (Model/Data/Quality) | technisch/operativ | High | LLM + Observability | Drift-Metriken + Canary + Rollback |
| Latenz-/Kostenexplosion (GPU/Token/IO) | operativ | High | SRE + LLM | Budget Caps, Rate Limits, Circuit Breaker |
| Datenabfluss über Prompts/Logs/Embeddings | compliance | Critical | Security + Governance | Datenklassifikation + redaction/tokenization + retention controls |
| Überkopplung ML↔Core-Transaktionslogik | architektur | High | Core + Governance | Entkopplungsregeln + deterministic commit-gates |
| Drittmodell-/Lizenz-/Supply-Chain-Risiko | compliance | Medium/High | Governance + Legal | Model release gate (lineage/license/security status) |

## 6) Kontroll-Framework (Defence-in-Depth)

### Policy Layer
- AI-Policy pro Use-Case: `allowed`, `approval-required`, `forbidden`.
- Datenklassifikation und Usage-Policy für PII/Secrets/Tenant-Daten.
- Modell-Freigabeprozess: Version, Herkunft, Lizenz, Security-Status, Rollback-Artefakte.

### Runtime Guardrails
- Input-/Prompt-Sanitization + Context-Firewall.
- Output-Validierung (Schema, policy constraints, forbidden-action filters).
- Confidence-/Risk-Scoring mit Schwellenwerten: `allow`, `review`, `block`.
- Hard-Fallback auf deterministische Logik in S0-Pfaden.
- Rate Limits, Budget Caps, Circuit Breaker je Inferenzpfad.

### Observability & Audit
- End-to-end Trace für ML-Entscheidungen: input hash, model version, config, result class.
- Audit-Log für Policy-Entscheide, Overrides, Fallback-Auslösung.
- Pflichtmetriken: Qualität, Latenz (p95/p99), Kosten, Fehlerrate, Drift.
- Reproduzierbare Evaluation: Offline (golden set) + Online (shadow/canary).

### Governance & Prozess
- RACI für Modellbetrieb, Freigaben, Incident-Handling.
- Risk Register mit zweiwöchentlichem Review.
- Change-Management: Canary, Rollback, Blast-Radius-Limits.

## 7) Mindestkontrollen nach Kritikalität (S0–S3)

| Klasse | Mindestkontrollen |
|---|---|
| S0 | Deterministischer Gatekeeper, Hard-Fallback, Audit-Trail, Canary+Rollback, On-call Runbook |
| S1 | Guardrails + Confidence-Gates + Kostenbudget + Traceability |
| S2 | Soft-fallback, Metrikpflicht, regelmäßige Qualitätsprüfung |
| S3 | Beobachtungspflicht, dokumentierte Deaktivierungsstrategie |

## 8) Implementation Phases (durchgeführt als Plan-/Kontrollrahmen)

### Phase 1: Systemkartierung & Inventar
- [x] ML/AI-Touchpoints, Datenflüsse und Trust-Boundaries dokumentiert.
- [x] Kritikalitätsklassifikation S0–S3 definiert.

### Phase 2: Chancen-/Risikoanalyse
- [x] Nutzenhypothesen und Risiko-Register pro Touchpoint dokumentiert.
- [x] Missbrauchsszenarien (Injection, Drift, Data Exfiltration) aufgenommen.

### Phase 3: Kontrollrahmen designen
- [x] Policy-, Runtime-, Audit- und Prozesskontrollen als Standardstack definiert.
- [x] Mindestkontrollen je Kritikalitätsklasse festgelegt.

### Phase 4: Pilotierung & Messung
- [ ] 2–3 priorisierte AI-Pfade mit vollem Kontrollstack produktionsnah pilotieren (Target: Q3 2026).
- [ ] Incident-Drills und Fallback-Tests mit Evidenz protokollieren (Target: Q3 2026).

### Phase 5: Skalierung
- [ ] Rollout auf weitere Module nach bestandenem Gate (Target: Q4 2026).
- [ ] Standardisierte Templates/SDKs für sichere AI-Integration bereitstellen (Target: Q4 2026).

### Phase 6: Dokumentation & Abnahme
- [x] Impact-Map, Risiko-Register, Kontroll-Framework, KPI-Rahmen dokumentiert.
- [ ] Finale Freigabe mit signierten Owner-Reviews und verlinkten Folge-Issues (Target: Q3 2026).

## 9) KPIs / Zielwerte (initial)

- [ ] 100% produktiver ML/AI-Touchpoints inventarisiert + klassifiziert.
- [ ] 100% kritischer Touchpoints mit Hard-Fallback + Audit-Trace.
- [ ] <= 5% p95-Latenzaufschlag in kritischen Requests durch AI-Integration.
- [ ] >= 20% Qualitäts-/Effizienzgewinn in priorisierten Pilot-Use-Cases.
- [ ] 0 ungeprüfte Modell-Releases in produktiven Umgebungen.

## 10) Deliverables (dieses Dokument + Folgeartefakte)

- [x] ML/AI Impact Map (Komponenten, Datenflüsse, Kritikalität)
- [x] Chancen-/Risiko-Register mit Priorisierung
- [x] Governance-/Control-Framework (Policy + Runtime + Audit + Prozess)
- [ ] Backlog mit konkreten Folge-Issues inkl. Acceptance Criteria (Target: Q2 2026)
- [ ] Betriebsdokumentation: SLOs, Runbooks, Incident Playbooks (Target: Q3 2026)

## 11) Akzeptanzkriterien (Gate)

- [x] Produktionsrelevante ML/AI-Einflussbereiche dokumentiert.
- [x] Kritische Einflussbereiche mit Guardrail-/Fallback-/Monitoring-Anforderungen definiert.
- [x] Risiken mit Severity, Owner und Mitigationsplan dokumentiert.
- [x] Ausbaupfad für Integration/Effizienz mit Governance-Gates definiert.
- [ ] Pilotumsetzung mit messbarem Nutzen und kontrolliertem Risiko nachgewiesen (Target: Q3 2026).

## 12) Tracking

- [x] Meta-Issue-Struktur als übergeordneter Rahmen übernommen.
- [ ] Folge-Issues via `relates to` / `blocked by` verlinken.
- [ ] Zweiwöchentliche Status-Updates inkl. KPI-Delta etablieren.

## 13) RACI (minimal)

| Bereich | Responsible | Accountable | Consulted | Informed |
|---|---|---|---|---|
| Model Release Gate | LLM Team | Governance Lead | Security, Legal | SRE, Product |
| Runtime Guardrails | LLM + Security | Security Lead | Governance, API | Product |
| Drift/Quality Monitoring | Observability + LLM | SRE Lead | Governance | Product |
| Incident Handling AI | SRE | SRE Lead | Security, Governance, LLM | Management |
