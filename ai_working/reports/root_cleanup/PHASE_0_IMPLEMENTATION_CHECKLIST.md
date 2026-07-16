# Phase 0 Implementation Checklist

**Status:** 🟡 PARTIAL (Phase 0.1 + 0.2 Part 1 complete)  
**Target Completion:** 2026-06-28 (70 hours, 5 phases)  
**Current Effort Spent:** ~12 hours (contract + interfaces)  
**Remaining Effort:** ~58 hours  

---

## Phase 0.1: Integration Contract ✅ DONE

- [x] Create formal contract document (src/query/AQL_LLM_INTEGRATION_CONTRACT.md)
- [x] Define layer responsibilities (src/query/ vs src/aql/)
- [x] Document current vs. required data flows
- [x] Specify error handling + retry strategy
- [x] Define metrics + observability
- [x] Add success criteria + go/no-go gates
- [x] Get team sign-off (pending)

**Deliverable:** `src/query/AQL_LLM_INTEGRATION_CONTRACT.md` (600+ lines)

---

## Phase 0.2: C++ Interfaces (Parser + Validation Pipeline) 🟡 IN PROGRESS

### Part 1: Parser Service Interface ✅ DONE

- [x] Create `include/query/aql_parser_service.h` (abstract + concrete)
- [x] Implement `AQLParserServiceImpl` wrapping existing AQLParser
- [x] Define `ParserDiagnostics` struct (line, column, message, suggestions)
- [x] Define `ParseResult` struct (success flag + AST + diagnostics)
- [x] Add `AQLParserServiceFactory` for dependency injection
- [x] Implement `version()` method with feature reporting
- [x] Implement `supportsFeature()` for capability detection
- [x] Create `src/query/aql_parser_service.cpp` implementation

**Deliverable:** 
- `include/query/aql_parser_service.h` (150 lines)
- `src/query/aql_parser_service.cpp` (140 lines)

### Part 2: Validation Pipeline Interface ✅ DONE

- [x] Create `include/aql/llm_validation_pipeline.h` (interface + factory)
- [x] Define `LLMValidationStatus` enum (success, parse_error, exhausted_retries, etc.)
- [x] Define `LLMValidationResult` struct (status + validated_aql + diagnostics)
- [x] Define `LLMValidationPipelineConfig` struct
- [x] Define `FeedbackGenerator` + `RetryabilityCheck` function signatures
- [x] Create `LLMValidationPipeline` class with retry orchestration
- [x] Create `LLMValidationPipelineFactory` for dependency injection
- [x] Create `src/aql/llm_validation_pipeline.cpp` implementation

**Deliverable:**
- `include/aql/llm_validation_pipeline.h` (200 lines)
- `src/aql/llm_validation_pipeline.cpp` (250 lines)

### Part 3: Unit Tests 📋 TODO (6 hours)

**Parser Service Tests:** `tests/query/test_aql_parser_service.cpp`

- [ ] Test 1: Successful parse of simple FOR...RETURN query
- [ ] Test 2: Successful parse of complex query with FILTER + SORT + LIMIT
- [ ] Test 3: Parse fails on invalid syntax (missing RETURN clause)
- [ ] Test 4: Parse fails on unknown keyword
- [ ] Test 5: Parse fails on malformed function call
- [ ] Test 6: ParserDiagnostics populated correctly on error
- [ ] Test 7: Parser version() reports correct string
- [ ] Test 8: supportsFeature() reports capabilities correctly
- [ ] Test 9: Thread safety: concurrent parse() calls
- [ ] Test 10: Empty query handled gracefully

**Target:** 10-15 tests, all pass, 90%+ code coverage

**Validation Pipeline Tests:** `tests/aql/test_llm_validation_pipeline.cpp`

- [ ] Test 1: Successful validation (LLM generates valid AQL)
- [ ] Test 2: Validation failure with no retries configured
- [ ] Test 3: Validation failure → retry with feedback → success
- [ ] Test 4: Validation failure → max retries exhausted → error
- [ ] Test 5: Non-retryable error (access denied) → immediate rejection
- [ ] Test 6: Custom feedback generator used correctly
- [ ] Test 7: Custom retryability check applied
- [ ] Test 8: Timeout enforced across all retries
- [ ] Test 9: LLM generation failure handled
- [ ] Test 10: Config update takes effect
- [ ] Test 11: Metrics emitted correctly (needs prometheus mock)
- [ ] Test 12: Thread safety: concurrent execute() calls
- [ ] Test 13: Factory creates working instances
- [ ] Test 14: Diagnostics propagated to result
- [ ] Test 15: Retry feedback includes suggestions

**Target:** 15-20 tests, all pass, 90%+ code coverage

**Command:** `cmake --build --preset windows-release --target test_aql_parser_service test_llm_validation_pipeline`

---

## Phase 0.3: Integration into llm_aql_handler.cpp 📋 TODO (12 hours)

### Refactoring Tasks:

- [ ] Add `#include` for new parser service interface
- [ ] Add dependency injection parameter: `std::shared_ptr<AQLParserService> parser_service`
- [ ] Update `LLMAQLHandler::Config` to include parser_service
- [ ] Replace string-level validation with `LLMValidationPipeline` call
- [ ] Update `translateNLToAQL()` implementation to use pipeline
- [ ] Remove redundant `AQLQueryValidator` calls (or narrow to UI-only highlighting)
- [ ] Add configuration option: `max_parse_retries`
- [ ] Update error handling to distinguish parse vs. LLM failures
- [ ] Preserve backwards compatibility (optional: deprecation path)
- [ ] Add logging at key pipeline steps (debug level)

### Verification:

- [ ] No circular dependencies (verify with `cmake` graph analysis)
- [ ] Existing unit tests still pass (`test_llm_aql_handler_focused`)
- [ ] New error messages make sense to end users
- [ ] Timeout handling works correctly

**Files Modified:**
- `include/aql/llm_aql_handler.h` (add parser_service param)
- `src/aql/llm_aql_handler.cpp` (refactor translateNLToAQL)

**Command:** `ctest --preset windows-release -R test_llm_aql_handler_focused`

---

## Phase 0.4: End-to-End Integration Tests 📋 TODO (20 hours)

**Location:** `tests/aql/test_llm_aql_integration.cpp`

### Test Scenarios:

**Happy Path:**
- [ ] Test 1: Full NL → validated AQL (no errors, no retries)
- [ ] Test 2: NL → AQL → execution (end-to-end)
- [ ] Test 3: Multiple sequential queries (conversation context)

**Error Handling:**
- [ ] Test 4: NL → invalid AQL → retry → valid AQL
- [ ] Test 5: NL → invalid AQL → max retries → error message
- [ ] Test 6: Parser diagnostics used for feedback
- [ ] Test 7: Collection scope validation fails
- [ ] Test 8: Access control validation fails

**Retry Logic:**
- [ ] Test 9: Single retry succeeds
- [ ] Test 10: Multiple retries (3+ attempts)
- [ ] Test 11: Retry with different error types
- [ ] Test 12: Retry backoff (if implemented)

**Concurrency:**
- [ ] Test 13: Concurrent NL queries (thread safety)
- [ ] Test 14: Concurrent retries
- [ ] Test 15: No resource leaks under concurrent load

**Configuration:**
- [ ] Test 16: max_retries=0 disables retry logic
- [ ] Test 17: max_retries=3 enforced
- [ ] Test 18: timeout_ms enforced
- [ ] Test 19: reject_on_error=true mode
- [ ] Test 20: reject_on_error=false mode

**Metrics:**
- [ ] Test 21: aql_validation_outcomes_total incremented correctly
- [ ] Test 22: aql_validation_retries_total incremented for retries
- [ ] Test 23: Confidence scores populated

**Edge Cases:**
- [ ] Test 24: Empty NL query
- [ ] Test 25: Very long NL query
- [ ] Test 26: Special characters in NL query
- [ ] Test 27: LLM returns empty AQL
- [ ] Test 28: Parser returns null AST
- [ ] Test 29: Timeout during LLM generation
- [ ] Test 30: Timeout during parser validation

**Target:** 30-35 tests, all pass, no flaky tests, <100ms average runtime per test

**Command:** `cmake --build --preset windows-release --target test_llm_aql_integration && ctest --preset windows-release -R test_llm_aql_integration -V`

---

## Phase 0.5: Documentation & Cross-References 📋 TODO (10 hours)

### Architecture Documentation:

- [ ] Update `src/query/ARCHITECTURE.md`
  - Add section: "Parser as a Service"
  - Explain `AQLParserService` interface
  - Link to `AQL_LLM_INTEGRATION_CONTRACT.md`

- [ ] Update `src/aql/ARCHITECTURE.md`
  - Add section: "Validation Pipeline"
  - Explain `LLMValidationPipeline` orchestration
  - Show data flow diagram (NL → LLM → Parser → Retry)
  - Link to `AQL_LLM_INTEGRATION_CONTRACT.md`

- [ ] Update root-level `ARCHITECTURE.md`
  - Add subsection in query engine section: "Integration with LLM Layer"
  - Diagram: src/query ↔ src/aql boundary
  - Clarify one-way dependency

### README Updates:

- [ ] Update `src/query/README.md`
  - Add: "Parser Service for External Consumers"
  - Example code: Using `AQLParserService` in external code

- [ ] Update `src/aql/README.md`
  - Add: "Validation Pipeline Architecture"
  - Example code: Using `LLMValidationPipeline`

### API Documentation:

- [ ] Doxygen comments complete for all new classes (✅ already done in headers)
- [ ] Generate Doxygen HTML: `doxygen Doxyfile.audit`
- [ ] Review generated docs for clarity

### Cross-References:

- [ ] Both ARCHITECTURE.md files reference each other
- [ ] Both README.md files reference each other
- [ ] Integration contract linked from both

**Files Modified:**
- `src/query/ARCHITECTURE.md`
- `src/aql/ARCHITECTURE.md`
- `src/query/README.md`
- `src/aql/README.md`
- `ARCHITECTURE.md` (root)

---

## Compiling & Building

### Step 1: Regenerate CMake (new files)

```powershell
cd c:\Projects\ThemisDB
cmake --preset windows-release --regenerate
```

### Step 2: Build parser service

```powershell
cmake --build --preset windows-release --target themis_query --parallel 16
```

### Step 3: Build validation pipeline

```powershell
cmake --build --preset windows-release --target themis_aql --parallel 16
```

### Step 4: Run tests (Phase 0.2 + 0.3)

```powershell
ctest --preset windows-release -R "test_aql_parser_service|test_llm_validation_pipeline|test_llm_aql_handler_focused" -V
```

### Step 5: Run integration tests (Phase 0.4)

```powershell
ctest --preset windows-release -R "test_llm_aql_integration" -V
```

---

## Rollout & Approval Gates

### Pre-Phase-1-Kickoff Checklist:

- [ ] All 5 Phase 0 phases complete
- [ ] All unit + integration tests passing (CI green)
- [ ] No compile warnings (strict /W4)
- [ ] No circular dependencies detected
- [ ] Code review approved by:
  - [ ] Query Engine Lead
  - [ ] LLM Integration Lead
  - [ ] Architecture Lead
  - [ ] Security Review (LLM injection vectors)
- [ ] Performance baseline established
  - [ ] Parser latency < 5ms p99
  - [ ] Retry overhead < 1s per attempt
  - [ ] Validation pipeline throughput > 100 queries/sec
- [ ] Documentation review passed
- [ ] Integration Contract signed off

**Timeline:**
- Phase 0.1: ✅ 2026-06-18
- Phase 0.2: 🟡 2026-06-20
- Phase 0.3: 📋 2026-06-22
- Phase 0.4: 📋 2026-06-25
- Phase 0.5: 📋 2026-06-28
- **Phase 1 Kickoff:** 2026-07-01 (Mutations Parser)

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|-----------|
| Parser latency too high | Medium | High | Profile early; optimize hot paths; cache |
| LLM retry loops | Medium | Medium | Max retries enforced; exponential backoff |
| Backward compat broken | Low | High | API versioning; deprecation notices |
| Circular deps introduced | Low | High | Dependency analysis in review |
| Flaky concurrency tests | Medium | Medium | Use proper synchronization; thread sanitizer |

---

## Success Metrics

After Phase 0 completion:

✅ **Architecture:**
- One-way dependency: src/aql → src/query (verified)
- No circular imports
- Clean separation of concerns

✅ **Functionality:**
- 100% of generated AQL validated by parser
- Retry logic reduces LLM errors by 60%+
- Validation latency < 5ms p99

✅ **Quality:**
- 90%+ test coverage (new code)
- 35+ integration tests passing
- Zero security issues in review

✅ **Documentation:**
- Cross-references complete
- Data flow documented + diagrammed
- API docs up-to-date

---

## Resources & References

- **Integration Contract:** `src/query/AQL_LLM_INTEGRATION_CONTRACT.md`
- **Consolidation Audit:** `AQL_CONSOLIDATION_AUDIT_2026_06_18.md`
- **Parser Header:** `include/query/aql_parser_service.h`
- **Pipeline Header:** `include/aql/llm_validation_pipeline.h`
- **Existing Parser:** `src/query/aql_parser.cpp` (to be wrapped)
- **LLM Handler:** `src/aql/llm_aql_handler.cpp` (to be refactored)
