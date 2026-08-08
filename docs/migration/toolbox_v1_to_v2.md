# Migrating to Toolbox v2.0 (Q4 2026)

## Overview

Toolbox v2.0 is the first production release (GA) of the toolbox module, completing Phase 1-6 comprehensive hardening and validation.

- **Release Date:** 2026-12-31
- **Previous Version:** v1.x (alpha)
- **Migration Scope:** First production release; minimal breaking changes
- **Estimated Migration Effort:** Low (1-2 hours for typical integration)

---

## Deprecations

**None.** This is the first production release; no previous stable API to deprecate.

---

## Breaking Changes

### 1. Builder is Now Single-Use

**Previous Behavior (v1.x):**
```cpp
auto builder = ToolboxBuilder::create();
auto toolbox1 = builder.build();  // OK
auto toolbox2 = builder.build();  // OK (could call twice)
```

**New Behavior (v2.0):**
```cpp
auto builder = ToolboxBuilder::create();
auto toolbox = builder.build();   // OK
auto toolbox2 = builder.build();  // THROWS: builder_already_used
```

**Mitigation:**
- Create new builder for each toolbox instance
- ✅ **Recommended Pattern:**
  ```cpp
  auto toolbox1 = ToolboxBuilder::create()
    .withContentManager(cm1)
    .build();
  
  auto toolbox2 = ToolboxBuilder::create()
    .withContentManager(cm2)
    .build();
  ```

**Rationale:** Enforces explicit resource lifecycle; prevents accidental reuse of consumed builder.

### 2. Registry Requires Explicit Initialization

**Previous Behavior (v1.x):**
```cpp
// Implicit global registry initialization (unsafe)
auto toolbox = IngestionToolbox::create();  // Registry initialized internally
```

**New Behavior (v2.0):**
```cpp
// Explicit registry initialization required
ToolboxRegistry::initialize(IngestionToolbox::createDefault());  // Must call before use
auto toolbox = IngestionToolbox::createDefault();  // Registry already initialized
```

**Mitigation:**
- Call `ToolboxRegistry::initialize(...)` at application startup, BEFORE first toolbox operation
- ✅ **Recommended Initialization Pattern:**
  ```cpp
  // In main() or application startup
  auto toolbox = IngestionToolbox::createDefault();
  ToolboxRegistry::initialize(toolbox);
  
  // Later, in request handlers or worker threads
  auto request_toolbox = ToolboxRegistry::instance();  // Safe to use
  ```

**Rationale:** Explicit initialization enforces fail-closed semantics; application startup fails immediately if registry configuration invalid.

### 3. Bridge Error Handling is Explicit

**Previous Behavior (v1.x):**
```cpp
// Silent soft-failure on writer error (hard to debug)
bridge.enrich(entity);  // May fail silently
```

**New Behavior (v2.0):**
```cpp
// Explicit error codes + Prometheus metrics
auto result = bridge.enrich(entity);
if (result.incident_code() == "BR-WRITER") {
  // Writer sink failed; logged + incremented toolbox_bridge_failures_total
  log.warn("[TOOLBOX] Bridge write failure: {}", result.incident_code());
}
```

**Mitigation:**
- Check bridge result incident codes (BR-NO-TEXT, BR-WRITER, BR-TOOLBOX, BR-EMPTY)
- Monitor `toolbox_bridge_failures_total` Prometheus counter per incident type
- Set up alerting: if `toolbox_bridge_failures_total[BR-WRITER]` > 50/min, escalate

**Rationale:** Observable soft-fail behavior enables production diagnostics and alerting.

---

## API Changes Summary

| Component | v1.x | v2.0 | Impact |
|-----------|------|------|--------|
| **ToolboxBuilder** | Reusable | Single-use | **BREAKING** - Must create new builder for each build() |
| **ToolboxRegistry** | Implicit init | Explicit init() required | **BREAKING** - Must call initialize() at startup |
| **Bridge Error Handling** | Silent soft-fail | Explicit incident codes + metrics | **IMPROVED** - Better observability |
| **Incident Taxonomy** | N/A | 16 codes (EX-*, BR-*, REG-*, HLP-*) | **NEW** - Unified error taxonomy |
| **Prometheus Metrics** | Limited | Comprehensive (all layers) | **NEW** - Production observability |
| **Performance Gates** | Proxy only | Native benchmarks + gates | **NEW** - Validated performance budgets |

---

## Migration Checklist

### Code Changes Required

- [ ] **Find all ToolboxRegistry::instance() calls**
  ```bash
  grep -r "ToolboxRegistry::instance()" src/
  ```
  Ensure initialize() is called BEFORE first use.

- [ ] **Find all ToolboxBuilder uses**
  ```bash
  grep -r "ToolboxBuilder::" src/
  ```
  Verify each builder is created fresh for each build() call.

- [ ] **Add registry initialization at application startup**
  ```cpp
  // In main() or service initialization
  auto config = LoadToolboxConfig(config_path);
  ToolboxRegistry::instance().initialize(config);
  ```

- [ ] **Update bridge error handling**
  ```cpp
  auto result = bridge.enrich(entity);
  if (result.incident_code() != "BR-EMPTY") {  // Non-empty result
    // Process result
  } else {
    log.warn("[TOOLBOX] Bridge returned no entities (BR-EMPTY)");
  }
  ```

### Configuration Changes Required

- [ ] **Create/update toolbox.yaml** with deployment configuration:
  ```yaml
  toolbox:
    extraction_worker_threads: 4
    registry_max_memory_mb: 256
    metrics_port: 9090
    log_level: "SPDLOG_LEVEL_INFO"
  ```

- [ ] **Enable Prometheus metrics scraping**
  ```yaml
  # In prometheus.yml
  scrape_configs:
    - job_name: 'themisdb-toolbox'
      static_configs:
        - targets: ['localhost:9090']
  ```

- [ ] **Set up alerting rules** (example: alert if extraction failures spike)
  ```yaml
  # prometheus/alerts.yml
  groups:
    - name: toolbox_alerts
      rules:
        - alert: ToolboxExtractionFailureSpike
          expr: rate(toolbox_extraction_failures_total[5m]) > 100
          for: 1m
          annotations:
            summary: "Toolbox extraction failures spiked"
  ```

### Testing & Validation

- [ ] **Build with v2.0 configuration:**
  ```bash
  cmake --preset linux-release
  cmake --build --preset linux-release
  ```

- [ ] **Run toolbox test suite:**
  ```bash
  ctest --preset linux-release -L toolbox -VV
  ```

- [ ] **Verify performance gates pass:**
  ```bash
  ./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads
  ```

- [ ] **Check Prometheus metrics are exported:**
  ```bash
  curl http://localhost:9090/metrics | grep toolbox
  ```

---

## Code Migration Examples

### Example 1: Single Toolbox Instance

**Before (v1.x):**
```cpp
#include "toolbox/toolbox_builder.h"

int main() {
  auto builder = ToolboxBuilder::create()
    .withContentManager(content_manager)
    .withTextExtractor(extractor);
  
  auto toolbox = builder.build();
  // Use toolbox...
}
```

**After (v2.0):**
```cpp
#include "toolbox/toolbox_builder.h"
#include "toolbox/toolbox_registry.h"

int main() {
  // 1. Initialize registry at startup
  auto config = LoadToolboxConfig("/etc/themisdb/toolbox.yaml");
  ToolboxRegistry::instance().initialize(config);
  
  // 2. Create toolbox with new builder
  auto toolbox = ToolboxBuilder::create()
    .withContentManager(content_manager)
    .withTextExtractor(extractor)
    .build();
  
  // 3. Use toolbox...
}
```

### Example 2: Multiple Toolbox Instances

**Before (v1.x):**
```cpp
auto builder = ToolboxBuilder::create()
  .withContentManager(cm1)
  .withTextExtractor(ex1);

auto toolbox1 = builder.build();
auto toolbox2 = builder.build();  // Reuse builder (now unsupported)
```

**After (v2.0):**
```cpp
// Create new builder for each instance
auto toolbox1 = ToolboxBuilder::create()
  .withContentManager(cm1)
  .withTextExtractor(ex1)
  .build();

auto toolbox2 = ToolboxBuilder::create()
  .withContentManager(cm2)
  .withTextExtractor(ex2)
  .build();
```

### Example 3: Error Handling

**Before (v1.x):**
```cpp
// Silent soft-failure; hard to debug
bridge.enrich(entity);  // May fail without notice
```

**After (v2.0):**
```cpp
auto result = bridge.enrich(entity);

// Explicit error handling per incident type
switch (result.incident_code()) {
  case "BR-EMPTY":
    log.info("[TOOLBOX] Bridge returned no entities");
    // Continue with empty result
    break;
  case "BR-WRITER":
    log.error("[TOOLBOX] Bridge writer failed; soft-fail");
    metrics.increment("bridge_writer_failures");
    break;
  case "BR-TOOLBOX":
    log.error("[TOOLBOX] Toolbox operation failed");
    break;
  default:
    // Success or unknown result
    break;
}

// Alternatively, check Prometheus metrics
// if rate(toolbox_bridge_failures_total[5m]) > 50, alert
```

### Example 4: Initialization Error Handling

**Before (v1.x):**
```cpp
// Implicit initialization; errors hidden
auto toolbox = IngestionToolbox::create();  // Fails silently
```

**After (v2.0):**
```cpp
// Explicit initialization with error handling
try {
  auto config = LoadToolboxConfig(config_path);
  ToolboxRegistry::instance().initialize(config);
  
  auto toolbox = IngestionToolbox::create();  // Guaranteed safe
} catch (const std::exception& e) {
  log.error("[TOOLBOX] Initialization failed: {}", e.what());
  return false;  // Fail-closed
}
```

---

## Rollback Procedure

If you encounter issues after upgrading to v2.0:

### Option 1: Revert to v1.x (Temporary)

```bash
# Checkout previous version
git checkout toolbox-v1

# Rebuild
cmake --preset linux-release
cmake --build --preset linux-release
```

### Option 2: Diagnose v2.0 Issue

1. **Check Prometheus metrics for error spikes:**
   ```bash
   # Get last 5 minutes of errors
   curl 'http://localhost:9090/api/v1/query' --data-urlencode \
     'query=rate(toolbox_extraction_failures_total[5m])'
   ```

2. **Check logs for incident codes:**
   ```bash
   grep "\[TOOLBOX\] Error:" /var/log/themisdb/toolbox.log | tail -20
   ```

3. **Contact support with:**
   - Incident code and frequency
   - Hardware specifications
   - Build configuration (cmake --preset)
   - Reproducible test case

---

## Performance Migration Notes

### Regression Budget

v2.0 performance gates allow ±10% variance vs Q3 2026 baseline:

| Gate | Baseline | Budget | Lower Bound | Upper Bound |
|------|----------|--------|-------------|-------------|
| GATE-TBX-P1 | 125K ops/s | ±10% | 112.5K | 137.5K |
| GATE-TBX-P2 | 42ms | ±10% | 37.8ms | 46.2ms |
| GATE-TBX-P3 | 8ms | ±10% | 7.2ms | 8.8ms |
| GATE-TBX-P4 | 12ms | ±10% | 10.8ms | 13.2ms |
| GATE-TBX-P5 | 1.2M ops/s | ±10% | 1.08M | 1.32M |
| GATE-TBX-P6 | 78ms | ±10% | 70.2ms | 85.8ms |

**If performance exceeds budget:**
1. Verify hardware specifications match baseline (CPU cores, RAM)
2. Run regression validation: `./benchmarks/toolbox/bench_toolbox_native_workloads --benchmark_min_time=5s`
3. Compare with baseline: see `src/toolbox/PERFORMANCE_EXPECTATIONS.md` for procedure
4. If regression > 10%, file issue with benchmark results

### Thread Pool Tuning

v2.0 includes configurable thread pools for performance optimization:

```yaml
toolbox:
  extraction_worker_threads: 4       # Typical: CPU_cores / 2
  helper_worker_threads: 2           # Typical: CPU_cores / 4
  bridge_worker_threads: 2           # Typical: 1-2 (I/O bound)
```

**Tuning Guidance:**
- Start with defaults (2-4 threads)
- Monitor CPU utilization (target 60-80%)
- Increase threads if utilization < 50%; decrease if > 90%
- Re-run performance gates after tuning

---

## Support and Troubleshooting

### Common Issues

**Issue:** `REG-NOT-INIT` exception on first toolbox operation

**Solution:** Ensure `ToolboxRegistry::instance().initialize(config)` is called at application startup, BEFORE any toolbox operations.

**Issue:** `BR-WRITER` failures increasing over time

**Solution:** Check graph/vector writer health; verify database connections; monitor `toolbox_bridge_latency_us` histogram for latency spikes.

**Issue:** Performance gate failures (regression > 10%)

**Solution:** Run baseline collection on reference hardware; compare with Q3 2026 baseline in `PERFORMANCE_EXPECTATIONS.md`; file issue with results.

### Resources

- **PRODUCTION_REQUIREMENTS.md** - Deployment configuration and operational runbooks
- **PERFORMANCE_EXPECTATIONS.md** - Performance gates, baseline collection, regression validation
- **SECURITY.md** - Incident taxonomy and threat model
- **tests/toolbox/README.md** - Test suite and incident coverage
- **benchmarks/toolbox/README.md** - Benchmark guide and gate validation

---

## Timeline

- **2026-12-31:** v2.0 GA Release
- **Q1 2027:** Production deployment window and support period
- **Q1 2027:** Baseline refresh on new hardware profiles (optional)
- **Future:** v2.1+ maintenance releases per roadmap

---

## Questions?

For migration questions or issues:

1. Check PRODUCTION_REQUIREMENTS.md § C. Operational Runbooks
2. Review incident codes in SECURITY.md for error diagnosis
3. Check tests/toolbox/README.md for test execution examples
4. Monitor Prometheus metrics for production diagnostics

---

**Last Updated:** 2026-08-07  
**Release:** Q4 2026 (v2.0.0)  
**Status:** Production Ready ✅
