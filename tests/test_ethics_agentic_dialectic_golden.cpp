/*
 * Comprehensive golden-dataset test for the Ethics AI dialectic workflow.
 *
 * Scope:
 * - Article-driven dialectic initialization
 * - Agentic decision synthesis across philosophy schools
 * - Validation against a golden dataset (exact decision text + metrics)
 * - Cross-check of evaluator scores for deterministic regression coverage
 */

#include <gtest/gtest.h>

#include "ethics_ai/argument_store.h"
#include "ethics_ai/discourse_engine.h"
#include "ethics_ai/ethics_evaluator.h"
#include "ethics_ai/philosophy_loader.h"
#include "ethics_ai/rag_context_engine.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace themis::plugins::ethics;

namespace {

class ScopedEnvVar {
public:
    ScopedEnvVar(const char* name, const char* value)
        : name_(name), had_value_(std::getenv(name) != nullptr) {
        if (had_value_) {
            original_value_ = std::getenv(name);
        }
        set(value);
    }

    ~ScopedEnvVar() {
        if (had_value_) {
            set(original_value_.c_str());
        } else {
            unset();
        }
    }

private:
    void set(const char* value) {
#if defined(_WIN32)
        _putenv_s(name_.c_str(), value ? value : "");
#else
        setenv(name_.c_str(), value ? value : "", 1);
#endif
    }

    void unset() {
#if defined(_WIN32)
        _putenv_s(name_.c_str(), "");
#else
        unsetenv(name_.c_str());
#endif
    }

    std::string name_;
    bool had_value_;
    std::string original_value_;
};

struct GoldenDialecticCase {
    std::string name;
    std::string article_text;
    std::vector<std::string> schools;
    std::vector<ArgumentType> expected_argument_types;
    std::string category;
    bool use_rag;

    std::string expected_primary;
    std::string expected_decision_text;
    double expected_consensus;
    double expected_overall_score;
};

const std::vector<GoldenDialecticCase> kGoldenDialecticDataset = {
    {
        "Public Surveillance In Train Stations",
        "An investigative article reports that a city plans to deploy AI-driven "
        "face recognition in major train stations to reduce violent crime. Civil "
        "rights organizations warn about false positives and discrimination risks.",
        {"kant", "utilitarianism"},
        {ArgumentType::PRO, ArgumentType::CONTRA},
        "public_safety",
        true,
        "kant",
        "After considering perspectives from kant and utilitarianism, the primary "
        "recommendation based on kant is to proceed with careful consideration of "
        "all ethical dimensions.",
        0.70,
        0.85625,
    },
    {
        "Hospital Triage Under Resource Scarcity",
        "A medical ethics article discusses ICU triage during a pandemic. The "
        "hospital has limited ventilators and must prioritize treatment while "
        "maintaining fairness and protecting vulnerable groups.",
        {"rawls", "care_ethics", "utilitarianism"},
        {ArgumentType::PRO, ArgumentType::CONTRA, ArgumentType::REBUTTAL},
        "healthcare",
        true,
        "rawls",
        "After considering perspectives from rawls, care_ethics and utilitarianism, "
        "the primary recommendation based on rawls is to proceed with careful "
        "consideration of all ethical dimensions.",
        0.70,
        0.85625,
    },
    {
        "Automated Hiring For High-Volume Recruiting",
        "A labor policy article describes a company using an AI model to screen "
        "tens of thousands of applications. Unions demand transparent criteria "
        "and stronger bias audits.",
        {"kant"},
        {ArgumentType::PRO},
        "employment",
        false,
        "kant",
        "After considering perspectives from kant, the primary recommendation based "
        "on kant is to proceed with careful consideration of all ethical dimensions.",
        1.0,
        0.85125,
    },
    {
        "Cross-Sector Autonomous Decision Governance",
        "A policy whitepaper proposes a shared autonomous decision platform for "
        "healthcare, transit, and hiring workflows. Civil society groups request "
        "strict fairness guarantees and explicit appeal paths.",
        {"rawls", "care_ethics", "utilitarianism", "kant"},
        {ArgumentType::PRO, ArgumentType::CONTRA, ArgumentType::REBUTTAL, ArgumentType::SYNTHESIS},
        "governance",
        true,
        "rawls",
        "After considering perspectives from rawls, care_ethics, utilitarianism and "
        "kant, the primary recommendation based on rawls is to proceed with careful "
        "consideration of all ethical dimensions.",
        0.70,
        0.855,
    },
};

void registerProfiles(const std::shared_ptr<PhilosophyLoader>& loader) {
    PhilosophyProfile kant;
    kant.school_id = "kant";
    kant.name = "Kantian Ethics";
    kant.main_theses = {
        "Act only according to maxims that can be universalized.",
    };
    loader->addProfile(kant);

    PhilosophyProfile util;
    util.school_id = "utilitarianism";
    util.name = "Utilitarianism";
    util.main_theses = {
        "Choose the action that maximizes well-being for the greatest number.",
    };
    loader->addProfile(util);

    PhilosophyProfile care;
    care.school_id = "care_ethics";
    care.name = "Ethics of Care";
    care.main_theses = {
        "Prioritize relationships, dependency, and context-sensitive care.",
    };
    loader->addProfile(care);

    PhilosophyProfile rawls;
    rawls.school_id = "rawls";
    rawls.name = "Rawlsian Justice";
    rawls.main_theses = {
        "Social arrangements should protect the least advantaged.",
    };
    loader->addProfile(rawls);
}

std::vector<EthicalArgument> collectDecisionArguments(
    const std::shared_ptr<ArgumentStore>& store,
    const std::vector<std::string>& schools) {
    std::vector<EthicalArgument> arguments;
    arguments.reserve(schools.size());

    for (const auto& school : schools) {
        auto by_school = store->getArgumentsByPhilosophy(school, {}, 10);
        EXPECT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(by_school));
        if (!std::holds_alternative<std::vector<EthicalArgument>>(by_school)) {
            continue;
        }

        const auto& list = std::get<std::vector<EthicalArgument>>(by_school);
        EXPECT_EQ(list.size(), 1u) << "Standalone engine should emit one argument per school";
        if (!list.empty()) {
            arguments.push_back(list.front());
        }
    }

    return arguments;
}

} // namespace

TEST(EthicsAgenticDialecticGoldenTest, ArticleDialecticMatchesGoldenDataset) {
    for (const auto& golden : kGoldenDialecticDataset) {
        SCOPED_TRACE(golden.name);

        auto loader = std::make_shared<PhilosophyLoader>();
        auto store = std::make_shared<ArgumentStore>();
        ASSERT_TRUE(store->initialize(nullptr, nullptr).isOK());
        auto rag = std::make_shared<RAGContextEngine>(store);
        auto engine = std::make_unique<EthicalDiscourseEngine>(loader, store, rag);
        auto evaluator = std::make_unique<EthicsEvaluator>();

        registerProfiles(loader);

        auto debate_init = engine->initializeDebate(
            golden.article_text,
            golden.schools,
            golden.category);
        ASSERT_TRUE(std::holds_alternative<DebateInitialization>(debate_init));
        const auto& debate = std::get<DebateInitialization>(debate_init);
        EXPECT_FALSE(debate.debate_id.empty());
        EXPECT_EQ(debate.philosophy_schools, golden.schools);
        EXPECT_EQ(debate.category, golden.category);

        auto decision_result = engine->makeDecision(
            golden.article_text,
            golden.schools,
            golden.category,
            golden.use_rag);
        ASSERT_TRUE(std::holds_alternative<EthicalDecision>(decision_result));
        const auto& decision = std::get<EthicalDecision>(decision_result);

        EXPECT_EQ(decision.primary_philosophy, golden.expected_primary);
        EXPECT_EQ(decision.decision_text, golden.expected_decision_text);
        EXPECT_EQ(decision.supporting_philosophies, golden.schools);
        EXPECT_NEAR(decision.consensus_level, golden.expected_consensus, 1e-12);

        auto arguments = collectDecisionArguments(store, golden.schools);
        ASSERT_EQ(arguments.size(), golden.schools.size());
        ASSERT_EQ(golden.expected_argument_types.size(), golden.schools.size());
        for (size_t i = 0; i < arguments.size(); ++i) {
            EXPECT_EQ(arguments[i].argument_type, golden.expected_argument_types[i]);
        }

        auto eval_result = evaluator->evaluateDecision(decision, arguments);
        ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(eval_result));
        const auto& eval = std::get<EthicsEvaluationResult>(eval_result);

        EXPECT_NEAR(eval.overall_score, golden.expected_overall_score, 1e-9);
        EXPECT_GT(eval.decision_quality_score, 0.90);
        EXPECT_GE(eval.fairness_score, 0.80);
        EXPECT_DOUBLE_EQ(eval.detailed_metrics.at("num_arguments"),
                         static_cast<double>(golden.schools.size()));
    }
}

TEST(EthicsAgenticDialecticGoldenTest, DialecticStepOrderIsDeterministicForFourSchools) {
    auto loader = std::make_shared<PhilosophyLoader>();
    auto store = std::make_shared<ArgumentStore>();
    ASSERT_TRUE(store->initialize(nullptr, nullptr).isOK());
    auto rag = std::make_shared<RAGContextEngine>(store);
    auto engine = std::make_unique<EthicalDiscourseEngine>(loader, store, rag);

    registerProfiles(loader);

    const std::vector<std::string> schools = {
        "rawls", "care_ethics", "utilitarianism", "kant"
    };

    auto decision_result = engine->makeDecision(
        "A cross-sector governance framework is proposed for healthcare, mobility, and employment systems.",
        schools,
        "governance",
        true);
    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(decision_result));

    auto arguments = collectDecisionArguments(store, schools);
    ASSERT_EQ(arguments.size(), schools.size());

    const std::vector<ArgumentType> expected = {
        ArgumentType::PRO,
        ArgumentType::CONTRA,
        ArgumentType::REBUTTAL,
        ArgumentType::SYNTHESIS,
    };
    const std::vector<ArgumentType> wrong_order = {
        ArgumentType::PRO,
        ArgumentType::REBUTTAL,
        ArgumentType::CONTRA,
        ArgumentType::SYNTHESIS,
    };

    std::vector<ArgumentType> actual;
    actual.reserve(arguments.size());
    for (const auto& arg : arguments) {
        actual.push_back(arg.argument_type);
    }

    EXPECT_EQ(actual, expected);
    EXPECT_NE(actual, wrong_order);
}

TEST(EthicsAgenticDialecticGoldenTest, LlmEnabledInvalidModelFallsBackDeterministically) {
    ScopedEnvVar llm_enabled("THEMIS_ETHICS_LLM_INFERENCE", "1");
    ScopedEnvVar llm_model("THEMIS_ETHICS_LLM_MODEL_PATH", "models/does_not_exist.gguf");

    auto loader = std::make_shared<PhilosophyLoader>();
    auto store = std::make_shared<ArgumentStore>();
    ASSERT_TRUE(store->initialize(nullptr, nullptr).isOK());
    auto rag = std::make_shared<RAGContextEngine>(store);
    auto engine = std::make_unique<EthicalDiscourseEngine>(loader, store, rag);

    registerProfiles(loader);

    auto decision_result = engine->makeDecision(
        "A municipal AI policy is proposed for schools and transport systems.",
        {"kant", "utilitarianism"},
        "public_policy",
        true);
    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(decision_result));

    auto arguments = collectDecisionArguments(store, {"kant", "utilitarianism"});
    ASSERT_EQ(arguments.size(), 2u);
    EXPECT_EQ(arguments[0].argument_type, ArgumentType::PRO);
    EXPECT_EQ(arguments[1].argument_type, ArgumentType::CONTRA);

    // Invalid model path must not break the workflow and should trigger deterministic fallback text.
    EXPECT_NE(arguments[0].content.find("we should consider the ethical implications carefully."),
              std::string::npos);
    EXPECT_NE(arguments[1].content.find("there are substantial objections that should be weighed before acting."),
              std::string::npos);
}

TEST(EthicsAgenticDialecticGoldenTest, LlmEnabledWithRealModelGeneratesNonFallbackContent) {
    const char* integration_model = std::getenv("THEMIS_ETHICS_LLM_INTEGRATION_MODEL_PATH");
    if (!integration_model || std::string(integration_model).empty()) {
        GTEST_SKIP() << "Set THEMIS_ETHICS_LLM_INTEGRATION_MODEL_PATH to run real llama.cpp integration.";
    }

    if (!std::filesystem::exists(integration_model)) {
        GTEST_SKIP() << "Model path does not exist: " << integration_model;
    }

    ScopedEnvVar llm_enabled("THEMIS_ETHICS_LLM_INFERENCE", "1");
    ScopedEnvVar llm_model("THEMIS_ETHICS_LLM_MODEL_PATH", integration_model);

    auto loader = std::make_shared<PhilosophyLoader>();
    auto store = std::make_shared<ArgumentStore>();
    ASSERT_TRUE(store->initialize(nullptr, nullptr).isOK());
    auto rag = std::make_shared<RAGContextEngine>(store);
    auto engine = std::make_unique<EthicalDiscourseEngine>(loader, store, rag);

    registerProfiles(loader);

    auto decision_result = engine->makeDecision(
        "A city ethics board evaluates autonomous incident-response drones in dense neighborhoods.",
        {"kant", "utilitarianism", "rawls"},
        "public_safety",
        true);
    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(decision_result));

    auto arguments = collectDecisionArguments(store, {"kant", "utilitarianism", "rawls"});
    ASSERT_EQ(arguments.size(), 3u);

    for (const auto& argument : arguments) {
        EXPECT_FALSE(argument.content.empty());
    }

    // In real integration mode, generated text should differ from deterministic fallback signatures.
    EXPECT_EQ(arguments[0].content.find("we should consider the ethical implications carefully."),
              std::string::npos);
    EXPECT_EQ(arguments[1].content.find("there are substantial objections that should be weighed before acting."),
              std::string::npos);
    EXPECT_EQ(arguments[2].content.find("the strongest objections can be addressed with transparent safeguards."),
              std::string::npos);
}

TEST(EthicsAgenticDialecticGoldenTest, LlmModelPathAloneDoesNotEnableInference) {
    const char* integration_model = std::getenv("THEMIS_ETHICS_LLM_INTEGRATION_MODEL_PATH");
    if (!integration_model || std::string(integration_model).empty()) {
        GTEST_SKIP() << "Set THEMIS_ETHICS_LLM_INTEGRATION_MODEL_PATH to validate opt-in behavior with a real model path.";
    }

    if (!std::filesystem::exists(integration_model)) {
        GTEST_SKIP() << "Model path does not exist: " << integration_model;
    }

    ScopedEnvVar llm_enabled("THEMIS_ETHICS_LLM_INFERENCE", "0");
    ScopedEnvVar llm_model("THEMIS_ETHICS_LLM_MODEL_PATH", integration_model);

    auto loader = std::make_shared<PhilosophyLoader>();
    auto store = std::make_shared<ArgumentStore>();
    ASSERT_TRUE(store->initialize(nullptr, nullptr).isOK());
    auto rag = std::make_shared<RAGContextEngine>(store);
    auto engine = std::make_unique<EthicalDiscourseEngine>(loader, store, rag);

    registerProfiles(loader);

    auto decision_result = engine->makeDecision(
        "A national policy board evaluates deployment criteria for autonomous enforcement systems.",
        {"kant", "utilitarianism"},
        "public_policy",
        true);
    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(decision_result));

    auto arguments = collectDecisionArguments(store, {"kant", "utilitarianism"});
    ASSERT_EQ(arguments.size(), 2u);

    EXPECT_NE(arguments[0].content.find("we should consider the ethical implications carefully."),
              std::string::npos);
    EXPECT_NE(arguments[1].content.find("there are substantial objections that should be weighed before acting."),
              std::string::npos);
}
