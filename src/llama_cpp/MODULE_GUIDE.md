---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources: "src/llama_cpp/CMakeLists.txt; tests/llama_cpp/CMakeLists.txt; src/llama_cpp implementation and focused tests"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

# llama_cpp Module Developer Guide

For developers building, testing or extending the plugin. This guide focuses on actionable repository paths; implementation details are in [`IMPLEMENTATION.md`](IMPLEMENTATION.md).

> Exact model backend not verifiable from final document text alone.

## Source map and boundaries

- Implementation: [`llama_cpp_plugin.cpp`](llama_cpp_plugin.cpp), [`llama_cpp_registrar.cpp`](llama_cpp_registrar.cpp), [`llama_cpp_plugin_validation_gates.cpp`](llama_cpp_plugin_validation_gates.cpp).
- Public plugin and registrar APIs: [`../../include/llama_cpp/llama_cpp_plugin.h`](../../include/llama_cpp/llama_cpp_plugin.h) and [`../../include/llama_cpp/llama_cpp_registrar.h`](../../include/llama_cpp/llama_cpp_registrar.h).
- Shared interface and wrapper contract: [`../../include/llm/llm_plugin_interface.h`](../../include/llm/llm_plugin_interface.h), [`../../include/llm/llama_wrapper.h`](../../include/llm/llama_wrapper.h).
- CMake plugin target: [`CMakeLists.txt`](CMakeLists.txt); focused test target definition: [`../../tests/llama_cpp/CMakeLists.txt`](../../tests/llama_cpp/CMakeLists.txt).

The plugin owns its adapter-level lifecycle and delegates real model operations to `LlamaWrapper` only when `THEMIS_LLM_ENABLED` is compiled in. It does not own model training, public transport, or the upstream authorization policy.

## Build and test

Build from the repository root using a configured preset/build tree, e.g.:

```bash
cmake --preset community-debug
cmake --build --preset community-debug
ctest --preset community-debug -R 'llama_cpp' --output-on-failure
```

The exact preset availability depends on the checkout/toolchain. The root README's Linux/Windows release presets are alternatives, not requirements for focused tests. For real-backend validation, use an LLM-enabled build with an approved test model and record the precise model artifact and config; stub tests do not validate model loading.

Focused source coverage includes:

- [`test_llama_cpp_inference_contract_focused.cpp`](tests/test_llama_cpp_inference_contract_focused.cpp)
- [`test_llama_cpp_plugin_lifecycle_focused.cpp`](tests/test_llama_cpp_plugin_lifecycle_focused.cpp)
- [`test_llama_cpp_registrar_integration_focused.cpp`](tests/test_llama_cpp_registrar_integration_focused.cpp)
- [`test_llama_cpp_validation_gates_focused.cpp`](tests/test_llama_cpp_validation_gates_focused.cpp)
- [`../../tests/llama_cpp/test_model_loading.cpp`](../../tests/llama_cpp/test_model_loading.cpp), [`../../tests/llama_cpp/test_inference_execution.cpp`](../../tests/llama_cpp/test_inference_execution.cpp), and [`../../tests/llama_cpp/test_llama_errors.cpp`](../../tests/llama_cpp/test_llama_errors.cpp).

The CMake test definition marks source-focused targets with `THEMIS_LLAMA_CPP_STUB_MODE`. Integration soak coverage at [`../../tests/integration/test_llama_cpp_soak.cpp`](../../tests/integration/test_llama_cpp_soak.cpp) uses in-process stubs. Neither should be presented as real model performance evidence.

## Change checklist

1. Check public declaration, implementation and relevant test together.
2. Preserve failure distinctions: unloaded, policy denied, cancelled, wrapper failure/fallback, and test stub.
3. If changing inference or embedding configuration, verify real backend behavior as well as stub contract tests.
4. If changing registrar behavior, test absent/empty/nonempty `llm.model_path` cases and registration ownership.
5. If changing trust-sensitive path handling, review [`TRUST.md`](TRUST.md) and module [`SECURITY.md`](SECURITY.md).
6. Run focused CTest; report tests actually run and backend mode. Do not cite the wiki snapshot as implementation evidence.

## Related module documents

[`RESEARCH.md`](RESEARCH.md) · [`MODELING.md`](MODELING.md) · [`EMBEDDING.md`](EMBEDDING.md) · [`IMPLEMENTATION.md`](IMPLEMENTATION.md) · [`API.md`](API.md) · [`RUNBOOK.md`](RUNBOOK.md) · [`OPERATIONS.md`](OPERATIONS.md) · [`TRUST.md`](TRUST.md)

Also see [`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md), [`ROADMAP.md`](ROADMAP.md), [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md), [`SECURITY.md`](SECURITY.md), and [`../../include/llama_cpp/README.md`](../../include/llama_cpp/README.md).
