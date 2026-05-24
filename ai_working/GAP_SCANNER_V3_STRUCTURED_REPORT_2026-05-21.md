# Gap Scanner v3 Structured Report

Source: `ai_working/gap_scan_v3_aggregate.json`

## By Module

| Module | Total | Critical | High | Medium | Dominant Vulnerability |
|---|---:|---:|---:|---:|---|
| llm | 24,531 | 2,188 | 18,803 | 3,540 | oop_design (7,997) |
| server | 19,261 | 668 | 13,980 | 4,613 | oop_design (5,982) |
| query | 15,519 | 698 | 12,920 | 1,901 | query_correctness (6,909) |
| sharding | 11,078 | 1,076 | 7,770 | 2,232 | oop_design (3,769) |
| index | 8,772 | 367 | 6,485 | 1,920 | oop_design (3,088) |
| storage | 7,494 | 543 | 5,681 | 1,270 | oop_design (2,604) |
| analytics | 7,030 | 273 | 4,712 | 2,045 | oop_design (1,960) |
| rag | 6,742 | 454 | 4,652 | 1,636 | oop_design (1,817) |
| security | 5,048 | 310 | 3,774 | 964 | oop_design (1,518) |
| content | 4,597 | 140 | 3,407 | 1,050 | oop_design (1,478) |
| utils | 4,496 | 195 | 3,239 | 1,062 | oop_design (1,454) |
| acceleration | 4,270 | 236 | 3,011 | 1,023 | oop_design (1,074) |
| ingestion | 3,461 | 178 | 2,462 | 821 | oop_design (1,163) |
| auth | 3,391 | 183 | 2,601 | 607 | oop_design (1,232) |
| network | 3,318 | 241 | 2,497 | 580 | oop_design (1,026) |

## By Vulnerability

| Vulnerability | Total |
|---|---:|
| oop_design | 58,596 |
| uninitialized | 40,003 |
| type_conversion | 15,958 |
| reliability | 14,748 |
| query_correctness | 11,436 |
| determinism | 8,523 |
| input_validation | 8,377 |
| container | 7,741 |
| performance_patterns | 6,798 |
| observability | 5,948 |
| llm_ai_safety | 3,425 |
| memory | 2,234 |
| concurrency | 1,855 |
| raii | 1,847 |
| security | 1,525 |
| exception_safety | 1,494 |
| distributed_consistency | 1,186 |
| platform | 1,147 |
| performance | 1,018 |
| audit_logging | 760 |
| gpu_memory_safety | 229 |
| deprecated_apis | 4 |

## Notes

- The scan is structurally module-driven: each `gap_scan_v3_<module>.json` file contains `total`, severities, `by_category`, and `by_file`.
- The dominant module pattern is OOP design debt, followed by uninitialized-state issues and type-conversion risk.
- The raw aggregate report remains available in `ai_working/gap_scan_v3_aggregate.json` for per-file drill-down.