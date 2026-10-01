---
Author: ThemisDB Maintainers
Created: 2026-10-01
Last Updated: 2026-10-01
Status: active
---
# Marker-Klassifikation 2026-10-01 (Reality Check)

- Quelle: `audit/MARKER_LOCATIONS_2026-10-01.md`
- Methode: Reproduzierbarer Marker-Scan gegen aktuellen `src/`-Stand auf `develop`.

## Gesamtbild

- Marker gesamt: **218**
- TODO-Marker (actionable backlog candidate): **30**
- STUB/MOCK/FIXME-Marker (separate non-production-path triage): **188**

## Klassifikation

| Klasse | Beschreibung | Anzahl |
|---|---|---:|
| `actionable_todo_candidate` | Verbleibende TODO-Marker nach Reality-Check-Bereinigung | 30 |
| `non_todo_marker` | STUB/MOCK/FIXME-Marker; gesondert gegen Governance/Activation-Regeln triagieren | 188 |

## Modulverteilung (gesamt)

| Modul | Marker |
|---|---:|
| `(root)` | 1 |
| `acceleration` | 13 |
| `analytics` | 8 |
| `api` | 1 |
| `cache` | 2 |
| `cdc` | 1 |
| `ethics_ai` | 1 |
| `geo` | 3 |
| `governance` | 1 |
| `gpu` | 1 |
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

## Verknüpfte Artefakte

- `audit/ACTIONABLE_TODOS_2026-10-01.md` (detaillierte TODO-Reality-Check-Liste)
- `audit/MARKER_LOCATIONS_2026-10-01.md` (vollständige Roh-Fundstellenliste)
