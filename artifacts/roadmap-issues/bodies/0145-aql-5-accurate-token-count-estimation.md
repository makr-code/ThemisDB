### Context

This issue implements the roadmap item 'Accurate Token-Count Estimation' for the aql domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: 5 · Accurate Token-Count Estimation

### Goal

Deliver the scoped changes for Accurate Token-Count Estimation in src/aql/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### 5 · Accurate Token-Count Estimation
**Priority:** Medium
**Target Version:** v1.6.0

**Problem (from code):** `llm_aql_handler.cpp:Impl` (line 238) defines `static constexpr size_t CHARS_PER_TOKEN = 4` and uses `text.length() / CHARS_PER_TOKEN` (line 242) as the sole token-count estimator for prompt budget checks. This approximation is derived from English-language ASCII text and BPE tokenizers; it is materially wrong for multilingual content, code, and especially for few-shot schema context blocks that are dominated by JSON/AQL keywords (which tokenize more compactly). An underestimate causes context-window overflow inside the model; an overestimate wastes capacity, truncating schema context unnecessarily.

**Implementation Notes:**
- `[ ]` Introduce a `TokenEstimator` abstraction in `include/aql/llm_token_estimator.h` with `virtual size_t estimate(const std::string& text) const`; provide two implementations: `CharDivisionEstimator` (current behaviour, ratio configurable) and `TiktokenEstimator` (wraps the `tiktoken-cpp` or llama.cpp tokenizer)
- `[ ]` Inject `TokenEstimator` into `LLMAQLHandler::Impl`; default to `CharDivisionEstimator` with `ratio=4` for no breaking change
- `[ ]` Replace all three call-sites of `estimateTokenCount()` in `llm_aql_handler.cpp` (lines 336, 492, 658) with the injected estimator
- `[ ]` Add a benchmark comparing estimator accuracy against the actual llama.cpp tokenizer on the built-in few-shot corpus from `aql_fewshot_example_library.cpp`; accuracy target: ≤ 10 % error at the 95th percentile

---

### Acceptance Criteria

- [ ] Introduce a `TokenEstimator` abstraction in `include/aql/llm_token_estimator.h` with `virtual size_t estimate(const std::string& text) const`; provide two implementations: `CharDivisionEstimator` (current behaviour, ratio configurable) and `TiktokenEstimator` (wraps the `tiktoken-cpp` or llama.cpp tokenizer)
- [ ] Inject `TokenEstimator` into `LLMAQLHandler::Impl`; default to `CharDivisionEstimator` with `ratio=4` for no breaking change
- [ ] Replace all three call-sites of `estimateTokenCount()` in `llm_aql_handler.cpp` (lines 336, 492, 658) with the injected estimator
- [ ] Add a benchmark comparing estimator accuracy against the actual llama.cpp tokenizer on the built-in few-shot corpus from `aql_fewshot_example_library.cpp`; accuracy target: ≤ 10 % error at the 95th percentile

### Relationships

- Roadmap row: #145 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/aql/FUTURE_ENHANCEMENTS.md#5--accurate-token-count-estimation
- Source key: roadmap:145:aql:v1.6.0:5-accurate-token-count-estimation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:145:aql:v1.6.0:5-accurate-token-count-estimation -->
<!-- roadmap-ref: row=145;module=aql;target=v1.6.0 -->
<!-- roadmap-detail: src/aql/FUTURE_ENHANCEMENTS.md#5--accurate-token-count-estimation -->
