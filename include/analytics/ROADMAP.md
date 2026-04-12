<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · SECURITY.md -->

# Roadmap — Analytics Module Public Headers

**Module Path:** `include/analytics/`  
**Implementation Roadmap:** `../../src/analytics/ROADMAP.md`

---

## Current Status

Public headers at v1.7.0. CEP, OLAP, ML serving, forecasting, anomaly detection, NLP,
process mining, and Arrow interop headers are all stable and have corresponding
implementations in `src/analytics/`.

---

## Completed Features

- [x] `ICEPEngine` and streaming window interfaces
- [x] `IOLAPEngine`, `IColumnarExecutor`, `IJITAggregator`
- [x] `IForecaster`, `IAnomalyDetector`
- [x] `IMLServingEngine`, `IModelServingBackend`, `IAutoMLEngine`
- [x] `IDistributedAnalytics` for cross-shard analytics
- [x] Arrow IPC export and Arrow Flight headers
- [x] `IDiffEngine`, `IIncrementalView`
- [x] `INLPTextAnalyzer`, `ILLMProcessAnalyzer`, `IProcessMiner`, `IProcessPatternMatcher`

---

## Planned Features

- [x] `IStreamingJoin` header for multi-stream join operations (Target: Q3 2026)
- [ ] `IFederatedAnalytics` for privacy-preserving federated query execution (Target: Q4 2026)
- [ ] `IExplainableML` interface for model explanation / feature importance (Target: Q4 2026)

---

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Core streaming and OLAP interface definitions
- [x] ML serving and forecasting interfaces

### Phase 2: Core Implementation
- [x] All 20 header files published with stable interfaces

### Phase 3: Error Handling & Edge Cases
- [x] Error types defined in `detail/` for each subsystem

### Phase 4: Tests
- [x] Interface tests in `src/analytics/` (see `../../src/analytics/AUDIT.md`)

### Phase 5: Performance / Hardening
- [x] `IJITAggregator` for compiled aggregation
- [x] `IStreamingJoin` for multi-stream operations (Q3 2026)

### Phase 6: Documentation & Acceptance
- [x] Architecture and audit docs present
- [ ] Doxygen fully annotated on all exported types

---

## Production Readiness Checklist

- [x] All major analytics categories have stable public headers
- [x] Arrow interop headers present and flag-guarded
- [x] Export visibility macro established
- [ ] Doxygen fully annotated
- [x] `IStreamingJoin` header published (`streaming_join.h`; HashJoin + IntervalJoin; 15 tests SJ-01…SJ-15)
- [ ] `IFederatedAnalytics` header published
