---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# ThemisDB Server Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument fasst deployment-seitige Sicherheitsanforderungen für das
Server-Modul zusammen. Die Anforderungen sind operational guidance, keine
Garantie, dass jeder Listener oder Handler sie automatisch durchsetzt.
Prüfe für das konkrete Deployment die aktivierte Transport-, Auth- und
Admission-Konfiguration.

## Dokumentabgrenzung (Canonical Split)

- **`src/server/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/server/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/server/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/server/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Transport configuration

- `HttpServer::Config::enable_tls` defaults to `false`; plaintext HTTP is
  therefore possible unless TLS is configured by the deployment.
- TLS startup requires certificate and key paths when TLS is enabled.
- `ApiAuthConfig::createSecureDefaults()` and `createDevDefaults()` are
  explicit profiles. The selected profile and the actual `AuthMiddleware`
  configuration determine which credentials and scopes are accepted.
- No `validateTransportSecurity()` API or `THEMIS_PRODUCTION_MODE` /
  `THEMIS_ENVIRONMENT` switch is defined by the server module; do not rely on
  those names as startup enforcement.

## Authentication and request controls

- Configure and inject the authentication and admission components required
  by each listener and route. The presence of an auth or limiter API alone
  does not enable it globally.
- Select JWT, API-key, Kerberos, mTLS, or USB-admin authentication only when
  its provider and corresponding credentials are configured.
- The server includes rate-limiting and load-shedding components, but their
  use and rejection behavior are determined by the runtime path and config.
- Configure and test each protocol session's timeout and close behavior
  independently; do not assume one lifecycle policy applies to all protocols.

## Source-grounded runtime notes

- `HttpServer::routeRequest()` and `APIGateway::handleRequest()` are separate
  request paths; confirm which one handles the endpoint being changed.
- TLS, HTTP/2, and HTTP/3 are configured separately through server settings;
  enabling TLS is not implied by selecting a protocol.
- Authentication failures, rate-limit decisions, and unknown routes are
  returned through the relevant handler or gateway response path; avoid
  assuming a single status mapping for every endpoint.

## Deployment checklist

- [ ] Confirm TLS and certificate configuration for every externally reachable listener.
- [ ] Confirm the intended auth profile and enabled provider for every protected route.
- [ ] Set and verify the rate-limit and load-shed thresholds used by the deployment.
- [ ] Exercise health, timeout, and shutdown behavior for enabled protocols.
- [ ] Verify public status routes do not expose sensitive metadata.

## Source references

### Betroffene Dateien im Review

- `include/server/http_server.h`
- `include/server/api_auth_config.h`
- `include/server/auth_middleware.h`
- `include/server/rate_limiting_middleware.h`
- `include/server/load_shedder.h`
- `src/server/http_server.cpp`
- `src/server/api_auth_config.cpp`
- `src/server/auth_middleware.cpp`
- `src/server/rate_limiting_middleware.cpp`
- `src/server/load_shedder.cpp`
