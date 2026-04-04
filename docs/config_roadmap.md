# Config Subsystem Production-Readiness Assessment & Roadmap

## Current Assessment

The config path resolver and migration utilities (`src/config/config_path_resolver.{h,cpp}`) are **not 100% production ready**. While the subsystem provides basic functionality for resolving legacy config paths to new hierarchical locations, significant gaps remain:

### Identified Gaps

- **Static Mapping Table**: No validation or coverage checks for the mapping table; missing paths are silently ignored
- **No Telemetry**: No metrics for resolution hits/misses, legacy fallbacks, or unmapped path requests
- **No Config Validation**: No lint checks or schema validation for config files
- **No Migration Auditing**: No logging or audit trail for migration operations
- **No Deprecation Policy**: No deprecation deadlines, alerts, or enforcement mechanisms
- **Thread Safety**: No documented concurrency policy or thread-safety guarantees
- **Insufficient Testing**: Lacks unit tests for normalize/map/resolve operations, fuzz testing, and filesystem edge cases
- **No Authorization**: No ACL or authz controls for config file access
- **No Integrity Checks**: No config file signature verification or integrity validation
- **No Caching**: No caching strategy for frequently accessed configs
- **No Observability**: No tracing spans or distributed tracing support
- **No Operations Tooling**: No admin/reporting interface for unmapped paths or legacy usage

---

## Roadmap

### Stabilität & Sicherheit (Stability & Security)

- **Config Integrity & Signature Checks**
  - Implement manifest-based config verification
  - Add optional cryptographic signature validation for critical configs
  - Support hash-based integrity checks (SHA-256)

- **ACL & Authorization**
  - Add role-based access control for config file reads
  - Implement granular permissions for sensitive configs (security/, compliance/)
  - Audit all config access attempts

- **Caching with TTL/Invalidation**
  - Implement LRU cache for resolved config paths
  - Add configurable TTL for cache entries
  - Support cache invalidation on config file changes (inotify/fswatch)

- **Legacy Fallback Limits & Deprecation Policy**
  - Define deprecation timeline for legacy paths (e.g., 6 months warning period)
  - Add configurable limits for legacy fallback usage
  - Implement gradual deprecation phases (warn → error → removal)

- **Thread-Safety Guarantees**
  - Document thread-safety model (lock-free reads, synchronized mapping updates)
  - Add thread-safe caching implementation
  - Provide concurrent access guarantees in API documentation

---

### Korrektheit & Tests (Correctness & Tests)

- **Unit & Integration Tests**
  - Add unit tests for `normalizePath()`, `mapLegacyToNew()`, `resolve()`, `tryResolve()`
  - Test all mapping table entries (100% coverage)
  - Add integration tests for filesystem operations

- **Coverage for Missing Mappings**
  - Test behavior when mapping is not found
  - Validate error messages and fallback paths
  - Test optional resolution with `tryResolve()`

- **Filesystem Edge Cases**
  - Test with missing files, invalid permissions
  - Test symlinks, circular references
  - Test case sensitivity on different filesystems
  - Test long paths and special characters

- **Fuzz Testing**
  - Add fuzz tests for path normalization
  - Test with malformed paths, path traversal attempts
  - Validate handling of unicode and special characters

- **Regression Tests for Mapping Table**
  - Add CI validation for mapping table completeness
  - Test that all legacy paths have valid new paths
  - Validate path format consistency

---

### Observability & Operations (Observability & Operations)

- **Metrics (Prometheus/OpenTelemetry)**
  - Resolution hits/misses counter
  - Legacy path usage counter (by path)
  - Unmapped path requests counter
  - Cache hit/miss ratio
  - Resolution latency histogram

- **Tracing Spans**
  - Add OpenTelemetry spans for `resolve()` operations
  - Trace filesystem lookups and fallbacks
  - Include mapping table lookups in traces

- **Audit Logs for Fallback Usage**
  - Log all legacy path fallbacks with caller context
  - Record unmapped path access attempts
  - Include timestamps, user context, and request metadata

- **Alerts for Unmapped/Deprecated Paths**
  - Alert on unmapped path requests (potential misconfiguration)
  - Alert on deprecated path usage exceeding threshold
  - Notify on mapping table inconsistencies

- **Admin Reporting Dashboard**
  - Report legacy path usage frequency
  - List unmapped paths with request count
  - Visualize migration progress (legacy → new path adoption)

---

### API/Config & DX (Developer Experience)

- **Schema Validation & Linting**
  - Define JSON Schema for config mapping table
  - Add pre-commit hook for mapping table validation
  - Validate config file formats (YAML/JSON) on load

- **Admin/Reporting of Legacy Usage**
  - CLI tool to list all legacy path usage
  - Generate migration reports (which files still use legacy paths)
  - Export usage statistics for analysis

- **Deprecation Timelines**
  - Add machine-readable deprecation metadata to mapping table
  - Display deprecation warnings with timeline in logs
  - Provide migration deadline information in API responses

- **Structured Errors**
  - Use structured error types (not generic `std::runtime_error`)
  - Include detailed error context (attempted paths, permissions, etc.)
  - Provide actionable error messages with migration guidance

- **Optional Dry-Run Mode**
  - Add `--dry-run` flag to list missing config files
  - Validate all config paths without loading files
  - Generate migration checklist for deployment

---

### Security/Privacy (Security/Privacy)

- **Signature/Manifest Verification**
  - Implement Ed25519/RSA signature verification for config files
  - Support manifest files listing expected configs with checksums
  - Reject tampered or unsigned configs in strict mode

- **Controlled Search Roots**
  - Restrict config resolution to predefined root directories
  - Prevent path traversal attacks (e.g., `../../../etc/passwd`)
  - Validate resolved paths stay within allowed boundaries

- **Optional Encryption at Rest**
  - Support encrypted config files for sensitive data
  - Integrate with key management systems (KMS, Vault)
  - Decrypt configs in-memory only (no plaintext on disk)

---

### Delivery & Governance (Delivery & Governance)

- **CI Gates for Mapping Table Validation**
  - Add CI check to validate mapping table schema
  - Ensure all legacy paths map to valid new paths
  - Verify new paths follow directory structure conventions

- **CI Gates for Schema Lint**
  - Add CI check to lint all config files (YAML/JSON)
  - Validate config file structure against schemas
  - Fail build on schema validation errors

- **Feature Flags for Strict Mode**
  - Add `strict_mode` flag to reject legacy paths entirely
  - Support gradual rollout of strict mode (per-environment)
  - Allow runtime toggling for testing

- **Runbooks for Migration**
  - Document step-by-step migration process
  - Provide rollback procedures
  - Include testing and validation steps
  - Add troubleshooting guide for common issues

---

## Implementation Priority

1. **High Priority** (Weeks 1-4): Testing, thread safety, metrics, structured errors
2. **Medium Priority** (Weeks 5-8): Caching, deprecation policy, schema validation, CI gates
3. **Low Priority** (Weeks 9-12): ACL/authz, encryption, signature verification, admin dashboard

## Related Documentation

- [Config Path Resolver Source Code](../src/config/config_path_resolver.h)
- [Architecture Overview](de/architecture/ARCHITECTURE_OVERVIEW.md)
- [Security Framework](../SECURITY.md)
