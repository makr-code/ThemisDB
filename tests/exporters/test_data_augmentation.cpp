#include <gtest/gtest.h>
#include "exporters/data_augmentation.h"
#include "storage/base_entity.h"
#include <algorithm>
#include <set>
#include <string>
#include <vector>

using namespace themis::exporters;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static BaseEntity makeEntity(
    const std::string& pk,
    const std::string& question,
    const std::string& answer,
    const std::string& context = ""
) {
    BaseEntity e;
    e.setPrimaryKey(pk);
    e.setField("question", question);
    e.setField("answer",   answer);
    if (!context.empty()) {
        e.setField("context", context);
    }
    return e;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class DataAugmentationTest : public ::testing::Test {
protected:
    std::vector<BaseEntity> sampleEntities() const {
        return {
            makeEntity("e1", "What is a big database?", "It is a large system.", "context1"),
            makeEntity("e2", "How fast can you find data?", "Very quickly.", "context2"),
            makeEntity("e3", "Is this example correct?", "Yes, it is good.", ""),
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// AugmentationConfig defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, EmptyStrategiesReturnsOriginalsOnly) {
    AugmentationConfig cfg;
    // no strategies configured
    DataAugmentationPipeline pipeline(cfg);

    auto entities = sampleEntities();
    AugmentationStats stats;
    auto result = pipeline.augment(entities, &stats);

    EXPECT_EQ(result.size(), entities.size());
    EXPECT_EQ(stats.input_entities,     entities.size());
    EXPECT_EQ(stats.augmented_entities, 0u);
    EXPECT_EQ(stats.output_entities,    entities.size());
    EXPECT_EQ(stats.strategies_applied, 0u);
}

TEST_F(DataAugmentationTest, ExcludeOriginalsReturnsOnlySynthetic) {
    AugmentationConfig cfg;
    cfg.include_originals = false;
    cfg.strategies.push_back({AugmentationStrategy::LOWERCASE, 1});

    DataAugmentationPipeline pipeline(cfg);
    auto entities = sampleEntities();
    AugmentationStats stats;
    auto result = pipeline.augment(entities, &stats);

    EXPECT_EQ(stats.augmented_entities, entities.size());
    EXPECT_EQ(result.size(), entities.size());   // only synthetic copies
}

TEST_F(DataAugmentationTest, IncludeOriginalsAddsToResult) {
    AugmentationConfig cfg;
    cfg.include_originals = true;
    cfg.strategies.push_back({AugmentationStrategy::LOWERCASE, 1});

    DataAugmentationPipeline pipeline(cfg);
    auto entities = sampleEntities();
    AugmentationStats stats;
    auto result = pipeline.augment(entities, &stats);

    EXPECT_EQ(result.size(), entities.size() * 2);
    EXPECT_EQ(stats.output_entities, entities.size() * 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// Primary-key uniqueness
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, SyntheticEntitiesHaveUniqueKeys) {
    AugmentationConfig cfg;
    cfg.include_originals = false;
    cfg.strategies.push_back({AugmentationStrategy::LOWERCASE, 2});

    DataAugmentationPipeline pipeline(cfg);
    auto entities = sampleEntities();
    auto result   = pipeline.augment(entities);

    std::vector<std::string> pks = {};

    for (const auto& e : result) {
        pks.push_back(e.getPrimaryKey());
    }
    std::sort(pks.begin(), pks.end());
    auto uniq_end = std::unique(pks.begin(), pks.end());
    EXPECT_EQ(uniq_end, pks.end()) << "Duplicate primary keys found in augmented output";
}

TEST_F(DataAugmentationTest, AugmentedKeyPrefixIsApplied) {
    AugmentationConfig cfg;
    cfg.include_originals   = false;
    cfg.augmented_key_prefix = "synth_";
    cfg.strategies.push_back({AugmentationStrategy::LOWERCASE, 1});

    DataAugmentationPipeline pipeline(cfg);
    auto result = pipeline.augment(sampleEntities());

    for (const auto& e : result) {
        EXPECT_EQ(e.getPrimaryKey().substr(0, 6), "synth_")
            << "Key '" << e.getPrimaryKey() << "' does not start with 'synth_'";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LOWERCASE strategy
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, LowercaseStrategyConvertsAllCaps) {
    AugmentationConfig cfg;
    cfg.include_originals = false;
    cfg.strategies.push_back({AugmentationStrategy::LOWERCASE, 1});

    DataAugmentationPipeline pipeline(cfg);

    BaseEntity e;
    e.setPrimaryKey("upper");
    e.setField("question", "WHAT IS A DATABASE?");
    e.setField("answer",   "IT IS A SYSTEM.");

    auto result = pipeline.augment({e});
    ASSERT_EQ(result.size(), 1u);

    auto q = result[0].getFieldAsString("question");
    ASSERT_TRUE(q.has_value());
    EXPECT_EQ(*q, "what is a database?");

    auto a = result[0].getFieldAsString("answer");
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(*a, "it is a system.");
}

// ─────────────────────────────────────────────────────────────────────────────
// SENTENCE_CASING strategy
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, SentenceCasingUppercasesFirstChar) {
    AugmentationConfig cfg;
    cfg.include_originals = false;
    cfg.strategies.push_back({AugmentationStrategy::SENTENCE_CASING, 1});

    DataAugmentationPipeline pipeline(cfg);

    BaseEntity e;
    e.setPrimaryKey("sc");
    e.setField("question", "what is a database?");
    e.setField("answer",   "it is a system. it stores data.");

    auto result = pipeline.augment({e});
    ASSERT_EQ(result.size(), 1u);

    auto q = result[0].getFieldAsString("question");
    ASSERT_TRUE(q.has_value());
    EXPECT_EQ((*q)[0], 'W') << "First character should be uppercased";

    auto a = result[0].getFieldAsString("answer");
    ASSERT_TRUE(a.has_value());
    // "It is a system. It stores data."
    EXPECT_EQ((*a)[0], 'I');
}

// ─────────────────────────────────────────────────────────────────────────────
// WHITESPACE_NORMALIZATION strategy
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, WhitespaceNormalizationCollapseSpaces) {
    AugmentationConfig cfg;
    cfg.include_originals = false;
    cfg.strategies.push_back({AugmentationStrategy::WHITESPACE_NORMALIZATION, 1});

    DataAugmentationPipeline pipeline(cfg);

    BaseEntity e;
    e.setPrimaryKey("ws");
    e.setField("question", "  What   is   a  database?  ");
    e.setField("answer",   " It   is   a system. ");

    auto result = pipeline.augment({e});
    ASSERT_EQ(result.size(), 1u);

    auto q = result[0].getFieldAsString("question");
    ASSERT_TRUE(q.has_value());
    EXPECT_EQ(*q, "What is a database?");

    auto a = result[0].getFieldAsString("answer");
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(*a, "It is a system.");
}

// ─────────────────────────────────────────────────────────────────────────────
// SYNONYM_REPLACEMENT strategy
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, SynonymReplacementChangesKnownWord) {
    AugmentationConfig cfg;
    cfg.include_originals = false;
    cfg.strategies.push_back({AugmentationStrategy::SYNONYM_REPLACEMENT, 1});

    DataAugmentationPipeline pipeline(cfg);

    BaseEntity e;
    e.setPrimaryKey("syn");
    e.setField("question", "What is a big database?");
    e.setField("answer",   "It is a fast system.");

    auto result = pipeline.augment({e});
    ASSERT_EQ(result.size(), 1u);

    auto q = result[0].getFieldAsString("question");
    ASSERT_TRUE(q.has_value());
    // "big" should be replaced with a synonym (e.g. "large")
    EXPECT_NE(*q, "What is a big database?")
        << "Expected synonym replacement to change 'big'";

    auto a = result[0].getFieldAsString("answer");
    ASSERT_TRUE(a.has_value());
    // "fast" should be replaced
    EXPECT_NE(*a, "It is a fast system.")
        << "Expected synonym replacement to change 'fast'";
}

TEST_F(DataAugmentationTest, SynonymReplacementPreservesCapitalization) {
    AugmentationConfig cfg;
    cfg.include_originals = false;
    cfg.strategies.push_back({AugmentationStrategy::SYNONYM_REPLACEMENT, 1});

    DataAugmentationPipeline pipeline(cfg);

    BaseEntity e;
    e.setPrimaryKey("cap");
    // "Big" at the start of a sentence → should remain capitalized after replacement
    e.setField("question", "Big databases are important.");
    e.setField("answer",   "answer");

    auto result = pipeline.augment({e});
    ASSERT_EQ(result.size(), 1u);

    auto q = result[0].getFieldAsString("question");
    ASSERT_TRUE(q.has_value());
    // The synonym for "Big" should also start with an uppercase letter.
    EXPECT_TRUE(std::isupper(static_cast<unsigned char>((*q)[0])))
        << "Capitalization of first word not preserved: " << *q;
}

TEST_F(DataAugmentationTest, SynonymReplacementMultipleVariants) {
    AugmentationConfig cfg;
    cfg.include_originals = false;
    cfg.strategies.push_back({AugmentationStrategy::SYNONYM_REPLACEMENT, 3});

    DataAugmentationPipeline pipeline(cfg);

    BaseEntity e;
    e.setPrimaryKey("multi");
    e.setField("question", "How fast can you find data?");
    e.setField("answer",   "answer");

    AugmentationStats stats;
    auto result = pipeline.augment({e}, &stats);
    EXPECT_EQ(result.size(), 3u);
    EXPECT_EQ(stats.augmented_entities, 3u);
}

TEST_F(DataAugmentationTest, CustomSynonymIsUsed) {
    AugmentationConfig cfg;
    cfg.include_originals    = false;
    cfg.custom_synonyms["database"] = "datastore";
    cfg.strategies.push_back({AugmentationStrategy::SYNONYM_REPLACEMENT, 1});

    DataAugmentationPipeline pipeline(cfg);

    BaseEntity e;
    e.setPrimaryKey("custom");
    e.setField("question", "What is a database?");
    e.setField("answer",   "answer");

    auto result = pipeline.augment({e});
    ASSERT_EQ(result.size(), 1u);

    auto q = result[0].getFieldAsString("question");
    ASSERT_TRUE(q.has_value());
    EXPECT_NE(q->find("datastore"), std::string::npos)
        << "Custom synonym 'datastore' not found in: " << *q;
}

// ─────────────────────────────────────────────────────────────────────────────
// QUESTION_REFORMULATION strategy
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, QuestionReformulationChangesPrefix) {
    AugmentationConfig cfg;
    cfg.include_originals = false;
    cfg.strategies.push_back({AugmentationStrategy::QUESTION_REFORMULATION, 1});

    DataAugmentationPipeline pipeline(cfg);

    BaseEntity e;
    e.setPrimaryKey("qr");
    e.setField("question", "What is a database?");
    e.setField("answer",   "answer");

    auto result = pipeline.augment({e});
    ASSERT_EQ(result.size(), 1u);

    auto q = result[0].getFieldAsString("question");
    ASSERT_TRUE(q.has_value());
    // The reformulated question should end with '?' and differ from the original.
    EXPECT_NE(*q, "What is a database?");
    EXPECT_EQ(q->back(), '?');
}

TEST_F(DataAugmentationTest, QuestionReformulationMultipleVariantsDiffer) {
    AugmentationConfig cfg;
    cfg.include_originals = false;
    cfg.strategies.push_back({AugmentationStrategy::QUESTION_REFORMULATION, 4});

    DataAugmentationPipeline pipeline(cfg);

    BaseEntity e;
    e.setPrimaryKey("qrm");
    e.setField("question", "What is a database?");
    e.setField("answer",   "answer");

    auto result = pipeline.augment({e});
    ASSERT_EQ(result.size(), 4u);

    // At least two of the four variants should be distinct.
    std::set<std::string> questions = {};

    for (const auto& ent : result) {
        auto q = ent.getFieldAsString("question");
        if (q) {
          questions.insert(*q);
        }
    }
    EXPECT_GT(questions.size(), 1u) << "All question reformulations are identical";
}

TEST_F(DataAugmentationTest, QuestionReformulationCustomInstructionField) {
    AugmentationConfig cfg;
    cfg.include_originals     = false;
    cfg.instruction_field     = "prompt";
    cfg.strategies.push_back({AugmentationStrategy::QUESTION_REFORMULATION, 1});

    DataAugmentationPipeline pipeline(cfg);

    BaseEntity e;
    e.setPrimaryKey("qf");
    e.setField("prompt",   "What is an index?");
    e.setField("response", "An index speeds up searches.");

    auto result = pipeline.augment({e});
    ASSERT_EQ(result.size(), 1u);

    auto p = result[0].getFieldAsString("prompt");
    ASSERT_TRUE(p.has_value());
    EXPECT_NE(*p, "What is an index?") << "Instruction field not reformulated";
}

// ─────────────────────────────────────────────────────────────────────────────
// augment_fields restriction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, AugmentFieldsRestrictsScope) {
    AugmentationConfig cfg;
    cfg.include_originals = false;
    cfg.augment_fields    = {"answer"};   // only augment the "answer" field
    cfg.strategies.push_back({AugmentationStrategy::LOWERCASE, 1});

    DataAugmentationPipeline pipeline(cfg);

    BaseEntity e;
    e.setPrimaryKey("af");
    e.setField("question", "What IS a DATABASE?");
    e.setField("answer",   "IT IS A SYSTEM.");

    auto result = pipeline.augment({e});
    ASSERT_EQ(result.size(), 1u);

    // "question" must NOT be changed (not in augment_fields)
    auto q = result[0].getFieldAsString("question");
    ASSERT_TRUE(q.has_value());
    EXPECT_EQ(*q, "What IS a DATABASE?");

    // "answer" MUST be lowercased
    auto a = result[0].getFieldAsString("answer");
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(*a, "it is a system.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Multiple strategies composed
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, MultipleStrategiesProduceExpectedCount) {
    AugmentationConfig cfg;
    cfg.include_originals = true;
    cfg.strategies.push_back({AugmentationStrategy::LOWERCASE,               1});
    cfg.strategies.push_back({AugmentationStrategy::SYNONYM_REPLACEMENT,     2});
    cfg.strategies.push_back({AugmentationStrategy::QUESTION_REFORMULATION,  1});

    DataAugmentationPipeline pipeline(cfg);
    auto entities = sampleEntities();  // 3 entities

    AugmentationStats stats;
    auto result = pipeline.augment(entities, &stats);

    // originals: 3, per entity: 1 + 2 + 1 = 4 → total synthetic: 12
    // grand total: 15
    size_t expected_synthetic = entities.size() * (1 + 2 + 1);
    size_t expected_total     = entities.size() + expected_synthetic;

    EXPECT_EQ(stats.augmented_entities, expected_synthetic);
    EXPECT_EQ(result.size(),            expected_total);
    EXPECT_EQ(stats.output_entities,    expected_total);
    EXPECT_EQ(stats.strategies_applied, 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Zero-count strategy is a no-op
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, ZeroCountStrategyIsNoop) {
    AugmentationConfig cfg;
    cfg.include_originals = true;
    cfg.strategies.push_back({AugmentationStrategy::LOWERCASE, 0});  // disabled

    DataAugmentationPipeline pipeline(cfg);
    auto entities = sampleEntities();

    AugmentationStats stats;
    auto result = pipeline.augment(entities, &stats);

    EXPECT_EQ(result.size(),            entities.size());  // no copies added
    EXPECT_EQ(stats.augmented_entities, 0u);
    EXPECT_EQ(stats.strategies_applied, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// applyStrategy API
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, ApplyStrategyReturnsRequestedCount) {
    DataAugmentationPipeline pipeline;

    BaseEntity e = makeEntity("pk1", "What is a fast database?", "answer");
    auto copies = pipeline.applyStrategy(e, AugmentationStrategy::SYNONYM_REPLACEMENT, 3);

    EXPECT_EQ(copies.size(), 3u);
    for (const auto& c : copies) {
        EXPECT_NE(c.getPrimaryKey(), "pk1")
            << "Augmented entity must have a different primary key";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Empty input
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, EmptyInputReturnsEmpty) {
    AugmentationConfig cfg;
    cfg.strategies.push_back({AugmentationStrategy::LOWERCASE, 1});

    DataAugmentationPipeline pipeline(cfg);
    AugmentationStats stats;
    auto result = pipeline.augment({}, &stats);

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(stats.input_entities, 0u);
    EXPECT_EQ(stats.augmented_entities, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Config getters/setters
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DataAugmentationTest, GetSetConfigRoundTrip) {
    AugmentationConfig cfg;
    cfg.augmented_key_prefix = "test_prefix_";
    cfg.include_originals    = false;

    DataAugmentationPipeline pipeline;
    pipeline.setConfig(cfg);

    const auto& got = pipeline.getConfig();
    EXPECT_EQ(got.augmented_key_prefix, "test_prefix_");
    EXPECT_EQ(got.include_originals,    false);
}

// ─────────────────────────────────────────────────────────────────────────────
// Integration: augment then export via JSONL pipeline
// ─────────────────────────────────────────────────────────────────────────────

// This test verifies that augmented entities pass through the JSONL exporter
// without error (it does NOT verify JSONL output content, only stats).
#include "exporters/jsonl_llm_exporter.h"
#include <filesystem>
#include <ctime>

TEST_F(DataAugmentationTest, AugmentedEntitiesExportToJSONL) {
    // Create a temp output directory
    auto temp_base = std::filesystem::temp_directory_path();
    auto test_dir  = (temp_base /
        ("themis_aug_export_test_" + std::to_string(std::time(nullptr))))
        .string();
    std::filesystem::create_directories(test_dir);

    AugmentationConfig aug_cfg;
    aug_cfg.include_originals = true;
    aug_cfg.strategies.push_back({AugmentationStrategy::SYNONYM_REPLACEMENT, 1});
    aug_cfg.strategies.push_back({AugmentationStrategy::QUESTION_REFORMULATION, 1});

    DataAugmentationPipeline pipeline(aug_cfg);
    auto expanded = pipeline.augment(sampleEntities());

    JSONLLLMConfig exp_cfg;
    exp_cfg.style = JSONLFormat::Style::INSTRUCTION_TUNING;
    JSONLLLMExporter exporter(exp_cfg);

    ExportOptions opts;
    opts.output_path = test_dir + "/augmented_output.jsonl";

    auto stats = exporter.exportEntities(expanded, opts);

    // Should export at least the synthetic copies; failures are not expected.
    EXPECT_GT(stats.exported_entities, 0u);
    EXPECT_EQ(stats.failed_entities,   0u);

    std::filesystem::remove_all(test_dir);
}
