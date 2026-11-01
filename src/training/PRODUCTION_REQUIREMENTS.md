---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

> **Implementation status (2026-11-01):** This document states production
> requirements; it is not evidence that the module implements them. The
> training module has no post-training adapter-signing and persistence path.
> A separate core `SignedAdapterValidator` compares a SHA-256 digest of adapter
> metadata for registry validation; it does not sign or persist trained adapter
> artifacts or prove signer identity. `QueryEngine` exposes an optional
> collection-access callback, but training does not install or require it, so
> data authorization is not established by this module. Adapter signing,
> fail-closed authorization, and the listed deployment-mode variables remain
> unverified requirements; integration owners must provide evidence before
> claiming compliance.

# ThemisDB Training Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Training-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Training-Pipeline, LoRA-Adapter, Training-Lifecycle.

## Dokumentabgrenzung (Canonical Split)

- **`src/training/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/training/README.md`:** Funktionsübersicht, Architekturkontext, Build- und Testhinweise.
- **`src/training/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/training/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Training-Pipeline mit Datenzugriffs-Autorisierung; kein Zugriff auf Trainingsdaten ohne Autorisierung.
- **MUST:** LoRA-Adapter-Training nur für autorisierte Modell-Pfade; Adapter-Signatur nach Training erzeugt und persistiert.
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.

## Source Verification of Signing and Authorization

- [`SignedAdapterValidator`](../../include/core/concerns/adapter_signing.h)
  validates a `sha256` digest of the
  canonical adapter metadata (`id:apiVersion:description`) before adapter
  registry registration. This is digest comparison, not private-key signing,
  and it is not wired into the training completion/checkpoint persistence path
  ([trainer](./incremental_lora_trainer.cpp)).
- [`QueryEngine::setCollectionAccessChecker()`](../../include/query/query_engine.h)
  accepts a caller-provided collection/caller check. Query execution checks in
  [`query_engine.cpp`](../query/query_engine.cpp) are conditional on that
  callback being configured; the training module neither installs the callback
  nor rejects an unconfigured `QueryEngine`.
- Therefore, the production requirements below remain acceptance requirements,
  not verified controls. The deployment owner must provide integration evidence
  that every training read is authorized and that trained adapter artifacts
  receive a persisted, verifiable signature before these requirements can be
  marked complete.

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

- `src/training/PRODUCTION_REQUIREMENTS.md`
- `src/training/training_pipeline.cpp`
- `src/training/lora_adapter.cpp`
