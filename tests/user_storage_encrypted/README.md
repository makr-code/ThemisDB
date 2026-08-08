# tests/user_storage_encrypted

Test suites for the encrypted user storage module, covering unit tests, integration tests, and stress tests.

## Overview

This directory contains focused test suites that validate encrypted storage functionality across multiple phases:

- **Phase 1-2:** Unit tests for core components (I/O safety, command injection prevention)
- **Phase 3:** Contract hardening and failure handling
- **Phase 4:** End-to-end integration tests and stress tests
- **Phase 5:** Performance gates and release validation

## Test Suites

### Phase 1-2: Core Component Tests

#### I/O Timeout & Safety (UST-IO-01 through UST-IO-08)

**File:** `test_backend_io_timeout_focused.cpp`

Tests for timeout handling and I/O safety in gocryptfs_backend operations.

- **UST-IO-01:** PipeGuard creation and validity checks
- **UST-IO-02 through UST-IO-08:** Timeout handling, RAII cleanup, exception safety

**Run:**
```bash
ctest --preset linux-release -L "user_storage_encrypted,io_safety" -V
```

#### Command Injection Prevention

**File:** `test_backend_command_injection_focused.cpp`

Tests for protection against command injection attacks in backend operations.

**Run:**
```bash
ctest --preset linux-release -L "user_storage_encrypted,command_injection" -V
```

### Phase 3: Contract Hardening

**File:** `test_user_storage_encrypted_contract_hardening_focused.cpp`

Tests for hardening failure handling across hostile or degraded host environments.

**Run:**
```bash
ctest --preset linux-release -L "user_storage_encrypted" -V
```

### Phase 4: E2E Integration Tests

#### Vault Integration (E2E-01 through E2E-08)

**File:** `test_user_storage_encrypted_e2e_vault_integration_focused.cpp`

End-to-end integration tests requiring Docker Compose with Vault.

| Test ID | Description                                        | Prerequisites              |
|---------|----------------------------------------------------|-----------------------------|
| E2E-01  | Create, mount, write, unmount, remount lifecycle  | Docker + Vault             |
| E2E-02  | All four security levels (OFFEN, VERTRAUT, ...)  | Docker + Vault             |
| E2E-03  | Zero-downtime key rotation with concurrent load  | Docker + Vault + gocryptfs |
| E2E-04  | Mount failure recovery (FUSE unavailable)        | Docker                     |
| E2E-05  | Invalid container path handling                  | No special prerequisites   |
| E2E-06  | Vault timeout and automatic recovery             | Docker + Vault             |
| E2E-07  | Stale mount reconciliation on startup            | Docker + gocryptfs         |
| E2E-08  | Multi-tenant isolation                           | Docker + Vault             |

**Prerequisites:**
```bash
# Start Docker Compose with Vault
docker-compose -f docker-compose.user-storage.yml up -d vault vault-init
sleep 10  # Wait for Vault to be ready
```

**Run:**
```bash
ctest --preset linux-release -L "user_storage_encrypted,e2e" -V
```

**Cleanup:**
```bash
docker-compose -f docker-compose.user-storage.yml down
```

### Phase 4: Stress & Failure Injection Tests

**File:** `test_user_storage_encrypted_stress_focused.cpp`

Stress tests with error injection for resilience validation.

| Test ID   | Description                                   | Category                  |
|-----------|-----------------------------------------------|---------------------------|
| STRESS-01 | Concurrent mount/unmount operations          | Concurrency / Deadlock    |
| STRESS-02 | Key rotation under high concurrency           | Concurrency / Rotation    |
| STRESS-03 | Error injection: command timeout             | Error Handling / Timeout  |
| STRESS-04 | Error injection: Vault unavailable           | Error Handling / Resilience |
| STRESS-05 | Error injection: disk full                    | Error Handling / Resources |
| STRESS-06 | Error injection: permission denied           | Error Handling / Security |

**Run:**
```bash
ctest --preset linux-release -L "user_storage_encrypted,stress" -V
```

## Running Tests

### All user_storage_encrypted Tests

```bash
ctest --preset linux-release -L user_storage_encrypted -V
```

### By Test Tier

```bash
# Unit tests only
ctest --preset linux-release -L "user_storage_encrypted,unit" -V

# Integration tests only
ctest --preset linux-release -L "user_storage_encrypted,integration" -V

# Stress tests only
ctest --preset linux-release -L "user_storage_encrypted,stress" -V
```

### By Category

```bash
# Security tests
ctest --preset linux-release -L "user_storage_encrypted,security" -V

# I/O safety tests
ctest --preset linux-release -L "user_storage_encrypted,io_safety" -V

# E2E + Vault integration tests
ctest --preset linux-release -L "user_storage_encrypted,vault" -V
```

### With Verbose Output

```bash
ctest --preset linux-release -L user_storage_encrypted -V --output-on-failure
```

### With Timeout Handling

```bash
ctest --preset linux-release -L user_storage_encrypted --timeout 300 -V
```

## Test Infrastructure

### Docker Compose Setup

The E2E integration tests require Docker Compose with Vault:

**File:** `docker-compose.user-storage.yml` (repo root)

Services:
- **vault:** HashiCorp Vault dev server on port 8200
- **vault-init:** One-time initialization job for Vault keys
- **themisdb:** ThemisDB application container

Health checks ensure Vault is ready before tests run.

### Environment Variables

For local testing, set:

```bash
export VAULT_ADDR=http://localhost:8200
export VAULT_TOKEN=dev-token-12345
```

## Test Configuration

### CMake Integration

Tests are registered via `themis_register_module_focused_test()` in `CMakeLists.txt`:

```cmake
themis_register_module_focused_test(
    MODULE user_storage_encrypted
    NAME   test_name
    TARGET target_name
    TIER   unit|integration|stress
    TIMEOUT seconds
    LABELS  "label1;label2"
)
```

### Timeouts

| Test Type | Timeout   | Rationale                         |
|-----------|-----------|-----------------------------------|
| Unit      | 120 sec   | Quick validation                  |
| Integration | 300 sec | Vault + mount/unmount operations |
| Stress    | 300 sec   | Multiple concurrent operations   |

## Known Limitations

### Docker Dependency

E2E tests require Docker and docker-compose to be available:

```bash
docker --version
docker-compose --version
```

If Docker is unavailable, E2E tests will be skipped with `GTEST_SKIP()`.

### System Requirements

- **FUSE support:** gocryptfs operations require FUSE kernel support
- **Temporary storage:** Tests use `/tmp/` for encrypted containers (requires ~1GB free space)
- **User privileges:** Some tests may require sufficient permissions (non-root recommended for isolation)

### Vault Connectivity

E2E tests assume:
- Vault is accessible at `http://vault:8200` (inside Docker)
- Default token: `dev-token-12345` (dev mode only)
- KV v2 secrets engine enabled at path `/themis/`

## Acceptance Criteria

All tests must pass for a release:

- ✅ All unit tests pass (zero failures)
- ✅ All integration tests pass (with Docker + Vault running)
- ✅ All stress tests complete without deadlocks or resource leaks
- ✅ No flaky tests (99%+ pass rate on 10 consecutive runs)
- ✅ Performance gates met (see benchmarks/user_storage_encrypted/README.md)

## Troubleshooting

### Test Hangs / Timeouts

If a test times out:

1. Check Docker Compose status:
   ```bash
   docker-compose -f docker-compose.user-storage.yml ps
   ```

2. Check Vault health:
   ```bash
   curl -s http://localhost:8200/v1/sys/health | jq .
   ```

3. Check mount points:
   ```bash
   mount | grep gocryptfs
   ```

4. Increase timeout in CTest:
   ```bash
   ctest --preset linux-release -L user_storage_encrypted --timeout 600 -V
   ```

### Permission Errors

If you see permission errors:

1. Ensure `/tmp/themis_*` directories are readable/writable
2. For FUSE errors, verify gocryptfs is installed:
   ```bash
   which gocryptfs
   gocryptfs --version
   ```

3. Check FUSE device availability:
   ```bash
   ls -la /dev/fuse
   ```

### Vault Connection Refused

If Vault initialization fails:

1. Ensure Docker daemon is running:
   ```bash
   docker ps
   ```

2. Check Vault container logs:
   ```bash
   docker logs themis-vault
   docker logs themis-vault-init
   ```

3. Manually initialize Vault:
   ```bash
   docker-compose -f docker-compose.user-storage.yml exec vault \
       vault secrets enable -path=themis kv-v2
   ```

## Performance Expectations

Tests should complete in:
- Unit tests: ~10-30 seconds (all 8 tests)
- Integration tests: ~2-5 minutes (with Docker overhead)
- Stress tests: ~2-3 minutes (concurrent operations)
- Full suite: ~5-10 minutes (with Docker + Vault)

## References

- [API Contract](../../include/user_storage_encrypted/user_storage_encrypted_api_contract.h)
- [Architecture](../../include/user_storage_encrypted/ARCHITECTURE.md)
- [ROADMAP](../../src/user_storage_encrypted/ROADMAP.md)
- [Docker Compose Setup](../../docker-compose.user-storage.yml)

