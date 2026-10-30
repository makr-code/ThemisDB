---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: api-contract
Canonical references: `include/query/aql_parser.h`, `include/query/query_engine.h`, `src/query/aql_parser.cpp`, `src/query/query_engine.cpp`, `tests/query/test_aql_parser_service.cpp`
Related issues: #7093, #7167

# Query API contract

## Scope

This page is for callers and maintainers integrating the AQL parser and query execution APIs. Public declarations in `include/query/` take precedence over this summary.

## Public surfaces

- `AQLParser::parse(const std::string&)` returns `Result<std::shared_ptr<Query>>`. The header also declares transaction-block, DDL, mutation, and schema-DDL parse methods with typed results.
- `ParserScopeContext` registers collections, tracks scopes, and exposes `validateCollectionAccess()` as `Result<bool>`.
- `QueryEngine` exposes typed `Result<T>` execution methods for query forms including traversal, conjunction/disjunction, joins, grouping, and vector/geospatial operations. Consult the declaration for the exact method and result type.

## Caller obligations and failure handling

- Check `Result` before consuming a parsed query or execution result; preserve its error details when forwarding failures.
- Treat parsing, scope validation, and execution as distinct calls. The headers do not promise that every caller automatically invokes every validation helper.
- Use the matching parse/execution method for the requested query form; do not infer support from a similarly named AST node alone.
- Add regression coverage for malformed input and error propagation on the changed method.

## Validation references

Use `tests/query/test_aql_parser_service.cpp` and the focused test closest to the changed executor. Test targets are registered under `tests/query/CMakeLists.txt`.

See `IMPLEMENTATION.md` for source locations and `MODULE_GUIDE.md` for maintainer workflow.
