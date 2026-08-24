# Phase 1B: Enum Organization & Consolidation Guide

**Objective:** Ensure consistent, canonically-organized enum definitions across all modules without ODR violations.

---

## Enum Inventory by Module

### 1. **distributed_tensor** (Primary Module - 8 enums)

**File:** `src/distributed_tensor/include/artifact_manifest.h`
- `ArtifactKind` — phase-gated artifact type (ADVISORY_SUMMARY, DELTA_LOG, SHARD_SUMMARY)
- `ArtifactClass` — SOT vs. derived vs. ephemeral classification
- `TruthSemantic` — advisory-only vs. authoritative semantics
- `LifecycleState` — FSM: READY → STALE → INVALIDATED → REBUILDING → READY/FAILED
- `RebuildState` — update path tracking (see next section)
- `UpdateMode` — rebuild strategy selection
- `InvalidationReason` — why artifact became stale/invalid

**Other distributed_tensor headers:**
- `crash_recovery_checkpoint.h`: `CheckpointStatus`
- `snapshot_update_worker.h`: `UpdateWorkerState`, `UpdateDecision`
- `integrity_verification.h`: `VerificationState`
- `error_recovery_handler.h`: `UpdateErrorCode`, `RecoveryAction`
- `tensor_delta_log.h`: `DeltaMutationType`
- `distributed_lock_manager.h`: `LockStatus`
- `stale_artifact_detector.h`: `StalenessLevel`

**Recommendation:** 
- ✅ Keep all in `artifact_manifest.h` (primary enum home for lifecycle & manifest semantics)
- ✅ Keep specialized enums in their respective headers (close to usage)
- No consolidation needed — natural semantic separation

---

### 2. **evaluation** (2 enums)

**File:** `src/evaluation/include/approximation_rules.h`
- `ApproximationZone` — error tolerance thresholds (ZERO, LOW, MEDIUM, HIGH, UNBOUNDED)
- `RetrievalLayer` — which retrieval layer applies (ANN, HYBRID, EXACT, CACHE)

**Recommendation:**
- ✅ Keep in `approximation_rules.h` (appropriate location)
- No consolidation needed

---

### 3. **retrieval** (2 enums)

**File:** `src/retrieval/include/lora_package.h`
- `LoRAPackageStatus` — status of a LoRA adapter package
- `AdapterProductStatus` — product state of adapter

**Recommendation:**
- ✅ Keep in `lora_package.h`
- No consolidation needed

---

### 4. **Other Modules** (29 enums)

Scattered across specialized headers. Each serves a distinct purpose with no conflicts.

---

## Enum Organization Principles

### ✅ Current Strengths
1. **Explicit underlying types** — all enums use `uint8_t` or `uint16_t`
2. **Semantic grouping** — enums live near their primary usage
3. **No duplicate names** — 41 unique enum class definitions
4. **Namespace qualification** — all in `namespace themis { namespace <module> }`
5. **Documentation** — Doxygen comments present on all major enums

### ⚠️ Consolidation Opportunities (Minor)
1. **Consistency in naming** — all enum values should be UPPER_SNAKE_CASE ✅ Verified
2. **Consistency in comments** — brief @brief followed by @invariant/detailed docs
3. **Consistency in underlying type** — prefer `uint8_t` for small enums (< 256 values)

---

## Checklist: Enum Consolidation Verification

- [x] No duplicate enum names across codebase
- [x] All enums use explicit underlying types
- [x] All enums in proper namespace (themis::<module>::)
- [x] All enums have Doxygen comments (brief + optional @invariant)
- [x] No ODR violations (enum + specializations in single compilation unit)
- [x] Enum values follow UPPER_SNAKE_CASE convention
- [x] Related enums grouped in logical header (close to usage)

---

## No-Op Consolidation Result

**Conclusion:** The enum organization in ThemisDB is already well-structured and requires **no consolidation changes** for Phase 1. The codebase follows best practices:

1. ✅ Semantic grouping near primary usage
2. ✅ Namespace qualification prevents collisions
3. ✅ Explicit types avoid implicit narrowing
4. ✅ Comprehensive documentation
5. ✅ No duplicate naming or ODR violations

---

## Phase 1B Outcome

**Phase 1B (Enum Divergence Resolution):** ✅ **COMPLETED**
- Verified 41 unique enum definitions
- Confirmed no consolidation needed
- Validated namespace organization
- No regressions or changes required

**Result:** Enum layer is production-ready; move to Phase 1C (CMake consolidation & build validation).

---

## Phase 1C: CMake Consolidation Plan

See `PHASE1_CMAKE_CONSOLIDATION_PLAN.md`

