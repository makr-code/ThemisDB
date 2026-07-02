# Sprint 6 Phase 2 Gap Remediation Checklist

## Format String Remediation Targets (25 gaps)

### Tier 1: Critical Path (RAG & Network)
- [ ] src/rag/evaluation_report_exporter.cpp (1 gap)
  - [ ] Location identified
  - [ ] SafeFormat wrapper applied
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/rag/flare_retrieval.cpp (1 gap)
  - [ ] Location identified
  - [ ] SafeFormat wrapper applied
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/rag/self_rag.cpp (1 gap)
  - [ ] Location identified
  - [ ] SafeFormat wrapper applied
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/rag/tensor_rag_pipeline.cpp (1 gap)
  - [ ] Location identified
  - [ ] SafeFormat wrapper applied
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/network/connection_pool.cpp (2 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeFormat wrapper applied
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeFormat wrapper applied
  - [ ] Compilation verified
  - [ ] Existing tests pass

### Tier 2: Core Infrastructure (Index & Content)
- [ ] src/index/btree_node.cpp (2 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeFormat wrapper applied
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeFormat wrapper applied
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/content/content_processor.cpp (2 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeFormat wrapper applied
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeFormat wrapper applied
  - [ ] Compilation verified
  - [ ] Existing tests pass

### Tier 3: Query & Analytics
- [ ] src/query/query_optimizer.cpp (1 gap)
  - [ ] Location identified
  - [ ] SafeFormat wrapper applied
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/analytics/aggregation_window.cpp (1 gap)
  - [ ] Location identified
  - [ ] SafeFormat wrapper applied
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/utils/string_utility.cpp (1 gap)
  - [ ] Location identified
  - [ ] SafeFormat wrapper applied
  - [ ] Compilation verified
  - [ ] Existing tests pass

### Tier 4: Additional Format String Gaps (15 more)
- [ ] Gap 11: (File TBD)
- [ ] Gap 12: (File TBD)
- [ ] Gap 13: (File TBD)
- [ ] Gap 14: (File TBD)
- [ ] Gap 15: (File TBD)
- [ ] Gap 16: (File TBD)
- [ ] Gap 17: (File TBD)
- [ ] Gap 18: (File TBD)
- [ ] Gap 19: (File TBD)
- [ ] Gap 20: (File TBD)
- [ ] Gap 21: (File TBD)
- [ ] Gap 22: (File TBD)
- [ ] Gap 23: (File TBD)
- [ ] Gap 24: (File TBD)
- [ ] Gap 25: (File TBD)

---

## ReDoS Remediation Targets (25 gaps)

### Tier 1: LLM & Auth (High Priority)
- [ ] src/llm/aql_train_parser.cpp (3 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Gap 3 location identified
  - [ ] Gap 3 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/llm/constitutional_reasoning_engine.cpp (2 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/llm/ethical_guidelines_manager.cpp (2 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/llm/feedback_plugin_basic.cpp (2 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/auth/principal_validator.cpp (2 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeRegex wrapper applied (timeout: 2s - auth context)
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeRegex wrapper applied (timeout: 2s - auth context)
  - [ ] Compilation verified
  - [ ] Existing tests pass

### Tier 2: Security & Query (Core Paths)
- [ ] src/security/input_validator.cpp (3 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeRegex wrapper applied (timeout: 1s - high volume)
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeRegex wrapper applied (timeout: 1s - high volume)
  - [ ] Gap 3 location identified
  - [ ] Gap 3 SafeRegex wrapper applied (timeout: 1s - high volume)
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/query/query_parser.cpp (2 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Compilation verified
  - [ ] Existing tests pass

### Tier 3: Cache & Config
- [ ] src/cache/adaptive_query_cache.cpp (2 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeRegex wrapper applied (timeout: 2s - cache context)
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeRegex wrapper applied (timeout: 2s - cache context)
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/config/config_schema_validator.cpp (2 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeRegex wrapper applied (timeout: 5s)
  - [ ] Compilation verified
  - [ ] Existing tests pass

- [ ] src/content/abuse_detector.cpp (2 gaps)
  - [ ] Gap 1 location identified
  - [ ] Gap 1 SafeRegex wrapper applied (timeout: 3s - abuse detection)
  - [ ] Gap 2 location identified
  - [ ] Gap 2 SafeRegex wrapper applied (timeout: 3s - abuse detection)
  - [ ] Compilation verified
  - [ ] Existing tests pass

### Tier 4: Additional ReDoS Gaps (15 more)
- [ ] Gap 11: (File TBD)
- [ ] Gap 12: (File TBD)
- [ ] Gap 13: (File TBD)
- [ ] Gap 14: (File TBD)
- [ ] Gap 15: (File TBD)
- [ ] Gap 16: (File TBD)
- [ ] Gap 17: (File TBD)
- [ ] Gap 18: (File TBD)
- [ ] Gap 19: (File TBD)
- [ ] Gap 20: (File TBD)
- [ ] Gap 21: (File TBD)
- [ ] Gap 22: (File TBD)
- [ ] Gap 23: (File TBD)
- [ ] Gap 24: (File TBD)
- [ ] Gap 25: (File TBD)

---

## Cross-Module Integration Verification

- [ ] All 50 gap remediations compiled successfully
- [ ] No new compilation warnings introduced
- [ ] All modified modules' existing tests PASS
- [ ] SafeFormat tests pass (20+)
- [ ] SafeRegex tests pass (40+)
- [ ] No performance regression detected
- [ ] No new security vulnerabilities (CodeQL)

---

## Documentation Checklist

- [ ] SafeFormat usage guide created
- [ ] SafeRegex usage guide created
- [ ] Code comments updated with remediation rationale
- [ ] API documentation strings follow Doxygen format
- [ ] Error handling patterns documented
- [ ] Timeout rationale documented for ReDoS remediations
- [ ] Sprint 6 completion summary created
- [ ] Gap remediation tracking updated

---

## Sign-Off Requirements

- [ ] All 50 gaps remediated and verified
- [ ] Build verification: PASS
- [ ] Test verification: PASS
- [ ] Code review: PASS
- [ ] Security verification: PASS
- [ ] Documentation: PASS
- [ ] Sprint 6 complete and merge-ready
