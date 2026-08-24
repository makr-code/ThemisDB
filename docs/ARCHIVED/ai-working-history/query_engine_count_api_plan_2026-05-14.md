# QueryEngine Count API Plan (2026-05-14)

## Scope
- Add a production QueryEngine API that returns match count without entity materialization.
- Route benchmark join candidate through this new API instead of direct SecondaryIndexManager access.

## Affected Files
- include/query/query_engine.h
- src/query/query_engine.cpp
- benchmarks/bench_query.cpp

## Acceptance Criteria
- New public method is Doxygen-documented.
- Method uses QueryEngine path and error handling (Result) consistently.
- bench_query builds and new count-path benchmarks run.
- Existing baseline and batched comparison remain available.

## Validation
- Build target: bench_query
- Run benchmark filter with baseline + batched + index-count variants
