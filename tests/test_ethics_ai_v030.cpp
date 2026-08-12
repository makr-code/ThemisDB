/**
 * Ethics AI v0.3.0 unit tests — hot-reload, multi-round debates, Prometheus metrics.
 *
 * Test IDs:
 *   EAM-01 … EAM-04  — PhilosophyLoader::reloadProfiles()
 *   EAM-05 … EAM-08  — EthicalDiscourseEngine::continueDebate() + ArgumentStore::getDebateTranscript()
 *   EAM-09 … EAM-12  — EthicsEvaluator Prometheus metrics
 */

#include <gtest/gtest.h>
#include "ethics_ai/discourse_engine.h"
#include "ethics_ai/philosophy_loader.h"
#include "ethics_ai/argument_store.h"
#include "ethics_ai/ethics_evaluator.h"
#include "ethics_ai/ethics_ai_types.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

using namespace themis::plugins::ethics;

// ============================================================================
// Helpers
// ============================================================================

static void writeYaml(const fs::path& path, const std::string& school_id,
                       const std::string& name)
{
    std::ofstream f(path);
    f << "school_id: " << school_id << "\n"
      << "name: " << name << "\n"
      << "main_theses:\n"
      << "  - The greatest good for the greatest number.\n"
      << "secondary_theses:\n"
      << "  - Consequences determine morality.\n"
      << "decision_framework:\n"
      << "  primary: maximize utility\n"
      << "strengths:\n"
      << "  - pragmatic: Highly practical\n"
      << "weaknesses:\n"
      << "  - minority_rights: May ignore minorities\n";
}

static std::shared_ptr<PhilosophyLoader> makeLoader(
    const std::vector<std::pair<std::string,std::string>>& profiles)
{
    auto loader = std::make_shared<PhilosophyLoader>();
    for (const auto& [id, name] : profiles) {
        PhilosophyProfile p;
        p.school_id = id;
        p.name = name;
        p.main_theses.push_back("Core thesis of " + name);
        p.secondary_theses.push_back("Secondary thesis of " + name);
        loader->addProfile(p);
    }
    return loader;
}

static std::shared_ptr<ArgumentStore> makeInitializedStore() {
    auto store = std::make_shared<ArgumentStore>();
    store->initialize(nullptr, nullptr);
    return store;
}

// ============================================================================
// EAM-01: reloadProfiles() returns error for non-existent directory
// ============================================================================
TEST(EthicsAIv030, EAM01_ReloadProfilesNonExistentDir) {
    PhilosophyLoader loader;
    auto result = loader.reloadProfiles("/non/existent/path/12345");
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    EXPECT_FALSE(std::get<Status>(result).isOK());
}

// ============================================================================
// EAM-02: reloadProfiles() loads profiles from a real directory
// ============================================================================
TEST(EthicsAIv030, EAM02_ReloadProfilesLoadsFromDir) {
    auto tmp = fs::temp_directory_path() / "ethics_reload_test_02";
    fs::create_directories(tmp);
    writeYaml(tmp / "utilitarianism.yaml", "utilitarianism", "Utilitarianism");
    writeYaml(tmp / "kantian.yaml",        "kantian",        "Kantian Ethics");

    PhilosophyLoader loader;
#ifdef HAVE_YAML_CPP
    auto result = loader.reloadProfiles(tmp.string());
    ASSERT_TRUE(std::holds_alternative<size_t>(result));
    EXPECT_EQ(2u, std::get<size_t>(result));
    EXPECT_TRUE(loader.hasProfile("utilitarianism"));
    EXPECT_TRUE(loader.hasProfile("kantian"));
#else
    // Without yaml-cpp, reloadProfiles returns 0 (nothing parsed but no error)
    (void)loader.reloadProfiles(tmp.string());
    SUCCEED() << "yaml-cpp not available; skipping file content check";
#endif

    fs::remove_all(tmp);
}

// ============================================================================
// EAM-03: reloadProfiles() atomically replaces old profiles
// ============================================================================
TEST(EthicsAIv030, EAM03_ReloadProfilesAtomicSwap) {
    PhilosophyLoader loader;
    // Pre-load a profile programmatically
    PhilosophyProfile old_p;
    old_p.school_id = "old_school";
    old_p.name      = "Old School";
    loader.addProfile(old_p);
    ASSERT_TRUE(loader.hasProfile("old_school"));

    auto tmp = fs::temp_directory_path() / "ethics_reload_test_03";
    fs::create_directories(tmp);
    writeYaml(tmp / "new_school.yaml", "new_school", "New School");

#ifdef HAVE_YAML_CPP
    auto result = loader.reloadProfiles(tmp.string());
    ASSERT_TRUE(std::holds_alternative<size_t>(result));
    EXPECT_FALSE(loader.hasProfile("old_school"))
        << "Old profile should be replaced after reload";
    EXPECT_TRUE(loader.hasProfile("new_school"))
        << "New profile should be present after reload";
#else
    SUCCEED() << "yaml-cpp not available; reload replaces with 0 profiles";
#endif

    fs::remove_all(tmp);
}

// ============================================================================
// EAM-04: reloadProfiles() is thread-safe (concurrent reload + addProfile)
// ============================================================================
TEST(EthicsAIv030, EAM04_ReloadProfilesThreadSafe) {
    auto tmp = fs::temp_directory_path() / "ethics_reload_test_04";
    fs::create_directories(tmp);
    writeYaml(tmp / "school_a.yaml", "school_a", "School A");

    PhilosophyLoader loader;
    // Start a reload in a background thread; simultaneously call addProfile.
    std::thread t([&] {
        for (int i = 0; i < 5; ++i) {
            loader.reloadProfiles(tmp.string());
        }
    });
    for (int i = 0; i < 5; ++i) {
        PhilosophyProfile p;
        p.school_id = "thread_school_" + std::to_string(i);
        p.name      = "Thread School " + std::to_string(i);
        loader.addProfile(p);
    }
    t.join();
    // No crash or data race = test passes
    SUCCEED();

    fs::remove_all(tmp);
}

// ============================================================================
// EAM-05: continueDebate() returns error when debate_id is unknown
// ============================================================================
TEST(EthicsAIv030, EAM05_ContinueDebateUnknownId) {
    auto loader = makeLoader({{"school_a", "School A"}});
    auto store  = makeInitializedStore();
    auto rag    = std::make_shared<RAGContextEngine>(store);
    EthicalDiscourseEngine engine(loader, store, rag);

    auto result = engine.continueDebate("non_existent_debate_id", 1);
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    EXPECT_FALSE(std::get<Status>(result).isOK());
}

// ============================================================================
// EAM-06: continueDebate() produces one argument per school per round
// ============================================================================
TEST(EthicsAIv030, EAM06_ContinueDebateProducesArguments) {
    auto loader = makeLoader({{"school_a", "School A"}, {"school_b", "School B"}});
    auto store  = makeInitializedStore();
    auto rag    = std::make_shared<RAGContextEngine>(store);
    EthicalDiscourseEngine engine(loader, store, rag);

    auto init_result = engine.initializeDebate(
        "Test dilemma", {"school_a", "school_b"}, "test");
    ASSERT_TRUE(std::holds_alternative<DebateInitialization>(init_result));
    const auto& init = std::get<DebateInitialization>(init_result);

    auto round_result = engine.continueDebate(init.debate_id, 1);
    ASSERT_TRUE(std::holds_alternative<DebateRound>(round_result))
        << "Expected DebateRound, got Status: "
        << std::get<Status>(round_result).message;
    const auto& round = std::get<DebateRound>(round_result);

    EXPECT_EQ(init.debate_id, round.debate_id);
    EXPECT_EQ(1, round.round_number);
    EXPECT_EQ(2u, round.arguments.size()); // one per school
}

// ============================================================================
// EAM-07: Round number is capped at 3
// ============================================================================
TEST(EthicsAIv030, EAM07_ContinueDebateRoundCappedAt3) {
    auto loader = makeLoader({{"school_a", "School A"}});
    auto store  = makeInitializedStore();
    auto rag    = std::make_shared<RAGContextEngine>(store);
    EthicalDiscourseEngine engine(loader, store, rag);

    auto init_result = engine.initializeDebate("Dilemma", {"school_a"}, "cat");
    ASSERT_TRUE(std::holds_alternative<DebateInitialization>(init_result));
    const auto& debate_id = std::get<DebateInitialization>(init_result).debate_id;

    auto r = engine.continueDebate(debate_id, 99); // should be capped to 3
    ASSERT_TRUE(std::holds_alternative<DebateRound>(r));
    EXPECT_EQ(3, std::get<DebateRound>(r).round_number);
}

// ============================================================================
// EAM-08: getDebateTranscript() returns rounds ordered by round_number
// ============================================================================
TEST(EthicsAIv030, EAM08_GetDebateTranscriptOrdered) {
    auto loader = makeLoader({{"school_a", "School A"}});
    auto store  = makeInitializedStore();
    auto rag    = std::make_shared<RAGContextEngine>(store);
    EthicalDiscourseEngine engine(loader, store, rag);

    auto init_result = engine.initializeDebate("Dilemma", {"school_a"}, "cat");
    ASSERT_TRUE(std::holds_alternative<DebateInitialization>(init_result));
    const auto& debate_id = std::get<DebateInitialization>(init_result).debate_id;

    // Run 3 rounds
    for (int r = 1; r <= 3; ++r) {
        auto res = engine.continueDebate(debate_id, r);
        ASSERT_TRUE(std::holds_alternative<DebateRound>(res))
            << "Round " << r << " failed";
    }

    auto transcript = store->getDebateTranscript(debate_id);
    ASSERT_TRUE(std::holds_alternative<std::vector<DebateRound>>(transcript));
    const auto& rounds = std::get<std::vector<DebateRound>>(transcript);
    ASSERT_EQ(3u, rounds.size());
    EXPECT_EQ(1, rounds[0].round_number);
    EXPECT_EQ(2, rounds[1].round_number);
    EXPECT_EQ(3, rounds[2].round_number);
}

// ============================================================================
// EAM-09: getMetricsText() returns empty string before any decisions
// ============================================================================
TEST(EthicsAIv030, EAM09_MetricsTextEmptyBeforeDecisions) {
    EthicsEvaluator ev;
    EXPECT_EQ("", ev.getMetricsText());
}

// ============================================================================
// EAM-10: recordDecision() increments counter and getMetricsText() is non-empty
// ============================================================================
TEST(EthicsAIv030, EAM10_MetricsTextNonEmptyAfterRecord) {
    EthicsEvaluator ev;
    ev.recordDecision(0.8, /*rag_hit=*/true, /*latency_ms=*/50);
    auto text = ev.getMetricsText();
    EXPECT_NE("", text);
    EXPECT_NE(std::string::npos, text.find("ethics_decisions_total"))
        << "Missing ethics_decisions_total";
    EXPECT_NE(std::string::npos, text.find("ethics_rag_context_hits_total"))
        << "Missing ethics_rag_context_hits_total";
    EXPECT_NE(std::string::npos, text.find("ethics_argument_confidence_avg"))
        << "Missing ethics_argument_confidence_avg";
    EXPECT_NE(std::string::npos, text.find("ethics_argument_store_size"))
        << "Missing ethics_argument_store_size";
}

// ============================================================================
// EAM-11: recordDecision() with rag_hit=false does not count RAG hit
// ============================================================================
TEST(EthicsAIv030, EAM11_MetricsRagHitCounter) {
    EthicsEvaluator ev;
    ev.recordDecision(0.5, /*rag_hit=*/false, 10);
    ev.recordDecision(0.7, /*rag_hit=*/true,  20);
    ev.recordDecision(0.9, /*rag_hit=*/true,  30);
    auto text = ev.getMetricsText();
    // Should contain "ethics_decisions_total 3"
    EXPECT_NE(std::string::npos, text.find("ethics_decisions_total 3"))
        << text;
    // Should contain "ethics_rag_context_hits_total 2"
    EXPECT_NE(std::string::npos, text.find("ethics_rag_context_hits_total 2"))
        << text;
}

// ============================================================================
// EAM-12: setArgumentStoreSize() updates the gauge
// ============================================================================
TEST(EthicsAIv030, EAM12_MetricsArgumentStoreSizeGauge) {
    EthicsEvaluator ev;
    ev.recordDecision(0.5, false, 1); // ensure non-empty metrics output
    ev.setArgumentStoreSize(42);
    auto text = ev.getMetricsText();
    EXPECT_NE(std::string::npos, text.find("ethics_argument_store_size 42"))
        << text;
}
