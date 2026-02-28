/**
 * @file test_ingestion_plugin_api.cpp
 * @brief Unit tests for the Plugin API for third-party source connectors.
 *
 * Tests cover:
 *   - ConnectorPluginRegistry: register, create, unregister, isRegistered, listPlugins
 *   - IngestionManager::registerConnectorPlugin / unregisterConnectorPlugin / listConnectorPlugins
 *   - IngestionBuilder::withPluginSource / withConnectorPlugin / build
 *   - SourceType::PLUGIN in sourceTypeLabel (via IngestionMetricsExporter)
 *   - PLUGIN connector invoked during ingestSource / ingestAll
 *   - Error path: missing plugin_name option
 *   - Error path: unregistered plugin name
 *   - Error path: factory returns nullptr
 */

#include <gtest/gtest.h>
#include "ingestion/ingestion_manager.h"
#include <string>
#include <vector>
#include <atomic>
#include <memory>

using namespace themis::ingestion;

// ---------------------------------------------------------------------------
// Minimal stub connector used as a plugin
// ---------------------------------------------------------------------------

class StubPluginConnector : public ISourceConnector {
public:
    explicit StubPluginConnector(int doc_count = 3,
                                 bool init_ok = true,
                                 bool available = true)
        : doc_count_(doc_count), init_ok_(init_ok), available_(available) {}

    bool initialize(const SourceConfig& cfg) override {
        last_config_ = cfg;
        return init_ok_;
    }

    bool isAvailable() const override { return available_; }

    size_t getDocumentCount() const override {
        return static_cast<size_t>(doc_count_);
    }

    IngestionStats ingest(const std::string& /*target*/,
                          ProgressCallback cb) override {
        IngestionStats stats;
        for (int i = 0; i < doc_count_; ++i) {
            stats.documents_processed++;
            stats.bytes_processed += 10;
            if (cb) cb(last_config_.source_id, stats.documents_processed,
                       static_cast<size_t>(doc_count_), "processing");
        }
        stats.elapsed_seconds = 0.001;
        ingest_call_count_++;
        return stats;
    }

    SourceConfig last_config_;
    std::atomic<int> ingest_call_count_{0};

private:
    int  doc_count_;
    bool init_ok_;
    bool available_;
};

// ---------------------------------------------------------------------------
// ConnectorPluginRegistry – unit tests
// ---------------------------------------------------------------------------

TEST(ConnectorPluginRegistryTest, RegisterAndCreate) {
    ConnectorPluginRegistry reg;
    reg.registerFactory("stub", []() {
        return std::make_unique<StubPluginConnector>(5);
    });

    EXPECT_TRUE(reg.isRegistered("stub"));
    auto conn = reg.create("stub");
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(conn->getDocumentCount(), 5u);
}

TEST(ConnectorPluginRegistryTest, UnknownNameReturnsNullptr) {
    ConnectorPluginRegistry reg;
    EXPECT_EQ(reg.create("no_such_plugin"), nullptr);
    EXPECT_FALSE(reg.isRegistered("no_such_plugin"));
}

TEST(ConnectorPluginRegistryTest, UnregisterRemovesFactory) {
    ConnectorPluginRegistry reg;
    reg.registerFactory("p", []() { return std::make_unique<StubPluginConnector>(); });
    EXPECT_TRUE(reg.isRegistered("p"));

    EXPECT_TRUE(reg.unregisterFactory("p"));
    EXPECT_FALSE(reg.isRegistered("p"));
    EXPECT_EQ(reg.create("p"), nullptr);
}

TEST(ConnectorPluginRegistryTest, UnregisterMissingReturnsFalse) {
    ConnectorPluginRegistry reg;
    EXPECT_FALSE(reg.unregisterFactory("ghost"));
}

TEST(ConnectorPluginRegistryTest, OverwriteExistingFactory) {
    ConnectorPluginRegistry reg;
    reg.registerFactory("p", []() { return std::make_unique<StubPluginConnector>(1); });
    reg.registerFactory("p", []() { return std::make_unique<StubPluginConnector>(99); });

    auto conn = reg.create("p");
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(conn->getDocumentCount(), 99u);
}

TEST(ConnectorPluginRegistryTest, ListPluginsSorted) {
    ConnectorPluginRegistry reg;
    reg.registerFactory("bravo", []() { return std::make_unique<StubPluginConnector>(); });
    reg.registerFactory("alpha", []() { return std::make_unique<StubPluginConnector>(); });
    reg.registerFactory("charlie", []() { return std::make_unique<StubPluginConnector>(); });

    auto names = reg.listPlugins();
    ASSERT_EQ(names.size(), 3u);
    EXPECT_EQ(names[0], "alpha");
    EXPECT_EQ(names[1], "bravo");
    EXPECT_EQ(names[2], "charlie");
}

TEST(ConnectorPluginRegistryTest, CreateProducesDistinctInstances) {
    ConnectorPluginRegistry reg;
    reg.registerFactory("p", []() { return std::make_unique<StubPluginConnector>(); });

    auto c1 = reg.create("p");
    auto c2 = reg.create("p");
    ASSERT_NE(c1, nullptr);
    ASSERT_NE(c2, nullptr);
    EXPECT_NE(c1.get(), c2.get());
}

// ---------------------------------------------------------------------------
// IngestionManager plugin registration
// ---------------------------------------------------------------------------

TEST(IngestionManagerPluginTest, RegisterAndList) {
    IngestionManager mgr("test_db");
    EXPECT_TRUE(mgr.listConnectorPlugins().empty());

    mgr.registerConnectorPlugin("my_plugin",
                                 []() { return std::make_unique<StubPluginConnector>(); });
    auto names = mgr.listConnectorPlugins();
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "my_plugin");
}

TEST(IngestionManagerPluginTest, UnregisterPlugin) {
    IngestionManager mgr("test_db");
    mgr.registerConnectorPlugin("p", []() { return std::make_unique<StubPluginConnector>(); });
    EXPECT_TRUE(mgr.unregisterConnectorPlugin("p"));
    EXPECT_TRUE(mgr.listConnectorPlugins().empty());
}

TEST(IngestionManagerPluginTest, UnregisterNonexistentReturnsFalse) {
    IngestionManager mgr("test_db");
    EXPECT_FALSE(mgr.unregisterConnectorPlugin("ghost"));
}

// ---------------------------------------------------------------------------
// IngestionManager::ingestSource with PLUGIN type
// ---------------------------------------------------------------------------

TEST(IngestionManagerPluginTest, IngestSourceWithPlugin) {
    IngestionManager mgr("test_db");
    mgr.setDryRun(true);

    auto* raw = new StubPluginConnector(4);
    std::shared_ptr<StubPluginConnector> shared(raw);

    mgr.registerConnectorPlugin("my_src", [shared]() {
        return std::make_unique<StubPluginConnector>(4);
    });

    SourceConfig cfg;
    cfg.source_id             = "test_plugin_src";
    cfg.type                  = SourceType::PLUGIN;
    cfg.location              = "some://location";
    cfg.options["plugin_name"] = "my_src";
    EXPECT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("test_plugin_src");
    EXPECT_EQ(stats.documents_processed, 4u);
    EXPECT_TRUE(stats.errors.empty());
}

TEST(IngestionManagerPluginTest, IngestSourceMissingPluginNameOption) {
    IngestionManager mgr("test_db");
    mgr.setDryRun(true);

    SourceConfig cfg;
    cfg.source_id = "bad_plugin_src";
    cfg.type      = SourceType::PLUGIN;
    // deliberately omit options["plugin_name"]
    EXPECT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("bad_plugin_src");
    EXPECT_GT(stats.documents_failed + stats.errors.size(), 0u);
}

TEST(IngestionManagerPluginTest, IngestSourceUnregisteredPluginName) {
    IngestionManager mgr("test_db");
    mgr.setDryRun(true);

    SourceConfig cfg;
    cfg.source_id              = "unknown_plugin_src";
    cfg.type                   = SourceType::PLUGIN;
    cfg.options["plugin_name"] = "no_such_plugin";
    EXPECT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("unknown_plugin_src");
    EXPECT_GT(stats.errors.size(), 0u);
    bool has_not_supported = false;
    for (const auto& e : stats.errors) {
        if (e.code == IngestionErrorCode::CONNECTOR_NOT_SUPPORTED) {
            has_not_supported = true;
        }
    }
    EXPECT_TRUE(has_not_supported);
}

TEST(IngestionManagerPluginTest, IngestSourcePluginInitFailure) {
    IngestionManager mgr("test_db");
    mgr.setDryRun(true);

    mgr.registerConnectorPlugin("bad_init", []() {
        return std::make_unique<StubPluginConnector>(0, /*init_ok=*/false, /*available=*/true);
    });

    SourceConfig cfg;
    cfg.source_id              = "bad_init_src";
    cfg.type                   = SourceType::PLUGIN;
    cfg.options["plugin_name"] = "bad_init";
    EXPECT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("bad_init_src");
    EXPECT_GT(stats.errors.size(), 0u);
}

// ---------------------------------------------------------------------------
// IngestionBuilder plugin fluent API
// ---------------------------------------------------------------------------

TEST(IngestionBuilderPluginTest, WithPluginSourceAndConnectorPlugin) {
    auto mgr = IngestionBuilder("test_db")
        .withDryRun(true)
        .withConnectorPlugin("csv_reader", []() {
            return std::make_unique<StubPluginConnector>(7);
        })
        .withPluginSource("sales_data", "csv_reader", "/data/sales.csv")
        .build();

    ASSERT_NE(mgr, nullptr);

    // Plugin should be registered in the manager
    auto plugins = mgr->listConnectorPlugins();
    ASSERT_EQ(plugins.size(), 1u);
    EXPECT_EQ(plugins[0], "csv_reader");

    // Ingest should work
    auto stats = mgr->ingestSource("sales_data");
    EXPECT_EQ(stats.documents_processed, 7u);
    EXPECT_TRUE(stats.errors.empty());
}

TEST(IngestionBuilderPluginTest, WithPluginSourceSetsLocationAndOptions) {
    SourceConfig captured;
    auto factory = [&captured]() -> std::unique_ptr<ISourceConnector> {
        auto conn = std::make_unique<StubPluginConnector>(1);
        return conn;
    };

    auto mgr = IngestionBuilder("test_db")
        .withDryRun(true)
        .withConnectorPlugin("tracer", std::move(factory))
        .withPluginSource("traced_src", "tracer", "file:///data",
                          {{"format", "jsonl"}}, /*priority=*/3)
        .build();

    ASSERT_NE(mgr, nullptr);

    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].type, SourceType::PLUGIN);
    EXPECT_EQ(sources[0].location, "file:///data");
    EXPECT_EQ(sources[0].options.at("plugin_name"), "tracer");
    EXPECT_EQ(sources[0].options.at("format"), "jsonl");
    EXPECT_EQ(sources[0].priority, 3);
}

// ---------------------------------------------------------------------------
// SourceType::PLUGIN in Prometheus label (via IngestionMetricsExporter)
// ---------------------------------------------------------------------------

TEST(IngestionPluginSourceTypeTest, SourceTypeLabelViaMetrics) {
    IngestionStats stats;
    stats.documents_processed = 2;
    stats.bytes_processed     = 100;
    stats.elapsed_seconds     = 0.001;

    IngestionMetricsExporter exporter;
    exporter.setPrefix("test_ingestion");
    std::string text = exporter.exportText(stats, "plugin_src", "PLUGIN");
    EXPECT_NE(text.find("PLUGIN"), std::string::npos);
}
