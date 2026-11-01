---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - ../../include/sharding/sharding_api_contract.h
  - ../../include/sharding/auto_rebalancer.h
  - ../../include/sharding/gpu_erasure_coder.h
  - ../../src/sharding/auto_rebalancer.cpp
Related_Issue: "#7381"
Related_Milestone: DOC-WEEKLY-2026-W44
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Sharding Module - Production Requirements

> **Verification status:** The prior 2026-06-01 code-alignment statement is stale. This page is a
> proposed operator checklist, not proof that the listed controls are enforced by the sharding
> implementation or a project-wide production policy. **TODO:** obtain maintainer/security review
> and verify each proposed control against the selected deployment components before treating it
> as normative.

## Zweck und Geltungsbereich

Dieses Dokument sammelt vorgeschlagene Betriebsanforderungen für ausgewählte Sharding-Komponenten.
Es ist nicht die kanonische Projekt-Sicherheits- oder Release-Policy; maßgeblich sind die
konkreten Komponentenverträge und die projektweiten Governance-/Security-Dokumente.

## Dokumentabgrenzung (Canonical Split)

- **`src/sharding/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/sharding/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/sharding/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/sharding/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Vorgeschlagene Produktionskontrollen (noch zu bestätigen)

- **Vorschlag:** Rebalancing in produktiven Deployments nur gemäß dokumentierter Operator- und
  Deployment-Policy ausführen. **TODO:** Der konkrete Approval-Mechanismus ist hier nicht verifiziert.
- **Vorschlag:** GPU-Erasure-Coding nur für explizit unterstützte und konfigurierte Backends
  aktivieren. **TODO:** Plattform-/Build-Kombinationen und OpenCL-Verhalten verifizieren.
- **Vorschlag:** sicherheitsrelevante Konfiguration beim Start validieren und Fehler sichtbar
  behandeln. **TODO:** pro Komponente prüfen, welche Werte tatsächlich validiert werden.
- **Vorschlag:** Sicherheits- und Autorisierungsprüfungen in Produktionspfaden nicht abschalten;
  konkretes Enforcement ist nicht durch diese Datei belegt.

## Prüfpunkte für Sicherheitsreview

- Für jede administrative oder sicherheitsrelevante Operation den tatsächlichen Kontrollpfad,
  Fehlercode und Berechtigungscheck am entsprechenden Header/Implementierung prüfen.
- Audit-Logging und deployment-seitige Konfiguration sind **TODO**-Prüfpunkte; ihre Aktivierung
  wird durch diese Modulübersicht nicht garantiert.

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

## Historischer Review-Verweis

### In der älteren Notiz genannte Dateien

- `src/sharding/PRODUCTION_REQUIREMENTS.md`
- `src/sharding/paxos_snapshot.cpp`
- `src/sharding/gpu_erasure_coder_opencl.cpp`

Diese Liste belegt keine aktuelle Prüfung oder Laufzeitvalidierung. Der frühere Audit-Vermerk vom
2026-06-01 ist durch den statischen Review vom 2026-10-31 nicht bestätigt worden.
