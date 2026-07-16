# B-04 gRPC Error-Mapping Plan (2026-06-18)

## Scope
- Implement first production mapping from themis::Error / ErrorCode to grpc::StatusCode in the ThemisDB gRPC service.
- Keep change set minimal and local to `src/api/themisdb_grpc_service.cpp`.

## Affected Files
- `src/api/themisdb_grpc_service.cpp`
- `FUTURE_ENHANCEMENTS.md` (status/checklist update after validation)

## Implementation Steps
1. Add a local helper that maps `themis::Error` to `grpc::Status`.
2. Use the helper in AQL code paths that currently collapse all engine failures to HTTP-style code 500 + grpc OK/INTERNAL.
3. Preserve response payload behavior (`resp->error`) while returning meaningful transport-level gRPC status.

## Acceptance Criteria
- Build target `themis_server` succeeds on `windows-release`.
- Existing core gRPC tests still pass:
  - `GrpcApiServerTests`
  - `ThemisDBGrpcServiceTests`
- No behavioral regression for successful requests.

## Notes
- This is an incremental B-04 step; per-RPC metrics and TLS hardening remain open.
