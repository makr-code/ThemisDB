# SPRINT 8: MOVE SEMANTICS REMEDIATION — PHASE 1A IMPLEMENTATION GUIDE

## Executive Summary

This document defines the systematic remediation of move semantics gaps across the LLM and Query modules in ThemisDB. This addresses CWE-457 (Use of Uninitialized Variable), CWE-415 (Double Free), and CWE-672 (Use After Free) vulnerabilities that arise from incomplete move semantics implementations.

**Target:** 22 critical gaps (12 LLM + 10 Query)  
**Scope:** Resource-holding classes with destructors but missing move operations  
**Effort:** ~40-50 hours of systematic remediation  

---

## IDENTIFIED GAPS & REMEDIATION STRATEGY

### Category A: Classes with Destructor + Unique_ptr (Require Move Semantics)

These classes **MUST** have move constructor and move assignment operator:

#### LLM Module (8 files)

1. **MultiLoRAManager** (`include/llm/multi_lora_manager.h`)
   - Members: `std::unique_ptr<std::thread> eviction_thread_`
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

2. **MultiGPUMemoryCoordinator** (`include/llm/multi_gpu_memory_coordinator.h`)
   - Members: `std::unique_ptr<...>` (GPU memory objects)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

3. **LLMPluginManager** (`include/llm/llm_plugin_manager.h`)
   - Members: `std::unique_ptr<...>` (plugin instances)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

4. **InlineTrainingEngine** (`include/llm/inline_training_engine.h`)
   - Members: `std::unique_ptr<...>` (training contexts)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

5. **LLMDeploymentPlugin** (`include/llm/llm_deployment_plugin.h`)
   - Members: `std::unique_ptr<...>` (deployment resources)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

6. **AsyncInferenceEngine** (`include/llm/async_inference_engine.h`)
   - Members: `std::unique_ptr<...>` (inference queues)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

7. **FederatedInferenceCoordinator** (`include/llm/federated_inference_coordinator.h`)
   - Members: `std::unique_ptr<...>` (federated state)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

8. **DistributedTrainingCoordinator** (`include/llm/distributed_training_coordinator.h`)
   - Members: `std::unique_ptr<...>` (training coordinator state)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

#### Query Module (6 files)

1. **AQLParserService** (`include/query/aql_parser_service.h`)
   - Members: `std::unique_ptr<...>` (parser instances)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

2. **ParallelExecutor** (`include/query/parallel_executor.h`)
   - Members: `std::unique_ptr<...>` (thread pool, execution state)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

3. **ContinuousQueryEngine** (`include/query/continuous_query_engine.h`)
   - Members: `std::unique_ptr<...>` (CQ state machines)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

4. **QueryEngine** (`include/query/query_engine.h`)
   - Members: `std::unique_ptr<...>` (optimizer, executor)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

5. **QueryProfiler** (`include/query/query_profiler.h`)
   - Members: `std::unique_ptr<...>` (profiling data)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

6. **QueryFederation** (`include/query/query_federation.h`)
   - Members: `std::unique_ptr<...>` (federation state)
   - Fix: Add move constructor + move assignment operator
   - Status: ⏳ PENDING

---

### Category B: Classes with Destructor + Mutex (Delete Move Operations)

These classes **MUST** explicitly DELETE move operations (mutexes are non-moveable):

#### LLM Module (4 files)

1. **TokenQuotaManager** (`include/llm/token_quota_manager.h`)
   - Members: `std::mutex`
   - Fix: Add `= delete;` for move constructor and move assignment
   - Status: ⏳ PENDING

2. **DecisionRecordYAMLProcessor** (`include/llm/decision_record_yaml_processor.h`)
   - Members: `std::mutex`
   - Fix: Add `= delete;` for move constructor and move assignment
   - Status: ⏳ PENDING

3. **LLMPrefixCache** (`include/llm/llm_prefix_cache.h`)
   - Members: `std::mutex`
   - Fix: Add `= delete;` for move constructor and move assignment
   - Status: ⏳ PENDING

4. **LLMResponseCache** (`include/llm/llm_response_cache.h`)
   - Members: `std::mutex`
   - Fix: Add `= delete;` for move constructor and move assignment
   - Status: ⏳ PENDING

#### Query Module (4 files)

1. **PlanCache** (`include/query/plan_cache.h`)
   - Members: `std::mutex`
   - Fix: Add `= delete;` for move constructor and move assignment
   - Status: ⏳ PENDING

2. **QueryCacheManager** (`include/query/query_cache_manager.h`)
   - Members: `std::mutex`
   - Fix: Add `= delete;` for move constructor and move assignment
   - Status: ⏳ PENDING

3. **MaterializedView** (`include/query/materialized_view.h`)
   - Members: `std::mutex`
   - Fix: Add `= delete;` for move constructor and move assignment
   - Status: ⏳ PENDING

4. **RuntimeReoptimizer** (`include/query/runtime_reoptimizer.h`)
   - Members: `std::mutex`
   - Fix: Add `= delete;` for move constructor and move assignment
   - Status: ⏳ PENDING

---

### Category C: Classes with Destructor Only (Likely Need Move Semantics)

These classes have destructors but no obvious resource-holding members in headers. Add move semantics if needed per implementation.

#### LLM Module (7 files)

- ContinuousBatchScheduler
- EthicalGuidelinesManager
- GGUFLoader
- LoRARouter
- ThemisToolInterface
- BlockTable
- AIDecisionAuditor

#### Query Module (7 files)

- AQLSafetyValidator
- IncrementalView
- AQLParser
- SemanticCache
- ResultStream
- ApproximateAggregator
- GraphQLDialect
- QueryCanceller

---

## IMPLEMENTATION PATTERN

### Pattern 1: Classes with Unique Pointers (Category A)

```cpp
// In header file (.h)
class MyResourceClass {
public:
    // Destructor
    ~MyResourceClass();
    
    // Move constructor
    MyResourceClass(MyResourceClass&& other) noexcept;
    
    // Move assignment operator
    MyResourceClass& operator=(MyResourceClass&& other) noexcept;
    
    // Delete copy operations (Rule of Five)
    MyResourceClass(const MyResourceClass&) = delete;
    MyResourceClass& operator=(const MyResourceClass&) = delete;
    
private:
    std::unique_ptr<std::thread> thread_;
    std::unique_ptr<OtherResource> resource_;
};

// In implementation file (.cpp)
MyResourceClass::MyResourceClass(MyResourceClass&& other) noexcept
    : thread_(std::move(other.thread_)),
      resource_(std::move(other.resource_)) {
}

MyResourceClass& MyResourceClass::operator=(MyResourceClass&& other) noexcept {
    if (this != &other) {
        thread_ = std::move(other.thread_);
        resource_ = std::move(other.resource_);
    }
    return *this;
}
```

### Pattern 2: Classes with Mutexes (Category B)

```cpp
// In header file (.h)
class MutexedResourceClass {
public:
    // Destructor
    ~MutexedResourceClass();
    
    // Explicitly DELETE move operations
    MutexedResourceClass(MutexedResourceClass&&) = delete;
    MutexedResourceClass& operator=(MutexedResourceClass&&) = delete;
    
    // Delete copy operations as well
    MutexedResourceClass(const MutexedResourceClass&) = delete;
    MutexedResourceClass& operator=(const MutexedResourceClass&) = delete;
    
private:
    std::mutex lock_;  // Non-moveable
};
```

### Pattern 3: Noexcept Specification

Move operations should be marked `noexcept` when possible:

```cpp
// Good: Noexcept because underlying operations don't throw
MyClass(MyClass&& other) noexcept : member_(std::move(other.member_)) {}
MyClass& operator=(MyClass&& other) noexcept {
    if (this != &other) {
        member_ = std::move(other.member_);
    }
    return *this;
}

// Still valid: Without noexcept if complex operations might throw
// But document why in comments
```

---

## TESTING STRATEGY

### Unit Tests (in `tests/llm/test_move_semantics_llm.cpp` and `tests/query/test_move_semantics_query.cpp`)

1. **Move Constructor Test**: Verify ownership transfer, source is in valid state
2. **Move Assignment Test**: Verify cleanup of destination, no double-free
3. **Self-Assignment Test**: Verify `a = std::move(a)` is safe
4. **Moved-From Validity Test**: Verify moved-from object can be destroyed/reassigned (CWE-457)
5. **Stress Tests**: Many moves without leaks (CWE-415 fix validation)
6. **Chained Moves**: Verify use-after-move is impossible (CWE-672 fix validation)

### Integration Tests

1. Use classes in `std::vector` (requires move semantics)
2. Use classes in `std::unique_ptr<T>` and move between contexts
3. Return classes from functions (RVO + move optimization)

---

## VERIFICATION CHECKLIST

For each file being fixed, verify:

- [ ] Class has destructor
- [ ] Class has move constructor defined (or deleted)
- [ ] Class has move assignment operator defined (or deleted)
- [ ] Copy constructor/assignment handled (either defined or deleted)
- [ ] Move constructor marked `noexcept` if appropriate
- [ ] Move assignment marked `noexcept` if appropriate
- [ ] Moved-from state is valid (no dangling pointers)
- [ ] No self-assignment issues in move assignment
- [ ] Proper use of `std::move()` in implementations
- [ ] Unit tests added for move semantics
- [ ] All existing tests still pass

---

## CWE MAPPINGS

| CWE | Description | Fix Pattern |
|-----|-------------|------------|
| CWE-457 | Use of Uninitialized Variable | Ensure moved-from objects have valid state |
| CWE-415 | Double Free | Ensure move assignment properly cleans up destination |
| CWE-672 | Use After Free | Verify moved-from objects are not accessed |

---

## COMPLETION CRITERIA

Phase 1A is complete when:

1. ✅ All 22 identified files have move semantics properly implemented
2. ✅ 30+ move semantics unit tests written and passing
3. ✅ All existing tests still pass
4. ✅ No memory leaks detected in stress tests
5. ✅ Code review completed with security team
6. ✅ Documentation updated in Doxygen comments
7. ✅ Changelog updated

---

## REFERENCES

- **Modern C++ (C++17+)**: https://en.cppreference.com/w/cpp/language/move_semantics
- **Rule of Five**: https://en.cppreference.com/w/cpp/language/rule_of_three
- **Noexcept Specification**: https://en.cppreference.com/w/cpp/language/noexcept_spec
- **std::move**: https://en.cppreference.com/w/cpp/utility/move
- **CWE-457**: https://cwe.mitre.org/data/definitions/457.html
- **CWE-415**: https://cwe.mitre.org/data/definitions/415.html
- **CWE-672**: https://cwe.mitre.org/data/definitions/672.html
