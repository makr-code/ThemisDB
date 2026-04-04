# Local Production Readiness Summary

- Generated: 2026-04-03T20:53:04.3644007+02:00
- Build preset: msvc-ninja-release
- Repeat count: 20

## Gates

- **openapi-completeness-local**: PASS
  - Details: All source route hints in src/server are documented in openapi/openapi.yaml
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_205301\openapi-completeness.json
- **content-focused-local**: FAIL
  - Details: Skipped by user
- **content-processor-coverage-local**: FAIL
  - Details: Skipped by user
- **content-benchmark-local**: FAIL
  - Details: Skipped by user
- **geo-readiness-local**: FAIL
  - Details: Skipped by user
- **process-readiness-local**: PASS
  - Details: bench_process_retrieval count=18 (min=12); embedding_field=present; findSimilar_api=present; search_api=present; BM_ProcessEmbeddingGenerate=True; BM_ProcessHnswRetrieve=True; BM_ProcessFullTextSearch=True; BM_ProcessStateChangeEmbed=True; security_items_tracked=True; embedding_audit_tracked=True
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_205301\process_retrieval_bench.json
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_205301\beta_modules.txt

## Result

Failed gates: 7
