# Architecture - Auth Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The auth module composes multiple authentication and verification planes behind shared identity and policy contracts. It acts as the primary gate for identity validation before downstream module authorization and business logic execution.

## Main Execution Planes

1. Token and identity plane
- JWT/OIDC validation and key/JWKS handling
- principal extraction and identity normalization

2. Interactive and federated auth plane
- Kerberos, LDAP, SAML, OAuth, WebAuthn, mTLS, and API-key flows
- federated identity and provider coordination

3. Session and revocation plane
- session lifecycle and revocation management
- blacklist and replay/resilience controls

4. Trust and policy plane
- rate-limiting, policy checks, and zero-trust verification paths
- audit and metrics emission for auth decisions

## Core Contracts

| Contract | Behavior |
|---|---|
| authentication interfaces | validate credentials/tokens and produce principal context |
| session/revocation interfaces | track and enforce active/revoked identity state |
| policy interfaces | evaluate access constraints and trust posture |
| audit/metrics interfaces | emit operational and security decision signals |

## Failure Semantics

- invalid credentials/tokens fail closed with structured auth errors.
- unavailable optional providers produce explicit integration failures.
- trust/policy checks gate request continuation by design.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| utils | `include/utils/jwt_utils.h`, `include/utils/mfa_utils.h`, `include/utils/session_utils.h` | JWT parsing/signing helpers, MFA token utilities, session ID and TTL helpers |
| security | `include/security/key_provider.h` | Cryptographic key provisioning for JWT signature verification and mTLS cert handling |
| observability | `include/observability/audit_logger.h` | Emits structured audit events for authentication decisions and revocation actions |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/auth/jwt_validator.h`, `include/auth/mtls_authenticator.h`, `include/auth/oauth_pkce_flow.h`, `include/auth/oidc_provider.h`, `include/auth/saml_authenticator.h`, `include/auth/session_manager.h` | HTTP server validates every inbound request identity before routing; all six auth surfaces are consumed at request ingestion time |

## Integration Points

### Critical Integration: JWT Validation ↔ HTTP Server
**Files:** `include/auth/jwt_validator.h` ↔ `src/server/http_server.cpp`
**Contract:** `JwtValidator::validate()` must return a fully-resolved `PrincipalContext` or a typed `AuthError`; the server treats any non-success result as a 401 and does not continue routing.
**Thread Safety:** `JwtValidator` instances are read-shared across request threads after construction; JWKS key rotation is protected by an internal `std::shared_mutex`.
**Failure Mode:** Fail-closed — expired/invalid tokens produce structured `AuthError`; no partial principal context is forwarded downstream.

### Critical Integration: Session Manager ↔ Token Blacklist
**Files:** `include/auth/session_manager.h` ↔ `src/auth/token_blacklist.cpp`
**Contract:** `SessionManager::revoke()` atomically inserts the token JTI into the distributed blacklist before returning; subsequent `validate()` calls for the same JTI return `REVOKED` without contacting the upstream IdP.
**Thread Safety:** Blacklist entries are written under a distributed lock; reads are lock-free via a local in-process Bloom filter.
**Failure Mode:** If the distributed store is unreachable during revocation the operation fails with `BlacklistUnavailable`; the session is NOT marked revoked locally, preserving correctness over availability.

### Critical Integration: mTLS Authenticator ↔ Key Provider
**Files:** `include/auth/mtls_authenticator.h` ↔ `include/security/key_provider.h`
**Contract:** `MtlsAuthenticator` calls `KeyProvider::getCertChain()` per handshake; the key provider is responsible for CRL/OCSP freshness.
**Thread Safety:** `KeyProvider` implementations must be thread-safe; the authenticator holds no cert state between requests.
**Failure Mode:** `KeyProvider` errors propagate as `CertProviderUnavailable`; the mTLS handshake is rejected.



- Verified files:
  - src/auth/jwt_validator.cpp
  - src/auth/oidc_provider.cpp
  - src/auth/federated_identity_manager.cpp
  - src/auth/session_manager.cpp
  - src/auth/token_blacklist.cpp
  - src/auth/zero_trust_auth_verifier.cpp
  - src/auth/auth_rate_limiter.cpp
- Verified architecture claims:
  - multi-plane authentication composition
  - explicit session/revocation and policy layers
  - dedicated trust and observability surfaces in module