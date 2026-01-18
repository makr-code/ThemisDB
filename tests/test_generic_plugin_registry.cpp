#include <gtest/gtest.h>
#include "plugins/plugin_registry.h"
#include "plugins/plugin_api.h"
#include "storage/blob_storage_backend.h"
#include "importers/importer_interface.h"
#include <memory>

namespace themis {
namespace plugins {

// ============================================================================
// Mock Plugin Implementations for Testing
// ============================================================================

// Mock Blob Storage Backend
class MockBlobStorage : public storage::IBlobStorageBackend {
public:
    storage::BlobRef put(
        const std::string& blob_id,
        const std::vector<uint8_t>& data
    ) override {
        storage::BlobRef ref;
        ref.id = blob_id;
        ref.type = storage::BlobStorageType::CUSTOM;
        ref.uri = "mock://" + blob_id;
        ref.size_bytes = data.size();
        ref.hash_sha256 = "mock_hash";
        return ref;
    }
    
    std::optional<std::vector<uint8_t>> get(
        const storage::BlobRef& ref
    ) override {
        return std::vector<uint8_t>{1, 2, 3, 4};
    }
    
    bool remove(const storage::BlobRef& ref) override {
        return true;
    }
    
    bool exists(const storage::BlobRef& ref) override {
        return true;
    }
    
    std::string name() const override {
        return "mock_blob_storage";
    }
    
    bool isAvailable() const override {
        return true;
    }
};

// Mock Importer
class MockImporter : public importers::IImporter {
public:
    const char* getName() const override {
        return "mock_importer";
    }
    
    std::vector<std::string> getSupportedTypes() const override {
        return {"csv", "json"};
    }
    
    bool initialize(const std::string& config) override {
        return true;
    }
    
    bool validateSource(const std::string& source_path, std::vector<std::string>& errors) override {
        return true;
    }
    
    importers::ImportStats importData(
        const std::string& source_path,
        const importers::ImportOptions& options,
        importers::ProgressCallback progress_callback
    ) override {
        importers::ImportStats stats;
        stats.total_records = 100;
        stats.imported_records = 100;
        return stats;
    }
    
    void cancel() override {}
    
    nlohmann::json getSourceSchema(const std::string& source_path) override {
        return nlohmann::json{{"schema", "mock"}};
    }
};

// Another Mock Blob Storage (for testing multiple plugins of same type)
class AnotherMockBlobStorage : public storage::IBlobStorageBackend {
public:
    storage::BlobRef put(
        const std::string& blob_id,
        const std::vector<uint8_t>& data
    ) override {
        storage::BlobRef ref;
        ref.id = blob_id;
        ref.type = storage::BlobStorageType::S3;
        ref.uri = "s3://mock/" + blob_id;
        ref.size_bytes = data.size();
        return ref;
    }
    
    std::optional<std::vector<uint8_t>> get(const storage::BlobRef& ref) override {
        return std::vector<uint8_t>{5, 6, 7, 8};
    }
    
    bool remove(const storage::BlobRef& ref) override { return true; }
    bool exists(const storage::BlobRef& ref) override { return true; }
    std::string name() const override { return "s3_mock"; }
    bool isAvailable() const override { return true; }
};

// ============================================================================
// Test Fixture
// ============================================================================

class GenericPluginRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        PluginRegistry::clearRegistry();
    }
    
    void TearDown() override {
        PluginRegistry::clearRegistry();
    }
};

// ============================================================================
// Basic Registry Tests
// ============================================================================

TEST_F(GenericPluginRegistryTest, RegisterBlobStoragePlugin) {
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "mock_s3",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    EXPECT_TRUE(PluginRegistry::hasPlugin<storage::IBlobStorageBackend>("mock_s3"));
}

TEST_F(GenericPluginRegistryTest, RegisterMultiplePluginTypes) {
    // Register blob storage plugin
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "mock_s3",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    // Register importer plugin
    PluginRegistry::registerFactory<importers::IImporter>(
        "mock_importer",
        []() { return std::make_unique<MockImporter>(); }
    );
    
    EXPECT_TRUE(PluginRegistry::hasPlugin<storage::IBlobStorageBackend>("mock_s3"));
    EXPECT_TRUE(PluginRegistry::hasPlugin<importers::IImporter>("mock_importer"));
}

TEST_F(GenericPluginRegistryTest, CreatePluginInstance) {
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "mock_s3",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    auto plugin = PluginRegistry::create<storage::IBlobStorageBackend>("mock_s3");
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->name(), "mock_blob_storage");
    EXPECT_TRUE(plugin->isAvailable());
}

TEST_F(GenericPluginRegistryTest, PluginNotFoundThrows) {
    EXPECT_THROW(
        PluginRegistry::create<storage::IBlobStorageBackend>("non_existent"),
        std::runtime_error
    );
}

TEST_F(GenericPluginRegistryTest, TypeMismatchThrows) {
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "mock_s3",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    // Try to access as wrong type
    EXPECT_THROW(
        PluginRegistry::create<importers::IImporter>("mock_s3"),
        std::runtime_error
    );
}

TEST_F(GenericPluginRegistryTest, ListPluginsByType) {
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "s3_plugin",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "azure_plugin",
        []() { return std::make_unique<AnotherMockBlobStorage>(); }
    );
    
    auto names = PluginRegistry::listPlugins<storage::IBlobStorageBackend>();
    EXPECT_EQ(names.size(), 2);
    
    // Check both names are present
    bool has_s3 = false;
    bool has_azure = false;
    for (const auto& name : names) {
        if (name == "s3_plugin") has_s3 = true;
        if (name == "azure_plugin") has_azure = true;
    }
    EXPECT_TRUE(has_s3);
    EXPECT_TRUE(has_azure);
}

TEST_F(GenericPluginRegistryTest, ListPluginsReturnsEmptyForNoRegistrations) {
    auto names = PluginRegistry::listPlugins<storage::IBlobStorageBackend>();
    EXPECT_TRUE(names.empty());
}

// ============================================================================
// Plugin Functionality Tests
// ============================================================================

TEST_F(GenericPluginRegistryTest, BlobStoragePluginFunctionality) {
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "mock_s3",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    auto plugin = PluginRegistry::create<storage::IBlobStorageBackend>("mock_s3");
    ASSERT_NE(plugin, nullptr);
    
    // Test put operation
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    auto ref = plugin->put("test_blob", data);
    EXPECT_EQ(ref.id, "test_blob");
    EXPECT_EQ(ref.size_bytes, 5);
    
    // Test get operation
    auto retrieved = plugin->get(ref);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->size(), 4);  // MockBlobStorage returns {1,2,3,4}
}

TEST_F(GenericPluginRegistryTest, ImporterPluginFunctionality) {
    PluginRegistry::registerFactory<importers::IImporter>(
        "csv_importer",
        []() { return std::make_unique<MockImporter>(); }
    );
    
    auto plugin = PluginRegistry::create<importers::IImporter>("csv_importer");
    ASSERT_NE(plugin, nullptr);
    
    EXPECT_STREQ(plugin->getName(), "mock_importer");
    
    auto types = plugin->getSupportedTypes();
    EXPECT_EQ(types.size(), 2);
    EXPECT_EQ(types[0], "csv");
    EXPECT_EQ(types[1], "json");
    
    EXPECT_TRUE(plugin->initialize("{}"));
}

// ============================================================================
// PluginAPI Tests
// ============================================================================

TEST_F(GenericPluginRegistryTest, PluginAPIGet) {
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "s3_plugin",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    auto plugin = PluginAPI::get<storage::IBlobStorageBackend>("s3_plugin");
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->name(), "mock_blob_storage");
}

TEST_F(GenericPluginRegistryTest, PluginAPIGetReturnsNullptrForNonExistent) {
    auto plugin = PluginAPI::get<storage::IBlobStorageBackend>("non_existent");
    EXPECT_EQ(plugin, nullptr);
}

TEST_F(GenericPluginRegistryTest, PluginAPIHas) {
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "s3_plugin",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    EXPECT_TRUE(PluginAPI::has<storage::IBlobStorageBackend>("s3_plugin"));
    EXPECT_FALSE(PluginAPI::has<storage::IBlobStorageBackend>("non_existent"));
}

TEST_F(GenericPluginRegistryTest, PluginAPIGetAll) {
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "s3_plugin",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "azure_plugin",
        []() { return std::make_unique<AnotherMockBlobStorage>(); }
    );
    
    auto plugins = PluginAPI::getAll<storage::IBlobStorageBackend>();
    EXPECT_EQ(plugins.size(), 2);
    
    // Verify we got different plugins
    EXPECT_NE(plugins[0]->name(), plugins[1]->name());
}

TEST_F(GenericPluginRegistryTest, PluginAPIGetWithFallback) {
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "s3_plugin",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    auto plugin = PluginAPI::getWithFallback<storage::IBlobStorageBackend>();
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->name(), "mock_blob_storage");
}

TEST_F(GenericPluginRegistryTest, PluginAPIGetWithFallbackReturnsNullptrWhenEmpty) {
    auto plugin = PluginAPI::getWithFallback<storage::IBlobStorageBackend>();
    EXPECT_EQ(plugin, nullptr);
}

// ============================================================================
// Auto-Register Tests
// ============================================================================

TEST_F(GenericPluginRegistryTest, AutoRegisterPlugin) {
    // Use PluginAutoRegister to automatically register
    PluginAutoRegister<storage::IBlobStorageBackend> registrar(
        "auto_plugin",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    EXPECT_TRUE(PluginRegistry::hasPlugin<storage::IBlobStorageBackend>("auto_plugin"));
    
    auto plugin = PluginRegistry::create<storage::IBlobStorageBackend>("auto_plugin");
    ASSERT_NE(plugin, nullptr);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(GenericPluginRegistryTest, MultipleRegistrationsSameNameLastWins) {
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "plugin",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "plugin",
        []() { return std::make_unique<AnotherMockBlobStorage>(); }
    );
    
    auto plugin = PluginRegistry::create<storage::IBlobStorageBackend>("plugin");
    ASSERT_NE(plugin, nullptr);
    // Last registration wins - should be AnotherMockBlobStorage
    EXPECT_EQ(plugin->name(), "s3_mock");
}

TEST_F(GenericPluginRegistryTest, ClearRegistryRemovesAllPlugins) {
    PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
        "plugin1",
        []() { return std::make_unique<MockBlobStorage>(); }
    );
    
    PluginRegistry::registerFactory<importers::IImporter>(
        "plugin2",
        []() { return std::make_unique<MockImporter>(); }
    );
    
    EXPECT_TRUE(PluginRegistry::hasPlugin<storage::IBlobStorageBackend>("plugin1"));
    EXPECT_TRUE(PluginRegistry::hasPlugin<importers::IImporter>("plugin2"));
    
    PluginRegistry::clearRegistry();
    
    EXPECT_FALSE(PluginRegistry::hasPlugin<storage::IBlobStorageBackend>("plugin1"));
    EXPECT_FALSE(PluginRegistry::hasPlugin<importers::IImporter>("plugin2"));
}

} // namespace plugins
} // namespace themis

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
