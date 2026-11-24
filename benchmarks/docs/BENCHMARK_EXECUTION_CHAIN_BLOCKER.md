# Local benchmark execution evidence: blocker

## Decision

No execution-evidence chain has been added or enabled. The existing CTest
manifest and MSVC runner cannot safely be wrapped as-is to meet the requested
no-overwrite, fail-closed and owned-process-cleanup requirements. The existing
identity capture remains a post-run self-description; the optional evidence
inspector remains a consistency check of supplied declarations, not proof of
execution. Neither should be promoted into an execution receipt or sidecar.
This assessment makes no performance, baseline or release claim.

## Findings

- `benchmarks/cmake/benchmark_run_manifest.py` checks that the command matches
  its registration and redirects one output argument to a staged path, but it
  does not record executable hashes before and after the child execution or
  hash the result bytes it publishes. Its lease coordinates participating CTest
  runs, not other writers. The runner unlinks the registered result before
  starting and later uses `os.replace`; preparation can move an existing
  results directory. This is incompatible with a no-overwrite wrapper.
- The CTest runner reports nonzero child exits and timeouts, but its
  `execution_status` describes the child, not artifact validity. A zero-exit
  child with no or invalid JSON is `completed` even though the runner fails.
  It preserves case errors and skips in the manifest, but a skipped case alone
  returns zero, and valid error-marked JSON can be published before the runner
  returns failure. `read_result` does not enforce the stricter result contract
  used by `tools/ci/validate_benchmark_smoke.py`, such as consistent iteration
  names and metrics. Neither process status nor JSON status alone is a success
  predicate.
- On Windows the CTest runner terminates the direct child on timeout; this is
  not containment of descendants, nor does it guarantee cleanup after an
  abrupt wrapper death. POSIX process-group handling likewise does not contain
  a child that escapes its group. The MSVC runner uses `Kill(true)` on timeout,
  but starts without prior containment and disposes the process in `finally`;
  this alone does not establish descendant containment or cleanup on every
  exceptional/interrupted path.
- `benchmarks/run_benchmarks_msvc.ps1` hashes discovered executables when
  constructing its manifest, not immediately on both sides of each execution.
  It creates/reuses an output directory named to the second and writes
  fixed-name artifacts. It is not a no-clobber single-execution primitive.

## Focused local probes (Windows)

The assessment used only a newly created private project-directory workspace,
the existing manifest runner, and short-lived synthetic Python child processes.
No benchmark executable, server, or foreign process was started or terminated.
The runner was called directly with a private manifest and lease; this was not
a CTest integration run.

| Synthetic child | Observed runner result |
|---|---|
| Writes JSON then exits 7 | wrapper exit 7, `process_failure`; no published result |
| Sleeps past one-second timeout | wrapper exit 124, `timeout`; a handle opened while the owned child was alive became signaled |
| Zero exit, no result | wrapper exit 1, child status `completed`, JSON `missing` |
| Zero exit, malformed JSON | wrapper exit 1, child status `completed`, JSON `invalid` |
| Zero exit, skipped row | wrapper exit 0, case `skipped` |
| Zero exit, inconsistent iteration `run_name` | wrapper exit 0, JSON `valid` |
| Existing private result sentinel | sentinel replaced by the new result |

Two independent SHA-256 reads of a private fixture before and after changing
its bytes differed and matched independently calculated digests. This is not a
test of executable mutation during a run: no such chain exists. The timeout
probe verified termination of **only the direct owned child**, not descendants,
wrapper crash cleanup, or a general guarantee. The probe's first workspace
cleanup encountered a transient Windows sharing violation; retry removed the
owned workspace. These observations do not establish benchmark correctness.

## Requirements before a narrow opt-in implementation

An implementation needs a newly reserved, private output location with
exclusive creation and no replacement of pre-existing paths; caller-provided
paths and links must not allow writes or cleanup outside it. One explicitly
selected Google Benchmark executable should be launched without a shell, with
one owned output path and bounded timeout. The launch, wait, failure reporting,
and cleanup must be one controlled lifecycle, rather than a post-hoc join of
independently supplied files. A Windows implementation needs containment
established before the child can spawn descendants, reliable cleanup and wait
on timeout and interruption, and tests for containment failure and termination
of only owned processes. Existing runner behavior cannot simply be assumed to
provide those guarantees.

Hash the executable immediately before launch and after it exits, and hash the
exact result bytes used for strict JSON and case/repetition validation. A
changed, unreadable, missing or inconsistent input, skipped/error case,
nonzero exit, timeout, or incomplete output must never yield a successful
receipt. The expected case inventory must be explicit and reviewed; catalog
links and self-declared sidecars do not supply that proof. Tests need private
synthetic processes for all these paths, including executable hash changes,
cleanup and no-clobber behavior; a successful synthetic test is not a
representative benchmark. Do not automatically activate any CI gate.

Even that design is local observation, not attestation. Pre/post path hashes
cannot rule out replacement between hashing and launch, transient changes
restored before the second hash, modification of the executing image, or
concurrent alteration of outputs. A hostile or compromised same-user process
or operating system can interfere with paths, process containment, clocks,
and evidence. Dynamic DLLs, dependencies, kernel behavior, environment and
external load are not captured by an executable hash. CPU/host fingerprints
sampled at endpoints do not attest identity or isolation throughout execution.
Receipt files themselves remain editable. These limits require separate
threat-model and baseline review, not stronger wording around local hashes.

## Follow-up probe (WSL2, 2026-11-23)

The scope approach was re-probed with a newly compiled, private synthetic ELF
only; no W4A or other benchmark was launched. In one WSL invocation where the
user manager was available, `systemd-run --user --scope` created a transient
scope reporting `ActiveState=active` and `KillMode=control-group`. The
synthetic ELF's `/proc/<pid>/cgroup` contained the exact `ControlGroup`
reported by systemd, and its PID was the `systemd-run` client's PID with the
probe process as parent. `systemctl --user stop` returned successfully; the
scope then reported `ActiveState=inactive` and `Result=success`. This
establishes direct-child membership and stop behavior for that simple,
non-escaping synthetic process in that invocation. It does not establish
descendant containment or a complete evidence chain.

The user manager was not persistent across subsequent WSL invocations:
`systemctl --user is-active default.target` and `systemd-run --user` failed
with `Failed to connect to bus: No such file or directory`; read-only
inspection reported `user@1000.service` inactive/dead and no logged-in or
lingering user. A later direct probe confirmed uid 1000, `XDG_RUNTIME_DIR`,
and `DBUS_SESSION_BUS_ADDRESS` were set, but the bus socket was absent. Starting
a temporary `systemd --user` process in that shell also failed to create the
bus. Running the new focused integration-test command therefore collected zero
tests and skipped the class because no active user manager was available.
The transient-scope test for an escaping child and an independently observed
manager `RuntimeMaxSec` timeout could not be run in this tool session. A
separate earlier invocation from this task had returned exit 143 and
`Result=timeout` for a 2-second `RuntimeMaxSec`/1-second `TimeoutStopSec`
scope; that probe was not repeated under the later unavailable-manager
conditions and did not verify child-process containment.

No runner or test change is retained. The successful single-process probe is
not enough to satisfy the requested testable containment and timeout chain;
implementing a success receipt without the escaping-child and manager-timeout
tests would be speculative. Once a WSL login context keeps the actual
`systemd --user` manager available, repeat the synthetic-only direct-child,
escaping-child, stop and manager-timeout tests, then reassess. Do not start a
representative W4A run.
