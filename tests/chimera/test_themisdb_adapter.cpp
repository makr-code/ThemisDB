/**
 * @file test_themisdb_adapter.cpp
 * @brief Unit tests and performance overhead benchmarks for ThemisDBAdapter
 *
 * @details
 * Tests cover:
 *   - Connection management (connect, disconnect, invalid URI)
 *   - Relational operations (execute_query, insert_row, batch_insert)
 *   - Vector operations (insert_vector, batch_insert_vectors, search_vectors,
 *     create_index)
 *   - Graph operations (insert_node, insert_edge, shortest_path, traverse,
 *     execute_graph_query)
 *   - Document operations (insert_document, batch_insert_documents,
 *     find_documents, update_documents)
 *   - Transaction lifecycle (begin, commit, rollback)
 *   - Capability reporting and system information
 *   - Performance overhead benchmarks (adapter call overhead measurement)
 *
 * All tests run without a live ThemisDB server – the adapter operates in
 * its in-process simulation mode.
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"
#include "chimera/themisdb_adapter.hpp"

#include <chrono>
#include <algorithm>
#include <string>
#include <vector>

using namespace chimera;

namespace {
inline Capability connection_pooling_capability() {
#if defined(CONNECTION_POOLING)
    return Capability::CONNECTION_POOLING;
#else
    // Fallback token for older CHIMERA enum surfaces.
    return Capability::REPLICATION;
#endif
}
} // namespace

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static Document make_doc(const std::string& id,
                          const std::string& name,
                          int64_t value) {
    Document doc;
    doc.id = id;
    doc.fields["name"]  = Scalar{name};
    doc.fields["value"] = Scalar{value};
    return doc;
}

static Vector make_vector(size_t dim, float fill) {
    Vector v;
    v.data.assign(dim, fill);
    return v;
}

// ---------------------------------------------------------------------------
// Connection tests
// ---------------------------------------------------------------------------

class ThemisDBConnectionTest : public ::testing::Test {
protected:
    ThemisDBAdapter adapter;
};

TEST_F(ThemisDBConnectionTest, InitiallyDisconnected) {
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(ThemisDBConnectionTest, ConnectWithValidUri) {
    auto result = adapter.connect("themisdb://localhost:7777");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(ThemisDBConnectionTest, ConnectWithOptionsSucceeds) {
    auto result = adapter.connect("themisdb://localhost:7777",
                                  {{"timeout", "5000"}, {"pool_size", "10"}});
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(ThemisDBConnectionTest, DisconnectAfterConnect) {
    adapter.connect("themisdb://localhost:7777");
    auto result = adapter.disconnect();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(ThemisDBConnectionTest, DisconnectWhenAlreadyDisconnected) {
    auto result = adapter.disconnect();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(adapter.is_connected());
}

// ---------------------------------------------------------------------------
// Relational operations
// ---------------------------------------------------------------------------

class ThemisDBRelationalTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;
};

TEST_F(ThemisDBRelationalTest, ExecuteQueryReturnsOk) {
    auto result = adapter.execute_query("SELECT * FROM users");
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBRelationalTest, ExecuteQueryWithParams) {
    auto result = adapter.execute_query(
        "SELECT * FROM users WHERE id = ?",
        {Scalar{int64_t(42)}}
    );
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBRelationalTest, ExecuteQueryWhenDisconnectedReturnsError) {
    adapter.disconnect();
    auto result = adapter.execute_query("SELECT 1");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(ThemisDBRelationalTest, InsertRowReturnsOk) {
    RelationalRow row;
    row.columns["name"]  = Scalar{std::string("Alice")};
    row.columns["score"] = Scalar{int64_t(100)};

    auto result = adapter.insert_row("users", row);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 1u);
}

TEST_F(ThemisDBRelationalTest, BatchInsertReturnsCorrectCount) {
    std::vector<RelationalRow> rows(5);
    for (size_t i = 0; i < rows.size(); ++i) {
        rows[i].columns["idx"] = Scalar{int64_t(i)};
    }

    auto result = adapter.batch_insert("users", rows);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), rows.size());
}

TEST_F(ThemisDBRelationalTest, GetQueryStatisticsReturnsOk) {
    auto result = adapter.get_query_statistics();
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// Vector operations
// ---------------------------------------------------------------------------

class ThemisDBVectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;
    static constexpr const char* kCollection = "embeddings";
    static constexpr size_t kDim = 16;
};

TEST_F(ThemisDBVectorTest, CreateIndexReturnsOk) {
    auto result = adapter.create_index(kCollection, kDim);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBVectorTest, InsertVectorReturnsId) {
    auto v = make_vector(kDim, 0.5f);
    auto result = adapter.insert_vector(kCollection, v);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(ThemisDBVectorTest, BatchInsertVectorsReturnsCorrectCount) {
    std::vector<Vector> vecs = {};

    for (size_t i = 0; i < 10; ++i) {
        vecs.push_back(make_vector(kDim, static_cast<float>(i) / 10.0f));
    }

    auto result = adapter.batch_insert_vectors(kCollection, vecs);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), vecs.size());
}

TEST_F(ThemisDBVectorTest, SearchVectorsReturnsOk) {
    auto query = make_vector(kDim, 0.5f);
    auto result = adapter.search_vectors(kCollection, query, 5);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBVectorTest, SearchVectorsWhenDisconnectedReturnsError) {
    adapter.disconnect();
    auto query = make_vector(kDim, 0.5f);
    auto result = adapter.search_vectors(kCollection, query, 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// Graph operations
// ---------------------------------------------------------------------------

class ThemisDBGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;
};

TEST_F(ThemisDBGraphTest, InsertNodeReturnsId) {
    GraphNode node;
    node.id    = "node_1";
    node.label = "Person";
    node.properties["name"] = Scalar{std::string("Alice")};

    auto result = adapter.insert_node(node);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), "node_1");
}

TEST_F(ThemisDBGraphTest, InsertNodeWithEmptyIdGeneratesId) {
    GraphNode node;
    node.label = "Entity";

    auto result = adapter.insert_node(node);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(ThemisDBGraphTest, InsertEdgeReturnsId) {
    GraphEdge edge;
    edge.id        = "edge_1";
    edge.source_id = "node_1";
    edge.target_id = "node_2";
    edge.label     = "KNOWS";

    auto result = adapter.insert_edge(edge);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), "edge_1");
}

TEST_F(ThemisDBGraphTest, ShortestPathReturnsOk) {
    auto result = adapter.shortest_path("node_1", "node_2");
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBGraphTest, TraverseReturnsOk) {
    auto result = adapter.traverse("node_1", 3);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBGraphTest, ExecuteGraphQueryReturnsOk) {
    auto result = adapter.execute_graph_query(
        "FOR v IN 1..2 OUTBOUND 'node_1' GRAPH 'g' RETURN v"
    );
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// Document operations
// ---------------------------------------------------------------------------

class ThemisDBDocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;
    static constexpr const char* kCollection = "items";
};

TEST_F(ThemisDBDocumentTest, InsertDocumentReturnsId) {
    auto doc = make_doc("doc_1", "Item A", 10);
    auto result = adapter.insert_document(kCollection, doc);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), "doc_1");
}

TEST_F(ThemisDBDocumentTest, InsertDocumentWithEmptyIdGeneratesId) {
    Document doc;
    doc.fields["x"] = Scalar{int64_t(1)};

    auto result = adapter.insert_document(kCollection, doc);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(ThemisDBDocumentTest, BatchInsertDocumentsReturnsCorrectCount) {
    std::vector<Document> docs = {};

    for (size_t i = 0; i < 8; ++i) {
        docs.push_back(make_doc("d_" + std::to_string(i), "item", int64_t(i)));
    }

    auto result = adapter.batch_insert_documents(kCollection, docs);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), docs.size());
}

TEST_F(ThemisDBDocumentTest, FindDocumentsReturnsOk) {
    auto result = adapter.find_documents(kCollection, {{"name", Scalar{std::string("Item A")}}});
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBDocumentTest, UpdateDocumentsReturnsOk) {
    auto result = adapter.update_documents(
        kCollection,
        {{"name", Scalar{std::string("Item A")}}},
        {{"value", Scalar{int64_t(99)}}}
    );
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBDocumentTest, BatchInsertWhenDisconnectedReturnsError) {
    adapter.disconnect();
    std::vector<Document> docs = {make_doc("x", "y", 0)};
    auto result = adapter.batch_insert_documents(kCollection, docs);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// Transaction lifecycle
// ---------------------------------------------------------------------------

class ThemisDBTransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;
};

TEST_F(ThemisDBTransactionTest, BeginTransactionReturnsTxnId) {
    auto result = adapter.begin_transaction();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(ThemisDBTransactionTest, CommitTransactionReturnsOk) {
    auto begin = adapter.begin_transaction();
    ASSERT_TRUE(begin.is_ok());

    auto commit = adapter.commit_transaction(begin.value.value());
    EXPECT_TRUE(commit.is_ok());
}

TEST_F(ThemisDBTransactionTest, RollbackTransactionReturnsOk) {
    auto begin = adapter.begin_transaction();
    ASSERT_TRUE(begin.is_ok());

    auto rollback = adapter.rollback_transaction(begin.value.value());
    EXPECT_TRUE(rollback.is_ok());
}

TEST_F(ThemisDBTransactionTest, BeginTransactionWhenDisconnectedReturnsError) {
    adapter.disconnect();
    auto result = adapter.begin_transaction();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(ThemisDBTransactionTest, MultipleTransactionsGetUniqueIds) {
    auto txn1 = adapter.begin_transaction();
    auto txn2 = adapter.begin_transaction();
    ASSERT_TRUE(txn1.is_ok());
    ASSERT_TRUE(txn2.is_ok());
    EXPECT_NE(txn1.value.value(), txn2.value.value());
    adapter.rollback_transaction(txn1.value.value());
    adapter.rollback_transaction(txn2.value.value());
}

TEST_F(ThemisDBTransactionTest, CommitUnknownTransactionReturnsNotFound) {
    auto result = adapter.commit_transaction("nonexistent_txn");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(ThemisDBTransactionTest, RollbackUnknownTransactionReturnsNotFound) {
    auto result = adapter.rollback_transaction("nonexistent_txn");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(ThemisDBTransactionTest, CommitEmptyIdReturnsInvalidArgument) {
    auto result = adapter.commit_transaction("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

// ---------------------------------------------------------------------------
// Savepoints
// ---------------------------------------------------------------------------

TEST_F(ThemisDBTransactionTest, CreateSavepointSucceeds) {
    auto txn = adapter.begin_transaction();
    ASSERT_TRUE(txn.is_ok());
    const std::string txn_id = txn.value.value();

    auto sp = adapter.create_savepoint(txn_id, "sp1");
    EXPECT_TRUE(sp.is_ok());
    EXPECT_EQ(sp.value.value(), "sp1");

    adapter.rollback_transaction(txn_id);
}

TEST_F(ThemisDBTransactionTest, CreateDuplicateSavepointReturnsAlreadyExists) {
    auto txn = adapter.begin_transaction();
    ASSERT_TRUE(txn.is_ok());
    const std::string txn_id = txn.value.value();

    ASSERT_TRUE(adapter.create_savepoint(txn_id, "sp1").is_ok());
    auto sp2 = adapter.create_savepoint(txn_id, "sp1");
    EXPECT_TRUE(sp2.is_err());
    EXPECT_EQ(sp2.error_code, ErrorCode::ALREADY_EXISTS);

    adapter.rollback_transaction(txn_id);
}

TEST_F(ThemisDBTransactionTest, RollbackToSavepointSucceeds) {
    auto txn = adapter.begin_transaction();
    ASSERT_TRUE(txn.is_ok());
    const std::string txn_id = txn.value.value();

    ASSERT_TRUE(adapter.create_savepoint(txn_id, "sp1").is_ok());
    ASSERT_TRUE(adapter.create_savepoint(txn_id, "sp2").is_ok());

    auto rollback = adapter.rollback_to_savepoint(txn_id, "sp1");
    EXPECT_TRUE(rollback.is_ok());

    // After rolling back to sp1, only sp1 should remain
    auto state = adapter.get_transaction_state(txn_id);
    ASSERT_TRUE(state.is_ok());
    EXPECT_EQ(state.value.value().savepoints.size(), 1u);
    EXPECT_EQ(state.value.value().savepoints[0], "sp1");

    adapter.rollback_transaction(txn_id);
}

TEST_F(ThemisDBTransactionTest, RollbackToNonExistentSavepointReturnsNotFound) {
    auto txn = adapter.begin_transaction();
    ASSERT_TRUE(txn.is_ok());
    const std::string txn_id = txn.value.value();

    auto result = adapter.rollback_to_savepoint(txn_id, "nonexistent");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);

    adapter.rollback_transaction(txn_id);
}

TEST_F(ThemisDBTransactionTest, ReleaseSavepointSucceeds) {
    auto txn = adapter.begin_transaction();
    ASSERT_TRUE(txn.is_ok());
    const std::string txn_id = txn.value.value();

    ASSERT_TRUE(adapter.create_savepoint(txn_id, "sp1").is_ok());
    auto release = adapter.release_savepoint(txn_id, "sp1");
    EXPECT_TRUE(release.is_ok());

    // sp1 should no longer exist
    auto state = adapter.get_transaction_state(txn_id);
    ASSERT_TRUE(state.is_ok());
    EXPECT_TRUE(state.value.value().savepoints.empty());

    adapter.rollback_transaction(txn_id);
}

TEST_F(ThemisDBTransactionTest, ReleaseNonExistentSavepointReturnsNotFound) {
    auto txn = adapter.begin_transaction();
    ASSERT_TRUE(txn.is_ok());
    const std::string txn_id = txn.value.value();

    auto result = adapter.release_savepoint(txn_id, "sp_missing");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);

    adapter.rollback_transaction(txn_id);
}

TEST_F(ThemisDBTransactionTest, SavepointOperationsOnClosedTransactionReturnNotFound) {
    auto txn = adapter.begin_transaction();
    ASSERT_TRUE(txn.is_ok());
    const std::string txn_id = txn.value.value();
    adapter.commit_transaction(txn_id);

    EXPECT_EQ(adapter.create_savepoint(txn_id, "sp").error_code, ErrorCode::NOT_FOUND);
    EXPECT_EQ(adapter.rollback_to_savepoint(txn_id, "sp").error_code, ErrorCode::NOT_FOUND);
    EXPECT_EQ(adapter.release_savepoint(txn_id, "sp").error_code, ErrorCode::NOT_FOUND);
}

// ---------------------------------------------------------------------------
// Transaction statistics and state
// ---------------------------------------------------------------------------

TEST_F(ThemisDBTransactionTest, GetTransactionStatsReturnsValidData) {
    TransactionOptions opts;
    opts.isolation_level = TransactionOptions::IsolationLevel::SERIALIZABLE;
    opts.read_only = true;

    auto txn = adapter.begin_transaction(opts);
    ASSERT_TRUE(txn.is_ok());
    const std::string txn_id = txn.value.value();

    ASSERT_TRUE(adapter.create_savepoint(txn_id, "sp1").is_ok());
    ASSERT_TRUE(adapter.create_savepoint(txn_id, "sp2").is_ok());

    auto stats = adapter.get_transaction_stats(txn_id);
    ASSERT_TRUE(stats.is_ok());
    const auto& s = stats.value.value();

    EXPECT_EQ(s.transaction_id, txn_id);
    EXPECT_EQ(s.savepoint_count, 2u);
    EXPECT_TRUE(s.is_read_only);
    EXPECT_EQ(s.isolation_level, TransactionOptions::IsolationLevel::SERIALIZABLE);

    adapter.rollback_transaction(txn_id);
}

TEST_F(ThemisDBTransactionTest, GetTransactionStatsForUnknownIdReturnsNotFound) {
    auto result = adapter.get_transaction_stats("no_such_txn");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(ThemisDBTransactionTest, GetTransactionStateReturnsCorrectIsolationLevel) {
    TransactionOptions opts;
    opts.isolation_level = TransactionOptions::IsolationLevel::REPEATABLE_READ;

    auto txn = adapter.begin_transaction(opts);
    ASSERT_TRUE(txn.is_ok());
    const std::string txn_id = txn.value.value();

    auto state = adapter.get_transaction_state(txn_id);
    ASSERT_TRUE(state.is_ok());
    EXPECT_EQ(state.value.value().isolation_level,
              TransactionOptions::IsolationLevel::REPEATABLE_READ);
    EXPECT_EQ(state.value.value().transaction_id, txn_id);
    EXPECT_TRUE(state.value.value().elapsed_time.has_value());

    adapter.rollback_transaction(txn_id);
}

TEST_F(ThemisDBTransactionTest, GetTransactionStateForUnknownIdReturnsNotFound) {
    auto result = adapter.get_transaction_state("no_such_txn");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(ThemisDBTransactionTest, GetTransactionStateListsSavepoints) {
    auto txn = adapter.begin_transaction();
    ASSERT_TRUE(txn.is_ok());
    const std::string txn_id = txn.value.value();

    ASSERT_TRUE(adapter.create_savepoint(txn_id, "alpha").is_ok());
    ASSERT_TRUE(adapter.create_savepoint(txn_id, "beta").is_ok());

    auto state = adapter.get_transaction_state(txn_id);
    ASSERT_TRUE(state.is_ok());
    EXPECT_EQ(state.value.value().savepoints.size(), 2u);
    EXPECT_EQ(state.value.value().savepoints[0], "alpha");
    EXPECT_EQ(state.value.value().savepoints[1], "beta");

    adapter.rollback_transaction(txn_id);
}

// ---------------------------------------------------------------------------
// Deadlock retry (execute_with_retry)
// ---------------------------------------------------------------------------

TEST_F(ThemisDBTransactionTest, ExecuteWithRetrySucceedsOnFirstAttempt) {
    int call_count = 0;
    auto result = adapter.execute_with_retry<bool>(
        [&]() -> Result<bool> {
            ++call_count;
            return Result<bool>::ok(true);
        },
        3
    );
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(call_count, 1);
}

TEST_F(ThemisDBTransactionTest, ExecuteWithRetryRetriesOnDeadlock) {
    int call_count = 0;
    constexpr int kFailTimes = 2;
    auto result = adapter.execute_with_retry<bool>(
        [&]() -> Result<bool> {
            ++call_count;
            if (call_count <= kFailTimes) {
                return Result<bool>::err(ErrorCode::DEADLOCK, "simulated deadlock");
            }
            return Result<bool>::ok(true);
        },
        5,
        std::chrono::milliseconds{0}  // zero backoff for fast test
    );
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(call_count, kFailTimes + 1);
}

TEST_F(ThemisDBTransactionTest, ExecuteWithRetryExhaustsMaxRetries) {
    int call_count = 0;
    constexpr size_t kMaxRetries = 3;
    auto result = adapter.execute_with_retry<bool>(
        [&]() -> Result<bool> {
            ++call_count;
            return Result<bool>::err(ErrorCode::DEADLOCK, "always deadlocked");
        },
        kMaxRetries,
        std::chrono::milliseconds{0}
    );
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::DEADLOCK);
    EXPECT_EQ(static_cast<size_t>(call_count), kMaxRetries + 1);
}

TEST_F(ThemisDBTransactionTest, ExecuteWithRetryDoesNotRetryNonDeadlockError) {
    int call_count = 0;
    auto result = adapter.execute_with_retry<bool>(
        [&]() -> Result<bool> {
            ++call_count;
            return Result<bool>::err(ErrorCode::INTERNAL_ERROR, "fatal");
        },
        3,
        std::chrono::milliseconds{0}
    );
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INTERNAL_ERROR);
    EXPECT_EQ(call_count, 1);  // no retry for non-deadlock errors
}

// ---------------------------------------------------------------------------
// TransactionOptions – nested / retry configuration
// ---------------------------------------------------------------------------

TEST_F(ThemisDBTransactionTest, BeginTransactionWithAllIsolationLevels) {
    const std::vector<TransactionOptions::IsolationLevel> levels = {
        TransactionOptions::IsolationLevel::READ_UNCOMMITTED,
        TransactionOptions::IsolationLevel::READ_COMMITTED,
        TransactionOptions::IsolationLevel::REPEATABLE_READ,
        TransactionOptions::IsolationLevel::SERIALIZABLE,
    };
    for (auto level : levels) {
        TransactionOptions opts;
        opts.isolation_level = level;
        auto txn = adapter.begin_transaction(opts);
        ASSERT_TRUE(txn.is_ok()) << "Failed for isolation level: " << static_cast<int>(level);
        auto state = adapter.get_transaction_state(txn.value.value());
        ASSERT_TRUE(state.is_ok());
        EXPECT_EQ(state.value.value().isolation_level, level);
        adapter.rollback_transaction(txn.value.value());
    }
}

TEST_F(ThemisDBTransactionTest, BeginTransactionWithAllowNestedOption) {
    TransactionOptions opts;
    opts.allow_nested = true;
    auto txn = adapter.begin_transaction(opts);
    ASSERT_TRUE(txn.is_ok());

    // Savepoints act as nested transaction boundaries
    ASSERT_TRUE(adapter.create_savepoint(txn.value.value(), "nested_sp").is_ok());
    adapter.rollback_transaction(txn.value.value());
}



class ThemisDBCapabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;
};

TEST_F(ThemisDBCapabilityTest, HasRelationalCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::RELATIONAL_QUERIES));
}

TEST_F(ThemisDBCapabilityTest, HasVectorSearchCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::VECTOR_SEARCH));
}

TEST_F(ThemisDBCapabilityTest, HasGraphTraversalCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::GRAPH_TRAVERSAL));
}

TEST_F(ThemisDBCapabilityTest, HasDocumentStoreCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::DOCUMENT_STORE));
}

TEST_F(ThemisDBCapabilityTest, HasTransactionCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::TRANSACTIONS));
}

TEST_F(ThemisDBCapabilityTest, ConnectionPoolingNotAvailableWithoutInjection) {
    // Without a pool provider injected, has_capability() returns false.
    EXPECT_FALSE(adapter.has_capability(connection_pooling_capability()));
}

TEST_F(ThemisDBCapabilityTest, GetCapabilitiesExcludesConnectionPoolingWithoutInjection) {
    auto caps = adapter.get_capabilities();
    EXPECT_FALSE(caps.empty());
    auto it = std::find(caps.begin(), caps.end(), connection_pooling_capability());
    EXPECT_EQ(it, caps.end())
        << "CONNECTION_POOLING must not appear in get_capabilities() when no pool is injected";
}

TEST_F(ThemisDBCapabilityTest, ConnectionPoolingAvailableAfterInjection) {
    // After injecting a pool provider, has_capability() must return true.
    adapter.setConnectionPool([]() -> void* { return reinterpret_cast<void*>(0x1); });
    EXPECT_TRUE(adapter.has_capability(connection_pooling_capability()));
}

TEST_F(ThemisDBCapabilityTest, GetCapabilitiesIncludesConnectionPoolingAfterInjection) {
    adapter.setConnectionPool([]() -> void* { return reinterpret_cast<void*>(0x1); });
    auto caps = adapter.get_capabilities();
    auto it = std::find(caps.begin(), caps.end(), connection_pooling_capability());
    EXPECT_NE(it, caps.end())
        << "CONNECTION_POOLING must appear in get_capabilities() when a pool is injected";
}

TEST_F(ThemisDBCapabilityTest, ConnectionPoolingDisabledAfterNullInjection) {
    adapter.setConnectionPool([]() -> void* { return reinterpret_cast<void*>(0x1); });
    ASSERT_TRUE(adapter.has_capability(connection_pooling_capability()));
    // Removing the pool reverts the capability.
    adapter.setConnectionPool(nullptr);
    EXPECT_FALSE(adapter.has_capability(connection_pooling_capability()));
}

TEST_F(ThemisDBCapabilityTest, GetSystemInfoReturnsOk) {
    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().system_name.empty());
    EXPECT_FALSE(result.value.value().version.empty());
}

TEST_F(ThemisDBCapabilityTest, GetMetricsReturnsOk) {
    auto result = adapter.get_metrics();
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// Performance / overhead benchmarks
//
// All tests run against the in-process simulation layer (no live ThemisDB
// server required). Limits are intentionally generous to accommodate slow
// debug/CI builds while still guarding against major regressions.
// ---------------------------------------------------------------------------

class ThemisDBPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;

    static constexpr const char* kDocCol = "perf_docs";
    static constexpr const char* kVecCol = "perf_vecs";
    static constexpr const char* kRowTbl = "perf_rows";

    static constexpr size_t kDocCount   = 1000;
    static constexpr size_t kVecDim     = 32;
    static constexpr size_t kVecCount   = 200;
    static constexpr size_t kSearchK    = 10;
    static constexpr size_t kSearchRuns = 20;
    static constexpr size_t kRowCount   = 500;
    static constexpr size_t kTxnRuns    = 100;

    // Generous wall-clock limits for the in-process simulation layer.
    static constexpr int64_t kBulkDocMs  = 2000;
    static constexpr int64_t kBulkVecMs  = 2000;
    static constexpr int64_t kSearchMs   = 2000;
    static constexpr int64_t kBulkRowMs  = 2000;
    static constexpr int64_t kTxnMs      = 2000;
};

TEST_F(ThemisDBPerformanceTest, BulkDocumentInsertOverhead) {
    std::vector<Document> docs;
    docs.reserve(kDocCount);
    for (size_t i = 0; i < kDocCount; ++i) {
        Document doc;
        doc.id = "pdoc_" + std::to_string(i);
        doc.fields["idx"]  = Scalar{int64_t(i)};
        doc.fields["data"] = Scalar{std::string(32, 'x')};
        docs.push_back(std::move(doc));
    }

    auto t0 = std::chrono::steady_clock::now();
    auto result = adapter.batch_insert_documents(kDocCol, docs);
    auto t1 = std::chrono::steady_clock::now();

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), kDocCount);

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kBulkDocMs)
        << "Bulk insert of " << kDocCount << " documents took "
        << elapsed_ms << "ms (limit: " << kBulkDocMs << "ms)";
}

TEST_F(ThemisDBPerformanceTest, BulkVectorInsertOverhead) {
    std::vector<Vector> vecs;
    vecs.reserve(kVecCount);
    for (size_t i = 0; i < kVecCount; ++i) {
        vecs.push_back(make_vector(kVecDim, static_cast<float>(i) / kVecCount));
    }

    auto t0 = std::chrono::steady_clock::now();
    auto result = adapter.batch_insert_vectors(kVecCol, vecs);
    auto t1 = std::chrono::steady_clock::now();

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), kVecCount);

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kBulkVecMs)
        << "Bulk insert of " << kVecCount << " vectors (" << kVecDim
        << "D) took " << elapsed_ms << "ms (limit: " << kBulkVecMs << "ms)";
}

TEST_F(ThemisDBPerformanceTest, VectorSearchOverhead) {
    for (size_t i = 0; i < kVecCount; ++i) {
        adapter.insert_vector(kVecCol, make_vector(kVecDim, static_cast<float>(i) / kVecCount));
    }

    auto query = make_vector(kVecDim, 0.5f);

    auto t0 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < kSearchRuns; ++i) {
        auto result = adapter.search_vectors(kVecCol, query, kSearchK);
        ASSERT_TRUE(result.is_ok());
    }
    auto t1 = std::chrono::steady_clock::now();

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kSearchMs)
        << kSearchRuns << " vector searches (k=" << kSearchK << ") over "
        << kVecCount << " vectors took " << elapsed_ms
        << "ms (limit: " << kSearchMs << "ms)";
}

TEST_F(ThemisDBPerformanceTest, BulkRelationalInsertOverhead) {
    std::vector<RelationalRow> rows;
    rows.reserve(kRowCount);
    for (size_t i = 0; i < kRowCount; ++i) {
        RelationalRow row;
        row.columns["idx"]  = Scalar{int64_t(i)};
        row.columns["data"] = Scalar{std::string(16, 'y')};
        rows.push_back(std::move(row));
    }

    auto t0 = std::chrono::steady_clock::now();
    auto result = adapter.batch_insert(kRowTbl, rows);
    auto t1 = std::chrono::steady_clock::now();

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), kRowCount);

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kBulkRowMs)
        << "Bulk insert of " << kRowCount << " rows took "
        << elapsed_ms << "ms (limit: " << kBulkRowMs << "ms)";
}

TEST_F(ThemisDBPerformanceTest, TransactionBeginCommitOverhead) {
    auto t0 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < kTxnRuns; ++i) {
        auto begin = adapter.begin_transaction();
        ASSERT_TRUE(begin.is_ok());
        auto commit = adapter.commit_transaction(begin.value.value());
        ASSERT_TRUE(commit.is_ok());
    }
    auto t1 = std::chrono::steady_clock::now();

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kTxnMs)
        << kTxnRuns << " begin+commit cycles took "
        << elapsed_ms << "ms (limit: " << kTxnMs << "ms)";
}

// ---------------------------------------------------------------------------
// Integration tests — in-process simulation (wired adapter, no live server)
//
// These tests verify that the production-mode data path works correctly:
// insert operations persist data in the in-memory collections and retrieval
// operations return the previously inserted data.
// ---------------------------------------------------------------------------

class ThemisDBIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto connect_result = adapter.connect("themisdb://localhost:7777");
        ASSERT_TRUE(connect_result.is_ok());
    }

    ThemisDBAdapter adapter;
};

// ── UUID-based ID generation ─────────────────────────────────────────────────

TEST_F(ThemisDBIntegrationTest, InsertVectorGeneratesUniqueIds) {
    constexpr size_t kCount = 50;
    std::vector<std::string> ids;
    ids.reserve(kCount);

    for (size_t i = 0; i < kCount; ++i) {
        auto v = make_vector(8, static_cast<float>(i) / kCount);
        auto res = adapter.insert_vector("id_test", v);
        ASSERT_TRUE(res.is_ok());
        ids.push_back(res.value.value());
    }

    // All IDs must be unique.
    std::sort(ids.begin(), ids.end());
    const auto dup = std::adjacent_find(ids.begin(), ids.end());
    EXPECT_EQ(dup, ids.end()) << "Duplicate vector IDs detected";
}

TEST_F(ThemisDBIntegrationTest, TransactionIdsAreUniqueUuids) {
    constexpr size_t kCount = 20;
    std::vector<std::string> txn_ids;
    txn_ids.reserve(kCount);

    for (size_t i = 0; i < kCount; ++i) {
        auto res = adapter.begin_transaction();
        ASSERT_TRUE(res.is_ok());
        txn_ids.push_back(res.value.value());
        auto commit_res = adapter.commit_transaction(res.value.value());
        ASSERT_TRUE(commit_res.is_ok());
    }

    std::sort(txn_ids.begin(), txn_ids.end());
    const auto dup = std::adjacent_find(txn_ids.begin(), txn_ids.end());
    EXPECT_EQ(dup, txn_ids.end()) << "Duplicate transaction IDs detected";
}

// ── Relational round-trip ────────────────────────────────────────────────────

TEST_F(ThemisDBIntegrationTest, InsertRowAndRetrieveViaExecuteQuery) {
    RelationalRow row;
    row.columns["name"]  = Scalar{std::string("Alice")};
    row.columns["score"] = Scalar{int64_t(99)};

    auto ins = adapter.insert_row("users", row);
    ASSERT_TRUE(ins.is_ok());
    EXPECT_EQ(ins.value.value(), 1u);

    // execute_query in simulation mode: passing the table name as the query
    // returns all rows stored under that key.
    auto qr = adapter.execute_query("users");
    ASSERT_TRUE(qr.is_ok());
    EXPECT_EQ(qr.value.value().rows.size(), 1u);
}

TEST_F(ThemisDBIntegrationTest, BatchInsertRowsPersistedCorrectly) {
    constexpr size_t kRows = 10;
    std::vector<RelationalRow> rows;
    rows.reserve(kRows);
    for (size_t i = 0; i < kRows; ++i) {
        RelationalRow r;
        r.columns["idx"] = Scalar{static_cast<int64_t>(i)};
        rows.push_back(std::move(r));
    }

    auto ins = adapter.batch_insert("batch_tbl", rows);
    ASSERT_TRUE(ins.is_ok());
    EXPECT_EQ(ins.value.value(), kRows);

    auto qr = adapter.execute_query("batch_tbl");
    ASSERT_TRUE(qr.is_ok());
    EXPECT_EQ(qr.value.value().rows.size(), kRows);
}

// ── Vector search round-trip ─────────────────────────────────────────────────

TEST_F(ThemisDBIntegrationTest, VectorSearchReturnsInsertedVectors) {
    constexpr size_t kDim    = 8;
    constexpr size_t kCount  = 20;
    constexpr size_t kK      = 5;

    for (size_t i = 0; i < kCount; ++i) {
        auto v = make_vector(kDim, static_cast<float>(i) / kCount);
        adapter.insert_vector("search_col", v);
    }

    // Query vector close to the 0.5-normalised vectors.
    auto query = make_vector(kDim, 0.5f);
    auto res = adapter.search_vectors("search_col", query, kK);
    ASSERT_TRUE(res.is_ok());
    EXPECT_EQ(res.value.value().size(), kK)
        << "Expected top-" << kK << " results from " << kCount << " stored vectors";
}

TEST_F(ThemisDBIntegrationTest, VectorSearchOnEmptyCollectionReturnsEmpty) {
    auto query = make_vector(4, 1.0f);
    auto res = adapter.search_vectors("empty_col", query, 5);
    ASSERT_TRUE(res.is_ok());
    EXPECT_TRUE(res.value.value().empty());
}

TEST_F(ThemisDBIntegrationTest, VectorSearchResultsAreSortedByDistance) {
    // Insert two clearly different vectors.
    Vector near, far;
    near.data = {1.0f, 0.0f, 0.0f, 0.0f};
    far.data  = {0.0f, 0.0f, 0.0f, 1.0f};
    adapter.insert_vector("dist_col", near);
    adapter.insert_vector("dist_col", far);

    Vector query;
    query.data = {1.0f, 0.0f, 0.0f, 0.0f}; // identical to "near"

    auto res = adapter.search_vectors("dist_col", query, 2);
    ASSERT_TRUE(res.is_ok());
    const auto& results = res.value.value();
    ASSERT_EQ(results.size(), 2u);
    // The nearest result should have a smaller (or equal) distance.
    EXPECT_LE(results[0].second, results[1].second);
}

// ── Graph round-trip ─────────────────────────────────────────────────────────

TEST_F(ThemisDBIntegrationTest, ShortestPathFindsDirectEdge) {
    GraphNode a, b;
    a.id = "A"; b.id = "B";
    adapter.insert_node(a);
    adapter.insert_node(b);

    GraphEdge e;
    e.id        = "e_AB";
    e.source_id = "A";
    e.target_id = "B";
    e.label     = "LINKS";
    e.weight    = 2.5;
    adapter.insert_edge(e);

    auto res = adapter.shortest_path("A", "B");
    ASSERT_TRUE(res.is_ok());
    const auto& path = res.value.value();
    ASSERT_GE(path.nodes.size(), 2u);
    EXPECT_EQ(path.nodes.front().id, "A");
    EXPECT_EQ(path.nodes.back().id, "B");
    EXPECT_DOUBLE_EQ(path.total_weight, 2.5);
}

TEST_F(ThemisDBIntegrationTest, ShortestPathSameSourceAndTarget) {
    GraphNode n;
    n.id = "solo";
    adapter.insert_node(n);

    auto res = adapter.shortest_path("solo", "solo");
    ASSERT_TRUE(res.is_ok());
    const auto& path = res.value.value();
    ASSERT_EQ(path.nodes.size(), 1u);
    EXPECT_EQ(path.nodes[0].id, "solo");
}

TEST_F(ThemisDBIntegrationTest, ShortestPathNoRouteReturnsEmptyPath) {
    GraphNode x, y;
    x.id = "X"; y.id = "Y";
    adapter.insert_node(x);
    adapter.insert_node(y);
    // No edge between X and Y.

    auto res = adapter.shortest_path("X", "Y");
    ASSERT_TRUE(res.is_ok());
    EXPECT_TRUE(res.value.value().nodes.empty());
}

TEST_F(ThemisDBIntegrationTest, TraverseReturnsReachableNodes) {
    // Build a simple chain: P -> Q -> R
    for (const auto& id : {"P", "Q", "R"}) {
        GraphNode n; n.id = id;
        adapter.insert_node(n);
    }
    GraphEdge pq, qr;
    pq.id = "pq"; pq.source_id = "P"; pq.target_id = "Q"; pq.label = "NEXT";
    qr.id = "qr"; qr.source_id = "Q"; qr.target_id = "R"; qr.label = "NEXT";
    adapter.insert_edge(pq);
    adapter.insert_edge(qr);

    auto res = adapter.traverse("P", 2);
    ASSERT_TRUE(res.is_ok());
    const auto& nodes = res.value.value();
    EXPECT_GE(nodes.size(), 3u) << "Should reach P, Q, and R within depth 2";
}

TEST_F(ThemisDBIntegrationTest, TraverseWithEdgeLabelFilterApplied) {
    // Build a fork: S -> T (KNOWS), S -> U (LIKES)
    for (const auto& id : {"S", "T", "U"}) {
        GraphNode n; n.id = id;
        adapter.insert_node(n);
    }
    GraphEdge st, su;
    st.id = "st"; st.source_id = "S"; st.target_id = "T"; st.label = "KNOWS";
    su.id = "su"; su.source_id = "S"; su.target_id = "U"; su.label = "LIKES";
    adapter.insert_edge(st);
    adapter.insert_edge(su);

    // Traverse only KNOWS edges: should not reach U.
    auto res = adapter.traverse("S", 1, {"KNOWS"});
    ASSERT_TRUE(res.is_ok());
    const auto& nodes = res.value.value();
    bool found_T = false, found_U = false;
    for (const auto& n : nodes) {
        if (n.id == "T") {
          found_T = true;
        }
        if (n.id == "U") {
          found_U = true;
        }
    }
    EXPECT_TRUE(found_T);
    EXPECT_FALSE(found_U);
}

// ── Document round-trip ───────────────────────────────────────────────────────

TEST_F(ThemisDBIntegrationTest, InsertAndFindDocumentRoundTrip) {
    auto doc = make_doc("d1", "Widget", 42);
    auto ins = adapter.insert_document("things", doc);
    ASSERT_TRUE(ins.is_ok());
    EXPECT_EQ(ins.value.value(), "d1");

    auto found = adapter.find_documents(
        "things", {{"name", Scalar{std::string("Widget")}}});
    ASSERT_TRUE(found.is_ok());
    ASSERT_EQ(found.value.value().size(), 1u);
    EXPECT_EQ(found.value.value()[0].id, "d1");
}

TEST_F(ThemisDBIntegrationTest, FindDocumentsWithNoMatchReturnsEmpty) {
    auto doc = make_doc("d2", "Gadget", 7);
    adapter.insert_document("things2", doc);

    auto found = adapter.find_documents(
        "things2", {{"name", Scalar{std::string("NoSuchItem")}}});
    ASSERT_TRUE(found.is_ok());
    EXPECT_TRUE(found.value.value().empty());
}

TEST_F(ThemisDBIntegrationTest, UpdateDocumentsPersistsChanges) {
    auto doc = make_doc("u1", "OldName", 10);
    adapter.insert_document("things3", doc);

    auto updated = adapter.update_documents(
        "things3",
        {{"name", Scalar{std::string("OldName")}}},
        {{"name", Scalar{std::string("NewName")}}}
    );
    ASSERT_TRUE(updated.is_ok());
    EXPECT_EQ(updated.value.value(), 1u);

    auto found = adapter.find_documents(
        "things3", {{"name", Scalar{std::string("NewName")}}});
    ASSERT_TRUE(found.is_ok());
    ASSERT_EQ(found.value.value().size(), 1u);
    EXPECT_EQ(found.value.value()[0].id, "u1");
}

TEST_F(ThemisDBIntegrationTest, InsertDocumentWithEmptyIdGeneratesUuid) {
    Document doc;
    doc.fields["x"] = Scalar{int64_t(99)};

    auto res = adapter.insert_document("auto_id_col", doc);
    ASSERT_TRUE(res.is_ok());
    const std::string& id = res.value.value();
    EXPECT_FALSE(id.empty());

    // UUID v4 format: 8-4-4-4-12 hex chars separated by hyphens (36 chars total).
    ASSERT_EQ(id.size(), 36u) << "Expected UUID v4 format, got: " << id;
    EXPECT_EQ(id[8],  '-') << "UUID v4: missing first hyphen";
    EXPECT_EQ(id[13], '-') << "UUID v4: missing second hyphen";
    EXPECT_EQ(id[18], '-') << "UUID v4: missing third hyphen";
    EXPECT_EQ(id[23], '-') << "UUID v4: missing fourth hyphen";
    // Version nibble must be '4' (UUID v4).
    EXPECT_EQ(id[14], '4') << "UUID v4: version nibble must be '4'";
    // Variant nibble must be one of '8', '9', 'a', 'b' (RFC 4122 variant 10xx).
    const char variant = id[19];
    EXPECT_TRUE(variant == '8' || variant == '9' ||
                variant == 'a' || variant == 'b')
        << "UUID v4: variant nibble must be 8/9/a/b, got: " << variant;
}

// ── shortest_path with custom max_depth ──────────────────────────────────────

TEST_F(ThemisDBIntegrationTest, ShortestPathRespectMaxDepth) {
    // Build chain: D0 -> D1 -> D2 -> D3
    for (const auto& id : {"D0", "D1", "D2", "D3"}) {
        GraphNode n; n.id = id;
        adapter.insert_node(n);
    }
    auto make_edge = [&](const std::string& id,
                         const std::string& src,
                         const std::string& tgt) {
        GraphEdge e;
        e.id = id; e.source_id = src; e.target_id = tgt;
        e.weight = 1.0;
        adapter.insert_edge(e);
    };
    make_edge("d01", "D0", "D1");
    make_edge("d12", "D1", "D2");
    make_edge("d23", "D2", "D3");

    // Without depth cap: D0 -> D3 reachable (3 hops)
    auto res_unbounded = adapter.shortest_path("D0", "D3");
    ASSERT_TRUE(res_unbounded.is_ok());
    EXPECT_FALSE(res_unbounded.value.value().nodes.empty());

    // With max_depth=2: D3 is 3 hops away and should NOT be reachable
    auto res_limited = adapter.shortest_path("D0", "D3", 2);
    ASSERT_TRUE(res_limited.is_ok()) << "Operation must succeed, not return NOT_IMPLEMENTED";
    // The path should be empty because D3 is beyond depth 2
    EXPECT_TRUE(res_limited.value.value().nodes.empty())
        << "D3 is 3 hops away; should be unreachable within max_depth=2";

    // With max_depth=3: exactly enough hops to reach D3
    auto res_exact = adapter.shortest_path("D0", "D3", 3);
    ASSERT_TRUE(res_exact.is_ok());
    EXPECT_FALSE(res_exact.value.value().nodes.empty())
        << "D3 should be reachable within max_depth=3";
}

TEST_F(ThemisDBIntegrationTest, ShortestPathMaxDepthOne) {
    // A -> B (direct), A -> C -> B (via C, 2 hops)
    for (const auto& id : {"MA", "MB", "MC"}) {
        GraphNode n; n.id = id;
        adapter.insert_node(n);
    }
    GraphEdge ab, ac, cb;
    ab.id = "mab"; ab.source_id = "MA"; ab.target_id = "MB"; ab.weight = 5.0;
    ac.id = "mac"; ac.source_id = "MA"; ac.target_id = "MC"; ac.weight = 1.0;
    cb.id = "mcb"; cb.source_id = "MC"; cb.target_id = "MB"; cb.weight = 1.0;
    adapter.insert_edge(ab);
    adapter.insert_edge(ac);
    adapter.insert_edge(cb);

    // max_depth=1: only direct 1-hop path A->B is visible
    auto res = adapter.shortest_path("MA", "MB", 1);
    ASSERT_TRUE(res.is_ok()) << "max_depth=1 must not return NOT_IMPLEMENTED";
    const auto& path = res.value.value();
    // Direct path A->B should still be found (1 hop, within depth=1)
    EXPECT_FALSE(path.nodes.empty());
    EXPECT_EQ(path.nodes.front().id, "MA");
    EXPECT_EQ(path.nodes.back().id, "MB");
}

// ── traverse with multiple edge labels ───────────────────────────────────────

TEST_F(ThemisDBIntegrationTest, TraverseMultiLabelReachesNodesOfEitherType) {
    // Build: N -> A (KNOWS), N -> B (LIKES), N -> C (HATES)
    for (const auto& id : {"N", "A", "B", "C"}) {
        GraphNode n; n.id = id;
        adapter.insert_node(n);
    }
    GraphEdge na, nb, nc;
    na.id = "na"; na.source_id = "N"; na.target_id = "A"; na.label = "KNOWS";
    nb.id = "nb"; nb.source_id = "N"; nb.target_id = "B"; nb.label = "LIKES";
    nc.id = "nc"; nc.source_id = "N"; nc.target_id = "C"; nc.label = "HATES";
    adapter.insert_edge(na);
    adapter.insert_edge(nb);
    adapter.insert_edge(nc);

    // Traverse via both KNOWS and LIKES: should reach A and B, but NOT C.
    auto res = adapter.traverse("N", 1, {"KNOWS", "LIKES"});
    ASSERT_TRUE(res.is_ok()) << "Multi-label traverse must not return NOT_IMPLEMENTED";
    const auto& nodes = res.value.value();

    bool found_A = false, found_B = false, found_C = false;
    for (const auto& n : nodes) {
        if (n.id == "A") {
          found_A = true;
        }
        if (n.id == "B") {
          found_B = true;
        }
        if (n.id == "C") {
          found_C = true;
        }
    }
    EXPECT_TRUE(found_A) << "Node A (via KNOWS) must be reachable";
    EXPECT_TRUE(found_B) << "Node B (via LIKES) must be reachable";
    EXPECT_FALSE(found_C) << "Node C (via HATES) must NOT be reachable";
}

TEST_F(ThemisDBIntegrationTest, TraverseMultiLabelDeduplicatesSharedNodes) {
    // V -> W (L1), V -> W (L2) — W reachable via both labels; should appear once.
    GraphNode v, w;
    v.id = "V"; w.id = "W";
    adapter.insert_node(v);
    adapter.insert_node(w);
    GraphEdge e1, e2;
    e1.id = "vw1"; e1.source_id = "V"; e1.target_id = "W"; e1.label = "L1";
    e2.id = "vw2"; e2.source_id = "V"; e2.target_id = "W"; e2.label = "L2";
    adapter.insert_edge(e1);
    adapter.insert_edge(e2);

    auto res = adapter.traverse("V", 1, {"L1", "L2"});
    ASSERT_TRUE(res.is_ok());
    const auto& nodes = res.value.value();

    int w_count = 0;
    for (const auto& n : nodes) {
        if (n.id == "W") {
          w_count++;
        }
    }
    EXPECT_EQ(w_count, 1) << "W reachable via both labels must appear exactly once";
}

TEST_F(ThemisDBIntegrationTest, TraverseThreeLabelsAllReachable) {
    // Hub H -> X (T1), H -> Y (T2), H -> Z (T3)
    for (const auto& id : {"H", "X", "Y", "Z"}) {
        GraphNode n; n.id = id;
        adapter.insert_node(n);
    }
    GraphEdge hx, hy, hz;
    hx.id = "hx"; hx.source_id = "H"; hx.target_id = "X"; hx.label = "T1";
    hy.id = "hy"; hy.source_id = "H"; hy.target_id = "Y"; hy.label = "T2";
    hz.id = "hz"; hz.source_id = "H"; hz.target_id = "Z"; hz.label = "T3";
    adapter.insert_edge(hx);
    adapter.insert_edge(hy);
    adapter.insert_edge(hz);

    auto res = adapter.traverse("H", 1, {"T1", "T2", "T3"});
    ASSERT_TRUE(res.is_ok()) << "Three-label traverse must not return NOT_IMPLEMENTED";
    const auto& nodes = res.value.value();

    bool found_X = false, found_Y = false, found_Z = false;
    for (const auto& n : nodes) {
        if (n.id == "X") {
          found_X = true;
        }
        if (n.id == "Y") {
          found_Y = true;
        }
        if (n.id == "Z") {
          found_Z = true;
        }
    }
    EXPECT_TRUE(found_X) << "X via T1 must be reachable";
    EXPECT_TRUE(found_Y) << "Y via T2 must be reachable";
    EXPECT_TRUE(found_Z) << "Z via T3 must be reachable";
}

// ============================================================================
// CHI-EI: Engine-Injection NOT_IMPLEMENTED guard tests
// ============================================================================
//
// These tests verify that injecting engine pointers (query_engine_,
// vector_index_, graph_index_) while THEMISDB_ENGINE_AVAILABLE is NOT defined
// at compile time causes the adapter to return ErrorCode::NOT_IMPLEMENTED
// deterministically — rather than silently falling back to simulation or
// exhibiting undefined behaviour.
//
// Design: a non-null sentinel pointer is constructed from the address of a
// statically-allocated placeholder object.  This address is never dereferenced
// inside the adapter because the #else branch (active when
// THEMISDB_ENGINE_AVAILABLE is absent) returns the error immediately without
// touching the pointer value.
//
// Removal Plan: when THEMISDB_ENGINE_AVAILABLE is universally defined by the
// build system for engine-linked configurations, these tests should be replaced
// by integration tests against a real engine (ChimeraAdapters.cmake, Q3 2026).

namespace {
// Shared address-of-static sentinel used as a non-null, non-dereferenceable
// engine pointer for the tests below.
static int g_engine_sentinel = 0;
} // namespace

class ThemisDBEngineInjectionTest : public ::testing::Test {
protected:
    // Helper: produce a non-null sentinel cast to the requested engine type.
    // The sentinel is NEVER dereferenced — it only makes the adapter's
    // `if (query_engine_)` / `if (vector_index_)` / `if (graph_index_)` guards
    // evaluate to true so that the #else NOT_IMPLEMENTED branch is exercised.
    static themis::QueryEngine* fake_qe() {
        return reinterpret_cast<themis::QueryEngine*>(&g_engine_sentinel);
    }
    static themis::VectorIndexManager* fake_vi() {
        return reinterpret_cast<themis::VectorIndexManager*>(&g_engine_sentinel);
    }
    static themis::GraphIndexManager* fake_gi() {
        return reinterpret_cast<themis::GraphIndexManager*>(&g_engine_sentinel);
    }
};

// CHI-EI-01: execute_query returns NOT_IMPLEMENTED when QueryEngine is
//            injected but THEMISDB_ENGINE_AVAILABLE is absent.
TEST_F(ThemisDBEngineInjectionTest, ExecuteQueryNotImplementedWithoutEngineHeaders) {
    ThemisDBAdapter adapter(fake_qe());
    ASSERT_TRUE(adapter.connect("themisdb://localhost:7777").is_ok());

    auto res = adapter.execute_query("FOR doc IN items RETURN doc");
    EXPECT_TRUE(res.is_err());
    EXPECT_EQ(res.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// CHI-EI-02: execute_query with empty query string and injected QueryEngine.
TEST_F(ThemisDBEngineInjectionTest, ExecuteQueryEmptyQueryNotImplemented) {
    ThemisDBAdapter adapter(fake_qe());
    ASSERT_TRUE(adapter.connect("themisdb://localhost:7777").is_ok());

    auto res = adapter.execute_query("");
    EXPECT_TRUE(res.is_err());
    EXPECT_EQ(res.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// CHI-EI-03: execute_query with params and injected QueryEngine.
TEST_F(ThemisDBEngineInjectionTest, ExecuteQueryWithParamsNotImplemented) {
    ThemisDBAdapter adapter(fake_qe());
    ASSERT_TRUE(adapter.connect("themisdb://localhost:7777").is_ok());

    auto res = adapter.execute_query("SELECT ?", {Scalar{int64_t(42)}});
    EXPECT_TRUE(res.is_err());
    EXPECT_EQ(res.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// CHI-EI-04: search_vectors returns NOT_IMPLEMENTED when VectorIndexManager
//            is injected but THEMISDB_ENGINE_AVAILABLE is absent.
TEST_F(ThemisDBEngineInjectionTest, SearchVectorsNotImplementedWithoutEngineHeaders) {
    ThemisDBAdapter adapter(nullptr, fake_vi());
    ASSERT_TRUE(adapter.connect("themisdb://localhost:7777").is_ok());

    Vector qv;
    qv.data = {1.0f, 0.0f, 0.0f};
    auto res = adapter.search_vectors("embeddings", qv, 5);
    EXPECT_TRUE(res.is_err());
    EXPECT_EQ(res.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// CHI-EI-05: search_vectors with k=0 and injected VectorIndexManager.
TEST_F(ThemisDBEngineInjectionTest, SearchVectorsKZeroNotImplemented) {
    ThemisDBAdapter adapter(nullptr, fake_vi());
    ASSERT_TRUE(adapter.connect("themisdb://localhost:7777").is_ok());

    Vector qv;
    qv.data = {0.5f, 0.5f};
    auto res = adapter.search_vectors("col", qv, 0);
    EXPECT_TRUE(res.is_err());
    EXPECT_EQ(res.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// CHI-EI-06: shortest_path returns NOT_IMPLEMENTED when GraphIndexManager
//            is injected but THEMISDB_ENGINE_AVAILABLE is absent.
TEST_F(ThemisDBEngineInjectionTest, ShortestPathNotImplementedWithoutEngineHeaders) {
    ThemisDBAdapter adapter(nullptr, nullptr, fake_gi());
    ASSERT_TRUE(adapter.connect("themisdb://localhost:7777").is_ok());

    auto res = adapter.shortest_path("A", "B");
    EXPECT_TRUE(res.is_err());
    EXPECT_EQ(res.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// CHI-EI-07: shortest_path with custom max_depth and injected GraphIndexManager.
TEST_F(ThemisDBEngineInjectionTest, ShortestPathCustomDepthNotImplemented) {
    ThemisDBAdapter adapter(nullptr, nullptr, fake_gi());
    ASSERT_TRUE(adapter.connect("themisdb://localhost:7777").is_ok());

    auto res = adapter.shortest_path("src", "dst", 3);
    EXPECT_TRUE(res.is_err());
    EXPECT_EQ(res.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// CHI-EI-08: traverse returns NOT_IMPLEMENTED when GraphIndexManager is
//            injected but THEMISDB_ENGINE_AVAILABLE is absent.
TEST_F(ThemisDBEngineInjectionTest, TraverseNotImplementedWithoutEngineHeaders) {
    ThemisDBAdapter adapter(nullptr, nullptr, fake_gi());
    ASSERT_TRUE(adapter.connect("themisdb://localhost:7777").is_ok());

    // No edge labels — unfiltered BFS path
    auto res = adapter.traverse("root", 3);
    EXPECT_TRUE(res.is_err());
    EXPECT_EQ(res.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// CHI-EI-08b: traverse with edge labels and injected GraphIndexManager.
TEST_F(ThemisDBEngineInjectionTest, TraverseWithLabelsNotImplemented) {
    ThemisDBAdapter adapter(nullptr, nullptr, fake_gi());
    ASSERT_TRUE(adapter.connect("themisdb://localhost:7777").is_ok());

    auto res = adapter.traverse("root", 2, {"KNOWS", "LIKES"});
    EXPECT_TRUE(res.is_err());
    EXPECT_EQ(res.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// CHI-EI-09: All three engines injected simultaneously — each of the three
//            engine-dispatched methods must return NOT_IMPLEMENTED.
TEST_F(ThemisDBEngineInjectionTest, AllEnginesInjectedEachMethodReturnsNotImplemented) {
    ThemisDBAdapter adapter(fake_qe(), fake_vi(), fake_gi());
    ASSERT_TRUE(adapter.connect("themisdb://localhost:7777").is_ok());

    auto q_res = adapter.execute_query("SELECT 1");
    EXPECT_TRUE(q_res.is_err());
    EXPECT_EQ(q_res.error_code, ErrorCode::NOT_IMPLEMENTED);

    Vector qv; qv.data = {1.0f};
    auto v_res = adapter.search_vectors("col", qv, 1);
    EXPECT_TRUE(v_res.is_err());
    EXPECT_EQ(v_res.error_code, ErrorCode::NOT_IMPLEMENTED);

    auto g_res = adapter.shortest_path("X", "Y");
    EXPECT_TRUE(g_res.is_err());
    EXPECT_EQ(g_res.error_code, ErrorCode::NOT_IMPLEMENTED);

    auto t_res = adapter.traverse("X", 1);
    EXPECT_TRUE(t_res.is_err());
    EXPECT_EQ(t_res.error_code, ErrorCode::NOT_IMPLEMENTED);
}
