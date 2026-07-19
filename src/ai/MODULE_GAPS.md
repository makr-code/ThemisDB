# ai — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **ai** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 134
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)
- **Production Readiness**: 🟡 BETA - 13 HIGH-severity gaps must be addressed before prod deployment

### By Severity

- **CRITICAL**: 0
- **HIGH**: 13 ⚠️ **Blocking prod readiness** (see Gap Resolution Roadmap below)
- **MEDIUM**: 119
- **LOW**: 2

### By Type

- module_doc_linkset_drift: 2
- no_retry_logic: 1
- pointer_arithmetic_unbounded: 8
- range_temporary: 1
- scope_mismatch: 117
- todo_as_productionlogic: 2
- unchecked_result: 1
- unvalidated_llm_output: 2

## Top 20 Gaps

- [unchecked_result] cai_ethics_integration.cpp:169 (HIGH)
- [no_retry_logic] ai_plugin_generator.cpp:263 (HIGH)
- [pointer_arithmetic_unbounded] cai_ethics_integration.cpp:271 (HIGH)
- [pointer_arithmetic_unbounded] cai_ethics_integration.cpp:272 (HIGH)
- [pointer_arithmetic_unbounded] cai_ethics_integration.cpp:273 (HIGH)
- [pointer_arithmetic_unbounded] cai_ethics_integration.cpp:274 (HIGH)
- [pointer_arithmetic_unbounded] cai_ethics_integration.cpp:293 (HIGH)
- [pointer_arithmetic_unbounded] cai_ethics_integration.cpp:294 (HIGH)
- [range_temporary] ai_plugin_generator.cpp:296 (HIGH)
- [pointer_arithmetic_unbounded] cai_ethics_integration.cpp:306 (HIGH)
- [pointer_arithmetic_unbounded] cai_ethics_integration.cpp:307 (HIGH)
- [unvalidated_llm_output] ai_plugin_generator.cpp:408 (HIGH)
- [unvalidated_llm_output] ai_plugin_generator.cpp:447 (HIGH)
- [todo_as_productionlogic] ai_plugin_generator.cpp:7 (MEDIUM)
- [todo_as_productionlogic] cai_ethics_integration.cpp:7 (MEDIUM)
- [scope_mismatch] cai_ethics_integration.cpp:43 (MEDIUM)
- [scope_mismatch] cai_ethics_integration.cpp:49 (MEDIUM)
- [scope_mismatch] cai_ethics_integration.cpp:55 (MEDIUM)
- [scope_mismatch] cai_ethics_integration.cpp:58 (MEDIUM)
- [scope_mismatch] cai_ethics_integration.cpp:61 (MEDIUM)

... and 114 more gaps.

---

## Gap Resolution Roadmap (BLOCKING PROD MERGE)

### HIGH-Severity Gaps (13 total) - MUST RESOLVE BEFORE PRODUCTION

#### Pointer Arithmetic (8 cases) - cai_ethics_integration.cpp
- **Lines**: 271, 272, 273, 274, 293, 294, 306, 307
- **Issue**: gap_scanner flags variable assignments as pointer_arithmetic_unbounded
- **Assessment**: Likely false positives (standard vector access with bounds); requires verification
- **Action**: 
  - [ ] Verify actual pointer arithmetic exists (or confirm false positive)
  - [ ] If false positive, document in CODE_REVIEW_EXCEPTIONS.md
  - [ ] If real, add bounds checking or use safe accessors

#### Unvalidated LLM Output (2 cases) - ai_plugin_generator.cpp
- **Lines**: 408, 447
- **Issue**: LLM endpoint response not fully validated for schema/structure
- **Current**: Size limits enforced (8 MiB); JSON parsing with exception handling
- **Gap**: Missing per-field validation (e.g., code field ≤ 1 MiB, version format, etc.)
- **Action**:
  - [ ] Add schema validation after JSON parse (lines 449+)
  - [ ] Enforce field-size bounds before assignment to GeneratedPlugin
  - [ ] Document validation contract in docstring

#### No Retry Logic (1 case) - ai_plugin_generator.cpp
- **Line**: 263 (or near endpoint invocation)
- **Issue**: gap_scanner flags missing retry logic
- **Current**: Retry logic exists at lines 415-430 (3 attempts, exponential backoff)
- **Assessment**: Likely false positive; scanner may not detect retry in loop
- **Action**:
  - [ ] Verify scanner is reading current code version
  - [ ] If false positive confirmed, document and close

#### Unchecked Result (1 case) - cai_ethics_integration.cpp
- **Line**: 169 (ostringstream oss)
- **Issue**: Result object created but not explicitly checked for state
- **Current**: ostringstream is used correctly; oss.str() returns constructed string
- **Assessment**: Likely false positive (ostringstream::str() is safe)
- **Action**:
  - [ ] Add explicit state check or document as false positive

#### Range Temporary (1 case) - ai_plugin_generator.cpp
- **Line**: 296 (std::unordered_set::insert)
- **Issue**: Temporary return value from insert() may reference temporary container
- **Current**: insert() returns pair<iterator, bool>; second value used for duplicate check
- **Assessment**: Likely false positive (pair is not a range; value used immediately)
- **Action**:
  - [ ] Verify actual issue exists; document if false positive

---

## Phase 5 Verification Notes

External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).

## Next Steps for Production Readiness

1. **Immediate** (blocking prod merge):
   - Resolve or document all 13 HIGH-severity gaps
   - Provide gap-resolution evidence (code review, test coverage)

2. **Short-term** (Q1 2027):
   - Address MEDIUM-severity gaps (scope_mismatch mainly)
   - Run full test suite with coverage reporting
   - Integrate with downstream systems (Query, Evaluation modules)

3. **Medium-term** (Q2 2027):
   - Wave B integration (Self-RAG, RotatE, LoRA)
   - Federated learning coordination
   - Performance benchmarking

---

**Status**: 🟡 BETA - Validation in progress; blockers documented above.
