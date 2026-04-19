<!-- Status: current | validated: 2026-04-06 -->

# Security — include/temporal/

> Security scope, threat model, and mitigations for the public temporal headers.
> Implementation-level security controls are documented in [`../../src/temporal/SECURITY.md`](../../src/temporal/SECURITY.md).

---

## Scope

This document covers security considerations for code that **includes** the
public headers in `include/temporal/`.  It addresses:

- Unsafe use of the `TemporalCDC` ring-buffer API
- Integer/time-point overflow in temporal arithmetic
- Conflict-resolution policy security implications
- Snapshot and retention data-leakage risks
- Compression side-channel considerations

---

## Threat Model

| Threat | Impact | Mitigation |
|--------|--------|-----------|
| **Ring-buffer overflow in `CDCRingBuffer`** (65 536 capacity) | Event loss; audit-trail gaps; potential replay-attack opportunity if attacker can predict overwritten offsets | Document and enforce one of: BLOCK / DROP / OVERWRITE policy; callers must monitor `CDCRingBuffer::size()` and handle back-pressure |
| **Time-point integer overflow** in `TimePoint` arithmetic | Silent wrap-around producing negative or far-future timestamps; temporal query returns incorrect result set | `TimePoint` uses `std::chrono::nanoseconds` (int64); inputs must be validated to [0, 9223372036854775807 ns]; `BiTemporalStore::insert()` rejects invalid intervals since v1.6.0 |
| **Bi-temporal record spoofing** — caller sets arbitrary `transaction_time` | Attacker can back-date records, bypassing audit | `SystemVersionedTable` sets `transaction_time` from system clock inside the library; callers cannot override system-generated transaction timestamps |
| **Conflict resolver injection** — malicious `CustomResolver` callback | Arbitrary code execution at resolution time; silent data corruption | `CustomResolver` callbacks run in the caller's trust domain; the library does not execute them with elevated privilege; validate all externally-supplied resolvers |
| **Snapshot information disclosure** — `SnapshotCatalog` enumerates all snapshots | Unauthorised enumeration of historical data versions | Enforce access control at the `SnapshotManager` level before exposing catalog to untrusted callers; catalog does not itself decrypt data |
| **Retention policy bypass** — caller disables `RetentionPolicy` | Historical PII / sensitive records retained indefinitely | Retention policies should be enforced server-side and not be configurable by unprivileged callers; audit policy changes |
| **Compression oracle / side-channel** — `ZstdStrategy` / Gorilla compression ratio leaks data | Compression-ratio side-channel (CRIME-style) if compressed ciphertext is observable | Do not compress and encrypt the same secret data in a way that exposes compressed size to adversary; encrypt *after* compression |
| **`replayChanges()` replay attack** — CDC log replayed to re-apply old writes | Duplicate or stale writes applied to live store | Callers must validate event sequence numbers and idempotency tokens before applying replayed events |
| **Denial of service via large interval insert** — pathological interval tree construction | O(n²) degenerate tree if adversary controls insert order | `IntervalTreeIndex` uses augmented BST with rebalancing; `bulkLoad()` pre-sorts input; reject externally-supplied intervals exceeding configurable maximum span |

---

## Security Controls

### Input Validation
- `BiTemporalStore::insert()` throws `std::invalid_argument` for `valid_start >= valid_end` (v1.6.0+)
- `TimeInterval` constructor validates that start ≤ end; asserts are active in debug builds and enforce contract in release builds via exceptions
- `RetentionRule` validation is performed at policy-registration time, not at eviction time

### Audit Trail Integrity
- `SystemVersionedTable` uses a library-controlled `system_clock` for `transaction_time`; callers cannot inject synthetic transaction timestamps
- CDC events include a monotonically increasing sequence number; gaps indicate dropped events
- `replayChanges()` exposes sequence numbers to allow callers to detect replay and out-of-order delivery

### Memory Safety
- All public headers use RAII (`std::unique_ptr`, `std::shared_ptr`) for resource ownership
- No raw owning pointers in public API
- Ring-buffer indices are bounded; no pointer arithmetic exposed to callers

### Cryptographic Note
- The temporal headers do **not** provide cryptographic primitives
- Encryption of stored data is the responsibility of the storage layer (see `include/themis/`)
- `module_hash_verifier.h` (in `include/themis/`) should be used to verify integrity of loaded temporal plugins

---

## Known Limitations

1. **CDC ring-buffer overflow policy** is not yet documented in the public header (see AUDIT.md Finding 1); callers must consult `../../src/temporal/` source for current behaviour until v1.6.1.
2. **No built-in rate limiting** on `IntervalTreeIndex` insert; DoS via adversarial insert patterns requires application-level guards.
3. **`CustomResolver` callbacks** are not sandboxed; loading resolvers from untrusted sources is the caller's responsibility.
4. **Snapshot encryption** is not provided by `snapshot_manager.h`; snapshots written to disk must be encrypted by the hosting application.
5. **Time-source trust** — `BiTemporalStore` valid-time is caller-supplied; the library does not validate that valid-time reflects wall-clock reality.  Applications handling legally significant temporal records must enforce valid-time policies out-of-band.
