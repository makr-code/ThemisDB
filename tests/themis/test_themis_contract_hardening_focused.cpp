/**
 * @file test_themis_contract_hardening_focused.cpp
 * @brief Phase 1–6 contract-hardening tests for the themis (engine) module.
 * @note Test IDs: THE-01..THE-08
 */

#include <gtest/gtest.h>
#include "themis/themis_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace themis::engine;

TEST(ThemisContractTest, THE01_ErrorCodesUnique) {
    std::vector<int32_t> codes = {
        static_cast<int32_t>(ThemisError::kEditionMismatch),
        static_cast<int32_t>(ThemisError::kFeatureUnknown),
    };
    EXPECT_NE(codes[0], codes[1]);
}

TEST(ThemisContractTest, THE02_ErrorCodesInRange) {
    EXPECT_GE(static_cast<int32_t>(ThemisError::kEditionMismatch), 7800);
    EXPECT_LE(static_cast<int32_t>(ThemisError::kFeatureUnknown),  7899);
}

TEST(ThemisContractTest, THE03_EditionValuesAreDistinct) {
    std::vector<int32_t> editions = {
        static_cast<int32_t>(Edition::kMinimal),
        static_cast<int32_t>(Edition::kCommunity),
        static_cast<int32_t>(Edition::kEnterprise),
        static_cast<int32_t>(Edition::kHyperscaler),
        static_cast<int32_t>(Edition::kMilitary),
    };
    std::sort(editions.begin(), editions.end());
    EXPECT_EQ(std::unique(editions.begin(), editions.end()), editions.end());
}

TEST(ThemisContractTest, THE04_MinimalEditionLowest) {
    EXPECT_EQ(static_cast<int32_t>(Edition::kMinimal), 1);
}

TEST(ThemisContractTest, THE05_MilitaryEditionHighest) {
    int32_t mil = static_cast<int32_t>(Edition::kMilitary);
    EXPECT_GT(mil, static_cast<int32_t>(Edition::kHyperscaler));
}

TEST(ThemisContractTest, THE06_EditionSwitchDispatch) {
    Edition ed = Edition::kEnterprise;
    std::string name;
    switch (ed) {
        case Edition::kMinimal:     name = "minimal";     break;
        case Edition::kCommunity:   name = "community";   break;
        case Edition::kEnterprise:  name = "enterprise";  break;
        case Edition::kHyperscaler: name = "hyperscaler"; break;
        case Edition::kMilitary:    name = "military";    break;
    }
    EXPECT_EQ(name, "enterprise");
}

TEST(ThemisContractTest, THE07_EditionMismatchIsFirstCode) {
    EXPECT_EQ(static_cast<int32_t>(ThemisError::kEditionMismatch), 7800);
}

TEST(ThemisContractTest, THE08_FeatureUnknownIsSecondCode) {
    EXPECT_EQ(static_cast<int32_t>(ThemisError::kFeatureUnknown), 7801);
}
