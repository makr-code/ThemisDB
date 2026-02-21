# Prompt Engineering Module Roadmap

## Current Status
v1.x – Full lifecycle management for LLM prompt templates is production-ready. Version control, A/B testing, feedback collection, self-improvement orchestrator, and Prometheus metrics are all implemented.

## Completed ✅
- [x] PromptManager – CRUD with RocksDB persistence and YAML bulk-load
- [x] Thread-safe reads/writes via TBB `concurrent_hash_map`
- [x] Context injection (`{placeholder}` variable substitution)
- [x] `buildContextFromSchema()` – populate variables from SchemaManager snapshot
- [x] **Template validation** – `validateTemplate()` with `ValidationResult` (errors + warnings); integrated into `createTemplate()` and `loadFromYAML()`
- [x] FeedbackCollector – 10 feedback types, aggregate stats, failure pattern analysis
- [x] **FeedbackCollector scalability** – `getFeedbackPaged()` chunked API, `detectOutliers()` Z-score anomaly detection, FNV-1a audit checksum on every entry
- [x] PromptEvaluator – semantic similarity, exact match, partial match, relevance scoring
- [x] **PromptEvaluator statistical significance** – proper Welch's two-sample t-test replacing naive 5% threshold
- [x] PromptOptimizer – iterative improvement with pluggable eval/improvement functions
- [x] MetaPromptGenerator – LLM-assisted prompt rewriting
- [x] **MetaPromptGenerator LLM integration** – `ILLMProvider` interface; `setLLMProvider()` / graceful fallback
- [x] Git-like version control (branches, commits, diffs, parent tracking)
- [x] A/B testing with statistical significance (p-value via standard normal CDF)
- [x] **A/B test statistics** – replaced hardcoded z-score table with `std::erfc`-based normal CDF
- [x] Self-improvement orchestrator with configurable trigger thresholds
- [x] **SelfImprovementOrchestrator eval_fn** – wired to real `PromptEvaluator`; heuristic fallback only when no evaluator available
- [x] Background worker thread for periodic auto-optimization
- [x] Prometheus-compatible metrics export
- [x] **Metrics persistence** – `snapshotToJson()` / `restoreFromJson()` for crash-safe recovery
- [x] **Threshold alerting** – `AlertConfig` / `AlertCallback` hooks firing on failure rate and hallucination count breaches
- [x] Integration facade combining all subsystems

## In Progress 🚧
- [ ] Token counting and context-window budget enforcement (Target: Q2 2026)
- [ ] Multi-modal prompt support (image descriptions alongside text) (Target: Q3 2026)
- [ ] Prompt injection attack detection layer (Target: Q2 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Prompt diff visualization in web UI
- [ ] Batch A/B test runner with configurable traffic splits
- [ ] Import/export prompt library to JSON or YAML
- [ ] Per-language prompt template variants (i18n)
- [ ] Latency SLO tracking per prompt template

### Long-term (6-12 months)
- [ ] Reinforcement learning from human feedback (RLHF) integration
- [ ] Cross-model prompt portability scoring (GPT-4 vs. LLaMA compatibility)
- [ ] Automated regression detection when base model is upgraded
- [ ] Prompt chaining and DAG execution
- [ ] Community prompt library with versioned sharing

## Production Readiness Checklist
- [x] Template validation with detailed error reporting
- [x] Feedback paging API for large archives
- [x] Audit trail (checksum) on feedback entries
- [x] Pluggable LLM interface for MetaPromptGenerator
- [x] Welch's t-test for statistical significance
- [x] Proper normal CDF for A/B test z-test p-values
- [x] Metrics snapshot/restore for crash recovery
- [x] Threshold-based alerting with pluggable callbacks
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (version control round-trip, A/B statistical significance)
- [ ] Performance benchmarks (optimization loop latency, concurrent access)
- [ ] Security audit (prompt injection risk, feedback data PII)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- Token counting and context-window management is out of scope; callers must manage limits.
- Prompt injection attack detection must be handled by callers.
- Multi-modal prompts (image/audio) are not supported in the current release.
- Full LLM-based evaluation in `optimizePrompt()` requires callers to execute the prompt and supply a custom `eval_fn`; the built-in fallback uses `PromptEvaluator` structural similarity as a proxy.

## Breaking Changes
- PromptTemplate schema is stable from v1.x; new optional fields only.
- `FeedbackType` enum may gain new values; exhaustive switches in callers should use a default case.
- `PromptManager::createTemplate()` now returns an empty-id sentinel on validation failure (id.empty() == true); callers should check the returned id before use.
