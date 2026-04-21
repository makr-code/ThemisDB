# Follow-up Issues — Quality Audit Wave 1 (Q2 2026)

## Cluster A — Build/Test Reproducibility (S1)

- [ ] **QA-W1-A1:** Linux bootstrap path für `vcpkg` + `ninja` zuverlässig machen (Target: Q2 2026)
  - Scope: `CMakePresets.json`, bootstrap scripts, CI bootstrap guard
  - Acceptance:
    - `cmake --preset linux-ninja-release` in frischer Umgebung erfolgreich
    - prerequisite check liefert klare Fehlermeldung + auto-fix-Hinweis
    - dokumentierte Repro-Schritte in CI und lokal

## Cluster B — Benchmark Mapping Integrity (S1)

- [x] **QA-W1-B1:** `CHI-*` benchmark mapping korrigiert ✅ (2026-04-21)
  - `tools/verify_benchmark_mapping.py`: `_resolve_bench_file()` fallback auf REPO_ROOT ergänzt
  - `benchmarks/benchmark_target_mapping.json`: `primary_benchmark` CHI-1..4 auf workload_id-Bezeichner synchronisiert
  - `python3 tools/verify_benchmark_mapping.py` → Exit `0`, 207 IDs ✅

## Cluster C — Performance Gate Completeness (S2)

- [x] **QA-W1-C1:** Voice/GPU/Nightly workflow hooks aktiviert ✅ (2026-04-21)
  - `.github/workflows/02-feature-modules_llm_voice-benchmark-ci.yml` → perf audit check #4d ✅
  - `.github/workflows/06-infrastructure_gpu_gpu-benchmark-matrix-ci.yml` → perf audit check #5c ✅
  - `.github/workflows/07-quality_nightly-benchmark-sweep.yml` → perf audit check #10a ✅
  - `python3 tools/perf_expectations_audit.py --strict` → 9 Pass / 1 Warn / 0 Fail ✅
  - `.github/WORKFLOW_REGISTRY.md` + `.github/WORKFLOW_GUIDELINES.md` aktualisiert

## Cluster D — Maintainability Debt Reduction (S2)

- [ ] **QA-W1-D1:** Top-10 source-audit Hotspots abbauen (Target: Q3 2026)
  - Scope: höchste Issue-Dichte aus `source_audit.py` (Findings cluster)
  - Acceptance:
    - mindestens 15% Finding-Reduktion in priorisierten Modulen
    - keine neuen S1/S0 Findings in denselben Bereichen
    - je Hotspot Regressionstest oder statischer Guard ergänzt

## Cluster E — Concurrency / Memory Sanitizer Gates (S1)

- [ ] **QA-W1-E1:** TSan + ASan/UBSan/LSan Gate-Läufe für Kern-Targets aktivieren (Target: Q2 2026)
  - Scope: sanitizer-fähiger Preset + CI Gate integration
  - Acceptance:
    - dokumentierte Sanitizer-Commands + Presets
    - 0 neue kritische Sanitizer Findings im Gate-Umfang
    - kritische Paralleltests mit 100 Wiederholungen ohne Flake

## Cluster F — Duplicate-Code Governance (S2)

- [ ] **QA-W1-F1:** Duplicate scan baseline + no-new-clone policy aktivieren (Target: Q2 2026)
  - Scope: `jscpd/CPD` Integration, threshold definitions
  - Acceptance:
    - baseline clone coverage pro Subsystem dokumentiert
    - no-new-high-similarity-cross-module gate aktiv
    - Top-5 Hotspot-Reduktionsplan mit KPI-Tracking

