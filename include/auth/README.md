# Auth Module Headers

This directory contains header files (.h, .hpp) for the auth module.

## Components

### JWT Validator (`jwt_validator.h`)
- Validates JWT tokens from OpenID Connect providers (Keycloak, etc.)
- Verifies signatures using JWKS
- Extracts claims for authorization and encryption key derivation
- Supports RS256 algorithm
- Caches JWKS for performance

### GSSAPI Authenticator (`gssapi_authenticator.h`) - NEW
- Implements Kerberos/GSSAPI authentication for enterprise SSO
- Supports MIT Kerberos, Active Directory, and Heimdal
- Maps Kerberos principals to ThemisDB RBAC roles
- Provides fallback to basic authentication
- Cross-platform (Linux, Windows, macOS)

## Purpose

Public interfaces and declarations for auth functionality.

## Implementation

See `../../src/auth/` for the implementation code.

## Documentation

- See `../../docs/en/security/KERBEROS_AUTHENTICATION.md` for Kerberos setup
- See `../../docs/src/auth/` for detailed module documentation.
