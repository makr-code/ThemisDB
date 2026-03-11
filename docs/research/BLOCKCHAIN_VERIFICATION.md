# Blockchain Verification

**Module:** `include/importers/blockchain_integrity.h` / `src/importers/blockchain_integrity.cpp`
**Version:** 3.0.0
**Status:** ✅ Implemented

---

## Overview

The `BlockchainIntegrityVerifier` provides cryptographic tamper-evidence for
mission-critical data imports (healthcare, financial, legal).  It:

1. Computes a **SHA-256 Merkle tree** over a batch of imported records.
2. Optionally **anchors the Merkle root** on an external blockchain for
   independently verifiable tamper-evidence.
3. Allows any individual record to be **proven present** in the original batch
   via a sibling-hash proof path.

---

## Scientific Foundations

| Paper / Standard | Relevance |
|-----------------|-----------|
| Merkle, R. C. (1987) *A Digital Signature Based on a Conventional Encryption Function* | Merkle tree original paper |
| Nakamoto, S. (2008) *Bitcoin: A Peer-to-Peer Electronic Cash System* | Blockchain anchoring concept |
| NIST FIPS 180-4 (2015) *Secure Hash Standard* | SHA-256 specification |
| ISO/IEC 9796-2 (2002) *Digital signature schemes* | Integrity proof patterns |
| HIPAA § 164.312(c)(1) | Integrity controls for PHI |
| SOX Section 404 | Internal control over financial reporting |

---

## Merkle Tree Construction

### Algorithm

```
1. For each record rᵢ in batch:
   leafᵢ = SHA-256(canonical_json(rᵢ))

2. Build binary tree bottom-up:
   If odd number of leaves, duplicate the last leaf.
   parentᵢ = SHA-256(childₗₑₓₜ ∥ childᵣᵢᵍₕₜ)

3. Root = hash at apex of tree
```

**Canonical JSON**: keys are sorted alphabetically before hashing to ensure
that `{"b":2,"a":1}` and `{"a":1,"b":2}` produce identical hashes.

### Implementation Detail

ThemisDB uses `std::hash<std::string>` in the default build (portable, no OpenSSL
dependency).  Production builds that define `THEMIS_ENABLE_OPENSSL` replace this
with `EVP_DigestFinal_ex(SHA256)` for FIPS 140-2 compliance.

```cpp
BlockchainIntegrityVerifier::MerkleTreeBuilder builder;
auto root = builder.buildMerkleTree(records);
// root == "a3f4b2c1..." (hex-encoded SHA-256 Merkle root)
```

---

## Proof-of-Inclusion

A single record can be verified against the Merkle root using only the
**sibling hashes** along the path from the leaf to the root.  This allows
verification without storing the full batch.

```
Leaf: SHA-256(record[3])
       │
       ├── sibling[0]: SHA-256(record[2])  (paired at depth 0)
       ├── sibling[1]: parent of [record[0], record[1]] (at depth 1)
       └── root = SHA-256(sibling[1] ∥ SHA-256(leaf ∥ sibling[0]))
```

```cpp
bool ok = builder.verifyRecordInTree(record, root, sibling_hashes);
```

---

## Blockchain Anchoring

### Offline / Air-Gapped Mode (default)

When no blockchain connection is available, `BlockchainAnchor::anchorToBlockchain()`
produces a **synthetic proof** whose `blockchain_tx_hash` is derived deterministically
from the Merkle root.  This allows the full verification workflow to be exercised in
testing environments.

```cpp
BlockchainIntegrityVerifier::BlockchainAnchor anchor;
auto proof = anchor.anchorToBlockchain(root);
// proof.merkle_root           == root
// proof.blockchain_tx_hash    == <synthetic deterministic hash>
// proof.timestamp_rfc3339     == <current time>
// proof.smart_contract_address == "0x0000...0000"
```

### Production Mode (Ethereum / Hyperledger)

In production, `anchorToBlockchain` submits the Merkle root to a smart contract via
the EVM JSON-RPC interface.  The transaction hash serves as an immutable, independently
verifiable record of the import event.

```solidity
// Solidity smart contract (reference)
contract ThemisDBIntegrity {
    event MerkleRootAnchored(bytes32 root, uint256 timestamp, address importer);

    function anchorRoot(bytes32 root) external {
        emit MerkleRootAnchored(root, block.timestamp, msg.sender);
    }
}
```

---

## Verification Workflow

```
Import batch
    │
    ▼
MerkleTreeBuilder::buildMerkleTree(records)
    │
    ├── Merkle root = "a3f4b2c1..."
    │
    ▼
BlockchainAnchor::anchorToBlockchain(root)
    │
    ├── IntegrityProof {
    │     merkle_root: "a3f4b2c1...",
    │     blockchain_tx_hash: "0xdeadbeef...",
    │     timestamp_rfc3339: "2024-06-01T12:00:00Z"
    │   }
    │
    ▼  (days / weeks / months later)

Auditor queries blockchain for tx_hash
    │
    ▼
Retrieve original records from archive
    │
    ▼
MerkleTreeBuilder::buildMerkleTree(archived_records)
    │
    ├── Recomputed root matches blockchain-anchored root?
    │   YES → data integrity confirmed
    │   NO  → tampering detected
    │
    ▼
BlockchainAnchor::verifyBlockchainAnchor(proof)
```

---

## Integration with Audit Trail

The `ImmutableAuditLog` (see `audit_trail.h`) records a `RECORD_IMPORTED` event that
includes the `merkle_root` in its `details` field.  This links every import batch to
its integrity proof without requiring a separate lookup.

```cpp
AuditedImporter::AuditEvent ev;
ev.type    = AuditedImporter::EventType::RECORD_IMPORTED;
ev.details = json{
    {"batch_size",    records.size()},
    {"merkle_root",   proof.merkle_root},
    {"blockchain_tx", proof.blockchain_tx_hash}
};
audit_log.recordEvent(ev);
```

---

## Compliance Mapping

| Regulation | Requirement | ThemisDB Implementation |
|-----------|-------------|------------------------|
| HIPAA § 164.312(c)(1) | Integrity controls for ePHI | Merkle root + blockchain anchor |
| SOX § 404 | Internal control over financial reporting | Immutable audit log + Merkle proof |
| GDPR Art. 5(1)(f) | Integrity and confidentiality | Chain hash + tamper-detection |
| eIDAS | Electronic signature equivalent | Blockchain timestamp |

---

## Limitations

- The default `std::hash` is NOT cryptographically secure; it is collision-resistant only
  under random inputs.  Enable `THEMIS_ENABLE_OPENSSL` for production SHA-256.
- Blockchain anchoring requires network access and gas fees (Ethereum) or channel setup
  (Hyperledger); in air-gapped environments use the offline synthetic proof.
- Merkle proof verification requires retaining the sibling-hash path at import time; if
  only the root is stored, full batch reverification requires recomputing from the archive.

---

## References

- Merkle, R. C. (1987). A Digital Signature Based on a Conventional Encryption Function. *CRYPTO 1987*, LNCS 293, pp. 369–378.
- Nakamoto, S. (2008). Bitcoin: A Peer-to-Peer Electronic Cash System. *bitcoin.org/bitcoin.pdf*.
- NIST FIPS 180-4 (2015). *Secure Hash Standard (SHS)*. National Institute of Standards and Technology.
- US HIPAA (1996). Health Insurance Portability and Accountability Act, § 164.312(c)(1).
- US Sarbanes-Oxley Act (2002). Section 404 – Management Assessment of Internal Controls.
