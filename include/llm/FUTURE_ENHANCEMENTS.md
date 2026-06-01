> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/llm/FUTURE_ENHANCEMENTS.md -->

# LLM Module — Public Header Future Enhancements

**Module Path:** `include/llm/`
**Canonical implementation enhancements:** [`../../src/llm/FUTURE_ENHANCEMENTS.md`](../../src/llm/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/llm/`. Runtime inference internals, LoRA hot-swap mechanics, paged-allocator implementation, and GPU memory coordinator work remain tracked in:

→ [`../../src/llm/FUTURE_ENHANCEMENTS.md`](../../src/llm/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Inference engine headers must define stable request/response contracts; GGUF and engine internals must remain opaque.
- `[x]` Paged KV-cache headers must expose allocation and eviction contracts without leaking physical block layout.
- `[x]` LoRA adapter headers must enforce security validation before any hot-swap operation.
- `[x]` Ethics and safety headers must enforce fail-closed behaviour for policy-violating inputs.
- `[x]` `IFederatedInferenceBackend` and `ILLMPlugin` must remain stable extension points for embedders.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `LlamaCppInferenceEngine` infer API | `llamacpp_inference_engine.h` | Server and RAG pipeline | ✅ Stable |
| `PagedKVCacheManager` alloc/evict | `paged_kv_cache_manager.h` | Batch scheduler | ✅ Stable |
| `LoRARouter` route API | `lora_router.h` | Multi-adapter serving layer | ✅ Stable |
| `OpenAICompatAdapter` request/response | `openai_compat_adapter.h` | API gateway | ✅ Stable |
| `ConstitutionalReasoningEngine` evaluate | `constitutional_reasoning_engine.h` | Safety middleware | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Document GPU-fallback paths and capability requirements across VRAM allocator and GPU-safe-fail headers.
- Align `IFederatedInferenceBackend` contract with cross-shard coordinator expectations from `include/sharding/`.
- Add explicit stability annotations to experimental `inline_training_engine.h` and `speculative_decoder.h` APIs.

### Medium-Term (Q4 2026)

- Introduce `llm_policy.h` to provide per-request resource quotas, safety gates, and access-policy contract.
- Expose benchmark-reference throughput and latency targets for paged-KV-cache, continuous batching, and Gorilla-encoded KV prefix transfer hot paths.
- Deprecate any legacy fixed-batch inference paths superseded by continuous batching and annotate migration paths.

### Long-Term

- Unify all inference result types behind a shared generation-context envelope for consistent consumer integration.
- Add structured token-budget and quota extension hooks for embedders consuming `token_quota_manager.h`.
- Provide inference explain output via `inference_engine_enhanced.h` to surface scheduling and KV-eviction decisions to consumers.
