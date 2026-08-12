## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


# EPIC 3.4 Merkle/Receipt Integration Notes

<!-- Status: complete | planning scaffold | validated: 2026-07-04 -->

## Summary

Integration guide for Merkle fragment verification and receipt-chain verification with other EPIC 3 subsystems (manifest, placement, recovery).

---

## 1. Integration with EPIC 3.2 Manifest Schema

### Manifest Fields

The `ArtifactManifest` struct includes these integrity fields:

```cpp
struct ArtifactManifest {
    // ... existing fields ...

    // ====== Integrity Fields (EPIC 3.4) ======
    
    // Content hash for full-artifact integrity
    std::string content_hash;                   // SHA-256 of artifact content
    std::string manifest_hash;                  // SHA-256 of manifest itself
    
    // Merkle tree for fragment verification
    std::string artifact_root_hash;             // Root of fragment Merkle tree
    json        fragment_merkle_paths;          // JSON array of Merkle proofs (or empty)
    
    // Receipt chain for history and provenance
    json        receipt_chain_head;             // Most recent VerificationReceipt as JSON
    json        receipt_chain_metadata;         // Chain statistics (length, genesis_id, etc.)
    
    // Provenance linkage
    std::string package_lineage_hash;           // Link to package rebuild source
    std::string shard_placement_id;             // Placement strategy at creation time
};
```

### Manifest Synchronization

When manifest is updated:

1. **On artifact store**: 
   - Compute `content_hash` from artifact bytes
   - Compute `manifest_hash` from manifest JSON
   - Append receipt to `receipt_chain_head`

2. **On manifest load**:
   - Verify `content_hash` matches artifact content (if full artifact loaded)
   - Verify `manifest_hash` matches manifest JSON
   - Validate receipt chain integrity

3. **On shard sync**:
   - Merkle proofs may be pruned to save space (lazy computation)
   - Receipt chain head is synced; genesis may be archived

---

## 2. Integration with EPIC 3.3 Shard Placement

### Placement Strategy Recording

When an artifact is placed via `ShardPlacementStrategy`:

```cpp
// After placement completes
ArtifactManifest manifest = get_manifest();
manifest.shard_placement_id = strategy.getStrategyId();  // e.g., "factorized_v1"

// Record in receipt chain
VerificationReceipt receipt;
receipt.artifact_id = manifest.artifact_id;
receipt.shard_placement_id = strategy.getStrategyId();
receipt.package_lineage_hash = current_lineage;
receipt.content_hash = manifest.content_hash;
receipt.timestamp = now_iso8601();
receipt.metadata["fragments_count"] = strategy.getFragmentCount();

manifest.appendToReceiptChain(receipt);
```

### Fragment Merkle Proof Construction

For Merkle-based placement strategies:

```cpp
// After fragmenting artifact
std::vector<FragmentData> fragments = strategy.fragment(artifact);

// Build Merkle tree bottom-up
std::vector<std::string> leaf_hashes;
for (const auto& frag : fragments) {
    leaf_hashes.push_back(computeSHA256(frag.data));
}

MerkleTree tree(leaf_hashes);
std::string artifact_root_hash = tree.getRootHash();

// Store root in manifest
manifest.artifact_root_hash = artifact_root_hash;

// Generate proofs (lazy: compute on verification demand)
manifest.fragment_merkle_paths = "";  // Empty; computed lazily
```

---

## 3. Integration with EPIC 3.5 Recovery Strategy

### Corruption Detection

Recovery manager detects corruption via integrity checks:

```cpp
// In recovery manager
VerificationResult result = verify_artifact(manifest);
if (result.state == VerificationState::CORRUPT) {
    spdlog::error("Artifact corrupted; triggering recovery");
    
    // Determine recovery strategy based on placement
    RecoveryStrategy strategy = select_recovery_strategy(manifest);
    
    // Execute recovery
    recover(manifest, strategy);
}
```

### Recovery Source Validation

After recovery, verify recovered artifact:

```cpp
// Post-recovery verification
VerificationResult post_recovery = verify_artifact(recovered_manifest);

// Append new receipt to chain recording recovery event
VerificationReceipt recovery_receipt;
recovery_receipt.artifact_id = manifest.artifact_id;
recovery_receipt.content_hash = post_recovery.actual_hash;
recovery_receipt.timestamp = now_iso8601();
recovery_receipt.metadata["recovery_event"] = json{
    {"prior_state", "CORRUPT"},
    {"recovery_strategy", strategy_name},
    {"recovery_duration_ms", elapsed_ms}
};

manifest.appendToReceiptChain(recovery_receipt);
```

### Stale Artifact Rebuild

When artifact is marked STALE:

```cpp
if (result.state == VerificationState::STALE) {
    spdlog::info("Artifact stale; recommending rebuild");
    
    // Check if rebuild is feasible
    if (manifest.isReconstructible()) {
        // Trigger rebuild from source
        rebuild_from_package_lineage(manifest);
    } else {
        // Mark for manual inspection
        spdlog::warn("Artifact not reconstructible; manual intervention needed");
    }
}
```

---

## 4. Integration with Query Planner (EPIC 3.6)

### Fragment-Level Verification in Query Path

When planner loads a fragment:

```cpp
// Planner requests specific fragment
Fragment frag = shard.loadFragment(fragment_index);

// Optionally verify via Merkle proof (cost: O(log N) ops)
if (need_fragment_verification()) {
    MerkleProof proof = manifest.getProofForFragment(fragment_index);
    if (!proof.verify(manifest.artifact_root_hash)) {
        spdlog::error("Fragment verification failed; using full-artifact verification");
        if (!verify_artifact(manifest).success) {
            return ERROR;
        }
    }
}

// Use fragment
return frag;
```

### Summary-First Retrieval

For summary-first retrieval with integrity:

```cpp
// Load shard summary (typically small)
SummaryTensor summary = load_summary();

// Verify summary integrity
std::string summary_hash = computeSHA256(serialize_summary());
if (summary_hash != manifest.summary_hash) {
    spdlog::error("Summary corrupted");
    // Fall back to exact retrieval
    load_exact_tensor();
}
```

---

## 5. Integration with Graph Validation (EPIC 1)

### Provenance Hook Implementation

Graph validation module implements `ProvenanceVerificationHook`:

```cpp
class GraphProvenanceVerificationHook : public ProvenanceVerificationHook {
public:
    VerificationResult verifyProvenance(
        const std::string& artifact_id,
        const std::string& content_hash,
        const std::string& package_lineage_hash) override {
        
        // Check graph provenance
        auto graph = get_graph();
        auto prov_node = graph->findProvenanceNode(artifact_id);
        
        if (!prov_node) {
            return VerificationResult{
                .success = false,
                .state = VerificationState::UNVERIFIED,
                .error_messages = {"Artifact not found in provenance graph"}
            };
        }
        
        // Check lineage matches
        if (prov_node->lineage_hash != package_lineage_hash) {
            return VerificationResult{
                .success = true,  // Content OK
                .state = VerificationState::STALE,
                .error_messages = {"Lineage mismatch; rebuild recommended"}
            };
        }
        
        return VerificationResult{
            .success = true,
            .state = VerificationState::VERIFIED
        };
    }
    
    std::string getCurrentPackageLineage() override {
        return get_graph()->getCurrentPackageLineageHash();
    }
};
```

---

## 6. Cross-Epic Dependencies

### On EPIC 3.1 (Artifact Classes)

- Integrity model applies to all artifact classes
- Different classes may have different verification requirements:
  - Primary artifacts: full verification required
  - Derived artifacts: verification inherited from source
  - Ephemeral artifacts: verification optional

### On EPIC 3.2 (Manifest)

- Manifest stores integrity metadata (content_hash, receipt_chain)
- Manifest itself has a hash to detect tampering
- Manifest sync includes integrity metadata

### On EPIC 3.3 (Shard Placement)

- Placement strategy ID recorded in receipt chain
- Fragment indices linked to Merkle proofs
- Different strategies may have different verification costs

### On EPIC 3.5 (Recovery)

- Integrity checks trigger recovery
- Recovery events recorded in receipt chain
- Post-recovery verification validates recovered data

### On EPIC 3.6 (Distributed Planner)

- Planner can request fragment-level verification (O(log N) cost)
- Planner decides when full vs. fragment verification needed
- Verification state informs planner routing decisions

### On EPIC 1 (Graph Validation)

- Provenance hook integrates graph validation
- Package lineage hash links artifact to rebuild source
- Compatibility checks integrated via hook

---

## 7. Failure Mode Handling

### Scenario: Fragment Corruption in Multi-Shard Artifact

```
Artifact is 4 fragments across 4 shards:
  Shard A: Fragment 0 ← CORRUPTED
  Shard B: Fragment 1 ← OK
  Shard C: Fragment 2 ← OK
  Shard D: Fragment 3 ← OK

Detection:
  1. Load fragments
  2. Fragment 0 integrity check: FAILED
  3. Merkle proof verification: FAILED (expected vs. actual root mismatch)
  4. State → CORRUPT

Recovery:
  1. Identify replacement strategy (erasure decode vs. replication)
  2. Use fragments 1,2,3 to reconstruct Fragment 0 (if erasure-coded)
  3. Recompute Merkle tree with corrected fragments
  4. Verify: all fragments OK, Merkle proofs OK
  5. State → VERIFIED
  6. Record recovery event in receipt chain
```

### Scenario: Partial Receipt Chain Corruption

```
Receipt chain has 5 receipts; attacker modifies Receipt #2:

Before: Receipt_0 → Receipt_1 → Receipt_2 → Receipt_3 → Receipt_4
                              ↑ (modified content_hash)

Verification:
  1. Load all 5 receipts
  2. Check Receipt_0: parent="" ✓
  3. Check Receipt_1: parent=Receipt_0.hash ✓
  4. Check Receipt_2: 
     - Compute expected_hash from content
     - Actual receipt_hash ≠ expected_hash ✗
  5. Chain verification fails
  6. State → CORRUPT (cannot trust any artifact history)

Resolution:
  - Cannot recover corrupted chain (integrity is the point)
  - Invalidate receipt chain; force full verification
  - Alert compliance/audit team
```

---

## 8. Performance Characteristics

### Verification Costs

| Operation | Cost | When |
|-----------|------|------|
| Content-hash verification | O(N) | On load |
| Fragment Merkle proof | O(log F) | On fragment load |
| Receipt chain verification | O(R) | Compliance query |
| Provenance hook | O(G) | On verification |

N = artifact size, F = fragment count, R = receipt count, G = graph traversal

### Optimization Opportunities

1. **Hardware acceleration**: SHA-256 via SHA-NI or AVX-512
2. **Batch verification**: Verify multiple fragments in parallel
3. **Lazy computation**: Skip Merkle proofs unless fragment load is needed
4. **Incremental verification**: Cache intermediate proof results
5. **Receipt pruning**: Archive old receipts, keep recent history

---

## 9. Example Integration Scenario

### Scenario: Multi-Shard LoRA Adapter Distribution

```
1. LoRA adapter is trained (Phase: EPIC 1)
   → Package lineage created
   → Adapter provenance recorded

2. Adapter is placed across 8 shards (Phase: EPIC 3.3)
   → Fragmented into 8 pieces
   → Merkle tree built: 8 leaves → 4 → 2 → 1 (root)
   → root_hash stored in manifest

3. Integrity verification on each shard (Phase: EPIC 3.4)
   → Each shard verifies its fragment via Merkle proof
   → Cost: O(3) hash operations per shard (3 = log2(8))
   → All shards report VERIFIED

4. Query planner selects fragments (Phase: EPIC 3.6)
   → Needs adapter for inference
   → Requests fragment from Shard A
   → Receives fragment + Merkle proof
   → Verifies proof: O(3) ops
   → Uses fragment

5. Model switch triggers rebuild (Later: EPIC 3.5)
   → Adapter lineage now outdated
   → Integrity state → STALE
   → Recovery manager rebuilds adapter from source
   → New manifest created with fresh receipt
   → Verification state → VERIFIED
   → New placement across shards
```

---

## 10. References

- `src/distributed_tensor/include/integrity_verification.h`
- `src/distributed_tensor/src/integrity_verification.cc`
- `docs/EPIC3_INTEGRITY_MODEL_IMPLEMENTATION.md`
- `docs/EPIC3_MANIFEST_SCHEMA.md`
- `docs/EPIC3_SHARD_PLACEMENT.md`
- `docs/EPIC3_RECOVERY_STRATEGY.md`

---
