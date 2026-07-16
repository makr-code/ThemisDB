### Context

This issue implements the roadmap item 'Dynamic Adapter Reconfiguration' for the core domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Dynamic Adapter Reconfiguration

### Goal

Deliver the scoped changes for Dynamic Adapter Reconfiguration in src/core/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Dynamic Adapter Reconfiguration
**Priority:** High  
**Target Version:** v1.6.0

Enable runtime switching of adapters without restarting the database.

```cpp
// Future API
context->replaceLogger(new_logger_adapter);
context->reloadMetricsConfig(new_config);
```

**Benefits:**
- Zero-downtime logging level changes
- Switch between tracing backends without restart
- Enable/disable metrics dynamically

**Implementation Considerations:**
- Thread-safe adapter swapping
- Graceful handling of in-flight operations
- Configuration validation before swap

---

### Acceptance Criteria

- [ ] Zero-downtime logging level changes
- [ ] Switch between tracing backends without restart
- [ ] Enable/disable metrics dynamically
- [ ] Thread-safe adapter swapping
- [ ] Graceful handling of in-flight operations
- [ ] Configuration validation before swap

### Relationships

- Roadmap row: #63 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/core/FUTURE_ENHANCEMENTS.md#dynamic-adapter-reconfiguration
- Source key: roadmap:63:core:v1.6.0:dynamic-adapter-reconfiguration

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:63:core:v1.6.0:dynamic-adapter-reconfiguration -->
<!-- roadmap-ref: row=63;module=core;target=v1.6.0 -->
<!-- roadmap-detail: src/core/FUTURE_ENHANCEMENTS.md#dynamic-adapter-reconfiguration -->
