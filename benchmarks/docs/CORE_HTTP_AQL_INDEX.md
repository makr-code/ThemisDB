# Core HTTP AQL/index workload (partial Block C)

Catalog profile: [`themisdb:e2e/core-http-aql-index-equality@1`](../workload_profiles.json).
Successful reports include `workload_id` as an additive semantic identity, not
a run or configuration identifier. Older reports omit it; record the timeout,
server configuration, build, hardware and source revision separately.
Suite-map relationships are descriptive and do not schedule this Python runner.

Run against a dedicated, explicitly configured server:

```powershell
python -m benchmarks.core_http_aql_index --endpoint http://127.0.0.1:8765 `
  --records 1000 --queries 10000 --output <new-report.json> --token-env THEMIS_TOKEN
```

`--token-env` is optional and names an environment variable containing a bearer
token; the same header is sent to index, entity and query endpoints. The server
needs the `aql_request` schema in its configured schema directory (for example
via `THEMIS_SCHEMAS_DIR`). The runner does not launch or configure the server.
Respect rate limits and do not use a shared or production instance. Errors abort
publication; after a timeout, a write or index change may still commit after
cleanup. The CLI reports the owned collection for manual investigation.

The runner creates a non-unique secondary index on the synthetic `lookup` field
before loading records through batch HTTP. Its alphabetic values are unique
across the configured dataset and avoid the default detector's digit-pattern
masking; no masking configuration is changed. It verifies loaded entities,
then performs 50 warmup writes, 100 sequential and 200 seeded random reads,
up to 10 validated AQL warmup lookups, and one explain check. The separate
measurement phase issues seeded uniform equality queries with full-scan fallback
disabled. Every measured response must contain exactly the expected entity;
all loaded entities are checked again after measurement.

The explain check requires `index_optimized` and the expected predicate. This is
the server-reported plan, not an independent trace of physical I/O. Cleanup
deletes only attempted entity keys while index metadata exists, checks their
absence and a zero index-entry count, then drops the index and checks its
metadata is absent. If entity cleanup or its checks fail, the index is retained
for recovery. Phase times, client-request latency summaries and wall-clock request
throughput are separate; individual latency samples are not in the report, and
p95/p99 are null below 20/100 samples. No transaction, restart
durability, concurrent-load, cross-database or broad AQL coverage is claimed.

## Process resource observation

When `psutil` can identify the Python client process, `resources.client`
observes it; otherwise it is `null`. `resources.server` is `null` unless `--server-pid PID`
is supplied. The selected PID must be local, positive and distinct from the
client, and must own a TCP listener on the requested numeric IPv4/IPv6 loopback
address and port before index creation or writes. A same-family wildcard
listener is accepted. With `--server-pid`, hostnames such as `localhost` are
rejected to avoid ambiguous DNS address selection. Without it, existing endpoint
behavior is unchanged. Missing `psutil`, inaccessible listener information,
a wrong PID/address/port, or an exited process aborts before writes. Listener
ownership is checked at preflight, not attributed to every request.

The client and optional server each have before/after process snapshots and
CPU/process-I/O deltas for index/load, warmup/plan check, measurement,
validation and cleanup. Snapshot-to-snapshot wall time includes observation
overhead and is not the request throughput denominator; server deltas may
include background activity. RSS, virtual memory and optional private memory
are endpoint samples, not phase peaks. OS-provided peak working-set and commit
fields are process-lifetime high waters, not phase peaks. Unsupported or
inaccessible counters and unavailable deltas are `null`. Children, other
processes, GPU, energy and disk footprint are excluded; process I/O is not
physical disk I/O. A detected process exit or reused PID prevents report
publication, but owned cleanup is still attempted.

## Functional smoke, 2026-11-23

The copied Community Release executable SHA-256
`26f3d9c85f8efbbcfa243ccc4217879e870ef5e1baac049c4396b25d621e3bbf`
ran in a private temporary directory with copied AQL schema, separate database,
minimal profile, two threads, loopback-only HTTP and an explicitly raised
100000/minute rate limit. Listener PID and health were verified first. Three
loaded records and four measured lookups passed result and reported-plan
validation; owned entities and index entries/metadata were absent after cleanup,
and an unrelated entity was preserved. The harness terminated its server rather
than performing a graceful shutdown. Evidence:
`%TEMP%\themis-aql-smoke-6qly8c7d\smoke-report.json` and adjacent logs/database.
The copied binary was not rebuilt from the current worktree. This short run is
functional evidence only, not a performance result. Its local server did not
exercise authentication; header propagation and HTTP failures are covered by
the test transport, not a live authenticated deployment.

Earlier probes correctly rejected masked email and digit-pattern lookup results;
they were not accepted as measurements. The initial schema-less probe returned
HTTP 400 and the required schema was then copied to the isolated environment.

### Resource-observed functional smoke, 2026-11-23

The owned server and private database used for the resource-observed CRUD smoke
also ran the AQL path with `--server-pid`, three loaded records and four
verified queries. Its PID and HTTP/health loopback listeners were checked before
requests. The report contained distinct client/server PIDs and all five phase
snapshots and delta fields; RSS and virtual memory were present in every
snapshot. The workload's entity and index cleanup checks passed, and an owned
sentinel survived before being deleted and checked absent. The harness
terminated only its owned server, not gracefully. No pre-existing
`themis_server.exe` process was present at the process check.

An initial server start was stopped before HTTP requests when its health
listener used default port 9090; the retry used a separately selected loopback
health port. Evidence is under `%TEMP%\themis-resource-functional-fkylyq60`,
not a durable artifact. The copied Community Release binary was not rebuilt
from the current worktree. Startup logged a WAL gRPC TLS/insecure-mode error;
WAL gRPC was not exercised by this smoke. This smoke is not performance,
authentication or restart-durability evidence.

Focused tests:
`python -m unittest discover -s benchmarks/tests -p test_core_http_aql_index.py -v`.

### Authenticated isolated HTTP smoke, 2026-11-23

A separate live functional smoke used a private copy of
`build-msvc-windows-bench/bin/themis_server.exe` and its adjacent DLLs. The
binary reported version `2.4.0` and build identifier
`a187bc52-82ca-5d1a-8b54-a61ff5390c06`; its SHA-256 was
`26f3d9c85f8efbbcfa243ccc4217879e870ef5e1baac049c4396b25d621e3bbf`.
The source executable's modification time was `2026-11-22T20:27:38.8261396+01:00`.
It was an existing build-cache binary, not freshly built from the current
worktree; this is not evidence that the current source checkout was built or
that a release binary has this behavior.

The copied server ran with an ACL-restricted temporary directory, a private
JSON configuration and RocksDB path, a copied AQL request schema, two workers,
the `minimal` profile, and independently allocated HTTP and health ports bound
to `127.0.0.1`. The private process environment contained only required Windows
variables, test settings, and a freshly generated 256-bit admin token. The
token was supplied through environment variables to that server and the
benchmark client only; it was not written to a file or included in output.
Both listeners were confirmed to belong to the copied server PID before
requests. No pre-existing server or process was modified.

On the protected `GET /config` endpoint, missing auth returned HTTP 401, an
incorrect bearer token returned HTTP 403, and the valid temporary admin token
returned HTTP 200. An authenticated empty AQL query was rejected with HTTP 400.
The benchmark then completed a three-record run with four verified queries:
authenticated index creation/statistics, entity load/readback, the AQL equality
queries, and `explain` validation of the reported `index_optimized` plan all
passed. Cleanup verified the attempted entities were absent, the index had
zero entries before drop, and its metadata was absent afterward. The small
smoke report and temporary database were removed with the isolated directory.

This confirms only the HTTP functional path of the identified cached binary.
It does not establish current-worktree freshness, physical index I/O,
performance, durability, concurrency, or production authentication policy.
The elevated request-rate setting was private to this smoke and is not a
production recommendation.
