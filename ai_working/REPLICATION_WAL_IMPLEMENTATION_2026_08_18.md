# Async WAL Shipping with Lag Alerts Implementation
## Wave A Block 2 — Replication Module

**Date**: 2026-08-18  
**Target Completion**: Wave A deliverable  
**Acceptance Criteria Source**: `src/replication/ROADMAP.md` §3.1

### Requirements

**Feature**: Async cross-region WAL shipping with configurable lag limits
- Configuration: `replication.wal_shipping.max_lag_ms` (default 1s)
- Remote DC endpoint configuration
- Throughput target: ≥ 80 MB/s on GbE link
- Lag alert fires within 2× lag window
- Prometheus histogram: `replication_wal_lag_ms`

### Implementation Phases

- [ ] Phase 1: Production code completion
  - [ ] AsyncWalShipper production implementation (backpressure, timeout wrapping, zero-copy serialization)
  - [ ] LagAlertManager production implementation (event emission, threshold checking)
  - [ ] Lock hierarchy and deadlock analysis documentation

- [ ] Phase 2: Comprehensive test suite
  - [ ] Normal WAL shipping under load (≥3 tests)
  - [ ] Lag alert triggering at configured threshold (≥2 tests)
  - [ ] Backpressure handling (≥2 tests)
  - [ ] Network failure recovery (≥2 tests)
  - [ ] Concurrent shipping to multiple DCs (≥1 test)
  - [ ] Total: ≥10 focused test cases

- [ ] Phase 3: Verification
  - [ ] Build and unit test verification
  - [ ] No unresolved CRITICAL/HIGH paths with TODO/STUB/FIXME
  - [ ] Fail-closed behavior under lag limit exceeded
  - [ ] Prometheus metrics export validation

- [ ] Phase 4: Documentation
  - [ ] ROADMAP.md completion evidence update
  - [ ] Doxygen API documentation verification
  - [ ] Configuration key documentation in README.md
  - [ ] Lock hierarchy documentation

### Code Quality Checklist

- [ ] Modern C++: auto, constexpr, smart pointers (no raw new/delete)
- [ ] RAII: All resources bound to object lifetime
- [ ] Thread safety: All public methods thread-safe
- [ ] Timeout wrapping: All blocking I/O has timeout guards
- [ ] Zero-copy where possible: string_view, move semantics
- [ ] No silent error swallowing
- [ ] Fail-closed on lag limit exceeded

### Deliverables

1. **Updated source files**:
   - `include/replication/async_wal_shipper.h` (complete Doxygen docs)
   - `src/replication/async_wal_shipper.cpp` (production implementation)
   - `include/replication/lag_alert_manager.h` (complete Doxygen docs)
   - `src/replication/lag_alert_manager.cpp` (production implementation)

2. **New test file**:
   - `tests/replication/test_replication_async_wal_lag_alerts.cpp` (≥10 focused test cases)

3. **Documentation**:
   - Lock hierarchy analysis with deadlock guard documentation
   - Prometheus metrics validation
   - Configuration key documentation

### Status

| Item | Status | Notes |
|------|--------|-------|
| Phase 1 Implementation | Pending | |
| Phase 2 Tests | Pending | |
| Phase 3 Verification | Pending | |
| Phase 4 Documentation | Pending | |
