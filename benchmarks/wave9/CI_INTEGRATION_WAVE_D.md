# Wave D CI Integration for Benchmarks

## Overview
This document describes the CI integration strategy for Wave D observability and soak test gates.

### Workflow Structure

#### 1. Wave D Observability Gates (Phase 2: Optional in PR, Required for Release)

**Trigger:** `release_critical` CI label + manual workflow dispatch  
**Frequency:** Per-commit for `release_critical` PRs, nightly for `develop` branch  

Workflow: `.github/workflows/wave-d-observability-gates.yml`

```yaml
name: Wave D — Observability Gates

on:
  workflow_dispatch:
    inputs:
      observability_only:
        description: 'Run observability gates only'
        type: boolean
        default: true
  schedule:
    - cron: '30 2 * * *'  # Nightly 02:30 UTC (after full benchmark sweep)

jobs:
  observability-gates:
    runs-on: ubuntu-22.04
    timeout-minutes: 45
    steps:
      - uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683
      
      - name: Build observability tests
        run: |
          cmake --preset linux-release \
            -DTHEMIS_BUILD_BENCHMARKS=ON \
            -DTHEMIS_WAVE_D_OBSERVABILITY=ON
          cmake --build build --target \
            test_otel_trace_overhead_measurement \
            test_high_cardinality_metrics_explosion \
            test_otel_exporter_stress
      
      - name: Run W4A-TRACE-01 (trace overhead)
        run: |
          ./build/benchmarks/test_otel_trace_overhead_measurement \
            --benchmark_format=json \
            --benchmark_out=results/trace_overhead.json
      
      - name: Run W4A-METRICS-01 (cardinality explosion)
        run: |
          ./build/benchmarks/test_high_cardinality_metrics_explosion \
            --benchmark_format=json \
            --benchmark_out=results/metrics_cardinality.json
      
      - name: Run W4A-EXPORTER-01..05 (exporter stress)
        run: |
          ./build/benchmarks/test_otel_exporter_stress \
            --benchmark_format=json \
            --benchmark_out=results/exporter_stress.json
      
      - name: Verify gates
        run: |
          python3 benchmarks/scripts/verify_wave_d_gates.py \
            --manifest benchmarks/wave9/release_gate_manifest_wave_d.json \
            --results results/ \
            --phase observability
      
      - uses: actions/upload-artifact@65c4c4a1ddee5b72f698fdd19549f0f0fb45cf08
        if: always()
        with:
          name: wave-d-observability-results-${{ github.run_number }}
          path: results/
```

#### 2. Wave D Soak Test Gates (Phase 4: Nightly Only, 24-48 Hour Runs)

**Trigger:** Nightly schedule (Sunday 04:00 UTC) + manual workflow dispatch  
**Frequency:** Weekly for production validation  

Workflow: `.github/workflows/wave-d-soak-tests.yml`

```yaml
name: Wave D — Soak Test Gates (24-48h)

on:
  workflow_dispatch:
    inputs:
      override_duration_hours:
        description: 'Override soak test duration (for CI smoke: 2)'
        type: number
        default: 2
  schedule:
    - cron: '0 4 * * 0'  # Weekly Sunday 04:00 UTC

concurrency:
  group: wave-d-soak-${{ github.ref }}
  cancel-in-progress: false  # Let soak tests complete

jobs:
  soak-telemetry-24h:
    runs-on: ubuntu-22.04
    timeout-minutes: 1440  # 24 hours
    steps:
      - uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683
      - run: |
          cmake --preset linux-release \
            -DTHEMIS_BUILD_BENCHMARKS=ON \
            -DTHEMIS_ENABLE_SOAK_TESTS=ON
          cmake --build build --target test_wave9_otel_24h_soak
      - name: Run 24h telemetry soak
        env:
          SOAK_DURATION_HOURS: ${{ github.event.inputs.override_duration_hours || 24 }}
        run: |
          mkdir -p soak_results
          ./build/benchmarks/wave9/test_wave9_otel_24h_soak \
            --benchmark_min_time=0 \
            --benchmark_out=soak_results/telemetry_24h.json
      - uses: actions/upload-artifact@65c4c4a1ddee5b72f698fdd19549f0f0fb45cf08
        if: always()
        with:
          name: soak-telemetry-24h-${{ github.run_number }}
          path: soak_results/

  soak-replication-48h:
    runs-on: ubuntu-22.04
    timeout-minutes: 2880  # 48 hours
    steps:
      - uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683
      - run: |
          cmake --preset linux-release \
            -DTHEMIS_BUILD_BENCHMARKS=ON \
            -DTHEMIS_ENABLE_SOAK_TESTS=ON
          cmake --build build --target test_wave9_replication_48h_soak
      - name: Run 48h replication soak
        env:
          SOAK_DURATION_HOURS: ${{ github.event.inputs.override_duration_hours || 48 }}
        run: |
          mkdir -p soak_results
          ./build/benchmarks/wave9/test_wave9_replication_48h_soak \
            --benchmark_min_time=0 \
            --benchmark_out=soak_results/replication_48h.json
      - uses: actions/upload-artifact@65c4c4a1ddee5b72f698fdd19549f0f0fb45cf08
        if: always()
        with:
          name: soak-replication-48h-${{ github.run_number }}
          path: soak_results/

  soak-sharding-48h:
    runs-on: ubuntu-22.04
    timeout-minutes: 2880  # 48 hours
    steps:
      - uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683
      - run: |
          cmake --preset linux-release \
            -DTHEMIS_BUILD_BENCHMARKS=ON \
            -DTHEMIS_ENABLE_SOAK_TESTS=ON
          cmake --build build --target test_wave9_sharding_topology_soak
      - name: Run 48h sharding soak
        env:
          SOAK_DURATION_HOURS: ${{ github.event.inputs.override_duration_hours || 48 }}
        run: |
          mkdir -p soak_results
          ./build/benchmarks/wave9/test_wave9_sharding_topology_soak \
            --benchmark_min_time=0 \
            --benchmark_out=soak_results/sharding_48h.json
      - uses: actions/upload-artifact@65c4c4a1ddee5b72f698fdd19549f0f0fb45cf08
        if: always()
        with:
          name: soak-sharding-48h-${{ github.run_number }}
          path: soak_results/

  soak-acceleration-24h:
    runs-on: ubuntu-22.04
    timeout-minutes: 1440  # 24 hours
    steps:
      - uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683
      - run: |
          cmake --preset linux-release \
            -DTHEMIS_BUILD_BENCHMARKS=ON \
            -DTHEMIS_ENABLE_SOAK_TESTS=ON
          cmake --build build --target test_wave9_mixed_acceleration_soak
      - name: Run 24h acceleration soak
        env:
          SOAK_DURATION_HOURS: ${{ github.event.inputs.override_duration_hours || 24 }}
        run: |
          mkdir -p soak_results
          ./build/benchmarks/wave9/test_wave9_mixed_acceleration_soak \
            --benchmark_min_time=0 \
            --benchmark_out=soak_results/acceleration_24h.json
      - uses: actions/upload-artifact@65c4c4a1ddee5b72f698fdd19549f0f0fb45cf08
        if: always()
        with:
          name: soak-acceleration-24h-${{ github.run_number }}
          path: soak_results/

  gate-verification:
    runs-on: ubuntu-22.04
    needs:
      - soak-telemetry-24h
      - soak-replication-48h
      - soak-sharding-48h
      - soak-acceleration-24h
    if: always()
    steps:
      - uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683
      - name: Download all soak results
        uses: actions/download-artifact@65c4c4a1ddee5b72f698fdd19549f0f0fb45cf08
        with:
          path: all_soak_results/
      - name: Verify all Wave D gates
        run: |
          python3 benchmarks/scripts/verify_wave_d_gates.py \
            --manifest benchmarks/wave9/release_gate_manifest_wave_d.json \
            --results all_soak_results/ \
            --phase soak_tests \
            --output gate_verification_report.json
      - name: Generate summary report
        run: |
          echo "## Wave D Soak Test Results" >> $GITHUB_STEP_SUMMARY
          python3 benchmarks/scripts/summarize_wave_d_results.py \
            --report gate_verification_report.json >> $GITHUB_STEP_SUMMARY
      - uses: actions/upload-artifact@65c4c4a1ddee5b72f698fdd19549f0f0fb45cf08
        if: always()
        with:
          name: wave-d-gate-verification-${{ github.run_number }}
          path: gate_verification_report.json
```

### CMakeLists Integration

#### benchmarks/wave9/CMakeLists.txt

```cmake
# Wave D Observability Tests (Phase 2A-2C)
add_executable(test_otel_trace_overhead_measurement
  test_otel_trace_overhead.cpp
)
target_link_libraries(test_otel_trace_overhead_measurement
  PRIVATE benchmark::benchmark observability_sdk
)
add_test(NAME test_otel_trace_overhead COMMAND test_otel_trace_overhead_measurement)

# Wave D Soak Tests (Phase 4)
if(THEMIS_ENABLE_SOAK_TESTS)
  add_executable(test_wave9_otel_24h_soak test_wave9_otel_24h_soak.cpp)
  target_link_libraries(test_wave9_otel_24h_soak
    PRIVATE benchmark::benchmark observability_sdk
  )
  
  add_executable(test_wave9_replication_48h_soak test_wave9_replication_48h_soak.cpp)
  target_link_libraries(test_wave9_replication_48h_soak
    PRIVATE benchmark::benchmark replication_sdk
  )
  
  add_executable(test_wave9_sharding_topology_soak test_wave9_sharding_topology_soak.cpp)
  target_link_libraries(test_wave9_sharding_topology_soak
    PRIVATE benchmark::benchmark sharding_sdk
  )
  
  add_executable(test_wave9_mixed_acceleration_soak test_wave9_mixed_acceleration_soak.cpp)
  target_link_libraries(test_wave9_mixed_acceleration_soak
    PRIVATE benchmark::benchmark gpu_sdk compute_sdk
  )
endif()
```

### Release Criteria Gate

Wave D gates are enforced in the release promotion pipeline:

- **PR gates (optional):** Observability tests (W4A-*) run in nightly but don't block PRs
- **Release gates (required):** All Wave D gates must PASS before v2.5.0-rc1 tag
- **Promotion gating:** Evidence from soak tests must be reviewed and approved in `docs/governance/GA_PROMOTION_SIGN_OFF.md`

