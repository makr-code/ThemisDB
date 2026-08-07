> **Status:** 2026-06-01 – mit aktuellem Toolbox-Code (`text_normalizer.cpp`) abgeglichen.

# ThemisDB Toolbox Module - Production Requirements

<!-- Status: current | validated: 2026-06-01 | re-verified: 2026-08-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PERFORMANCE_EXPECTATIONS.md · SECURITY.md -->

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Toolbox-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Text-Normalizer, Language-Detector, Toolbox-Utilities.

## Dokumentabgrenzung (Canonical Split)

- **`src/toolbox/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/toolbox/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/toolbox/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/toolbox/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Toolbox-Input-Validierung aktiv; malformed Input wird mit explizitem Fehlercode abgewiesen.
- **MUST:** Language-Detector-Ergebnisse werden mit Konfidenz-Threshold versehen; Ergebnisse unter Threshold werden geflaggt.
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.

## Incident Taxonomy & Observable Diagnostics

Phase 3 implements a unified incident taxonomy across 4 execution planes (Orchestration, Bridge, Registry, Helper).
All incident classes are observable via Prometheus metrics and logging.

Refer to `src/toolbox/SECURITY.md` for the complete **Unified Incident Taxonomy** section, which defines:
- **Layer 1 (Orchestration):** extraction_empty, extraction_failed, extraction_timeout, extraction_overflow (EX-*)
- **Layer 2 (Bridge):** bridge_no_text, bridge_writer_failed, bridge_toolbox_failed, bridge_empty_result (BR-*)
- **Layer 3 (Registry):** registry_not_initialized, registry_double_init, registry_reset_during_active (REG-*)
- **Layer 4 (Helper):** helper_empty_input, helper_encoding_unsupported, helper_size_exceeded, helper_malformed_input (HLP-*)

All incidents are tracked via Prometheus metrics:
- `toolbox_extraction_failures_total` — Layer 1 (Orchestration) errors
- `toolbox_bridge_failures_total` + `toolbox_bridge_latency_us` — Layer 2 (Bridge) errors and latency
- `toolbox_registry_misuse_total` — Layer 3 (Registry) errors
- `toolbox_text_*_errors_total` — Layer 4 (Helper) errors

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

- `src/toolbox/PRODUCTION_REQUIREMENTS.md`
- `src/toolbox/text_normalizer.cpp`
- `src/toolbox/language_detector.cpp`
