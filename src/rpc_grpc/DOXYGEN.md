# RPC_GRPC DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\rpc_grpc\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\rpc_grpc\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 12
- Compounds: 46
- Classes/Structs: 11
- Namespaces: 15
- File Compounds: 12

## Namespaces
- @324323103062272117361231050006144204341245210332
- std::chrono_literals
- themis
- themis::bench
- themis::bench::rpc
- themis::bench::rpc_dedicated
- themis::plugins
- themis::plugins::rpc
- themis::plugins::rpc::grpc_plugin
- themis::plugins::rpc::grpc_plugin::@165306217175100040331337374364355040176263222271
- themis::plugins::rpc::grpc_plugin::test
- themis::plugins::rpc::grpc_plugin::test::@260005254017274226247134331042220325105170105003
- themis::rpc
- themis::rpc_grpc
- themis::rpc_grpc::test

## Types
### Classes
- StubServiceRegistry
- StubServiceReloader
- StubStreamAdapterPool
- themis::plugins::rpc::grpc_plugin::BidiStreamAdapter
- themis::plugins::rpc::grpc_plugin::GRPCPlugin
- themis::plugins::rpc::grpc_plugin::GRPCServer
- themis::rpc::CircuitBreaker

### Structs
- themis::plugins::rpc::grpc_plugin::GRPCServer::MethodMetrics
- themis::rpc::CircuitBreakerConfig
- themis::rpc::CircuitBreakerStats
- themis::rpc_grpc::RpcServiceDescriptor

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 144

### StubServiceRegistry

#### `bool deregisterService(const std::string &service_name) noexcept`
- Source: `tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`:48
- Brief: n/a
- Parameters:
  - `service_name` (const std::string &): n/a

#### `uint64_t deregistrations() const noexcept`
- Source: `tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`:54
- Brief: n/a
- Parameters: none

#### `bool registerService(const std::string &service_name) noexcept`
- Source: `tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`:43
- Brief: n/a
- Parameters:
  - `service_name` (const std::string &): n/a

#### `uint64_t registrations() const noexcept`
- Source: `tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`:53
- Brief: n/a
- Parameters: none

### StubServiceReloader

#### `bool reload(const std::string &service_name) noexcept`
- Source: `tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`:74
- Brief: n/a
- Parameters:
  - `service_name` (const std::string &): n/a

#### `uint64_t reloads() const noexcept`
- Source: `tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`:79
- Brief: n/a
- Parameters: none

### StubStreamAdapterPool

#### `bool send(uint32_t stream_id, uint64_t payload) noexcept`
- Source: `tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`:62
- Brief: n/a
- Parameters:
  - `stream_id` (uint32_t): n/a
  - `payload` (uint64_t): n/a

#### `uint64_t totalSends() const noexcept`
- Source: `tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`:67
- Brief: n/a
- Parameters: none

### bench_rpc_grpc_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:151
- Brief: n/a
- Parameters: none

### bench_rpc_grpc_release_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp`:277
- Brief: n/a
- Parameters: none

### grpc_plugin.cpp

#### `themis::plugins::IThemisPlugin * createPlugin()`
- Source: `src/rpc_grpc/grpc_plugin.cpp`:796
- Brief: Create Plugin.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result. Calls: release().

#### `void destroyPlugin(themis::plugins::IThemisPlugin *plugin)`
- Source: `src/rpc_grpc/grpc_plugin.cpp`:806
- Brief: Destroy Plugin.
- Parameters:
  - `plugin` (themis::plugins::IThemisPlugin *): Input/output parameter.
- Details: plugin Input/output parameter. Implements destroyPlugin without additional internal calls.

### test_rpc_grpc_circuit_breaker_focused.cpp

#### `TEST(CircuitBreakerFocused, CB01_InitialStateClosed)`
- Source: `tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerFocused): n/a
  - `<unnamed>` (CB01_InitialStateClosed): n/a

#### `TEST(CircuitBreakerFocused, CB02_AllowRequestWhenClosed)`
- Source: `tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerFocused): n/a
  - `<unnamed>` (CB02_AllowRequestWhenClosed): n/a

#### `TEST(CircuitBreakerFocused, CB03_TripsOpenAfterThreshold)`
- Source: `tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerFocused): n/a
  - `<unnamed>` (CB03_TripsOpenAfterThreshold): n/a

#### `TEST(CircuitBreakerFocused, CB04_RejectWhenOpen)`
- Source: `tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerFocused): n/a
  - `<unnamed>` (CB04_RejectWhenOpen): n/a

#### `TEST(CircuitBreakerFocused, CB05_HalfOpenAfterRecoveryWindow)`
- Source: `tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerFocused): n/a
  - `<unnamed>` (CB05_HalfOpenAfterRecoveryWindow): n/a

#### `TEST(CircuitBreakerFocused, CB06_ProbeSuccessCloses)`
- Source: `tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerFocused): n/a
  - `<unnamed>` (CB06_ProbeSuccessCloses): n/a

#### `TEST(CircuitBreakerFocused, CB07_ProbeFailureReopens)`
- Source: `tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerFocused): n/a
  - `<unnamed>` (CB07_ProbeFailureReopens): n/a

#### `TEST(CircuitBreakerFocused, CB08_ResetCloses)`
- Source: `tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerFocused): n/a
  - `<unnamed>` (CB08_ResetCloses): n/a

#### `TEST(CircuitBreakerFocused, CB09_StatsConsistency)`
- Source: `tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerFocused): n/a
  - `<unnamed>` (CB09_StatsConsistency): n/a

#### `TEST(CircuitBreakerFocused, CB10_TransitionCallbackFires)`
- Source: `tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerFocused): n/a
  - `<unnamed>` (CB10_TransitionCallbackFires): n/a

#### `TEST(CircuitBreakerFocused, CB11_ForceState)`
- Source: `tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerFocused): n/a
  - `<unnamed>` (CB11_ForceState): n/a

#### `TEST(CircuitBreakerFocused, CB12_MinCallsWindowPreventsEarlyTrip)`
- Source: `tests/rpc_grpc/test_rpc_grpc_circuit_breaker_focused.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (CircuitBreakerFocused): n/a
  - `<unnamed>` (CB12_MinCallsWindowPreventsEarlyTrip): n/a

### test_rpc_grpc_highcardinality_stress.cpp

#### `TEST(RpcGrpcHighCardinalityStress, GSTR01_HighCardinalityServiceRegistration)`
- Source: `tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcHighCardinalityStress): n/a
  - `<unnamed>` (GSTR01_HighCardinalityServiceRegistration): n/a

#### `TEST(RpcGrpcHighCardinalityStress, GSTR02_ConcurrentStreamAdapterStress)`
- Source: `tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcHighCardinalityStress): n/a
  - `<unnamed>` (GSTR02_ConcurrentStreamAdapterStress): n/a

#### `TEST(RpcGrpcHighCardinalityStress, GSTR03_ServiceReloadUnderLoad)`
- Source: `tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcHighCardinalityStress): n/a
  - `<unnamed>` (GSTR03_ServiceReloadUnderLoad): n/a

### themis::bench::rpc

#### `void BM_RPC01_ErrorEnumCast(benchmark::State &state)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp`:62
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RPC02_SwitchDispatch(benchmark::State &state)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp`:79
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RPC03_StructAlloc(benchmark::State &state)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp`:116
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RPC04_BatchCast(benchmark::State &state)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp`:134
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RPC05_ConcurrentDispatch(benchmark::State &state)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp`:164
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RPC06_StateConstruction(benchmark::State &state)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp`:196
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RPC07_BulkDescriptorOps(benchmark::State &state)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp`:221
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RPC08_FailClosedThroughput(benchmark::State &state)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp`:247
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

#### `Threads(8) -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_release_gates.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (8): n/a

### themis::bench::rpc_dedicated

#### `void BM_GRPC_BatchServiceCall(benchmark::State &state)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:131
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GRPC_CredentialReload(benchmark::State &state)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:115
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GRPC_ServiceRegistration(benchmark::State &state)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:82
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GRPC_StreamAdapterSend(benchmark::State &state)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:98
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Name("GRPC-BM-01/ServiceRegistration") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` ("GRPC-BM-01/ServiceRegistration"): n/a

#### `Name("GRPC-BM-02/StreamAdapterSend") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` ("GRPC-BM-02/StreamAdapterSend"): n/a

#### `Name("GRPC-BM-03/CredentialReload") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` ("GRPC-BM-03/CredentialReload"): n/a

#### `Name("GRPC-BM-04/BatchServiceCall1000") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` ("GRPC-BM-04/BatchServiceCall1000"): n/a

#### `bool stubRegisterService(const std::string &name) noexcept`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:58
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `bool stubReloadCredential(const std::string &path) noexcept`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:69
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

#### `bool stubServiceCall(uint64_t call_id) noexcept`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:74
- Brief: n/a
- Parameters:
  - `call_id` (uint64_t): n/a

#### `bool stubStreamSend(uint32_t stream_id, uint64_t payload) noexcept`
- Source: `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp`:64
- Brief: n/a
- Parameters:
  - `stream_id` (uint32_t): n/a
  - `payload` (uint64_t): n/a

### themis::plugins::rpc::grpc_plugin::BidiStreamAdapter

#### `BidiStreamAdapter(BidiStreamAdapter &&)=delete`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidiStreamAdapter &&): n/a

#### `BidiStreamAdapter(Stream *stream, std::size_t max_queue_depth=100)`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:35
- Brief: n/a
- Parameters:
  - `stream` (Stream *): n/a
  - `max_queue_depth` (std::size_t): n/a

#### `BidiStreamAdapter(const BidiStreamAdapter &)=delete`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BidiStreamAdapter &): n/a

#### `void finish(grpc::Status status)`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:106
- Brief: Finish.
- Parameters:
  - `status` (grpc::Status): Input parameter.
- Details: status Input parameter. Calls: lock(), std::move(), notify_all().

#### `grpc::Status finishStatus() const`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:115
- Brief: n/a
- Parameters: none

#### `void flush()`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:144
- Brief: Flush.
- Parameters: none
- Details: Calls: lock(), empty(), std::move(), front(), pop(), unlock(), Write(), notify_one().

#### `bool isFinished() const`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:135
- Brief: n/a
- Parameters: none

#### `void onMessage(MessageHandler handler)`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:58
- Brief: On Message.
- Parameters:
  - `handler` (MessageHandler): Input parameter.
- Details: handler Input parameter. Calls: std::move().

#### `BidiStreamAdapter & operator=(BidiStreamAdapter &&)=delete`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (BidiStreamAdapter &&): n/a

#### `BidiStreamAdapter & operator=(const BidiStreamAdapter &)=delete`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BidiStreamAdapter &): n/a

#### `std::size_t queueDepth() const`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:125
- Brief: n/a
- Parameters: none

#### `void run()`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:66
- Brief: Run.
- Parameters: none
- Details: Calls: Read(), handler_(), std::move().

#### `bool write(Resp response)`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:82
- Brief: Write.
- Parameters:
  - `response` (Resp): Input parameter.
- Return: True when the operation succeeds.
- Details: response Input parameter. True when the operation succeeds. Calls: lock(), wait(), size(), push(), std::move(), flush().

#### `~BidiStreamAdapter()=default`
- Source: `src/rpc_grpc/bidi_stream_adapter.h`:51
- Brief: n/a
- Parameters: none

### themis::plugins::rpc::grpc_plugin::GRPCPlugin

#### `GRPCPlugin()=default`
- Source: `src/rpc_grpc/grpc_plugin.h`:184
- Brief: n/a
- Parameters: none

#### `std::unique_ptr< IRPCServer > createServer() override`
- Source: `src/rpc_grpc/grpc_plugin.h`:197
- Brief: Create Server.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements createServer without additional internal calls.

#### `PluginCapabilities getCapabilities() const override`
- Source: `src/rpc_grpc/grpc_plugin.h`:191
- Brief: n/a
- Parameters: none

#### `uint16_t getDefaultPort() const override`
- Source: `src/rpc_grpc/grpc_plugin.h`:199
- Brief: n/a
- Parameters: none

#### `void * getInstance() override`
- Source: `src/rpc_grpc/grpc_plugin.h`:194
- Brief: Get Instance.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result. Implements getInstance without additional internal calls.

#### `const char * getName() const override`
- Source: `src/rpc_grpc/grpc_plugin.h`:188
- Brief: n/a
- Parameters: none

#### `RPCProtocol getProtocol() const override`
- Source: `src/rpc_grpc/grpc_plugin.h`:198
- Brief: n/a
- Parameters: none

#### `const char * getProtocolDescription() const override`
- Source: `src/rpc_grpc/grpc_plugin.h`:200
- Brief: n/a
- Parameters: none

#### `PluginType getType() const override`
- Source: `src/rpc_grpc/grpc_plugin.h`:190
- Brief: n/a
- Parameters: none

#### `const char * getVersion() const override`
- Source: `src/rpc_grpc/grpc_plugin.h`:189
- Brief: n/a
- Parameters: none

#### `bool initialize(const char *config_json) override`
- Source: `src/rpc_grpc/grpc_plugin.h`:192
- Brief: Initialize.
- Parameters:
  - `config_json` (const char *): Input parameter.
- Return: True when the operation succeeds.
- Details: config_json Input parameter. True when the operation succeeds. Implements initialize without additional internal calls.

#### `void shutdown() override`
- Source: `src/rpc_grpc/grpc_plugin.h`:193
- Brief: Shutdown.
- Parameters: none
- Details: Implements shutdown without additional internal calls.

#### `~GRPCPlugin() override=default`
- Source: `src/rpc_grpc/grpc_plugin.h`:185
- Brief: n/a
- Parameters: none

### themis::plugins::rpc::grpc_plugin::GRPCServer

#### `GRPCServer()`
- Source: `src/rpc_grpc/grpc_plugin.h`:33
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< grpc::ServerCredentials > buildSslCredentials(const std::string &cert_pem, const std::string &key_pem, const std::string &ca_pem, bool require_client_cert)`
- Source: `src/rpc_grpc/grpc_plugin.h`:168
- Brief: Build Ssl Credentials.
- Parameters:
  - `cert_pem` (const std::string &): Input parameter.
  - `key_pem` (const std::string &): Input parameter.
  - `ca_pem` (const std::string &): Input parameter.
  - `require_client_cert` (bool): Input parameter.
- Return: Return value.
- Details: cert_pem Input parameter. key_pem Input parameter. ca_pem Input parameter. require_client_cert Input parameter. Return value.

#### `std::shared_ptr< grpc::ServerCredentials > configureCredentials()`
- Source: `src/rpc_grpc/grpc_plugin.h`:158
- Brief: Configure Credentials.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string getAddress() const override`
- Source: `src/rpc_grpc/grpc_plugin.h`:44
- Brief: n/a
- Parameters: none

#### `std::string getAdminAddress() const`
- Source: `src/rpc_grpc/grpc_plugin.h`:64
- Brief: Get Admin Address.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string getMetricsText() const`
- Source: `src/rpc_grpc/grpc_plugin.h`:94
- Brief: Get Metrics Text.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `RPCProtocol getProtocol() const override`
- Source: `src/rpc_grpc/grpc_plugin.h`:37
- Brief: n/a
- Parameters: none

#### `RPCServerStats getStats() const override`
- Source: `src/rpc_grpc/grpc_plugin.h`:42
- Brief: n/a
- Parameters: none

#### `bool initialize(const RPCServerConfig &config) override`
- Source: `src/rpc_grpc/grpc_plugin.h`:38
- Brief: Initialize.
- Parameters:
  - `config` (const RPCServerConfig &): Input parameter.
- Return: True when the operation succeeds.
- Details: config Input parameter. True when the operation succeeds. Calls: std::to_string().

#### `bool isRunning() const override`
- Source: `src/rpc_grpc/grpc_plugin.h`:41
- Brief: n/a
- Parameters: none

#### `bool isServiceHealthy(const std::string &service_name) const`
- Source: `src/rpc_grpc/grpc_plugin.h`:80
- Brief: Is Service Healthy.
- Parameters:
  - `service_name` (const std::string &): Name of the service.
- Return: True when the operation succeeds.
- Details: service_name Name of the service. True when the operation succeeds.

#### `std::string loadFile(const std::string &path)`
- Source: `src/rpc_grpc/grpc_plugin.h`:152
- Brief: Load File.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: path Input parameter. Return value. path Input parameter. Return value. std::runtime_error if an error occurs. Calls: file(), rdbuf(), str().

#### `void logAccess(const std::string &method, int status_code, uint64_t duration_ms, const std::string &client_cn="")`
- Source: `src/rpc_grpc/grpc_plugin.h`:98
- Brief: Log Access.
- Parameters:
  - `method` (const std::string &): Input parameter.
  - `status_code` (int): Input parameter.
  - `duration_ms` (uint64_t): Input parameter.
  - `client_cn` (const std::string &): Input parameter.
- Details: method Input parameter. status_code Input parameter. duration_ms Input parameter. client_cn Input parameter.

#### `MethodMetrics & methodMetricsLocked(const std::string &method)`
- Source: `src/rpc_grpc/grpc_plugin.h`:179
- Brief: Method Metrics Locked.
- Parameters:
  - `method` (const std::string &): Input parameter.
- Return: Return value.
- Details: ============================================================================ v0. method Input parameter. Return value. method Input parameter. Return value. 3.0 — Interceptor Metrics ============================================================================ Calls: find(), end(), emplace().

#### `void recordRPC(const std::string &method, bool success, uint64_t duration_ms)`
- Source: `src/rpc_grpc/grpc_plugin.h`:88
- Brief: Record RPC.
- Parameters:
  - `method` (const std::string &): Input parameter.
  - `success` (bool): Input parameter.
  - `duration_ms` (uint64_t): Input parameter.
- Details: method Input parameter. success Input parameter. duration_ms Input parameter.

#### `void registerService(void *service_impl) override`
- Source: `src/rpc_grpc/grpc_plugin.h`:43
- Brief: Register Service.
- Parameters:
  - `service_impl` (void *): Input/output parameter.
- Details: service_impl Input/output parameter. Calls: push_back(), size().

#### `bool reloadTls(const std::string &cert_path, const std::string &key_path, const std::string &ca_path)`
- Source: `src/rpc_grpc/grpc_plugin.h`:56
- Brief: -------------------------------------------------------------------- v0.
- Parameters:
  - `cert_path` (const std::string &): Path to the cert.
  - `key_path` (const std::string &): Path to the key.
  - `ca_path` (const std::string &): Path to the ca.
- Return: True when the operation succeeds.
- Details: ============================================================================ v0. cert_path Path to the cert. key_path Path to the key. ca_path Path to the ca. True when the operation succeeds. 2.0 extensions -------------------------------------------------------------------- cert_path Path to the cert. key_path Path to the key. ca_path Path to the ca. True when the operation succeeds. 2.0 — TLS Hot-Reload (Phase 3: Fail-Safe, Deterministic Hardening) ============================================================================

#### `void resetStats() override`
- Source: `src/rpc_grpc/grpc_plugin.h`:45
- Brief: Reset Stats.
- Parameters: none
- Details: Calls: lock().

#### `void setAccessLogSink(std::function< void(const std::string &)> sink)`
- Source: `src/rpc_grpc/grpc_plugin.h`:96
- Brief: n/a
- Parameters:
  - `sink` (std::function< void(const std::string &)>): n/a

#### `void setServiceHealth(const std::string &service_name, bool serving)`
- Source: `src/rpc_grpc/grpc_plugin.h`:73
- Brief: -------------------------------------------------------------------- v0.
- Parameters:
  - `service_name` (const std::string &): Name of the service.
  - `serving` (bool): Input parameter.
- Details: ============================================================================ v0. service_name Name of the service. serving Input parameter. 3.0 — Health & Observability -------------------------------------------------------------------- service_name Name of the service. serving Input parameter. 3.0 — Health Service ============================================================================ Calls: lock().

#### `bool start() override`
- Source: `src/rpc_grpc/grpc_plugin.h`:39
- Brief: Start.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Calls: configureCredentials(), what(), clear(), AddListeningPort(), find(), end(), tryParseConfigInt(), AddChannelArgument().

#### `void stop() override`
- Source: `src/rpc_grpc/grpc_plugin.h`:40
- Brief: Stop.
- Parameters: none
- Details: Calls: lock(), Shutdown(), Next(), reset(), what().

#### `~GRPCServer() override`
- Source: `src/rpc_grpc/grpc_plugin.h`:34
- Brief: n/a
- Parameters: none

### themis::plugins::rpc::grpc_plugin::GRPCServer::MethodMetrics

#### `MethodMetrics()=default`
- Source: `src/rpc_grpc/grpc_plugin.h`:131
- Brief: n/a
- Parameters: none

#### `MethodMetrics(const MethodMetrics &)=delete`
- Source: `src/rpc_grpc/grpc_plugin.h`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MethodMetrics &): n/a

#### `MethodMetrics & operator=(const MethodMetrics &)=delete`
- Source: `src/rpc_grpc/grpc_plugin.h`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MethodMetrics &): n/a

### themis::plugins::rpc::grpc_plugin::test

#### `TEST(GrpcPluginIntegration, INT01_PluginCreatesServer)`
- Source: `tests/rpc_grpc/test_rpc_grpc_integration_focused.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcPluginIntegration): n/a
  - `<unnamed>` (INT01_PluginCreatesServer): n/a

#### `TEST(GrpcPluginIntegration, INT02_PluginMetadata)`
- Source: `tests/rpc_grpc/test_rpc_grpc_integration_focused.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcPluginIntegration): n/a
  - `<unnamed>` (INT02_PluginMetadata): n/a

#### `TEST(GrpcPluginIntegration, INT03_PluginInitializeShutdown)`
- Source: `tests/rpc_grpc/test_rpc_grpc_integration_focused.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcPluginIntegration): n/a
  - `<unnamed>` (INT03_PluginInitializeShutdown): n/a

#### `TEST(GrpcPluginIntegration, INT04_ServerInitialState)`
- Source: `tests/rpc_grpc/test_rpc_grpc_integration_focused.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcPluginIntegration): n/a
  - `<unnamed>` (INT04_ServerInitialState): n/a

#### `TEST(GrpcPluginIntegration, INT05_ServerInitializeInsecure)`
- Source: `tests/rpc_grpc/test_rpc_grpc_integration_focused.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcPluginIntegration): n/a
  - `<unnamed>` (INT05_ServerInitializeInsecure): n/a

#### `TEST(GrpcPluginIntegration, INT06_InitializeFailClosedMissingCerts)`
- Source: `tests/rpc_grpc/test_rpc_grpc_integration_focused.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcPluginIntegration): n/a
  - `<unnamed>` (INT06_InitializeFailClosedMissingCerts): n/a

#### `TEST(GrpcPluginIntegration, INT07_StartStopCycle)`
- Source: `tests/rpc_grpc/test_rpc_grpc_integration_focused.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcPluginIntegration): n/a
  - `<unnamed>` (INT07_StartStopCycle): n/a

#### `TEST(GrpcPluginIntegration, INT08_DoubleStopIsIdempotent)`
- Source: `tests/rpc_grpc/test_rpc_grpc_integration_focused.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcPluginIntegration): n/a
  - `<unnamed>` (INT08_DoubleStopIsIdempotent): n/a

### themis::rpc

#### `const char * circuitStateToString(CircuitState state) noexcept`
- Source: `include/rpc_grpc/circuit_breaker.h`:86
- Brief: Human-readable label for state.
- Parameters:
  - `state` (CircuitState): n/a

### themis::rpc::CircuitBreaker

#### `CircuitBreaker(CircuitBreaker &&) noexcept`
- Source: `include/rpc_grpc/circuit_breaker.h`:154
- Brief: Movable.
- Parameters:
  - `other` (CircuitBreaker &&): n/a

#### `CircuitBreaker(CircuitBreakerConfig config={})`
- Source: `include/rpc_grpc/circuit_breaker.h`:140
- Brief: Construct a new circuit breaker.
- Parameters:
  - `config` (CircuitBreakerConfig): Tuning configuration. Copied on construction.
- Details: config Tuning configuration. Copied on construction.

#### `CircuitBreaker(const CircuitBreaker &)=delete`
- Source: `include/rpc_grpc/circuit_breaker.h`:150
- Brief: Non-copyable; circuit breakers hold live state.
- Parameters:
  - `<unnamed>` (const CircuitBreaker &): n/a

#### `CircuitBreaker(std::string name, CircuitBreakerConfig config={})`
- Source: `include/rpc_grpc/circuit_breaker.h`:147
- Brief: Construct a new circuit breaker with an explicit name.
- Parameters:
  - `name` (std::string): Descriptive name used in metrics and logs.
  - `config` (CircuitBreakerConfig): Tuning configuration.
- Details: name Descriptive name used in metrics and logs. config Tuning configuration.

#### `bool allowRequest()`
- Source: `include/rpc_grpc/circuit_breaker.h`:176
- Brief: Query whether the next call should be allowed.
- Parameters: none
- Return: true if the call may proceed; false if it must be rejected.
- Details: Allow Request. CLOSED → always returns true. OPEN → returns false unless the recovery window has elapsed, in which case the state transitions to HALF_OPEN and true is returned for the first probe call. HALF_OPEN → returns true only for the first probe; subsequent concurrent callers get false until the probe outcome is recorded. true if the call may proceed; false if it must be rejected. True when the operation succeeds. Calls: lk(), std::chrono::steady_clock::now(), transitionTo().

#### `void forceState(CircuitState state)`
- Source: `include/rpc_grpc/circuit_breaker.h`:204
- Brief: Manually force the circuit into a specific state.
- Parameters:
  - `state` (CircuitState): Input parameter.
- Details: Force State. Intended for operator overrides and test fixtures. The forced state is fully honoured by allowRequest() / recordResult(). state Target state. state Input parameter. Calls: lk(), std::chrono::steady_clock::now(), transitionTo().

#### `const std::string & name() const noexcept`
- Source: `include/rpc_grpc/circuit_breaker.h`:224
- Brief: Return the configured name.
- Parameters: none

#### `CircuitBreaker & operator=(CircuitBreaker &&) noexcept`
- Source: `include/rpc_grpc/circuit_breaker.h`:155
- Brief: n/a
- Parameters:
  - `other` (CircuitBreaker &&): n/a

#### `CircuitBreaker & operator=(const CircuitBreaker &)=delete`
- Source: `include/rpc_grpc/circuit_breaker.h`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CircuitBreaker &): n/a

#### `void recordResult(bool success)`
- Source: `include/rpc_grpc/circuit_breaker.h`:194
- Brief: Record the outcome of a call that was allowed by allowRequest().
- Parameters:
  - `success` (bool): Input parameter.
- Details: Record Result. Must be called exactly once for every call where allowRequest() returned true. Updating state based on the outcome: CLOSED + success → failure counter reset (sliding window). CLOSED + failure → failure counter incremented; if threshold is reached the circuit trips to OPEN. HALF_OPEN + success → success counter incremented; when half_open_success_threshold is reached the circuit closes. HALF_OPEN + failure → circuit reopens immediately. success true = call succeeded; false = call failed. success Input parameter. Calls: lk(), transitionTo(), std::chrono::steady_clock::now().

#### `void reset()`
- Source: `include/rpc_grpc/circuit_breaker.h`:211
- Brief: Reset all counters and return the circuit to CLOSED.
- Parameters: none
- Details: Reset the modification detection flag. Intended for operator overrides after a sustained outage. Calls: lk(), transitionTo().

#### `void setTransitionCallback(std::function< void(CircuitState, CircuitState, const std::string &)> cb)`
- Source: `include/rpc_grpc/circuit_breaker.h`:239
- Brief: Register a listener that is called on every state transition.
- Parameters:
  - `cb` (std::function< void(CircuitState, CircuitState, const std::string &)>): Callable matching void(CircuitState old, CircuitState next, const std::string& name).
- Details: The callback receives the old state, new state, and circuit name. Called under the internal mutex; keep it non-blocking. cb Callable matching void(CircuitState old, CircuitState next, const std::string& name).

#### `CircuitState state() const noexcept`
- Source: `include/rpc_grpc/circuit_breaker.h`:218
- Brief: Return the current circuit state (lock-free read).
- Parameters: none

#### `CircuitBreakerStats stats() const`
- Source: `include/rpc_grpc/circuit_breaker.h`:221
- Brief: Return a consistent point-in-time metrics snapshot.
- Parameters: none

#### `void transitionTo(CircuitState next_state)`
- Source: `include/rpc_grpc/circuit_breaker.h`:243
- Brief: Caller must hold mutex_.
- Parameters:
  - `next_state` (CircuitState): Input parameter.
- Details: Transition To. next_state Input parameter. Calls: transition_cb_().

#### `~CircuitBreaker()=default`
- Source: `include/rpc_grpc/circuit_breaker.h`:157
- Brief: n/a
- Parameters: none

### themis::rpc_grpc

#### `bool isRpcGrpcFailClosed(RpcGrpcError e) noexcept`
- Source: `include/rpc_grpc/rpc_grpc_api_contract.h`:141
- Brief: Returns true when the given error mandates fail-closed denial.
- Parameters:
  - `e` (RpcGrpcError): n/a

### themis::rpc_grpc::test

#### `TEST(RpcGrpcContractHardening, RPC01_ErrorCodeUniqueness)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC01_ErrorCodeUniqueness): n/a

#### `TEST(RpcGrpcContractHardening, RPC02_ErrorCodeRange)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC02_ErrorCodeRange): n/a

#### `TEST(RpcGrpcContractHardening, RPC03_SwitchDispatch)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC03_SwitchDispatch): n/a

#### `TEST(RpcGrpcContractHardening, RPC04_ServerStateDistinct)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC04_ServerStateDistinct): n/a

#### `TEST(RpcGrpcContractHardening, RPC05_ServiceDescriptorDefaults)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC05_ServiceDescriptorDefaults): n/a

#### `TEST(RpcGrpcContractHardening, RPC06_ServiceDescriptorCopy)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC06_ServiceDescriptorCopy): n/a

#### `TEST(RpcGrpcContractHardening, RPC07_ServiceDescriptorMove)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC07_ServiceDescriptorMove): n/a

#### `TEST(RpcGrpcContractHardening, RPC08_FailClosedPredicate)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC08_FailClosedPredicate): n/a

#### `TEST(RpcGrpcContractHardening, RPC09_ErrorCodeSerialization)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC09_ErrorCodeSerialization): n/a

#### `TEST(RpcGrpcContractHardening, RPC10_StateTransitionInvariants)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC10_StateTransitionInvariants): n/a

#### `TEST(RpcGrpcContractHardening, RPC11_ServiceDescriptorValidation)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC11_ServiceDescriptorValidation): n/a

#### `TEST(RpcGrpcContractHardening, RPC12_ErrorTaxonomyCompleteness)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:276
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC12_ErrorTaxonomyCompleteness): n/a

#### `TEST(RpcGrpcContractHardening, RPC13_KeepaliveTimingConstants)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:301
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC13_KeepaliveTimingConstants): n/a

#### `TEST(RpcGrpcContractHardening, RPC14_MessageSizeBounds)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:311
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC14_MessageSizeBounds): n/a

#### `TEST(RpcGrpcContractHardening, RPC15_ServiceNameLengthConstraint)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:321
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC15_ServiceNameLengthConstraint): n/a

#### `TEST(RpcGrpcContractHardening, RPC16_MethodNameLengthConstraint)`
- Source: `tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (RpcGrpcContractHardening): n/a
  - `<unnamed>` (RPC16_MethodNameLengthConstraint): n/a

