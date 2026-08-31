/**
 * @file test_retrieval_llm_reranking_focused.cpp
 * @brief Contract tests for LoRAManifestStore lifecycle APIs.
 */

#include <gtest/gtest.h>

#include "retrieval/include/lora_package.h"

using namespace themis::retrieval;

TEST(RetrievalLlmRerankingContract, StoreAndLoadById) {
    LoRAManifestStore store;
    LoRAPackage pkg;
    pkg.package_id = "pkg-001";
    pkg.status = LoRAPackageStatus::DRAFT;

    ASSERT_TRUE(store.storePackage(pkg));
    auto loaded = store.loadPackage("pkg-001");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->package_id, "pkg-001");
}

TEST(RetrievalLlmRerankingContract, ListByStatus) {
    LoRAManifestStore store;

    LoRAPackage draft;
    draft.package_id = "pkg-draft";
    draft.status = LoRAPackageStatus::DRAFT;
    ASSERT_TRUE(store.storePackage(draft));

    LoRAPackage validated;
    validated.package_id = "pkg-validated";
    validated.status = LoRAPackageStatus::VALIDATED;
    ASSERT_TRUE(store.storePackage(validated));

    const auto drafts = store.listPackagesByStatus(LoRAPackageStatus::DRAFT);
    ASSERT_EQ(drafts.size(), 1u);
    EXPECT_EQ(drafts[0].package_id, "pkg-draft");
}
