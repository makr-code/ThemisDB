# LLM Module — Gap Closure Implementation Guide

**Status**: In Progress (2026-08-17)  
**Target Completion**: 2026-08-31  
**Total Gaps**: 12,474 (1,400 IMPL + 11,074 DOC)

---

## Executive Summary

This guide orchestrates the systematic closure of LLM module gaps across implementation (IMPL), documentation (DOC), and operational readiness.

### Gap Categories & Work Streams

| Category | Count | Type | Owner | Status |
|----------|-------|------|-------|--------|
| **CRITICAL** | 155 | IMPL+DOC | Sub-Agent 1 | 🔄 In Progress |
| Braces Imbalance | 37 | IMPL | llm-braces-critical-fixes | 🔄 In Progress |
| Data Race | 11 | IMPL | llm-thread-safety-fixes | 🔄 In Progress |
| Resource Leak | 108 | IMPL | llm-raii-resource-fixes | 🔄 In Progress |
| **HIGH** | 1,095 | IMPL+DOC | Sub-Agents 2-4 | 📋 Queued |
| Thread Safety | 150+ | IMPL | llm-thread-safety-fixes | 🔄 In Progress |
| Exception Safety | 61+ | IMPL | llm-raii-resource-fixes | 🔄 In Progress |
| **MEDIUM** | 11,223 | DOC | Sub-Agent 4 | 🔄 In Progress |
| Documentation | 11,074 | DOC | llm-documentation-enhancements | 🔄 In Progress |
| Code Quality | ~150 | IMPL | Follow-up agents | 📋 Planned |
| **LOW** | 1 | IMPL | Not required | ⏭️ Deferred |

---

## Phase 1: Critical Structural Fixes (In Progress)

### 1.1 Braces Imbalance (37 Critical Files)

**Objective**: Fix mismatched braces in 20 top-priority files at line 1.

**Files**:
- active_vram_allocator.cpp
- adapter_registry.cpp
- async_inference_engine.cpp
- block_table.cpp
- ethics_aware_confidence_detector.cpp
- gguf_loader.cpp
- grafana_metrics.cpp
- inference_engine_enhanced.cpp
- llama_wrapper.cpp
- llm_model_storage.cpp
- llm_prefix_cache.cpp
- meta_prompt_generator.cpp
- model_downloader.cpp
- model_loader.cpp
- multi_lora_manager.cpp
- multi_perspective_generator.cpp
- prompt_evaluator.cpp
- prompt_optimizer.cpp
- streaming_handler.cpp
- token_quota_manager.cpp

**Remediation**:
1. Identify missing opening/closing braces at file boundaries
2. Validate nested scope closure
3. Apply clang-format for consistency
4. Verify syntax with compiler pass (warnings only, no errors)

**Exit Criteria**:
- [ ] All 37 files parse without brace-related errors
- [ ] clang-format exits 0
- [ ] No functional code changes (brace fixes only)

**Owner**: llm-braces-critical-fixes (Sub-Agent)  
**Estimated Effort**: 2-4 hours  
**Status**: 🔄 In Progress

---

### 1.2 Thread-Safety & Data-Race Fixes

**Objective**: Fix 330 data-race patterns across LLM module via synchronized access.

**High-Impact Files**:
- active_vram_allocator.cpp (GPU memory state)
- async_inference_engine.cpp (queue/scheduler state)
- llm_plugin_manager.cpp (plugin registry)
- model_loader.cpp (model cache)

**Remediation Strategy**:
1. Map all shared mutable state
2. Add std::mutex or std::atomic<> guards per state
3. Use std::lock_guard for RAII locking
4. Document thread-safety contracts

**High-Priority Patterns**:
- **circular_lock_ordering** (108): Establish single, well-documented lock order
- **lock_contention** (15): Reduce critical sections; use read-write locks where applicable
- **deadlock_risk** (11): Enforce timeout-aware locking; document wait-free paths
- **primitive_no_volatile** (22): Replace with std::atomic<T> or mutex-protected access

**Exit Criteria**:
- [ ] All data-race detectors (TSan) pass with existing tests
- [ ] No new lock-order violations
- [ ] Thread-safety documented in public API

**Owner**: llm-thread-safety-fixes (Sub-Agent)  
**Estimated Effort**: 6-8 hours  
**Status**: 🔄 In Progress

---

### 1.3 RAII & Resource Management Fixes

**Objective**: Eliminate 108+ resource leaks via exception-safe RAII patterns.

**High-Impact Leak Categories**:
- **resource_leaked_in_exception** (108): Wrap in std::unique_ptr/shared_ptr
- **db_connection_leak** (192): RAII DB connection wrapper
- **gpu_memory_leak** (10): CUDA memory guard with cleanup on exception
- **manual_cleanup** (44): Replace manual delete with smart pointers
- **null_dereference** (59): Add strategic nullptr checks

**Remediation Approach**:
1. Identify all heap allocations (new) without corresponding delete guards
2. Replace with std::unique_ptr<T> (default) or std::shared_ptr<T> (shared ownership)
3. Add exception handlers that ensure cleanup
4. Remove manual delete statements (RAII will cleanup)

**Key Files**:
- gguf_loader.cpp (GPU model loading)
- model_loader.cpp (model lifecycle)
- llm_plugin_manager.cpp (plugin resource tracking)

**Exit Criteria**:
- [ ] AddressSanitizer (ASan) reports 0 leaks on module tests
- [ ] No manual delete without preceding nullptr initialization
- [ ] All constructors/destructors follow RAII principle

**Owner**: llm-raii-resource-fixes (Sub-Agent)  
**Estimated Effort**: 8-10 hours  
**Status**: 🔄 In Progress

---

## Phase 2: Documentation Enhancements (In Progress)

### 2.1 Module-Level Architecture Documentation

**Objective**: Close 500 module-level architecture documentation gaps.

**Artifacts to Update/Create**:
1. **ARCHITECTURE.md**: System design, component interactions, data flows
2. **README.md**: Quick-start, API overview, configuration
3. **PRODUCTION_REQUIREMENTS.md**: SLOs, resource constraints, operational expectations
4. **SECURITY.md**: Security model, audit/compliance, threat model

**Content Outline**:

#### ARCHITECTURE.md
```markdown
## Thread-Safety Model
- Inference engine: thread-safe queue + worker pool
- Model cache: read-write lock protected
- Plugin registry: immutable snapshots for iteration

## Resource Ownership
- Models: lifetime tied to EngineLifecycleManager
- Adapters: ref-counted via LLMAdapterFactory
- GPU Memory: VRAM allocator with fallback to host memory

## Error Handling & Fail-Closed Behavior
- Inference timeout → return error, not partial result
- Model load failure → fallback to previous model + alert
- Plugin crash → isolated + continue with degraded service
```

### 2.2 Inline Code Documentation

**Objective**: Add 8,000+ inline code comments and docstring headers.

**Coverage**:
1. **@file Headers** (90 .cpp files): Maturity level, responsibility, key classes
2. **Function Documentation**: Purpose, parameters, return values, exceptions, threading
3. **Thread-Safety Annotations**: Documents which fields are guarded by which locks
4. **Resource Lifecycle Comments**: Ownership, cleanup triggers, exception-safety level

**Template**:
```cpp
/// @brief <one-line purpose>
/// @param <name> <description>
/// @return <description>
/// @throws <exception> <when/why>
/// @thread_safety <THREAD_SAFE|CALLER_MUST_SYNCHRONIZE> -- <details>
/// @exception_safety <STRONG|BASIC|NOTHROW> -- <details>
```

### 2.3 Operational Runbooks

**Objective**: Create 200+ lines of operational guidance.

**Runbooks to Create**:
1. **Model Loading & Lifecycle**: Diagnosis, recovery steps
2. **Adapter Management**: Hot-swap procedures, failure modes
3. **Performance Tuning**: VRAM allocation, batch size optimization
4. **Debugging Guide**: Common errors, logging configuration

**Owner**: llm-documentation-enhancements (Sub-Agent)  
**Estimated Effort**: 4-6 hours  
**Status**: 🔄 In Progress

---

## Phase 3: Code Quality & Performance (Queued)

### 3.1 Exception Safety (61+ Occurrences)

**Objective**: Ensure all resource-allocating operations provide exception safety guarantees.

**Patterns**:
- **Strong Guarantee**: Operation either succeeds completely or has no effect
- **Basic Guarantee**: Operation succeeds or leaves system in valid (but different) state
- **No-Throw Guarantee**: Operation never throws (e.g., destructors, cleanup)

**Remediation**:
1. Audit all model load/unload paths for exception-safety level
2. Add exception-safety tests per Phase 4
3. Document guarantees in public API

### 3.2 Performance Optimization (150+ Issues)

**High-Impact Patterns**:
- **Copy Overhead** (109): Use move semantics, std::string_view
- **String Concat in Loop** (51): Use std::stringstream, pre-allocate capacity
- **O(n²) Operations** (36): Replace nested loops with indexed structures

**Constraints**:
- Only optimize after profiling data
- Preserve functional behavior
- Add performance tests to prevent regression

---

## Phase 4: Testing & Validation

### 4.1 Focused Test Expansion

**New Test Targets**:
- Thread-safety under concurrent load (data-race patterns)
- Exception-safety in model load/unload
- Resource cleanup on exception paths
- VRAM allocation/deallocation under pressure

**Test Framework**:
- Use themis_register_module_focused_test()
- Label: release_critical, llm, phase_N
- Timeout: 120s per test
- Deterministic (fixed seed, no timing dependencies)

### 4.2 Regression Gates

**Performance Gates** (GATE-LLM-01..08):
- Inference latency (p50, p95, p99)
- Model load time
- Adapter switch overhead
- Memory pressure under sustained load

---

## Deliverables & Milestones

### Milestone 1: Critical Structural Fixes (2026-08-20)
- [ ] All 37 braces_imbalance issues fixed
- [ ] Data-race patterns documented and synchronized
- [ ] RAII wrappers in place for all resource allocations
- [ ] Compile with clang -Wall -Wextra -pedantic

### Milestone 2: Documentation Complete (2026-08-24)
- [ ] Module ARCHITECTURE.md up to date
- [ ] README.md with quick-start and API overview
- [ ] 8,000+ inline documentation added
- [ ] Operational runbooks created

### Milestone 3: Tests & Validation (2026-08-28)
- [ ] 40+ focused hardening tests created
- [ ] ASan/TSan pass on module test suite
- [ ] Performance gates established and passing
- [ ] Code review complete, no critical findings

### Milestone 4: Production Release (2026-08-31)
- [ ] All gaps closed with evidence documented
- [ ] CHANGELOG.md updated with improvements
- [ ] Root ROADMAP.md reflects status
- [ ] PR merged to develop

---

## Sub-Agent Coordination

### Active Sub-Agents

| Agent ID | Task | Status | Expected Completion |
|----------|------|--------|---------------------|
| llm-braces-critical-fixes | Fix 37 brace imbalance issues | 🔄 | 2026-08-18 |
| llm-thread-safety-fixes | Fix data-race & sync patterns | 🔄 | 2026-08-20 |
| llm-raii-resource-fixes | Fix resource leaks & RAII | 🔄 | 2026-08-20 |
| llm-documentation-enhancements | Create/update docs + runbooks | 🔄 | 2026-08-22 |

### Coordination Points

1. **Braces fixes** must complete before thread-safety fixes (to avoid conflicting edits)
2. **RAII fixes** can run in parallel with braces fixes (independent files)
3. **Documentation** can proceed in parallel (non-blocking)
4. **All fixes** must merge cleanly before testing phase

---

## Definition of Done

For each gap category:
1. ✅ Code/docs updated with improvement
2. ✅ Changes build without errors/critical warnings
3. ✅ Existing tests still pass (no regressions)
4. ✅ New tests added for fixed issues (if applicable)
5. ✅ Documentation updated with changes
6. ✅ PR description captures what was fixed and why

---

## References

- Root: `/home/runner/work/ThemisDB/ThemisDB/ROADMAP.md` (Wave A–D model)
- Module: `src/llm/MODULE_GAPS.md` (gap taxonomy)
- Module: `src/llm/TODO_CRITICAL_GAPS.md` (priority checklist)
- Module: `src/llm/PRODUCTION_REQUIREMENTS.md` (acceptance criteria)
- Docs: `docs/governance/GA_PROMOTION_SIGN_OFF.md` (release gate model)

---

**Last Updated**: 2026-08-17  
**Next Review**: After each sub-agent completes (daily sync)
