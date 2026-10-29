/**
 * @file test_importers_entity_matching_focused.cpp
 * @brief Focused tests for the production semantic entity-matching helpers.
 */

#include <gtest/gtest.h>

#include "importers/entity_matcher.h"

namespace {

TEST(ImporterEntityMatchingTest, NorthAmericanCountryCodeNormalizesToLocalDigits) {
    using themis::importers::SemanticMatcher;

    EXPECT_EQ(SemanticMatcher::normalizePhoneNumber("+1 (415) 555-2671"),
              "4155552671");
    EXPECT_EQ(SemanticMatcher::normalizePhoneNumber("415.555.2671"),
              "4155552671");
    EXPECT_DOUBLE_EQ(
        SemanticMatcher::scorePhonePair("+1 (415) 555-2671", "415.555.2671"),
        1.0);
}

TEST(ImporterEntityMatchingTest, NonNorthAmericanCountryCodeIsNotStripped) {
    using themis::importers::SemanticMatcher;

    EXPECT_EQ(SemanticMatcher::normalizePhoneNumber("+44 20 7946 0958"),
              "442079460958");
}

} // namespace
