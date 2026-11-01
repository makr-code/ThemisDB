---
AI_Used: true
AI_Models: unknown
AI_Contributor: "ThemisDB documentation orchestrator"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Security — RAG Module

<!-- Status: source-aligned | reviewed: 2026-10-31 -->

Report vulnerabilities through the repository's root `SECURITY.md`.

## Security boundary

RAG exposes retrieval, context, evaluation, ingestion, and safety components.
Their presence does not mean every request path invokes them. The service that
composes these components owns authentication, authorization, control ordering,
and response policy.

| Risk | Current boundary |
|---|---|
| Prompt injection in queries or retrieved content | `PromptInjectionDetector` and `PromptInjectionSanitizer` are explicit, heuristic APIs. The caller chooses where to scan, block, or sanitize. |
| Unauthorized or cross-tenant retrieval | `HybridRetriever` accepts candidate lists and `RAGContextAssembler` accepts chunks; neither accepts an authenticated principal or authorization policy. Filter candidates in the owning search/access-control layer. |
| Ingestion into an unauthorized collection | `RAGIngestionBridge` validates input shape and limits, but does not authorize collection writes. |
| Unsafe or low-confidence generated output | Evaluation APIs return results; the caller must apply its response policy. They are not automatically enforced by every server path. |
| Sensitive data in logs and telemetry | The quality-control implementation logs up to the first 50 query characters. Review application log access and retention; do not add raw content or credentials to telemetry. |
| Model artifact integrity | The reranker checks a `.sha256` sidecar if present and warns if absent. Its `require_model_checksum` and `expected_model_checksum` configuration fields are not enforced. Verify model digests independently. |

## Current controls and limits

- The injection detector uses pattern/heuristic checks. A clean scan is not
  proof that input is safe; sanitizing one path does not establish that another
  path received sanitized data.
- The quality-control pipeline does not itself enforce authorization or block
  every response. A caller must inspect its returned status.
- The ingestion bridge has input-size and metadata validation, but vector and
  graph writes are optional. A result with `ok == true` does not by itself prove
  that every configured sink persisted data.
- `CrossEncoderReranker::computeScore()` currently uses the heuristic scorer.
  `loadModel()` does not establish ONNX inference; `used_model` is not proof of
  a model-scored result.
- `OTELSpanEmitter` does not send spans over the network. Avoid treating its
  in-process counters as collector acknowledgements.

## Required integration practices

1. Authenticate and authorize tenant, collection, document, and model access in
   the owning service/search/storage layers.
2. Apply access-control filters before candidate documents enter RAG. Prompt
   sanitization and quality scoring are not authorization controls.
3. Treat retrieved text as untrusted data. Invoke scanning and any blocking or
   sanitization policy on each relevant query/document path.
4. Preserve source IDs and retrieval provenance. Verify partial writes and
   model artifacts independently.
5. Redact logs, constrain access and retention, and avoid raw queries,
   retrieved content, prompts, and credentials in telemetry fields.
6. On uncertain authorization or failed safety evaluation, apply the
   integration's explicit fail-closed policy; the module does not supply a
   universal policy.

## Source references

- `include/rag/prompt_injection_detector.h`
- `src/rag/prompt_injection_detector.cpp`
- `include/rag/rag_ingestion_bridge.h`
- `src/rag/rag_ingestion_bridge.cpp`
- `include/rag/quality_control_pipeline.h`
- `src/rag/quality_control_pipeline.cpp`
- `include/rag/reranker.h`
- `src/rag/reranker.cpp`
- `src/rag/otel_span_emitter.cpp`
- [Trust boundaries](TRUST.md)
- [Production requirements and implementation limits](PRODUCTION_REQUIREMENTS.md)
