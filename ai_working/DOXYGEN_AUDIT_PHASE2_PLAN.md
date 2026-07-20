# Doxygen Audit Completion - Phase 2 Plan
**Date:** 2026-07-20  
**Status:** Planning → Implementation  
**Target:** Reduce remaining 121 @param violations to <50

---

## Executive Summary

Phase 1 (Q2 2026):
- ✅ Unsupported XML/HTML tags: 241 → 0 (query module completed)
- ✅ @param violations Phase 1: 152 → 121 (20% reduction)
- ⚠️  **Remaining work:** 121 @param inconsistencies + 31 undocumented APIs

Phase 2 targets (this sprint):
- Reduce @param violations to ≤50 (60% reduction)
- Document 31 missing APIs
- Establish quality gate for future PRs

---

## Issue Classification

Based on progress notes, Phase 1 fixed 5 files:
- `rag_judge.h` - @param split for overloaded functions
- `vram_secure_clear.h`
- `graph_query_optimizer.h`
- `timeseries.h`
- `query_federation.h`

Remaining 121 violations distributed across:
- **Query module** (69 warnings) - likely @param + undocumented signatures
- **Index module** (54 warnings) - similar patterns
- **Analytics, RAG, Content, Plugins** (150+ combined)

---

## Phase 2 Implementation Strategy

### Batch 1: Query Module Deep Scan (20 files)
**Target:** Reduce from 69 → 10 warnings

Files to prioritize (high-use APIs):
- `query_engine.h` - Core query execution interface
- `query_executor.h` - Executor implementation
- `query_compiler.h` - Compilation pipeline
- `aql_parser.h` - Parser interface
- `query_optimizer.h` - Query optimization
- `query_cache.h` - Cache manager
- And 14 others by warning volume

**Actions per file:**
1. Identify all public function signatures
2. Verify each function has matching @param/@return docs
3. For overloaded functions: split documentation into separate \@overload blocks
4. For missing docs: add @brief, @param, @return, @throws based on implementation
5. Validate parameter names match exactly

### Batch 2: Index Module (15 files)
**Target:** Reduce from 54 → 8 warnings

### Batch 3: High-Value Modules (Server, Analytics, RAG)
**Target:** Reduce 101 combined → 15 warnings

---

## Acceptance Criteria

Phase 2 completion requires:
- [ ] All public APIs have @brief and @param documentation
- [ ] Parameter names in docs match function signatures exactly (case-sensitive)
- [ ] Overloaded functions have separate @overload docs
- [ ] No unsupported XML/HTML tags in comments
- [ ] Doxygen audit: Total warnings <200 (50% reduction from 716)
- [ ] All new/modified APIs documented in PR

---

## Quality Gate (for future PRs)

Going forward:
- **Requirement:** Every PR must not introduce new Doxygen warnings
- **Verification:** Run doxygen audit pre-commit or in CI
- **Escape hatch:** Only with documented exception (e.g., vendor code)

---

## Related Files

- `tmp_tracking_stub_priorities.md` - Previous audit results
- `include/query/*` - Target module headers
- `DOCUMENTATION_GOVERNANCE.md` - Canonical style guide
- `.github/instructions/documentation-enforcement.instructions.md` - CI gate

---

## Next Steps

1. Use @ollama for C++ header documentation improvements (Batch 1)
2. Create GitHub issues for each batch (if needed)
3. Run doxygen audit post-implementation to verify
4. Update this document with results
