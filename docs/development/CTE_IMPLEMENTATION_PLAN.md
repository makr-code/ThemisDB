# CTE Support Implementation Plan - v1.3.0

**Status:** Planning Phase  
**Complexity:** HIGH  
**Estimated Effort:** 1-2 weeks full implementation  
**Date:** December 16, 2025

---

## 🎯 Implementation Strategy

Given the complexity of full CTE support, I'm proposing an **incremental delivery approach**:

### Option A: Minimal Viable CTE (2-3 days)
Deliver basic non-recursive CTE support that covers 80% of use cases:

**Scope:**
- ✅ Non-recursive CTEs (WITH clause)
- ✅ CTE result materialization
- ✅ Sequential CTE dependencies (CTE2 can reference CTE1)
- ❌ Recursive CTEs (deferred)
- ❌ Complex correlated subqueries (deferred)

**Deliverables:**
```sql
-- SUPPORTED:
WITH high_earners AS (
  FOR u IN users
  FILTER u.salary > 100000
  RETURN u
)
FOR h IN high_earners
  FILTER h.department == "Engineering"
  RETURN h

-- NOT YET SUPPORTED:
WITH RECURSIVE org_tree AS (...)  -- Recursive CTEs
```

**Estimated Time:** 2-3 days

---

### Option B: Full CTE Support (1-2 weeks)
Complete implementation including recursive CTEs:

**Scope:**
- ✅ Non-recursive CTEs
- ✅ Recursive CTEs with fixpoint iteration
- ✅ Cycle detection
- ✅ UNION semantics
- ✅ Correlated subqueries
- ✅ Variable binding

**Estimated Time:** 1-2 weeks

---

### Option C: Focus on Simpler Gaps
Instead of CTE, address multiple smaller gaps:

**Candidates:**
1. **Process Mining TODOs** (2-3 days)
   - Graph-based event log extraction
   - Token replay implementation
   
2. **Stream Protocol** (3-4 days)
   - File chunk creation
   - Network transfer
   - Checksum verification

3. **Video Processor** (2-3 days)
   - LibAVFormat integration
   - Frame extraction
   - Thumbnail generation

---

## 📊 Recommendation

**RECOMMENDED: Option A - Minimal Viable CTE**

**Rationale:**
1. Delivers immediate value (80% of CTE use cases)
2. Faster delivery (2-3 days vs 1-2 weeks)
3. Can be enhanced incrementally
4. Lower risk of incomplete work
5. Matches "weiter" (continue) request better

**Implementation Plan for Option A:**

### Day 1: Foundation
- Implement `CTEEvaluator::evaluateCTE()` with real query execution
- Add CTE context management
- Test basic single CTE

### Day 2: Dependencies & Materialization
- Support multiple CTEs with dependencies
- Implement result materialization
- Add error handling
- Test CTE chains

### Day 3: Integration & Polish
- Integrate with QueryEngine
- Add logging and diagnostics
- Write comprehensive tests
- Update documentation

---

## 🔧 Technical Details (Option A)

### Key Changes Required:

**1. CTEEvaluator::evaluateCTE()**
```cpp
bool CTEEvaluator::evaluateCTE(
    const CTEDefinition& cte,
    QueryEngine& queryEngine
) {
    // Execute CTE subquery via QueryEngine
    auto result = queryEngine.executeQuery(cte.subquery);
    
    if (!result.ok) {
        return false;
    }
    
    // Materialize results
    cteResults_[cte.name] = result.data;
    
    return true;
}
```

**2. SubqueryEvaluator::evaluateScalarSubquery()**
```cpp
nlohmann::json SubqueryEvaluator::evaluateScalarSubquery(
    const std::shared_ptr<query::Query>& query,
    QueryEngine& queryEngine,
    const nlohmann::json& outerRow
) {
    // Bind outer variables if correlated
    if (!outerRow.empty()) {
        bindOuterVariables(query, outerRow);
    }
    
    // Execute subquery
    auto result = queryEngine.executeQuery(query);
    
    if (!result.ok || result.data.empty()) {
        return nullptr;
    }
    
    // Validate single-row constraint
    if (result.data.size() > 1) {
        throw std::runtime_error("Scalar subquery returned multiple rows");
    }
    
    // Extract first result value
    return result.data[0];
}
```

**3. QueryEngine Integration**
- Use existing `executeCTEs()` method
- Add CTE context to query execution
- Support CTE name resolution

---

## 📝 User Decision Required

**Please choose:**

**A)** Implement Minimal Viable CTE (2-3 days) - RECOMMENDED
- Non-recursive CTEs only
- Fast delivery
- Covers 80% of use cases

**B)** Implement Full CTE Support (1-2 weeks)
- Includes recursive CTEs
- Complete solution
- Longer timeline

**C)** Switch to simpler gaps (2-4 days each)
- Process Mining
- Stream Protocol
- Video Processor

---

**Awaiting decision to proceed...**

**Current Status:**
- Phase 1 complete (Embedding Cache + Hybrid Search)
- Ready to start Phase 2 based on choice above
- All previous work committed and reviewed
