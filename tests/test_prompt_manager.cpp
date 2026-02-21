/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_prompt_manager.cpp                            ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     67                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
