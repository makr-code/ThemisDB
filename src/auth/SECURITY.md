---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Security - Auth Module

<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · TRUST.md -->

Report vulnerabilities privately via [project SECURITY.md](../../SECURITY.md). This page describes module controls, not product-wide certification.

## Security Scope

Security in the auth module focuses on fail-closed identity validation, resilient credential/token handling, replay/revocation controls, and continuous trust verification boundaries.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| forged or tampered tokens | signature/claim validation and revocation checks in token validators |
| credential brute-force and replay | rate-limiting, replay-cache, and blacklist/session controls |
| provider-integration abuse | explicit protocol adapters with structured validation and failure handling |
| cross-realm identity confusion | federated identity and principal validation boundaries |
| long-lived trust drift | zero-trust continuous verification and session enforcement paths |

## Implemented Security Controls

- authentication adapters are explicit and method-specific.
- token/session flows include revocation/replay-aware support surfaces.
- audit and metrics paths provide security-relevant observability.
- policy/trust checks can gate downstream execution before business handlers.

## Deployment Risks and Follow-ups

- continue hardening distributed revocation and federation edge behavior.
- maintain strict handling for optional provider degradation scenarios.
- keep threat-specific diagnostics and enforcement behavior testable and observable.
- JWT issuer and audience defaults depend on the constructor: the URL-only constructor disables both checks and warns; `JWTValidatorConfig` defaults both requirements to true and construction fails if the expected values are missing.
- The default JWKS cache lifetime is 600 seconds. Operators must account for cache freshness and key-revocation propagation when rotating issuer keys.
- Authentication secrets, bearer tokens, session identifiers, and provider credentials are sensitive. Do not put them in logs, issue reports, command-line arguments, or unprotected configuration.

## Incident Handling

For suspected credential or token compromise, revoke the token/key at its issuer, invalidate affected sessions and blacklist entries, preserve authentication audit events, and verify issuer/audience and JWKS configuration before restoring access. Follow [SOP-05](../../SOP.md) and [SOP-08](../../SOP.md).

## Sourcecode Verification (Module: auth/security)

- Verified files:
  - src/auth/jwt_validator.cpp
  - src/auth/token_blacklist.cpp
  - src/auth/totp_replay_cache.cpp
  - src/auth/session_manager.cpp
  - src/auth/auth_rate_limiter.cpp
  - src/auth/zero_trust_auth_verifier.cpp
  - src/auth/auth_audit_logger.cpp
- Verified controls:
  - token and session validation surfaces (revocation and issuer/audience enforcement depend on caller configuration)
  - replay/revocation and rate-limiting controls
  - audit/trust verification paths

- Additional source references:
  - `src/auth/jwt_validator.cpp`
  - `src/auth/token_blacklist.cpp`
  - `src/auth/session_manager.cpp`
  - `src/auth/auth_rate_limiter.cpp`
  - [Trust and risk model](TRUST.md)