---
AI_Used: true
AI_Models: unknown
AI_Contributor: "ThemisDB documentation orchestrator"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# RAG Production Requirements

## Purpose and status

This document states integration requirements for deployments using RAG. A
requirement below is not evidence that the RAG module enforces it on every
request path or that a deployment is production-ready. Current behavior is
described by the public headers and implementations; the service integrating
them owns authorization, composition, and response policy.

## Security and response requirements

| Requirement | Component evidence | Current enforcement boundary |
|---|---|---|
| Scan untrusted queries and retrieved content for injection | `PromptInjectionDetector` / `PromptInjectionSanitizer` | Explicit APIs; callers must invoke them on the paths they own. Heuristic detection is not a safety guarantee. |
| Authorize tenant, collection, document, and model access | Search, storage, and service access-control layers | Not enforced by `HybridRetriever`, `RAGContextAssembler`, or `RAGIngestionBridge`. Filter candidates before passing them to RAG. |
| Apply a response policy to evaluation results | `QualityControlPipeline` / `RAGJudge` return result objects | Caller responsibility. The module does not impose a universal response gate. |
| Handle partial ingestion writes | `RAGIngestionBridge::indexDocument()` returns `IndexResult` | Check `ok`, counts, and logs; independently verify vector/graph sink state. |
| Verify model artifact integrity | `CrossEncoderReranker::loadModel()` | A present `.sha256` sidecar is checked. Missing sidecars warn and continue; `require_model_checksum` and `expected_model_checksum` are not enforced. Verify hashes independently. |
| Protect telemetry and logs | Service logging and telemetry integration | The quality-control implementation logs up to the first 50 query characters. Apply redaction, access controls, and retention limits at the service boundary. |

## Implemented resource bounds

The ingestion bridge rejects empty or oversized document input and validates
collection and metadata fields. The current source limits include a 5 MiB
document maximum, a 256-character collection maximum, and bounded metadata
values. These checks are input-validation controls, not authorization or
content-safety controls. Consult `src/rag/rag_ingestion_bridge.cpp` for the
current constants and rejection behavior.

`RAGContextAssembler` accounts for a configured model context window and
reserves response tokens. Its result describes an assembly attempt; the caller
must use the returned budget when constructing the generation request.

## Operational requirements

- Treat monitor values as component-local measurements unless the integrating
  service establishes collection, persistence, and alert delivery.
- `OTELSpanEmitter` currently serializes local span data but does not transmit
  it to an OTLP endpoint.
- `CostAttributionTracker::Initialize(path)` does not open RocksDB or persist
  cost records. `IsPersistentStorageAvailable()` reflects directory setup, not
  a working persistent backend. Do not rely on it for durable accounting.
- Reranker model loading does not perform ONNX inference in the current
  implementation; its score path is heuristic.
- Evaluation thresholds and benchmark targets are acceptance criteria only.
  Do not report them as measured outcomes without reproducible dataset and
  benchmark evidence.

## Validation references

Focused tests are registered from `tests/rag/CMakeLists.txt`; test sources
validate their covered behavior, not complete deployment wiring. The root
`benchmarks/CMakeLists.txt` conditionally registers `bench_rag_evaluation`,
`bench_rag_hybrid_retriever`, and `bench_rag_ethics` when Google Benchmark and
the corresponding source files are available. This registration does not
provide representative evaluation corpora or prove any performance target.

For source-level API contracts, see [API.md](API.md) and
[IMPLEMENTATION.md](IMPLEMENTATION.md). For trust boundaries, see
[TRUST.md](TRUST.md); for current operational caveats, see [OPERATIONS.md](OPERATIONS.md).
