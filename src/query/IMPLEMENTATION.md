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
SOT domain: module-behavior
Canonical references: `include/query/aql_parser.h`, `include/query/query_engine.h`, `src/query/aql_parser.cpp`, `src/query/query_engine.cpp`, `tests/query/test_aql_parser_service.cpp`
Related issues: #7093, #7167

# Query implementation

## Purpose and boundary

For query maintainers changing AQL parsing or query execution. The public parser and execution declarations are in `include/query/`; implementation belongs in `src/query/`. This guide describes those contracts, not a guarantee that every query form uses one identical runtime pipeline.

## Implementation map

- `aql_parser.cpp` implements `AQLParser`; `parse()` returns `Result<std::shared_ptr<Query>>`.
- `query_engine.cpp` implements query execution entry points declared by `QueryEngine`; these return typed `Result<T>` values.
- Parser scope bookkeeping and collection validation are exposed separately through `ParserScopeContext`.

## Invariants and failure cases

- Propagate parse and execution errors as errors; do not turn invalid input into an apparently valid query.
- Preserve the parser’s AST and `Result` contract when changing syntax or translation.
- Scope validation returns a result and is a distinct API; callers must not assume it is automatically applied to every execution path.
- Query forms and execution methods are distinct; update only the path and result type affected by a change.

## Change and validation steps

1. Identify the affected declaration in `include/query/` and its implementation in `src/query/`.
2. Keep input validation and execution errors observable to the caller.
3. Add or adjust focused tests; start with `tests/query/test_aql_parser_service.cpp` for parser-service changes and `tests/query/test_continuous_query_engine.cpp` for continuous-query changes.
4. Run the corresponding test target registered by `tests/query/CMakeLists.txt`.

## Related module references

See `API.md`, `MODULE_GUIDE.md`, `ARCHITECTURE.md`, and `ROADMAP.md` in this directory.
