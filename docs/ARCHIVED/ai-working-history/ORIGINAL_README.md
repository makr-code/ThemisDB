# ai_working

Geschützter Entwurfsraum für iterative KI-Artefakte.

## Regeln

- Kein produktiver Code als finale Quelle in diesem Verzeichnis
- Erlaubt: Zwischenstände, Analyse-Notizen, temporäre Entwürfe
- Vor Merge müssen relevante Ergebnisse in `src/`, `include/` oder `docs/` überführt werden
- Temporäre Dateien müssen dem Ignore-Muster folgen (`*.tmp.*`, `debug_*`)

## Erwarteter Workflow

1. Entwurf in `ai_working/` erstellen
2. Validieren und fachlich reviewen
3. Reife Ergebnisse in Zielpfade migrieren
4. Veraltete Entwurfsartefakte entfernen

## Installation

Keine Installation erforderlich; dieses Verzeichnis ist Teil des Repository-Inhalts.

## Usage

Dateien hier als Referenz lesen und bei inhaltlichen Änderungen im selben PR mit den betroffenen Code-/Dokumentationsänderungen synchron halten.

## Aktueller Fokus (Code-Maturity/GAP Framework)

- Konsolidierungsplan: `ai_working/CODE_MATURITY_FRAMEWORK_REALIGNMENT_PLAN_2026-05-20.md`
- Priorisierte TODO: `ai_working/CODE_MATURITY_FRAMEWORK_TODO_2026-05-20.md`
- Aktueller Ausfuehrungsplan nach CTest-Registry-Stabilisierung: `ai_working/MODULE_GAPS_EXECUTION_PLAN_2026-06-08.md`

## Aktuelle Scanner-Referenz (kanonisch)

- Current-Management-Report: `ai_working/GAP_SCANNER_CURRENT_REPORT_2026-06-09.md`
- Kanonische Scan-Basis: `ai_working/gap_scan_results.json`
- Scope-Aufbereitung (Themis-core/tests/bench vs third-party): `ai_working/gap_scope_breakdown_20260604.md`
- Legacy-Archiv alter v3-Reports: `ai_working/archive/gap_scanner_legacy_2026-06-09/`

## Hierher verschobene AI-Artefakte (2026-05-26)

Folgende Dateien wurden aus dem Root-Verzeichnis und `docs/` hierher verschoben,
da sie keine Produktdokumentation, sondern KI-Agent-Arbeitsartefakte sind:

### Phase-3-Ollama-Workflow
- `PHASE3_ROADMAP.md` — Ollama-basierte Code-Generierungs-Roadmap
- `PHASE3_CLEANUP_GUIDE.md` — Speicher-Cleanup für Ollama-Modelle
- `PHASE3_OLLAMA_SETUP.md` — Ollama-Einrichtungsanleitung für lokale KI-Codegenerierung
- `PHASE3_VALIDATION_CRITERIA.md` — PoC-Validierungs-Entscheidungsmatrix
- `PHASE3_VALIDATION_NEXT_STEPS.md` — Nächste Schritte nach Phase-3-Validierung

### Wave-A / ML-Implementierungsberichte
- `WAVE_A_PHASE2_REPORT.md` — Wave-A-ML-Implementierungsbericht (Phase 2)
- `WAVE_A_REMAINING_WORK.md` — Offene Punkte Wave A (Speculative Decoding, DPR, etc.)

### Workflow-Berichte
- `WORKFLOW_ACTIVATION_REPORT.md` — Gap-Scan + Issue-Erstellungs-Workflow
- `WORKFLOW_COMPLETION_REPORT.md` — Abschlussbericht des 7-Phasen-Workflows

### Temporäre Blueprints und Tracker
- `tmp_ai_blueprint_4931.md`, `tmp_acceleration_blueprint.md`, `tmp_analytics_blueprint.md`,
  `tmp_api_blueprint.md`, `tmp_scraper_blueprint.md`, `tmp_tensor_blueprint.md`
- `tmp_review_audit_block.md`, `tmp_stub_missing.txt`, `tmp_created_doc_issues.json`
- `tmp_tracker_new_modules.md`, `tmp_tracking_priorities_comment.md`, `tmp_tracking_stub_priorities.md`

### Sonstige
- `TODO_TENSOR.md` — Tensor-spezifisches Stub-Backlog
- `AI_DECISION_AUDITING_IMPLEMENTATION_SUMMARY.md` — Implementierungszusammenfassung (ex docs/)

## Hierher verschobene Root-Artefakte (2026-07-28)

Die folgenden vormals unreferenzierten Root-Markdown-Dateien wurden aus dem Root erst archiviert
und anschliessend nach `ai_working/root-md-relocated/2026-07-28/` umgezogen, da sie
inhaltlich als Arbeits-/Zwischenstandsartefakte besser in `ai_working/` verortet sind.

- `CLEAN_ROOT_FILE_INVENTORY.md`
- `CPU_GPU_OFFLOAD_DECISION_MATRIX.md`
- `CTEST_TODO.md`
- `EPIC3.5_RECOVERY_STRATEGY_SUMMARY.md`
- `ISSUE_5382_COMPLETION_REPORT.md`
- `ISSUE_5635_CLOSURE_SUMMARY.md`
- `LLM_MODULE_PHASE3_FINDINGS.md`
- `P2_D02_CUDA_IMPLEMENTATION_REPORT.md`
- `P2_D02_IMPLEMENTATION_REPORT.md`
- `P2_D02_QUICK_REFERENCE.md`
- `PHASE1_ONNX_INTEGRATION_SUMMARY.md`
- `PHASE3_FIELD_DIAGNOSTICS_COMPLETION_REPORT.md`
- `Q3_2026_BATCH_2_VERIFICATION_REPORT.md`
- `SPRINT8_PHASE1C_COMPLETION.md`
- `SPRINT8_WAVE1_SUMMARY.md`
- `SPRINT_7_BATCH_C_PHASE_2_SUMMARY.md`
- `TENSOR_ARTIFACT_HARDWARE_ASSESSMENT.md`
