---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: DOCUMENTATION_GOVERNANCE.md; .github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md; .github/copilot/module-quick-rules.md; include/process/process_model_manager.h; src/process/process_model_manager.cpp; include/process/llm_process_descriptor.h
Related_Issue: "#7139"
Related_Milestone: DOC-WEEKLY-2026-44
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module Embedding Integration

**Purpose and audience:** Explain where vector data enters process-model and process-instance retrieval, and what an integrator must configure. This is not a claim of a self-contained embedding service.

**Provenance:** Source level: level 1 (`src/process`); SOT domain: module-behavior; issue: #7139; milestone: `DOC-WEEKLY-2026-44`. Governance: [DOCUMENTATION_GOVERNANCE.md](../../DOCUMENTATION_GOVERNANCE.md), [module documentation guidelines](../../.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md), and [Copilot quick rules](../../.github/copilot/module-quick-rules.md). Exact model backend not verifiable from final document text alone.

## Supported paths

### Process-model vectors

`ProcessModelRecord` has an optional `std::vector<float> embedding`. `ProcessModelManager::setEmbedder()` accepts a caller-provided function. When configured, `save()` invokes it only if the record has no vector, using the concatenation of the model name, short description, and long description. The generated vector is stored with the model record. An exception from the callback is logged; the save path continues without that generated vector.

`setVectorIndex()` optionally connects a `VectorIndexManager`. A saved non-empty model vector is upserted when the integration is wired. `findSimilar()` accepts a query vector from its caller and uses the configured vector index when available, otherwise attempting a linear cosine scan over stored model vectors. Index initialization, vector dimensionality, and the embedder implementation are integration responsibilities.

`LlmProcessDescriptor::Config::include_embedding` controls whether a non-empty record vector is copied into the descriptor JSON. The descriptor serializes existing data; it is not itself an embedding generator.

### Similar process instances

`ProcessGraphRag::findSimilarCases()` looks for the reference instance vector under the `proc:inst_emb:<instance-id>` key and scans stored instance vectors for cosine similarity. If this path yields no candidates, it falls back to Jaccard similarity over variable-key sets for instances from the same process definition. This is separate from the model manager's optional vector index.

## Integration checklist

1. Choose and own the text-embedding implementation; the process manager accepts a callback but does not choose a model/backend.
2. Ensure query vectors and stored vectors use compatible dimensions and semantics before calling `findSimilar()`.
3. If using the HNSW integration, initialize the vector index for the documented process-model object and dimension before saving vectors; verify index status and the fallback behavior.
4. Decide explicitly whether descriptor consumers may receive vector values; leave `include_embedding` disabled unless required.
5. Test missing vectors, callback failure, incompatible dimensions, empty query vectors, unavailable index, and the instance-similarity fallback.

## Limits and interpretation

- No embedding model, endpoint, credentials policy, vector dimension, or quality score is selected by these interfaces.
- The descriptor's embedding field is optional pass-through data. It does not create an embedding index or promise that one exists.
- Cosine/Jaccard similarity is a ranking signal, not proof that two processes or cases are equivalent.
- Retrieval result quality and index freshness depend on the caller's embedder and index lifecycle. The module interfaces do not provide a complete operational freshness protocol.

## Canonical references

- [`include/process/process_model_manager.h`](../../include/process/process_model_manager.h) — optional embedder, vector-index, and `findSimilar()` contracts.
- [`src/process/process_model_manager.cpp`](process_model_manager.cpp) — callback invocation, persistence, index upsert, and linear fallback.
- [`include/process/llm_process_descriptor.h`](../../include/process/llm_process_descriptor.h) · [`src/process/llm_process_descriptor.cpp`](llm_process_descriptor.cpp) — optional embedding field in descriptor output.
- [`include/process/process_graph_rag.h`](../../include/process/process_graph_rag.h) · [`src/process/process_graph_rag.cpp`](process_graph_rag.cpp) — stored instance vectors and similar-case behavior.
- [Research notes](RESEARCH.md) · [API contracts](API.md) · [Module overview](README.md)
