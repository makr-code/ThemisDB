---
Author: ThemisDB Maintainers
Created: 2026-09-30
Last Updated: 2026-09-30
Status: active
---
# Marker-Klassifikation 2026-09-30 (Current Snapshot)

- Quelle: `audit/MARKER_LOCATIONS_2026-09-30.md`
- Methode: Reproduzierbarer Rohscan (`TODO|STUB|MOCK|FIXME`) mit aktuellem Source-Stand auf `develop`.
- Fokus dieser Klassifikation: Aktualitätsabgleich und historische Entkopplung.

## Gesamtbild

- Roh-Marker gesamt: **228**
- `@note Gap Summary`-Leckmuster: **0**
- Historische Referenz (2026-08-31 Baseline): **1730** gesamt / **1473** Doku-Leaks

## Status-Klassifikation (2026-09-30)

| Klasse | Beschreibung | Anzahl |
|---|---|---:|
| `documentation_leak` | Auto-generierte `@note Gap Summary` Header | 0 |
| `implementation_marker` | Reale Marker im Code/Kommentar (TODO/STUB/MOCK/FIXME), weitere fachliche Triage nötig | 228 |
| `historical_only` | Nur in historischen Snapshots, nicht als aktueller Rohstand zu verwenden | n/a |

## Modulverteilung (roh)

| Modul | Marker |
|---|---:|
| `(root)` | 1 |
| `acceleration` | 13 |
| `analytics` | 17 |
| `api` | 1 |
| `cache` | 2 |
| `cdc` | 1 |
| `ethics_ai` | 1 |
| `geo` | 3 |
| `governance` | 1 |
| `gpu` | 1 |
| `graph` | 1 |
| `index` | 2 |
| `ingestion` | 6 |
| `llama_cpp` | 26 |
| `llm` | 19 |
| `onnx_clip` | 2 |
| `performance` | 3 |
| `plugins` | 1 |
| `process` | 1 |
| `rag` | 31 |
| `security` | 49 |
| `server` | 4 |
| `storage` | 10 |
| `tensor` | 20 |
| `training` | 4 |
| `transaction` | 4 |
| `voice` | 4 |

## Hinweis zur Verwendung

- Diese Datei ersetzt nicht die fachliche Priorisierung einzelner Marker.
- Sie liefert den aktuellen, reproduzierbaren Marker-Rohzustand als Grundlage für Follow-up-Triage.
- Historische Dateien (`MARKER_GAP_CLASSIFICATION_2026-08-31.md`, `MARKER_LOCATIONS_2026-08-31.md`) bleiben als Delta-Referenz erhalten, sind aber keine Current-State-Evidenz.
