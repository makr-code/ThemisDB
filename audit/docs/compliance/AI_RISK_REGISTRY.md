# ThemisDB AI Risk Registry

**Version:** v2.4.0
**Date:** 2026-09-21
**Owner:** ThemisDB Contributors
**EU AI Act Scope:** Article 9 (Risk Management System), Article 13 (Transparency)
**Status:** active

---

## Scope

This registry documents all identified High-Risk and Notable-Risk AI functions within ThemisDB, their risk classification, mitigations, and monitoring status.

## Risk Classification Criteria

- **High-Risk**: Meets EU AI Act Annex III criteria or operator-assessed safety-critical impact
- **Medium-Risk**: Significant failure impact; requires monitoring and mitigation
- **Low-Risk**: Limited impact; standard operational controls sufficient

---

## Registry

| ID | Component | Function | Risk Level | EU AI Act Ref | Mitigation | Monitoring | Status |
|---|---|---|---|---|---|---|---|
| AR-001 | LLM Backend | Natural language query inference | Medium | Art. 13, 22 | RAG grounding; judge scoring; `[AI_DECISION]` audit marker | Audit log review; judge scoring trend | ✅ Active |
| AR-002 | RAG Pipeline | Retrieval-augmented answer generation | Medium | Art. 13, 22 | Source traceability; judge integration; fail-safe `llm_unavailable` | Retrieval relevance metrics; audit logs | ✅ Active |
| AR-003 | LLM Judge | Automated response quality scoring | Medium | Art. 22 | Human-in-the-loop override; explicit `llm_unavailable` on backend failure | Judge score distribution monitoring | ✅ Active |
| AR-004 | Ethics AI Plugin | Ethical constraint enforcement | Medium | Art. 9, 22 | Fail-closed gate; plugin integrity via SBOM attestation | Plugin health checks; CI gate | 🟡 Partial (plugin in progress) |
| AR-005 | AI Snapshot Cleanup | Automated AI-assisted data retention decisions | Low–Medium | Art. 9 | Human approval required for bulk deletions; audit logging | Deletion audit events | ✅ Active |
| AR-006 | Knowledge Graph | Automated semantic relationship inference | Low | Art. 13 | Traceability to source documents; no autonomous actuation | Retrieval audit logs | ✅ Active |
| AR-007 | GPU Kernel Scheduler | Workload prioritization (automated) | Low | — | KernelSLAGuard fail-safe; timeout/exhaust tests (`src/gpu/ROADMAP.md §WaveA`) | GPU SLA metrics | ✅ Active |
| AR-008 | AI Safety Chaos Test | Adversarial input / boundary testing | N/A (test-only) | Art. 9 (testing) | Isolated test environment; no production deployment | CI chaos test runs | ✅ CI-only |

---

## High-Risk Functions (EU AI Act Annex III)

No ThemisDB component is currently classified as **High-Risk** under EU AI Act Annex III (e.g., biometric identification, critical infrastructure, employment decision systems). The AI functions in this registry are general-purpose retrieval/inference assistants.

If deployment context changes (e.g., integration with employment, healthcare, or critical infrastructure systems), re-classification and a conformity assessment are required before deployment.

---

## Human-in-the-Loop Controls

| Component | HITL Mechanism | Source |
|---|---|---|
| LLM Judge | Override via application layer; score exposed to operator | `include/rag/llm_judge_integration.h` |
| Ethics AI | Ethics plugin gate (partial; in progress) | `plugins/themisdb_ethic_ai` |
| AI Snapshot Cleanup | Human approval for bulk deletions | `include/security/ai_snapshot_cleanup.h` |

---

## Review Cadence

- Quarterly risk registry review aligned with BSI C5 quarterly drill cycle
- Next review: Q4 2026
- Registry is updated on each major release (vX.Y.0)

---

## References

| Document | Scope |
|---|---|
| `audit/docs/compliance/EU_AI_ACT_COMPLIANCE.md` | Full compliance mapping |
| `audit/docs/compliance/EU_AI_ACT_RISK_MAPPING.md` | Module-level risk mapping |
| `docs/model_cards/MODEL_CARD_LLM.md` | LLM model card |
| `docs/model_cards/MODEL_CARD_RAG.md` | RAG model card |
| `include/utils/audit_logger.h` | Audit event definitions |
