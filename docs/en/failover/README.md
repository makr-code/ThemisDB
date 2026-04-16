[docs](../../index.md) > [en](../index.md) > [failover](./README.md) > [overview](./README.md)
**Date:** 2026-04-16
**Status:** review
**Primary Source:**
- `src/failover/README.md`
- `src/failover/ARCHITECTURE.md`
- `src/failover/ROADMAP.md`
- `src/failover/FUTURE_ENHANCEMENTS.md`
- `include/failover/README.md`
- `include/failover/ROADMAP.md`

**Reference:**
- Issue: `[MODULE] failover`
- Context: Reality check and module-level Secondary documentation sync for the failover module.

---

# Failover Module

## TL;DR

Core failover/DR functionality is implemented and test-covered.
Remaining gaps are roadmap/enhancement items (cross-region integration, ABI/soak validation, extended metrics export), not missing baseline runtime logic.

## Reality Check Summary

- Implemented components:
  - `AutoFailoverManager` (`include/failover/auto_failover_manager.h`, `src/failover/auto_failover_manager.cpp`)
  - `DisasterRecoveryManager` (`include/failover/disaster_recovery_manager.h`, `src/failover/disaster_recovery_manager.cpp`)
- Existing tests:
  - `tests/test_auto_failover_manager.cpp` (39 tests)
  - `tests/test_disaster_recovery_manager.cpp` (8 tests)
  - `tests/test_failover_chaos_scenarios.cpp` (17 tests)

## Roadmap / Future Enhancements Verification

- `src/failover/ROADMAP.md` and `include/failover/ROADMAP.md` match implementation status for completed and open items.
- `src/failover/FUTURE_ENHANCEMENTS.md` and `include/failover/FUTURE_ENHANCEMENTS.md` are actionable and follow the required structure.

## Installation

The module is part of the standard ThemisDB build; no separate installation step is required.

## Usage

- Use `AutoFailoverManager::start/stop/triggerManualFailover` for failover orchestration control.
- Use `DisasterRecoveryManager::executePlan` for DR execution.
- See [Missing implementations report (DE)](../../de/failover/missing-implementations.md) for open delivery gaps.

## Related Documents

- [PRIMARY_SOURCES.md](./PRIMARY_SOURCES.md)
- [Missing implementations report (DE)](../../de/failover/missing-implementations.md)
