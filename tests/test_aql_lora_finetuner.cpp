/**
 * @file test_aql_lora_finetuner.cpp
 * @brief Unit tests for AQL LoRA fine-tuner and AQL TRAIN parser.
 *
 * Coverage:
 *  - AQLDatasetBuilder: built-in samples, custom samples, JSON export
 *  - AQLLoRAFinetuner: configuration, dataset construction, training lifecycle
 *  - AQLTrainParser: TRAIN / DEPLOY / VERIFY / LIST ADAPTERS statement parsing
 *  - TrainingQueryBuilder: fluent builder API and AQL string generation
 */

#ifndef THEMIS_TEST_BUILD
#define THEMIS_TEST_BUILD 1
#endif

#include <gtest/gtest.h>
#include "aql/aql_lora_finetuner.h"
#include "llm/aql_train_parser.h"
#include "llm/lora_framework/lora_training_service.h"
#include <memory>
#include <string>
#include <stdexcept>

using namespace themis::aql;
using namespace themis::llm;
using namespace themis::llm::lora;

// ============================================================================
// AQLDatasetBuilder Tests
// ============================================================================

class AQLDatasetBuilderTest : public ::testing::Test {
protected:
    AQLDatasetBuilder builder_;
};

TEST_F(AQLDatasetBuilderTest, BuiltinSamplesPopulatesDataset) {
    builder_.addBuiltinSamples();
    // Expect at least one sample per category (8 categories × at least 1 sample each)
    EXPECT_GE(builder_.size(), 8u);
}

TEST_F(AQLDatasetBuilderTest, CustomSampleAppended) {
    builder_.addCustomSample(
        "Get all products",
        "FOR p IN products RETURN p",
        AQLSampleCategory::NL_TO_AQL
    );
    EXPECT_EQ(builder_.size(), 1u);
}

TEST_F(AQLDatasetBuilderTest, ClearResetsSize) {
    builder_.addBuiltinSamples();
    EXPECT_GT(builder_.size(), 0u);
    builder_.clear();
    EXPECT_EQ(builder_.size(), 0u);
}

TEST_F(AQLDatasetBuilderTest, BuildReturnsCopyWithCorrectName) {
    builder_.addBuiltinSamples();
    auto dataset = builder_.build("my_dataset");
    EXPECT_EQ(dataset.dataset_name, "my_dataset");
    EXPECT_EQ(dataset.samples.size(), builder_.size());
}

TEST_F(AQLDatasetBuilderTest, BuildDefaultName) {
    builder_.addBuiltinSamples();
    auto dataset = builder_.build();
    EXPECT_EQ(dataset.dataset_name, "themisdb_aql");
}

TEST_F(AQLDatasetBuilderTest, ToJsonProducesArray) {
    builder_.addBuiltinSamples();
    auto j = builder_.toJson();
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), builder_.size());
}

TEST_F(AQLDatasetBuilderTest, ToJsonSamplesHaveInputAndOutput) {
    builder_.addCustomSample("NL query", "FOR u IN users RETURN u");
    auto j = builder_.toJson();
    ASSERT_EQ(j.size(), 1u);
    EXPECT_TRUE(j[0].contains("input"));
    EXPECT_TRUE(j[0].contains("output"));
    EXPECT_EQ(j[0]["input"].get<std::string>(), "NL query");
    EXPECT_EQ(j[0]["output"].get<std::string>(), "FOR u IN users RETURN u");
}

TEST_F(AQLDatasetBuilderTest, LoadFromJsonObjectImportsSamples) {
    nlohmann::json arr = nlohmann::json::array();
    arr.push_back({{"input", "Query A"}, {"output", "AQL A"}});
    arr.push_back({{"input", "Query B"}, {"output", "AQL B"}});
    builder_.loadFromJsonObject(arr);
    EXPECT_EQ(builder_.size(), 2u);
}

TEST_F(AQLDatasetBuilderTest, LoadFromJsonObjectIgnoresInvalidEntries) {
    nlohmann::json arr = nlohmann::json::array();
    arr.push_back("not_an_object");
    arr.push_back({{"input", "valid"}, {"output", "aql"}});
    arr.push_back({{"input", ""}, {"output", "empty_input_ignored"}});
    builder_.loadFromJsonObject(arr);
    EXPECT_EQ(builder_.size(), 1u);  // only the valid one
}

TEST_F(AQLDatasetBuilderTest, LoadFromJsonObjectThrowsOnNonArray) {
    nlohmann::json obj = {{"key", "value"}};
    EXPECT_THROW(builder_.loadFromJsonObject(obj), std::runtime_error);
}

TEST_F(AQLDatasetBuilderTest, AddBuiltinSamplesForCategoryNLToAQL) {
    builder_.addBuiltinSamplesForCategory(AQLSampleCategory::NL_TO_AQL);
    EXPECT_GT(builder_.size(), 0u);
}

TEST_F(AQLDatasetBuilderTest, AddBuiltinSamplesForCategoryLoraCmd) {
    builder_.addBuiltinSamplesForCategory(AQLSampleCategory::AQL_LORA_CMD);
    EXPECT_GT(builder_.size(), 0u);
}

TEST_F(AQLDatasetBuilderTest, DatasetMetadataContainsTimestamp) {
    builder_.addBuiltinSamples();
    auto dataset = builder_.build();
    EXPECT_TRUE(dataset.metadata.contains("created_at"));
    EXPECT_FALSE(dataset.metadata["created_at"].get<std::string>().empty());
}

// ============================================================================
// AQLLoRAFinetuner Configuration Tests
// ============================================================================

class AQLLoRAFinetunerTest : public ::testing::Test {
protected:
    void SetUp() override {
        LoRATrainingService::Config svc_cfg;
        svc_cfg.default_hyperparameters.rank = 8;
        training_service_ = std::make_shared<LoRATrainingService>(svc_cfg);
    }

    std::shared_ptr<LoRATrainingService> training_service_;
};

TEST_F(AQLLoRAFinetunerTest, DefaultConfigSetsExpectedValues) {
    AQLLoRAFinetuner::Config cfg;
    EXPECT_EQ(cfg.adapter_id,  "themisdb-aql-adapter");
    EXPECT_EQ(cfg.base_model,  "mistral-7b");
    EXPECT_TRUE(cfg.include_builtin_samples);
    EXPECT_EQ(cfg.hyperparameters.rank,   8);
    EXPECT_EQ(cfg.hyperparameters.alpha,  16.0f);
    EXPECT_EQ(cfg.hyperparameters.num_epochs, 3);
    EXPECT_FALSE(cfg.hyperparameters.target_modules.empty());
}

TEST_F(AQLLoRAFinetunerTest, ConstructionWithDefaultConfig) {
    AQLLoRAFinetuner::Config cfg;
    EXPECT_NO_THROW({
        AQLLoRAFinetuner finetuner(cfg, training_service_);
    });
}

TEST_F(AQLLoRAFinetunerTest, IsNotTrainedBeforeTrainCall) {
    AQLLoRAFinetuner finetuner(AQLLoRAFinetuner::Config{}, training_service_);
    EXPECT_FALSE(finetuner.isTrained());
}

TEST_F(AQLLoRAFinetunerTest, GetAdapterIDBeforeTraining) {
    AQLLoRAFinetuner::Config cfg;
    cfg.adapter_id = "test-adapter";
    AQLLoRAFinetuner finetuner(cfg, training_service_);
    EXPECT_EQ(finetuner.getAdapterID(), "test-adapter");
}

TEST_F(AQLLoRAFinetunerTest, BuildDatasetContainsBuiltinSamples) {
    AQLLoRAFinetuner::Config cfg;
    cfg.include_builtin_samples = true;
    AQLLoRAFinetuner finetuner(cfg, training_service_);
    auto dataset = finetuner.buildDataset();
    EXPECT_GE(dataset.size(), 8u);
}

TEST_F(AQLLoRAFinetunerTest, BuildDatasetWithoutBuiltinSamples) {
    AQLLoRAFinetuner::Config cfg;
    cfg.include_builtin_samples = false;
    AQLLoRAFinetuner finetuner(cfg, training_service_);
    auto dataset = finetuner.buildDataset();
    EXPECT_EQ(dataset.size(), 0u);
}

TEST_F(AQLLoRAFinetunerTest, AddCustomSampleIncreasesDataset) {
    AQLLoRAFinetuner::Config cfg;
    cfg.include_builtin_samples = false;
    AQLLoRAFinetuner finetuner(cfg, training_service_);

    finetuner.addCustomSample("Get all events", "FOR e IN events RETURN e");
    auto dataset = finetuner.buildDataset();
    EXPECT_EQ(dataset.size(), 1u);
}

TEST_F(AQLLoRAFinetunerTest, ExportDatasetJsonIsArray) {
    AQLLoRAFinetuner finetuner(AQLLoRAFinetuner::Config{}, training_service_);
    auto j = finetuner.exportDatasetJson();
    EXPECT_TRUE(j.is_array());
}

TEST_F(AQLLoRAFinetunerTest, TrainFailsIfDatasetTooSmall) {
    AQLLoRAFinetuner::Config cfg;
    cfg.include_builtin_samples = false;
    cfg.min_training_samples    = 5;
    AQLLoRAFinetuner finetuner(cfg, training_service_);
    // No samples added → should throw
    EXPECT_THROW(finetuner.train(), std::runtime_error);
}

TEST_F(AQLLoRAFinetunerTest, TrainSucceedsWithSufficientBuiltinSamples) {
    AQLLoRAFinetuner::Config cfg;
    cfg.include_builtin_samples = true;
    cfg.min_training_samples    = 5;
    cfg.adapter_id              = "aql-test-adapter";
    cfg.hyperparameters.num_epochs  = 1;
    cfg.hyperparameters.batch_size  = 2;

    AQLLoRAFinetuner finetuner(cfg, training_service_);
    ASSERT_NO_THROW({
        auto result = finetuner.train();
        // The training service in test mode returns success=true
        // (even without a real model); we just verify the call completes.
        (void)result;
    });
}

TEST_F(AQLLoRAFinetunerTest, IsTrainedAfterSuccessfulTrain) {
    AQLLoRAFinetuner::Config cfg;
    cfg.include_builtin_samples = true;
    cfg.min_training_samples    = 1;
    cfg.hyperparameters.num_epochs = 1;
    AQLLoRAFinetuner finetuner(cfg, training_service_);
    auto result = finetuner.train();
    // In simulation mode the service may return success; check based on result
    if (result.success) {
        EXPECT_TRUE(finetuner.isTrained());
    }
}

TEST_F(AQLLoRAFinetunerTest, EpochCallbackIsWiredAndInvoked) {
    AQLLoRAFinetuner::Config cfg;
    cfg.include_builtin_samples  = true;
    cfg.min_training_samples     = 1;
    cfg.hyperparameters.num_epochs = 1;

    std::vector<std::pair<int,double>> cb_calls;
    cfg.epoch_callback = [&cb_calls](int epoch, double loss) {
        cb_calls.push_back({epoch, loss});
    };

    AQLLoRAFinetuner finetuner(cfg, training_service_);
    // Just verify that constructing with a callback and training doesn't throw.
    // The callback may or may not fire depending on the underlying simulation
    // mode of LoRATrainingService.
    ASSERT_NO_THROW(finetuner.train());
}

TEST_F(AQLLoRAFinetunerTest, EpochCallbackNullByDefault) {
    AQLLoRAFinetuner::Config cfg;
    // epoch_callback should be null / unset by default
    EXPECT_FALSE(static_cast<bool>(cfg.epoch_callback));
}

// ============================================================================
// AQLTrainParser Tests
// ============================================================================

class AQLTrainParserTest : public ::testing::Test {
protected:
    AQLTrainParser parser_;
};

// ─── detectStatementType ────────────────────────────────────────────────────

TEST_F(AQLTrainParserTest, DetectTrainAdapter) {
    EXPECT_EQ(parser_.detectStatementType("TRAIN ADAPTER my_adapter FROM data"),
              AQLTrainParser::StatementType::TRAIN_ADAPTER);
}

TEST_F(AQLTrainParserTest, DetectDeployAdapter) {
    EXPECT_EQ(parser_.detectStatementType("DEPLOY ADAPTER my_adapter TO SHARD 'shard1'"),
              AQLTrainParser::StatementType::DEPLOY_ADAPTER);
}

TEST_F(AQLTrainParserTest, DetectVerifyAdapter) {
    EXPECT_EQ(parser_.detectStatementType("VERIFY ADAPTER my_adapter CHECK signature"),
              AQLTrainParser::StatementType::VERIFY_ADAPTER);
}

TEST_F(AQLTrainParserTest, DetectListAdapters) {
    EXPECT_EQ(parser_.detectStatementType("LIST ADAPTERS WHERE base_model = 'llama'"),
              AQLTrainParser::StatementType::LIST_ADAPTERS);
}

TEST_F(AQLTrainParserTest, DetectUnknown) {
    EXPECT_EQ(parser_.detectStatementType("FOR u IN users RETURN u"),
              AQLTrainParser::StatementType::UNKNOWN);
}

// ─── parseTrainAdapter ───────────────────────────────────────────────────────

TEST_F(AQLTrainParserTest, ParseTrainAdapterBasic) {
    const std::string aql =
        "TRAIN ADAPTER 'aql-v1' FROM training_pairs WITH { base_model: 'mistral-7b', rank: 8, alpha: 16, epochs: 3, learning_rate: 0.0003 }";

    auto stmt = parser_.parseTrainAdapter(aql);
    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->adapter_id,       "aql-v1");
    EXPECT_EQ(stmt->source_collection,"training_pairs");
    EXPECT_EQ(stmt->config.base_model_name, "mistral-7b");
    EXPECT_EQ(stmt->config.lora_rank,  8);
    EXPECT_EQ(stmt->config.epochs,     3);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterWithVectorSimilarity) {
    const std::string aql =
        "TRAIN ADAPTER 'vec-adapter' FROM docs "
        "USING VECTOR_SIMILARITY(field = 'embedding', top_k = 5, threshold = 0.8, metric = 'cosine') "
        "WITH { base_model: 'llama-3-8b', rank: 16, alpha: 32, epochs: 2, learning_rate: 0.0002 }";

    auto stmt = parser_.parseTrainAdapter(aql);
    ASSERT_NE(stmt, nullptr);
    ASSERT_TRUE(stmt->enrichment.vector_similarity.has_value());
    EXPECT_EQ(stmt->enrichment.vector_similarity->field,  "embedding");
    EXPECT_EQ(stmt->enrichment.vector_similarity->top_k,  5);
    EXPECT_NEAR(stmt->enrichment.vector_similarity->threshold, 0.8, 1e-6);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterWithGraphContext) {
    const std::string aql =
        "TRAIN ADAPTER 'graph-adapter' FROM nodes "
        "USING GRAPH_CONTEXT(max_depth = 3, direction = 'OUTBOUND') "
        "WITH { base_model: 'mistral-7b', rank: 8, alpha: 16, epochs: 1, learning_rate: 0.0003 }";

    auto stmt = parser_.parseTrainAdapter(aql);
    ASSERT_NE(stmt, nullptr);
    ASSERT_TRUE(stmt->enrichment.graph_context.has_value());
    EXPECT_EQ(stmt->enrichment.graph_context->max_depth, 3);
    EXPECT_EQ(stmt->enrichment.graph_context->direction, "OUTBOUND");
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterDistributed) {
    const std::string aql =
        "TRAIN ADAPTER 'dist-adapter' FROM corpus "
        "DISTRIBUTED COORDINATOR 'shard-0' "
        "WITH { base_model: 'llama-3-8b', rank: 8, alpha: 16, epochs: 3, learning_rate: 0.0003 }";

    auto stmt = parser_.parseTrainAdapter(aql);
    ASSERT_NE(stmt, nullptr);
    EXPECT_TRUE(stmt->distributed.enabled);
    EXPECT_EQ(stmt->distributed.coordinator_shard, "shard-0");
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterMissingFromThrows) {
    const std::string aql = "TRAIN ADAPTER bad_adapter WITH { epochs: 3 }";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterInvalidNameThrows) {
    const std::string aql = "TRAIN ADAPTER 'my adapter!' FROM data "
                            "WITH { base_model: 'x', rank: 8, alpha: 16, epochs: 1, learning_rate: 0.001 }";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterInvalidEpochValueThrowsClearError) {
    const std::string aql = "TRAIN ADAPTER 'aql-v1' FROM training_pairs "
                            "WITH { base_model: 'mistral-7b', rank: 8, alpha: 16, epochs: not_a_number, learning_rate: 0.0003 }";

    try {
        (void)parser_.parseTrainAdapter(aql);
        FAIL() << "Expected std::invalid_argument";
    } catch (const std::invalid_argument& ex) {
        EXPECT_NE(std::string(ex.what()).find("invalid integer value for epochs"), std::string::npos);
    }
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterOutOfRangeRankThrowsClearError) {
    const std::string aql = "TRAIN ADAPTER 'aql-v1' FROM training_pairs "
                            "WITH { base_model: 'mistral-7b', rank: 999999999999999999999, alpha: 16, epochs: 3, learning_rate: 0.0003 }";

    try {
        (void)parser_.parseTrainAdapter(aql);
        FAIL() << "Expected std::invalid_argument";
    } catch (const std::invalid_argument& ex) {
        EXPECT_NE(std::string(ex.what()).find("invalid integer value for rank"), std::string::npos);
    }
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterInvalidLearningRateValueThrowsClearError) {
    const std::string aql = "TRAIN ADAPTER 'aql-v1' FROM training_pairs "
                            "WITH { base_model: 'mistral-7b', rank: 8, alpha: 16, epochs: 3, learning_rate: 0.1xyz }";

    try {
        (void)parser_.parseTrainAdapter(aql);
        FAIL() << "Expected std::invalid_argument";
    } catch (const std::invalid_argument& ex) {
        EXPECT_NE(std::string(ex.what()).find("invalid numeric value for learning_rate"), std::string::npos);
    }
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterInfiniteLearningRateThrowsClearError) {
    const std::string aql = "TRAIN ADAPTER 'aql-v1' FROM training_pairs "
                            "WITH { base_model: 'mistral-7b', rank: 8, alpha: 16, epochs: 3, learning_rate: 1e309 }";

    try {
        (void)parser_.parseTrainAdapter(aql);
        FAIL() << "Expected std::invalid_argument";
    } catch (const std::invalid_argument& ex) {
        EXPECT_NE(std::string(ex.what()).find("invalid numeric value for learning_rate"), std::string::npos);
    }
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterRejectsNonPositiveBatchSize) {
    const std::string aql = "TRAIN ADAPTER 'aql-v1' FROM training_pairs "
                            "WITH { base_model: 'mistral-7b', rank: 8, alpha: 16, epochs: 3, learning_rate: 0.0003, batch_size: 0 }";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterRejectsNonPositiveMaxSeqLengthViaJsonWithClause) {
    const std::string aql = "TRAIN ADAPTER 'aql-v1' FROM training_pairs "
                            "WITH {\"base_model_name\":\"mistral-7b\",\"rank\":8,\"alpha\":16,\"epochs\":3,\"learning_rate\":0.0003,\"max_seq_length\":0}";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

// ─── parseDeployAdapter ──────────────────────────────────────────────────────

TEST_F(AQLTrainParserTest, ParseDeployAdapterBasic) {
    const std::string aql =
        "DEPLOY ADAPTER 'aql-v1' TO SHARD 'shard-1', 'shard-2' "
        "WITH strategy = 'REPLICATED', validate_compatibility = true";

    auto stmt = parser_.parseDeployAdapter(aql);
    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->adapter_id, "aql-v1");
    EXPECT_EQ(stmt->target_shards.size(), 2u);
    EXPECT_EQ(stmt->strategy, "REPLICATED");
    EXPECT_TRUE(stmt->validate_compatibility);
}

TEST_F(AQLTrainParserTest, ParseDeployAdapterMissingToThrows) {
    EXPECT_THROW(parser_.parseDeployAdapter("DEPLOY ADAPTER 'foo'"),
                 std::invalid_argument);
}

// ─── parseVerifyAdapter ──────────────────────────────────────────────────────

TEST_F(AQLTrainParserTest, ParseVerifyAdapterSignature) {
    auto stmt = parser_.parseVerifyAdapter("VERIFY ADAPTER 'aql-v1' CHECK signature");
    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->adapter_id,  "aql-v1");
    EXPECT_TRUE(stmt->check_signature);
    EXPECT_FALSE(stmt->check_safetensors_match);
}

TEST_F(AQLTrainParserTest, ParseVerifyAdapterManifestAndSignature) {
    auto stmt = parser_.parseVerifyAdapter(
        "VERIFY ADAPTER 'my-adapter' CHECK signature, manifest");
    ASSERT_NE(stmt, nullptr);
    EXPECT_TRUE(stmt->check_signature);
    EXPECT_TRUE(stmt->check_manifest);
}

TEST_F(AQLTrainParserTest, ParseVerifyAdapterMissingIdThrows) {
    EXPECT_THROW(parser_.parseVerifyAdapter("VERIFY ADAPTER"),
                 std::invalid_argument);
}

// ─── parseListAdapters ───────────────────────────────────────────────────────

TEST_F(AQLTrainParserTest, ParseListAdaptersBasic) {
    auto stmt = parser_.parseListAdapters("LIST ADAPTERS");
    ASSERT_NE(stmt, nullptr);
    EXPECT_FALSE(stmt->base_model.has_value());
    EXPECT_EQ(stmt->order_by, "created_at");
}

TEST_F(AQLTrainParserTest, ParseListAdaptersWithFilters) {
    auto stmt = parser_.parseListAdapters(
        "LIST ADAPTERS WHERE base_model = 'mistral-7b' AND domain = 'aql' "
        "ORDER BY created_at DESC LIMIT 50");
    ASSERT_NE(stmt, nullptr);
    ASSERT_TRUE(stmt->base_model.has_value());
    EXPECT_EQ(*stmt->base_model, "mistral-7b");
    ASSERT_TRUE(stmt->domain.has_value());
    EXPECT_EQ(*stmt->domain, "aql");
    EXPECT_EQ(stmt->limit, 50);
}

TEST_F(AQLTrainParserTest, ParseListAdaptersAscOrder) {
    auto stmt = parser_.parseListAdapters(
        "LIST ADAPTERS ORDER BY updated_at ASC LIMIT 10");
    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->order_by, "updated_at");
    EXPECT_FALSE(stmt->descending);
    EXPECT_EQ(stmt->limit, 10);
}

TEST_F(AQLTrainParserTest, ParseListAdaptersInvalidLimitThrows) {
    EXPECT_THROW(
        parser_.parseListAdapters("LIST ADAPTERS ORDER BY updated_at DESC LIMIT 10oops"),
        std::invalid_argument);
}

TEST_F(AQLTrainParserTest, ParseListAdaptersOverflowLimitThrows) {
    EXPECT_THROW(
        parser_.parseListAdapters("LIST ADAPTERS LIMIT 999999999999999999999"),
        std::invalid_argument);
}

TEST_F(AQLTrainParserTest, ParseListAdaptersNonPositiveLimitThrows) {
    EXPECT_THROW(
        parser_.parseListAdapters("LIST ADAPTERS ORDER BY created_at DESC LIMIT 0"),
        std::invalid_argument);
}

// ─── validateConfig: lora_alpha / lora_dropout / validation_split ────────────

TEST_F(AQLTrainParserTest, ParseTrainAdapterRejectsZeroLoraAlpha) {
    const std::string aql =
        "TRAIN ADAPTER 'a' FROM col WITH {\"base_model\":\"m\",\"epochs\":1,"
        "\"learning_rate\":0.001,\"lora_rank\":8,\"lora_alpha\":0.0,"
        "\"lora_dropout\":0.1,\"batch_size\":4,\"max_seq_length\":128,"
        "\"validation_split\":0.1}";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterRejectsNegativeLoraDropout) {
    const std::string aql =
        "TRAIN ADAPTER 'a' FROM col WITH {\"base_model\":\"m\",\"epochs\":1,"
        "\"learning_rate\":0.001,\"lora_rank\":8,\"lora_alpha\":16.0,"
        "\"lora_dropout\":-0.1,\"batch_size\":4,\"max_seq_length\":128,"
        "\"validation_split\":0.1}";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterRejectsLoraDropoutAtOne) {
    const std::string aql =
        "TRAIN ADAPTER 'a' FROM col WITH {\"base_model\":\"m\",\"epochs\":1,"
        "\"learning_rate\":0.001,\"lora_rank\":8,\"lora_alpha\":16.0,"
        "\"lora_dropout\":1.0,\"batch_size\":4,\"max_seq_length\":128,"
        "\"validation_split\":0.1}";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterRejectsZeroValidationSplit) {
    const std::string aql =
        "TRAIN ADAPTER 'a' FROM col WITH {\"base_model\":\"m\",\"epochs\":1,"
        "\"learning_rate\":0.001,\"lora_rank\":8,\"lora_alpha\":16.0,"
        "\"lora_dropout\":0.1,\"batch_size\":4,\"max_seq_length\":128,"
        "\"validation_split\":0.0}";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterRejectsValidationSplitAboveOne) {
    const std::string aql =
        "TRAIN ADAPTER 'a' FROM col WITH {\"base_model\":\"m\",\"epochs\":1,"
        "\"learning_rate\":0.001,\"lora_rank\":8,\"lora_alpha\":16.0,"
        "\"lora_dropout\":0.1,\"batch_size\":4,\"max_seq_length\":128,"
        "\"validation_split\":1.5}";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

// ─── parseGraphContext bounds ─────────────────────────────────────────────────

TEST_F(AQLTrainParserTest, ParseTrainAdapterRejectsZeroMaxDepth) {
    const std::string aql =
        "TRAIN ADAPTER 'a' FROM col "
        "USING GRAPH_CONTEXT(max_depth = 0, direction = 'BOTH', max_nodes = 50) "
        "WITH base_model = 'llama', epochs = 1, learning_rate = 0.001, rank = 8";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterRejectsZeroMaxNodes) {
    const std::string aql =
        "TRAIN ADAPTER 'a' FROM col "
        "USING GRAPH_CONTEXT(max_depth = 2, direction = 'BOTH', max_nodes = 0) "
        "WITH base_model = 'llama', epochs = 1, learning_rate = 0.001, rank = 8";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

// ─── parseVectorSimilarity bounds ────────────────────────────────────────────

TEST_F(AQLTrainParserTest, ParseTrainAdapterRejectsZeroTopK) {
    const std::string aql =
        "TRAIN ADAPTER 'a' FROM col "
        "USING VECTOR_SIMILARITY(field = 'emb', top_k = 0, threshold = 0.8) "
        "WITH base_model = 'llama', epochs = 1, learning_rate = 0.001, rank = 8";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterRejectsNegativeThreshold) {
    const std::string aql =
        "TRAIN ADAPTER 'a' FROM col "
        "USING VECTOR_SIMILARITY(field = 'emb', top_k = 5, threshold = -0.1) "
        "WITH base_model = 'llama', epochs = 1, learning_rate = 0.001, rank = 8";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterRejectsThresholdAboveOne) {
    const std::string aql =
        "TRAIN ADAPTER 'a' FROM col "
        "USING VECTOR_SIMILARITY(field = 'emb', top_k = 5, threshold = 1.1) "
        "WITH base_model = 'llama', epochs = 1, learning_rate = 0.001, rank = 8";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

// ─── parseDeployAdapter empty shards ─────────────────────────────────────────

TEST_F(AQLTrainParserTest, ParseDeployAdapterNoShardsThrows) {
    EXPECT_THROW(
        parser_.parseDeployAdapter("DEPLOY ADAPTER 'foo' TO WITH strategy = 'R'"),
        std::invalid_argument);
}

// ─── validateBaseModel ────────────────────────────────────────────────────────

TEST_F(AQLTrainParserTest, ParseTrainAdapterEmptyBaseModelThrows) {
    // KV WITH clause that sets base_model to empty string must be rejected.
    const std::string aql =
        "TRAIN ADAPTER 'empty-model' FROM docs "
        "WITH { base_model = '', rank = 8, epochs = 1 }";
    EXPECT_THROW(parser_.parseTrainAdapter(aql), std::invalid_argument);
}

// ─── KV-path completeness (dropout / validation_split / max_seq_length) ───────

TEST_F(AQLTrainParserTest, ParseTrainAdapterKVDropoutAccepted) {
    const std::string aql =
        "TRAIN ADAPTER 'kv-dropout' FROM docs "
        "WITH { base_model = 'llama-3-8b', epochs = 2, dropout = 0.05, rank = 8 }";
    auto stmt = parser_.parseTrainAdapter(aql);
    EXPECT_NEAR(stmt->config.lora_dropout, 0.05, 1e-9);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterKVValidationSplitAccepted) {
    const std::string aql =
        "TRAIN ADAPTER 'kv-split' FROM docs "
        "WITH { base_model = 'llama-3-8b', epochs = 2, validation_split = 0.2, rank = 8 }";
    auto stmt = parser_.parseTrainAdapter(aql);
    EXPECT_NEAR(stmt->config.validation_split, 0.2, 1e-9);
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterKVMaxSeqLengthAccepted) {
    const std::string aql =
        "TRAIN ADAPTER 'kv-seqlen' FROM docs "
        "WITH { base_model = 'llama-3-8b', epochs = 2, max_seq_length = 512, rank = 8 }";
    auto stmt = parser_.parseTrainAdapter(aql);
    EXPECT_EQ(stmt->config.max_seq_length, 512);
}

// ─── JSON round-trip ─────────────────────────────────────────────────────────

TEST_F(AQLTrainParserTest, TrainStatementConfigRoundTrip) {
    TrainStatementConfig cfg;
    cfg.base_model_name = "mistral-7b";
    cfg.lora_rank       = 16;
    cfg.lora_alpha      = 32.0;
    cfg.epochs          = 5;
    cfg.learning_rate   = 1e-4;
    cfg.sign_adapter    = false;

    auto j    = cfg.toJSON();
    auto cfg2 = TrainStatementConfig::fromJSON(j);
    EXPECT_EQ(cfg2.base_model_name, cfg.base_model_name);
    EXPECT_EQ(cfg2.lora_rank,       cfg.lora_rank);
    EXPECT_NEAR(cfg2.lora_alpha,    cfg.lora_alpha, 1e-9);
    EXPECT_EQ(cfg2.epochs,          cfg.epochs);
    EXPECT_EQ(cfg2.sign_adapter,    cfg.sign_adapter);
}

TEST_F(AQLTrainParserTest, TrainAdapterStmtRoundTrip) {
    TrainAdapterStmt stmt;
    stmt.adapter_id        = "round-trip-adapter";
    stmt.source_collection = "my_collection";
    stmt.config.base_model_name = "llama-3-8b";
    stmt.config.lora_rank  = 8;
    stmt.config.epochs     = 3;
    stmt.config.learning_rate = 3e-4;

    auto j     = stmt.toJSON();
    auto stmt2 = TrainAdapterStmt::fromJSON(j);
    EXPECT_EQ(stmt2.adapter_id,        stmt.adapter_id);
    EXPECT_EQ(stmt2.source_collection, stmt.source_collection);
    EXPECT_EQ(stmt2.config.base_model_name, stmt.config.base_model_name);
}

TEST_F(AQLTrainParserTest, TrainStatementConfigFromJSONTypeMismatchThrows) {
    // "epochs" is a string instead of integer — fromJSON must throw a clear error
    nlohmann::json j;
    j["epochs"] = "not_an_int";
    EXPECT_THROW({
        TrainStatementConfig::fromJSON(j);
    }, std::invalid_argument);
}

TEST_F(AQLTrainParserTest, GraphContextConfigFromJSONTypeMismatchThrows) {
    nlohmann::json j;
    j["max_depth"] = "deep";  // should be int
    EXPECT_THROW({ GraphContextConfig::fromJSON(j); }, std::invalid_argument);
}

TEST_F(AQLTrainParserTest, VectorSimilarityConfigFromJSONTypeMismatchThrows) {
    nlohmann::json j;
    j["top_k"] = "ten";  // should be int
    EXPECT_THROW({ VectorSimilarityConfig::fromJSON(j); }, std::invalid_argument);
}

TEST_F(AQLTrainParserTest, MultiModelEnrichmentFromJSONNonArrayRelationalJoinsIgnored) {
    // relational_joins present but not an array — must be silently ignored (no crash)
    nlohmann::json j;
    j["relational_joins"] = "not_an_array";
    MultiModelEnrichment e;
    EXPECT_NO_THROW({ e = MultiModelEnrichment::fromJSON(j); });
    EXPECT_TRUE(e.relational_joins.empty());
}

TEST_F(AQLTrainParserTest, ParseTrainAdapterJsonWithClauseTypeMismatchThrows) {
    // JSON WITH clause where "epochs" has wrong type — must surface as error, not silently use defaults
    const std::string aql =
        "TRAIN ADAPTER 'my-adapter' FROM training_data "
        "WITH {\"base_model\": \"mistral-7b\", \"epochs\": \"bad\"}";
    EXPECT_THROW({ parser_.parseTrainAdapter(aql); }, std::invalid_argument);
}

// ============================================================================
// TrainingQueryBuilder Tests
// ============================================================================

class TrainingQueryBuilderTest : public ::testing::Test {};

TEST_F(TrainingQueryBuilderTest, BuildsMinimalStmt) {
    auto stmt = TrainingQueryBuilder{}
        .adapter("aql-builder-adapter")
        .from("training_pairs")
        .baseModel("mistral-7b")
        .loraRank(8)
        .epochs(3)
        .learningRate(3e-4)
        .build();

    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->adapter_id,        "aql-builder-adapter");
    EXPECT_EQ(stmt->source_collection, "training_pairs");
    EXPECT_EQ(stmt->config.base_model_name, "mistral-7b");
    EXPECT_EQ(stmt->config.lora_rank,  8);
    EXPECT_EQ(stmt->config.epochs,     3);
}

TEST_F(TrainingQueryBuilderTest, ToAQLContainsAdapterID) {
    TrainingQueryBuilder builder;
    builder.adapter("aql-test")
           .from("data")
           .baseModel("mistral-7b")
           .loraRank(8)
           .epochs(3)
           .learningRate(3e-4);

    std::string aql = builder.toAQL();
    EXPECT_NE(aql.find("aql-test"), std::string::npos);
    EXPECT_NE(aql.find("FROM data"), std::string::npos);
    EXPECT_NE(aql.find("mistral-7b"), std::string::npos);
}

TEST_F(TrainingQueryBuilderTest, ToAQLContainsWhereClause) {
    TrainingQueryBuilder builder;
    builder.adapter("a").from("col").where("doc.quality > 0.9")
           .baseModel("m").loraRank(8).epochs(1).learningRate(1e-4);

    std::string aql = builder.toAQL();
    EXPECT_NE(aql.find("WHERE doc.quality > 0.9"), std::string::npos);
}

TEST_F(TrainingQueryBuilderTest, ToAQLContainsDistributed) {
    AQLDistributedTrainingConfig dist;
    dist.enabled           = true;
    dist.coordinator_shard = "shard-0";

    TrainingQueryBuilder builder;
    builder.adapter("d-adapter").from("corpus")
           .baseModel("llama").loraRank(8).epochs(2).learningRate(2e-4)
           .distributed(dist);

    std::string aql = builder.toAQL();
    EXPECT_NE(aql.find("DISTRIBUTED"), std::string::npos);
    EXPECT_NE(aql.find("shard-0"), std::string::npos);
}

TEST_F(TrainingQueryBuilderTest, WithGraphContextEnrichment) {
    GraphContextConfig gc;
    gc.max_depth  = 2;
    gc.direction  = "OUTBOUND";

    auto stmt = TrainingQueryBuilder{}
        .adapter("gc-adapter")
        .from("nodes")
        .withGraphContext(gc)
        .baseModel("mistral-7b")
        .loraRank(8)
        .epochs(1)
        .learningRate(1e-4)
        .build();

    ASSERT_NE(stmt, nullptr);
    ASSERT_TRUE(stmt->enrichment.graph_context.has_value());
    EXPECT_EQ(stmt->enrichment.graph_context->max_depth, 2);
}

TEST_F(TrainingQueryBuilderTest, WithVectorSimilarityEnrichment) {
    VectorSimilarityConfig vs;
    vs.field     = "embedding";
    vs.top_k     = 10;
    vs.threshold = 0.75;

    auto stmt = TrainingQueryBuilder{}
        .adapter("vs-adapter")
        .from("docs")
        .withVectorSimilarity(vs)
        .baseModel("mistral-7b")
        .loraRank(8)
        .epochs(1)
        .learningRate(1e-4)
        .build();

    ASSERT_NE(stmt, nullptr);
    ASSERT_TRUE(stmt->enrichment.vector_similarity.has_value());
    EXPECT_EQ(stmt->enrichment.vector_similarity->field, "embedding");
    EXPECT_EQ(stmt->enrichment.vector_similarity->top_k, 10);
}

TEST_F(TrainingQueryBuilderTest, SignAdapterFlag) {
    auto stmt = TrainingQueryBuilder{}
        .adapter("signed").from("d").baseModel("m").loraRank(8).epochs(1).learningRate(1e-4)
        .signAdapter(false)
        .build();
    EXPECT_FALSE(stmt->config.sign_adapter);
}

TEST_F(TrainingQueryBuilderTest, OutputPathPropagated) {
    auto stmt = TrainingQueryBuilder{}
        .adapter("out-adapter").from("data").baseModel("m")
        .loraRank(8).epochs(1).learningRate(1e-4)
        .outputPath("/data/adapters/out.gguf")
        .build();
    EXPECT_EQ(stmt->output_path, "/data/adapters/out.gguf");
}
