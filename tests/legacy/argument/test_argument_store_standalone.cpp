#include <gtest/gtest.h>

#include "ethics_ai/argument_store.h"
#include "ethics_ai/ethics_ai_types.h"

#include <memory>
#include <thread>
#include <variant>

using namespace themis::plugins::ethics;

// ============================================================================
// Helpers
// ============================================================================

static EthicalArgument makeArg(const std::string& id,
                                const std::string& school,
                                ArgumentType type = ArgumentType::PRO) {
    EthicalArgument a;
    a.id               = id;
    a.philosophy_school = school;
    a.argument_type    = type;
    a.content          = "Content for " + id;
    a.strength         = ArgumentStrength::MODERATE;
    a.principle_basis  = {"universalizability"};
    return a;
}

static EthicalDecision makeDecision(const std::string& did,
                                    const std::string& dilemma,
                                    const std::string& primary = "kant") {
    EthicalDecision d;
    d.decision_id        = did;
    d.dilemma_id         = dilemma;
    d.decision_text      = "The decision: " + did;
    d.primary_philosophy = primary;
    d.confidence         = 0.82;
    d.consensus_level    = 0.65;
    return d;
}

static PhilosophyProfile makeProfile(const std::string& sid) {
    PhilosophyProfile p;
    p.school_id   = sid;
    p.name        = sid + " Ethics";
    p.main_theses = {"Primary thesis of " + sid};
    p.strengths   = {"clarity"};
    p.weaknesses  = {"abstraction"};
    return p;
}

// ============================================================================
// Test fixture – standalone (nullptr) mode
// ============================================================================

class ArgumentStoreStandaloneTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = std::make_unique<ArgumentStore>();
        // null storage → in-memory map mode
        auto s = store_->initialize(nullptr, nullptr);
        ASSERT_TRUE(s.isOK()) << "initialize failed: " << s.message;
    }

    std::unique_ptr<ArgumentStore> store_;
};

// ============================================================================
// Initialization guard tests
// ============================================================================

TEST(ArgumentStoreInitStandalone, InitializeStandaloneSucceeds) {
    ArgumentStore s;
    EXPECT_TRUE(s.initialize(nullptr, nullptr).isOK());
}

TEST(ArgumentStoreInitStandalone, InitializeTwiceReturnsError) {
    ArgumentStore s;
    s.initialize(nullptr, nullptr);
    auto st2 = s.initialize(nullptr, nullptr);
    EXPECT_FALSE(st2.isOK());
    EXPECT_NE(std::string::npos, st2.message.find("already initialized"));
}

TEST(ArgumentStoreInitStandalone, OperationsFailBeforeInit) {
    ArgumentStore s;
    auto r = s.storeArgument(makeArg("x", "kant"));
    EXPECT_FALSE(r.isOK());
    EXPECT_NE(std::string::npos, r.message.find("not initialized"));

    auto r2 = s.getArgument("x");
    ASSERT_TRUE(std::holds_alternative<Status>(r2));
    EXPECT_FALSE(std::get<Status>(r2).isOK());
}

// ============================================================================
// storeArgument / getArgument
// ============================================================================

TEST_F(ArgumentStoreStandaloneTest, StoreAndRetrieveArgument) {
    auto arg = makeArg("a001", "kant");
    ASSERT_TRUE(store_->storeArgument(arg).isOK());

    auto result = store_->getArgument("a001");
    ASSERT_TRUE(std::holds_alternative<EthicalArgument>(result));
    const auto& got = std::get<EthicalArgument>(result);
    EXPECT_EQ("a001", got.id);
    EXPECT_EQ("kant", got.philosophy_school);
    EXPECT_EQ(ArgumentType::PRO, got.argument_type);
    EXPECT_EQ("Content for a001", got.content);
}

TEST_F(ArgumentStoreStandaloneTest, GetNonExistentArgumentReturnsError) {
    auto r = store_->getArgument("does-not-exist");
    ASSERT_TRUE(std::holds_alternative<Status>(r));
    EXPECT_FALSE(std::get<Status>(r).isOK());
    EXPECT_NE(std::string::npos, std::get<Status>(r).message.find("not found"));
}

TEST_F(ArgumentStoreStandaloneTest, StoreArgumentEmptyIdFails) {
    auto arg = makeArg("", "kant");
    auto s = store_->storeArgument(arg);
    EXPECT_FALSE(s.isOK());
    EXPECT_NE(std::string::npos, s.message.find("empty"));
}

TEST_F(ArgumentStoreStandaloneTest, StoreArgumentOverwritesExisting) {
    auto arg = makeArg("a002", "kant");
    ASSERT_TRUE(store_->storeArgument(arg).isOK());

    arg.content = "updated";
    ASSERT_TRUE(store_->storeArgument(arg).isOK());

    auto r = store_->getArgument("a002");
    ASSERT_TRUE(std::holds_alternative<EthicalArgument>(r));
    EXPECT_EQ("updated", std::get<EthicalArgument>(r).content);
}

// ============================================================================
// getArgumentsByPhilosophy
// ============================================================================

TEST_F(ArgumentStoreStandaloneTest, GetByPhilosophyReturnsOnlyMatching) {
    store_->storeArgument(makeArg("k1", "kant"));
    store_->storeArgument(makeArg("k2", "kant"));
    store_->storeArgument(makeArg("u1", "utilitarianism"));

    auto r = store_->getArgumentsByPhilosophy("kant", {}, 100);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(r));
    const auto& args = std::get<std::vector<EthicalArgument>>(r);
    EXPECT_EQ(2u, args.size());
    for (const auto& a : args) {
        EXPECT_EQ("kant", a.philosophy_school);
    }
}

TEST_F(ArgumentStoreStandaloneTest, GetByPhilosophyEmptyForUnknownSchool) {
    store_->storeArgument(makeArg("u1", "utilitarianism"));

    auto r = store_->getArgumentsByPhilosophy("virtue_ethics", {}, 100);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(r));
    EXPECT_TRUE(std::get<std::vector<EthicalArgument>>(r).empty());
}

TEST_F(ArgumentStoreStandaloneTest, GetByPhilosophyRespectLimit) {
    for (int i = 0; i < 6; ++i)
        store_->storeArgument(makeArg("lim-" + std::to_string(i), "kant"));

    auto r = store_->getArgumentsByPhilosophy("kant", {}, 3);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(r));
    EXPECT_LE(std::get<std::vector<EthicalArgument>>(r).size(), 3u);
}

TEST_F(ArgumentStoreStandaloneTest, GetByPhilosophyFiltersArgumentType) {
    store_->storeArgument(makeArg("pro-1",    "kant", ArgumentType::PRO));
    store_->storeArgument(makeArg("contra-1", "kant", ArgumentType::CONTRA));
    store_->storeArgument(makeArg("synth-1",  "kant", ArgumentType::SYNTHESIS));

    auto r = store_->getArgumentsByPhilosophy("kant", {ArgumentType::PRO}, 100);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(r));
    const auto& args = std::get<std::vector<EthicalArgument>>(r);
    EXPECT_EQ(1u, args.size());
    EXPECT_EQ(ArgumentType::PRO, args[0].argument_type);
}

TEST_F(ArgumentStoreStandaloneTest, GetByPhilosophyMultiTypeFilter) {
    store_->storeArgument(makeArg("p1", "kant", ArgumentType::PRO));
    store_->storeArgument(makeArg("c1", "kant", ArgumentType::CONTRA));
    store_->storeArgument(makeArg("r1", "kant", ArgumentType::REBUTTAL));

    auto r = store_->getArgumentsByPhilosophy(
        "kant", {ArgumentType::PRO, ArgumentType::CONTRA}, 100);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(r));
    EXPECT_EQ(2u, std::get<std::vector<EthicalArgument>>(r).size());
}

// ============================================================================
// storeDecision / getDecision
// ============================================================================

TEST_F(ArgumentStoreStandaloneTest, StoreAndRetrieveDecision) {
    auto d = makeDecision("dec-001", "dilemma-A");
    ASSERT_TRUE(store_->storeDecision(d).isOK());

    auto r = store_->getDecision("dec-001");
    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(r));
    const auto& got = std::get<EthicalDecision>(r);
    EXPECT_EQ("dec-001", got.decision_id);
    EXPECT_EQ("dilemma-A", got.dilemma_id);
    EXPECT_EQ("kant", got.primary_philosophy);
    EXPECT_DOUBLE_EQ(0.82, got.confidence);
}

TEST_F(ArgumentStoreStandaloneTest, GetNonExistentDecisionReturnsError) {
    auto r = store_->getDecision("no-such-decision");
    ASSERT_TRUE(std::holds_alternative<Status>(r));
    EXPECT_FALSE(std::get<Status>(r).isOK());
}

// ============================================================================
// storePhilosophyProfile / getPhilosophyProfile
// ============================================================================

TEST_F(ArgumentStoreStandaloneTest, StoreAndRetrieveProfile) {
    auto p = makeProfile("stoicism");
    ASSERT_TRUE(store_->storePhilosophyProfile(p).isOK());

    auto r = store_->getPhilosophyProfile("stoicism");
    ASSERT_TRUE(std::holds_alternative<PhilosophyProfile>(r));
    const auto& got = std::get<PhilosophyProfile>(r);
    EXPECT_EQ("stoicism", got.school_id);
    EXPECT_EQ("stoicism Ethics", got.name);
    ASSERT_EQ(1u, got.main_theses.size());
}

TEST_F(ArgumentStoreStandaloneTest, GetNonExistentProfileReturnsError) {
    auto r = store_->getPhilosophyProfile("nonexistent_school");
    ASSERT_TRUE(std::holds_alternative<Status>(r));
    EXPECT_FALSE(std::get<Status>(r).isOK());
}

// ============================================================================
// shutdown
// ============================================================================

TEST(ArgumentStoreShutdownStandalone, ShutdownPreventsAllOps) {
    ArgumentStore s;
    s.initialize(nullptr, nullptr);
    ASSERT_TRUE(s.storeArgument(makeArg("x1", "kant")).isOK());

    s.shutdown();

    EXPECT_FALSE(s.storeArgument(makeArg("x1", "kant")).isOK());
    auto gr = s.getArgument("x1");
    ASSERT_TRUE(std::holds_alternative<Status>(gr));
    EXPECT_FALSE(std::get<Status>(gr).isOK());
}

// ============================================================================
// Thread-safety (standalone in-memory)
// ============================================================================

TEST_F(ArgumentStoreStandaloneTest, ConcurrentStoreIsThreadSafe) {
    const int kThreads   = 8;
    const int kPerThread = 10;
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([this, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                std::string id = "th-" + std::to_string(t) + "-" + std::to_string(i);
                store_->storeArgument(makeArg(id, "kant"));
            }
        });
    }
    for (auto& th : threads) th.join();

    auto r = store_->getArgumentsByPhilosophy("kant", {}, 1000);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(r));
    EXPECT_EQ(static_cast<size_t>(kThreads * kPerThread),
              std::get<std::vector<EthicalArgument>>(r).size());
}
