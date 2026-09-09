# RAG/BPMN Governance Guardrail - Analyse und Plan

Datum: 2026-09-06  
Scope: Erkenntnisse fuer spaetere Sourcecode-Evaluierung und Planungsphase

## 1) Problemraum und Ziel

Die Organisation definiert per JSON/YAML verbindlich:
- welche Daten-Domaenen existieren,
- wie diese Domaenen in Beziehung stehen duerfen,
- welche Aktionen auf den Daten erlaubt/verboten sind,
- welche Guardrails und Sicherheitsinvarianten niemals verletzt werden duerfen.

Benutzer duerfen darauf aufbauend nur die spezifischen RAG-Abfragen ausfuehren, die innerhalb dieser Governance-Regeln erlaubt sind.

Wesentliche Anforderung:
- Nicht primär Dokument-zu-Dokument-Graphbeziehungen lernen,
- sondern Prozess-Dokument-Zusammenhaenge (BPMN-/Workflow-Kontext) auch dann robust behandeln, wenn der konkrete Prozess zu Beginn unbekannt ist.

## 2) Konsolidierte Erkenntnisse (Stand Technik/Wissenschaft/Informatik)

### 2.1 Stand der Technik (Engineering)
- Hybrid-RAG mit Planer ist Standard: erst harte Filter/Constraints, dann semantische Kandidaten, dann Re-Ranking/Fusion.
- Retrieval-Routing wird in kleine Teilauftraege zerlegt (intent -> evidenzbedarf -> retrievalpfad -> synthese).
- Laufzeitoptimierung erfolgt bevorzugt via Contextual Bandits (stabiler/auditierbarer als volles RL im Produktionspfad).
- Guardrails werden als harte Policy-Gates umgesetzt, nicht als rein nachgelagerte Best-Effort-Pruefung.
- Vollstaendige Provenienz (Policy-Snapshot, Reason-Codes, Confidence, Chain-Length) ist fuer regulatorisch belastbare KI-Pfade Pflicht.

### 2.2 Stand der Wissenschaft
- Process Mining: Discovery, Conformance, Enhancement auf Event-Logs.
- Object-Centric Process Mining: Mehr-Objekt-/Dokumentkontext statt rein linearer Cases.
- Predictive Process Monitoring: Vorhersage naechster Schritte, SLA-/Fehlerrisiken.
- Learning-to-Rank + Counterfactual Evaluation fuer sichere Retriever-/Prompt-Optimierung.
- Human-in-the-loop Re-Anchoring gegen semantischen Drift.

### 2.3 Stand der Informatik (Formalisierung)
- Problem als constrained decision process mit unvollstaendiger Beobachtbarkeit.
- Multi-Objective Optimierung mit Sicherheitsnebenbedingungen.
- Empfehlenswerte Praxis: kontextuelle Bandits + harte Invarianten + fail-closed.

## 3) Fit zum aktuellen ThemisDB-Zustand

Vorhandene Bausteine (positive Basis):
- LLM-Wiki Prozesspolicy in YAML vorhanden.
- JSON-Schema fuer die Prozesspolicy vorhanden.
- Guardrail-/Entitlement-/Edition-Gating bereits architektonisch verankert.
- Provenienz-/Degradationsmodell inklusive Drift-/Re-Anchor-Konzept vorhanden.
- Prompt-Engineering als separates Optimierungs-/Evaluierungs-Subsystem vorhanden.
- Process-Modul mit BPMN/OCEL/Retrieval-Support und deterministischen Verträgen vorhanden.

Quellen im Repository:
- src/llm_wiki/process/llm_wiki_process_policy.yaml
- src/llm_wiki/schema/llm_wiki_process_policy.schema.json
- src/llm_wiki/ARCHITECTURE.md
- src/llm_wiki/README.md
- src/llm_wiki/ROADMAP.md
- src/prompt_engineering/ARCHITECTURE.md
- src/process/README.md
- ai_context/developer_llm_wiki/AI_METADATA_AND_PROVENANCE.md

## 4) Governance-Guardrail Zielarchitektur

### 4.1 Vier-Ebenen-Modell
1. Schema-Ebene (JSON-Schema):
   - Strukturelle Validierung aller Policies.
   - Ungueltige/inkonsistente Policies werden fail-closed abgelehnt.

2. Organisations-Policy (YAML):
   - Definiert Domaenen, erlaubte/verbotene Beziehungen, Rollenrechte, harte Invarianten.
   - Definiert auch ML-tunable und ML-never-adjust Parameter.

3. Workspace-/Tenant-Overlay:
   - Darf nur restriktiver werden (niemals global erweitern).

4. Request-Laufzeit:
   - User-Intent + Rolle + Domaenen + Aktionstypen.
   - Ergebnis ist deterministisch: allow/deny + reason_code + policy_snapshot_id.

### 4.2 Nicht verhandelbare Sicherheitsinvarianten
- fail_closed = true
- policy_snapshot_required = true
- require_reason_codes = true
- second_planner_allowed = false
- entitlement/edition/guardrail gates sind nicht durch ML modifizierbar

### 4.3 ML darf nur innerhalb harter Grenzen tunen
Erlaubte Knobs (Beispiele):
- synthesize.max_evidence_items
- synthesize.min_provenance_confidence
- extract.max_candidates_per_doc
- re_anchor.trigger.confidence_lt

Verbotene Knobs:
- entitlement rules
- forbidden relationships
- fail_closed
- mandatory audit evidence fields

## 5) Erforderliche Policy-Artefakte (Erweiterung)

Neben der bereits vorhandenen Process-Policy wird eine zweite, domaenenbezogene Governance-Policy empfohlen:
- src/llm_wiki/process/governance_data_relations.yaml
- src/llm_wiki/schema/governance_data_relations.schema.json

Zweck:
- Explizite Modellierung, welche Daten in welchen Relationen zusammen genutzt werden duerfen.
- Trennung von Prozess-Orchestrierung und Governance-Domainlogik.

Implementierungsstatus (2026-09-06):
- [x] Initiale Draft-Datei erstellt: src/llm_wiki/process/governance_data_relations.yaml
- [x] Initiales JSON-Schema erstellt: src/llm_wiki/schema/governance_data_relations.schema.json
- [ ] Runtime-Loader/Validator gegen neues Schema verdrahten
- [ ] Deterministische Deny-Path-Tests fuer Beziehungsgates und Rollenrechte erweitern

## 6) Referenzmodell fuer Organisationspolicy (Entwurf)

```yaml
version: 2
policy_id: org_rag_governance_v2

roles:
  analyst:
    allow_actions:
      - rag.query_process_context
      - rag.retrieve_evidence
  operator:
    allow_actions:
      - rag.query_process_context
      - rag.retrieve_evidence
      - rag.generate_summary
  auditor:
    allow_actions:
      - rag.retrieve_evidence
      - rag.generate_trace_report

data_domains:
  - process_models
  - process_events
  - legal_docs
  - technical_docs
  - pii
  - secrets

allowed_relationships:
  - from: process_events
    to: process_models
    relation: supports_discovery
  - from: legal_docs
    to: process_models
    relation: constrains_execution

forbidden_relationships:
  - from: pii
    to: public_answers
    reason_code: GOV-PII-001
  - from: secrets
    to: llm_prompt_context
    reason_code: GOV-SEC-001

hard_invariants:
  fail_closed: true
  require_policy_snapshot: true
  require_reason_codes: true
  min_provenance_confidence: 0.65
  max_synthetic_chain_length: 3

ml_control:
  adjustable_knobs:
    - synthesize.max_evidence_items
    - extract.max_candidates_per_doc
  never_adjust:
    - entitlement_rules
    - forbidden_relationships
    - hard_invariants.fail_closed
```

## 7) Laufzeitvertrag fuer Benutzer-RAG (Request/Decision)

### 7.1 Request (JSON)
```json
{
  "user_id": "u-4711",
  "workspace_id": "ws-proc-a",
  "role": "analyst",
  "intent": "Vergleich aehnlicher Faelle fuer Prozessschritt Genehmigung",
  "requested_actions": [
    "rag.query_process_context",
    "rag.retrieve_evidence"
  ],
  "requested_domains": [
    "process_events",
    "process_models",
    "legal_docs"
  ]
}
```

### 7.2 Decision Evidence (persistiert, verpflichtend)
- decision_id
- policy_snapshot_id
- user_role
- requested_actions
- gate_outcomes
- allow_or_deny
- reason_code
- provenance_confidence
- synthetic_chain_length
- selected_knobs

## 8) Evaluierungsrahmen fuer Sourcecode-Phase

### 8.1 Pflicht-Checks (Code + Runtime)
1. Policy-Laden/Validieren fail-closed?
2. Rollen-/Entitlement-Gate vor Retrievalplan?
3. Domaenenbeziehungen (allow/deny) vor Kontextsynthese?
4. Reason-Codes in allen deny-Pfaden deterministisch?
5. Policy-Snapshot-ID pro Entscheidung persistiert?
6. ML-Knobs nur aus allowlist und innerhalb hard_bounds?
7. Re-Anchor-Trigger bei Drift/Chain-Length aktiv?
8. Shadow/Canary/Promote/Rollback-Kriterien technisch erzwungen?

### 8.2 Test-Mindestset (12 Governance-Guardrail-Tests)
- GG-01 invalid policy schema -> startup deny
- GG-02 missing snapshot id -> deny
- GG-03 role action violation -> deny
- GG-04 forbidden domain relationship -> deny
- GG-05 allowed relationship happy path -> allow
- GG-06 low provenance confidence -> deny/limited mode
- GG-07 synthetic chain too long -> re-anchor required
- GG-08 ML attempts never_adjust knob -> deny
- GG-09 knob out of hard_bounds -> deny
- GG-10 stage disabled by policy -> deterministic deny reason
- GG-11 shadow-mode update no production side-effects
- GG-12 canary rollback trigger fires on security regression

## 9) Delta zu aktuellem Roadmap-Stand

Bereits umgesetzt (aus Dokumentlage):
- Baseline Process Policy + Schema vorhanden.
- Stage-Gates teilweise in Runtime verdrahtet.
- Governance Evidence bei bestimmten Deny-Pfaden bereits persistiert.

Noch offen (fuer naechste Iteration prior):
- Pre-synthesis evidence allowlist gate vollstaendig durchziehen.
- Per-request governance evidence fuer alle Pfade vereinheitlichen.
- Datenbeziehungs-Policy (domain relation contract) als separates Artefakt einfuehren.
- Deterministische Deny-Path-Regressionssuite auf 12+ Kernfaelle ausbauen.

## 10) Umsetzungsplan (Planungsphase)

Phase A (Kurzfristig, 2-4 Wochen):
- governance_data_relations Schema + YAML einführen.
- Request-Decision-Vertrag finalisieren.
- Reason-Code-Katalog normieren.

Phase B (4-8 Wochen):
- Vollstaendige Gate-Verdrahtung vor Retrieval/Synthese.
- Pflicht-Telemetrie auf allen Allow/Deny Pfaden.
- Testset GG-01..GG-12 gruen ziehen.

Phase C (8-12 Wochen):
- Contextual-Bandit nur fuer freigegebene Knobs.
- Shadow/Canary/Promotion mit automatischem Rollback.
- Process-Mining Feedback in Governance-konformes Routing integrieren.

## 11) Risiko- und Compliance-Hinweise

- Kein Legacy-/Fallback-Bypass fuer Governance-Gates ohne explizite Human-Freigabe.
- Keine private/geschuetzte Inhalte in Community/Minimal Pfade.
- Governance-Aenderungen muessen Dokumentation synchron halten (Roadmap/Release/Governance-Corpus).

## 12) Ergebnis fuer Folgephase

Dieses Dokument definiert:
- die fachliche Leitlinie,
- den technischen Governance-Vertrag,
- den Evaluierungsrahmen fuer Sourcecode,
- und den stufenweisen Implementierungsplan.

Es kann direkt als Arbeitsgrundlage fuer die naechste Coding-/Review-Iteration verwendet werden.
