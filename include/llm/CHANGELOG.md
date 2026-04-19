<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — LLM Module (Public Headers)

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For implementation-level changes see `../../src/llm/CHANGELOG.md`.

## [Unreleased]
- Federated inference across remote nodes (Issue #1928)
- Real draft-model logits for speculative decoding
- Persistent KV-cache across process restarts (disk-backed)
- Hard cancellation for in-flight requests

## [1.16.0] — 2026-03-12
### Added
- `model_router.h`: `ModelRouter` with regex/tag-based routing
- `openai_compat_adapter.h`: OpenAI `/v1/chat/completions` drop-in adapter
- `speculative_decoder.h`: draft-model speculative decoding interface

## [1.15.0] — 2026-02-01
### Added
- `constitutional_reasoning_engine.h`: Constitutional AI reasoning
- `ethical_guidelines_manager.h`, `ethics_aware_confidence_detector.h`, `moral_analyzer.h`
- `byzantine_detector.h`: Byzantine fault detection
- `multi_gpu_memory_coordinator.h`, `active_vram_allocator.h`

## [1.10.0] — 2025-09-01
### Added
- `paged_kv_cache.h`, `paged_kv_cache_manager.h`, `paged_block_manager.h`, `block_table.h`
- `continuous_batch_scheduler.h`
- `multi_lora_manager.h`, `lora_router.h`, `lora_certificate_store.h`, `lora_security_validator.h`
- `mixed_precision_inference.h`, `kernel_fusion.h`, `kernel_fusion_cuda.h`

## [1.0.0] — 2024-01-01
### Added
- `i_llm_plugin.h`, `llm_plugin_manager.h`, `llamacpp_inference_engine.h`
- Core inference, grammar, prompt, sampling, streaming headers
- `gguf_loader.h`, `model_loader.h`, `llama_wrapper.h`
