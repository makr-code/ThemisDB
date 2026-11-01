---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "themisdb-doc-orchestrator"
Human_Review: false
Human_Author: "unknown"
---

> **Validation note (2026-10-31):** The requirements below are deployment
> policy, not proof that every build/deployment enforces them. The current
> `ModuleSecurityVerifier` selects signature-required/unsigned-denied defaults
> under `NDEBUG` and unsigned-allowed defaults otherwise; deployments must
> verify their build mode and configured policy.

# ThemisDB Themis Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Themis-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Module-Signature-Verifier, Module-Dependency-Resolver, ThemisDB-Core-Orchestration.

## Dokumentabgrenzung (Canonical Split)

- **`src/themis/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/themis/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/themis/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/themis/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Module-Signature-Verifier aktiv; alle geladenen Module werden mit Signatur validiert bevor Aktivierung.
- **MUST:** Module-Dependency-Resolver prüft Kompatibilitäts-Matrix; inkompatible Module-Kombinationen werden abgewiesen.
- **MUST:** Alle für die jeweilige Produktionskonfiguration sicherheitsrelevanten Werte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten. Die konkrete Konfigurationsliste ist deployment-spezifisch und muss belegt werden.
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
- [ ] Produktions-Buildmodus und Signatur-/Unsigned-Policy anhand der tatsächlich verwendeten Build- und Laufzeitkonfiguration nachgewiesen. Für `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` wurde in den geprüften Quellen keine Definition gefunden; diese Variablennamen sind daher keine belegte Modulkonfiguration.

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/themis/PRODUCTION_REQUIREMENTS.md`
- `src/themis/module_signature_verifier.cpp`
- `src/themis/module_dependency_resolver.cpp`
