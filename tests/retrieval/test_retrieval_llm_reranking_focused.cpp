/**
 * @file test_retrieval_llm_reranking_focused.cpp
 * @brief Group RL — LoRAManifestStore LLM-adapter lifecycle and retrieval path tests.
 *
 * The retrieval module manages LoRA adapters (LoRAPackage / LoRAManifestStore)
 * that are used at inference time. These tests verify the store lifecycle and
 * integrity-check paths relevant to the LLM reranking pipeline.
 */

#include <gtest/gtest.h>
#include "retrieval/include/lora_package.h"

#include <string>
#include <vector>
#include <optional>

using namespace themis::retrieval;

// ── RL1: storePackage with empty package_id returns false ────────────────────
TEST(RetrievalLlmRerankingFocused, RL1_StoreEmptyId_ReturnsFalse) {
    LoRAManifestStore store;
    LoRAPackage pkg;
    pkg.package_id = "";
    EXPECT_FALSE(store.storePackage(pkg));
}

// ── RL2: storePackage with valid id → loadPackage returns that package ─────
TEST(RetrievalLlmRerankingFocused, RL2_StoreAndLoad_Roundtrip) {
    LoRAManifestStore store;
    LoRAPackage pkg;
    pkg.package_id    = "pkg-001";
    pkg.adapter_name  = "mistral-lora-v1";
    pkg.status        = LoRAPackageStatus::DRAFT;

    ASSERT_TRUE(store.storePackage(pkg));

    auto loaded = store.loadPackage("pkg-001");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->adapter_name, "mistral-lora-v1");
    EXPECT_EQ(loaded->status, LoRAPackageStatus::DRAFT);
}

// ── RL3: loadPackage for unknown id returns nullopt ──────────────────────────
TEST(RetrievalLlmRerankingFocused, RL3_LoadUnknownId_ReturnsNullopt) {
    LoRAManifestStore store;
    auto result = store.loadPackage("does-not-exist");
    EXPECT_FALSE(result.has_value());
}

// ── RL4: packageCount starts at 0 ────────────────────────────────────────────
TEST(RetrievalLlmRerankingFocused, RL4_InitialPackageCount_IsZero) {
    LoRAManifestStore store;
    EXPECT_EQ(store.packageCount(), 0u);
}

// ── RL5: packageCount increments after storePackage ──────────────────────────
TEST(RetrievalLlmRerankingFocused, RL5_PackageCount_IncreasesAfterStore) {
    LoRAManifestStore store;
    LoRAPackage pkg;
    pkg.package_id   = "pkg-count-1";
    pkg.adapter_name = "adapter-a";
    ASSERT_TRUE(store.storePackage(pkg));
    EXPECT_EQ(store.packageCount(), 1u);
}

// ── RL6: deletePackage removes stored package ────────────────────────────────
TEST(RetrievalLlmRerankingFocused, RL6_DeletePackage_RemovesFromStore) {
    LoRAManifestStore store;
    LoRAPackage pkg;
    pkg.package_id = "pkg-del";
    ASSERT_TRUE(store.storePackage(pkg));
    ASSERT_TRUE(store.deletePackage("pkg-del"));
    EXPECT_FALSE(store.loadPackage("pkg-del").has_value());
    EXPECT_EQ(store.packageCount(), 0u);
}

// ── RL7: listPackagesByStatus filters correctly ───────────────────────────────
TEST(RetrievalLlmRerankingFocused, RL7_ListByStatus_FiltersCorrectly) {
    LoRAManifestStore store;

    LoRAPackage draft;
    draft.package_id = "pkg-draft";
    draft.status     = LoRAPackageStatus::DRAFT;
    ASSERT_TRUE(store.storePackage(draft));

    LoRAPackage ready;
    ready.package_id = "pkg-ready";
    ready.status     = LoRAPackageStatus::READY;
    ASSERT_TRUE(store.storePackage(ready));

    auto drafts = store.listPackagesByStatus(LoRAPackageStatus::DRAFT);
    ASSERT_EQ(drafts.size(), 1u);
    EXPECT_EQ(drafts[0].package_id, "pkg-draft");

    auto readies = store.listPackagesByStatus(LoRAPackageStatus::READY);
    ASSERT_EQ(readies.size(), 1u);
    EXPECT_EQ(readies[0].package_id, "pkg-ready");
}

// ── RL8: overwrite existing package_id updates the entry ─────────────────────
TEST(RetrievalLlmRerankingFocused, RL8_OverwritePackage_UpdatesEntry) {
    LoRAManifestStore store;
    LoRAPackage v1;
    v1.package_id   = "pkg-overwrite";
    v1.adapter_name = "old-name";
    ASSERT_TRUE(store.storePackage(v1));

    LoRAPackage v2 = v1;
    v2.adapter_name = "new-name";
    ASSERT_TRUE(store.storePackage(v2));
    EXPECT_EQ(store.packageCount(), 1u);  // still 1, not 2

    auto loaded = store.loadPackage("pkg-overwrite");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->adapter_name, "new-name");
}
