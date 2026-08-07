> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-08-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/search/FUTURE_ENHANCEMENTS.md -->

# Search Module — Public Header Future Enhancements

**Module Path:** `include/search/`
**Canonical implementation enhancements:** [`../../src/search/FUTURE_ENHANCEMENTS.md`](../../src/search/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/search/`. Runtime hardening and benchmark work remain tracked in:

→ [`../../src/search/FUTURE_ENHANCEMENTS.md`](../../src/search/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Retrieval, fusion, and rerank outcomes must remain explicit and deterministic.
- `[x]` Partial-result and degraded-shard behavior must remain visible to consumers.
- `[x]` LLM-assisted and multimodal headers must preserve optional/degradable semantics.
- `[x]` Analytics and streaming contracts must remain consumable by API and monitoring layers.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| hybrid/distributed retrieval APIs | `hybrid_search.h`, `distributed_hybrid_search.h` | Query and API layers | ✅ Stable |
| query shaping and rerank APIs | `query_expander.h`, `llm_query_rewriter.h`, `llm_reranker.h` | Search pipeline composition | ✅ Stable |
| result stream / analytics APIs | `search_result_stream.h`, `search_analytics.h` | UI and operational tooling | ✅ Stable |
| multimodal and federated APIs | `multi_modal_search.h`, `federated_search.h` | Advanced retrieval deployments | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Document degraded-shard, overlap-variance, and candidate-limit behavior consistently across distributed/public retrieval headers.
- Standardize naming for search incident, capability, and rerank-result DTOs.
- Clarify optional/degradable semantics for LLM-assisted and multimodal search headers.

### Medium-Term (Q4 2026)

- Introduce `search_incident.h` and `search_capability_profile.h` for shared diagnostics/capability exchange.
- Document benchmark-reference expectations for hybrid merge, rerank, and distributed hot paths.
- Align federated and distributed headers around a shared partial-result vocabulary.

### Long-Term

- Add extension hooks for custom fusion and reranking strategies without replacing search core contracts.
- Unify retrieval, rerank, and analytics outputs under a shared search result envelope.
