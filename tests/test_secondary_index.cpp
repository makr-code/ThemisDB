#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include "storage/key_schema.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "index/secondary_index_metadata_cache.h"

using namespace themis;

// ----------------- KeySchema unit tests (existing) -----------------

TEST(KeySchemaTest, MakeRelationalKey) {
    std::string key = KeySchema::makeRelationalKey("users", "123");
    EXPECT_EQ(key, "rel:users:123"); // v1.5.0+ format with prefix
}

TEST(KeySchemaTest, MakeGraphNodeKey) {
    std::string key = KeySchema::makeGraphNodeKey("user/alice");
    EXPECT_EQ(key, "node:user/alice");
}

TEST(KeySchemaTest, MakeGraphEdgeKey) {
    std::string key = KeySchema::makeGraphEdgeKey("edge_1");
    EXPECT_EQ(key, "edge:edge_1");
}

TEST(KeySchemaTest, MakeSecondaryIndexKey) {
    std::string key = KeySchema::makeSecondaryIndexKey("users", "age", "30", "user_123");
    EXPECT_EQ(key, "idx:users:age:30:user_123");
}

TEST(KeySchemaTest, MakeGraphOutdexKey) {
    std::string key = KeySchema::makeGraphOutdexKey("user/alice", "edge_1");
    EXPECT_EQ(key, "graph:out:user/alice:edge_1");
}

TEST(KeySchemaTest, MakeGraphIndexKey) {
    std::string key = KeySchema::makeGraphIndexKey("company/acme", "edge_1");
    EXPECT_EQ(key, "graph:in:company/acme:edge_1");
}

TEST(KeySchemaTest, ExtractPrimaryKey) {
    // v1.5.0+ format with prefixes
    std::string pk = KeySchema::extractPrimaryKey("rel:users:123");
    EXPECT_EQ(pk, "123");
    
    pk = KeySchema::extractPrimaryKey("idx:users:age:30:user_456");
    EXPECT_EQ(pk, "user_456");
}

TEST(KeySchemaTest, ParseKeyType) {
    // v1.5.0+ format tests
    EXPECT_EQ(KeySchema::parseKeyType("rel:users:123"), KeySchema::KeyType::RELATIONAL);
    EXPECT_EQ(KeySchema::parseKeyType("doc:orders:456"), KeySchema::KeyType::DOCUMENT);
    EXPECT_EQ(KeySchema::parseKeyType("idx:users:age:30:pk"), KeySchema::KeyType::SECONDARY_INDEX);
    EXPECT_EQ(KeySchema::parseKeyType("graph:out:alice:e1"), KeySchema::KeyType::GRAPH_OUTDEX);
    EXPECT_EQ(KeySchema::parseKeyType("graph:in:bob:e1"), KeySchema::KeyType::GRAPH_INDEX);
    EXPECT_EQ(KeySchema::parseKeyType("node:alice"), KeySchema::KeyType::GRAPH_NODE);
    EXPECT_EQ(KeySchema::parseKeyType("edge:e1"), KeySchema::KeyType::GRAPH_EDGE);
    
    // Legacy format (pre-1.5.0) - defaults to DOCUMENT for backward compatibility
    EXPECT_EQ(KeySchema::parseKeyType("users:123"), KeySchema::KeyType::DOCUMENT);
}

// ----------------- SecondaryIndex integration tests -----------------

static std::string makeTempDbPath(const std::string& name) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto base = fs::temp_directory_path() / (name + std::to_string(now));
    return base.string();
}

TEST(SecondaryIndexTest, CreatePutScanDelete) {
    // Arrange: RocksDB wrapper
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_secidx_create_put_");
    cfg.enable_blobdb = false; // not needed in tests
    RocksDBWrapper db(cfg);
    
    std::cout << "\n=== TEST DEBUG: DB OPEN ===" << std::endl;
    std::cout << "DB path: " << cfg.db_path << std::endl;
    
    bool opened = db.open();
    ASSERT_TRUE(opened) << "Failed to open RocksDB";
    ASSERT_TRUE(db.isOpen()) << "DB not open after open() returned true";
    
    std::cout << "DB opened successfully, attempting put()..." << std::endl;

    // Test direct put to isolate the problem
    std::vector<uint8_t> test_val{0x01, 0x02};
    bool direct_put_result = db.put("test_key", test_val);
    
    std::cout << "Direct put() result: " << (direct_put_result ? "SUCCESS" : "FAILED") << std::endl;
    
    ASSERT_TRUE(direct_put_result) << "Direct put() to RocksDB failed";
    
    SecondaryIndexManager idx(db);
    auto st = idx.createIndex("users", "age");
    ASSERT_TRUE(st.ok) << "createIndex failed: " << st.message << " (db.isOpen=" << db.isOpen() << ")";

    // Insert entity
    BaseEntity::FieldMap fields1{{"name","Alice"}, {"age", int64_t(30)}, {"city","Berlin"}};
    BaseEntity e1 = BaseEntity::fromFields("u1", fields1);
    st = idx.put("users", e1);
    ASSERT_TRUE(st.ok) << st.message;

    // Scan equals age=30 -> expect u1
    auto [status1, keys] = idx.scanKeysEqual("users", "age", "30");
    ASSERT_TRUE(status1.ok);
    ASSERT_EQ(keys.size(), 1);
    EXPECT_EQ(keys[0], "u1");

    // Update: change age to 31
    BaseEntity::FieldMap fields2{{"name","Alice"}, {"age", int64_t(31)}, {"city","Berlin"}};
    BaseEntity e2 = BaseEntity::fromFields("u1", fields2);
    st = idx.put("users", e2);
    ASSERT_TRUE(st.ok) << st.message;

    // Old index should be gone
    auto [status2a, keys_old] = idx.scanKeysEqual("users", "age", "30");
    ASSERT_TRUE(status2a.ok);
    EXPECT_TRUE(keys_old.empty());
    // New index
    auto [status2b, keys_new] = idx.scanKeysEqual("users", "age", "31");
    ASSERT_TRUE(status2b.ok);
    ASSERT_EQ(keys_new.size(), 1);
    EXPECT_EQ(keys_new[0], "u1");

    // Delete entity
    st = idx.erase("users", "u1");
    ASSERT_TRUE(st.ok) << st.message;
    auto [status3, keys_post] = idx.scanKeysEqual("users", "age", "31");
    ASSERT_TRUE(status3.ok);
    EXPECT_TRUE(keys_post.empty());

    db.close();
}

TEST(SecondaryIndexTest, EstimateCountAndNoIndex) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_secidx_estimate_");
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    // No index yet -> scans should error, estimate = 0
    auto [status0, keys0] = idx.scanKeysEqual("users", "age", "30");
    EXPECT_FALSE(status0.ok);
    bool capped=false; 
    EXPECT_EQ(idx.estimateCountEqual("users","age","30", 10, &capped), 0u);
    EXPECT_FALSE(capped);

    // Create index and insert 3 entries with same age
    ASSERT_TRUE(idx.createIndex("users","age").ok);
    for (int i=0; i<3; ++i) {
        BaseEntity::FieldMap f{{"name","N"+std::to_string(i)}, {"age", int64_t(30)}};
        BaseEntity e = BaseEntity::fromFields("u"+std::to_string(i), f);
        ASSERT_TRUE(idx.put("users", e).ok);
    }

    capped = false;
    auto c = idx.estimateCountEqual("users","age","30", 2, &capped);
    EXPECT_EQ(c, 2u);
    EXPECT_TRUE(capped);

    auto [status1, keys] = idx.scanKeysEqual("users","age","30");
    ASSERT_TRUE(status1.ok);
    EXPECT_EQ(keys.size(), 3u);

    db.close();
}

TEST(SecondaryIndexTest, PutBatch_DefaultAndConfigurableBatchSize) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_secidx_put_batch_cfg_");
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());

    SecondaryIndexManager idx(db);
    EXPECT_EQ(idx.getTransactionalPutBatchSize(), 64u);

    idx.setTransactionalPutBatchSize(32);
    EXPECT_EQ(idx.getTransactionalPutBatchSize(), 32u);

    idx.setTransactionalPutBatchSize(0);
    EXPECT_EQ(idx.getTransactionalPutBatchSize(), 1u);

    db.close();
}

TEST(SecondaryIndexTest, PutBatch_ChunkedTransactionsMaintainIndexCorrectness) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_secidx_put_batch_chunk_");
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());

    SecondaryIndexManager idx(db);
    ASSERT_TRUE(idx.createIndex("users", "group").ok);

    std::vector<BaseEntity> entities;
    entities.reserve(130);
    for (int i = 0; i < 130; ++i) {
        entities.emplace_back(
            "u" + std::to_string(i),
            BaseEntity::FieldMap{
                {"group", std::string("g") + std::to_string(i % 2)},
                {"name", std::string("User") + std::to_string(i)}
            }
        );
    }

    auto st = idx.putBatch("users", entities, 64);
    ASSERT_TRUE(st.ok) << st.message;

    auto [scan_status_g0, keys_g0] = idx.scanKeysEqual("users", "group", "g0");
    auto [scan_status_g1, keys_g1] = idx.scanKeysEqual("users", "group", "g1");
    ASSERT_TRUE(scan_status_g0.ok) << scan_status_g0.message;
    ASSERT_TRUE(scan_status_g1.ok) << scan_status_g1.message;
    EXPECT_EQ(keys_g0.size(), 65u);
    EXPECT_EQ(keys_g1.size(), 65u);

    db.close();
}

// ─────────────────────────────────────────────────────────────────────────────
// Partial (filtered) index tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(SecondaryIndexTest, PartialIndex_CreateHasDrop) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_partidx_lifecycle_");
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    // Initially no partial index
    EXPECT_FALSE(idx.hasPartialIndex("orders", "customer_id"));
    EXPECT_FALSE(idx.getPartialIndexPredicate("orders", "customer_id").has_value());

    // Create partial index
    auto st = idx.createPartialIndex("orders", "customer_id", "status = 'active'");
    ASSERT_TRUE(st.ok) << st.message;
    EXPECT_TRUE(idx.hasPartialIndex("orders", "customer_id"));

    // Predicate should be retrievable
    auto pred = idx.getPartialIndexPredicate("orders", "customer_id");
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(*pred, "status = 'active'");

    // Drop partial index
    st = idx.dropPartialIndex("orders", "customer_id");
    ASSERT_TRUE(st.ok) << st.message;
    EXPECT_FALSE(idx.hasPartialIndex("orders", "customer_id"));

    db.close();
}

TEST(SecondaryIndexTest, PartialIndex_FilteringOnPut) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_partidx_filter_");
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    // Create partial index on "email" for active users only
    auto st = idx.createPartialIndex("users", "email", "status = 'active'");
    ASSERT_TRUE(st.ok) << st.message;

    // Insert 3 users: 2 active, 1 inactive
    BaseEntity::FieldMap f1{{"email", std::string("alice@example.com")}, {"status", std::string("active")}};
    BaseEntity::FieldMap f2{{"email", std::string("bob@example.com")},   {"status", std::string("active")}};
    BaseEntity::FieldMap f3{{"email", std::string("carol@example.com")}, {"status", std::string("inactive")}};

    ASSERT_TRUE(idx.put("users", BaseEntity::fromFields("u1", f1)).ok);
    ASSERT_TRUE(idx.put("users", BaseEntity::fromFields("u2", f2)).ok);
    ASSERT_TRUE(idx.put("users", BaseEntity::fromFields("u3", f3)).ok);

    // scanKeysEqualPartial should return only the 2 active users for their emails
    auto [st1, keys1] = idx.scanKeysEqualPartial("users", "email", "alice@example.com");
    ASSERT_TRUE(st1.ok) << st1.message;
    ASSERT_EQ(keys1.size(), 1u);
    EXPECT_EQ(keys1[0], "u1");

    auto [st2, keys2] = idx.scanKeysEqualPartial("users", "email", "bob@example.com");
    ASSERT_TRUE(st2.ok) << st2.message;
    ASSERT_EQ(keys2.size(), 1u);
    EXPECT_EQ(keys2[0], "u2");

    // Inactive user should NOT be in the partial index
    auto [st3, keys3] = idx.scanKeysEqualPartial("users", "email", "carol@example.com");
    ASSERT_TRUE(st3.ok) << st3.message;
    EXPECT_TRUE(keys3.empty());

    db.close();
}

TEST(SecondaryIndexTest, PartialIndex_UpdateRemovesOldEntry) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_partidx_update_");
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    // Partial index on "score" for active entities
    ASSERT_TRUE(idx.createPartialIndex("items", "score", "active = '1'").ok);

    // Insert active entity
    BaseEntity::FieldMap f1{{"score", std::string("100")}, {"active", std::string("1")}};
    ASSERT_TRUE(idx.put("items", BaseEntity::fromFields("item1", f1)).ok);

    // Verify it's indexed
    auto [st1, keys1] = idx.scanKeysEqualPartial("items", "score", "100");
    ASSERT_TRUE(st1.ok);
    ASSERT_EQ(keys1.size(), 1u);

    // Deactivate the entity (predicate no longer matches)
    BaseEntity::FieldMap f2{{"score", std::string("100")}, {"active", std::string("0")}};
    ASSERT_TRUE(idx.put("items", BaseEntity::fromFields("item1", f2)).ok);

    // Should no longer be in the partial index
    auto [st2, keys2] = idx.scanKeysEqualPartial("items", "score", "100");
    ASSERT_TRUE(st2.ok);
    EXPECT_TRUE(keys2.empty());

    db.close();
}

TEST(SecondaryIndexTest, PartialIndex_NumericPredicate) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_partidx_numeric_");
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    // Partial index on "product_id" for high-value orders (amount > 500)
    ASSERT_TRUE(idx.createPartialIndex("orders", "product_id", "amount > 500").ok);

    BaseEntity::FieldMap f1{{"product_id", std::string("p1")}, {"amount", std::string("1000")}};
    BaseEntity::FieldMap f2{{"product_id", std::string("p1")}, {"amount", std::string("200")}};
    BaseEntity::FieldMap f3{{"product_id", std::string("p2")}, {"amount", std::string("750")}};

    ASSERT_TRUE(idx.put("orders", BaseEntity::fromFields("o1", f1)).ok); // high value
    ASSERT_TRUE(idx.put("orders", BaseEntity::fromFields("o2", f2)).ok); // low value
    ASSERT_TRUE(idx.put("orders", BaseEntity::fromFields("o3", f3)).ok); // high value

    // High-value orders for product p1
    auto [st1, keys1] = idx.scanKeysEqualPartial("orders", "product_id", "p1");
    ASSERT_TRUE(st1.ok);
    ASSERT_EQ(keys1.size(), 1u);
    EXPECT_EQ(keys1[0], "o1");

    // High-value orders for product p2
    auto [st2, keys2] = idx.scanKeysEqualPartial("orders", "product_id", "p2");
    ASSERT_TRUE(st2.ok);
    ASSERT_EQ(keys2.size(), 1u);
    EXPECT_EQ(keys2[0], "o3");

    // Low-value order o2 should not be indexed
    auto [st3, keys3] = idx.scanKeysEqualPartial("orders", "product_id", "p1");
    ASSERT_TRUE(st3.ok);
    // o2 has product_id p1 but amount 200, so not indexed - only o1
    EXPECT_EQ(keys3.size(), 1u);

    db.close();
}

TEST(SecondaryIndexTest, PartialIndex_IsNotNullPredicate) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_partidx_isnull_");
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    // Partial index on "category" for entities that have a non-null "tag"
    ASSERT_TRUE(idx.createPartialIndex("products", "category", "tag IS NOT NULL").ok);

    // Entity with a tag
    BaseEntity::FieldMap f1{{"category", std::string("electronics")}, {"tag", std::string("sale")}};
    // Entity without a tag
    BaseEntity::FieldMap f2{{"category", std::string("electronics")}};

    ASSERT_TRUE(idx.put("products", BaseEntity::fromFields("prod1", f1)).ok);
    ASSERT_TRUE(idx.put("products", BaseEntity::fromFields("prod2", f2)).ok);

    // Only prod1 should appear in partial index (has tag)
    auto [st, keys] = idx.scanKeysEqualPartial("products", "category", "electronics");
    ASSERT_TRUE(st.ok);
    ASSERT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "prod1");

    db.close();
}

TEST(SecondaryIndexTest, PartialIndex_ErrorOnNoIndex) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_partidx_noindex_");
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    // Scan on non-existent partial index should return error
    auto [st, keys] = idx.scanKeysEqualPartial("users", "email", "foo@bar.com");
    EXPECT_FALSE(st.ok);
    EXPECT_TRUE(keys.empty());

    db.close();
}

TEST(SecondaryIndexTest, PartialIndex_DeleteRemovesEntry) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_partidx_delete_");
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    ASSERT_TRUE(idx.createPartialIndex("sessions", "user_id", "valid = '1'").ok);

    BaseEntity::FieldMap f{{"user_id", std::string("u42")}, {"valid", std::string("1")}};
    ASSERT_TRUE(idx.put("sessions", BaseEntity::fromFields("s1", f)).ok);

    // Verify indexed
    auto [st1, keys1] = idx.scanKeysEqualPartial("sessions", "user_id", "u42");
    ASSERT_TRUE(st1.ok);
    ASSERT_EQ(keys1.size(), 1u);

    // Erase entity
    ASSERT_TRUE(idx.erase("sessions", "s1").ok);

    // Should be gone from partial index
    auto [st2, keys2] = idx.scanKeysEqualPartial("sessions", "user_id", "u42");
    ASSERT_TRUE(st2.ok);
    EXPECT_TRUE(keys2.empty());

    db.close();
}

// ─── KeySchema hot-path optimization: correctness parity ─────────────────────

TEST(KeySchemaOptTest, RelationalKey_Parity) {
    // The optimized append-path must produce identical output to the old format
    EXPECT_EQ(KeySchema::makeRelationalKey("users", "u123"), "rel:users:u123");
    EXPECT_EQ(KeySchema::makeRelationalKey("", "pk"), "rel::pk");
    EXPECT_EQ(KeySchema::makeRelationalKey("t", ""), "rel:t:");
}

TEST(KeySchemaOptTest, DocumentKey_Parity) {
    EXPECT_EQ(KeySchema::makeDocumentKey("orders", "o456"), "doc:orders:o456");
}

TEST(KeySchemaOptTest, GraphNodeKey_Parity) {
    EXPECT_EQ(KeySchema::makeGraphNodeKey("alice"), "node:alice");
}

TEST(KeySchemaOptTest, GraphEdgeKey_Parity) {
    EXPECT_EQ(KeySchema::makeGraphEdgeKey("e1"), "edge:e1");
}

TEST(KeySchemaOptTest, VectorKey_Parity) {
    EXPECT_EQ(KeySchema::makeVectorKey("embeddings", "v7"), "vec:embeddings:v7");
}

TEST(KeySchemaOptTest, SecondaryIndexKey_Parity) {
    EXPECT_EQ(KeySchema::makeSecondaryIndexKey("users", "age", "30", "u1"),
              "idx:users:age:30:u1");
}

TEST(KeySchemaOptTest, GraphOutdexKey_Parity) {
    EXPECT_EQ(KeySchema::makeGraphOutdexKey("alice", "e1"), "graph:out:alice:e1");
}

TEST(KeySchemaOptTest, GraphIndexKey_Parity) {
    EXPECT_EQ(KeySchema::makeGraphIndexKey("bob", "e2"), "graph:in:bob:e2");
}

// ─── SecondaryIndexMetadataCache: unique-flag caching ─────────────────────────

TEST(MetadataCacheUniqueFlagTest, RegularUniqueFlag_SetAndGet) {
    SecondaryIndexMetadataCache::IndexMetadata m;
    m.regular_indexes = {"email", "name"};
    m.regular_unique["email"] = true;
    m.regular_unique["name"]  = false;

    SecondaryIndexMetadataCache& cache = SecondaryIndexMetadataCache::instance();
    cache.invalidate("tbl_unique_reg");
    cache.set("tbl_unique_reg", m);

    auto opt = cache.get("tbl_unique_reg");
    ASSERT_TRUE(opt.has_value());
    EXPECT_TRUE(opt->regular_unique.at("email"));
    EXPECT_FALSE(opt->regular_unique.at("name"));
}

TEST(MetadataCacheUniqueFlagTest, SparseUniqueFlag_SetAndGet) {
    SecondaryIndexMetadataCache::IndexMetadata m;
    m.sparse_indexes = {"phone"};
    m.sparse_unique["phone"] = true;

    SecondaryIndexMetadataCache& cache = SecondaryIndexMetadataCache::instance();
    cache.invalidate("tbl_unique_sparse");
    cache.set("tbl_unique_sparse", m);

    auto opt = cache.get("tbl_unique_sparse");
    ASSERT_TRUE(opt.has_value());
    EXPECT_TRUE(opt->sparse_unique.at("phone"));
}

TEST(MetadataCacheUniqueFlagTest, PartialUniqueFlag_SetAndGet) {
    SecondaryIndexMetadataCache::IndexMetadata m;
    m.partial_indexes = {"coupon"};
    m.partial_predicates["coupon"] = "active = '1'";
    m.partial_unique["coupon"] = false;

    SecondaryIndexMetadataCache& cache = SecondaryIndexMetadataCache::instance();
    cache.invalidate("tbl_unique_partial");
    cache.set("tbl_unique_partial", m);

    auto opt = cache.get("tbl_unique_partial");
    ASSERT_TRUE(opt.has_value());
    EXPECT_FALSE(opt->partial_unique.at("coupon"));
    EXPECT_EQ(opt->partial_predicates.at("coupon"), "active = '1'");
}

TEST(MetadataCacheUniqueFlagTest, Invalidate_ClearsUniqueMaps) {
    SecondaryIndexMetadataCache::IndexMetadata m;
    m.regular_unique["x"] = true;

    SecondaryIndexMetadataCache& cache = SecondaryIndexMetadataCache::instance();
    cache.set("tbl_inv", m);
    ASSERT_TRUE(cache.get("tbl_inv").has_value());

    cache.invalidate("tbl_inv");
    EXPECT_FALSE(cache.get("tbl_inv").has_value());
}

TEST(MetadataCacheUniqueFlagTest, UniqueIndex_EnforcedViaCache) {
    // Integration: creating a unique index and putting a duplicate key should fail.
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_unique_cache_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    // Create unique index on "email"; this populates the metadata cache on the
    // first put() call, so ensure it is warm by invalidating stale entries first.
    ASSERT_TRUE(idx.createIndex("accounts", "email", /*unique=*/true).ok);
    // Confirm no stale entry exists before the first write populates the cache
    SecondaryIndexMetadataCache::instance().invalidate("accounts");
    EXPECT_FALSE(SecondaryIndexMetadataCache::instance().get("accounts").has_value())
        << "Cache should be empty before the first put()";

    BaseEntity::FieldMap f1{{"email", std::string("a@b.com")}};
    BaseEntity::FieldMap f2{{"email", std::string("a@b.com")}};  // duplicate

    ASSERT_TRUE(idx.put("accounts", BaseEntity::fromFields("acc1", f1)).ok);
    // Second put with same email must be rejected (unique constraint)
    auto st2 = idx.put("accounts", BaseEntity::fromFields("acc2", f2));
    EXPECT_FALSE(st2.ok) << "Duplicate unique-index value should be rejected";

    db.close();
}

// ────────────────────────────────────────────────────────────────────────────
// ConcurrentUniqueLücke fix tests
//
// These tests verify that the Concurrent-Unique-Lücke fix (using
// TransactionWrapper::getForUpdate for unique-index sentinel locking) correctly
// enforces unique constraints in the transactional (MVCC) put path.
// ────────────────────────────────────────────────────────────────────────────

// CU-1: Transaction overload enforces single-column unique constraint
//       (sequential: commit, then attempt duplicate in a new transaction).
TEST(ConcurrentUniqueLückeTest, TxnUnique_SingleColumn_SequentialReject) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_cu1_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    ASSERT_TRUE(idx.createIndex("users", "email", /*unique=*/true).ok);
    SecondaryIndexMetadataCache::instance().invalidate("users");

    // First transaction: insert email=x@y.com under pk=u1
    {
        auto txn = db.beginTransaction();
        ASSERT_NE(txn, nullptr);
        BaseEntity::FieldMap f1{{"email", std::string("x@y.com")}};
        auto st = idx.put("users", BaseEntity::fromFields("u1", f1), *txn);
        ASSERT_TRUE(st.ok) << "First txn put should succeed: " << st.message;
        ASSERT_TRUE(txn->commit()) << "First txn commit should succeed";
    }

    // Second transaction: attempt to insert same email under pk=u2 — must be rejected
    {
        auto txn = db.beginTransaction();
        ASSERT_NE(txn, nullptr);
        BaseEntity::FieldMap f2{{"email", std::string("x@y.com")}};
        auto st = idx.put("users", BaseEntity::fromFields("u2", f2), *txn);
        EXPECT_FALSE(st.ok) << "Duplicate unique-index value must be rejected in txn path";
        txn->rollback();
    }

    db.close();
}

// CU-2: Transaction overload enforces composite unique constraint
//       (sequential: commit, then attempt duplicate in a new transaction).
TEST(ConcurrentUniqueLückeTest, TxnUnique_Composite_SequentialReject) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_cu2_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    ASSERT_TRUE(idx.createCompositeIndex("orders", {"tenant", "order_no"}, /*unique=*/true).ok);
    SecondaryIndexMetadataCache::instance().invalidate("orders");

    // First transaction: insert (tenant=acme, order_no=42) under pk=o1
    {
        auto txn = db.beginTransaction();
        ASSERT_NE(txn, nullptr);
        BaseEntity::FieldMap f1{{"tenant", std::string("acme")}, {"order_no", std::string("42")}};
        auto st = idx.put("orders", BaseEntity::fromFields("o1", f1), *txn);
        ASSERT_TRUE(st.ok) << "First composite txn put should succeed: " << st.message;
        ASSERT_TRUE(txn->commit());
    }

    // Second transaction: attempt same (tenant, order_no) under pk=o2 — must be rejected
    {
        auto txn = db.beginTransaction();
        ASSERT_NE(txn, nullptr);
        BaseEntity::FieldMap f2{{"tenant", std::string("acme")}, {"order_no", std::string("42")}};
        auto st = idx.put("orders", BaseEntity::fromFields("o2", f2), *txn);
        EXPECT_FALSE(st.ok) << "Duplicate composite unique value must be rejected in txn path";
        txn->rollback();
    }

    db.close();
}

// CU-3: Concurrent transactions — at most one transaction may commit with a
//       given unique value.  The sentinel getForUpdate lock ensures that two
//       transactions racing to insert the same unique value result in exactly
//       one success and at least one failure (conflict or unique violation).
TEST(ConcurrentUniqueLückeTest, TxnUnique_Concurrent_OnlyOneCommits) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_cu3_");
    cfg.enable_blobdb = false;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    ASSERT_TRUE(idx.createIndex("accounts", "ssn", /*unique=*/true).ok);
    SecondaryIndexMetadataCache::instance().invalidate("accounts");

    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};

    // Barrier so both threads start as simultaneously as possible
    std::atomic<bool> go{false};

    auto worker = [&](const std::string& pk) {
        while (!go.load(std::memory_order_acquire)) { /* spin */ }

        auto txn = db.beginTransaction();
        if (!txn) { failure_count.fetch_add(1, std::memory_order_relaxed); return; }

        BaseEntity::FieldMap fields{{"ssn", std::string("123-45-6789")}};
        auto st = idx.put("accounts", BaseEntity::fromFields(pk, fields), *txn);
        if (!st.ok) {
            txn->rollback();
            failure_count.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        if (txn->commit()) {
            success_count.fetch_add(1, std::memory_order_relaxed);
        } else {
            failure_count.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::thread t1(worker, "acc_a");
    std::thread t2(worker, "acc_b");

    go.store(true, std::memory_order_release);
    t1.join();
    t2.join();

    // Exactly one transaction must have committed; the unique index must be
    // clean (no two different PKs holding the same SSN value).
    EXPECT_EQ(success_count.load(), 1)
        << "Exactly one concurrent transaction should commit the unique value";
    EXPECT_GE(failure_count.load(), 1)
        << "At least one concurrent transaction must be rejected";

    // Verify the committed state: only one row with ssn=123-45-6789 exists
    auto [st, results] = idx.scanKeysEqual("accounts", "ssn", "123-45-6789");
    EXPECT_EQ(results.size(), 1u)
        << "Unique index must contain exactly one entry after concurrent inserts";
    if (results.size() == 1u) {
        EXPECT_TRUE(results[0] == "acc_a" || results[0] == "acc_b")
            << "Committed PK must be one of the two worker PKs, got: " << results[0];
    }

    db.close();
}
