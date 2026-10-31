---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

# Timeseries Trust Model

This page identifies trust boundaries relevant to developers and operators
handling time-series data. It is not a system-wide security certification.
Authentication and authorization at client and HTTP boundaries must be
verified in the server integration.

Exact model backend not verifiable from final document text alone.

## Trust boundaries and data flow

1. **Caller to module:** Metric/entity names, timestamps, values, tags, and
   metadata originate with callers. `TSStore` rejects empty metric/entity
   strings, but the module does not establish caller identity or
   authorization.
2. **Remote-write payload to parser:** Prometheus protobuf bytes are untrusted
   input. `PromWriteRequest` decoders check wire bounds and reject malformed
   input; parsing does not provide network authentication, TLS, rate limiting,
   or request authorization.
3. **Module to RocksDB:** The store receives non-owning database and
   column-family handles. Protect the storage process, backup, and filesystem
   according to deployment policy.
4. **Encrypted chunk key providers:** `EncryptedChunkStore` relies on
   externally supplied current-key and historical-key lookup callbacks.
   Protect key bytes and identifiers at that boundary, and keep historical
   keys available for decryption of existing chunks.
5. **Diagnostics and metadata:** Tags, metadata, metric names, entity IDs,
   access identities, and error text may be sensitive or high-cardinality.
   Apply the deployment's redaction, access, and retention controls before
   exporting logs, metrics, or incidents.

## Controls present in the module

- Gorilla chunk encryption uses AES-256-GCM with HKDF-SHA256-derived
  per-series encryption material. The envelope carries a key identifier and
  authenticated ciphertext; failed authentication must be handled as a
  decryption failure, not as valid data.
- The encrypt operation returns the key ID used with the ciphertext so the
  stored metadata can remain consistent if key rotation overlaps a write.
- The key-rotation worker writes a replacement value before discarding the
  previous value; read snapshots can continue using their prior value.
- Remote-write protobuf parsing checks payload boundaries, length fields, and
  varint overflow.
- Buffer, lifecycle, and integration failures can be represented with
  explicit result/error codes and structured incidents.

The audit logger supplied to `EncryptedChunkStore` is optional at construction.
Production deployments that require key-access auditing must configure and
verify that logger; the module cannot guarantee an audit sink merely because
the encryption class is used.

## Risks and required handling

| Risk | Required control |
|---|---|
| Unauthorized read/write | Enforce identity, authorization, and transport security at the server/API boundary; these are not supplied by `TSStore`. |
| Lost historical key | Retain and test lookup for every key ID referenced by retained chunks; do not delete a key based only on the current key ID. |
| Corrupt or tampered chunk | Preserve the original value and report decryption/codec errors; do not silently accept partial decoding or overwrite evidence. |
| Malformed or oversized remote-write data | Reject parser errors, apply request-size/rate limits in the server boundary, and avoid logging raw payloads. |
| Sensitive/high-cardinality metadata | Minimize and redact exported labels/log fields; assess per-metric metrics before enabling them at scale. |
| Irreversible retention | Review the policy and backup state before applying deletion; retention has no module-level undo. |

## Incident handling

Preserve sanitized request or chunk identifiers, error code, key ID (never the
key material), time range, and structured incident output. Escalate
authentication/authorization issues to the server security owner, key
failures to key management and storage owners, and suspected tampering or data
loss through the project's security incident process. Do not disable
authentication, integrity checks, audit controls, or retention protections as
a workaround.

## References

- [Module security page](SECURITY.md)
- [Encrypted chunk API](../../include/timeseries/encrypted_chunk_store.h)
- [Key rotation API](../../include/timeseries/ts_encrypted_key_rotation.h)
- [Remote-write parser](../../include/timeseries/prometheus_remote_write.h)
- [Store API](../../include/timeseries/tsstore.h)
- [Production requirements](PRODUCTION_REQUIREMENTS.md)
- [Incident runbook](RUNBOOK.md)
