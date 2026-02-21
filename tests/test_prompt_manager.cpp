/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_prompt_manager.cpp                            ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:35:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     67                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
