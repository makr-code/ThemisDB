# Server benchmark module regrouping plan

Scope: move the next server/protocol benchmark targets out of the root `benchmarks/` directory into a dedicated module-local folder.

Files:
- move: `bench_api_endpoints.cpp` -> `benchmarks/server/`
- move: `bench_auth_token_validation.cpp` -> `benchmarks/server/`
- move: `bench_postgres_e2e.cpp` -> `benchmarks/server/`
- move: `bench_postgres_protocol.cpp` -> `benchmarks/server/`
- move: `bench_postgres_transactions.cpp` -> `benchmarks/server/`
- move: `bench_stream_protocol.cpp` -> `benchmarks/server/`
- move: `bench_wal_apply_grpc.cpp` -> `benchmarks/server/`
- add: `benchmarks/server/CMakeLists.txt`
- update: `benchmarks/CMakeLists.txt`
- update: `benchmarks/README.md`, `benchmarks/INDEX.md`

Acceptance:
- `python3 benchmarks/scripts/audit_benchmark_registration.py` still reports `Unregistered: 0`
- moved targets are registered through `benchmarks/server/CMakeLists.txt`
- benchmark docs mention the new `server/` module-local layout
