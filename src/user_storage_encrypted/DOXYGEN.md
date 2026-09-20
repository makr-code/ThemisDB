# USER_STORAGE_ENCRYPTED DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\user_storage_encrypted\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\user_storage_encrypted\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 30
- Compounds: 89
- Classes/Structs: 35
- Namespaces: 14
- File Compounds: 30

## Namespaces
- benchmark
- testing
- themis
- themis::bench
- themis::bench::use
- themis::plugins
- themis::plugins::user_storage
- themis::plugins::user_storage::@005166074022122332075232003111242113056050020150
- themis::plugins::user_storage::@157145267066264247054361266231343235216251102132
- themis::plugins::user_storage::@251005120243005021261315251066020231010226102177
- themis::plugins::user_storage::CommandArgumentValidator
- themis::plugins::user_storage::test
- themis::user_storage_encrypted
- themis::user_storage_encrypted::test

## Types
### Classes
- GocryptfsBenchFixture
- StubEncryptedStoreBench
- StubEncryptedStoreStress
- themis::plugins::user_storage::Argon2idKeyDerivationService
- themis::plugins::user_storage::CommandTimeoutManager
- themis::plugins::user_storage::EncryptionBackendInterface
- themis::plugins::user_storage::FileRotationStore
- themis::plugins::user_storage::GocryptfsBackend
- themis::plugins::user_storage::IKeyDerivationService
- themis::plugins::user_storage::IRotationStore
- themis::plugins::user_storage::KeyDerivationService
- themis::plugins::user_storage::KeyRotationScheduler
- themis::plugins::user_storage::MultiLevelEncryptedStorage
- themis::plugins::user_storage::NullRotationStore
- themis::plugins::user_storage::PipeGuard
- themis::plugins::user_storage::Result
- themis::plugins::user_storage::Result< void >
- themis::plugins::user_storage::TimedFileOperation
- themis::plugins::user_storage::test::ErrorCodesTest
- themis::user_storage_encrypted::test::E2EIntegrationTest
- themis::user_storage_encrypted::test::StressTest

### Structs
- StoreSeedFixture
- themis::plugins::user_storage::Argon2idParams
- themis::plugins::user_storage::DiagnosticEvent
- themis::plugins::user_storage::GocryptfsBackend::Impl
- themis::plugins::user_storage::Group
- themis::plugins::user_storage::HealthStatus
- themis::plugins::user_storage::KeyRotationScheduler::Impl
- themis::plugins::user_storage::LevelConfig
- themis::plugins::user_storage::MultiLevelEncryptedStorage::Impl
- themis::plugins::user_storage::RotationSchedule
- themis::plugins::user_storage::StorageMetrics
- themis::plugins::user_storage::User
- themis::user_storage_encrypted::EncryptedMountDescriptor
- themis::user_storage_encrypted::KeyDerivationRequest

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 330

### GocryptfsBenchFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### StoreSeedFixture

#### `StoreSeedFixture()`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:73
- Brief: n/a
- Parameters: none

### StubEncryptedStoreBench

#### `std::string decrypt(const std::string &ct)`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:54
- Brief: n/a
- Parameters:
  - `ct` (const std::string &): n/a

#### `std::string encrypt(const std::string &pt)`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:53
- Brief: n/a
- Parameters:
  - `pt` (const std::string &): n/a

#### `bool read(const std::string &key, std::string &out)`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:41
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `out` (std::string &): n/a

#### `void rotateKey()`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:48
- Brief: n/a
- Parameters: none

#### `void write(const std::string &key, const std::string &value)`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:37
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

### StubEncryptedStoreStress

#### `uint64_t decCount() const noexcept`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`:71
- Brief: n/a
- Parameters: none

#### `std::string decrypt(const std::string &ciphertext)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`:55
- Brief: n/a
- Parameters:
  - `ciphertext` (const std::string &): n/a

#### `uint64_t encCount() const noexcept`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`:70
- Brief: n/a
- Parameters: none

#### `std::string encrypt(const std::string &plaintext)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`:50
- Brief: n/a
- Parameters:
  - `plaintext` (const std::string &): n/a

#### `void rotateKey()`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`:45
- Brief: n/a
- Parameters: none

#### `uint64_t rotationCount() const noexcept`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`:69
- Brief: n/a
- Parameters: none

#### `void write(const std::string &key, const std::string &value)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`:39
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

#### `uint64_t writeCount() const noexcept`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`:68
- Brief: n/a
- Parameters: none

### bench_encrypted_storage_dedicated_gates.cpp

#### `BENCHMARK(ES_BM_01_EncryptedWrite_Throughput) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (ES_BM_01_EncryptedWrite_Throughput): n/a

#### `BENCHMARK(ES_BM_02_EncryptedRead_Throughput) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (ES_BM_02_EncryptedRead_Throughput): n/a

#### `BENCHMARK(ES_BM_03_KeyRotation_Latency) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (ES_BM_03_KeyRotation_Latency): n/a

#### `BENCHMARK(ES_BM_04_EncDec_RoundTrip_Latency) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (ES_BM_04_EncDec_RoundTrip_Latency): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:142
- Brief: n/a
- Parameters: none

#### `void ES_BM_01_EncryptedWrite_Throughput(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:83
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void ES_BM_02_EncryptedRead_Throughput(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:98
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void ES_BM_03_KeyRotation_Latency(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:114
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void ES_BM_04_EncDec_RoundTrip_Latency(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`:130
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_user_storage_encrypted_release_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_release_gates.cpp`:160
- Brief: n/a
- Parameters: none

### bench_user_storage_mount_latency.cpp

#### `BENCHMARK(BM_MountDispatch_NoKDF)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MountDispatch_NoKDF): n/a

#### `BENCHMARK_F(GocryptfsBenchFixture, BackendMeta)(benchmark`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (GocryptfsBenchFixture): n/a
  - `<unnamed>` (BackendMeta): n/a

#### `BENCHMARK_F(GocryptfsBenchFixture, CheckAvailability)(benchmark`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (GocryptfsBenchFixture): n/a
  - `<unnamed>` (CheckAvailability): n/a

#### `BENCHMARK_F(GocryptfsBenchFixture, Initialize)(benchmark`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (GocryptfsBenchFixture): n/a
  - `<unnamed>` (Initialize): n/a

#### `BENCHMARK_F(GocryptfsBenchFixture, IsMounted)(benchmark`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (GocryptfsBenchFixture): n/a
  - `<unnamed>` (IsMounted): n/a

#### `BENCHMARK_F(GocryptfsBenchFixture, MountContainer_FastFail)(benchmark`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (GocryptfsBenchFixture): n/a
  - `<unnamed>` (MountContainer_FastFail): n/a

#### `BENCHMARK_F(GocryptfsBenchFixture, MountUnmountCycle)(benchmark`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (GocryptfsBenchFixture): n/a
  - `<unnamed>` (MountUnmountCycle): n/a

#### `BENCHMARK_F(GocryptfsBenchFixture, UnmountContainer_FastFail)(benchmark`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (GocryptfsBenchFixture): n/a
  - `<unnamed>` (UnmountContainer_FastFail): n/a

#### `void BM_MountDispatch_NoKDF(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:167
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `UseRealTime() -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:157
- Brief: n/a
- Parameters: none

#### `std::vector< uint8_t > makeKey(size_t len=32)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`:41
- Brief: n/a
- Parameters:
  - `len` (size_t): n/a

### test_user_storage_encrypted_highcardinality_stress.cpp

#### `TEST(ConcurrentKeyRotationStress, MultiThreadedRotation)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrentKeyRotationStress): n/a
  - `<unnamed>` (MultiThreadedRotation): n/a
- Details: TestConcurrentKeyRotationStress Runs 8 concurrent key rotation threads issuing 1 000 rotations each and verifies that all 8 000 rotations complete without error.

#### `TEST(EncDecUnderLoadStress, RoundTripIntegrity)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (EncDecUnderLoadStress): n/a
  - `<unnamed>` (RoundTripIntegrity): n/a
- Details: TestEncDecUnderLoadStress Runs 8 concurrent enc/dec threads each performing 10 000 round-trips and verifies that every plaintext is recovered correctly.

#### `TEST(HighCardinalityEncryptedWrite, ConcurrentWrite)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (HighCardinalityEncryptedWrite): n/a
  - `<unnamed>` (ConcurrentWrite): n/a
- Details: TestHighCardinalityEncryptedWrite Writes 100 000 records across 8 concurrent threads and verifies that all writes complete successfully (no exceptions, correct count).

### themis::bench::use

#### `void BM_USE01_ErrorEnumCast(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_release_gates.cpp`:63
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_USE02_SwitchDispatch(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_release_gates.cpp`:80
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_USE03_StructAlloc(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_release_gates.cpp`:115
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_USE04_BatchCast(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_release_gates.cpp`:133
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_USK_P5_01_MountLatency(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp`:107
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_USK_P5_02_UnmountLatency(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp`:144
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_USK_P5_03_KeyDerivationLatency(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp`:179
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_USK_P5_04_KeyRotationLatency(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp`:215
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_USK_P5_05_ConcurrentMountThroughput(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp`:248
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_USK_P5_06_EncryptedWriteThroughput_100MB(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp`:335
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_USK_P5_06_EncryptedWriteThroughput_10MB(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp`:310
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_USK_P5_06_EncryptedWriteThroughput_1MB(benchmark::State &state)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp`:284
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `double ComputePercentile(std::vector< double > &times, double percentile)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp`:93
- Brief: Helper to compute percentile from a vector of times (in milliseconds).
- Parameters:
  - `times` (std::vector< double > &): n/a
  - `percentile` (double): n/a

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(false)`
- Source: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

### themis::plugins::user_storage

#### `void emitDiagnosticEvent(DiagnosticEvent event)`
- Source: `src/user_storage_encrypted/diagnostic_events.cpp`:97
- Brief: Emit Diagnostic Event.
- Parameters:
  - `event` (DiagnosticEvent): Input parameter.
- Details: event Input parameter. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), g_event_handler().

#### `std::string errorCodeToString(ErrorCode code)`
- Source: `include/user_storage_encrypted/error_codes.hpp`:89
- Brief: Convert error code to human-readable string.
- Parameters:
  - `code` (ErrorCode): The error code
- Return: Human-readable error description
- Details: code The error code Human-readable error description

#### `void registerDiagnosticEventHandler(DiagnosticEventHandler handler)`
- Source: `src/user_storage_encrypted/diagnostic_events.cpp`:86
- Brief: Register Diagnostic Event Handler.
- Parameters:
  - `handler` (DiagnosticEventHandler): Input parameter.
- Details: handler Input parameter. Calls: std::move().

#### `std::string securityLevelToString(SecurityLevel level)`
- Source: `include/user_storage_encrypted/security_level.hpp`:40
- Brief: Convert SecurityLevel to string.
- Parameters:
  - `level` (SecurityLevel): n/a

#### `SecurityLevel stringToSecurityLevel(const std::string &str)`
- Source: `include/user_storage_encrypted/security_level.hpp`:53
- Brief: Parse SecurityLevel from string.
- Parameters:
  - `str` (const std::string &): n/a

### themis::plugins::user_storage::Argon2idKeyDerivationService

#### `Argon2idKeyDerivationService(const Argon2idParams &params=Argon2idParams{})`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:117
- Brief: n/a
- Parameters:
  - `params` (const Argon2idParams &): n/a

#### `std::vector< uint8_t > derive(const std::vector< uint8_t > &master_key, const std::string &user_id, const std::string &container_id, const std::vector< uint8_t > &salt) override`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:143
- Brief: Derive.
- Parameters:
  - `master_key` (const std::vector< uint8_t > &): Input parameter.
  - `user_id` (const std::string &): Identifier of the user.
  - `container_id` (const std::string &): Identifier of the container.
  - `salt` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
  - std::runtime_error: if an error occurs.
- Details: master_key Input parameter. user_id Identifier of the user. container_id Identifier of the container. salt Input parameter. Return value. std::invalid_argument if an error occurs. std::runtime_error if an error occurs. Calls: empty(), size(), reserve(), insert(), end(), begin(), data(), derived_key().

#### `Result< std::vector< uint8_t > > deriveKey(const std::vector< uint8_t > &master_key, const std::vector< uint8_t > &salt) const override`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:131
- Brief: Derive a 256-bit (32-byte) key from master_key and salt.
- Parameters:
  - `master_key` (const std::vector< uint8_t > &): Raw key material (must not be empty).
  - `salt` (const std::vector< uint8_t > &): Per-container or per-user salt (must not be empty).
- Return: 32-byte derived key on success, or error.
- Details: master_key Raw key material (must not be empty). salt Per-container or per-user salt (must not be empty). 32-byte derived key on success, or error.

#### `Result< std::vector< uint8_t > > generateSalt() const override`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:136
- Brief: Generate a cryptographically random salt suitable for this KDF.
- Parameters: none
- Return: 32-byte random salt.
- Details: 32-byte random salt.

#### `std::vector< uint8_t > generateSalt(size_t length) override`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:150
- Brief: Generate Salt.
- Parameters:
  - `length` (size_t): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
  - std::runtime_error: if an error occurs.
- Details: length Input parameter. Return value. std::invalid_argument if an error occurs. std::runtime_error if an error occurs. Calls: salt(), rd(), open(), read(), data(), close().

#### `Result< std::vector< uint8_t > > loadOrCreateSalt(const std::string &salt_file_path) const override`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:138
- Brief: Load salt from a file, creating it with generateSalt() if absent.
- Parameters:
  - `salt_file_path` (const std::string &): Path to the salt file (e.g., ".themis_kdf_salt").
- Return: Salt bytes.
- Details: salt_file_path Path to the salt file (e.g., ".themis_kdf_salt"). Salt bytes.

#### `void setDeriveKeyFn(DeriveKeyFn fn)`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:128
- Brief: Inject a custom KDF (e.g., real Argon2id). Pass empty fn to restore default.
- Parameters:
  - `fn` (DeriveKeyFn): Input parameter.
- Details: Set Derive Key Fn. fn Input parameter. Calls: lk(), std::move().

#### `~Argon2idKeyDerivationService() override=default`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:118
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::CommandTimeoutManager

#### `CommandTimeoutManager(std::chrono::milliseconds timeout)`
- Source: `include/user_storage_encrypted/command_timeout_manager.hpp`:44
- Brief: Create a timeout manager with specified duration.
- Parameters:
  - `timeout` (std::chrono::milliseconds): Duration until timeout occurs
- Details: timeout Duration until timeout occurs

#### `std::chrono::milliseconds getElapsed() const`
- Source: `include/user_storage_encrypted/command_timeout_manager.hpp`:79
- Brief: Get elapsed time since creation.
- Parameters: none
- Return: Elapsed duration
- Details: Elapsed duration

#### `std::chrono::milliseconds getRemaining() const`
- Source: `include/user_storage_encrypted/command_timeout_manager.hpp`:64
- Brief: Get remaining time until timeout.
- Parameters: none
- Return: Duration remaining, or 0ms if already timed out
- Details: Duration remaining, or 0ms if already timed out

#### `bool hasTimedOut() const`
- Source: `include/user_storage_encrypted/command_timeout_manager.hpp`:52
- Brief: Check if timeout has been exceeded.
- Parameters: none
- Return: true if timeout has expired
- Details: true if timeout has expired

#### `int terminateProcess(int pid)`
- Source: `include/user_storage_encrypted/command_timeout_manager.hpp`:95
- Brief: Terminate a process with grace period.
- Parameters:
  - `pid` (int): Process ID to terminate
- Return: Exit status from waitpid, or -1 if error
- Details: Attempts SIGTERM first, waits briefly, then SIGKILL if needed. Always cleans up child process via waitpid. pid Process ID to terminate Exit status from waitpid, or -1 if error

### themis::plugins::user_storage::DiagnosticEvent

#### `std::string toJsonString() const`
- Source: `include/user_storage_encrypted/error_codes.hpp`:166
- Brief: Convert event to JSON string (simple format).
- Parameters: none
- Return: JSON-formatted diagnostic event
- Details: JSON-formatted diagnostic event

### themis::plugins::user_storage::EncryptionBackendInterface

#### `Result< void > checkAvailability()=0`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:177
- Brief: Check if backend is available on the system.
- Parameters: none
- Return: Result with error message if backend is not available
- Details: Result with error message if backend is not available

#### `Result< void > createContainer(const std::string &encrypted_dir, const std::string &mount_point, const std::vector< uint8_t > &key_material)=0`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:124
- Brief: Create and initialize an encrypted container.
- Parameters:
  - `encrypted_dir` (const std::string &): Path to encrypted data directory
  - `mount_point` (const std::string &): Path where decrypted data will be accessible
  - `key_material` (const std::vector< uint8_t > &): Encryption key bytes (32 bytes for AES-256)
- Return: Result indicating success or error
- Details: encrypted_dir Path to encrypted data directory mount_point Path where decrypted data will be accessible key_material Encryption key bytes (32 bytes for AES-256) Result indicating success or error

#### `std::string getBackendName() const =0`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:165
- Brief: Get backend name (e.g., "gocryptfs", "fscrypt").
- Parameters: none

#### `std::string getBackendVersion() const =0`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:170
- Brief: Get backend version.
- Parameters: none

#### `Result< void > initialize(const std::string &config_json)=0`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:114
- Brief: Initialize encryption backend.
- Parameters:
  - `config_json` (const std::string &): Configuration as JSON string
- Return: Result indicating success or error
- Details: config_json Configuration as JSON string Result indicating success or error

#### `bool isMounted(const std::string &mount_point)=0`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:160
- Brief: Check if a container is currently mounted.
- Parameters:
  - `mount_point` (const std::string &): Path to check
- Return: true if mounted, false otherwise
- Details: mount_point Path to check true if mounted, false otherwise

#### `Result< void > mountContainer(const std::string &encrypted_dir, const std::string &mount_point, const std::vector< uint8_t > &key_material)=0`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:138
- Brief: Mount an encrypted container.
- Parameters:
  - `encrypted_dir` (const std::string &): Path to encrypted data directory
  - `mount_point` (const std::string &): Path where decrypted data will be accessible
  - `key_material` (const std::vector< uint8_t > &): Encryption key bytes
- Return: Result indicating success or error
- Details: encrypted_dir Path to encrypted data directory mount_point Path where decrypted data will be accessible key_material Encryption key bytes Result indicating success or error

#### `Result< void > unmountContainer(const std::string &mount_point)=0`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:150
- Brief: Unmount an encrypted container.
- Parameters:
  - `mount_point` (const std::string &): Path to mounted container
- Return: Result indicating success or error
- Details: mount_point Path to mounted container Result indicating success or error

#### `~EncryptionBackendInterface()=default`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:106
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::FileRotationStore

#### `FileRotationStore(std::string path)`
- Source: `include/user_storage_encrypted/irotation_store.hpp`:115
- Brief: n/a
- Parameters:
  - `path` (std::string): n/a

#### `bool get(const std::string &key, std::string &out) const override`
- Source: `include/user_storage_encrypted/irotation_store.hpp`:117
- Brief: Read a value by string key.
- Parameters:
  - `key` (const std::string &): Storage key
  - `out` (std::string &): Value (set only when true is returned)
- Return: true if the key existed
- Details: key Storage key out Value (set only when true is returned) true if the key existed

#### `nlohmann::json load_json() const`
- Source: `include/user_storage_encrypted/irotation_store.hpp`:143
- Brief: n/a
- Parameters: none

#### `bool put(const std::string &key, const std::string &value) override`
- Source: `include/user_storage_encrypted/irotation_store.hpp`:127
- Brief: Write a key-value pair.
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a
- Return: true on success
- Details: true on success

### themis::plugins::user_storage::GocryptfsBackend

#### `GocryptfsBackend()`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:54
- Brief: Construct backend without KDF (key_material used directly).
- Parameters: none

#### `GocryptfsBackend(KeyDerivationService *kdf_service)`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:62
- Brief: Construct backend with optional Argon2id KDF.
- Parameters:
  - `kdf_service` (KeyDerivationService *): Pointer to KDF service (not owned; must outlive this object). Pass nullptr to use key_material directly.
- Details: kdf_service Pointer to KDF service (not owned; must outlive this object). Pass nullptr to use key_material directly.

#### `Result< void > checkAvailability() override`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:93
- Brief: Check Availability.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: executeCommandSafe(), isError(), emitDiagnosticEvent(), error(), stat(), modules(), is_open(), std::getline().

#### `Result< void > createContainer(const std::string &encrypted_dir, const std::string &mount_point, const std::vector< uint8_t > &key_material) override`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:69
- Brief: Create Container.
- Parameters:
  - `encrypted_dir` (const std::string &): Input parameter.
  - `mount_point` (const std::string &): Input parameter.
  - `key_material` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: encrypted_dir Input parameter. mount_point Input parameter. key_material Input parameter. Return value. Calls: CommandArgumentValidator::validatePath(), isError(), error(), directoryExists(), value(), createDirectory(), resolveKey(), executeCommandWithStdin().

#### `bool createDirectory(const std::string &path)`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:197
- Brief: Create Directory.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: path Input parameter. True when the operation succeeds. Calls: std::filesystem::exists(), std::filesystem::create_directories().

#### `Result< std::string > createPasswordFile(const std::vector< uint8_t > &key_material)`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:136
- Brief: Create a secure temporary password file; returns the path.
- Parameters:
  - `key_material` (const std::vector< uint8_t > &): n/a

#### `Result< std::string > deliverKeyViaStdin(const std::vector< std::string > &args, const std::string &key_hex)`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:124
- Brief: Deliver key_hex to a command via stdin, then clear the buffer.
- Parameters:
  - `args` (const std::vector< std::string > &): Input parameter.
  - `key_hex` (const std::string &): Input parameter.
- Return: Child output on success, or error description
- Details: Deliver Key Via Stdin. Equivalent to executeCommandWithStdin(args, key_hex) but explicitly zeroes key_hex after the write to limit key material exposure. args argv for execvp key_hex Hex-encoded key to write (cleared on return) Child output on success, or error description args Input parameter. key_hex Input parameter. Return value. Calls: executeCommandWithStdin().

#### `Result< void > deliverKeyViaStdin(int write_fd, const std::vector< uint8_t > &key_material)`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:165
- Brief: Write hex-encoded key to write_fd, then clear the buffer.
- Parameters:
  - `write_fd` (int): Input parameter.
  - `key_material` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: ------------------------------------------------------------------------ Stdin key delivery (Feature 1) ------------------------------------------------------------------------ write_fd Write end of the stdin pipe (closed on return) key_material Key bytes write_fd Input parameter. key_material Input parameter. Return value. Calls: defined(), error(), reserve(), size(), std::setfill(), std::setw(), str(), data().

#### `bool directoryExists(const std::string &path)`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:196
- Brief: Directory Exists.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: path Input parameter. True when the operation succeeds. Calls: std::filesystem::is_directory().

#### `Result< std::string > executeCommandSafe(const std::vector< std::string > &args)`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:173
- Brief: Execute command safely via fork/execvp (no shell).
- Parameters:
  - `args` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: Execute Command Safe. args Input parameter. Return value. Calls: defined(), error(), empty(), PipeGuard::create(), isValid(), fork(), closeRead(), dup2().

#### `Result< std::string > executeCommandWithStdin(const std::vector< std::string > &args, const std::string &stdin_data)`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:109
- Brief: Execute a command with stdin_data written to its stdin.
- Parameters:
  - `args` (const std::vector< std::string > &): Input parameter.
  - `stdin_data` (const std::string &): Input parameter.
- Return: Child output on success, or error description
- Details: ------------------------------------------------------------------------ Public string-based stdin helpers (test/integration interface) ------------------------------------------------------------------------ Forks the process, executes args[0] via execvp, writes stdin_data to its standard input, and returns the collected stdout/stderr. args argv for execvp (args[0] is the executable) stdin_data Data to write to the child's stdin Child output on success, or error description args Input parameter. stdin_data Input parameter. Return value. Calls: defined(), error(), empty(), PipeGuard::create(), isValid(), fork(), closeWrite(), closeRead().

#### `Result< std::string > executeCommandWithStdin(const std::vector< std::string > &args, const std::vector< uint8_t > &key_material)`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:154
- Brief: Fork, exec gocryptfs and deliver the key via stdin pipe.
- Parameters:
  - `args` (const std::vector< std::string > &): Input parameter.
  - `key_material` (const std::vector< uint8_t > &): Input parameter.
- Return: Child stdout/stderr output on success, error otherwise
- Details: Execute Command With Stdin. The child receives the key as a hex string on STDIN. gocryptfs is invoked with "-passfile /dev/stdin" so that no key material touches the filesystem. The pipe write buffer is cleared with explicit_bzero after the write completes. args Full argv for execvp (args[0] = executable path) key_material Key bytes to hex-encode and pipe to stdin Child stdout/stderr output on success, error otherwise args Input parameter. key_material Input parameter. Return value. Calls: defined(), error(), empty(), PipeGuard::create(), isValid(), fork(), closeWrite(), closeRead().

#### `std::string getBackendName() const override`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:87
- Brief: Get backend name (e.g., "gocryptfs", "fscrypt").
- Parameters: none

#### `std::string getBackendVersion() const override`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:91
- Brief: Get backend version.
- Parameters: none

#### `Result< void > initialize(const std::string &config_json) override`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:67
- Brief: Initialize.
- Parameters:
  - `config_json` (const std::string &): Input parameter.
- Return: Return value.
- Details: config_json Input parameter. Return value. Implements initialize without additional internal calls.

#### `bool isMounted(const std::string &mount_point) override`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:85
- Brief: Is Mounted.
- Parameters:
  - `mount_point` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: mount_point Input parameter. True when the operation succeeds. Calls: mounts(), std::getline(), find(), executeCommandSafe(), isSuccess(), value().

#### `Result< void > mountContainer(const std::string &encrypted_dir, const std::string &mount_point, const std::vector< uint8_t > &key_material) override`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:75
- Brief: Mount Container.
- Parameters:
  - `encrypted_dir` (const std::string &): Input parameter.
  - `mount_point` (const std::string &): Input parameter.
  - `key_material` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: encrypted_dir Input parameter. mount_point Input parameter. key_material Input parameter. Return value. Calls: CommandArgumentValidator::validatePath(), isError(), error(), isMounted(), value(), resolveKey(), executeCommandWithStdin().

#### `Result< std::vector< uint8_t > > resolveKey(const std::string &encrypted_dir, const std::vector< uint8_t > &key_material, bool create_salt)`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:190
- Brief: Derive or return key for a container.
- Parameters:
  - `encrypted_dir` (const std::string &): Input parameter.
  - `key_material` (const std::vector< uint8_t > &): Input parameter.
  - `create_salt` (bool): Input parameter.
- Return: Return value.
- Details: Resolve Key. If a KDF service is configured: reads/writes the per-container salt file ("{encrypted_dir}/.themis_kdf_salt"), then derives the key via Argon2id. Otherwise returns key_material unchanged. encrypted_dir Ciphertext directory (salt file lives here) key_material Master key (or direct key when no KDF) create_salt When true, generate and persist a new salt encrypted_dir Input parameter. key_material Input parameter. create_salt Input parameter. Return value. Calls: generateSalt(), error(), std::string(), what(), out(), write(), data(), size().

#### `Result< void > unmountContainer(const std::string &mount_point) override`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:81
- Brief: Unmount Container.
- Parameters:
  - `mount_point` (const std::string &): Input parameter.
- Return: Return value.
- Details: mount_point Input parameter. Return value. Calls: isMounted(), CommandArgumentValidator::validatePath(), isError(), error(), value(), executeCommandSafe().

#### `~GocryptfsBackend() override`
- Source: `include/user_storage_encrypted/gocryptfs_backend.hpp`:64
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::GocryptfsBackend::Impl

#### `Impl()`
- Source: `src/user_storage_encrypted/gocryptfs_backend.cpp`:209
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::Group

#### `Group()`
- Source: `include/user_storage_encrypted/user_models.hpp`:54
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::HealthStatus

#### `HealthStatus()`
- Source: `include/user_storage_encrypted/user_models.hpp`:69
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::IKeyDerivationService

#### `Result< std::vector< uint8_t > > deriveKey(const std::vector< uint8_t > &master_key, const std::vector< uint8_t > &salt) const =0`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:41
- Brief: Derive a 256-bit (32-byte) key from master_key and salt.
- Parameters:
  - `master_key` (const std::vector< uint8_t > &): Raw key material (must not be empty).
  - `salt` (const std::vector< uint8_t > &): Per-container or per-user salt (must not be empty).
- Return: 32-byte derived key on success, or error.
- Details: master_key Raw key material (must not be empty). salt Per-container or per-user salt (must not be empty). 32-byte derived key on success, or error.

#### `Result< std::vector< uint8_t > > generateSalt() const =0`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:51
- Brief: Generate a cryptographically random salt suitable for this KDF.
- Parameters: none
- Return: 32-byte random salt.
- Details: 32-byte random salt.

#### `Result< std::vector< uint8_t > > loadOrCreateSalt(const std::string &salt_file_path) const =0`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:59
- Brief: Load salt from a file, creating it with generateSalt() if absent.
- Parameters:
  - `salt_file_path` (const std::string &): Path to the salt file (e.g., ".themis_kdf_salt").
- Return: Salt bytes.
- Details: salt_file_path Path to the salt file (e.g., ".themis_kdf_salt"). Salt bytes.

#### `~IKeyDerivationService()=default`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:32
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::IRotationStore

#### `bool get(const std::string &key, std::string &out) const =0`
- Source: `include/user_storage_encrypted/irotation_store.hpp`:47
- Brief: Read a value by string key.
- Parameters:
  - `key` (const std::string &): Storage key
  - `out` (std::string &): Value (set only when true is returned)
- Return: true if the key existed
- Details: key Storage key out Value (set only when true is returned) true if the key existed

#### `Result< int64_t > load(SecurityLevel level) const`
- Source: `include/user_storage_encrypted/irotation_store.hpp`:72
- Brief: Load the last-check timestamp for level (returns 0 if not found).
- Parameters:
  - `level` (SecurityLevel): n/a

#### `bool put(const std::string &key, const std::string &value)=0`
- Source: `include/user_storage_encrypted/irotation_store.hpp`:53
- Brief: Write a key-value pair.
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a
- Return: true on success
- Details: true on success

#### `Result< void > save(SecurityLevel level, int64_t last_check_ms)`
- Source: `include/user_storage_encrypted/irotation_store.hpp`:60
- Brief: Persist the last-check timestamp for level.
- Parameters:
  - `level` (SecurityLevel): n/a
  - `last_check_ms` (int64_t): n/a

#### `~IRotationStore()=default`
- Source: `include/user_storage_encrypted/irotation_store.hpp`:39
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::KeyDerivationService

#### `std::vector< uint8_t > derive(const std::vector< uint8_t > &master_key, const std::string &user_id, const std::string &container_id, const std::vector< uint8_t > &salt)=0`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:71
- Brief: n/a
- Parameters:
  - `master_key` (const std::vector< uint8_t > &): n/a
  - `user_id` (const std::string &): n/a
  - `container_id` (const std::string &): n/a
  - `salt` (const std::vector< uint8_t > &): n/a

#### `std::vector< uint8_t > generateSalt(size_t length)=0`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:78
- Brief: n/a
- Parameters:
  - `length` (size_t): n/a

#### `~KeyDerivationService()=default`
- Source: `include/user_storage_encrypted/key_derivation_service.hpp`:69
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::KeyRotationScheduler

#### `KeyRotationScheduler()`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:52
- Brief: n/a
- Parameters: none

#### `void cancelRotation(SecurityLevel level)`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:107
- Brief: Cancel scheduled rotation for a level.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
- Details: Cancel Rotation. level Input parameter. Calls: lock(), erase().

#### `int64_t getCurrentTimeMs() const`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:135
- Brief: n/a
- Parameters: none

#### `int64_t getNextRotationTime(SecurityLevel level)`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:123
- Brief: Get next rotation time for a level.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
- Return: Timestamp in milliseconds, 0 if not scheduled
- Details: Get Next Rotation Time. Timestamp in milliseconds, 0 if not scheduled level Input parameter. Return value. Calls: lock(), find(), end().

#### `Result< void > initialize(int check_interval_seconds=3600, std::shared_ptr< IRotationStore > store=nullptr)`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:79
- Brief: Initialize scheduler, optionally loading persisted rotation state.
- Parameters:
  - `check_interval_seconds` (int): Input parameter.
  - `store` (std::shared_ptr< IRotationStore >): Input parameter.
- Return: Return value.
- Details: Initialize. When a non-null store is supplied the scheduler loads any previously persisted last_check_ms values for each SecurityLevel so that rotation intervals survive process restarts. After each successful callback invocation the updated state is written back to the store. RocksDB integration: autostore=makeRocksDBRotationStore(&rocksdb_wrapper); scheduler.initialize(3600,std::move(store)); check_interval_seconds How often to check for rotation needs. store Optional persistence backend (may be nullptr). check_interval_seconds Input parameter. store Input parameter. Return value. Calls: error(), std::move(), std::thread(), schedulerLoop().

#### `bool isRotationDue(SecurityLevel level, int64_t last_rotation_ms)`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:116
- Brief: Check if rotation is due for a level.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
  - `last_rotation_ms` (int64_t): Input parameter.
- Return: true if rotation is due
- Details: Is Rotation Due. level Security level last_rotation_ms Last rotation timestamp true if rotation is due level Input parameter. last_rotation_ms Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), getCurrentTimeMs().

#### `void loadRotationState(SecurityLevel level)`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:138
- Brief: Load Rotation State.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
- Details: level Input parameter. Calls: find(), end(), securityLevelToString(), get(), nlohmann::json::parse(), contains().

#### `void persistRotationState(SecurityLevel level)`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:137
- Brief: ------------------------------------------------------------------------ Persistence helpers (called with mutex held) ------------------------------------------------------------------------
- Parameters:
  - `level` (SecurityLevel): Input parameter.
- Details: level Input parameter. Calls: find(), end(), securityLevelToString(), put(), dump().

#### `Result< void > scheduleRotation(SecurityLevel level, int interval_days, bool auto_rotate, RotationCallback callback)`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:97
- Brief: Schedule rotation for a security level.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
  - `interval_days` (int): Input parameter.
  - `auto_rotate` (bool): Input parameter.
  - `callback` (RotationCallback): Input parameter.
- Return: Return value.
- Details: Schedule Rotation. level Security level to rotate interval_days Rotation interval in days auto_rotate Enable automatic rotation callback Callback function for rotation events level Input parameter. interval_days Input parameter. auto_rotate Input parameter. callback Input parameter. Return value. Calls: lock(), getCurrentTimeMs(), securityLevelToString(), get(), nlohmann::json::parse(), contains().

#### `void schedulerLoop()`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:134
- Brief: Scheduler Loop.
- Parameters: none
- Details: Calls: lock(), getCurrentTimeMs(), callback(), persistRotationState(), cv_lock(), wait_for(), std::chrono::seconds(), load().

#### `void setRotationStore(std::shared_ptr< IRotationStore > store)`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:60
- Brief: Attach a persistence store for last_check_ms.
- Parameters:
  - `store` (std::shared_ptr< IRotationStore >): Input parameter.
- Details: Set Rotation Store. Must be called before initialize(). Defaults to NullRotationStore. store Input parameter. Calls: lock(), std::move().

#### `void shutdown()`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:87
- Brief: Shutdown scheduler.
- Parameters: none
- Details: Shutdown. Calls: notify_all(), joinable(), join().

#### `void triggerRotation(SecurityLevel level)`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:128
- Brief: Manually trigger rotation check for a level (for testing).
- Parameters:
  - `level` (SecurityLevel): Input parameter.
- Details: Trigger Rotation. level Input parameter. Calls: lock(), find(), end(), getCurrentTimeMs(), callback(), persistRotationState().

#### `~KeyRotationScheduler()`
- Source: `include/user_storage_encrypted/key_rotation_scheduler.hpp`:53
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::KeyRotationScheduler::Impl

#### `Impl()`
- Source: `src/user_storage_encrypted/key_rotation_scheduler.cpp`:53
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::LevelConfig

#### `LevelConfig()`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:56
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::MultiLevelEncryptedStorage

#### `MultiLevelEncryptedStorage()`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:106
- Brief: n/a
- Parameters: none

#### `Result< HealthStatus > checkHealth()`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:157
- Brief: Check Health.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), checkLevelHealth(), isError(), push_back(), error(), value().

#### `Result< HealthStatus > checkLevelHealth(SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:158
- Brief: Check Level Health.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), find(), end(), isMounted().

#### `Result< void > createGroup(const Group &group, SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:143
- Brief: Group Management API Implementation.
- Parameters:
  - `group` (const Group &): Input parameter.
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: group Input parameter. level Input parameter. Return value. Calls: getGroupPath(), empty(), error(), writeGroupFile().

#### `Result< void > createUser(const User &user, SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:136
- Brief: User Management API Implementation.
- Parameters:
  - `user` (const User &): Input parameter.
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: user Input parameter. level Input parameter. Return value. Calls: getUserPath(), empty(), error(), writeUserFile().

#### `Result< void > deleteGroup(const std::string &group_id, SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:146
- Brief: Delete Group.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: group_id Identifier of the group. level Input parameter. Return value. Calls: getGroupPath(), empty(), error(), std::remove(), c_str().

#### `Result< void > deleteUser(const std::string &user_id, SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:139
- Brief: Delete User.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: user_id Identifier of the user. level Input parameter. Return value. Calls: getUserPath(), empty(), error(), std::remove(), c_str().

#### `std::string getBasePath(SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:222
- Brief: Get Base Path.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Calls: find(), end().

#### `PluginCapabilities getCapabilities() const override`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:122
- Brief: n/a
- Parameters: none

#### `Result< Group > getGroup(const std::string &group_id, SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:144
- Brief: Get Group.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: group_id Identifier of the group. level Input parameter. Return value. Calls: getGroupPath(), empty(), error(), readGroupFile().

#### `std::string getGroupPath(SecurityLevel level, const std::string &group_id)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:221
- Brief: Get Group Path.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
  - `group_id` (const std::string &): Identifier of the group.
- Return: Return value.
- Details: level Input parameter. group_id Identifier of the group. Return value. Calls: getBasePath(), empty().

#### `void * getInstance() override`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:133
- Brief: n/a
- Parameters: none

#### `Result< std::shared_ptr< KeyProvider > > getKeyProvider(const LevelConfig &config)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:211
- Brief: Get Key Provider.
- Parameters:
  - `config` (const LevelConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value. Calls: find(), end(), empty(), error(), std::getenv(), initialize().

#### `std::string getMetricsText() const`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:173
- Brief: Emit Prometheus text format (v0.0.4) for encrypted-storage metrics.
- Parameters: none
- Details: Exposed metric families: user_storage_mounts_active Gauge Currently mounted containers user_storage_mount_operations_total Counter Total mount + unmount ops (label: operation) user_storage_key_rotations_total Counter Key rotation callbacks fired (label: level) user_storage_container_size_bytes Gauge Sum of encrypted container sizes on disk Thread-safe (reads std::atomic values).

#### `const char * getName() const override`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:110
- Brief: n/a
- Parameters: none

#### `PluginType getType() const override`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:118
- Brief: n/a
- Parameters: none

#### `Result< User > getUser(const std::string &user_id, SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:137
- Brief: Get User.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: user_id Identifier of the user. level Input parameter. Return value. Calls: getUserPath(), empty(), error(), readUserFile().

#### `std::string getUserPath(SecurityLevel level, const std::string &user_id)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:220
- Brief: Get User Path.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: level Input parameter. user_id Identifier of the user. Return value. Calls: getBasePath(), empty().

#### `const char * getVersion() const override`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:114
- Brief: n/a
- Parameters: none

#### `bool initialize(const char *config_json) override`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:131
- Brief: Initialize.
- Parameters:
  - `config_json` (const char *): Input parameter.
- Return: True when the operation succeeds.
- Details: config_json Input parameter. True when the operation succeeds. Calls: loadConfiguration(), isError(), validateConfiguration(), reconcileStaleMounts(), empty(), rfind(), substr(), insert().

#### `Result< void > initializeLevel(const LevelConfig &config)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:192
- Brief: Initialize Level.
- Parameters:
  - `config` (const LevelConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value. Calls: lock(), initialize(), isError(), checkAvailability(), getKeyProvider(), error(), mountLevel(), std::filesystem::exists().

#### `Result< std::vector< Group > > listGroups(SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:147
- Brief: List Groups.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Calls: getBasePath(), empty(), error(), std::filesystem::path(), std::filesystem::exists(), std::filesystem::is_directory(), std::filesystem::directory_iterator(), is_regular_file().

#### `Result< std::vector< User > > listUsers(SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:140
- Brief: List Users.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Calls: getBasePath(), empty(), error(), std::filesystem::path(), std::filesystem::exists(), std::filesystem::is_directory(), std::filesystem::directory_iterator(), is_regular_file().

#### `Result< void > loadConfiguration(const std::string &config_json)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:188
- Brief: Load Configuration.
- Parameters:
  - `config_json` (const std::string &): Input parameter.
- Return: Return value.
- Details: config_json Input parameter. Return value. Calls: json::parse(), contains(), LevelConfig(), error(), value(), stringToSecurityLevel(), std::string(), what().

#### `Result< void > mountAll()`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:150
- Brief: Mount All.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: mountLevel(), isError().

#### `Result< void > mountLevel(SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:152
- Brief: Mount Level.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Calls: find(), end(), error(), securityLevelToString().

#### `Result< void > mountLevel(const LevelConfig &config)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:193
- Brief: Mount Level.
- Parameters:
  - `config` (const LevelConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value. Calls: find(), end(), error(), getKeyProvider(), isError(), value(), getKey(), std::string().

#### `Result< Group > readGroupFile(const std::string &path)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:217
- Brief: Read Group File.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: path Input parameter. Return value. Calls: file(), is_open(), error(), value(), stringToSecurityLevel(), std::string(), what().

#### `Result< User > readUserFile(const std::string &path)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:215
- Brief: Read User File.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: path Input parameter. Return value. Calls: file(), is_open(), error(), value(), stringToSecurityLevel(), std::string(), what().

#### `void reconcileStaleMounts()`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:225
- Brief: Reconcile Stale Mounts.
- Parameters: none
- Details: Calls: empty(), push_back(), mounts_file(), std::getline(), iss(), find(), fork(), c_str().

#### `void reconcileStaleMounts(const std::string &base_path)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:208
- Brief: Reconcile stale gocryptfs mounts left over from a previous crash.
- Parameters:
  - `base_path` (const std::string &): Path to the base.
- Details: Reconcile Stale Mounts. Scans /proc/mounts for any mount point that is a direct child of base_path and is currently not among the configured mount points. Each stale mount is unmounted via "fusermount -u" (Linux) / "umount" (macOS). A WARN-level log message is emitted per stale mount; if unmounting fails the error is logged and startup continues — it is never fatal. base_path Directory prefix to scan (e.g. "/var/lib/themisdb"). base_path Path to the base. Calls: empty(), insert(), defined(), mounts(), std::getline(), iss(), size(), compare().

#### `void recordKeyRotation(SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:181
- Brief: Manually record a key rotation event for level.
- Parameters:
  - `level` (SecurityLevel): n/a
- Details: Record Key Rotation. Called automatically by rotateKey(); exposed for testing and for callers that manage rotation outside this class. SecurityLevel Input parameter. Implements recordKeyRotation without additional internal calls.

#### `Result< void > rotateKey(SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:154
- Brief: Rotate Key.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Calls: find(), end(), error(), securityLevelToString(), getKeyProvider(), isError(), value(), getKey().

#### `void shutdown() override`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:132
- Brief: Shutdown.
- Parameters: none
- Details: Calls: unmountAll(), clear().

#### `Result< void > unmountAll()`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:151
- Brief: Unmount All.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: unmountLevel().

#### `Result< void > unmountLevel(SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:153
- Brief: Unmount Level.
- Parameters:
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Calls: find(), end(), error(), securityLevelToString().

#### `Result< void > unmountLevel(const LevelConfig &config)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:194
- Brief: Unmount Level.
- Parameters:
  - `config` (const LevelConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value. Calls: find(), end(), unmountContainer(), isSuccess().

#### `Result< void > updateGroup(const Group &group, SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:145
- Brief: Update Group.
- Parameters:
  - `group` (const Group &): Input parameter.
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: group Input parameter. level Input parameter. Return value. Calls: createGroup().

#### `Result< void > updateUser(const User &user, SecurityLevel level)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:138
- Brief: Update User.
- Parameters:
  - `user` (const User &): Input parameter.
  - `level` (SecurityLevel): Input parameter.
- Return: Return value.
- Details: user Input parameter. level Input parameter. Return value. Calls: createUser().

#### `Result< void > validateConfiguration()`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:189
- Brief: Validate Configuration.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: empty(), error().

#### `Result< void > writeGroupFile(const std::string &path, const Group &group)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:216
- Brief: Write Group File.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `group` (const Group &): Input parameter.
- Return: Return value.
- Details: path Input parameter. group Input parameter. Return value. Calls: securityLevelToString(), std::filesystem::path(), parent_path(), empty(), std::filesystem::create_directories(), error(), string(), message().

#### `Result< void > writeUserFile(const std::string &path, const User &user)`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:214
- Brief: Write User File.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `user` (const User &): Input parameter.
- Return: Return value.
- Details: path Input parameter. user Input parameter. Return value. Calls: securityLevelToString(), std::filesystem::path(), parent_path(), empty(), std::filesystem::create_directories(), error(), string(), message().

#### `~MultiLevelEncryptedStorage() override`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:107
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::MultiLevelEncryptedStorage::Impl

#### `Impl()`
- Source: `src/user_storage_encrypted/multi_level_storage.cpp`:51
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::NullRotationStore

#### `bool get(const std::string &, std::string &) const override`
- Source: `include/user_storage_encrypted/irotation_store.hpp`:94
- Brief: Read a value by string key.
- Parameters:
  - `key` (const std::string &): Storage key
  - `out` (std::string &): Value (set only when true is returned)
- Return: true if the key existed
- Details: key Storage key out Value (set only when true is returned) true if the key existed

#### `bool put(const std::string &, const std::string &) override`
- Source: `include/user_storage_encrypted/irotation_store.hpp`:97
- Brief: Write a key-value pair.
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a
- Return: true on success
- Details: true on success

### themis::plugins::user_storage::PipeGuard

#### `PipeGuard() noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:63
- Brief: Default constructor: creates an invalid (empty) pipe.
- Parameters: none
- Details: Call create() to initialize, or move-assign from another PipeGuard.

#### `PipeGuard(PipeGuard &&other) noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:89
- Brief: Move constructor: takes ownership of another pipe.
- Parameters:
  - `other` (PipeGuard &&): Pipe to move from (will be invalidated)
- Details: other Pipe to move from (will be invalidated)

#### `PipeGuard(const PipeGuard &)=delete`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PipeGuard &): n/a

#### `bool closeAll() noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:230
- Brief: Close both ends.
- Parameters: none
- Return: true if both closes succeeded or were already closed
- Details: Closes both read and write ends. Errors are only reported for the first failed close (others are attempted regardless). true if both closes succeeded or were already closed

#### `bool closeRead() noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:196
- Brief: Close the read end (index 0).
- Parameters: none
- Return: true if close succeeded or was already closed, false on close() error
- Details: If already closed (fd == -1), this is a no-op. If close() fails, errno is set and false is returned. true if close succeeded or was already closed, false on close() error

#### `bool closeWrite() noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:213
- Brief: Close the write end (index 1).
- Parameters: none
- Return: true if close succeeded or was already closed, false on close() error
- Details: If already closed (fd == -1), this is a no-op. If close() fails, errno is set and false is returned. true if close succeeded or was already closed, false on close() error

#### `PipeGuard create() noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:123
- Brief: Create a new pipe.
- Parameters: none
- Return: PipeGuard with newly created pipe, or invalid PipeGuard on error
- Details: Calls pipe() to create a pipe pair. On failure, returns an invalid PipeGuard (isValid() returns false) and errno is set. PipeGuard with newly created pipe, or invalid PipeGuard on error

#### `int detach(size_t index) noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:246
- Brief: Manually detach (release) an end without closing.
- Parameters:
  - `index` (size_t): 0 for read end, 1 for write end
- Return: File descriptor at that index, or -1 if already closed
- Details: Useful for passing ownership of a pipe end to another process/owner. index 0 for read end, 1 for write end File descriptor at that index, or -1 if already closed After calling this, the caller is responsible for closing the fd

#### `bool isReadOpen() const noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:147
- Brief: Check if read end is open.
- Parameters: none

#### `bool isValid() const noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:140
- Brief: Check if pipe is valid (both fds open).
- Parameters: none
- Return: true if read and write ends are both open (fd >= 0)
- Details: true if read and write ends are both open (fd >= 0)

#### `bool isWriteOpen() const noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:154
- Brief: Check if write end is open.
- Parameters: none

#### `PipeGuard & operator=(PipeGuard &&other) noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:104
- Brief: Move assignment: takes ownership of another pipe.
- Parameters:
  - `other` (PipeGuard &&): Pipe to move from (will be invalidated)
- Return: Reference to this
- Details: Closes any existing file descriptors before taking ownership. other Pipe to move from (will be invalidated) Reference to this

#### `PipeGuard & operator=(const PipeGuard &)=delete`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PipeGuard &): n/a

#### `int operator[](size_t index) const noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:181
- Brief: Access read end by index [0].
- Parameters:
  - `index` (size_t): n/a
- Return: File descriptor at index 0
- Details: File descriptor at index 0

#### `int readFd() const noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:163
- Brief: Get read end file descriptor.
- Parameters: none
- Return: File descriptor, or -1 if already closed
- Details: File descriptor, or -1 if already closed

#### `std::string status() const noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:260
- Brief: Get remaining owner of this pipe (for debug/logging).
- Parameters: none
- Return: Status string like "rw" (both), "r" (read only), "w" (write only), "-" (closed)
- Details: Status string like "rw" (both), "r" (read only), "w" (write only), "-" (closed)

#### `int writeFd() const noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:172
- Brief: Get write end file descriptor.
- Parameters: none
- Return: File descriptor, or -1 if already closed
- Details: File descriptor, or -1 if already closed

#### `~PipeGuard() noexcept`
- Source: `include/user_storage_encrypted/pipe_guard.hpp`:75
- Brief: Destructor: closes all open file descriptors.
- Parameters: none
- Details: Both read and write ends are closed. If close() fails, errno is set but no exception is thrown. noexcept: safe to call from other destructors

### themis::plugins::user_storage::Result

#### `Result()`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:65
- Brief: n/a
- Parameters: none

#### `Result(T value)`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:32
- Brief: n/a
- Parameters:
  - `value` (T): n/a

#### `const std::string & error() const`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:62
- Brief: n/a
- Parameters: none

#### `Result error(const std::string &error_msg)`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:38
- Brief: n/a
- Parameters:
  - `error_msg` (const std::string &): n/a

#### `bool isError() const`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:46
- Brief: n/a
- Parameters: none

#### `bool isSuccess() const`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:45
- Brief: n/a
- Parameters: none

#### `T & value()`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:55
- Brief: n/a
- Parameters: none

#### `const T & value() const`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:48
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::Result< void >

#### `Result()`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:77
- Brief: n/a
- Parameters: none

#### `const std::string & error() const`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:88
- Brief: n/a
- Parameters: none

#### `Result error(const std::string &error_msg)`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:79
- Brief: n/a
- Parameters:
  - `error_msg` (const std::string &): n/a

#### `bool isError() const`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:87
- Brief: n/a
- Parameters: none

#### `bool isSuccess() const`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:86
- Brief: n/a
- Parameters: none

#### `const void & value() const`
- Source: `include/user_storage_encrypted/encryption_backend_interface.hpp`:48
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::RotationSchedule

#### `RotationSchedule()`
- Source: `src/user_storage_encrypted/key_rotation_scheduler.cpp`:33
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::StorageMetrics

#### `StorageMetrics()=default`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:81
- Brief: n/a
- Parameters: none

#### `StorageMetrics(const StorageMetrics &)=delete`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StorageMetrics &): n/a

#### `StorageMetrics & operator=(const StorageMetrics &)=delete`
- Source: `include/user_storage_encrypted/multi_level_storage.hpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StorageMetrics &): n/a

### themis::plugins::user_storage::TimedFileOperation

#### `TimedFileOperation(const TimedFileOperation &)=delete`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TimedFileOperation &): n/a

#### `TimedFileOperation(int fd, DurationMs timeout=DurationMs(5000), IoTimeoutPolicy policy=IoTimeoutPolicy::kPerOperation) noexcept`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:86
- Brief: Construct a timed I/O operator for a file descriptor.
- Parameters:
  - `fd` (int): File descriptor to operate on (not owned)
  - `timeout` (DurationMs): Maximum time to wait for I/O to complete
  - `policy` (IoTimeoutPolicy): How to interpret timeout (absolute or per-operation)
- Details: fd File descriptor to operate on (not owned) timeout Maximum time to wait for I/O to complete policy How to interpret timeout (absolute or per-operation) Does NOT close the file descriptor on destruction fd must be valid and not closed while this object is in use

#### `TimedFileOperation(int fd, std::chrono::duration< Rep, Period > timeout, IoTimeoutPolicy policy=IoTimeoutPolicy::kPerOperation) noexcept`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:101
- Brief: Construct with std::chrono duration of any type.
- Parameters:
  - `fd` (int): n/a
  - `timeout` (std::chrono::duration< Rep, Period >): n/a
  - `policy` (IoTimeoutPolicy): n/a

#### `int fd() const noexcept`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:232
- Brief: Get the file descriptor.
- Parameters: none

#### `int getRemainingTimeoutMs() const noexcept`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:249
- Brief: Get remaining time for absolute timeouts (or timeout_ if per-operation).
- Parameters: none
- Return: Milliseconds remaining, or negative if expired
- Details: Milliseconds remaining, or negative if expired

#### `bool hasTimedOut() const noexcept`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:222
- Brief: Check if operation has timed out.
- Parameters: none
- Return: true if total elapsed time >= timeout (in absolute policy)
- Details: true if total elapsed time >= timeout (in absolute policy)

#### `TimedFileOperation & operator=(const TimedFileOperation &)=delete`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TimedFileOperation &): n/a

#### `int poll_for_readability(int timeout_ms) const noexcept`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:265
- Brief: Wait for fd to be readable (using poll).
- Parameters:
  - `timeout_ms` (int): Timeout in milliseconds (-1 = forever)
- Return: 1 if readable, 0 if timeout, -1 if error
- Details: timeout_ms Timeout in milliseconds (-1 = forever) 1 if readable, 0 if timeout, -1 if error

#### `int poll_for_writability(int timeout_ms) const noexcept`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:275
- Brief: Wait for fd to be writable (using poll).
- Parameters:
  - `timeout_ms` (int): Timeout in milliseconds (-1 = forever)
- Return: 1 if writable, 0 if timeout, -1 if error
- Details: timeout_ms Timeout in milliseconds (-1 = forever) 1 if writable, 0 if timeout, -1 if error

#### `std::optional< ssize_t > read(void *buffer, size_t count) noexcept`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:133
- Brief: Non-blocking read with timeout.
- Parameters:
  - `buffer` (void *): Buffer to read into (not null)
  - `count` (size_t): Number of bytes to attempt to read
- Return: Number of bytes read, or empty optional on timeout/error
- Details: Attempts to read up to count bytes from the file descriptor. If the read would block, waits up to timeout_ milliseconds using poll(). buffer Buffer to read into (not null) count Number of bytes to attempt to read Number of bytes read, or empty optional on timeout/error Error codes (errno): EAGAIN: Timeout occurred EBADF: Invalid file descriptor EIO: I/O error Other POSIX errors from read()

#### `int timeoutMs() const noexcept`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:237
- Brief: Get configured timeout in milliseconds.
- Parameters: none

#### `std::optional< ssize_t > write(const void *buffer, size_t count) noexcept`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:183
- Brief: Non-blocking write with timeout.
- Parameters:
  - `buffer` (const void *): Buffer to write from (not null)
  - `count` (size_t): Number of bytes to write
- Return: Number of bytes written, or empty optional on timeout/error
- Details: Attempts to write count bytes to the file descriptor. If the write would block, waits up to timeout_ milliseconds using poll(). buffer Buffer to write from (not null) count Number of bytes to write Number of bytes written, or empty optional on timeout/error Error codes (errno): EAGAIN: Timeout occurred EBADF: Invalid file descriptor EPIPE: Pipe closed by peer Other POSIX errors from write()

#### `~TimedFileOperation() noexcept=default`
- Source: `include/user_storage_encrypted/timed_file_operation.hpp`:115
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::User

#### `User()`
- Source: `include/user_storage_encrypted/user_models.hpp`:36
- Brief: n/a
- Parameters: none

### themis::plugins::user_storage::test

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_01_PipeGuardCreation)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_01_PipeGuardCreation): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_02_PipeGuardCloseOperations)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_02_PipeGuardCloseOperations): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_03_PipeGuardMoveSemantics)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_03_PipeGuardMoveSemantics): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_04_TimedFileOperationReadAvailable)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_04_TimedFileOperationReadAvailable): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_05_TimedFileOperationWriteAvailable)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_05_TimedFileOperationWriteAvailable): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_06_TimedFileOperationReadTimeout)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_06_TimedFileOperationReadTimeout): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_07_PipeGuardDetach)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_07_PipeGuardDetach): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_08_MultipleIoCycles)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_08_MultipleIoCycles): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_09_PipeGuardOperatorAccess)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_09_PipeGuardOperatorAccess): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_10_TimedFileOperationDifferentTimeouts)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_10_TimedFileOperationDifferentTimeouts): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_11_ExceptionSafety)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_11_ExceptionSafety): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_12_PipeGuardDefaultConstructor)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_12_PipeGuardDefaultConstructor): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_13_TimedFileOperationErrorHandling)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:313
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_13_TimedFileOperationErrorHandling): n/a

#### `TEST(UserStorageEncryptedIoTimeout, UST_IO_14_PipeGuardNonCopyable)`
- Source: `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedIoTimeout): n/a
  - `<unnamed>` (UST_IO_14_PipeGuardNonCopyable): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_01_PipeGuardRAII)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_01_PipeGuardRAII): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_02_PipeGuardMoveSemantics)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_02_PipeGuardMoveSemantics): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_03_PipeGuardSelectiveClose)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_03_PipeGuardSelectiveClose): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_04_PipeGuardConcurrentMoves)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_04_PipeGuardConcurrentMoves): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_05_PipeGuardDestructorNoThrow)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_05_PipeGuardDestructorNoThrow): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_06_GocryptfsBackendInitialization)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_06_GocryptfsBackendInitialization): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_07_TimedFileOperationCreation)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_07_TimedFileOperationCreation): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_08_UniquePtr_ExceptionSafety)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_08_UniquePtr_ExceptionSafety): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_09_CommandArgumentValidator_Paths)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_09_CommandArgumentValidator_Paths): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_10_SecureZero)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_10_SecureZero): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_11_ExceptionSafeKeyMaterial)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_11_ExceptionSafeKeyMaterial): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_12_PipeGuardNoLeaks)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_12_PipeGuardNoLeaks): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_13_ConstReferenceLoops)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_13_ConstReferenceLoops): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_14_MoveSemanticsVectors)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_14_MoveSemanticsVectors): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_15_VectorPreallocation)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:283
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_15_VectorPreallocation): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_16_ConstReferenceParams)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:302
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_16_ConstReferenceParams): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_17_FailClosedBehavior)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:318
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_17_FailClosedBehavior): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_18_ProcessExecutionTimeout)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:335
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_18_ProcessExecutionTimeout): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_19_BoundaryInputValidation)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_19_BoundaryInputValidation): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_20_ExceptionSafetyGuarantees)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:367
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_20_ExceptionSafetyGuarantees): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_21_DestructorExceptionSafety)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:387
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_21_DestructorExceptionSafety): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_22_ZeroInitialization)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:416
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_22_ZeroInitialization): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_23_RAIIFileHandles)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:429
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_23_RAIIFileHandles): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_24_PlatformCompatibility)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:463
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_24_PlatformCompatibility): n/a

#### `TEST(UserStoragePhase1Hardening, USE_PHASE1_25_AcceptanceCriteria)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_phase1_hardening.cpp`:480
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStoragePhase1Hardening): n/a
  - `<unnamed>` (USE_PHASE1_25_AcceptanceCriteria): n/a

#### `TEST_F(ErrorCodesTest, CustomHandlerCanBeRegistered)`
- Source: `tests/user_storage_encrypted/test_phase2_error_codes_focused.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorCodesTest): n/a
  - `<unnamed>` (CustomHandlerCanBeRegistered): n/a

#### `TEST_F(ErrorCodesTest, DiagnosticEventEmissionWorks)`
- Source: `tests/user_storage_encrypted/test_phase2_error_codes_focused.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorCodesTest): n/a
  - `<unnamed>` (DiagnosticEventEmissionWorks): n/a

#### `TEST_F(ErrorCodesTest, DiagnosticEventJsonSerialization)`
- Source: `tests/user_storage_encrypted/test_phase2_error_codes_focused.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorCodesTest): n/a
  - `<unnamed>` (DiagnosticEventJsonSerialization): n/a

#### `TEST_F(ErrorCodesTest, ErrorCodeCategoriesAreCorrect)`
- Source: `tests/user_storage_encrypted/test_phase2_error_codes_focused.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorCodesTest): n/a
  - `<unnamed>` (ErrorCodeCategoriesAreCorrect): n/a

#### `TEST_F(ErrorCodesTest, ErrorCodeEnumValuesAreValid)`
- Source: `tests/user_storage_encrypted/test_phase2_error_codes_focused.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorCodesTest): n/a
  - `<unnamed>` (ErrorCodeEnumValuesAreValid): n/a

#### `TEST_F(ErrorCodesTest, ErrorCodeToStringMappingIsComplete)`
- Source: `tests/user_storage_encrypted/test_phase2_error_codes_focused.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorCodesTest): n/a
  - `<unnamed>` (ErrorCodeToStringMappingIsComplete): n/a

#### `TEST_F(ErrorCodesTest, ErrorEventWithRemediation)`
- Source: `tests/user_storage_encrypted/test_phase2_error_codes_focused.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorCodesTest): n/a
  - `<unnamed>` (ErrorEventWithRemediation): n/a

#### `TEST_F(ErrorCodesTest, MultipleEventsAreCapatured)`
- Source: `tests/user_storage_encrypted/test_phase2_error_codes_focused.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorCodesTest): n/a
  - `<unnamed>` (MultipleEventsAreCapatured): n/a

### themis::plugins::user_storage::test::ErrorCodesTest

#### `void SetUp() override`
- Source: `tests/user_storage_encrypted/test_phase2_error_codes_focused.cpp`:21
- Brief: n/a
- Parameters: none

### themis::user_storage_encrypted

#### `bool isUserStorageEncryptedFailClosed(UserStorageEncryptedError e) noexcept`
- Source: `include/user_storage_encrypted/user_storage_encrypted_api_contract.h`:143
- Brief: Returns true when the given error mandates fail-closed denial of storage access.
- Parameters:
  - `e` (UserStorageEncryptedError): n/a

### themis::user_storage_encrypted::test

#### `TEST(CommandInjectionPrevention, USEG_INJ_01_AbsolutePathValidation)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_01_AbsolutePathValidation): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_02_RelativePathValidation)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_02_RelativePathValidation): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_03_PathTraversalRejection)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_03_PathTraversalRejection): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_04_ShellMetacharacterRejection)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_04_ShellMetacharacterRejection): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_05_ParenthesesAndBracesRejection)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_05_ParenthesesAndBracesRejection): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_06_EnvironmentVariableRejection)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_06_EnvironmentVariableRejection): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_07_EscapeAndQuoteRejection)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_07_EscapeAndQuoteRejection): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_08_EdgeCasesRejection)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_08_EdgeCasesRejection): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_09_ValidHexKeyFormat)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_09_ValidHexKeyFormat): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_10_InvalidHexKeyFormat)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_10_InvalidHexKeyFormat): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_11_SafeFlagsValidation)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:302
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_11_SafeFlagsValidation): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_12_DangerousFlagsRejection)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:323
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_12_DangerousFlagsRejection): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_13_SQLInjectionPattern)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:360
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_13_SQLInjectionPattern): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_14_LDAPInjectionPattern)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_14_LDAPInjectionPattern): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_15_XMLInjectionPattern)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:389
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_15_XMLInjectionPattern): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_16_UnicodeEncodingAttacks)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:413
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_16_UnicodeEncodingAttacks): n/a

#### `TEST(CommandInjectionPrevention, USEG_INJ_17_FuzzTestRandomMutations)`
- Source: `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`:440
- Brief: n/a
- Parameters:
  - `<unnamed>` (CommandInjectionPrevention): n/a
  - `<unnamed>` (USEG_INJ_17_FuzzTestRandomMutations): n/a

#### `TEST(UserStorageEncryptedContractHardening, USE01_ErrorCodeUniqueness)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedContractHardening): n/a
  - `<unnamed>` (USE01_ErrorCodeUniqueness): n/a

#### `TEST(UserStorageEncryptedContractHardening, USE02_ErrorCodeRange)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedContractHardening): n/a
  - `<unnamed>` (USE02_ErrorCodeRange): n/a

#### `TEST(UserStorageEncryptedContractHardening, USE03_SwitchDispatch)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedContractHardening): n/a
  - `<unnamed>` (USE03_SwitchDispatch): n/a

#### `TEST(UserStorageEncryptedContractHardening, USE04_EncryptedMountDescriptorDefaults)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedContractHardening): n/a
  - `<unnamed>` (USE04_EncryptedMountDescriptorDefaults): n/a

#### `TEST(UserStorageEncryptedContractHardening, USE05_KeyDerivationRequestDefaults)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedContractHardening): n/a
  - `<unnamed>` (USE05_KeyDerivationRequestDefaults): n/a

#### `TEST(UserStorageEncryptedContractHardening, USE06_EncryptedMountDescriptorCopy)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedContractHardening): n/a
  - `<unnamed>` (USE06_EncryptedMountDescriptorCopy): n/a

#### `TEST(UserStorageEncryptedContractHardening, USE07_KeyDerivationRequestMove)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedContractHardening): n/a
  - `<unnamed>` (USE07_KeyDerivationRequestMove): n/a

#### `TEST(UserStorageEncryptedContractHardening, USE08_FailClosedPredicate)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (UserStorageEncryptedContractHardening): n/a
  - `<unnamed>` (USE08_FailClosedPredicate): n/a

#### `TEST_F(E2EIntegrationTest, E2E_01_LifecycleOFFEN)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (E2EIntegrationTest): n/a
  - `<unnamed>` (E2E_01_LifecycleOFFEN): n/a

#### `TEST_F(E2EIntegrationTest, E2E_02_AllSecurityLevels)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (E2EIntegrationTest): n/a
  - `<unnamed>` (E2E_02_AllSecurityLevels): n/a

#### `TEST_F(E2EIntegrationTest, E2E_03_KeyRotationConcurrentLoad)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (E2EIntegrationTest): n/a
  - `<unnamed>` (E2E_03_KeyRotationConcurrentLoad): n/a

#### `TEST_F(E2EIntegrationTest, E2E_04_MountFailureRecovery)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (E2EIntegrationTest): n/a
  - `<unnamed>` (E2E_04_MountFailureRecovery): n/a

#### `TEST_F(E2EIntegrationTest, E2E_05_InvalidContainerPath)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (E2EIntegrationTest): n/a
  - `<unnamed>` (E2E_05_InvalidContainerPath): n/a

#### `TEST_F(E2EIntegrationTest, E2E_06_VaultTimeoutRecovery)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (E2EIntegrationTest): n/a
  - `<unnamed>` (E2E_06_VaultTimeoutRecovery): n/a

#### `TEST_F(E2EIntegrationTest, E2E_07_StaleMountReconciliation)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (E2EIntegrationTest): n/a
  - `<unnamed>` (E2E_07_StaleMountReconciliation): n/a

#### `TEST_F(E2EIntegrationTest, E2E_08_MultiTenantIsolation)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`:313
- Brief: n/a
- Parameters:
  - `<unnamed>` (E2EIntegrationTest): n/a
  - `<unnamed>` (E2E_08_MultiTenantIsolation): n/a

#### `TEST_F(StressTest, STRESS_01_ConcurrentMountUnmount)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_stress_focused.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressTest): n/a
  - `<unnamed>` (STRESS_01_ConcurrentMountUnmount): n/a

#### `TEST_F(StressTest, STRESS_02_KeyRotationHighConcurrency)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_stress_focused.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressTest): n/a
  - `<unnamed>` (STRESS_02_KeyRotationHighConcurrency): n/a

#### `TEST_F(StressTest, STRESS_03_CommandTimeout)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_stress_focused.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressTest): n/a
  - `<unnamed>` (STRESS_03_CommandTimeout): n/a

#### `TEST_F(StressTest, STRESS_04_VaultUnavailable)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_stress_focused.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressTest): n/a
  - `<unnamed>` (STRESS_04_VaultUnavailable): n/a

#### `TEST_F(StressTest, STRESS_05_DiskFull)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_stress_focused.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressTest): n/a
  - `<unnamed>` (STRESS_05_DiskFull): n/a

#### `TEST_F(StressTest, STRESS_06_PermissionDenied)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_stress_focused.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressTest): n/a
  - `<unnamed>` (STRESS_06_PermissionDenied): n/a

### themis::user_storage_encrypted::test::E2EIntegrationTest

#### `std::string MakeTempContainer(const std::string &name)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`:85
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `std::string MakeTempMountPoint(const std::string &name)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`:91
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `void SetUp() override`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`:69
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`:75
- Brief: n/a
- Parameters: none

### themis::user_storage_encrypted::test::StressTest

#### `std::string MakeTempContainer(const std::string &name)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_stress_focused.cpp`:77
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `std::string MakeTempMountPoint(const std::string &name)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_stress_focused.cpp`:83
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `void RecordError(const std::string &error)`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_stress_focused.cpp`:89
- Brief: n/a
- Parameters:
  - `error` (const std::string &): n/a

#### `void SetUp() override`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_stress_focused.cpp`:60
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/user_storage_encrypted/test_user_storage_encrypted_stress_focused.cpp`:68
- Brief: n/a
- Parameters: none

