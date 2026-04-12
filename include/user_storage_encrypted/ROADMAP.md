<!-- Status: current | validated: 2026-04-06 -->

# User Storage Encrypted — Roadmap

## Current Status
**v0.0.1** — Initial public headers released. No production deployments yet. Core interfaces defined; implementation in progress.

## Completed
- [x] Define `IEncryptionBackend` abstract interface
- [x] Initial `GocryptfsBackend` header (fork/execvp, mkstemp 0600 key files)
- [x] `KeyRotationScheduler` public API header
- [x] `MultiLevelEncryptedStorage` HOT/WARM/COLD tier interface
- [x] `SecurityLevel` enum (STANDARD/HIGH/MAXIMUM)
- [x] `UserModels` metadata types

## Planned Features

- [ ] Secure stdin-pipe key delivery in `GocryptfsBackend` (Target: Q2 2026)
  - Replace mkstemp /tmp key files with pipe(2)-based delivery
  - Zeroize key buffer post-write with `explicit_bzero`
- [ ] HKDF-based per-rotation subkey derivation in `KeyRotationScheduler` (Target: Q2 2026)
- [ ] Unit test suite for all six headers (Target: Q2 2026)
- [ ] Integration tests with real gocryptfs binary (Target: Q3 2026)
- [ ] Second encryption backend (eCryptfs or LUKS) implementing `IEncryptionBackend` (Target: Q3 2026)
- [ ] Async key rotation callbacks (Target: Q3 2026)
- [ ] Metrics/observability hooks in `MultiLevelEncryptedStorage` (Target: Q4 2026)
- [ ] Formal security audit of key-delivery path (Target: Q4 2026)

## Implementation Phases

### Phase 1 — Design / API Contract
- [x] Define `IEncryptionBackend` interface
- [x] Define `SecurityLevel` enum
- [x] Define `MultiLevelEncryptedStorage` tier API
- [ ] Finalize `MountOptions` struct (add `pipe_fd` field)

### Phase 2 — Core Implementation
- [ ] Implement stdin-pipe key delivery in `GocryptfsBackend`
- [ ] Implement HKDF subkey derivation in `KeyRotationScheduler`
- [ ] Implement tier promotion/demotion logic in `MultiLevelEncryptedStorage`

### Phase 3 — Error Handling & Edge Cases
- [ ] Handle gocryptfs subprocess failures (non-zero exit, timeout)
- [ ] Handle key rotation failure with rollback
- [ ] Handle tier unavailability (COLD tier offline)

### Phase 4 — Tests
- [ ] Unit tests for all six public interfaces
- [ ] Integration tests with gocryptfs binary
- [ ] Security tests (no key material in /tmp, no key in argv)

### Phase 5 — Performance / Hardening
- [x] Benchmark mount latency (pipe vs file approach) (`bench_user_storage_mount_latency.cpp`)
- [ ] Memory zeroization audit
- [ ] Fuzzing of `UserModels` deserialization

### Phase 6 — Documentation & Sign-off
- [ ] Complete API documentation for all headers
- [ ] Security review sign-off
- [ ] Update CHANGELOG with v0.1.0 entries

## Production Readiness Checklist
- [ ] stdin-pipe key delivery implemented and audited
- [ ] All unit and integration tests pass
- [ ] No critical/high findings in security audit
- [ ] Key rotation tested under failure conditions
- [ ] Observability hooks present in storage layer
- [ ] Documentation complete
