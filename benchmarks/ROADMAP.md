# ROADMAP

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** unknown

Exact model backend not verifiable from final document text alone.

## Current Status
- [x] Wissenschaftliches Evaluations-Framework in `benchmarks/scripts/scientific_evaluation_framework.py` implementiert (Target: 2026-Q2)
- [x] Dokumentierter End-to-End-Playbook- und Katalogpfad in `benchmarks/docs/SCIENTIFIC_EVALUATION_FRAMEWORK.md` bereitgestellt (Target: 2026-Q2)
- [x] CI-Gate-Spezifikation für Framework-CLI als verpflichtender Gate-Job in `benchmarks/docs/CI_GATE.md` dokumentiert (Target: 2026-Q2)
- [x] Root-Benchmark-Dokumentation auf aktuelle Presets und CMake-Gating normalisiert (README, INDEX, AUDIT, SECURITY, ROADMAP) (Target: 2026-Q2)
- [x] Static coverage audit `tools/ci/audit_benchmark_coverage.py` — Re-Audit 2026-11-23: 389 Quellen entdeckt, 386 eligible benchmark targets mit CTest-Smoke registriert, 3 intentional excluded; zwei weitere stub-only diagnostics sind separat als normale CTest-Targets registriert. Die 51 unassigned paths sind ein src-Modul-Zuordnungsproblem, keine CMake-Registrierungslücke.
- [x] Syntax-Fehler (C-Block-Header) in `scientific_evaluation_framework.py` behoben (Target: 2026-Q2)
- [x] Wave-6-Release-Candidate-Suite inkl. finaler Guardrails/Drift-/Sättigungs-/Recovery-Auswertung über `benchmarks/ci_wave6_release_candidate_experiments.json` und Framework-Reportfelder umgesetzt (Target: 2026-Q3)

## In Progress
- [x] Final CTest post-processing for Google Benchmark smoke runs: `vscode-windows-benchmark-smokes` is serial, uses a result-archive fixture, and runs `compare_expectations.py` as the final cleanup step even after benchmark failures (Target: 2026-Q4)
- [~] Cross-database comparison readiness (Target: 2026-Q4)
  - [x] Define domain-specific YCSB/ANN-Benchmarks/VectorDBBench/ClickBench/LDBC SNB evidence requirements in `BENCHMARK_STANDARDS.md`.
  - [x] MSVC runner: process CPU/peak memory/I/O, run manifest, case filtering, repetitions, bounded execution and error-aware JSON acceptance; verified with importer smoke runs.
  - [x] Python YCSB-inspired A/B/C/F canonical HTTP adapter and CLI: corrected F mix, seeded operation selection, readback validation, p50/p95/p99 and owned cleanup. Twenty focused tests pass; isolated Community Debug live smokes completed all four workloads (100 records/200 operations each) with optional server-PID phase CPU/I/O and memory snapshots. No upstream conformance or production performance is claimed.
  - [x] Central complete/unified Python entry points use validated ThemisDB HTTP reads and writes for YCSB-inspired and scientific measurements; errors abort publication.
  - [x] Batch HTTP runner uses distinct per-case keys, validates every batch response and readback, and cleans attempted keys; the server registers the correct batch validation schema. An isolated live smoke verified 18 measured entities, cleanup of all 118 warmup/measured keys, and preservation of an unrelated record. It used a freshly compiled HTTP-server object linked to existing build artifacts and an explicitly raised rate limit; it is not RAID or performance evidence.
    - Wiki gap: no curated server module page; use the server API, validation middleware and handler sources as the authoritative context.
  - [x] Wikipedia JSONL runner measures actual ThemisDB entity PUT/readback latency and throughput with owned cleanup; canned query/stress success metrics removed.
  - [x] Wikipedia runner supports bounded concurrent entity write/readback pairs, reports configured concurrency, and keeps serial mode as the default.
  - [x] Former LLM/NLP simulation runner replaced by a ThemisDB vector HTTP workload with synthetic vectors, verified search self-matches and owned cleanup; no LLM/embedding/RAG claims.
  - [x] `specialized_comparative_benchmarks.py` disabled after removing fabricated competitor metrics, unchecked API calls, and unsupported winner/operational-cost claims; validated competitor drivers remain open.
  - [x] `polyglot_benchmark_simple.sh` disabled after removing unchecked HTTP calls and extrapolated competitor results; validated polyglot comparison remains open.
  - [x] Retired the linked 5GB shell load/stress coordinator after removing unsupported endpoints, false-success handling, incompatible database workloads, and canned conclusions.
  - [x] `benchmark_suite.sh` disabled after removing swallowed operation failures, average-derived p99 estimates, and unsupported comparative conclusions.
  - [x] `run_performance_benchmarks.sh` now validates nonempty Google Benchmark measurements, excludes invalid outputs, and returns nonzero when no executable measurement succeeds.
  - [x] Retired the Multi-Shard RAID PowerShell/Python entry points and quickstart because the Compose target was absent, RAID was not configured, HTTP operations were unverified, and resource metrics were placeholders.
  - [?] Implement a real local multi-process MIRROR benchmark (Target: Q4 2026).
    - Scope: start three isolated ThemisDB instances on separate loopback ports and data directories; configure MIRROR, replication factor 3 and write concern ALL. RAID parity, failover and recovery are outside this first scope.
    - Blocker: the modular CMake graph omitted `THEMIS_ENABLE_GRPC` from `themis_server` and the gRPC feature definitions plus `themis_shard_proto` link from `themis_network`. This wiring is now added when the gRPC and shard-proto targets are available. An isolated configure on 2026-11-18 generated the expected definitions for `main_server.cpp` and the RPC service sources; it did not build a binary or validate runtime behavior. The previous binary reported gRPC disabled, and the three-node smoke returned HTTP 200 on all `/health` checks but logged neither a gRPC listener nor RAID initialization. It aborted before issuing a write; no MIRROR result was produced. Rebuild and verify startup before retrying the three-node correctness gate.
    - Local diagnostic, 2026-11-17: three healthy processes configured for MIRROR/RF=3/ALL each logged a three-shard repair scan. One PUT returned HTTP 201; the originating instance returned HTTP 200 on readback, but both other instances returned HTTP 404. The scan is only indirect evidence of RAID initialization, not proof that ALL was enforced. This is a correctness failure, not a benchmark result.
    - Runtime prerequisite: verify authenticated inter-shard writes reach the replica persistence/index path, preserve security and entity semantics, and return success only after all three replicas acknowledge. On failure or timeout after a local commit, report an indeterminate partial state, not a rollback.
    - Correctness gate: for each successful benchmark write, read and compare its payload on all three distinct instances. Any failed write, failed or mismatched readback, unexpected topology, startup failure or cleanup failure prevents a successful result.
    - Measurement: report successful operation counts, errors, wall-clock throughput, and request-latency p50/p95/p99 with explicit units; distinguish measured requests from verification and report unavailable metrics as unavailable. Do not claim RAID levels other than MIRROR or external comparability.
    - Verification: cover missing/failed replicas, partial writes and mismatched readback, then execute a real three-process integration run with the selected server build. Use per-run owned directories and stop only processes started by the benchmark; preserve diagnostic evidence on failure.
  - [ ] Implement server-backed Wikipedia full-text/query and recovery scenarios against verified APIs before reporting those dimensions.
  - [ ] Implement and validate database-backed TPC-C, TPC-H, Sysbench and cross-database comparison drivers; the central entry points do not claim these results.
  - [ ] Integrate pinned reference drivers/adapters and dataset/query/ground-truth checksums; validate operation semantics and correctness before claiming external comparability.
  - [ ] Collect server-scoped resources, storage footprint and request latency histograms; keep client cost and setup/index build separate.
  - [ ] Complete the 273-binary resource pass; triage native crashes, hardware-dependent measurement errors and the gossip network/scalability timeout. The interrupted baseline pass is incomplete.
- [~] Wave-6-Operability-Härtung: reproduzierbarer Repro-/Triage-Runbook-Pfad für finale Go/No-Go-Entscheidungen (Target: 2026-Q3)
- [x] **Wave 4 (B4-A):** Release-kritische Governance-Benchmarks (W4A-01..W4A-06) + `release_gate_manifest.json` implementiert (Target: 2026-Q3)
- [x] **Wave 4 (B4-B):** Resilience/Degradations-Szenarien (W4B-01..W4B-06): Latenz-Injektion, Backpressure, CPU-Contention, Partial-Failure, Recovery (Target: 2026-Q3)
- [x] **Wave 4 (B4-C):** Determinismus- und Varianz-Benchmarks (W4C-01..W4C-06): CV-Akzeptanzgates, Seed-Stabilität, Warmup-Effektivität, Teardown-Isolation (Target: 2026-Q3)
- [x] **Wave 4 (B4-D):** Diagnose- und Reporting-Benchmarks (W4D-01..W4D-05) + `report_variance.py` + `RUNBOOK.md` implementiert (Target: 2026-Q3)

- [~] Welle 3 (B3-A/B3-B/B3-C): Full-Function Critical Workloads + Scale/Stress + Regression-Guardrails in `wave3_benchmark_suite.py` und `wave3_workload_profiles.json` (Target: 2026-Q3)
- [x] CMake-Registrierungsdrift für 11 Benchmark-Quellen (`wave5/`, `wave7/`, Root-Dispatch-Binaries) abgebaut — Re-Audit bestätigt 0 Unregistrierte (Target: 2026-Q3)
- [~] Schichtbezogene Benchmark-Matrix (ANN/Tensor/Graph) mit evidenzbasierten CPU-SIMD-vs-GPU-Szenarien, Break-Even-Schwellenwerten und Dynamic-Tensor-Update-Track (Issue: #5466, Target: 2026-Q3)
  - [x] Benchmark-Matrix-Dokument erstellt: `docs/benchmarks/CPU_SIMD_GPU_DISPATCH_BENCHMARK_MATRIX.md`
  - [x] Szenario-Katalog erstellt: `docs/benchmarks/CPU_SIMD_GPU_BENCHMARK_SCENARIO_CATALOG.md`
  - [~] Benchmark-Binaries implementieren: `bench_ann_cpu_gpu_dispatch` ✅, `bench_tensor_cpu_gpu_dispatch` ✅, `bench_graph_cpu_gpu_dispatch`, `bench_tensor_update_dispatch`, `bench_tensor_commit_overhead`, `bench_cross_cutting` (Target: 2026-Q3)
  - [~] CTest-Smoke-Targets für neue Binaries registrieren: `smoke_bench_ann_cpu_gpu_dispatch` ✅, `smoke_bench_tensor_cpu_gpu_dispatch` ✅, weitere Targets offen (Target: 2026-Q3)
  - [~] Hardware-Runs auf GPU-Runner für Break-Even-Validierung (Target: 2026-Q3) — Workflow `build-wave-b-llm-benchmarks.yml` erzeugt jetzt reale ANN/Tensor/LLM-Artefakte inkl. Break-Even-Report; repräsentative Self-Hosted-Hardware-Evidenz bleibt offen
  - [ ] Performance-Baselines in `benchmarks/baselines/` eintragen (Target: 2026-Q3)

## Planned Features
- [x] Phase-0 CRUD Baseline Benchmark Suite anlegen (P0-D04) — `benchmarks/phase0/` inkl. `bench_p0_crud_baseline.cpp` mit 4 Workloads (insert, read, update, delete) (Target: 2026-Q3)
- [x] Phase-0 Messpfad + Baseline-JSON implementiert: `phase0_fixtures.h` mit kanonischen Konstanten, `baseline_p0_v0.json` mit Erwartungen (Target: 2026-Q3)
- [x] Phase-0 Runbook & Measurement Protocol dokumentiert für manuellen + CI-Messpfad (Target: 2026-Q3)
- [ ] Persistente Historisierung von Eval-Reports für Trendanalysen über Releases hinweg (Target: 2026-Q3)
- [ ] Erweiterte Kostenmodelle (Cloud-Instance-Typen, Energie/KWh) in Gate-Entscheidungen integrieren (Target: 2026-Q3)
- [ ] Schichtbezogene Benchmark-Matrix (ANN/Tensor/Graph/LLM) mit evidenzbasierten Referenzsuites dokumentieren (Target: 2026-Q3)
- [~] Thematisch getrennte CI-Suiten (`baseline|core|index|search|llm`) über `.github/workflows/benchmark-thematic-suites.yml` + reusable Pipeline operationalisieren (Target: 2026-Q4)
  - [x] Suite-zu-Benchmark-Mapping in `benchmarks/benchmark_suite_map.json` versioniert
  - [x] Cadence split: baseline daily, heavy suites weekly
- [x] Baseline-Verzeichnis mit initialen Wave-4-Baseline-JSONs befüllen und in CI integrieren (Target: 2026-Q4)
- [~] GitHub Actions Workflow `.github/workflows/benchmark-gate.yml` als Wave-4-Smoke betreiben; ein echtes Performance-Gate bleibt bis zu validierter Baseline und vergleichbarer Hardware deaktiviert (Target: 2026-Q4)
- [x] `report_variance.py` als eigenständiges CI-Test-Target registrieren (Target: 2026-Q4)

## Benchmark source inventory and workload identity (Blocks A+B)

**Scope:** Keep the 389-source inventory auditable; distinguish source discovery,
CMake/CTest registration, measured execution class, correctness evidence, and
reference-workload conformance. Define stable workload IDs and machine-readable
profile fields without duplicating
[`BENCHMARK_STANDARDS.md`](BENCHMARK_STANDARDS.md) or
[`MEASUREMENT_HYGIENE.md`](MEASUREMENT_HYGIENE.md), and without introducing
performance targets.

**Current state:** The source-to-CMake-to-CTest static audit passes for 386
eligible benchmark sources; three sources are excluded from benchmark-tier
coverage. It also reports 51 registered paths that cannot be assigned to an
immediate `src/` module directory and two `src/` directories with no assigned
sources. These counts describe the dirty worktree snapshot and are not release
evidence. The detailed snapshot and source-backed triage are in
[`SOURCE_CATALOG.md`](SOURCE_CATALOG.md). The machine-readable profile contract
and its first source-described profiles are in
[`workload_profiles.schema.json`](workload_profiles.schema.json) and
[`workload_profiles.json`](workload_profiles.json). Workload-profile references
are linked from `benchmark_suite_map.json`; they are descriptive and do not
alter runner selection.

### Phases and tasks

1. **Inventory and attribution**
   - [x] Re-run source-to-CMake-to-CTest audit; record eligible and excluded
     denominators separately from source-to-module assignment.
   - [x] Record all 51 unassigned paths, no-source module directories, and the
     three benchmark-tier exclusions with their distinct CTest treatment.
   - [~] Review remaining sources individually and assign `micro`, `component`,
     `integration`, `e2e`, or `reference_workload` only from the measured code
     path and correctness checks; retain source/target/CTest evidence.
2. **Stable profile catalog**
   - [x] Define immutable `themisdb:<class>/<domain-workload>@<major>` IDs and
     require semantics, dataset, seed, concurrency, duration, warmup,
     correctness, metrics, environment, evidence outputs, source references,
     and limitations.
   - [x] Add source-described examples for the simulated Phase-0 workload,
     persisted MMDB component workload, and non-conformant HTTP YCSB-inspired
     workload. Use explicit nulls or caller-configured values where evidence is
     absent; define no performance targets.
   - [~] Expand the profile catalog only as source reviews verify semantics;
     do not allocate reference-workload IDs until upstream driver, protocol,
     datasets, and correctness evidence are pinned.
3. **Run evidence integration**
   - [x] Add descriptive profile-catalog references to the existing suite map
     without changing its binary filters.
   - [x] Add additive workload-ID references to CTest and MSVC run records only
     for exact, source-verifiable observed target/case pairs. Unknown and
     ambiguous cases remain `null`; no suite regex or filename heuristic is used.
     Current concrete links cover the four Phase-0 source cases and the four
     registered MMDB source cases (including their explicit Google Benchmark
     argument names).
   - [x] Validate profile schema, suite-map references, CTest case linkage and
     unknown/ambiguous/backward-compatible manifest behavior in focused static
     and runner tests.

### Acceptance criteria

- Every discovered source appears in the current static inventory; excluded,
  registered, module-assigned, and semantically classified counts are reported
  as separate denominators.
- Each source has one reviewed execution class or an explicit unresolved
  classification with an owner/action; registration and naming alone never
  determine the class.
- Simulated, stub, placeholder, synthetic, and label-only/non-failing gates
  are explicitly marked and excluded from production performance claims unless
  their measured path and gate behavior are independently verified.
- Stable profile IDs resolve from the suite map and include all required
  dimensions. Unknown values stay null/unknown, not zero or inferred defaults.
- Run manifests link an ID only when the exact observed case has a unique
  mapping. Raw result, manifest, source revision, configuration, and failed or
  skipped case evidence remain retained under the existing standards.
- Reference-workload classification requires pinned upstream protocol/driver
  and data provenance; `YCSB-inspired`, TPC-named, or other familiar labels are
  not compliance evidence.

### Dependencies

- The static inventory depends on `tools/ci/audit_benchmark_coverage.py`,
  source-tree module names, and benchmark CMake registration.
- Semantic class review depends on source-level inspection of measured APIs,
  setup/warmup boundaries, result validation, and gate failure behavior.
- Profile-to-manifest linkage depends on exact target/case mappings in the
  CTest and MSVC runners; suite regexes alone are not case mappings.
- Reference profiles depend on upstream revisions, legal/provenance review,
  pinned datasets and correctness/ground-truth checks.

### Known limitations

- The static audit proves discovery and registration, not production-path
  fidelity or workload correctness. Its 51 unassigned paths are module
  attribution gaps despite full eligible CTest registration.
- Semantic classification is not complete for all 389 sources; the catalog
  intentionally does not infer a class from filenames or folders.
- Profile references in the suite map are descriptive; CTest case records and
  MSVC normalized/repeatability
  records now add `workload_id` only for the explicitly source-listed Phase-0
  and MMDB exact target/case pairs. This is an additive field; no gate or
  workload-selection behavior changes.
- The Phase-0 profile is a mock-only simulation. The MMDB profile is source
  evidence only. The HTTP YCSB-inspired profile is not upstream YCSB and does
  not establish cross-database comparability.

## Core HTTP workload and CI governance (Blocks C+D)

- [~] Implement a single-node core workload using the real Entity API (Target: 2026-Q4).
  `core_http_mix.py` runs a seeded, bounded closed-loop CRUD mix with separate load, warmup,
  measurement, validation and owned-key cleanup phases. Request-level
  percentiles use individual HTTP timings. Contract tests and a two-record isolated live
  functional smoke passed on an existing copied server binary; this is neither
  evidence for the current worktree build nor a performance baseline.
- [~] Implement a real HTTP AQL equality workload with an owned secondary
  index and validated results (Target: 2026-Q4). Contract tests and an isolated
  three-record live smoke, including authenticated requests, passed against an
  existing copied server binary. Its reported indexed plan is not a physical
  I/O trace, and the smoke does not verify the current source build or
  production authentication policy.
- [x] Add source-described profile IDs for both HTTP workloads to the profile
  catalog, descriptive suite-map references and their JSON reports
  (Target: 2026-Q4). They do not alter Google Benchmark selection or claim CI
  execution; CTest/MSVC manifest linkage remains open.
- [?] Add a verified HTTP transaction workload only after transaction session
  finalization and rollback reporting are fixed and covered by production tests
  (Target: 2027-Q1). An isolated diagnostic observed a retained active session
  after a rolled-back batch and a successful response for an unknown rollback
  ID; it did not establish atomic visibility or durability. See
  `docs/CORE_HTTP_TRANSACTION_BLOCKER.md`. The required C++ symbol-info,
  reference and call-hierarchy tools were unavailable during this attempt;
  no production C++ fix or associated build was performed.
- [~] Capture separately scoped client and optional local server process
  snapshots and CPU/process-I/O deltas for each phase of the CRUD and AQL
  workloads (Target: 2026-Q4). Missing counters remain unavailable; endpoint
  memory samples are not phase peaks, and server PID/listener ownership is
  verified before writes. Isolated functional smokes observed both processes;
  they do not establish performance or current-build compatibility.
- [~] Add bounded concurrent mixed CRUD load with validated results and owned
  cleanup (Target: 2026-Q4). The default remains serial; an isolated
  four-worker functional smoke passed on an existing copied server binary.
  Additional workload profiles, scale and representative performance evidence
  remain open.
- [~] Add restart/durability checks where applicable; retain explicit failures
  and workload-specific correctness checks (Target: 2027-Q1). An opt-in,
  isolated Windows check verified clean shutdown and HTTP readback after restart
  on an existing copied server binary. Console membership checks are not atomic
  against concurrent attachment. Linux, current-build compatibility, crash and
  physical durability remain unverified; see
  `docs/CORE_HTTP_RESTART_DURABILITY_BLOCKER.md`.
- [~] Separate hosted Wave-4 smoke, expectations and thematic diagnostics
  from performance/release decisions (Target: 2026-Q4). The Wave-4 smoke checks
  all 13 registered W4A cases and five raw repetitions per case; it does not
  establish performance evidence.
- [~] Provide an opt-in identity and complete-case/repetition preflight in
  `tools/ci/inspect_benchmark_evidence.py` (Target: 2026-Q4). It checks
  declared baseline/current identities and raw result consistency, but does not
  attest declarations or enable a performance gate.
- [?] Bind executable and result identity to an isolated, bounded benchmark
  execution without overwriting existing results (Target: 2027-Q1). Existing
  runners do not establish this chain or Windows descendant containment; see
  `docs/BENCHMARK_EXECUTION_CHAIN_BLOCKER.md`.
- [x] Add optional paired local-observation consistency checks to the offline
  preflight (Target: 2026-Q4): compare compatible artifact/workload hashes and
  CPU/memory fields with each sidecar, result/catalog bytes with their hashes,
  and structured OS fingerprints with each other. Configuration hashes and
  the sidecar's free-form OS image have no unambiguous mapping and are not
  compared. Capture time, source, execution and host/file association remain
  unattested claims; this is not a gate or baseline approval.
- [x] Add an offline local identity-capture CLI for exact executable, runner,
  raw-result and effective-configuration hashes plus an observed Windows/Linux
  OS/CPU/memory fingerprint. Workload IDs require exact existing
  target/case/catalog resolution; the record is explicitly not attestation or
  a comparison sidecar and does not execute benchmarks (Target: 2026-Q4).
- [ ] Define dedicated runner identity, collect build/hardware fingerprints and
  reviewed comparable baseline evidence, and validate complete expected cases,
  repetitions and comparison policy before enabling performance gates
  (Target: 2027-Q1). Missing, failed, skipped or mismatched evidence must not
  pass. Record PR/nightly/heavy/release lane ownership and enforce the approved
  cadence only after this evidence exists.

The current diagnostic workflow names and reporter CLI change existing check
names and invocation behavior; branch-protection rules and direct callers must
be reviewed before rollout. See `docs/CORE_HTTP_MIX.md`,
`docs/CORE_HTTP_AQL_INDEX.md`,
`docs/CORE_HTTP_TRANSACTION_BLOCKER.md`,
`docs/CORE_HTTP_RESTART_DURABILITY_BLOCKER.md`, and
`.github/BENCHMARK_CI.md` for current scope and limitations.

## Implementation Phases
### Phase 1: Design / API-Vertrag
- [x] Hypothesenvertrag (H0/H1, Effekt-Richtung, Risiken, Stop-Kriterien) als strukturiertes JSON-Schema implementiert (Target: 2026-Q2)
- [x] Workload-Matrix-Validierung für `oltp|olap|graph|vector|rag|hybrid` inkl. Dataset/Query/Concurrency-Profil umgesetzt (Target: 2026-Q2)
- [x] Interferenzfaktoren (Warmup, Caching, NUMA, I/O, GPU) als Pflichtfelder im Szenariovertrag verankert (Target: 2026-Q2)

### Phase 2: Core-Implementierung
- [x] Seeded Runner für deterministische Bootstrap-/Permutation-Analysen implementiert (Target: 2026-Q2)
- [x] Baseline-Freeze-Validierung (Compiler, Flags, Preset, Hardware, OS) als harte Vorbedingung implementiert (Target: 2026-Q2)
- [x] Mindest-Samplegröße `n >= 30` pro Baseline/Treatment technisch erzwungen (Target: 2026-Q2)
- [~] W3-Suite: produktionsnahe Workload-Profile (`read-heavy|write-heavy|mixed`) mit priorisierten Critical-Flows implementiert (Target: 2026-Q3)

### Phase 3: Fehlerbehandlung & Edge Cases
- [x] Guardrails für fehlende/ungültige Felder und NaN/Inf-Samples implementiert (Target: 2026-Q2)
- [x] Fehlende oder fehlerhafte Experimente werden mit klaren Exceptions blockiert statt stillschweigend ignoriert (Target: 2026-Q2)
- [~] W3-Guardrails für offensichtliche Regressionen (`throughput_drop`, `p95`, `p99`) pro Workload-Profil ergänzt (Target: 2026-Q3)

### Phase 4: Tests
- [x] Fokussierte Unittests für Klassifikation, Determinismus, n>=30-Validierung und Gate/Ticket-Logik hinzugefügt (Target: 2026-Q2)
- [x] Regressionspfad (kritische Metrik verletzt Budget) via Ticket-Autogenerierung testabgedeckt (Target: 2026-Q2)

### Phase 5: Performance/Hardening
- [x] Statistikpfad mit Bootstrap-CI, Cohen's d, Cliff's Delta, Permutationstest und Practical Significance umgesetzt (Target: 2026-Q2)
- [x] Ergebnisklassifikation `regressiv|neutral|signifikant_positiv` für Merge-/Release-Gates eingeführt (Target: 2026-Q2)
- [x] Performance-Budget pro Experiment/Subsystem (`performance_budget_percent`) in Governance integriert (Target: 2026-Q2)

### Phase 6: Dokumentation & Abnahme
- [x] Experiment-Playbook inkl. Ausführung/Statistik/Gates dokumentiert (Target: 2026-Q2)
- [x] Benchmark-Katalog mit initialer Family/Metrik-Zuordnung dokumentiert (Target: 2026-Q2)
- [x] CI-Report- und evidenzbasierter Backlog-Mechanismus dokumentiert (Target: 2026-Q2)
- [x] Wave 4 Runbook (`benchmarks/wave4/RUNBOOK.md`) — Gates, Baselines, Varianz- und Diagnosekonzept, bekannte Restlücken (Target: 2026-Q3)

## Production Readiness Checklist
- [x] Definierte Fehlersemantik und Recovery-Pfade (harte Validierungsfehler)
- [x] Ausreichende Testabdeckung inkl. Regression für neue Framework-Logik
- [x] Security-Review: keine externen Netzwerkabhängigkeiten, nur lokale JSON-I/O
- [x] Monitoring/Observability via strukturierte JSON-Reports und Ticket-Outputs
- [x] Dokumentation für Betrieb und Entwicklung vollständig für das Framework-CLI
- [~] Wave-4-Messfälle W4A-01..W4A-06 und Manifest vorhanden; Performance-Gate bis zu validierter Baseline deaktiviert
- [x] Wave 4 Resilience/Degradation-Szenarien (W4B) für verwertbare Performance-Aussagen implementiert
- [x] Wave 4 Varianz-Akzeptanzkriterien (CV ≤ 15%) und Determinismus-Protokoll definiert und benchmarkabgedeckt

## Known Issues & Limitations
- [I] Statistische Verfahren basieren auf CPU-seitiger Python-Auswertung; sehr große Stichproben erhöhen Laufzeit deutlich.
- [I] `.github/workflows/benchmark-gate.yml` ist ein Smoke, kein aktives Wave-4-Performance-Gate; repräsentative Hardware und überprüfte Baselines fehlen weiterhin.
- [I] `baseline_v1.5.0.json` ist vorhanden, aber mangels vergleichbarer
  Hardware-/Build-Identität und dokumentierter Baseline-Evidenz nicht als
  Performance-Gate-Baseline freigegeben.
- [I] `BackpressureSimulator` in `wave4_fixtures.h` ist nicht thread-safe für parallele Benchmark-Bodies; für Multi-Thread-Szenarien pro Thread instanziieren.

## Breaking Changes
- Python YCSB now rejects missing operation adapters rather than simulating database operations. YCSB suite callers must supply `operation_adapters`; old adapter-free demo/full-suite invocations intentionally stop.
- Central complete/unified Python runners now require a ThemisDB HTTP endpoint and emit only measured scientific/YCSB-inspired results. The complete runner requires a new output path; unvalidated TPC-C/TPC-H/Sysbench/full modes stop instead of generating simulated results. Unified JSON uses `runs`, with `--operations` per workload and repetition; prior reports and schema are not interchangeable.
- `benchmark_batch_operations.py` now requires `--endpoint` and a new `--output` path; reports contain verified HTTP batch PUTs, not the former fixed-endpoint output or durability projections.
- `standard_benchmarks_integration.py` CLI now requires `--endpoint`, `--workload` and `--output` and runs only the canonical HTTP YCSB-inspired path. Workloads D/E are not supported by this adapter. Optional `--server-pid` observes a verified local listening process; no SDK namespace encoding is used.
- `llm_nlp_integration_test_suite.py` now runs only ThemisDB vector insert/search HTTP measurements over synthetic vectors. Its previous mock LLM/NLP scenarios and fixed-success outputs are removed; this runner does not measure embedding generation or RAG.
- Unverified YCSB expectation/ratio fields are now `null`; industry-grade comparisons are no longer emitted. Consumers must not interpret missing reference evidence as zero performance.
- Wave-4 `report_variance.py` requires `--no-gates` for diagnostic use; gate
  evaluation now fails closed until comparable baseline evidence is available.
  Benchmark workflow display/check names changed; update branch protection only
  after reviewing the replacement checks.
