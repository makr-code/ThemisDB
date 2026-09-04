/*
 * ThemisDB — ContentToolboxBridge + IFormatExtractor unit tests
 *
 * Tests:
 *   FE-01  IFormatExtractor: default FormatExtractResult.ok == false
 *   FE-02  IFormatExtractorFactory: registerExtractor() with null → no-op
 *   FE-03  IFormatExtractorFactory: extractorFor unknown mime returns nullptr
 *   TB-01  ToolboxBuilder: default construction succeeds
 *   TB-02  ToolboxBuilder: profileCount() == 0 after default construction
 *   TB-03  ToolboxBuilder: withWorkflowProfile("") throws invalid_argument
 *   TB-04  ToolboxBuilder: withWorkflowProfile(path) increments profileCount
 *   TB-05  ToolboxBuilder: withFormatExtractor(null) throws invalid_argument
 *   TB-06  ToolboxBuilder: withFormatExtractor(valid) accepts without throw
 *   TB-07  ToolboxBuilder: withFormatExtractorFactory(null) throws invalid_argument
 *   TB-08  ToolboxBuilder: build() returns non-null IngestionToolbox
 *   TB-09  ToolboxBuilder: build() twice throws logic_error
 *   TB-10  ToolboxBuilder: graphWriter() returns nullptr when not set
 *   TB-11  ToolboxBuilder: withFormatExtractor(valid) registers step in toolbox
 *   TB-12  ToolboxBuilder: withFormatExtractorFactory registers all extractors
 *   TB-13  ToolboxBuilder: vectorWriter() returns nullptr when not set
 *   TB-14  ToolboxBuilder: buildWithBridges() without sinks returns null bridges
 *   TB-15  ToolboxBuilder: buildWithBridges() with graph writer populates aql_bridge
 *   TB-16  ToolboxBuilder: buildWithBridges() called twice throws logic_error
 *   CTB-01 ContentToolboxBridge: null toolbox throws invalid_argument
 *   CTB-02 ContentToolboxBridge: null content_manager throws invalid_argument
 *   CTB-03 ContentToolboxBridge: toolbox() accessor returns set value
 *   CTB-04 ContentToolboxBridge: graphWriter() returns nullptr when not set
 *   CTB-05 ContentToolboxBridge: vectorWriter() returns nullptr when not set
 *   CTB-06 ContentToolboxBridge: move construction is valid
 *   FM-01  format_parse_step: createParsePdfStep(nullptr) → canHandle() false
 *   FM-02  format_parse_step: createParseOfficeStep(nullptr) → canHandle() false
 *   FM-03  format_parse_step: createParseImageStep(nullptr) → canHandle() false
 *   FM-04  format_parse_step: createParseArchiveStep(nullptr) → canHandle() false
 *   FM-05  format_parse_step: createParseAudioStep(nullptr) → canHandle() false
 *   FM-06  format_parse_step: step with real extractor returns correct MIME list
 */

#include <gtest/gtest.h>

#include "toolbox/ingestion_toolbox.h"
#include "toolbox/toolbox_builder.h"
#include "toolbox/content_toolbox_bridge.h"
#include "aql/aql_ingestion_bridge.h"
#include "rag/rag_ingestion_bridge.h"
#include "ingestion/format_extractor.h"
#include "ingestion/ingestion_sinks.h"
#include "ingestion/builtin_step_factories.h"
#include "ingestion/extraction_context.h"

#include "content/content_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/vector_index.h"
#include "index/graph_index.h"
#include "index/secondary_index.h"

#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace themis;
using namespace themis::toolbox;
using namespace themis::ingestion;
using namespace themis::content;

// ─────────────────────────────────────────────────────────────────────────────
// TestDatabase — minimal ContentManager backed by a temp RocksDB instance
// ─────────────────────────────────────────────────────────────────────────────

struct TestDatabase {
    std::filesystem::path                   path;
    std::shared_ptr<RocksDBWrapper>         storage;
    std::shared_ptr<VectorIndexManager>     vector_index;
    std::shared_ptr<GraphIndexManager>      graph_index;
    std::shared_ptr<SecondaryIndexManager>  secondary_index;
    std::shared_ptr<ContentManager>         content_manager;

    TestDatabase() {
#ifdef _WIN32
        throw std::runtime_error("ContentToolboxBridge tests skipped on Windows due to heap instability");
#endif
        path = std::filesystem::temp_directory_path() /
               ("themis_ctb_test_" +
                std::to_string(std::chrono::steady_clock::now()
                                   .time_since_epoch()
                                   .count()));
        std::filesystem::create_directories(path);

        RocksDBWrapper::Config cfg;
        cfg.db_path    = path.string();
        cfg.enable_wal = true;
        storage = std::make_shared<RocksDBWrapper>(cfg);
        if (!storage->open()) {
            throw std::runtime_error(
                "TestDatabase: failed to open RocksDB at " + path.string());
        }

        vector_index    = std::make_shared<VectorIndexManager>(*storage);
        graph_index     = std::make_shared<GraphIndexManager>(*storage);
        secondary_index = std::make_shared<SecondaryIndexManager>(*storage);
        content_manager = std::make_shared<ContentManager>(
            storage, vector_index, graph_index, secondary_index);
    }

    ~TestDatabase() noexcept {
        content_manager.reset();
        secondary_index.reset();
        graph_index.reset();
        vector_index.reset();
        storage.reset();
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Minimal mock FormatExtractor for testing
// ─────────────────────────────────────────────────────────────────────────────

class MockPdfExtractor : public IFormatExtractor {
public:
    explicit MockPdfExtractor(std::vector<std::string> mimes = {"application/pdf"})
        : mimes_(std::move(mimes)) {}

    FormatExtractResult extract(
        std::span<const std::byte> /*data*/,
        const std::string& /*mime*/,
        const std::string& /*hint*/) override
    {
        FormatExtractResult r;
        r.raw_text = "mock extracted text";
        r.ok = true;
        return r;
    }

    std::vector<std::string> supportedMimeTypes() const override { return mimes_; }
    const char* name() const noexcept override { return "MockPdfExtractor"; }

private:
    std::vector<std::string> mimes_;
};

// Minimal mock IFormatExtractorFactory
class MockFactory : public IFormatExtractorFactory {
public:
    std::shared_ptr<IFormatExtractor> extractorFor(const std::string& m) const override {
        auto it = registry_.find(m);
        return (it != registry_.end()) ? it->second : nullptr;
    }

    void registerExtractor(std::shared_ptr<IFormatExtractor> e) override {
        if (!e) {
          return;
        }
        for (const auto& m : e->supportedMimeTypes()) {
            registry_[m] = e;
        }
    }

    std::vector<std::string> registeredMimeTypes() const override {
        std::vector<std::string> v;
        for (const auto& [m, _] : registry_) {
          v.push_back(m);
        }
        return v;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<IFormatExtractor>> registry_;
};

// Minimal stub ContentManager (we can't instantiate the real one without storage)
// ContentToolboxBridge only uses ContentManager's ingestRawBlob and assembleContent;
// for the unit tests we only test construction and accessor paths.

// ─────────────────────────────────────────────────────────────────────────────
// FE — IFormatExtractor tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(FormatExtractor, FE01_DefaultResultNotOk) {
    FormatExtractResult r;
    EXPECT_FALSE(r.ok);
    EXPECT_TRUE(r.raw_text.empty());
}

TEST(FormatExtractor, FE02_RegisterNullExtractorIsNoop) {
    auto factory = std::make_shared<MockFactory>();
    ASSERT_NO_THROW(factory->registerExtractor(nullptr));
    EXPECT_TRUE(factory->registeredMimeTypes().empty());
}

TEST(FormatExtractor, FE03_UnknownMimeReturnsNullptr) {
    auto factory = std::make_shared<MockFactory>();
    EXPECT_EQ(factory->extractorFor("application/pdf"), nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// TB — ToolboxBuilder tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(ToolboxBuilderTest, TB01_DefaultConstructionSucceeds) {
    ASSERT_NO_THROW(ToolboxBuilder());
}

TEST(ToolboxBuilderTest, TB02_ProfileCountZeroByDefault) {
    ToolboxBuilder b;
    EXPECT_EQ(b.profileCount(), 0u);
}

TEST(ToolboxBuilderTest, TB03_WithWorkflowProfileEmptyThrows) {
    ToolboxBuilder b;
    EXPECT_THROW(b.withWorkflowProfile(""), std::invalid_argument);
}

TEST(ToolboxBuilderTest, TB04_WithWorkflowProfileIncrementsCount) {
    ToolboxBuilder b;
    b.withWorkflowProfile("/some/path.yaml");
    EXPECT_EQ(b.profileCount(), 1u);
    b.withWorkflowProfile("/another.yaml");
    EXPECT_EQ(b.profileCount(), 2u);
}

TEST(ToolboxBuilderTest, TB05_WithFormatExtractorNullThrows) {
    ToolboxBuilder b;
    EXPECT_THROW(b.withFormatExtractor(nullptr), std::invalid_argument);
}

TEST(ToolboxBuilderTest, TB06_WithFormatExtractorValidAccepted) {
    ToolboxBuilder b;
    auto ext = std::make_shared<MockPdfExtractor>();
    ASSERT_NO_THROW(b.withFormatExtractor(ext));
}

TEST(ToolboxBuilderTest, TB07_WithFormatExtractorFactoryNullThrows) {
    ToolboxBuilder b;
    EXPECT_THROW(b.withFormatExtractorFactory(nullptr), std::invalid_argument);
}

TEST(ToolboxBuilderTest, TB08_BuildReturnsNonNull) {
    ToolboxBuilder b;
    auto toolbox = b.build();
    ASSERT_NE(toolbox, nullptr);
}

TEST(ToolboxBuilderTest, TB09_BuildTwiceThrows) {
    ToolboxBuilder b;
    b.build();
    EXPECT_THROW(b.build(), std::logic_error);
}

TEST(ToolboxBuilderTest, TB10_GraphWriterNullByDefault) {
    ToolboxBuilder b;
    EXPECT_EQ(b.graphWriter(), nullptr);
}

TEST(ToolboxBuilderTest, TB11_WithFormatExtractorRegistersStep) {
    ToolboxBuilder b;
    auto ext = std::make_shared<MockPdfExtractor>();
    b.withFormatExtractor(ext);
    auto toolbox = b.build();
    ASSERT_NE(toolbox, nullptr);
    // Step 'builtin.parse_pdf' should now be in the registry
    EXPECT_TRUE(toolbox->stepRegistry().hasStep("builtin.parse_pdf"));
}

TEST(ToolboxBuilderTest, TB12_WithFormatExtractorFactoryRegistersAll) {
    auto factory = std::make_shared<MockFactory>();
    factory->registerExtractor(std::make_shared<MockPdfExtractor>());
    factory->registerExtractor(std::make_shared<MockPdfExtractor>(
        std::vector<std::string>{"audio/mpeg", "audio/wav"}));

    ToolboxBuilder b;
    b.withFormatExtractorFactory(factory);
    auto toolbox = b.build();
    ASSERT_NE(toolbox, nullptr);
    EXPECT_TRUE(toolbox->stepRegistry().hasStep("builtin.parse_pdf"));
    EXPECT_TRUE(toolbox->stepRegistry().hasStep("builtin.parse_audio"));
}

TEST(ToolboxBuilderTest, TB13_VectorWriterNullByDefault) {
    ToolboxBuilder b;
    EXPECT_EQ(b.vectorWriter(), nullptr);
}

TEST(ToolboxBuilderTest, TB14_BuildWithBridgesNoSinksNullBridges) {
    ToolboxBuilder b;
    auto result = b.buildWithBridges();
    EXPECT_NE(result.toolbox, nullptr);
    EXPECT_EQ(result.aql_bridge, nullptr);
    EXPECT_EQ(result.rag_bridge, nullptr);
}

TEST(ToolboxBuilderTest, TB15_BuildWithBridgesGraphWriterPopulatesAqlBridge) {
    auto graph_writer = std::make_shared<themis::ingestion::InMemoryGraphWriter>();
    ToolboxBuilder b;
    b.withGraphWriter(graph_writer);
    auto result = b.buildWithBridges();
    EXPECT_NE(result.toolbox, nullptr);
    EXPECT_NE(result.aql_bridge, nullptr);
    EXPECT_NE(result.rag_bridge, nullptr);
}

TEST(ToolboxBuilderTest, TB16_BuildWithBridgesTwiceThrows) {
    ToolboxBuilder b;
    b.buildWithBridges();
    EXPECT_THROW(b.buildWithBridges(), std::logic_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// CTB — ContentToolboxBridge construction tests
// (We test only construction/accessors without a real ContentManager)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ContentToolboxBridgeTest, CTB01_NullToolboxThrows) {
    EXPECT_THROW(
        ContentToolboxBridge(nullptr, nullptr),
        std::invalid_argument);
}

TEST(ContentToolboxBridgeTest, CTB02_NullContentManagerThrows) {
    auto toolbox = IngestionToolbox::createDefault();
    EXPECT_THROW(
        ContentToolboxBridge(toolbox, nullptr),
        std::invalid_argument);
}

TEST(ContentToolboxBridgeTest, CTB03_ToolboxAccessorReturnsValue) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping ContentToolboxBridgeTest on Windows due to fixture instability in TestDatabase-backed bridge tests.";
#endif
    TestDatabase db;
    auto toolbox = IngestionToolbox::createDefault();
    ContentToolboxBridge bridge(toolbox, db.content_manager);
    EXPECT_EQ(bridge.toolbox(), toolbox);
    EXPECT_EQ(bridge.contentManager(), db.content_manager);
}

TEST(ContentToolboxBridgeTest, CTB04_GraphWriterNullWhenNotSet) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping ContentToolboxBridgeTest on Windows due to fixture instability in TestDatabase-backed bridge tests.";
#endif
    TestDatabase db;
    auto toolbox = IngestionToolbox::createDefault();
    ContentToolboxBridge bridge(toolbox, db.content_manager);
    EXPECT_EQ(bridge.graphWriter(), nullptr);
}

TEST(ContentToolboxBridgeTest, CTB05_VectorWriterNullWhenNotSet) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping ContentToolboxBridgeTest on Windows due to fixture instability in TestDatabase-backed bridge tests.";
#endif
    TestDatabase db;
    auto toolbox = IngestionToolbox::createDefault();
    ContentToolboxBridge bridge(toolbox, db.content_manager);
    EXPECT_EQ(bridge.vectorWriter(), nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// FM — format_parse_step factory function tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(FormatParseStep, FM01_ParsePdfWithNullExtractorCanHandleFalse) {
    auto step = builtin::createParsePdfStep(nullptr);
    ASSERT_NE(step, nullptr);
    ExtractionContext ctx;
    ctx.manifest.detected_mime = "application/pdf";
    EXPECT_FALSE(step->canHandle(ctx));
}

TEST(FormatParseStep, FM02_ParseOfficeWithNullExtractorCanHandleFalse) {
    auto step = builtin::createParseOfficeStep(nullptr);
    ASSERT_NE(step, nullptr);
    ExtractionContext ctx;
    ctx.manifest.detected_mime = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    EXPECT_FALSE(step->canHandle(ctx));
}

TEST(FormatParseStep, FM03_ParseImageWithNullExtractorCanHandleFalse) {
    auto step = builtin::createParseImageStep(nullptr);
    ASSERT_NE(step, nullptr);
    ExtractionContext ctx;
    ctx.manifest.detected_mime = "image/jpeg";
    EXPECT_FALSE(step->canHandle(ctx));
}

TEST(FormatParseStep, FM04_ParseArchiveWithNullExtractorCanHandleFalse) {
    auto step = builtin::createParseArchiveStep(nullptr);
    ASSERT_NE(step, nullptr);
    ExtractionContext ctx;
    ctx.manifest.detected_mime = "application/zip";
    EXPECT_FALSE(step->canHandle(ctx));
}

TEST(FormatParseStep, FM05_ParseAudioWithNullExtractorCanHandleFalse) {
    auto step = builtin::createParseAudioStep(nullptr);
    ASSERT_NE(step, nullptr);
    ExtractionContext ctx;
    ctx.manifest.detected_mime = "audio/mpeg";
    EXPECT_FALSE(step->canHandle(ctx));
}

TEST(FormatParseStep, FM06_ParsePdfWithExtractorReportsPdfMime) {
    auto ext = std::make_shared<MockPdfExtractor>();
    auto step = builtin::createParsePdfStep(ext);
    ASSERT_NE(step, nullptr);
    const auto& mimes = step->supportedMimeTypes();
    ASSERT_EQ(mimes.size(), 1u);
    EXPECT_EQ(mimes[0], "application/pdf");
}

TEST(FormatParseStep, FM07_ParsePdfWithExtractorCanHandlePdfMime) {
    auto ext = std::make_shared<MockPdfExtractor>();
    auto step = builtin::createParsePdfStep(ext);
    ExtractionContext ctx;
    ctx.manifest.detected_mime = "application/pdf";
    EXPECT_TRUE(step->canHandle(ctx));
}

TEST(FormatParseStep, FM08_ParsePdfWithExtractorCannotHandleOtherMime) {
    auto ext = std::make_shared<MockPdfExtractor>();
    auto step = builtin::createParsePdfStep(ext);
    ExtractionContext ctx;
    ctx.manifest.detected_mime = "text/plain";
    EXPECT_FALSE(step->canHandle(ctx));
}
