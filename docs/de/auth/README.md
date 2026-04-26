# Authentication-Modul

**Stand:** 6. April 2026
**Version:** 2.0.0
**Kategorie:** Auth

---

## Übersicht

Das Auth-Modul implementiert enterprise-grade Authentifizierung und Autorisierung für ThemisDB.
Es unterstützt alle gängigen Standards und Protokolle für moderne Sicherheitsinfrastrukturen.

**Reifegrad:** 🟢 Production-Ready — alle Kernkomponenten sind vollständig implementiert und getestet.

---

## Unterstützte Authentifizierungsverfahren

| Verfahren | Status | Standard / Protokoll |
|-----------|--------|----------------------|
| JWT / OpenID Connect | ✅ Production | RFC 7519, OIDC Core 1.0 |
| Kerberos / GSSAPI | ✅ Production | RFC 4120 |
| TOTP Multi-Factor Authentication | ✅ Production | RFC 6238 |
| OAuth 2.0 Device Flow | ✅ Production | RFC 8628 |
| OAuth 2.0 PKCE | ✅ Production | RFC 7636 |
| SAML 2.0 SP/IdP SSO | ✅ Production | SAML 2.0 Web SSO Profile |
| WebAuthn / FIDO2 | ✅ Production | WebAuthn Level 2 |
| LDAP / Active Directory | ✅ Production | RFC 4510 |
| mTLS (Mutual TLS) | ✅ Production | RFC 8446 |
| API Key Authentifizierung | ✅ Production | |
| Federated Identity | ✅ Production | |
| Zero-Trust Verifizierung | ✅ Production | NIST SP 800-207 |
| Session Management | ✅ Production | |
| OIDC Provider Discovery | ✅ Production | OIDC Discovery 1.0 |

---

## Security Note: LDAP DN/Filter Escaping

- LDAP-Benutzereingaben werden bei DN-Erstellung gemäß **RFC 4514** escaped (`buildUserDN`).
- LDAP-Filter-Platzhalter `{dn}` und `{username}` werden gemäß **RFC 4515** escaped (`buildGroupSearchFilter`).
- Dadurch werden DN-/Filter-Injection-Angriffe wie `*)(|(member=*))` in LDAP-Operationen verhindert.

---

## Komponenten-Übersicht

### Authentifikatoren

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| JWTValidator | `include/auth/jwt_validator.h` | `src/auth/jwt_validator.cpp` | JWT/OIDC Token-Validierung mit RS256/ES256/EdDSA und JWKS-Caching |
| GSSAPIAuthenticator | `include/auth/gssapi_authenticator.h` | `src/auth/gssapi_authenticator.cpp` | Kerberos/GSSAPI SSO für Active Directory |
| MFAAuthenticator | `include/auth/mfa_authenticator.h` | `src/auth/mfa_authenticator.cpp` | TOTP MFA mit Recovery Codes |
| OAuthDeviceFlow | `include/auth/oauth_device_flow.h` | `src/auth/oauth_device_flow.cpp` | OAuth 2.0 Device Authorization Grant |
| OAuthPKCEFlow | `include/auth/oauth_pkce_flow.h` | `src/auth/oauth_pkce_flow.cpp` | OAuth 2.0 Authorization Code + PKCE |
| OIDCProvider | `include/auth/oidc_provider.h` | `src/auth/oidc_provider.cpp` | OIDC Provider Discovery und Federation |
| SAMLAuthenticator | `include/auth/saml_authenticator.h` | `src/auth/saml_authenticator.cpp` | SAML 2.0 SP- und IdP-initiiertes SSO |
| LDAPAuthenticator | `include/auth/ldap_authenticator.h` | `src/auth/ldap_authenticator.cpp` | LDAP/AD Direct-Bind Authentifizierung |
| MTLSAuthenticator | `include/auth/mtls_authenticator.h` | `src/auth/mtls_authenticator.cpp` | Mutual TLS Zertifikat-Authentifizierung |
| WebAuthnAuthenticator | `include/auth/webauthn_authenticator.h` | `src/auth/webauthn_authenticator.cpp` | WebAuthn/FIDO2 Hardware-Token |
| ApiKeyAuthenticator | `include/auth/api_key_authenticator.h` | `src/auth/api_key_authenticator.cpp` | Statischer API Key + Secret |
| FederatedIdentityManager | `include/auth/federated_identity_manager.h` | `src/auth/federated_identity_manager.cpp` | Multi-Realm Federated Identity |
| ZeroTrustAuthVerifier | `include/auth/zero_trust_auth_verifier.h` | `src/auth/zero_trust_auth_verifier.cpp` | Kontinuierliche Per-Request Verifikation |

### Sicherheits- und Hilfskomponenten

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| SessionManager | `include/auth/session_manager.h` | `src/auth/session_manager.cpp` | Session-Lifecycle mit Revocation |
| TokenBlacklist | `include/auth/token_blacklist.h` | `src/auth/token_blacklist.cpp` | JTI-basierte Token-Sperrung |
| PasswordPolicy | `include/auth/password_policy.h` | `src/auth/password_policy.cpp` | Passwort-Komplexitätsregeln |
| PrincipalValidator | `include/auth/principal_validator.h` | `src/auth/principal_validator.cpp` | Principal-Validierung und Rollen-Mapping |
| AuthRateLimiter | `include/auth/auth_rate_limiter.h` | `src/auth/auth_rate_limiter.cpp` | Brute-Force und Replay-Schutz |
| JWKSValidator | `include/auth/jwks_validator.h` | `src/auth/jwks_validator.cpp` | JWKS-Endpoint Fetching und Caching |
| JWKSSecureFetcher | `include/auth/jwks_security.h` | `src/auth/jwks_security.cpp` | Sicherheitsvalidierung von JWKS Keys |
| JWTKeyRotationManager | `include/auth/jwt_key_rotation_manager.h` | `src/auth/jwt_key_rotation_manager.cpp` | Automatische JWT Key-Rotation |
| TOTPReplayCache | `include/auth/totp_replay_cache.h` | `src/auth/totp_replay_cache.cpp` | Replay-Schutz für TOTP-Codes |
| TOTPSecretEncryption | `include/auth/totp_secret_encryption.h` | `src/auth/totp_secret_encryption.cpp` | Verschlüsselte TOTP-Secret-Ablage |
| KerberosSecurityValidator | `include/auth/kerberos_security.h` | `src/auth/kerberos_security.cpp` | Kerberos-Sicherheitsvalidierungen |
| AuthAuditLogger | `include/auth/auth_audit_logger.h` | `src/auth/auth_audit_logger.cpp` | Audit-Logging aller Auth-Events |
| AuthMetrics | `include/auth/auth_metrics.h` | `src/auth/auth_metrics.cpp` | Prometheus-Metriken für Auth |
| AuthError | `include/auth/auth_error.h` | `src/auth/auth_error.cpp` | Strukturierte Fehlertypen |

---

## Schnellstart-Beispiele

### JWT-Validierung

```cpp
#include "auth/jwt_validator.h"
using namespace themis::auth;

JWTValidatorConfig cfg;
cfg.jwks_url       = "https://auth.example.com/.well-known/jwks.json";
cfg.expected_issuer   = "https://auth.example.com";
cfg.expected_audience = "themisdb-api";
cfg.jwks_cache_ttl = std::chrono::seconds(300);
cfg.clock_skew     = std::chrono::seconds(60);

JWTValidator validator(cfg);
auto claims = validator.parseAndValidate(bearer_token);
// claims.sub, claims.roles, claims.email verfügbar
```

### Kerberos/GSSAPI

```cpp
#include "auth/gssapi_authenticator.h"
using namespace themis::auth;

KerberosConfig cfg;
cfg.service_principal = "themisdb/db.example.com@EXAMPLE.COM";
cfg.keytab_file = "/etc/themisdb/themisdb.keytab";
cfg.principal_mappings = {{"admin@EXAMPLE.COM", "admin"}, {"*@EXAMPLE.COM", "user"}};

GSSAPIAuthenticator auth;
auth.initialize(cfg);
auto result = auth.authenticateToken(negotiate_token);
```

### TOTP MFA

```cpp
#include "auth/mfa_authenticator.h"
using namespace themis::auth;

MFAAuthenticator mfa;

// Enrollment
auto enrollment = mfa.generateEnrollment("alice@example.com");
std::string qr_uri = mfa.generateProvisioningURI(enrollment);

// Validierung
bool ok = mfa.validateTOTP(enrollment.secret_base32, user_code);
```

### Session Management

```cpp
#include "auth/session_manager.h"
using namespace themis::auth;

SessionManager sessions;
auto session_id = sessions.createSession("user-123", {"admin"}, 3600);
auto session    = sessions.validateSession(session_id);
sessions.revokeSession(session_id);
```

---

## Sicherheitsmerkmale

- **JWKS-Caching** — Public Keys werden mit konfigurierbarem TTL gecacht (Standard: 5 min)
- **Clock Skew Toleranz** — ±60 Sekunden für verteilte Umgebungen (konfigurierbar)
- **Replay-Schutz** — TOTP-Codes und Token-Nonces werden verfolgt (lock-free Ring-Buffer)
- **Brute-Force-Schutz** — Token-Bucket Rate Limiting mit exponentiellem Backoff
- **TOTP-Secret-Verschlüsselung** — AES-256-GCM / libsodium secretbox
- **Zero-Trust** — Kontinuierliche Per-Request Identitäts- und Netzwerk-Policy-Verifikation
- **Audit Logging** — Alle Auth-Events werden strukturiert protokolliert

---

## Compliance

| Standard | Status |
|----------|--------|
| RFC 6238 (TOTP) | ✅ |
| RFC 7519 (JWT) | ✅ |
| RFC 7636 (OAuth 2.0 PKCE) | ✅ |
| RFC 8628 (OAuth 2.0 Device Flow) | ✅ |
| RFC 4120 (Kerberos v5) | ✅ |
| OpenID Connect Core 1.0 | ✅ |
| SAML 2.0 Web Browser SSO Profile | ✅ |
| WebAuthn Level 2 / FIDO2 | ✅ |
| NIST SP 800-63B Level 2 | ✅ |
| SOC 2 CC6.1 | ✅ |

---

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [jwt.md](jwt.md) | JWT-Validierung: Konfiguration, Beispiel-JWKS, Test-Anleitung |
| [jwks_example.json](jwks_example.json) | Beispiel-JWKS-Datei für Tests |

---

## Weiterführende Dokumentation (Primary)

| Dokument | Pfad | Beschreibung |
|----------|------|--------------|
| Modul-README | [`src/auth/README.md`](../../../src/auth/README.md) | Vollständige Komponentenbeschreibung, Flows, Konfiguration, Beispiele |
| Architektur | [`src/auth/ARCHITECTURE.md`](../../../src/auth/ARCHITECTURE.md) | Komponentendiagramme, Datenflüsse, Threading-Modell |
| Roadmap | [`src/auth/ROADMAP.md`](../../../src/auth/ROADMAP.md) | Implementierungsstatus und geplante Features |
| Future Enhancements | [`src/auth/FUTURE_ENHANCEMENTS.md`](../../../src/auth/FUTURE_ENHANCEMENTS.md) | Detaillierte Planung zukünftiger Features |
| Public API Headers | [`include/auth/README.md`](../../../include/auth/README.md) | API-Referenz für alle 26 Header-Dateien |

---

## Verwandte Module

- [Security Module](../security/README.md) — Verschlüsselung, TLS, Key Management
