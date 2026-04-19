> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — LLM Module

All notable changes to the LLM module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Federated inference across remote nodes (Issue #1928)
- Hard cancellation for in-flight requests (currently best-effort)
- Speculative decoding using real draft-model logits (currently synthetic arrays)
- Persistent KV-cache across process restarts (disk-backed)

## [1.16.0] — 2026-03-12
### Added
- `ModelRouter`: regex- and tag-based routing rules dispatching requests to the appropriate backend model without caller awareness
- OpenAI-compatible `/v1/chat/completions` adapter for drop-in integration with OpenAI-API clients
- Speculative decoding: draft model generates candidate tokens; verifier model accepts/rejects in batch, reducing per-token latency
- `ActiveVRAMAllocator`: GPU VRAM management with OOM recovery, LRU eviction of inactive model weights, and CPU spilling
- KV-cache prewarming with embedding-based similarity lookup to reuse cached computation for near-duplicate prompts
- LoRA adapter hot-loading: adapters can be swapped at runtime without model reload
- Model quantization pipeline supporting GGUF, AWQ, and GPTQ formats
- Multi-modal input/vision support (experimental): image tokens interleaved with text for vision-language models
- Constitutional reasoning engine and ethical guidelines manager as inference-time guardrails

### Changed
- `InferenceEngineEnhanced` KV-cache eviction policy changed from FIFO to LRU
- Worker pool resizing is now dynamic (grows/shrinks based on queue depth)
- GGUF loader validation tightened to reject files with unexpected magic bytes or version fields

### Fixed
- VRAM allocator: residual activations from previous model not zeroed on model swap (security fix — see SECURITY.md)
- Request deduplication cache: stale cache entry returned after model hot-swap
- Streaming SSE: connection not closed on generator exhaustion in edge-case empty-response scenario

## [1.15.0] — 2025-09-01
### Added
- `InferenceEngineEnhanced`: enterprise multi-model engine with KV-cache, dynamic batching, and load balancing across GPU devices
- Shared worker pool using work-stealing algorithm for low-latency request dispatch
- Request deduplication cache: identical prompts within a configurable TTL window return cached responses
- Per-model resource quotas (max concurrent requests, VRAM ceiling, token rate)
- Function/tool calling with JSON schema binding: model outputs are validated against caller-supplied JSON Schema before return

### Changed
- `AsyncInferenceEngine` is now the recommended single-model interface; synchronous blocking interface deprecated

### Fixed
- Grammar-constrained generation: stack overflow on deeply recursive grammars

## [1.14.0] — 2025-03-01
### Added
- `AsyncInferenceEngine`: non-blocking inference with `std::future`-based result delivery
- Streaming token output via Server-Sent Events (SSE)
- Grammar-constrained generation: BNF grammar supplied per-request to restrict output vocabulary
- Model hot-swap: load a new model checkpoint and atomically replace the active model without restarting the server
- `lora_security_validator.cpp`: SHA-256 integrity check for LoRA adapter files before loading
- `vram_secure_clear.cpp`: explicit VRAM zeroing on model unload to prevent cross-model data leakage
- `llm_security_utils.cpp`: prompt injection detection utilities used by ingestion and query modules

## [1.13.0] — 2024-09-01
### Added
- GGUF model loader with format validation (magic bytes, version, metadata field checks)
- Initial KV-cache implementation (per-sequence, fixed-size)
- `sampling_strategy.cpp`: temperature, top-p, and top-k sampling strategies

## [1.0.0] — 2024-01-01
### Added
- Initial implementation of the LLM module
- Basic synchronous inference engine backed by llama.cpp
- Model load/unload lifecycle management
