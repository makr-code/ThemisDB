# Model Card: ThemisDB RAG Pipeline

**Model:** Retrieval-Augmented Generation (RAG) Pipeline
**Version:** v2.4.0
**Date:** 2026-09-21
**Owner:** ThemisDB Contributors
**Source Module:** `src/rag/`, `include/rag/`
**EU AI Act Scope:** Article 13 (Transparency), Article 22 (Bias & Fairness)

---

## 1. Purpose & Application

**Primary Use:** Combines ThemisDB's vector search, knowledge graph retrieval, and LLM inference to answer natural language queries using database-grounded evidence.

**Context:** Core production feature. Receives a query, retrieves semantically relevant chunks from the vector index and knowledge graph, constructs a prompt, invokes the configured LLM backend, and scores the response via `LLMJudgeIntegration`.

**Regulatory Scope:**
- [x] Transparency requirement (Article 13) — AI decisions logged with `[AI_DECISION]` marker
- [ ] High-Risk AI — Not classified as high-risk; general-purpose retrieval assistant
- [x] Bias & Fairness monitoring — LLM judge evaluation is applied to scored responses

---

## 2. Pipeline Components

| Stage | Component | Source |
|---|---|---|
| Query encoding | Vector search / HNSW index | `src/index/`, `include/index/` |
| Context retrieval | Knowledge graph neighbors | `src/rag/knowledge_graph_retriever.cpp` |
| Prompt assembly | RAG prompt builder | `src/rag/` |
| Inference | LLM Plugin (see LLM Model Card) | `src/llm/` |
| Response scoring | `LLMJudgeIntegration` | `src/rag/llm_judge_integration.cpp` |

---

## 3. Known Limitations

- Retrieval quality depends on vector index coverage; sparse or stale indices yield lower relevance.
- RAG grounding reduces but does not eliminate hallucination risk.
- Judge scoring requires a configured LLM judge backend; if unavailable, `llm_unavailable` is returned — no silent mock fallback.
- Multi-modal RAG (image/audio) is partial; ONNX clip integration is in progress.

---

## 4. Transparency Indicators

- All RAG-assisted decisions produce `[AI_DECISION]` audit events.
- Retrieval sources are traceable to document IDs and vector index offsets in the audit log.
- `LLMJudgeIntegration::isMockMode()` is permanently `false` in production.
- Source: `include/rag/llm_judge_integration.h`, `src/rag/llm_judge_integration.cpp`

---

## 5. Risk Indicators

| Risk | Level | Mitigation |
|---|---|---|
| Context poisoning (malicious documents in index) | Medium | Document provenance logging; input sanitization |
| Relevance drift (index staleness) | Low–Medium | Incremental index updates; HNSW refresh policies |
| Score manipulation (judge gaming) | Low | Judge model is operator-controlled; audit logging of all scores |
| Privacy leakage (PII in retrieved context) | Medium | Encrypted storage; VRAM secure clear (`src/security/`) |

---

## 6. Compliance References

| Document | Scope |
|---|---|
| `audit/docs/compliance/EU_AI_ACT_COMPLIANCE.md` | Full EU AI Act compliance mapping |
| `audit/docs/compliance/EU_AI_ACT_RISK_MAPPING.md` | Module-level risk mapping |
| `docs/model_cards/MODEL_CARD_LLM.md` | LLM backend model card |
| `include/rag/llm_judge_integration.h` | Judge integration API |
