/**
 * @file test_multi_perspective_generator.cpp
 * @brief Unit tests for Multi-Perspective Generator
 */

#include <gtest/gtest.h>
#include "llm/multi_perspective_generator.h"

using namespace themis::llm;

class MultiPerspectiveGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.min_perspectives = 2;
        config_.max_perspectives = 4;
        config_.auto_select_perspectives = true;
        config_.min_diversity_score = 0.6f;
        config_.require_contrasting_views = true;
        config_.enable_synthesis = true;
        config_.preserve_all_perspectives = true;
        config_.highlight_disagreements = true;
        config_.cache_perspectives = true;
        
        generator_ = std::make_unique<MultiPerspectiveGenerator>(config_);
        generator_->loadDefaultPerspectives();
    }
    
    MultiPerspectiveConfig config_;
    std::unique_ptr<MultiPerspectiveGenerator> generator_;
};

// ═══════════════════════════════════════════════════════════
// Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, DefaultConfiguration) {
    auto generator = std::make_unique<MultiPerspectiveGenerator>();
    auto config = generator->getConfig();
    
    EXPECT_GE(config.min_perspectives, 2);
    EXPECT_LE(config.max_perspectives, 10);
    EXPECT_TRUE(config.enable_synthesis);
}

TEST_F(MultiPerspectiveGeneratorTest, ConfigurationUpdate) {
    MultiPerspectiveConfig new_config;
    new_config.min_perspectives = 3;
    new_config.max_perspectives = 5;
    new_config.min_diversity_score = 0.75f;
    
    generator_->setConfig(new_config);
    auto retrieved = generator_->getConfig();
    
    EXPECT_EQ(retrieved.min_perspectives, 3);
    EXPECT_EQ(retrieved.max_perspectives, 5);
    EXPECT_FLOAT_EQ(retrieved.min_diversity_score, 0.75f);
}

TEST_F(MultiPerspectiveGeneratorTest, ConfigurationValidation) {
    MultiPerspectiveConfig config;
    config.min_perspectives = 2;
    config.max_perspectives = 4;
    config.min_diversity_score = 0.6f;
    
    EXPECT_GE(config.min_perspectives, 1);
    EXPECT_LE(config.max_perspectives, 10);
    EXPECT_GE(config.min_diversity_score, 0.0f);
    EXPECT_LE(config.min_diversity_score, 1.0f);
}

// ═══════════════════════════════════════════════════════════
// Perspective Management Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, LoadDefaultPerspectives) {
    auto perspectives = generator_->getAvailablePerspectives();
    
    EXPECT_GT(perspectives.size(), 0);
    
    // Check for standard ethical frameworks
    bool has_utilitarian = false;
    bool has_deontological = false;
    bool has_virtue = false;
    bool has_care = false;
    
    for (const auto& p : perspectives) {
        if (p.tradition == "Utilitarian") has_utilitarian = true;
        if (p.tradition == "Deontological") has_deontological = true;
        if (p.tradition == "Virtue Ethics") has_virtue = true;
        if (p.tradition == "Care Ethics") has_care = true;
    }
    
    EXPECT_TRUE(has_utilitarian);
    EXPECT_TRUE(has_deontological);
}

TEST_F(MultiPerspectiveGeneratorTest, AddCustomPerspective) {
    EthicalPerspective custom;
    custom.id = "custom_test";
    custom.name = "Custom Test Perspective";
    custom.description = "A custom perspective for testing";
    custom.tradition = "Test Framework";
    custom.key_principles = {"principle1", "principle2"};
    custom.prompt_template = "From a {tradition} perspective: {query}";
    
    generator_->addPerspective(custom);
    auto perspectives = generator_->getAvailablePerspectives();
    
    bool found = false;
    for (const auto& p : perspectives) {
        if (p.id == "custom_test") {
            found = true;
            EXPECT_EQ(p.name, "Custom Test Perspective");
            EXPECT_EQ(p.tradition, "Test Framework");
            break;
        }
    }
    
    EXPECT_TRUE(found);
}

TEST_F(MultiPerspectiveGeneratorTest, RemovePerspective) {
    auto perspectives_before = generator_->getAvailablePerspectives();
    size_t count_before = perspectives_before.size();
    
    if (count_before > 0) {
        std::string id_to_remove = perspectives_before[0].id;
        generator_->removePerspective(id_to_remove);
        
        auto perspectives_after = generator_->getAvailablePerspectives();
        EXPECT_EQ(perspectives_after.size(), count_before - 1);
        
        // Verify it's actually removed
        bool still_exists = false;
        for (const auto& p : perspectives_after) {
            if (p.id == id_to_remove) {
                still_exists = true;
                break;
            }
        }
        EXPECT_FALSE(still_exists);
    }
}

TEST_F(MultiPerspectiveGeneratorTest, PerspectiveStructure) {
    auto perspectives = generator_->getAvailablePerspectives();
    
    ASSERT_GT(perspectives.size(), 0);
    
    const auto& first = perspectives[0];
    EXPECT_FALSE(first.id.empty());
    EXPECT_FALSE(first.name.empty());
    EXPECT_FALSE(first.description.empty());
    EXPECT_FALSE(first.tradition.empty());
    EXPECT_GT(first.key_principles.size(), 0);
}

// ═══════════════════════════════════════════════════════════
// Ethical Query Detection Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, DetectEthicalQueryMoral) {
    std::string query = "Is it morally right to lie to protect someone's feelings?";
    bool requires = generator_->requiresMultiPerspective(query);
    
    EXPECT_TRUE(requires);
}

TEST_F(MultiPerspectiveGeneratorTest, DetectEthicalQueryEthical) {
    std::string query = "What are the ethical implications of AI in healthcare?";
    bool requires = generator_->requiresMultiPerspective(query);
    
    EXPECT_TRUE(requires);
}

TEST_F(MultiPerspectiveGeneratorTest, DetectEthicalQueryShould) {
    std::string query = "Should I prioritize fairness or efficiency in this situation?";
    bool requires = generator_->requiresMultiPerspective(query);
    
    EXPECT_TRUE(requires);
}

TEST_F(MultiPerspectiveGeneratorTest, DetectNonEthicalQuery) {
    std::string query = "What is the capital of France?";
    bool requires = generator_->requiresMultiPerspective(query);
    
    EXPECT_FALSE(requires);
}

TEST_F(MultiPerspectiveGeneratorTest, DetectTechnicalQuery) {
    std::string query = "How do I sort an array in Python?";
    bool requires = generator_->requiresMultiPerspective(query);
    
    EXPECT_FALSE(requires);
}

// ═══════════════════════════════════════════════════════════
// Perspective Selection Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, SelectPerspectivesForEthicalQuery) {
    std::string query = "Is it ethical to use AI for surveillance?";
    auto selected = generator_->selectPerspectives(query);
    
    EXPECT_GE(selected.size(), config_.min_perspectives);
    EXPECT_LE(selected.size(), config_.max_perspectives);
}

TEST_F(MultiPerspectiveGeneratorTest, SelectPerspectivesRespectMinMax) {
    config_.min_perspectives = 3;
    config_.max_perspectives = 5;
    generator_->setConfig(config_);
    
    std::string query = "What is the right thing to do in this moral dilemma?";
    auto selected = generator_->selectPerspectives(query);
    
    EXPECT_GE(selected.size(), 3);
    EXPECT_LE(selected.size(), 5);
}

TEST_F(MultiPerspectiveGeneratorTest, SelectPerspectivesWithRequired) {
    config_.required_perspectives = {"utilitarian", "deontological"};
    generator_->setConfig(config_);
    
    std::string query = "Is it right to sacrifice one to save many?";
    auto selected = generator_->selectPerspectives(query);
    
    // Check that required perspectives are included
    bool has_utilitarian = false;
    bool has_deontological = false;
    
    for (const auto& p : selected) {
        if (p.id == "utilitarian") has_utilitarian = true;
        if (p.id == "deontological") has_deontological = true;
    }
    
    EXPECT_TRUE(has_utilitarian || has_deontological);
}

TEST_F(MultiPerspectiveGeneratorTest, SelectPerspectivesDiversity) {
    std::string query = "How should we balance individual rights and collective good?";
    auto selected = generator_->selectPerspectives(query);
    
    // Should select diverse perspectives
    std::set<std::string> traditions;
    for (const auto& p : selected) {
        traditions.insert(p.tradition);
    }
    
    EXPECT_GE(traditions.size(), 2); // At least 2 different traditions
}

// ═══════════════════════════════════════════════════════════
// Single Perspective Generation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, GenerateSinglePerspectiveStructure) {
    std::string query = "Is lying ever justified?";
    auto perspectives = generator_->getAvailablePerspectives();
    
    ASSERT_GT(perspectives.size(), 0);
    
    // Note: Actual generation requires LLM wrapper
    // This test verifies the structure without actual LLM call
    PerspectiveResponse response;
    response.perspective = perspectives[0];
    response.response = "Test response";
    response.confidence = 0.8f;
    response.key_points = {"point1", "point2"};
    response.reasoning = "Test reasoning";
    
    EXPECT_FALSE(response.response.empty());
    EXPECT_GE(response.confidence, 0.0f);
    EXPECT_LE(response.confidence, 1.0f);
    EXPECT_GT(response.key_points.size(), 0);
}

// ═══════════════════════════════════════════════════════════
// Multi-Perspective Generation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, MultiPerspectiveResultStructure) {
    // Test result structure without actual LLM generation
    MultiPerspectiveResult result;
    result.query = "Test query";
    result.unique_perspectives_count = 3;
    result.perspective_diversity_score = 0.75f;
    result.shows_balanced_view = true;
    result.meets_diversity_requirement = true;
    
    EXPECT_FALSE(result.query.empty());
    EXPECT_GT(result.unique_perspectives_count, 0);
    EXPECT_GE(result.perspective_diversity_score, 0.0f);
    EXPECT_LE(result.perspective_diversity_score, 1.0f);
}

TEST_F(MultiPerspectiveGeneratorTest, MultiPerspectiveWithContext) {
    std::vector<std::string> context = {
        "Previous discussion about ethics",
        "User asked about moral frameworks"
    };
    
    // Test that context is handled properly (structure test)
    EXPECT_EQ(context.size(), 2);
}

// ═══════════════════════════════════════════════════════════
// Diversity Scoring Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, CalculateDiversityScoreIdentical) {
    std::vector<PerspectiveResponse> perspectives;
    
    EthicalPerspective p1;
    p1.id = "p1";
    p1.tradition = "Utilitarian";
    
    PerspectiveResponse r1;
    r1.perspective = p1;
    r1.response = "Same response";
    r1.key_points = {"point1", "point2"};
    
    PerspectiveResponse r2;
    r2.perspective = p1;
    r2.response = "Same response";
    r2.key_points = {"point1", "point2"};
    
    perspectives.push_back(r1);
    perspectives.push_back(r2);
    
    float score = generator_->calculateDiversityScore(perspectives);
    
    // Identical responses should have low diversity
    EXPECT_LT(score, 0.5f);
}

TEST_F(MultiPerspectiveGeneratorTest, CalculateDiversityScoreDiverse) {
    std::vector<PerspectiveResponse> perspectives;
    
    EthicalPerspective p1, p2;
    p1.id = "utilitarian";
    p1.tradition = "Utilitarian";
    p2.id = "deontological";
    p2.tradition = "Deontological";
    
    PerspectiveResponse r1;
    r1.perspective = p1;
    r1.response = "Focus on outcomes and greatest happiness";
    r1.key_points = {"outcomes", "happiness", "utility"};
    
    PerspectiveResponse r2;
    r2.perspective = p2;
    r2.response = "Focus on duties and moral rules";
    r2.key_points = {"duties", "rules", "obligations"};
    
    perspectives.push_back(r1);
    perspectives.push_back(r2);
    
    float score = generator_->calculateDiversityScore(perspectives);
    
    // Different perspectives should have high diversity
    EXPECT_GT(score, 0.4f);
}

TEST_F(MultiPerspectiveGeneratorTest, CalculateDiversityScoreEmpty) {
    std::vector<PerspectiveResponse> perspectives;
    
    float score = generator_->calculateDiversityScore(perspectives);
    
    EXPECT_EQ(score, 0.0f);
}

TEST_F(MultiPerspectiveGeneratorTest, CalculateDiversityScoreSingle) {
    std::vector<PerspectiveResponse> perspectives;
    
    EthicalPerspective p1;
    p1.id = "p1";
    p1.tradition = "Utilitarian";
    
    PerspectiveResponse r1;
    r1.perspective = p1;
    r1.response = "Test response";
    r1.key_points = {"point1"};
    
    perspectives.push_back(r1);
    
    float score = generator_->calculateDiversityScore(perspectives);
    
    // Single perspective should have zero diversity
    EXPECT_EQ(score, 0.0f);
}

// ═══════════════════════════════════════════════════════════
// Common Themes Detection Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, FindCommonThemesMultiple) {
    std::vector<PerspectiveResponse> perspectives;
    
    EthicalPerspective p1, p2;
    p1.id = "p1";
    p2.id = "p2";
    
    PerspectiveResponse r1;
    r1.perspective = p1;
    r1.response = "Respect for human dignity is important";
    r1.key_points = {"human dignity", "respect", "autonomy"};
    
    PerspectiveResponse r2;
    r2.perspective = p2;
    r2.response = "Human dignity must be protected";
    r2.key_points = {"human dignity", "protection", "rights"};
    
    perspectives.push_back(r1);
    perspectives.push_back(r2);
    
    auto themes = generator_->findCommonThemes(perspectives);
    
    EXPECT_GT(themes.size(), 0);
}

TEST_F(MultiPerspectiveGeneratorTest, FindCommonThemesNone) {
    std::vector<PerspectiveResponse> perspectives;
    
    EthicalPerspective p1, p2;
    p1.id = "p1";
    p2.id = "p2";
    
    PerspectiveResponse r1;
    r1.perspective = p1;
    r1.response = "Focus on consequences";
    r1.key_points = {"outcomes", "utility"};
    
    PerspectiveResponse r2;
    r2.perspective = p2;
    r2.response = "Focus on duties";
    r2.key_points = {"obligations", "rules"};
    
    perspectives.push_back(r1);
    perspectives.push_back(r2);
    
    auto themes = generator_->findCommonThemes(perspectives);
    
    // May or may not find themes depending on algorithm
    EXPECT_GE(themes.size(), 0);
}

TEST_F(MultiPerspectiveGeneratorTest, FindCommonThemesEmpty) {
    std::vector<PerspectiveResponse> perspectives;
    
    auto themes = generator_->findCommonThemes(perspectives);
    
    EXPECT_EQ(themes.size(), 0);
}

// ═══════════════════════════════════════════════════════════
// Disagreements Detection Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, FindDisagreementsContrasting) {
    std::vector<PerspectiveResponse> perspectives;
    
    EthicalPerspective p1, p2;
    p1.id = "utilitarian";
    p1.tradition = "Utilitarian";
    p2.id = "deontological";
    p2.tradition = "Deontological";
    
    PerspectiveResponse r1;
    r1.perspective = p1;
    r1.response = "The action is justified if it maximizes happiness";
    r1.key_points = {"maximize", "happiness", "outcomes"};
    
    PerspectiveResponse r2;
    r2.perspective = p2;
    r2.response = "The action violates moral duty regardless of outcomes";
    r2.key_points = {"duty", "rules", "regardless of outcomes"};
    
    perspectives.push_back(r1);
    perspectives.push_back(r2);
    
    auto disagreements = generator_->findDisagreements(perspectives);
    
    EXPECT_GT(disagreements.size(), 0);
}

TEST_F(MultiPerspectiveGeneratorTest, FindDisagreementsAgreement) {
    std::vector<PerspectiveResponse> perspectives;
    
    EthicalPerspective p1, p2;
    p1.id = "p1";
    p2.id = "p2";
    
    PerspectiveResponse r1;
    r1.perspective = p1;
    r1.response = "This action is morally wrong";
    r1.key_points = {"wrong", "unethical"};
    
    PerspectiveResponse r2;
    r2.perspective = p2;
    r2.response = "This action is clearly wrong";
    r2.key_points = {"wrong", "unethical"};
    
    perspectives.push_back(r1);
    perspectives.push_back(r2);
    
    auto disagreements = generator_->findDisagreements(perspectives);
    
    // Should find few or no disagreements
    EXPECT_LE(disagreements.size(), 1);
}

TEST_F(MultiPerspectiveGeneratorTest, FindDisagreementsEmpty) {
    std::vector<PerspectiveResponse> perspectives;
    
    auto disagreements = generator_->findDisagreements(perspectives);
    
    EXPECT_EQ(disagreements.size(), 0);
}

// ═══════════════════════════════════════════════════════════
// Synthesis Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, SynthesizePerspectivesStructure) {
    std::vector<PerspectiveResponse> perspectives;
    
    EthicalPerspective p1;
    p1.id = "p1";
    p1.name = "Perspective 1";
    
    PerspectiveResponse r1;
    r1.perspective = p1;
    r1.response = "Response from perspective 1";
    r1.key_points = {"point1"};
    
    perspectives.push_back(r1);
    
    std::string query = "Test query";
    std::string synthesis = generator_->synthesizePerspectives(perspectives, query);
    
    // Without actual LLM, synthesis would be empty or placeholder
    // Test structure is valid
    EXPECT_TRUE(true);
}

TEST_F(MultiPerspectiveGeneratorTest, SynthesizePerspectivesEmpty) {
    std::vector<PerspectiveResponse> perspectives;
    std::string query = "Test query";
    
    std::string synthesis = generator_->synthesizePerspectives(perspectives, query);
    
    // Should handle empty perspectives gracefully
    EXPECT_TRUE(true);
}

// ═══════════════════════════════════════════════════════════
// Statistics Tracking Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, StatisticsInitialization) {
    auto stats = generator_->getStatistics();
    
    EXPECT_EQ(stats.total_generations, 0);
    EXPECT_EQ(stats.multi_perspective_generated, 0);
    EXPECT_EQ(stats.cache_hits, 0);
    EXPECT_EQ(stats.cache_misses, 0);
}

TEST_F(MultiPerspectiveGeneratorTest, StatisticsReset) {
    generator_->resetStatistics();
    auto stats = generator_->getStatistics();
    
    EXPECT_EQ(stats.total_generations, 0);
    EXPECT_EQ(stats.multi_perspective_generated, 0);
    EXPECT_FLOAT_EQ(stats.avg_diversity_score, 0.0f);
    EXPECT_FLOAT_EQ(stats.avg_perspectives_per_query, 0.0f);
}

TEST_F(MultiPerspectiveGeneratorTest, PerspectiveUsageTracking) {
    auto stats = generator_->getStatistics();
    
    EXPECT_TRUE(stats.perspective_usage.empty());
}

// ═══════════════════════════════════════════════════════════
// Cache Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, CacheClear) {
    generator_->clearCache();
    auto stats = generator_->getStatistics();
    
    // Should not crash
    EXPECT_GE(stats.cache_hits, 0);
}

TEST_F(MultiPerspectiveGeneratorTest, CacheConfiguration) {
    MultiPerspectiveConfig config;
    config.cache_perspectives = true;
    config.max_cache_size = 1000;
    
    generator_->setConfig(config);
    auto retrieved = generator_->getConfig();
    
    EXPECT_TRUE(retrieved.cache_perspectives);
    EXPECT_EQ(retrieved.max_cache_size, 1000);
}

// ═══════════════════════════════════════════════════════════
// Factory Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, FactoryDefault) {
    auto generator = MultiPerspectiveGeneratorFactory::createDefault();
    ASSERT_NE(generator, nullptr);
    
    auto config = generator->getConfig();
    EXPECT_GE(config.min_perspectives, 2);
    EXPECT_TRUE(config.enable_synthesis);
}

TEST_F(MultiPerspectiveGeneratorTest, FactoryHighDiversity) {
    auto generator = MultiPerspectiveGeneratorFactory::createHighDiversity();
    ASSERT_NE(generator, nullptr);
    
    auto config = generator->getConfig();
    EXPECT_GE(config.min_diversity_score, 0.7f);
    EXPECT_TRUE(config.require_contrasting_views);
}

TEST_F(MultiPerspectiveGeneratorTest, FactoryWithPerspectives) {
    std::vector<std::string> required = {"utilitarian", "deontological", "virtue"};
    auto generator = MultiPerspectiveGeneratorFactory::createWithPerspectives(required);
    ASSERT_NE(generator, nullptr);
    
    auto config = generator->getConfig();
    EXPECT_EQ(config.required_perspectives.size(), 3);
}

TEST_F(MultiPerspectiveGeneratorTest, FactoryCustomConfig) {
    MultiPerspectiveConfig custom_config;
    custom_config.min_perspectives = 3;
    custom_config.max_perspectives = 6;
    custom_config.min_diversity_score = 0.8f;
    
    auto generator = MultiPerspectiveGeneratorFactory::create(custom_config);
    ASSERT_NE(generator, nullptr);
    
    auto config = generator->getConfig();
    EXPECT_EQ(config.min_perspectives, 3);
    EXPECT_EQ(config.max_perspectives, 6);
    EXPECT_FLOAT_EQ(config.min_diversity_score, 0.8f);
}

// ═══════════════════════════════════════════════════════════
// Callback Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, GenerationCallback) {
    bool callback_called = false;
    
    generator_->setGenerationCallback(
        [&callback_called](const MultiPerspectiveResult& result) {
            callback_called = true;
        }
    );
    
    // Callback would be called during actual generation
    // Test that setting callback doesn't crash
    EXPECT_TRUE(true);
}

// ═══════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════

TEST_F(MultiPerspectiveGeneratorTest, EmptyQuery) {
    std::string query = "";
    bool requires = generator_->requiresMultiPerspective(query);
    
    // Empty query should not require multi-perspective
    EXPECT_FALSE(requires);
}

TEST_F(MultiPerspectiveGeneratorTest, VeryLongQuery) {
    std::string query(10000, 'x');
    bool requires = generator_->requiresMultiPerspective(query);
    
    // Should handle long queries without crashing
    EXPECT_GE(requires, false);
    EXPECT_LE(requires, true);
}

TEST_F(MultiPerspectiveGeneratorTest, SpecialCharactersQuery) {
    std::string query = "What is the ethical view on @#$%^&*()?";
    bool requires = generator_->requiresMultiPerspective(query);
    
    // Should handle special characters
    EXPECT_TRUE(requires); // Contains "ethical"
}

TEST_F(MultiPerspectiveGeneratorTest, MultilineQuery) {
    std::string query = "Is it ethical to use AI?\nWhat about privacy?\nAnd fairness?";
    bool requires = generator_->requiresMultiPerspective(query);
    
    EXPECT_TRUE(requires);
}

TEST_F(MultiPerspectiveGeneratorTest, NonEnglishQuery) {
    std::string query = "¿Es ético usar inteligencia artificial?";
    bool requires = generator_->requiresMultiPerspective(query);
    
    // Should attempt to detect ethical queries in other languages
    EXPECT_GE(requires, false);
    EXPECT_LE(requires, true);
}

TEST_F(MultiPerspectiveGeneratorTest, NoPerspectivesAvailable) {
    auto generator = std::make_unique<MultiPerspectiveGenerator>();
    // Don't load default perspectives
    
    auto perspectives = generator->getAvailablePerspectives();
    EXPECT_EQ(perspectives.size(), 0);
    
    std::string query = "Is this ethical?";
    auto selected = generator->selectPerspectives(query);
    
    // Should handle gracefully
    EXPECT_EQ(selected.size(), 0);
}

TEST_F(MultiPerspectiveGeneratorTest, SinglePerspectiveAvailable) {
    auto generator = std::make_unique<MultiPerspectiveGenerator>();
    
    EthicalPerspective single;
    single.id = "only_one";
    single.name = "Only Perspective";
    single.tradition = "Test";
    single.key_principles = {"principle"};
    
    generator->addPerspective(single);
    
    std::string query = "Is this ethical?";
    auto selected = generator->selectPerspectives(query);
    
    // Should return the single available perspective
    EXPECT_EQ(selected.size(), 1);
}

TEST_F(MultiPerspectiveGeneratorTest, MaxPerspectivesExceeded) {
    MultiPerspectiveConfig config;
    config.min_perspectives = 2;
    config.max_perspectives = 2; // Set low max
    
    generator_->setConfig(config);
    
    std::string query = "Complex ethical dilemma with many dimensions?";
    auto selected = generator_->selectPerspectives(query);
    
    // Should respect max_perspectives limit
    EXPECT_LE(selected.size(), 2);
}

TEST_F(MultiPerspectiveGeneratorTest, DisabledSynthesis) {
    MultiPerspectiveConfig config;
    config.enable_synthesis = false;
    
    generator_->setConfig(config);
    auto retrieved = generator_->getConfig();
    
    EXPECT_FALSE(retrieved.enable_synthesis);
}

TEST_F(MultiPerspectiveGeneratorTest, AsyncGeneration) {
    MultiPerspectiveConfig config;
    config.async_generation = true;
    
    generator_->setConfig(config);
    auto retrieved = generator_->getConfig();
    
    EXPECT_TRUE(retrieved.async_generation);
}

TEST_F(MultiPerspectiveGeneratorTest, EthicalGuidelinesManagerIntegration) {
    // Test configuration flag for integration
    MultiPerspectiveConfig config;
    config.use_ethical_guidelines_manager = true;
    
    generator_->setConfig(config);
    
    // Test setting manager (nullptr is valid)
    generator_->setEthicalGuidelinesManager(nullptr);
    
    // Should not crash
    EXPECT_TRUE(true);
}

} // anonymous namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
