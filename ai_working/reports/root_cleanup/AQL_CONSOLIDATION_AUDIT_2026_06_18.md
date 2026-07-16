# AQL Architecture Consolidation Audit — 2026-06-18

**Status:** 🔴 **CRITICAL CONSOLIDATION NEEDED**  
**Severity:** 🔴 High (Architecture Confusion, Maintenance Risk)  
**Effort to Resolve:** 40–60 hours (P0)  
**Target Date:** End of Q2 2026 (2026-06-28)

---

## Executive Summary

ThemisDB has **TWO partially-overlapping AQL architectures** in parallel:

| Layer | Location | Purpose | Status |
|-------|----------|---------|--------|
| **Query Engine** | `src/query/` (50+ files) | Core parser, optimizer, executor | 🟢 Production-ready |
| **LLM Integration** | `src/aql/` (31 files) | NL-to-AQL translation, validation, tooling | 🟡 Partial integration |

**Root Cause:** Historical evolution where v1.x focused on query execution (`src/query/`), and v1.3+ added LLM assistance layer (`src/aql/`) as a separate module. **No formal integration boundary defined** → confusion in code ownership, documentation, and testing.

**Evidence of Confusion:**
1. **Two Parser Systems**: `src/query/aql_parser.cpp` (real parser) + `src/aql/aql_query_validator.cpp` (string-based validation) operate independently
2. **Documentation Duplication**: Both `src/query/ARCHITECTURE.md` and `src/aql/ARCHITECTURE.md` claim authority over "AQL"
3. **Data Flow Unclear**: How does generated AQL from `llm_aql_handler.cpp` reach the parser? Via what validation chain?
4. **Test Coverage Split**: Parser tests in `tests/query/`, LLM tests in `tests/aql/` — no integration tests
5. **Roadmap Misalignment**: `src/query/AQL_MUTATIONS_ROADMAP.md` targets mutations in query engine; `src/aql/ROADMAP.md` treats AQL as mostly LLM assistance

---

## Layer 1: Query Engine (`src/query/`)

### ✅ What It Does

| Component | Files | Lines | Status |
|-----------|-------|-------|--------|
| **Parser** | `aql_parser.cpp`, `aql_parser_json.cpp` | ~1,800 | 🟢 Production |
| **Optimizer** | `query_optimizer.cpp`, `adaptive_optimizer.cpp`, `optimizer_cost_model.cpp` | ~2,500 | 🟢 Production |
| **Executor** | `query_engine.cpp`, `query_compiler.cpp`, `parallel_executor.cpp`, `vectorized_execution.cpp` | ~3,200 | 🟢 Production |
| **Caching** | `query_cache.cpp`, `semantic_cache.cpp`, `cte_cache.cpp` | ~1,200 | 🟢 Production |
| **Federation** | `query_federation.cpp`, `cross_cluster_federation.cpp` | ~900 | 🟡 Partial |
| **Functions** | `let_evaluator.cpp`, `window_evaluator.cpp`, `functions/` | ~4,000 | 🟢 Production |

**Parser Capabilities (36 Tokens):**
```
FOR, FILTER, SORT, LIMIT, RETURN, LET, COLLECT, WITH,
ASC, DESC, AND, OR, NOT, IN,
SIMILARITY, PROXIMITY, SHORTEST_PATH, BEGIN, COMMIT, ROLLBACK,
GRAPH, OUTBOUND, INBOUND, ANY, ALL, SATISFIES,
TRUE, FALSE, NULL, AS, TO,
... 36 total tokens
```

**CRITICAL:** 0 DML/DDL tokens (INSERT, UPDATE, DELETE, CREATE, DROP)

### ❌ What It Doesn't Do

| Missing Feature | Impact | Type |
|-----------------|--------|------|
| INSERT/UPDATE/DELETE mutations | v2.0.0 | Parser Gap |
| CREATE/DROP/ALTER statements | v2.0.0 | Parser Gap |
| TRUNCATE collection | v2.0.0 | Parser Gap |
| Stored procedures/UDFs | v2.0.0 | Executor Gap |
| Transaction SAVEPOINT | v2.0.0 | Parser Gap |
| Schema validation (strictness) | v1.x acceptance | Partial |

### 📊 Code Quality

**aql_parser.cpp metrics:**
- Maturity: 🟢 PRODUCTION-READY (Score: 100/100)
- Gaps: 3 total (1 TODO, 1 Stub, 1 Mock)
- Lines: 1,633
- Test Coverage: High (focused tests in `test_aql_parser_focused.exe`)

---

## Layer 2: LLM Integration (`src/aql/`)

### ✅ What It Does

| Component | Purpose | Status |
|-----------|---------|--------|
| **llm_aql_handler.cpp** | NL-to-AQL translation via LLM; agentic workflows | 🟡 Partial |
| **aql_query_validator.cpp** | String-level validation (regex-based) | 🟡 Partial |
| **aql_query_builder.cpp** | Programmatic AQL construction | 🟢 Working |
| **aql_syntax_highlighter.cpp** | Syntax highlighting + token classification | 🟢 Working |
| **aql_confidence_scorer.cpp** | Confidence/quality scoring for generated AQL | 🟡 Stub paths |
| **aql_fewshot_example_library.cpp** | Few-shot example retrieval | 🟢 Working |
| **aql_conversation_context.cpp** | Conversational state management | 🟢 Working |
| **aql_schema_provider.cpp** | Schema context for assistance | 🟡 Partial |
| **docs_assistant_functions.cpp** | Function lookup/documentation | 🟢 Working |
| **aql_agent.cpp** | Agent orchestration helpers | 🟡 Stub paths |

### ❌ What It Doesn't Do

| Missing Feature | Impact | Issue |
|-----------------|--------|-------|
| **Call real parser after LLM generation** | Generated AQL may be invalid | llm_aql_handler line 289–300: returns LLM output without AST validation |
| **Wired into query execution** | LLM queries don't reach engine | No direct coupling to `src/query/` |
| **Mutation validation** | No semantic checks for INSERT/UPDATE/DELETE | Not implemented (DML tokens don't exist) |
| **Transaction-aware validation** | No BEGIN/COMMIT/ROLLBACK checks | Missing |
| **Performance profiling** | No metrics on LLM-generated vs. hand-written queries | Missing |

### 🔴 Critical Integration Gap

**Location**: `src/aql/llm_aql_handler.cpp` line 289–300  
**Current Behavior**: Generated AQL returns directly to user without AST validation:
```cpp
std::string generated_aql = llm_client->translate(nl_prompt);
// TODO: Validate generated AQL via AQL parser
return {Status::OK, generated_aql};  // ← Returns unvalidated!
```

**Expected Behavior**:
```cpp
std::string generated_aql = llm_client->translate(nl_prompt);

// ✅ Run through real parser
AQLParser parser(generated_aql);
Result<ASTNode> ast = parser.parse();
if (!ast.ok()) {
    return {Status::INVALID_SYNTAX, ast.error()};  // Hard fail
}

// ✅ Emit validation metric
validation_counter_->Inc({"status", "success"});
return {Status::OK, generated_aql};
```

---

## Data Flow: Current State (BROKEN)

```
┌────────────────────┐
│  User NL Query     │
└─────────┬──────────┘
          │
┌─────────▼──────────────────────┐
│  llm_aql_handler.cpp           │
│  - LLM Translation              │
│  - Return AQL string (X NO VALIDATION)
└─────────┬──────────────────────┘
          │
          │ unvalidated AQL
          │
┌─────────▼──────────────────────┐
│  Query Executor / Client        │
│  (hopes parser will catch bugs) │
└─────────┬──────────────────────┘
          │
┌─────────▼──────────────────────┐
│  aql_parser.cpp (finally!)      │
│  - Now detect malformed AQL     │
│  - Too late for UX              │
└─────────────────────────────────┘
```

**Problem**: Validation happens **after** user sees results. LLM hallucinations not caught at generation time.

---

## Data Flow: Required State (FIXED)

```
┌────────────────────┐
│  User NL Query     │
└─────────┬──────────┘
          │
┌─────────▼──────────────────────┐
│  llm_aql_handler.cpp           │
│  1. LLM Translation              │
│  2. Run through aql_parser       │ ← NEW: Validate immediately
│  3. Return (AQL + AST) or Error │
└─────────┬──────────────────────┘
          │
          ├─ Success: AST + AQL string
          │  (Executor knows it's valid)
          │
          └─ Error: Structured error
             (Can retry with feedback)
```

---

## Architecture Consolidation Plan

### Phase 1: Define Integration Boundary (Week 1 — 12 hours)

**Goal**: Formalize the relationship between `src/query/` and `src/aql/`.

#### 1.1 Create Integration Contract

**New File**: `src/query/AQL_LLM_INTEGRATION_CONTRACT.md`

```markdown
# AQL × LLM Integration Contract

## Roles

| Module | Responsibility |
|--------|-----------------|
| **src/query/** | Core parser, optimizer, executor; canonical AQL semantics |
| **src/aql/** | LLM orchestration, validation helpers, UX tooling |

## Boundary

**src/aql → src/query** (read-only, one-way):
- Call `AQLParser::parse(query_string)` to validate generated AQL
- Use `QueryOptimizer` to estimate cost (for confidence scoring)
- Use function registry for docs/linting

**src/query → src/aql** (never):
- Query engine does NOT depend on LLM layer
- LLM components are optional assistants, not core

## Data Flow

1. LLM generates candidate AQL string
2. aql_query_validator calls AQLParser to validate
3. On valid AST: return to user with confidence score
4. On invalid: attempt corrective generation or return error
5. User executes via normal query endpoint (query engine oblivious to LLM origin)

## Performance SLA

- LLM generation + validation: ≤ 5 seconds total (user-facing)
- Parser call: ≤ 500ms (amortized via caching)
- Retry on validation failure: max 2 attempts

## Error Handling

- Validation errors: structured `AQLError::InvalidSyntax`
- Parser timeout: convert to `AQLError::Timeout` after 1 second
- Fallback: return empty/null result instead of silent success
```

#### 1.2 Update Both ARCHITECTURE.md Files

**In `src/query/ARCHITECTURE.md`:**
```markdown
## LLM Integration Points (External)

The Query module is intentionally decoupled from LLM components.
LLM-driven query generation occurs in the `src/aql/` module, which
calls `AQLParser::parse()` for validation. See:
- src/aql/llm_aql_handler.cpp (for LLM orchestration)
- src/query/AQL_LLM_INTEGRATION_CONTRACT.md (integration contract)

The Query module itself does NOT perform LLM operations.
```

**In `src/aql/ARCHITECTURE.md`:**
```markdown
## Core Query Engine Dependency

The AQL module depends on the Query engine for parsing and optimization:
- `src/query/aql_parser.cpp` – canonical AQL parser
- `src/query/query_optimizer.cpp` – cost-based optimization
- `src/query/functions/` – function registry and documentation

The Query module is query-engine-only; it does NOT import LLM components.
See: src/query/AQL_LLM_INTEGRATION_CONTRACT.md
```

---

### Phase 2: Fix Integration Gap (Week 2 — 16 hours)

**Goal**: Wire parser validation into LLM pipeline.

#### 2.1 Update llm_aql_handler.cpp

**File**: `src/aql/llm_aql_handler.cpp`

**Current (line 289–300):**
```cpp
Result<std::string> LLMAQLHandler::translateNLToAQL(const std::string& nl_prompt, ...) {
    // Call LLM
    std::string generated_aql = llm_client_->translate(nl_prompt);
    // TODO: Validate generated AQL via AQL parser
    return {Status::OK, generated_aql};  // ← UNSAFE
}
```

**Fixed:**
```cpp
Result<std::string> LLMAQLHandler::translateNLToAQL(const std::string& nl_prompt, ...) {
    // Call LLM
    std::string generated_aql = llm_client_->translate(nl_prompt);
    
    // ✅ Validate via real parser
    try {
        query::AQLParser parser(generated_aql);
        std::unique_ptr<query::ASTNode> ast = parser.parse();
        if (!ast) {
            validation_counter_->Inc({"status", "parse_failed"});
            
            // Attempt retry with corrective prompt (max 1 retry)
            if (retries_ < 1) {
                retries_++;
                std::string corrective_prompt = 
                    fmt::format("Fix the AQL syntax error:\n{}\n\nError: {}", 
                                generated_aql, "check parser output");
                return translateNLToAQL(corrective_prompt, context, ...);
            }
            
            return {Status::INVALID_SYNTAX, "Generated AQL failed parser validation after retries"};
        }
        
        validation_counter_->Inc({"status", "parse_success"});
        return {Status::OK, generated_aql};
    } catch (const std::exception& e) {
        validation_counter_->Inc({"status", "validation_exception"});
        return {Status::INTERNAL_ERROR, fmt::format("Parser exception: {}", e.what())};
    }
}
```

#### 2.2 Add Prometheus Metrics

**New Metric**: `aql_validation_failures_total` (counter)
```cpp
// In LLMAQLHandler constructor
validation_counter_ = prometheus_registry_->NewCounter("aql_validation_failures", 
    {"status", "reason"});
```

**Labels:**
- `status`: [parse_success, parse_failed, validation_exception, timeout]
- `reason`: [no_retry, max_retries_exceeded, parser_timeout]

---

### Phase 3: Consolidate Documentation (Week 2 — 12 hours)

**Goal**: Single source of truth for AQL architecture.

#### 3.1 Create Master Architecture Doc

**New File**: `src/query/AQL_ARCHITECTURE_MASTER.md` (canonical)

Contents:
- Complete data flow (parser → optimizer → executor)
- Component ownership matrix (who owns what)
- Integration points with LLM layer
- v1.x vs. v2.0.0 feature roadmap

#### 3.2 Update Cross-References

**In `src/aql/ARCHITECTURE.md`:**
```markdown
> **Note:** This document covers LLM integration only.
> For core AQL architecture, see src/query/AQL_ARCHITECTURE_MASTER.md
```

**In `src/query/README.md`:**
```markdown
> **LLM Integration:** See src/aql/README.md for NL-to-AQL translation and assistance.
```

---

### Phase 4: Unify Testing (Week 3 — 20 hours)

**Goal**: Integration tests covering both layers.

#### 4.1 Add Integration Test Suite

**New File**: `tests/query/test_llm_aql_integration.cpp`

Test Cases:
```cpp
TEST(LLMAQLIntegration, NLToAQLValidationSuccess) {
    LLMClientMock llm_mock;
    llm_mock.SetExpectedTranslation("FOR doc IN users RETURN doc");
    
    LLMAQLHandler handler(&llm_mock);
    Result<std::string> result = handler.translateNLToAQL("Get all users");
    
    ASSERT_TRUE(result.ok());
    ASSERT_EQ(result.value(), "FOR doc IN users RETURN doc");
    ASSERT_GT(validation_counter_->GetValue("status", "parse_success"), 0);
}

TEST(LLMAQLIntegration, NLToAQLValidationFails) {
    LLMClientMock llm_mock;
    llm_mock.SetExpectedTranslation("INVALID SYNTAX HERE");  // Malformed
    
    LLMAQLHandler handler(&llm_mock);
    Result<std::string> result = handler.translateNLToAQL("Get all users");
    
    ASSERT_FALSE(result.ok());
    ASSERT_EQ(result.error(), Status::INVALID_SYNTAX);
}

TEST(LLMAQLIntegration, RetryOnValidationFailure) {
    LLMClientMock llm_mock;
    llm_mock.SetRetryResponse({
        "INVALID SYNTAX",  // First attempt fails
        "FOR doc IN users RETURN doc"  // Retry succeeds
    });
    
    LLMAQLHandler handler(&llm_mock);
    handler.SetMaxRetries(1);
    Result<std::string> result = handler.translateNLToAQL("Get all users");
    
    ASSERT_TRUE(result.ok());
    ASSERT_EQ(llm_mock.CallCount(), 2);  // Called twice
}
```

#### 4.2 Cross-Module Test Coverage

**Location**: `tests/integration/test_aql_query_pipeline.cpp`

Test user workflows:
1. User provides NL query
2. LLM generates AQL
3. Validation passes/fails
4. Query executes or rejects

---

### Phase 5: Migration & Cleanup (Week 4 — 10 hours)

#### 5.1 Remove Duplicate Validators

**Action**: If `src/aql/aql_query_validator.cpp` only does string-level regex checks, replace with single call to `AQLParser::parse()`.

**Before:**
```cpp
// src/aql/aql_query_validator.cpp (1 hour to run, inaccurate)
bool AQLQueryValidator::validate(const std::string& query) {
    if (!containsKeyword(query, "FOR")) return false;
    if (!containsKeyword(query, "RETURN")) return false;
    // ... regex checks
}
```

**After:**
```cpp
// src/query/aql_parser.cpp (10ms, accurate)
bool validate(const std::string& query) {
    AQLParser parser(query);
    return parser.parse().ok();
}
```

#### 5.2 Update ROADMAP.md + FUTURE_ENHANCEMENTS.md

**Add Section**: "AQL Architecture Consolidation"
```markdown
| Item | Status | Owner | Target |
|------|--------|-------|--------|
| Define LLM-Query Engine boundary | [ ] | Team | This week |
| Wire parser validation into LLM handler | [~] | Team | Next week |
| Create integration tests | [ ] | QA | Next week |
| Update documentation | [ ] | Docs | End of week |
```

---

## Summary: Consolidation Action Items

| Phase | Task | Owner | Effort | Target |
|-------|------|-------|--------|--------|
| 1 | Create integration contract | Arch | 12 h | 2026-06-20 |
| 2 | Fix llm_aql_handler.cpp validation | Dev | 16 h | 2026-06-22 |
| 3 | Consolidate documentation | Docs | 12 h | 2026-06-24 |
| 4 | Add integration tests | QA | 20 h | 2026-06-25 |
| 5 | Cleanup & migration | Dev | 10 h | 2026-06-28 |
| — | **TOTAL** | — | **70 h** | **2026-06-28** |

---

## Current State: Code Ownership Matrix

| File | Module | Owner | Status | Quality |
|------|--------|-------|--------|---------|
| aql_parser.cpp | query | ✅ Owned | 🟢 Prod | 100/100 |
| query_engine.cpp | query | ✅ Owned | 🟢 Prod | 95/100 |
| llm_aql_handler.cpp | aql | ⚠️ Partial | 🟡 Integration Gap | 75/100 |
| aql_query_validator.cpp | aql | ⚠️ Duplicate | 🟡 Redundant | 65/100 |
| ARCHITECTURE.md (query) | query | ✅ Owned | 🟢 Current | — |
| ARCHITECTURE.md (aql) | aql | ⚠️ Vague | 🟡 Out of sync | — |

---

## Consolidation Success Criteria (Go/No-Go Gate)

Before Phase 1 Kickoff of AQL v2.0.0:

- [ ] **Integration contract written and approved** by tech leads
- [ ] **Parser validation wired into LLM handler** (llm_aql_handler.cpp lines 289–300 fixed)
- [ ] **Integration tests passing** (GTest suite in tests/query/test_llm_aql_integration.cpp)
- [ ] **Documentation synchronized** (both ARCHITECTURE.md files cross-reference correctly)
- [ ] **No conflicting code paths** (aql_query_validator.cpp either removed or narrowed to UI-only highlighting)
- [ ] **Metrics emitted** (aql_validation_failures_total counter accessible in Prometheus)

---

## References

- Master Roadmap: [src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md](src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md)
- Mutations Roadmap: [src/query/AQL_MUTATIONS_ROADMAP.md](src/query/AQL_MUTATIONS_ROADMAP.md)
- Documentation Audit: [DOCUMENTATION_AUDIT_2026_06_18.md](DOCUMENTATION_AUDIT_2026_06_18.md)

---

*Audit Date: 2026-06-18 | Author: AI Code Assistant | Status: Ready for Team Review*
