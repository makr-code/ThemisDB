**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `security`
**Canonical source references:** [security API](../../include/content/content_security.h), [manager API](../../include/content/content_manager.h), [security implementation](content_security.cpp), [trust-boundary review](TRUST.md)
**Related milestone:** DOC-WEEKLY-2026-44
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Provenance note:** Exact model backend not verifiable from final document text alone.

> Historischer Review-Marker (2026-06-01): zuvor als mit `abuse_detector.cpp` abgeglichen
> dokumentiert; dieser Marker belegt nicht die aktuell offene Deployment-Verdrahtung.

# ThemisDB Content Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Content-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Abuse-Detektor, Async-Ingestion-Worker, Content-Processing.

### Evidenzgrenze und offene Verifikation

Die nachfolgenden **MUST**-Punkte sind verbindliche Produktionsanforderungen, keine
Aussage, dass das Content-Modul sie automatisch erfüllt. `ContentManager` nimmt keinen
`ContentSecurityManager` entgegen; die Security-API hat separat konfigurierbare Detektoren
und einige Prüfungen sind standardmäßig deaktiviert. Aus den hier verlinkten Modulquellen
ist außerdem keine allgemeine Startzeit-Validierung aller sicherheitsrelevanten
Deployment-Konfigurationen ableitbar.

- **TODO (blocking):** Für jede tatsächlich genutzte Intake-Route nachweisen, dass
  benötigte Detektoren/Policies verdrahtet sind und Fehler wie vorgeschrieben fail-closed
  behandeln.
- **TODO (blocking):** Startzeit-Validierung, Audit-Logging und Produktionsmodus für das
  jeweilige Deployment anhand der owning service/configuration sources belegen.

Bis diese Nachweise vorliegen, dürfen diese Punkte nicht als durch den Modulcode
implementiert oder erfüllt dargestellt werden.

## Dokumentabgrenzung (Canonical Split)

- **`src/content/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/content/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/content/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/content/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Abuse-Detektor aktiv für alle Ingestion-Pfade; verdächtige Inhalte werden geflaggt.
- **MUST:** Async-Ingestion-Worker mit Queue-Limits konfiguriert; kein unbegrenztes Queue-Wachstum.
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

- `src/content/PRODUCTION_REQUIREMENTS.md`
- `src/content/abuse_detector.cpp`
- `src/content/async_ingestion_worker.cpp`
