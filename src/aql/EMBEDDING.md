---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Few-Shot Embedding Integration

## Purpose and boundary

This page documents optional semantic ranking for examples used in AQL
assistance. It covers `AQLFewShotExampleLibrary` and
`LLMAQLEmbeddingBridge`; it is not a general vector-search or embedding-model
deployment guide.

`IEmbeddingProvider` is the library's adapter interface. An injected provider
embeds the natural-language request and examples; the library ranks compatible
vectors by cosine similarity. If no provider is installed or an embedding call
returns an empty vector, the library can use its lexical Jaccard relevance
path instead. `LLMAQLEmbeddingBridge` converts handler exceptions to an empty
vector; the library is not a general exception boundary for arbitrary custom
providers.

## Ownership and lifecycle

- `AQLFewShotExampleLibrary::setEmbeddingProvider()` stores a non-owning
  `IEmbeddingProvider*`. The provider must outlive the library's use of it.
- `LLMAQLEmbeddingBridge` holds a non-owning reference to an
  `LLMAQLHandler`; the handler must outlive the bridge.
- `LLMAQLEmbeddingBridge::embed()` returns an empty vector for empty input or
  when the handler throws. The latter is logged with
  `[BRIDGE:ExecutionFailed]`; the library uses lexical ranking when an
  embedding is unavailable.
- Replacing the embedding provider clears the cached embeddings.
  `rebuildEmbeddingIndex()` eagerly computes embeddings; otherwise the
  library can populate them on demand.
- Do not retain a bridge beyond its handler or use a provider pointer after
  its owner is destroyed.

## Integration sequence

1. Create a live `LLMAQLHandler` with the intended embedding provider
   configuration.
2. Create the example library and register or load its examples.
3. Obtain the bridge with `handler.makeEmbeddingBridge()` and keep the handler
   alive for at least as long as the bridge.
4. Set the provider on the library, then optionally call
   `rebuildEmbeddingIndex()` when eager embedding cost is acceptable.
5. Exercise the semantic and provider-failure paths in
   [`test_aql_fewshot_example_library.cpp`](../../tests/aql/test_aql_fewshot_example_library.cpp)
   and the bridge path in
   [`test_aql_bridge_degradation.cpp`](../../tests/aql/test_aql_bridge_degradation.cpp).

## Failure handling and evaluation

An empty embedding is an unavailable-signal condition, not evidence that two
texts are unrelated. Confirm provider readiness and `[BRIDGE:ExecutionFailed]`
logs before investigating ranking behavior. Compare semantic and lexical
ranking on a fixed, reviewed query/example set; record relevance judgments and
provider/configuration details. Do not claim a quality improvement without
that evaluation. See [`RESEARCH.md`](RESEARCH.md) for broader evaluation
limits and [`RUNBOOK.md`](RUNBOOK.md) for operational triage.

## Source references

- [`include/aql/aql_fewshot_example_library.h`](../../include/aql/aql_fewshot_example_library.h)
- [`include/aql/llm_aql_embedding_bridge.h`](../../include/aql/llm_aql_embedding_bridge.h)
- [`llm_aql_embedding_bridge.cpp`](llm_aql_embedding_bridge.cpp)
- [`aql_fewshot_example_library.cpp`](aql_fewshot_example_library.cpp)
