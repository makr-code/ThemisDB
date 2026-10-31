---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# RAG Trust Boundaries

AI provenance note: Exact model backend not verifiable from final document text alone.

## Purpose

This document identifies where RAG data and decisions cross trust boundaries
and which responsibilities belong to callers. It is for service implementers,
security reviewers, and operators. The public headers and current
implementations define what the module actually enforces; the module
`SECURITY.md` and `PRODUCTION_REQUIREMENTS.md` state intended requirements and
must not be treated as proof that every integration path enforces them.

## Trust boundaries

| Boundary | Data crossing it | Required control |
|---|---|---|
| Client/service to retrieval | Query, tenant identity, collection, requested scope | Authenticate and authorize before choosing the retrieval scope. A collection string is not an authorization token. |
| Search backend to RAG components | Candidate IDs, contents, scores, metadata | Enforce tenant and document ACL filters in the search layer; validate IDs and provenance before fusion or assembly. |
| Ingestion caller to sinks | Source text, MIME/filename, extracted entities and chunks | Validate source permissions and metadata; handle partial writes and independently verify sink persistence. |
| Retrieved content to prompt/generation/judge | Untrusted document text and query | Treat content as data, not instructions. Apply detector/sanitizer controls on each relevant caller path and keep quality checks separate from authorization. |
| Model artifact to inference | ONNX/model files and tokenizer/config | Obtain artifacts from an approved source, record hashes and runtime versions, and apply the model loader's configured integrity checks. |
| RAG process to logs/telemetry | IDs, scores, event fields, baggage, error text | Redact sensitive values; restrict access and retention; do not place secrets, raw queries, or retrieved content in telemetry fields. |
| RAG decision to client | Retrieved evidence, generated answer, citations, quality status | Apply caller-owned response policy; do not silently treat an unavailable or failed judge as a pass. |

## Authentication and authorization

The RAG interfaces described here operate on supplied text, candidate
documents, and collection identifiers. `HybridRetriever::fuse()` receives
already-produced candidates; `RAGContextAssembler` receives chunks; neither
accepts an authenticated principal or an access-policy object. `RAGIngestionBridge`
validates input sizes and metadata syntax but does not itself establish that a
caller may write to a collection.

Therefore:

- Authenticate the request and authorize tenant, collection, document, and
  model access in the service/search/storage layers.
- Apply ACL filters before returning candidates to RAG. Do not rely on
  post-retrieval prompt sanitization to prevent cross-tenant disclosure.
- Ensure index writers and readers use the same tenant/collection mapping;
  test isolation with adversarial IDs and duplicate document IDs.
- If authorization state is missing, stale, or cannot be evaluated, fail closed
  in the owning access-control layer.

## Prompt injection and content provenance

`PromptInjectionDetector` uses configured pattern rules, Unicode checks, and
injection-density scoring. `PromptInjectionSanitizer` replaces matched
fragments above its threshold and can strip direction-override characters.
These controls reduce known risks but are heuristic: false positives and
unmatched attacks remain possible, and sanitizing one component does not prove
another path received sanitized data.

Keep the source ID and retrieval provenance attached to each chunk. If a
finding is high severity, block or escalate according to the calling service's
policy and review/quarantine the upstream source. Do not use a positive quality
score or an LLM judge as proof that input is benign or authorized.

## Model and dependency integrity

Use approved query/passage encoder pairs with matching preprocessing and
dimensions. A deterministic vectorizer fallback is not a semantic substitute
for a model. The current reranker loader checks a `.sha256` sidecar only when
one exists; it warns and proceeds when the sidecar is absent. Its
`require_model_checksum` and `expected_model_checksum` configuration fields
are not enforced by the implementation. Verify model artifacts independently
against a trusted digest before promotion.

The reranker also does not currently create an ONNX inference session:
`loadModel()` validates a file and sets a state flag, while `computeScore()`
uses the heuristic scorer. Neither successful loading nor `used_model == true`
proves that model inference occurred. Unit tests using placeholder model files
validate code paths, not artifact authenticity or semantic relevance.

## Sensitive-data handling

- Do not add raw query/document contents, prompts, model credentials, or
  authorization tokens as telemetry attributes or baggage.
- `QualityControlPipeline` currently logs the first 50 characters of a query;
  treat application logs as potentially containing user data and apply the
  deployment's access and retention controls.
- The injection detector logs finding count/severity and input length rather
  than matched text, but exceptions and caller-provided span attributes may
  still contain sensitive data.
- Restrict access to evaluation results and exported traces, which may expose
  document IDs, tenant IDs, quality judgments, and operational behavior.

## Security incident handling

1. Contain the affected retrieval or generation path using service-level
   controls; do not disable authorization or release a failed quality result.
2. Preserve request IDs, timestamps, relevant artifact/index versions, and
   redacted detector findings.
3. Identify the source record and assess other collections/tenants where it
   could have been indexed.
4. Revoke or quarantine the source through the owning ingestion/index system;
   verify removal from the active index and caches before resuming.
5. Run the focused prompt-injection, ingestion, and tenant-isolation tests
   owned by the integration. Record any remaining coverage gap.
6. Escalate suspected cross-tenant exposure, model artifact compromise, or
   secret disclosure through the repository's security incident process.

## References

- [Module security overview](./SECURITY.md)
- [Production requirements](./PRODUCTION_REQUIREMENTS.md)
- [Runbook](./RUNBOOK.md)
- [Operations](./OPERATIONS.md)
- [API boundaries](./API.md)
- `include/rag/hybrid_retriever.h`
- `include/rag/rag_context_assembler.h`
- `include/rag/rag_ingestion_bridge.h`
- `include/rag/prompt_injection_detector.h`
- `include/rag/reranker.h`
- `src/rag/prompt_injection_detector.cpp`
- `src/rag/quality_control_pipeline.cpp`
