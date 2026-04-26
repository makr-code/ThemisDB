> **Status:** 2026-04-19 – Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos ggf. korrigiert.

# ThemisDB Core Module - Production Requirements

## Overview

This document outlines the production requirements for deploying ThemisDB with proper security hardening. The Core module enforces strict security policies when running in production mode to prevent accidental deployment of insecure configurations.

## Production Mode Detection

ThemisDB automatically detects production mode through environment variables:

### Environment Variables

```bash
# Option 1: Explicit production mode flag
export THEMIS_PRODUCTION_MODE=1

# Option 2: Environment-based detection
export THEMIS_ENVIRONMENT=production
```

**Recognized values:**
- `THEMIS_PRODUCTION_MODE`: `1`, `true`, `True`, `TRUE`, `yes`, `Yes`, `on`, `On`
- `THEMIS_ENVIRONMENT`: `production`, `prod`

When production mode is enabled, ThemisDB enforces strict security requirements and rejects insecure defaults.

## Key Provider Requirements

### Production Mode Restrictions

In production mode, ThemisDB **requires** a secure key provider and **rejects** mock/test providers:

**❌ PROHIBITED in Production:**
- `LOCAL` key provider (mock/in-memory keys)
- No key provider configured

**✅ ALLOWED in Production:**
- `VAULT` - HashiCorp Vault integration (recommended)
- `HSM` - Hardware Security Module (PKCS#11)

### Vault Configuration

**Required Fields:**
```json
{
  "vault_addr": "https://vault.example.com:8200",
  "vault_token": "s.xxxxxxxxxxxxxxxxxxxxxx",
  "kv_mount_path": "secret"  // optional, defaults to "secret"
}
```

**Optional Fields:**
```json
{
  "namespace": "production",       // Vault namespace (Enterprise)
  "role": "themisdb-production",  // AppRole name
  "tls_skip_verify": false        // MUST be false in production
}
```

**Example:**
```cpp
nlohmann::json vault_config = {
    {"vault_addr", "https://vault.example.com:8200"},
    {"vault_token", "s.abc123..."},
    {"kv_mount_path", "themisdb/keys"},
    {"namespace", "production"}
};

auto security = SecurityLayerBuilder()
    .withKeyProvider(
        SecurityLayerBuilder::KeyProviderType::VAULT,
        vault_config.dump()
    )
    .build();
```

**Validation Rules:**
1. `vault_addr` must start with `http://` or `https://`
2. `vault_addr` and `vault_token` cannot be empty
3. `tls_skip_verify=true` triggers a warning (not recommended for production)

**Error Messages:**
```
Production mode violation: LOCAL (mock) key provider is not allowed in production.
Use VAULT or HSM key provider instead.
Set THEMIS_PRODUCTION_MODE=0 or THEMIS_ENVIRONMENT=development for testing.
```

### HSM Configuration

HSM support is gated behind a feature flag:

**Enable HSM:**
```bash
export THEMIS_HSM_ENABLED=1
```

**Required Fields:**
```json
{
  "library_path": "/usr/lib/softhsm/libsofthsm2.so",
  "slot_id": "0",
  "pin": "1234"
}
```

**Example:**
```cpp
nlohmann::json hsm_config = {
    {"library_path", "/usr/lib/pkcs11/libpkcs11.so"},
    {"slot_id", "0"},
    {"pin", std::getenv("HSM_PIN")}  // Use env var for sensitive data
};

auto security = SecurityLayerBuilder()
    .withKeyProvider(
        SecurityLayerBuilder::KeyProviderType::HSM,
        hsm_config.dump()
    )
    .build();
```

**Error Messages:**
```
HSM key provider is not enabled.
Set THEMIS_HSM_ENABLED=1 to enable HSM support.
Note: HSM support requires PKCS#11 libraries to be installed.
```

## JWT Validation Requirements

### Production Mode Restrictions

In production mode, JWT validation **requires** proper configuration:

**❌ PROHIBITED in Production:**
- Empty `jwks_url`
- Empty `expected_issuer`
- No JWT validator configured

**✅ REQUIRED in Production:**
- Valid JWKS URL or certificate file
- Expected issuer specification
- Algorithm validation (only RS256 supported)

### JWT Configuration

**Required Fields:**
```cpp
auth::JWTValidatorConfig config;
config.jwks_url = "https://auth.example.com/realms/themis/protocol/openid-connect/certs";
config.expected_issuer = "https://auth.example.com/realms/themis";
```

**Optional But Recommended:**
```cpp
config.expected_audience = "themisdb-api";        // Audience validation
config.cache_ttl = std::chrono::seconds(600);     // JWKS cache TTL (10 min)
config.clock_skew = std::chrono::seconds(60);     // Clock skew tolerance (1 min)
config.revoked_kids = {"old-key-id", "leaked-key"}; // Revoked key IDs
```

**Example:**
```cpp
auth::JWTValidatorConfig jwt_config;
jwt_config.jwks_url = "https://keycloak.example.com/realms/prod/protocol/openid-connect/certs";
jwt_config.expected_issuer = "https://keycloak.example.com/realms/prod";
jwt_config.expected_audience = "themisdb-api";
jwt_config.cache_ttl = std::chrono::seconds(600);
jwt_config.clock_skew = std::chrono::seconds(60);

auto security = SecurityLayerBuilder()
    .withKeyProvider(vault_type, vault_config)
    .withJWT(jwt_config)
    .build();
```

**Validation Rules:**
1. In production: `jwks_url` and `expected_issuer` cannot be empty
2. Only RS256 algorithm is supported
3. Tokens must include `exp` (expiration) claim
4. `nbf` (not before) and `iat` (issued at) are validated if present
5. Clock skew allows for time synchronization issues (default 60s)

**Error Messages:**
```
Production mode violation: No JWT validation configured.
Call withJWT() with proper configuration before build().
JWT validation is required in production mode.
```

```
JWT validation failed: Issuer mismatch (expected: https://auth.example.com, got: https://evil.com)
```

### JWT Token Validation

The validator enforces strict checks on all tokens:

**Checked Fields:**
- `alg` - Must be RS256
- `kid` - Must not be revoked
- `exp` - Must not be expired (with clock skew)
- `nbf` - Token not valid before this time (if present)
- `iat` - Issued at time must not be in future
- `iss` - Must match expected issuer
- `aud` - Must match expected audience (if configured)

**Revocation Support:**
```cpp
// Add kid to denylist at runtime
validator->revokeKid("compromised-key-id");

// Or configure at initialization
config.revoked_kids = {"old-key-1", "old-key-2"};
```

**Logging:**
All validation failures are logged with details:
```
JWT validation failed: Token expired
JWT validation failed: Issuer mismatch (expected: ..., got: ...)
JWT validation failed: Revoked kid: compromised-key-id
```

## Observability Requirements

### Production Mode Restrictions

In production mode, observability features **must** be enabled:

**❌ PROHIBITED in Production:**
- No-op tracer (tracing disabled)
- No-op metrics (metrics disabled)
- `createNoOp()` context

**✅ REQUIRED in Production:**
- Tracing enabled with valid endpoint
- Metrics enabled
- Proper logging configuration

### Logging Configuration

**Required:**
```cpp
ConcernsContext::Config config;
config.logLevel = "info";  // Valid: trace, debug, info, warn, error, critical, off
config.logFile = "/var/log/themisdb/themisdb.log";
config.logPattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v";
```

**Validation Rules:**
1. `logLevel` must be one of: trace, debug, info, warn, error, critical, off
2. Invalid log level causes immediate failure

**Error Messages:**
```
Invalid log_level: 'invalid'. Must be one of: trace, debug, info, warn, error, critical, off
```

### Tracing Configuration

**Required in Production:**
```cpp
config.tracingEnabled = true;
config.tracingEndpoint = "http://localhost:4318";  // OpenTelemetry collector
config.tracingServiceName = "themisdb-production";
```

**Validation Rules:**
1. If `tracingEnabled=false` in production, build fails
2. `tracingEndpoint` cannot be empty when tracing is enabled

**Error Messages:**
```
Production mode violation: Tracing is disabled.
Set tracingEnabled=true in ConcernsContext::Config for production deployments.
```

### Metrics Configuration

**Required in Production:**
```cpp
config.metricsEnabled = true;
```

**Error Messages:**
```
Production mode violation: Metrics are disabled.
Set metricsEnabled=true in ConcernsContext::Config for production deployments.
```

### Cache Configuration

**Optional but Recommended:**
```cpp
config.cacheMaxSize = 10000;        // Max number of cache entries
config.cacheDefaultTTL = 3600;      // Default TTL in seconds (0 = no TTL)
```

**Validation Rules:**
- Warning if `cacheMaxSize = 0` (cache effectively disabled)
- Warning if `cacheMaxSize > 1000000` (may consume significant memory)
- Warning if `cacheDefaultTTL < 60` (very short TTL)

## Complete Production Configuration Example

### Environment Variables

```bash
# Production mode
export THEMIS_PRODUCTION_MODE=1

# HSM support (if using HSM)
export THEMIS_HSM_ENABLED=1

# Vault authentication
export VAULT_ADDR=https://vault.example.com:8200
export VAULT_TOKEN=s.xxxxxxxxxxxxxxxxxxxxxx

# HSM PIN (if using HSM)
export HSM_PIN=1234
```

### Application Code

```cpp
#include "core/security_initialization.h"
#include "core/concerns/concerns_context.h"

// Configure Vault key provider
nlohmann::json vault_config = {
    {"vault_addr", std::getenv("VAULT_ADDR")},
    {"vault_token", std::getenv("VAULT_TOKEN")},
    {"kv_mount_path", "themisdb/keys"},
    {"namespace", "production"}
};

// Configure JWT validation
auth::JWTValidatorConfig jwt_config;
jwt_config.jwks_url = "https://auth.example.com/realms/prod/protocol/openid-connect/certs";
jwt_config.expected_issuer = "https://auth.example.com/realms/prod";
jwt_config.expected_audience = "themisdb-api";
jwt_config.cache_ttl = std::chrono::seconds(600);
jwt_config.clock_skew = std::chrono::seconds(60);
jwt_config.revoked_kids = {};  // Can be populated from config file

// Configure field encryption
EncryptionConfig encryption_config;
encryption_config.encrypted_fields = {"ssn", "credit_card", "password"};
encryption_config.field_key_mapping = {
    {"ssn", "pii_key"},
    {"credit_card", "payment_key"},
    {"password", "auth_key"}
};

// Build security layer
auto security = SecurityLayerBuilder()
    .withKeyProvider(
        SecurityLayerBuilder::KeyProviderType::VAULT,
        vault_config.dump()
    )
    .withFieldEncryption(encryption_config)
    .withRBACPolicy("/etc/themisdb/rbac.json")
    .withJWT(jwt_config)
    .build();

// Configure observability
ConcernsContext::Config concerns_config;
concerns_config.logLevel = "info";
concerns_config.logFile = "/var/log/themisdb/themisdb.log";
concerns_config.logPattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v";
concerns_config.tracingEnabled = true;
concerns_config.tracingEndpoint = "http://localhost:4318";
concerns_config.tracingServiceName = "themisdb-production";
concerns_config.metricsEnabled = true;
concerns_config.cacheMaxSize = 10000;
concerns_config.cacheDefaultTTL = 3600;

auto concerns = ConcernsContext::create(concerns_config);

// Wire ConcernsContext into the HTTP server for lifecycle and probe integration
auto http_server = std::make_unique<themis::server::HttpServer>(
    server_config, storage, secondary_index, graph_index, vector_index, tx_manager
);
http_server->setConcerns(concerns);
http_server->start();

// The server now:
//   - Passes `concerns` to MonitoringApiHandler so /health/live and
//     /health/ready report per-concern status (HTTP 503 if any concern
//     is unhealthy)
//   - Calls concerns->shutdown() at the end of HttpServer::stop() to
//     flush and release all concern resources in the correct order

// Use components
security.field_encryption->encrypt_field("ssn", plaintext);
security.rbac->checkPermission({"admin"}, "data", "write");
auto claims = security.jwt->parseAndValidate(token);

concerns->logger().info("ThemisDB started in production mode");
auto span = concerns->tracer().startSpan("startup");
concerns->metrics().incrementCounter("startup_total");
```

## Lifecycle Management

`ConcernsContext` exposes three methods for production lifecycle management:

| Method | When to call |
|--------|-------------|
| `flush()` | Before process suspension or between test cases; does **not** tear down the context. |
| `shutdown()` | Once, during process exit (e.g. `HttpServer::stop()`). Flushes then releases resources. |
| `healthCheck()` | Liveness probe (`/health/live`). Returns `HealthStatus{logger, tracer, metrics, cache}`. |
| `readinessCheck()` | Readiness probe (`/health/ready`). Same as `healthCheck()` for in-process backends. |

### Shutdown Order (guaranteed by `ConcernsContext::shutdown()`)

1. Flush logger, tracer, and metrics (ensures no pending data is lost)
2. `tracer.shutdown()` — exports remaining spans
3. `metrics.shutdown()` — publishes final snapshot
4. `cache.shutdown()` — persists or releases cache entries
5. `logger.shutdown()` — final flush and sink teardown

`HttpServer::stop()` calls `concerns->shutdown()` **after** all RocksDB and
network teardown to guarantee the logger is available for shutdown messages.



## Failure Modes and Error Messages

### Production Mode Violations

All production mode violations throw `std::runtime_error` with clear messages:

| Violation | Error Message |
|-----------|--------------|
| Mock key provider | `Production mode violation: LOCAL (mock) key provider is not allowed in production. Use VAULT or HSM key provider instead.` |
| No key provider | `Production mode violation: No key provider configured. Call withKeyProvider() with VAULT or HSM configuration before build().` |
| No JWT config | `Production mode violation: No JWT validation configured. Call withJWT() with proper configuration before build().` |
| Tracing disabled | `Production mode violation: Tracing is disabled. Set tracingEnabled=true in ConcernsContext::Config for production deployments.` |
| Metrics disabled | `Production mode violation: Metrics are disabled. Set metricsEnabled=true in ConcernsContext::Config for production deployments.` |
| NoOp context | `Production mode violation: Cannot create no-op ConcernsContext in production. Use create() or createCustom() with real implementations instead.` |

### Configuration Validation Errors

| Error Type | Error Message |
|------------|--------------|
| Invalid Vault config | `Invalid Vault configuration:\nERROR: vault_addr is required and cannot be empty\nERROR: vault_token is required and cannot be empty` |
| Invalid JWT config | `Invalid JWT configuration:\nERROR: JWT jwks_url is required in production mode\nERROR: JWT expected_issuer is required in production mode` |
| Invalid log level | `Invalid log_level: 'invalid'. Must be one of: trace, debug, info, warn, error, critical, off` |
| HSM not enabled | `HSM key provider is not enabled. Set THEMIS_HSM_ENABLED=1 to enable HSM support.` |

### JWT Validation Failures

| Failure Type | Error Message |
|--------------|--------------|
| Invalid format | `JWT validation failed: Invalid format (expected 3 parts)` |
| Unsupported algorithm | `JWT validation failed: Unsupported algorithm: HS256 (only RS256 is supported)` |
| Revoked kid | `JWT validation failed: Revoked kid: compromised-key-id` |
| Missing exp | `JWT validation failed: Missing exp claim` |
| Expired token | `JWT validation failed: Token expired` |
| Not yet valid | `JWT validation failed: Token not yet valid (nbf)` |
| Issuer mismatch | `JWT validation failed: Issuer mismatch (expected: https://auth.example.com, got: https://evil.com)` |
| Audience mismatch | `JWT validation failed: Audience mismatch` |
| JWK not found | `JWT validation failed: JWK not found for kid: unknown-key` |
| Signature failed | `JWT validation failed: Signature verification failed for kid: key-123` |

## Development vs Production

### Development Mode

When production mode is **not** enabled:

**Allowed:**
- Mock/LOCAL key providers
- Empty JWT configuration
- Tracing and metrics disabled
- No-op implementations

**Use Cases:**
- Unit testing
- Local development
- CI/CD test environments

**Example:**
```bash
# Development mode (no env vars set)
./themisdb --config dev.yaml
```

### Production Mode

When production mode **is** enabled:

**Required:**
- Secure key provider (VAULT or HSM)
- JWT validation configured
- Tracing enabled
- Metrics enabled
- Proper RBAC policy

**Use Cases:**
- Production deployment
- Staging environments
- Security compliance validation

**Example:**
```bash
export THEMIS_PRODUCTION_MODE=1
export VAULT_ADDR=https://vault.example.com:8200
export VAULT_TOKEN=$(cat /run/secrets/vault-token)
./themisdb --config production.yaml
```

## Best Practices

### 1. Use Environment Variables for Secrets

```cpp
// ❌ BAD: Hardcoded credentials
nlohmann::json config = {
    {"vault_token", "s.hardcoded123"}
};

// ✅ GOOD: Environment variables
nlohmann::json config = {
    {"vault_token", std::getenv("VAULT_TOKEN")}
};
```

### 2. Rotate Keys Regularly

```bash
# Rotate Vault keys
vault kv put themis/keys/pii_key key=$(openssl rand -base64 32) version=2

# Revoke compromised JWT keys
validator->revokeKid("old-key-id");
```

### 3. Monitor Validation Failures

JWT validation failures are logged automatically. Set up alerts:

```
# Example log pattern
JWT validation failed: Issuer mismatch (expected: https://auth.example.com, got: https://attacker.com)

# Alert rule (Prometheus/Grafana)
rate(jwt_validation_failures_total[5m]) > 10
```

### 4. Test Production Configuration

```cpp
// Test configuration in staging first
try {
    auto security = SecurityLayerBuilder()
        .withKeyProvider(vault_type, vault_config)
        .withJWT(jwt_config)
        .build();
    std::cout << "Configuration valid" << std::endl;
} catch (const std::runtime_error& e) {
    std::cerr << "Configuration error: " << e.what() << std::endl;
    return 1;
}
```

### 5. Use Separate Vault Namespaces

```json
{
  "vault_addr": "https://vault.example.com:8200",
  "vault_token": "s.prod_token",
  "namespace": "production",  // Separate from dev/staging
  "kv_mount_path": "themisdb/keys"
}
```

## Troubleshooting

### Issue: "Production mode violation: No key provider configured"

**Cause:** `build()` called without `withKeyProvider()`

**Solution:**
```cpp
auto security = SecurityLayerBuilder()
    .withKeyProvider(SecurityLayerBuilder::KeyProviderType::VAULT, vault_config)
    .build();
```

### Issue: "Invalid Vault configuration: vault_addr is required"

**Cause:** Missing or empty `vault_addr` in configuration

**Solution:**
```cpp
nlohmann::json config = {
    {"vault_addr", "https://vault.example.com:8200"},  // Must be set
    {"vault_token", "s.xxx"}
};
```

### Issue: "JWT validation failed: Issuer mismatch"

**Cause:** Token issuer doesn't match `expected_issuer`

**Solution:**
1. Check token: `jwt.io` to decode and verify `iss` claim
2. Update configuration to match actual issuer
3. Or update auth server to use correct issuer

### Issue: "HSM key provider is not enabled"

**Cause:** HSM provider requested but feature flag not set

**Solution:**
```bash
export THEMIS_HSM_ENABLED=1
```

## Related Documentation

- [Core Module README](README.md) - General core module documentation
- [Security Architecture](../../docs/security/ARCHITECTURE.md) - Overall security design
- [Vault Integration Guide](../../docs/security/VAULT_INTEGRATION.md) - Detailed Vault setup
- [JWT Integration Guide](../../docs/security/JWT_INTEGRATION.md) - JWT configuration examples
