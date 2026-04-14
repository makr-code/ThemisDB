/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_prompt_manager.cpp                            ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:49:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     63                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "prompt_engineering/prompt_manager.h"

using namespace themis::prompt_engineering;

TEST(PromptManagerTest, CreateAndGetTemplate) {
    PromptManager pm;

    PromptManager::PromptTemplate t;
    t.name = "summarize";
    t.version = "v1";
    t.content = "Summarize: {text}";

    auto created = pm.createTemplate(t);
    ASSERT_FALSE(created.id.empty());
    EXPECT_EQ(created.name, "summarize");

    auto fetched = pm.getTemplate(created.id);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->content, "Summarize: {text}");

    auto list = pm.listTemplates();
    EXPECT_GE(list.size(), 1u);
}

TEST(PromptManagerTest, AssignExperiment) {
    PromptManager pm;
    PromptManager::PromptTemplate t;
    t.name = "compare";
    t.version = "v2";
    t.content = "Prompt v2";
    auto created = pm.createTemplate(t);

    bool ok = pm.assignExperiment(created.id, "ab_test_42");
    EXPECT_TRUE(ok);

    auto fetched = pm.getTemplate(created.id);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_TRUE(fetched->metadata.contains("experiment_id"));
    EXPECT_EQ(fetched->metadata["experiment_id"].get<std::string>(), "ab_test_42");
}
