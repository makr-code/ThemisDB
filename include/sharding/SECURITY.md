<!-- Status: current | validated: 2026-04-06 -->

# include/sharding/ — Security

> Security scope covers **public header interfaces** of the sharding module.
> Runtime security implementation is in [`../../src/sharding/`](../../src/sharding/).

---

## Scope

This document covers the security properties exposed through the public sharding
headers, including:
- Inter-shard transport security (`mtls_client.h`, `mtls_connection_pool.h`,
  `secure_transport_client.h`, `signed_request.h`)
- Consensus integrity (`raft_consensus.h`, `paxos_consensus.h`)
- WAL integrity (`wal_manager.h`, `transaction_wal.h`, `metadata_wal.h`)
- Administrative access control (`admin_api.h`, `admin_operations.h`)
- Distributed transaction atomicity (`two_phase_commit_coordinator.h`,
  `cross_shard_transaction.h`)

---

## Threat Model

| Threat | Impact | Mitigation |
|--------|--------|-----------|
| Man-in-the-middle on shard RPC | Data exfiltration / injection | Mandatory mTLS (`mtls_client.h`); clear-text paths not supported |
| Forged RPC requests | Unauthorized shard operations | Ed25519 / HMAC request signing (`signed_request.h`) |
| Consensus log tampering | Incorrect state replication | WAL entries are CRC-verified; Raft term + log index checked on apply |
| Split-brain / Byzantine leader | Data inconsistency | Quorum enforcement (`quorum_manager.h`); leader lease validation |
| Malicious admin commands | Cluster takeover | Admin API requires mTLS client cert + RBAC role assertion |
| Replay attacks on 2PC messages | Double-commit / double-abort | Monotonic transaction IDs; idempotency tokens in `write_concern.h` |
| Network partition exploitation | Stale reads exposed as fresh | `replica_consistency.h` enforces read-your-writes at LINEARIZABLE level |
| Gossip poisoning | Config corruption across cluster | Gossip messages are signed; unknown senders rejected |
| Orphaned shard access | Unauthorized data access | `orphan_detector.h` marks orphans read-only pending GC |
| Timing oracle via TrueTime | Clock skew exploitation | `truetime.h` exposes uncertainty interval; callers must respect bounds |
| GPU memory side-channel | Erasure code key leakage | `gpu_erasure_coder.h` erases device buffers post-operation |

---

## Security Controls

### Transport Security
- All inter-shard communication uses TLS 1.3 via `mtls_client.h` /
  `mtls_connection_pool.h`.  Certificate pinning is configurable.
- `secure_transport_client.h` provides a unified abstraction; callers must
  not bypass it.
- `signed_request.h` adds an application-layer signature (Ed25519 or HMAC-SHA256)
  to every mutating RPC.

### Consensus Integrity
- Raft WAL entries (`raft_wal_integration.h`) include CRC32C checksums.
- Paxos state persistence (`paxos_state_persistence.h`) uses atomic writes;
  partial writes are detected on recovery.
- `quorum_manager.h` prevents read or commit without a valid quorum even
  during configuration changes.

### Administrative Access
- `admin_api.h` requires a client TLS certificate with a `themis-admin` OID.
- Destructive operations (`admin_operations.h`) require a two-step
  confirmation token to prevent accidental execution.

### Distributed Transaction Safety
- `two_phase_commit_coordinator.h` times out and aborts on participant silence.
- `cross_shard_transaction.h` includes a fencing token to prevent stale
  coordinators from committing after a failover.
- All WAL shipping (`wal_shipper.h`) is authenticated; replicas reject
  unauthenticated log entries.

### Observability Security
- `prometheus_metrics.h` exposes metrics on a dedicated port; the metrics
  endpoint must not be exposed to the public internet without authentication.
- `slo_monitor.h` alert callbacks must not leak internal topology details
  in error messages.

---

## Known Limitations

1. **No Post-Quantum TLS** — `mtls_client.h` uses classical ECDHE ciphersuites.
   Migration to X25519Kyber768 is tracked in the ROADMAP (Q4 2026).
2. **Gossip message authentication requires pre-shared cluster key** — the key
   rotation procedure is manual.  Automated rotation is planned for Q3 2026.
3. **`gpu_erasure_coder.h` device-buffer zeroing** — on CUDA < 12.0 the async
   memset may not complete before buffer reuse if the caller does not synchronise
   the CUDA stream.  Callers are responsible for stream synchronisation.
4. **Admin API rate limiting** — `admin_api.h` does not expose a built-in rate
   limiter; deployments should front the admin endpoint with a reverse proxy.

---

## Reporting Vulnerabilities

See [`../../SECURITY.md`](../../SECURITY.md) for the project-wide responsible
disclosure policy.
