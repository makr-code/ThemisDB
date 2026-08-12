#include "llm/ethical_guidelines_manager.h"
#include "ethics_ai/ethics_ai_types.h"
#include <gtest/gtest.h>
#include <memory>

namespace themis {
namespace test {

/**
 * @brief Integration test for Ethics AI Plugin with EthicalGuidelinesManager
 * 
 * This test validates that the plugin can successfully register philosophy
 * profiles with the base ethical guidelines system.
 */
class EthicsPluginIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create manager instance
        manager_ = std::make_unique<llm::EthicalGuidelinesManager>(
            "config/ethical_guidelines.yaml"
        );
    }
    
    std::unique_ptr<llm::EthicalGuidelinesManager> manager_;
};

TEST_F(EthicsPluginIntegrationTest, PluginCanRegisterPhilosophies) {
    // Simulate plugin registering philosophies
    std::map<std::string, plugins::ethics::PhilosophyProfile> plugin_profiles;
    
    // Create Kantian ethics profile
    plugins::ethics::PhilosophyProfile kant_profile;
    kant_profile.school_id = "kant";
    kant_profile.name = "Kantian Ethics";
    kant_profile.main_theses.push_back("Act only according to that maxim whereby you can, at the same time, will that it should become a universal law");
    kant_profile.main_theses.push_back("Treat humanity, whether in your own person or in the person of any other, never merely as a means to an end, but always at the same time as an end");
    kant_profile.decision_framework["categorical_imperative"] = "Test if action can be universalized";
    kant_profile.decision_framework["dignity"] = "Respect human dignity as end-in-itself";
    kant_profile.strengths.push_back("Universal moral principles");
    kant_profile.strengths.push_back("Respects human dignity");
    kant_profile.weaknesses.push_back("May be too rigid in edge cases");
    plugin_profiles["kant"] = kant_profile;
    
    // Create Utilitarian ethics profile
    plugins::ethics::PhilosophyProfile util_profile;
    util_profile.school_id = "utilitarianism";
    util_profile.name = "Utilitarianism";
    util_profile.main_theses.push_back("The greatest happiness for the greatest number");
    util_profile.main_theses.push_back("Actions are right in proportion as they tend to promote happiness");
    util_profile.decision_framework["happiness_calculus"] = "Calculate overall happiness/suffering";
    util_profile.decision_framework["consequences"] = "Evaluate outcomes for all affected parties";
    util_profile.strengths.push_back("Pragmatic and outcome-focused");
    util_profile.strengths.push_back("Considers all stakeholders");
    util_profile.weaknesses.push_back("Difficult to calculate all consequences");
    util_profile.weaknesses.push_back("May justify harm to minorities");
    plugin_profiles["utilitarianism"] = util_profile;
    
    // Merge profiles (simulating plugin initialization)
    size_t registered_count = manager_->mergePhilosophies(plugin_profiles);
    
    // Verify registration
    EXPECT_EQ(registered_count, 2);
    
    // Verify philosophies are registered
    auto registered = manager_->getRegisteredPhilosophies();
    EXPECT_GE(registered.size(), 2);
    
    bool found_kant = false;
    bool found_util = false;
    for (const auto& school_id : registered) {
        if (school_id == "kant") found_kant = true;
        if (school_id == "utilitarianism") found_util = true;
    }
    
    EXPECT_TRUE(found_kant) << "Kantian ethics should be registered";
    EXPECT_TRUE(found_util) << "Utilitarianism should be registered";
}

TEST_F(EthicsPluginIntegrationTest, MinimalBaseStillWorksWithoutPlugin) {
    // Test that the base system works without any plugin registrations
    std::string ethical_query = "Is it morally acceptable to lie to protect someone?";
    
    auto result = manager_->detectEthicalContext(ethical_query, "en");
    
    // Should detect ethical context even without plugin
    EXPECT_TRUE(result.has_ethical_context);
    EXPECT_GT(result.confidence, 0.6f);
    
    // Should be able to augment prompts
    std::string original_prompt = "You are a helpful AI assistant.";
    std::string augmented = manager_->augmentPrompt(original_prompt, result);
    
    EXPECT_GT(augmented.length(), original_prompt.length());
}

TEST_F(EthicsPluginIntegrationTest, PluginExtensionIsAdditive) {
    // Get initial count
    auto initial_philosophies = manager_->getRegisteredPhilosophies();
    size_t initial_count = initial_philosophies.size();
    
    // Add plugin philosophies
    std::map<std::string, plugins::ethics::PhilosophyProfile> plugin_profiles;
    
    plugins::ethics::PhilosophyProfile virtue_profile;
    virtue_profile.school_id = "virtue_ethics";
    virtue_profile.name = "Virtue Ethics";
    virtue_profile.main_theses.push_back("Cultivate virtuous character traits");
    virtue_profile.main_theses.push_back("Act as a virtuous person would act");
    plugin_profiles["virtue_ethics"] = virtue_profile;
    
    plugins::ethics::PhilosophyProfile care_profile;
    care_profile.school_id = "care_ethics";
    care_profile.name = "Ethics of Care";
    care_profile.main_theses.push_back("Emphasize relationships and care");
    care_profile.main_theses.push_back("Context-sensitive moral reasoning");
    plugin_profiles["care_ethics"] = care_profile;
    
    size_t added = manager_->mergePhilosophies(plugin_profiles);
    EXPECT_EQ(added, 2);
    
    // Should have more philosophies now
    auto after_philosophies = manager_->getRegisteredPhilosophies();
    EXPECT_EQ(after_philosophies.size(), initial_count + 2);
}

TEST_F(EthicsPluginIntegrationTest, ThreadSafeRegistration) {
    // Test that concurrent registration is thread-safe
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([this, i, &success_count]() {
            plugins::ethics::PhilosophyProfile profile;
            profile.school_id = "philosophy_" + std::to_string(i);
            profile.name = "Test Philosophy " + std::to_string(i);
            profile.main_theses.push_back("Test thesis");
            
            if (manager_->registerPhilosophy(profile.school_id, profile)) {
                success_count++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All 10 should succeed
    EXPECT_EQ(success_count.load(), 10);
    
    // Verify all are registered
    auto registered = manager_->getRegisteredPhilosophies();
    EXPECT_GE(registered.size(), 10);
}

TEST_F(EthicsPluginIntegrationTest, DuplicateRegistrationUpdates) {
    // First registration
    plugins::ethics::PhilosophyProfile profile1;
    profile1.school_id = "test_phil";
    profile1.name = "Original Name";
    profile1.main_theses.push_back("Original thesis");
    
    bool first = manager_->registerPhilosophy("test_phil", profile1);
    EXPECT_TRUE(first);
    
    // Duplicate registration should update (not fail)
    plugins::ethics::PhilosophyProfile profile2;
    profile2.school_id = "test_phil";
    profile2.name = "Updated Name";
    profile2.main_theses.push_back("Updated thesis");
    
    bool second = manager_->registerPhilosophy("test_phil", profile2);
    EXPECT_TRUE(second);  // Should succeed (update)
    
    // Should still only have one entry
    auto registered = manager_->getRegisteredPhilosophies();
    int count = 0;
    for (const auto& school_id : registered) {
        if (school_id == "test_phil") {
            count++;
        }
    }
    EXPECT_EQ(count, 1);
}

} // namespace test
} // namespace themis
