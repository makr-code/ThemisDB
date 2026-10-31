> **Status:** 2026-10-31 – mit aktuellem Auth-Code (`api_key_authenticator.cpp`) abgeglichen.

# ThemisDB Auth Module - Production Requirements

- **AI_Used:** true
- **AI_Models:** unknown
- **AI_Contributor:** GitHub Copilot
- **Human_Review:** false
- **Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Auth-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für API-Key-Authenticator, Auth-Rate-Limiter, Federated-Identity.

Die folgenden MUST-Anforderungen sind normative Deployment-Anforderungen,
keine Aussage, dass der Auth-Code sie automatisch erzwingt. Die Implementierung
und Einbindung müssen pro Deployment geprüft werden.

## Dokumentabgrenzung (Canonical Split)

- **`src/auth/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/auth/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/auth/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/auth/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.
- **`src/auth/RUNBOOK.md`:** Operator-Runbook für kritische Betriebsszenarien (Backend-Ausfall, Leader-Election-Stall, Provider-Timeout, Session-Overflow, Redis-Ausfall).

## Verbindliche Produktionsanforderungen

- **MUST:** Auth-Rate-Limiter aktiv; Brute-Force-Schutz konfiguriert.
- **MUST:** Federated-Identity-Konfiguration vor Produktionsstart validiert.
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
- [ ] Produktionsmodus gemäß dem Deployment-Vertrag gesetzt; ein Auth-Modul-Umgebungsvariablenname ist in den geprüften Auth-Quellen nicht belegt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/auth/PRODUCTION_REQUIREMENTS.md`
- `src/auth/api_key_authenticator.cpp`
- `src/auth/auth_rate_limiter.cpp`
