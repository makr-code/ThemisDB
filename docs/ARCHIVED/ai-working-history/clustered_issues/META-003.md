# Resolve all TODO/FIXME comments: create linked issues or complete

**Issue Type:** Implementation Gap Audit  
**Priority:** MEDIUM  
**Affected Modules:** 10  
**Total Gaps:** 29  

## Summary


This meta-issue ensures all TODO/FIXME comments are either:
1. Completed (code implemented)
2. Linked to a GitHub issue
3. Explicitly decided as "not needed"

**Scope:** 29 TODO/FIXME items across 10 modules.

Each TODO should be actionable and tracked. This prevents technical debt from silently accumulating.


## Gap Breakdown

- **Todo:** 29

## Example Gaps

- `src\ai\ai_plugin_generator.cpp:65` — // TODO (Phase 2, v1.6.0): replace the error below with a real HTTP call to
- `tests\test_aql_st_predicates.cpp:33` — // TODO: Update tests to use correct TranslationResult API once spatial predicat
- `tests\test_aql_st_predicates.cpp:71` — // TODO: Rewrite tests once spatial predicates are fully implemented and API is 
- ... and 2 more

## Acceptance Criteria

1. All TODO/FIXME comments are reviewed
2. Each TODO either: (a) completed, (b) linked to GitHub issue, or (c) deleted with reason
3. TODO count reduced by >75% or all remaining TODOs have linked issues
4. New code additions have zero TODO comments (enforce at PR review)

## Related

- [Gap Scan Results](ai_working/) — Full scan reports
- [ROADMAP.md](ROADMAP.md) — Project roadmap
