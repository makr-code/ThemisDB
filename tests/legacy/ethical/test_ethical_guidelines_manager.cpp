#include "llm/ethical_guidelines_manager.h"
#include "ethics_ai/ethics_ai_types.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

namespace themis {
namespace llm {
namespace test {

class EthicalGuidelinesManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test will use the actual config file
        if (!std::filesystem::exists("config/ethical_guidelines.yaml")) {
            GTEST_SKIP() << "Missing ethical guidelines config file";
        }

        manager_ = std::make_unique<EthicalGuidelinesManager>(
            "config/ethical_guidelines.yaml"
        );

        if (manager_->getPrinciples().empty() || manager_->getDomainGuidelines().empty()) {
            GTEST_SKIP() << "Ethical guidelines not loaded in this test environment";
        }
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

// ═══════════════════════════════════════════════════════════
// Plugin Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(EthicalGuidelinesManagerTest, RegisterPhilosophy) {
    // Create a test philosophy profile
    themis::plugins::ethics::PhilosophyProfile profile;
    profile.school_id = "test_philosophy";
    profile.name = "Test Philosophy School";
    profile.main_theses.push_back("Test thesis 1");
    profile.main_theses.push_back("Test thesis 2");
    
    // Register the philosophy
    bool success = manager_->registerPhilosophy("test_philosophy", profile);
    EXPECT_TRUE(success);
    
    // Verify it's in the registered list
    auto registered = manager_->getRegisteredPhilosophies();
    EXPECT_GT(registered.size(), 0);
    
    bool found = false;
    for (const auto& school_id : registered) {
        if (school_id == "test_philosophy") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(EthicalGuidelinesManagerTest, RegisterPhilosophyInvalidEmpty) {
    // Try to register with empty school_id
    themis::plugins::ethics::PhilosophyProfile profile;
    profile.name = "Test";
    
    bool success = manager_->registerPhilosophy("", profile);
    EXPECT_FALSE(success);
}

TEST_F(EthicalGuidelinesManagerTest, RegisterPhilosophyInvalidName) {
    // Try to register with empty name
    themis::plugins::ethics::PhilosophyProfile profile;
    profile.school_id = "test";
    profile.name = "";  // Empty name
    
    bool success = manager_->registerPhilosophy("test", profile);
    EXPECT_FALSE(success);
}

TEST_F(EthicalGuidelinesManagerTest, MergePhilosophies) {
    // Create multiple philosophy profiles
    std::map<std::string, themis::plugins::ethics::PhilosophyProfile> profiles;
    
    themis::plugins::ethics::PhilosophyProfile profile1;
    profile1.school_id = "stoicism";
    profile1.name = "Stoicism";
    profile1.main_theses.push_back("Accept what you cannot control");
    profiles["stoicism"] = profile1;
    
    themis::plugins::ethics::PhilosophyProfile profile2;
    profile2.school_id = "existentialism";
    profile2.name = "Existentialism";
    profile2.main_theses.push_back("Existence precedes essence");
    profiles["existentialism"] = profile2;
    
    // Merge profiles
    size_t count = manager_->mergePhilosophies(profiles);
    EXPECT_EQ(count, 2);
    
    // Verify both are registered
    auto registered = manager_->getRegisteredPhilosophies();
    EXPECT_GE(registered.size(), 2);
    
    bool found_stoicism = false;
    bool found_existentialism = false;
    for (const auto& school_id : registered) {
        if (school_id == "stoicism") {
          found_stoicism = true;
        }
        if (school_id == "existentialism") {
          found_existentialism = true;
        }
    }
    EXPECT_TRUE(found_stoicism);
    EXPECT_TRUE(found_existentialism);
}

TEST_F(EthicalGuidelinesManagerTest, GetRegisteredPhilosophies) {
    // Initially should be empty or have base philosophies
    auto registered_before = manager_->getRegisteredPhilosophies();
    size_t initial_count = registered_before.size();
    
    // Register a new philosophy
    themis::plugins::ethics::PhilosophyProfile profile;
    profile.school_id = "pragmatism";
    profile.name = "Pragmatism";
    profile.main_theses.push_back("Truth is what works");
    
    manager_->registerPhilosophy("pragmatism", profile);
    
    // Should now have one more
    auto registered_after = manager_->getRegisteredPhilosophies();
    EXPECT_EQ(registered_after.size(), initial_count + 1);
}

TEST_F(EthicalGuidelinesManagerTest, MinimalModeStillWorks) {
    // Test that manager works without any plugin registrations
    // This tests backward compatibility
    
    std::string text = "Is this ethical? What should I do?";
    auto result = manager_->detectEthicalContext(text, "en");
    
    // Should still detect ethical context
    EXPECT_TRUE(result.has_ethical_context);
    EXPECT_GT(result.confidence, 0.6f);
    
    // Should still augment prompts
    std::string original = "You are a helpful assistant.";
    std::string augmented = manager_->augmentPrompt(original, result);
    EXPECT_GT(augmented.length(), original.length());
}

} // namespace test
} // namespace llm
} // namespace themis
