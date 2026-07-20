> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

# FUTURE_ENHANCEMENTS

## tools

### Scope
- Scanner-Tuning für Gap-Qualität mit eingefrorener Baseline (TP 24%, CRITICAL-TP 50%).
- Reproduzierbare Pipeline `scan -> sample -> validation -> remediation backlog`.
- Delivery-Wellen mit harten Qualitätsgates und evidenzbasiertem Reporting.

### Design Constraints
- HIGH/MEDIUM Findings nur bei mindestens zwei unabhängigen Signalen priorisieren.
- High-FP-Kategorien (`observability`, `copy_overhead`, `db_connection_leak`, `no_health_check`, `hardcoded_path`) standardmäßig aus unmittelbarem Remediation-Backlog ausklammern.
- Prioritätskategorien (`legacy_duplication`, `smart_ptr_misuse`, `memory_order`, `uncaught_exception`) als CRITICAL/High-Confidence bevorzugt behandeln.
- Sampling muss deterministisch sein (Seed explizit und in Metadaten dokumentiert).

### Required Interfaces
- `/home/runner/work/ThemisDB/ThemisDB/tools/gap_scanner_v3.py` (Orchestrierung + Tuning-Policy + KPI/Queue-Artefakte)
- `/home/runner/work/ThemisDB/ThemisDB/tools/generate_validation_sample.py` (stratifiziertes, reproduzierbares Sampling)
- `/home/runner/work/ThemisDB/ThemisDB/tools/analyze_validation_sample.py` (TP/FP-Analyse + validierter Backlog)
- Ausgaben unter `/home/runner/work/ThemisDB/ThemisDB/ai_working/` für Governance und Tracking.

### Implementation Notes
- Scanner-Orchestrierung muss `tools/` und `tools/legacy/` robust importieren können.
- Policy-Artefakte erzeugen: Baseline-Freeze, Preflight-Queue, Weekly-KPI, Batch-Gates.
- Backlog-Ausgaben müssen Modulpriorität `llm -> server -> sharding` explizit enthalten.
- Abschluss je Batch mit Status `erledigt/offen/risiko`.

### Test Strategy
- Script-Validierung via `python -m py_compile` für geänderte Scanner-/Analyse-Skripte.
- Pipeline-Validierung: Vollscan, danach 50er-Stratifizierung und Analyse-Report auf gleicher Datenbasis.
- Regression: Keine Verschlechterung der CRITICAL-Präzision gegenüber Baseline.
- Gate-Validierung: `release_critical` plus Wave-5/6-Schutztests als Pflichtkriterium zwischen Batches.

### Performance Targets
- Gesamt-TP-Rate nach Tuning >= 50%.
- CRITICAL-TP-Rate mindestens auf Baseline halten (>= 50%).
- FP-Anteil in High-FP-Kategorien deutlich reduzieren (qualitatives Ziel: klarer Rückgang ggü. Baseline).
- Preflight-Queue muss ausschließlich CRITICAL/HIGH actionable fokussieren.

### Security / Reliability
- Keine neuen CRITICAL Findings pro Batch zulassen.
- Für jede Remediation-Welle müssen Fehler-/Recovery-Semantik dokumentiert sein.
- Wave-7-Baseline stabil halten; Wave-8/Fault-Injection als nachgelagerter Sign-off.
- Governance-Sync mit Root-Dokumenten (`ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, Release-Artefakte) fortlaufend sicherstellen.
