# Phase 1: CMake Duplicate + Enum Divergence Resolution
**Target Duration:** 1-2 days  
**No Dependencies** — Standalone Phase 1 go/no-go: **Successful Build**

---

## 1. Scope: What needs to be fixed

### 1.1 CMake Duplicates
- Identify and consolidate redundant CMake directives across 613+ CMakeLists.txt files
- Remove duplicate `add_executable()`, `add_library()` patterns
- Consolidate repeated `target_link_libraries()` and `target_include_directories()` logic
- Rationalize preset/toolchain configurations (overlappping `-DTHEMIS_*` options)

### 1.2 Enum Divergence
- Map all enum definitions across `src/` (distributed_tensor, retrieval, evaluation, etc.)
- Identify "duplicate intent" enums (e.g., Status/State patterns in multiple modules)
- Define canonical enum locations for shared cross-module semantics
- Ensure consistent underlying types (`uint8_t`, `uint16_t`) where appropriate
- Add namespace qualification to prevent ODR violations

---

## 2. Initial Findings

### 2.1 CMake Files Count
- **613 CMakeLists.txt** files across the codebase
- Root CMakeLists.txt delegates to modular includes in `cmake/` directory

### 2.2 Enum Locations Found
- `src/distributed_tensor/include/artifact_manifest.h` — 6 enums (ArtifactKind, ArtifactClass, TruthSemantic, LifecycleState, RebuildState, UpdateMode, InvalidationReason)
- `src/distributed_tensor/include/crash_recovery_checkpoint.h` — CheckpointStatus
- `src/distributed_tensor/include/snapshot_update_worker.h` — UpdateWorkerState, UpdateDecision
- `src/distributed_tensor/include/integrity_verification.h` — VerificationState
- `src/distributed_tensor/include/error_recovery_handler.h` — UpdateErrorCode, RecoveryAction
- `src/distributed_tensor/include/tensor_delta_log.h` — DeltaMutationType
- `src/distributed_tensor/include/distributed_lock_manager.h` — LockStatus
- `src/distributed_tensor/include/stale_artifact_detector.h` — StalenessLevel
- `src/retrieval/include/lora_package.h` — LoRAPackageStatus, AdapterProductStatus
- `src/evaluation/include/approximation_rules.h` — ApproximationZone, RetrievalLayer

---

## 3. Phase 1 Action Items

### 3.1 CMake Consolidation (Day 1)
- [ ] Audit root CMakeLists.txt and `cmake/*.cmake` for duplicate logic
- [ ] Create a CMake consolidation log (patterns found, redundancies)
- [ ] Implement refactored CMake helpers (avoid duplication)
- [ ] Test: Successful `cmake --preset community-release` configuration

### 3.2 Enum Divergence Resolution (Day 1-2)
- [ ] Create `src/common/include/enum_defs.h` — canonical enum home for cross-module types
- [ ] Move/consolidate divergent enum definitions into canonical location
- [ ] Update all include statements across modules
- [ ] Add doxygen documentation to all enums
- [ ] Ensure consistent namespace qualification (`themis::artifact::Status`, etc.)

### 3.3 Build Verification (Day 2)
- [ ] Test build with `cmake --preset community-release`
- [ ] Run targeted unit tests for affected modules
- [ ] Ensure no linker errors or ODR violations

---

## 4. Success Criteria (Go/No-Go Gate)

✅ **Successful Build** means:
- `cmake --preset community-release` completes without errors
- No C++ compiler errors or warnings (related to enums/includes)
- No linker/ODR violations
- Targeted module tests pass (affected by changes)

---

## 5. Phase 1 → Phase 2 Handoff

After Phase 1 completes, Phase 2 (SHA-256 Crypto) depends on:
- ✅ Stable, working build environment
- ✅ Canonical enum definitions in place
- ✅ No build regressions

---

## Notes

- Large batches preferred per user workflow preference ("weiter"/"nächster block")
- This phase is foundational for all subsequent phases (2-8)
- CMake consolidation may discover deeper structural issues; document and prioritize
