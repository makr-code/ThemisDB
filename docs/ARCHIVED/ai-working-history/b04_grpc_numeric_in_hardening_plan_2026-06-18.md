# B-04 gRPC Numeric IN Hardening Plan (2026-06-18)

## Scope
- Harden `FilteredVectorSearch` fallback handling for numeric `in` filters.
- Add regression tests for mixed-type and invalid numeric `in` payloads.
- Keep behavior backward-compatible for existing `_key`/`_id` and string attribute filters.

## Acceptance Criteria
- Numeric `in` works for top-level numeric attributes.
- Mixed-type `in` payloads for numeric attributes are rejected at parse-time for that clause and logged as unsupported (no silent numeric coercion from mixed arrays).
- Completely invalid numeric `in` payloads do not crash and are handled via unsupported-filter path.
- Existing focused gRPC tests remain green.

## Files
- `src/api/themisdb_grpc_service.cpp`
- `tests/themisdb/test_themisdb_grpc_service.cpp`
- `FUTURE_ENHANCEMENTS.md` (progress sync)

## Verification
- Build: `cmake --build --preset windows-release --target test_themisdb_grpc_service --parallel 16`
- GTest focused: `build-msvc-windows-release/bin/test_themisdb_grpc_service.exe --gtest_filter=GrpcVectorFetchDocsFallbackTest.*`
- CTest focused: `ctest --preset windows-release --output-on-failure -R "^GrpcApiServerTests$|^ThemisDBGrpcServiceTests$|^TransactionManagerFocusedTests$" -j 1`
