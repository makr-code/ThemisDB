/**
 * Test: CDC Schema Evolution Interfaces
 * Tests for ICDCEventSchema / InMemoryCDCEventSchema:
 *   - registerSchema returns true on first call, false on duplicate
 *   - getSchema returns the definition by version or latest (-1)
 *   - currentVersion returns -1 before any schema is registered
 *   - currentVersion returns the highest registered version
 *   - onSchemaEvolution registers a callback
 *   - triggerEvolution calls onCompatible when conflicts is empty
 *   - triggerEvolution calls onIncompatible with the first conflict
 *   - triggerEvolution returns false when no callback is registered
 *   - multiple collections are independent
 *   - SchemaEvolutionDescriptor fields are passed through correctly
 *   - SchemaConflict fields are accessible
 *   - ISchemaEvolutionCallback polymorphic usage
 *   - Thread-safety: concurrent registerSchema / getSchema
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "cdc/icdc_event_schema.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis::cdc;

// ─────────────────────────────────────────────────────────────────────────────
// Test callback implementation
// ─────────────────────────────────────────────────────────────────────────────

struct TestCallback : public ISchemaEvolutionCallback {
    std::atomic<int>        compatible_count{0};
    std::atomic<int>        incompatible_count{0};
    SchemaEvolutionDescriptor last_descriptor;
    SchemaConflict            last_conflict;
    mutable std::mutex        mutex;

    void onCompatible(const SchemaEvolutionDescriptor& d) override {
        std::unique_lock<std::mutex> lk(mutex);
        ++compatible_count;
        last_descriptor = d;
    }

    void onIncompatible(const SchemaEvolutionDescriptor& d,
                        const SchemaConflict& c) override {
        std::unique_lock<std::mutex> lk(mutex);
        ++incompatible_count;
        last_descriptor = d;
        last_conflict   = c;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// registerSchema / getSchema / currentVersion
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryCDCEventSchemaTest, RegisterAndRetrieve) {
    InMemoryCDCEventSchema schema;
    EXPECT_TRUE(schema.registerSchema("orders",
        R"({"type":"object"})", SchemaFormat::JSON, 1));

    EXPECT_EQ(schema.getSchema("orders", 1), R"({"type":"object"})");
    EXPECT_EQ(schema.currentVersion("orders"), 1);
}

TEST(InMemoryCDCEventSchemaTest, RegisterDuplicateVersionReturnsFalse) {
    InMemoryCDCEventSchema schema;
    EXPECT_TRUE(schema.registerSchema("orders",  R"({"v":1})", SchemaFormat::JSON, 1));
    EXPECT_FALSE(schema.registerSchema("orders", R"({"v":2})", SchemaFormat::JSON, 1));
    // Original schema is preserved
    EXPECT_EQ(schema.getSchema("orders", 1), R"({"v":1})");
}

TEST(InMemoryCDCEventSchemaTest, CurrentVersionBeforeRegistrationIsMinusOne) {
    InMemoryCDCEventSchema schema;
    EXPECT_EQ(schema.currentVersion("unknown"), -1);
}

TEST(InMemoryCDCEventSchemaTest, GetSchemaUnknownCollectionReturnsEmpty) {
    InMemoryCDCEventSchema schema;
    EXPECT_TRUE(schema.getSchema("unknown").empty());
}

TEST(InMemoryCDCEventSchemaTest, GetSchemaUnknownVersionReturnsEmpty) {
    InMemoryCDCEventSchema schema;
    schema.registerSchema("orders", R"({"v":1})", SchemaFormat::JSON, 1);
    EXPECT_TRUE(schema.getSchema("orders", 99).empty());
}

TEST(InMemoryCDCEventSchemaTest, GetLatestSchema) {
    InMemoryCDCEventSchema schema;
    schema.registerSchema("orders", R"({"v":1})", SchemaFormat::JSON, 1);
    schema.registerSchema("orders", R"({"v":2})", SchemaFormat::JSON, 2);

    // -1 returns the latest (version 2)
    EXPECT_EQ(schema.getSchema("orders", -1), R"({"v":2})");
    EXPECT_EQ(schema.currentVersion("orders"), 2);
}

TEST(InMemoryCDCEventSchemaTest, MultipleFormats) {
    InMemoryCDCEventSchema schema;
    const std::string avro_def = R"({"type":"record","name":"Order"})";
    EXPECT_TRUE(schema.registerSchema("orders", avro_def, SchemaFormat::AVRO, 1));
    EXPECT_EQ(schema.getSchema("orders", 1), avro_def);
}

TEST(InMemoryCDCEventSchemaTest, IndependentCollections) {
    InMemoryCDCEventSchema schema;
    schema.registerSchema("orders",    R"({"v":"orders-1"})",    SchemaFormat::JSON, 1);
    schema.registerSchema("inventory", R"({"v":"inventory-1"})", SchemaFormat::JSON, 1);

    EXPECT_EQ(schema.getSchema("orders",    1), R"({"v":"orders-1"})");
    EXPECT_EQ(schema.getSchema("inventory", 1), R"({"v":"inventory-1"})");
    EXPECT_EQ(schema.currentVersion("orders"),    1);
    EXPECT_EQ(schema.currentVersion("inventory"), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// triggerEvolution — compatible path
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryCDCEventSchemaTest, TriggerCompatibleEvolution) {
    InMemoryCDCEventSchema schema;
    auto cb = std::make_shared<TestCallback>();
    schema.onSchemaEvolution("orders", cb);

    SchemaEvolutionDescriptor desc;
    desc.collection  = "orders";
    desc.old_version = 1;
    desc.new_version = 2;
    desc.format      = SchemaFormat::JSON;
    // conflicts is empty → compatible

    EXPECT_TRUE(schema.triggerEvolution(desc));
    EXPECT_EQ(cb->compatible_count.load(), 1);
    EXPECT_EQ(cb->incompatible_count.load(), 0);
    EXPECT_EQ(cb->last_descriptor.collection, "orders");
    EXPECT_EQ(cb->last_descriptor.old_version, 1);
    EXPECT_EQ(cb->last_descriptor.new_version, 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// triggerEvolution — incompatible path
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryCDCEventSchemaTest, TriggerIncompatibleEvolution) {
    InMemoryCDCEventSchema schema;
    auto cb = std::make_shared<TestCallback>();
    schema.onSchemaEvolution("orders", cb);

    SchemaConflict conflict;
    conflict.field       = "price";
    conflict.old_type    = "float";
    conflict.new_type    = "string";
    conflict.description = "Type change float→string is incompatible";

    SchemaEvolutionDescriptor desc;
    desc.collection  = "orders";
    desc.old_version = 2;
    desc.new_version = 3;
    desc.conflicts.push_back(conflict);

    EXPECT_TRUE(schema.triggerEvolution(desc));
    EXPECT_EQ(cb->compatible_count.load(), 0);
    EXPECT_EQ(cb->incompatible_count.load(), 1);
    EXPECT_EQ(cb->last_conflict.field,    "price");
    EXPECT_EQ(cb->last_conflict.old_type, "float");
    EXPECT_EQ(cb->last_conflict.new_type, "string");
}

// ─────────────────────────────────────────────────────────────────────────────
// triggerEvolution — no callback registered
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryCDCEventSchemaTest, TriggerWithoutCallbackReturnsFalse) {
    InMemoryCDCEventSchema schema;

    SchemaEvolutionDescriptor desc;
    desc.collection = "orders";

    EXPECT_FALSE(schema.triggerEvolution(desc));
}

// ─────────────────────────────────────────────────────────────────────────────
// Callback replacement
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryCDCEventSchemaTest, ReplaceCallback) {
    InMemoryCDCEventSchema schema;
    auto cb1 = std::make_shared<TestCallback>();
    auto cb2 = std::make_shared<TestCallback>();

    schema.onSchemaEvolution("orders", cb1);
    schema.onSchemaEvolution("orders", cb2); // replaces cb1

    SchemaEvolutionDescriptor desc;
    desc.collection = "orders";

    schema.triggerEvolution(desc);
    EXPECT_EQ(cb1->compatible_count.load(), 0);
    EXPECT_EQ(cb2->compatible_count.load(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Multiple conflicts: first conflict passed to onIncompatible
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryCDCEventSchemaTest, FirstConflictPassedToCallback) {
    InMemoryCDCEventSchema schema;
    auto cb = std::make_shared<TestCallback>();
    schema.onSchemaEvolution("orders", cb);

    SchemaConflict c1; c1.field = "first";
    SchemaConflict c2; c2.field = "second";

    SchemaEvolutionDescriptor desc;
    desc.collection = "orders";
    desc.conflicts  = {c1, c2};

    schema.triggerEvolution(desc);
    EXPECT_EQ(cb->last_conflict.field, "first");
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread-safety: concurrent registerSchema / getSchema
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryCDCEventSchemaTest, ConcurrentRegisterAndGet) {
    InMemoryCDCEventSchema schema;
    constexpr int kVersions = 50;
    constexpr int kReaders  = 4;

    // Writer thread
    std::thread writer([&] {
        for (int v = 1; v <= kVersions; ++v) {
            schema.registerSchema("orders",
                R"({"v":)" + std::to_string(v) + "}",
                SchemaFormat::JSON, v);
        }
    });

    // Reader threads
    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int t = 0; t < kReaders; ++t) {
        readers.emplace_back([&] {
            for (int i = 0; i < 200; ++i) {
                (void)schema.getSchema("orders", -1);
                (void)schema.currentVersion("orders");
            }
        });
    }

    writer.join();
    for (auto& r : readers) r.join();

    // After all writes, the version should be kVersions
    EXPECT_EQ(schema.currentVersion("orders"), kVersions);
}

// ─────────────────────────────────────────────────────────────────────────────
// Polymorphic usage
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryCDCEventSchemaTest, PolymorphicUsage) {
    std::unique_ptr<ICDCEventSchema> schema =
        std::make_unique<InMemoryCDCEventSchema>();

    EXPECT_EQ(schema->currentVersion("col"), -1);
    EXPECT_TRUE(schema->registerSchema("col", R"({})", SchemaFormat::JSON, 1));
    EXPECT_EQ(schema->currentVersion("col"), 1);
    EXPECT_EQ(schema->getSchema("col", 1), R"({})");

    auto cb = std::make_shared<TestCallback>();
    schema->onSchemaEvolution("col", cb);

    SchemaEvolutionDescriptor desc;
    desc.collection = "col";
    EXPECT_TRUE(schema->triggerEvolution(desc));
    EXPECT_EQ(cb->compatible_count.load(), 1);
}
