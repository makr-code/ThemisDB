### Context

This issue implements the roadmap item '`LLMIngestionAdapter` Phase 2: Wire llama.cpp' for the ingestion domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `LLMIngestionAdapter` Phase 2: Wire llama.cpp

### Goal

Deliver the scoped changes for `LLMIngestionAdapter` Phase 2: Wire llama.cpp in src/ingestion/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `LLMIngestionAdapter` Phase 2: Wire llama.cpp
**Priority:** High
**Target Version:** v1.8.0

`ingestion/llm_adapter.cpp` documents "Phase 2: wire to Mistral 7B via llama.cpp" and contains 3 explicit Phase 2 TODOs/comments. The adapter currently returns an empty embedding vector from a stub lambda (line 77: "Temporary fallback until Phase 2 wiring is complete").

**Implementation Notes:**
- `[ ]` Uncomment `#include "llm/llama_resource_manager.h"` (line 22) and wire the `ILLMAdapter::embed()` callback to `LlamaResourceManager::embed()` when `THEMIS_ENABLE_LLM` is defined.
- `[ ]` Replace the naive line-by-line JSON parser at line 130 ("Phase 2 TODO: replace with a proper JSON parser") with `nlohmann::json::parse()`.
- `[ ]` Add a Phase 2 model health check at `initialize()` (line 47: "Phase 2: check that the model file exists and is readable") — fail fast with a clear error if the model file is missing rather than silently using the stub embedding.
- `[ ]` Add integration tests for `LLMIngestionAdapter` with a real (small) GGUF model file.

---


**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented (Issue: INGESTION-MISSING-001, PR: 2026-03-11)

Replace the simulated HTTP response stubs in `api_connector.cpp` and `huggingface_connector.cpp` with a real `libcurl`-based `HttpClient` class. The stubs currently return hardcoded JSON payloads, making both connectors non-functional in production deployments.

**Implementation Notes:**
- `hfHttpGet()` / `hfHttpPost()` in `huggingface_connector.cpp` and `apiHttpGet()` / `apiHttpPost()` in `api_connector.cpp` use `curl_easy_perform` with TLS verification, Bearer-Token header, and configurable timeout.
- `RetryConfig::ca_bundle_path` (std::string, default empty) added to `include/ingestion/ingestion_manager.h`. When non-empty, `CURLOPT_CAINFO` is set to override the system CA bundle. `CURLOPT_SSL_VERIFYPEER = 1L` is always enabled.
- `ca_bundle_path` is parsed from `SourceConfig::options["ca_bundle_path"]` in both `initialize()` methods and can also be set directly via `setRetryConfig()`.
- Both connectors expose `setHttpGetForTesting()` / `setHttpPostForTesting()` injection points so unit tests run without network access.

**Performance Targets:**
- HTTP GET round-trip to a local test server ≤ 5 ms overhead vs. raw TCP (measured with `benchmarks/ingestion_bench.cpp`).
- Handle pool of 16 reusable CURL handles per thread; handle acquisition must not block under normal load.

---

### Acceptance Criteria

- [ ] Uncomment `#include "llm/llama_resource_manager.h"` (line 22) and wire the `ILLMAdapter::embed()` callback to `LlamaResourceManager::embed()` when `THEMIS_ENABLE_LLM` is defined.
- [ ] Replace the naive line-by-line JSON parser at line 130 ("Phase 2 TODO: replace with a proper JSON parser") with `nlohmann::json::parse()`.
- [ ] Add a Phase 2 model health check at `initialize()` (line 47: "Phase 2: check that the model file exists and is readable") — fail fast with a clear error if the model file is missing rather than silently using the stub embedding.
- [ ] Add integration tests for `LLMIngestionAdapter` with a real (small) GGUF model file.
- [ ] `hfHttpGet()` / `hfHttpPost()` in `huggingface_connector.cpp` and `apiHttpGet()` / `apiHttpPost()` in `api_connector.cpp` use `curl_easy_perform` with TLS verification, Bearer-Token header, and configurable timeout.
- [ ] `RetryConfig::ca_bundle_path` (std::string, default empty) added to `include/ingestion/ingestion_manager.h`. When non-empty, `CURLOPT_CAINFO` is set to override the system CA bundle. `CURLOPT_SSL_VERIFYPEER = 1L` is always enabled.
- [ ] `ca_bundle_path` is parsed from `SourceConfig::options["ca_bundle_path"]` in both `initialize()` methods and can also be set directly via `setRetryConfig()`.
- [ ] Both connectors expose `setHttpGetForTesting()` / `setHttpPostForTesting()` injection points so unit tests run without network access.
- [ ] HTTP GET round-trip to a local test server ≤ 5 ms overhead vs. raw TCP (measured with `benchmarks/ingestion_bench.cpp`).
- [ ] Handle pool of 16 reusable CURL handles per thread; handle acquisition must not block under normal load.

### Relationships

- Roadmap row: #73 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/ingestion/FUTURE_ENHANCEMENTS.md#llmingestionadapter-phase-2-wire-llamacpp
- Source key: roadmap:73:ingestion:v1.8.0:llmingestionadapter-phase-2-wire-llamacpp

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:73:ingestion:v1.8.0:llmingestionadapter-phase-2-wire-llamacpp -->
<!-- roadmap-ref: row=73;module=ingestion;target=v1.8.0 -->
<!-- roadmap-detail: src/ingestion/FUTURE_ENHANCEMENTS.md#llmingestionadapter-phase-2-wire-llamacpp -->
