# Model Card: ThemisDB LLM Integration

**Model:** LLM Plugin (pluggable backend — Ollama, llama.cpp, ONNX)
**Version:** v2.4.0
**Date:** 2026-09-21
**Owner:** ThemisDB Contributors
**Source Module:** `src/llm/`, `include/llm/`
**EU AI Act Scope:** Article 13 (Transparency), Article 22 (Bias & Fairness monitoring)

---

## 1. Purpose & Application

**Primary Use:** Language model inference backend for ThemisDB's RAG (Retrieval-Augmented Generation) pipeline and knowledge graph query augmentation.

**Context:** Deployed as a plugin-loadable inference backend. Supports Ollama-hosted models, llama.cpp direct inference, and ONNX runtime. Used for semantic search augmentation, natural language query parsing, and LLM-assisted retrieval scoring.

**Regulatory Scope:**
- [x] Transparency requirement (Article 13) — AI-generated content is marked with `[AI_DECISION]` in audit logs
- [ ] High-Risk AI (Article 6 Annex III) — Not classified as high-risk; general-purpose assistant only
- [x] Bias & Fairness monitoring (Article 22) — LLMJudgeIntegration provides evaluation scoring

---

## 2. Supported Model Backends

| Backend | Interface | Notes |
|---|---|---|
| Ollama | HTTP REST API | Remote or local; `copilotOllamaRouter.endpoint` configurable |
| llama.cpp | Plugin adapter | Direct GGUF loading via `themisdb_llm_llamacpp` plugin |
| ONNX Runtime | Plugin adapter | Clip/embedding models via `src/onnx_clip/` |

Model selection is user-/operator-controlled via plugin configuration. ThemisDB does not ship a bundled LLM.

---

## 3. Known Limitations

- Output quality depends entirely on the operator-supplied model.
- No built-in content filter; operators must enforce content policies at the model or application layer.
- Inference latency is hardware-dependent; no SLA guarantee in non-GPU environments.
- Temporal knowledge cutoff is model-specific.

---

## 4. Transparency Indicators

- All LLM-assisted decisions are logged with `[AI_DECISION]` audit marker via `SecurityEventType` in `include/utils/audit_logger.h`.
- `LLMJudgeIntegration::isMockMode()` returns `false` in production; missing backends return `llm_unavailable` explicitly (no silent mock fallback).
- Source: `include/rag/llm_judge_integration.h`, `src/rag/llm_judge_integration.cpp`

---

## 5. Risk Indicators

| Risk | Level | Mitigation |
|---|---|---|
| Hallucination / factual error | Medium | RAG retrieval grounds answers in database content; judge scoring applied |
| Prompt injection | Medium | Input sanitization in HTTP server; audit logging of all prompts |
| Model supply chain compromise | Medium | Plugin integrity verified via supply-chain workflow SBOM attestation |
| Bias / fairness drift | Low–Medium | `LLMJudgeIntegration` evaluation scoring; ethics-ai plugin (partial) |

---

## 6. Incident & Monitoring

- Abnormal inference latency → `SecurityEventType::ANOMALY_DETECTED` audit event
- Backend unavailability → graceful `llm_unavailable` return; no silent degradation
- Audit log retention: per `docs/operations/RUNBOOKS.md`

---

## 7. Compliance References

| Document | Scope |
|---|---|
| `audit/docs/compliance/EU_AI_ACT_COMPLIANCE.md` | Full EU AI Act compliance mapping |
| `audit/docs/compliance/EU_AI_ACT_RISK_MAPPING.md` | Module-level risk mapping |
| `include/utils/audit_logger.h` | Audit event definitions |
| `src/rag/llm_judge_integration.cpp` | Judge integration and fail-safe behavior |
