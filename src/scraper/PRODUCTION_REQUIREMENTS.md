> **Dokumentstatus:** Anforderungen; Umsetzung gegen Quellcode geprüft am 2026-10-31.
>
> **Author:** ThemisDB Contributors<br>
> **Created:** 2026-10-31<br>
> **Last Updated:** 2026-10-31<br>
> **Status:** review<br>
> **AI_Used:** true<br>
> **AI_Models:** unknown<br>
> **AI_Contributor:** GitHub Copilot; themisdb-doc-orchestrator<br>
> **Human_Review:** false<br>
> **Human_Author:** unknown<br>
> Exact model backend not verifiable from final document text alone.

# ThemisDB Scraper Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Scraper-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Scraper-Metadata-Writer, Gov-Source-Catalog und Web-Scraping. Die MUST-Aussagen sind Anforderungen an Deployment/Integration; sie beschreiben keine bereits vollständig erzwungenen Modul-Controls.

## Dokumentabgrenzung (Canonical Split)

- **`src/scraper/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/scraper/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/scraper/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/scraper/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Scraper-Zielquellen aus konfiguriertem und geprüftem Source-Catalog oder einer ausdrücklich freigegebenen Seed-Konfiguration; kein Scraping beliebiger Ziele ohne Freigabe.
- **MUST:** Rate-Limits, Delay und Retry-Policy in der Integration passend zur Quelle konfigurieren. Der Plugin-Pfad implementiert keine allgemeine Retry-Policy; `request_delay_ms` gilt nur für die HTML-Suchstrecke.
- **MUST:** Sicherheitsrelevante Konfiguration vor dem Start in der Integration validieren und bei ungültigen Werten fail-closed reagieren. `ScraperPlugin::initialize()` führt selbst keine umfassende semantische Validierung durch.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten müssen mit expliziten Verbindungs-Timeouts und Retry-Policies konfiguriert sein.
- `respect_robots`, `same_domain_only`, `max_depth`, crawl `max_pages`, `ScraperConfig::api_options`, `SearchOptions::results_per_page` sowie `LlmOptions::model_path` und `temperature` werden im aktuellen Plugin-Pfad nicht angewendet. `js_timeout_ms` wird an das Renderer-Kommando übergeben, beendet aber keinen hängenden Subprozess durch eine plugin-eigene Deadline.
- `UrlPolicy` nutzt einfache URL-String-Präfix-/Wildcard-Prüfungen; Redirect-Ziele werden nicht erneut geprüft. Es ersetzt keine Netzwerk-Egress-Kontrolle.
- `scraper_diagnostics.h` definiert Diagnose-Primitiven, aber der Plugin-Pfad emittiert keine strukturierten Diagnoseereignisse.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Modul-Konfiguration vollständig und beim Start validiert
- [ ] Sicherheits- und Autorisierungs-Checks aktiv
- [ ] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [ ] Audit-Logging aktiv
- [ ] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/scraper/PRODUCTION_REQUIREMENTS.md`
- `src/scraper/scraper_metadata_writer.cpp`
- `src/scraper/gov_source_catalog.cpp`
