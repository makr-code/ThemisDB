/**
 * @file test_projects_contract_hardening_focused.cpp
 * @brief Phase 1–6 contract-hardening tests for the projects module.
 * @note Test IDs: PRJ-01..PRJ-08
 */

#include <gtest/gtest.h>
#include "projects/project_diff.h"
#include "projects/projects_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace themis::projects;

TEST(ProjectsContractTest, PRJ01_ErrorCodesUnique) {
    std::vector<int32_t> codes = {
        static_cast<int32_t>(ProjError::kMemberNotFound),
        static_cast<int32_t>(ProjError::kProjectNotFound),
        static_cast<int32_t>(ProjError::kQuotaExceeded),
        static_cast<int32_t>(ProjError::kAuditOverflow),
    };
    std::sort(codes.begin(), codes.end());
    EXPECT_EQ(std::unique(codes.begin(), codes.end()), codes.end());
}

TEST(ProjectsContractTest, PRJ02_ErrorCodesInRange) {
    auto check = [](ProjError e) {
        int32_t v = static_cast<int32_t>(e);
        EXPECT_GE(v, 7700); EXPECT_LE(v, 7799);
    };
    check(ProjError::kMemberNotFound);
    check(ProjError::kProjectNotFound);
    check(ProjError::kQuotaExceeded);
    check(ProjError::kAuditOverflow);
}

TEST(ProjectsContractTest, PRJ03_MemberNotFoundDistinctFromProject) {
    EXPECT_NE(static_cast<int32_t>(ProjError::kMemberNotFound),
              static_cast<int32_t>(ProjError::kProjectNotFound));
}

TEST(ProjectsContractTest, PRJ04_QuotaDistinctFromAuditOverflow) {
    EXPECT_NE(static_cast<int32_t>(ProjError::kQuotaExceeded),
              static_cast<int32_t>(ProjError::kAuditOverflow));
}

TEST(ProjectsContractTest, PRJ05_MemberNotFoundLowestCode) {
    EXPECT_EQ(static_cast<int32_t>(ProjError::kMemberNotFound), 7700);
}

TEST(ProjectsContractTest, PRJ06_AuditOverflowHighestCode) {
    EXPECT_EQ(static_cast<int32_t>(ProjError::kAuditOverflow), 7703);
}

TEST(ProjectsContractTest, PRJ07_ErrorSwitchDispatch) {
    ProjError err = ProjError::kQuotaExceeded;
    bool handled = false;
    switch (err) {
        case ProjError::kMemberNotFound:  break;
        case ProjError::kProjectNotFound: break;
        case ProjError::kQuotaExceeded:   handled = true; break;
        case ProjError::kAuditOverflow:   break;
    }
    EXPECT_TRUE(handled);
}

TEST(ProjectsContractTest, PRJ08_AllCodesGe7700) {
    for (auto v : {
        static_cast<int32_t>(ProjError::kMemberNotFound),
        static_cast<int32_t>(ProjError::kProjectNotFound),
        static_cast<int32_t>(ProjError::kQuotaExceeded),
        static_cast<int32_t>(ProjError::kAuditOverflow),
    }) {
        EXPECT_GE(v, 7700);
    }
}

TEST(ProjectsContractTest, PRJ09_NestedDocumentChangesReportFullFieldPaths) {
    const themis::projects::ProjectDiff diff(nullptr);
    const nlohmann::json before = {
        {"metadata", {{"owner", {{"name", "Ada"}}}, {"retired", true}}}
    };
    const nlohmann::json after = {
        {"metadata", {{"owner", {{"name", "Grace"}}}, {"active", true}}}
    };

    const auto changes = diff.diffDocuments(before, after);

    ASSERT_EQ(changes.totalChanges(), 3u);
    const auto has_change = [&changes](const std::string& path,
                                       themis::projects::DeltaType type) {
        return std::any_of(changes.entries.begin(), changes.entries.end(),
                           [&path, type](const auto& entry) {
                               return entry.field_path == path && entry.type == type;
                           });
    };
    EXPECT_TRUE(has_change("/metadata/owner/name",
                           themis::projects::DeltaType::MODIFIED));
    EXPECT_TRUE(has_change("/metadata/retired",
                           themis::projects::DeltaType::REMOVED));
    EXPECT_TRUE(has_change("/metadata/active",
                           themis::projects::DeltaType::ADDED));
}
