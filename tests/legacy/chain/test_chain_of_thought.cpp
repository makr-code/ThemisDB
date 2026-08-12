#include <gtest/gtest.h>
#include "prompt_engineering/chain_of_thought.h"

using namespace themis::prompt_engineering;

// ============================================================================
// Builder mode
// ============================================================================

TEST(ChainOfThoughtBuilderTest, EmptyBuildReturnsEmptyString) {
    ChainOfThoughtBuilder builder;
    EXPECT_TRUE(builder.build().empty());
    EXPECT_EQ(builder.stepCount(), 0u);
}

TEST(ChainOfThoughtBuilderTest, SingleStepAutoLabel) {
    ChainOfThoughtBuilder builder;
    builder.addStep("Identify the subject of the sentence.");
    EXPECT_EQ(builder.stepCount(), 1u);

    std::string result = builder.build();
    EXPECT_NE(result.find("Step 1:"), std::string::npos);
    EXPECT_NE(result.find("Identify the subject"), std::string::npos);
}

TEST(ChainOfThoughtBuilderTest, MultipleStepsAutoNumbered) {
    ChainOfThoughtBuilder builder;
    builder.addStep("Parse the input.")
           .addStep("Extract entities.")
           .addStep("Summarise findings.");

    EXPECT_EQ(builder.stepCount(), 3u);
    std::string result = builder.build();
    EXPECT_NE(result.find("Step 1:"), std::string::npos);
    EXPECT_NE(result.find("Step 2:"), std::string::npos);
    EXPECT_NE(result.find("Step 3:"), std::string::npos);
}

TEST(ChainOfThoughtBuilderTest, ExplicitLabelOverridesAutoNumber) {
    ChainOfThoughtBuilder builder;
    builder.addStep("First sub-task", "Observation");

    std::string result = builder.build();
    EXPECT_NE(result.find("Observation:"), std::string::npos);
    // Default "Step 1" should NOT appear when an explicit label is given
    EXPECT_EQ(result.find("Step 1:"), std::string::npos);
}

TEST(ChainOfThoughtBuilderTest, ReasoningStepLabel) {
    ChainOfThoughtBuilder builder;
    builder.addReasoningStep("The cat sat on the mat, therefore …");

    std::string result = builder.build();
    EXPECT_NE(result.find("Reasoning:"), std::string::npos);
    EXPECT_NE(result.find("The cat sat"), std::string::npos);
}

TEST(ChainOfThoughtBuilderTest, FinalAnswerAppended) {
    ChainOfThoughtBuilder builder;
    builder.addStep("Analyse the data.")
           .setFinalAnswer("The answer is 42.");

    std::string result = builder.build();
    EXPECT_NE(result.find("Answer:"), std::string::npos);
    EXPECT_NE(result.find("The answer is 42"), std::string::npos);
}

TEST(ChainOfThoughtBuilderTest, FinalAnswerOnlyNoPriorSteps) {
    ChainOfThoughtBuilder builder;
    builder.setFinalAnswer("42");

    std::string result = builder.build();
    EXPECT_NE(result.find("Answer:"), std::string::npos);
    EXPECT_NE(result.find("42"), std::string::npos);
}

TEST(ChainOfThoughtBuilderTest, ClearResetsState) {
    ChainOfThoughtBuilder builder;
    builder.addStep("Step A").setFinalAnswer("Done.");
    builder.clear();

    EXPECT_EQ(builder.stepCount(), 0u);
    EXPECT_TRUE(builder.build().empty());
}

TEST(ChainOfThoughtBuilderTest, ConfigAccessor) {
    CoTConfig cfg;
    cfg.step_prefix    = "Phase ";
    cfg.number_steps   = true;

    ChainOfThoughtBuilder builder(cfg);
    builder.addStep("Work.");

    std::string result = builder.build();
    EXPECT_NE(result.find("Phase 1:"), std::string::npos);
    EXPECT_EQ(builder.getConfig().step_prefix, "Phase ");
}

TEST(ChainOfThoughtBuilderTest, NumberingDisabledNoLabel) {
    CoTConfig cfg;
    cfg.number_steps = false;

    ChainOfThoughtBuilder builder(cfg);
    builder.addStep("Content without label.");

    std::string result = builder.build();
    // When numbering disabled and no explicit label, content still present
    EXPECT_NE(result.find("Content without label"), std::string::npos);
    // No "Step" prefix should appear
    EXPECT_EQ(result.find("Step"), std::string::npos);
}

// ============================================================================
// Static helpers
// ============================================================================

TEST(ChainOfThoughtBuilderTest, BuildZeroShot) {
    std::string result = ChainOfThoughtBuilder::buildZeroShot("What is 2+2?");
    EXPECT_NE(result.find("What is 2+2?"), std::string::npos);
    EXPECT_NE(result.find("step by step"), std::string::npos);
}

TEST(ChainOfThoughtBuilderTest, BuildFewShotIncludesExamples) {
    std::vector<std::pair<std::string, std::string>> examples = {
        {"What is 3+3?", "3+3=6"},
        {"What is 4+4?", "4+4=8"}
    };
    std::string result = ChainOfThoughtBuilder::buildFewShot(
        "What is 5+5?", examples);

    EXPECT_NE(result.find("Q: What is 3+3?"), std::string::npos);
    EXPECT_NE(result.find("A: 3+3=6"), std::string::npos);
    EXPECT_NE(result.find("Q: What is 5+5?"), std::string::npos);
    // Last "A:" should be present with no answer (eliciting the model)
    EXPECT_NE(result.find("A:"), std::string::npos);
}

TEST(ChainOfThoughtBuilderTest, BuildFewShotNoExamples) {
    std::string result = ChainOfThoughtBuilder::buildFewShot("Why is the sky blue?", {});
    EXPECT_NE(result.find("Q: Why is the sky blue?"), std::string::npos);
    EXPECT_NE(result.find("A:"), std::string::npos);
}

TEST(ChainOfThoughtBuilderTest, WrapWithCoTBasic) {
    std::string original = "Summarise this document.";
    std::string wrapped  = ChainOfThoughtBuilder::wrapWithCoT(original);

    EXPECT_NE(wrapped.find(original), std::string::npos);
    EXPECT_NE(wrapped.find("step by step"), std::string::npos);
}

TEST(ChainOfThoughtBuilderTest, WrapWithCoTExplicitSteps) {
    std::string original = "Classify the entity.";
    std::string wrapped  = ChainOfThoughtBuilder::wrapWithCoT(original, true);

    EXPECT_NE(wrapped.find("Step 1:"), std::string::npos);
    EXPECT_NE(wrapped.find(original), std::string::npos);
}

TEST(ChainOfThoughtBuilderTest, StepDelimiterApplied) {
    CoTConfig cfg;
    cfg.step_delimiter = "|||";
    ChainOfThoughtBuilder builder(cfg);
    builder.addStep("First.").addStep("Second.");

    std::string result = builder.build();
    EXPECT_NE(result.find("|||"), std::string::npos);
}
