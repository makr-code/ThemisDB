# ThemisDB Auth Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The auth module provides authentication and authorization runtime surfaces for ThemisDB, including token validation, federated identity integration, MFA, session handling, and policy/verification support.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| jwt_validator.cpp | JWT/OIDC token validation paths |
| jwks_validator.cpp | JWKS fetch/validation helpers |
| gssapi_authenticator.cpp | Kerberos/GSSAPI authentication support |
| mfa_authenticator.cpp | MFA and TOTP-based authentication support |
| oauth_device_flow.cpp | OAuth device-flow authentication support |
| oauth_pkce_flow.cpp | OAuth PKCE authentication support |
| saml_authenticator.cpp | SAML authentication integration |
| ldap_authenticator.cpp | LDAP authentication path |
| api_key_authenticator.cpp | API-key authentication path |
| mtls_authenticator.cpp | mTLS authentication support |
| webauthn_authenticator.cpp | WebAuthn/FIDO2 authentication support |
| session_manager.cpp | session lifecycle and revocation handling |
| token_blacklist.cpp | token revocation/blacklist support |
| zero_trust_auth_verifier.cpp | zero-trust verification checks |

## Scope

In scope:
- multi-protocol authentication method support
- token/session validation and revocation flows
- RBAC/ABAC-adjacent policy enforcement surfaces in auth module
- audit/rate/metrics support for auth decisions

Out of scope:
- business-domain authorization logic in non-auth modules
- transport routing internals outside auth integration points
- non-auth storage/query execution internals

## Runtime Behavior and Limits

- auth behavior depends on configured providers, credentials, and feature flags.
- module paths include both local and federated authentication modes.
- optional integrations degrade through structured failure behavior when unavailable.

## Sourcecode Verification (Module: auth/readme)

- Verified files:
  - src/auth/jwt_validator.cpp
  - src/auth/jwks_validator.cpp
  - src/auth/gssapi_authenticator.cpp
  - src/auth/mfa_authenticator.cpp
  - src/auth/oauth_device_flow.cpp
  - src/auth/oauth_pkce_flow.cpp
  - src/auth/saml_authenticator.cpp
  - src/auth/ldap_authenticator.cpp
  - src/auth/api_key_authenticator.cpp
  - src/auth/mtls_authenticator.cpp
  - src/auth/webauthn_authenticator.cpp
  - src/auth/session_manager.cpp
  - src/auth/token_blacklist.cpp
  - src/auth/zero_trust_auth_verifier.cpp
- Verified behavior surfaces:
  - authentication method adapters
  - token/session verification and revocation paths
  - policy and zero-trust validation helpers
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical completion remains in CHANGELOG.md