/**
 * @file test_thesis_budget_management.cpp
 * @brief Unit tests for §9.1 Per-Thesis Token Budget and Activation Rounds —
 *        specifically ContextWindowBudgetManager::selectThesesForRound().
 *
 * Tests cover:
 *  TBM-01  Thesis with activation_rounds:[1,2] injected full in R1, headline-only in R3
 *  TBM-02  Total budget exceeded → lowest-weight thesis downgraded first
 *  TBM-03  Profile with no token_budget fields behaves as unlimited (backward compat)
 *  TBM-04  round_role_weights.REBUTTAL=1.0 → thesis selected before others in R2
 *  TBM-05  available_tokens=100 with all theses requiring 200 → top-1 selected only
 *  TBM-06  Empty typed_theses returns empty vector (no crash)
 *  TBM-07  Thesis with token_budget cap smaller than full text → respects cap
 *  TBM-08  All theses in activation round; none downgraded when budget is ample
 *  TBM-09  Inactive theses always appear as headline regardless of available budget
 *  TBM-10  PhilosophyLoader parses token_budget + activation_rounds from YAML
 */

#include <gtest/gtest.h>

#include "prompt_engineering/context_window_manager.h"
#include "ethics_ai/ethics_ai_types.h"
#include "ethics_ai/philosophy_loader.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace themis::prompt_engineering;
using namespace themis::plugins::ethics;

// ─── helpers ─────────────────────────────────────────────────────────────────

namespace {

/// Build a PhilosophyThesis with a long description (> 100 chars).
PhilosophyThesis make_thesis(const std::string& id,
                              const std::string& name,
                              const std::string& description,
                              int token_budget,
                              std::vector<int> active_rounds,
                              std::map<std::string, float> weights = {}) {
    PhilosophyThesis t;
    t.thesis_id          = id;
    t.name               = name;
    t.description        = description;
    t.token_budget       = token_budget;
    t.activation_rounds  = std::move(active_rounds);
    t.round_role_weights = std::move(weights);
    return t;
}

/// Build a profile with the supplied typed theses.
PhilosophyProfile make_profile(const std::string& school_id,
                               std::vector<PhilosophyThesis> theses) {
    PhilosophyProfile p;
    p.school_id    = school_id;
    p.name         = school_id;
    p.typed_theses = std::move(theses);
    return p;
}

/// CharDivisionCounter approximation: len / 4 (rounds up).
int approx_tokens(const std::string& s) {
    return static_cast<int>((s.size() + 3) / 4);
}

} // namespace

// ─── test fixture ────────────────────────────────────────────────────────────

class ThesisBudgetTest : public ::testing::Test {
protected:
    ContextWindowBudgetManager mgr;

    void SetUp() override {
        ModelTokenBudget budget;
        budget.model_name    = "test-model";
        budget.max_tokens    = 4096;
        budget.reserved_completion_tokens = 512;
        mgr.setModel(budget);
    }
};

// ─── TBM-01 ──────────────────────────────────────────────────────────────────
TEST_F(ThesisBudgetTest, TBM_01_ActiveRoundFullInactive_Headline) {
    // Thesis active only in rounds 1 and 2.
    const std::string desc(200, 'A'); // 200 chars → ~50 tokens
    PhilosophyProfile profile = make_profile("school_a", {
        make_thesis("t1", "Thesis One", desc, -1, {1, 2})
    });

    // Round 1 → must be full
    auto r1 = mgr.selectThesesForRound(profile, 1, "PRO", 1000);
    ASSERT_EQ(r1.size(), 1u);
    EXPECT_TRUE(r1[0].is_full) << "TBM-01: thesis active in R1 must be full";
    EXPECT_EQ(r1[0].thesis_id, "t1");

    // Round 3 → not active → headline only
    auto r3 = mgr.selectThesesForRound(profile, 3, "SYNTHESIS", 1000);
    ASSERT_EQ(r3.size(), 1u);
    EXPECT_FALSE(r3[0].is_full) << "TBM-01: thesis inactive in R3 must be headline";
    EXPECT_NE(r3[0].text.find("[t1:"), std::string::npos);
}

// ─── TBM-02 ──────────────────────────────────────────────────────────────────
TEST_F(ThesisBudgetTest, TBM_02_BudgetExceeded_LowestWeightDowngraded) {
    // Thesis A: weight 0.9 (high); Thesis B: weight 0.2 (low)
    // Description lengths chosen so both together exceed the small budget.
    const std::string descA(100, 'A'); // ~25 tokens
    const std::string descB(120, 'B'); // ~30 tokens

    PhilosophyProfile profile = make_profile("school_b", {
        make_thesis("tA", "Thesis A", descA, -1, {}, {{"PRO", 0.9f}}),
        make_thesis("tB", "Thesis B", descB, -1, {}, {{"PRO", 0.2f}}),
    });

    // Available budget: enough for A but not A+B
    const int budget_tokens = approx_tokens(descA) + 5; // just over A, short of B

    auto result = mgr.selectThesesForRound(profile, 1, "PRO", budget_tokens);
    ASSERT_EQ(result.size(), 2u);

    // tA sorted first (higher weight) → must be full
    auto it_A = std::find_if(result.begin(), result.end(),
        [](const ThesisInjection& ti){ return ti.thesis_id == "tA"; });
    ASSERT_NE(it_A, result.end());
    EXPECT_TRUE(it_A->is_full) << "TBM-02: high-weight thesis must be full";

    // tB sorted second → budget exhausted → headline
    auto it_B = std::find_if(result.begin(), result.end(),
        [](const ThesisInjection& ti){ return ti.thesis_id == "tB"; });
    ASSERT_NE(it_B, result.end());
    EXPECT_FALSE(it_B->is_full) << "TBM-02: low-weight thesis must be downgraded";
}

// ─── TBM-03 ──────────────────────────────────────────────────────────────────
TEST_F(ThesisBudgetTest, TBM_03_NoTokenBudget_UnlimitedBackwardCompat) {
    const std::string desc(80, 'C'); // ~20 tokens
    PhilosophyProfile profile = make_profile("school_c", {
        // No token_budget (-1) and no activation_rounds (empty = all)
        make_thesis("tc", "TC", desc, -1, {})
    });

    // Generous budget
    auto result = mgr.selectThesesForRound(profile, 1, "PRO", 10000);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_TRUE(result[0].is_full)
        << "TBM-03: no token_budget should behave as unlimited → full text";
}

// ─── TBM-04 ──────────────────────────────────────────────────────────────────
TEST_F(ThesisBudgetTest, TBM_04_RoundRoleWeight_HighWeightSelectedFirst) {
    const std::string descX(60, 'X');
    const std::string descY(60, 'Y');

    PhilosophyProfile profile = make_profile("school_d", {
        make_thesis("tX", "TX", descX, -1, {}, {{"REBUTTAL", 0.1f}}),  // low weight
        make_thesis("tY", "TY", descY, -1, {}, {{"REBUTTAL", 1.0f}}),  // high weight
    });

    // Budget fits exactly ONE full thesis
    const int budget = approx_tokens(descY) + 2; // barely fits one

    auto result = mgr.selectThesesForRound(profile, 2, "REBUTTAL", budget);
    ASSERT_EQ(result.size(), 2u);

    auto it_Y = std::find_if(result.begin(), result.end(),
        [](const ThesisInjection& ti){ return ti.thesis_id == "tY"; });
    ASSERT_NE(it_Y, result.end());
    EXPECT_TRUE(it_Y->is_full)
        << "TBM-04: thesis with REBUTTAL weight=1.0 must be selected first and injected full";

    auto it_X = std::find_if(result.begin(), result.end(),
        [](const ThesisInjection& ti){ return ti.thesis_id == "tX"; });
    ASSERT_NE(it_X, result.end());
    EXPECT_FALSE(it_X->is_full)
        << "TBM-04: thesis with REBUTTAL weight=0.1 must be downgraded when budget exhausted";
}

// ─── TBM-05 ──────────────────────────────────────────────────────────────────
TEST_F(ThesisBudgetTest, TBM_05_VerySmallBudget_TopOneOnly) {
    // 3 theses all active; budget only fits top-1
    const std::string desc(200, 'Z'); // ~50 tokens each

    PhilosophyProfile profile = make_profile("school_e", {
        make_thesis("t1", "T1", desc, -1, {}, {{"PRO", 0.9f}}),
        make_thesis("t2", "T2", desc, -1, {}, {{"PRO", 0.5f}}),
        make_thesis("t3", "T3", desc, -1, {}, {{"PRO", 0.2f}}),
    });

    const int budget = approx_tokens(desc) + 5; // fits only 1

    auto result = mgr.selectThesesForRound(profile, 1, "PRO", budget);
    ASSERT_EQ(result.size(), 3u);

    int full_count = 0;
    for (const auto& ti : result) {
      full_count += (ti.is_full ? 1 : 0);
    }
    EXPECT_EQ(full_count, 1) << "TBM-05: with budget for 1, only top-1 should be full";

    // top-1 must be t1 (weight 0.9)
    auto it_t1 = std::find_if(result.begin(), result.end(),
        [](const ThesisInjection& ti){ return ti.thesis_id == "t1"; });
    EXPECT_TRUE(it_t1->is_full) << "TBM-05: highest-weight thesis t1 must be the full one";
}

// ─── TBM-06 ──────────────────────────────────────────────────────────────────
TEST_F(ThesisBudgetTest, TBM_06_EmptyProfile_ReturnsEmptyNocrash) {
    PhilosophyProfile profile = make_profile("empty_school", {});

    EXPECT_NO_THROW({
        auto result = mgr.selectThesesForRound(profile, 1, "PRO", 1000);
        EXPECT_TRUE(result.empty());
    });
}

// ─── TBM-07 ──────────────────────────────────────────────────────────────────
TEST_F(ThesisBudgetTest, TBM_07_TokenBudgetCap_Respected) {
    const std::string desc(400, 'W'); // ~100 tokens — deliberately large
    const int cap = 20;               // per-thesis cap: 20 tokens

    PhilosophyProfile profile = make_profile("school_f", {
        make_thesis("tw", "TW", desc, cap, {})
    });

    auto result = mgr.selectThesesForRound(profile, 1, "PRO", 1000);
    ASSERT_EQ(result.size(), 1u);
    // thesis may be full (cap respected) but tokens_used must be <= cap
    EXPECT_LE(result[0].tokens_used, cap)
        << "TBM-07: tokens_used must not exceed per-thesis token_budget cap";
}

// ─── TBM-08 ──────────────────────────────────────────────────────────────────
TEST_F(ThesisBudgetTest, TBM_08_AllActiveAmpleBudget_NoneDowngraded) {
    const std::string desc(80, 'Q'); // ~20 tokens each

    PhilosophyProfile profile = make_profile("school_g", {
        make_thesis("a", "A", desc, -1, {}),
        make_thesis("b", "B", desc, -1, {}),
        make_thesis("c", "C", desc, -1, {}),
    });

    // 10× the needed budget
    const int budget = 3 * approx_tokens(desc) * 10;

    auto result = mgr.selectThesesForRound(profile, 3, "SYNTHESIS", budget);
    ASSERT_EQ(result.size(), 3u);
    for (const auto& ti : result) {
        EXPECT_TRUE(ti.is_full)
            << "TBM-08: with ample budget all active theses must be full";
    }
}

// ─── TBM-09 ──────────────────────────────────────────────────────────────────
TEST_F(ThesisBudgetTest, TBM_09_InactiveAlwaysHeadline) {
    const std::string desc(80, 'I');

    PhilosophyProfile profile = make_profile("school_h", {
        make_thesis("active", "Active", desc, -1, {2}),   // only round 2
        make_thesis("inactive", "Inactive", desc, -1, {3}) // only round 3
    });

    // Round 2: 'active' is full; 'inactive' must be headline
    auto result = mgr.selectThesesForRound(profile, 2, "PRO", 100000);
    ASSERT_EQ(result.size(), 2u);

    for (const auto& ti : result) {
        if (ti.thesis_id == "active") {
            EXPECT_TRUE(ti.is_full) << "TBM-09: active thesis must be full";
        } else {
            EXPECT_FALSE(ti.is_full)
                << "TBM-09: inactive thesis must be headline even with ample budget";
        }
    }
}

// ─── TBM-10 ──────────────────────────────────────────────────────────────────
// Requires HAVE_YAML_CPP and a real YAML file to be written.
TEST_F(ThesisBudgetTest, TBM_10_PhilosophyLoaderParsesTypedThesis) {
    // Build a minimal YAML with a structured thesis that has token_budget and
    // activation_rounds, write it to a temp file, load it, and verify.

    const auto suffix =
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const fs::path tmp_dir  = fs::temp_directory_path() / ("test_tbm_" + suffix);
    const fs::path yaml_path = tmp_dir / "test_school.yaml";
    fs::create_directories(tmp_dir);

    {
        std::ofstream f(yaml_path);
        f << R"yaml(
school_id: test_school
name: Test School
taxonomy_class: deontological
tags: [test]
applicable_domains: [ai_ethics]

main_theses:
  - thesis_id: "test_school:thesis_one"
    name: "Thesis One"
    description: "Core principle of test school."
    token_budget: 150
    activation_rounds: [1, 3]
    round_role_weights:
      PRO: 0.8
      SYNTHESIS: 0.5

secondary_theses:
  - thesis_id: "test_school:thesis_two"
    name: "Thesis Two"
    description: "Secondary principle."
    token_budget: 80
    activation_rounds: [2, 4]
)yaml";
    }

    PhilosophyLoader loader;
    auto load_status = loader.loadFromFile(yaml_path.string());

    fs::remove_all(tmp_dir);

#ifdef HAVE_YAML_CPP
    ASSERT_TRUE(load_status.isOK())
        << "TBM-10: loadFromFile must succeed for valid YAML: " << load_status.message;

    auto variant = loader.getProfile("test_school");
    ASSERT_TRUE(std::holds_alternative<PhilosophyProfile>(variant))
        << "TBM-10: getProfile must return PhilosophyProfile after successful load";

    const auto& profile = std::get<PhilosophyProfile>(variant);
    ASSERT_EQ(profile.typed_theses.size(), 2u)
        << "TBM-10: loader must parse both typed theses";

    // Check thesis_one
    const auto& t0 = profile.typed_theses[0];
    EXPECT_EQ(t0.thesis_id, "test_school:thesis_one");
    EXPECT_EQ(t0.token_budget, 150);
    ASSERT_EQ(t0.activation_rounds.size(), 2u);
    EXPECT_EQ(t0.activation_rounds[0], 1);
    EXPECT_EQ(t0.activation_rounds[1], 3);
    EXPECT_NEAR(t0.round_role_weights.at("PRO"), 0.8f, 0.001f);

    // Check thesis_two
    const auto& t1 = profile.typed_theses[1];
    EXPECT_EQ(t1.thesis_id, "test_school:thesis_two");
    EXPECT_EQ(t1.token_budget, 80);
    ASSERT_EQ(t1.activation_rounds.size(), 2u);
    EXPECT_EQ(t1.activation_rounds[0], 2);
    EXPECT_EQ(t1.activation_rounds[1], 4);
#else
    GTEST_SKIP() << "TBM-10: HAVE_YAML_CPP not defined — skipping YAML parse test";
#endif
}

// ─── main ────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
