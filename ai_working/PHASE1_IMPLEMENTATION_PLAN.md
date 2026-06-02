# Phase 1 Implementation Plan - Quick Wins

**Date:** 2026-06-02  
**Objective:** Whitelists + Context Expansion  
**Expected Impact:** 24% → 35-40% TP rate  
**Timeline:** 3-4 days

---

## Phase 1 Tasks (Prioritized)

### TASK 1: Global Context Window Expansion (±5 → ±15 lines)
**Priority:** HIGHEST | **Effort:** 1 hour | **Impact:** +5-8% TP across all categories

**Files to Modify:**
1. `gap_scanner_v3_progressive_context_filters.py` (main filter)
2. All individual scanner files (adjust CONTEXT_LINES constant)

**Changes:**
```python
# Before:
CONTEXT_LINES = 5

# After:
CONTEXT_LINES = 15
```

**Expected Result:** Better understanding of loop/scope context, fewer false positives

---

### TASK 2: Copy_Overhead Whitelists
**Priority:** HIGH | **Effort:** 2 hours | **Impact:** +30% TP for copy_overhead

**File:** `gap_scanner_v3_phase8_performance_patterns.py`

**Concrete Fixes:**
1. Whitelist `std::make_shared` / `std::make_unique` calls
2. Whitelist `std::move()` operations
3. Require loop context for "copy in loop" detection
4. Add POD type detection (skip cheap copies)

**Implementation:**
```python
def should_skip_copy_overhead(line, context):
    """Check if copy is safe and should be whitelisted"""
    
    # Whitelist 1: make_shared / make_unique
    if 'make_shared' in line or 'make_unique' in line:
        return True  # These are safe, never flag
    
    # Whitelist 2: std::move operations
    if 'std::move' in line or 'std::forward' in line:
        return True  # These are optimization, not overhead
    
    # Filter: Require actual loop context
    full_context = '\n'.join(context)
    has_loop = any(kw in full_context for kw in ['for', 'while', 'std::for_each', 'std::ranges::for_each'])
    if not has_loop:
        return True  # Not in loop, safe
    
    # Filter: POD type check (integers, floats, bools are cheap)
    pod_types = ['int', 'float', 'double', 'bool', 'uint', 'size_t', 'char']
    if any(t in line for t in pod_types):
        return True  # POD copies are cheap
    
    return False  # Flag it: complex type copy in loop
```

---

### TASK 3: Observability Scoping
**Priority:** HIGH | **Effort:** 2 hours | **Impact:** +40% TP for observability

**File:** `gap_scanner_v3_phase10_observability.py`

**Concrete Fixes:**
1. Skip functions in internal/detail/impl namespaces
2. Skip trivial functions (< 5 lines)
3. Require public API marker (THEMIS_API) before flagging
4. Skip getters/setters/operators

**Implementation:**
```python
def should_skip_observability_check(func_name, file_path, function_body):
    """Check if function needs observability logging"""
    
    # Skip 1: Internal namespaces
    internal_patterns = ['::internal::', '::detail::', '::impl::', '_internal', '__detail']
    if any(p in func_name for p in internal_patterns):
        return True  # Internal function, no need for logging
    
    # Skip 2: Trivial functions (< 5 lines)
    code_lines = [l for l in function_body.split('\n') if l.strip() and not l.strip().startswith('//')]
    if len(code_lines) < 5:
        return True  # Trivial function, no logging needed
    
    # Skip 3: Common patterns that don't need logging
    if any(pat in func_name for pat in ['operator', 'get_', 'set_', '::~', '::operator']):
        return True  # Getter/setter/operator, skip
    
    # Require: Public API scope
    has_api_marker = 'THEMIS_API' in file_path or 'THEMIS_.*_API' in function_body
    if not has_api_marker:
        return True  # Not public API, skip
    
    return False  # Flag it: public API function without logging
```

---

### TASK 4: DB Connection Leak Context Fix
**Priority:** MEDIUM | **Effort:** 1.5 hours | **Impact:** +40% TP for db_connection_leak

**File:** `gap_scanner_v3_raii.py` or memory scanner

**Concrete Fixes:**
1. Expand context from ±5 to ±20 lines (catch destructor cleanup)
2. Whitelist smart_ptr wrapped connections
3. Recognize ConnectionPool patterns

**Implementation:**
```python
def should_skip_connection_leak(line, context):
    """Check if connection has proper cleanup"""
    
    # Whitelist 1: RAII wrapped
    if 'std::unique_ptr' in context or 'std::shared_ptr' in context:
        return True  # Smart pointer handles cleanup
    
    # Whitelist 2: ConnectionPool pattern
    if 'ConnectionPool' in context or 'connection_pool' in context:
        return True  # Pool manages cleanup
    
    # Whitelist 3: Explicit close() call in context
    if '.close()' in context or '->close()' in context:
        return True  # Explicit cleanup present
    
    # Check full scope for try/finally or RAII
    # Need ±20 lines to see destructor
    
    return False  # Flag it: connection without cleanup path
```

---

### TASK 5: No_Health_Check Scope Filter
**Priority:** MEDIUM | **Effort:** 1 hour | **Impact:** +30% TP for no_health_check

**File:** `gap_scanner_v3_phase10_observability.py` or reliability.py

**Concrete Fixes:**
1. Only flag HTTP/gRPC handlers
2. Only flag critical paths
3. Skip internal utilities

**Implementation:**
```python
def should_flag_missing_health_check(func_name, file_path):
    """Check if function is critical path needing health checks"""
    
    # Only flag known handler patterns
    handler_patterns = [
        'Handler', 'handler', 'process', 'handle',
        'Route', 'route', 'Endpoint', 'endpoint',
        'execute', 'Execute', 'Run', 'run'
    ]
    
    if not any(pat in func_name for pat in handler_patterns):
        return False  # Not a handler, skip
    
    # Skip internal functions
    if any(p in func_name for p in ['::internal::', '::detail::']):
        return False
    
    # Skip if it's just a data processor, not entry point
    if 'process_data' in func_name or 'transform' in func_name:
        if 'HTTP' not in file_path and 'gRPC' not in file_path:
            return False  # Internal processor, not entry point
    
    return True  # Flag it: critical handler without health checks
```

---

### TASK 6: Hardcoded_Path Runtime Detection
**Priority:** LOW | **Effort:** 1 hour | **Impact:** +30% TP for hardcoded_path

**File:** Input validation or deprecated APIs scanner

**Concrete Fixes:**
1. Distinguish compile-time (constexpr, #define) vs runtime
2. Whitelist configuration paths
3. Skip test code

**Implementation:**
```python
def should_flag_hardcoded_path(line, context, file_path):
    """Check if path is truly hardcoded at runtime"""
    
    # Skip 1: Compile-time constants
    if 'constexpr' in context or '#define' in context:
        return False  # Compile-time constant, safe
    
    # Skip 2: From configuration
    if 'config.' in context or 'Config::' in context:
        return False  # Configuration source, not hardcoded
    
    # Skip 3: Environment variables
    if 'getenv' in context or 'std::getenv' in context:
        return False  # From environment, not hardcoded
    
    # Skip 4: Test code
    if '_test.cpp' in file_path or '_unittest.cpp' in file_path:
        return False  # Test code, acceptable
    
    # Flag it: true hardcoded path at runtime
    return True
```

---

## Implementation Sequence

### Day 1 (4-5 hours)
- [ ] Task 1: Context window expansion (1 hour)
- [ ] Task 2: Copy_overhead whitelists (2 hours)
- [ ] Task 3: Observability scoping (2 hours)

### Day 2 (3-4 hours)
- [ ] Task 4: DB connection leak context (1.5 hours)
- [ ] Task 5: Health check scoping (1 hour)
- [ ] Task 6: Hardcoded path runtime detection (1 hour)

### Day 3 (2-3 hours)
- [ ] Integrate all changes
- [ ] Test individual scanners
- [ ] Prepare for re-run

---

## Testing Strategy

After each change:

```bash
# 1. Verify syntax
python -m py_compile gap_scanner_v3_*.py

# 2. Run individual scanner
python tools/gap_scanner_v3_performance.py --test-mode

# 3. Full pipeline dry-run
python tools/gap_audit_pipeline_v3.py --repo . --dry-run --sample 50

# 4. Re-validate
python tools/analyze_validation_sample.py
```

---

## Success Criteria (Day 3 End)

| Category | Current | Target | Status |
|----------|---------|--------|--------|
| copy_overhead | 0% TP | ≥50% TP | ? |
| observability | 0% TP | ≥60% TP | ? |
| db_connection_leak | 0% TP | ≥40% TP | ? |
| no_health_check | 0% TP | ≥40% TP | ? |
| hardcoded_path | 0% TP | ≥30% TP | ? |
| **OVERALL** | **24% TP** | **≥35% TP** | ? |

If overall TP < 35% after Day 3 → adjust strategies and continue Phase 2 sooner

---

## Rollback Plan

If changes break pipelines:

```bash
git stash                  # Save work
git checkout develop       # Back to last stable
python tools/gap_audit_pipeline_v3.py --repo . --quick-test
```

---

**Status:** Ready to start implementation  
**Next Command:** Let's begin with Task 1 (Context window expansion)

