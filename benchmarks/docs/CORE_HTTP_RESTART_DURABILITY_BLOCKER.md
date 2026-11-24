# Core HTTP restart check: Windows opt-in verified; durability limits remain

The original `CTRL_BREAK_EVENT` blocker is reproduced below for context. A
different, **private-console Ctrl+C** lifecycle was verified on Windows on
2026-11-23 and is implemented separately in
[`core_http_restart.py`](../core_http_restart.py). It is an explicit opt-in
functional clean-restart check, **not** a general durability benchmark.
The existing Core HTTP mix/AQL runners remain observation/request-only and
never acquire lifecycle control over their caller-selected endpoint or PID.

## Original lifecycle blocker

The server entry point (`src/main_server.cpp`) registers `SIGINT` and `SIGTERM`
handlers and waits for its shutdown flag. The reviewed Core HTTP runner and
operator instructions do not define a supported authenticated HTTP shutdown
operation. On Windows, Python's `Popen.terminate()`/`SIGTERM` is not a graceful
server-stop contract. A private-process probe sent `CTRL_BREAK_EVENT` to a
server started in its own process group; it exited with status `3221225786`
(`0xC000013A`) and did not emit the registered handler's
`Received shutdown signal, initiating graceful shutdown...` message. This
provides no evidence that the process flushed/closed storage through the
server's controlled shutdown path. Broadcasting Ctrl+C in a user's/shared
console or using an undocumented endpoint is not a safe substitute.

Forced process termination is never a successful restart boundary. A
same-process readback alone is never restart evidence.

## Windows API contract and safety boundary

Local SDK `10.0.26100.0` headers `um\consoleapi.h`, `consoleapi2.h` and
`consoleapi3.h` were checked for the real `Kernel32` declarations of
`AttachConsole`, `FreeConsole`, `SetConsoleCtrlHandler`,
`GenerateConsoleCtrlEvent` and `GetConsoleProcessList`. ctypes declares their
Win32 argument/result types explicitly. Microsoft documents the semantics:

- [`GenerateConsoleCtrlEvent`](https://learn.microsoft.com/en-us/windows/console/generateconsolectrlevent):
  **CTRL_C_EVENT cannot be restricted to a process group**. Group ID zero
  sends to all attached console clients; a nonzero group ID is not a safe
  Ctrl+C targeting mechanism.
- [`SetConsoleCtrlHandler`](https://learn.microsoft.com/en-us/windows/console/setconsolectrlhandler):
  attach/detach resets handler registration. Only the detached helper sets
  its own ignore-Ctrl+C attribute, **after** attaching. The server is not
  created with `CREATE_NEW_PROCESS_GROUP`, which would affect Ctrl+C handling.
- [`GetConsoleProcessList`](https://learn.microsoft.com/en-us/windows/console/getconsoleprocesslist):
  a zero result is an error; an oversized required count must not be
  mistaken for a complete list.

The runner creates a new unique private directory under `--scratch-parent`,
copies the supplied server and adjacent DLLs, generates a new token and fresh
DB/configuration, and starts the copy with **CREATE_NEW_CONSOLE**. It accepts
no existing endpoint, PID, database or configuration. HTTP and health bind
only to `127.0.0.1` on separately selected ports. The WAL gRPC host is also
loopback-only; insecure WAL gRPC is not enabled. The inherited server
environment is allowlisted; user profile, application-data and temporary
paths point into the private directory. Caller Themis/proxy/auth/replication
environment variables are not inherited. HTTPX ignores proxy environment.
The executable and working directory are nested under `runtime` inside the
exclusive private container: the server's `../logs/themis_server.log` path
therefore also remains private, rather than touching the scratch parent's logs.
Source executable/DLL size and modification-time snapshots must remain stable
through copying; an observed concurrent build change rejects the runtime
before launch. This is not a source-revision or reproducible-build attestation.
Port selection has a bind/release race: a collision fails readiness/listener
ownership, rather than permitting writes to another server.

The helper is a short-lived Python child started with **DETACHED_PROCESS**;
it must first prove it has no console (`ERROR_INVALID_HANDLE`). It verifies
its own parent, the server's parent, creation time, executable path and
liveness. Only then does it attach to that owned server PID. All three
membership snapshots, including the final one immediately before generation,
must contain **exactly the server PID and this helper PID**, without duplicates.
Any unexpected process, API failure, incomplete/overflowing list or identity
loss aborts without generating Ctrl+C. The user shell never attaches or
detaches, and no console `ATTACH_PARENT_PROCESS` operation is used.
The caller also validates helper identity and returned snapshots.

**Important residual Windows boundary:** membership enumeration and signal
generation are separate APIs; no atomic membership-lock-and-signal primitive
was established. These checks are snapshots, not a security boundary against
a hostile concurrent `AttachConsole` by another same-user process. Use only in
a controlled local session/runner with trusted executables and no concurrent
console-attachment tooling. If that assumption cannot be ensured, do **not**
opt in; the stronger adversarial-isolation requirement remains blocked.
This is not a broadcast-safe mechanism for arbitrary/shared servers.

Each successful stop independently requires all of:

1. helper membership evidence and successful Ctrl+C generation;
2. the server's stderr handler message
   `Received shutdown signal, initiating graceful shutdown...`;
3. its completion message `Shutdown complete. All data saved.`;
4. a waited **normal exit code 0** (default deadline 65 seconds).

Only after all four conditions does restart occur on the **same private DB**.
Two uniquely named owned entities are read back exactly, including their keys
and JSON types. Deleting the record must leave the owned sentinel unchanged;
then the sentinel is deleted and both absences are verified with HTTP 404.
Both launches require listener ownership and `/config` HTTP **401 without**
and **200 with** the temporary token before Entity requests.
A second proven clean stop precedes removal of the private directory.
The new JSON report is published only on complete success, including directory
cleanup; an existing report is never overwritten.

On failure there is no success report and no restart after an unproven stop.
If necessary, final failure cleanup terminates **only the retained Popen handle
created by this invocation**, never a process found by name or an arbitrary PID.
That forced cleanup is explicitly reported and is not clean-stop evidence.
Cleanup cannot remove private data while its owned server still runs; if OS
cleanup fails, inspect the reported private path, not shared databases.

## Reproducible opt-in invocation

From the repository root, with existing `httpx` and `psutil` dependencies:

```powershell
python -m benchmarks.core_http_restart `
  --allow-private-windows-restart `
  --server-exe build-msvc-windows-bench\bin\themis_server.exe `
  --scratch-parent . `
  --output core-http-restart-result.json
```

`--scratch-parent` must already exist and be trusted/local; only a newly
created `themis-private-restart-<uuid>` child is used and removed. No helper
script is generated in global OS temp: the versioned internal helper is run
directly. The success report remains at the requested new output path.
Nothing is scheduled in CI or existing workload runners automatically.

## Real Windows evidence, 2026-11-23

Windows API version `(10, 0, 26100)`, Python 3.13, HTTPX 0.28.1. All probes
copied the supplied `build-msvc-windows-bench\bin\themis_server.exe` and adjacent
DLLs into private directories. The initial probes used SHA-256:
`26f3d9c85f8efbbcfa243ccc4217879e870ef5e1baac049c4396b25d621e3bbf`.
The existing binary was previously identified as `2.4.0`, build identifier
`a187bc52-82ca-5d1a-8b54-a61ff5390c06`, source file timestamp
`2026-11-22T20:27:38.8261396+01:00`.
The check did **not** build it or establish source-revision provenance.

| Real run | Owned servers / signal helpers | Result |
|---|---|---|
| Initial private-console experiment | `4692 / 38016`, then `37080 / 22956` | Handler + completion logs, exit 0 twice; real restart readback and owned-key HTTP cleanup passed |
| Versioned opt-in runner, run `07daffdcab02457e90a025ab0955eec5` | `33544 / 36284`, then `12308 / 34824` | All three console lists were exact owned pairs at both stops; two entity restart readbacks, sentinel isolation and both HTTP 404 checks passed; private directory removed |
| Real negative membership probe, run `d7fddd1da71c4054958b57cee095f776` | Server `31232`, extra explicitly owned attached child `19768` | Stop helper rejected `Unexpected console member; no Ctrl+C sent`; server remained alive and no handler message appeared |
| Same negative probe after extra child detached/exited 0 | `31232 / 36536`, then `37752 / 14800` | Normal stop, restart readback, own-key removal and second stop all passed; handler + completion + exit 0 twice; private directory removed |
| Nested-runtime containment run `a827038cac124671a7568fd3ac6dde30` | `36884 / 4696`, then `36220 / 14184` | Private `runtime` contains cwd data and keeps `../logs` inside its exclusive parent; exact lists, both clean exits, restart readback, auth, own-key removal and directory cleanup passed |
| Final guarded-copy negative + positive probe `8d286d7709a841eea86b4c4d0240f98a` | Server `31112`, extra owned child `35220`; successful pairs `31112 / 36336`, then `3400 / 38656` | With extra child: rejected without signal, server alive and no handler message. Child detached/exited 0; final code then passed both exact-list stops, handler/completion/exit 0, auth `[401, 200]`, exact restart readback, both owned-key deletions and full nested-container cleanup |

The shared build-cache executable changed **externally during this task**.
The nested-runtime run copied SHA-256
`9e0b7b7842d319d31d42cc92ab8b702602a1033e7b6291bf6dc9f87808189dc6`,
source mtime `1795449419603038200` ns since Unix epoch. Its source revision and
version/build identifier were not independently established. Do not attribute
the initial binary's version/build identifier to this later hash. The runner
now checks source size/mtime stability during copying and reports the copied
executable's own hash.

Both versioned-run starts in each run independently verified auth statuses
`[401, 200]`; the successful stop snapshots contained exactly their tabulated
server/helper PIDs. The extra child in the negative probe belonged only to
this experiment, did not generate events, and detached on its own input pipe.
No existing server, user shell, foreign console client or shared data was used.
Exploratory scripts, copied binaries, logs, private DBs and scratch reports
were removed after recording this evidence. This task edited/built no C++ and
did not stage or commit changes.

## Focused regression tests

```powershell
python -m unittest discover -s benchmarks\tests -p test_core_http_restart.py -v
python -m unittest discover -s benchmarks\tests -p test_core_http_mix.py -v
```

Restart tests cover opt-in/platform refusal before process launch, unknown/
changed/duplicate membership at each of the three checkpoints, API/identity
errors and list overflow, console-wide rather than process-group event
arguments, missing handler/completion/nonzero-exit proof, stop timeout,
non-loopback listeners, invalid/wrong-type readback, no restart after failed
proof, owned-key-only cleanup, nested `../logs` containment, concurrent build
input-change rejection before launch, no overwrite and no failure success-report.
These unit tests use controlled test transports/mocks, not real durability
evidence; the separate real runs above establish the observed Windows result.
All **15** restart tests passed. The existing **28** Core HTTP mix tests also
passed.

## Explicitly unproven / still blocked

- Linux/POSIX execution is rejected before process launch. This Windows result
  does not verify a Linux `SIGINT`/`SIGTERM` harness or any other platform.
- Crash/kill/recovery, power loss, physical media persistence, fsync ordering,
  WAL sync guarantees, transaction atomicity/isolation and general ACID
  properties are **not established** by a successful controlled restart.
- The server completion log is required evidence of this executable's cleanup
  path, not independent proof of all internal flushes or filesystem durability.
- No request latency, throughput, p95/p99, resource or performance conclusion
  follows. Startup/stop deadlines are safety bounds, not measured objectives.
- Compatibility with a newly rebuilt/current-source binary, another edition,
  configuration, Windows service/GUI executable or hostile concurrent local
  console attachment requires separate evidence. Missing handler logs or a
  nonzero/timeout exit keep that run blocked, regardless of earlier successes.
