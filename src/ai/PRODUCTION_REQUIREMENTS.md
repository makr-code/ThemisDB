---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-11-18"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB AI Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument definiert release-relevante Mindestanforderungen für die
Integration des AI-Moduls. Es ist kein Nachweis, dass `src/ai/` selbst alle
Kontrollen implementiert. Die Generator-API validiert Prompts und bestimmte
Runtime-Konfigurationen; Plugin-Policy, Autorisierung, Audit-Aufbewahrung und
isolierter Build/Execution-Schutz müssen durch die aufrufende Anwendung oder
die separate Plugin-/Security-Infrastruktur nachgewiesen werden.

## Dokumentabgrenzung (Canonical Split)

- **`src/ai/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/ai/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/ai/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/ai/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Aufrufende Anwendungen setzen Plugin-Policy durch und laden generierte oder ungeprüfte Artefakte nicht.
- **MUST:** Autorisierung und Operations-Guarding erfolgen an der Integrationsgrenze; `AIPluginGenerator` implementiert keinen `AiOperationGuard`.
- **MUST:** Endpoint, positive Timeouts/Größenlimits und konfigurierte Gates werden validiert; fehlende oder ungültige Werte führen vor Endpoint-I/O zu Fail-Closed-Verhalten.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces der Integration geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen ist eine Deployment-Anforderung; die Generator-API bietet kumulative Prozesszähler und optionale Traces, jedoch kein dauerhaftes Audit-Log.
- Materialisierung nach `sandbox_dir` und Round-trip-Prüfung bieten keine Betriebssystem-Isolation, keinen Compile-/Testlauf und keine Code-Sicherheitsprüfung.

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten (Backends, Registries, Koordinatoren) müssen mit expliziten Verbindungs-Timeouts und Retry-Policies konfiguriert sein.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Integrations- und Modulkonfiguration vollständig vor Nutzung validiert
- [ ] Sicherheits- und Autorisierungs-Checks aktiv
- [ ] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [ ] Audit-Logging aktiv
- [ ] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/ai/PRODUCTION_REQUIREMENTS.md`
- `src/ai/ai_plugin_generator.cpp`
