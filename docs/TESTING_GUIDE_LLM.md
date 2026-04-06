# LLM Testing Guide

Complete guide for testing the ThemisDB LLM module (`src/llm`).

**Last Updated:** April 2026 | **Version:** 2.0

---

## Overview

This guide covers unit tests, integration tests, chaos/adversarial tests, fuzz harnesses, and benchmarks for the ThemisDB LLM implementation. The test suite exercises the entire stack from kernel-level CUDA operations up through the inference API, prompt-safety policy layer, quota enforcement, and production admin/ops endpoints.

---

## Test Structure

```
tests/
├── test_gguf_loader.cpp                   # GGUF format loading & validation (23 tests)
├── test_continuous_batch_scheduler.cpp     # Batch scheduler unit tests
├── test_prompt_policy.cpp                  # Prompt-safety policy (24 adversarial tests)
├── test_token_quota_manager.cpp            # Per-user/model token quota (22 tests)
├── test_llm_grafana_metrics.cpp            # MetricsServer HTTP endpoints (29 tests)
│
└── llm/
    ├── test_grammar_integration.cpp        # Grammar API (14 integration tests)
    ├── test_kernel_fusion_cpu_fallback.cpp # CPU fused-kernel paths (13 chaos tests)
    ├── test_kernel_fusion_cuda.cpp         # CUDA kernel correctness + cross-path (11 tests)
    ├── test_llm_audit_logger.cpp           # Audit logger + JSONL export (17 tests)
    ├── test_llama_wrapper_state.cpp        # LlamaWrapper state machine
    ├── test_llm_validator.cpp              # Input validation
    ├── test_lora_adapters.cpp              # LoRA adapter management
    ├── test_model_loader_async.cpp         # Async model loading
    ├── test_model_loader_error_handling.cpp# Model loader error paths
    ├── test_inference_performance.cpp      # Inference latency tests
    ├── test_inference_quality.cpp          # Output quality / regression
    └── ... (other LLM sub-component tests)

benchmarks/
└── llm/
    └── bench_continuous_batch_scheduler.cpp  # Scheduler throughput/latency (5 benchmarks)

fuzz/
└── harnesses/
    ├── gguf_loader_harness.cpp             # libFuzzer harness for GGUFLoader::parseFile()
    └── grammar_harness.cpp                 # libFuzzer harness for Grammar::compile()
```

---

## Prerequisites

### Build Configuration

Tests that require real model files are automatically skipped when no model is available (via `GTEST_SKIP()`). Tests that require a CUDA-capable GPU skip gracefully via `cudaGetDeviceCount()`.

```bash
# Minimal: run all tests that don't need real models or GPU
cmake -DCMAKE_BUILD_TYPE=Release \
      -DTHEMIS_ENABLE_LLM=ON \
      -DTHEMIS_ENABLE_CUDA=OFF \
      -DTHEMIS_BUILD_TESTS=ON \
      -B build && cmake --build build -j$(nproc)

# Full: with GPU support (requires CUDA toolkit)
cmake -DCMAKE_BUILD_TYPE=Release \
      -DTHEMIS_ENABLE_LLM=ON \
      -DTHEMIS_ENABLE_CUDA=ON \
      -DCMAKE_CUDA_ARCHITECTURES="80;86;89" \
      -DTHEMIS_BUILD_TESTS=ON \
      -B build && cmake --build build -j$(nproc)
```

### Optional: Real Model File

Some tests (`test_inference_quality`, `test_real_embeddings`, `test_grammar_integration` when using live vocab) require a GGUF model file:

```bash
export THEMIS_LLM_MODEL_PATH=/path/to/llama-3.2-1b-q8_0.gguf
# Or for the smallest viable model:
export THEMIS_LLM_MODEL_PATH=/path/to/tinyllama-1.1b-q4_0.gguf
```

---

## Running Tests

### All LLM tests (no GPU, no model file required)

```bash
cd build
ctest --output-on-failure -R "LLM|Grammar|GGUF|PromptPolicy|TokenQuota|MetricsServer|AuditLogger|KernelFusion"
```

### By category

```bash
# Kernel fusion — CPU fallback path (no GPU needed)
ctest --output-on-failure -R "KernelFusionCPUFallback"

# Kernel fusion — CUDA paths (skips gracefully without GPU)
ctest --output-on-failure -R "KernelFusionCUDATest|KernelFusionCrossPath"

# Prompt safety policy
ctest --output-on-failure -R "PromptPolicyTest"

# Token quota enforcement
ctest --output-on-failure -R "TokenQuotaManagerTest"

# MetricsServer HTTP endpoints (starts a real HTTP server on 127.0.0.1:19091)
ctest --output-on-failure -R "MetricsServerHTTPTest"

# Grammar API integration
ctest --output-on-failure -R "GrammarIntegrationTest"

# Audit logger + JSONL export
ctest --output-on-failure -R "LLMAuditLoggerTest"

# GGUF loader — format validation + unsupported type errors
ctest --output-on-failure -R "GGUFLoaderTest"
```

### Using GTest filters directly

```bash
./build/tests/themisdb_tests \
    --gtest_filter="PromptPolicyTest.*:TokenQuotaManagerTest.*:LLMAuditLoggerTest.*"
```

---

## Test Categories

### 1. Kernel Fusion — CPU Fallback Chaos Tests (`test_kernel_fusion_cpu_fallback.cpp`)

Validates that all fused kernel functions in `kernel_fusion.cpp` produce finite, non-NaN output on the CPU path. Tests exercise:

- `fusedLayerNormLinearResidual` — varied inputs, zero residual, small dimensions
- `fusedAttentionQKV` — identity-like weights
- `fusedSoftmaxDropoutAttention` — uniform and peaked attention distributions
- `fusedGatedFFN` — SiLU gate activation, varied weights
- `fusedRMSNormLinear` — near-zero inputs for numerical stability

No GPU hardware is required.

### 2. CUDA Kernel Correctness (`test_kernel_fusion_cuda.cpp`)

Tests that require a CUDA GPU (skip automatically on CPU-only machines):

| Test | Description |
|------|-------------|
| `FlashAttentionForward_OutputFinite` | Output contains no NaN or Inf |
| `FlashAttentionForward_MatchesCPUReference` | Max relative error < 1e-3 vs naive CPU reference |
| `FlashAttentionForward_CausalMaskingApplied` | Causal and non-causal outputs differ |
| `FusedQKVProjection_OutputFinite` | Q, K, V all finite |
| `FusedLayerNormLinear_OutputFinite` | No NaN or Inf |
| `FusedGatedFFN_OutputFinite` | No NaN or Inf |

**Cross-path correctness tests (Research task 4.5):**

| Test | Description |
|------|-------------|
| `FusedLayerNormLinear_CPUMatchesCUDA` | CPU fallback and CUDA agree within 1e-3 |
| `FusedQKVProjection_CPUMatchesCUDA` | CPU and CUDA Q, K, V outputs agree within 1e-3 |
| `FusedGatedFFN_CPUMatchesCUDA` | CPU and CUDA FFN outputs agree within 1e-3 |
| `FlashAttentionForward_CPUMatchesCUDA` | CPU naive reference and CUDA kernel agree within 1e-3 |

### 3. Prompt-Safety Policy (`test_prompt_policy.cpp`)

24 adversarial tests covering:
- Keyword blocking (exact match and substring)
- Regex-based blocking (phone numbers, credit cards, injection patterns)
- PII redaction
- Jailbreak attempt patterns
- Multi-rule interaction (block + redact in same request)
- `apply()` result types (ALLOWED, BLOCKED, REDACTED)

### 4. Token Quota Management (`test_token_quota_manager.cpp`)

22 tests covering:
- Per-user quota enforcement (60-second sliding window)
- Per-model quota enforcement
- Quota reset after window expiry
- Concurrent access from multiple threads
- Integration with `ContinuousBatchScheduler::submitRequest()`

### 5. MetricsServer HTTP (`test_llm_grafana_metrics.cpp`)

29 tests covering:
- `GET /metrics` — Prometheus exposition format
- `GET /health` and `GET /ready` — liveness/readiness
- `GET /models` — model list (with and without callback)
- `GET /admin/sessions` — session list (with and without callback)
- `DELETE /admin/sessions/{id}` — session deletion (with and without callback)
- `POST /admin/models/reload` — hot-reload (not-implemented fallback)
- `POST /admin/prompt/simulate` — dry-run policy check (with callback)
- Unknown path → 404

### 6. Grammar Integration (`test_grammar_integration.cpp`)

14 tests covering:
- Empty EBNF → structured error
- Empty start symbol → error
- Null model pointer → hard error (no silent fallback)
- API-unavailable path → logged error (not warning)
- Accessor correctness (`ebnf()`, `startRule()`, `isValid()`)
- Move semantics

### 7. Audit Logger + JSONL Export (`test_llm_audit_logger.cpp`)

17 tests covering:
- `logEvent()` appended to in-memory store
- `queryLogs()` filtering by `model_id`
- `logPolicyViolation()` — `PROMPT_BLOCKED` / `PROMPT_REDACTED` event types
- `logInference()` — `INFERENCE_COMPLETED` / `INFERENCE_FAILED`
- `exportAnalytics()` — valid JSONL output, ISO-8601 timestamps, model filter
- `getModelStats()` — correct aggregation
- `setEnabled(false)` — suppresses all writes

### 8. GGUF Loader (`test_gguf_loader.cpp`)

23 tests covering:
- Valid FP32 and Q4_K_M loading
- `UnsupportedQuantizationFormat` errors for Q4_0, Q5_K, and other unsupported types
- Corrupt file header → structured error (no crash)
- Missing file → structured error

---

## Benchmarks (`tests/llm/bench_continuous_batch_scheduler.cpp`)

Five GTest-based throughput/latency benchmarks:

| Benchmark | Measurement |
|-----------|-------------|
| Submit throughput | Requests/second under 1000-request load |
| Batch latency | `scheduleNextBatch()` time with 64 pending requests |
| Rejection latency | p99 latency for queue-full rejection |
| Quota rejection throughput | `TokenQuotaManager` integrated rejection rate |
| `getStats()` cost | Lock contention under 8-thread concurrent submit |

**SLA targets (CI gate):**
- `submitRequest()` p99 < 100 µs
- `scheduleNextBatch()` with 64 items < 5 ms
- Queue-full rejection p99 < 50 µs

---

## Fuzz Testing (`fuzz/harnesses/`)

Fuzz targets use libFuzzer. Build with `-DTHEMIS_ENABLE_FUZZ=ON`:

```bash
cmake -DTHEMIS_ENABLE_LLM=ON -DTHEMIS_ENABLE_FUZZ=ON -B build_fuzz
cmake --build build_fuzz --target gguf_loader_harness grammar_harness

# Run for 60 seconds each
./build_fuzz/fuzz/gguf_loader_harness -max_total_time=60 fuzz/corpus/gguf/
./build_fuzz/fuzz/grammar_harness       -max_total_time=60 fuzz/corpus/grammar/
```

---

## CI Workflows

| Workflow | Triggers | Runner |
|----------|---------|--------|
| `llm-cpu-fallback-ci.yml` | Changes to `kernel_fusion.*` | `ubuntu-latest` |
| `llm-cuda-gpu-ci.yml` (compile check) | Changes to `kernel_fusion.cu` | `ubuntu-22.04` + CUDA 12.4 |
| `llm-cuda-gpu-ci.yml` (GPU tests) | Push to main/develop, manual | Self-hosted `gpu-cuda` runner |
| `chimera-tests.yml` | All PRs | `ubuntu-latest` |

---

## Performance Baselines

### Inference (requires real model)

| Metric | CPU target | GPU target (A100) |
|--------|-----------|-------------------|
| First token latency (TTFT) | < 2 s (7B Q4_K_M) | < 100 ms (7B Q4_K_M) |
| Throughput | ≥ 10 tokens/sec (7B Q4_K_M) | ≥ 2 000 tokens/sec (7B Q4_K_M) |
| p99 end-to-end latency (256 tokens) | < 30 s | < 1 s |

### Scheduler

| Metric | Target |
|--------|--------|
| `submitRequest()` p99 | < 100 µs |
| Queue-full rejection p99 | < 50 µs |
| `scheduleNextBatch()` (64 requests) | < 5 ms |

---

## Troubleshooting

### Tests that require GPU skip — is that expected?

Yes. All CUDA tests call `cudaGetDeviceCount()` at runtime. If the count is 0 (no GPU driver or hardware), `GTEST_SKIP()` is invoked. The test binary reports these as `SKIPPED`, not `FAILED`.

```
[  SKIPPED ] KernelFusionCUDATest.FlashAttentionForward_OutputFinite
Reason: No CUDA-capable GPU detected — skipping CUDA kernel tests.
```

To run the CUDA tests, provide a machine with an NVIDIA GPU and the appropriate driver.

### MetricsServer tests fail with "address already in use"

The HTTP tests start a real server on port 19091. If a previous test run crashed and left the port bound, kill it:

```bash
lsof -i :19091 | grep LISTEN | awk '{print $2}' | xargs kill -9
```

### GGUF unsupported format test fails

The unsupported-format test writes a synthetic GGUF header with an unsupported `GGMLType` value. If it fails, check that `GGUFLoader::isFormatSupported()` covers the type being tested. See `docs/GGUF_SUPPORT.md` for the full format support matrix.

### Grammar null-model test crashes

If `Grammar(ebnf, start, nullptr)` crashes instead of returning an error, check that `src/llm/grammar.cpp` validates the `model` pointer before calling `llama_model_get_vocab()`.

---

## Related Documents

- `docs/llm_roadmap.md` — LLM production-readiness roadmap
- `docs/GGUF_SUPPORT.md` — GGUF format support matrix
- `docs/observability/llm_metrics_schema.md` — Canonical LLM metrics schema
- `docs/operations/llm/` — Operator runbooks (GPU OOM, model swap, quota tuning)
- `prometheus/rules/llm_alerts.yml` — Prometheus alerting rules
- `.github/workflows/llm-cpu-fallback-ci.yml` — CPU fallback CI
- `.github/workflows/llm-cuda-gpu-ci.yml` — CUDA compile + GPU CI
- `fuzz/harnesses/` — libFuzzer fuzz targets

