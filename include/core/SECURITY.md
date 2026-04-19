<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Core Module Public Headers

**Module Path:** `include/core/`
**Implementation Security:** `../../src/core/SECURITY.md`

---

## Scope

Security considerations for the core module's public header API surface. The core module
provides the foundational dependency injection interfaces used throughout the entire
ThemisDB codebase; its security properties propagate to all other modules.

---

## Threat Model

| Threat | Vector | Mitigation Header |
|--------|--------|------------------|
| Secrets leakage via logging | Logger printing `ISecrets` values | `i_secrets.h` — secrets are opaque; returned as `SecureView` not `std::string` |
| Debug paths in production | Debug code active in production build | `production_mode.h` — `isProductionBuild()` gate enforced |
| Secrets hot-rotation race | Old secret used after rotation | `i_secrets.h` — `watchSecret()` callback enables atomic rotation |
| Misconfiguration at startup | Invalid config silently accepted | `config_validator.h` — fail-fast with `ConfigValidationResult` error list |
| Feature flag bypass | Attacker disables security features via flags | `i_feature_flags.h` — security-critical flags documented as non-overridable |
| Circuit breaker abuse | Attacker forces circuit open (DoS) | `i_circuit_breaker.h` — state transitions require privilege context |
| Metrics side-channel | Per-user metrics leaking activity | `i_metrics.h` — metric labels must not include PII |
| Trace data exfiltration | Spans containing sensitive query data | `i_tracer.h` — span attributes must be reviewed for PII before export |
| Secrets in config files | Plaintext secrets in `ConfigValidationResult` | `config_validator.h` — `ConfigValidationResult` redacts secret values in error messages |
| No-op implementations in production | `NoopLogger` left active | `noop_implementations.h` — no-ops must not be registered via `ConcernsContext` in production mode |

---

## Security Controls

### Production Mode Gate
`production_mode.h` provides `isProductionBuild()` and `ProductionMode::assertProduction()`;
debug paths and no-op implementations are blocked in production builds.

### Secrets as Opaque Types
`ISecrets::get(key)` returns a `SecureView` — a non-copyable, non-printable view into
secure memory. Secrets are never exposed as `std::string` in the public API.

### Secrets Hot Rotation
`ISecrets::watchSecret(key, callback)` enables zero-downtime secret rotation; the callback
is called with the new value in `SecureBuffer` before the old value is invalidated.

### Configuration Validation
`IConfigValidator::validate()` returns a `ConfigValidationResult` with structured error
list; invalid configurations cause `std::terminate()` in production mode.

### Metric Label PII Policy
`MetricLabels` documentation requires that label values must not include PII (user IDs,
IP addresses, query content); this is enforced by code review policy.

### Audit Log Non-Repudiation
`IAuditLog` entries include a monotonic sequence number and timestamp; out-of-order
entries are rejected by the audit log implementation.

---

## Known Limitations

- `InMemorySecrets` is suitable for development only; production deployments must use
  a secrets manager (HashiCorp Vault, AWS Secrets Manager, etc.).
- Trace span attributes are not automatically redacted; operators must configure the
  OTLP exporter with a `redacted_attributes` list.
- Feature flags that control security behaviour (e.g., auth bypass flags) must be
  documented as non-overridable; this is a convention, not a compile-time enforcement.
