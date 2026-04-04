# LoRA Adapter Provenance – Compliance and Security Audit Guide

**Version:** 1.0  
**Date:** 2026-02-21  
**Scope:** ThemisDB LoRA Adapter Provenance System  

---

## Table of Contents

1. [Overview](#1-overview)
2. [Architecture Summary](#2-architecture-summary)
3. [Local Adapter Provenance](#3-local-adapter-provenance)
4. [External Adapter Import and Validation](#4-external-adapter-import-and-validation)
5. [MVCC Snapshots and Time-Travel](#5-mvcc-snapshots-and-time-travel)
6. [Merkle-Chained Inference Audit Log](#6-merkle-chained-inference-audit-log)
7. [AdapterRegistry Integration](#7-adapterregistry-integration)
8. [CLI Tooling for Provenance Proof](#8-cli-tooling-for-provenance-proof)
9. [Compliance Requirements Matrix](#9-compliance-requirements-matrix)
10. [Security Threat Model](#10-security-threat-model)
11. [Operational Runbook](#11-operational-runbook)

---

## 1. Overview

ThemisDB's LoRA Adapter Provenance system provides **full cryptographic auditability** for every
LoRA adapter and every inference event that uses one. The design satisfies the requirements of:

- **eIDAS Regulation (EU 910/2014)** – qualified electronic signatures for adapter artefacts
- **GDPR / DSGVO** – traceability of automated decision making
- **ISO 27001** – information security event logging and monitoring
- **SOC 2 Type II** – cryptographic evidence of operational integrity

The system consists of four interlocking components:

| Component | File(s) | Purpose |
|---|---|---|
| `LoRAProvenanceRecord` | `lora_provenance.h/.cpp` | Cryptographic provenance for locally trained adapters |
| `ExternalAdapterProvenance` | `lora_provenance.h/.cpp` | Provenance and validation for imported adapters |
| `AdapterSnapshot` | `lora_provenance.h/.cpp` | MVCC snapshots enabling time-travel |
| `InferenceAuditEntry` | `lora_provenance.h/.cpp` | Immutable Merkle-chained per-inference log entries |
| `LoRAProvenanceManager` | `lora_provenance.h/.cpp` | Central manager for all of the above |
| `AdapterRegistry` (extended) | `adapter_registry.h/.cpp` | Provenance-aware registry integration |
| `lora-provenance` CLI | `tools/lora_provenance_cli.cpp` | Operator tooling for provenance proof |

---

## 2. Architecture Summary

```
┌──────────────────────────────────────────────────────────────────┐
│                        AdapterRegistry                           │
│  registerAdapter()  → attachProvenance()  → recordInferenceAudit │
│  verifyAuditChain() ← getInferenceAuditLog() ← getProvenance()  │
└──────────────┬───────────────────────────────────────────────────┘
               │ delegates to
               ▼
┌──────────────────────────────────────────────────────────────────┐
│                    LoRAProvenanceManager                         │
│                                                                  │
│  storeProvenance()          importExternalAdapter()              │
│  getProvenance()            getExternalProvenance()              │
│  createSnapshot()           listSnapshots()  getSnapshot()       │
│  appendAuditEntry()         getAuditLog()    verifyAuditChain()  │
│  sha256Hex()                sha256File()                         │
└──────────────────────────────────────────────────────────────────┘
               │
               │ persists (in-memory; production: RocksDB / blob storage)
               ▼
┌──────────────────────────────────────────────────────────────────┐
│  LoRAProvenanceRecord  ExternalAdapterProvenance                 │
│  AdapterSnapshot       InferenceAuditEntry (Merkle chain)        │
└──────────────────────────────────────────────────────────────────┘
```

---

## 3. Local Adapter Provenance

### 3.1 What is recorded

A `LoRAProvenanceRecord` captures the complete training environment at the moment an adapter is
created:

| Field | Type | Description |
|---|---|---|
| `dataset_hash` | SHA-256 hex | Hash of the full training dataset bytes |
| `base_model_hash` | SHA-256 hex | Hash of the base LLM weight file |
| `hyperparameter_hash` | SHA-256 hex | Hash of the serialised hyperparameter JSON |
| `adapter_weights_hash` | SHA-256 hex | Hash of the trained adapter weight file |
| `trainer_id` | string | Identity of the entity (user/service) that ran training |
| `ca_chain` | PEM string | CA / eIDAS certificate chain of the trainer |
| `signature` | base64 | Ed25519 or ECDSA signature over the record fields |
| `created_at` | ISO 8601 UTC | Wall-clock timestamp of record creation |
| `rfc3161_timestamp` | base64 | RFC 3161 timestamp token (TSA notarisation) |
| `training_duration_secs` | double | Wall-clock training time in seconds |
| `hardware_info` | JSON object | GPU model, VRAM, CPU, RAM used during training |
| `custom_metadata` | JSON object | Application-specific extensions |

### 3.2 How to create a provenance record

```cpp
#include "llm/lora_framework/lora_provenance.h"
using namespace themis::llm::lora;

LoRAProvenanceRecord prov;
prov.dataset_hash        = LoRAProvenanceManager::sha256File("/data/legal_docs.parquet");
prov.base_model_hash     = LoRAProvenanceManager::sha256File("/models/mistral-7b.gguf");
prov.hyperparameter_hash = LoRAProvenanceManager::sha256Hex(training_config_json_str);
prov.adapter_weights_hash = LoRAProvenanceManager::sha256File("/adapters/legal-lora-v2.safetensors");
prov.trainer_id          = "ci-runner-007";
prov.created_at          = "2026-02-21T12:00:00Z";
prov.rfc3161_timestamp   = obtain_rfc3161_token(prov.adapter_weights_hash);
prov.training_duration_secs = 3600.0;
prov.hardware_info       = {{"gpu", "A100-80GB"}, {"vram_mb", 81920}};

// Attach to registry
registry.attachProvenance("legal-lora-v2", prov);
```

### 3.3 How to verify

```cpp
auto record_opt = registry.getProvenanceRecord("legal-lora-v2");
if (record_opt) {
    // Recompute adapter hash from disk and compare
    std::string live_hash = LoRAProvenanceManager::sha256File(adapter_path);
    bool intact = (live_hash == record_opt->adapter_weights_hash);
}
```

---

## 4. External Adapter Import and Validation

### 4.1 Import flow

External adapters (e.g., from HuggingFace or a partner organisation) are subject to mandatory
provenance validation before use. The validation checks:

1. **Adapter hash** – 64-character SHA-256 hex digest (structural check)
2. **Provenance signature** – non-empty (structural check; full cryptographic verification
   requires integrating an Ed25519/ECDSA implementation against the supplier's public key)
3. **Certificate chain** – non-empty and trusted CA bundle provided

Adapters that fail any check are **rejected** (not stored) and the caller receives a
`validation_errors` list.

### 4.2 Example

```cpp
ExternalAdapterProvenance ep;
ep.source_url           = "https://huggingface.co/org/legal-lora/resolve/main/adapter.safetensors";
ep.commit_hash          = "abc123def456";
ep.description          = "Legal domain German LoRA v1.2";
ep.adapter_hash         = LoRAProvenanceManager::sha256File(downloaded_file);
ep.provenance_signature = supplier_signature_base64;
ep.certificate_chain    = supplier_cert_pem;

LoRAProvenanceManager mgr;
auto result = mgr.importExternalAdapter("ext-legal-de", ep, trusted_ca_pem);

if (!result.validation_errors.empty()) {
    for (const auto& err : result.validation_errors)
        spdlog::error("Provenance validation error: {}", err);
    // Adapter NOT stored; reject it
}
```

### 4.3 CLI usage

```bash
# Validate and import an external adapter
lora-provenance import-external legal-lora-de provenance.json \
    --trusted-ca /etc/ssl/certs/ca-certificates.crt

# Allow import without signature (internal/trusted sources only)
lora-provenance import-external internal-adapter provenance.json --allow-unsigned
```

---

## 5. MVCC Snapshots and Time-Travel

### 5.1 Purpose

Snapshots preserve the complete state of an adapter (weights hash + provenance) at a point in
time. They form a singly-linked chain (via `parent_snapshot_id`), enabling deterministic
point-in-time recovery.

### 5.2 Creating a snapshot

```cpp
auto snap = mgr.createSnapshot(
    "legal-lora-v2",
    "v2.1",
    LoRAProvenanceManager::sha256File(adapter_weights_path),
    current_provenance_record
);
// snap.snapshot_id is a unique ID; snap.parent_snapshot_id links to the previous snapshot
```

### 5.3 Listing and restoring

```bash
# List snapshots
lora-provenance list-snapshots legal-lora-v2

# Output:
# Snapshot ID                        Version     Timestamp                      Parent
# ─────────────────────────────────────────────────────────────────────────────────────────
# a3f9c12e...                        v2.0        2026-02-15T09:00:00Z           (root)
# b7d1e45f...                        v2.1        2026-02-21T12:00:00Z           a3f9c12e...
```

To restore a historical state, retrieve the snapshot's `weights_hash`, verify the stored adapter
file produces the same hash, and re-deploy that version.

---

## 6. Merkle-Chained Inference Audit Log

### 6.1 Structure

Each inference is recorded as an `InferenceAuditEntry`:

| Field | Description |
|---|---|
| `entry_id` | Unique identifier (auto-generated UUID-like hex) |
| `previous_hash` | SHA-256 of the preceding entry (empty for genesis) |
| `entry_hash` | SHA-256 of this entry's canonical JSON (excl. `entry_hash` itself) |
| `timestamp` | ISO 8601 UTC |
| `request_id` | Caller-supplied correlation ID |
| `query_hash` | SHA-256 of the input prompt |
| `response_hash` | SHA-256 of the generated response |
| `model_hash` | SHA-256 of the base model weights used |
| `adapter_hash` | SHA-256 of the adapter weights used |
| `commitments` | Additional cryptographic commitments (e.g., ZK proofs) |
| `metadata` | Request-level context (user ID, session ID, etc.) |

### 6.2 Chain integrity

Each entry's `entry_hash = SHA-256(canonical_json(all_fields_except_entry_hash))`.

The `previous_hash` of entry _n_ equals the `entry_hash` of entry _n-1_, creating a tamper-
evident Merkle chain.  Any modification to a historical entry (including the `previous_hash`
field) invalidates all subsequent entries.

### 6.3 Recording an inference

```cpp
InferenceAuditEntry e;
e.request_id    = request_correlation_id;
e.query_hash    = LoRAProvenanceManager::sha256Hex(prompt_text);
e.response_hash = LoRAProvenanceManager::sha256Hex(generated_text);
e.model_hash    = base_model_hash;          // Pre-computed at model load time
e.adapter_hash  = adapter_weights_hash;     // From provenance record
e.metadata      = {{"user_id", user_id}, {"session_id", session_id}};

// Via AdapterRegistry (recommended):
registry.recordInferenceAudit("legal-lora-v2", e);

// Or directly via LoRAProvenanceManager:
mgr.appendAuditEntry("legal-lora-v2", e);
```

### 6.4 Verifying the chain

```cpp
// Programmatic verification
bool intact = registry.verifyAuditChain("legal-lora-v2");

// CLI verification (exit code 0 = intact, 1 = tampered)
lora-provenance verify legal-lora-v2
```

---

## 7. AdapterRegistry Integration

The `AdapterRegistry` class has been extended with five provenance-aware methods:

| Method | Description |
|---|---|
| `attachProvenance(id, record)` | Attach a `LoRAProvenanceRecord` to a registered adapter |
| `getProvenanceRecord(id)` | Retrieve the attached provenance record |
| `recordInferenceAudit(id, entry)` | Append an `InferenceAuditEntry` to the Merkle chain |
| `getInferenceAuditLog(id)` | Retrieve all entries in order |
| `verifyAuditChain(id)` | Verify Merkle chain integrity |

The registry delegates to an embedded `LoRAProvenanceManager` instance, so no separate
instantiation is required when using the registry.

---

## 8. CLI Tooling for Provenance Proof

The `lora-provenance` CLI tool provides operator-friendly commands for provenance workflows.

### Installation

The tool is built when `THEMIS_ENABLE_LLM=ON`:

```bash
cmake -DTHEMIS_ENABLE_LLM=ON -DTHEMIS_BUILD_TOOLS=ON ..
make lora-provenance
```

### Commands

#### `hash-file` – Compute SHA-256 hash of any file

```bash
lora-provenance hash-file /models/mistral-7b.gguf
# 3a7e9c2f...  /models/mistral-7b.gguf
```

#### `show-provenance` – Display provenance record

```bash
# Human-readable output
lora-provenance show-provenance legal-lora-v2

# Machine-readable JSON (suitable for SIEM ingestion)
lora-provenance show-provenance legal-lora-v2 --json
```

Reads `<adapter_id>.provenance.json` from the working directory.

#### `list-snapshots` – List MVCC snapshots

```bash
lora-provenance list-snapshots legal-lora-v2
```

Reads `<adapter_id>.snapshots.jsonl` from the working directory.

#### `verify` – Verify Merkle audit chain integrity

```bash
lora-provenance verify legal-lora-v2
# [OK] Merkle audit chain is intact (1847 entries).
```

- **Exit code 0**: chain is intact
- **Exit code 1**: chain is tampered or corrupt
- **Exit code 3**: audit file not found

#### `export-audit` – Export audit log as JSONL

```bash
lora-provenance export-audit legal-lora-v2 --output /tmp/audit-2026-02.jsonl
# [OK] Exported 1847 audit entries to: /tmp/audit-2026-02.jsonl
```

#### `import-external` – Import and validate external adapter

```bash
lora-provenance import-external ext-legal-de provenance.json \
    --trusted-ca /etc/ssl/certs/ca-certificates.crt
```

---

## 9. Compliance Requirements Matrix

| Requirement | Standard | Implementation |
|---|---|---|
| Cryptographic hash of training data | GDPR Art. 25, ISO 27001 A.14 | `dataset_hash` in `LoRAProvenanceRecord` |
| Hash of model weights | eIDAS, ISO 27001 | `base_model_hash` + `adapter_weights_hash` |
| Qualified electronic signature | eIDAS Art. 26 | `signature` + `ca_chain` (eIDAS certificate chain) |
| RFC 3161 timestamp proof | ETSI EN 319 421 | `rfc3161_timestamp` in `LoRAProvenanceRecord` |
| Immutable audit log | SOC 2 CC7.2, ISO 27001 A.12.4 | Merkle-chained `InferenceAuditEntry` |
| External adapter validation | ISO 27001 A.14.2 | `ExternalAdapterProvenance` + cert chain check |
| Point-in-time recovery | ISO 22301, SOC 2 A1.2 | `AdapterSnapshot` (MVCC) |
| Non-repudiation | eIDAS, GDPR | SHA-256 commitments on query/response pairs |
| Trainer identity | GDPR Art. 30 | `trainer_id` + `ca_chain` |
| Hardware reproducibility | ISO 27001 | `hardware_info` in `LoRAProvenanceRecord` |

---

## 10. Security Threat Model

### 10.1 Threats addressed

| Threat | Mitigation |
|---|---|
| **Tampering with adapter weights** | `adapter_weights_hash` + RFC 3161 timestamp prove original content |
| **Substitution of training data** | `dataset_hash` in provenance record |
| **Log tampering** | Merkle chain: modifying any entry breaks all subsequent hashes |
| **Rogue external adapter import** | Signature + certificate chain validation; absent provenance → rejection |
| **Model regression denial** | MVCC snapshots allow forensic comparison against any prior state |
| **Audit log deletion** | `verifyAuditChain()` detects missing/broken chain links |
| **Timestamp forgery** | RFC 3161 token from independent TSA provides non-forgeable proof-of-existence |

### 10.2 Known limitations

- **Full cryptographic signature verification** of the `signature` field requires the signer's
  public key or certificate. The current implementation performs structural validation only.
  A production deployment must integrate an OpenSSL/BoringSSL Ed25519 or ECDSA verification
  call against the presented `ca_chain`.

- **Persistent storage** of the `LoRAProvenanceManager`'s in-memory state requires integration
  with ThemisDB's RocksDB or blob storage backends. The current implementation is in-memory only.

- **HSM-backed signing** of the provenance record and RFC 3161 token acquisition require
  HSM/TSA integration (see `lora_storage_service.h` HSM configuration section).

---

## 11. Operational Runbook

### 11.1 Routine audit verification

Run daily via CI or cron:

```bash
#!/bin/bash
ADAPTERS=("legal-lora-v2" "medical-lora-v1" "finance-lora-v3")
EXIT=0

for ADAPTER in "${ADAPTERS[@]}"; do
    lora-provenance verify "$ADAPTER" || EXIT=1
done

exit $EXIT
```

### 11.2 Incident investigation

When a compliance audit or security incident requires proof of a specific inference:

```bash
# 1. Export the adapter's audit log
lora-provenance export-audit legal-lora-v2 --output /tmp/audit-evidence.jsonl

# 2. Verify chain integrity
lora-provenance verify legal-lora-v2

# 3. Show current provenance
lora-provenance show-provenance legal-lora-v2 --json

# 4. Find the relevant entry in the audit log
grep '"request_id":"REQ-20260221-0042"' /tmp/audit-evidence.jsonl | python3 -m json.tool
```

### 11.3 New adapter deployment checklist

Before deploying a new or updated LoRA adapter to production:

- [ ] Compute `dataset_hash`, `base_model_hash`, `hyperparameter_hash`
- [ ] Compute `adapter_weights_hash` after training completes
- [ ] Obtain RFC 3161 timestamp token from the TSA
- [ ] Sign the provenance record with the CA/eIDAS private key
- [ ] Call `registry.attachProvenance(adapter_id, record)`
- [ ] Call `mgr.createSnapshot(...)` to record the deployment point
- [ ] Run `lora-provenance show-provenance <adapter_id>` to confirm record is stored
- [ ] Enable `recordInferenceAudit()` calls in the inference path

### 11.4 External adapter import checklist

- [ ] Obtain `provenance.json` from the supplier (containing signature and cert chain)
- [ ] Verify the supplier's certificate against the organisation's trusted CA bundle
- [ ] Run `lora-provenance import-external <id> provenance.json --trusted-ca <ca.pem>`
- [ ] Confirm exit code 0 and no validation errors
- [ ] Register the adapter in `AdapterRegistry`
- [ ] Attach the external provenance via `registry.attachProvenance(id, local_prov)`

---

*This document is generated from the ThemisDB source code and should be kept in sync with
`include/llm/lora_framework/lora_provenance.h`,
`src/llm/lora_framework/lora_provenance.cpp`, and
`tools/lora_provenance_cli.cpp`.*
