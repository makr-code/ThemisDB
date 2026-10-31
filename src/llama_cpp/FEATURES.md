---
Author: "ThemisDB Contributors"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources: "include/llama_cpp/llama_cpp_plugin.h; include/llama_cpp/llama_cpp_registrar.h; src/llama_cpp/llama_cpp_plugin.cpp; src/llama_cpp/llama_cpp_registrar.cpp"
Related_Issue: "#7356"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

> Exact model backend not verifiable from final document text alone.

# llama_cpp Features

`LlamaCppPlugin` implements the `ILLMPlugin` integration. Runtime behavior is
conditional on build configuration and backend availability; advertised
capabilities and `loadModel()` success alone do not prove production readiness.
See [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md).

## Implemented surfaces

- **Model lifecycle and generation:** optional `LlamaWrapper` integration when
  `THEMIS_LLM_ENABLED` is compiled in; test-only synthetic generation is
  controlled by `THEMIS_LLAMA_CPP_STUB_MODE`.
- **RAG prompt assembly:** `generateRAG()` builds bounded context using
  `RAGContextAssembler`, then dispatches through `generate()`.
- **Embeddings:** wrapper or injected callback output when available; otherwise
  the loaded-plugin fallback is a 384-element zero vector, not a semantic
  embedding.
- **LoRA operations:** an in-memory registry and size/GGUF-magic checks for
  imports. Registry entries and accepted bytes do not themselves activate
  adapter weights.
- **Integration helpers:** plugin/adapter creation and registration through
  `LlamaCppPluginRegistrar`; generation supports policy and pre-dispatch
  cancellation hooks, streaming callbacks, and sequential `generateBatch()`.
- **Observability:** inference, error, and stream-retry counters plus model and
  registry metadata.

The public declarations are in
[`../../include/llama_cpp/llama_cpp_plugin.h`](../../include/llama_cpp/llama_cpp_plugin.h)
and [`../../include/llama_cpp/llama_cpp_registrar.h`](../../include/llama_cpp/llama_cpp_registrar.h).
