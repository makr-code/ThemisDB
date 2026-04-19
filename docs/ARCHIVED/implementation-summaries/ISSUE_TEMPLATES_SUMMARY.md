# HTTP Server Refactoring - Issue Templates Summary

## Overview

This document provides a quick reference for the 15 issue templates created for the http_server.cpp refactoring project.

## Issue Templates Created

All templates are located in `.github/ISSUE_TEMPLATE/` and follow the naming convention: `refactor-http-server-{handler}-api-handler.md`

### Priority P1 (High Priority)

| Handler | File | Lines | Effort | Labels | Notes |
|---------|------|-------|--------|--------|-------|
| MonitoringApiHandler | refactor-http-server-monitoring-api-handler.md | ~300 | Medium | P1, good first issue | **Critical for observability** |

### Priority P2 (Standard Priority)

#### Good First Issues (Recommended for Early Implementation)

| Handler | File | Lines | Effort | Complexity |
|---------|------|-------|--------|------------|
| CacheApiHandler | refactor-http-server-cache-api-handler.md | ~200 | Small | Low |
| PromptApiHandler | refactor-http-server-prompt-api-handler.md | ~250 | Small | Low |
| GraphApiHandler | refactor-http-server-graph-api-handler.md | ~150 | Small | Low-Medium |
| SpatialApiHandler | refactor-http-server-spatial-api-handler.md | ~200 | Small | Low-Medium |
| EntityApiHandler | refactor-http-server-entity-api-handler.md | ~880 | Large | Medium-High |

#### Medium Complexity

| Handler | File | Lines | Effort | Complexity |
|---------|------|-------|--------|------------|
| IndexApiHandler | refactor-http-server-index-api-handler.md | ~400 | Medium | Medium |
| VectorApiHandler | refactor-http-server-vector-api-handler.md | ~450 | Medium | Medium |
| TransactionApiHandler | refactor-http-server-transaction-api-handler.md | ~250 | Small | Medium |
| TimeSeriesApiHandler | refactor-http-server-timeseries-api-handler.md | ~350 | Medium | Medium |
| ChangefeedApiHandler | refactor-http-server-changefeed-api-handler.md | ~400 | Medium | Medium-High |
| PolicyApiHandler | refactor-http-server-policy-api-handler.md | ~200 | Small | Low-Medium |
| WALApiHandler | refactor-http-server-wal-api-handler.md | ~220 | Small | Medium |

#### High Complexity (Requires More Experience)

| Handler | File | Lines | Effort | Complexity |
|---------|------|-------|--------|------------|
| QueryApiHandler | refactor-http-server-query-api-handler.md | ~850 | X-Large | High |
| ContentApiHandler | refactor-http-server-content-api-handler.md | ~900 | X-Large | High |

## Recommended Implementation Order

### Phase 1: Simple Handlers (Get familiar with pattern)
1. **MonitoringApiHandler** (P1 - critical)
2. CacheApiHandler
3. PromptApiHandler
4. GraphApiHandler
5. SpatialApiHandler

### Phase 2: Medium Complexity
6. PolicyApiHandler
7. WALApiHandler
8. TransactionApiHandler
9. TimeSeriesApiHandler
10. IndexApiHandler
11. VectorApiHandler

### Phase 3: Complex Handlers
12. ChangefeedApiHandler
13. EntityApiHandler

### Phase 4: Most Complex
14. QueryApiHandler
15. ContentApiHandler

## Template Structure

Each issue template includes:

1. **Title** - Following convention: `[REFACTOR] Implement {Handler}ApiHandler - Extract {Operations} from http_server.cpp`
2. **Labels** - priority, type, area, effort, and special tags (good first issue)
3. **Handler Details** - Class name, files, lines to extract, complexity
4. **Endpoints to Implement** - List of all handler methods
5. **Key Dependencies** - Constructor parameters
6. **Features** - What the handler provides
7. **Documentation References** - Links to implementation and integration guides
8. **Acceptance Criteria** - Checklist for completion
9. **Effort Metadata** - Effort, priority, complexity summary

## Creating GitHub Issues

Maintainers can create issues directly from templates:

```bash
# Option 1: Via GitHub UI
1. Go to Issues → New Issue
2. Select template from dropdown
3. Fill in any additional details
4. Create issue

# Option 2: Via GitHub CLI
gh issue create --template refactor-http-server-{handler}-api-handler.md
```

## Labels Applied

All templates use consistent labeling:

- **Priority**: `priority:P1` or `priority:P2`
- **Type**: `type:refactoring`
- **Area**: `area:{specific-area}` (api, query, indexing, vector, etc.)
- **Effort**: `effort:small`, `effort:medium`, `effort:large`, or `effort:x-large`
- **Special**: `good first issue` (for simpler handlers)

## Total Refactoring Scope

- **Total Handlers**: 15 remaining (+ 1 already implemented: AdminApiHandler)
- **Total Lines**: ~5,970 lines to migrate
- **Estimated Total Effort**: ~40-60 hours for experienced developers
- **Current Status**: 
  - ✅ AdminApiHandler (80 lines) - **COMPLETE**
  - ⬜ 15 handlers remaining - **READY FOR IMPLEMENTATION**

## Documentation References

All templates reference these comprehensive guides:

1. **`docs/HANDLER_IMPLEMENTATION_GUIDE.md`** (5,940 characters)
   - Step-by-step implementation instructions
   - Code patterns and helper methods
   - Testing strategy
   - Priority order recommendations

2. **`docs/INTEGRATION_GUIDE.md`** (11,320 characters)
   - CMakeLists.txt setup
   - HttpServer class modifications
   - Route delegation patterns
   - Troubleshooting guide

3. **`docs/HTTP_SERVER_REFACTORING.md`** (comprehensive plan)
   - Overall refactoring strategy
   - Phase breakdown
   - File size analysis

4. **Reference Implementation**: `src/server/admin_api_handler.cpp`
   - Working example to follow
   - Complete pattern demonstration

## Quality Assurance

All templates have been:
- ✅ Code reviewed
- ✅ Verified for consistency
- ✅ Checked for accuracy
- ✅ Tested for completeness
- ✅ Updated based on review feedback

## Contact

For questions about the refactoring process or issue templates, refer to:
- The documentation guides listed above
- The reference implementation (AdminApiHandler)
- The original refactoring PR discussion

---

**Last Updated**: 2026-04-06  
**Status**: Ready for GitHub issue creation  
**Total Templates**: 15  
**Completion**: All preparatory work complete
