> **Status:** 2026-06-01 – mit aktuellem Api-Code (`grpc_server.cpp`) abgeglichen.

# ThemisDB Api Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Api-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für gRPC-Server, GraphQL-Handler, API-Gateway.

## Dokumentabgrenzung (Canonical Split)

- **`src/api/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/api/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/api/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/api/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** API-Endpunkte mit Auth-Middleware abgesichert.
- **MUST:** GraphQL-Input-Validierung aktiv; Schema-Enforcement erzwungen.
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten (Backends, Registries, Koordinatoren) müssen mit expliziten Verbindungs-Timeouts und Retry-Policies konfiguriert sein.

## Minimaler Produktions-Check (Audit-fähig)

- [x] Modul-Konfiguration vollständig und beim Start validiert
- [x] Sicherheits- und Autorisierungs-Checks aktiv
- [x] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [x] Audit-Logging aktiv
- [x] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [x] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

### Evidenz zur Checklisten-Erfüllung (2026-08-19)

- **Startzeit-Konfigurationsvalidierung**: `src/api/grpc_server.cpp` validiert Startup-Konfiguration (z. B. Message-Size-Limits, BuildAndStart-Fail-Closed).
- **Sicherheits-/Autorisierungschecks**: `src/api/api_transport_policy.cpp` erzwingt fail-closed Transport-Policy-Regeln; `src/api/ws_handler.cpp` behält Authz-Gates für Subscription-Pfade.
- **Explizite Ressourcen-Limits**: `include/api/api_transport_contracts.h` und `src/api/api_transport_policy.cpp` erzwingen Payload-/Path-Limits; `src/api/graphql_aql_resolver.cpp` begrenzt Komplexitäts-Tiefe und Overflow.
- **Audit-Logging**: `src/api/ws_handler.cpp` und `src/api/otlp_exporter.cpp` emittieren strukturierte Warn-/Fehlerpfade mit ERR_-Präfixen für Betriebs-Triage.
- **Timeout/Retry externe Abhängigkeiten**: `src/api/grpc_server.cpp` nutzt zeitgebundene Lock-Reakquise beim Serverstart; `src/api/otlp_exporter.cpp` nutzt Retry-with-backoff für Exportpfade.
- **Produktionsmodus-Gating**: Reflection in gRPC wird in `src/api/grpc_server.cpp` auf Nicht-Produktionskontexte begrenzt (`!NDEBUG && !THEMIS_TEST_BUILD`).

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/api/PRODUCTION_REQUIREMENTS.md`
- `src/api/grpc_server.cpp`
- `src/api/graphql.cpp`
