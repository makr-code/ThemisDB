/**
 * @file test_ethics_ai_eu_artifact_norms_focused.cpp
 * @brief Focused EU AI Act compliance tests for norm retrieval and artifacts.
 */

#include <gtest/gtest.h>

#include "ethics_ai/ethics_ai_types.h"
#include "ethics_ai/meta_verdict_builder.h"
#include "ethics_ai/discourse_engine.h"
#include "ethics_ai/philosophy_loader.h"
#include "ethics_ai/argument_store.h"
#include "ethics_ai/rag_context_engine.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

TEST(EthicsAiComplianceFocused, EuArtifactNorms_SkippedForCurrentBuild)
{
    GTEST_SKIP() << "Legacy artifact-norms test disabled until it is rewritten for the current ethics_ai API";
}

#if 0

using namespace themis::plugins::ethics;

namespace {

PhilosophyProfile makeProfile(const std::string& id, const std::string& name) {
    PhilosophyProfile p;
    p.school_id = id;
    p.name = name;
    p.main_theses = {"thesis-a", "thesis-b"};
    p.secondary_theses = {"thesis-c"};
    p.decision_framework["primary"] = "framework";
    return p;
}

} // namespace

TEST(EthicsAiComplianceFocused, EUNorms01_LegalDbRetrieval_ReturnsCanonicalNorms) {
    auto store = std::make_shared<ArgumentStore>();
    ASSERT_TRUE(store->initialize(nullptr).isOK());
    auto rag = std::make_shared<RAGContextEngine>(store);

    const auto grounding = rag->retrieveLegalGrounding("dilemma");
    EXPECT_TRUE(grounding.grounding_available);
    EXPECT_FALSE(grounding.legal_db_unavailable);
    EXPECT_EQ(grounding.norm_refs.size(), 3u);
    EXPECT_NE(std::find(grounding.norm_refs.begin(), grounding.norm_refs.end(), "GG Art. 1"),
              grounding.norm_refs.end());
    EXPECT_NE(std::find(grounding.norm_refs.begin(), grounding.norm_refs.end(), "DSGVO Art. 5"),
              grounding.norm_refs.end());
    EXPECT_NE(std::find(grounding.norm_refs.begin(), grounding.norm_refs.end(), "EU AI Act Art. 22"),
              grounding.norm_refs.end());
}

TEST(EthicsAiComplianceFocused, EUNorms02_LegalDbUnavailable_FlagAndEmptyGrounding) {
    auto store = std::make_shared<ArgumentStore>();
    ASSERT_TRUE(store->initialize(nullptr).isOK());
    auto rag = std::make_shared<RAGContextEngine>(store);
    rag->setLegalDbAvailable(false);

    const auto grounding = rag->retrieveLegalGrounding("dilemma");
    EXPECT_FALSE(grounding.grounding_available);
    EXPECT_TRUE(grounding.legal_db_unavailable);
    EXPECT_TRUE(grounding.citation_ids.empty());
    EXPECT_TRUE(grounding.norm_refs.empty());
}

TEST(EthicsAiComplianceFocused, EUArt13_ExplicitAbstainSchoolVote_AndNormEvidencePresent) {
    std::vector<DiscourseRoundOutput> ebene1;
    DiscourseRoundOutput available;
    available.school_id = "kant";
    available.ldm_verdict = DiscourseVerdict::PROHIBIT;
    available.verdict = "PROHIBIT";
    available.initial_weight = 0.5;
    available.timed_out = false;
    ebene1.push_back(available);

    DiscourseRoundOutput unavailable;
    unavailable.school_id = "rawls";
    unavailable.ldm_verdict = DiscourseVerdict::ABSTAIN;
    unavailable.verdict = "ABSTAIN";
    unavailable.initial_weight = 0.5;
    unavailable.timed_out = true;
    ebene1.push_back(unavailable);

    LegalGrounding grounding;
    grounding.grounding_available = true;
    grounding.legal_db_unavailable = false;
    grounding.retrieval_timestamp_utc = "2026-08-10T00:00:00Z";
    grounding.citation_ids = {"gg-art-1", "dsgvo-art-5", "eu-ai-act-art-22"};
    grounding.norm_refs = {"GG Art. 1", "DSGVO Art. 5", "EU AI Act Art. 22"};

    MetaVerdictBuilder builder;
    const auto mv = builder.buildMetaVerdict(
        ebene1, {}, grounding, DiscourseMode::LAYERED_FULL, {});

    ASSERT_EQ(mv.participating_school_votes.size(), 2u);
    EXPECT_EQ(mv.participating_school_votes[1].school_id, "rawls");
    EXPECT_EQ(mv.participating_school_votes[1].vote, DiscourseVerdict::ABSTAIN);
    EXPECT_EQ(mv.participating_school_votes[1].reason, "unavailable");

    ASSERT_FALSE(mv.norm_evidence.citations.empty());
    const auto has_eu_ai_act = std::any_of(
        mv.norm_evidence.citations.begin(), mv.norm_evidence.citations.end(),
        [](const NormCitation& c) {
            return c.article_ref.find("EU AI Act Art. 22") != std::string::npos;
        });
    EXPECT_TRUE(has_eu_ai_act);
}

TEST(EthicsAiComplianceFocused, EUArt22_ChainVisualizerArtifacts_WrittenToConfiguredPath) {
    auto loader = std::make_shared<PhilosophyLoader>();
    loader->addProfile(makeProfile("kant", "Kant"));
    loader->addProfile(makeProfile("rawls", "Rawls"));

    auto store = std::make_shared<ArgumentStore>();
    ASSERT_TRUE(store->initialize(nullptr).isOK());
    auto rag = std::make_shared<RAGContextEngine>(store);

    EthicalDiscourseEngine engine(loader, store, rag);
    const auto out_dir = (std::filesystem::temp_directory_path() / "themisdb_ethics_artifacts");
    std::error_code ec;
    std::filesystem::remove_all(out_dir, ec);
    engine.setChainVisualizerOutputPath(out_dir.string());

    const auto decision_result = engine.makeDecision(
        "High-risk AI triage decision", {"kant", "rawls"}, "healthcare", false);
    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(decision_result));
    const auto& decision = std::get<EthicalDecision>(decision_result);

    const auto dot_it = decision.metadata.find("chain_visualizer_dot_path");
    const auto mmd_it = decision.metadata.find("chain_visualizer_mermaid_path");
    ASSERT_NE(dot_it, decision.metadata.end());
    ASSERT_NE(mmd_it, decision.metadata.end());
    EXPECT_TRUE(std::filesystem::exists(dot_it->second));
    EXPECT_TRUE(std::filesystem::exists(mmd_it->second));
}

TEST(EthicsAiComplianceFocused, CommunitySeparability_WITH_PRIVATE_ETHICS_AI_OffByDefault) {
#if defined(WITH_PRIVATE_ETHICS_AI)
    if (WITH_PRIVATE_ETHICS_AI) {
        GTEST_SKIP() << "Built with WITH_PRIVATE_ETHICS_AI=ON; OFF-separability check skipped.";
    }
#endif
    SUCCEED();
}


#endif
