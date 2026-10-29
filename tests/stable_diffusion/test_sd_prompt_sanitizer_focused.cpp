#include <gtest/gtest.h>

#include "stable_diffusion/sd_prompt_sanitizer.h"

namespace themis::imggen {
namespace {

TEST(SDPromptSanitizer, RemovesEveryCaseInsensitiveOccurrence) {
    const SDPromptSanitizer sanitizer({"blocked"});

    EXPECT_EQ(sanitizer.sanitize("A BLOCKED word and blocked suffix"),
              "A  word and  suffix");
}

TEST(SDPromptSanitizer, EmptyBlockedKeywordListPreservesPrompt) {
    const SDPromptSanitizer sanitizer({});
    const std::string prompt = "A prompt with punctuation: {}, [] and UTF-8 café.";

    EXPECT_TRUE(sanitizer.isAllowed(prompt));
    EXPECT_EQ(sanitizer.sanitize(prompt), prompt);
}

} // namespace
} // namespace themis::imggen
