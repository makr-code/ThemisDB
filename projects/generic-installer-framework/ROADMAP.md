# ROADMAP

## Current Status

- [x] Initial repository bootstrap
- [x] Initial schemas for manifest/product/recipe
- [x] Initial sample configuration and release manifest
- [~] Core workflow implementation started (skeleton only)

## In Progress

- [~] Define stable API contracts for check/install/update/uninstall (Target: Q2 2026)
- [~] Define error model and rollback semantics (Target: Q2 2026)

## Planned Features

- [ ] Manifest signature verification implementation (Target: Q2 2026)
- [ ] Artifact SHA-256 streaming verification (Target: Q2 2026)
- [ ] Reliable download manager with resume/retry (Target: Q2 2026)
- [ ] Atomic install/update transaction engine (Target: Q3 2026)
- [ ] Uninstall engine with preserve policy (Target: Q3 2026)
- [ ] Platform adapters for Windows/Linux/macOS (Target: Q3 2026)
- [ ] Reference CLI with machine-readable output mode (Target: Q3 2026)
- [ ] Integration test harness for end-to-end flows (Target: Q3 2026)

## Implementation Phases

### Phase 1: Design / API Contract

- [ ] Freeze operation input/output contracts for all public operations (Target: Q2 2026)
- [ ] Define typed error categories and recoverability semantics (Target: Q2 2026)
- [ ] Define installation-state schema and migration strategy (Target: Q2 2026)
- [ ] Define policy behavior for downgrade blocking and channel switching (Target: Q2 2026)

### Phase 2: Core Implementation

- [ ] Implement configuration loader with schema validation (Target: Q2 2026)
- [ ] Implement manifest loader and signature verification pipeline (Target: Q2 2026)
- [ ] Implement source provider abstraction and GitHub provider (Target: Q2 2026)
- [ ] Implement artifact selection by os/arch/channel (Target: Q2 2026)

### Phase 3: Error Handling & Edge Cases

- [ ] Handle partial downloads and recovery via resume checkpoints (Target: Q3 2026)
- [ ] Handle file-lock and permission edge cases per platform (Target: Q3 2026)
- [ ] Implement deterministic rollback when health checks fail (Target: Q3 2026)
- [ ] Add strict failure modes for invalid signatures/hashes/config (Target: Q3 2026)

### Phase 4: Tests

- [ ] Unit tests for schema, signature, hash, selector, policy logic (Target: Q3 2026)
- [ ] Integration tests for install/update/uninstall happy path (Target: Q3 2026)
- [ ] Fault-injection tests for network and disk failures (Target: Q3 2026)
- [ ] Cross-platform CI smoke tests for CLI workflows (Target: Q3 2026)

### Phase 5: Performance/Hardening

- [ ] Streamed verification to avoid full-memory artifact loading (Target: Q3 2026)
- [ ] Parallel checksum and staged I/O optimization where safe (Target: Q3 2026)
- [ ] Structured security audit logs and redaction policy (Target: Q3 2026)
- [ ] Hardened trust-store loading and key-rotation support (Target: Q4 2026)

### Phase 6: Documentation & Acceptance

- [ ] Complete architecture and security model docs (Target: Q3 2026)
- [ ] Publish integration guide for host products (Target: Q3 2026)
- [ ] Publish release and signing runbook (Target: Q4 2026)
- [ ] Define and pass MVP acceptance checklist (Target: Q4 2026)

## Production Readiness Checklist

- [ ] Signature verification enabled and non-optional
- [ ] SHA-256 verification enabled and non-optional
- [ ] Atomic install/update with rollback proven in integration tests
- [ ] Platform-specific lock/permission handling validated
- [ ] Installation-state migration path tested
- [ ] CLI diagnostics stable and documented
- [ ] Security logs available and redacted
- [ ] Reproducible release pipeline with signing gate

## Known Issues & Limitations

- [I] Core implementation still mostly scaffolded
- [I] No production-grade download manager yet
- [I] No production-grade cryptographic verification backend wired
- [I] No complete E2E test matrix yet

## Breaking Changes

- [!] Public API and schema fields can change until v0.5.0
- [!] State file layout can change before migration policy is finalized
