#include <gtest/gtest.h>
#include "ethics_ai/ethics_ai_types.h"
#include "ethics_ai/ethics_ai_plugin_interface.h"
#include <string>
#include <vector>

using namespace themis::plugins::ethics;

// ========== Type Tests ==========

TEST(EthicsAITypes, ArgumentTypeConversion) {
    EXPECT_STREQ("pro", argumentTypeToString(ArgumentType::PRO));
    EXPECT_STREQ("contra", argumentTypeToString(ArgumentType::CONTRA));
    EXPECT_STREQ("rebuttal", argumentTypeToString(ArgumentType::REBUTTAL));
    EXPECT_STREQ("synthesis", argumentTypeToString(ArgumentType::SYNTHESIS));
    
    EXPECT_EQ(ArgumentType::PRO, stringToArgumentType("pro"));
    EXPECT_EQ(ArgumentType::CONTRA, stringToArgumentType("CONTRA"));
    EXPECT_EQ(ArgumentType::REBUTTAL, stringToArgumentType("Rebuttal"));
    
    EXPECT_THROW(stringToArgumentType("invalid"), std::invalid_argument);
}

TEST(EthicsAITypes, ArgumentStrengthConversion) {
    EXPECT_STREQ("weak", argumentStrengthToString(ArgumentStrength::WEAK));
    EXPECT_STREQ("moderate", argumentStrengthToString(ArgumentStrength::MODERATE));
    EXPECT_STREQ("strong", argumentStrengthToString(ArgumentStrength::STRONG));
    EXPECT_STREQ("decisive", argumentStrengthToString(ArgumentStrength::DECISIVE));
    
    EXPECT_EQ(ArgumentStrength::WEAK, stringToArgumentStrength("weak"));
    EXPECT_EQ(ArgumentStrength::MODERATE, stringToArgumentStrength("MODERATE"));
    EXPECT_EQ(ArgumentStrength::STRONG, stringToArgumentStrength("Strong"));
    
    EXPECT_THROW(stringToArgumentStrength("invalid"), std::invalid_argument);
}

TEST(EthicsAITypes, EthicalArgumentCreation) {
    EthicalArgument arg;
    EXPECT_EQ(ArgumentType::PRO, arg.argument_type);
    EXPECT_EQ(ArgumentStrength::MODERATE, arg.strength);
    EXPECT_TRUE(arg.id.empty());
    EXPECT_TRUE(arg.content.empty());
    
    arg.id = "test_arg_1";
    arg.philosophy_school = "kant";
    arg.content = "Test argument content";
    
    EXPECT_EQ("test_arg_1", arg.id);
    EXPECT_EQ("kant", arg.philosophy_school);
    EXPECT_EQ("Test argument content", arg.content);
}

TEST(EthicsAITypes, EthicalDecisionCreation) {
    EthicalDecision decision;
    EXPECT_EQ(0.0, decision.confidence);
    EXPECT_EQ(0.0, decision.consensus_level);
    EXPECT_TRUE(decision.decision_id.empty());
    
    decision.decision_id = "dec_001";
    decision.primary_philosophy = "kant";
    decision.confidence = 0.85;
    
    EXPECT_EQ("dec_001", decision.decision_id);
    EXPECT_EQ(0.85, decision.confidence);
}

TEST(EthicsAITypes, StatusOperations) {
    Status ok = Status::OK();
    EXPECT_TRUE(ok.isOK());
    EXPECT_TRUE(ok.ok);
    EXPECT_TRUE(ok);
    
    Status error = Status::Error("Test error", 42);
    EXPECT_FALSE(error.isOK());
    EXPECT_FALSE(error.ok);
    EXPECT_FALSE(error);
    EXPECT_EQ("Test error", error.message);
    EXPECT_EQ(42, error.code);
}

TEST(EthicsAITypes, EvaluationResultDefaults) {
    EthicsEvaluationResult result;
    EXPECT_EQ(0.0, result.overall_score);
    EXPECT_EQ(0.0, result.decision_quality_score);
    EXPECT_EQ(0.0, result.consistency_score);
    EXPECT_EQ(0.0, result.fairness_score);
    EXPECT_EQ(0.0, result.alignment_score);
    EXPECT_EQ(0.0, result.transparency_score);
    EXPECT_TRUE(result.detailed_metrics.empty());
}

TEST(EthicsAITypes, RAGContextCreation) {
    RAGContext context;
    EXPECT_TRUE(context.similar_dilemmas.empty());
    EXPECT_TRUE(context.philosophy_arguments.empty());
    EXPECT_TRUE(context.best_practices.empty());
    EXPECT_TRUE(context.recent_debates.empty());
    EXPECT_TRUE(context.consensus_decisions.empty());
    
    context.similar_dilemmas.push_back("dilemma_1");
    context.similar_dilemmas.push_back("dilemma_2");
    EXPECT_EQ(2u, context.similar_dilemmas.size());
}

TEST(EthicsAITypes, PhilosophyProfileCreation) {
    PhilosophyProfile profile;
    profile.school_id = "kant";
    profile.name = "Kantian Ethics";
    profile.main_theses.push_back("Categorical Imperative");
    profile.main_theses.push_back("Respect for Persons");
    
    EXPECT_EQ("kant", profile.school_id);
    EXPECT_EQ("Kantian Ethics", profile.name);
    EXPECT_EQ(2u, profile.main_theses.size());
}

TEST(EthicsAITypes, ArgumentChainCreation) {
    ArgumentChain chain;
    EXPECT_EQ(0.0, chain.coherence_score);
    EXPECT_TRUE(chain.id.empty());
    EXPECT_TRUE(chain.argument_ids.empty());
    
    chain.id = "chain_001";
    chain.argument_ids = {"arg_1", "arg_2", "arg_3"};
    chain.coherence_score = 0.75;
    
    EXPECT_EQ("chain_001", chain.id);
    EXPECT_EQ(3u, chain.argument_ids.size());
    EXPECT_EQ(0.75, chain.coherence_score);
}

TEST(EthicsAITypes, DebateInitializationCreation) {
    DebateInitialization debate;
    EXPECT_TRUE(debate.debate_id.empty());
    EXPECT_TRUE(debate.dilemma_description.empty());
    EXPECT_TRUE(debate.philosophy_schools.empty());
    
    debate.debate_id = "debate_001";
    debate.dilemma_description = "Test dilemma";
    debate.philosophy_schools = {"kant", "utilitarianism"};
    debate.category = "bioethics";
    
    EXPECT_EQ("debate_001", debate.debate_id);
    EXPECT_EQ("Test dilemma", debate.dilemma_description);
    EXPECT_EQ(2u, debate.philosophy_schools.size());
    EXPECT_EQ("bioethics", debate.category);
}

// ========== Plugin Interface Tests (Structure Only) ==========

// Note: These tests verify the interface exists and can be compiled
// Actual plugin functionality tests would require loading the plugin

TEST(EthicsAIPlugin, InterfaceExists) {
    // Just verify that the interface can be referenced
    // This is a compile-time check more than a runtime test
    EXPECT_TRUE(true);
}

// ========== Main ==========