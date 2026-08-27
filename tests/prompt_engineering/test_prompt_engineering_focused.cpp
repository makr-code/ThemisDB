/**
 * @file test_prompt_engineering_focused.cpp
 * @brief API-aligned focused smoke tests for prompt_engineering module.
 */

#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "prompt_engineering/feedback_collector.h"
#include "prompt_engineering/prompt_engineering_metrics.h"
#include "prompt_engineering/prompt_evaluator.h"
#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/prompt_version_control.h"

using namespace themis::prompt_engineering;

TEST(PromptEngineeringFocused, PromptManagerCreateGetValidateAndInject) {
    PromptManager mgr;

    PromptManager::PromptTemplate t;
    t.name = "focused_template";
    t.version = "v1";
    t.content = "Question: {query}";
    t.description = "focused smoke";

    const auto created = mgr.createTemplate(t);
    ASSERT_FALSE(created.id.empty());

    const auto fetched = mgr.getTemplate(created.id);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->name, "focused_template");

    const auto validation = PromptManager::validateTemplate(created);
    EXPECT_TRUE(validation.valid);

    const auto rendered = mgr.getPromptWithContext(
        created.id,
        std::unordered_map<std::string, std::string>{{"query", "What is ThemisDB?"}}
    );
    ASSERT_TRUE(rendered.has_value());
    EXPECT_NE(rendered->find("What is ThemisDB?"), std::string::npos);
}

TEST(PromptEngineeringFocused, PromptVersionControlCommitAndHistory) {
    PromptVersionControl vcs;
    const std::string prompt_id = "focused_prompt";

    const auto v1 = vcs.commit(prompt_id, "content v1", "init");
    const auto v2 = vcs.commit(prompt_id, "content v2", "update");

    EXPECT_FALSE(v1.empty());
    EXPECT_FALSE(v2.empty());

    const auto history = vcs.getHistory(prompt_id, "main", 10);
    EXPECT_GE(history.size(), 2u);
}

TEST(PromptEngineeringFocused, FeedbackCollectorRecordAndStats) {
    FeedbackCollector collector;
    const auto id = collector.recordFeedback(
        "tmpl-1",
        "query",
        "response",
        FeedbackType::USER_POSITIVE,
        "looks good",
        0.2
    );

    EXPECT_FALSE(id.empty());

    const auto stats = collector.getStats("tmpl-1");
    EXPECT_GE(stats.total_feedback, 1u);
}

TEST(PromptEngineeringFocused, OptimizerEvaluatorAndMetricsSmoke) {
    PromptOptimizer optimizer;
    PromptEvaluator evaluator;
    PromptEngineeringMetrics metrics;

    const std::vector<TestCase> tests = {
        {"input", "expected", {}}
    };

    const auto opt = optimizer.optimize(
        "Respond with expected",
        tests,
        [](const std::string& prompt, const std::vector<TestCase>&) {
            return prompt.empty() ? 0.0 : 0.8;
        }
    );
    EXPECT_FALSE(opt.optimized_prompt.empty());

    const auto eval = evaluator.evaluateSingle("expected", "expected");
    EXPECT_DOUBLE_EQ(eval.exact_match, 1.0);

    metrics.recordPromptExecution("tmpl-1", true, 5.0);
    const auto exported = metrics.exportMetrics();
    EXPECT_FALSE(exported.empty());
}
