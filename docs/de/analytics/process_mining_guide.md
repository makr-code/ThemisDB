# Process Mining Guide

**Version:** v1.3.0 Phase 2  
**Status:** Production-Ready  
**Last Updated:** December 22, 2025

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
- **Bottleneck Detection**: Find performance issues and delays
- **Social Network Mining**: Analyze collaboration patterns between resources

### Conformance Checking
- Token replay for fitness calculation
- Deviation detection and analysis
- Model-reality alignment

### Export Formats
- BPMN 2.0 XML for standard process modeling
- Petri Net (PNML) for formal analysis
- JSON for programmatic access

## Usage Examples

### Basic Process Discovery

```cpp
ProcessMining mining(db);

// Extract event log from collection
EventLogConfig config;
config.case_id_field = "order_id";
config.activity_field = "action";
config.timestamp_field = "timestamp";

auto event_log = mining.extractEventLog("audit_log", config);

// Discover process model
auto model = mining.discoverProcess(event_log, MiningAlgorithm::HEURISTIC);

// Export to BPMN
std::string bpmn = mining.exportToBPMN(model);
```

### Variant Analysis

```cpp
auto variants = mining.analyzeVariants(event_log);

// Find most common process variant
auto most_common = variants[0];
std::cout << "Most common variant occurs " << most_common.frequency << " times" << std::endl;
```

### Bottleneck Detection

```cpp
auto bottlenecks = mining.detectBottlenecks(event_log, 0.9); // 90th percentile

for (const auto& bottleneck : bottlenecks) {
    std::cout << "Bottleneck: " << bottleneck.activity 
              << " (avg: " << bottleneck.avg_duration_ms << "ms)" << std::endl;
}
```

### Social Network Analysis

```cpp
auto social_network = mining.extractSocialNetwork(event_log);
auto handovers = mining.analyzeHandovers(social_network);

// Analyze collaboration patterns
auto metrics = mining.calculateCollaborationMetrics(social_network);
```

---

## Related Documentation

- [Future Enhancements](./bpmn_future_enhancements.md) - Planned improvements for BPMN/process engine

See full documentation at https://github.com/makr-code/ThemisDB

---

**Last Updated:** December 22, 2025  
**Version:** v1.3.0 Phase 2  
**Status:** Production-Ready
