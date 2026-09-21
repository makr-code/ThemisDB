# CHAOS DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\chaos\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\chaos\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 14
- Compounds: 41
- Classes/Structs: 10
- Namespaces: 8
- File Compounds: 14

## Namespaces
- @016364325202017356361365136124342130123343170230
- @054250161267217272261346060074345377204060112111
- @262100306263156221216106223356360343174013262061
- benchmark
- std::chrono_literals
- testing
- themis
- themis::chaos

## Types
### Classes
- ChaosSchedulerTest
- FaultInjectorFixture
- StubChaosScheduler
- StubFaultInjector
- themis::chaos::ChaosScheduler
- themis::chaos::FaultInjector

### Structs
- themis::chaos::ActiveFault
- themis::chaos::ChaosScheduleEntry
- themis::chaos::ChaosSchedulerConfig
- themis::chaos::FaultSpec

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 202

### ChaosSchedulerTest

#### `void SetUp() override`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:39
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:60
- Brief: n/a
- Parameters: none

#### `void makeScheduler(size_t max_concurrent=8)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:75
- Brief: n/a
- Parameters:
  - `max_concurrent` (size_t): n/a

#### `std::string registerFn(const std::string &name, std::function< nlohmann::json(const nlohmann::json &)> fn)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:88
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `fn` (std::function< nlohmann::json(const nlohmann::json &)>): n/a

### FaultInjectorFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### StubChaosScheduler

#### `bool schedule(const std::string &node_id, std::chrono::milliseconds)`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:119
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a
  - `<unnamed>` (std::chrono::milliseconds): n/a

#### `uint64_t scheduleCount() const noexcept`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:125
- Brief: n/a
- Parameters: none

#### `void start()`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:104
- Brief: n/a
- Parameters: none

#### `State state() const`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:114
- Brief: n/a
- Parameters: none

#### `void stop()`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:109
- Brief: n/a
- Parameters: none

### StubFaultInjector

#### `std::size_t activeCount() const`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:81
- Brief: n/a
- Parameters: none

#### `bool inject(const std::string &node_id, double probability=1.0)`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:60
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a
  - `probability` (double): n/a

#### `uint64_t injectCount() const noexcept`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:86
- Brief: n/a
- Parameters: none

#### `bool isActive(const std::string &node_id) const`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:76
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `bool recover(const std::string &node_id)`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:68
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a

#### `uint64_t recoverCount() const noexcept`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:87
- Brief: n/a
- Parameters: none

### bench_chaos_microbenchmarks.cpp

#### `BENCHMARK(BM_CHAOSМB01_InjectLatencyP99) -> UseRealTime() ->MinTime(1.0)`
- Source: `benchmarks/chaos/bench_chaos_microbenchmarks.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_CHAOSМB01_InjectLatencyP99): n/a

#### `BENCHMARK(BM_CHAOSМB02_RecoverLatencyP99) -> UseRealTime() ->MinTime(1.0)`
- Source: `benchmarks/chaos/bench_chaos_microbenchmarks.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_CHAOSМB02_RecoverLatencyP99): n/a

#### `BENCHMARK(BM_CHAOSМB03_ConcurrentFaultThroughput) -> UseRealTime() ->MinTime(1.0)`
- Source: `benchmarks/chaos/bench_chaos_microbenchmarks.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_CHAOSМB03_ConcurrentFaultThroughput): n/a

#### `void BM_CHAOSМB01_InjectLatencyP99(benchmark::State &state)`
- Source: `benchmarks/chaos/bench_chaos_microbenchmarks.cpp`:80
- Brief: CHAOS-MB-01 — injectFault() p99 latency microbenchmark.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Injects one unique fault per iteration (fresh node ID to avoid rejection due to duplicate active fault). Measures per-call latency for the fast registry-insertion path. Gate: p99 latency ≤ 5 µs. Counter: "gate_threshold_inject_p99_us"

#### `void BM_CHAOSМB02_RecoverLatencyP99(benchmark::State &state)`
- Source: `benchmarks/chaos/bench_chaos_microbenchmarks.cpp`:112
- Brief: CHAOS-MB-02 — recoverFault() p99 latency microbenchmark.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Pre-injects a fault in setup; measures only the recovery call latency. Resets the fault injector at the start of each iteration to ensure a stable pre-condition (fault always active before recover). Gate: p99 latency ≤ 5 µs. Counter: "gate_threshold_recover_p99_us"

#### `void BM_CHAOSМB03_ConcurrentFaultThroughput(benchmark::State &state)`
- Source: `benchmarks/chaos/bench_chaos_microbenchmarks.cpp`:142
- Brief: CHAOS-MB-03 — concurrent inject+recover throughput (4 threads).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Runs 4 threads, each performing inject→recover cycles on a disjoint set of node IDs. Measures combined throughput across all threads. Gate: ≥ 100 000 ops/s combined. Counter: "gate_threshold_concurrent_throughput_ops_per_sec"

#### `FaultSpec makeMBSpec(const std::string &node, FaultType type=FaultType::NODE_FAILURE)`
- Source: `benchmarks/chaos/bench_chaos_microbenchmarks.cpp`:63
- Brief: n/a
- Parameters:
  - `node` (const std::string &): n/a
  - `type` (FaultType): n/a

### bench_chaos_release_gates.cpp

#### `Arg(0) -> Arg(10) ->UseRealTime() ->MinTime(1.0)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a

#### `BENCHMARK(BM_GATE_CHS01_InjectThroughput) -> UseRealTime() ->MinTime(1.0)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GATE_CHS01_InjectThroughput): n/a

#### `BENCHMARK(BM_GATE_CHS02_QueryLatency) -> UseRealTime() ->MinTime(1.0)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GATE_CHS02_QueryLatency): n/a

#### `BENCHMARK(BM_GATE_CHS03_RecoverThroughput) -> UseRealTime() ->MinTime(1.0)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GATE_CHS03_RecoverThroughput): n/a

#### `BENCHMARK(BM_GATE_CHS04_ConcurrentInjectQuery) -> UseRealTime() ->Iterations(8)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GATE_CHS04_ConcurrentInjectQuery): n/a

#### `BENCHMARK(BM_GATE_CHS05_ScheduleThroughput) -> UseRealTime() ->MinTime(1.0)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_GATE_CHS05_ScheduleThroughput): n/a

#### `void BM_GATE_CHS01_InjectThroughput(benchmark::State &state)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:62
- Brief: GATE-CHS-01 — injectFault() throughput (single-threaded).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Injects one fault per unique node per iteration. Measures pure registry insertion throughput on the fast path (no expiry, no callbacks). Gate: ≥ 500 000 ops/s. Emitted as counter "gate_threshold_ops_per_sec".

#### `void BM_GATE_CHS02_QueryLatency(benchmark::State &state)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:92
- Brief: GATE-CHS-02 — isFaultActive() positive-path latency.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Queries a single permanently-active fault. Measures the per-call cost of the mutex-protected hash-map lookup on the hot path. Gate: average latency ≤ 1 µs. Gate threshold emitted as counter.

#### `void BM_GATE_CHS03_RecoverThroughput(benchmark::State &state)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:117
- Brief: GATE-CHS-03 — recoverFault() single-threaded throughput.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Injects a fault then immediately recovers it; measures the inject+recover round-trip cost as a proxy for recover throughput. Gate: ≥ 200 000 recover ops/s.

#### `void BM_GATE_CHS04_ConcurrentInjectQuery(benchmark::State &state)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:145
- Brief: GATE-CHS-04 — Concurrent inject + query throughput (8 threads).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Four inject threads and four query threads run concurrently against the same FaultInjector. Measures combined ops/s. Gate: ≥ 200 000 combined ops/s.

#### `void BM_GATE_CHS05_ScheduleThroughput(benchmark::State &state)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:205
- Brief: GATE-CHS-05 — ChaosScheduler schedule() throughput.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Enqueues entries with a far-future trigger_at while the scheduler is stopped to measure pure queue-insertion throughput without firing overhead. Gate: ≥ 100 000 schedule() ops/s.

#### `void BM_GATE_CHS06_CallbackOverhead(benchmark::State &state)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:241
- Brief: GATE-CHS-06 — Event callback dispatch overhead.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Compares injectFault() throughput with 0 callbacks (baseline) vs. 10 callbacks registered. Gate: ratio ≤ 2× (measured externally from JSON output). Parameterised via state.range(0) = number of callbacks.

#### `FaultSpec makeGateSpec(const std::string &node, FaultType type=FaultType::NODE_FAILURE)`
- Source: `benchmarks/chaos/bench_chaos_release_gates.cpp`:47
- Brief: n/a
- Parameters:
  - `node` (const std::string &): n/a
  - `type` (FaultType): n/a

### bench_chaos_stress.cpp

#### `Arg(0) -> Arg(8) ->Arg(64) ->Arg(256) ->Arg(1024)`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a

#### `Arg(1) -> Arg(2) ->Arg(4) ->Arg(8) ->Arg(16) ->UseRealTime() ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `BENCHMARK(BM_CallbackDispatch) -> Arg(0) ->Arg(1) ->Arg(5) ->Arg(10)`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_CallbackDispatch): n/a

#### `BENCHMARK(BM_ChaosScheduler_Schedule)`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ChaosScheduler_Schedule): n/a

#### `BENCHMARK_F(FaultInjectorFixture, ActiveFaultCount_Throughput)(benchmark`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorFixture): n/a
  - `<unnamed>` (ActiveFaultCount_Throughput): n/a

#### `BENCHMARK_F(FaultInjectorFixture, ExpiredFaultPruning)(benchmark`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorFixture): n/a
  - `<unnamed>` (ExpiredFaultPruning): n/a

#### `BENCHMARK_F(FaultInjectorFixture, GetActiveFaults_HighChurn)(benchmark`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorFixture): n/a
  - `<unnamed>` (GetActiveFaults_HighChurn): n/a

#### `BENCHMARK_F(FaultInjectorFixture, InjectFault_AllTypes)(benchmark`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorFixture): n/a
  - `<unnamed>` (InjectFault_AllTypes): n/a

#### `BENCHMARK_F(FaultInjectorFixture, InjectFault_Throughput)(benchmark`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorFixture): n/a
  - `<unnamed>` (InjectFault_Throughput): n/a

#### `BENCHMARK_F(FaultInjectorFixture, IsFaultActive_Negative)(benchmark`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorFixture): n/a
  - `<unnamed>` (IsFaultActive_Negative): n/a

#### `BENCHMARK_F(FaultInjectorFixture, IsFaultActive_Positive)(benchmark`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorFixture): n/a
  - `<unnamed>` (IsFaultActive_Positive): n/a

#### `BENCHMARK_F(FaultInjectorFixture, RecoverFault_Throughput)(benchmark`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorFixture): n/a
  - `<unnamed>` (RecoverFault_Throughput): n/a

#### `void BM_CallbackDispatch(benchmark::State &state)`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:196
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ChaosScheduler_Schedule(benchmark::State &state)`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:270
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ConcurrentStress(benchmark::State &state)`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:224
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `FaultSpec makeSpec(const std::string &node_id, FaultType type, int ttl_ms=0)`
- Source: `benchmarks/chaos/bench_chaos_stress.cpp`:47
- Brief: Build a FaultSpec with a short TTL so it expires quickly.
- Parameters:
  - `node_id` (const std::string &): n/a
  - `type` (FaultType): n/a
  - `ttl_ms` (int): n/a

### test_chaos_callback_determinism.cpp

#### `TEST(ChaosCallbackDeterminismTest, CCD01_RecoverCallbackFires)`
- Source: `tests/chaos/test_chaos_callback_determinism.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosCallbackDeterminismTest): n/a
  - `<unnamed>` (CCD01_RecoverCallbackFires): n/a
- Details: TestCCD-01 — The event callback is called with injected=false for every successful recoverFault() call.

#### `TEST(ChaosCallbackDeterminismTest, CCD02_DuplicateInjectLWWSemanticsFiresCallbackAgain)`
- Source: `tests/chaos/test_chaos_callback_determinism.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosCallbackDeterminismTest): n/a
  - `<unnamed>` (CCD02_DuplicateInjectLWWSemanticsFiresCallbackAgain): n/a
- Details: TestCCD-02 — injectFault() on an already-active (node, type) pair uses last-writer-wins (LWW) semantics: it updates the existing entry, returns true, and fires the callback a second time. This matches the implementation contract: re-injection extends/refreshes the active fault rather than being silently rejected.

#### `TEST(ChaosCallbackDeterminismTest, CCD03_EmptyNodeIdRejectedNoCallback)`
- Source: `tests/chaos/test_chaos_callback_determinism.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosCallbackDeterminismTest): n/a
  - `<unnamed>` (CCD03_EmptyNodeIdRejectedNoCallback): n/a
- Details: TestCCD-03 — injectFault() with an empty node_id returns false without touching the registry or firing any callback. chaos_contract.h § 1 — kMinNodeIdBytes = 1

#### `TEST(ChaosCallbackDeterminismTest, CCD04_InvalidProbabilityRejectedNoCallback)`
- Source: `tests/chaos/test_chaos_callback_determinism.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosCallbackDeterminismTest): n/a
  - `<unnamed>` (CCD04_InvalidProbabilityRejectedNoCallback): n/a
- Details: TestCCD-04 — injectFault() with probability outside [0.0, 1.0] returns false and does not fire any callback. chaos_contract.h § 1 — kMinProbability / kMaxProbability

#### `TEST(ChaosCallbackDeterminismTest, CCD05_ClearAllFaultsNoCallbacks)`
- Source: `tests/chaos/test_chaos_callback_determinism.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosCallbackDeterminismTest): n/a
  - `<unnamed>` (CCD05_ClearAllFaultsNoCallbacks): n/a
- Details: TestCCD-05 — clearAllFaults() removes all faults from the registry without invoking any event callbacks. chaos_contract.h § 5 — callbacks are invoked only on inject/recover

#### `TEST(ChaosCallbackDeterminismTest, CCD06_ScheduleInZeroDelayFires)`
- Source: `tests/chaos/test_chaos_callback_determinism.cpp`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosCallbackDeterminismTest): n/a
  - `<unnamed>` (CCD06_ScheduleInZeroDelayFires): n/a
- Details: TestCCD-06 — scheduleIn() with a zero-or-minimal delay fires the fault through the injector within one tick window after start(). chaos_contract.h § 6 — scheduler state contract

#### `TEST(ChaosCallbackDeterminismTest, CCD07_ClearPendingThenLifecycle)`
- Source: `tests/chaos/test_chaos_callback_determinism.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosCallbackDeterminismTest): n/a
  - `<unnamed>` (CCD07_ClearPendingThenLifecycle): n/a
- Details: TestCCD-07 — clearPending() reduces pendingCount() to zero, and the scheduler can be started and stopped cleanly afterwards. chaos_contract.h § 6 — clearPending() is safe from any state

#### `TEST(ChaosCallbackDeterminismTest, CCD08_MultipleSchedulesSameNodeFireSequentially)`
- Source: `tests/chaos/test_chaos_callback_determinism.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosCallbackDeterminismTest): n/a
  - `<unnamed>` (CCD08_MultipleSchedulesSameNodeFireSequentially): n/a
- Details: TestCCD-08 — Scheduling multiple faults for the same node with staggered delays fires them sequentially; the injector ends with the last fault active (earlier ones are overwritten if same node+type, or co-exist if different types). chaos_contract.h § 6 — pending entries fire in temporal order

### test_chaos_concurrency_hardening.cpp

#### `TEST(ChaosConcurrencyHardeningTest, CCH01_ConcurrentInjectNoRace)`
- Source: `tests/chaos/test_chaos_concurrency_hardening.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConcurrencyHardeningTest): n/a
  - `<unnamed>` (CCH01_ConcurrentInjectNoRace): n/a
- Details: TestCCH-01 — Concurrent injectFault() from N threads does not crash and results in a consistent active fault count. Each thread injects a unique node; after join, activeFaultCount() must equal the number of successful injections.

#### `TEST(ChaosConcurrencyHardeningTest, CCH02_ConcurrentRecoverDuringInject)`
- Source: `tests/chaos/test_chaos_concurrency_hardening.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConcurrencyHardeningTest): n/a
  - `<unnamed>` (CCH02_ConcurrentRecoverDuringInject): n/a
- Details: TestCCH-02 — Concurrent recoverFault() while inject threads are running produces no crash or data race.

#### `TEST(ChaosConcurrencyHardeningTest, CCH03_ConcurrentQueryDuringMutations)`
- Source: `tests/chaos/test_chaos_concurrency_hardening.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConcurrencyHardeningTest): n/a
  - `<unnamed>` (CCH03_ConcurrentQueryDuringMutations): n/a
- Details: TestCCH-03 — isFaultActive() called concurrently with inject and recover threads always returns a valid boolean (no throw, no crash).

#### `TEST(ChaosConcurrencyHardeningTest, CCH04_CallbackFiresOncePerInject)`
- Source: `tests/chaos/test_chaos_concurrency_hardening.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConcurrencyHardeningTest): n/a
  - `<unnamed>` (CCH04_CallbackFiresOncePerInject): n/a
- Details: TestCCH-04 — A callback registered before inject fires exactly once per successful injectFault() call.

#### `TEST(ChaosConcurrencyHardeningTest, CCH05_MultipleCallbacksFIFOOrder)`
- Source: `tests/chaos/test_chaos_concurrency_hardening.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConcurrencyHardeningTest): n/a
  - `<unnamed>` (CCH05_MultipleCallbacksFIFOOrder): n/a
- Details: TestCCH-05 — Multiple registered callbacks all fire in FIFO registration order on a single injectFault() call.

#### `TEST(ChaosConcurrencyHardeningTest, CCH06_LateCallbackDoesNotFireRetroactively)`
- Source: `tests/chaos/test_chaos_concurrency_hardening.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConcurrencyHardeningTest): n/a
  - `<unnamed>` (CCH06_LateCallbackDoesNotFireRetroactively): n/a
- Details: TestCCH-06 — A callback registered after injectFault() does NOT fire retroactively for the already-injected fault.

#### `TEST(ChaosConcurrencyHardeningTest, CCH07_ConcurrentClearAndInject)`
- Source: `tests/chaos/test_chaos_concurrency_hardening.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConcurrencyHardeningTest): n/a
  - `<unnamed>` (CCH07_ConcurrentClearAndInject): n/a
- Details: TestCCH-07 — clearAllFaults() called concurrently with inject threads does not crash and leaves the registry in a consistent (empty or partially-populated) state after all threads complete.

#### `TEST(ChaosConcurrencyHardeningTest, CCH08_GetActiveFaultsSnapshotUnderChurn)`
- Source: `tests/chaos/test_chaos_concurrency_hardening.cpp`:321
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConcurrencyHardeningTest): n/a
  - `<unnamed>` (CCH08_GetActiveFaultsSnapshotUnderChurn): n/a
- Details: TestCCH-08 — getActiveFaults() snapshot is safe to call under concurrent inject and recover churn: no crash, no stale dangling entry.

### test_chaos_framework.cpp

#### `TEST(ChaosSchedulerTest, ClearPendingRemovesAllEntries)`
- Source: `tests/chaos/test_chaos_framework.cpp`:266
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (ClearPendingRemovesAllEntries): n/a

#### `TEST(ChaosSchedulerTest, DoubleStartIsIdempotent)`
- Source: `tests/chaos/test_chaos_framework.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (DoubleStartIsIdempotent): n/a

#### `TEST(ChaosSchedulerTest, NullInjectorThrows)`
- Source: `tests/chaos/test_chaos_framework.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (NullInjectorThrows): n/a

#### `TEST(ChaosSchedulerTest, PendingCountReflectsScheduledEntries)`
- Source: `tests/chaos/test_chaos_framework.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (PendingCountReflectsScheduledEntries): n/a

#### `TEST(ChaosSchedulerTest, ScheduledFaultFiresViaInjector)`
- Source: `tests/chaos/test_chaos_framework.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (ScheduledFaultFiresViaInjector): n/a

#### `TEST(ChaosSchedulerTest, StartAndStopLifecycle)`
- Source: `tests/chaos/test_chaos_framework.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (StartAndStopLifecycle): n/a

#### `TEST(ChaosSchedulerTest, StartsNotRunning)`
- Source: `tests/chaos/test_chaos_framework.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (StartsNotRunning): n/a

#### `TEST(ChaosSchedulerTest, StopDoesNotFirePendingFaults)`
- Source: `tests/chaos/test_chaos_framework.cpp`:275
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (StopDoesNotFirePendingFaults): n/a

#### `TEST(FaultInjectorTest, CallbackFiredOnInject)`
- Source: `tests/chaos/test_chaos_framework.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (CallbackFiredOnInject): n/a

#### `TEST(FaultInjectorTest, CallbackFiredOnRecover)`
- Source: `tests/chaos/test_chaos_framework.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (CallbackFiredOnRecover): n/a

#### `TEST(FaultInjectorTest, ClearAllFaultsEmptiesRegistry)`
- Source: `tests/chaos/test_chaos_framework.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (ClearAllFaultsEmptiesRegistry): n/a

#### `TEST(FaultInjectorTest, CustomIdIsPreserved)`
- Source: `tests/chaos/test_chaos_framework.cpp`:40
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (CustomIdIsPreserved): n/a

#### `TEST(FaultInjectorTest, DefaultIdIsDefault)`
- Source: `tests/chaos/test_chaos_framework.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (DefaultIdIsDefault): n/a

#### `TEST(FaultInjectorTest, EmptyNodeIdIsRejected)`
- Source: `tests/chaos/test_chaos_framework.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (EmptyNodeIdIsRejected): n/a

#### `TEST(FaultInjectorTest, ExpiredFaultIsNotActive)`
- Source: `tests/chaos/test_chaos_framework.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (ExpiredFaultIsNotActive): n/a

#### `TEST(FaultInjectorTest, GetActiveFaultsSnapshotSize)`
- Source: `tests/chaos/test_chaos_framework.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (GetActiveFaultsSnapshotSize): n/a

#### `TEST(FaultInjectorTest, InjectNodeFailureIsActive)`
- Source: `tests/chaos/test_chaos_framework.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (InjectNodeFailureIsActive): n/a

#### `TEST(FaultInjectorTest, InjectedFaultCountIsOne)`
- Source: `tests/chaos/test_chaos_framework.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (InjectedFaultCountIsOne): n/a

#### `TEST(FaultInjectorTest, InvalidProbabilityIsRejected)`
- Source: `tests/chaos/test_chaos_framework.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (InvalidProbabilityIsRejected): n/a

#### `TEST(FaultInjectorTest, PermanentFaultRemainsActive)`
- Source: `tests/chaos/test_chaos_framework.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (PermanentFaultRemainsActive): n/a

#### `TEST(FaultInjectorTest, RecoverAllFaultsOnNode)`
- Source: `tests/chaos/test_chaos_framework.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (RecoverAllFaultsOnNode): n/a

#### `TEST(FaultInjectorTest, RecoverNonexistentFaultReturnsFalse)`
- Source: `tests/chaos/test_chaos_framework.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (RecoverNonexistentFaultReturnsFalse): n/a

#### `TEST(FaultInjectorTest, RecoverSpecificFaultType)`
- Source: `tests/chaos/test_chaos_framework.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (RecoverSpecificFaultType): n/a

#### `TEST(FaultInjectorTest, SameNodeTwoDifferentTypesAreBothActive)`
- Source: `tests/chaos/test_chaos_framework.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (SameNodeTwoDifferentTypesAreBothActive): n/a

#### `TEST(FaultInjectorTest, StartsWithZeroFaults)`
- Source: `tests/chaos/test_chaos_framework.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (StartsWithZeroFaults): n/a

#### `TEST(FaultInjectorTest, TwoDifferentNodesAreBothActive)`
- Source: `tests/chaos/test_chaos_framework.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (TwoDifferentNodesAreBothActive): n/a

#### `TEST(FaultInjectorTest, UnaffectedNodeIsNotActive)`
- Source: `tests/chaos/test_chaos_framework.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (FaultInjectorTest): n/a
  - `<unnamed>` (UnaffectedNodeIsNotActive): n/a

### test_chaos_highcardinality_stress.cpp

#### `TEST(WaveD_ChaosHighCardinalityStress, ConcurrentInjectRecoverStress)`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_ChaosHighCardinalityStress): n/a
  - `<unnamed>` (ConcurrentInjectRecoverStress): n/a

#### `TEST(WaveD_ChaosHighCardinalityStress, HighCardinalityFaultDescriptors)`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_ChaosHighCardinalityStress): n/a
  - `<unnamed>` (HighCardinalityFaultDescriptors): n/a

#### `TEST(WaveD_ChaosHighCardinalityStress, SchedulerHighConcurrencyLoad)`
- Source: `tests/chaos/test_chaos_highcardinality_stress.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_ChaosHighCardinalityStress): n/a
  - `<unnamed>` (SchedulerHighConcurrencyLoad): n/a

### test_chaos_network.cpp

#### `TEST(ChaosAccessLog, ClientIpFromXForwardedFor)`
- Source: `tests/chaos/test_chaos_network.cpp`:306
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosAccessLog): n/a
  - `<unnamed>` (ClientIpFromXForwardedFor): n/a

#### `TEST(ChaosAccessLog, ClientIpFromXRealIp)`
- Source: `tests/chaos/test_chaos_network.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosAccessLog): n/a
  - `<unnamed>` (ClientIpFromXRealIp): n/a

#### `TEST(ChaosAccessLog, RequestIdIsGeneratedWhenMissing)`
- Source: `tests/chaos/test_chaos_network.cpp`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosAccessLog): n/a
  - `<unnamed>` (RequestIdIsGeneratedWhenMissing): n/a

#### `TEST(ChaosAccessLog, RequestIdIsPreservedWhenPresent)`
- Source: `tests/chaos/test_chaos_network.cpp`:300
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosAccessLog): n/a
  - `<unnamed>` (RequestIdIsPreservedWhenPresent): n/a

#### `TEST(ChaosConnectionCounter, ConcurrentConnectsAndDisconnects)`
- Source: `tests/chaos/test_chaos_network.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConnectionCounter): n/a
  - `<unnamed>` (ConcurrentConnectsAndDisconnects): n/a

#### `TEST(ChaosConnectionCounter, DecrementOnDisconnect)`
- Source: `tests/chaos/test_chaos_network.cpp`:243
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConnectionCounter): n/a
  - `<unnamed>` (DecrementOnDisconnect): n/a

#### `TEST(ChaosConnectionCounter, IncrementOnConnect)`
- Source: `tests/chaos/test_chaos_network.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConnectionCounter): n/a
  - `<unnamed>` (IncrementOnConnect): n/a

#### `TEST(ChaosConnectionCounter, MaxConnectionsEnforcement)`
- Source: `tests/chaos/test_chaos_network.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConnectionCounter): n/a
  - `<unnamed>` (MaxConnectionsEnforcement): n/a

#### `TEST(ChaosConnectionCounter, StartsAtZero)`
- Source: `tests/chaos/test_chaos_network.cpp`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosConnectionCounter): n/a
  - `<unnamed>` (StartsAtZero): n/a

#### `TEST(ChaosHealthEndpoint, ConcurrentLivenessCalls)`
- Source: `tests/chaos/test_chaos_network.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosHealthEndpoint): n/a
  - `<unnamed>` (ConcurrentLivenessCalls): n/a

#### `TEST(ChaosHealthEndpoint, LivenessRapidCalls)`
- Source: `tests/chaos/test_chaos_network.cpp`:322
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosHealthEndpoint): n/a
  - `<unnamed>` (LivenessRapidCalls): n/a

#### `TEST(ChaosHealthEndpoint, ReadinessRapidCalls)`
- Source: `tests/chaos/test_chaos_network.cpp`:332
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosHealthEndpoint): n/a
  - `<unnamed>` (ReadinessRapidCalls): n/a

#### `TEST(ChaosNetworkConfig, MaxConnectionsCanBeSet)`
- Source: `tests/chaos/test_chaos_network.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosNetworkConfig): n/a
  - `<unnamed>` (MaxConnectionsCanBeSet): n/a

#### `TEST(ChaosNetworkConfig, MaxConnectionsLimitOf1)`
- Source: `tests/chaos/test_chaos_network.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosNetworkConfig): n/a
  - `<unnamed>` (MaxConnectionsLimitOf1): n/a

#### `TEST(ChaosNetworkConfig, ZeroBodySizeAllowsAnyBody)`
- Source: `tests/chaos/test_chaos_network.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosNetworkConfig): n/a
  - `<unnamed>` (ZeroBodySizeAllowsAnyBody): n/a

#### `TEST(ChaosNetworkConfig, ZeroHeaderSizeDisablesCheck)`
- Source: `tests/chaos/test_chaos_network.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosNetworkConfig): n/a
  - `<unnamed>` (ZeroHeaderSizeDisablesCheck): n/a

#### `TEST(ChaosNetworkConfig, ZeroMaxConnectionsMeansUnlimited)`
- Source: `tests/chaos/test_chaos_network.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosNetworkConfig): n/a
  - `<unnamed>` (ZeroMaxConnectionsMeansUnlimited): n/a

#### `TEST(ChaosOversizedBody, EmptyBodyIsAlwaysAllowed)`
- Source: `tests/chaos/test_chaos_network.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosOversizedBody): n/a
  - `<unnamed>` (EmptyBodyIsAlwaysAllowed): n/a

#### `TEST(ChaosOversizedBody, ExactlyAtLimitIsAllowed)`
- Source: `tests/chaos/test_chaos_network.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosOversizedBody): n/a
  - `<unnamed>` (ExactlyAtLimitIsAllowed): n/a

#### `TEST(ChaosOversizedBody, ExceedsDefaultBodyLimit)`
- Source: `tests/chaos/test_chaos_network.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosOversizedBody): n/a
  - `<unnamed>` (ExceedsDefaultBodyLimit): n/a

#### `TEST(ChaosOversizedBody, OneByteOverLimitIsRejected)`
- Source: `tests/chaos/test_chaos_network.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosOversizedBody): n/a
  - `<unnamed>` (OneByteOverLimitIsRejected): n/a

#### `TEST(ChaosOversizedHeaders, ExactlyAtHeaderLimitIsAllowed)`
- Source: `tests/chaos/test_chaos_network.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosOversizedHeaders): n/a
  - `<unnamed>` (ExactlyAtHeaderLimitIsAllowed): n/a

#### `TEST(ChaosOversizedHeaders, ExceedsDefaultHeaderLimit)`
- Source: `tests/chaos/test_chaos_network.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosOversizedHeaders): n/a
  - `<unnamed>` (ExceedsDefaultHeaderLimit): n/a

#### `TEST(ChaosOversizedHeaders, OneByteOverHeaderLimitIsRejected)`
- Source: `tests/chaos/test_chaos_network.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosOversizedHeaders): n/a
  - `<unnamed>` (OneByteOverHeaderLimitIsRejected): n/a

#### `TEST(ChaosOversizedHeaders, ZeroHeaderLimitDisablesCheck)`
- Source: `tests/chaos/test_chaos_network.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosOversizedHeaders): n/a
  - `<unnamed>` (ZeroHeaderLimitDisablesCheck): n/a

#### `TEST(ChaosPayloads, DeeplyNestedJsonDoesNotCrash)`
- Source: `tests/chaos/test_chaos_network.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosPayloads): n/a
  - `<unnamed>` (DeeplyNestedJsonDoesNotCrash): n/a

#### `TEST(ChaosPayloads, EmptyBodyIsValidForGetRequests)`
- Source: `tests/chaos/test_chaos_network.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosPayloads): n/a
  - `<unnamed>` (EmptyBodyIsValidForGetRequests): n/a

#### `TEST(ChaosPayloads, InvalidUtf8InBodyDoesNotCrash)`
- Source: `tests/chaos/test_chaos_network.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosPayloads): n/a
  - `<unnamed>` (InvalidUtf8InBodyDoesNotCrash): n/a

#### `TEST(ChaosPayloads, MalformedJsonBodyDoesNotCrash)`
- Source: `tests/chaos/test_chaos_network.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosPayloads): n/a
  - `<unnamed>` (MalformedJsonBodyDoesNotCrash): n/a

#### `TEST(ChaosPayloads, TruncatedJsonBodyDoesNotCrash)`
- Source: `tests/chaos/test_chaos_network.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosPayloads): n/a
  - `<unnamed>` (TruncatedJsonBodyDoesNotCrash): n/a

#### `TEST(ChaosPayloads, ValidEmptyJsonArray)`
- Source: `tests/chaos/test_chaos_network.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosPayloads): n/a
  - `<unnamed>` (ValidEmptyJsonArray): n/a

#### `TEST(ChaosPayloads, ValidEmptyJsonObject)`
- Source: `tests/chaos/test_chaos_network.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosPayloads): n/a
  - `<unnamed>` (ValidEmptyJsonObject): n/a

#### `std::unique_ptr< themis::server::MonitoringApiHandler > make_monitoring_handler()`
- Source: `tests/chaos/test_chaos_network.cpp`:53
- Brief: n/a
- Parameters: none

#### `http::request< http::string_body > make_request(http::verb method, const std::string &target, const std::string &body="", const std::string &content_type="application/json")`
- Source: `tests/chaos/test_chaos_network.cpp`:37
- Brief: n/a
- Parameters:
  - `method` (http::verb): n/a
  - `target` (const std::string &): n/a
  - `body` (const std::string &): n/a
  - `content_type` (const std::string &): n/a

### test_chaos_scheduler.cpp

#### `TEST_F(ChaosSchedulerTest, AllTasksFailSimultaneously)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:233
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (AllTasksFailSimultaneously): n/a

#### `TEST_F(ChaosSchedulerTest, ConcurrentManualExecutionsSameTask)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (ConcurrentManualExecutionsSameTask): n/a

#### `TEST_F(ChaosSchedulerTest, ConcurrentRegistrationWhileSchedulerRunning)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:469
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (ConcurrentRegistrationWhileSchedulerRunning): n/a

#### `TEST_F(ChaosSchedulerTest, DuplicateTaskNameGetsDifferentId)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:445
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (DuplicateTaskNameGetsDifferentId): n/a

#### `TEST_F(ChaosSchedulerTest, ExportMetricsUnderConcurrentModificationDoesNotCrash)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:513
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (ExportMetricsUnderConcurrentModificationDoesNotCrash): n/a

#### `TEST_F(ChaosSchedulerTest, HighThroughputManualExecutionMeetsBaseline)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:351
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (HighThroughputManualExecutionMeetsBaseline): n/a

#### `TEST_F(ChaosSchedulerTest, LargeNumberOfRegisteredTasksRemainAccessible)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (LargeNumberOfRegisteredTasksRemainAccessible): n/a

#### `TEST_F(ChaosSchedulerTest, MixedSuccessFailureTasksUnderLoop)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (MixedSuccessFailureTasksUnderLoop): n/a

#### `TEST_F(ChaosSchedulerTest, RapidEnableDisableUnderLoad)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (RapidEnableDisableUnderLoad): n/a

#### `TEST_F(ChaosSchedulerTest, RapidRegisterUnregisterDoesNotCorruptState)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (RapidRegisterUnregisterDoesNotCorruptState): n/a

#### `TEST_F(ChaosSchedulerTest, StopWhileTasksRunningDoesNotDeadlock)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (StopWhileTasksRunningDoesNotDeadlock): n/a

#### `TEST_F(ChaosSchedulerTest, TaskThrowingNonStdExceptionHandledGracefully)`
- Source: `tests/chaos/test_chaos_scheduler.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTest): n/a
  - `<unnamed>` (TaskThrowingNonStdExceptionHandledGracefully): n/a

### test_chaos_scheduler_timing.cpp

#### `TEST(ChaosSchedulerTimingTest, CTI01_ImmediateStopAfterStart)`
- Source: `tests/chaos/test_chaos_scheduler_timing.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTimingTest): n/a
  - `<unnamed>` (CTI01_ImmediateStopAfterStart): n/a
- Details: TestCTI-01 — stop() called immediately after start() must return within kSchedulerStopTimeout and leave isRunning() == false. chaos_contract.h § 6 — kSchedulerStopTimeout

#### `TEST(ChaosSchedulerTimingTest, CTI02_RepeatRestartCycle)`
- Source: `tests/chaos/test_chaos_scheduler_timing.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTimingTest): n/a
  - `<unnamed>` (CTI02_RepeatRestartCycle): n/a
- Details: TestCTI-02 — Repeated stop + start cycles are stable; the scheduler can be restarted after every stop without crashing or leaking resources.

#### `TEST(ChaosSchedulerTimingTest, CTI03_ScheduleAfterStopNotFired)`
- Source: `tests/chaos/test_chaos_scheduler_timing.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTimingTest): n/a
  - `<unnamed>` (CTI03_ScheduleAfterStopNotFired): n/a
- Details: TestCTI-03 — Entries added with scheduleIn() when the scheduler is STOPPED are added to the pending queue but MUST NOT fire until start() is called. chaos_contract.h § 6 — scheduleIn() on STOPPED scheduler may queue entry

#### `TEST(ChaosSchedulerTimingTest, CTI04_DoubleStartIdempotent)`
- Source: `tests/chaos/test_chaos_scheduler_timing.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTimingTest): n/a
  - `<unnamed>` (CTI04_DoubleStartIdempotent): n/a
- Details: TestCTI-04 — Calling start() while the scheduler is already running is idempotent: it must not launch a second worker thread or crash. chaos_contract.h § 6 — start() on RUNNING is idempotent

#### `TEST(ChaosSchedulerTimingTest, CTI05_StopOnStoppedIdempotent)`
- Source: `tests/chaos/test_chaos_scheduler_timing.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTimingTest): n/a
  - `<unnamed>` (CTI05_StopOnStoppedIdempotent): n/a
- Details: TestCTI-05 — Calling stop() when the scheduler has never been started (or has already been stopped) must be a no-op without throwing. chaos_contract.h § 6 — stop() on STOPPED is idempotent

#### `TEST(ChaosSchedulerTimingTest, CTI06_CondvarStopLatencyBounded)`
- Source: `tests/chaos/test_chaos_scheduler_timing.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTimingTest): n/a
  - `<unnamed>` (CTI06_CondvarStopLatencyBounded): n/a
- Details: TestCTI-06 — With WakeStrategy::CONDVAR, stop() completes within themis::chaos::kSchedulerStopTimeout regardless of tick_interval. chaos_contract.h § 6 — kSchedulerStopTimeout = 500 ms

#### `TEST(ChaosSchedulerTimingTest, CTI07_FixedTickFaultFiresInWindow)`
- Source: `tests/chaos/test_chaos_scheduler_timing.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTimingTest): n/a
  - `<unnamed>` (CTI07_FixedTickFaultFiresInWindow): n/a
- Details: TestCTI-07 — With WakeStrategy::FIXED_TICK, a scheduled fault fires within the expected deadline (delay + 3 × tick_interval as headroom).

#### `TEST(ChaosSchedulerTimingTest, CTI08_TwoSchedulersConcurrentNoInterference)`
- Source: `tests/chaos/test_chaos_scheduler_timing.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosSchedulerTimingTest): n/a
  - `<unnamed>` (CTI08_TwoSchedulersConcurrentNoInterference): n/a
- Details: TestCTI-08 — Two independent ChaosScheduler instances (one FIXED_TICK, one CONDVAR) running concurrently inject into separate FaultInjectors without interfering with each other.

### test_chaos_stress.cpp

#### `TEST(ChaosStressBurst, BurstScheduleAllFire_Condvar)`
- Source: `tests/chaos/test_chaos_stress.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressBurst): n/a
  - `<unnamed>` (BurstScheduleAllFire_Condvar): n/a

#### `TEST(ChaosStressBurst, BurstScheduleAllFire_FixedTick)`
- Source: `tests/chaos/test_chaos_stress.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressBurst): n/a
  - `<unnamed>` (BurstScheduleAllFire_FixedTick): n/a

#### `TEST(ChaosStressConcurrent, ConcurrentInjectAndRecover)`
- Source: `tests/chaos/test_chaos_stress.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressConcurrent): n/a
  - `<unnamed>` (ConcurrentInjectAndRecover): n/a

#### `TEST(ChaosStressConcurrent, ConcurrentScheduleFromMultipleThreads)`
- Source: `tests/chaos/test_chaos_stress.cpp`:365
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressConcurrent): n/a
  - `<unnamed>` (ConcurrentScheduleFromMultipleThreads): n/a

#### `TEST(ChaosStressCondvar, ClearPendingDoesNotFireFaults)`
- Source: `tests/chaos/test_chaos_stress.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressCondvar): n/a
  - `<unnamed>` (ClearPendingDoesNotFireFaults): n/a

#### `TEST(ChaosStressCondvar, FaultFiresWithCondvarStrategy)`
- Source: `tests/chaos/test_chaos_stress.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressCondvar): n/a
  - `<unnamed>` (FaultFiresWithCondvarStrategy): n/a

#### `TEST(ChaosStressCondvar, StartStopRepeatedCycles)`
- Source: `tests/chaos/test_chaos_stress.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressCondvar): n/a
  - `<unnamed>` (StartStopRepeatedCycles): n/a

#### `TEST(ChaosStressCondvar, StopWakesCondvarPromptly)`
- Source: `tests/chaos/test_chaos_stress.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressCondvar): n/a
  - `<unnamed>` (StopWakesCondvarPromptly): n/a

#### `TEST(ChaosStressConfig, CondvarStrategyIsSetCorrectly)`
- Source: `tests/chaos/test_chaos_stress.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressConfig): n/a
  - `<unnamed>` (CondvarStrategyIsSetCorrectly): n/a

#### `TEST(ChaosStressConfig, CustomTickIntervalIsPreserved)`
- Source: `tests/chaos/test_chaos_stress.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressConfig): n/a
  - `<unnamed>` (CustomTickIntervalIsPreserved): n/a

#### `TEST(ChaosStressConfig, DefaultConfigIsFixedTick10ms)`
- Source: `tests/chaos/test_chaos_stress.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressConfig): n/a
  - `<unnamed>` (DefaultConfigIsFixedTick10ms): n/a

#### `TEST(ChaosStressConfig, SchedulerAcceptsCustomConfig)`
- Source: `tests/chaos/test_chaos_stress.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressConfig): n/a
  - `<unnamed>` (SchedulerAcceptsCustomConfig): n/a

#### `TEST(ChaosStressFixedTick, FaultFiresWithShortTick)`
- Source: `tests/chaos/test_chaos_stress.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressFixedTick): n/a
  - `<unnamed>` (FaultFiresWithShortTick): n/a

#### `TEST(ChaosStressFixedTick, PendingCountAfterBurstSchedule)`
- Source: `tests/chaos/test_chaos_stress.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressFixedTick): n/a
  - `<unnamed>` (PendingCountAfterBurstSchedule): n/a

#### `TEST(ChaosStressFixedTick, StartStopRepeatedCycles)`
- Source: `tests/chaos/test_chaos_stress.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressFixedTick): n/a
  - `<unnamed>` (StartStopRepeatedCycles): n/a

#### `TEST(ChaosStressJitter, NearSimultaneousFaultsAllFire)`
- Source: `tests/chaos/test_chaos_stress.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressJitter): n/a
  - `<unnamed>` (NearSimultaneousFaultsAllFire): n/a

#### `TEST(ChaosStressJitter, NearSimultaneousFaultsFixedTick)`
- Source: `tests/chaos/test_chaos_stress.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressJitter): n/a
  - `<unnamed>` (NearSimultaneousFaultsFixedTick): n/a

#### `TEST(ChaosStressTick, LongTickDelaysFaultFiring)`
- Source: `tests/chaos/test_chaos_stress.cpp`:428
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressTick): n/a
  - `<unnamed>` (LongTickDelaysFaultFiring): n/a

#### `TEST(ChaosStressTick, ShortTickAllowsFineGrainedScheduling)`
- Source: `tests/chaos/test_chaos_stress.cpp`:411
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChaosStressTick): n/a
  - `<unnamed>` (ShortTickAllowsFineGrainedScheduling): n/a

#### `std::shared_ptr< FaultInjector > make_injector(const std::string &id="stress-fi")`
- Source: `tests/chaos/test_chaos_stress.cpp`:35
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a

### themis::chaos

#### `bool isFailClosedClass(ChaosFailureClass fc) noexcept`
- Source: `include/chaos/chaos_contract.h`:142
- Brief: Returns true when the given failure class mandates a fail-closed no-op.
- Parameters:
  - `fc` (ChaosFailureClass): The failure class to classify.
- Return: true if the class requires silent rejection (no state mutation).
- Details: fc The failure class to classify. true if the class requires silent rejection (no state mutation). All classes currently mandate fail-closed semantics; this helper exists to allow future fine-grained recovery for non-critical classes.

### themis::chaos::ActiveFault

#### `bool isExpired() const noexcept`
- Source: `include/chaos/chaos_framework.h`:61
- Brief: n/a
- Parameters: none

### themis::chaos::ChaosScheduler

#### `ChaosScheduler(std::shared_ptr< FaultInjector > injector, Config cfg=Config{})`
- Source: `include/chaos/chaos_framework.h`:207
- Brief: n/a
- Parameters:
  - `injector` (std::shared_ptr< FaultInjector >): n/a
  - `cfg` (Config): n/a

#### `void clearPending()`
- Source: `include/chaos/chaos_framework.h`:250
- Brief: Clear all pending (unfired) schedule entries.
- Parameters: none
- Details: Clear Pending. Calls: lock(), clear().

#### `bool isRunning() const noexcept`
- Source: `include/chaos/chaos_framework.h`:239
- Brief: Is Running.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Exception safety: noexcept.

#### `size_t pendingCount() const`
- Source: `include/chaos/chaos_framework.h`:245
- Brief: Pending Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void runLoop()`
- Source: `include/chaos/chaos_framework.h`:256
- Brief: Run Loop.
- Parameters: none
- Details: Calls: load(), std::chrono::steady_clock::now(), lock(), std::remove_if(), begin(), end(), push_back(), erase().

#### `void schedule(ChaosScheduleEntry entry)`
- Source: `include/chaos/chaos_framework.h`:215
- Brief: Schedule a future fault injection.
- Parameters:
  - `entry` (ChaosScheduleEntry): Input parameter.
- Details: Schedule. entry Input parameter. entry Input parameter. Calls: lock(), push_back(), std::move(), notify_one().

#### `void scheduleIn(std::chrono::milliseconds delay, const FaultSpec &fault)`
- Source: `include/chaos/chaos_framework.h`:222
- Brief: Schedule using a relative delay from "now".
- Parameters:
  - `delay` (std::chrono::milliseconds): Input parameter.
  - `fault` (const FaultSpec &): Input parameter.
- Details: Schedule In. delay Input parameter. fault Input parameter. delay Input parameter. fault Input parameter. Calls: schedule(), std::chrono::steady_clock::now().

#### `void start()`
- Source: `include/chaos/chaos_framework.h`:227
- Brief: Start the scheduler background thread.
- Parameters: none
- Details: Start. Calls: exchange(), std::thread().

#### `void stop()`
- Source: `include/chaos/chaos_framework.h`:232
- Brief: Stop the scheduler (drains pending entries, does not fire them).
- Parameters: none
- Details: Stop. Calls: store(), notify_all(), joinable(), join().

#### `~ChaosScheduler()`
- Source: `include/chaos/chaos_framework.h`:209
- Brief: n/a
- Parameters: none

### themis::chaos::FaultInjector

#### `FaultInjector(std::string injector_id="default")`
- Source: `include/chaos/chaos_framework.h`:80
- Brief: n/a
- Parameters:
  - `injector_id` (std::string): n/a

#### `size_t activeFaultCount()`
- Source: `include/chaos/chaos_framework.h`:132
- Brief: Total active (non-expired) fault count.
- Parameters: none
- Return: Return value.
- Details: Active Fault Count. Return value. Return value. Calls: pruneExpired(), lock(), size().

#### `void clearAllFaults()`
- Source: `include/chaos/chaos_framework.h`:137
- Brief: Remove all active faults.
- Parameters: none
- Details: Clear All Faults. Calls: lock(), clear().

#### `std::string faultTypeName(FaultType type) noexcept`
- Source: `include/chaos/chaos_framework.h`:174
- Brief: Fault Type Name.
- Parameters:
  - `type` (FaultType): Input parameter.
- Return: Return value.
- Details: type Input parameter. Return value. Exception safety: noexcept.

#### `std::vector< ActiveFault > getActiveFaults()`
- Source: `include/chaos/chaos_framework.h`:126
- Brief: Snapshot of all faults (expires ones are pruned on access).
- Parameters: none
- Return: Return value.
- Details: Get Active Faults. Return value. Return value. Calls: pruneExpired(), lock(), reserve(), size(), push_back().

#### `const std::string & id() const noexcept`
- Source: `include/chaos/chaos_framework.h`:145
- Brief: n/a
- Parameters: none

#### `bool injectFault(const FaultSpec &fault)`
- Source: `include/chaos/chaos_framework.h`:89
- Brief: Inject a fault.
- Parameters:
  - `fault` (const FaultSpec &): Input parameter.
- Return: True when the operation succeeds.
- Details: Inject Fault. fault Input parameter. True when the operation succeeds. If the same node + type is already active, the existing entry is updated in-place (last-writer-wins) and true is returned. fault Input parameter. True when the operation succeeds. Calls: empty(), makeKey(), std::chrono::steady_clock::now(), count(), std::chrono::steady_clock::time_point::max(), lock(), emplace(), cb().

#### `bool isFaultActive(const std::string &target_node_id) const`
- Source: `include/chaos/chaos_framework.h`:112
- Brief: Returns true when target_node_id currently has any active, non-expired fault.
- Parameters:
  - `target_node_id` (const std::string &): Identifier of the target node.
- Return: True when the operation succeeds.
- Details: target_node_id Identifier of the target node. True when the operation succeeds.

#### `bool isFaultActive(const std::string &target_node_id, FaultType type) const`
- Source: `include/chaos/chaos_framework.h`:120
- Brief: Returns true when target_node_id has an active fault of the given type.
- Parameters:
  - `target_node_id` (const std::string &): Identifier of the target node.
  - `type` (FaultType): Input parameter.
- Return: True when the operation succeeds.
- Details: target_node_id Identifier of the target node. type Input parameter. True when the operation succeeds.

#### `std::string makeKey(const std::string &node_id, FaultType type)`
- Source: `include/chaos/chaos_framework.h`:167
- Brief: Make Key.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
  - `type` (FaultType): Input parameter.
- Return: Return value.
- Details: node_id Identifier of the node. type Input parameter. Return value. node_id Identifier of the node. type Input parameter. Return value. Calls: faultTypeName().

#### `void pruneExpired()`
- Source: `include/chaos/chaos_framework.h`:151
- Brief: Prune Expired.
- Parameters: none
- Details: Calls: lock(), begin(), end(), isExpired(), erase().

#### `bool recoverFault(const std::string &target_node_id)`
- Source: `include/chaos/chaos_framework.h`:97
- Brief: Clear the active fault on target_node_id (all types).
- Parameters:
  - `target_node_id` (const std::string &): Identifier of the target node.
- Return: True when the operation succeeds.
- Details: Recover Fault. target_node_id Identifier of the target node. True when the operation succeeds. Returns false if no fault was registered. target_node_id Identifier of the target node. True when the operation succeeds. Calls: lock(), begin(), end(), cb(), erase().

#### `bool recoverFault(const std::string &target_node_id, FaultType type)`
- Source: `include/chaos/chaos_framework.h`:105
- Brief: Clear the active fault for a specific type on target_node_id.
- Parameters:
  - `target_node_id` (const std::string &): Identifier of the target node.
  - `type` (FaultType): Input parameter.
- Return: True when the operation succeeds.
- Details: Recover Fault. target_node_id Identifier of the target node. type Input parameter. True when the operation succeeds. target_node_id Identifier of the target node. type Input parameter. True when the operation succeeds. Calls: makeKey(), lock(), find(), end(), cb(), erase().

#### `void registerEventCallback(EventCallback cb)`
- Source: `include/chaos/chaos_framework.h`:143
- Brief: Register a callback invoked on every inject/recover event.
- Parameters:
  - `cb` (EventCallback): Input parameter.
- Details: Register Event Callback. cb Input parameter. cb Input parameter. Calls: push_back(), std::move().

#### `~FaultInjector()`
- Source: `include/chaos/chaos_framework.h`:81
- Brief: n/a
- Parameters: none

### themis::chaos::FaultSpec

#### `FaultSpec()=default`
- Source: `include/chaos/chaos_framework.h`:47
- Brief: n/a
- Parameters: none

#### `FaultSpec(FaultType t, std::string node, std::chrono::milliseconds dur={}, double prob=1.0, std::string desc="")`
- Source: `include/chaos/chaos_framework.h`:48
- Brief: n/a
- Parameters:
  - `t` (FaultType): n/a
  - `node` (std::string): n/a
  - `dur` (std::chrono::milliseconds): n/a
  - `prob` (double): n/a
  - `desc` (std::string): n/a

