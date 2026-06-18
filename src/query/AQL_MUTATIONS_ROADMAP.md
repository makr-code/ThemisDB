# AQL Mutations Implementation Roadmap

**Status:** [~] In Planning  
**Target Release:** v2.0.0 (Q3 2026)  
**Owner:** query module  
**Last Updated:** 2026-06-18

---

## Executive Summary

Implement full DML (Data Manipulation Language) support in AQL to enable **INSERT, UPDATE, REPLACE, REMOVE, UPSERT** statements alongside existing read-only queries (FOR/FILTER/RETURN). This enables AQL to become a **complete data manipulation language** for ThemisDB, replacing the need for separate RPC mutation calls.

**Scope:**
- Standalone mutation statements (e.g., `INSERT INTO collection VALUES {...}`)
- Mutations in transaction blocks (e.g., `BEGIN INSERT...; UPDATE...; COMMIT;`)
- ACID semantics with BEGIN/COMMIT/ROLLBACK
- Deterministic error handling and validation
- Full backward compatibility with read-only AQL

**Non-Scope (Phase 2+):**
- Advanced UPDATE/DELETE predicates with subqueries
- Bulk mutations with streaming/batching
- Multi-collection mutations
- Time-travel/versioning for mutations

---

## Overview

### Current State (v1.3.0)

| Feature | Status | Evidence |
|---------|--------|----------|
| Read-only queries (FOR/FILTER/RETURN) | ✅ Production | Parser + Translator + Engine complete |
| Transaction blocks (BEGIN/COMMIT/ROLLBACK) | ✅ Implemented (2026-06-18) | Semicolon support + semantic validation |
| Mutations (INSERT/UPDATE/DELETE/etc.) | ❌ Not Implemented | Keywords not in TokenType enum; no AST nodes; no translator paths |
| Safety validator | ✅ Blocks mutations | aql_safety_validator.cpp detects mutation keywords |

### Target State (v2.0.0)

| Feature | Status | Target Phase |
|---------|--------|--------------|
| INSERT statements | 🔄 Phase 1-3 | Fully functional with validation |
| UPDATE statements | 🔄 Phase 1-3 | Partial (simple predicates) |
| REPLACE statements | 🔄 Phase 1-3 | Fully functional |
| REMOVE/DELETE statements | 🔄 Phase 1-3 | Fully functional |
| UPSERT statements | 🔄 Phase 1-3 | Conditional logic |
| Transaction mutations | 🔄 Phase 4 | BEGIN...COMMIT with multi-statement batching |
| Safety validator bypass | 🔄 Phase 1 | New flag: `enforce_mutations_allowed=true` |
| Deterministic error semantics | 🔄 Phase 3-4 | Per-mutation error codes + recovery |

---

## Architecture Overview

### Current Architecture (Read-Only)

```
AQL String Input
    ↓
Tokenizer (aql_parser.cpp)
    ↓
Parser (recursive descent) → Query AST
    ↓
AQL Translator (aql_translator.cpp) → Execution Plan
    ↓
Query Engine (query_engine.cpp) → Storage Layer (RocksDB)
    ↓
Result Set (JSON)
```

### New Architecture (With Mutations)

```
AQL String Input
    ↓
Tokenizer (enhanced) → Mutation TokenTypes
    ↓
Parser (enhanced) → Query AST + MutationNode
    ↓
Safety Validator (enhanced) → Check enforce_mutations_allowed
    ↓
Translator (enhanced) → Mutation Execution Plan
    ↓
Transaction Manager (if BEGIN/COMMIT)
    ↓
Mutation Executor
    ├─ Lock collection
    ├─ Validate predicates
    ├─ Apply changes to RocksDB
    ├─ Update indexes
    ├─ Log transaction
    └─ Commit/Rollback
    ↓
Result Summary (affected_count, inserted_ids, errors)
```

### New Affected Files

| Layer | File | Changes |
|-------|------|---------|
| **Tokenizer** | `src/query/aql_parser.cpp` | Add INSERT/UPDATE/REMOVE/REPLACE/UPSERT/DELETE tokens |
| **AST** | `include/query/aql_parser.h` | MutationNode, InsertNode, UpdateNode, RemoveNode, ReplaceNode, UpsertNode |
| **Parser** | `src/query/aql_parser.cpp` | parseMutation(), parseInsert(), parseUpdate(), etc. |
| **Safety** | `src/query/aql_safety_validator.cpp` | Flag to allow/deny mutation keywords |
| **Translator** | `src/query/aql_translator.cpp` | translateMutation(), translateInsert(), etc. |
| **Engine** | `src/query/query_engine.cpp` | executeMutation(), lockCollection(), applyChanges() |
| **Executor** | `src/query/mutation_executor.cpp` | **NEW** – mutation-specific execution logic |
| **Transactions** | `src/query/aql_runner.cpp` | Multi-statement runner with mutations |
| **Tests** | `tests/aql/test_aql_mutations.cpp` | **NEW** – comprehensive mutation tests |

---

## Implementation Phases

### Phase 1: Parser & Tokenizer Enhancement

**Goal:** Tokenize and parse mutation statements into AST; no execution yet.

**Duration:** 2-3 weeks  
**Target:** Q3 2026 Week 1-2

#### 1.1 Tokenizer Enhancement

**Tasks:**
- [ ] Add mutation keywords to TokenType enum (Target: Q3 Week 1)
  - INSERT, UPDATE, DELETE, REMOVE, REPLACE, UPSERT
  - INTO, SET, WHERE, VALUES, ON (conflict resolution)
  
- [ ] Update Tokenizer::readIdentifierOrKeyword() (Target: Q3 Week 1)
  - Recognize "insert", "update", "delete", "remove", "replace", "upsert" as keywords
  - Add "into", "set", "values", "on" as contextual keywords

**File:** `src/query/aql_parser.cpp` (lines 37-80, 222-276)

**Example Token Stream:**
```
INSERT INTO users VALUES {name: "Alice", age: 30}
→ TokenType::INSERT, TokenType::INTO, TokenType::IDENTIFIER("users"),
  TokenType::VALUES, TokenType::LBRACE, ...
```

#### 1.2 AST Node Definitions

**Tasks:**
- [ ] Define MutationNode base structure in aql_parser.h (Target: Q3 Week 1)
  ```cpp
  struct MutationNode : public ASTNode {
      virtual ~MutationNode() = default;
      virtual ASTNodeType getType() const = 0;
      virtual nlohmann::json toJSON() const = 0;
  };
  ```

- [ ] Define specific mutation node types (Target: Q3 Week 1-2)
  - `InsertNode`: collection, documents[], return_new (bool)
  - `UpdateNode`: collection, filter, update_spec, return_new/old, limit (optional)
  - `RemoveNode`: collection, filter, return_removed (bool), limit (optional)
  - `ReplaceNode`: collection, filter, replacement, return_new/old
  - `UpsertNode`: collection, condition, insert_doc, update_spec, return_new/old

**File:** `include/query/aql_parser.h` (new section after line 600)

**Example AST:**
```json
{
  "type": "INSERT",
  "collection": "users",
  "documents": [
    {
      "name": "Alice",
      "age": 30,
      "email": "alice@example.com"
    }
  ],
  "return_new": true
}
```

#### 1.3 Parser Methods

**Tasks:**
- [ ] Add Query::parseMutation() entry point (Target: Q3 Week 2)
  - Detect leading mutation keyword (INSERT/UPDATE/etc.)
  - Route to appropriate parseXxx() method

- [ ] Implement parseInsert() (Target: Q3 Week 2)
  - Syntax: `INSERT INTO collection VALUES {...} | doc_expr [RETURN NEW|OLD]`
  - Validate collection name
  - Parse document values (object literals or expressions)

- [ ] Implement parseUpdate() (Target: Q3 Week 2)
  - Syntax: `UPDATE collection SET key = value [, ...] WHERE filter [LIMIT count] [RETURN NEW|OLD]`
  - Parse SET clauses (field assignments)
  - Parse WHERE predicate

- [ ] Implement parseRemove() (Target: Q3 Week 2)
  - Syntax: `REMOVE doc_expr | FOR var IN collection FILTER... REMOVE var [RETURN removed] [LIMIT count]`
  - Support both expression-based and query-based removal

- [ ] Implement parseReplace() (Target: Q3 Week 2)
  - Syntax: `REPLACE collection WITH {...} [RETURN NEW|OLD]`
  - Fully replace document matching key

- [ ] Implement parseUpsert() (Target: Q3 Week 2)
  - Syntax: `UPSERT {_key: value} INSERT {...} UPDATE {...} [RETURN NEW|OLD]`
  - Conditional insert-or-update semantics

**File:** `src/query/aql_parser.cpp` (new methods after line 1400)

#### 1.4 Parser Integration Tests

**Tasks:**
- [ ] Create test_aql_mutations_parser.cpp (Target: Q3 Week 2)
  - Test parsing of each mutation type
  - Validate AST structure
  - Test error cases (invalid syntax, malformed predicates)

**File:** `tests/aql/test_aql_mutations_parser.cpp` (**NEW**)

**Example Test:**
```cpp
TEST(AQLMutationParser, ParseInsertBasic) {
  AQLParser parser;
  auto result = parser.parseMutation("INSERT INTO users VALUES {name: \"Alice\"}");
  ASSERT_TRUE(result);
  auto mutation = *result;
  ASSERT_EQ(mutation->getType(), ASTNodeType::INSERT);
  auto insert_node = std::dynamic_pointer_cast<InsertNode>(mutation);
  ASSERT_EQ(insert_node->collection, "users");
  ASSERT_EQ(insert_node->documents.size(), 1);
}
```

---

### Phase 2: Safety & Semantic Validation

**Goal:** Enhance safety validator and semantic analysis for mutations.

**Duration:** 1-2 weeks  
**Target:** Q3 2026 Week 2-3

#### 2.1 Safety Validator Enhancement

**Tasks:**
- [ ] Add `enforce_mutations_allowed` flag to AqlSafetyValidator (Target: Q3 Week 2)
  - If false: current behavior (block all mutations)
  - If true: allow mutations, but still check for injection patterns

- [ ] Implement predicate safety checks for UPDATE/REMOVE (Target: Q3 Week 2-3)
  - Prevent predicates that could delete entire collections
  - Validate LIMIT clauses to prevent accidental bulk deletes
  - Check for common injection patterns in WHERE clauses

**File:** `src/query/aql_safety_validator.cpp` (enhance validate() method)

#### 2.2 Semantic Validation

**Tasks:**
- [ ] Create AqlMutationValidator class (Target: Q3 Week 3)
  - Validate collection exists
  - Validate field names in SET/INSERT clauses
  - Validate _key uniqueness for REPLACE/UPSERT
  - Validate predicate structure (no complex subqueries yet)

- [ ] Implement collection schema validation (Target: Q3 Week 3)
  - Check if collection requires specific fields
  - Validate field types (if schema enforced)
  - Return detailed error messages per field

**File:** `src/query/aql_mutation_validator.cpp` (**NEW**)

#### 2.3 Semantic Validation Tests

**Tasks:**
- [ ] Create test_aql_mutations_validation.cpp (Target: Q3 Week 3)
  - Test collection existence checks
  - Test schema validation
  - Test error reporting

**File:** `tests/aql/test_aql_mutations_validation.cpp` (**NEW**)

---

### Phase 3: Translation & Execution Plan

**Goal:** Translate mutations to storage-layer operations; prepare execution plans.

**Duration:** 3-4 weeks  
**Target:** Q3 2026 Week 3-4 and into Week 1 (adjusted)

#### 3.1 Mutation Translator

**Tasks:**
- [ ] Create translateMutation() entry point in AqlTranslator (Target: Q3 Week 3)
  - Route mutation type to specific translator method
  - Return MutationExecutionPlan

- [ ] Implement translateInsert() (Target: Q3 Week 3-4)
  - Generate unique _key per document (if not provided)
  - Plan: serialize document → RocksDB put
  - Plan: update all secondary indexes
  - Generate metrics (inserted_count, inserted_ids)

- [ ] Implement translateUpdate() (Target: Q3 Week 4)
  - Translate filter predicate to key-value scans
  - Plan: fetch document → apply updates → serialize → RocksDB put
  - Plan: update affected indexes
  - Handle conditional updates (only update if predicate matches)

- [ ] Implement translateRemove() (Target: Q3 Week 4)
  - Translate filter to key scans
  - Plan: fetch document → serialize to tombstone → RocksDB delete
  - Plan: remove from all indexes
  - Generate affected_count

- [ ] Implement translateReplace() (Target: Q3 Week 4)
  - Similar to INSERT but require _key match
  - Plan: RocksDB put (overwrites existing)

- [ ] Implement translateUpsert() (Target: Q3 Week 4)
  - Generate two-branch plan: INSERT-path or UPDATE-path
  - Decide at runtime based on _key existence

**File:** `src/query/aql_translator.cpp` (new methods)

**Example Execution Plan:**
```json
{
  "mutation_type": "INSERT",
  "collection": "users",
  "steps": [
    {
      "step": "generate_keys",
      "count": 1,
      "id_field": "_key"
    },
    {
      "step": "serialize",
      "format": "json"
    },
    {
      "step": "rocksdb_put",
      "key_prefix": "users/",
      "ttl": null
    },
    {
      "step": "update_indexes",
      "indexes": ["idx_email", "idx_age"]
    }
  ],
  "estimated_latency_ms": 5,
  "locks_required": ["users_write"]
}
```

#### 3.2 Mutation Executor Framework

**Tasks:**
- [ ] Create MutationExecutor class (Target: Q3 Week 4)
  - Interface: execute(MutationExecutionPlan, context) → MutationResult
  - MutationResult: success (bool), affected_count, inserted_ids, errors[]

- [ ] Implement lock manager integration (Target: Q3 Week 4)
  - Acquire collection write lock before mutation
  - Release lock on success/failure
  - Handle lock timeout (return error)

- [ ] Implement transaction journal (Target: Q3 Week 4)
  - Log mutation operation to write-ahead log (WAL)
  - Prepare rollback plan

**File:** `src/query/mutation_executor.cpp` (**NEW**)

#### 3.3 RocksDB Integration

**Tasks:**
- [ ] Implement RocksDB write operations for mutations (Target: Q3 Week 4)
  - INSERT: WriteBatch::Put(key, value)
  - REMOVE: WriteBatch::Delete(key)
  - REPLACE: WriteBatch::Put(key, new_value)

- [ ] Implement index update logic (Target: Q3 Week 4)
  - For each affected index, compute old/new index keys
  - Delete old entries, insert new entries
  - Maintain index consistency

- [ ] Add transaction support to WriteBatch (Target: Q3 Week 4)
  - Atomic writes: all-or-nothing semantics
  - Rollback: reverse WriteBatch

**File:** `src/storage/rocksdb_wrapper.cpp` (enhance existing methods)

#### 3.4 Translation & Executor Tests

**Tasks:**
- [ ] Create test_aql_mutations_translator.cpp (Target: Q3 Week 4)
  - Test execution plan generation
  - Validate plan structure and correctness

- [ ] Create test_aql_mutations_executor.cpp (Target: Q3 Week 4)
  - Test actual mutation execution
  - Test lock management
  - Test transaction journal

**Files:** `tests/aql/test_aql_mutations_translator.cpp`, `test_aql_mutations_executor.cpp` (**NEW**)

---

### Phase 4: Transaction Support & Atomicity

**Goal:** Integrate mutations into BEGIN/COMMIT transaction blocks for atomic multi-statement batches.

**Duration:** 2-3 weeks  
**Target:** Q3 2026 Week 5-6 (adjusted)

#### 4.1 Multi-Statement Mutation Runner

**Tasks:**
- [ ] Enhance AqlRunner::executeTransactionBlock() (Target: Q3 Week 5)
  - Support mutations alongside queries
  - Collect all mutations in a batch
  - Plan: execute all statements in single transaction

- [ ] Implement batch executor (Target: Q3 Week 5)
  - Execute mutations in order
  - Maintain isolation: mutations don't see uncommitted changes
  - Accumulate results per statement

**File:** `src/query/aql_runner.cpp` (enhance transaction methods)

#### 4.2 Atomicity Guarantees

**Tasks:**
- [ ] Implement all-or-nothing semantics (Target: Q3 Week 5-6)
  - If any mutation fails: rollback all previous mutations
  - If any query fails: rollback all mutations
  - Error reporting: which statement failed + why

- [ ] Add savepoint support for nested transactions (Target: Q3 Week 6)
  - Optional: support nested BEGIN/COMMIT (Phase 2 feature)

**File:** `src/query/mutation_executor.cpp` (transaction orchestration)

#### 4.3 Error Recovery & Rollback

**Tasks:**
- [ ] Implement rollback executor (Target: Q3 Week 6)
  - Reverse each mutation using inverse operations
  - DELETE → re-INSERT original doc
  - UPDATE → restore original field values
  - REPLACE → restore original document

- [ ] Add transaction state machine (Target: Q3 Week 6)
  - States: PENDING → EXECUTING → COMMITTED | ABORTED
  - Idempotent rollback: safe to retry

**File:** `src/query/mutation_executor.cpp` (rollback logic)

#### 4.4 Integration Tests

**Tasks:**
- [ ] Create test_aql_mutations_transactions.cpp (Target: Q3 Week 6)
  - Test BEGIN...INSERT...UPDATE...COMMIT scenarios
  - Test rollback on error
  - Test partial failures
  - Test atomicity (no partial commits)

**File:** `tests/aql/test_aql_mutations_transactions.cpp` (**NEW**)

**Example Test:**
```cpp
TEST(AQLMutationTransaction, MultiStatementCommit) {
  AqlRunner runner(db);
  
  std::string aql = R"(
    BEGIN;
      INSERT INTO users VALUES {name: "Alice", age: 30};
      INSERT INTO users VALUES {name: "Bob", age: 25};
    COMMIT;
  )";
  
  auto result = runner.executeTransactionBlock(aql);
  ASSERT_TRUE(result);
  ASSERT_EQ(result->statements.size(), 2);
  ASSERT_EQ(result->action, AqlTransactionAction::Commit);
}

TEST(AQLMutationTransaction, RollbackOnError) {
  // INSERT → UPDATE (fails due to constraint) → should rollback INSERT
  std::string aql = R"(
    BEGIN;
      INSERT INTO users VALUES {email: "alice@example.com", name: "Alice"};
      UPDATE users SET email = "alice@example.com" WHERE name = "Bob"; -- Error: no match
    COMMIT;
  )";
  
  auto result = runner.executeTransactionBlock(aql);
  ASSERT_FALSE(result);  // Transaction failed
  // Verify INSERT was rolled back: alice@example.com not in DB
}
```

---

### Phase 5: Testing, Performance & Documentation

**Goal:** Comprehensive testing, benchmarking, and production documentation.

**Duration:** 2-3 weeks  
**Target:** Q3 2026 Week 7-8

#### 5.1 Unit & Integration Tests

**Tasks:**
- [ ] Expand mutation test coverage (Target: Q3 Week 7)
  - Edge cases: empty collections, duplicate keys, null values
  - Error cases: malformed predicates, missing fields, type violations
  - Performance tests: bulk insert/update/remove benchmarks

- [ ] Cross-feature tests (Target: Q3 Week 7)
  - Mutations + graph indexes
  - Mutations + vector indexes
  - Mutations + full-text indexes
  - Concurrent mutations (stress test)

**Files:** `tests/aql/test_aql_mutations_*.cpp`

#### 5.2 Performance Benchmarking

**Tasks:**
- [ ] Create mutation benchmark suite (Target: Q3 Week 7-8)
  - BM_InsertSingle, BM_InsertBatch(N=100, 1000, 10000)
  - BM_UpdateByPredicate, BM_RemoveByFilter
  - BM_TransactionBatch (BEGIN...multiple mutations...COMMIT)
  - BM_IndexUpdateOverhead

- [ ] Establish performance targets (Target: Q3 Week 8)
  - INSERT single doc: < 5ms (RocksDB baseline)
  - INSERT batch 1000: < 500ms
  - UPDATE/REMOVE by predicate: < 50ms (assuming indexed predicate)
  - Transaction overhead: < 2x non-transactional cost

**File:** `benchmarks/benchmark_aql_mutations.cpp` (**NEW**)

#### 5.3 Documentation

**Tasks:**
- [ ] Update AQL Syntax Guide (Target: Q3 Week 8)
  - Document INSERT/UPDATE/REPLACE/REMOVE/UPSERT syntax
  - Provide examples for each mutation type
  - Document error codes and recovery

- [ ] Create Mutation API Reference (Target: Q3 Week 8)
  - Parameter descriptions
  - Return value schema
  - Limitations and constraints

- [ ] Write Migration Guide (Target: Q3 Week 8)
  - How to migrate from RPC mutation calls to AQL mutations
  - Performance comparison table
  - Best practices for transaction batching

**Files:** `docs/de/aql/aql_mutations_guide.md`, `docs/de/aql/aql_mutations_reference.md` (**NEW**)

#### 5.4 API Documentation Updates

**Tasks:**
- [ ] Update C++ header documentation (Target: Q3 Week 8)
  - Doxygen comments for new MutationNode classes
  - Doxygen comments for AqlMutationValidator, MutationExecutor
  - Document error codes and semantics

**File:** `include/query/aql_parser.h`, `include/query/query_engine.h` (new methods)

#### 5.5 Final QA & Release Checklist

**Tasks:**
- [ ] Run full regression suite (Target: Q3 Week 8)
  - Existing read-only queries should not be affected
  - No performance regression in non-mutation paths

- [ ] Security audit (Target: Q3 Week 8)
  - Verify mutations cannot bypass collection-level permissions
  - Verify transaction isolation is enforced
  - Verify WAL prevents data loss

- [ ] Documentation audit (Target: Q3 Week 8)
  - All new APIs documented with Doxygen
  - All public methods have usage examples
  - No broken cross-references

---

## Production Readiness Checklist

### Parser & Tokenizer
- [ ] All mutation keywords tokenized correctly
- [ ] AST nodes serialize/deserialize correctly
- [ ] Parser handles all valid mutation syntaxes
- [ ] Parser rejects invalid syntaxes with clear errors
- [ ] No regression in existing read-only query parsing

### Safety & Validation
- [ ] Safety validator correctly gates mutation keywords
- [ ] enforce_mutations_allowed flag works as documented
- [ ] Semantic validator catches collection/field errors
- [ ] Error messages are actionable and specific

### Translation & Execution
- [ ] Execution plans generated correctly per mutation type
- [ ] Lock manager prevents concurrent writes to same collection
- [ ] RocksDB WriteBatch atomicity is guaranteed
- [ ] Index updates are consistent with data updates
- [ ] No data loss on system crash (WAL recovery verified)

### Transactions
- [ ] BEGIN...COMMIT preserves atomicity
- [ ] Rollback correctly reverses all mutations
- [ ] Partial failures trigger full transaction rollback
- [ ] Nested transactions supported (if planned)
- [ ] Transaction isolation levels documented and enforced

### Testing
- [ ] Unit test coverage >= 90% for new code
- [ ] Integration tests cover all mutation × transaction combinations
- [ ] Performance benchmarks meet targets
- [ ] Stress tests (concurrent mutations) pass
- [ ] Security audit passed (no injection vulnerabilities)

### Documentation
- [ ] AQL Syntax Guide updated with mutation examples
- [ ] API reference complete with return value schemas
- [ ] Migration guide covers RPC → AQL mutation translation
- [ ] Error codes documented with recovery steps
- [ ] Backward compatibility notes clear

---

## Known Issues & Limitations

### Phase 1-3 (MVP)

- **Complex Predicates:** UPDATE/REMOVE with complex WHERE clauses (OR, subqueries) deferred to Phase 1.5.
  - MVP supports: simple equality/comparison and AND combinations
  - Complex queries should decompose into multiple simpler mutations

- **Bulk Mutations:** Streaming/batching for high-volume inserts not yet optimized.
  - Workaround: BEGIN...INSERT...INSERT...INSERT...COMMIT for atomic batches
  - Phase 2 feature: async bulk insert API

- **Partial Indexes:** Mutations don't support partial index updates yet.
  - Workaround: rebuild index after bulk mutation
  - Phase 2 feature: incremental index maintenance

- **Multi-Collection Mutations:** Single transaction cannot mutate multiple collections atomically.
  - Workaround: separate transactions per collection
  - Phase 2 feature: distributed transaction coordinator

### Known Risks

| Risk | Mitigation |
|------|-----------|
| **Data Corruption** on crash during WriteBatch | WAL recovery + idempotent rollback |
| **Lock Deadlock** in concurrent mutations | Consistent lock ordering + timeout detection |
| **Index Inconsistency** after partial failure | Transaction coordinator ensures all-or-nothing |
| **Performance Regression** in read queries | Separate index structures for mutations (no shared latches) |

---

## Breaking Changes

### For End Users

- **Safe:** AQL mutations are additive; existing read-only queries unchanged
- **Safe:** Safety validator defaults to `enforce_mutations_allowed=false`, blocking mutations by default
- **Minor:** MCP tools/LLM agents using AQL must opt-in via `enforce_mutations_allowed=true` in config

### For API Consumers (C++)

- **New Type:** `MutationNode` base class added to AST hierarchy (non-breaking, polymorphic)
- **New Return Type:** `AQLParser::parse()` can now return either `Query` or `MutationNode` (requires type checking)
- **New Methods:** `MutationExecutor::execute()` et al. (additions only, no removals)

### Deprecations

- None planned; RPC mutation methods (`CreateDocument`, etc.) remain available but not required

---

## Timeline & Milestones

| Phase | Target | Duration | Deliverable |
|-------|--------|----------|-------------|
| 1: Parser | Q3 Week 1-2 | 2-3 weeks | Tokenizer + AST + Parser (no execution) |
| 2: Safety | Q3 Week 2-3 | 1-2 weeks | Validator + semantic checks |
| 3: Translation | Q3 Week 3-4 + | 3-4 weeks | Executor + RocksDB integration + tests |
| 4: Transactions | Q3 Week 5-6 + | 2-3 weeks | Multi-statement atomicity |
| 5: Testing & Docs | Q3 Week 7-8 | 2-3 weeks | Benchmarks + guides + QA |
| **Release** | Q3 2026 EOQ | — | v2.0.0-beta |

**Total Effort:** ~12-15 weeks (3+ calendar months, accounting for review/iteration cycles)

---

## Dependencies & Prerequisites

### Internal
- [ ] Transaction block support (BEGIN/COMMIT/ROLLBACK) — **Already implemented 2026-06-18**
- [ ] Collection-level write locking mechanism — **Needs implementation**
- [ ] Write-ahead log (WAL) for crash recovery — **Existing in RocksDB, needs integration**
- [ ] Index update pipeline — **Existing but needs enhancement for mutations**

### External
- [ ] C++20 concepts (RAII for lock guards) — Available
- [ ] RocksDB API knowledge — Team familiar
- [ ] Thread safety guidelines (this repo) — CLAUDE.md + copilot-instructions.md

---

## Acceptance Criteria

### Definition of Done (per Phase)

**Phase 1 Complete When:**
- All mutation keywords tokenize correctly
- Parser generates correct AST for all mutation syntaxes
- AST serializes to/from JSON correctly
- Parser tests pass (>= 90% coverage)
- No regression in read-only query parsing

**Phase 2 Complete When:**
- Safety validator enforces `enforce_mutations_allowed` flag
- Semantic validator catches all documented validation errors
- Validation tests pass with clear error messages
- No false positives in error detection

**Phase 3 Complete When:**
- Execution plans generated correctly per mutation type
- Mutations execute on RocksDB correctly
- Indexes updated consistently with data changes
- Transaction executor tests pass (>= 85% coverage)
- Single-statement mutations work end-to-end

**Phase 4 Complete When:**
- Multi-statement transaction blocks execute atomically
- Rollback correctly reverses all mutations
- Transaction tests pass (all combinations)
- Atomicity guarantee verified under failure scenarios

**Phase 5 Complete When:**
- All unit/integration/performance tests pass
- Documentation complete and reviewed
- Security audit passed
- Performance benchmarks meet targets
- Release notes prepared

---

## References

- **ArangoDB AQL Spec:** https://docs.arangodb.com/3.13/aql/fundamentals/data-manipulation/
- **ThemisDB Transaction Design:** `docs/de/concurrency/transaction_semantics.md`
- **ThemisDB Query Engine:** `docs/de/aql/aql_query_engine.md`
- **CLAUDE.md:** Repository-level coding guidelines (RAII, symbol-first refactoring, no stubs)
- **copilot-instructions.md:** Section 8 (C++ Best Practices)

---

## Contact & Questions

**Phase Owner:** query module team  
**Escalation:** architecture review board

For questions or blocking issues, file GitHub issue with `[AQL-Mutations]` prefix.
