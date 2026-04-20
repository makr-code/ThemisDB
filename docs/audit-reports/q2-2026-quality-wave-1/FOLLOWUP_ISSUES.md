# Follow-up Issues — Quality Audit Wave 1 (Q2 2026)

## Cluster A — Build/Test Reproducibility (S1)

- [ ] **QA-W1-A1:** Linux bootstrap path für `vcpkg` + `ninja` zuverlässig machen (Target: Q2 2026)
  - Scope: `CMakePresets.json`, bootstrap scripts, CI bootstrap guard
  - Acceptance:
    - `cmake --preset linux-ninja-release` in frischer Umgebung erfolgreich
    - prerequisite check liefert klare Fehlermeldung + auto-fix-Hinweis
    - dokumentierte Repro-Schritte in CI und lokal

## Cluster B — Benchmark Mapping Integrity (S1)

- [ ] **QA-W1-B1:** `CHI-*` benchmark mapping korrigieren (Target: Q2 2026)
  - Scope: `benchmarks/benchmark_target_mapping.json`, chimera benchmark references
  - Acceptance:
    - `python3 tools/verify_benchmark_mapping.py` Exit-Code `0`
    - `CHI-1..CHI-4` referenzieren existierende Dateien + Primärsymbole
    - Regression-test für Mapping-Gültigkeit aktiv

## Cluster C — Performance Gate Completeness (S2)

- [ ] **QA-W1-C1:** Fehlende Voice/GPU workflow hooks nachziehen (Target: Q2 2026)
  - Scope: Performance workflow registry / benchmark CI hooks
  - Acceptance:
    - `tools/perf_expectations_audit.py --strict` ohne Voice/GPU-Warnungen
    - Nightly benchmark sweep workflow nachweisbar
    - orphan-benchmark-source guard in CI aktiv

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

