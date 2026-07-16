# Gap Scanner Current Report (2026-06-09)

## Status

Dieser Report ersetzt den alten v3-Structured-Report als aktuelle Management-Sicht.

## Gueltige Datenbasis

- Kanonische Scan-Basis: ai_working/gap_scan_results.json (LastWriteTime: 2026-06-04 12:19)
- Scope-Auswertung: ai_working/gap_scope_breakdown_20260604.md
- Repro-Lauf (juenger, aber nicht als Hauptreferenz gesetzt): ai_working/gap_scan_results.repro_20260604_122540.json

## Gesamtbild (aus gap_scan_results.json + Scope-Breakdown)

- Total gaps: 253903
- themis_core: 38582 (15.2%)
- themis_tests: 6808 (2.68%)
- themis_benchmarks: 1079 (0.42%)
- third_party: 207434 (81.7%)

## themis_core: Schwerpunkte

Top Types:
- missing_doxygen_comment: 3987
- missing_doxygen_return: 3275
- docs_broken_markdown_link: 2766
- missing_doxygen_param: 2643
- missing_doxygen_brief: 2327
- resource_leaked_in_exception: 1394
- no_key_rotation: 1193
- string_concat_loop: 963
- data_race: 946
- hardcoded_output: 905

Top Scanner:
- Uniform::themis_cpp_doxygen_policy_rules: 12232
- Uniform::themis_docs_markdown_rules: 2789
- Uniform::container: 2197
- Uniform::exception_safety: 1526
- Uniform::key_failure: 1362
- Uniform::performance_patterns: 1272
- Uniform::raii: 1251
- Uniform::e2e_encryption: 1182
- Uniform::phase1_error_handling: 1105
- Uniform::concurrency: 1079

## Priorisierte Quickwin-Richtung

1. Core-first: technische Critical/High-Themen vor reiner Doku-Masse priorisieren.
2. Parallel dokumentarische Last abbauen (Doxygen + broken links), da hoher Anteil.
3. Third-party strikt getrennt berichten, damit ThemisDB-interne Fortschritte sichtbar bleiben.

## Archivierung alter V3-Artefakte (durchgefuehrt 2026-06-09)

Archivpfad:
- ai_working/archive/gap_scanner_legacy_2026-06-09/

Archivierte Dateien:
- GAP_SCANNER_V3_STRUCTURED_REPORT_2026-05-21.md
- gap_scan_v3_aggregate.json
- gap_scan_v3_summary.json
- gap_scan_v3_summary_before_fp_tuning.json
