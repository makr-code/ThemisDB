### Context

This issue implements the roadmap item 'Runtime-Configurable Confidence Scoring Weights' for the aql domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: 4 · Runtime-Configurable Confidence Scoring Weights

### Goal

Deliver the scoped changes for Runtime-Configurable Confidence Scoring Weights in src/aql/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### 4 · Runtime-Configurable Confidence Scoring Weights
**Priority:** Medium
**Target Version:** v1.6.0

**Problem (from code):** `aql_confidence_scorer.cpp` (lines 58–61) hard-codes the final scoring formula as `structural_score * 0.50f + completeness_score * 0.30f + schema_match_score * 0.20f`. The keyword-bonus table (lines 100–111) is also a hard-coded `std::vector<std::pair<std::string, float>>` with values like `{"filter", 0.20f}`, `{"sort ", 0.15f}`. The `0.5f` neutral return value for missing schema (line 129 and 134) and the `0.1f` floor for zero collection matches (line 145) are also untunable. Field names that appear in confidence scoring (like keyword `"upsert"` on line 111) use substring matching which can accidentally match sub-tokens.

**Implementation Notes:**
- `[ ]` Introduce an `AQLConfidenceScorer::Config` struct with fields: `float structural_weight`, `float completeness_weight`, `float schema_match_weight`, keyword `std::unordered_map<std::string, float> keyword_bonuses`, `float no_schema_neutral`, `float zero_match_floor`; default values match current hard-coded constants for backward compatibility
- `[ ]` Inject `Config` via constructor; `AQLConfidenceScorer()` (the default ctor) keeps existing behaviour
- `[ ]` Fix substring keyword matching (e.g. `"insert"` inside `"upsert"`) by checking word boundaries with `\b` regex or a tokenised lookup
- `[ ]` Add a `calibrate(const std::vector<std::pair<std::string,float>>& labelled_pairs)` method that fits the three top-level weights via least-squares regression on (query, ground-truth-confidence) pairs
- `[ ]` Unit-test: verify that calling `score()` on an empty query returns 0.0 and on a complete `FOR x IN c FILTER x.a == 1 RETURN x` returns > 0.7

---

### Acceptance Criteria

- [ ] Introduce an `AQLConfidenceScorer::Config` struct with fields: `float structural_weight`, `float completeness_weight`, `float schema_match_weight`, keyword `std::unordered_map<std::string, float> keyword_bonuses`, `float no_schema_neutral`, `float zero_match_floor`; default values match current hard-coded constants for backward compatibility
- [ ] Inject `Config` via constructor; `AQLConfidenceScorer()` (the default ctor) keeps existing behaviour
- [ ] Fix substring keyword matching (e.g. `"insert"` inside `"upsert"`) by checking word boundaries with `\b` regex or a tokenised lookup
- [ ] Add a `calibrate(const std::vector<std::pair<std::string,float>>& labelled_pairs)` method that fits the three top-level weights via least-squares regression on (query, ground-truth-confidence) pairs
- [ ] Unit-test: verify that calling `score()` on an empty query returns 0.0 and on a complete `FOR x IN c FILTER x.a == 1 RETURN x` returns > 0.7

### Relationships

- Roadmap row: #144 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/aql/FUTURE_ENHANCEMENTS.md#4--runtime-configurable-confidence-scoring-weights
- Source key: roadmap:144:aql:v1.6.0:4-runtime-configurable-confidence-scoring-weights

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:144:aql:v1.6.0:4-runtime-configurable-confidence-scoring-weights -->
<!-- roadmap-ref: row=144;module=aql;target=v1.6.0 -->
<!-- roadmap-detail: src/aql/FUTURE_ENHANCEMENTS.md#4--runtime-configurable-confidence-scoring-weights -->
