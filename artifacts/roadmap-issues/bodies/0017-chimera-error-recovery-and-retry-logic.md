### Context

This issue implements the roadmap item 'Error Recovery and Retry Logic' for the chimera domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.1.0.

Primary detail section: Error Recovery and Retry Logic

### Goal

Deliver the scoped changes for Error Recovery and Retry Logic in src/chimera/ and complete the linked detail section in a release-ready state for v1.1.0.

### Detailed Scope

### Error Recovery and Retry Logic
**Priority:** High  
**Target Version:** v1.1.0

Automatic error recovery for transient failures.

**Features:**
- Exponential backoff retry
- Circuit breaker pattern
- Health check on retry
- Configurable retry policies
- Error classification (transient vs permanent)

**API:**
```cpp
struct RetryPolicy {
    size_t max_retries = 3;
    std::chrono::milliseconds initial_delay = std::chrono::milliseconds(100);
    double backoff_multiplier = 2.0;
    std::chrono::milliseconds max_delay = std::chrono::seconds(10);
    
    // Error classification
    std::function<bool(ErrorCode)> is_transient;
};

class ConnectionWithRetry {
public:
    ConnectionWithRetry(
        std::unique_ptr<IDatabaseAdapter> adapter,
        const RetryPolicy& policy
    );
    
    // Execute with automatic retry
    template<typename T>
    Result<T> execute_with_retry(std::function<Result<T>()> operation);
};
```

**Usage:**
```cpp
RetryPolicy policy;
policy.max_retries = 5;
policy.is_transient = [](ErrorCode code) {
    return code == ErrorCode::TIMEOUT || 
           code == ErrorCode::CONNECTION_ERROR;
};

ConnectionWithRetry conn(std::move(adapter), policy);

// Automatically retries on transient errors
auto result = conn.execute_with_retry([&]() {
    return adapter->execute_query(query);
});
```

---

### Acceptance Criteria

- [x] Exponential backoff retry
- [x] Circuit breaker pattern
- [x] Health check on retry
- [x] Configurable retry policies
- [x] Error classification (transient vs permanent)

### Relationships

- Roadmap row: #17 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/chimera/FUTURE_ENHANCEMENTS.md#error-recovery-and-retry-logic
- Source key: roadmap:17:chimera:v1.1.0:error-recovery-and-retry-logic

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:17:chimera:v1.1.0:error-recovery-and-retry-logic -->
<!-- roadmap-ref: row=17;module=chimera;target=v1.1.0 -->
<!-- roadmap-detail: src/chimera/FUTURE_ENHANCEMENTS.md#error-recovery-and-retry-logic -->
