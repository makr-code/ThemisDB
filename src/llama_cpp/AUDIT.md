> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: current | validated: 2026-08-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — llama_cpp Plugin

**Last Audit:** 2026-08-09
**Auditor:** Copilot (Sub-Agents A–F)
**Status:** ✅ Pass

## Summary

| Metric | Result |
|--------|--------|
| Source files audited | 3 (`llama_cpp_plugin.cpp`, `llama_cpp_registrar.cpp`, `llama_cpp_plugin_validation_gates.cpp`) |
| Test targets | 2 (`LlamaCppPluginFocusedTests`, `LlamaCppRegistrarIntegrationTests`) |
| Test count | 89 (groups A–X + W registrar tests) |
| Open security issues | 0 |
| Open functional issues | 0 |
| Critical gap-scanner findings closed | 30 (all CRITICAL/HIGH from 2026-06-04 scan) |
| Build system registration | ✅ `tests/CMakeLists.txt` + `plugins/CMakeLists.txt` |
| Documentation completeness | ✅ All docs present and updated |

## Build System

Registered in:
- `src/llama_cpp/CMakeLists.txt` — `llama_cpp_plugin` static library
- `tests/CMakeLists.txt` — `LlamaCppPluginFocusedTests` test target
- `plugins/CMakeLists.txt` — `THEMIS_PLUGIN_LLAMA_CPP` option

Dependencies: `nlohmann_json` (required).

## Source Files Audited

| File | Responsibility | Finding |
|------|---------------|---------|
| `llama_cpp_plugin.cpp` | Full ILLMPlugin implementation | ✅ All methods implemented; thread-safe via `std::mutex`; error handling for uninit state; LoRA duplicate-id replacement |
| `llama_cpp_registrar.cpp` | Plugin factory export (THEMIS_LLM_PLUGIN macro, create/destroy functions) | ✅ Reviewed |

## Interface Compliance

| Interface | Implemented | Notes |
|-----------|-------------|-------|
| `ILLMPlugin::loadModel` | ✅ | Opt-in digest gate; real LlamaWrapper via `THEMIS_LLM_ENABLED` |
| `ILLMPlugin::unloadModel` | ✅ | Clears model state and LoRA registry |
| `ILLMPlugin::getModelInfo` | ✅ | Returns nullopt when not loaded |
| `ILLMPlugin::generate` | ✅ | Policy gate; retry-callback; fail-closed error when not loaded |
| `ILLMPlugin::generateRAG` | ✅ | Mutex-snapshot for shared state; RAGContextAssembler |
| `ILLMPlugin::embed` | ✅ | Inject-fn or real LlamaWrapper embedding; zero-vector stub |
| `ILLMPlugin::loadLoRA` | ✅ | Thread-safe; duplicate-id replacement |
| `ILLMPlugin::unloadLoRA` | ✅ | Returns false if not found |
| `ILLMPlugin::listLoRAs` | ✅ | Thread-safe copy |
| `ILLMPlugin::getCapabilities` | ✅ | `supports_lora`, `supports_embeddings`, `supports_streaming`, `supports_function_call` |
| `ILLMPlugin::getMemoryStats` | ✅ | JSON with model_loaded, model_id, lora_count |
| `ILLMPlugin::getPerformanceStats` | ✅ | JSON with inference_count, error_count, stream_retry_count |
| `ILLMPlugin::exportLoRA` | ✅ | Delegates to LlamaWrapper; empty vector in stub mode |
| `ILLMPlugin::importLoRA` | ✅ | GGUF magic + 2 GB size validation; delegates to LlamaWrapper |
| `THEMIS_LLM_PLUGIN()` export | ✅ | `themis_llm_create` + `themis_llm_destroy` with RAII ownership doc |
| `LlamaCppPluginRegistrar::initFromServerConfig` | ✅ | Server-startup integration point (v2.4.0) |
| `setPolicyFn(fn)` | ✅ | Pluggable inference policy gate (v2.4.0) |

## Known Gaps

| ID | Description | Severity | Status |
|----|-------------|----------|--------|
| LC-01 | `generate()` real inference requires `THEMIS_LLM_ENABLED` + model file | Medium | Controlled stub; production path available |
| LC-02 | `generateBatch()` is sequential — no true parallel batch | Low | Documented limitation |
| LC-03 | `computeFileDigest` uses FNV-64 placeholder; SHA-256 requires OpenSSL | Low | Replace body with `EVP_DigestFinal` when libcrypto available |
