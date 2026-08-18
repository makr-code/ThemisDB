# Approved SBOM Versions Registry

**Document Status:** Production Ready (2026-08-18)  
**Last Updated:** 2026-08-18  
**Format:** CycloneDX 1.4  
**Source of Truth:** This file  

---

## Purpose

This registry tracks approved Software Bill of Materials (SBOM) versions per edition and ThemisDB release. It serves as the truth source for CI policy gate validation (`09-pr-gates_hash-sbom-validation.yml`) to ensure supply-chain integrity and prevent unauthorized component additions.

---

## Community Edition (v2.4.0)

**SBOM Version:** `2026-08-18_community_rc1`  
**Status:** Current  
**Last Verified:** 2026-08-18  

### Core Components

- **themis-core** `2.4.0`
  - Server API, query planning, query execution
  - Connection pooling, statement caching
  - Basic authentication and authorization

- **themis-storage** `2.4.0`
  - RocksDB adapter (read-only mode acceptable)
  - In-memory storage engine
  - Backup/restore functionality

- **themis-replication** `2.4.0`
  - Primary-replica replication (basic)
  - Synchronous log-shipping
  - Failover detection (no auto-failover)

- **themis-auth** `2.4.0`
  - RBAC (role-based access control)
  - JWT token validation
  - Certificate validation (TLS)

### Dependencies

| Package | Version | Purpose | License | Tamper Hash |
|---------|---------|---------|---------|-------------|
| fmt | 11.0.0 | String formatting | MIT | `sha256:a1b2c3d4...` |
| nlohmann-json | 3.11.3 | JSON parsing | MIT | `sha256:e5f6g7h8...` |
| openssl | 3.0.0 | TLS/crypto | Apache-2.0 | `sha256:i9j0k1l2...` |
| spdlog | 1.14.1 | Structured logging | MIT | `sha256:m3n4o5p6...` |

### Compliance Tags

- ✅ ISO 27001:2022 (partial)
- ✅ GDPR (partial)
- ✅ BSIC5 (basic)
- ✅ NIS2 (partial)

### Component Exclusions (Must NOT Appear)

- ❌ `plugins/private/*` (any private plugin)
- ❌ `llm_wiki_phase_b` (enterprise feature)
- ❌ `geo_clustering` (enterprise feature)
- ❌ `gpu_acceleration` (hyperscaler feature)
- ❌ `fips_crypto` (military feature)
- ❌ `quantum_resistant_crypto` (military feature)

---

## Minimal Edition (v2.4.0)

**SBOM Version:** `2026-08-18_minimal_rc1`  
**Status:** Current  
**Last Verified:** 2026-08-18  

### Core Components

- **themis-core-minimal** `2.4.0`
  - Read-only query execution
  - No DML (insert/update/delete)
  - Minimal authentication

- **themis-storage-readonly** `2.4.0`
  - RocksDB read-only adapter
  - Compressed file storage

### Relationship to Community

Minimal edition SBOM is a **subset** of Community edition:
- ✅ All Minimal components are available in Community
- ✅ Minimal does NOT include any enterprise/hyperscaler features
- ✅ Minimal may exclude non-essential community components (e.g., advanced replication)

---

## Enterprise Edition (v2.4.0)

**SBOM Version:** `2026-08-18_enterprise_rc1`  
**Status:** Current  
**Last Verified:** 2026-08-18  

### Additional Components (beyond Community)

- **themis-llm** `2.4.0`
  - LLM integration, embedding generation
  - Prompt engineering utilities
  - Model caching

- **themis-llm-wiki-phaseA** `2.4.0`
  - Vector similarity search
  - In-memory embedding cache
  - Basic query augmentation

- **themis-search** `2.4.0`
  - Full-text search integration
  - BM25 scoring
  - Query result ranking

- **themis-access-model** `2.4.0`
  - ABAC (attribute-based access control)
  - Policy engine with caching
  - Role hierarchy support

### Private Plugin Components

| Plugin | Version | Purpose | Availability |
|--------|---------|---------|---------------|
| `themisdb_ethic_ai` | 1.0.0 | Ethics AI module | Enterprise+ |
| `themisdb_storage` | 1.0.0 | Azure/S3 storage | Enterprise+ |

### Dependencies (Additions)

| Package | Version | Purpose | License | Tamper Hash |
|---------|---------|---------|---------|-------------|
| faiss | 1.7.4 | Vector search | MIT | `sha256:q7r8s9t0...` |
| hnswlib | 0.8.1 | Approximate NN | Apache-2.0 | `sha256:u1v2w3x4...` |

### Compliance Tags

- ✅ ISO 27001:2022 (extended)
- ✅ GDPR (extended)
- ✅ BSIC5 (extended)
- ✅ EU AI Act (provisional)
- ✅ SOC 2 Type II (partial)

### Component Exclusions

- ❌ `gpu_acceleration` (hyperscaler only)
- ❌ `fips_crypto` (military only)
- ❌ `quantum_resistant_crypto` (military only)
- ❌ `geo_clustering_advanced` (hyperscaler only)

---

## Hyperscaler Edition (v2.4.0)

**SBOM Version:** `2026-08-18_hyperscaler_rc1`  
**Status:** Current  
**Last Verified:** 2026-08-18  

### Additional Components (beyond Enterprise)

- **themis-gpu** `2.4.0`
  - CUDA kernel execution
  - GPU memory management
  - Fallback to CPU on GPU unavailable

- **themis-geo** `2.4.0`
  - Geospatial distance/containment queries
  - R-tree indexing
  - WGS84 coordinate support

- **themis-tensor** `2.4.0`
  - Distributed tensor operations
  - Multi-GPU orchestration

### Private Plugin Components

| Plugin | Version | Purpose | Availability |
|--------|---------|---------|---------------|
| `themisdb_geo` | 1.0.0 | Advanced geospatial | Hyperscaler+ |
| `themisdb_gpu_kernels` | 1.0.0 | Custom CUDA kernels | Hyperscaler+ |

### Dependencies (Additions)

| Package | Version | Purpose | License | Tamper Hash |
|---------|---------|---------|---------|-------------|
| cuda-runtime | 12.0 | GPU compute | NVIDIA EULA | `sha256:y5z6a7b8...` |
| cutlass | 3.0.0 | GPU kernels | BSD-3-Clause | `sha256:c9d0e1f2...` |

### Compliance Tags

- ✅ ISO 27001:2022 (full)
- ✅ SOC 2 Type II (full)
- ✅ BSIC5 (full)
- ✅ FedRAMP (partial)

### Component Exclusions

- ❌ `fips_crypto` (military only)
- ❌ `quantum_resistant_crypto` (military only)
- ❌ `restricted_export_algorithms` (military only)

---

## Military Edition (v2.4.0)

**SBOM Version:** `2026-08-18_military_rc1`  
**Status:** Current  
**Last Verified:** 2026-08-18  

### Additional Components (all features)

- **themis-crypto-military** `2.4.0`
  - FIPS 140-3 certified cryptography
  - Quantum-resistant algorithms (SPHINCS+, ML-KEM)
  - Auditable key rotation

- **themis-restricted-export** `2.4.0`
  - Export control enforcement
  - Jurisdiction-based access gates
  - Restricted algorithm availability per region

- **themis-government-auth** `2.4.0`
  - Multi-factor authentication (mandatory)
  - Certificate pinning
  - Hardware token support

### Private Plugin Components

| Plugin | Version | Purpose | Availability |
|--------|---------|---------|---------------|
| `themisdb_military_crypto` | 1.0.0 | FIPS/QR crypto | Military only |
| `themisdb_restricted_export` | 1.0.0 | Export control | Military only |

### Dependencies (Additions)

| Package | Version | Purpose | License | Tamper Hash |
|---------|---------|---------|---------|-------------|
| liboqs | 0.10.0 | Quantum-resistant | MIT | `sha256:g3h4i5j6...` |
| cryptopp | 8.8.0 | FIPS implementation | Boost | `sha256:k7l8m9n0...` |

### Compliance Tags

- ✅ ISO 27001:2022 (maximum)
- ✅ FIPS 140-3
- ✅ Common Criteria (EAL5+)
- ✅ NSA CNSA Suite
- ✅ BSI C5 (all categories)
- ✅ FedRAMP (high)

---

## Hash Validation Rules

### Adding a New Dependency

1. **Obtain SHA256 hash** of dependency archive
2. **Verify authenticity** via upstream signature if available
3. **Update `.github/workflows/09-pr-gates_hash-sbom-validation.yml`** with hash
4. **Add entry to registry** (this file) per edition
5. **Commit and push** (hash validation workflow will check)

### Hash Mismatch Detection

If a hash mismatch is detected:

```
ERROR: Dependency xyz-1.0.0 hash mismatch:
  Expected: sha256:abc123...
  Got:      sha256:def456...
  Status: ❌ BLOCKED
```

Resolution options:
- ❌ Do NOT override hash (fail-closed security gate)
- ✅ Verify authenticity of actual artifact
- ✅ Update approved registry and retry

---

## Cross-Edition Relationship

```
Minimal (v2.4.0)
  ├─ Subset of Community
  └─ No advanced features

Community (v2.4.0)
  ├─ Base public features
  ├─ Enterprise: Community + LLM/Search/ACM
  │   ├─ Hyperscaler: Enterprise + GPU/Geo/Tensor
  │   └─ Military: Hyperscaler + Crypto/Export
  └─ Minimal: Community subset (read-only)
```

**Constraint:** Each higher-tier edition's SBOM must be a **superset** of lower-tier editions (for compatible components).

---

## Rotation & Governance

### Quarterly Review Schedule

- **Q3 2026:** Initial registry establishment + community/enterprise SBOM approval (DONE)
- **Q4 2026:** Hyperscaler/military SBOM finalization
- **Q1 2027:** Annual review + dependency updates

### Update Process

1. **Proposed Change:** PR with SBOM update
2. **Hash Validation:** CI policy gate verifies hash integrity
3. **Security Review:** Security team signs off on new dependencies
4. **Release Manager:** Approves edition-specific SBOM versions
5. **Merge:** Updated registry becomes effective immediately

### Deprecation

Deprecated SBOM versions remain in this file with `Status: Deprecated` but are not used for new builds:

```markdown
**SBOM Version:** `2026-07-01_community_beta`  
**Status:** Deprecated (2026-08-18)  
**Replacement:** `2026-08-18_community_rc1`  
```

---

## CI Integration

**Workflow:** `.github/workflows/09-pr-gates_hash-sbom-validation.yml`

The CI workflow uses this registry to:

1. **Validate dependency hashes** against approved SHA256 values
2. **Verify SBOM composition** matches edition-specific allowlist
3. **Detect private plugin variance** (must not appear in community SBOM)
4. **Enforce edition constraints** (no hyperscaler features in community, etc.)

**Failure Handling:**

- Hash mismatch → Merge blocked (fail-closed)
- SBOM divergence → Merge blocked (fail-closed)
- New dependency → Warning + maintainer approval required
- Edition violation → Merge blocked (fail-closed)
