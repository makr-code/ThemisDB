> **Status:** Deployment policy; source-enforcement boundary reviewed 2026-10-31.
<!-- Last Updated: 2026-10-31 | AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot Coding Agent | Human_Review: false | Human_Author: ThemisDB Team
Exact model backend not verifiable from final document text alone. -->

# ThemisDB Cache Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Cache-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Bounded-LRU-Cache, Distributed-Cache-Coordinator, Cache-Replication.

## Dokumentabgrenzung (Canonical Split)

- **`src/cache/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/cache/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/cache/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/cache/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Cache-Size-Limits explizit konfiguriert; kein Unlimited-Cache.
- **MUST (wenn verteilte Koordination für das Deployment vorgesehen ist):** Distributed-Cache-Coordinator und Replications-Policy explizit konfigurieren. `AdaptiveQueryCache::Config::enable_replication` ist standardmäßig deaktiviert.
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

- [ ] Modul-Konfiguration vollständig und beim Start validiert
- [ ] Sicherheits- und Autorisierungs-Checks aktiv
- [ ] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [ ] Audit-Logging aktiv
- [ ] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Abgrenzung zur Implementierung

Die obigen MUST-Aussagen sind Deployment-Anforderungen, keine Zusicherung, dass jede Cache-Klasse sie selbst erzwingt. Im Quellcode ist `AdaptiveQueryCache::Config::validate()` als Konfigurationsprüfung belegt; ein modulweiter Validator für alle Cache-, Coordinator- und Backend-Konfigurationen ist nicht belegt. Cache-interne Durchsetzung von Audit-Logging und Produktionsmodus-Umgebungsvariablen ist ebenfalls **TODO/unverified**; diese Kontrollen müssen an der zuständigen Service-/Storage-Grenze geprüft werden. `THEMIS_PRODUCTION_MODE` und `THEMIS_ENVIRONMENT` sind projektweite Storage-/Service-Konventionen, keine hier nachgewiesenen Cache-Modul-Schalter.

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/cache/PRODUCTION_REQUIREMENTS.md`
- `src/cache/bounded_lru_cache.cpp`
- `src/cache/distributed_cache_coordinator.cpp`
