/**
 * @file test_metadata_contract_hardening_focused.cpp
 * @brief Phase 1–6 contract-hardening tests for the metadata module.
 * @note Test IDs: MET-01..MET-08
 */

#include <gtest/gtest.h>
#include "metadata/metadata_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace themis::metadata;

TEST(MetadataContractTest, MET01_ErrorCodesUnique) {
    std::vector<int32_t> codes = {
        static_cast<int32_t>(MetaError::kCollectionNotFound),
        static_cast<int32_t>(MetaError::kFieldNotFound),
        static_cast<int32_t>(MetaError::kSchemaMismatch),
        static_cast<int32_t>(MetaError::kExportFailed),
    };
    std::sort(codes.begin(), codes.end());
    EXPECT_EQ(std::unique(codes.begin(), codes.end()), codes.end());
}

TEST(MetadataContractTest, MET02_ErrorCodesInRange) {
    auto check = [](MetaError e) {
        int32_t v = static_cast<int32_t>(e);
        EXPECT_GE(v, 7900); EXPECT_LE(v, 7999);
    };
    check(MetaError::kCollectionNotFound);
    check(MetaError::kFieldNotFound);
    check(MetaError::kSchemaMismatch);
    check(MetaError::kExportFailed);
}

TEST(MetadataContractTest, MET03_CollectionNotFoundLowest) {
    EXPECT_EQ(static_cast<int32_t>(MetaError::kCollectionNotFound), 7900);
}

TEST(MetadataContractTest, MET04_ExportFailedHighest) {
    EXPECT_EQ(static_cast<int32_t>(MetaError::kExportFailed), 7903);
}

TEST(MetadataContractTest, MET05_CollectionDistinctFromField) {
    EXPECT_NE(static_cast<int32_t>(MetaError::kCollectionNotFound),
              static_cast<int32_t>(MetaError::kFieldNotFound));
}

TEST(MetadataContractTest, MET06_SchemaMismatchDistinctFromExport) {
    EXPECT_NE(static_cast<int32_t>(MetaError::kSchemaMismatch),
              static_cast<int32_t>(MetaError::kExportFailed));
}

TEST(MetadataContractTest, MET07_ErrorSwitchDispatch) {
    MetaError err = MetaError::kSchemaMismatch;
    bool handled = false;
    switch (err) {
        case MetaError::kCollectionNotFound: break;
        case MetaError::kFieldNotFound:      break;
        case MetaError::kSchemaMismatch:     handled = true; break;
        case MetaError::kExportFailed:       break;
    }
    EXPECT_TRUE(handled);
}

TEST(MetadataContractTest, MET08_AllCodesGe7900) {
    for (auto v : {
        static_cast<int32_t>(MetaError::kCollectionNotFound),
        static_cast<int32_t>(MetaError::kFieldNotFound),
        static_cast<int32_t>(MetaError::kSchemaMismatch),
        static_cast<int32_t>(MetaError::kExportFailed),
    }) {
        EXPECT_GE(v, 7900);
    }
}
