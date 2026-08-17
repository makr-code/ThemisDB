# AI Watermark Detector Plugin — Open Enhancements

**Version:** 1.0.0-phase1  
**Status:** Tracking open stubs and enhancement backlog  
**Scope:** Phase 2–6 implementation items and known limitations

---

## Summary

This document tracks all open stubs, mock implementations, and enhancement backlog items for the AI Watermark Detector plugin. It complements `ROADMAP.md` (which tracks phase execution) with detailed stub replacement tasks.

---

## Phase 2 Stubs: Tokenization & Language Detection

### [STUB] `tokenize()` — Text → Token IDs

**Location:** `src/detector_impl.cpp:WatermarkDetectorImpl::tokenize()`

**Current Behavior:**
- Returns dummy token sequence (length-based estimate)
- Does not perform actual tokenization

**Required Replacement:**
- Integrate with LLM module's tokenizer interface (e.g., `llm::ITokenizer`)
- Support Claude's tokenizer (BPE, ~100k vocab)
- Handle multilingual token ID mapping
- Add tokenizer cache (avoid repeated tokenization of same text)

**Target Phase:** Phase 2  
**Estimated Effort:** 4–6 hours  
**Dependencies:** LLM module's tokenizer interface finalized

**Acceptance:**
- [ ] Tokenization matches Claude's token boundaries
- [ ] Multilingual support (UTF-8, CJK handling)
- [ ] Performance: <5ms per 1k tokens
- [ ] No memory leaks (valgrind clean)
- [ ] Tests cover edge cases (empty strings, special chars, BOM)

---

### [STUB] `detect_language()` — Language Detection

**Location:** `src/detector_impl.cpp:WatermarkDetectorImpl::detect_language()`

**Current Behavior:**
- Always returns "en" (English)

**Required Replacement:**
- Integrate language detection library (fastText, langdetect, or textcat)
- Return ISO 639-1 language code (e.g., "en", "de", "fr", "ja")
- Handle edge cases (mixed-language texts, short texts)
- Return "unknown" if confidence <0.7

**Target Phase:** Phase 2  
**Estimated Effort:** 2–3 hours  
**Dependencies:** Language detection library (vcpkg or system)

**Acceptance:**
- [ ] Correctly identifies 9+ languages
- [ ] Handles mixed-language text gracefully
- [ ] Performance: <2ms per text
- [ ] Tested on 100+ multilingual samples

---

## Phase 2 Stubs: Heuristic Algorithms

### [ENHANCEMENT] Claude Watermark Calibration

**Location:** `src/token_analyzer.cpp:TokenDistributionAnalyzer::estimate_claude_watermark()`

**Current Behavior:**
- Uses simplified z-score approach with fixed green-list fraction (0.5)
- Length-dependent confidence adjustment (rough linear scaling)

**Required Enhancement:**
- Calibrate against curated Claude corpus (1000+ texts)
- Fine-tune green-list fraction (Anthropic indicates ~0.5, but may vary)
- Validate z-score threshold (currently ±2 standard deviations)
- Optimize length-dependent adjustment curve
- Add model-specific calibration (Claude 1, 2, 3, 3.5, etc.)

**Target Phase:** Phase 2–3  
**Estimated Effort:** 8–12 hours (includes corpus labeling)  
**Dependencies:** Curated Claude text corpus (labeled AI-generated)

**Acceptance:**
- [ ] >90% recall on curated Claude corpus
- [ ] <5% false positive on human text
- [ ] Calibration documented in PERFORMANCE_CALIBRATION.md
- [ ] Confidence interval validation (Fisher's exact test)

---

### [ENHANCEMENT] N-gram Entrenchment Tuning

**Location:** `src/token_analyzer.cpp:TokenDistributionAnalyzer::compute_ngram_entrenchment()`

**Current Behavior:**
- Computes entropy-based n-gram "lockedness" (currently n=3 only)
- Normalized via max entropy

**Required Enhancement:**
- Test n=2, 3, 4, 5 and select optimal
- Validate on corpus (Claude vs. human text)
- Add adaptive n selection (based on text length)
- Optimize entropy normalization (currently log-based)
- Add skip-gram and positional variants

**Target Phase:** Phase 2–3  
**Estimated Effort:** 6–8 hours  
**Dependencies:** Corpus validation pipeline

**Acceptance:**
- [ ] Tuned n-gram order (2, 3, or 4) selected
- [ ] Contribution to final score >0.1 (significant)
- [ ] Ablation study results documented
- [ ] <10ms overhead per text

---

### [STUB] KL-Divergence Reference Distribution

**Location:** `src/token_analyzer.cpp:TokenDistributionAnalyzer::compute_kl_divergence()`

**Current Behavior:**
- Falls back to uniform reference distribution if none provided
- Does not use pre-computed human-text reference

**Required Replacement:**
- Compute and cache reference token distribution (human text corpus)
- Validate distribution stability across domains
- Add lazy loading (load on first use)
- Support multiple reference distributions (by language, domain)

**Target Phase:** Phase 2–3  
**Estimated Effort:** 4–6 hours  
**Dependencies:** Representative human text corpora (news, literature, social media, technical)

**Acceptance:**
- [ ] Reference distributions computed and validated
- [ ] Cached reference loaded <100ms
- [ ] KL-divergence differentiates Claude vs. human (effect size >0.5)
- [ ] Multilingual reference sets included

---

## Phase 3 Enhancements: Error Handling & Robustness

### [ENHANCEMENT] Timeout Enforcement

**Location:** `src/detector_impl.cpp:WatermarkDetectorImpl::detect_text()`

**Current Behavior:**
- No timeout enforcement; code can run indefinitely on pathological inputs

**Required Enhancement:**
- Implement interrupt mechanism (thread-local state or callback)
- Abort long-running heuristics (token analysis, KL-divergence)
- Return partial results if timeout triggered
- Update `DetectionResult::status` to `TimeoutExceeded`

**Target Phase:** Phase 3  
**Estimated Effort:** 3–5 hours  
**Dependencies:** std::chrono, std::atomic for thread-safe flagging

**Acceptance:**
- [ ] No detection exceeds `DetectionConfig::timeout_ms`
- [ ] Partial results are meaningful (confidence score remains valid)
- [ ] No thread leaks (all async operations cleaned up)
- [ ] TSAN clean under timeout scenarios

---

### [ENHANCEMENT] Multilingual Robustness

**Location:** `src/detector_impl.cpp` + `src/token_analyzer.cpp`

**Current Behavior:**
- Language detection always returns "en"
- No UTF-8 validation or special character handling
- No CJK (Chinese, Japanese, Korean) specific logic

**Required Enhancement:**
- UTF-8 validation and normalization (NFC)
- Per-language token analysis adjustments
- CJK-specific green-list calibration
- Right-to-left (Arabic, Hebrew) text handling
- Mixed-language text detection

**Target Phase:** Phase 3  
**Estimated Effort:** 8–12 hours  
**Dependencies:** Unicode libraries (ICU or similar), multilingual test corpora

**Acceptance:**
- [ ] UTF-8 validation catches invalid sequences
- [ ] All 9+ supported languages produce valid results
- [ ] No crashes on pathological unicode (emoji, RTL, combining marks)
- [ ] Accuracy within ±5% across languages

---

## Phase 4 Enhancements: Testing & Validation

### [ENHANCEMENT] Paraphrase Robustness Framework

**Location:** `tests/test_paraphrased_text.cpp`

**Current Behavior:**
- Manual paraphrase examples only

**Required Enhancement:**
- Automated paraphrase generation (synonym replacement, structural changes, back-translation)
- Systematic robustness measurement (detection rate vs. edit distance)
- Parameterized paraphrase intensity levels (light, medium, heavy)
- Cross-dataset validation (different Claude corpus)

**Target Phase:** Phase 4  
**Estimated Effort:** 8–10 hours  
**Dependencies:** Paraphrase generation library (e.g., T5, BART), edit distance metrics

**Acceptance:**
- [ ] Robustness curve generated (edit distance vs. detection rate)
- [ ] >80% detection after light paraphrase
- [ ] <50% detection after heavy paraphrase
- [ ] Results documented in ROBUSTNESS_REPORT.md

---

### [ENHANCEMENT] False Positive Corpus Expansion

**Location:** `tests/test_false_positives.cpp`

**Current Behavior:**
- Placeholder test data only

**Required Enhancement:**
- Curate 100+ human-written texts from diverse domains:
  - Literary (Shakespeare, Austen, contemporary novels)
  - Technical (APIs, manuals, code comments)
  - News (Reuters, AP, BBC, Der Spiegel)
  - Social media (Twitter/X, Reddit)
  - Academic (papers, dissertations)
  - Legal (contracts, court documents)
  - Medical (journal articles, case studies)
- Validate false positive rate <5% across all domains
- Document any domain-specific biases

**Target Phase:** Phase 4  
**Estimated Effort:** 10–15 hours (mostly corpus curation)  
**Dependencies:** Text corpus sources, copyright/license verification

**Acceptance:**
- [ ] 100+ diverse human texts collected
- [ ] <5% false positive rate overall
- [ ] No domain with >10% false positive rate
- [ ] Bias analysis documented

---

## Phase 5 Enhancements: Performance Optimization

### [ENHANCEMENT] Heuristic Caching & Precomputation

**Location:** `src/token_analyzer.cpp`

**Current Behavior:**
- Recomputes all heuristics for every detect_text() call
- No pattern precomputation

**Required Enhancement:**
- Cache entropy values (repeated token IDs)
- Precompute common n-gram patterns
- Lazy evaluation (skip heuristics if prior result sufficient)
- SIMD-optimized token frequency computation

**Target Phase:** Phase 5  
**Estimated Effort:** 6–8 hours  
**Dependencies:** SIMD library (e.g., xsimd), profiling tools

**Acceptance:**
- [ ] 20–30% latency reduction vs. Phase 2 baseline
- [ ] Memory overhead <10MB for caches
- [ ] Cache hit rate >60% on real workloads
- [ ] Benchmarks locked in CI

---

### [ENHANCEMENT] Token Analyzer SIMD Vectorization

**Location:** `src/token_analyzer.cpp:compute_entropy()`

**Current Behavior:**
- Scalar loop-based computation

**Required Enhancement:**
- Vectorize frequency histogram computation (AVX2, SSE4.2)
- Vectorized entropy calculation (if numerically stable)
- Conditional SIMD support (graceful fallback)

**Target Phase:** Phase 5  
**Estimated Effort:** 6–8 hours  
**Dependencies:** xsimd or libsimd, profiling tools

**Acceptance:**
- [ ] 2–3x speedup on entropy computation vs. scalar
- [ ] Numerical accuracy maintained (relative error <1e-6)
- [ ] Fallback path for non-SIMD platforms
- [ ] Benchmarks documented

---

## Phase 6 Enhancements: Documentation & Production Readiness

### [ENHANCEMENT] Integration Guide (LLM Module)

**Location:** (New) `docs/WATERMARK_INTEGRATION_GUIDE.md`

**Required Content:**
- How to hook `WatermarkDetector` into `LLMInferenceResult` pipeline
- Example code: detecting text from LLM output
- Configuration examples for strict, moderate, lenient modes
- How to annotate knowledge graph with watermark metadata
- How to filter queries on watermark confidence
- Best practices (caching, batching, thread models)

**Target Phase:** Phase 6  
**Estimated Effort:** 4–6 hours  
**Acceptance:**
- [ ] Code examples compile and run
- [ ] Integration tested end-to-end with mock LLM module
- [ ] Performance characteristics documented

---

### [ENHANCEMENT] Performance Tuning Manual

**Location:** (New) `docs/WATERMARK_TUNING_GUIDE.md`

**Required Content:**
- Cache configuration (max_entries, eviction policy)
- Thread pool sizing for batch detection
- Timeout tuning (latency vs. accuracy tradeoff)
- Multilingual best practices
- Domain-specific calibration (technical vs. literary)
- Hardware profiling results
- SLA calculation (p95, p99, memory)

**Target Phase:** Phase 6  
**Estimated Effort:** 3–5 hours  
**Acceptance:**
- [ ] Tuning guide improves user performance by 10–20%
- [ ] Real-world deployment scenarios covered
- [ ] Troubleshooting section included

---

### [STUB] Doxygen Comment Completion

**Location:** All header files

**Current Behavior:**
- Headers have good Doxygen coverage, but implementation details in .cpp files need documentation

**Required Enhancement:**
- Complete Doxygen comments on all implementation methods
- Add example usage to main classes
- Generate HTML docs and verify rendering
- Link to performance benchmarks in docs

**Target Phase:** Phase 6  
**Estimated Effort:** 3–4 hours  
**Acceptance:**
- [ ] doxygen Doxyfile builds without warnings
- [ ] HTML docs are comprehensive and readable
- [ ] No undocumented public methods

---

## Known Limitations & Design Constraints

### Adversarial Robustness
- **Limitation:** Detector is vulnerable to attacks if algorithm is known
- **Mitigation:** Use as one signal; not sole authority on content origin
- **No Phase:** Out of scope for initial release (Phase 6)

### Model Family Diversity
- **Phase 1–2:** Claude only
- **Phase 3+:** Basic heuristics for GPT, Gemini, Llama
- **Future:** Model-specific watermark families as they emerge

### Computational Overhead
- **Target:** <15ms per 1k tokens
- **Trade-off:** Accuracy vs. latency tunable via heuristic selection

---

## Enhancement Prioritization

**Must-Have (Phase 2–3):**
- [ ] Tokenization integration (blocks all heuristics)
- [ ] Language detection (UTF-8 handling, 9+ languages)
- [ ] Claude watermark calibration (tuned to corpus)
- [ ] Timeout enforcement (resource safety)

**Should-Have (Phase 3–4):**
- [ ] Multilingual robustness (CJK, RTL, mixed-language)
- [ ] False positive corpus validation (bias detection)
- [ ] Paraphrase robustness framework (external attack model)

**Nice-to-Have (Phase 5–6):**
- [ ] SIMD optimization (performance tuning)
- [ ] Heuristic caching (incremental improvement)
- [ ] Integration guides (documentation)

---

## Testing Strategy for Enhancements

Each enhancement must include:
1. Unit tests (>=2 test cases per enhancement)
2. Integration tests (if cross-module)
3. Performance benchmarks (if Phase 5+)
4. Documentation (README, example code)
5. Ablation study (if changes core algorithm)

---

## Links & References

- **Phase Roadmap:** `ROADMAP.md`
- **Issue Tracker:** (To be created in Phase 2)
- **Performance Baselines:** `benchmarks/watermark_detector/` (Phase 5+)
- **Test Reports:** `tests/results/` (Phase 4+)

---

**Last Updated:** 2026-08-17  
**Next Review:** Post-Phase 1 acceptance
