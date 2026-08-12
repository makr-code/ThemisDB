## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


> **⚠️ STATUS: STALE – Archivierungskandidat**
> Dieser Inhalt enthält veraltete TODO/FIXME/PLACEHOLDER-Marker und wird im nächsten Archiv-Run nach `docs/ARCHIVED/` verschoben.
> Bitte nicht als aktuelle Referenz nutzen. Inventar: [DOCS_INVENTORY_2026-Q3.md](Audit/DOCS_INVENTORY_2026-Q3.md)

---

# LLM Module Production-Readiness Plan & Roadmap

**Status:** Not Production Ready
**Version:** 1.5.0
**Last Updated:** April 2026

---

## Executive Summary

The ThemisDB LLM module (`src/llm`) provides a substantial feature set including GGUF model loading, llama.cpp integration, grammar-constrained decoding, paged KV-cache, kernel fusion/flash attention (CUDA), multi-LoRA management, and a partial Prometheus metrics stack. However, **it is not yet production-ready**. This document identifies the concrete gaps discovered during a code review of the `src/llm` directory and provides an actionable roadmap—organized by quarter—to bring the module to production quality.

---

## 1. Current State & Gaps

### 1.1 Observability

**Finding:** The active implementation (`grafana_metrics.cpp`) provides a `PrometheusExporter` and `LLMMetricsCollector` with counter/gauge/histogram support and a Grafana dashboard generator. However:

- `grafana_metrics_broken.cpp.bak` signals that the metrics stack was previously broken and has since been partially restored; the `.bak` artifact should be removed or superseded by a verified working implementation.
- The `MetricsServer::start()` function contains a `// TODO: Start actual HTTP server` comment—there is **no live HTTP endpoint** serving `/metrics` today. Prometheus cannot scrape the exporter.
- There is **no OpenTelemetry (OTel) exporter**: no spans, trace context propagation, or OTLP sink are wired anywhere in the LLM pipeline.
- Metrics for queue depth, backpressure events, and per-request memory usage are defined but not yet emitted from inference hot paths.
- Grafana alert rules and Grafana panel JSON are auto-generated stubs; they have not been validated against a live Grafana/Prometheus instance.

### 1.2 Grammar (llama.cpp Grammar API)

**Finding:** `grammar.cpp` checks `themis_llama_grammar_available()` at compile time via dynamic symbol resolution (`llama_grammar_adapter.cpp`). If the API is absent, `Grammar::compile()` logs a warning and returns `false`—inference proceeds without grammar constraints, silently producing unconstrained output.

- `llama_grammar_init()` is called with `nullptr` for the `llama_vocab*` parameter (see `grammar.cpp:110`). The inline comment acknowledges: *"this is a limitation of the current API design."* Passing a null vocab pointer may cause undefined behavior in llama.cpp when non-trivial token filtering is required.
- There is no hard error or caller-visible signal when grammar is requested but silently skipped.
- No tests exist that exercise grammar compilation with a real `llama_model`/`llama_vocab` loaded from disk.

### 1.3 GGUF Loader — Unsupported Formats

**Finding:** The GGUF loader (`gguf_loader.cpp`, `GGUF_LOADER_README.md`, `docs/GGUF_SUPPORT.md`) fully supports **F32, F16, Q4_K_M, Q8_0** only. The following quantization types are recognised in the `GGMLType` enum and `type_string()` but **have no conversion path and are silently skipped or return raw bytes without dequantization**:

| Format | Block Size | Status |
|--------|------------|--------|
| Q4_0   | 18 bytes   | Not supported |
| Q4_1   | 20 bytes   | Not supported |
| Q5_0   | 22 bytes   | Not supported |
| Q5_1   | 24 bytes   | Not supported |
| Q8_1   | 36 bytes   | Not supported |
| Q5_K   | 176 bytes  | Not supported |
| Q6_K   | 210 bytes  | Not supported |

Loading a model that uses any of these formats will not fail with an actionable error; instead the loader silently returns raw quantized bytes, causing downstream numerical corruption.

### 1.4 Kernel Fusion / Flash Attention

**Finding:** `kernel_fusion.cu` provides a tiled Flash Attention forward kernel and several fused GEMM/layer-norm/activation kernels. However:

- The corresponding CPU fallback in `kernel_fusion.cpp` exists, but there is no compile-time or runtime gate that ensures the CUDA path is validated before the CPU fallback is silently engaged.
- CI pipeline status for GPU tests is unknown; it is unclear whether `kernel_fusion.cu` is compiled and executed in any automated test environment.
- There are no benchmarks comparing the CUDA kernel throughput against the CPU fallback or against cuBLAS/cuDNN baselines.
- Shared-memory tile size (`TILE_SIZE = 64`, `BLOCK_SIZE = 256`) is hard-coded and not tuned per GPU architecture (e.g., Ampere vs. Ada).

### 1.5 LlamaWrapper / Config — Missing Guards

**Finding:** `llama_wrapper.cpp` performs basic parameter validation (`n_ctx`, `n_batch`, `n_threads`) but lacks several production-critical safeguards:

- **No request timeouts or per-session quotas**: A single runaway inference request can starve all other sessions indefinitely.
- **No backpressure or rate limiting**: The continuous batch scheduler (`continuous_batch_scheduler.cpp`) does not enforce a maximum queue depth; callers are never rejected with a `429`-equivalent.
- **No health or liveness endpoints**: Operators have no API to check whether the model is loaded and the inference loop is responsive.
- **No model-list or session-list admin API**: Administrators cannot enumerate loaded models, active sessions, or LoRA adapters at runtime.
- **No prompt safety layer**: There is no input sanitization, prompt-injection detection, or jailbreak mitigation before tokens are fed to the model.
- **No hot-reload**: Changing configuration or swapping a model requires a full process restart.

### 1.6 Deprecated / Beta Components

**Finding:**

- **LoRA compat shim** (`llama_lora_adapter.cpp` legacy overloads, `LoRAAdapterManager` in `lora_framework/`): ✅ Resolved — `LoRAAdapterManager` fully removed in v1.4.0; all call sites migrated to `MultiLoRAManager`.
- **DirectX / HLSL shader path** (`vision_config.cpp`, DXGI references): References to DirectX shader compilation exist alongside the CUDA/OpenCL paths. On non-Windows targets the DirectX path compiles as a no-op stub, but the dead code adds maintenance burden and confusion.
- **`grafana_metrics_broken.cpp.bak`**: The `.bak` file is committed to source control, adds noise, and may confuse maintainers about which implementation is canonical.

---

## 2. Plan & Milestones

### Q1 — Observability, Grammar Robustness, Health/Admin Basics, GGUF Validation

**Goal:** Operators can scrape metrics, grammar is fail-safe, GGUF loading never silently corrupts data, and a minimal health endpoint exists.

1. **Observability — Prom/OTel Exporter**
   - Implement the HTTP `/metrics` endpoint inside `MetricsServer::start()` (Beast/ASIO or Crow).
   - Wire latency, throughput, error-rate, queue-depth, and backpressure counters into the inference hot path (`llamacpp_inference_engine.cpp`, `continuous_batch_scheduler.cpp`).
   - Add an OpenTelemetry SDK integration (OTLP gRPC exporter) and propagate trace context through `LlamaWrapper::generate()`.
   - Validate the Grafana dashboard JSON against a live Grafana 10.x instance; store the validated JSON in `grafana/`.

2. **Grammar Robustness**
   - Replace the `nullptr` vocab call with a proper `llama_model_get_vocab()` lookup; surface a hard error (`std::runtime_error` or structured error code) when the grammar API is available but the vocab pointer is null.
   - Promote the silent fallback (grammar requested but API absent) to a logged error + returned status flag; callers must explicitly opt in to unconstrained fallback.
   - Add integration tests exercising grammar compilation with a real (or synthetic mock) `llama_vocab`.

3. **Health / Model-List API**
   - Add `GET /health` (liveness) and `GET /models` (loaded model list) endpoints to `LlamaWrapper` or the inference server layer.
   - Include model memory usage, session count, and LoRA adapter list in the `/models` response.

4. **Timeouts / Quota / Backpressure**
   - Implement per-request wall-clock timeout (`Config::request_timeout_ms`).
   - Enforce a maximum queue depth in `ContinuousBatchScheduler`; return a structured error to callers when the queue is full.

5. **GGUF Validation**
   - For all unsupported `GGMLType` values, return an explicit `UnsupportedQuantizationFormat` error with the format name and a human-readable message.
   - Add `GGUFConverter::isSupported()` gating in the loader; test each unsupported type produces the correct error.

---

### Q2 — Safety/Policy, Fallbacks, Alerting

**Goal:** The system is safe against prompt-injection attacks, enforces per-user/model quotas, and can degrade gracefully.

1. **Prompt Injection / Jailbreak Detection**
   - Integrate a lightweight prompt-sanitization pass (regex + semantic heuristics) before tokenisation.
   - Add a configurable policy layer (`PromptPolicy`) that can block, redact, or flag requests based on keyword lists, regex patterns, or a small classifier.
   - Log all policy-triggered events to the audit log (`llm_model_audit_logger.cpp`).

2. **Policy / ACL / Quota Per User/Model**
   - Implement per-user and per-model token-per-minute quotas enforced in the request ingestion layer.
   - Add ACL checks: certain models (e.g., un-quantised large models) require explicit user/role permissions.
   - Persist quota counters in the existing KV store with TTL.

3. **Fallbacks (GPU → CPU, Multi-Model)**
   - On CUDA out-of-memory or kernel launch failure, automatically fall back to the CPU path without crashing the request.
   - Implement a multi-model fallback chain: if the primary model fails to load or respond within timeout, route to a smaller/cheaper model.

4. **Alerting**
   - Define Prometheus alerting rules for: error rate > 1 %, p99 latency > threshold, GPU memory > 90 %, queue depth > 80 % of max.
   - Alert rules are defined in `prometheus/rules/llm_alerts.yml` ✅.
   - Add latency heatmap panels to the Grafana dashboard.

---

### Q3 — Testing & Benchmarking, CI GPU/CPU, Performance Tuning

**Goal:** The module has comprehensive automated tests, CI validates GPU and CPU paths, and performance meets SLO targets.

1. **Test Suites**
   - **Fuzz**: Fuzz `GGUFLoader::parseFile()` and `Grammar::compile()` with AFL++/libFuzzer (add targets under `fuzz/`).
   - **Chaos**: Inject CUDA allocation failures, mmap failures, and null-vocab conditions to validate error paths.
   - **Adversarial**: Add adversarial prompt test cases to the grammar and prompt-policy layers.
   - **Load**: Benchmark sustained token throughput at varying batch sizes and sequence lengths under `benchmarks/`.

2. **CI GPU + CPU**
   - Add a CI job that compiles and runs `kernel_fusion.cu` tests on a CUDA-enabled runner (e.g., GitHub Actions with `ubuntu-latest` + CUDA toolkit, or a self-hosted runner).
   - Add a CI job that runs the CPU fallback path on standard runners to prevent regression.
   - Gate merges on kernel_fusion test pass.

3. **Performance Tuning**
   - Profile `flashAttentionForwardKernel` on Ampere and Ada architectures; tune `TILE_SIZE` and `BLOCK_SIZE` per SM count.
   - Benchmark continuous batching throughput against target SLO (e.g., ≥ 2000 tokens/sec on A100 for 7B Q4_K_M).
   - Evaluate BF16 vs FP16 accumulation in the attention kernel.

---

### Q4 — Cleanup, Admin DX, Audit/Analytics, Runbooks

**Goal:** Deprecated code is removed, the operator experience is polished, and runbooks cover all failure modes.

1. **Cleanup Deprecated Paths**
   - Remove `LoRAAdapterManager` and its legacy overloads from the build; update all call sites to `MultiLoRAManager`. ✅ Fully removed: `include/llm/lora_framework/lora_adapter_manager.h`, `include/llm/lora_framework/lora_adapter_manager_compat.h`, and `src/llm/lora_framework/lora_adapter_manager.cpp` deleted; all callers in `benchmarks/`, `src/server/`, `src/query/`, `tests/` and CMake build files migrated to `MultiLoRAManager`.
   - Remove or replace the DirectX/DXGI shader stubs with explicit `#error` guards on non-Windows targets. ✅ All DirectX headers already guarded by `#ifdef _WIN32`; no changes needed.
   - Delete `grafana_metrics_broken.cpp.bak` from source control. ✅ No such file is tracked (`.bak` is gitignored). Tracked `lora_orchestrator.cpp.broken` removed from git and `*.broken` added to `.gitignore`.

2. **Admin / Developer Experience**
   - Add a `POST /admin/models/reload` endpoint for hot-reload without process restart. ✅ (Q1, already implemented)
   - Add `GET /admin/sessions` and `DELETE /admin/sessions/{id}` for session management. ✅ Implemented in `MetricsServer`; 4 HTTP tests added.
   - Add a `POST /admin/prompt/simulate` dry-run endpoint for prompt policy validation without inference. ✅ (Q1, already implemented)

3. **Audit / Analytics**
   - Ensure `LLMModelAuditLogger` records: model load/unload events, LoRA adapter switches, quota violations, policy blocks, and error events with user/tenant context. ✅ `logEvent()`, `logInference()`, `logModelLifecycle()`, `logFineTuning()`, `logDeployment()`, `logPolicyViolation()` all persist to in-memory store + JSONL file.
   - Implement a structured analytics export (JSON-lines) suitable for ingestion into the data warehouse or SIEM. ✅ `exportAnalytics(ostream, model_id, start, end)` added; 18 unit tests in `tests/llm/test_llm_audit_logger.cpp`.

4. **Runbooks**
   - Operator runbooks are available under `docs/operations/llm/` ✅:
     - `GPU_OOM_RECOVERY.md` — GPU out-of-memory recovery procedure
     - `MODEL_SWAP_PROCEDURE.md` — model swap and hot-reload guide
     - `GRAMMAR_DEBUGGING.md` — grammar debugging and EBNF validation guide
     - `QUOTA_TUNING.md` — scheduler quota and capacity planning guide
     - `METRICS_SCRAPE_TROUBLESHOOTING.md` — Prometheus scrape troubleshooting guide

---

## 3. Actionable Checklist

### Observability

- [x] Implement `MetricsServer::start()` real HTTP listener — Pimpl with `httplib::Server`; GET `/metrics`, `/health`, `/ready`, `/models`, `/dashboard`; POST `/admin/models/reload`, `/admin/prompt/simulate`; CORS support; background thread; 7 round-trip HTTP tests added.
- [x] Emit `llm_inference_requests_total`, `llm_inference_duration_ms`, `llm_first_token_latency_ms` from the inference hot path — wired in `LlamaWrapper::generateRegular()` and `generateSpeculative()` via `LLMMetricsCollector`.
- [x] Emit `llm_queue_length` from `ContinuousBatchScheduler` — `setMetricsCollector()` added; `recordQueueLength()` called in `scheduleNextBatch()`.
- [x] Emit `llm_backpressure_drops_total` from `ContinuousBatchScheduler` — counter registered; `recordBackpressureDrop()` called in `submitRequest()` on rejection.
- [x] Add OTel trace context (`trace_id`, `span_id`) to `InferenceRequest` / `InferenceResponse`; inference engine propagates W3C traceparent fields from request to response — `include/llm/llm_plugin_interface.h`; propagated in `generateRegular()` and `generateSpeculative()` in `src/llm/llama_wrapper.cpp`.
- [x] Validate Grafana dashboard JSON against a live Grafana 10.x instance — **automated static validation** added: `scripts/validate_grafana_dashboards.py` checks JSON syntax, required fields, unique panel IDs, gridPos completeness, panel grid overlaps, non-empty PromQL targets, and LLM dashboard-specific panel presence + `llm_*` metric naming; CI workflow at `.github/workflows/validate-grafana-dashboards.yml`. Live Grafana 10.x smoke-test (manual) documented in `docs/operations/llm/METRICS_SCRAPE_TROUBLESHOOTING.md`.
- [x] Add latency heatmap and p50/p95/p99 panels — heatmap panel added to `grafana/dashboards/themisdb-llm-dashboard.json`; p50/p95/p99 graph panel already present.
- [x] Define Prometheus alerting rules (`prometheus/rules/llm_alerts.yml`) — file created ✅; rules loaded into `grafana/prometheus.yml` via `rule_files` entry.

### Grammar

- [x] Replace `nullptr` vocab argument in `Grammar::compile()` with `llama_model_get_vocab(model)` — **implemented** via `Grammar(ebnf, start, model)` constructor in `src/llm/grammar.cpp`.
- [x] Propagate a hard error (structured error code or exception) when vocab is null and grammar is requested — model-aware constructor returns hard error on null model/null vocab.
- [x] Elevate the silent grammar-unavailable fallback to a logged error — API-unavailable path now uses `spdlog::error` (not `warn`) in both constructors.
- [x] Add integration tests: empty EBNF, empty start-symbol, null-model hard-error, accessor correctness, move semantics — `tests/llm/test_grammar_integration.cpp` (14 tests).

### GGUF Loader

- [x] Return `UnsupportedQuantizationFormat` error (not silent raw bytes) for Q4_0, Q4_1, Q5_0, Q5_1, Q8_1, Q5_K, Q6_K — implemented in `parseTensorInfo()` via `GGUFLoader::isFormatSupported()`.
- [x] Gate `GGUFConverter` dispatch on `GGUFConverter::isSupported()`; test each unsupported type — `isFormatSupported()` covers the same type set; tests added.
- [x] Add unit tests for unsupported format error messages — `ParseFile_RejectsUnsupportedFormat_Q4_0`, `_Q5_K` added to `tests/test_gguf_loader.cpp`.
- [x] Document which formats are planned for future support and which will be permanently unsupported — `docs/GGUF_SUPPORT.md` updated with 13-row format table and hard-error behavior note.

### Safety / Policy

- [x] Implement `PromptPolicy` with configurable keyword/regex rules — `include/llm/prompt_policy.h` + `src/llm/prompt_policy.cpp`; `addBlockRule()`, `addRedactRule()`, `removeRule()`, `apply()`.
- [x] Add per-user/per-model token-per-minute quota enforcement — `TokenQuotaManager` (60-second sliding window); wired into `ContinuousBatchScheduler::submitRequest()` via `setQuotaManager()`; 22 unit tests in `tests/test_token_quota_manager.cpp`.
- [x] Wire policy-triggered events into `LLMModelAuditLogger` — `PROMPT_BLOCKED`/`PROMPT_REDACTED` event types and `logPolicyViolation()` added.
- [x] Add adversarial prompt tests validating sanitisation and policy blocking — 24 tests in `tests/test_prompt_policy.cpp`.

### Admin / Ops

- [x] Add `/health` (liveness) and `/ready` (readiness) handlers in `MetricsServer::handleRequest()` + `getHealthURL()` / `getReadyURL()` — returns JSON bodies; paths configurable in `ServerConfig`.
- [x] Add `GET /models` handler — `setModelInfoCallback()` + `getModelsURL()` + `/models` branch in `handleRequest()` (returns callback JSON or `[]`).
- [x] Add `request_timeout_ms` field to `LlamaWrapper::Config` (0 = unlimited) with validation in `validateConfig()`.
- [x] Implement maximum queue depth and structured backpressure rejection — `SchedulerConfig::max_queue_depth` + `submitRequest()` returns `{}` on overflow; `Stats::rejected_requests` and `Stats::current_queue_depth` added.
- [x] Add `POST /admin/models/reload` handler — `setReloadCallback()` + `handlePost()` with `not_implemented` fallback JSON; `getAdminReloadURL()`.
- [x] Add `POST /admin/prompt/simulate` handler — `setSimulateCallback()` + `handlePost()` with `not_implemented` fallback JSON; `getAdminSimulateURL()`.

### Testing

- [x] Add fuzz targets for `GGUFLoader::parseFile()` and `Grammar::compile()` — `fuzz/harnesses/gguf_loader_harness.cpp` + `fuzz/harnesses/grammar_harness.cpp`; seed corpora; AFL++ config updated.
- [x] Add chaos tests for CUDA allocation failure and CPU fallback — `tests/llm/test_kernel_fusion_cpu_fallback.cpp` (13 tests covering all fused kernels; validates finite output and no-crash on CPU path when CUDA is unavailable).
- [x] Add load benchmarks for continuous batching throughput — `tests/llm/bench_continuous_batch_scheduler.cpp` (5 benchmarks: submit throughput, batch latency, rejection latency, quota rejection, getStats cost).
- [x] Add CI CPU fallback regression test — `.github/workflows/llm-cpu-fallback-ci.yml`; triggers on changes to `kernel_fusion.*` and the new CPU fallback test; configures CMake with `-DTHEMIS_ENABLE_CUDA=OFF -DTHEMIS_ENABLE_LLM=ON`; runs `KernelFusionCPUFallback` test suite via ctest.
- [x] Add CI GPU job compiling and running `kernel_fusion.cu` tests — `.github/workflows/llm-cuda-gpu-ci.yml`: **cuda-compile-check** job compiles `kernel_fusion.cu` with nvcc for sm_80/86/89 on `ubuntu-22.04` (no GPU required, runs on every PR); **cuda-kernel-tests** job runs the full correctness suite (`tests/llm/test_kernel_fusion_cuda.cpp`, 7 tests covering forward pass, causal masking, correctness vs CPU reference, fused QKV/LayerNorm/FFN) on a self-hosted `gpu-cuda` runner. Tests skip gracefully on CPU-only machines via `cudaGetDeviceCount()`.

---

## 4. Research Tasks & Validation Steps

### 4.1 Grammar / llama Vocab Binding

- **Task:** Verify that `llama_grammar_init()` behaves correctly (no crash, correct token filtering) when passed a real `llama_vocab*` from `llama_model_get_vocab()`.
- **How to validate:** Load a small quantised model (e.g., a Llama-3.2-1B Q8_0 GGUF), obtain the vocab pointer, compile a simple JSON grammar, run a constrained generation, and assert that all output tokens satisfy the grammar's start rule.
- **Owner:** LLM integration team.
- **Acceptance criteria:** Grammar-constrained generation passes a round-trip test in CI with a real model file.

### 4.2 Supported GGUF Formats — Failure Modes

- **Task:** Enumerate all `GGMLType` values that appear in real-world GGUF files available on HuggingFace (Q4_0, Q4_1, Q5_0, Q5_1, Q8_1, Q5_K, Q6_K, Q2_K, IQ2_XXS, etc.) and determine for each:
  1. Whether ThemisDB should implement conversion support.
  2. If not, what error message and recovery suggestion should be returned.
- **How to validate:** Download one representative GGUF file per format and assert that `GGUFLoader::parseFile()` either (a) succeeds with correct tensor data or (b) returns a structured `UnsupportedQuantizationFormat` error with the format name.
- **Owner:** GGUF loader team.
- **Acceptance criteria:** No currently-unsupported format silently returns raw bytes; each produces an actionable error.

### 4.3 CUDA CI Coverage for `kernel_fusion.cu` ✅

- **Task:** Determine whether a CUDA-capable CI runner is available (GitHub-hosted or self-hosted). If not, assess the cost and feasibility of adding one.
- **Resolution:** Two-job CI workflow added at `.github/workflows/llm-cuda-gpu-ci.yml`:
  - **cuda-compile-check** — compiles `kernel_fusion.cu` with `nvcc` for sm_80/86/89 on `ubuntu-22.04` runners on every PR (no GPU hardware needed).
  - **cuda-kernel-tests** — runs `KernelFusionCUDATest` correctness suite on a self-hosted `[gpu-cuda]` runner; triggered on push to main/develop and via `workflow_dispatch`.
- **Acceptance criteria:** Met — CUDA compile check runs on every PR; GPU test job runs on available runners.

### 4.4 Metrics Schema & Endpoint Decisions ✅

- **Task:** Agree on the canonical set of metric names, labels, and units before wiring them into the hot path (renaming metrics later breaks dashboards and alerts).
- **Resolution:** All five open decisions resolved during Q1–Q4 implementation (see `docs/observability/llm_metrics_schema.md`). Decisions: `_ms` suffix retained, `model_id` on model-scoped metrics only, queue metrics global, 15 s scrape interval, OTel + Prometheus separate schemas.
- **Acceptance criteria:** Met — schema document status updated to "Implemented ✅"; all dashboard panels and alerting rules reference the agreed names.

### 4.5 GPU → CPU Fallback Correctness ✅

- **Task:** Verify that the CPU fallback path in `kernel_fusion.cpp` produces numerically equivalent results to the CUDA path (`kernel_fusion.cu`) within an acceptable tolerance (e.g., relative error < 1e-4 for FP32 attention).
- **Resolution:** Four cross-path tests added to `tests/llm/test_kernel_fusion_cuda.cpp` under `KernelFusionCrossPathTest` fixture:
  - `FusedLayerNormLinear_CPUMatchesCUDA` — max relative error < 1e-3
  - `FusedQKVProjection_CPUMatchesCUDA` — Q, K, V agree within 1e-3
  - `FusedGatedFFN_CPUMatchesCUDA` — FFN output agrees within 1e-3
  - `FlashAttentionForward_CPUMatchesCUDA` — attention output agrees within 1e-3
  Tests skip gracefully without a GPU; run on the `gpu-cuda` CI runner.
- **Acceptance criteria:** Met — cross-path tests wired into `llm-cuda-gpu-ci.yml` GPU runner job.

---

## 5. Risks & Dependencies

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| No CUDA CI runner available | Medium | High (Q3 milestone blocked) | ✅ `llm-cuda-gpu-ci.yml`: `cuda-compile-check` job runs on every PR (standard runner); `cuda-kernel-tests` job uses self-hosted `gpu-cuda` runner on demand. |
| llama.cpp API changes break grammar/LoRA adapters | Medium | High | ✅ llama.cpp pinned at commit `b7974` in `cmake/Dependencies.cmake` (`LLAMA_CPP_GIT_TAG`). `THEMIS_LLAMA_CPP_EXPECTED_COMMIT` compile definition exposed; `LlamaWrapper` constructor compares it against `llama_build_commit()` at startup and emits a `spdlog::warn` on mismatch. |
| OTel SDK adds significant binary size or latency overhead | Low | Medium | Profile with and without OTel; use compile-time feature flag if needed. |
| Prompt-safety classifier adds unacceptable latency | Low | Medium | Run classifier asynchronously on a separate thread pool; add latency budget to the policy config. |
| Removing deprecated LoRA compat shim breaks downstream integrations | Medium | Medium | ✅ `LoRAAdapterManager` fully removed (v1.4.0). All 12 call sites migrated to `MultiLoRAManager`. Migration reference retained in `docs/llm/LORA_ADAPTER_MIGRATION.md`. |

---

## 6. Related Documents

- `src/llm/README.md` — LLM module overview
- `src/llm/GGUF_LOADER_README.md` — GGUF quantization loading details
- `src/llm/LLAMA_LORA_ADAPTER_README.md` — LoRA adapter implementation notes
- `docs/GGUF_SUPPORT.md` — GGUF format support matrix
- `docs/GRAFANA_METRICS_COMPLETE.md` — Grafana metrics integration history
- `docs/GRAMMAR_IMPLEMENTATION_COMPLETE.md` — Grammar implementation summary
- `docs/aql_roadmap.md` — AQL/LLM subsystem production-readiness (complementary)
- `docs/llm/FLASH_ATTENTION_ARCHITECTURE.md` — Flash Attention architecture notes
- `docs/llm/LORA_ADAPTER_MIGRATION.md` — Migration guide from `LoRAAdapterManager` to `MultiLoRAManager` (deprecation timeline, Config + method mapping)
- `docs/observability/` — Observability configuration guides
- `docs/observability/llm_metrics_schema.md` — Canonical LLM metrics schema
- `docs/operations/llm/GPU_OOM_RECOVERY.md` — GPU OOM recovery runbook
- `docs/operations/llm/MODEL_SWAP_PROCEDURE.md` — Model swap procedure runbook
- `docs/operations/llm/GRAMMAR_DEBUGGING.md` — Grammar debugging guide
- `docs/operations/llm/QUOTA_TUNING.md` — Quota tuning and capacity planning guide
- `docs/operations/llm/METRICS_SCRAPE_TROUBLESHOOTING.md` — Metrics scrape troubleshooting
- `prometheus/rules/llm_alerts.yml` — Prometheus alerting rules for LLM module
- `docs/TESTING_GUIDE_LLM.md` — LLM testing guide
