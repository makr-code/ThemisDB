---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> Original creation date was not recorded; this provenance block was first recorded on 2026-10-31.
> Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: security
Canonical references: `../../include/query/query_engine.h`, `query_engine.cpp`, `aql_parser.cpp`, `../../tests/query/`, `../../tests/query/CMakeLists.txt`
Related issue: #7371
Related milestone: `DOC-WEEKLY-2026-44`

# Access Validation Consistency Checklist

**Phase 1: Safety and Access Hardening** | Q3 2026

## Overview

This checklist is a review aid, not evidence that every query path enforces the same three-stage policy. The source separates parsing, optional caller-provided access checks, and federated execution; parsing does not itself authenticate or authorize a caller. Verify the affected execution method and federation route before making a security claim.

1. **Parser Stage**: Syntax parsing and scope bookkeeping; `ParserScopeContext` exposes separate validation helpers.
2. **Execution Stage**: `QueryEngine` has caller-provided `collection_access_checker_` checks on specific methods; do not infer coverage for every overload or query form.
3. **Federation Stage**: `QueryFederation` and cross-cluster paths require separate route and remote-policy verification.

## Verification Matrix

### Selected entry points in `src/query/query_engine.cpp`

| Entry Point | Parser stage | Access-check evidence | Federation status |
|---|---|---|---|---|
| `executeAndKeys(const ConjunctiveQuery& q)` | Receives a pre-parsed query | Explicit callback check is present | Not established by this entry-point check |
| `executeAndKeysWithScores(const ConjunctiveQuery& q)` | Receives a pre-parsed query | Trace delegated calls and verify the actual path | Not established by this entry-point check |
| `executeAndEntities(const ConjunctiveQuery& q)` | Receives a pre-parsed query | Explicit callback check is present | Not established by this entry-point check |
| `executeOrKeys(const DisjunctiveQuery& q)` | Receives a pre-parsed query | Callback checks are present in execution paths | Verify each disjunct and any federation separately |
| `executeRangeQuery(...)` | Receives a pre-parsed query | Verify the concrete overload before relying on this checklist | Verify route separately |

### Parser Stage (`AQLParser` in `src/query/aql_parser.cpp`)

**Responsibility**: Parse syntax and maintain parser scope information. Collection access authorization is not established by parsing alone.

| Component | Control | Verification |
|---|---|---|
| `parse(const std::string& input)` | Returns the declared parse result; malformed input is reported through the parser result contract | See `include/query/aql_parser.h` and focused parser tests |
| Collection/access validation | `ParserScopeContext` exposes separate validation; callers must follow the relevant API contract | Not a substitute for execution-time authorization |
| Expression depth bounds | Recursion limited to `kMaxExprDepth = 500` | ✓ Test: DeeplyNestedExpression |
| Graph traversal depth | Min/max depth validated at parse time | ✓ Test: GraphTraversalWithDepthConstraints |

### Execution Stage (`QueryEngine` in `src/query/query_engine.cpp`)

**Responsibility**: Enforce caller-provided access checks at runtime before execution.

**Key Control**: `collection_access_checker_` callback
- **Set by caller**: Via `setCollectionAccessChecker(callback, caller_id)`
- **Invocation**: Explicit checks are present in selected `QueryEngine` methods. Inspect the concrete method and delegation path; this checklist does not establish blanket coverage of every overload.
- **Behavior on denial**: Return `ERR_QUERY_ACCESS_DENIED`
- **Coverage**: Examples verified in this source review include `executeAndKeys`, `executeAndEntities`, `executeOrKeys`, and `executeOrKeysWithFallback`. This list is not an exhaustive audit of query execution paths.

**Implementation Example** (`executeAndKeys`; other entry points may use different result types):

```cpp
if (collection_access_checker_ && !collection_access_checker_(q.table, collection_access_caller_id_)) {
    return Err<std::vector<std::string>>(
        errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
        "Access denied for collection: " + q.table
    );
}
// Proceed to execution
```

### Federation Stage (`QueryFederation` in `src/query/query_federation.cpp`)

**Responsibility**: Route queries across cluster boundaries. Authorization behavior must be checked in the actual local and remote execution paths; routing alone does not establish access control.

**Review points** (not verified as current behavior in this checklist):
- Confirm endpoint URL and redirect validation in the active cross-cluster implementation.
- Confirm result-size and timeout limits at the concrete merge/execution boundary.
- Confirm how local caller identity and remote authorization context are propagated.

**Multi-Stage Access Check**:
1. **Local phase**: Verify the selected `QueryEngine` method invokes its configured checker.
2. **Remote phase**: Verify the remote service's independent authorization behavior; it is not established by this client-side checklist.
3. **Result merge**: Verify size and timeout limits against the active implementation.

## Regression Tests

The test names, case numbers, line numbers, and checkmarks in the remaining
tables originated in an earlier checklist and were not re-run as part of this
documentation update. Treat them as historical review notes, not as current
test-run or production-readiness evidence. Confirm registration and execution
in `tests/query/CMakeLists.txt` before using them as release evidence.

### Test File: `tests/query/test_query_parser_edge_cases.cpp`

**Access Control Tests** (Tests 33–42):

| Test ID | Scenario | Entry Point | Expected Behavior |
|---|---|---|---|
| 33 | Valid simple query | Parser | ✅ Parse succeeds |
| 34 | Collection name with special chars | Parser | ✅ Parse succeeds; no false-positive escaping |
| 35 | Variable shadowing | Parser | ✅ Scoping handled correctly |
| 36 | Complex multi-condition FILTER | Parser | ✅ All conditions parse |
| 37 | COLLECT with aggregation | Parser | ✅ Aggregation functions recognized |
| 38 | Subquery in LET | Parser | ✅ Nested query structure parsed |
| 39 | Graph traversal with depth | Parser | ✅ Depth constraints validated |

**Safety Validator Tests** (Tests 24–32):

| Test ID | Scenario | Validator | Expected Behavior |
|---|---|---|---|
| 24 | NUL character injection | SafetyValidator | ✅ Detected as NUL_INJECTION |
| 25 | INSERT mutation | SafetyValidator | ✅ Detected as MUTATION_DETECTED |
| 26 | UPDATE mutation | SafetyValidator | ✅ Detected as MUTATION_DETECTED |
| 27 | REMOVE mutation | SafetyValidator | ✅ Detected as MUTATION_DETECTED |
| 28 | DELETE mutation | SafetyValidator | ✅ Detected as MUTATION_DETECTED |
| 29 | UPSERT mutation | SafetyValidator | ✅ Detected as MUTATION_DETECTED |
| 30 | REPLACE mutation | SafetyValidator | ✅ Detected as MUTATION_DETECTED |
| 31 | DROP mutation | SafetyValidator | ✅ Detected as MUTATION_DETECTED |
| 32 | READ query | SafetyValidator | ✅ No violation |

## Cross-Stage Validation Flow

```
User Query
    ↓
[PARSER STAGE]
    ├─ Tokenize
    ├─ Syntax validation (depth, nesting, structure)
    ├─ Extract collection names → AST nodes
    └─ Return ParseResult (success/error)
    ↓
[EXECUTION STAGE]
    ├─ Check: collection_access_checker_(table, caller_id) ?
    ├─ If DENY → ERR_QUERY_ACCESS_DENIED (stop)
    ├─ If ALLOW → continue
    ├─ Execute query plan
    └─ Collect results
    ↓
[FEDERATION STAGE] (if federated query)
    ├─ Scatter to remote clusters
    ├─ Remote cluster: enforces its own access_checker
    ├─ Gather results (respecting max_result_size_bytes)
    └─ Merge and return
```

## Known Gaps and Remediation

| ID | Gap | Remediation | Status |
|---|---|---|---|
| KL-G1 | Error observability for access denials in federation | Add structured logging with audit trail | **In Progress** |
| KL-G2 | Mutation detection coverage for SQL/Cypher dialects | Extend `AqlSafetyValidator` to cover translated queries | **Q3 2026 Phase 2** |
| KL-G3 | Real-time access policy invalidation | Implement callback refresh mechanism | **Q4 2026** |

## Production Readiness Checklist

- [✅] Parser safety tests added (41 edge-case scenarios)
- [✅] Access validation consistency documented
- [✅] Entry point verification matrix created
- [✅] Regression tests registered in CMakeLists.txt
- [ ] Audit logging enhanced for access denials
- [ ] Architecture documentation updated
- [ ] Security policy reviewed and published

## Next Steps

1. **Verify Build**: Run `ctest -R "ParserEdgeCases"` to validate all 41 tests
2. **Audit Logging**: Enhance `utils/audit_logger.h` for structured access denial events
3. **Documentation Sync**: Update `src/query/ARCHITECTURE.md` § 3.2 with this checklist
4. **Review**: Schedule security review with @sec-team for gap analysis

## References

- RFC: src/query/ROADMAP.md § Phase 1 (lines 255–257)
- Threat Model: src/query/SECURITY.md § T3 (Cross-Tenant Data Access)
- Production Requirements: src/query/PRODUCTION_REQUIREMENTS.md
- Federal Controls: src/query/query_federation.cpp, include/query/query_federation.h
