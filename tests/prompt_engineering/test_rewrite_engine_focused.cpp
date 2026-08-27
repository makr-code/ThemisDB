/**
 * @file test_rewrite_engine_focused.cpp
 * @brief Focused compile-contract checks for prompt rewriting interfaces.
 */

#include <gtest/gtest.h>

#include <type_traits>

#include "prompt_engineering/rewrite_engine.h"
#include "prompt_engineering/rewrite_rule.h"

using namespace themis::prompt_engineering;

TEST(RewriteEngineFocused, RWContract01_RewritePhaseValuesAreDistinct) {
    EXPECT_NE(RewritePhase::PHASE_1_INPUT_NORMALIZATION,
              RewritePhase::PHASE_2_POLICY_ENFORCEMENT);
    EXPECT_NE(RewritePhase::PHASE_2_POLICY_ENFORCEMENT,
              RewritePhase::PHASE_3_NL_AQL_PREPROCESSING);
    EXPECT_NE(RewritePhase::PHASE_3_NL_AQL_PREPROCESSING,
              RewritePhase::PHASE_4_POST_GENERATION);
}

TEST(RewriteEngineFocused, RWContract02_InterfacesRemainAbstract) {
    EXPECT_TRUE(std::is_abstract_v<IRewriteEngine>);
    EXPECT_TRUE(std::is_abstract_v<IRewriteRule>);
}

TEST(RewriteEngineFocused, RWContract03_RewriteDocumentIsUsable) {
    RewriteDocument doc;
    doc.document_id = "doc-1";
    doc.content = "hello";

    EXPECT_EQ(doc.document_id, "doc-1");
    EXPECT_EQ(doc.content, "hello");
}
