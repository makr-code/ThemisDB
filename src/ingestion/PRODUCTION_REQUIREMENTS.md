> **Status:** Deployment-Anforderungen fuer Integratoren; nicht als Implementierungsnachweis zu lesen. Quelle zuletzt geprueft: 2026-10-31.

# ThemisDB Ingestion Module - Production Requirements

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team

> Exact model backend not verifiable from final document text alone.

## Zweck und Geltungsbereich

Dieses Dokument ist der kanonische **Deployment-Anforderungskatalog** fuer
Integratoren. Die MUST-Regeln sind keine Behauptung, dass `IngestionManager`
selbst Autorisierung, eine deploymentweite Queue-Grenze, Audit-Logging oder
vollstaendige Fail-Closed-Konfigurationspruefung bereitstellt. Der in-process
Admin-Wrapper bietet keine HTTP-Authentisierung/Autorisierung; die Service- und
Deployment-Schicht muss die entsprechenden Kontrollen implementieren und
nachweisen.

## Dokumentabgrenzung (Canonical Split)

- **`src/ingestion/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/ingestion/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/ingestion/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/ingestion/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST (deployment):** Ingestion-Trigger und Admin-Methoden nur hinter der vom betreibenden Service bereitgestellten Autorisierung exponieren.
- **MUST (deployment):** Quellenlimits, Rate/Timeouts und etwaige Queue-/Backpressure-Grenzen explizit konfigurieren und die konkrete Connector-Unterstuetzung verifizieren. Es gibt keinen hier nachgewiesenen universellen Queue-Limit-Schalter.
- **MUST (deployment):** Sicherheitsrelevante Konfigurationen vor Freigabe validieren; eine repo-weite Durchsetzung von Fail-Closed beim Start ist fuer diesen Katalog nicht nachgewiesen (TODO: konkreten Aufrufer und Startpfad verifizieren).
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen (Deployment-Abnahmekriterien, keine Modul-Funktionszusage)

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging fuer sicherheitsrelevante Operationen ist eine Deployment-Anforderung; ein vollstaendiger Ingestion-Audit-Log ist durch die Modul-API nicht belegt.

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
- [ ] TODO: `THEMIS_PRODUCTION_MODE` / `THEMIS_ENVIRONMENT` ist in den geprueften Ingestion-Quellen nicht als Modul-Konfiguration belegt; nur dokumentieren, wenn die konkrete Deployment-Integration bestaetigt ist.

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/ingestion/PRODUCTION_REQUIREMENTS.md`
- `src/ingestion/ingestion_manager.cpp`
- `src/ingestion/ingestion_coordinator.cpp`
