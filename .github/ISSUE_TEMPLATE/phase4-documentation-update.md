---
name: Documentation Update for Phase 4 Migration
about: Update documentation to reflect new Result<T> error handling patterns
title: '[Docs] Update Query Engine Documentation for Phase 4 Migration'
labels: documentation, query-engine, error-handling, P2
assignees: ''
---

## 📋 Module: Documentation

**Priority:** P2 (Medium)  
**Estimated Effort:** 1-2 days  
**Complexity:** LOW  
**Dependencies:** Phase 4A-4C must be merged

## 🎯 Objective

Update ThemisDB documentation to reflect the new `Result<T>` error handling patterns introduced in Phase 4 Query Engine Migration.

## 📊 Scope

### Documentation Files to Update

**API Documentation**
- [ ] `docs/api/query_engine.md` - Query engine public APIs
- [ ] `docs/api/error_handling.md` - Error handling patterns
- [ ] `docs/api/aql_reference.md` - AQL query language reference

**Developer Guides**
- [ ] `docs/dev/contributing.md` - Contributing guidelines
- [ ] `docs/dev/error_handling_guide.md` - Error handling best practices
- [ ] `docs/dev/query_engine_internals.md` - Query engine architecture

**Examples**
- [ ] `examples/query_examples.cpp` - Query engine usage examples
- [ ] `examples/error_handling.cpp` - Error handling examples
- [ ] `README.md` - Main project README

**Migration Guides**
- [ ] Create `docs/migration/phase4_migration_guide.md`
- [ ] Update `CHANGELOG.md` with breaking changes
- [ ] Update `MIGRATION_ROADMAP.md` with completion status

## 🔧 Documentation Updates

### 1. API Documentation

#### File: `docs/api/query_engine.md`

**Add Section: Error Handling**
```markdown
## Error Handling

All major query engine APIs now return `Result<T>` for unified error handling:

### Expression Evaluation
```cpp
Result<nlohmann::json> evaluateExpression(
    const Expression& expr,
    const EvaluationContext& ctx
);
```

**Usage:**
```cpp
auto result = engine.evaluateExpression(expr, ctx);
if (!result) {
    std::cerr << "Error: " << result.error().message() << std::endl;
    return;
}
nlohmann::json value = *result;
```

### Join Queries
```cpp
Result<std::vector<nlohmann::json>> executeJoin(
    const std::vector<ForNode>& for_nodes,
    const std::vector<FilterNode>& filters,
    ...
);
```

**Usage:**
```cpp
auto result = engine.executeJoin(for_nodes, filters, ...);
if (!result) {
    handleError(result.error());
    return;
}
processResults(*result);
```
```

#### File: `docs/api/error_handling.md`

**Update Section: Error Codes**
```markdown
## Query Engine Error Codes

### ERR_QUERY_EXECUTION_FAILED (6102)
General query execution failures including:
- Expression evaluation errors
- Join execution failures
- Optimizer execution errors

**Example:**
```cpp
auto result = engine.executeJoin(...);
if (!result && result.error().code() == ERR_QUERY_EXECUTION_FAILED) {
    // Handle query execution failure
}
```

### ERR_QUERY_TYPE_MISMATCH (6106)
Type conversion and validation errors in expressions.

### ERR_QUERY_RESOURCE_EXHAUSTED (6107)
Resource limit errors (memory, time, etc.).
```

### 2. Developer Guides

#### File: `docs/dev/error_handling_guide.md`

**Add Section: Query Engine Patterns**
```markdown
## Query Engine Error Handling

### Pattern: Expression Evaluation

When evaluating expressions, use the `Result<T>` pattern:

```cpp
// Evaluate expression
auto result = evaluateExpression(expr, ctx);
if (!result) {
    // Log error
    THEMIS_WARN("Expression evaluation failed: {}", result.error().message());
    
    // Graceful degradation for non-critical paths
    continue; // Skip this value
    // OR
    return default_value; // Use fallback
}

// Use result
nlohmann::json value = *result;
```

### Pattern: Join Operations

For join operations, propagate errors to callers:

```cpp
Result<std::vector<nlohmann::json>> executeMyJoin(...) {
    auto result = engine.executeJoin(...);
    if (!result) {
        return result; // Propagate error
    }
    
    // Process results
    auto data = *result;
    return Ok(processedData);
}
```

### Pattern: Optimizer Functions

For optimizer functions, convert from underlying Status:

```cpp
Result<std::vector<std::string>> optimizedExecution(...) {
    auto [status, keys] = engine.underlyingFunction(...);
    if (!status.ok) {
        return Err<std::vector<std::string>>(
            ERR_QUERY_EXECUTION_FAILED,
            fmt::format("Optimization failed: {}", status.message)
        );
    }
    return Ok(std::move(keys));
}
```
```

### 3. Examples

#### File: `examples/query_examples.cpp`

**Update Examples:**
```cpp
// Example 1: Simple query with error handling
void example_simple_query() {
    QueryEngine engine(db, indexManager);
    
    auto result = engine.evaluateExpression(expr, ctx);
    if (!result) {
        std::cerr << "Query failed: " << result.error().message() << std::endl;
        return;
    }
    
    std::cout << "Result: " << result->dump() << std::endl;
}

// Example 2: Join query with error handling
void example_join_query() {
    QueryEngine engine(db, indexManager);
    
    auto result = engine.executeJoin(for_nodes, filters, ...);
    if (!result) {
        handleError(result.error());
        return;
    }
    
    for (const auto& row : *result) {
        std::cout << row.dump() << std::endl;
    }
}

// Example 3: Optimized query with error handling
void example_optimized_query() {
    QueryEngine engine(db, indexManager);
    QueryOptimizer optimizer(indexManager);
    
    auto plan = optimizer.chooseOrderForAndQuery(query);
    auto result = optimizer.executeOptimizedKeys(engine, query, plan);
    
    if (!result) {
        std::cerr << "Optimization failed: " << result.error().message() << std::endl;
        // Fall back to non-optimized execution
        return;
    }
    
    processKeys(*result);
}
```

### 4. Migration Guide

#### File: `docs/migration/phase4_migration_guide.md`

**Create New File:**
```markdown
# Phase 4 Migration Guide

## Overview

Phase 4 of the error handling migration introduces `Result<T>` return types for major query engine APIs. This is a **breaking change** that requires updates to all calling code.

## Affected APIs

### Phase 4A: Expression Evaluator
- `evaluateExpression()`: `nlohmann::json` → `Result<nlohmann::json>`

### Phase 4B: Join Operations
- `executeJoin()`: `pair<Status, T>` → `Result<T>`
- `executeGroupBy()`: `pair<Status, T>` → `Result<T>`

### Phase 4C: Query Planner
- `executeOptimizedKeys()`: `pair<Status, T>` → `Result<T>`
- `executeOptimizedEntities()`: `pair<Status, T>` → `Result<T>`

## Migration Steps

### Step 1: Update Function Calls

**Before:**
```cpp
auto [status, results] = engine.executeJoin(...);
if (!status.ok) {
    handleError(status.message);
    return;
}
processResults(results);
```

**After:**
```cpp
auto result = engine.executeJoin(...);
if (!result) {
    handleError(result.error().message());
    return;
}
processResults(*result);
```

### Step 2: Update Error Checking

**Before:**
```cpp
nlohmann::json value = engine.evaluateExpression(expr, ctx);
if (value.is_null()) {
    // Unclear if error or legitimate null
}
```

**After:**
```cpp
auto result = engine.evaluateExpression(expr, ctx);
if (!result) {
    // Clear error path
    handleError(result.error());
    return;
}
nlohmann::json value = *result; // Can be null legitimately
```

### Step 3: Enable Compiler-Enforced Error Checking

The new `Result<T>` pattern uses `[[nodiscard]]`, so the compiler will warn if you ignore return values:

```cpp
// Compiler warning: ignoring return value
engine.executeJoin(...); 

// Correct:
auto result = engine.executeJoin(...);
if (!result) { /* handle error */ }
```

## Benefits

1. **Type Safety**: Errors can't be ignored
2. **Clear Semantics**: No confusion between null and error
3. **Better Error Messages**: Full context in error objects
4. **Unified Pattern**: Consistent across all query APIs

## Troubleshooting

### Compiler Error: "No matching function"
**Cause:** Trying to use old API signature  
**Solution:** Update to use `Result<T>` return type

### Compiler Warning: "Ignoring return value"
**Cause:** Not checking `Result<T>` return value  
**Solution:** Add error checking with `if (!result)`

### Runtime Error: "Dereferencing empty Result"
**Cause:** Accessing `*result` without checking  
**Solution:** Always check `if (!result)` before dereferencing
```

## ✅ Acceptance Criteria

- [ ] All API documentation updated with Result<T> examples
- [ ] Error handling guide includes query engine patterns
- [ ] Code examples updated and tested
- [ ] Migration guide created with step-by-step instructions
- [ ] CHANGELOG.md updated with breaking changes
- [ ] README.md updated if needed
- [ ] All documentation builds without errors
- [ ] Documentation reviewed for clarity and accuracy

## 📚 Resources

- **Migration Summary:** `PHASE4_FINAL_SUMMARY.md`
- **Phase Details:** `PHASE4A_EXPRESSION_EVALUATOR_COMPLETE.md`, etc.
- **Error Infrastructure:** `include/utils/expected.h`

## 🔗 Related Issues

- Depends on: Phase 4A-4C merge
- Related to: Error Handling Documentation

## 💡 Notes

- Focus on practical examples
- Include troubleshooting section
- Provide before/after comparisons
- Link to error code reference
- Consider creating video tutorial
