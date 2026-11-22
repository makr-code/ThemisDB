---
Author: "ThemisDB Contributors"
Created: "2026-07-19"
Last Updated: "2026-11-04"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "ROADMAP.md"
  - "PERFORMANCE_EXPECTATIONS.md"
  - "../../include/storage/"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage Module - Future Enhancements

<!-- Planning document reviewed: 2026-10-31; items below are proposals, not verified current behavior or measured targets. -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- hardening and refinement of storage runtime behavior
- deterministic reliability improvements for durability/recovery paths
- stronger benchmark-backed guardrails for storage hot paths

## Design Constraints

- storage contracts remain backward compatible within major release line.
- persistence and recovery outcomes remain explicit and deterministic.
- degraded backend/tiering paths remain observable and non-silent.
- maintenance and replay behavior remains bounded and diagnosable.

## Required Interfaces

| Interface | Requirement |
|---|---|
| persistence interfaces | deterministic lifecycle and durable write behavior |
| versioning interfaces | stable MVCC and replay semantics |
| recovery interfaces | explicit backup/PITR result contracts |
| maintenance interfaces | bounded compaction/pruning/index-maintenance behavior |

## Implementation Notes

- tighten parity between replay, compaction, and recovery diagnostics.
- extend WAL recovery metrics and report snapshots for deployment monitoring
  without weakening strict replay failure; cumulative replay progress and
  terminal status/duration metrics are now wired to `MetricsCollector`.
- retain corruption/timeout audit evidence with segment and valid-prefix context.
- standardize incident taxonomy for durability and maintenance classes.
- expand resilience tests for prolonged mixed write and recovery traffic.
- broaden benchmark depth for allocator and mount-latency scenarios.
- Resolve the WAL length-header ambiguity with a versioned frame or commit-boundary
  design that validates declared lengths independently before destructive tail
  repair. A corrupted completed ACKed frame must not be silently dropped, while
  a genuine interrupted append remains recoverable. Specify migration, rollback,
  and compatibility behavior before changing the on-disk contract; do not add an
  unapproved legacy fallback or quarantine path.

## Test Strategy

- unit and integration suites for persistence, recovery, and maintenance paths.
- regressions for WAL replay, PITR faults, and tiered/blob edge failures.
- The WAL process-crash CTests require Python 3.10 or newer. Without a suitable
  interpreter, configuration warns and omits both tests; CI must treat missing
  crash-test registrations as a failure rather than evidence of a passing run.
- `WALProcessCrashRecoveryTests` writes a JSON report to
  `test-results/wal-process-crash.json` below the build directory. The report
  records the scenario, seed, requested and completed iterations, ACK counts,
  segment counts, elapsed time, and explicit running/passed/failed status.
  After each completed iteration the report is atomically replaced with the
  validated result prefix and elapsed time, while status remains `running`.
  A hard parent termination may leave this last checkpoint, not a terminal
  failure record or the currently executing iteration. Only final completion
  and fingerprint checks authorize `passed`; a `running` prefix is not release
  evidence for a complete run. Progress-write errors stop further iterations.
  Atomic replacement is not a power-loss durability/fsync guarantee.
  New reports include `worker_sha256` and `harness_sha256`, computed from the
  executable and Python source before starting iterations. Read errors fail
  explicitly; start/end fingerprint differences prevent `passed`. These identify
  files, not dependent DLLs, compiler options, or a signed build provenance.
  Changes reverted between the checks are not detected. Historical reports,
  including the earlier `*-hardened-1000.json` runs, are not retroactively
  fingerprinted.
  `tests/storage/test_wal_process_crash.py --iterations 1000 --worker <executable>
  --report <path>` runs the same ACK-boundary scenario at nightly-sized scale;
  this option alone does not establish scheduled CI execution or in-flight
  write/fsync crash coverage.
- CI callers in `gate-pr-core.yml` and `build-sanitizer-nightly.yml` now request
  50/1,000 iterations and secondary result uploads; verify executed GitHub
  results before treating that configuration as operational evidence.
- The harness rejects recognized ASan/LSan/UBSan/TSan/MSan diagnostic headers
  and source-location-prefixed UBSan `runtime error:` lines independently of
  exit status. Writer checks run before ACK acceptance and again on the full
  transcript after reader join, before recovery. Verifier checks cover both
  stdout/stderr and the optional N+1 retry. Benign sanitizer enablement messages
  are allowed. This signature check is not exhaustive and cannot inspect
  unflushed output; nonrecovering sanitizer build flags remain required.
- Each verifier process retains the existing 30-second timeout. Timeout
  failures include expected replay count and captured stdout/stderr in the
  failed report and retained `failure.txt`, including N+1 retries. Output is
  represented with Python `repr` to preserve bytes and partial/malformed text
  without replacement decoding. A timeout never permits an additional retry.
  This subprocess deadline does not establish production recovery latency.
- The additional `--scenario unobserved-append` releases one further write
  and terminates the child without awaiting its ACK. Replay must contain the
  entire observed ACK prefix plus zero or one fully validated entry.
  A missing ACK is not evidence that a write was uncommitted. Reports record
  observed ACKs, recovered entries, the released-append flag and requested
  delay; scheduling delays do not prove interruption within an fsync syscall.
- Each crash iteration now verifies two separate recovery processes. The
  second must replay the exact count validated by the first, with no pending
  entry allowance; segment names, sizes and exact byte contents must remain
  unchanged. Reports include `verified_reopens` and WAL byte counts before
  and after the first validated recovery. A size reduction may indicate a
  tail repair; it is not itself proof of a kill during fsync.
- First-recovery snapshots now check exact bytes as well: segment names must
  stay unchanged, older segments must remain byte-identical, and only the
  numerically newest segment may become a strictly shorter identical prefix
  in the unobserved-append scenario. ACK-boundary recovery must not modify any
  WAL bytes. The worker still checks every recovered entry's contents.
- With `--report`, failed iterations retain evidence in
  `<report-stem>-failures/iteration-<number>-<unique-id>` beside the report.
  `before-recovery/` preserves the WAL before the verifier starts, `wal/`
  preserves the failure-time state, and `reproduction.json` and `failure.txt`
  identify the requested ACK boundary/delay and error. Failures before recovery
  may have no pre-recovery snapshot or WAL. JSON exposes `failure_artifact`.
  Workspaces are created directly below the evidence root before the writer
  starts; failed originals do not require a post-failure copy. Successful
  workspaces are deleted (the empty evidence root may remain). If writing
  `failure.txt` fails, retain the original workspace and report the diagnostic
  I/O error separately as `retention_error`, alongside the original error.
  An unwritable evidence root fails before starting the writer.
  Without `--report`, temporary evidence is not retained.
  The combined writer stdout/stderr is saved as UTF-8 `writer.log` after
  process shutdown and reader join, including consumed ACKs and remaining
  output. Only text actually emitted before termination can be captured;
  unflushed child buffers are not evidence. Transcript write failure fails
  the iteration and includes any original writer error in the failed report.
  Failures before process startup or reader shutdown may have no transcript.
  Pipe I/O and decoding failures are delivered to the parent through the ACK
  queue, not left as background-thread exceptions. After reader join, remaining
  errors must also fail before recovery, even if all requested ACKs arrived.
  Preserve the partial transcript and explicit error; do not replace malformed
  output with silently substituted text.
- deterministic stress runs for sustained mixed read/write + maintenance load.
- Exhaustive local disk-fault evidence on 2026-11-04 covers all 40 prefixes
  (0..39 retained bytes) of the last 39-byte PUT frame in a four-segment,
  killed-writer fixture. Prefixes 0..38 recover exactly entries 1..11; a full
  frame recovers all 12. First recovery repairs 38 nonempty partial tails;
  the second replay preserves exact bytes. Separately, 18 payload/CRC byte
  bit-flips and 36 partial older-segment tails each fail closed twice without
  changing WAL bytes. Report: `wal-tail-boundaries-20261104.json` under the
  Windows build's `test-results/`. The writer ACKed 12 entries before fault
  injection; deliberately deleting part of entry 12 is not an ACK-loss result
  attributable to the process kill. This campaign is ad-hoc, not CI-registered,
  and does not prove interruption within write/fsync or production backup repair.
- The same 94-case campaign is now reproducible via
  `python tests/storage/test_wal_tail_boundaries.py --worker <executable>
  --report <path>`. It obtains frame boundaries from ACK-gated snapshots and
  fails explicitly if the known fixture layout changes. Per-case reports,
  retained failing WALs and fingerprints for worker, campaign and shared
  harness reuse the crash-harness infrastructure. Both PR/nightly callers
  run it and declare `test-results/wal-tail-boundaries.json` for upload.
  Local execution passed; actual GitHub execution/publication remains unverified.
- The checked-in campaign now covers all three older segment positions, with
  36 incomplete-frame cuts each, not only the immediately preceding segment.
  Results and retained reproduction metadata identify `affected_segment`.
  `wal-tail-all-segments.json` passed 166 cases locally: 80 successful reopens,
  252 fail-closed opens without mutation, 333 verifier processes (43.566 s).
  All 38 nonempty partial latest tails were repaired exactly; historical
  94-case reports remain unchanged. Existing CI invocations use the expanded
  script, but successful GitHub/sanitizer execution remains unverified.
- Before either boundary-campaign recovery path, copy the injected WAL to
  `before-recovery/`. Failed cases retain both this input and current `wal/`,
  even when recovery mutated the latter. Snapshot I/O failure must stop
  before any verifier and preserve the original files. Regressions cover
  both recovery paths and snapshot failure. All 166 real-worker cases passed
  again in `wal-tail-forensic-retention.json` (40.822 s); a diagnostic count
  mismatch after real tail repair retained the exact 79-byte input and
  78-byte output. This is forensic retention, not production backup durability.
- Extend complete-frame corruption to all twelve fixture frames: one low-bit
  flip at each payload/CRC byte (198 positions total), with two fail-closed,
  byte-preserving opens per fault. `segment_byte_offset` identifies absolute
  offsets in results and retained reproduction metadata. The fixed fixture
  contract now also rejects unexpected older-segment lengths. Local report
  `wal-all-frame-corruption.json` passed 346 cases: 80 successful reopens,
  612 closed failures, 693 verifier processes (83.410 s). Exact byte sets,
  fingerprints and cleanup were independently verified; 67 tests plus 39
  subtests passed. This does not cover every bit or header field and does
  not prove syscall interruption, primary-store recovery or sanitizer execution.
- Header corruption now covers magic, sequence and entry type (frame offsets
  0..12) in all twelve frames: 156 low-bit flips, each failing closed twice
  without mutation. Results and reproduction data retain absolute byte offsets.
  `wal-all-frame-header-corruption.json` passed 502 local cases: 80 successful
  reopens, 924 closed failures, 1,005 verifier processes (120.286 s).
  Exact header and payload/CRC sets, fingerprints and cleanup were independently
  checked; this initial report covered low-bit flips only and is superseded for
  non-length bytes by the subsequent all-bit run below. Length fields at offsets 13..20
  are deliberately separate: corrupted lengths can resemble an incomplete
  newest tail, so blanket rejection is not asserted by this campaign.
  Targeted syscall/primary-store/sanitizer evidence remains open.
- All eight bit positions at every non-length header, payload, and CRC byte
  were then exercised across the 12-frame fixture: 2,832 individual faults,
  each rejected twice without WAL mutation. `wal-all-protected-bits.json`
  passed 2,980 cases (40 tail prefixes, 108 older-segment tails, 2,832 bit
  faults), 80 successful reopens, 5,880 closed opens, and 5,961 verifier
  processes in 826.929 s. Exact per-segment bit sets, fingerprints, report
  status and cleanup were independently verified; 67 tests plus 40 subtests
  passed. The eight length bytes are intentionally excluded: their separate
  64-bit probe found 61 destructive truncations and three fail-closed errors.
- A real-worker probe on the ACKed 12-entry fixture flipped each of the 64
  individual bits across the eight key/value length bytes (absolute final-frame
  offsets 91..98). 61 mutations made recovery report `TruncatedTail`, replay
  only 11 entries, and truncate `wal_000004.log` from 117 to 78 bytes; three
  smaller declared lengths reached CRC validation and failed closed without
  truncation. Each byte's low-bit flip was among the 61 data-loss outcomes.
  The verifier exited 6 only after recovery because its expected count was 12.
  This confirms loss of an acknowledged PUT on most length-bit corruptions. No production fix is claimed:
  resolving it requires an approved versioned framing/commit-boundary design
  that distinguishes a genuinely torn append from corrupted declared lengths.
  Diagnostic report: `wal-tail-length-all-bits.json` in the Windows build's
  `test-results/`.
  Until then, STO-01 is not fail-closed for this case. Required acceptance:
  all eight length-byte faults preserve the acknowledged prefix and original
  evidence, while each genuine partial-tail boundary retains its specified
  recovery behavior. Migration and rollback must be explicit; no silent
  compatibility fallback is allowed.
- The current hardened harness passed 1,000 local Windows iterations per
  scenario on 2026-11-04, including first-recovery boundaries, repeat-recovery
  byte stability and reader error checks. Reports
  `wal-process-crash-hardened-1000.json` and
  `wal-unobserved-append-hardened-1000.json` under the Windows build's
  `test-results/` are distinct from historical 1,000-iteration reports.
  This is local scale evidence, not scheduled sanitizer execution.
- release-profile benchmark runs for mapped storage targets.

## Performance Targets

- storage hot paths remain inside regression budgets.
- write/replay/recovery-sensitive operations remain stable at p95/p99 envelopes.
- mapped benchmark manifests reach no-missing-case status for release gating.

## Security / Reliability

- maintain strict bounded behavior for durability and recovery transitions.
- preserve explicit failure signaling for storage and maintenance faults.
- enforce predictable degradation under storage pressure and backend faults.
- keep diagnostics actionable for production storage incidents.