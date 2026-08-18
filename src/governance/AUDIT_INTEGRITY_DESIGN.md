# Audit Trail Integrity & Immutability Design

## Overview

This document describes the design and implementation of the audit trail integrity
and immutability subsystem in the ThemisDB governance module (Critical Path 3).

## 1. Audit Log Schema and Format

Each audit log entry is a JSON-serializable `AuditEntry` containing:

| Field | Type | Description |
|---|---|---|
| `entry_id` | `string` | Globally unique entry identifier (UUIDv4) |
| `timestamp_ms` | `int64` | Unix epoch time in milliseconds |
| `actor_id` | `string` | Identity performing the action |
| `action` | `string` | Action performed (e.g. `policy.create`, `data.access`) |
| `resource_id` | `string` | Resource affected |
| `result` | `string` | Outcome: `SUCCESS`, `FAILURE`, `PARTIAL` |
| `prev_hash` | `string` | SHA-256 of the preceding entry (chain linkage) |
| `entry_hash` | `string` | SHA-256 of this entry's canonical fields |
| `signature` | `string` | HMAC-SHA256 signature under the current signing key |
| `metadata` | `map<string,string>` | Extensible key/value context |

## 2. Immutability Guarantees

- Entries are append-only; no update or delete operation is exposed on the audit store.
- Each entry's `prev_hash` links it to its predecessor, forming a tamper-evident chain.
- Any modification of a stored entry invalidates `entry_hash` and breaks the chain.
- The `AuditIntegrityVerifier` detects both single-entry tampering and chain breaks.

## 3. Chain-of-Custody Model

```
Entry[n-1]          Entry[n]           Entry[n+1]
┌─────────┐         ┌─────────┐        ┌─────────┐
│ hash_n-1│←────────│prev_hash│        │prev_hash│←── hash_n
│ sig_n-1 │         │ hash_n  │←───────│ sig_n+1 │
└─────────┘         │ sig_n   │        └─────────┘
                    └─────────┘
```

Verification walks the chain in sequence, recomputing each `entry_hash` from
canonical fields and validating `prev_hash == hash[n-1]`.

## 4. Cryptographic Signing

- Algorithm: HMAC-SHA256
- Key source: `AuditKeyStore` (see `include/governance/audit_key_store.h`)
- Key identifier embedded in each entry via `metadata["signing_key_id"]`
- Verification uses the key active at the entry's `timestamp_ms`

## 5. Tamper Detection

`AuditIntegrityVerifier::verifyChain()` reports:

- `CHAIN_BREAK`: `prev_hash` mismatch between consecutive entries
- `HASH_MISMATCH`: recomputed `entry_hash` differs from stored value
- `SIGNATURE_INVALID`: HMAC verification fails with the recorded key
- `MISSING_ENTRY`: gap detected in the sequence

## 6. Retention Policy Enforcement

Retention is governed by `AuditRetentionPolicy`:

| Field | Description |
|---|---|
| `retention_period_days` | Minimum days entries must be retained |
| `archive_after_days` | Move to cold storage after N days |
| `purge_after_days` | Hard delete after N days (must be ≥ `retention_period_days`) |

Enforcement is performed by `AuditRetentionEnforcer` during scheduled maintenance
windows. Entries within the retention window are never purged.

## 7. Key Rotation

Key rotation follows a zero-downtime model:

1. New key generated and stored in `AuditKeyStore` with a future `active_from_ms`.
2. At `active_from_ms`, all new entries are signed with the new key.
3. Old key is retained in `AuditKeyStore` for verification of historical entries.
4. Keys are never deleted; they transition to `RETIRED` status.

## 8. Operator Procedures

### Verifying chain integrity
```bash
themisdb-admin audit verify --from <ISO8601> --to <ISO8601>
```

### Exporting audit entries for external SIEM
```bash
themisdb-admin audit export --format json --output /var/audit/export.json
```

### Rotating the signing key
```bash
themisdb-admin audit rotate-key --effective-at <ISO8601>
```

### Querying by actor or resource
```bash
themisdb-admin audit query --actor <actor_id> --resource <resource_id>
```
