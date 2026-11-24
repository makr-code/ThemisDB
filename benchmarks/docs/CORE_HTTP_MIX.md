# Core HTTP CRUD mix (partial Block C)

Catalog profile: [`themisdb:e2e/core-http-crud-mix@1`](../workload_profiles.json).
Successful reports include `workload_id` as an additive semantic identity, not
a run or configuration identifier. Older reports omit it; record the timeout,
server configuration, build, hardware and source revision separately.
Suite-map relationships are descriptive and do not schedule this Python runner.

`python -m benchmarks.core_http_mix --endpoint http://127.0.0.1:8765 --records 1000 --output <new-report.json>`

Run only against a dedicated, explicitly configured server. Optional `--token-env`
names an environment variable containing a bearer token. Respect the server's
rate limits; errors abort rather than silently retry. The runner does not launch,
configure, restart or stop the server. Use a new report path. Inspect the unique
collection named in errors before manual recovery; a timed-out request may still
commit after cleanup. Do not use shared or production instances.

The seeded, synthetic workload loads and validates three records per
set, performs the canonical 50 writes/100 sequential reads/200 random reads
warmup, then shuffles equal numbers of single Entity API create, read, update
and delete requests on disjoint keys. Measurement defaults to serial execution;
`--concurrency N` selects 1–64 bounded workers. The shared HTTPX client supports
concurrent requests. This is a closed-loop workload: after a request completes,
the coordinator can submit another, with no fixed arrival rate. The report
records configured concurrency, the maximum in-flight request bound and the
arrival model. The seed fixes the submission order, not concurrent execution or
completion order. Setup, warmup, validation and cleanup remain sequential.
Validation checks measured reads and all
final entity states, including deletes. Cleanup batch-deletes only attempted
keys and verifies their absence. Each measured create key is registered for
cleanup before submission, since a timed-out request may still commit. On
failure, no new work is scheduled after the coordinator
observes it, and all submitted workers finish before cleanup begins. This
cannot cancel a request already accepted by the server: a timed-out or
otherwise indeterminate write can commit after the absence check. Treat the
collection as needing manual inspection/recovery; successful immediate cleanup
is not proof that no late write will occur. A failed operation, validation or cleanup
prevents publication. Phase wall times are separate. Request latency measures
the actual individual HTTP calls. Each operation's `request_latencies_ms`
contains one sample per completed request in collection order (not keyed to
entities or guaranteed to reflect completion or server ordering).
p95/p99 are null below 20/100 samples per
operation. Throughput divides completed measured requests by the whole
measurement window, including scheduling and response envelope checks.

## Process resource observation

When `psutil` can identify the Python client process, `resources.client`
observes it. Without `psutil` or process-identification access it is `null`.
`resources.server` remains `null` unless
`--server-pid PID` is supplied. For example, append
`--server-pid <local-server-pid>` to the command above. The PID must be
positive, distinct from the client, and own a TCP listener on the requested
numeric loopback address and port (`127.0.0.1`, another IPv4 loopback address,
or `[::1]`). An appropriate same-family wildcard listener is accepted.
Hostnames including `localhost` are rejected with `--server-pid` to avoid
ambiguous DNS address selection; endpoints without that option retain their
existing behavior. Missing `psutil`, inaccessible listener information, a
wrong PID/address/port, or an exited process aborts before writes. The check
establishes listener ownership at preflight, not per-request attribution.

For each load, warmup, measurement, validation and cleanup phase, the report
records separate before/after process snapshots and CPU/process-I/O deltas.
Resource-phase wall time is between snapshot timestamps and includes
observation overhead; it is not the request-latency or throughput denominator.
Server deltas may include unrelated background work. RSS, virtual and optional
private memory are endpoint samples, not phase peaks. Where provided by the OS,
peak working-set and commit counters are **process-lifetime** high waters,
not per-phase peaks. Unsupported or inaccessible individual counters and
unavailable deltas are `null`, never zero by default. Observation excludes
children, other processes, GPU and energy; process I/O is not physical disk I/O,
and storage footprint is not measured. A detected process exit or PID reuse
prevents publication; owned cleanup is still attempted.

This exercises the client-to-Entity API storage path, not restart durability.
It is not a transaction, index, AQL, multi-node or upstream-standard workload.
The existing separate batch runner covers batch PUT, not mixed batch operations.
The unit tests use an HTTP test transport and do not constitute performance evidence.

## Functional smoke, 2026-11-23

A copied Community Release server executable (SHA-256
`26f3d9c85f8efbbcfa243ccc4217879e870ef5e1baac049c4396b25d621e3bbf`)
and copied runtime DLLs ran in a private temporary directory with separate
database and logs, minimal profile, two worker threads, loopback-only HTTP and
an explicit 100000 requests/minute rate limit. Its listener PID and health were
checked before requests. The two-record smoke completed two requests of each
CRUD kind, final-state validation and owned-key cleanup/absence checks; an
unrelated entity survived. The server was terminated by the harness, not
gracefully shut down. This is not a restart-durability test.

A second isolated database with the same copied binary used test-only fault
injection to make one initial batch operation invalid. The real server reported
two successes and one failure; the runner rejected the partial response, deleted
all three attempted keys and verified they were absent, without publishing a
report. Both server processes were stopped. Temporary evidence is under
`%TEMP%\themis-core-smoke-8pc1ioof`; it is not a durable repository artifact.
The binary came from an existing shared build, not a build of the current
worktree, and the smoke does not establish version-wide compatibility or
performance. Isolated repeated performance runs remain separate future work.

### Resource-observed functional smoke, 2026-11-23

An owned server using a fresh copy of the same executable and DLLs ran with a
private database, minimal profile, two threads, explicit rate limit and two
separately selected loopback ports for HTTP and the health service. Its PID and
listeners were checked before HTTP requests. With `--server-pid`, two records
completed eight CRUD requests, validation and cleanup. The report contained
distinct client/server PIDs and all five phase snapshots and delta fields;
RSS and virtual memory were present in every snapshot. An owned sentinel
survived the workload and was then deleted and verified absent. The harness
terminated only its owned server; it was not a graceful shutdown.

An initial attempt was stopped before HTTP requests because the server opened
its default loopback health port 9090; the successful retry explicitly selected
an isolated health port. No pre-existing `themis_server.exe` process was
present at the process check. Reports and private data are under
`%TEMP%\themis-resource-functional-fkylyq60`; this temporary evidence is not
a durable artifact. The binary was not rebuilt from the current worktree.
Startup logged a WAL gRPC TLS/insecure-mode error; WAL gRPC was not exercised
by this smoke. This is functional verification, not performance or durability
evidence.

### Concurrent functional smoke, 2026-11-23

A fresh private copy of the same Community Release binary and DLLs ran with
minimal profile, two server threads, separate loopback HTTP/health ports and
the explicit rate limit above. After checking both listeners belonged to the
owned PID, a four-record run with `--concurrency 4 --server-pid PID` completed
16 requests, with four latency samples per operation, final-state validation
and owned-key cleanup. Both process resource reports had all five phases. A
sentinel survived, was deleted and verified absent; only the owned server was
terminated. Temporary evidence is under
`%TEMP%\themis-concurrent-functional-final-z51xtwve`.

An earlier harness attempt completed the workload but its phase-schema check
failed; the server was stopped before the sentinel was removed from its
private database (`%TEMP%\themis-concurrent-functional-r_rp4e2i`). A corrected
retry succeeded. The executable was not rebuilt from this worktree; the
previously described WAL gRPC startup error is outside this smoke's coverage.
These runs provide functional evidence only, not performance or durability
evidence.

Focused contract tests:
`python -m unittest discover -s benchmarks/tests -p test_core_http_mix.py -v`.
