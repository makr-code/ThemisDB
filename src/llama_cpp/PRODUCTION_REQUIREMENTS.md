---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB llama_cpp Plugin — Production Requirements

## Scope and source of truth

This document defines deployment acceptance requirements for the `llama_cpp`
module. It distinguishes behavior present in the implementation from checks that
the service owner must perform before production traffic is enabled.

Canonical implementation and contracts:

- [`llama_cpp_plugin.cpp`](llama_cpp_plugin.cpp)
- [`llama_cpp_registrar.cpp`](llama_cpp_registrar.cpp)
- [`llama_cpp_plugin_validation_gates.cpp`](llama_cpp_plugin_validation_gates.cpp)
- [`../../include/llama_cpp/llama_cpp_plugin.h`](../../include/llama_cpp/llama_cpp_plugin.h)
- [`../../include/llama_cpp/llama_cpp_registrar.h`](../../include/llama_cpp/llama_cpp_registrar.h)
- [`../../include/llm/llm_plugin_interface.h`](../../include/llm/llm_plugin_interface.h)

## Implemented behavior and limits

- `loadModel()` is the explicit model setup call. It reads runtime configuration
  and attempts a `LlamaWrapper` load only when `THEMIS_LLM_ENABLED` is compiled
  in and a non-empty model path is supplied.
- Wrapper initialization/load exceptions are caught. Unless an enabled digest
  comparison fails, the plugin can set `model_loaded_` and return `true` even
  when no live wrapper was installed. Neither that return value nor the loaded
  flag is a model-readiness signal.
- Without a wrapper or injected `GenerateFn`, normal generation returns an error.
  Synthetic echo/tool responses require the test-only
  `THEMIS_LLAMA_CPP_STUB_MODE` compile definition.
- `PolicyFn` is optional and unset by default. A denial blocks generation and
  RAG dispatch, but the callback is not built-in authentication or authorization.
- `loadLoRA()` updates plugin registry metadata; it does not activate weights.
  `importLoRA()` checks size and GGUF magic, but a stub/no-wrapper path can return
  success without loading adapter weights.
- Embedding fallback can return 384 zeroes after wrapper/callback failure.
  This vector is not model output and must not enter production indexes.
- The opt-in model digest uses FNV-64 and is not a cryptographic integrity or
  authenticity mechanism. The separate validation helpers are not automatically
  invoked by `loadModel()`.

## Deployment acceptance requirements

Before admitting traffic, the service owner MUST:

1. Build with the intended LLM backend and ensure test-only stub mode is disabled.
2. Restrict model and adapter paths to deployment-controlled locations and
   verify artifact provenance with a trusted cryptographic signature or digest
   outside this plugin.
3. Run a real-backend inference probe against the exact deployed artifact and
   configuration. Verify a successful response; do not rely on `loadModel()`,
   `isModelLoaded()`, capability flags, or stub output.
4. Verify embedding output is real, finite, dimensionally compatible with the
   target index, and not the all-zero fallback before indexing production data.
5. Install and test the caller's policy integration before sensitive requests.
   The serving layer remains responsible for authentication, authorization,
   tenant isolation, quotas, rate limiting, backpressure, and request timeouts.
6. Restrict access to plugin statistics and logs because model identifiers may
   expose configured paths. Do not log prompts or model contents by default.
7. Retain the previous approved model/configuration and use the deployment
   system's rollback process; this plugin does not provide atomic promotion or
   rollback.

## Production readiness evidence

The repository provides focused tests for plugin lifecycle, inference contracts,
registrar behavior, validation helpers, and stress paths. The focused CMake tests
compile test fixtures; the llama_cpp integration soak also uses in-process
simulation and is not evidence of real model throughput or load reliability.
Attach real-backend probe evidence separately for each production model/config.

Run the relevant repository checks:

```bash
ctest --preset windows-release -R 'llama_cpp' --output-on-failure
python scripts/check_module_direct_doxygen.py --module llama_cpp
```

## Related module documents

[`README.md`](README.md) · [`ARCHITECTURE.md`](ARCHITECTURE.md) ·
[`ROADMAP.md`](ROADMAP.md) · [`SECURITY.md`](SECURITY.md) ·
[`TRUST.md`](TRUST.md) · [`OPERATIONS.md`](OPERATIONS.md) ·
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md)
