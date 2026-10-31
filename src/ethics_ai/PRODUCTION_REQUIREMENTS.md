---
Author: ThemisDB Team
Created: 2026-08-18
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

> **Historical status note (2026-08-18):** The prior retrospective reported Art. 13/22 closure and a production-grade baseline. That report is not current test, hardware, production-integration, or legal compliance evidence. The requirements and checklist below remain the normative requirements; current status is tracked in `ROADMAP.md`.

> This file defines requirements, not a claim that the current plugin enforces them. In particular, this source review did not find `THEMIS_PRODUCTION_MODE` or `THEMIS_ENVIRONMENT` handling in the `ethics_ai` plugin; verify deployment-specific enforcement outside this module before relying on that checklist item.

# ThemisDB Ethics_ai Module - Production Requirements

<!-- Links: FUTURE_ENHANCEMENTS.md · README.md · ROADMAP.md -->

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Ethics_ai-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Argument-Store, Chain-Visualizer, Convergence-Marker-Engine.

## Dokumentabgrenzung (Canonical Split)

- **`src/ethics_ai/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/ethics_ai/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/ethics_ai/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/ethics_ai/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Ethics-AI-Entscheidungen werden auditiert; Audit-Trail für alle Ethical-Reasoning-Chains aktiv.
- **MUST:** Cross-School-Tension-Resolver mit definierten Policy-Bounds konfiguriert; kein unbegrenztes Reasoning ohne Convergence-Marker.
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten müssen mit expliziten Verbindungs-Timeouts und Retry-Policies konfiguriert sein.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Modul-Konfiguration vollständig und beim Start validiert
- [ ] Sicherheits- und Autorisierungs-Checks aktiv
- [ ] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [ ] Audit-Logging aktiv
- [ ] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/ethics_ai/PRODUCTION_REQUIREMENTS.md`
- `src/ethics_ai/argument_store.cpp`
- `src/ethics_ai/chain_visualizer.cpp`
