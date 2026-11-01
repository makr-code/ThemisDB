---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Ingestion in ThemisDB

## Ueberblick
Das Modul bietet konfigurierbare Connectoren, Manager-Orchestrierung,
optionale Workflow-Ausfuehrung und Validierungs-/Quarantaene-Funktionen. Es ist
eine Entwickler-Capability-Uebersicht, keine Zusicherung von Produktionsreife,
Persistenz, globalem Ordering oder garantierter Backpressure. Siehe
[Architektur](./ARCHITECTURE.md), [API](./API.md),
[Implementierung](./IMPLEMENTATION.md) und
[Produktionsanforderungen](./PRODUCTION_REQUIREMENTS.md).

## Kernfaehigkeiten und Grenzen

- `IngestionBuilder` konfiguriert Quellen und baut einen `IngestionManager`;
  dieser liefert `IngestionStats`/`IngestionReport` mit strukturierten
  Ergebnissen. Berichte koennen Teilerfolge enthalten.
- `WorkflowEngine` verarbeitet geordnete Schritte auf einem
  `ExtractionContext`; das Resultat wird nicht automatisch in jedem
  Ausfuehrungspfad persistiert.
- `IngestionCoordinator` bietet Worker-Partitionierung und Report-Aggregation.
  Dauerhafte oder prozessuebergreifende Koordination ist nicht durch die
  in-memory Defaults belegt.
- Sink-Interfaces kapseln Ausgabepfade, muessen aber explizit verdrahtet sein.
- Schema-Feldregeln fuer JSON sind keine universelle Schema-Pruefung fuer alle
  Dokumentformate.

## Quellen und Freigabegrenze

Die aktuelle API ist in `include/ingestion/` definiert; die Implementierungen
und focused tests sind unter `src/ingestion/` und `tests/ingestion/`. Diese
Uebersicht ersetzt keine connector-spezifische Pruefung oder
deploymentbezogene Freigabe. Alte Batch- und Scannerwerte sind historische
Snapshots.

## Einordnung und Quellen
Kanonische Quellen: [`include/ingestion/ingestion_manager.h`](../../include/ingestion/ingestion_manager.h),
[`include/ingestion/workflow_engine.h`](../../include/ingestion/workflow_engine.h),
[`include/ingestion/ingestion_coordinator.h`](../../include/ingestion/ingestion_coordinator.h),
[`include/ingestion/ingestion_sinks.h`](../../include/ingestion/ingestion_sinks.h)
und die korrespondierenden `.cpp`-Dateien. Weiterfuehrend:
[ROADMAP](./ROADMAP.md), [FUTURE_ENHANCEMENTS](./FUTURE_ENHANCEMENTS.md),
[SECURITY](./SECURITY.md).
