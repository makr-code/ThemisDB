# Phase 2 Implementation Guide - Plugins Module

**Date**: 2026-08-05  
**Scope**: Q4 2026 Phase 2 Hardening (Core Implementation)  
**Status**: Phase 2A Complete, Phase 2B/2C Test Infrastructure Ready

## Overview

Phase 2 delivers core hardening for plugin lifecycle and registry internals. The work is split into three areas:

- **Phase 2A**: Lifecycle state machine (✓ COMPLETE)
- **Phase 2B**: Registry concurrency hardening (Tests ready, implementation pending)
- **Phase 2C**: Manifest/signature validation tightening (Tests ready, implementation pending)

## Phase 2A: Lifecycle State Machine (COMPLETE)

### What Was Done

1. **Added PluginLifecycleState enum** to `include/plugins/plugin_interface.h`:
   - 5 states: UNLOADED, LOADING, LOADED, UNLOADING, UNKNOWN
   - Each state has explicit allowed transitions
   - Clear semantics for each state

2. **Implemented transition validation**:
   - `isValidLifecycleTransition()` function validates state machine rules
   - Ensures only legitimate transitions are allowed
   - Reload path (LOADED → LOADED) supported

3. **Added helper function**:
   - `lifecycleStateToString()` for diagnostics

4. **Created focused tests** (PLG-09..PLG-16):
   - Test enum values (PLG-09)
   - Test string conversion (PLG-10)
   - Test transitions from each state (PLG-11..PLG-14)
   - Test complete lifecycle path (PLG-15)
   - Test reload path (PLG-16)

### Integration Steps for plugin_manager.cpp

To fully integrate Phase 2A into the runtime:

1. **Add state tracking to PluginInfo struct**:
   ```cpp
   struct PluginInfo {
       // ... existing fields ...
       PluginLifecycleState state = PluginLifecycleState::UNLOADED;
       std::mutex state_mutex;  // Guard state transitions
   };
   ```

2. **Modify load operation** to use state machine:
   ```cpp
   // Before starting load: verify transition UNLOADED → LOADING is valid
   if (!isValidLifecycleTransition(current_state, PluginLifecycleState::LOADING)) {
       return PluginsError::kLifecycleTransition;
   }
   // Set state to LOADING
   plugin_info.state = PluginLifecycleState::LOADING;
   
   // Perform actual load...
   
   // If load succeeds: transition to LOADED
   // If load fails: transition back to UNLOADED
   ```

3. **Modify unload operation** similarly:
   ```cpp
   // LOADED → UNLOADING → UNLOADED
   ```

4. **Add error code for invalid transitions**:
   - Use `PluginsError::kLifecycleTransition` (8203) when state machine rejects transition

---

## Phase 2B: Registry Concurrency & Atomicity

### What Was Done

1. **Created focused tests** (PLG-17..PLG-22) in `test_registry_concurrency_hardening.cpp`:
   - PLG-17: Single registration
   - PLG-18: Multiple registrations of different types
   - PLG-19: Unregistration
   - PLG-20: Error handling (non-existent plugins)
   - PLG-21: Atomic re-registration (simulates hot-reload)
   - PLG-22: Concurrent read stress test (5 threads × 10 plugins)

### What Needs to Be Done

1. **Review PluginRegistry atomicity**:
   - Check `plugin_registry.cpp` for race conditions
   - Verify mutex protection covers all registry operations
   - Ensure re-registration is truly atomic

2. **Add timeout semantics** (optional for Q4 2026):
   - Consider adding timeout parameters to registry operations
   - Useful for hot-plug scenarios where quick plugin replacement is needed

3. **Test concurrent writes**:
   - Extend Phase 2B tests to include concurrent registration attempts
   - Verify only one factory is active at a time for a given plugin name

### Expected Behavior

- Single-threaded operations: instant success/failure
- Concurrent reads: lock-free or read-lock, no blocking
- Concurrent writes: exclusive lock, serialized
- Re-registration: atomic operation (factory replaced atomically)
- Error handling: no partial state on failed operations

---

## Phase 2C: Manifest/Signature Validation

### What Was Done

1. **Created validation contract tests** (PLG-23..PLG-28) in `test_validation_contract_hardening.cpp`:
   - PLG-23: Validation error code semantics
   - PLG-24: Manifest invalid contract
   - PLG-25: Signature verification contract
   - PLG-26: Capability validation contract
   - PLG-27: Fail-safe validation semantics
   - PLG-28: Validation determinism

### What Needs to Be Done

1. **Unify validation logic** in `plugin_manager.cpp`:
   - Current code has separate `verifyManifestSignature()` and manifest loading logic
   - Create a unified `validatePluginForLoad()` function with clear contract

   ```cpp
   PluginsError validatePluginForLoad(
       const PluginManifest& manifest,
       const std::string& manifest_path,
       std::string& error_details
   ) {
       // Stage 1: Manifest schema validation
       if (!isValidManifestSchema(manifest)) {
           error_details = "Manifest schema validation failed";
           return PluginsError::kManifestInvalid;
       }
       
       // Stage 2: Manifest semantic validation
       if (!isValidManifestSemantics(manifest)) {
           error_details = "Manifest semantic validation failed";
           return PluginsError::kManifestInvalid;
       }
       
       // Stage 3: Signature verification
       if (!verifyManifestSignature(manifest_path, error_details)) {
           return PluginsError::kSignatureVerifyFailed;
       }
       
       // Stage 4: Capability validation
       if (!validateCapabilities(manifest)) {
           error_details = "Capability requirements not met";
           return PluginsError::kCapabilityDenied;
       }
       
       return PluginsError::kSuccess;
   }
   ```

2. **Document validation order**:
   - Add comments explaining fail-safe semantics
   - Why each validation stage must complete before next
   - Which errors trigger rollback vs. partial activation

3. **Ensure fail-safe semantics**:
   - No plugin enters LOADING state until all pre-load validations pass
   - If any validation fails, plugin remains in UNLOADED state
   - No partial activation

---

## Implementation Checklist for Q4 2026

### Week 1-2: Phase 2A Integration
- [ ] Integrate lifecycle state tracking into PluginInfo
- [ ] Modify load/unload/reload operations to use state machine
- [ ] Test state transitions in plugin_manager_test.cpp
- [ ] Verify error codes (kLifecycleTransition) are returned correctly

### Week 3: Phase 2B Audit
- [ ] Review PluginRegistry for concurrency issues
- [ ] Run PLG-17..PLG-22 tests against current implementation
- [ ] Fix any atomicity issues found
- [ ] Document concurrency guarantees

### Week 4: Phase 2C Implementation
- [ ] Create unified validation function
- [ ] Update plugin_manager.cpp to use new function
- [ ] Run PLG-23..PLG-28 tests
- [ ] Update documentation with validation contract

### Week 5: Testing & Validation
- [ ] Run full plugins test suite
- [ ] Verify benchmarks still pass (GATE-PLG-01..04)
- [ ] Integration testing with hot-plug scenarios
- [ ] Performance regression testing

### Documentation Updates
- [ ] Update ROADMAP.md Phase 2 sections to COMPLETE
- [ ] Add lifecycle state machine documentation
- [ ] Add validation contract documentation
- [ ] Create operator runbook for common plugin lifecycle scenarios

---

## Testing Strategy

### Unit Tests (Focused Tests)
- PLG-09..PLG-16: Lifecycle state machine
- PLG-17..PLG-22: Registry concurrency
- PLG-23..PLG-28: Validation contract

### Integration Tests
- Plugin load/unload/reload scenarios
- Concurrent hot-plug operations
- Manifest/signature validation failures
- Capability negotiation edge cases

### Performance Tests
- Verify GATE-PLG-01..04 still pass
- Measure state machine overhead (should be negligible)
- Measure registry operation latency

### Stress Tests
- Rapid plugin load/unload cycles
- Concurrent plugin creation/destruction
- Invalid manifest rejection performance

---

## Known Risks & Mitigation

| Risk | Severity | Mitigation |
|------|----------|-----------|
| State machine overhead | Low | Verify with benchmarks; state transitions should be <1ns |
| Registry lock contention | Medium | Use lock-free reads for common case; profile concurrent scenarios |
| Validation performance regression | Medium | Keep validation logic efficient; profile against baseline |
| Backward compatibility | Low | State machine is internal; no API changes |

---

## Success Criteria

- [x] Phase 2A code changes complete
- [x] Phase 2A tests complete and passing
- [ ] Phase 2B tests passing against updated registry
- [ ] Phase 2C validation logic unified
- [ ] All Phase 2 error codes properly returned
- [ ] Benchmarks (GATE-PLG-01..04) still passing
- [ ] Documentation complete and aligned

---

**Document History**
- 2026-08-05: Initial Phase 2 implementation guide created
