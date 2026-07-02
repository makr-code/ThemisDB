/// @file ARTIFACT_MANIFEST_IMPLEMENTATION.md
/// @brief Artifact Manifest Schema Implementation Guide
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-02

# Distributed Tensor Artifact Manifest Implementation Guide

## Overview

The Artifact Manifest Schema provides a comprehensive, durable coordination object for distributed tensor artifacts in ThemisDB. It captures all metadata required for artifact placement, reconstruction, provenance tracking, and freshness assessment across shards.

**Status:** Phase 1 Complete (Design & Core Implementation)  
**Files:** `src/distributed_tensor/include/artifact_manifest.h`, `src/distributed_tensor/src/artifact_manifest.cc`, `tests/epic3_distributed_tensor/artifact_manifest_test.cc`

## Design Principles

### 1. Manifest-First Architecture
The manifest is the durable, authoritative coordination object. The tensor payload is managed relative to the manifest.

**Key Invariant:** Manifest state determines what operations are valid on the artifact.

### 2. Lifecycle Management
Artifacts progress through well-defined lifecycle states:
```
CREATED → ACTIVE → (STALE) → INVALIDATED/REBUILT/DELETED
```

Only ACTIVE and STALE artifacts are usable. CREATED and REBUILT require verification.

### 3. Planner Integration
Fields marked with `[PLANNER]` are specifically designed for query planner consumption:
- **Freshness Tracking:** staleness_threshold_sec, last_verified_unix_sec, artifact_age_ms
- **Rank Information:** rank_cap, rank_status (for decomposition quality)
- **Sequence Tracking:** source_seq_start, source_seq_end, delta_lag
- **Quality Metrics:** residual (approximation error)
- **Rebuild State:** rebuild_state, update_mode

Planners use these fields to determine:
- Whether artifact meets freshness requirements
- Whether artifact precision is sufficient (residual vs. accuracy needs)
- Whether summary-first or exact loading is required
- Whether rebuild is necessary before use

## Schema Overview

### Core Field Categories

#### 1. Identity & Classification
```cpp
std::string artifact_id;           // Unique identifier (e.g., "LoRA:v2.1:adapter:model-x")
ArtifactClass artifact_class;      // PRIMARY, DERIVED, EPHEMERAL, ADVISORY_ONLY
TruthSemantic truth_semantic;      // SOURCE_OF_TRUTH, TRUTH_ADJACENT, ADVISORY
LifecycleState lifecycle_state;    // CREATED, ACTIVE, STALE, INVALIDATED, REBUILT, DELETED
```

**Usage:** Determines artifact's role and how it can be used.

#### 2. Versioning & Integrity
```cpp
std::string version;               // Semantic version or content hash
std::string content_hash;          // SHA-256 hash for integrity verification
std::string manifest_hash;         // Hash of manifest itself for change detection
```

**Usage:** Enable deduplication, corruption detection, and cache invalidation.

#### 3. Temporal Metadata
```cpp
int64_t created_at_unix_sec;       // When artifact was created
int64_t updated_at_unix_sec;       // Last update/refresh timestamp
int64_t last_verified_unix_sec;    // [PLANNER] Last freshness check
int64_t last_rebuild_at_unix_sec;  // Last rebuild operation
int64_t staleness_threshold_sec;   // [PLANNER] Max age before stale
uint64_t artifact_age_ms;          // [PLANNER] Age in milliseconds
```

**Usage:** Freshness assessment and staleness detection.

#### 4. Dynamic Update Tracking
```cpp
uint64_t source_seq_start;         // [PLANNER] Source sequence range
uint64_t source_seq_end;           // [PLANNER] (inclusive)
uint64_t delta_lag;                // [PLANNER] How far behind source
```

**Example:** If source is at seq 10000 and artifact covers up to seq 9500, delta_lag = 500.  
**Planner Use:** Determine freshness relative to latest source data.

#### 5. Quality Metrics
```cpp
double residual;                   // [PLANNER] Approximation error [0.0, 1.0]
uint32_t rank_cap;                 // [PLANNER] Maximum tensor rank captured
uint32_t rank_status;              // [PLANNER] Current active rank
```

**Usage:** Determine whether artifact precision is suitable for accuracy-critical queries.

#### 6. Rebuild & Update State
```cpp
RebuildState rebuild_state;        // PRISTINE, PATCHED, PARTIAL_REFITTED, REBUILT
UpdateMode update_mode;            // patch, partial_refit, rebuild
InvalidationReason invalidation_reason;  // Why was it invalidated?
```

**Rebuild States:**
- `PRISTINE`: Unmodified original artifact
- `PATCHED`: Small, localized changes applied
- `PARTIAL_REFITTED`: Selective tensor retraining for subset of components
- `REBUILT`: Complete recomputation from source

**Update Modes:**
- `patch`: Delta-based updates (O(1) per change)
- `partial_refit`: Selective retraining (O(k) where k is subset size)
- `rebuild`: Full recomputation (O(n) where n is full size)

**Invalidation Reasons (8 types):**
- `INTEGRITY_CHECK_FAILED`: Content hash mismatch
- `STALENESS_EXCEEDED`: Exceeds staleness threshold
- `SOURCE_INVALIDATED`: Source or dependency invalidated
- `SOURCE_LINEAGE_CORRUPTED`: Source lineage incomplete/corrupted
- `POLICY_VIOLATION`: Class/semantic policy change
- `ADMIN_REQUESTED`: Administrative directive
- `SHARD_UNAVAILABLE`: Shard placement no longer valid
- `UNKNOWN`: Unspecified reason

#### 7. Provenance & Reconstruction
```cpp
std::string source_artifact_id;         // Parent artifact
std::vector<std::string> provenance_chain;  // Full lineage
std::string reconstruction_instructions;    // How to rebuild from source
```

**Example Provenance Chain:** `["base:v1", "adapter:v1", "patch:v1"]`

**Reconstruction Instructions:** 
```
"apply patches [p1, p2, p3]" or 
"refit with config v2.1" or 
"regenerate from lineage"
```

#### 8. Placement & Distribution
```cpp
std::vector<std::string> shard_placements;      // Where artifact is located
bool requires_full_replication;                  // Must replicate to all shards?
bool is_rebuildable;                            // Can be safely rebuilt?
uint32_t replication_factor;                    // Number of copies to maintain
std::string erasure_coding_scheme;              // e.g., "reed_solomon_4_2"
std::vector<std::string> backup_shard_placements; // Backup locations
```

**Usage:** Query routing, retrieval strategy, failure recovery.

#### 9. Compatibility & Constraints
```cpp
std::map<std::string, std::string> compatibility_metadata;
std::string min_planner_version;        // [PLANNER] Minimum version required
bool advisory_only;                     // [PLANNER] Binding truth or hints only?
```

**Example Metadata:** 
```
{"model_version": "1.2.3", "gpu_arch": "RTX40xx", "cuda_version": "12.2"}
```

#### 10. Metadata & Extensibility
```cpp
std::map<std::string, std::string> custom_attributes;
std::string description;
```

## Validation Rules

The manifest implements comprehensive validation through `validate()`:

1. **Identity:** artifact_id must not be empty
2. **Hash Format:** content_hash must be valid hex (32-128 characters)
3. **Timestamps:** Monotonically increasing (created ≤ updated ≤ verified)
4. **Sequences:** source_seq_end ≥ source_seq_start (if both set)
5. **Residual:** 0.0 ≤ residual ≤ 1000.0
6. **Rank:** rank_status ≤ rank_cap
7. **Replication:** replication_factor ≥ 1
8. **Compatibility:** artifact_class & truth_semantic combination is valid

## Freshness Assessment for Planner

### isUsable()
Returns true only if lifecycle_state is ACTIVE or STALE.
- ACTIVE: Fully fresh and trustworthy
- STALE: Usable but should trigger background refresh

### isStale()
Checks if artifact exceeds staleness threshold:
```cpp
if (staleness_threshold_sec == 0) return false;  // No threshold
if (last_verified_unix_sec == 0) return true;   // Never verified
age_sec = now - last_verified_unix_sec;
return age_sec > staleness_threshold_sec;
```

### getFreshnessScore()
Returns normalized freshness score [0.0, 1.0]:
- 1.0 = Just verified
- 0.5 = Half staleness threshold exceeded
- 0.0 = Exceeds threshold (stale)

**Calculation:** `freshness = 1.0 - (age / threshold)` clamped to [0.0, 1.0]

## Serialization Support

### JSON Format
Uses `nlohmann/json` library. All fields are serializable.

**Example:**
```json
{
  "artifact_id": "LoRA:v2.1:medical:gpt4-32k",
  "artifact_class": "DERIVED",
  "truth_semantic": "TRUTH_ADJACENT",
  "lifecycle_state": "ACTIVE",
  "version": "2.1.0-patch-3",
  "content_hash": "a1b2c3d4...",
  "source_seq_start": 0,
  "source_seq_end": 50000,
  "delta_lag": 2500,
  "residual": 0.025,
  "rank_cap": 512,
  "rank_status": 480,
  "rebuild_state": "PATCHED",
  "update_mode": "partial_refit",
  "provenance_chain": ["base:v1", "adapter:v1", "medical-refit:v1"],
  "shard_placements": ["shard-us-west-1", "shard-us-east-1"],
  "compatibility_metadata": {"model_version": "GPT-4-32k-v1"},
  "advisory_only": false,
  "custom_attributes": {"training_domain": "medical"}
}
```

### Round-Trip Guarantee
Manifest → JSON → Manifest preserves all data integrity.

### YAML Support
Placeholder for future yaml-cpp integration. Currently uses JSON serialization.

## Enum Conversion Utilities

### RebuildStateUtils
```cpp
std::string RebuildStateUtils::stateToString(RebuildState state);
std::optional<RebuildState> RebuildStateUtils::stringToState(const std::string& state_str);
```

### UpdateModeUtils
```cpp
std::string UpdateModeUtils::modeToString(UpdateMode mode);
std::optional<UpdateMode> UpdateModeUtils::stringToMode(const std::string& mode_str);
```

### InvalidationReasonUtils
```cpp
std::string InvalidationReasonUtils::reasonToString(InvalidationReason reason);
std::optional<InvalidationReason> InvalidationReasonUtils::stringToReason(const std::string& reason_str);
```

## Test Coverage

### Validation Tests (11 cases)
- Valid manifest passes validation
- Invalid artifact IDs, content hashes, timestamps
- Inconsistent sequences, invalid residuals
- Invalid rank states and replication factors
- Invalid class/semantic combinations

### Freshness Tests (13 cases)
- Usability in different lifecycle states
- Staleness detection and thresholds
- Freshness score calculations
- Edge cases (never verified, no threshold)

### Serialization Tests (7 cases)
- JSON serialization and deserialization
- Round-trip data integrity
- Invalid JSON error handling
- Complex nested structures (provenance, attributes)

### State Transition Tests (9 cases)
- Rebuild state conversions
- Update mode conversions
- Invalidation reason conversions

### Planner Integration Tests (6 cases)
- Advisory-only flag tracking
- Rank cap validation
- Delta lag freshness calculation
- Residual accuracy assessment
- Artifact age tracking
- Reconstruction instruction preservation

### Complex Integration Scenario
- LoRA adapter with dynamic updates
- Full lifecycle from creation through patches
- Planner decision-making based on manifest fields

## Integration with Query Planner

The manifest enables planners to make informed decisions:

1. **Freshness-Aware Planning:**
   - Use `getFreshnessScore()` to prioritize fresh artifacts
   - Check `isStale()` to trigger background refresh

2. **Precision-Aware Planning:**
   - Use `residual` to determine if approximation is acceptable
   - Use `rank_status` vs `rank_cap` to assess decomposition quality

3. **Routing Decisions:**
   - Use `shard_placements` to route queries efficiently
   - Use `source_seq_end` vs `delta_lag` to assess data coverage

4. **Rebuild Decisions:**
   - Use `rebuild_state` to determine if incremental updates are possible
   - Use `reconstruction_instructions` to guide rebuild strategy

5. **Validity Assessment:**
   - Use `lifecycle_state` to determine usability
   - Use `invalidation_reason` for recovery guidance

## Future Enhancements

**Phase 2:** Persistence layer, cross-shard synchronization  
**Phase 3:** Error recovery, manifest repair utilities  
**Phase 4:** Performance benchmarks, stress testing  
**Phase 5:** Manifest versioning, compatibility checking  
**Phase 6:** Complete API documentation  
**Phase 7:** Full planner integration, monitoring

## References

- `DISTRIBUTED_TENSOR_SHARDING.md` § 7. Manifest-first Design
- `docs/EPIC3_MANIFEST_SCHEMA.md` - Schema planning and roadmap
- `src/distributed_tensor/include/tensor_artifact_classes.h` - Artifact classification
- Issue #5430 - Design manifest schema for distributed tensor artifacts
