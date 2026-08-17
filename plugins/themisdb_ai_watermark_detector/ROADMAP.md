# AI Watermark Detector Plugin — Phase 1–6 Roadmap

**Version:** 1.0.0-phase1  
**Target Completion:** Q4 2026  
**Status:** Phase 1 (Design & API Contract) in progress

---

## Overview

The AI Watermark Detector plugin detects AI-generated content (Claude, GPT, Gemini, Llama) via probabilistic watermarking and statistical heuristics. It integrates with ThemisDB's LLM, Ethics AI, and Knowledge Graph modules to provide:

- **Content Provenance Tracking**: Identify AI-generated outputs
- **Policy Enforcement**: Block or flag sensitive AI-generated content
- **Audit Trails**: GDPR/EU AI Act compliance
- **Knowledge Graph Integrity**: Warn on hallucinated/synthetic content

---

## Current Status

### Phase 1 — Design & API Contract ✓ (COMPLETE 2026-08-17)

- [x] Define `WatermarkDetector` abstract interface contract
- [x] Define `DetectionResult` output structure (confidence, model family, heuristic scores)
- [x] Define `DetectionConfig` parameter structure (thresholds, timeouts, caching, language support)
- [x] Define `TokenDistributionAnalyzer` for watermark heuristics
- [x] Document threading model (thread-safe concurrent detect_text())
- [x] Specify async detection option (Phase 2+: implement callback-based async)
- [x] Create integration specification with LLM module (output interception hook)
- [x] Document error codes: [8100–8199]
  - 8101: TokenizationFailed
  - 8102: InvalidConfiguration
  - 8103: TimeoutExceeded
  - 8104: UnsupportedLanguage
  - 8105: MemoryExhausted
  - 8106: InvalidInput

**Phase 1 Acceptance:** ✅
- Interface approved and documented
- Header-only API contract complete
- Error handling contract defined
- Integration points identified
- Phase 1 acceptance ready for review

---

## Implementation Phases (2-6)

### Phase 2 — Core Detection Logic (Target: Q3/Q4 2026)

**Acceptance Criteria:**
- [ ] Implement Claude watermark baseline detector (green/red list pattern matching)
- [ ] Integrate tokenization adapter (with LLM module's tokenizer)
- [ ] Implement green/red list pattern matching (deterministic hash-based)
- [ ] Add confidence scoring algorithm (aggregate heuristics)
- [ ] Add caching layer (LRU cache, with statistics)
- [ ] Implement language detection (ISO 639-1 codes)
- [ ] 10+ unit tests pass
- [ ] Baseline accuracy >85% on test corpus
- [ ] No uncaught exceptions in normal path

**Key Components:**
- `TokenDistributionAnalyzer::estimate_claude_watermark()` — Main Claude detector
- `TokenDistributionAnalyzer::compute_ngram_entrenchment()` — Entrenchment heuristic
- `TokenDistributionAnalyzer::compute_kl_divergence()` — Distribution shift detection
- Tokenization integration with LLM module
- Result caching (hash-based, LRU eviction)

---

### Phase 3 — Error Handling & Edge Cases (Target: Q4 2026)

**Acceptance Criteria:**
- [ ] Handle malformed/incomplete inputs (truncated text, encoding errors)
- [ ] Implement timeouts (enforce via DetectionConfig::timeout_ms)
- [ ] Add multilingual support (UTF-8 robustness, language detection)
- [ ] Graceful degradation (fallback to neutral score 0.5 if detection fails)
- [ ] Resource limit enforcement (memory, thread pool size)
- [ ] Handle empty, very short (<50 tokens), and very long (>100k tokens) texts
- [ ] No uncaught exceptions; all error paths return valid DetectionResult
- [ ] All edge case tests pass

**Key Areas:**
- Input validation (empty, oversized, encoding)
- Timeout handling (interrupt long-running heuristics)
- Multilingual robustness (UTF-8, language detection fallback)
- Memory management (cache eviction, large text truncation)
- Error status propagation (DetectionStatus enum)

---

### Phase 4 — Comprehensive Testing (Target: Q4 2026)

**Acceptance Criteria:**
- [ ] Unit tests: 50+ test cases
  - Detector basics (initialization, configuration, interface contract)
  - Token analyzer (entropy, green-list, n-gram)
  - Configuration validation
  - Edge cases (empty, short, long texts)
- [ ] Integration tests: LLM module interception
  - Mock LLMInferenceResult capture
  - Detection pass on inferred text
  - Result annotation to knowledge graph
- [ ] False positive test suite: 100+ human-written samples
  - Literary texts (Shakespeare, contemporary novels)
  - Technical documentation (APIs, manuals)
  - News articles (Reuters, AP)
  - Social media (Twitter/X posts, Reddit)
  - Academic papers
- [ ] Paraphrase robustness tests
  - Automatically rewritten (synonym replacement, structural changes)
  - Multi-pass paraphrasing
  - Expected detection rate >80% after light paraphrase, <50% after heavy
- [ ] Multilingual tests
  - English, German, French, Spanish, Italian, Portuguese, Dutch, Japanese, Chinese
  - Expected accuracy >85% per language

**Test Coverage:**
- ≥95% precision on known-human corpus (false positive rate <5%)
- ≥90% recall on known-Claude corpus (false negative rate <10%)
- Edge case handling (timeouts, resource limits, encoding)
- Thread safety (TSAN, concurrent requests)

---

### Phase 5 — Performance & Hardening (Target: Q4 2026)

**Acceptance Criteria:**
- [ ] Profile and optimize token analyzer
  - Target: <10ms per 1k tokens
  - Actual: measure and optimize bottlenecks
- [ ] Benchmark memory footprint
  - Target: <50MB resident per detector instance
  - Actual: measure with valgrind/heaptrack
- [ ] Stress test: concurrent detection requests
  - 100+ parallel requests
  - No race conditions (TSAN clean)
  - No memory leaks (valgrind clean)
- [ ] Cache warm-up strategies
  - Pre-load common token patterns
  - LRU eviction policy tested
- [ ] Performance gates locked
  - p95 latency <15ms
  - Memory stable under sustained load (1M texts)

**Benchmarking:**
- End-to-end latency vs. text length
- Cache hit rate under realistic workloads
- Memory usage scaling
- Thread contention under high concurrency

---

### Phase 6 — Documentation & Release (Target: Q4 2026)

**Acceptance Criteria:**
- [ ] API documentation (Doxygen comments on all public methods)
  - Purpose, parameters, return values, exceptions
  - Example code snippets
- [ ] Integration guide (LLM + Ethics AI modules)
  - How to hook WatermarkDetector into LLMInferenceResult
  - How to annotate knowledge graph
  - Policy enforcement examples
- [ ] Performance tuning manual
  - Cache configuration
  - Thread pool sizing
  - Timeout tuning
  - Multilingual best practices
- [ ] Limitation disclosure
  - Adversarial robustness (attacker with algorithm knowledge)
  - False positive/negative rates by domain
  - Language-dependent accuracy
  - Computational cost (5-15ms per 1k tokens)
- [ ] ROADMAP.md completion
- [ ] README.md with quick-start
- [ ] Doxygen builds cleanly
- [ ] No breaking changes pending
- [ ] Integration tests pass on `release_critical` CI lane

---

## Success Metrics (Phase 6 Exit Criteria)

- ✅ Detector reliably identifies Claude-generated text (≥90% recall on curated corpus)
- ✅ False positive rate <5% on diverse human-written text
- ✅ Performance: p95 latency <15ms per 1k tokens
- ✅ Thread-safe under concurrent load (≥100 parallel requests)
- ✅ Graceful error handling: no uncaught exceptions
- ✅ Comprehensive Doxygen API documentation
- ✅ Integration tests verify LLM module interception works correctly
- ✅ CI green on `release_critical` lane
- ✅ Phase 6 acceptance checklist all PASS

---

## Known Limitations

### Non-Adversarial Context Only
If an attacker has access to the detection algorithm, watermarks can be removed via:
- Paraphrasing / rewriting
- Multi-pass watermark fragmentation
- Model-specific bypass techniques

**Mitigation:** Use as one signal among many; not sole authority on content origin.

### Claude-Specific
Initial release detects Claude-specific watermarks. Other models (GPT, Gemini, Llama) detected via:
- Heuristic baselines (entropy, n-gram entrenchment, KL-divergence)
- Model-family fingerprinting (Phase 2+)

**Expected accuracy by model:**
- Claude: ≥90% recall
- GPT: ≥70% recall (heuristic-based)
- Gemini, Llama: ≥60% recall (generic heuristics)

### Language & Domain Dependent
Accuracy varies by:
- **Text length**: Short texts (<100 tokens) have reduced reliability
- **Domain**: Technical vs. literary vs. social media have different signatures
- **Language**: Non-English texts may have ±15% accuracy variance
- **Hybrid text**: Mixture of AI + human content is ambiguous

### Not Foolproof
Edge cases with high false positive/negative rates:
- Stylized writing (poetry, formal legal text)
- Very short texts (<50 tokens)
- Highly repetitive content (product listings, code)
- Content heavily edited by human after AI generation

### Computational Cost
- Latency: 5-15ms per 1k tokens (~200 chars average)
- Memory: <50MB per detector instance
- CPU: Single-threaded analysis (parallelizable)

---

## Integration Points

### LLM Module
- Hook into `LLMInferenceResult` to automatically detect generated text
- Configuration flag: `llm_enable_watermark_detection` (default: true)
- Annotation: `LLMInferenceResult::ai_watermark_metadata`

### Knowledge Graph / Storage
- Annotate ingested documents with watermark metadata
- Query filter: `WHERE content_watermark_confidence > 0.95`
- Faceted search on AI-generated vs. human-generated

### Ethics AI Module
- Policy enforcement: reject/flag high-confidence AI-generated content in sensitive contexts
- Audit logging: all detections logged with timestamp, source, confidence, model family

### Query Engine
- Optional WHERE clause: `content_watermark_confidence > threshold`
- Faceted search aggregations on AI-generated flag

---

## Roadmap Integration

### Wave B — Performance Consolidation (Q4 2026)
- Integrate watermark detection into Search and Access Model result scoring
- Optimize performance gates for search latency SLA

### Wave C — Security Production Validation (Q4 2026)
- Policy enforcement gates (reject/flag high-confidence AI-generated in audit logs)
- Penetration testing (can attacker evade detection?)
- Compliance validation (GDPR, EU AI Act)

### Future Work
- Extend to GPT, Gemini, Llama watermark families as they emerge
- Adversarial robustness improvements
- Multi-model ensemble detection
- Real-time watermark tracking (updates when new models detected)

---

## Error Codes

**Range:** [8100–8199]

| Code | Name | Meaning | Recovery |
|------|------|---------|----------|
| 8101 | TokenizationFailed | Unable to tokenize input text | Return neutral score (0.5) |
| 8102 | InvalidConfiguration | Detection threshold out of range | Use safe default (0.7) |
| 8103 | TimeoutExceeded | Detection took >timeout_ms | Return partial result or timeout status |
| 8104 | UnsupportedLanguage | Language not in supported set | Attempt best-effort or skip |
| 8105 | MemoryExhausted | Pattern cache unable to allocate | Downgrade to lightweight detector |
| 8106 | InvalidInput | Input validation failed (empty, encoding) | Return neutral score (0.5) |
| 8199 | Unknown | Unknown error | Return neutral score (0.5) |

---

## Testing Strategy Summary

| Suite | Count | Target | Status |
|-------|-------|--------|--------|
| Unit tests (detector) | 20 | All pass | Phase 2–3 |
| Token analyzer tests | 15 | All pass | Phase 2 |
| Configuration tests | 5 | All pass | Phase 2 |
| Claude watermark validation | 15 | >90% recall | Phase 4 |
| False positive (human text) | 30 | <5% FP rate | Phase 4 |
| Paraphrase robustness | 10 | >80% detection after rewrite | Phase 4 |
| Multilingual samples | 10 | >85% accuracy | Phase 4 |
| Performance benchmarks | 5 | p95 <15ms | Phase 5 |
| Thread safety (TSAN) | 5 | No races | Phase 5 |
| Resource limits | 5 | No OOM | Phase 5 |
| Integration (LLM module) | 10 | All pass | Phase 4 |
| End-to-end | 5 | Sanity checks | Phase 6 |

**Total:** 135+ test cases targeting Phase 6 delivery

---

## Links & References

- **API Docs:** `include/detector_interface.h`, `include/detection_result.h`
- **Implementation:** `src/detector_impl.cpp`, `src/token_analyzer.cpp`
- **Configuration:** `include/detection_config.h`
- **Tests:** `tests/test_*.cpp`
- **Integration Guide:** (Phase 6) `docs/WATERMARK_INTEGRATION_GUIDE.md`
- **Performance Manual:** (Phase 6) `docs/WATERMARK_TUNING_GUIDE.md`
- **Research Sources:** `research/papers/watermark_detection.md` (to be added Phase 2+)

---

**Last Updated:** 2026-08-17  
**Next Review:** Post-Phase 1 acceptance
