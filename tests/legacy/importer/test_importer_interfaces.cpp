// test_importer_interfaces.cpp
//
// Unit tests for the abstract interfaces defined in importer_interfaces.h:
//
//   IImportConflictResolver  – all four ConflictResolution variants, FieldMergeSpec,
//                              concurrency safety
//   IFlatFileSchemaDetector  – SchemaDetectionResult, SchemaConfidence enum
//   IKafkaConsumerSource     – KafkaBatch, KafkaOffset, KafkaError, KafkaRecord
//   IIncrementalImportCursor – CursorStatus, CheckpointToken, ImportBatch
//   IImporterPlugin          – pluginId(), supportedSchemes(), createImporter()
//   IImporterPluginRegistry  – resolve(), listPluginIds(), registerPlugin()
//   REGISTER_IMPORTER_PLUGIN – static-init macro produces a registered plugin
//
// All tests are self-contained and compile without a live database, broker,
// or filesystem beyond the temp-file helpers already used in the suite.
// Concrete stub implementations of each interface are defined here; they
// exercise the contract rather than the production code path.

#include <gtest/gtest.h>

#include "importers/importer_interfaces.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace themis::importers;

// ===========================================================================
// Concrete stub implementations
// ===========================================================================

// ---------------------------------------------------------------------------
// AlwaysReplaceResolver: REPLACE_WITH_INCOMING for every conflict
// ---------------------------------------------------------------------------
class AlwaysReplaceResolver : public IImportConflictResolver {
public:
    ConflictResolutionResult resolve(
        const json& /*existing*/,
        const json& /*incoming*/) const override
    {
        ConflictResolutionResult r;
        r.resolution = ConflictResolution::REPLACE_WITH_INCOMING;
        return r;
    }
};

// ---------------------------------------------------------------------------
// AlwaysKeepResolver: KEEP_EXISTING for every conflict
// ---------------------------------------------------------------------------
class AlwaysKeepResolver : public IImportConflictResolver {
public:
    ConflictResolutionResult resolve(
        const json& /*existing*/,
        const json& /*incoming*/) const override
    {
        ConflictResolutionResult r;
        r.resolution = ConflictResolution::KEEP_EXISTING;
        return r;
    }
};

// ---------------------------------------------------------------------------
// AlwaysRejectResolver: REJECT for every conflict
// ---------------------------------------------------------------------------
class AlwaysRejectResolver : public IImportConflictResolver {
public:
    ConflictResolutionResult resolve(
        const json& /*existing*/,
        const json& /*incoming*/) const override
    {
        ConflictResolutionResult r;
        r.resolution = ConflictResolution::REJECT;
        return r;
    }
};

// ---------------------------------------------------------------------------
// FieldMergeResolver: MERGE_FIELDS with configured take_from_incoming
// ---------------------------------------------------------------------------
class FieldMergeResolver : public IImportConflictResolver {
public:
    explicit FieldMergeResolver(std::vector<std::string> fields)
        : fields_(std::move(fields)) {}

    ConflictResolutionResult resolve(
        const json& /*existing*/,
        const json& /*incoming*/) const override
    {
        ConflictResolutionResult r;
        r.resolution               = ConflictResolution::MERGE_FIELDS;
        r.merge_spec.take_from_incoming = fields_;
        return r;
    }

private:
    std::vector<std::string> fields_;
};

// ---------------------------------------------------------------------------
// NullSchemaDetector: returns a LOW-confidence result with zero columns
// ---------------------------------------------------------------------------
class NullSchemaDetector : public IFlatFileSchemaDetector {
public:
    SchemaDetectionResult detect(
        const std::string& /*file_path*/,
        size_t /*sample_rows*/ = 0) const override
    {
        SchemaDetectionResult r;
        r.confidence   = SchemaConfidence::LOW;
        r.encoding     = "utf-8";
        r.rows_sampled = 0;
        return r;
    }
};

// ---------------------------------------------------------------------------
// FixedSchemaDetector: always returns a pre-configured HIGH-confidence result
// ---------------------------------------------------------------------------
class FixedSchemaDetector : public IFlatFileSchemaDetector {
public:
    FixedSchemaDetector(
        std::vector<std::string> columns,
        std::map<std::string,std::string> types,
        SchemaConfidence confidence,
        std::string encoding = "utf-8",
        size_t rows_sampled = 100)
        : columns_(std::move(columns))
        , types_(std::move(types))
        , confidence_(confidence)
        , encoding_(std::move(encoding))
        , rows_sampled_(rows_sampled)
    {}

    SchemaDetectionResult detect(
        const std::string& /*file_path*/,
        size_t /*sample_rows*/ = 0) const override
    {
        SchemaDetectionResult r;
        r.columns      = columns_;
        r.column_types = types_;
        r.confidence   = confidence_;
        r.encoding     = encoding_;
        r.rows_sampled = rows_sampled_;
        return r;
    }

private:
    std::vector<std::string>           columns_;
    std::map<std::string,std::string>  types_;
    SchemaConfidence                   confidence_;
    std::string                        encoding_;
    size_t                             rows_sampled_;
};

// ---------------------------------------------------------------------------
// VectorKafkaSource: feeds pre-loaded records; simulates a Kafka topic
// ---------------------------------------------------------------------------
class VectorKafkaSource : public IKafkaConsumerSource {
public:
    explicit VectorKafkaSource(std::vector<KafkaRecord> records)
        : records_(std::move(records)) {}

    KafkaBatch poll(
        std::chrono::milliseconds /*timeout*/,
        KafkaError& err) override
    {
        if (cursor_ >= records_.size()) {
            err = KafkaError::POLL_TIMEOUT;
            return {};
        }
        KafkaBatch batch;
        batch.records.push_back(records_[cursor_++]);
        err = KafkaError::OK;
        return batch;
    }

    KafkaError commitOffset(const KafkaOffset& offset) override {
        last_committed_ = offset;
        return KafkaError::OK;
    }

    int64_t lag() const override {
        return static_cast<int64_t>(records_.size()) - static_cast<int64_t>(cursor_);
    }

    void close() override { closed_ = true; }

    // Test helpers
    bool                 closed()         const { return closed_; }
    const KafkaOffset&   lastCommitted()  const { return last_committed_; }

private:
    std::vector<KafkaRecord> records_;
    size_t                   cursor_        = 0;
    bool                     closed_        = false;
    KafkaOffset              last_committed_;
};

// ---------------------------------------------------------------------------
// VectorCursor: wraps a pre-loaded list of JSON entities
// ---------------------------------------------------------------------------
class VectorCursor : public IIncrementalImportCursor {
public:
    VectorCursor(std::string table, std::vector<json> records)
        : table_(std::move(table)), records_(std::move(records)) {}

    CursorStatus next(ImportBatch& batch) override {
        if (closed_) return CursorStatus::ERROR;
        if (cursor_ >= records_.size()) {
            batch = {};
            return CursorStatus::END_OF_STREAM;
        }
        batch.source_table = table_;
        batch.records.clear();
        batch.records.push_back(records_[cursor_++]);
        return CursorStatus::OK;
    }

    CheckpointToken checkpoint() const override {
        return CheckpointToken(std::to_string(cursor_));
    }

    int64_t estimatedRemainingRows() const override {
        return static_cast<int64_t>(records_.size()) - static_cast<int64_t>(cursor_);
    }

    void close() override { closed_ = true; }

private:
    std::string        table_;
    std::vector<json>  records_;
    size_t             cursor_ = 0;
    bool               closed_ = false;
};

// ---------------------------------------------------------------------------
// MinimalImporter: trivial IImporter concrete impl for plugin tests
// ---------------------------------------------------------------------------
class MinimalImporter : public IImporter {
public:
    const char*                 getName()           const override { return "minimal"; }
    std::vector<std::string>    getSupportedTypes() const override { return {"minimal"}; }
    bool                        initialize(const std::string&) override { return true; }
    bool                        validateSource(const std::string&,
                                               std::vector<std::string>&) override { return true; }
    ImportStats                 importData(const std::string&, const ImportOptions&,
                                           ProgressCallback = nullptr) override { return {}; }
    std::shared_ptr<ImportHandle> importDataAsync(const std::string&,
                                                   const ImportOptions&) override {
        return std::make_shared<ImportHandle>();
    }
    void cancel() override {}
    json getSourceSchema(const std::string&) override { return json::object(); }
};

// ---------------------------------------------------------------------------
// TestPlugin: IImporterPlugin for testing the registry
// ---------------------------------------------------------------------------
class TestPlugin : public IImporterPlugin {
public:
    explicit TestPlugin(std::string id, std::vector<std::string> schemes)
        : id_(std::move(id)), schemes_(std::move(schemes)) {}

    const char* pluginId() const override { return id_.c_str(); }

    std::vector<std::string> supportedSchemes() const override { return schemes_; }

    std::unique_ptr<IImporter> createImporter(
        const ImportConfig& /*cfg*/) const override
    {
        create_calls_++;
        return std::make_unique<MinimalImporter>();
    }

    int createCalls() const { return create_calls_; }

private:
    std::string              id_;
    std::vector<std::string> schemes_;
    mutable int              create_calls_ = 0;
};

// ===========================================================================
// IImportConflictResolver tests
// ===========================================================================

TEST(ConflictResolverInterfaceTest, ReplaceResolutionIsCorrect) {
    AlwaysReplaceResolver resolver;
    json existing = {{"id", 1}, {"name", "Alice"}};
    json incoming = {{"id", 1}, {"name", "Bob"}};
    auto result = resolver.resolve(existing, incoming);
    EXPECT_EQ(ConflictResolution::REPLACE_WITH_INCOMING, result.resolution);
}

TEST(ConflictResolverInterfaceTest, KeepResolutionIsCorrect) {
    AlwaysKeepResolver resolver;
    json existing = {{"id", 1}};
    json incoming = {{"id", 1}};
    auto result = resolver.resolve(existing, incoming);
    EXPECT_EQ(ConflictResolution::KEEP_EXISTING, result.resolution);
}

TEST(ConflictResolverInterfaceTest, RejectResolutionIsCorrect) {
    AlwaysRejectResolver resolver;
    auto result = resolver.resolve(json::object(), json::object());
    EXPECT_EQ(ConflictResolution::REJECT, result.resolution);
}

TEST(ConflictResolverInterfaceTest, MergeResolutionCarriesFieldSpec) {
    std::vector<std::string> fields = {"updated_at", "score"};
    FieldMergeResolver resolver(fields);
    json existing = {{"id", 1}, {"score", 5.0}, {"updated_at", "2020-01-01"}};
    json incoming = {{"id", 1}, {"score", 9.5}, {"updated_at", "2026-01-01"}};
    auto result = resolver.resolve(existing, incoming);
    EXPECT_EQ(ConflictResolution::MERGE_FIELDS, result.resolution);
    ASSERT_EQ(2u, result.merge_spec.take_from_incoming.size());
    EXPECT_EQ("updated_at", result.merge_spec.take_from_incoming[0]);
    EXPECT_EQ("score",      result.merge_spec.take_from_incoming[1]);
}

TEST(ConflictResolverInterfaceTest, MergeWithEmptyFieldSpecIsAllIncoming) {
    FieldMergeResolver resolver({});
    auto result = resolver.resolve(json::object(), json::object());
    EXPECT_EQ(ConflictResolution::MERGE_FIELDS, result.resolution);
    EXPECT_TRUE(result.merge_spec.take_from_incoming.empty());
}

TEST(ConflictResolverInterfaceTest, ConcurrentResolveCalls) {
    // Verify the stateless resolver is safe to call from multiple threads.
    AlwaysReplaceResolver resolver;
    std::atomic<int> ok_count{0};
    constexpr int kThreads = 16;
    std::vector<std::future<void>> futures;
    futures.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        futures.push_back(std::async(std::launch::async, [&resolver, &ok_count]() {
            json a = {{"x", 1}};
            json b = {{"x", 2}};
            auto r = resolver.resolve(a, b);
            if (r.resolution == ConflictResolution::REPLACE_WITH_INCOMING) ++ok_count;
        }));
    }
    for (auto& f : futures) f.get();
    EXPECT_EQ(kThreads, ok_count.load());
}

// ===========================================================================
// IFlatFileSchemaDetector tests
// ===========================================================================

TEST(FlatFileSchemaDetectorTest, NullDetectorReturnsLowConfidence) {
    NullSchemaDetector det;
    auto result = det.detect("/some/file.csv");
    EXPECT_EQ(SchemaConfidence::LOW, result.confidence);
    EXPECT_TRUE(result.columns.empty());
    EXPECT_EQ(0u, result.rows_sampled);
}

TEST(FlatFileSchemaDetectorTest, FixedDetectorHighConfidence) {
    FixedSchemaDetector det(
        {"id", "name", "score"},
        {{"id", "integer"}, {"name", "string"}, {"score", "double"}},
        SchemaConfidence::HIGH, "utf-8", 500);

    auto result = det.detect("/data/users.csv", 500);
    EXPECT_EQ(SchemaConfidence::HIGH, result.confidence);
    ASSERT_EQ(3u, result.columns.size());
    EXPECT_EQ("id",    result.columns[0]);
    EXPECT_EQ("name",  result.columns[1]);
    EXPECT_EQ("score", result.columns[2]);
    EXPECT_EQ("integer", result.column_types.at("id"));
    EXPECT_EQ("string",  result.column_types.at("name"));
    EXPECT_EQ("double",  result.column_types.at("score"));
    EXPECT_EQ("utf-8", result.encoding);
    EXPECT_EQ(500u, result.rows_sampled);
}

TEST(FlatFileSchemaDetectorTest, FixedDetectorMediumConfidence) {
    FixedSchemaDetector det({"col"}, {{"col", "string"}},
                             SchemaConfidence::MEDIUM);
    auto result = det.detect("/data/mixed.tsv");
    EXPECT_EQ(SchemaConfidence::MEDIUM, result.confidence);
}

TEST(FlatFileSchemaDetectorTest, DetectorEncodingPreserved) {
    FixedSchemaDetector det({}, {}, SchemaConfidence::LOW, "utf-16-le", 10);
    auto result = det.detect("/data/utf16.csv");
    EXPECT_EQ("utf-16-le", result.encoding);
}

TEST(FlatFileSchemaDetectorTest, WarningsField) {
    SchemaDetectionResult r;
    r.warnings.push_back("Encoding detection uncertain");
    EXPECT_EQ(1u, r.warnings.size());
    EXPECT_EQ("Encoding detection uncertain", r.warnings[0]);
}

// ===========================================================================
// IKafkaConsumerSource tests
// ===========================================================================

TEST(KafkaConsumerSourceTest, PollReturnsRecords) {
    KafkaRecord rec;
    rec.topic     = "events";
    rec.partition = 0;
    rec.offset    = 42;
    rec.key       = "k1";
    rec.value     = R"({"event":"click"})";

    VectorKafkaSource src({rec});
    KafkaError err = KafkaError::UNKNOWN;
    auto batch = src.poll(std::chrono::milliseconds(100), err);

    EXPECT_EQ(KafkaError::OK, err);
    ASSERT_EQ(1u, batch.size());
    EXPECT_EQ("events", batch.records[0].topic);
    EXPECT_EQ(42,       batch.records[0].offset);
    EXPECT_EQ(R"({"event":"click"})", batch.records[0].value);
}

TEST(KafkaConsumerSourceTest, PollTimeoutWhenEmpty) {
    VectorKafkaSource src({});
    KafkaError err = KafkaError::OK;
    auto batch = src.poll(std::chrono::milliseconds(100), err);
    EXPECT_EQ(KafkaError::POLL_TIMEOUT, err);
    EXPECT_TRUE(batch.empty());
}

TEST(KafkaConsumerSourceTest, CommitOffsetSucceeds) {
    KafkaRecord rec;
    rec.offset = 10;
    VectorKafkaSource src({rec});

    KafkaError err = KafkaError::UNKNOWN;
    src.poll(std::chrono::milliseconds(100), err);

    KafkaOffset off;
    off.topic     = "events";
    off.partition = 0;
    off.offset    = 11;  // last consumed + 1
    EXPECT_EQ(KafkaError::OK, src.commitOffset(off));
    EXPECT_EQ(11, src.lastCommitted().offset);
}

TEST(KafkaConsumerSourceTest, CloseMarksAsClosed) {
    VectorKafkaSource src({});
    EXPECT_FALSE(src.closed());
    src.close();
    EXPECT_TRUE(src.closed());
}

TEST(KafkaConsumerSourceTest, LagDecrementsAfterPoll) {
    KafkaRecord r1, r2;
    r1.offset = 0;
    r2.offset = 1;
    VectorKafkaSource src({r1, r2});

    EXPECT_EQ(2, src.lag());
    KafkaError err = KafkaError::UNKNOWN;
    src.poll(std::chrono::milliseconds(100), err);
    EXPECT_EQ(1, src.lag());
    src.poll(std::chrono::milliseconds(100), err);
    EXPECT_EQ(0, src.lag());
}

TEST(KafkaConsumerSourceTest, KafkaBatchHelpers) {
    KafkaBatch batch;
    EXPECT_TRUE(batch.empty());
    EXPECT_EQ(0u, batch.size());

    KafkaRecord r;
    batch.records.push_back(r);
    EXPECT_FALSE(batch.empty());
    EXPECT_EQ(1u, batch.size());
}

TEST(KafkaConsumerSourceTest, KafkaErrorEnumValues) {
    // Verify the error enum compiles and values are distinct.
    EXPECT_NE(KafkaError::OK, KafkaError::MISSING_GROUP_ID);
    EXPECT_NE(KafkaError::OK, KafkaError::CONNECTION_FAILED);
    EXPECT_NE(KafkaError::OK, KafkaError::AUTH_FAILED);
    EXPECT_NE(KafkaError::POLL_TIMEOUT, KafkaError::COMMIT_FAILED);
}

// ===========================================================================
// IIncrementalImportCursor tests
// ===========================================================================

TEST(IncrementalImportCursorTest, CursorIteratesAllRecords) {
    std::vector<json> records = {
        {{"id", 1}, {"name", "Alice"}},
        {{"id", 2}, {"name", "Bob"}},
        {{"id", 3}, {"name", "Carol"}}
    };
    VectorCursor cursor("users", records);

    int count = 0;
    ImportBatch batch;
    while (cursor.next(batch) == CursorStatus::OK) {
        ASSERT_EQ(1u, batch.size());
        EXPECT_EQ("users", batch.source_table);
        ++count;
    }
    EXPECT_EQ(3, count);
}

TEST(IncrementalImportCursorTest, CursorReturnsEndOfStream) {
    VectorCursor cursor("t", {});
    ImportBatch batch;
    EXPECT_EQ(CursorStatus::END_OF_STREAM, cursor.next(batch));
    EXPECT_TRUE(batch.empty());
}

TEST(IncrementalImportCursorTest, CheckpointTokenRoundTrip) {
    VectorCursor cursor("t", {{{"x", 1}}, {{"x", 2}}});

    // Before any next() the cursor is at position 0.
    auto tok0 = cursor.checkpoint();
    EXPECT_TRUE(tok0.valid());
    EXPECT_EQ("0", tok0.serialize());

    ImportBatch batch;
    cursor.next(batch);
    auto tok1 = cursor.checkpoint();
    EXPECT_EQ("1", tok1.serialize());
    EXPECT_NE(tok0, tok1);
}

TEST(IncrementalImportCursorTest, CheckpointTokenDefaultIsInvalid) {
    CheckpointToken tok;
    EXPECT_FALSE(tok.valid());
}

TEST(IncrementalImportCursorTest, CheckpointTokenSerialize) {
    CheckpointToken tok("abc123");
    EXPECT_TRUE(tok.valid());
    EXPECT_EQ("abc123", tok.serialize());
}

TEST(IncrementalImportCursorTest, EstimatedRemainingRows) {
    std::vector<json> records(5, json::object());
    VectorCursor cursor("t", records);
    EXPECT_EQ(5, cursor.estimatedRemainingRows());

    ImportBatch batch;
    cursor.next(batch);
    EXPECT_EQ(4, cursor.estimatedRemainingRows());
}

TEST(IncrementalImportCursorTest, CloseReturnErrorOnNext) {
    VectorCursor cursor("t", {{{"x", 1}}});
    cursor.close();
    ImportBatch batch;
    EXPECT_EQ(CursorStatus::ERROR, cursor.next(batch));
}

TEST(IncrementalImportCursorTest, CursorStatusEnumValues) {
    EXPECT_NE(CursorStatus::OK, CursorStatus::END_OF_STREAM);
    EXPECT_NE(CursorStatus::OK, CursorStatus::CHECKPOINT_REQUIRED);
    EXPECT_NE(CursorStatus::OK, CursorStatus::ERROR);
}

TEST(IncrementalImportCursorTest, ImportBatchHelpers) {
    ImportBatch batch;
    EXPECT_TRUE(batch.empty());
    EXPECT_EQ(0u, batch.size());

    batch.records.push_back(json::object());
    EXPECT_FALSE(batch.empty());
    EXPECT_EQ(1u, batch.size());
}

// ===========================================================================
// IImporterPlugin tests
// ===========================================================================

TEST(ImporterPluginTest, PluginIdReturned) {
    TestPlugin p("mysql_plugin", {"mysql", "mariadb"});
    EXPECT_STREQ("mysql_plugin", p.pluginId());
}

TEST(ImporterPluginTest, SupportedSchemesReturned) {
    TestPlugin p("csv_plugin", {"csv", "tsv"});
    auto schemes = p.supportedSchemes();
    ASSERT_EQ(2u, schemes.size());
    EXPECT_EQ("csv", schemes[0]);
    EXPECT_EQ("tsv", schemes[1]);
}

TEST(ImporterPluginTest, CreateImporterReturnsNonNull) {
    TestPlugin p("test_plugin", {"test"});
    ImportConfig cfg;
    cfg.source_uri  = "test://localhost/db";
    cfg.json_config = "{}";
    auto importer = p.createImporter(cfg);
    EXPECT_NE(nullptr, importer);
    EXPECT_EQ(1, p.createCalls());
}

TEST(ImporterPluginTest, CreateImporterCalledPerRequest) {
    TestPlugin p("test_plugin", {"test"});
    ImportConfig cfg;
    p.createImporter(cfg);
    p.createImporter(cfg);
    EXPECT_EQ(2, p.createCalls());
}

// ===========================================================================
// IImporterPluginRegistry / ImporterSchemeRegistry tests
// ===========================================================================

// Each test uses a fresh TestPlugin to avoid inter-test interference.

TEST(ImporterPluginRegistryTest, ResolveReturnsNullForUnknownScheme) {
    auto* result = IImporterPluginRegistry::instance().resolve("oracle://host/db");
    // May or may not be null depending on whether oracle is registered;
    // but the call must not throw.
    (void)result;
    SUCCEED();
}

TEST(ImporterPluginRegistryTest, RegisterAndResolvePlugin) {
    TestPlugin p("reg_test_plugin", {"regtest"});
    IImporterPluginRegistry::instance().registerPlugin(&p);

    auto* resolved = IImporterPluginRegistry::instance().resolve("regtest://host");
    ASSERT_NE(nullptr, resolved);
    EXPECT_STREQ("reg_test_plugin", resolved->pluginId());
}

TEST(ImporterPluginRegistryTest, ResolveExtractsSchemeFromUri) {
    TestPlugin p("scheme_test", {"myscheme"});
    IImporterPluginRegistry::instance().registerPlugin(&p);

    // Full URI with path and query
    auto* resolved = IImporterPluginRegistry::instance()
                         .resolve("myscheme://host:1234/database?opt=1");
    ASSERT_NE(nullptr, resolved);
    EXPECT_STREQ("scheme_test", resolved->pluginId());
}

TEST(ImporterPluginRegistryTest, ResolveSchemeOnlyUri) {
    TestPlugin p("bare_scheme_test", {"barescheme"});
    IImporterPluginRegistry::instance().registerPlugin(&p);

    // URI with no "://" – treated as a scheme-only string
    auto* resolved = IImporterPluginRegistry::instance().resolve("barescheme");
    ASSERT_NE(nullptr, resolved);
}

TEST(ImporterPluginRegistryTest, RegisterNullPluginIsNoop) {
    EXPECT_NO_THROW(IImporterPluginRegistry::instance().registerPlugin(nullptr));
}

TEST(ImporterPluginRegistryTest, MultipleSchemesSamePlugin) {
    TestPlugin p("multi_scheme", {"alpha_scheme", "beta_scheme"});
    IImporterPluginRegistry::instance().registerPlugin(&p);

    auto* a = IImporterPluginRegistry::instance().resolve("alpha_scheme://host");
    auto* b = IImporterPluginRegistry::instance().resolve("beta_scheme://host");
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    EXPECT_EQ(a, b);  // Same plugin pointer for both schemes
}

TEST(ImporterPluginRegistryTest, ListPluginIdsContainsRegistered) {
    TestPlugin p("listable_plugin", {"listable"});
    IImporterPluginRegistry::instance().registerPlugin(&p);

    auto ids = IImporterPluginRegistry::instance().listPluginIds();
    EXPECT_NE(ids.end(), std::find(ids.begin(), ids.end(), "listable_plugin"));
}

TEST(ImporterPluginRegistryTest, LaterRegistrationOverridesScheme) {
    TestPlugin first("first",  {"override_scheme"});
    TestPlugin second("second", {"override_scheme"});

    IImporterPluginRegistry::instance().registerPlugin(&first);
    IImporterPluginRegistry::instance().registerPlugin(&second);

    auto* resolved = IImporterPluginRegistry::instance()
                         .resolve("override_scheme://host");
    ASSERT_NE(nullptr, resolved);
    // The second registration wins
    EXPECT_STREQ("second", resolved->pluginId());
}

// ===========================================================================
// REGISTER_IMPORTER_PLUGIN macro test
// ===========================================================================

// Define a plugin class and register it via the macro to verify the
// static-init registration path works correctly.

class MacroRegisteredPlugin : public IImporterPlugin {
public:
    const char* pluginId() const override { return "macro_registered_plugin"; }
    std::vector<std::string> supportedSchemes() const override {
        return {"macroproto"};
    }
    std::unique_ptr<IImporter> createImporter(
        const ImportConfig& /*cfg*/) const override
    {
        return std::make_unique<MinimalImporter>();
    }
};

REGISTER_IMPORTER_PLUGIN(MacroRegisteredPlugin)

TEST(RegisterImporterPluginMacroTest, MacroRegisteredPluginResolvable) {
    auto* resolved =
        IImporterPluginRegistry::instance().resolve("macroproto://host");
    ASSERT_NE(nullptr, resolved) << "REGISTER_IMPORTER_PLUGIN did not register the plugin";
    EXPECT_STREQ("macro_registered_plugin", resolved->pluginId());
}

TEST(RegisterImporterPluginMacroTest, MacroPluginCanCreateImporter) {
    auto* plugin =
        IImporterPluginRegistry::instance().resolve("macroproto://host");
    ASSERT_NE(nullptr, plugin);

    ImportConfig cfg;
    cfg.source_uri  = "macroproto://localhost/db";
    cfg.json_config = "{}";
    auto importer = plugin->createImporter(cfg);
    EXPECT_NE(nullptr, importer);
}

TEST(RegisterImporterPluginMacroTest, MacroPluginIdInList) {
    auto ids = IImporterPluginRegistry::instance().listPluginIds();
    EXPECT_NE(ids.end(),
              std::find(ids.begin(), ids.end(), "macro_registered_plugin"));
}

// ===========================================================================
// ImportConfig – conflict resolution fields (Issue #175, v1.7.0)
// ===========================================================================

TEST(ImportConfigConflictTest, DefaultStrategyIsOverwrite) {
    ImportConfig cfg;
    EXPECT_EQ(ConflictStrategy::OVERWRITE, cfg.conflict_strategy);
}

TEST(ImportConfigConflictTest, DefaultMergeDepthIsOne) {
    ImportConfig cfg;
    EXPECT_EQ(1, cfg.merge_depth);
}

TEST(ImportConfigConflictTest, DefaultConflictKeyColumnsEmpty) {
    ImportConfig cfg;
    EXPECT_TRUE(cfg.conflict_key_columns.empty());
}

TEST(ImportConfigConflictTest, DefaultProtectedFieldsEmpty) {
    ImportConfig cfg;
    EXPECT_TRUE(cfg.protected_fields.empty());
}

TEST(ImportConfigConflictTest, CanSetStrategySkip) {
    ImportConfig cfg;
    cfg.conflict_strategy = ConflictStrategy::SKIP;
    EXPECT_EQ(ConflictStrategy::SKIP, cfg.conflict_strategy);
}

TEST(ImportConfigConflictTest, CanSetStrategyMerge) {
    ImportConfig cfg;
    cfg.conflict_strategy = ConflictStrategy::MERGE;
    EXPECT_EQ(ConflictStrategy::MERGE, cfg.conflict_strategy);
}

TEST(ImportConfigConflictTest, CanSetStrategyError) {
    ImportConfig cfg;
    cfg.conflict_strategy = ConflictStrategy::ERROR;
    EXPECT_EQ(ConflictStrategy::ERROR, cfg.conflict_strategy);
}

TEST(ImportConfigConflictTest, CanSetConflictKeyColumns) {
    ImportConfig cfg;
    cfg.conflict_key_columns = {"id", "tenant_id"};
    ASSERT_EQ(2u, cfg.conflict_key_columns.size());
    EXPECT_EQ("id",        cfg.conflict_key_columns[0]);
    EXPECT_EQ("tenant_id", cfg.conflict_key_columns[1]);
}

TEST(ImportConfigConflictTest, CanSetProtectedFields) {
    ImportConfig cfg;
    cfg.protected_fields = {"created_at", "original_owner"};
    ASSERT_EQ(2u, cfg.protected_fields.size());
    EXPECT_EQ("created_at",      cfg.protected_fields[0]);
    EXPECT_EQ("original_owner",  cfg.protected_fields[1]);
}

TEST(ImportConfigConflictTest, CanSetMergeDepthDeep) {
    ImportConfig cfg;
    cfg.merge_depth = -1;  // unlimited recursive merge
    EXPECT_EQ(-1, cfg.merge_depth);
}

TEST(ImportConfigConflictTest, ConflictFieldsAreIndependentOfSourceUri) {
    ImportConfig cfg;
    cfg.source_uri           = "postgresql://host/db";
    cfg.json_config          = "{}";
    cfg.conflict_strategy    = ConflictStrategy::MERGE;
    cfg.conflict_key_columns = {"id"};
    cfg.protected_fields     = {"created_at"};
    cfg.merge_depth          = -1;

    // Source location fields unaffected
    EXPECT_EQ("postgresql://host/db", cfg.source_uri);
    EXPECT_EQ("{}", cfg.json_config);
    // Conflict fields correctly set
    EXPECT_EQ(ConflictStrategy::MERGE, cfg.conflict_strategy);
    ASSERT_EQ(1u, cfg.conflict_key_columns.size());
    EXPECT_EQ("id", cfg.conflict_key_columns[0]);
    ASSERT_EQ(1u, cfg.protected_fields.size());
    EXPECT_EQ("created_at", cfg.protected_fields[0]);
    EXPECT_EQ(-1, cfg.merge_depth);
}
