#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "content/content_processor.h"

namespace {

using namespace std::chrono_literals;

TEST(TextProcessorCancellation, RequestAwareEmbeddingMatchesLegacyOutput) {
    themis::content::TextProcessor processor;
    const std::string text = "the quick brown fox jumps over the lazy dog";
    const auto legacy = processor.generateEmbedding(text);
    auto stop_reason = themis::query::StopReason::None;

    const auto request_aware = processor.generateEmbedding(
        text, themis::query::RequestContext{}, &stop_reason);

    EXPECT_EQ(stop_reason, themis::query::StopReason::None);
    EXPECT_EQ(request_aware, legacy);
}

TEST(TextProcessorCancellation, DeadlineStopsLargeEmbeddingAndDiscardsPartialVector) {
    themis::content::TextProcessor processor;
    std::string text;
    text.reserve(5000000);
    for (size_t i = 0; i < 1000000; ++i) {
        text += "token ";
    }

    auto stop_reason = themis::query::StopReason::None;
    const auto request = themis::query::RequestContext::withTimeout(20ms);
    const auto started = std::chrono::steady_clock::now();
    const auto embedding = processor.generateEmbedding(text, request, &stop_reason);
    const auto elapsed = std::chrono::steady_clock::now() - started;

    EXPECT_TRUE(embedding.empty());
    EXPECT_EQ(stop_reason, themis::query::StopReason::DeadlineExceeded);
    EXPECT_GE(elapsed, 15ms);
    EXPECT_LT(elapsed, 2s);
}

}  // namespace
