#include <gtest/gtest.h>
#include "llm/explanation_generator.h"

using namespace themis::llm;
using json = nlohmann::json;

class ExplanationGeneratorTest : public ::testing::Test {
protected:
    ExplanationGenerator generator_;
    
    std::string test_query_ = "What is the capital of France?";
    std::string test_response_ = "Paris is the capital of France.";
    std::vector<std::string> test_reasoning_ = {
        "Analyzed query as geographical question",
        "Identified country: France",
        "Retrieved capital from knowledge base",
        "Formatted response"
    };
    json test_factors_;
    
    void SetUp() override {
        test_factors_ = json::object();
        test_factors_["query_type"] = "factual";
        test_factors_["confidence"] = 0.95;
        test_factors_["knowledge_base"] = "geography";
    }
};

TEST_F(ExplanationGeneratorTest, GenerateUserFriendlyExplanation) {
    std::string explanation = generator_.generateExplanation(
        test_query_,
        test_response_,
        test_reasoning_,
        test_factors_,
        ExplanationGenerator::Format::USER_FRIENDLY
    );
    
    EXPECT_FALSE(explanation.empty());
    EXPECT_NE(explanation.find("Your question"), std::string::npos);
    EXPECT_NE(explanation.find("My thinking process"), std::string::npos);
    EXPECT_NE(explanation.find(test_query_), std::string::npos);
}

TEST_F(ExplanationGeneratorTest, GenerateTechnicalExplanation) {
    std::string explanation = generator_.generateExplanation(
        test_query_,
        test_response_,
        test_reasoning_,
        test_factors_,
        ExplanationGenerator::Format::TECHNICAL
    );
    
    EXPECT_FALSE(explanation.empty());
    EXPECT_NE(explanation.find("Technical Analysis"), std::string::npos);
    EXPECT_NE(explanation.find("INPUT QUERY"), std::string::npos);
    EXPECT_NE(explanation.find("REASONING CHAIN"), std::string::npos);
    EXPECT_NE(explanation.find("KEY DECISION FACTORS"), std::string::npos);
}

TEST_F(ExplanationGeneratorTest, GenerateComplianceExplanation) {
    std::string explanation = generator_.generateExplanation(
        test_query_,
        test_response_,
        test_reasoning_,
        test_factors_,
        ExplanationGenerator::Format::COMPLIANCE
    );
    
    EXPECT_FALSE(explanation.empty());
    EXPECT_NE(explanation.find("GDPR Article 22"), std::string::npos);
    EXPECT_NE(explanation.find("EU AI Act"), std::string::npos);
    EXPECT_NE(explanation.find("YOUR RIGHTS"), std::string::npos);
    EXPECT_NE(explanation.find("Request human review"), std::string::npos);
}

TEST_F(ExplanationGeneratorTest, GenerateJsonExplanation) {
    std::string explanation = generator_.generateExplanation(
        test_query_,
        test_response_,
        test_reasoning_,
        test_factors_,
        ExplanationGenerator::Format::JSON
    );
    
    EXPECT_FALSE(explanation.empty());
    
    // Parse as JSON
    json j = json::parse(explanation);
    EXPECT_TRUE(j.contains("query"));
    EXPECT_TRUE(j.contains("response"));
    EXPECT_TRUE(j.contains("reasoning_steps"));
    EXPECT_TRUE(j.contains("key_factors"));
    EXPECT_EQ(j["query"], test_query_);
}

TEST_F(ExplanationGeneratorTest, GenerateReasoningChain) {
    json intermediate = json::object();
    intermediate["parsed_intent"] = "geographical_query";
    intermediate["entity_extracted"] = "France";
    intermediate["fact_retrieved"] = "capital: Paris";
    
    auto reasoning = generator_.generateReasoningChain(test_query_, intermediate);
    
    EXPECT_FALSE(reasoning.empty());
    EXPECT_GE(reasoning.size(), 3); // At least query analysis, processing, response generation
    
    // Should contain some reference to the intermediate results
    bool found_processing = false;
    for (const auto& step : reasoning) {
        if (step.find("Processed") != std::string::npos) {
            found_processing = true;
            break;
        }
    }
    EXPECT_TRUE(found_processing);
}

TEST_F(ExplanationGeneratorTest, IdentifyKeyFactors) {
    json context;
    context["user_location"] = "Europe";
    context["previous_queries"] = 3;
    
    auto factors = generator_.identifyKeyFactors(
        test_query_,
        test_response_,
        context
    );
    
    EXPECT_FALSE(factors.empty());
    EXPECT_TRUE(factors.is_object());
    
    // Should identify query-response similarity
    EXPECT_TRUE(factors.contains("query_response_similarity"));
    
    // Should classify query complexity
    EXPECT_TRUE(factors.contains("query_complexity"));
    
    // Should include context
    EXPECT_TRUE(factors.contains("context_user_location"));
}

TEST_F(ExplanationGeneratorTest, IdentifyCommonKeywords) {
    std::string query = "What is the capital city of France in Europe?";
    std::string response = "Paris is the capital city of France.";
    
    auto factors = generator_.identifyKeyFactors(query, response);
    
    EXPECT_TRUE(factors.contains("query_terms_used"));
    
    auto terms = factors["query_terms_used"];
    EXPECT_TRUE(terms.is_array());
    
    // Common terms should include "capital" and "france"
    bool found_capital = false;
    bool found_france = false;
    for (const auto& term : terms) {
        std::string t = term.get<std::string>();
        if (t == "capital") found_capital = true;
        if (t == "france") found_france = true;
    }
    EXPECT_TRUE(found_capital || found_france);
}

TEST_F(ExplanationGeneratorTest, ExplainHighConfidence) {
    std::string explanation = generator_.explainConfidence(0.95f);
    
    EXPECT_FALSE(explanation.empty());
    EXPECT_NE(explanation.find("95%"), std::string::npos);
    EXPECT_NE(explanation.find("high-confidence"), std::string::npos);
    EXPECT_NE(explanation.find("very certain"), std::string::npos);
}

TEST_F(ExplanationGeneratorTest, ExplainMediumConfidence) {
    std::string explanation = generator_.explainConfidence(0.75f);
    
    EXPECT_FALSE(explanation.empty());
    EXPECT_NE(explanation.find("75%"), std::string::npos);
    EXPECT_NE(explanation.find("moderate-confidence"), std::string::npos);
}

TEST_F(ExplanationGeneratorTest, ExplainLowConfidence) {
    std::string explanation = generator_.explainConfidence(0.55f);
    
    EXPECT_FALSE(explanation.empty());
    EXPECT_NE(explanation.find("55%"), std::string::npos);
    EXPECT_NE(explanation.find("low-confidence"), std::string::npos);
    EXPECT_NE(explanation.find("Human review is recommended"), std::string::npos);
}

TEST_F(ExplanationGeneratorTest, ExplainVeryLowConfidence) {
    std::string explanation = generator_.explainConfidence(0.3f);
    
    EXPECT_FALSE(explanation.empty());
    EXPECT_NE(explanation.find("very low-confidence"), std::string::npos);
    EXPECT_NE(explanation.find("strongly recommended"), std::string::npos);
}

TEST_F(ExplanationGeneratorTest, ExplainConfidenceWithAlternatives) {
    std::vector<std::string> alternatives = {
        "Lyon is the capital of France",
        "Marseille is the capital of France",
        "Nice is the capital of France"
    };
    
    std::string explanation = generator_.explainConfidence(0.8f, alternatives);
    
    EXPECT_FALSE(explanation.empty());
    EXPECT_NE(explanation.find("Alternative responses"), std::string::npos);
    EXPECT_NE(explanation.find("Lyon"), std::string::npos);
}

TEST_F(ExplanationGeneratorTest, GenerateFullComplianceExplanation) {
    std::string model_info = "TestModel v1.0";
    float confidence = 0.88f;
    
    std::string explanation = generator_.generateComplianceExplanation(
        test_query_,
        test_response_,
        model_info,
        test_reasoning_,
        test_factors_,
        confidence
    );
    
    EXPECT_FALSE(explanation.empty());
    
    // Check all required sections
    EXPECT_NE(explanation.find("SECTION 1: SYSTEM IDENTIFICATION"), std::string::npos);
    EXPECT_NE(explanation.find("SECTION 2: INPUT"), std::string::npos);
    EXPECT_NE(explanation.find("SECTION 3: DECISION LOGIC"), std::string::npos);
    EXPECT_NE(explanation.find("SECTION 4: INFLUENCING FACTORS"), std::string::npos);
    EXPECT_NE(explanation.find("SECTION 5: OUTPUT"), std::string::npos);
    EXPECT_NE(explanation.find("SECTION 6: REGULATORY COMPLIANCE"), std::string::npos);
    EXPECT_NE(explanation.find("SECTION 7: USER RIGHTS"), std::string::npos);
    
    // Check compliance references
    EXPECT_NE(explanation.find("GDPR Article 22"), std::string::npos);
    EXPECT_NE(explanation.find("EU AI Act"), std::string::npos);
    EXPECT_NE(explanation.find("eIDAS Regulation"), std::string::npos);
    
    // Check model info
    EXPECT_NE(explanation.find(model_info), std::string::npos);
    EXPECT_NE(explanation.find("88"), std::string::npos); // Confidence percentage
}

TEST_F(ExplanationGeneratorTest, ComplianceExplanationFlagsLowConfidence) {
    std::string model_info = "TestModel v1.0";
    float low_confidence = 0.6f;
    
    std::string explanation = generator_.generateComplianceExplanation(
        test_query_,
        test_response_,
        model_info,
        test_reasoning_,
        test_factors_,
        low_confidence
    );
    
    // Should include warning about low confidence
    EXPECT_NE(explanation.find("NOTICE"), std::string::npos);
    EXPECT_NE(explanation.find("lower confidence"), std::string::npos);
    EXPECT_NE(explanation.find("flagged"), std::string::npos);
}

TEST_F(ExplanationGeneratorTest, EmptyReasoningStepsHandled) {
    std::vector<std::string> empty_reasoning;
    
    std::string explanation = generator_.generateExplanation(
        test_query_,
        test_response_,
        empty_reasoning,
        test_factors_,
        ExplanationGenerator::Format::USER_FRIENDLY
    );
    
    EXPECT_FALSE(explanation.empty());
    // Should still generate explanation even without reasoning steps
    EXPECT_NE(explanation.find("Your question"), std::string::npos);
}

TEST_F(ExplanationGeneratorTest, EmptyKeyFactorsHandled) {
    json empty_factors = json::object();
    
    std::string explanation = generator_.generateExplanation(
        test_query_,
        test_response_,
        test_reasoning_,
        empty_factors,
        ExplanationGenerator::Format::TECHNICAL
    );
    
    EXPECT_FALSE(explanation.empty());
    // Should still generate explanation even without key factors
    EXPECT_NE(explanation.find("Technical Analysis"), std::string::npos);
}

TEST_F(ExplanationGeneratorTest, QueryComplexityClassification) {
    // Simple query
    auto factors1 = generator_.identifyKeyFactors("What?", "Answer");
    EXPECT_EQ(factors1["query_complexity"], "simple");
    
    // Moderate query
    auto factors2 = generator_.identifyKeyFactors(
        "What is the capital of France and Germany?",
        "Paris and Berlin"
    );
    EXPECT_EQ(factors2["query_complexity"], "simple");
    
    // Complex query
    auto factors3 = generator_.identifyKeyFactors(
        "What are the historical, economic, and cultural factors that led to Paris "
        "becoming the capital of France in the medieval period?",
        "Complex answer"
    );
    EXPECT_EQ(factors3["query_complexity"], "moderate");
}
