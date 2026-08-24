# 📋 Documentation-Implementation Gap Audit (2026-06-18)

**Status**: ✅ **CRITICAL FINDING**  
**Scope**: Root documentation (`ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `ARCHITECTURE.md`) + AQL docs (`docs/de/aql/`)  
**Impact**: Docs claim features implemented that do NOT exist in code  
**Action Required**: Update root docs + create implementation plan

---

## 🔍 Executive Summary

| Category | Finding | Severity | Status |
|----------|---------|----------|--------|
| **AQL Mutations** | Documented as supported (DML keywords: INSERT, UPDATE, REPLACE, REMOVE, UPSERT) | 🔴 CRITICAL | ❌ Not in parser |
| **AQL DDL** | Documented as supported (CREATE, DROP, COLLECTION, INDEX, VIEW) | 🔴 CRITICAL | ❌ Not in parser |
| **Geospatial Functions** | Documented as "implemented" in docs | 🟡 HIGH | ✅ Functions exist, not wired |
| **Query Optimizer** | ARCHITECTURE.md lists "QueryOptimizer" as component | 🟡 HIGH | 🟡 Partial (only index selection) |
| **Multi-Model Queries** | Documented as unified multi-model support | 🟡 MEDIUM | 🟡 Partial (no cross-model joins) |

---

## 🔴 CRITICAL: AQL DML/DDL Mismatch

### Finding 1: DML Keywords Documented, Not Implemented

**File**: `docs/de/aql/AQL_COMPLETE_LANGUAGE_SCOPE.md` (lines 40-46)

```markdown
#### DML (6)
INSERT, UPDATE, REPLACE, REMOVE, UPSERT, INTO, WITH

#### DDL (5)
CREATE, DROP, COLLECTION, INDEX, VIEW
```

**Reality**: `src/query/aql_parser.cpp` TokenType enum has **ZERO** DML/DDL tokens

```cpp
// Lines 37-95 of aql_parser.cpp
enum class TokenType {
    FOR, IN, LET, FILTER, SORT, LIMIT, RETURN, AS, ALL,  // ✅ Present
    // ... no INSERT, UPDATE, DELETE, REPLACE, UPSERT, CREATE, DROP
};
```

**Root Cause**: Documentation written for v1.3.1 planning; implementation abandoned for v1.x (read-only architecture decision)

**Status**: 🔴 **DOCUMENTATION IS INCORRECT** — must be corrected

### Finding 2: Documentation Claims "72 Reserved Keywords" (v1.3.0)

**File**: `docs/de/aql/AQL_COMPLETE_LANGUAGE_SCOPE.md` (line 12)

```markdown
| Reservierte Keywords | 72 |
```

**Reality**: `aql_parser.cpp` has **36 tokens** in production

**Status**: 🔴 **DOCUMENTATION IS OUTDATED** — based on v1.3.1 proposal, not v1.x reality

---

## 🟡 HIGH: Geospatial Documentation vs. Implementation

### Finding 3: Geospatial Functions Listed, But Not Parser-Wired

**Files**:
- `docs/de/aql/aql_syntax.md` (line 85): "🌍 **Geospatial:** ST_Point, ST_Distance, ST_Within, ..."
- `docs/de/aql/aql_functions_reference.md`: Full geospatial API documented

**Reality**: Functions exist in `src/query/let_evaluator.cpp` but:
- NOT accessible in FILTER clauses
- NOT in AQL parser
- Only usable in LET expressions

**Status**: 🟡 **DOCUMENTATION OVERSTATES FEATURE** — marked as "fully supported" but only partially functional

---

## 🟡 MEDIUM: Multi-Model Queries

### Finding 4: ARCHITECTURE.md Claims Unified Multi-Model Support

**File**: `ARCHITECTURE.md` (line 10)

```markdown
"integrated relational, graph, vector, and document models"
```

**Reality**: Multi-model **storage** exists (RocksDB, indices, etc.) but:
- No cross-model JOIN syntax (can't join relational + graph in single query)
- No multi-model optimization (query planner treats separately)
- Must use separate queries + application-level merge

**Status**: 🟡 **DOCUMENTATION OVERSTATES FEATURE** — "integrated" implies unified queries, not just storage

---

## 📊 Documentation Gap Statistics

| Document | Status | Action |
|----------|--------|--------|
| `ROADMAP.md` | Aggregates module status; no AQL-specific mutations roadmap | ⚠️ Add reference to `src/query/AQL_MUTATIONS_ROADMAP.md` |
| `FUTURE_ENHANCEMENTS.md` | Generic backlog; no AQL mutations specific items | ⚠️ Add section "§ AQL 2.0.0 Roadmap" with link to roadmap files |
| `ARCHITECTURE.md` | Lists `/query/` module but no mutation/DDL detail | ✅ OK (design docs, not implementation claim) |
| `docs/de/aql/AQL_COMPLETE_LANGUAGE_SCOPE.md` | **INCORRECT** — claims 72 keywords, DML/DDL | 🔴 **MUST UPDATE**: Mark as v1.3.1 proposal, not current |
| `docs/de/aql/aql_syntax.md` | Claims full geospatial + multi-model support | 🟡 **MUST CLARIFY**: Note parser integration roadmap for v2.0.0 |
| `docs/de/aql/aql_functions_reference.md` | Documents all ST_* functions | ✅ OK but add disclaimer: "Usable in LET only, not FILTER (v1.x)" |

---

## ✅ Action Items (Priority Order)

### 🔴 **P0: Root Documentation Fixes** (This Week)

- [ ] **ROADMAP.md**: Add section "§ AQL 2.0.0 Feature Roadmap" with table:
  ```markdown
  | Feature | Status | Target | Roadmap |
  |---------|--------|--------|---------|
  | Mutations | ❌ Planned | Q4 2026 | src/query/AQL_MUTATIONS_ROADMAP.md |
  | DDL | ❌ Planned | Q4 2026 | [Pending] |
  | Geospatial Parser Integration | ⏳ In Progress | Q3 2026 | [Pending] |
  | FTS Query Enhancement | ❌ Planned | Q3 2026 | [Pending] |
  ```

- [ ] **FUTURE_ENHANCEMENTS.md**: Add subsection in "Wave C" (Medium / Long-term):
  ```markdown
  ### AQL 2.0.0 Standard Coverage (Q3-Q4 2026)
  - [ ] Mutations (INSERT/UPDATE/DELETE/REPLACE/UPSERT) — 12-15 weeks
  - [ ] DDL (CREATE/DROP/ALTER) — 4-6 weeks
  - [ ] Geospatial Parser Integration — 2-3 weeks (functions 70% exist)
  - [ ] FTS Query Enhancement — 2-3 weeks
  - See: src/query/AQL_MUTATIONS_ROADMAP.md + AQL_V2_0_0_COMPLETE_ROADMAP.md
  ```

- [ ] **ARCHITECTURE.md**: Add clarification in `/query/` entry:
  ```markdown
  | **query/** | AQL parser, optimizer, execution engine. v1.x: read-only; v2.0.0: full DML/DDL support planned (see ROADMAP.md) | QueryEngine, AqlParser, QueryOptimizer |
  ```

### 🟡 **P1: AQL Documentation Fixes** (Next 2 Weeks)

- [ ] **docs/de/aql/AQL_COMPLETE_LANGUAGE_SCOPE.md**: Mark as **v1.3.1 PROPOSAL (not current)**
  ```markdown
  ---
  **⚠️ DEPRECATION NOTICE:** This document describes v1.3.1 proposal (read-only AQL with OOP extensions).
  Current production: v1.x with **read-only** support only.
  For v2.0.0 roadmap (mutations/DDL/geospatial/FTS), see src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md
  ---
  ```

- [ ] **docs/de/aql/aql_syntax.md**: Add disclaimer to geospatial section:
  ```markdown
  ⚠️ **v1.x Status:** Geospatial functions available in LET expressions only.
  Parser integration for FILTER predicates planned for v2.0.0 (Q3 2026).
  See: src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md
  ```

- [ ] **docs/de/aql/aql_functions_reference.md**: Add per-function status marker:
  ```markdown
  | ST_Distance | ✅ Available | LET expressions only (v1.x) | FILTER support planned (v2.0.0 Q3) |
  | ST_Within | ✅ Available | LET expressions only (v1.x) | FILTER support planned (v2.0.0 Q3) |
  ```

- [ ] **docs/de/aql/README.md**: Add "Version Status" section:
  ```markdown
  ## Version Status
  - **v1.x (Current):** Read-only queries (FOR/FILTER/RETURN/SORT/LIMIT/LET/COLLECT)
  - **v2.0.0 (Planned Q4 2026):** Full AQL standard (Mutations, DDL, Geospatial, FTS)
  - See: docs/architecture/QUERYENGINE_IMPLEMENTATION_GUIDE.md + src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md
  ```

### 🟢 **P2: New Documentation** (Parallel with Implementation)

- [ ] Create `docs/de/aql/AQL_2_0_0_ROADMAP_LINK.md` (index page):
  ```markdown
  # AQL 2.0.0 Feature Roadmap
  Complete documentation for v2.0.0 planned features:
  - Mutations (v2.0.0-Phase-1): src/query/AQL_MUTATIONS_ROADMAP.md
  - DDL (v2.0.0-Phase-2): src/query/AQL_DDL_ROADMAP.md [TBD]
  - Geospatial Integration (v2.0.0-Phase-1b): src/geospatial/AQL_GEOSPATIAL_ROADMAP.md [TBD]
  - FTS Enhancement (v2.0.0-Phase-1c): src/index/AQL_FTS_ROADMAP.md [TBD]
  - Implementation Plan: src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md
  ```

- [ ] Update `docs/DOCUMENTATION_HUB.md`:
  ```markdown
  ## Query Language (AQL)
  - **v1.x (Current)**: docs/de/aql/README.md (read-only)
  - **v2.0.0 Roadmap**: src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md + src/query/AQL_MUTATIONS_ROADMAP.md
  ```

---

## 📐 Implementation vs. Documentation Timeline

| Document | Written For | Actual | Status |
|----------|-------------|--------|--------|
| `AQL_COMPLETE_LANGUAGE_SCOPE.md` | v1.3.1 proposal (planned/never shipped) | v1.x production (read-only) | ❌ Outdated |
| `aql_syntax.md` | v1.3+ features | v1.x features | 🟡 Overstated (geospatial only in LET) |
| `aql_functions_reference.md` | v1.3+ functions | v1.x functions (partial) | 🟡 Incomplete (missing context) |
| Root ARCHITECTURE.md | Module architecture | Current module architecture | ✅ OK (generic) |
| Root ROADMAP.md | Active release tracking | v1.9.0-beta | ⚠️ Needs AQL 2.0.0 roadmap link |

---

## Root Cause Analysis

**Why did this gap exist?**

1. **v1.3.1 Proposal → v1.x Production Shift**: Decision made to keep AQL read-only for v1.x stability
   - Documentation already written for v1.3.1 (with mutations)
   - Never updated when v1.3.1 was abandoned
   - Docs remained in repo as "future vision"

2. **Documentation Fragmentation**: No single source-of-truth for AQL feature state
   - `docs/de/aql/` is proposal-heavy
   - `ROADMAP.md` doesn't reference AQL status
   - No linking between root docs and implementation

3. **Missing Governance Rule**: `COPILOT_INSTRUCTIONS.md` exists but doesn't enforce AQL-specific sync requirements

---

## Recommended Governance Changes

**Update `COPILOT_INSTRUCTIONS.md` Section 1.1 (Branch/Edition Governance):**

```markdown
### 1.2) AQL Feature Status Governance (NEW)

When implementing AQL features (Mutations, DDL, Geospatial, FTS, etc.):
1. Update src/query/AQL_*.ROADMAP.md FIRST (before PR)
2. Reference roadmap in CHANGELOG.md and PR body
3. Update docs/de/aql/*.md with version-specific status markers:
   - ✅ (v1.x available)
   - 🟡 (v1.x partial/LET-only)
   - ❌ (v2.0.0 planned)
4. Link all feature docs back to src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md
```

---

## Summary: Documentation Integrity Score

| Category | Score | Status |
|----------|-------|--------|
| AQL Mutations Docs | 0/10 ❌ | Documented as feature, not implemented |
| AQL DDL Docs | 0/10 ❌ | Documented as feature, not implemented |
| Geospatial Docs | 5/10 🟡 | Functions documented, implementation incomplete |
| Multi-Model Docs | 4/10 🟡 | Overstates capability (storage unified, queries not) |
| Root Governance Docs | 7/10 🟡 | Sync rules exist, but AQL-specific rules missing |
| **Overall** | **3.2/10** 🔴 | **CRITICAL: Documentation-Implementation misalignment** |

**Recommendation**: Execute all P0 fixes immediately (2-3 hours); P1 fixes this sprint (8 hours).

---

*Audit completed: 2026-06-18*  
*Next review: After AQL 2.0.0 roadmap approval*
