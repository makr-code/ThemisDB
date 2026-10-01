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

- Marker gesamt: **172**
- TODO-Marker (actionable backlog candidate): **30**
- STUB/MOCK/FIXME-Marker (separate non-production-path triage): **142**

## Klassifikation

| Klasse | Beschreibung | Anzahl |
|---|---|---:|
| `actionable_todo_candidate` | Verbleibende TODO-Marker nach Reality-Check-Bereinigung | 30 |
| `non_todo_marker` | STUB/MOCK/FIXME-Marker; gesondert gegen Governance/Activation-Regeln triagieren | 142 |

## Modulverteilung (gesamt)

| Modul | Marker |
|---|---:|
| `(root)` | 1 |
| `acceleration` | 9 |
| `analytics` | 7 |
| `api` | 1 |
| `ethics_ai` | 1 |
| `geo` | 3 |
| `governance` | 1 |
| `index` | 2 |
| `ingestion` | 6 |
| `llama_cpp` | 26 |
| `llm` | 14 |
| `performance` | 3 |
| `plugins` | 1 |
| `process` | 1 |
| `rag` | 30 |
| `security` | 46 |
| `server` | 1 |
| `storage` | 5 |
| `tensor` | 4 |
| `training` | 4 |
| `transaction` | 2 |
| `voice` | 4 |

## Verknüpfte Artefakte

- `audit/ACTIONABLE_TODOS_2026-10-01.md` (detaillierte TODO-Reality-Check-Liste)
- `audit/MARKER_LOCATIONS_2026-10-01.md` (vollständige Roh-Fundstellenliste)
