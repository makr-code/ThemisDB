# RAG: Providers, Consumers and Helpers

The internal knowledge path is independent of the endpoint and of generation:

```text
HTTP / AQL / YAML orchestration / iterative retrieval controller
                           |
                   rag::RAGProvider
                           |
                       QueryEngine
                           |
       existing relational / graph / geo / vector access paths
                           |
                  RocksDB / BaseEntity

consumer -> optional LLM provider -> llama.cpp
```

## Responsibilities

| Role | Implementation | Responsibility |
|---|---|---|
| Knowledge provider | `rag::RAGProvider` | Execute typed queries, materialize evidence, enforce output budgets and report errors |
| Inference provider | Existing `ILLMPlugin` implementations | Embeddings and optional generation; own model, GPU, Flash-Attention and KV configuration |
| Consumer | HTTP, `LLMAQLHandler`, `AIOrchestrator`, iterative controllers | Supply the query and trusted execution dependencies; decide whether to generate or retrieve again |
| Pure helpers | `makeRAGCandidate`, `toRAGDocuments`, candidate conversion | Transform already supplied data without storage access or inference |
| Generation preparation | `prepareRAGRequest` | Shared sanitization, formatting and context-budget assembly for both llama.cpp wrappers |

These are responsibilities, not an inheritance hierarchy. There is one concrete
knowledge provider, no new plugin manager, no per-index RAG adapter family and no
mandatory in-process HTTP/MCP/JSON-RPC hop.

## Contract and integration

See [API.md](API.md) for request/result semantics, supported query combinations,
ownership, cancellation and authorization requirements.

`RAGCandidate` is shared with `ModularRAGPipeline`; its existing field layout is
preserved. Retrieval does not require the pipeline's Generate stage or a loaded
model when callers provide typed queries/embeddings. Both existing llama.cpp
wrappers use the same generation-preparation helper. GPU and attention settings
do not belong in the retrieval request.

Re-retrieval is a consumer decision: change the typed query, filters or candidate
budget and call the same provider again. This change does not automatically wire
Self-RAG/FLARE callbacks into a token-generation loop.

## Compatibility and boundaries

- HTTP RAG now performs retrieval before generation. The direct server route also
  requires `data:read`/`data.read` authorization for the selected collection in
  addition to its existing LLM authorization.
- Converted evidence uses stable document IDs as sources and collection/provenance
  metadata, rather than copying all source-row fields into generation metadata.
- AQL RAG requires `setQueryEngine()`; missing dependencies and retrieval failures
  no longer continue as generation with empty context or primary keys as content.
- YAML RAG uses `setQueryEngine()` for internal retrieval. Supplied documents and
  an explicitly registered external `docs_search` tool remain supported when no
  internal engine is configured; provider errors never fall back to that tool.
- gRPC's existing RAG RPC remains an externally supplied-document consumer. Its
  protobuf contract is not expanded into a new retrieval protocol.
- `rag_integration_helpers.h` retains legacy storage-acquisition overloads for
  source compatibility. They are provider-side compatibility code, not pure
  helpers. New consumers use `RAGProvider` plus the candidate-only overload.
- Internal C++ APIs are not a compiler-independent plugin ABI. The existing
  `ILLMPlugin` vtable and `RAGContext` layout are unchanged.

The separate `src/retrieval` module remains an artifact/validator facility; it is
not the generic knowledge-query provider.

## Evidence

Focused regression sources cover canonical binary entity loading, graph access
denial, content budgets, cancellation, invalid embeddings, evidence conversion,
shared generation preparation and literal-safe prompt templates. Their presence
is not evidence that a full server/model/GPU deployment has been validated.
