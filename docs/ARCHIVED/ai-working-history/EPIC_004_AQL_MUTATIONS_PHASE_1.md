# EPIC-004: AQL 2.0.0 Mutations Phase 1 (Parser + AST Foundation)

**Status:** 🔵 Planned  
**Target Release:** Q4 2026 (2026-10-15 Phase 1 completion)  
**Timeline:** 12-15 Weeks (Target Release: Q4 2026)  
**Owner:** Query Language Team  
**Scope:** AQL Tokenizer + Parser + AST for DML (INSERT/UPDATE/DELETE/REPLACE/UPSERT/REMOVE)

## Epic Summary

Establish the foundation for AQL 2.0.0 complete language standard by implementing Phase 1: DML (Data Manipulation Language) support. Focus on parser extensions and AST nodes required for mutation statements, without initial executor implementation.

**Phase 1 Scope:**
- Tokenizer: 15+ DML tokens (INSERT, UPDATE, DELETE, REPLACE, UPSERT, REMOVE, SET, VALUES, etc.)
- Parser: INSERT, UPDATE, DELETE statement recognition
- AST: MutationStatementAST node hierarchy
- Semantic Validation: Basic mutation type validation
- Tests: 50+ unit tests for parser scenarios

**Future Phases (not in this epic):**
- Phase 2 (Q1 2027): Safety guarantees + transaction wrapping
- Phase 3 (Q1 2027): Executor implementation
- Phase 4 (Q2 2027): Transactions + rollback
- Phase 5+ (Q2-Q3 2027): DDL, Geospatial, FTS

## Success Criteria

- ✅ **AQL Tokenizer Extended:** 15+ DML tokens added, backward compatible
- ✅ **DML Parser Complete:** parseInsertStatement(), parseUpdateStatement(), parseDeleteStatement()
- ✅ **MutationStatementAST Ready:** Full node hierarchy, semantic validation
- ✅ **Test Coverage:** 50+ unit tests covering parser scenarios
- ✅ **Documentation Complete:** Examples, syntax guide, v2.0.0 roadmap
- ✅ **Zero Breaking Changes:** All AQL v1 queries continue to work unchanged
- ✅ **Evaluation Ready:** Semantic validation tests pass

## Key Deliverables

| Deliverable | Target | Verification |
|-------------|--------|--------------|
| DML Tokenizer | 15+ tokens | Token recognition tests (20 cases) |
| DML Parser | 3 statements | Parser tests (30 cases) |
| MutationStatementAST | Full hierarchy | AST generation tests (20 cases) |
| Semantic Validator | Basic rules | Validation tests (20 cases) |
| Documentation | v2.0.0 specs | AQL_MUTATIONS_ROADMAP.md updated |
| Backward Compatibility | 100% | v1 query regression tests (50+ cases) |

## Dependencies

- **Prerequisite:** AQL v1 parser stable (no major refactoring expected)
- **Reference:** src/query/sql_parser.cpp (parseInsert pattern exists)
- **Supports:** Future phases (Phase 2-5) and AQL 2.0.0 roadmap
- **Related:** Issue #5172 (Master gap issue), AQL_V2_0_0_COMPLETE_ROADMAP.md

## Sub-Issues

### [004-A] AQL Tokenizer DML Extension
- **Title:** `AQL: Tokenizer DML Token Support (INSERT, UPDATE, DELETE, REPLACE, UPSERT, REMOVE)`
- **Type:** Enhancement / Parser
- **Labels:** `aql-2.0.0`, `mutations`, `parser`, `phase-1`, `query-language`
- **Estimated Effort:** 12 hours
- **Tokens to Add:** INSERT, UPDATE, DELETE, REPLACE, UPSERT, REMOVE, SET, VALUES, ON, CONFLICT, DO, NOTHING, ACTION, EXCLUDED, etc. (15+ total)
- **Acceptance Criteria:**
  - 15+ DML keyword tokens added to aql_tokenizer.cpp
  - 20+ test cases for token recognition
  - Backward compatibility verified (v1 queries unchanged)
  - No tokenizer perf regression
  - Documentation of token list + examples

### [004-B] AQL Parser DML Statement Support
- **Title:** `AQL: Parser DML Statement Recognition (INSERT, UPDATE, DELETE)`
- **Type:** Enhancement / Parser
- **Labels:** `aql-2.0.0`, `mutations`, `parser`, `phase-1`
- **Estimated Effort:** 20 hours
- **Statements to Implement:**
  - parseInsertStatement() → INSERT INTO collection (fields...) VALUES (values...)
  - parseUpdateStatement() → UPDATE collection SET field=value WHERE condition
  - parseDeleteStatement() → DELETE FROM collection WHERE condition
- **Acceptance Criteria:**
  - 3 DML statement parsers fully implemented
  - 30+ parser test cases covering variants
  - Reference implementation (sql_parser.cpp patterns) adapted
  - Error handling for syntax errors
  - AST generation verified

### [004-C] Mutation AST & Semantic Validation
- **Title:** `AQL: MutationStatementAST & Semantic Validator`
- **Type:** Enhancement / Parser
- **Labels:** `aql-2.0.0`, `mutations`, `semantics`, `phase-1`
- **Estimated Effort:** 15 hours
- **AST Nodes to Define:**
  - InsertStatementAST (target collection, field list, value expressions)
  - UpdateStatementAST (target collection, set clauses, where predicate)
  - DeleteStatementAST (target collection, where predicate)
  - MutationSetClauseAST (field, expression, optional constraint)
- **Validation Rules:**
  - INSERT: field count matches value count
  - UPDATE: SET clause syntax valid
  - DELETE: WHERE clause optional but recommended
  - Duplicate field checks
  - Type consistency (later phase)
- **Acceptance Criteria:**
  - Full AST node hierarchy designed
  - 20+ semantic validation tests
  - Error messages actionable and clear
  - AST serialization/deserialization works

### [004-D] Phase 1 Integration & Documentation
- **Title:** `AQL Mutations Phase 1: Documentation & Integration`
- **Type:** Documentation / Integration
- **Labels:** `aql-2.0.0`, `mutations`, `documentation`, `phase-1`
- **Estimated Effort:** 8 hours
- **Deliverables:**
  - AQL_MUTATIONS_ROADMAP.md Phase 1 section completed
  - docs/de/aql/aql_syntax.md updated with v2.0.0 disclaimer
  - 5+ example DML statements (INSERT, UPDATE, DELETE variants)
  - API documentation for parser functions
  - v2.0.0 roadmap index linking to implementation docs
- **Acceptance Criteria:**
  - Roadmap updated with Phase 1 completions
  - Examples cover basic + advanced scenarios
  - v2.0.0 disclaimer on all relevant docs
  - Links between docs verified
  - v1 vs. v2.0.0 feature separation clear

## Metrics & KPIs

| Metric | Baseline | Target | Success |
|--------|----------|--------|---------|
| Tokens Added | 0 | 15+ | ✅ |
| Parser Functions | 0 | 3 | ✅ |
| AST Nodes | 0 | 4+ | ✅ |
| Unit Tests | 0 | 50+ | ✅ |
| Backward Compat | 100% v1 | 100% v1 | ✅ |
| Documentation | TBD | Complete | ✅ |

## References & Evidence

- **Source:** ROADMAP.md §AQL 2.0.0 Feature Roadmap
- **Design Doc:** src/query/AQL_MUTATIONS_ROADMAP.md
- **Reference Implementation:** src/query/sql_parser.cpp (parseInsert pattern)
- **Codebase Audit:** DOCUMENTATION_AUDIT_2026_06_18.md (feature status analysis)
- **Related Issue:** #5172 (Master gap issue)

## Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Parser complexity | Low | Medium | Use reference implementation patterns |
| Backward compat breakage | Low | High | Comprehensive v1 regression tests |
| AST design changes | Medium | Medium | Design review before implementation |
| Documentation gaps | Low | Low | Template-based doc generation |

## Timeline & Milestones

```
W36-W39: [004-A] Tokenizer extension
W39-W42: [004-B] Parser implementation
W42-W43: [004-C] AST + semantic validation
W43-W45: [004-D] Documentation + integration testing
```

**Phase 1 Target:** 2026-10-15 (parser + AST ready for Phase 2)

---

**Last Updated:** 2026-07-05  
**Created By:** AI Deep-Dive Implementation Team  
**Related Epics:** None (independent), supports AQL 2.0.0 master roadmap
