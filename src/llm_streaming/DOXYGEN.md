# LLM_STREAMING DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\llm_streaming\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\llm_streaming\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 2
- Compounds: 14
- Classes/Structs: 7
- Namespaces: 0
- File Compounds: 2

## Namespaces
- none

## Types
### Classes
- StubBackpressureBench
- StubBackpressureStress
- StubChunkAssemblerBench
- StubChunkAssemblerStress
- StubStreamSession
- StubStreamSessionBench
- StubTokenStreamBench

### Structs
- none

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 34

### StubBackpressureBench

#### `StubBackpressureBench(std::size_t cap)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:49
- Brief: n/a
- Parameters:
  - `cap` (std::size_t): n/a

#### `bool pop(std::string &out)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:56
- Brief: n/a
- Parameters:
  - `out` (std::string &): n/a

#### `bool push(const std::string &chunk)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:50
- Brief: n/a
- Parameters:
  - `chunk` (const std::string &): n/a

### StubBackpressureStress

#### `StubBackpressureStress(std::size_t capacity)`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:59
- Brief: n/a
- Parameters:
  - `capacity` (std::size_t): n/a

#### `uint64_t overflow() const noexcept`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:79
- Brief: n/a
- Parameters: none

#### `bool pop(std::string &out)`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:71
- Brief: n/a
- Parameters:
  - `out` (std::string &): n/a

#### `bool push(const std::string &chunk)`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:61
- Brief: n/a
- Parameters:
  - `chunk` (const std::string &): n/a

### StubChunkAssemblerBench

#### `uint64_t count() const noexcept`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:73
- Brief: n/a
- Parameters: none

#### `void receive(uint64_t seq, const std::string &chunk)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:69
- Brief: n/a
- Parameters:
  - `seq` (uint64_t): n/a
  - `chunk` (const std::string &): n/a

### StubChunkAssemblerStress

#### `uint64_t orderErrors() const noexcept`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:101
- Brief: n/a
- Parameters: none

#### `void receiveChunk(uint64_t seq, const std::string &chunk)`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:90
- Brief: n/a
- Parameters:
  - `seq` (uint64_t): n/a
  - `chunk` (const std::string &): n/a

#### `uint64_t received() const noexcept`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:100
- Brief: n/a
- Parameters: none

### StubStreamSession

#### `StubStreamSession(uint64_t id)`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:40
- Brief: n/a
- Parameters:
  - `id` (uint64_t): n/a

#### `void close()`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:46
- Brief: n/a
- Parameters: none

#### `uint64_t id() const noexcept`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:49
- Brief: n/a
- Parameters: none

#### `bool isClosed() const noexcept`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:47
- Brief: n/a
- Parameters: none

#### `void sendToken(const std::string &token)`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:42
- Brief: n/a
- Parameters:
  - `token` (const std::string &): n/a

#### `uint64_t tokenCount() const noexcept`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:48
- Brief: n/a
- Parameters: none

### StubStreamSessionBench

#### `void close()`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:81
- Brief: n/a
- Parameters: none

#### `void open(uint64_t id)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:80
- Brief: n/a
- Parameters:
  - `id` (uint64_t): n/a

### StubTokenStreamBench

#### `uint64_t count() const noexcept`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:42
- Brief: n/a
- Parameters: none

#### `void sendToken(const std::string &token)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:38
- Brief: n/a
- Parameters:
  - `token` (const std::string &): n/a

### bench_llm_streaming_dedicated_gates.cpp

#### `BENCHMARK(LS_BM_01_Token_Send_Throughput) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (LS_BM_01_Token_Send_Throughput): n/a

#### `BENCHMARK(LS_BM_02_Backpressure_Push_Latency) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (LS_BM_02_Backpressure_Push_Latency): n/a

#### `BENCHMARK(LS_BM_03_ChunkAssembler_Receive_Throughput) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (LS_BM_03_ChunkAssembler_Receive_Throughput): n/a

#### `BENCHMARK(LS_BM_04_StreamSession_Create_Latency) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (LS_BM_04_StreamSession_Create_Latency): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:151
- Brief: n/a
- Parameters: none

#### `void LS_BM_01_Token_Send_Throughput(benchmark::State &state)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:89
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void LS_BM_02_Backpressure_Push_Latency(benchmark::State &state)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:104
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void LS_BM_03_ChunkAssembler_Receive_Throughput(benchmark::State &state)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:123
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void LS_BM_04_StreamSession_Create_Latency(benchmark::State &state)`
- Source: `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`:139
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### test_llm_streaming_highcardinality_stress.cpp

#### `TEST(ChunkAssemblyEdgeCaseStress, InOrderDelivery)`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChunkAssemblyEdgeCaseStress): n/a
  - `<unnamed>` (InOrderDelivery): n/a
- Details: TestChunkAssemblyEdgeCaseStress Sends 100 000 in-order chunks to the assembler from 8 threads (serialised via mutex) and verifies zero order violations.

#### `TEST(ConcurrentBackpressureStress, MultiThreadedPushPop)`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrentBackpressureStress): n/a
  - `<unnamed>` (MultiThreadedPushPop): n/a
- Details: TestConcurrentBackpressureStress Runs 8 concurrent producers against a single backpressure buffer and verifies no uncaught exceptions occur.

#### `TEST(HighCardinalityStreamSession, ConcurrentSessions)`
- Source: `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (HighCardinalityStreamSession): n/a
  - `<unnamed>` (ConcurrentSessions): n/a
- Details: TestHighCardinalityStreamSession Creates 1 000 streaming sessions distributed across 8 threads, sends tokens into each session, and verifies all sessions close cleanly.

