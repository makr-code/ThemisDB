# Process Mining Guide

**Version:** v1.7.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-04-06

---

## Overview

ThemisDB's Process Mining module enables automated discovery and analysis of business processes from event log data. It provides a comprehensive suite of algorithms for process discovery, conformance checking, and performance enhancement.

## Key Features

### Process Discovery Algorithms

- **Alpha Miner**: Classical algorithm for structured processes
- **Heuristic Miner**: Robust algorithm handling noise and exceptions
- **Inductive Miner**: Sound process models with guaranteed fitness

### Analysis Capabilities

- **Directly-Follows Graph (DFG)**: Visualize process flows with frequencies
- **Variant Analysis**: Identify common and rare process execution patterns
- **Bottleneck Detection**: Find performance issues and delays via performance enhancement

### Conformance Checking

- Token replay for fitness calculation (fast; measures how well the model explains the log)
- Alignment-based conformance (precise, but slower; finds minimal-cost edit sequence between model and log)
- Deviation detection and analysis

### Export Formats

- BPMN 2.0 XML for standard process modeling
- Petri Net (PNML) for formal analysis
- ThemisDB process definition (saved to `_process_definitions`)

## Usage Examples

### Basic Process Discovery

```cpp
ProcessMining mining(db);  // requires RocksDBWrapper&

// Extract event log from collection
EventLogConfig config;
config.case_id_field   = "order_id";
config.activity_field  = "action";
config.timestamp_field = "timestamp";

auto [status1, event_log] = mining.extractEventLog("audit_log", config);
if (!status1.ok) { /* handle error */ }

// Discover process model (MiningConfig wraps MiningAlgorithm)
MiningConfig mcfg;
mcfg.algorithm = MiningAlgorithm::HEURISTIC;

auto [status2, model] = mining.discoverProcess(event_log, mcfg);

// Export to BPMN
auto [status3, bpmn] = mining.exportToBPMN(model);
```

### Variant Analysis

```cpp
auto [status, variants] = mining.analyzeVariants(event_log);

// Find most common process variant
if (!variants.empty()) {
    const auto& most_common = variants[0]; // sorted by frequency desc
    std::cout << "Most common variant occurs "
              << most_common.frequency << " times ("
              << most_common.percentage << "%)" << std::endl;
}
```

### Bottleneck Detection

Bottleneck detection requires a performance-enhanced process model:

```cpp
// First enhance model with performance data
auto [s1, enhanced] = mining.enhanceWithPerformance(model, event_log);

// Then detect bottlenecks (returns names of bottleneck nodes)
auto [s2, bottleneck_nodes] = mining.detectBottlenecks(enhanced, 0.9); // 90th percentile

for (const auto& node_name : bottleneck_nodes) {
    std::cout << "Bottleneck node: " << node_name << std::endl;
    // Performance details are in enhanced.node_avg_duration[node_name]
}
```

### Conformance Checking

```cpp
auto [s, conformance] = mining.checkConformance(event_log, model);
std::cout << "Fitness: " << conformance.fitness << "\n";
for (const auto& dev : conformance.deviations)
    std::cout << "Deviation: " << dev << "\n";
```

---

## Related Documentation

- [Analytics Docs Hub](./README.md)
- [PROCESS_MINING_AQL_EXAMPLES](./PROCESS_MINING_AQL_EXAMPLES.md)
- [BPMN Future Enhancements](./BPMN_FUTURE_ENHANCEMENTS.md)
- [OLAP Guide](./olap_guide.md)
- [Forecasting Guide](./forecasting_guide.md)
- [API Reference](../../../include/analytics/README.md)
- [Implementation Overview](../../../src/analytics/README.md)

---

**Last Updated:** 2026-04-06
**Version:** v1.7.0
**Status:** 🟢 Production-Ready
