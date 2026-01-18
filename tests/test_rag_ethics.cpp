#include <gtest/gtest.h>
#include "rag/rag_judge.h"
#include "rag/knowledge_gap_detector.h"

using namespace themis::rag::judge;
using namespace themis::rag::knowledge_gap;

// ============================================================================
// Ethical Compliance Tests
// ============================================================================

class RAGEthicsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create judge with ethical evaluation enabled
        RAGJudgeConfig config;
        config.enable_ethical_evaluation = true;
        config.ethical_veto_power = true;
        config.ethical_compliance_threshold = 0.7;
        judge_ = std::make_unique<RAGJudge>(config);
        
        // Create gap detector with ethical gap detection enabled
        KnowledgeGapConfig gap_config;
        gap_config.enable_ethical_gap_detection = true;
        gap_config.min_ethical_perspectives = 2;
        gap_detector_ = std::make_unique<KnowledgeGapDetector>(gap_config);
    }
    
    std::unique_ptr<RAGJudge> judge_;
    std::unique_ptr<KnowledgeGapDetector> gap_detector_;
};

// Test patronizing language detection
TEST_F(RAGEthicsTest, DetectPatronizingLanguage) {
    EvaluationInput input;
    input.query = "Should I vaccinate my child?";
    input.generated_answer = "Obviously, you should know that vaccination is important. "
                            "It's simple - just do it. Anyone can understand this.";
    
    auto result = judge_->evaluate(input);
    
    // Patronizing language should reduce ethical compliance score
    EXPECT_LT(result.ethical_compliance_score, 0.9)
        << "Patronizing language should reduce ethical compliance score";
    EXPECT_FALSE(result.respects_human_autonomy)
        << "Patronizing language violates autonomy respect";
}

TEST_F(RAGEthicsTest, NoPatronizingLanguage) {
    EvaluationInput input;
    input.query = "Should I vaccinate my child?";
    input.generated_answer = "Vaccination is an important consideration for child health. "
                            "Medical experts generally recommend childhood vaccinations based on "
                            "extensive research. However, you may want to discuss your specific "
                            "situation with your healthcare provider.";
    
    auto result = judge_->evaluate(input);
    
    // No patronizing language should have better score
    EXPECT_GT(result.ethical_compliance_score, 0.7)
        << "Respectful language should have good ethical compliance";
}

// Test choice preservation
TEST_F(RAGEthicsTest, CheckChoicePreservation) {
    EvaluationInput input;
    input.query = "What diet should I follow?";
    input.generated_answer = "You must follow a vegan diet. You have to avoid all animal products. "
                            "The only way to be healthy is to eat plants only.";
    
    auto result = judge_->evaluate(input);
    
    // Forcing opinions should reduce ethical compliance
    EXPECT_LT(result.ethical_compliance_score, 0.8)
        << "Forced opinions should reduce ethical compliance score";
}

TEST_F(RAGEthicsTest, PreservesChoice) {
    EvaluationInput input;
    input.query = "What diet should I follow?";
    input.generated_answer = "There are several dietary approaches to consider, including "
                            "vegetarian, vegan, Mediterranean, or balanced omnivorous diets. "
                            "Each has different benefits and considerations. You might want to "
                            "consider your health goals, ethical values, and lifestyle when choosing.";
    
    auto result = judge_->evaluate(input);
    
    // Preserving choice should have better score
    EXPECT_GT(result.ethical_compliance_score, 0.7)
        << "Choice-preserving language should have good ethical compliance";
    EXPECT_TRUE(result.respects_human_autonomy)
        << "Choice preservation should respect autonomy";
}

// Test moral perspective counting
TEST_F(RAGEthicsTest, CountMoralPerspectives) {
    EvaluationInput input;
    input.query = "Is it ethical to eat meat?";
    input.generated_answer = "From a utilitarian perspective, the suffering of animals must be "
                            "weighed against human benefits. However, a rights-based approach "
                            "argues that animals have inherent rights that should be respected. "
                            "Virtue ethics considers what kind of character we develop through "
                            "our food choices.";
    
    auto result = judge_->evaluate(input);
    
    // Multiple perspectives should have high ethical compliance
    EXPECT_GT(result.ethical_compliance_score, 0.8)
        << "Multiple moral perspectives should increase ethical compliance";
    EXPECT_TRUE(result.shows_moral_diversity)
        << "Multiple frameworks should show moral diversity";
}

TEST_F(RAGEthicsTest, LackOfMoralDiversity) {
    EvaluationInput input;
    input.query = "Is it ethical to eat meat?";
    input.generated_answer = "Eating meat is clearly wrong because animals suffer. "
                            "There is no justification for causing animal suffering.";
    
    auto result = judge_->evaluate(input);
    
    // Single perspective should reduce ethical compliance
    EXPECT_LT(result.ethical_compliance_score, 0.9)
        << "Lack of perspective diversity should reduce ethical compliance";
}

// Test bias detection
TEST_F(RAGEthicsTest, DetectBias) {
    EvaluationInput input;
    input.query = "Should we implement universal healthcare?";
    input.generated_answer = "Universal healthcare is always the best solution. Everyone agrees "
                            "that it works perfectly. No one can deny its absolute benefits. "
                            "It never fails and certainly solves all problems.";
    
    auto result = judge_->evaluate(input);
    
    // Biased language should reduce ethical compliance
    EXPECT_LT(result.ethical_compliance_score, 0.9)
        << "Biased absolute statements should reduce ethical compliance";
}

TEST_F(RAGEthicsTest, BalancedNuancedView) {
    EvaluationInput input;
    input.query = "Should we implement universal healthcare?";
    input.generated_answer = "Universal healthcare systems have both strengths and challenges. "
                            "Some countries have implemented successful systems, while others "
                            "have faced difficulties. The effectiveness often depends on specific "
                            "implementation details and national contexts.";
    
    auto result = judge_->evaluate(input);
    
    // Nuanced view should have better ethical compliance
    EXPECT_GT(result.ethical_compliance_score, 0.7)
        << "Balanced nuanced view should have good ethical compliance";
}

// Test citation quality
TEST_F(RAGEthicsTest, EthicalClaimsWithCitations) {
    EvaluationInput input;
    input.query = "Do humans have a right to privacy?";
    input.generated_answer = "According to Article 12 of the Universal Declaration of Human Rights, "
                            "individuals have the right to privacy. This principle is also "
                            "referenced in various national constitutions and legal frameworks.";
    
    auto result = judge_->evaluate(input);
    
    // Citations for ethical claims should increase compliance
    EXPECT_GT(result.ethical_compliance_score, 0.8)
        << "Ethical claims with citations should have high compliance";
    EXPECT_TRUE(result.has_ethical_citations)
        << "Should detect presence of ethical citations";
}

TEST_F(RAGEthicsTest, EthicalClaimsWithoutCitations) {
    EvaluationInput input;
    input.query = "Do humans have a right to privacy?";
    input.generated_answer = "Humans have a fundamental right to privacy. "
                            "This is a basic moral principle that must be respected.";
    
    auto result = judge_->evaluate(input);
    
    // Missing citations should reduce compliance
    EXPECT_LT(result.ethical_compliance_score, 0.9)
        << "Ethical claims without citations should reduce compliance";
}

// Test VETO mechanism
TEST_F(RAGEthicsTest, EthicalVetoTriggered) {
    EvaluationInput input;
    input.query = "Should I get an abortion?";
    input.generated_answer = "You must not get an abortion. Obviously, it's always wrong. "
                            "Everyone knows this is the only correct answer.";
    
    auto result = judge_->evaluate(input);
    
    // VETO should be triggered for low ethical compliance
    EXPECT_FALSE(result.passed_quality_threshold)
        << "Ethical VETO should fail quality threshold";
    EXPECT_LT(result.ethical_compliance_score, 0.7)
        << "Ethical compliance should be below threshold";
    EXPECT_FALSE(result.ethical_violations.empty())
        << "Should report ethical violations";
}

TEST_F(RAGEthicsTest, EthicalVetoNotTriggered) {
    EvaluationInput input;
    input.query = "Should I get an abortion?";
    input.generated_answer = "This is a deeply personal decision that involves multiple "
                            "considerations including health, personal circumstances, and values. "
                            "Different ethical frameworks offer different perspectives on this issue. "
                            "According to medical ethics, the decision should be made by the individual "
                            "in consultation with healthcare providers. You may want to discuss your "
                            "specific situation with trusted medical professionals and advisors.";
    
    auto result = judge_->evaluate(input);
    
    // Should pass ethical compliance
    EXPECT_GE(result.ethical_compliance_score, 0.7)
        << "Ethical compliance should meet threshold";
    EXPECT_TRUE(result.respects_human_autonomy)
        << "Should respect human autonomy";
    EXPECT_TRUE(result.shows_moral_diversity)
        << "Should show moral diversity";
}

// ============================================================================
// Ethical Perspective Gap Detection Tests
// ============================================================================

TEST_F(RAGEthicsTest, DetectEthicalQuery) {
    std::string ethical_query = "Is it morally right to use AI for surveillance?";
    std::vector<RetrievedDocument> docs;
    
    auto result = gap_detector_->detectEthicalPerspectiveGap(ethical_query, docs);
    
    // Should detect as ethical query even with empty docs
    EXPECT_TRUE(result.gap_detected)
        << "Should detect gap for ethical query with no documents";
    EXPECT_EQ(result.gap_type, GapType::ETHICAL_PERSPECTIVE_GAP)
        << "Gap type should be ethical perspective gap";
}

TEST_F(RAGEthicsTest, NonEthicalQuery) {
    std::string non_ethical_query = "What is the capital of France?";
    std::vector<RetrievedDocument> docs;
    
    auto result = gap_detector_->detectEthicalPerspectiveGap(non_ethical_query, docs);
    
    // Should not detect ethical gap for non-ethical query
    EXPECT_FALSE(result.gap_detected)
        << "Should not detect gap for non-ethical query";
    EXPECT_EQ(result.gap_type, GapType::NONE)
        << "Gap type should be NONE";
}

TEST_F(RAGEthicsTest, EthicalGapWithInsufficientPerspectives) {
    std::string ethical_query = "What is the right approach to climate change ethics?";
    
    // Document with only one perspective
    std::vector<RetrievedDocument> docs;
    RetrievedDocument doc1;
    doc1.id = "doc1";
    doc1.content = "From a utilitarian perspective, we should maximize overall "
                  "welfare by reducing carbon emissions.";
    doc1.similarity_score = 0.9;
    docs.push_back(doc1);
    
    auto result = gap_detector_->detectEthicalPerspectiveGap(ethical_query, docs);
    
    // Should detect gap due to insufficient perspectives
    EXPECT_TRUE(result.gap_detected)
        << "Should detect gap with insufficient ethical perspectives";
    EXPECT_EQ(result.gap_type, GapType::ETHICAL_PERSPECTIVE_GAP);
    EXPECT_EQ(result.recommendation, FallbackStrategy::EXPAND_SEARCH)
        << "Should recommend expanding search";
}

TEST_F(RAGEthicsTest, EthicalGapWithSufficientPerspectives) {
    std::string ethical_query = "What are the ethical considerations of AI?";
    
    // Documents with multiple perspectives
    std::vector<RetrievedDocument> docs;
    
    RetrievedDocument doc1;
    doc1.id = "doc1";
    doc1.content = "From a utilitarian perspective, AI should maximize overall "
                  "happiness and minimize suffering across society.";
    doc1.similarity_score = 0.9;
    docs.push_back(doc1);
    
    RetrievedDocument doc2;
    doc2.id = "doc2";
    doc2.content = "A deontological approach emphasizes the duty to respect "
                  "human autonomy and dignity when developing AI systems.";
    doc2.similarity_score = 0.85;
    docs.push_back(doc2);
    
    RetrievedDocument doc3;
    doc3.id = "doc3";
    doc3.content = "Virtue ethics considers what character traits AI developers "
                  "should cultivate, such as responsibility and wisdom.";
    doc3.similarity_score = 0.8;
    docs.push_back(doc3);
    
    auto result = gap_detector_->detectEthicalPerspectiveGap(ethical_query, docs);
    
    // Should not detect gap with sufficient perspectives
    EXPECT_FALSE(result.gap_detected)
        << "Should not detect gap with sufficient ethical perspectives";
    EXPECT_EQ(result.gap_type, GapType::NONE);
}

TEST_F(RAGEthicsTest, PerspectiveDiversityCalculation) {
    std::string ethical_query = "Is capital punishment ethical?";
    
    // Documents with diverse perspectives
    std::vector<RetrievedDocument> docs;
    
    RetrievedDocument doc1;
    doc1.id = "doc1";
    doc1.content = "Utilitarian arguments suggest capital punishment may deter crime.";
    doc1.similarity_score = 0.9;
    docs.push_back(doc1);
    
    RetrievedDocument doc2;
    doc2.id = "doc2";
    doc2.content = "From a rights-based perspective, capital punishment violates "
                  "the right to life.";
    doc2.similarity_score = 0.85;
    docs.push_back(doc2);
    
    RetrievedDocument doc3;
    doc3.id = "doc3";
    doc3.content = "Religious traditions have varied views on capital punishment.";
    doc3.similarity_score = 0.8;
    docs.push_back(doc3);
    
    auto result = gap_detector_->detectEthicalPerspectiveGap(ethical_query, docs);
    
    // Should have high diversity score
    EXPECT_FALSE(result.gap_detected);
    EXPECT_GT(result.coverage_score, 0.6)
        << "Perspective diversity score should be high";
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(RAGEthicsTest, EndToEndEthicalEvaluation) {
    // Ethical query with good documents and response
    std::string query = "What are the ethical implications of genetic engineering?";
    
    std::vector<RetrievedDocument> docs;
    RetrievedDocument doc1;
    doc1.content = "Utilitarian ethics weighs the benefits of genetic engineering "
                  "against potential harms.";
    docs.push_back(doc1);
    
    RetrievedDocument doc2;
    doc2.content = "Deontological perspectives emphasize respect for human dignity "
                  "and natural rights in genetic engineering debates.";
    docs.push_back(doc2);
    
    std::string answer = "Genetic engineering raises several ethical considerations. "
                        "From a utilitarian perspective, it could reduce disease and suffering. "
                        "However, rights-based approaches emphasize concerns about human dignity "
                        "and consent. According to bioethics guidelines, such interventions "
                        "should respect individual autonomy and be carefully regulated.";
    
    EvaluationInput input;
    input.query = query;
    input.documents = docs;
    input.generated_answer = answer;
    
    // Check gap detection first
    auto gap_result = gap_detector_->detectEthicalPerspectiveGap(query, docs);
    EXPECT_FALSE(gap_result.gap_detected)
        << "Should not detect gap with sufficient perspectives";
    
    // Evaluate response
    auto eval_result = judge_->evaluate(input);
    
    EXPECT_GT(eval_result.ethical_compliance_score, 0.7)
        << "Should have good ethical compliance";
    EXPECT_TRUE(eval_result.passed_quality_threshold)
        << "Should pass quality threshold";
    EXPECT_TRUE(eval_result.respects_human_autonomy);
    EXPECT_TRUE(eval_result.shows_moral_diversity);
    EXPECT_TRUE(eval_result.has_ethical_citations);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
