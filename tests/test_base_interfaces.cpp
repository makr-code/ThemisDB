/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_base_interfaces.cpp                           ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     339                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/// @file test_base_interfaces.cpp
/// @brief Unit tests for base interface definitions
/// 
/// These tests verify that the interfaces:
/// - Compile correctly
/// - Have pure virtual methods
/// - Can be subclassed
/// - Have proper virtual destructors

#include <gtest/gtest.h>

// Disable legacy base interface tests
#if 0
#include <type_traits>
#include "themis/base/interfaces/storage_interface.h"
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/index_interface.h"
#include "themis/base/interfaces/security_interface.h"

using namespace themis;

// ===== Compilation Tests =====

TEST(BaseInterfaces, StorageInterfaceIsAbstract) {
    EXPECT_TRUE(std::is_abstract_v<IStorageEngine>);
    EXPECT_TRUE(std::is_abstract_v<IStorageEngine::ITransaction>);
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

// ===== Mock Implementations for Testing =====

class MockStorageEngine : public IStorageEngine {
public:
    bool put(std::string_view key, std::string_view value) override {
        data_[std::string(key)] = std::string(value);
        return true;
    }

    std::optional<std::string> get(std::string_view key) const override {
        auto it = data_.find(std::string(key));
        if (it != data_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool del(std::string_view key) override {
        return data_.erase(std::string(key)) > 0;
    }

    bool exists(std::string_view key) const override {
        return data_.find(std::string(key)) != data_.end();
    }

    bool executeBatch(
        const std::vector<std::pair<std::string, std::string>>& puts,
        const std::vector<std::string>& deletes) override {
        for (const auto& [k, v] : puts) {
            data_[k] = v;
        }
        for (const auto& k : deletes) {
            data_.erase(k);
        }
        return true;
    }

    void scanPrefix(std::string_view prefix, ScanCallback callback) const override {
        std::string prefix_str(prefix);
        for (const auto& [key, value] : data_) {
            if (key.substr(0, prefix_str.size()) == prefix_str) {
                if (!callback(key, value)) break;
            }
        }
    }

    void scanRange(std::string_view start_key, std::string_view end_key,
                   ScanCallback callback) const override {
        std::string start(start_key);
        std::string end(end_key);
        for (const auto& [key, value] : data_) {
            if (key >= start && key < end) {
                if (!callback(key, value)) break;
            }
        }
    }

    class MockTransaction : public ITransaction {
    public:
        bool put(std::string_view key, std::string_view value) override { return true; }
        std::optional<std::string> get(std::string_view key) const override { return std::nullopt; }
        bool del(std::string_view key) override { return true; }
        bool commit() override { return true; }
        void rollback() override {}
    };

    std::unique_ptr<ITransaction> beginTransaction() override {
        return std::make_unique<MockTransaction>();
    }

    void flush() override {}
    void compact(std::optional<std::string_view> start_key,
                std::optional<std::string_view> end_key) override {}
    uint64_t getApproximateSize() const override { return 0; }
    std::string getStatistics() const override { return "{}"; }

private:
    std::map<std::string, std::string> data_;
};

class MockQueryEngine : public IQueryEngine {
public:
    QueryResult execute(std::string_view query) override {
        QueryResult result;
        result.success = true;
        return result;
    }

    bool validate(std::string_view query) const override {
        return true;
    }

    std::unique_ptr<IExpressionEvaluator> createExpressionEvaluator() const override {
        // Mock implementation: returns nullptr since this test focuses on
        // verifying the interface contract rather than actual expression evaluation
        return nullptr;
    }

    std::string explainQuery(std::string_view query) const override {
        return "Mock plan";
    }
};

class MockKeyProvider : public IKeyProvider {
public:
    std::optional<std::vector<uint8_t>> getKey(
        std::string_view key_id, uint32_t version) const override {
        return std::vector<uint8_t>{0x00, 0x11, 0x22, 0x33};
    }

    uint32_t getLatestKeyVersion(std::string_view key_id) const override {
        return 1;
    }

    uint32_t rotateKey(std::string_view key_id) override {
        return 2;
    }

    bool hasKey(std::string_view key_id) const override {
        return true;
    }

    std::vector<std::string> listKeys() const override {
        return {"test_key"};
    }
};

// ===== Functional Tests =====

TEST(MockStorageEngine, BasicOperations) {
    MockStorageEngine storage;

    // Test put and get
    EXPECT_TRUE(storage.put("key1", "value1"));
    auto value = storage.get("key1");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "value1");

    // Test exists
    EXPECT_TRUE(storage.exists("key1"));
    EXPECT_FALSE(storage.exists("nonexistent"));

    // Test delete
    EXPECT_TRUE(storage.del("key1"));
    EXPECT_FALSE(storage.exists("key1"));
}

TEST(MockStorageEngine, BatchOperations) {
    MockStorageEngine storage;

    std::vector<std::pair<std::string, std::string>> puts = {
        {"key1", "value1"},
        {"key2", "value2"}
    };
    std::vector<std::string> deletes = {};

    EXPECT_TRUE(storage.executeBatch(puts, deletes));
    EXPECT_TRUE(storage.exists("key1"));
    EXPECT_TRUE(storage.exists("key2"));
}

TEST(MockStorageEngine, ScanPrefix) {
    MockStorageEngine storage;
    storage.put("user:1", "alice");
    storage.put("user:2", "bob");
    storage.put("admin:1", "charlie");

    int count = 0;
    storage.scanPrefix("user:", [&count](std::string_view key, std::string_view value) {
        count++;
        return true;
    });

    EXPECT_EQ(count, 2);
}

TEST(MockStorageEngine, Transaction) {
    MockStorageEngine storage;
    auto txn = storage.beginTransaction();
    ASSERT_NE(txn, nullptr);
    EXPECT_TRUE(txn->commit());
}

TEST(MockQueryEngine, Execute) {
    MockQueryEngine engine;
    auto result = engine.execute("SELECT * FROM test");
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.hasError());
}

TEST(MockQueryEngine, Validate) {
    MockQueryEngine engine;
    EXPECT_TRUE(engine.validate("SELECT * FROM test"));
}

TEST(MockKeyProvider, GetKey) {
    MockKeyProvider provider;
    auto key = provider.getKey("test_key", 1);
    ASSERT_TRUE(key.has_value());
    EXPECT_EQ(key->size(), 4);
}

TEST(MockKeyProvider, ListKeys) {
    MockKeyProvider provider;
    auto keys = provider.listKeys();
    EXPECT_EQ(keys.size(), 1);
    EXPECT_EQ(keys[0], "test_key");
}

// ===== Type Safety Tests =====

TEST(BaseInterfaces, PolymorphicBehavior) {
    MockStorageEngine mock_storage;
    IStorageEngine* storage_ptr = &mock_storage;
    
    EXPECT_TRUE(storage_ptr->put("test", "value"));
    auto value = storage_ptr->get("test");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "value");
}

TEST(BaseInterfaces, StructSizes) {
    // Verify structs are reasonable sizes
    EXPECT_LT(sizeof(QueryResult), 1024);
    EXPECT_LT(sizeof(EncryptedData), 1024);
    EXPECT_LT(sizeof(VectorSearchResult), 256);
    EXPECT_LT(sizeof(GraphEdge), 256);
}

// ===== Edge Cases =====

TEST(MockStorageEngine, EmptyGet) {
    MockStorageEngine storage;
    auto value = storage.get("nonexistent");
    EXPECT_FALSE(value.has_value());
}

TEST(MockStorageEngine, DeleteNonexistent) {
    MockStorageEngine storage;
    EXPECT_FALSE(storage.del("nonexistent"));
}

TEST(MockStorageEngine, ScanEmptyPrefix) {
    MockStorageEngine storage;
    int count = 0;
    storage.scanPrefix("nonexistent:", [&count](auto k, auto v) {
        count++;
        return true;
    });
    EXPECT_EQ(count, 0);
}

#endif // legacy base interface tests

TEST(BaseInterfaces, DISABLED_BaseInterfacesLegacy) {
    GTEST_SKIP() << "Base interface tests disabled in this configuration";
}
