<!-- Status: current | validated: 2026-04-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — llama_cpp Plugin

All notable changes to the llama_cpp LLM backend plugin are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Integration with real llama.cpp via the existing `LlamaWrapper`
- Streaming token output (`transcribeStream` equivalent for text generation)
- Real embedding model support (currently returns fixed 384-dim zero vector)
- Function/tool calling support

## [2.0.0] — 2026-04-07

### Added
- `LlamaCppPlugin : ILLMPlugin` — full interface implementation for dynamic loading
- `loadModel` / `unloadModel` with stub mode (no model file required for tests)
- `getModelInfo()` returning `std::optional<ModelInfo>` (nullopt when not loaded)
- `generate(InferenceRequest)` — stub returns echo response; error when not loaded
- `generateRAG(InferenceRequest, context_docs)` — prepends context docs and delegates
  to `generate()`
- `embed(text)` — returns 384-dim zero vector when loaded, empty when not loaded
- `loadLoRA` / `unloadLoRA` / `listLoRAs` — full LoRA registry with duplicate-id
  replacement and thread-safe access via `std::mutex`
- `getCapabilities()` — `supports_lora=true`, `supports_embeddings=true`,
  `plugin_version="2.0.0"`
- `getMemoryStats()` / `getPerformanceStats()` — JSON stats with `inference_count`,
  `error_count`, `model_loaded`, `lora_count`
- `exportLoRA` / `importLoRA` — stub implementations (return empty / false)
- `THEMIS_LLM_PLUGIN()` export macro in `include/llm/llm_plugin_interface.h`
- `themis_llm_create` / `themis_llm_destroy` C-linkage entry points
- 30 unit tests (`LlamaCppPluginFocusedTests`, groups A–J)
- `plugins/llama_cpp/plugin.json.in` — plugin manifest
- `src/llama_cpp/CMakeLists.txt` — build target
- `tests/CMakeLists.txt` — `LlamaCppPluginFocusedTests` registered
- `plugins/CMakeLists.txt` — `THEMIS_PLUGIN_LLAMA_CPP` option added
