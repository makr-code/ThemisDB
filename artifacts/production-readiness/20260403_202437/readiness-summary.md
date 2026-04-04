# Local Production Readiness Summary

- Generated: 2026-04-03T20:31:45.4228320+02:00
- Build preset: msvc-ninja-release
- Repeat count: 20

## Gates

- **openapi-completeness-local**: PASS
  - Details: All source route hints in src/server are documented in openapi/openapi.yaml
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_202437\openapi-completeness.json
- **content-focused-local**: FAIL
  - Details: Skipped by user
- **content-processor-coverage-local**: PASS
  - Details: Processor coverage=92,31% (12/13); threshold=80,00%
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_202437\content_processor_coverage.json
- **content-benchmark-local**: PASS
  - Details: bench_content_versioning count=18 (min=18); bench_text_extraction count=20 (min=20); bench_content_processor_paths count=12 (min=12); VersionCreation1MiB=16,089ms<= 50,000ms; Diff1MiB=17,550ms<= 50,000ms; Retrieval=0,374us<= 5,000us; PDF1MiB=2,414ms<= 500,000ms; DOCX1MiB=4,715ms<= 500,000ms; HTML1MiB=94,373ms<= 500,000ms; Plain1MiB=0,222ms<= 500,000ms; OfficePath1MiB=1,234ms<= 750,000ms; OcrPath1MiB=0,648ms<= 750,000ms; ArchivePath1MiB=0,000ms<= 750,000ms
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_202437\content_bench_processor_paths.json
- **geo-readiness-local**: FAIL
  - Details: bench_geo_cpu_gpu count=25 (min=25); CPU/GPU pairs present: ST_BUFFER=False, exactIntersects=False, geodesicDistance=False
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_202437\geo_cpu_gpu_parity.json
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_202437\beta_modules.txt

## Result

Failed gates: 5
