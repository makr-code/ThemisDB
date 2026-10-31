---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> **Status:** Anforderungen sind normativ; die Erfüllung durch eine konkrete Produktions-Deployment-Konfiguration wurde in diesem Dokumentations-Review nicht verifiziert.

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

- **MUST:** Geschützte API-Endpunkte müssen Auth-Middleware explizit in ihrer Request-Kette aktivieren.
- **MUST:** GraphQL-Eingaben müssen mit geeigneten `QueryLimits` validiert werden; Schema- und Autorisierungsprüfungen dürfen nicht allein aus dem Parser abgeleitet werden.
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen muss für das jeweilige Deployment konfiguriert und geprüft sein.

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch geprüft sein; Modul-Defaults sind keine Aussage zur Sicherheit einer Deployment-Konfiguration.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten (Backends, Registries, Koordinatoren) müssen mit expliziten Verbindungs-Timeouts und Retry-Policies konfiguriert sein.

## Minimaler Produktions-Check (Audit-fähig)

- [?] Modul-Konfiguration und Startvalidierung anhand des konkreten Deployment-Profils nachweisen
- [?] Authentifizierungs- und Autorisierungsprüfungen für jede geschützte Route nachweisen
- [?] Ressourcen-Limits explizit konfigurieren und ihre wirksame Durchsetzung im jeweiligen Handler-Pfad nachweisen
- [?] Audit-Logging für sicherheitsrelevante Operationen im Deployment nachweisen
- [?] Externe Abhängigkeiten und ihre konkreten Timeout-/Retry-Verträge pro Adapter prüfen
- [?] Produktionsmodus und zugehörige Laufzeitkonfiguration anhand des Deployments nachweisen; dieses Modul-Dokument belegt keine allgemeine Umgebungsvariable

### Historische Code-Evidenz (2026-08-19; kein aktueller Deployment-Nachweis)

- **gRPC-Startvalidierung**: `src/api/grpc_server.cpp` validiert unter anderem Message-Size-Limits und behandelt `BuildAndStart()`-Fehler.
- **Request- und Autorisierungspfade**: `TransportPolicyMiddleware` lehnt Policy-Verstöße fail-closed ab, wenn sie in die Handler-Kette eingebaut ist; `WsChangeHandler` prüft CDC-Berechtigungen vor der Upgrade-Entscheidung.
- **Ressourcen-Limits**: `api_transport_contracts.h`, `api_transport_policy.cpp`, `graphql.cpp` und `graphql_aql_resolver.cpp` enthalten Payload-/Path-, GraphQL- und Komplexitätsgrenzen.
- **Audit und Telemetrie sind getrennte Pfade**: `ws_handler.cpp` kann strukturierte Audit-Ereignisse für CDC-Autorisierungsfehler erfassen; `graphql_audit_logger.h` stellt Audit-Logging bereit. OTLP-Exporter-Warnungen sind kein Audit-Logging.
- **Exporter-Retry**: `src/api/otlp_exporter.cpp` implementiert Wiederholungs-/Backoff-Verhalten für den Exportpfad. Das belegt keine allgemeine Retry- oder Timeout-Policy aller externen Abhängigkeiten.
- **gRPC-Reflection**: Der Quellcode schränkt Reflection abhängig von Build-/Test-Kontexten ein; diese historische Codebeobachtung ersetzt keine Deployment-Prüfung.

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/api/PRODUCTION_REQUIREMENTS.md`
- `src/api/grpc_server.cpp`
- `src/api/graphql.cpp`
