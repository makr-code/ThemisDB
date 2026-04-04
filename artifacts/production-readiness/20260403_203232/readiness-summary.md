# Local Production Readiness Summary

- Generated: 2026-04-03T20:38:52.2922938+02:00
- Build preset: msvc-ninja-release
- Repeat count: 20

## Gates

- **openapi-completeness-local**: PASS
  - Details: All source route hints in src/server are documented in openapi/openapi.yaml
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_203232\openapi-completeness.json
- **content-focused-local**: FAIL
  - Details: Skipped by user
- **content-processor-coverage-local**: PASS
  - Details: Processor coverage=92,31% (12/13); threshold=80,00%
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_203232\content_processor_coverage.json
- **content-benchmark-local**: PASS
  - Details: bench_content_versioning count=18 (min=18); bench_text_extraction count=20 (min=20); bench_content_processor_paths count=12 (min=12); VersionCreation1MiB=16,310ms<= 50,000ms; Diff1MiB=16,179ms<= 50,000ms; Retrieval=0,311us<= 5,000us; PDF1MiB=2,521ms<= 500,000ms; DOCX1MiB=3,673ms<= 500,000ms; HTML1MiB=75,560ms<= 500,000ms; Plain1MiB=0,202ms<= 500,000ms; OfficePath1MiB=1,143ms<= 750,000ms; OcrPath1MiB=0,684ms<= 750,000ms; ArchivePath1MiB=0,000ms<= 750,000ms
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_203232\content_bench_processor_paths.json
- **geo-readiness-local**: PASS
  - Details: bench_geo_cpu_gpu count=25 (min=25); CPU/GPU pairs present: ST_BUFFER=True, exactIntersects=True, geodesicDistance=True
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_203232\geo_cpu_gpu_parity.json
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_203232\beta_modules.txt

## Result

Failed gates: 4
