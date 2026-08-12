#include <gtest/gtest.h>
#include "ethics_ai/philosophy_loader.h"
#include <filesystem>
#include <fstream>

using namespace themis::plugins::ethics;

class PhilosophyLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        loader_ = std::make_unique<PhilosophyLoader>();
        
        // Create temp directory for test files
        test_dir_ = "/tmp/test_philosophies_" + std::to_string(std::time(nullptr));
        std::filesystem::create_directories(test_dir_);
    }
    
    void TearDown() override {
        // Clean up temp directory
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }
    
    void createTestYAML(const std::string& filename, const std::string& content) {
        std::ofstream file(test_dir_ + "/" + filename);
        file << content;
        file.close();
    }
    
    std::unique_ptr<PhilosophyLoader> loader_;
    std::string test_dir_;
};

TEST_F(PhilosophyLoaderTest, LoadFromNonExistentDirectory) {
    auto result = loader_->loadFromDirectory("/nonexistent/path");
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    auto status = std::get<Status>(result);
    EXPECT_FALSE(status.isOK());
    EXPECT_NE(status.message.find("does not exist"), std::string::npos);
}

TEST_F(PhilosophyLoaderTest, LoadFromEmptyDirectory) {
    auto result = loader_->loadFromDirectory(test_dir_);
    ASSERT_TRUE(std::holds_alternative<size_t>(result));
    EXPECT_EQ(0u, std::get<size_t>(result));
}

TEST_F(PhilosophyLoaderTest, LoadValidYAML) {
    std::string yaml_content = R"(
school_id: test_kant
name: Kantian Ethics
main_theses:
  - Categorical Imperative
  - Respect for Persons
secondary_theses:
  - Autonomy
  - Universal Law
decision_framework:
  primary_test: Universalizability
  secondary_test: Respect
strengths:
  - Clear principles
  - Universal applicability
weaknesses:
  - Can be rigid
internal_debate:
  issue1: Application to edge cases
philosophical_positioning:
  vs_utilitarianism: Focuses on duty not consequences
)";
    
    createTestYAML("test_kant.yaml", yaml_content);
    
    auto result = loader_->loadFromDirectory(test_dir_);
    ASSERT_TRUE(std::holds_alternative<size_t>(result));
    EXPECT_EQ(1u, std::get<size_t>(result));
    
    // Verify profile was loaded
    EXPECT_TRUE(loader_->hasProfile("test_kant"));
    
    auto profile_result = loader_->getProfile("test_kant");
    ASSERT_TRUE(std::holds_alternative<PhilosophyProfile>(profile_result));
    
    auto profile = std::get<PhilosophyProfile>(profile_result);
    EXPECT_EQ("test_kant", profile.school_id);
    EXPECT_EQ("Kantian Ethics", profile.name);
    EXPECT_EQ(2u, profile.main_theses.size());
    EXPECT_EQ(2u, profile.secondary_theses.size());
    EXPECT_EQ(2u, profile.strengths.size());
    EXPECT_EQ(1u, profile.weaknesses.size());
}

TEST_F(PhilosophyLoaderTest, LoadMultipleFiles) {
    std::string kant_yaml = R"(
school_id: kant
name: Kantian Ethics
main_theses:
  - Categorical Imperative
)";
    
    std::string util_yaml = R"(
school_id: utilitarianism
name: Utilitarian Ethics
main_theses:
  - Greatest Happiness Principle
)";
    
    createTestYAML("kant.yaml", kant_yaml);
    createTestYAML("utilitarianism.yaml", util_yaml);
    
    auto result = loader_->loadFromDirectory(test_dir_);
    ASSERT_TRUE(std::holds_alternative<size_t>(result));
    EXPECT_EQ(2u, std::get<size_t>(result));
    
    EXPECT_TRUE(loader_->hasProfile("kant"));
    EXPECT_TRUE(loader_->hasProfile("utilitarianism"));
    
    auto school_ids = loader_->getSchoolIds();
    EXPECT_EQ(2u, school_ids.size());
}

TEST_F(PhilosophyLoaderTest, GetNonExistentProfile) {
    auto result = loader_->getProfile("nonexistent");
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    auto status = std::get<Status>(result);
    EXPECT_FALSE(status.isOK());
}

TEST_F(PhilosophyLoaderTest, ClearProfiles) {
    std::string yaml_content = R"(
school_id: test
name: Test
main_theses:
  - Test thesis
)";
    
    createTestYAML("test.yaml", yaml_content);
    loader_->loadFromDirectory(test_dir_);
    
    EXPECT_TRUE(loader_->hasProfile("test"));
    EXPECT_EQ(1u, loader_->count());
    
    loader_->clear();
    
    EXPECT_FALSE(loader_->hasProfile("test"));
    EXPECT_EQ(0u, loader_->count());
}

TEST_F(PhilosophyLoaderTest, LoadFromActualPhilosophiesDirectory) {
    // Test loading from actual philosophies directory if it exists
    std::string actual_dir = "/home/runner/work/ThemisDB/ThemisDB/plugins/ethics_ai/philosophies";
    
    if (std::filesystem::exists(actual_dir)) {
        auto result = loader_->loadFromDirectory(actual_dir);
        ASSERT_TRUE(std::holds_alternative<size_t>(result));
        
        size_t count = std::get<size_t>(result);
        EXPECT_GT(count, 0u) << "Should load at least one philosophy profile";
        
        // Check that common profiles exist
        if (loader_->hasProfile("kant")) {
            auto profile_result = loader_->getProfile("kant");
            ASSERT_TRUE(std::holds_alternative<PhilosophyProfile>(profile_result));
            
            auto profile = std::get<PhilosophyProfile>(profile_result);
            EXPECT_FALSE(profile.name.empty());
            EXPECT_FALSE(profile.main_theses.empty());
        }
    } else {
        GTEST_SKIP() << "Actual philosophies directory not found";
    }
}

TEST_F(PhilosophyLoaderTest, IgnoreNonYAMLFiles) {
    std::string yaml_content = R"(
school_id: test
name: Test
main_theses:
  - Test
)";
    
    createTestYAML("valid.yaml", yaml_content);
    createTestYAML("readme.txt", "This is not a YAML file");
    createTestYAML("data.json", "{}");
    
    auto result = loader_->loadFromDirectory(test_dir_);
    ASSERT_TRUE(std::holds_alternative<size_t>(result));
    EXPECT_EQ(1u, std::get<size_t>(result)) << "Should only load YAML files";
}