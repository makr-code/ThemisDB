## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


# EPIC 3.4 Integrity Model Implementation Guide

<!-- Status: complete | implementation | validated: 2026-07-04 -->

## Summary

This document defines the **integrity and verification model** for distributed tensor artifacts in the Themis sharding fabric. It covers:

1. **Content-Hash Model**: SHA-256 integrity detection of artifact content
2. **Merkle Fragment Verification**: O(log N) proof verification for shard fragments
3. **Receipt-Chain Semantics**: Tamper-evident audit trail linking artifacts to package lineage
4. **Provenance Verification Hooks**: Integration with graph and package validation
5. **Verification State Machine**: State transitions and failure handling

---

## 1. Overview

### Motivation

Distributed tensor artifacts must be **verifiable across shards and rebuild workflows**. This requires:

- **Detecting corruption**: Identify modified or corrupted tensor data
- **Fragment verification**: Prove that a shard fragment belongs to the full artifact
- **Lineage tracking**: Link artifacts to package rebuild sources
- **Audit trails**: Record verification events for compliance

### Design Principles

1. **Manifest-embedded metadata**: All integrity info lives in the artifact manifest (no separate verification DB)
2. **Lazy proof computation**: Merkle proofs computed on-demand, not pre-stored
3. **Receipt chains link to lineage**: Not isolated artifact chains, but connected to package events
4. **Ephemeral verification state**: Trust-on-load; corruption detection is automatic
5. **Provenance integration**: Verification hooks integrate with graph validation

---

## 2. Content-Hash Model

### Definition

A **content hash** is a SHA-256 cryptographic fingerprint of artifact content:

```cpp
struct ContentHash {
    std::string value;  // 64-char lowercase hex SHA-256
};
```

### Coverage

Content hash covers:
- ✅ Raw tensor payload (when available)
- ✅ Serialized metadata (deterministic JSON order)
- ✅ Reconstruction instructions (if present)

Content hash does NOT cover:
- ❌ Provenance references (may be updated)
- ❌ Manifest metadata (versioning, timestamps)
- ❌ Fragment placement (runtime decision)

### Computation

```cpp
// Example: compute content hash from tensor data
std::string tensor_data = get_tensor_bytes();
ContentHash hash;
hash.value = computeSHA256(tensor_data);
```

### Validation

```cpp
// Check if hash is valid
if (!hash.isValid()) {
    spdlog::error("Invalid hash format");
}
```

---

## 3. Merkle Fragment Verification

### Problem

Shards may contain fragments of large artifacts. Without Merkle proofs, verifying a fragment requires downloading the **entire artifact**.

### Solution

A **Merkle proof** is a minimal set of sibling hashes enabling verification without full download:

```cpp
struct MerkleProof {
    std::string artifact_id;
    uint64_t fragment_index;
    std::string fragment_hash;              // SHA-256 of fragment
    std::vector<MerkleProofComponent> proof_path;  // O(log N) hashes
    std::string artifact_root_hash;         // Expected root
};
```

### Merkle Semantics

**Merkle Tree Structure:**
```
              Root (artifact_root_hash)
             /                        \
          H(AB)                      H(CD)
         /    \                     /    \
      H(A)  H(B)               H(C)  H(D)
       |      |                 |      |
    Frag_A  Frag_B           Frag_C  Frag_D
```

**Verification (example for Fragment_A):**
1. Start: `current = SHA-256(Frag_A)`
2. Combine with sibling: `current = SHA-256(current || H(B))`
3. Combine with next: `current = SHA-256(current || H(CD))`
4. Verify: `current == Root`

### Verification Cost

- **Proof size**: O(log N) hashes (N = number of fragments)
- **Verification cost**: O(log N) hash operations
- **No download**: Unverified fragments are never downloaded

### API Usage

```cpp
// Verify a fragment against the proof
MerkleProof proof = get_proof_from_manifest();
bool verified = proof.verify(artifact_root_hash);

if (verified) {
    spdlog::info("Fragment {} verified to belong to artifact {}",
                 proof.fragment_index, proof.artifact_id);
}

// Verification cost (diagnostic)
size_t cost = proof.verificationCost();  // e.g., 8 for 256 fragments
```

---

## 4. Receipt-Chain Verification

### Problem

How do we prove that an artifact existed with a specific content hash at a historical point? How do we detect when artifacts are replaced or rebuilt?

### Solution

A **verification receipt** is an immutable record linking an artifact to:
- Its content hash (integrity proof)
- Its package lineage (provenance link)
- A timestamp (temporal anchor)
- Prior receipts (chain formation)

```cpp
struct VerificationReceipt {
    std::string receipt_id;              // Unique ID
    std::string artifact_id;             // What this receipt certifies
    std::string content_hash;            // SHA-256 at receipt time
    std::string timestamp;               // ISO 8601 UTC
    std::string parent_receipt_hash;     // Link to previous (empty for genesis)
    std::string receipt_hash;            // SHA-256 of this receipt
    std::string package_lineage_hash;    // Link to package rebuild source
};
```

### Receipt-Chain Semantics

**Tamper-Evident Linking:**
```
Genesis                Current
  |                      |
  v                      v
Receipt_0              Receipt_N
  |                      ^
  | parent_hash ← ─ ─ ─ ─ |
  |                        
  └─ ─ ─ ─ ─ ─ ─ ─ ─ ─ → ┘
         (chain)

Each receipt computes its hash AFTER linking to parent:
receipt_hash = SHA-256({
    receipt_id,
    artifact_id,
    content_hash,
    timestamp,
    parent_receipt_hash,    ← Ensures tampering is detected
    package_lineage_hash,
    metadata
})
```

### Verification Workflow

```cpp
ReceiptChain chain = load_receipt_chain(artifact_id);

// 1. Verify entire chain is unmodified
bool chain_valid = chain.verifyChainIntegrity();
if (!chain_valid) {
    spdlog::error("Receipt chain has been tampered with");
    return CORRUPT;
}

// 2. Check artifact state at historical point
auto receipts = chain.getAllReceipts();
for (const auto& receipt : receipts) {
    if (receipt.timestamp <= query_time) {
        // Artifact was in this state at query_time
        spdlog::info("Artifact state at {}: {}", query_time, receipt.content_hash);
    }
}

// 3. Link to package lineage
auto head = chain.getHeadReceipt();
if (head && head->package_lineage_hash != current_lineage) {
    spdlog::warn("Artifact lineage mismatch; rebuild recommended");
    return STALE;
}
```

### Use Cases

1. **Compliance queries**: "Prove artifact X existed with hash Y on date Z"
2. **Tamper detection**: "Detect when an artifact was replaced"
3. **Rebuild linkage**: "Link artifact state to model-switch or training event"
4. **Audit trails**: "Reconstruct artifact history for compliance review"

---

## 5. Verification State Machine

### States

```
UNVERIFIED
    ↓ (on load)
VERIFIED ←─ ┐
    ↓        │
VERIFIED_FRAGMENTS
    ↓ (or fragment check fails)
    ↓→ CORRUPT (if hash mismatch)
    ↓
STALE (if lineage outdated)
```

### State Definitions

| State | Meaning | Action |
|-------|---------|--------|
| **UNVERIFIED** | Initial; integrity unknown | Load and verify ASAP |
| **VERIFIED** | Content hash OK; full integrity confirmed | Safe to use |
| **VERIFIED_FRAGMENTS** | Fragment-level Merkle proofs verified | Use for partial loads |
| **CORRUPT** | Integrity check failed | Reject; trigger recovery |
| **STALE** | Content OK but provenance outdated | Use with warning; rebuild advised |

### State Transitions

```cpp
// Load from shard storage
VerificationState state = UNVERIFIED;

// Try to verify content hash
if (computed_hash == manifest_hash) {
    state = VERIFIED;
} else {
    state = CORRUPT;
    spdlog::error("Artifact corrupted");
    trigger_recovery();
    return;
}

// If fragment-level verification needed
if (verify_merkle_proofs(manifest)) {
    state = VERIFIED_FRAGMENTS;
} else {
    spdlog::warn("Fragment verification failed");
    // Still in VERIFIED state (full artifact is OK)
}

// Check provenance freshness
if (!check_lineage_current()) {
    state = STALE;
    spdlog::warn("Artifact lineage stale; rebuild recommended");
}
```

---

## 6. Provenance Verification Hooks

### Problem

Integrity verification must integrate with package lineage and graph provenance validation without circular dependencies.

### Solution

**Provenance Verification Hook**: A plugin point for custom validation logic.

```cpp
class ProvenanceVerificationHook {
public:
    virtual VerificationResult verifyProvenance(
        const std::string& artifact_id,
        const std::string& content_hash,
        const std::string& package_lineage_hash) = 0;

    virtual std::string getCurrentPackageLineage() = 0;
};
```

### Integration Pattern

```
Integrity Module          Provenance Module
    ↓                            ↑
    | verifyProvenance()         |
    | • artifact_id              | return compat status
    | • content_hash             |
    | • lineage_hash             |
    └─────────────────────→ Check package lineage
                                  & graph validation
                                   ↓
                           Compatible? Rebuild needed?
```

### Usage Example

```cpp
// Register provenance hook with integrity verifier
auto verifier = IntegrityVerifier();
verifier.setProvenanceHook(make_shared<MyProvenanceHook>());

// During verification, hook is called automatically
VerificationResult result = verifier.verify(manifest);
// → Hook checks package lineage
// → Hook checks graph compatibility
// → Result includes provenance status
```

---

## 7. Audit Trail Support

### Audit Callback

```cpp
class VerificationAuditTrail {
public:
    virtual void recordVerificationEvent(
        const std::string& artifact_id,
        const VerificationResult& result,
        const std::string& timestamp) = 0;
};
```

### Usage

```cpp
verifier.setAuditTrail(audit);

VerificationResult result = verifier.verify(manifest);
// → Audit trail automatically records:
//   - artifact_id
//   - result (success, state, hashes, errors)
//   - timestamp
```

---

## 8. Integration with Manifest

### Manifest Fields

The artifact manifest (`ArtifactManifest`) includes:

```cpp
struct ArtifactManifest {
    // ... existing fields ...

    // Integrity metadata
    std::string content_hash;                  // SHA-256 of artifact content
    std::string manifest_hash;                 // SHA-256 of manifest itself
    
    // Merkle fragment verification
    std::string artifact_root_hash;            // Root of fragment Merkle tree
    std::vector<MerkleProofComponent> fragment_merkle_path;
    
    // Receipt-chain linkage
    json receipt_chain_head;                   // Most recent receipt (JSON)
    
    // Provenance linkage
    std::string package_lineage_hash;          // Link to package rebuild source
};
```

---

## 9. Failure Modes & Recovery

### Failure Mode: Corruption Detected

```
Scenario: Fragment mismatch during load

1. Load fragment from shard
2. Compute hash: actual_hash ≠ manifest_hash
3. Verification State → CORRUPT
4. Action: Trigger recovery (erasure decode, replication, rebuild)
```

### Failure Mode: Partial Receipt Chain Corruption

```
Scenario: Attacker modifies receipt N in chain

1. Verify chain: compute_hash(receipt_N) ≠ receipt_N.receipt_hash
2. Verification fails
3. Entire chain is marked corrupt
4. Cannot trust any artifact history from this chain
```

### Failure Mode: Stale Lineage

```
Scenario: Package was rebuilt; old artifact still present

1. Verify content hash: OK
2. Check lineage: artifact_lineage_hash ≠ current_lineage_hash
3. Verification State → STALE
4. Action: Warn; recommend rebuild
```

---

## 10. Performance Considerations

### Content-Hash Verification

- **Cost**: O(N) where N = artifact size
- **Timing**: ~100 GB/sec for SHA-256 on modern CPUs
- **When**: On load, or periodic background audit

### Merkle Proof Verification

- **Cost**: O(log N) hash operations where N = number of fragments
- **Example**: 256 fragments = 8 hash ops
- **When**: On fragment load (optional; full artifact already verified)

### Receipt Chain Verification

- **Cost**: O(M) hash operations where M = receipt chain length
- **Example**: 1000 receipts = 1000 hash ops
- **When**: On compliance query; not in hot path

---

## 11. Example: Complete Verification Workflow

```cpp
// 1. Load manifest from shard
ArtifactManifest manifest = load_manifest(artifact_id);

// 2. Verify content hash
std::string actual_hash = computeSHA256(load_artifact_bytes());
if (actual_hash != manifest.content_hash) {
    spdlog::error("Corruption detected");
    return CORRUPT;
}

// 3. Verify fragment membership (if needed)
if (need_fragment_verification()) {
    MerkleProof proof = reconstructProofFromManifest(manifest);
    if (!proof.verify(manifest.artifact_root_hash)) {
        spdlog::error("Fragment mismatch");
        return CORRUPT;
    }
}

// 4. Check provenance (via hook)
auto prov_result = provenance_hook_->verifyProvenance(
    artifact_id,
    manifest.content_hash,
    manifest.package_lineage_hash
);
if (!prov_result.success) {
    spdlog::warn("Provenance check failed");
    // May return STALE, not CORRUPT
}

// 5. Verify receipt chain (if needed for audit)
if (need_audit_verification()) {
    ReceiptChain chain = load_receipt_chain(artifact_id);
    if (!chain.verifyChainIntegrity()) {
        spdlog::error("Receipt chain corrupted");
        return CORRUPT;
    }
}

// 6. Record in audit trail
audit_trail_->recordVerificationEvent(
    artifact_id,
    verification_result,
    now_timestamp()
);

// 7. Use artifact
if (result.state == VerificationState::VERIFIED ||
    result.state == VerificationState::VERIFIED_FRAGMENTS) {
    return use_artifact(artifact);
}
```

---

## 12. References

- `DISTRIBUTED_TENSOR_SHARDING.md` (Section 8: Integrity, Provenance, Auditability)
- `EPIC3_MANIFEST_SCHEMA.md` (Manifest field definitions)
- `include/llm/lora_framework/lora_provenance.h` (Existing receipt-chain pattern)
- `include/utils/hash_util.h` (Hash utilities)

---

## 13. Future Enhancements

### Short-term (Phase 2-3)

- [ ] Incremental Merkle proof updates (for growing artifacts)
- [ ] Batch verification optimization (multiple receipts in one pass)
- [ ] Hardware-accelerated SHA-256 (AVX-512, SHA-NI)

### Mid-term (Phase 4-5)

- [ ] Zero-knowledge proof integration (for privacy-preserving verification)
- [ ] Signature aggregation (BLS signatures for receipt chains)
- [ ] Cross-shard verification coordination

### Long-term (Phase 6-7)

- [ ] Distributed consensus for receipt chain heads
- [ ] Accountable recovery proofs (prove why recovery was needed)
- [ ] Cryptographic timestamping (RFC 3161 integration)

---

## 14. Acceptance Criteria (Phase 1)

- [x] Content-hash model documented and API defined
- [x] Merkle proof verification algorithm documented
- [x] Receipt-chain semantics and verification rules documented
- [x] Provenance hook interface defined
- [x] Verification state machine documented
- [x] Integration with manifest schema documented
- [x] Failure modes and recovery strategies documented
- [x] Example verification workflow provided

---
