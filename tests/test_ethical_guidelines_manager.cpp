#include "llm/ethical_guidelines_manager.h"
#include <gtest/gtest.h>
#include <fstream>

namespace themis {
namespace llm {
namespace test {

class EthicalGuidelinesManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test will use the actual config file
        manager_ = std::make_unique<EthicalGuidelinesManager>(
            "config/ethical_guidelines.yaml"
        );
    }
    
    std::unique_ptr<EthicalGuidelinesManager> manager_;
};

TEST_F(EthicalGuidelinesManagerTest, LoadConfiguration) {
    ASSERT_TRUE(manager_ != nullptr);
    EXPECT_TRUE(manager_->isEnabled());
    
    // Check that core principles were loaded
    auto principles = manager_->getPrinciples();
    EXPECT_GT(principles.size(), 0);
    
    // Check that domain guidelines were loaded
    auto domains = manager_->getDomainGuidelines();
    EXPECT_GT(domains.size(), 0);
}

TEST_F(EthicalGuidelinesManagerTest, DetectEthicalContextGerman) {
    std::string text = "Ist es ethisch vertretbar, dass ich diese Entscheidung treffe? "
                      "Was ist meine moralische Pflicht in dieser Situation?";
    
    auto result = manager_->detectEthicalContext(text, "de");
    
    EXPECT_TRUE(result.has_ethical_context);
    EXPECT_GT(result.confidence, 0.6f);
    EXPECT_GT(result.detected_keywords.size(), 0);
    EXPECT_FALSE(result.recommended_augmentation.empty());
}

TEST_F(EthicalGuidelinesManagerTest, DetectEthicalContextEnglish) {
    std::string text = "Is this ethical? What is my moral duty in this situation?";
    
    auto result = manager_->detectEthicalContext(text, "en");
    
    EXPECT_TRUE(result.has_ethical_context);
    EXPECT_GT(result.confidence, 0.6f);
    EXPECT_GT(result.detected_keywords.size(), 0);
}

TEST_F(EthicalGuidelinesManagerTest, DetectMoralImperative) {
    std::string text = "Was ist meine moralische Pflicht? Was soll ich tun? "
                      "Es ist ein kategorischer Imperativ.";
    
    auto result = manager_->detectEthicalContext(text, "de");
    
    EXPECT_TRUE(result.has_ethical_context);
    EXPECT_GT(result.confidence, 0.7f);
    EXPECT_EQ(result.recommended_augmentation, "moral_imperatives");
}

TEST_F(EthicalGuidelinesManagerTest, DetectMedicalDomain) {
    std::string text = "Ich habe gesundheitliche Probleme. Soll ich zum Arzt gehen?";
    
    auto result = manager_->detectEthicalContext(text, "de");
    
    EXPECT_TRUE(result.has_ethical_context);
    EXPECT_GT(result.detected_domains.size(), 0);
    
    // Should detect medical domain
    bool found_medical = false;
    for (const auto& domain : result.detected_domains) {
        if (domain == "medical") {
            found_medical = true;
            break;
        }
    }
    EXPECT_TRUE(found_medical);
}

TEST_F(EthicalGuidelinesManagerTest, DetectLegalDomain) {
    std::string text = "Ich brauche rechtliche Beratung. Sollte ich einen Anwalt konsultieren?";
    
    auto result = manager_->detectEthicalContext(text, "de");
    
    EXPECT_TRUE(result.has_ethical_context);
    
    // Should detect legal domain
    bool found_legal = false;
    for (const auto& domain : result.detected_domains) {
        if (domain == "legal") {
            found_legal = true;
            break;
        }
    }
    EXPECT_TRUE(found_legal);
}

TEST_F(EthicalGuidelinesManagerTest, NoEthicalContext) {
    std::string text = "What is the weather like today? I need to buy groceries.";
    
    auto result = manager_->detectEthicalContext(text, "en");
    
    EXPECT_FALSE(result.has_ethical_context);
    EXPECT_LT(result.confidence, 0.6f);
}

TEST_F(EthicalGuidelinesManagerTest, AugmentPrompt) {
    std::string original = "You are a helpful assistant.";
    
    EthicalGuidelinesManager::DetectionResult result;
    result.has_ethical_context = true;
    result.confidence = 0.8f;
    result.recommended_augmentation = "default";
    
    std::string augmented = manager_->augmentPrompt(original, result);
    
    EXPECT_GT(augmented.length(), original.length());
    EXPECT_NE(augmented.find("Menschenrechte"), std::string::npos);
    EXPECT_NE(augmented.find("Human Rights"), std::string::npos);
}

TEST_F(EthicalGuidelinesManagerTest, AugmentPromptMoralImperative) {
    std::string original = "You are a helpful assistant.";
    
    EthicalGuidelinesManager::DetectionResult result;
    result.has_ethical_context = true;
    result.confidence = 0.9f;
    result.recommended_augmentation = "moral_imperatives";
    
    std::string augmented = manager_->augmentPrompt(original, result);
    
    EXPECT_GT(augmented.length(), original.length());
    EXPECT_NE(augmented.find("MORALISCHE IMPERATIVE"), std::string::npos);
    EXPECT_NE(augmented.find("Asimov"), std::string::npos);
    EXPECT_NE(augmented.find("KANTISCHE ETHIK"), std::string::npos);
}

TEST_F(EthicalGuidelinesManagerTest, AugmentResponse) {
    std::string response = "Here is my answer to your question.";
    
    EthicalGuidelinesManager::DetectionResult result;
    result.has_ethical_context = true;
    result.confidence = 0.8f;
    result.recommended_augmentation = "high_autonomy";
    
    std::string augmented = manager_->augmentResponse(response, result);
    
    EXPECT_GT(augmented.length(), response.length());
    EXPECT_NE(augmented.find("HINWEIS"), std::string::npos);
}

TEST_F(EthicalGuidelinesManagerTest, DetectInRAG) {
    std::vector<std::string> documents = {
        "This document discusses ethical considerations in AI.",
        "Another document about data privacy and moral responsibilities.",
        "A third document about technical specifications."
    };
    
    std::string query = "What should I do ethically?";
    
    auto result = manager_->detectEthicalContextInRAG(documents, query);
    
    EXPECT_TRUE(result.has_ethical_context);
    EXPECT_GT(result.detected_keywords.size(), 0);
}

TEST_F(EthicalGuidelinesManagerTest, Statistics) {
    // Reset statistics
    manager_->resetStatistics();
    
    auto stats_before = manager_->getStatistics();
    EXPECT_EQ(stats_before.total_detections, 0);
    
    // Perform some detections
    manager_->detectEthicalContext("Is this ethical?", "en");
    manager_->detectEthicalContext("What is my moral duty?", "en");
    
    auto stats_after = manager_->getStatistics();
    EXPECT_EQ(stats_after.total_detections, 2);
    EXPECT_EQ(stats_after.ethical_contexts_found, 2);
}

TEST_F(EthicalGuidelinesManagerTest, HumanRightsFoundation) {
    // Verify that Human Rights are referenced in augmentation
    std::string original = "System prompt";
    
    EthicalGuidelinesManager::DetectionResult result;
    result.has_ethical_context = true;
    result.confidence = 0.9f;
    result.recommended_augmentation = "default";
    
    std::string augmented = manager_->augmentPrompt(original, result);
    
    // Should reference Universal Declaration of Human Rights
    EXPECT_NE(augmented.find("Menschenrechte"), std::string::npos);
    EXPECT_NE(augmented.find("Human Rights"), std::string::npos);
    EXPECT_NE(augmented.find("Art. 1"), std::string::npos);
}

TEST_F(EthicalGuidelinesManagerTest, AsimovLawsFoundation) {
    // Verify that Asimov's Laws are referenced
    std::string original = "System prompt";
    
    EthicalGuidelinesManager::DetectionResult result;
    result.has_ethical_context = true;
    result.confidence = 0.9f;
    result.recommended_augmentation = "moral_imperatives";
    
    std::string augmented = manager_->augmentPrompt(original, result);
    
    // Should reference Asimov's Laws
    EXPECT_NE(augmented.find("Asimov"), std::string::npos);
    EXPECT_NE(augmented.find("Second Law"), std::string::npos) 
        << "Should reference Asimov's Second Law (autonomy)";
}

} // namespace test
} // namespace llm
} // namespace themis
