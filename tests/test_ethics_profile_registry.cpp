/**
 * @file test_ethics_profile_registry.cpp
 * @brief Unit tests for EthicsProfileRegistry — EPR-01..12
 *
 * Tests cover:
 *  EPR-01  rebuildIndex() counts all YAML files in a directory
 *  EPR-02  queryIndex() with empty query returns all profiles
 *  EPR-03  queryIndex() filtered by taxonomy_class returns only matching
 *  EPR-04  queryIndex() filtered by tag returns only profiles with that tag
 *  EPR-05  queryIndex() filtered by domain uses ANY-match semantics
 *  EPR-06  queryIndex() max_results cap is respected
 *  EPR-07  getProfile() returns Status::Error for unknown school_id
 *  EPR-08  hasProfile() returns false for unknown, true after rebuildIndex
 *  EPR-09  getProfile() cold-loads and returns a valid profile
 *  EPR-10  getProfile() second call returns from LRU cache (warm path)
 *  EPR-11  rebuildIndex() on non-existent directory returns error
 *  EPR-12  LRU eviction: lru_capacity=2, loading 3 profiles evicts oldest
 */

#include <gtest/gtest.h>

#include "ethics_profile_registry.h"
#include "ethics_ai/ethics_ai_types.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <variant>

using namespace themis::plugins::ethics;
namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture: writes minimal YAML files to a temp directory
// ─────────────────────────────────────────────────────────────────────────────

static const char* kPhiloDir =
    THEMIS_PHILOSOPHIES_DIR;  // defined by CMake

class EthicsProfileRegistryTest : public ::testing::Test {
protected:
    std::string tmp_dir = {};

    void SetUp() override {
        // Use a time-based suffix to avoid collisions in parallel test runs
        const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        tmp_dir = (fs::temp_directory_path() / ("test_epr_" + suffix)).string();
        fs::create_directories(tmp_dir);
        writeProfile("school_a", "deontological", {"duty", "rights"}, {"medical"});
        writeProfile("school_b", "consequentialist", {"utility"}, {"ai_governance"});
        writeProfile("school_c", "deontological", {"duty", "autonomy"}, {"research", "medical"});
    }

    void TearDown() override {
        fs::remove_all(tmp_dir);
    }

    void writeProfile(const std::string& id,
                      const std::string& tax_class,
                      const std::vector<std::string>& tags,
                      const std::vector<std::string>& domains)
    {
        std::ofstream f(tmp_dir + "/" + id + ".yaml");
        f << "school_id: " << id << "\n";
        f << "name: \"" << id << " school\"\n";
        f << "taxonomy_class: " << tax_class << "\n";
        f << "tags:\n";
        for (const auto& t : tags) {
          f << "  - " << t << "\n";
        }
        f << "applicable_domains:\n";
        for (const auto& d : domains) {
          f << "  - " << d << "\n";
        }
        f << "description: |\n  A test profile for " << id << ".\n";
        f << "main_theses: []\n";
        f << "secondary_theses: []\n";
        f << "decision_framework: {}\n";
        f << "strengths: []\n";
        f << "weaknesses: []\n";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// EPR-01: rebuildIndex counts YAML files
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(EthicsProfileRegistryTest, EPR01_RebuildIndexCountsYAML) {
    EthicsProfileRegistry reg;
    auto result = reg.rebuildIndex(tmp_dir);
    ASSERT_TRUE(std::holds_alternative<size_t>(result));
    EXPECT_EQ(std::get<size_t>(result), 3u);
    EXPECT_EQ(reg.indexSize(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// EPR-02: queryIndex with empty query returns all profiles
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(EthicsProfileRegistryTest, EPR02_EmptyQueryReturnsAll) {
    EthicsProfileRegistry reg;
    reg.rebuildIndex(tmp_dir);
    EthicsIndexQuery q;
    auto metas = reg.queryIndex(q);
    EXPECT_EQ(metas.size(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// EPR-03: queryIndex filtered by taxonomy_class
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(EthicsProfileRegistryTest, EPR03_FilterByTaxonomyClass) {
    EthicsProfileRegistry reg;
    reg.rebuildIndex(tmp_dir);
    EthicsIndexQuery q;
    q.taxonomy_class = "deontological";
    auto metas = reg.queryIndex(q);
    EXPECT_EQ(metas.size(), 2u);
    for (const auto& m : metas)
        EXPECT_EQ(m.taxonomy_class, "deontological");
}

// ─────────────────────────────────────────────────────────────────────────────
// EPR-04: queryIndex filtered by tag (ALL must match)
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(EthicsProfileRegistryTest, EPR04_FilterByTag) {
    EthicsProfileRegistry reg;
    reg.rebuildIndex(tmp_dir);
    EthicsIndexQuery q;
    q.tags = {"duty"};
    auto metas = reg.queryIndex(q);
    EXPECT_EQ(metas.size(), 2u); // school_a and school_c have "duty"

    q.tags = {"duty", "autonomy"};
    metas = reg.queryIndex(q);
    EXPECT_EQ(metas.size(), 1u); // only school_c has both
    EXPECT_EQ(metas[0].school_id, "school_c");
}

// ─────────────────────────────────────────────────────────────────────────────
// EPR-05: queryIndex filtered by domain (ANY must match)
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(EthicsProfileRegistryTest, EPR05_FilterByDomainAnyMatch) {
    EthicsProfileRegistry reg;
    reg.rebuildIndex(tmp_dir);
    EthicsIndexQuery q;
    q.domains = {"medical"};
    auto metas = reg.queryIndex(q);
    EXPECT_EQ(metas.size(), 2u); // school_a (medical) and school_c (medical,research)

    q.domains = {"ai_governance"};
    metas = reg.queryIndex(q);
    EXPECT_EQ(metas.size(), 1u);
    EXPECT_EQ(metas[0].school_id, "school_b");
}

// ─────────────────────────────────────────────────────────────────────────────
// EPR-06: max_results cap
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(EthicsProfileRegistryTest, EPR06_MaxResultsCap) {
    EthicsProfileRegistry reg;
    reg.rebuildIndex(tmp_dir);
    EthicsIndexQuery q;
    q.max_results = 2;
    auto metas = reg.queryIndex(q);
    EXPECT_LE(metas.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// EPR-07: getProfile returns error for unknown school_id
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(EthicsProfileRegistryTest, EPR07_UnknownSchoolReturnsError) {
    EthicsProfileRegistry reg;
    reg.rebuildIndex(tmp_dir);
    auto result = reg.getProfile("nonexistent_school");
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    EXPECT_FALSE(std::get<Status>(result).isOK());
}

// ─────────────────────────────────────────────────────────────────────────────
// EPR-08: hasProfile
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(EthicsProfileRegistryTest, EPR08_HasProfile) {
    EthicsProfileRegistry reg;
    EXPECT_FALSE(reg.hasProfile("school_a"));
    reg.rebuildIndex(tmp_dir);
    EXPECT_TRUE(reg.hasProfile("school_a"));
    EXPECT_FALSE(reg.hasProfile("school_zzz"));
}

// ─────────────────────────────────────────────────────────────────────────────
// EPR-09: getProfile cold-loads a valid profile
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(EthicsProfileRegistryTest, EPR09_ColdLoadProfile) {
    EthicsProfileRegistry reg;
    reg.rebuildIndex(tmp_dir);
    auto result = reg.getProfile("school_a");
    ASSERT_TRUE(std::holds_alternative<PhilosophyProfile>(result));
    const auto& p = std::get<PhilosophyProfile>(result);
    EXPECT_EQ(p.school_id, "school_a");
}

// ─────────────────────────────────────────────────────────────────────────────
// EPR-10: getProfile second call hits LRU cache (no parse overhead)
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(EthicsProfileRegistryTest, EPR10_LRUCacheHit) {
    EthicsProfileRegistry reg;
    reg.rebuildIndex(tmp_dir);
    // First call: cold load
    auto r1 = reg.getProfile("school_b");
    ASSERT_TRUE(std::holds_alternative<PhilosophyProfile>(r1));
    // Second call: warm (should also succeed identically)
    auto r2 = reg.getProfile("school_b");
    ASSERT_TRUE(std::holds_alternative<PhilosophyProfile>(r2));
    EXPECT_EQ(std::get<PhilosophyProfile>(r1).school_id,
              std::get<PhilosophyProfile>(r2).school_id);
}

// ─────────────────────────────────────────────────────────────────────────────
// EPR-11: rebuildIndex on non-existent directory returns error
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(EthicsProfileRegistryTest, EPR11_NonExistentDirectoryError) {
    EthicsProfileRegistry reg;
    auto result = reg.rebuildIndex("/tmp/this_directory_does_not_exist_xyz");
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    EXPECT_FALSE(std::get<Status>(result).isOK());
}

// ─────────────────────────────────────────────────────────────────────────────
// EPR-12: LRU eviction with capacity=2 loading 3 profiles
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(EthicsProfileRegistryTest, EPR12_LRUEviction) {
    EthicsProfileRegistry reg(/*lru_capacity=*/2);
    reg.rebuildIndex(tmp_dir);

    // Load 3 profiles — the first loaded (school_a) should be evicted
    reg.getProfile("school_a");
    reg.getProfile("school_b");
    reg.getProfile("school_c"); // evicts school_a

    // All three must still be loadable (cold reload from file)
    EXPECT_TRUE(std::holds_alternative<PhilosophyProfile>(reg.getProfile("school_a")));
    EXPECT_TRUE(std::holds_alternative<PhilosophyProfile>(reg.getProfile("school_b")));
    EXPECT_TRUE(std::holds_alternative<PhilosophyProfile>(reg.getProfile("school_c")));
}

// ─────────────────────────────────────────────────────────────────────────────
// Bonus: rebuildIndex with actual philosophies directory (smoke test)
// ─────────────────────────────────────────────────────────────────────────────
TEST(EthicsProfileRegistrySmoke, SmokeRealPhilosophies) {
    if (!fs::exists(kPhiloDir)) {
        GTEST_SKIP() << "Philosophies dir not found: " << kPhiloDir;
    }
    EthicsProfileRegistry reg;
    auto result = reg.rebuildIndex(kPhiloDir);
    ASSERT_TRUE(std::holds_alternative<size_t>(result));
    EXPECT_GT(std::get<size_t>(result), 5u) << "Expected at least 6 philosophy profiles";

    // All indexed school_ids must be non-empty
    EthicsIndexQuery q;
    for (const auto& m : reg.queryIndex(q)) {
        EXPECT_FALSE(m.school_id.empty());
        EXPECT_FALSE(m.yaml_path.empty());
    }
}
