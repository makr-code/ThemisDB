# Prompt Engineering Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Full lifecycle management for LLM prompt templates is production-ready. Version control, A/B testing, feedback collection, self-improvement orchestrator, and Prometheus metrics are all implemented.

## Completed ✅
- [x] PromptManager – CRUD with RocksDB persistence and YAML bulk-load
- [x] Thread-safe reads/writes via TBB `concurrent_hash_map`
- [x] Context injection (`{placeholder}` variable substitution)
- [x] `buildContextFromSchema()` – populate variables from SchemaManager snapshot
- [x] FeedbackCollector – 10 feedback types, aggregate stats, failure pattern analysis
- [x] PromptEvaluator – semantic similarity, exact match, partial match, relevance scoring
- [x] PromptOptimizer – iterative improvement with pluggable eval/improvement functions
- [x] MetaPromptGenerator – LLM-assisted prompt rewriting
- [x] Git-like version control (branches, commits, diffs, parent tracking)
- [x] A/B testing with statistical significance (p-value)
- [x] Self-improvement orchestrator with configurable trigger thresholds
- [x] Background worker thread for periodic auto-optimization
- [x] Prometheus-compatible metrics export
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

## Implementation Phases

### Phase 1: Template Management & Evaluation (Status: Completed ✅)
- [x] PromptManager – CRUD with RocksDB persistence and YAML bulk-load
- [x] Context injection (`{placeholder}` variable substitution) and `buildContextFromSchema()`
- [x] Chain-of-thought (CoT) prompt support with step delimiters
- [x] RAG prompt construction helpers (retrieved context injection)
- [x] System prompt management and per-role override
- [x] FeedbackCollector, PromptEvaluator, PromptOptimizer, MetaPromptGenerator
- [x] Git-like version control (branches, commits, diffs)
- [x] A/B testing with statistical significance (p-value)
- [x] Prometheus-compatible metrics export

### Phase 2: Typed DSL & Context Budget (Status: In Progress 🚧)
- [~] Typed template DSL with compile-time placeholder validation (Target: Q2 2026)
- [~] Context window budget manager – enforce token limits before dispatch (Target: Q2 2026)
- [ ] Prompt injection attack detection layer (Target: Q2 2026)
- [ ] Multi-modal prompt support (image descriptions alongside text) (Target: Q3 2026)

### Phase 3: Tracing, Regression & Experiments (Status: Planned 📋)
- [ ] CoT execution tracer – record per-step reasoning chain with latency attribution
- [ ] Prompt regression suite – detect quality degradation on model upgrade
- [ ] A/B experiment framework with configurable traffic splits and automated winner selection
- [ ] Import/export prompt library to JSON / YAML for cross-environment portability
- [ ] Per-language prompt template variants (i18n support)

## Production Readiness Checklist
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

## Breaking Changes
- PromptTemplate schema is stable from v1.x; new optional fields only.
- `FeedbackType` enum may gain new values; exhaustive switches in callers should use a default case.
