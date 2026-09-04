/// @file test_base_interfaces.cpp
/// @brief Unit tests for base interface definitions
///
/// These tests verify that the interfaces:
/// - Compile correctly with the current Result<T> API
/// - Have pure virtual methods
/// - Can be subclassed
/// - Have proper virtual destructors
/// - Behave correctly through polymorphism

#include <gtest/gtest.h>
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "themis/base/interfaces/storage_interface.h"
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/index_interface.h"
#include "themis/base/interfaces/security_interface.h"

using namespace themis;

// =============================================================================
// Compile-time / abstract-class Tests
// =============================================================================

TEST(BaseInterfaces, StorageInterfaceIsAbstract) {
    EXPECT_TRUE(std::is_abstract_v<IStorageEngine>);
    EXPECT_TRUE(std::is_abstract_v<IStorageEngineFactory>);
}

TEST(BaseInterfaces, QueryInterfaceIsAbstract) {
    EXPECT_TRUE(std::is_abstract_v<IExpressionEvaluator>);
    EXPECT_TRUE(std::is_abstract_v<IQueryEngine>);
    EXPECT_TRUE(std::is_abstract_v<IQueryEngineFactory>);
}

TEST(BaseInterfaces, IndexInterfaceIsAbstract) {
    EXPECT_TRUE(std::is_abstract_v<ISecondaryIndex>);
    EXPECT_TRUE(std::is_abstract_v<IVectorIndex>);
    EXPECT_TRUE(std::is_abstract_v<IGraphIndex>);
    EXPECT_TRUE(std::is_abstract_v<IIndexManager>);
}

TEST(BaseInterfaces, SecurityInterfaceIsAbstract) {
    EXPECT_TRUE(std::is_abstract_v<IKeyProvider>);
    EXPECT_TRUE(std::is_abstract_v<IFieldEncryption>);
    EXPECT_TRUE(std::is_abstract_v<IFieldEncryptionFactory>);
    EXPECT_TRUE(std::is_abstract_v<IKeyProviderFactory>);
}

TEST(BaseInterfaces, HasVirtualDestructors) {
    EXPECT_TRUE(std::has_virtual_destructor_v<IStorageEngine>);
    EXPECT_TRUE(std::has_virtual_destructor_v<IQueryEngine>);
    EXPECT_TRUE(std::has_virtual_destructor_v<ISecondaryIndex>);
    EXPECT_TRUE(std::has_virtual_destructor_v<IKeyProvider>);
}

// =============================================================================
// Mock Implementations
// =============================================================================

// --- IStorageEngine mock ---

class MockStorageEngine : public IStorageEngine {
public:
    Result<void> open(const std::string& /*db_path*/) override {
        opened_ = true;
        return OkVoid();
    }

    void close() override { opened_ = false; }

    Result<void> put(const std::string& key, const std::string& value) override {
        data_[key] = value;
        return OkVoid();
    }

    Result<std::string> get(const std::string& key) override {
        auto it = data_.find(key);
        if (it != data_.end()) {
            return Ok(it->second);
        }
        return Err<std::string>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, key);
    }

    Result<void> del(const std::string& key) override {
        if (data_.erase(key) > 0) {
            return OkVoid();
        }
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, key);
    }

    bool opened_ = false;

private:
    std::map<std::string, std::string> data_;
};

// --- IExpressionEvaluator mock ---

class MockExpressionEvaluator : public IExpressionEvaluator {
public:
    bool evaluate(const std::string& /*expression*/,
                  const void* /*context*/) const override { return true; }

    std::string get_expression_type() const override { return "MOCK"; }
};

// --- IQueryEngine mock ---

class MockQueryEngine : public IQueryEngine {
public:
    Result<std::string> execute(const std::string& /*query*/) override {
        return Ok(std::string{"mock result"});
    }

    Result<void> validate(const std::string& /*query*/) const override {
        return OkVoid();
    }

    Result<std::unique_ptr<IExpressionEvaluator>>
    createExpressionEvaluator() const override {
        return Ok<std::unique_ptr<IExpressionEvaluator>>(
            std::make_unique<MockExpressionEvaluator>());
    }

    Result<std::string> explainQuery(const std::string& /*query*/) const override {
        return Ok(std::string{"mock plan"});
    }
};

// --- IFieldEncryption mock ---

class MockFieldEncryption : public IFieldEncryption {
public:
    std::vector<uint8_t> encrypt_field(
        const std::string& /*field_name*/,
        const std::vector<uint8_t>& plaintext) override {
        // XOR each byte with 0xFF as a trivial mock cipher
        std::vector<uint8_t> out(plaintext);
        for (auto& b : out) {
          b ^= 0xFF;
        }
        return out;
    }

    std::vector<uint8_t> decrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& ciphertext) override {
        return encrypt_field(field_name, ciphertext); // symmetric
    }

    bool should_encrypt(const std::string& field_name) const override {
        return field_name == "secret";
    }
};

// --- IKeyProvider mock ---

class MockKeyProvider : public IKeyProvider {
public:
    std::vector<uint8_t> get_key(const std::string& /*key_id*/) override {
        return {0x00, 0x11, 0x22, 0x33};
    }

    std::vector<uint8_t> rotate_key(const std::string& /*key_id*/) override {
        return {0xAA, 0xBB, 0xCC, 0xDD};
    }
};

// --- ISecondaryIndex mock ---

class MockSecondaryIndex : public ISecondaryIndex {
public:
    bool insert(std::string_view indexed_value,
                std::string_view primary_key) override {
        index_[std::string(indexed_value)].push_back(std::string(primary_key));
        return true;
    }

    bool remove(std::string_view indexed_value,
                std::string_view primary_key) override {
        auto it = index_.find(std::string(indexed_value));
        if (it == index_.end()) {
          return false;
        }
        auto& vec = it->second;
        auto pos = std::find(vec.begin(), vec.end(), std::string(primary_key));
        if (pos == vec.end()) {
          return false;
        }
        vec.erase(pos);
        return true;
    }

    std::vector<std::string> lookup(std::string_view value) const override {
        auto it = index_.find(std::string(value));
        if (it == index_.end()) return {};
        return it->second;
    }

    std::vector<std::string> rangeScan(
        std::string_view start, std::string_view end,
        ScanOrder /*order*/ = ScanOrder::ASCENDING) const override {
        std::vector<std::string> result;
        std::string s(start), e(end);
        for (const auto& [k, pks] : index_) {
            if (k >= s && k < e) {
                result.insert(result.end(), pks.begin(), pks.end());
            }
        }
        return result;
    }

    std::string getName() const override { return "mock_secondary"; }
    std::string getFieldName() const override { return "mock_field"; }
    std::string getStatistics() const override { return "{}"; }

private:
    std::map<std::string, std::vector<std::string>> index_;
};

// --- IVectorIndex mock ---

class MockVectorIndex : public IVectorIndex {
public:
    bool insert(std::string_view primary_key,
                const std::vector<float>& vector) override {
        vectors_[std::string(primary_key)] = vector;
        return true;
    }

    bool remove(std::string_view primary_key) override {
        return vectors_.erase(std::string(primary_key)) > 0;
    }

    std::vector<VectorSearchResult> search(
        const std::vector<float>& /*query*/, uint32_t k,
        const IExpressionEvaluator* /*filter*/ = nullptr) const override {
        std::vector<VectorSearchResult> results;
        for (const auto& [pk, _] : vectors_) {
            if (results.size() >= static_cast<size_t>(k)) {
              break;
            }
            results.emplace_back(pk, 0.0f);
        }
        return results;
    }

    std::vector<VectorSearchResult> rangeSearch(
        const std::vector<float>& /*query*/, float /*max_distance*/,
        const IExpressionEvaluator* /*filter*/ = nullptr) const override {
        std::vector<VectorSearchResult> results;
        for (const auto& [pk, _] : vectors_) {
            results.emplace_back(pk, 0.0f);
        }
        return results;
    }

    std::string getName() const override { return "mock_vector"; }
    uint32_t getDimension() const override { return 3; }
    std::string getStatistics() const override { return "{}"; }

private:
    std::map<std::string, std::vector<float>> vectors_;
};

// --- IGraphIndex mock ---

class MockGraphIndex : public IGraphIndex {
public:
    bool insertEdge(const GraphEdge& edge) override {
        edges_.push_back(edge);
        return true;
    }

    bool removeEdge(std::string_view from, std::string_view to,
                    std::string_view edge_type = "") override {
        auto it = std::remove_if(edges_.begin(), edges_.end(),
            [&](const GraphEdge& e) {
                return e.from_node == std::string(from) &&
                       e.to_node   == std::string(to)   &&
                       (edge_type.empty() || e.edge_type == std::string(edge_type));
            });
        bool removed = it != edges_.end();
        edges_.erase(it, edges_.end());
        return removed;
    }

    std::vector<GraphEdge> getOutgoingEdges(
        std::string_view node_id, std::string_view edge_type = "") const override {
        std::vector<GraphEdge> result;
        for (const auto& e : edges_) {
            if (e.from_node == std::string(node_id) &&
                (edge_type.empty() || e.edge_type == std::string(edge_type))) {
                result.push_back(e);
            }
        }
        return result;
    }

    std::vector<GraphEdge> getIncomingEdges(
        std::string_view node_id, std::string_view edge_type = "") const override {
        std::vector<GraphEdge> result;
        for (const auto& e : edges_) {
            if (e.to_node == std::string(node_id) &&
                (edge_type.empty() || e.edge_type == std::string(edge_type))) {
                result.push_back(e);
            }
        }
        return result;
    }

    std::vector<std::string> findShortestPath(
        std::string_view from, std::string_view to,
        std::string_view /*edge_type*/ = "",
        uint32_t /*max_depth*/ = 0) const override {
        // Simple mock: direct edge only
        for (const auto& e : edges_) {
            if (e.from_node == std::string(from) &&
                e.to_node   == std::string(to)) {
                return {std::string(from), std::string(to)};
            }
        }
        return {};
    }

    std::string getName() const override { return "mock_graph"; }
    std::string getStatistics() const override { return "{}"; }

private:
    std::vector<GraphEdge> edges_;
};

// =============================================================================
// IStorageEngine Tests
// =============================================================================

TEST(MockStorageEngine, OpenAndClose) {
    MockStorageEngine storage;
    auto res = storage.open("/tmp/test_db");
    EXPECT_TRUE(res.has_value());
    EXPECT_TRUE(storage.opened_);
    storage.close();
    EXPECT_FALSE(storage.opened_);
}

TEST(MockStorageEngine, PutAndGet) {
    MockStorageEngine storage;
    auto put_res = storage.put("key1", "value1");
    EXPECT_TRUE(put_res.has_value());

    auto get_res = storage.get("key1");
    ASSERT_TRUE(get_res.has_value());
    EXPECT_EQ(get_res.value(), "value1");
}

TEST(MockStorageEngine, GetNonexistentReturnsError) {
    MockStorageEngine storage;
    auto res = storage.get("no_such_key");
    EXPECT_FALSE(res.has_value());
}

TEST(MockStorageEngine, DeleteExistingKey) {
    MockStorageEngine storage;
    storage.put("k", "v");
    auto del_res = storage.del("k");
    EXPECT_TRUE(del_res.has_value());

    auto get_res = storage.get("k");
    EXPECT_FALSE(get_res.has_value());
}

TEST(MockStorageEngine, DeleteNonexistentReturnsError) {
    MockStorageEngine storage;
    auto res = storage.del("ghost");
    EXPECT_FALSE(res.has_value());
}

TEST(MockStorageEngine, ScanRangeDefaultImplementation) {
    MockStorageEngine storage;
    // Default scanRange returns error (not implemented in base)
    int count = 0;
    auto res = storage.scanRange("a", "z",
        [&](std::string_view, std::string_view) { ++count; return true; });
    EXPECT_FALSE(res.has_value()); // default impl returns error
    EXPECT_EQ(count, 0);
}

TEST(MockStorageEngine, ScanPrefixDefaultImplementation) {
    MockStorageEngine storage;
    int count = 0;
    auto res = storage.scanPrefix("user:",
        [&](std::string_view, std::string_view) { ++count; return true; });
    EXPECT_FALSE(res.has_value()); // default impl returns error
    EXPECT_EQ(count, 0);
}

TEST(MockStorageEngine, PolymorphicUsage) {
    MockStorageEngine mock;
    IStorageEngine* engine = &mock;

    EXPECT_TRUE(engine->put("poly_key", "poly_val").has_value());
    auto res = engine->get("poly_key");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), "poly_val");
}

// =============================================================================
// IQueryEngine Tests
// =============================================================================

TEST(MockQueryEngine, Execute) {
    MockQueryEngine engine;
    auto res = engine.execute("SELECT 1");
    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(res.value().empty());
}

TEST(MockQueryEngine, Validate) {
    MockQueryEngine engine;
    auto res = engine.validate("SELECT 1");
    EXPECT_TRUE(res.has_value());
}

TEST(MockQueryEngine, CreateExpressionEvaluator) {
    MockQueryEngine engine;
    auto res = engine.createExpressionEvaluator();
    ASSERT_TRUE(res.has_value());
    ASSERT_NE(res.value().get(), nullptr);
    EXPECT_EQ(res.value()->get_expression_type(), "MOCK");
    EXPECT_TRUE(res.value()->evaluate("true", nullptr));
}

TEST(MockQueryEngine, ExplainQuery) {
    MockQueryEngine engine;
    auto res = engine.explainQuery("SELECT 1");
    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(res.value().empty());
}

TEST(QueryResult, DefaultState) {
    QueryResult qr;
    EXPECT_FALSE(qr.success);
    EXPECT_FALSE(qr.hasError());
}

TEST(QueryResult, ErrorState) {
    QueryResult qr;
    qr.error_message = "something went wrong";
    EXPECT_TRUE(qr.hasError());
}

// =============================================================================
// IFieldEncryption Tests
// =============================================================================

TEST(MockFieldEncryption, EncryptDecryptRoundTrip) {
    MockFieldEncryption enc;
    std::vector<uint8_t> plaintext = {0x01, 0x02, 0x03};
    auto ciphertext = enc.encrypt_field("secret", plaintext);
    ASSERT_EQ(ciphertext.size(), plaintext.size());
    EXPECT_NE(ciphertext, plaintext);

    auto recovered = enc.decrypt_field("secret", ciphertext);
    EXPECT_EQ(recovered, plaintext);
}

TEST(MockFieldEncryption, ShouldEncryptSelectedFields) {
    MockFieldEncryption enc;
    EXPECT_TRUE(enc.should_encrypt("secret"));
    EXPECT_FALSE(enc.should_encrypt("public_name"));
}

// =============================================================================
// IKeyProvider Tests
// =============================================================================

TEST(MockKeyProvider, GetKeyReturnsBytes) {
    MockKeyProvider provider;
    auto key = provider.get_key("my_key");
    EXPECT_EQ(key.size(), 4u);
    EXPECT_EQ(key[0], 0x00);
}

TEST(MockKeyProvider, RotateKeyReturnsDifferentBytes) {
    MockKeyProvider provider;
    auto original = provider.get_key("my_key");
    auto rotated  = provider.rotate_key("my_key");
    EXPECT_NE(original, rotated);
    EXPECT_EQ(rotated.size(), 4u);
}

// =============================================================================
// ISecondaryIndex Tests
// =============================================================================

TEST(MockSecondaryIndex, InsertAndLookup) {
    MockSecondaryIndex idx;
    EXPECT_TRUE(idx.insert("alice", "pk1"));
    EXPECT_TRUE(idx.insert("alice", "pk2"));

    auto pks = idx.lookup("alice");
    EXPECT_EQ(pks.size(), 2u);
}

TEST(MockSecondaryIndex, LookupMissingValue) {
    MockSecondaryIndex idx;
    auto pks = idx.lookup("ghost");
    EXPECT_TRUE(pks.empty());
}

TEST(MockSecondaryIndex, RemoveEntry) {
    MockSecondaryIndex idx;
    idx.insert("bob", "pk10");
    EXPECT_TRUE(idx.remove("bob", "pk10"));
    EXPECT_TRUE(idx.lookup("bob").empty());
}

TEST(MockSecondaryIndex, RemoveNonexistentReturnsFalse) {
    MockSecondaryIndex idx;
    EXPECT_FALSE(idx.remove("none", "pk0"));
}

TEST(MockSecondaryIndex, RangeScan) {
    MockSecondaryIndex idx;
    idx.insert("b_entry", "pk1");
    idx.insert("c_entry", "pk2");
    idx.insert("z_entry", "pk3");

    auto result = idx.rangeScan("b", "d");
    EXPECT_EQ(result.size(), 2u);
}

TEST(MockSecondaryIndex, Metadata) {
    MockSecondaryIndex idx;
    EXPECT_EQ(idx.getName(), "mock_secondary");
    EXPECT_EQ(idx.getFieldName(), "mock_field");
    EXPECT_FALSE(idx.getStatistics().empty());
}

// =============================================================================
// IVectorIndex Tests
// =============================================================================

TEST(MockVectorIndex, InsertAndSearch) {
    MockVectorIndex idx;
    EXPECT_TRUE(idx.insert("doc1", {1.0f, 0.0f, 0.0f}));
    EXPECT_TRUE(idx.insert("doc2", {0.0f, 1.0f, 0.0f}));

    auto results = idx.search({1.0f, 0.0f, 0.0f}, 1);
    EXPECT_EQ(results.size(), 1u);
}

TEST(MockVectorIndex, RangeSearch) {
    MockVectorIndex idx;
    idx.insert("v1", {1.0f, 0.0f, 0.0f});

    auto results = idx.rangeSearch({1.0f, 0.0f, 0.0f}, 1.0f);
    EXPECT_FALSE(results.empty());
}

TEST(MockVectorIndex, RemoveVector) {
    MockVectorIndex idx;
    idx.insert("doc1", {1.0f, 2.0f, 3.0f});
    EXPECT_TRUE(idx.remove("doc1"));
    EXPECT_FALSE(idx.remove("doc1")); // already removed
}

TEST(MockVectorIndex, Metadata) {
    MockVectorIndex idx;
    EXPECT_EQ(idx.getName(), "mock_vector");
    EXPECT_EQ(idx.getDimension(), 3u);
    EXPECT_FALSE(idx.getStatistics().empty());
}

TEST(VectorSearchResult, Construction) {
    VectorSearchResult r("doc1", 0.5f);
    EXPECT_EQ(r.primary_key, "doc1");
    EXPECT_FLOAT_EQ(r.distance, 0.5f);
}

// =============================================================================
// IGraphIndex Tests
// =============================================================================

TEST(MockGraphIndex, InsertEdge) {
    MockGraphIndex idx;
    EXPECT_TRUE(idx.insertEdge(GraphEdge{"A", "B", "knows", 1.0}));
}

TEST(MockGraphIndex, GetOutgoingEdges) {
    MockGraphIndex idx;
    idx.insertEdge(GraphEdge{"A", "B", "knows"});
    idx.insertEdge(GraphEdge{"A", "C", "likes"});

    auto edges = idx.getOutgoingEdges("A");
    EXPECT_EQ(edges.size(), 2u);
}

TEST(MockGraphIndex, GetOutgoingEdgesFilteredByType) {
    MockGraphIndex idx;
    idx.insertEdge(GraphEdge{"A", "B", "knows"});
    idx.insertEdge(GraphEdge{"A", "C", "likes"});

    auto edges = idx.getOutgoingEdges("A", "knows");
    EXPECT_EQ(edges.size(), 1u);
    EXPECT_EQ(edges[0].to_node, "B");
}

TEST(MockGraphIndex, GetIncomingEdges) {
    MockGraphIndex idx;
    idx.insertEdge(GraphEdge{"A", "B"});
    idx.insertEdge(GraphEdge{"C", "B"});

    auto edges = idx.getIncomingEdges("B");
    EXPECT_EQ(edges.size(), 2u);
}

TEST(MockGraphIndex, RemoveEdge) {
    MockGraphIndex idx;
    idx.insertEdge(GraphEdge{"A", "B", "knows"});
    EXPECT_TRUE(idx.removeEdge("A", "B", "knows"));
    EXPECT_TRUE(idx.getOutgoingEdges("A").empty());
}

TEST(MockGraphIndex, FindShortestPathDirect) {
    MockGraphIndex idx;
    idx.insertEdge(GraphEdge{"X", "Y"});
    auto path = idx.findShortestPath("X", "Y");
    ASSERT_EQ(path.size(), 2u);
    EXPECT_EQ(path[0], "X");
    EXPECT_EQ(path[1], "Y");
}

TEST(MockGraphIndex, FindShortestPathNoPath) {
    MockGraphIndex idx;
    auto path = idx.findShortestPath("X", "Z");
    EXPECT_TRUE(path.empty());
}

TEST(MockGraphIndex, Metadata) {
    MockGraphIndex idx;
    EXPECT_EQ(idx.getName(), "mock_graph");
    EXPECT_FALSE(idx.getStatistics().empty());
}

TEST(GraphEdge, Construction) {
    GraphEdge e{"from", "to", "link", 2.5};
    EXPECT_EQ(e.from_node, "from");
    EXPECT_EQ(e.to_node, "to");
    EXPECT_EQ(e.edge_type, "link");
    EXPECT_DOUBLE_EQ(e.weight, 2.5);
}

TEST(GraphEdge, DefaultWeight) {
    GraphEdge e{"A", "B"};
    EXPECT_DOUBLE_EQ(e.weight, 1.0);
    EXPECT_TRUE(e.edge_type.empty());
}
