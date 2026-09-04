/*
 * ThemisDB — TensorCoreBridge Tests
 *
 * Tests for:
 *   ITensorCoreBridge interface validation     TCS-01..TCS-04
 *   InMemoryTensorCoreBridge                   TCS-05..TCS-09
 *   TensorCoreStorageBridge                    TCS-10..TCS-13
 *   RocksDBTensorBackend                       TCB-RDB-01..06
 *   builtin.tensor_core_bridge step            TCS-14..TCS-20
 *
 * Acceptance criteria:
 *
 * ITensorCoreBridge validation (TCS-01..TCS-04)
 *   TCS-01  write() rejects empty tenant_id with ERR_DOC_INVALID_ARGUMENT
 *   TCS-02  write() rejects tenant_id containing '/'
 *   TCS-03  write() rejects empty chunk_id
 *   TCS-04  write() rejects empty serialized_train
 *
 * InMemoryTensorCoreBridge (TCS-05..TCS-09)
 *   TCS-05  write() succeeds for valid record
 *   TCS-06  writeCount() increments on each successful write
 *   TCS-07  find() returns nullptr when key is absent
 *   TCS-08  find() returns record after successful write
 *   TCS-09  Second write to same chunk_id overwrites (upsert semantics)
 *
 * TensorCoreStorageBridge (TCS-10..TCS-13)
 *   TCS-10  Constructed with nullptr defaults to InMemoryTensorBackend
 *   TCS-11  write() stores bytes retrievable via getRaw()
 *   TCS-12  makeKey() builds correct key schema
 *   TCS-13  write() increments writeCount() atomically
 *
 * RocksDBTensorBackend (TCB-RDB-01..06)  — stubs #148, #160 resolved
 *   TCB-RDB-01  null db pointer throws std::invalid_argument
 *   TCB-RDB-02  put()/get() round-trip on real RocksDB
 *   TCB-RDB-03  get() returns nullopt for absent key
 *   TCB-RDB-04  del() removes entry
 *   TCB-RDB-05  listKeys() filters by prefix, returns sorted result
 *   TCB-RDB-06  RocksDBTensorBackend injected into TensorCoreStorageBridge
 *
 * builtin.tensor_core_bridge step (TCS-14..TCS-20)
 *   TCS-14  Step with empty tensor_cores is a no-op
 *   TCS-15  Step writes all tensor_cores to sink
 *   TCS-16  Step resolves tenant from config key
 *   TCS-17  Step resolves tenant from record metadata when config key absent
 *   TCS-18  Step skips records with empty serialized_train by default
 *   TCS-19  Step processes records with empty serialized_train when skip_empty=false
 *   TCS-20  Step propagates write errors when fail_on_write_error=true
 *   TCS-21  Step resolves tenant from ctx.extra when config key absent
 *   TCS-22  Step emits warning when non-persistent in-memory sink is active
 *   TCS-23  Step aborts when require_persistent_sink=true with in-memory sink
 *   TCS-24  Step records warning for non-fatal write failure
 */

#include <gtest/gtest.h>

#include "ingestion/ingestion_sinks.h"
#include "ingestion/builtin_step_factories.h"
#include "ingestion/extraction_context.h"
#include "tensor/tensor_core_bridge.h"
#include "storage/tensor_network_storage_engine.h"
#include "storage/rocksdb_wrapper.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>
#include <memory>

using namespace themis;
using namespace themis::ingestion;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static TensorCoreRecord makeRecord(const std::string& chunk_id = "file123:0",
                                    const std::string& source_file_id = "file123") {
    TensorCoreRecord rec;
    rec.chunk_id       = chunk_id;
    rec.source_file_id = source_file_id;
    rec.order          = 2;
    rec.max_rank       = 4;
    rec.compression_ratio = 1.5;
    rec.achieved_eps   = 0.01;
    rec.serialized_train = {0x54, 0x54, 0x01, 0x00, 0xAB, 0xCD}; // synthetic bytes
    return rec;
}

static TensorCoreRecord makeRecordWithTenant(const std::string& tenant,
                                              const std::string& chunk_id = "file1:0") {
    auto rec = makeRecord(chunk_id);
    rec.metadata["tenant_id"] = tenant;
    return rec;
}

static ExtractionContext makeCtxWithCores(std::vector<TensorCoreRecord> cores) {
    ExtractionContext ctx;
    ctx.tensor_cores = std::move(cores);
    return ctx;
}

// ─────────────────────────────────────────────────────────────────────────────
// TCS-01..TCS-04 — ITensorCoreBridge validation (via InMemoryTensorCoreBridge)
// ─────────────────────────────────────────────────────────────────────────────

TEST(TCS, TCS_01_EmptyTenantRejected) {
    InMemoryTensorCoreBridge sink;
    auto rec = makeRecord();
    auto res = sink.write(rec, "");
    ASSERT_FALSE(res);
}

TEST(TCS, TCS_02_SlashInTenantRejected) {
    InMemoryTensorCoreBridge sink;
    auto rec = makeRecord();
    auto res = sink.write(rec, "org/tenant");
    ASSERT_FALSE(res);
}

TEST(TCS, TCS_03_EmptyChunkIdRejected) {
    InMemoryTensorCoreBridge sink;
    auto rec = makeRecord();
    rec.chunk_id = "";
    auto res = sink.write(rec, "mytenant");
    ASSERT_FALSE(res);
}

TEST(TCS, TCS_04_EmptySerializedTrainRejected) {
    InMemoryTensorCoreBridge sink;
    auto rec = makeRecord();
    rec.serialized_train.clear();
    auto res = sink.write(rec, "mytenant");
    ASSERT_FALSE(res);
}

// ─────────────────────────────────────────────────────────────────────────────
// TCS-05..TCS-09 — InMemoryTensorCoreBridge
// ─────────────────────────────────────────────────────────────────────────────

TEST(TCS, TCS_05_WriteSucceeds) {
    InMemoryTensorCoreBridge sink;
    auto rec = makeRecord();
    auto res = sink.write(rec, "acme");
    ASSERT_TRUE(res) << res.error().message();
}

TEST(TCS, TCS_06_WriteCountIncrements) {
    InMemoryTensorCoreBridge sink;
    EXPECT_EQ(sink.writeCount(), 0u);
    auto write_result_1 = sink.write(makeRecord("file1:0"), "acme");
    ASSERT_TRUE(write_result_1) << write_result_1.error().message();
    EXPECT_EQ(sink.writeCount(), 1u);
    auto write_result_2 = sink.write(makeRecord("file1:1"), "acme");
    ASSERT_TRUE(write_result_2) << write_result_2.error().message();
    EXPECT_EQ(sink.writeCount(), 2u);
}

TEST(TCS, TCS_07_FindAbsentReturnsNullptr) {
    InMemoryTensorCoreBridge sink;
    EXPECT_EQ(sink.find("acme", "no-such-chunk"), nullptr);
}

TEST(TCS, TCS_08_FindAfterWrite) {
    InMemoryTensorCoreBridge sink;
    auto rec = makeRecord("file1:3");
    auto write_result = sink.write(rec, "acme");
    ASSERT_TRUE(write_result) << write_result.error().message();
    auto* found = sink.find("acme", "file1:3");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->chunk_id, "file1:3");
    EXPECT_EQ(found->serialized_train, rec.serialized_train);
}

TEST(TCS, TCS_09_UpsertSemantics) {
    InMemoryTensorCoreBridge sink;
    auto rec1 = makeRecord("file1:0");
    rec1.max_rank = 4;

    auto rec2 = makeRecord("file1:0");
    rec2.max_rank = 8;
    rec2.serialized_train = {0xFF, 0xEE};

    auto write_result_1 = sink.write(rec1, "acme");
    ASSERT_TRUE(write_result_1) << write_result_1.error().message();
    auto write_result_2 = sink.write(rec2, "acme");
    ASSERT_TRUE(write_result_2) << write_result_2.error().message();

    EXPECT_EQ(sink.writeCount(), 2u);
    auto* found = sink.find("acme", "file1:0");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->max_rank, 8u);
    EXPECT_EQ(found->serialized_train, rec2.serialized_train);
}

// ─────────────────────────────────────────────────────────────────────────────
// TCS-10..TCS-13 — TensorCoreStorageBridge
// ─────────────────────────────────────────────────────────────────────────────

TEST(TCS, TCS_10_NullBackendDefaultsToInMemory) {
    // Should not throw; defaults to InMemoryTensorBackend.
    EXPECT_NO_THROW({
        tensor::TensorCoreStorageBridge sink(nullptr);
    });
}

TEST(TCS, TCS_11_WriteThenGetRaw) {
    auto backend = std::make_shared<storage::InMemoryTensorBackend>();
    tensor::TensorCoreStorageBridge sink(backend);

    auto rec = makeRecord("file1:0", "sha256abcdef");
    auto res = sink.write(rec, "mytenant");
    ASSERT_TRUE(res) << res.error().message();

    auto raw = sink.getRaw("mytenant", "sha256abcdef", "file1:0");
    ASSERT_TRUE(raw.has_value());
    EXPECT_EQ(*raw, rec.serialized_train);
}

TEST(TCS, TCS_12_MakeKeySchema) {
    std::string key = tensor::TensorCoreStorageBridge::makeKey(
        "acme", "sha256file", "sha256file:2");
    EXPECT_EQ(key, "__ttcore__:acme:sha256file:sha256file:2");
}

TEST(TCS, TCS_13_WriteCountAtomic) {
    tensor::TensorCoreStorageBridge sink;
    EXPECT_EQ(sink.writeCount(), 0u);
    auto write_result_1 = sink.write(makeRecord("f:0"), "t1");
    ASSERT_TRUE(write_result_1) << write_result_1.error().message();
    auto write_result_2 = sink.write(makeRecord("f:1"), "t1");
    ASSERT_TRUE(write_result_2) << write_result_2.error().message();
    auto write_result_3 = sink.write(makeRecord("f:2"), "t1");
    ASSERT_TRUE(write_result_3) << write_result_3.error().message();
    EXPECT_EQ(sink.writeCount(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// TCB-RDB-01..05 — RocksDBTensorBackend (STUB #148, #160 resolved)
// Tests use a temporary RocksDB instance to verify durable put/get/del/listKeys.
// ─────────────────────────────────────────────────────────────────────────────

static std::string makeTempRdbPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_tcs_rdb_" + tag + "_" + std::to_string(ns))).string();
}

// Helper: open a temporary RocksDB and return wrapper (nullptr on failure).
static std::shared_ptr<themis::RocksDBWrapper>
openTempRdb(const std::string& path) {
    themis::RocksDBWrapper::Config cfg;
    cfg.db_path   = path;
    cfg.enable_wal = true;
    auto db = std::make_shared<themis::RocksDBWrapper>(cfg);
    if (!db->open()) {
      return nullptr;
    }
    return db;
}

static void closeAndCleanupTempRdb(const std::shared_ptr<themis::RocksDBWrapper>& db,
                                   const std::string& path) {
    if (db) {
        db->close();
    }
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
}

TEST(TCS, TCB_RDB_01_NullDbThrows) {
    EXPECT_THROW(
        { storage::RocksDBTensorBackend b(nullptr); },
        std::invalid_argument);
}

TEST(TCS, TCB_RDB_02_PutGetRoundTrip) {
    auto path = makeTempRdbPath("02");
    auto db   = openTempRdb(path);
    if (!db) {
      GTEST_SKIP() << "RocksDB unavailable";
    }

    storage::RocksDBTensorBackend backend(db);
    std::vector<uint8_t> val{1, 2, 3, 4, 5};

    EXPECT_TRUE(backend.put("__ttn__:t1:c1:f1:meta:1", val));
    auto got = backend.get("__ttn__:t1:c1:f1:meta:1");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(*got, val);

    closeAndCleanupTempRdb(db, path);
}

TEST(TCS, TCB_RDB_03_GetMissingReturnsNullopt) {
    auto path = makeTempRdbPath("03");
    auto db   = openTempRdb(path);
    if (!db) {
      GTEST_SKIP() << "RocksDB unavailable";
    }

    storage::RocksDBTensorBackend backend(db);
    auto got = backend.get("__ttn__:absent:key");
    EXPECT_FALSE(got.has_value());

    closeAndCleanupTempRdb(db, path);
}

TEST(TCS, TCB_RDB_04_DelRemovesEntry) {
    auto path = makeTempRdbPath("04");
    auto db   = openTempRdb(path);
    if (!db) {
      GTEST_SKIP() << "RocksDB unavailable";
    }

    storage::RocksDBTensorBackend backend(db);
    std::vector<uint8_t> val{0xAB};
    backend.put("__ttn__:t:c:f:meta:1", val);

    EXPECT_TRUE(backend.del("__ttn__:t:c:f:meta:1"));
    EXPECT_FALSE(backend.get("__ttn__:t:c:f:meta:1").has_value());

    closeAndCleanupTempRdb(db, path);
}

TEST(TCS, TCB_RDB_05_ListKeysByPrefix) {
    auto path = makeTempRdbPath("05");
    auto db   = openTempRdb(path);
    if (!db) {
      GTEST_SKIP() << "RocksDB unavailable";
    }

    storage::RocksDBTensorBackend backend(db);
    std::vector<uint8_t> dummy{0};
    backend.put("__ttn__:tenant1:col:field:meta:1", dummy);
    backend.put("__ttn__:tenant1:col:field:G0:1",   dummy);
    backend.put("__ttn__:tenant1:col:field:G1:1",   dummy);
    backend.put("__ttn__:OTHER:col:field:meta:1",   dummy);

    auto keys = backend.listKeys("__ttn__:tenant1:");
    ASSERT_EQ(keys.size(), 3u);
    EXPECT_TRUE(std::is_sorted(keys.begin(), keys.end()));
    for (const auto& k : keys)
        EXPECT_TRUE(k.rfind("__ttn__:tenant1:", 0) == 0);

    closeAndCleanupTempRdb(db, path);
}

// Also verify RocksDBTensorBackend can be injected into TensorCoreStorageBridge.
TEST(TCS, TCB_RDB_06_IntegratedWithTensorCoreStorageBridge) {
    auto path = makeTempRdbPath("06");
    auto db   = openTempRdb(path);
    if (!db) {
      GTEST_SKIP() << "RocksDB unavailable";
    }

    auto backend = std::make_shared<storage::RocksDBTensorBackend>(db);
    tensor::TensorCoreStorageBridge sink(backend);

    auto rec = makeRecord("chunk:42", "file_sha");
    auto res = sink.write(rec, "acme");
    ASSERT_TRUE(res) << res.error().message();

    auto raw = sink.getRaw("acme", "file_sha", "chunk:42");
    ASSERT_TRUE(raw.has_value());
    EXPECT_EQ(*raw, rec.serialized_train);
    EXPECT_EQ(sink.writeCount(), 1u);

    closeAndCleanupTempRdb(db, path);
}

// ─────────────────────────────────────────────────────────────────────────────
// TCS-14..TCS-20 — builtin.tensor_core_bridge step
// ─────────────────────────────────────────────────────────────────────────────

TEST(TCS, TCS_14_EmptyCoresNoOp) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    StepConfig sc;

    ExtractionContext ctx;
    auto res = step->execute(ctx, sc);
    ASSERT_TRUE(res);
    EXPECT_EQ(sink->writeCount(), 0u);
}

TEST(TCS, TCS_15_WritesAllCores) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    StepConfig sc;
    sc.config = json{{"tenant_id", "testorg"}};

    auto ctx = makeCtxWithCores({makeRecord("f:0"), makeRecord("f:1"), makeRecord("f:2")});
    auto res = step->execute(ctx, sc);
    ASSERT_TRUE(res) << res.error().message();
    EXPECT_EQ(sink->writeCount(), 3u);
}

TEST(TCS, TCS_16_TenantFromConfigKey) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    StepConfig sc;
    sc.config = json{{"tenant_id", "config-tenant"}};

    auto ctx = makeCtxWithCores({makeRecord("f:0")});
    auto execute_result = step->execute(ctx, sc);
    ASSERT_TRUE(execute_result) << execute_result.error().message();

    auto* found = sink->find("config-tenant", "f:0");
    EXPECT_NE(found, nullptr);
}

TEST(TCS, TCS_17_TenantFromRecordMetadata) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    StepConfig sc;
    // No tenant_id in config → falls back to record metadata.

    auto ctx = makeCtxWithCores({makeRecordWithTenant("meta-tenant", "f:0")});
    auto execute_result = step->execute(ctx, sc);
    ASSERT_TRUE(execute_result) << execute_result.error().message();

    // "default" is the global tenant_id; when it equals "default" the step
    // reads per-record metadata["tenant_id"].
    auto* found = sink->find("meta-tenant", "f:0");
    EXPECT_NE(found, nullptr);
}

TEST(TCS, TCS_18_SkipEmptySerializedByDefault) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    StepConfig sc;
    sc.config = json{{"tenant_id", "t1"}};

    TensorCoreRecord empty_rec = makeRecord("f:0");
    empty_rec.serialized_train.clear();

    auto ctx = makeCtxWithCores({empty_rec});
    auto res = step->execute(ctx, sc);
    ASSERT_TRUE(res); // no error — just skipped
    EXPECT_EQ(sink->writeCount(), 0u);
}

TEST(TCS, TCS_19_ProcessEmptySerializedWhenSkipFalse) {
    // With skip_empty=false the record reaches sink->write() and fails validation.
    // fail_on_write_error=true means the step itself returns an error.
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    StepConfig sc;
    sc.config = json{
        {"tenant_id",           "t1"},
        {"skip_empty",          false},
        {"fail_on_write_error", true}
    };

    TensorCoreRecord empty_rec = makeRecord("f:0");
    empty_rec.serialized_train.clear();

    auto ctx = makeCtxWithCores({empty_rec});
    auto res = step->execute(ctx, sc);
    EXPECT_FALSE(res); // write error propagated
}

TEST(TCS, TCS_20_FailOnWriteError) {
    // A sink that always rejects any tenant (overrides write to fail).
    class RejectAllSink : public ITensorCoreBridge {
    public:
        Result<void> write(const TensorCoreRecord&, const std::string&) override {
            return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                           "forced failure");
        }
        std::size_t writeCount() const override { return 0; }
    };

    auto sink = std::make_shared<RejectAllSink>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    StepConfig sc;
    sc.config = json{
        {"tenant_id",           "t1"},
        {"fail_on_write_error", true}
    };

    auto ctx = makeCtxWithCores({makeRecord("f:0")});
    auto res = step->execute(ctx, sc);
    EXPECT_FALSE(res); // error propagated
}

TEST(TCS, TCS_21_TenantFromContextExtra) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    StepConfig sc;

    auto ctx = makeCtxWithCores({makeRecord("f:0")});
    ctx.extra["tenant_id"] = "ctx-extra-tenant";

    auto res = step->execute(ctx, sc);
    ASSERT_TRUE(res) << res.error().message();

    auto* found = sink->find("ctx-extra-tenant", "f:0");
    EXPECT_NE(found, nullptr);
}

TEST(TCS, TCS_22_WarnsWhenInMemorySinkActive) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    StepConfig sc;
    sc.config = json{{"tenant_id", "t1"}};

    auto ctx = makeCtxWithCores({makeRecord("f:0")});
    auto res = step->execute(ctx, sc);
    ASSERT_TRUE(res) << res.error().message();

    bool saw_warning = false;
    for (const auto& warning : ctx.warnings) {
        if (warning.find("InMemoryTensorCoreBridge active") != std::string::npos) {
            saw_warning = true;
            break;
        }
    }
    EXPECT_TRUE(saw_warning);
}

TEST(TCS, TCS_23_RequirePersistentSinkRejectsInMemory) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    StepConfig sc;
    sc.config = json{{"require_persistent_sink", true}};

    auto ctx = makeCtxWithCores({makeRecord("f:0")});
    auto res = step->execute(ctx, sc);
    EXPECT_FALSE(res);
}

TEST(TCS, TCS_24_NonFatalWriteErrorAddsWarning) {
    class RejectAllSink : public ITensorCoreBridge {
    public:
        Result<void> write(const TensorCoreRecord&, const std::string&) override {
            return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                           "forced failure");
        }
        std::size_t writeCount() const override { return 0; }
    };

    auto sink = std::make_shared<RejectAllSink>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    StepConfig sc;
    sc.config = json{{"tenant_id", "t1"}, {"fail_on_write_error", false}};

    auto ctx = makeCtxWithCores({makeRecord("f:0")});
    auto res = step->execute(ctx, sc);
    ASSERT_TRUE(res);

    bool saw_failure_warning = false;
    for (const auto& warning : ctx.warnings) {
        if (warning.find("write failed for chunk_id='f:0'") != std::string::npos) {
            saw_failure_warning = true;
            break;
        }
    }
    EXPECT_TRUE(saw_failure_warning);
}


// ============================================================================
// TensorCoreStorageBridge — default backend factory bridge (STUB #269)
// TCB-FB-01..TCB-FB-03
// ============================================================================

// TCB-FB-01: Without a factory, constructor uses InMemoryTensorBackend.
TEST(TCS, TCB_FB_01_null_factory_uses_in_memory_backend) {
    tensor::TensorCoreStorageBridge::clearDefaultBackendFactory();
    tensor::TensorCoreStorageBridge bridge; // no arg → should use InMemoryTensorBackend
    ingestion::TensorCoreRecord rec;
    rec.chunk_id = "c1";
    rec.source_file_id = "f1";
    rec.serialized_train.assign(4, 0x01);
    auto r = bridge.write(rec, "tenant");
    EXPECT_TRUE(r);
    EXPECT_EQ(bridge.writeCount(), 1u);
}

// TCB-FB-02: Injected factory provides a custom backend used by the bridge.
TEST(TCS, TCB_FB_02_injected_factory_backend_is_used) {
    auto custom = std::make_shared<storage::InMemoryTensorBackend>();
    tensor::TensorCoreStorageBridge::setDefaultBackendFactory(
        [custom]() -> std::shared_ptr<storage::ITensorStorageBackend> {
            return custom;
        });

    tensor::TensorCoreStorageBridge bridge;
    ingestion::TensorCoreRecord rec;
    rec.chunk_id = "ck2";
    rec.source_file_id = "f2";
    rec.serialized_train.assign(8, 0xAB);
    auto r = bridge.write(rec, "T2");
    EXPECT_TRUE(r);

    // The custom backend should now hold the data.
    auto raw = custom->get("__ttcore__:T2:f2:ck2");
    EXPECT_TRUE(raw.has_value());

    tensor::TensorCoreStorageBridge::clearDefaultBackendFactory();
}

// TCB-FB-03: When the factory returns nullptr, bridge falls back to InMemoryTensorBackend.
//             This tests the case where a factory is registered but produces no backend.
TEST(TCS, TCB_FB_03_clear_factory_reverts_to_in_memory) {
    // Set a factory that returns nullptr (simulating a misconfigured bootstrap).
    // The bridge must then fall back to InMemoryTensorBackend automatically.
    tensor::TensorCoreStorageBridge::setDefaultBackendFactory(
        []() -> std::shared_ptr<storage::ITensorStorageBackend> {
            return nullptr;
        });

    tensor::TensorCoreStorageBridge bridge;
    ingestion::TensorCoreRecord rec;
    rec.chunk_id = "ck3";
    rec.source_file_id = "f3";
    rec.serialized_train.assign(4, 0xFF);
    auto r = bridge.write(rec, "T3");
    EXPECT_TRUE(r);
    EXPECT_EQ(bridge.writeCount(), 1u);

    tensor::TensorCoreStorageBridge::clearDefaultBackendFactory();
}
