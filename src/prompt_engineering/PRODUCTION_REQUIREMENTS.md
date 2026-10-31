---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Prompt_engineering Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument beschreibt Anforderungen an Dienste, die das `prompt_engineering`-Modul produktiv integrieren. Die Anforderungen sind keine Behauptung, dass das Modul selbst erzwingt oder bereitstellt; die Bibliothek führt weder LLM-Aufrufe noch deployment-weite Autorisierungs- oder Sicherheitsgates aus.

## Dokumentabgrenzung (Canonical Split)

- **`src/prompt_engineering/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/prompt_engineering/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/prompt_engineering/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/prompt_engineering/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Die integrierende Anwendung autorisiert Kontext und prüft Prompts vor dem Modellaufruf nach ihrer eigenen Sicherheitsrichtlinie.
- **MUST:** Die Anwendung darf `PromptEvaluator`-Scores nicht als Autorisierungs- oder Safety-Gate behandeln.
- **MUST:** Die Anwendung validiert ihre sicherheitsrelevante Konfiguration beim Start und definiert das Verhalten bei fehlenden oder ungültigen Werten.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über die Kontroll-Surfaces der integrierenden Anwendung geleitet.
- Fehlerbehandlung und Fail-Closed-Verhalten sind vom Host für seine sicherheitskritischen Pfade festzulegen.
- Der Host definiert Audit-Logging, Zugriffskontrolle und Aufbewahrung für Prompt-, Response- und Feedbackdaten.

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten wie Modell- und Embedding-Provider müssen vom Host mit passenden Timeouts und Retry-Policies konfiguriert sein.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Modul-Konfiguration vollständig und beim Start validiert
- [ ] Sicherheits- und Autorisierungs-Checks aktiv
- [ ] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [ ] Audit-Logging aktiv
- [ ] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [ ] Produktionsmodus und zugehörige Sicherheitsrichtlinien in der Host-Anwendung explizit konfiguriert

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/prompt_engineering/PRODUCTION_REQUIREMENTS.md`
- `src/prompt_engineering/chain_of_thought.cpp`
- `src/prompt_engineering/prompt_quality_evaluator.cpp`
