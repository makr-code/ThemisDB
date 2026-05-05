/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_tensor_core_bridge.cpp                          ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * ThemisDB — TensorCoreBridge Tests
 *
 * Tests for:
 *   ITensorCoreBridge interface validation     TCS-01..TCS-04
 *   InMemoryTensorCoreBridge                   TCS-05..TCS-09
 *   TensorCoreStorageBridge                    TCS-10..TCS-13
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
 * builtin.tensor_core_bridge step (TCS-14..TCS-20)
 *   TCS-14  Step with empty tensor_cores is a no-op
 *   TCS-15  Step writes all tensor_cores to sink
 *   TCS-16  Step resolves tenant from config key
 *   TCS-17  Step resolves tenant from record metadata when config key absent
 *   TCS-18  Step skips records with empty serialized_train by default
 *   TCS-19  Step processes records with empty serialized_train when skip_empty=false
 *   TCS-20  Step propagates write errors when fail_on_write_error=true
 */

#include <gtest/gtest.h>

#include "ingestion/ingestion_sinks.h"
#include "ingestion/builtin_step_factories.h"
#include "ingestion/extraction_context.h"
#include "tensor/tensor_core_bridge.h"
#include <nlohmann/json.hpp>
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
    sink.write(makeRecord("file1:0"), "acme");
    EXPECT_EQ(sink.writeCount(), 1u);
    sink.write(makeRecord("file1:1"), "acme");
    EXPECT_EQ(sink.writeCount(), 2u);
}

TEST(TCS, TCS_07_FindAbsentReturnsNullptr) {
    InMemoryTensorCoreBridge sink;
    EXPECT_EQ(sink.find("acme", "no-such-chunk"), nullptr);
}

TEST(TCS, TCS_08_FindAfterWrite) {
    InMemoryTensorCoreBridge sink;
    auto rec = makeRecord("file1:3");
    sink.write(rec, "acme");
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

    sink.write(rec1, "acme");
    sink.write(rec2, "acme");

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
    sink.write(makeRecord("f:0"), "t1");
    sink.write(makeRecord("f:1"), "t1");
    sink.write(makeRecord("f:2"), "t1");
    EXPECT_EQ(sink.writeCount(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// TCS-14..TCS-20 — builtin.tensor_core_bridge step
// ─────────────────────────────────────────────────────────────────────────────

TEST(TCS, TCS_14_EmptyCoresNoOp) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);

    ExtractionContext ctx;
    auto res = step->execute(ctx);
    ASSERT_TRUE(res);
    EXPECT_EQ(sink->writeCount(), 0u);
}

TEST(TCS, TCS_15_WritesAllCores) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    step->configure(json{{"tenant_id", "testorg"}});

    auto ctx = makeCtxWithCores({makeRecord("f:0"), makeRecord("f:1"), makeRecord("f:2")});
    auto res = step->execute(ctx);
    ASSERT_TRUE(res) << res.error().message();
    EXPECT_EQ(sink->writeCount(), 3u);
}

TEST(TCS, TCS_16_TenantFromConfigKey) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    step->configure(json{{"tenant_id", "config-tenant"}});

    auto ctx = makeCtxWithCores({makeRecord("f:0")});
    step->execute(ctx);

    auto* found = sink->find("config-tenant", "f:0");
    EXPECT_NE(found, nullptr);
}

TEST(TCS, TCS_17_TenantFromRecordMetadata) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    // No tenant_id in config → falls back to record metadata.

    auto ctx = makeCtxWithCores({makeRecordWithTenant("meta-tenant", "f:0")});
    step->execute(ctx);

    // "default" is the global tenant_id; when it equals "default" the step
    // reads per-record metadata["tenant_id"].
    auto* found = sink->find("meta-tenant", "f:0");
    EXPECT_NE(found, nullptr);
}

TEST(TCS, TCS_18_SkipEmptySerializedByDefault) {
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    step->configure(json{{"tenant_id", "t1"}});

    TensorCoreRecord empty_rec = makeRecord("f:0");
    empty_rec.serialized_train.clear();

    auto ctx = makeCtxWithCores({empty_rec});
    auto res = step->execute(ctx);
    ASSERT_TRUE(res); // no error — just skipped
    EXPECT_EQ(sink->writeCount(), 0u);
}

TEST(TCS, TCS_19_ProcessEmptySerializedWhenSkipFalse) {
    // With skip_empty=false the record reaches sink->write() and fails validation.
    // fail_on_write_error=true means the step itself returns an error.
    auto sink = std::make_shared<InMemoryTensorCoreBridge>();
    auto step = builtin::createTensorCoreBridgeStep(sink);
    step->configure(json{
        {"tenant_id",           "t1"},
        {"skip_empty",          false},
        {"fail_on_write_error", true}
    });

    TensorCoreRecord empty_rec = makeRecord("f:0");
    empty_rec.serialized_train.clear();

    auto ctx = makeCtxWithCores({empty_rec});
    auto res = step->execute(ctx);
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
    step->configure(json{
        {"tenant_id",           "t1"},
        {"fail_on_write_error", true}
    });

    auto ctx = makeCtxWithCores({makeRecord("f:0")});
    auto res = step->execute(ctx);
    EXPECT_FALSE(res); // error propagated
}
