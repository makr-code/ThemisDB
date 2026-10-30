---
Author: ThemisDB Contributors
Created: 2026-10-29
Last Updated: 2026-10-29
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: unknown
---

# Benchmark Comparability Delivery

Exact model backend not verifiable from final document text alone.

Canonical scope: `benchmarks/BENCHMARK_STANDARDS.md` and `benchmarks/ROADMAP.md`.
This file is execution evidence, not release or compliance status.

- [x] Validate process resources with an actual importer child process, not the shell.
- [x] Bound each executable and verify timeout classification with a one-second run.
- [x] Verify microsecond-to-nanosecond normalization against raw importer JSON.
- [x] Verify case filtering, three repetitions and hardware/source run context.
- [x] Define external reference-workload evidence without claiming current compliance.
- [x] Remove Python YCSB sleep fallback and unverified industry grades; correct F to read/RMW, count all operations and add seeded request p50/p95/p99 measurements.
- [x] Add real ThemisDB SDK adapter and fail-closed CLI for A/B/C/F. Eighteen focused tests pass, including SDK transport contracts, corrupted reads, uncertain-commit cleanup, successful CLI export and a real local connection failure.
- [x] Fix the SDK module docstring ordering that prevented Python import; install the SDK as a regular package because the editable finder loses to `benchmarks/themis` namespace resolution.
- [x] Verify the existing SDK topology/entity regression: `python -m pytest clients/python/tests/test_topology.py -q` reports 8 passed. One third-party anyio/pytest deprecation warning remains unrelated to this change.
- [x] Verify the canonical HTTP path against a live server: the SDK-generated dotted table names were rejected as invalid path segments, so the benchmark now uses direct `collection:pk` Entity URLs through httpx. Server validation and SDK namespaces remain unchanged.
- [x] Add optional server-PID observation with local listener verification, four memory/CPU/I/O snapshots and separate load/measurement/cleanup deltas. Twenty focused tests pass.
- [x] Complete all A/B/C/F live functional smokes (100 records/200 operations, Community Debug, minimal profile, loopback:18765, two threads, explicit finite rate budget 100000/minute). Evidence: `benchmarks/benchmark_results/live_ycsb/20261029_183902/live_summary.json`, per-workload JSON/CLI logs and server output. Lifetime peaks are cumulative, not per-workload peaks. Test server PID 62064 and its listener were verified stopped.
- [x] Configure the CTest benchmark preset to run serially and execute `compare_expectations.py` as the final CTest fixture-cleanup step; old JSON outputs are archived, and duplicate Google Benchmark case names resolve deterministically to the run with more iterations.
- [x] Build and verify the Community Release server, then complete twelve live runs (1000 records/2000 operations, three sequential repetitions per A/B/C/F, 24000 operations total). Evidence: `benchmarks/benchmark_results/release_ycsb/20261029_200312/release_summary.json`. Server PID 11136 and listener were verified stopped. These runs retain the original two-handler `/Od` warning and binary hash.
- [x] Add default-ON `THEMIS_MSVC_HEAVY_HANDLER_LOW_MEMORY` to preserve the existing CI workaround; local `vscode-windows-release` sets OFF. Both workspace Release tasks now use that explicit preset. Generated handler commands show `/O2` plus `/bigobj` without `/Od`; Release build passed without `C1060` or `D9025`.
- [x] Validate the newly optimized binary separately: Health, Metrics and JSON-body Index-Stats returned 200; A/B/C/F functional smokes completed 800 operations total. Evidence: `benchmarks/benchmark_results/optimized_release_validation/20261029_204406/validation_summary.json` and `handler_compile_commands.json`. Query-string Index-Stats returned 404 before dispatch and remains a separate unresolved routing observation. No speedup claim is made across different protocols.
- [ ] Complete external-driver adapters, correctness checks and representative resource runs.

Routing exception: the configured Ollama endpoint was unreachable. The user
explicitly approved direct Copilot implementation of this YCSB fix and its tests.
The user subsequently approved direct CMake changes for this benchmark-hardening
work while Ollama is unavailable. No C++ source changes were needed here.

Live limitation: Debug and Release validation runs now exist, with different
dataset sizes and binary hashes. They verify functional operation and process
resource capture, not an apples-to-apples speedup. Official drivers, representative workloads,
independent trials, disk/GPU/energy metrics and complete cross-database evidence
remain open. Test-only MockTransport still supplies focused contract tests; the
live results above are from actual Entity requests, not simulated measurements.

The old full pass was interrupted at `bench_gossip_config`; one 10-update
scalability iteration reported about 458 seconds. Its production call attempts
peer propagation despite the fixture disabling background gossip. Root cause
inside networking and a benchmark-isolation fix have not been verified.
Existing logs/raw JSON remain intact; no complete baseline or resource comparison
can be inferred from that pass.

Wiki gap: the 2026-10-26 generated developer wiki lists benchmark workflows as
binary/unreadable and has no substantive resource/comparability contract. Consult
the canonical benchmark standard directly; regenerate the wiki through its
normal source-sync pipeline after review rather than editing generated entries.