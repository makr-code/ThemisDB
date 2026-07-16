# AQL ↔ LLM Integration Contract

**Version:** 1.0  
**Date:** 2026-06-18  
**Status:** 🔴 DRAFT (awaiting team review)  
**Deadline:** Phase 0 completion 2026-06-28  

---

## §1 Executive Summary

This document formalizes the **boundary, responsibilities, and communication protocol** between:
- **Layer 1: src/query/** (AQL Query Engine) — canonical parser, optimizer, executor
- **Layer 2: src/aql/** (LLM Orchestration) — NL-to-AQL translation, confidence scoring, conversation management

**Key Principle:** ONE-WAY DEPENDENCY: `src/aql → src/query` (never reverse)

---

## §2 Layer Responsibilities

### §2.1 src/query/ — Query Engine Layer

**Owned Files (50+):**
- `aql_parser.cpp/h` — Tokenizer, recursive-descent parser, AST construction
- `query_engine.cpp/h` — Query optimization, execution planning
- `let_evaluator.cpp/h` — Expression evaluation (100+ AQL functions)
- `sql_parser.cpp/h` — SQL dialect compatibility (reference for AQL mutations)
- `aql_runner.cpp/h` — Multi-statement transaction execution

**Responsibilities:**
1. ✅ **Parse AQL strings into validated AST nodes**
   - Input: `std::string aql_query`
   - Output: `Result<ASTNode>` or error with diagnostics
   - Must detect ALL syntax errors (keywords, operators, structure)
   - Must reject invalid token sequences

2. ✅ **Execute parsed queries against storage**
   - Input: `ASTNode + execution context`
   - Output: `Result<QueryResult>` with data + metadata

3. ✅ **Report errors with diagnostic information**
   - Line numbers, column positions
   - Token context (what was found, what was expected)
   - Suggestions for common mistakes

**APIs Exposed (for src/aql/ consumption):**

```cpp
namespace themis::query {

/// @brief Parse AQL query string into AST; validate syntax
class AQLParser {
public:
    /// Parse AQL string into AST
    Result<std::unique_ptr<ASTNode>> parse(const std::string& aql_query);
    
    /// Get diagnostics from last parse attempt
    const ParserDiagnostics& diagnostics() const;
};

/// @brief Parse result with diagnostic information
struct ParseResult {
    bool success;
    std::unique_ptr<ASTNode> ast;
    ParserDiagnostics diagnostics;  // Line, column, message, suggestions
};

/// @brief Diagnostic info for parser errors
struct ParserDiagnostics {
    uint32_t line_number;
    uint32_t column_number;
    std::string error_message;
    std::string error_context;  // Snippet of offending line
    std::vector<std::string> suggestions;
};

} // themis::query
```

---

### §2.2 src/aql/ — LLM Integration Layer

**Owned Files (31):**
- `llm_aql_handler.cpp/h` — Main orchestration (NL → LLM → AQL)
- `aql_query_validator.cpp/h` — String-level validation (keyword checks, patterns)
- `aql_confidence_scorer.cpp/h` — Quality scoring
- `aql_syntax_highlighter.cpp/h` — Syntax highlighting + token classification
- `aql_conversation_context.cpp/h` — Multi-turn conversation state

**Responsibilities:**
1. ✅ **Generate AQL from natural language via LLM**
   - Input: `nl_query + schema_context`
   - Output: `Result<AQLTranslationResult>` (AQL string + confidence)
   - Must handle LLM failures, timeouts, invalid responses

2. ✅ **Validate generated AQL against parser**
   - **NEW CONTRACT**: Call `AQLParser::parse()` after LLM generation
   - **NEW CONTRACT**: Retry with corrective feedback on parse failure
   - **NEW CONTRACT**: Return structured error when parse fails after retries

3. ✅ **Manage conversation state for iterative refinement**
   - Store turn history (NL query → AQL result)
   - Provide context to LLM for follow-up questions

4. ✅ **Score quality + confidence of generated queries**
   - Use cost estimates from query_engine when available
   - Provide metrics: confidence_score, execution_cost_estimate, etc.

**APIs Exposed (public):**

```cpp
namespace themis::aql {

/// @brief Result of NL-to-AQL translation
struct AQLTranslationResult {
    bool success;
    std::string aql_query;           // The generated AQL
    double confidence_score;          // 0.0 to 1.0
    std::string diagnostic_message;   // Error or info
    query::ParserDiagnostics parse_diagnostics;  // Parser validation info
};

/// @brief Main handler for NL-to-AQL translation
class LLMAQLHandler {
public:
    /// Translate NL to AQL with full validation pipeline
    /// - Calls LLM
    /// - Validates via parser (NEW)
    /// - Retries on parse failure (NEW)
    /// - Returns AQL only if parser succeeds
    Result<AQLTranslationResult> translateNLToAQL(
        const std::string& nl_query,
        const std::string& schema_context
    );
};

} // themis::aql
```

---

## §3 Data Flow Contract

### §3.1 Current State (BROKEN) ❌

```
┌─────────────────┐
│  User NL Query  │
└────────┬────────┘
         │
         ▼
┌─────────────────────────────────────┐
│ LLM Translation                     │
│ (llm_aql_handler.translateNLToAQL) │
└────────┬────────────────────────────┘
         │
    ┌────▼────┐
    │ String  │
    │ Regex   │
    │ Checks  │
    └────┬────┘
         │
    ┌────▼──────────────────────────┐
    │ Return unvalidated AQL to User │◄─── 🔴 NO PARSER VALIDATION
    └───────────────────────────────┘
         │
         ▼
┌─────────────────┐
│  User executes  │
│  invalid AQL    │
└────────┬────────┘
         │
         ▼
    ❌ PARSER FAILS (too late for correction)
```

**Problems:**
- String-level validation (regex) cannot detect syntax errors
- No AST construction = no semantic validation
- User receives unvalidated AQL
- Errors discovered only at execution time
- No opportunity for LLM retry with feedback

### §3.2 New State (FIXED) ✅

```
┌─────────────────┐
│  User NL Query  │
└────────┬────────┘
         │
         ▼
┌─────────────────────────────────────┐
│ LLM Translation                     │
│ (llm_aql_handler.translateNLToAQL) │
└────────┬────────────────────────────┘
         │
    ┌────▼──────────────────┐
    │ Generate AQL (raw)    │
    └────┬───────────────────┘
         │
    ┌────▼──────────────────────────────┐
    │ Parse with src/query/AQLParser    │◄─── 🟢 CANONICAL VALIDATION
    │ (NEW: direct call to parser)      │
    └────┬────────────────────────────┬─┘
         │                            │
    ┌────▼─────┐            ┌────────▼────┐
    │ Success   │            │ Parse Error │
    └────┬─────┘            └────┬────────┘
         │                       │
         │        ┌──────────────┘
         │        │
         │    ┌───▼──────────────────┐
         │    │ Retry Decision       │
         │    │ max_retries reached? │
         │    └───┬──────────────┬───┘
         │        │              │
         │   ┌────▼──┐    ┌──────▼──┐
         │   │ Retry │    │  Reject │
         │   │ w/FB  │    │  Error  │
         │   └───┬───┘    └──────┬──┘
         │       │              │
         └───────┼──────────────┘
                 │
         ┌───────▼────────────────────┐
         │ Return validated AQL        │
         │ (only if parse succeeds)   │
         └───────┬────────────────────┘
                 │
                 ▼
         ┌──────────────────┐
         │ User can execute │
         │ with confidence  │
         └──────────────────┘
```

**Benefits:**
- Canonical parser validates ALL syntax
- Immediate feedback to LLM on errors
- Retry with corrective prompt (e.g., "Fixed: removed stray FILTER")
- User receives ONLY validated AQL
- End-to-end: NL → validated AQL → execution

---

## §4 Integration Points (C++ Interfaces)

### §4.1 Parser Service Interface (src/query/)

**Location:** `include/query/aql_parser_service.h` (NEW file)

```cpp
namespace themis::query {

/// @brief AQL parser service interface
/// Abstracts away parser implementation details for consumer layers
class AQLParserService {
public:
    virtual ~AQLParserService() = default;
    
    /// @brief Parse and validate AQL query string
    /// @param aql_query The AQL query to parse
    /// @return ParseResult with AST on success or diagnostics on failure
    virtual ParseResult parse(const std::string& aql_query) = 0;
    
    /// @brief Get parser version/capabilities
    /// @return Version string (e.g., "1.0-mutations")
    virtual std::string version() const = 0;
};

/// @brief Concrete implementation of AQL parser service
class AQLParserServiceImpl : public AQLParserService {
public:
    ParseResult parse(const std::string& aql_query) override;
    std::string version() const override;
    
private:
    AQLParser parser_;
};

} // themis::query
```

### §4.2 LLM Validation Pipeline (src/aql/)

**Location:** `include/aql/llm_validation_pipeline.h` (NEW file)

```cpp
namespace themis::aql {

/// @brief Validation result from pipeline
struct LLMValidationResult {
    enum class Status {
        SUCCESS,           ///< AQL is valid
        PARSE_ERROR,       ///< Parser detected syntax error
        RETRYABLE,         ///< Error, but LLM can retry
        EXHAUSTED_RETRIES, ///< All retries failed
        REJECTED,          ///< Validation mode is REJECT_ON_ERROR
    };
    
    Status status;
    std::string validated_aql;      // Only set if status == SUCCESS
    query::ParserDiagnostics diagnostics;
    std::string retry_feedback;     // For next LLM attempt
};

/// @brief LLM validation pipeline: generates AQL, validates, retries
/// Implements: NL → LLM → Parse → Retry (if needed) → validated AQL
class LLMValidationPipeline {
public:
    /// @param parser_service Parser interface from src/query
    /// @param llm_client LLM client for generation
    /// @param max_retries Max retry attempts on parse failure
    LLMValidationPipeline(
        std::shared_ptr<query::AQLParserService> parser_service,
        std::shared_ptr<llm::LLMClient> llm_client,
        size_t max_retries = 1
    );
    
    /// @brief Execute full NL-to-validated-AQL pipeline
    /// @param nl_query Natural language query
    /// @param schema_context Collection/field constraints
    /// @return Validation result with validated AQL or error details
    LLMValidationResult execute(
        const std::string& nl_query,
        const std::string& schema_context
    );
    
private:
    std::shared_ptr<query::AQLParserService> parser_service_;
    std::shared_ptr<llm::LLMClient> llm_client_;
    size_t max_retries_;
};

} // themis::aql
```

### §4.3 Dependency Injection Pattern

**Location:** `src/aql/llm_aql_handler_factory.h` (NEW file)

```cpp
namespace themis::aql {

/// @brief Factory for creating LLM handler with proper dependency injection
class LLMAQLHandlerFactory {
public:
    /// @brief Create handler with required dependencies
    /// @param parser_service Parser from src/query/
    /// @param llm_client LLM inference client
    /// @param config Configuration options
    static std::unique_ptr<LLMAQLHandler> create(
        std::shared_ptr<query::AQLParserService> parser_service,
        std::shared_ptr<llm::LLMClient> llm_client,
        const LLMAQLHandlerConfig& config
    );
};

} // themis::aql
```

---

## §5 Error Handling & Diagnostics

### §5.1 Parser Error Classification

Parser must classify errors into semantic categories for LLM feedback:

```
CATEGORY              | FEEDBACK TO LLM
─────────────────────────────────────────────────────────
UNKNOWN_KEYWORD      │ "Line X: unknown keyword '{token}'. Valid keywords: FOR, LET, FILTER..."
INVALID_SYNTAX       │ "Line X: expected '{expected}' but found '{actual}'"
MISSING_CLAUSE       │ "Missing RETURN clause. AQL requires: FOR ... FILTER ... RETURN"
MALFORMED_FUNCTION   │ "Line X: function '{name}' expects N args, found M"
INVALID_COLLECTION   │ "Line X: collection '{name}' not in schema. Available: ..."
```

**Implementation:**
- Parser generates `ParserDiagnostics` with categorized error
- LLM handler extracts category + context
- LLM receives feedback: "Fix syntax error at line 2: missing comma in function call"

### §5.2 Retry Strategy

```cpp
struct RetryStrategy {
    size_t max_retries = 1;
    
    /// Feedback message for LLM on first retry
    std::function<std::string(const query::ParserDiagnostics&)> 
        feedback_generator;
    
    /// Should we retry for this error type?
    std::function<bool(const query::ParserDiagnostics&)> 
        is_retryable;
};
```

**Guidance:**
- RETRYABLE: Syntax errors (missing clause, typos in keywords)
- NOT RETRYABLE: Access violations, schema errors

---

## §6 Metrics & Observability

### §6.1 Prometheus Metrics (NEW)

```cpp
// Counter: parse validation outcomes
prometheus::Counter aql_validation_outcomes{
    "aql_validation_outcomes_total",
    "Total AQL validations by outcome",
    {"outcome"}  // success, parse_error, retry_exhausted, rejected
};

// Counter: retry attempts
prometheus::Counter aql_validation_retries{
    "aql_validation_retries_total",
    "Total retry attempts for AQL validation",
    {"retry_count"}  // 1, 2, 3+
};

// Gauge: avg confidence after validation
prometheus::Gauge aql_validation_confidence{
    "aql_validation_confidence",
    "Average confidence score after validation",
    {"model"}
};
```

### §6.2 Logging

```cpp
// Before parse (debug level)
"NL query: '{}' → LLM generated: '{}'", nl_query, generated_aql

// Parse success (debug level)
"AQL validation succeeded on attempt {}/{}", attempt, max_retries

// Parse failure with retry (warn level)
"AQL validation failed: {} (attempt {}/{}); retrying with feedback", 
    error_message, attempt, max_retries

// Parse failure final (error level)
"AQL validation exhausted all retries ({}). Last error: {}", 
    max_retries, final_error
```

---

## §7 Implementation Phases

### Phase 0.1: Parser Service Interface (12 hours)

**Deliverable:** Abstract interface `AQLParserService` + impl

**Tasks:**
- [ ] Create `include/query/aql_parser_service.h` with abstract interface
- [ ] Create `src/query/aql_parser_service.cpp` with `AQLParserServiceImpl`
- [ ] Wrap existing `AQLParser` to expose `ParseResult` struct
- [ ] Add `ParserDiagnostics` struct with line/column/message/suggestions
- [ ] Unit tests: test_aql_parser_service.cpp (10 tests: success, error cases)

**Success Criteria:**
- ✅ Interface compiles without src/aql/ dependency
- ✅ 10 tests pass (success paths + error paths)
- ✅ Error diagnostics include line numbers and suggestions

---

### Phase 0.2: LLM Validation Pipeline (16 hours)

**Deliverable:** `LLMValidationPipeline` class with retry logic

**Tasks:**
- [ ] Create `include/aql/llm_validation_pipeline.h` with interface + result struct
- [ ] Create `src/aql/llm_validation_pipeline.cpp` with retry logic
- [ ] Implement feedback generator (error → corrective prompt)
- [ ] Add Prometheus metrics: validation_outcomes, retries, confidence
- [ ] Unit tests: test_llm_validation_pipeline.cpp (15 tests: success, parse errors, retries, exhausted)

**Success Criteria:**
- ✅ Retries work: parse fails → LLM retries with feedback → succeeds
- ✅ Max retries enforced: after N failures, return error
- ✅ Metrics emitted correctly
- ✅ 15 tests pass (all retry scenarios)

---

### Phase 0.3: Refactor llm_aql_handler.cpp (12 hours)

**Deliverable:** Wire `LLMValidationPipeline` into `translateNLToAQL()`

**Tasks:**
- [ ] Inject `AQLParserService` via constructor dependency
- [ ] Replace string-level validation with `LLMValidationPipeline`
- [ ] Update `translateNLToAQL()` to use pipeline
- [ ] Remove redundant `AQLQueryValidator` calls (or narrow to UI-only)
- [ ] Update configuration: add `max_parse_retries` option
- [ ] Regression tests: existing AQL tests still pass

**Success Criteria:**
- ✅ No more unvalidated AQL returns
- ✅ Parse errors trigger retry logic
- ✅ Existing tests still pass
- ✅ No circular dependencies (src/aql → src/query only)

---

### Phase 0.4: Integration Tests (20 hours)

**Deliverable:** `tests/aql/test_llm_aql_integration.cpp` (30+ tests)

**Test Scenarios:**
1. NL query → valid AQL (success)
2. NL query → invalid AQL → LLM retries → valid AQL
3. NL query → invalid AQL → retries exhausted → error
4. Parallel requests with different validation modes
5. Schema validation (collection scope checks)
6. Concurrent LLM calls with queue management
7. Timeout handling during LLM retry

**Success Criteria:**
- ✅ 30+ tests pass
- ✅ Coverage: happy path, error cases, retry logic, metrics
- ✅ No flaky tests (concurrency handled)
- ✅ End-to-end: NL → validated AQL

---

### Phase 0.5: Documentation & Cross-References (10 hours)

**Deliverable:** Architecture docs + cross-references

**Tasks:**
- [ ] Update `src/query/ARCHITECTURE.md`: mention LLM integration contract
- [ ] Update `src/aql/ARCHITECTURE.md`: link to query/ + integration contract
- [ ] Add `src/query/README.md` section: "Parser Service for External Consumers"
- [ ] Add `src/aql/README.md` section: "Validation Pipeline Architecture"
- [ ] Inline code documentation: Doxygen comments for all new classes
- [ ] Update root-level `ARCHITECTURE.md`: clarify src/query ↔ src/aql relationship

**Success Criteria:**
- ✅ Cross-references bi-directional
- ✅ Public APIs fully documented
- ✅ Data flow diagram clear to new readers
- ✅ Dependency chain explicit (no cycles)

---

## §8 Success Criteria & Go/No-Go Gates

### Phase 0 Complete When:

- ✅ All 5 phases complete with tests passing
- ✅ No parser validation bypasses remain
- ✅ LLM handler validates all generated AQL
- ✅ Integration tests comprehensive (30+ cases)
- ✅ Metrics confirm validation pipeline active
- ✅ Documentation reflects new architecture
- ✅ Circular dependencies eliminated: src/aql → src/query only
- ✅ Code review approval from both modules

### v2.0.0 Phase 1 Kickoff Blocked Until:

- ✅ Phase 0 complete + merged to `develop`
- ✅ Integration contract approved by team leads
- ✅ All unit tests passing (CI green)
- ✅ Performance baseline established (parser latency < 5ms p99)

---

## §9 Dependencies & Risks

### External Dependencies:
- `src/query/aql_parser.cpp` — must expose ParseResult interface
- `src/query/query_engine.cpp` — should provide cost estimates (for confidence scoring)
- `llm_client` — must be injectable

### Risks:
1. **Parsing latency**: Parser called for every LLM output; if slow, timeout issues
   - Mitigation: Profile; target < 5ms p99; cache common patterns
2. **LLM retry loops**: If LLM keeps generating same errors, infinite retry
   - Mitigation: Max retries enforced; exponential backoff
3. **Backward compatibility**: Existing code using raw `translateNLToAQL()` may break
   - Mitigation: API versioning (LLM_AQL_HANDLER_API_VERSION); deprecation notice

---

## §10 Approval & Signoff

| Role | Name | Date | Status |
|------|------|------|--------|
| Query Engine Lead | — | — | 🔄 Pending |
| LLM Integration Lead | — | — | 🔄 Pending |
| Architecture Lead | — | — | 🔄 Pending |
| Security Review | — | — | 🔄 Pending |

---

## References

- [AQL_CONSOLIDATION_AUDIT_2026_06_18.md](../../AQL_CONSOLIDATION_AUDIT_2026_06_18.md) — Architecture audit
- [AQL_MUTATIONS_ROADMAP.md](./AQL_MUTATIONS_ROADMAP.md) — Feature roadmap (blocked until Phase 0 complete)
- [AQL_V2_0_0_COMPLETE_ROADMAP.md](./AQL_V2_0_0_COMPLETE_ROADMAP.md) — Master v2.0.0 roadmap
