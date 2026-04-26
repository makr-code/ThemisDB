# BPMN/Process Engine - Future Enhancements

**Version:** 1.0  
**Date:** February 9, 2026  
**Status:** Planning

---

## Overview

This document outlines planned future enhancements for ThemisDB's BPMN/process engine and wire protocol. These improvements aim to enhance performance, accuracy, and integration capabilities.

---

## Planned Enhancements

### 1. Migrate Wire Protocol from JSON to Binary Protobuf Serialization

**Current State:**  
The wire protocol currently uses JSON for message serialization.

**Planned Enhancement:**  
Migrate to binary Protocol Buffers (protobuf) for improved performance and efficiency.

**Benefits:**
- **Performance**: Binary serialization is significantly faster than JSON parsing
- **Size**: Smaller message payloads reduce network overhead
- **Type Safety**: Strong typing and schema validation at compile time
- **Backward Compatibility**: Protobuf supports schema evolution

**References:**
- [Wire Protocol v1 Specification](../architecture/wire_protocol_v1.md)
- [Binary Protocol Buffers](../architecture/BINARY_PROTOCOL_BUFFERS.md)

---

### 2. Add Individual Node Visit Timestamp Tracking

**Current State:**  
The process history events currently use token creation time for all history events, which doesn't accurately reflect when each individual node was visited during execution.

**Planned Enhancement:**  
Implement individual node visit timestamp tracking to capture the actual time each process node is visited during execution.

**Benefits:**
- **Accurate Metrics**: Precise timing information for each process step
- **Better Analytics**: Improved bottleneck detection and performance analysis
- **Detailed Audit Trail**: Complete temporal tracking of process execution
- **SLA Monitoring**: Accurate measurement of time spent at each process node

**Implementation Notes:**
- Add `visited_at` timestamp field to process history events
- Update token advancement logic to record timestamp for each node visit
- Extend process mining queries to leverage individual node timestamps

**Current Code Location:**
- `src/index/process_graph.cpp` - Token creation and advancement logic
- `include/index/process_graph.h` - `ProcessToken` (`created_at_ms`, `visited_nodes`) and `ProcessGraphManager::getNodeHistory` APIs

---

### 3. ProcessGraphManager Integration Initialization

**Current State:**  
ProcessGraphManager integration assumes it is already initialized with process definitions before use.

**Planned Enhancement:**  
Implement explicit initialization checks and provide clear initialization patterns for ProcessGraphManager integration.

**Benefits:**
- **Robustness**: Prevent runtime errors from uninitialized state
- **Clear API**: Explicit initialization contracts for integrators
- **Better Error Messages**: Clear feedback when process definitions are missing
- **Developer Experience**: Easier integration and debugging

**Implementation Considerations:**
- Add `isInitialized()` method to ProcessGraphManager
- Provide `validateProcessDefinitions()` helper method
- Update documentation with initialization best practices
- Add initialization state checks before execution operations

**Current Code Location:**
- `include/index/process_graph.h` - ProcessGraphManager class definition
- `src/index/process_graph.cpp` - ProcessGraphManager implementation

---

## Related Documentation

- [Process Mining Guide](./process_mining_guide.md) - Process mining capabilities and usage
- [Process Mining Summary](./PROCESS_MINING_SUMMARY.md) - Process mining implementation overview
- [Process Mining Research and Roadmap](./PROCESS_MINING_RESEARCH_AND_ROADMAP.md) - Detailed research and roadmap
- [Wire Protocol v1](../architecture/wire_protocol_v1.md) - Current wire protocol specification
- [Binary Protocol Buffers](../architecture/BINARY_PROTOCOL_BUFFERS.md) - Binary protocol extensions

---

## Timeline

These enhancements are planned for future releases. Implementation priority and timeline will be determined based on project roadmap and resource availability.

---

**Last Updated:** April 2026  
**Status:** Planning
