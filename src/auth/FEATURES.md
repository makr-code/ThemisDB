---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: unknown
---

# Auth in ThemisDB

Exact model backend not verifiable from final document text alone.

## Ueberblick
Das Auth-Modul stellt in `include/auth` deklarierte Authentifizierungs-, Session-, Revokations- und Policy-Schnittstellen bereit. Die Implementierungen liegen in `src/auth`; konkrete Adapter und Integrationen hängen von Konfiguration und verfügbaren Build-Abhängigkeiten ab.

## Kern-Features im Detail
- **Token validation:** JWT/JWKS and OIDC provider APIs (`jwt_validator.h`, `jwks_validator.h`, `oidc_provider.h`).
- **Authentication adapters:** LDAP, Kerberos/GSSAPI, SAML, OAuth device/PKCE, mTLS, API keys, MFA/TOTP, WebAuthn/passkeys, and eID each have separate interfaces.
- **State and controls:** in-memory sessions, token-blacklist implementations, optional distributed blacklist synchronization, rate-limiter backends, audit/metrics surfaces, and authorization/trust helpers.

## Kundennutzen und Abgrenzung
The module provides reusable authentication and trust-control components; it does not by itself establish deployment policy, provider availability, session persistence, distributed consistency, or product-wide compliance certification. JWT issuer/audience checks and blacklist use depend on the selected constructor/backend and the embedding application's configuration. Failure types also vary by adapter.

## Einordnung und Quellen
Canonical references: [public auth headers](../../include/auth/README.md), [module README](./README.md), [architecture](./ARCHITECTURE.md), [security](./SECURITY.md), and [roadmap](./ROADMAP.md). Implementation claims should be checked against the corresponding `include/auth/*.h` and `src/auth/*.cpp` files.
