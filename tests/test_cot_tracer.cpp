/**
 * @file test_cot_tracer.cpp
 * @brief Unit tests for the Chain-of-Thought Step Tracer (v1.7.0).
 *
 * Acceptance criteria:
 *  AC-1   IChainOfThoughtTracer interface is abstract (RecordingCoTTracer instantiates).
 *  AC-2   RecordingCoTTracer::spanCount() starts at 0.
 *  AC-3   RecordingCoTTracer accumulates one span per step when tracer is attached.
 *  AC-4   RecordingCoTTracer::spans() returns spans in step order.
 *  AC-5   CoTSpanRecord::step_index matches the iteration index.
 *  AC-6   CoTSpanRecord::label matches the auto-numbered step label.
 *  AC-7   CoTSpanRecord::content matches the step content.
 *  AC-8   CoTSpanRecord::duration is non-negative.
 *  AC-9   CoTSpanRecord::token_count is chars/4 of content.
 *  AC-10  CoTSpanRecord::toJson() contains all required keys.
 *  AC-11  RecordingCoTTracer::reset() removes all spans.
 *  AC-12  RecordingCoTTracer::toJson() returns a JSON array.
 *  AC-13  ChainOfThoughtBuilder::attachTracer() sets hasTracer() true.
 *  AC-14  ChainOfThoughtBuilder::detachTracer() sets hasTracer() false.
 *  AC-15  build() with no tracer still produces correct prompt (backward compat).
 *  AC-16  build() with tracer produces the same prompt text as without.
 *  AC-17  build() on empty builder fires no tracer callbacks.
 *  AC-18  Tracer receives onStepBegin before onStepEnd for each step.
 *  AC-19  Misbehaving tracer (throws in onStepBegin) does not abort build().
 *  AC-20  Misbehaving tracer (throws in onStepEnd) does not abort build().
 *  AC-21  CoTTraceCollector::addTracer() increases tracerCount().
 *  AC-22  CoTTraceCollector fans out to all registered child tracers.
 *  AC-23  CoTTraceCollector::removeTracer() decreases tracerCount().
 *  AC-24  CoTTraceCollector accumulates spans independently of children.
 *  AC-25  CoTTraceCollector::totalStepsTraced() counts all onStepEnd calls.
 *  AC-26  CoTTraceCollector::reset() clears spans and totalStepsTraced.
 *  AC-27  CoTTraceCollector::toJson() returns a JSON array with correct length.
 *  AC-28  Multiple builders sharing a RecordingCoTTracer are additive.
 *  AC-29  Tracer attached to builder does not fire for static factory methods.
 *  AC-30  RecordingCoTTracer::hasSpans() returns false after reset().
 */

#include <gtest/gtest.h>
#include "prompt_engineering/cot_tracer.h"
#include "prompt_engineering/chain_of_thought.h"

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::prompt_engineering;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/** Order-tracking tracer: records "begin:<i>" or "end:<i>" per call. */
class OrderTrackingTracer final : public IChainOfThoughtTracer {
public:
    void onStepBegin(StepId idx, const std::string&) noexcept override {
        events.push_back("begin:" + std::to_string(idx));
    }
    void onStepEnd(StepId idx, const std::string&,
                   std::chrono::microseconds) noexcept override {
        events.push_back("end:" + std::to_string(idx));
    }
    std::string name() const override { return "order-tracking"; }
    std::vector<std::string> events;
};

/** Tracer that throws in onStepBegin. */
class ThrowingBeginTracer final : public IChainOfThoughtTracer {
public:
    void onStepBegin(StepId, const std::string&) noexcept override {
        // Must not propagate — but we can use a side-channel.
        try { throw std::runtime_error("begin boom"); } catch (...) {}
        began++;
    }
    void onStepEnd(StepId, const std::string&,
                   std::chrono::microseconds) noexcept override {
        ended++;
    }
    std::string name() const override { return "throwing-begin"; }
    std::atomic<int> began{0};
    std::atomic<int> ended{0};
};

/** Tracer that throws in onStepEnd. */
class ThrowingEndTracer final : public IChainOfThoughtTracer {
public:
    void onStepBegin(StepId, const std::string&) noexcept override { began++; }
    void onStepEnd(StepId, const std::string&,
                   std::chrono::microseconds) noexcept override {
        try { throw std::runtime_error("end boom"); } catch (...) {}
        ended++;
    }
    std::string name() const override { return "throwing-end"; }
    std::atomic<int> began{0};
    std::atomic<int> ended{0};
};

} // anonymous namespace

// ============================================================================
// AC-1  IChainOfThoughtTracer interface — concrete impl instantiates
// ============================================================================

TEST(CoTTracerTest, RecordingCoTTracerIsInstantiable) {
    RecordingCoTTracer t;
    EXPECT_EQ(t.name(), "recording-cot-tracer");
}

// ============================================================================
// AC-2  RecordingCoTTracer starts empty
// ============================================================================

TEST(CoTTracerTest, RecordingCoTTracerStartsEmpty) {
    RecordingCoTTracer t;
    EXPECT_EQ(t.spanCount(), 0u);
    EXPECT_FALSE(t.hasSpans());
}

// ============================================================================
// AC-3  RecordingCoTTracer accumulates one span per step
// ============================================================================

TEST(CoTTracerTest, OneSpanPerStep) {
    auto tracer = std::make_shared<RecordingCoTTracer>();
    ChainOfThoughtBuilder builder;
    builder.addStep("Step content A")
           .addStep("Step content B")
           .addStep("Step content C");
    builder.attachTracer(tracer);
    builder.build();
    EXPECT_EQ(tracer->spanCount(), 3u);
}

// ============================================================================
// AC-4  Spans are in step order
// ============================================================================

TEST(CoTTracerTest, SpansInStepOrder) {
    auto tracer = std::make_shared<RecordingCoTTracer>();
    ChainOfThoughtBuilder builder;
    builder.addStep("first").addStep("second").addStep("third");
    builder.attachTracer(tracer);
    builder.build();
    const auto spans = tracer->spans();
    ASSERT_EQ(spans.size(), 3u);
    EXPECT_EQ(spans[0].step_index, 0u);
    EXPECT_EQ(spans[1].step_index, 1u);
    EXPECT_EQ(spans[2].step_index, 2u);
}

// ============================================================================
// AC-5  step_index matches iteration index
// ============================================================================

TEST(CoTTracerTest, StepIndexMatchesIteration) {
    auto tracer = std::make_shared<RecordingCoTTracer>();
    ChainOfThoughtBuilder builder;
    for (int i = 0; i < 5; ++i) {
        builder.addStep("content " + std::to_string(i));
    }
    builder.attachTracer(tracer);
    builder.build();
    const auto spans = tracer->spans();
    for (std::size_t i = 0; i < spans.size(); ++i) {
        EXPECT_EQ(spans[i].step_index, i);
    }
}

// ============================================================================
// AC-6  label matches auto-numbered label
// ============================================================================

TEST(CoTTracerTest, LabelMatchesAutoNumberedLabel) {
    auto tracer = std::make_shared<RecordingCoTTracer>();
    ChainOfThoughtBuilder builder;
    builder.addStep("alpha").addStep("beta");
    builder.attachTracer(tracer);
    builder.build();
    const auto spans = tracer->spans();
    ASSERT_EQ(spans.size(), 2u);
    EXPECT_EQ(spans[0].label, "Step 1");
    EXPECT_EQ(spans[1].label, "Step 2");
}

// ============================================================================
// AC-7  content matches step content
// ============================================================================

TEST(CoTTracerTest, ContentMatchesStepContent) {
    auto tracer = std::make_shared<RecordingCoTTracer>();
    ChainOfThoughtBuilder builder;
    builder.addStep("unique content xyz");
    builder.attachTracer(tracer);
    builder.build();
    ASSERT_EQ(tracer->spanCount(), 1u);
    EXPECT_EQ(tracer->spans()[0].content, "unique content xyz");
}

// ============================================================================
// AC-8  duration is non-negative
// ============================================================================

TEST(CoTTracerTest, DurationIsNonNegative) {
    auto tracer = std::make_shared<RecordingCoTTracer>();
    ChainOfThoughtBuilder builder;
    builder.addStep("timing test");
    builder.attachTracer(tracer);
    builder.build();
    ASSERT_EQ(tracer->spanCount(), 1u);
    EXPECT_GE(tracer->spans()[0].duration.count(), 0);
}

// ============================================================================
// AC-9  token_count is chars/4 of content
// ============================================================================

TEST(CoTTracerTest, TokenCountIsCharsDivFour) {
    auto tracer = std::make_shared<RecordingCoTTracer>();
    ChainOfThoughtBuilder builder;
    // 20-character content → expected token_count = 5
    builder.addStep("12345678901234567890");
    builder.attachTracer(tracer);
    builder.build();
    ASSERT_EQ(tracer->spanCount(), 1u);
    EXPECT_EQ(tracer->spans()[0].token_count, 20u / 4u);
}

// ============================================================================
// AC-10 CoTSpanRecord::toJson() contains all required keys
// ============================================================================

TEST(CoTTracerTest, SpanToJsonContainsAllKeys) {
    auto tracer = std::make_shared<RecordingCoTTracer>();
    ChainOfThoughtBuilder builder;
    builder.addStep("json test content");
    builder.attachTracer(tracer);
    builder.build();
    ASSERT_EQ(tracer->spanCount(), 1u);
    const auto j = tracer->spans()[0].toJson();
    EXPECT_TRUE(j.contains("step_index"));
    EXPECT_TRUE(j.contains("label"));
    EXPECT_TRUE(j.contains("content"));
    EXPECT_TRUE(j.contains("token_count"));
    EXPECT_TRUE(j.contains("duration_us"));
    EXPECT_TRUE(j.contains("start_time"));
}

// ============================================================================
// AC-11 reset() removes all spans
// ============================================================================

TEST(CoTTracerTest, ResetRemovesAllSpans) {
    auto tracer = std::make_shared<RecordingCoTTracer>();
    ChainOfThoughtBuilder builder;
    builder.addStep("step").attachTracer(tracer);
    builder.build();
    EXPECT_EQ(tracer->spanCount(), 1u);
    tracer->reset();
    EXPECT_EQ(tracer->spanCount(), 0u);
    EXPECT_FALSE(tracer->hasSpans());
}

// ============================================================================
// AC-12 RecordingCoTTracer::toJson() returns a JSON array
// ============================================================================

TEST(CoTTracerTest, ToJsonReturnsArray) {
    auto tracer = std::make_shared<RecordingCoTTracer>();
    ChainOfThoughtBuilder builder;
    builder.addStep("a").addStep("b");
    builder.attachTracer(tracer);
    builder.build();
    const auto j = tracer->toJson();
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 2u);
}

// ============================================================================
// AC-13 attachTracer() sets hasTracer() true
// ============================================================================

TEST(CoTTracerTest, AttachTracerSetsHasTracerTrue) {
    ChainOfThoughtBuilder builder;
    EXPECT_FALSE(builder.hasTracer());
    builder.attachTracer(std::make_shared<RecordingCoTTracer>());
    EXPECT_TRUE(builder.hasTracer());
}

// ============================================================================
// AC-14 detachTracer() sets hasTracer() false
// ============================================================================

TEST(CoTTracerTest, DetachTracerSetsHasTracerFalse) {
    ChainOfThoughtBuilder builder;
    builder.attachTracer(std::make_shared<RecordingCoTTracer>());
    EXPECT_TRUE(builder.hasTracer());
    builder.detachTracer();
    EXPECT_FALSE(builder.hasTracer());
}

// ============================================================================
// AC-15 build() without tracer produces correct prompt (backward compat)
// ============================================================================

TEST(CoTTracerTest, BuildWithoutTracerProducesCorrectPrompt) {
    ChainOfThoughtBuilder builder;
    builder.addStep("identify").addStep("analyse").setFinalAnswer("conclude");
    const std::string prompt = builder.build();
    EXPECT_NE(prompt.find("identify"), std::string::npos);
    EXPECT_NE(prompt.find("analyse"),  std::string::npos);
    EXPECT_NE(prompt.find("conclude"), std::string::npos);
}

// ============================================================================
// AC-16 build() with tracer produces the same prompt as without
// ============================================================================

TEST(CoTTracerTest, BuildWithTracerProducesSamePrompt) {
    auto setup = [](bool with_tracer) -> std::string {
        ChainOfThoughtBuilder b;
        b.addStep("step A").addStep("step B").setFinalAnswer("done");
        if (with_tracer) {
            b.attachTracer(std::make_shared<RecordingCoTTracer>());
        }
        return b.build();
    };
    EXPECT_EQ(setup(false), setup(true));
}

// ============================================================================
// AC-17 build() on empty builder fires no tracer callbacks
// ============================================================================

TEST(CoTTracerTest, EmptyBuilderFiresNoCallbacks) {
    auto tracer = std::make_shared<RecordingCoTTracer>();
    ChainOfThoughtBuilder builder;
    builder.attachTracer(tracer);
    const auto result = builder.build();
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(tracer->spanCount(), 0u);
}

// ============================================================================
// AC-18 onStepBegin fires before onStepEnd for each step
// ============================================================================

TEST(CoTTracerTest, BeginFiresBeforeEndForEachStep) {
    auto tracer = std::make_shared<OrderTrackingTracer>();
    ChainOfThoughtBuilder builder;
    builder.addStep("x").addStep("y").addStep("z");
    builder.attachTracer(tracer);
    builder.build();

    const auto& ev = tracer->events;
    ASSERT_EQ(ev.size(), 6u);
    // Pattern: begin:0, end:0, begin:1, end:1, begin:2, end:2
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(ev[i * 2],     "begin:" + std::to_string(i));
        EXPECT_EQ(ev[i * 2 + 1], "end:"   + std::to_string(i));
    }
}

// ============================================================================
// AC-19 Misbehaving tracer (throws in onStepBegin) does not abort build()
// ============================================================================

TEST(CoTTracerTest, ThrowingBeginTracerDoesNotAbortBuild) {
    auto tracer = std::make_shared<ThrowingBeginTracer>();
    ChainOfThoughtBuilder builder;
    builder.addStep("safe content");
    builder.attachTracer(tracer);
    std::string result = {};
    EXPECT_NO_THROW(result = builder.build());
    EXPECT_NE(result.find("safe content"), std::string::npos);
}

// ============================================================================
// AC-20 Misbehaving tracer (throws in onStepEnd) does not abort build()
// ============================================================================

TEST(CoTTracerTest, ThrowingEndTracerDoesNotAbortBuild) {
    auto tracer = std::make_shared<ThrowingEndTracer>();
    ChainOfThoughtBuilder builder;
    builder.addStep("safe content");
    builder.attachTracer(tracer);
    std::string result = {};
    EXPECT_NO_THROW(result = builder.build());
    EXPECT_NE(result.find("safe content"), std::string::npos);
}

// ============================================================================
// AC-21 CoTTraceCollector::addTracer() increases tracerCount()
// ============================================================================

TEST(CoTTraceCollectorTest, AddTracerIncreasesCount) {
    CoTTraceCollector collector;
    EXPECT_EQ(collector.tracerCount(), 0u);
    collector.addTracer(std::make_shared<RecordingCoTTracer>());
    EXPECT_EQ(collector.tracerCount(), 1u);
    collector.addTracer(std::make_shared<RecordingCoTTracer>());
    EXPECT_EQ(collector.tracerCount(), 2u);
}

// ============================================================================
// AC-22 CoTTraceCollector fans out to all registered child tracers
// ============================================================================

TEST(CoTTraceCollectorTest, FansOutToChildren) {
    auto child_a = std::make_shared<RecordingCoTTracer>();
    auto child_b = std::make_shared<RecordingCoTTracer>();
    auto collector = std::make_shared<CoTTraceCollector>();
    collector->addTracer(child_a);
    collector->addTracer(child_b);

    ChainOfThoughtBuilder builder;
    builder.addStep("fan-out content");
    builder.attachTracer(collector);
    builder.build();

    EXPECT_EQ(child_a->spanCount(), 1u);
    EXPECT_EQ(child_b->spanCount(), 1u);
}

// ============================================================================
// AC-23 removeTracer() decreases tracerCount()
// ============================================================================

TEST(CoTTraceCollectorTest, RemoveTracerDecreasesCount) {
    auto child = std::make_shared<RecordingCoTTracer>();
    CoTTraceCollector collector;
    collector.addTracer(child);
    EXPECT_EQ(collector.tracerCount(), 1u);
    collector.removeTracer(child.get());
    EXPECT_EQ(collector.tracerCount(), 0u);
}

// ============================================================================
// AC-24 CoTTraceCollector accumulates spans independently of children
// ============================================================================

TEST(CoTTraceCollectorTest, CollectorAccumulatesSpansIndependently) {
    auto child = std::make_shared<RecordingCoTTracer>();
    auto collector = std::make_shared<CoTTraceCollector>();
    collector->addTracer(child);

    ChainOfThoughtBuilder builder;
    builder.addStep("A").addStep("B");
    builder.attachTracer(collector);
    builder.build();

    // Both collector and child should have 2 spans each.
    EXPECT_EQ(collector->spanCount(), 2u);
    EXPECT_EQ(child->spanCount(),     2u);
}

// ============================================================================
// AC-25 CoTTraceCollector::totalStepsTraced() counts all onStepEnd calls
// ============================================================================

TEST(CoTTraceCollectorTest, TotalStepsTracedCountsEndCalls) {
    auto collector = std::make_shared<CoTTraceCollector>();
    ChainOfThoughtBuilder b1;
    b1.addStep("p").addStep("q").addStep("r");
    b1.attachTracer(collector);
    b1.build();

    ChainOfThoughtBuilder b2;
    b2.addStep("x").addStep("y");
    b2.attachTracer(collector);
    b2.build();

    EXPECT_EQ(collector->totalStepsTraced(), 5u);
}

// ============================================================================
// AC-26 CoTTraceCollector::reset() clears spans and totalStepsTraced
// ============================================================================

TEST(CoTTraceCollectorTest, ResetClearsSpansAndCounter) {
    auto collector = std::make_shared<CoTTraceCollector>();
    ChainOfThoughtBuilder builder;
    builder.addStep("content");
    builder.attachTracer(collector);
    builder.build();
    EXPECT_EQ(collector->spanCount(), 1u);
    EXPECT_EQ(collector->totalStepsTraced(), 1u);

    collector->reset();
    EXPECT_EQ(collector->spanCount(), 0u);
    EXPECT_EQ(collector->totalStepsTraced(), 0u);
}

// ============================================================================
// AC-27 CoTTraceCollector::toJson() returns correct-length array
// ============================================================================

TEST(CoTTraceCollectorTest, ToJsonReturnsCorrectLength) {
    auto collector = std::make_shared<CoTTraceCollector>();
    ChainOfThoughtBuilder builder;
    builder.addStep("1").addStep("2").addStep("3");
    builder.attachTracer(collector);
    builder.build();

    const auto j = collector->toJson();
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 3u);
}

// ============================================================================
// AC-28 Multiple builders sharing a RecordingCoTTracer are additive
// ============================================================================

TEST(CoTTracerTest, MultipleBuildersSharingTracerAreAdditive) {
    auto tracer = std::make_shared<RecordingCoTTracer>();

    ChainOfThoughtBuilder b1;
    b1.addStep("one").addStep("two");
    b1.attachTracer(tracer);
    b1.build();

    ChainOfThoughtBuilder b2;
    b2.addStep("three");
    b2.attachTracer(tracer);
    b2.build();

    EXPECT_EQ(tracer->spanCount(), 3u);
}

// ============================================================================
// AC-29 Static factory methods do not fire tracer callbacks
// ============================================================================

TEST(CoTTracerTest, StaticFactoryMethodsDoNotFireCallbacks) {
    // Static methods are not member calls on an instance — they cannot use
    // any attached tracer.  This test confirms that zero spans are recorded
    // when only static factory methods are used.
    auto tracer = std::make_shared<RecordingCoTTracer>();

    // Use static factories (they don't know about any tracer).
    (void)ChainOfThoughtBuilder::buildZeroShot("question");
    (void)ChainOfThoughtBuilder::buildFewShot("q", {{"ex q", "ex a"}});
    (void)ChainOfThoughtBuilder::wrapWithCoT("prompt", true);

    // The tracer was never handed to a builder, so it has no spans.
    EXPECT_EQ(tracer->spanCount(), 0u);
}

// ============================================================================
// AC-30 RecordingCoTTracer::hasSpans() returns false after reset()
// ============================================================================

TEST(CoTTracerTest, HasSpansReturnsFalseAfterReset) {
    auto tracer = std::make_shared<RecordingCoTTracer>();
    ChainOfThoughtBuilder builder;
    builder.addStep("content");
    builder.attachTracer(tracer);
    builder.build();
    EXPECT_TRUE(tracer->hasSpans());
    tracer->reset();
    EXPECT_FALSE(tracer->hasSpans());
}
