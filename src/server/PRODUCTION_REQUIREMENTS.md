> **Status:** 2026-06-01 – mit aktuellem Server-Code (`auth_middleware.cpp`, `rate_limiting_middleware.cpp`, `load_shedder.cpp`) abgeglichen.

# ThemisDB Server Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Server-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für HTTP/2-, gRPC-, WebSocket-, MQTT- und PostgreSQL-Wire-Protokoll-Endpunkte.

## Dokumentabgrenzung (Canonical Split)

- **`src/server/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/server/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/server/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/server/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Transportanforderungen

- **MUST:** TLS-Transport (`validateTransportSecurity(...)`) in Produktionsdeployments aktivieren; unsichere HTTP-Endpunkte nur in explizit deklarierten Nicht-Produktionsumgebungen zulassen.
- **MUST:** `api_auth_config` mit gültigem `auth_token` konfigurieren; leere/ungültige Tokens werden beim Start abgewiesen.
- **MUST NOT:** `auth_middleware` in Produktionspfaden deaktivieren oder umgehen.

## Verbindliche Sicherheitsanforderungen

### 1) Authentifizierung und Request-Kontrolle

- **MUST:** Auth-Middleware (`auth_middleware.cpp`) läuft vor allen sensiblen Handlern.
- **MUST:** JWT-Validierung aktiviert, wenn JWT-basierter Zugang konfiguriert ist.
- **MUST:** Ungültige Authentifizierungsversuche werden mit `401 Unauthorized` abgebrochen (Fail Closed).

### 2) Rate Limiting und Overload-Schutz

- **MUST:** `rate_limiting_middleware` mit expliziten Grenzen konfigurieren (kein Default-Unlimited).
- **MUST:** `load_shedder.cpp` aktiv, um Overload-Zustände mit messbaren Schwellwerten abzufangen.
- **MUST:** `adaptive_rate_limiter.cpp` in Produktionspfaden aktiv; statische Limiter allein reichen nicht.

### 3) Session- und Protokollbindung

- **MUST:** WebSocket-Sessions (`websocket_session.cpp`) mit bounded Lifecycle (kein Silent-Hang auf close()).
- **MUST:** MQTT-Sessions mit expliziten Verbindungsabbruch-Signalen konfiguriert.
- **MUST NOT:** Session-State ohne explizite Fehlerbehandlung bei Protokoll-Downgrade-Versuchen beibehalten.

## Betriebsgrenzen (aktuelles Server-Verhalten)

- Request-Flooding und Connection-Limiter werden vom `adaptive_rate_limiter` durchgesetzt; Overload-Events sind beobachtbar via `health_error_service.cpp`.
- WebSocket und MQTT Sessions werden bei Schließen über `net::dispatch(executor)` beendet; synchrone `ws.close()` Aufrufe in Write-Lock-Pfaden sind behoben.
- `api_gateway.cpp` leitet Anfragen an registrierte Handler weiter; unbekannte Routen werden mit explizitem Fehlercode abgelehnt.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] TLS-Transport aktiviert (kein plain HTTP ohne explizite Deklaration)
- [ ] Auth-Middleware aktiviert und konfiguriert
- [ ] `auth_token` nicht leer/blank
- [ ] Rate-Limiter und Load-Shedder mit expliziten Schwellwerten konfiguriert
- [ ] `health_error_service` erreichbar und liefert observierbare Overload-Signale
- [ ] Session-Lifecycle für WebSocket/MQTT bounded und testbar
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/server/PRODUCTION_REQUIREMENTS.md`
- `src/server/auth_middleware.cpp`
- `src/server/api_auth_config.cpp`
- `src/server/rate_limiting_middleware.cpp`
- `src/server/adaptive_rate_limiter.cpp`
- `src/server/load_shedder.cpp`
- `src/server/health_error_service.cpp`
- `src/server/websocket_session.cpp`
- `src/server/mqtt_session.cpp`
