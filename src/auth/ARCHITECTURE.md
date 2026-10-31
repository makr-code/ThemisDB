# Architecture - Auth Module

- **AI_Used:** true
- **AI_Models:** unknown
- **AI_Contributor:** GitHub Copilot
- **Human_Review:** false
- **Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

<!-- Status: current | validated: 2026-10-31 -->
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

### Implementation dependencies (examples verified in source)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| utils | `include/utils/audit_logger.h` | Optional audit integration used by auth components |
| external libraries | OpenSSL, RocksDB, and feature-conditional LDAP/Kerberos/Redis integrations | Concrete cryptographic, persistence, and provider operations; availability depends on build configuration |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `src/server/auth_middleware.cpp`, `src/server/session_api_handler.cpp`, `src/server/oauth2_provider.cpp`, `src/server/saml_auth_provider.cpp` | These source files include and use the corresponding auth interfaces; this does not imply every request uses every provider. |

## Verified integration points

### JWT validation

`JWTValidator::parseAndValidate()` returns `JWTClaims` and reports validation
failures by throwing `std::runtime_error`. `src/server/auth_middleware.cpp`
constructs and uses `JWTValidator`; its HTTP response mapping is server behavior,
not a contract of this module architecture page.

### Sessions and revocation

`SessionManager` owns in-memory session lifecycle operations. `TokenBlacklist`
implements the in-memory JTI blacklist; RocksDB, Redis, and distributed
blacklists are separate types. The public `SessionManager` API does not expose a
`revoke()` operation or atomically update a token blacklist. Applications must
wire session and token-revocation checks explicitly.

### mTLS

`MTLSAuthenticator` is configured and used by server auth middleware. No
`KeyProvider::getCertChain()` integration is established by the auth headers or
the inspected server integration; certificate freshness and transport-layer
TLS policy remain deployment-specific.