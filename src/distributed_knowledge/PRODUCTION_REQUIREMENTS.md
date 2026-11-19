---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

> **Status:** Review 2026-11-19 – Anforderungen und tatsächlich implementierte Modulgrenzen sind getrennt ausgewiesen.

# ThemisDB Distributed_knowledge Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Distributed_knowledge-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Federated-RAG-Merger, Cross-Shard-Feedback-Sync, Federated-Distillation-Coordinator.

## Dokumentabgrenzung (Canonical Split)

- **`src/distributed_knowledge/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/distributed_knowledge/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/distributed_knowledge/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/distributed_knowledge/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Federated-RAG-Merge-Ergebnisse mit Autorisierungs-Check vor Auslieferung.
- **MUST:** Cross-Shard-Feedback-Sync mit expliziten Shard-Autorisierungen; kein unauthentifizierter Cross-Shard-Zugriff.
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden explizit signalisiert; die aktuellen C++ APIs verwenden je nach Pfad Exceptions, Rückgabewerte oder lokale Zähler und bieten keinen einheitlichen Fehlercode für jeden Vorgang.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.

## Implementierungsgrenze und Freigabevorbehalt

Die Anforderungen in diesem Dokument sind Integrations- und Release-Gates; sie
werden nicht alle durch `distributed_knowledge` selbst erzwungen. Der aktuelle
Modulcode implementiert weder Graph-Synchronisierung noch Peer-Transport,
Transport-Authentifizierung, dauerhafte Zustände, automatische Retries oder
Restart-Recovery. `FederatedRAGMerger` verarbeitet bereits vom Aufrufer
gelieferte Ergebnisse und führt selbst keine Autorisierung der Remote-Abfrage
durch. `GossipAdapterPublisher` verlangt für eingehende Capability-Meldungen
eine konfigurierte Trust-Policy; `AlwaysPermitTrustPolicy` ist kein
Produktions-Autorisierungsmechanismus. Die Integrationsschicht muss nicht
erfüllte Gates fail-closed behandeln und darf diese Modul-APIs nicht als
Nachweis einer Ende-zu-Ende-Sicherheits- oder Konsistenzgarantie ausgeben.

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

- `src/distributed_knowledge/PRODUCTION_REQUIREMENTS.md`
- `src/distributed_knowledge/federated_rag_merger.cpp`
- `src/distributed_knowledge/cross_shard_feedback_sync.cpp`
