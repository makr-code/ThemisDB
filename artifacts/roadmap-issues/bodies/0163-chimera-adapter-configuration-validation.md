### Context

This issue implements the roadmap item 'Adapter Configuration Validation' for the chimera domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.2.0.

Primary detail section: Adapter Configuration Validation

### Goal

Deliver the scoped changes for Adapter Configuration Validation in src/chimera/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### Adapter Configuration Validation
**Priority:** Medium  
**Target Version:** v1.2.0

Validate adapter configuration before connection.

**Features:**
- Configuration schema validation
- Required parameter checking
- Type validation
- Range checking
- Connection string parsing

**API:**
```cpp
struct AdapterConfig {
    std::string connection_string;
    std::map<std::string, Scalar> options;
    
    // Validate configuration
    Result<bool> validate() const;
    
    // Get validation errors
    std::vector<std::string> get_validation_errors() const;
};

// Usage
AdapterConfig config;
config.connection_string = "themisdb://localhost:8529/db";
config.options["pool_size"] = 50;
config.options["timeout_ms"] = 30000;

if (!config.validate().is_ok()) {
    for (const auto& error : config.get_validation_errors()) {
        std::cerr << "Config error: " << error << std::endl;
    }
}
```

---

### Acceptance Criteria

- [ ] Configuration schema validation
- [ ] Required parameter checking
- [ ] Type validation
- [ ] Range checking
- [ ] Connection string parsing

### Relationships

- Roadmap row: #163 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/chimera/FUTURE_ENHANCEMENTS.md#adapter-configuration-validation
- Source key: roadmap:163:chimera:v1.2.0:adapter-configuration-validation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:163:chimera:v1.2.0:adapter-configuration-validation -->
<!-- roadmap-ref: row=163;module=chimera;target=v1.2.0 -->
<!-- roadmap-detail: src/chimera/FUTURE_ENHANCEMENTS.md#adapter-configuration-validation -->
