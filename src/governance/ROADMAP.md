> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Governance Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status

**Production-Ready** — Policy-based data access control, GDPR/HIPAA/CCPA/CPRA/PCI-DSS/SOC 2 compliance rule evaluation, automated data retention, data classification, OPA integration, policy simulation, cross-tenant inheritance, data masking, data lineage, AI/ML model governance, and compliance reporting are all operational.

## Completed ✅

- [x] Policy engine for data access control
- [x] Compliance rule evaluation (GDPR, HIPAA, and other regulations)
- [x] Automated data retention policy enforcement
- [x] Data classification and labeling
- [x] Audit trail integration for governance events
- [x] Policy-based governance enforcement at query time
- [x] CCPA/CPRA data subject rights enforcement (right-to-know, right-to-delete, opt-out-of-sale, data portability)
- [x] Policy hot-reload on configuration change (Issue: #1762)
- [x] Conflict detection for overlapping access control policies (Issue: #1763)
- [x] CCPA / CPRA data subject rights enforcement (Issue: #1764)
- [x] Data lineage tracking for governed datasets (Issue: #1765)
- [x] Compliance report generation (PDF / JSON summary) (Issue: #1767)
- [x] SOC 2 compliance controls and evidence collection (Issue: #1769)
- [x] AI/ML model governance (training data lineage, bias auditing) (Issue: #1771)
- [x] Cross-tenant governance policy inheritance (Target: Q1 2026) (Issue: #1772)
- [x] Automated data masking for sensitive fields in query results (Issue: #1773)
- [x] Policy conflict detection and resolution reporting (Issue: #1760)
- [x] CCPA compliance rule set (`ccpa_rules.cpp`, `CcpaRuleSet`) (Issue: #1761)
- [x] Policy simulation / dry-run mode (`PolicyEngine::simulateDecision()`) (Issue: #1766)
- [x] OPA (Open Policy Agent) integration for policy-as-code (`opa_adapter.cpp`) (Issue: #1768)
- [x] PCI-DSS data isolation rules (`pci_dss_rules.cpp`) (Issue: #1770)
- [x] ISO 27001 Annex A control evaluators (`iso27001_rules.cpp`, `Iso27001ControlSet`) (v1.9.0)
- [x] HIPAA Security Rule evaluators (`hipaa_rules.cpp`, `HipaaRuleSet`) (v1.9.0)

## In Progress 🚧

- [~] ML/AI Impact Assessment & Governance framework (Target: Q2 2026)
  - Scope: full inventory of ML/AI touchpoints across retrieval, ranking, inference, embeddings, query-assist, and content pipelines
  - Deliverable: `src/governance/AI_ML_IMPACT_ASSESSMENT.md` (impact map, risk register, control stack, KPIs, pilot/scale path)
  - Exit criteria: critical touchpoints have guardrail policy, hard-fallback definition, and audit/trace requirements

## Planned Features 📋

- [ ] Pilot and operationalize ML/AI control stack for 2–3 critical paths (Target: Q3 2026)
- [ ] Enforce model release governance gate (lineage/license/security/rollback) in CI/CD (Target: Q3 2026)
- [ ] Add quarterly ML/AI risk review cadence with KPI deltas and remediation tracking (Target: Q4 2026)

## Implementation Phases

### Phase 1: Policy Engine and Compliance Rules (Status: Completed)

- [x] Implemented policy engine for attribute-based data access control (`governance/policy_engine.cpp`)
- [x] Implemented GDPR and HIPAA compliance rule evaluation at query time
- [x] Implemented automated data retention policy enforcement with configurable TTLs
- [x] Implemented data classification and labeling for PII, PHI, and confidential fields
- [x] Integrated audit trail recording all governance enforcement events

### Phase 2: Policy Versioning and Reporting (Status: Completed)

- [x] Implement policy versioning with rollback support (`governance/policy_manager_versioned.cpp`) (Issue: #1780)
- [x] Implement compliance report generation summarizing rule evaluations per time window (Issue: #1781)
- [x] Implement policy conflict detection for overlapping access control rules (Issue: #1782)

### Phase 3: Hot-Reload, CCPA, and OPA Integration (Status: Completed)

- [x] Implement policy hot-reload on config file change without service restart (Issue: #1774)
- [x] Implement CCPA/CPRA data subject rights enforcement (right-to-delete, right-to-know) (Issue: #1775)
- [x] Implement automated data masking for configured sensitive fields in query results (Issue: #1776)
- [x] Integrate Open Policy Agent (OPA) as an alternative policy evaluation engine (Issue: #1777)

### Phase 4: Cross-Tenant Policy Inheritance (Status: Completed)

- [x] Implement `CrossTenantPolicyInheritance` class for tenant hierarchy management (Target: Q1 2026) (`governance/cross_tenant_policy_inheritance.cpp`)
- [x] Implement cycle detection for parent-child tenant registration (Target: Q1 2026)
- [x] Implement most-restrictive-wins merge semantics across ancestor chains (Target: Q1 2026)
- [x] Implement `evaluateEffectivePolicy()` merging decisions from full ancestor chain (Target: Q1 2026)
- [x] Implement `resolveEffectiveRules()` returning flattened rule list (Target: Q1 2026)
- [x] Unit tests covering hierarchy registration, cycle prevention, merge semantics, and edge cases (Target: Q1 2026)

### Phase 5: ML/AI Impact Assessment & Governance (Status: In Progress 🚧)

- [x] System map of ML/AI touchpoints and trust boundaries documented (`AI_ML_IMPACT_ASSESSMENT.md`, Section 1/2)
- [x] Deterministic vs probabilistic decision taxonomy and criticality classes (S0–S3) documented (`AI_ML_IMPACT_ASSESSMENT.md`, Section 3)
- [x] Chance/risk catalog including hallucination, drift, injection, cost/latency, and coupling risks documented (`AI_ML_IMPACT_ASSESSMENT.md`, Section 4/5)
- [x] Defense-in-depth control framework (policy, runtime guardrails, observability/audit, governance process) defined (`AI_ML_IMPACT_ASSESSMENT.md`, Section 6/7)
- [ ] Pilot rollout for 2–3 critical AI paths with KPI baseline and incident drill evidence (Target: Q3 2026)

### Phase 6: Documentation & Acceptance (Status: In Progress 🚧)

- [x] Deliverables bundle documented: impact map, prioritized risks, control framework, KPI set, backlog seeds (`AI_ML_IMPACT_ASSESSMENT.md`)
- [ ] Governance runbooks/playbooks finalized and linked to operations docs (Target: Q3 2026)
- [ ] Acceptance gate review: all critical touchpoints show fallback + audit trace + owner sign-off (Target: Q3 2026)

## Production Readiness Checklist

- [x] Unit tests coverage > 80% (Issue: #1778) — 21 focused test targets registered in `tests/CMakeLists.txt`; CI: `governance-module-ci.yml`
- [x] ISO 27001 Annex A controls evaluated (`iso27001_rules.cpp`): A.9.1.2, A.10.1.1, A.12.4.1, A.12.4.2, A.13.2.3, A.18.1.3
- [x] HIPAA Security Rule checks implemented (`hipaa_rules.cpp`): §164.312(a)(1), §164.312(a)(2)(iv), §164.312(b), §164.312(c)(1), §164.312(e)(2)(ii), §164.530(j)
- [x] Integration tests (policy evaluation, retention enforcement, audit trail)
- [x] Performance benchmarks (policy evaluation latency at query time) (Issue: #1779)
  - Benchmark file: `benchmarks/bench_governance_policy_latency.cpp`
  - Subsystems: `governance/policy_engine.cpp`, `governance/data_masker.cpp`, CCPA opt-out registry
  - Measured operations: `PolicyEngine::evaluate()`, `PolicyEngine::checkQueryPermission()`
  - Test matrix: heuristic fallback vs YAML-loaded profiles; all four VS classifications; CCPA opt-out overhead; field-masking overhead; high-volume batches (1 / 10 / 100 / 1 000 requests)
  - Performance targets: `evaluate()` p99 < 0.5 ms, `checkQueryPermission()` p99 < 0.5 ms (single-threaded, no I/O, no audit log)
  - Error cases: YAML absent → heuristic fallback (benchmark runs without abort); CCPA registry empty → opted-out flag false
  - Build: registered in `benchmarks/CMakeLists.txt` as `bench_governance_policy_latency`; requires `themis_core`, `yaml-cpp`, `nlohmann_json`
- [x] Security audit (policy bypass prevention, audit trail integrity)
- [x] Documentation complete (policy engine, compliance governance, integration guide)
- [x] API stability guaranteed for policy engine and rule evaluation

## Known Issues & Limitations

- CCPA rule set is implemented in `src/governance/ccpa_rules.cpp` (CcpaRuleSet)
- OPA integration implemented: `governance::OpaAdapter` in `src/governance/opa_adapter.cpp`; attach via `PolicyEngine::setOpaEvaluator()`; falls back to native evaluation when OPA is unavailable and emits `governance_opa_fallback_total` counter
- Automated data masking in query results is implemented (`governance/data_masker.cpp`; `DataMasker`)
- Data lineage tracking is implemented (`governance/data_lineage.cpp`; `DataLineageTracker`)
- AI/ML model governance is implemented (`governance/model_governance.cpp`; `ModelGovernancePolicy`)

## Breaking Changes

- OPA integration introduces a new policy language alongside the existing native rule format (additive, non-breaking)
- Policy simulation API is a new endpoint (non-breaking)

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🧪 NUR_TESTS (implementiert, kein Produktions-Aufrufer)

- `CcpaRuleSet` – CCPA-Compliance-Regeln; geprüft in ccpa_rules-Tests und Compliance-Bench
  > **Aktion:** ROADMAP-Ticket für Produktions-Integration ergänzen oder als CANDIDATE_FOR_REMOVAL markieren.
