---
Author: makr-code
Created: 2026-11-11
Last Updated: 2026-11-11
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: makr-code
---

[docs](../README.md) > [RAG](README.md) > [API reference](API.md)
Datum: 2026-11-11
Status: review
Primary (Quelle der Wahrheit):
- `include/rag/rag_candidate.h`
- `include/rag/rag_retrieval_helpers.h`
- `include/ingestion/ingestion_sinks.h`
- `src/ingestion/ingestion_sinks.cpp`
- `src/content/content_manager.cpp`
- `src/query/query_engine.cpp`
Bezug / Reference: Core Provider/Consumer implementation follow-up; typed RAG retrieval API.

# Internal RAG API

> Exact model backend not verifiable from final document text alone.

Primary contracts:
[`rag_provider.h`](../../include/rag/rag_provider.h),
[`rag_candidate.h`](../../include/rag/rag_candidate.h),
[`rag_helpers.h`](../../include/llm/rag_helpers.h).

## Retrieval

```cpp
#include "rag/rag_provider.h"

themis::query::ConjunctiveQuery query;
query.table = "documents";
query.predicates.push_back({"category", "manual"});

themis::rag::RetrievalRequest request;
request.query = query;
request.max_candidates = 10;

// engine is an existing QueryEngine with its access policy already configured.
themis::rag::RAGProvider provider(engine);
auto result = provider.retrieve(request);
if (!result) {
    // Propagate result.error(); do not generate with an error string as evidence.
    return;
}
// result->candidates is usable without any generation backend.
```

`RetrievalQuery` reuses existing contracts rather than introducing a query DSL:

| Query alternative | Execution | Constraints |
|---|---|---|
| `query::ConjunctiveQuery` | Existing relational/secondary-index query and bounded entity hydration | Equality, range, fulltext and spatial predicates retain QueryEngine semantics |
| `query::VectorGeoQuery` | Existing vector or vector+geo execution | Caller supplies a finite, nonzero embedding; extra predicates and spatial restrictions remain hard filters |
| `query::ContentGeoQuery` | Existing fulltext+geo execution | Requires a spatial filter, collection, text field and search text |
| `rag::GraphRetrievalQuery` | Existing general graph traversal | Depth range within 0..16; collection checks precede vertex loading and expansion |

One request executes one typed query, including the combinations supported by
that query. It does not invent a universal optimizer or automatically fuse
independent query alternatives. QueryEngine remains responsible for plans,
index selection and storage reads.

Collections/graph keys must respect the existing key-schema separator rules.
The provider rejects primary-key fast-path requests combined with additional
predicates, because that QueryEngine fast path intentionally ignores predicates.
This prevents a RAG caller from accidentally dropping a hard restriction.

### Results and budgets

- `max_candidates`: 1..1000 returned candidates.
- `max_content_bytes`: 1..1 MiB cumulative evidence-content bytes. Overflow is an
  explicit resource error, not partial success.
- `RAGCandidate`: owned ID, content, higher-is-better score, source namespace and
  provenance annotations. Identity is `(source_namespace, doc_id)`.
- `rejected_documents`: materialized records without usable identity/content.
- `truncated`: the provider cut an available result list to the candidate limit;
  false does not prove that the underlying index has no additional matches.
- No matches is a successful empty result. Generating consumers using internal
  retrieval reject empty evidence instead of silently switching to ungrounded
  generation.
- Real structured rows can be serialized as evidence. Missing records,
  identifier-only graph placeholders and error messages are not evidence.

### Source identity and vector text

`RAGCandidate::source_reference` is an optional `RAGSourceReference` containing
the source `type`, exact `collection` and exact `key`, plus provenance and an
optional version value. The candidate helper reads these from complete
`source_ref_*` fields; it does not dereference the locator or guess keys.

The current ingestion sink stores `text_snippet` and an exact source reference
on each vector entity. ContentManager stores chunk `text` on its vector entity
and points to the exact `chunk:<id>` source key (the indexed vector key is
`chunks:<id>`). Candidate conversion can materialize those in-entity text fields.
QueryEngine permits vector-entity key hydration only when the initialized vector
index namespace exactly matches the query collection, after its existing
relational and legacy table-key lookups. This does not establish a general
authoritative-source fetch path; in particular, normal ingestion's snippet is
not proof that its source locator resolves the complete source text.

The broader Core implementation remains in progress. Real HTTP/AQL/YAML startup
wiring, complete request-context and authorization isolation, the complete
source-role map, and remaining task acceptance criteria are pending; see the
[implementation task](../architecture/CORE_PROVIDER_CONSUMER_IMPLEMENTATION_TASK.md).
The direct HTTP `/api/v1/llm/rag` route retains its existing `requireAccess`
check and now binds its request-local QueryEngine checker to the authorized
collection. This is a collection-boundary guard, not caller or row-level policy
propagation.

Scores are not probabilities and are not calibrated across access paths:
vector scores currently retain the existing `1 - distance` convention, content
queries return BM25 scores, graph retrieval uses inverse depth, and relational
results are unranked ties. Cross-path ranking requires an explicit fusion policy.

### Execution and trust

The provider borrows `QueryEngine`; it must outlive the provider. Configure its
collection access checker before concurrent execution. The provider does not
authenticate callers, infer a tenant from a string or mutate a global engine's
caller identity for each request. Consumer-specific authentication and row-level
policy still belong to the existing trusted execution path.

Cancellation and deadline checks are cooperative: before and after query calls
and between materialization steps. They do not preempt a blocking backend call.
Returned-result limits do not bound all work performed inside the query engine.
The existing graph traversal also retains its own internal result limit.

The provider propagates errors reported by QueryEngine through `Result<T>` with
their codes/context; backend/allocation exceptions reach the consumer boundary.
It adds no success-shaped fallback or invented content. Existing QueryEngine
materialization branches can still skip missing/malformed rows; the provider's
rejection count covers only records that reach it, not every backend read failure.

## Optional generation

`llm::toRAGDocuments()` performs the explicit pure conversion to the existing
`llm::RAGContext::Document` contract. `prepareRAGRequest()` is shared by
`LlamaWrapper` and `LlamaCppPlugin`; neither wrapper retrieves its own documents.

The preparation helper:

1. Sanitizes the query, system prompt, sources and document content.
2. Uses the loaded model window, capped by an optional smaller caller window.
3. Accounts for template/source framing and the response reservation.
4. Uses `RAGContextAssembler` to select whole evidence chunks; if none fit,
   returns an explicit exception rather than generating without supplied evidence.
5. Formats through `RAGPromptBuilder` without interpreting placeholders inside
   inserted evidence or query strings.

Budgets use the existing character-based token estimate, not tokenizer-exact
accounting. Backend tokenization remains authoritative. `tensor_hybrid` and
`tensor_prefix` remain text-memory-slot compatibility modes, not tensor/KV
injection. Templates must contain both `{context}` and `{query}`.

Assembly metadata reports `rag_documents_used`, `rag_context_truncated` and
`rag_blocked_documents`. GPU, Flash-Attention and KV-cache settings remain private
to the inference provider.

## Migration

Inject the existing QueryEngine into AQL/YAML consumers using `setQueryEngine()`.
For YAML, `OrchestratorContext::retrieval_request` optionally supplies any typed
query above; otherwise internal retrieval uses a vector query with
`extra.collection` (default `default`) and `extra.vector_field` (default
`embedding`). Vector embedding generation remains an inference-provider call.

Keep external `docs_search` only when intentionally using an external knowledge
source. Legacy acquisition helpers now throw on embedding/search/storage
failures; a successful empty result means no matches, not a swallowed failure.
Rebuild C++ consumers after header changes. No plugin ABI token, factory export,
`ILLMPlugin` vtable or `RAGContext` field is changed by this consolidation.
