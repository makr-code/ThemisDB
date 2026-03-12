### Context

This issue implements the roadmap item 'Speculative Decoding for Latency Reduction' for the llm domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0) and targets milestone v1.9.0.

Primary detail section: Speculative Decoding for Latency Reduction

### Goal

Deliver the scoped changes for Speculative Decoding for Latency Reduction in src/llm/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### Speculative Decoding for Latency Reduction
**Priority:** Medium
**Target Version:** v1.9.0

Implement speculative decoding in `InferenceEngineEnhanced` to reduce latency for long-response requests. A small draft model generates candidate tokens speculatively; the target model verifies them in a single forward pass. On acceptance, multiple tokens advance per step; on rejection, the target model's token is used.

**Implementation Notes:**
- Add `speculative_decoder.cpp` with `SpeculativeDecoder::verify(draft_tokens, target_logits)` implementing the acceptance criterion from the Leviathan et al. 2023 paper.
- Draft model is registered in `adapter_registry.cpp` as a `DRAFT` role adapter; `InferenceEngineEnhanced` selects a draft model based on the target model's family.
- `adaptive_vram_allocator.cpp` must be updated to reserve VRAM for both the target and draft model simultaneously; the draft model is quantized to INT4 by default to minimize VRAM footprint.
- Add a `speculative_k` config parameter (number of draft tokens per step, default 4); expose via `LlmConfig::speculative_draft_tokens`.
- Disable speculative decoding automatically if grammar constraints are active (grammar state cannot be efficiently speculated); log a debug-level notice when this occurs.

**Performance Targets:**
- ≥ 2× tokens/sec improvement for long responses (≥ 200 tokens) on text-generation tasks with a 7B target model + 0.5B draft model on an A10G.
- Speculative decoding overhead (rejected tokens) ≤ 15 % of accepted token latency on typical chat prompts.

---

### Acceptance Criteria

- [ ] Add `speculative_decoder.cpp` with `SpeculativeDecoder::verify(draft_tokens, target_logits)` implementing the acceptance criterion from the Leviathan et al. 2023 paper.
- [ ] Draft model is registered in `adapter_registry.cpp` as a `DRAFT` role adapter; `InferenceEngineEnhanced` selects a draft model based on the target model's family.
- [ ] `adaptive_vram_allocator.cpp` must be updated to reserve VRAM for both the target and draft model simultaneously; the draft model is quantized to INT4 by default to minimize VRAM footprint.
- [ ] Add a `speculative_k` config parameter (number of draft tokens per step, default 4); expose via `LlmConfig::speculative_draft_tokens`.
- [ ] Disable speculative decoding automatically if grammar constraints are active (grammar state cannot be efficiently speculated); log a debug-level notice when this occurs.
- [ ] ≥ 2× tokens/sec improvement for long responses (≥ 200 tokens) on text-generation tasks with a 7B target model + 0.5B draft model on an A10G.
- [ ] Speculative decoding overhead (rejected tokens) ≤ 15 % of accepted token latency on typical chat prompts.

### Relationships

- Roadmap row: #225 (🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/llm/FUTURE_ENHANCEMENTS.md#speculative-decoding-for-latency-reduction
- Source key: roadmap:225:llm:v1.9.0:speculative-decoding-for-latency-reduction

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:225:llm:v1.9.0:speculative-decoding-for-latency-reduction -->
<!-- roadmap-ref: row=225;module=llm;target=v1.9.0 -->
<!-- roadmap-detail: src/llm/FUTURE_ENHANCEMENTS.md#speculative-decoding-for-latency-reduction -->
