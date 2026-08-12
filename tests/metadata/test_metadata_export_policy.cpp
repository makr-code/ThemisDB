/**
 * Test: Metadata Export Policy
 *
 * Tests for IMetadataExportPolicy / AlwaysExportPolicy / NeverExportPolicy /
 * FilteredExportPolicy:
 *
 * Acceptance criteria:
 *   AC-EP-1  AlwaysExportPolicy::shouldExport returns true for every trigger
 *   AC-EP-2  AlwaysExportPolicy::exportDelay returns 0 ms
 *   AC-EP-3  NeverExportPolicy::shouldExport returns false for every trigger
 *   AC-EP-4  FilteredExportPolicy::shouldExport allows non-excluded tables
 *   AC-EP-5  FilteredExportPolicy::shouldExport denies excluded tables
 *   AC-EP-6  removeExclusion restores shouldExport to true
 *   AC-EP-7  FilteredExportPolicy::exportDelay returns the configured delay
 *   AC-EP-8  addExclusion/shouldExport are thread-safe under concurrent access
 *   AC-EP-9  Polymorphic usage via IMetadataExportPolicy*
 *   AC-EP-10 AlwaysExportPolicy accepts all MetadataExportTrigger variants
 *   AC-EP-11 NeverExportPolicy accepts all MetadataExportTrigger variants
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "metadata/imetadata_export_policy.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace themis::metadata;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// AC-EP-1/2 — AlwaysExportPolicy
// ─────────────────────────────────────────────────────────────────────────────

TEST(AlwaysExportPolicyTest, ShouldExportAlwaysTrue) {
    AlwaysExportPolicy policy;
    EXPECT_TRUE(policy.shouldExport("orders",  MetadataExportTrigger::SCHEMA_CREATED));
    EXPECT_TRUE(policy.shouldExport("users",   MetadataExportTrigger::SCHEMA_MODIFIED));
    EXPECT_TRUE(policy.shouldExport("archive", MetadataExportTrigger::SCHEMA_DROPPED));
    EXPECT_TRUE(policy.shouldExport("stats",   MetadataExportTrigger::STATISTICS_UPDATED));
}

TEST(AlwaysExportPolicyTest, ExportDelayIsZero) {
    AlwaysExportPolicy policy;
    EXPECT_EQ(policy.exportDelay("orders", MetadataExportTrigger::SCHEMA_CREATED),
              std::chrono::milliseconds{0});
    EXPECT_EQ(policy.exportDelay("users",  MetadataExportTrigger::SCHEMA_MODIFIED),
              std::chrono::milliseconds{0});
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-EP-3 — NeverExportPolicy
// ─────────────────────────────────────────────────────────────────────────────

TEST(NeverExportPolicyTest, ShouldExportAlwaysFalse) {
    NeverExportPolicy policy;
    EXPECT_FALSE(policy.shouldExport("orders",  MetadataExportTrigger::SCHEMA_CREATED));
    EXPECT_FALSE(policy.shouldExport("users",   MetadataExportTrigger::SCHEMA_MODIFIED));
    EXPECT_FALSE(policy.shouldExport("archive", MetadataExportTrigger::SCHEMA_DROPPED));
    EXPECT_FALSE(policy.shouldExport("stats",   MetadataExportTrigger::STATISTICS_UPDATED));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-EP-4/5 — FilteredExportPolicy: allow / exclude
// ─────────────────────────────────────────────────────────────────────────────

TEST(FilteredExportPolicyTest, NonExcludedTableIsExported) {
    FilteredExportPolicy policy;
    policy.addExclusion("_internal");
    EXPECT_TRUE(policy.shouldExport("orders", MetadataExportTrigger::SCHEMA_CREATED));
}

TEST(FilteredExportPolicyTest, ExcludedTableIsBlocked) {
    FilteredExportPolicy policy;
    policy.addExclusion("_internal");
    EXPECT_FALSE(policy.shouldExport("_internal", MetadataExportTrigger::SCHEMA_CREATED));
}

TEST(FilteredExportPolicyTest, ExclusionIsExactMatchOnly) {
    FilteredExportPolicy policy;
    policy.addExclusion("_internal");
    // Partial matches must still be allowed
    EXPECT_TRUE(policy.shouldExport("_internal_stats", MetadataExportTrigger::SCHEMA_MODIFIED));
    EXPECT_TRUE(policy.shouldExport("internal",        MetadataExportTrigger::SCHEMA_MODIFIED));
    EXPECT_TRUE(policy.shouldExport("_INTERNAL",       MetadataExportTrigger::SCHEMA_MODIFIED));
}

TEST(FilteredExportPolicyTest, MultipleExclusions) {
    FilteredExportPolicy policy;
    policy.addExclusion("tmp_a");
    policy.addExclusion("tmp_b");
    EXPECT_FALSE(policy.shouldExport("tmp_a",  MetadataExportTrigger::SCHEMA_CREATED));
    EXPECT_FALSE(policy.shouldExport("tmp_b",  MetadataExportTrigger::SCHEMA_CREATED));
    EXPECT_TRUE(policy.shouldExport("tmp_c",   MetadataExportTrigger::SCHEMA_CREATED));
    EXPECT_TRUE(policy.shouldExport("orders",  MetadataExportTrigger::SCHEMA_CREATED));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-EP-6 — removeExclusion restores export
// ─────────────────────────────────────────────────────────────────────────────

TEST(FilteredExportPolicyTest, RemoveExclusionRestoresExport) {
    FilteredExportPolicy policy;
    policy.addExclusion("orders");
    EXPECT_FALSE(policy.shouldExport("orders", MetadataExportTrigger::SCHEMA_MODIFIED));

    policy.removeExclusion("orders");
    EXPECT_TRUE(policy.shouldExport("orders", MetadataExportTrigger::SCHEMA_MODIFIED));
}

TEST(FilteredExportPolicyTest, RemoveNonExistentExclusionIsNoOp) {
    FilteredExportPolicy policy;
    EXPECT_NO_THROW(policy.removeExclusion("nonexistent"));
    EXPECT_TRUE(policy.shouldExport("orders", MetadataExportTrigger::SCHEMA_CREATED));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-EP-7 — exportDelay
// ─────────────────────────────────────────────────────────────────────────────

TEST(FilteredExportPolicyTest, DefaultDelayIsZero) {
    FilteredExportPolicy policy;
    EXPECT_EQ(policy.exportDelay("orders", MetadataExportTrigger::SCHEMA_CREATED),
              std::chrono::milliseconds{0});
}

TEST(FilteredExportPolicyTest, ConfiguredDelayIsReturned) {
    FilteredExportPolicy policy{500ms};
    EXPECT_EQ(policy.exportDelay("orders", MetadataExportTrigger::SCHEMA_MODIFIED),
              std::chrono::milliseconds{500});
    EXPECT_EQ(policy.exportDelay("any_table", MetadataExportTrigger::STATISTICS_UPDATED),
              std::chrono::milliseconds{500});
}

TEST(FilteredExportPolicyTest, DelayAppliesUniformlyToAllTables) {
    FilteredExportPolicy policy{250ms};
    policy.addExclusion("excluded");
    // Even for the excluded table, exportDelay is independent of shouldExport
    EXPECT_EQ(policy.exportDelay("excluded", MetadataExportTrigger::SCHEMA_DROPPED),
              std::chrono::milliseconds{250});
    EXPECT_EQ(policy.exportDelay("allowed",  MetadataExportTrigger::SCHEMA_DROPPED),
              std::chrono::milliseconds{250});
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-EP-8 — Thread-safety
// ─────────────────────────────────────────────────────────────────────────────

TEST(FilteredExportPolicyTest, ConcurrentAddAndCheck) {
    FilteredExportPolicy policy;
    constexpr int kIter    = 200;
    constexpr int kThreads = 4;

    std::atomic<int> exported{0};
    std::atomic<int> blocked{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads + 1);

    // Writer: continuously add and remove exclusion
    threads.emplace_back([&] {
        for (int i = 0; i < kIter; ++i) {
            policy.addExclusion("shared_table");
            policy.removeExclusion("shared_table");
        }
    });

    // Readers: check shouldExport concurrently
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kIter; ++i) {
                if (policy.shouldExport("shared_table", MetadataExportTrigger::SCHEMA_MODIFIED)) {
                    ++exported;
                } else {
                    ++blocked;
                }
            }
        });
    }

    for (auto& th : threads) th.join();

    // No crash; exported + blocked == kThreads * kIter
    EXPECT_EQ(exported.load() + blocked.load(), kThreads * kIter);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-EP-9 — Polymorphic usage
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataExportPolicyPolymorphismTest, AlwaysViaInterface) {
    std::unique_ptr<IMetadataExportPolicy> policy =
        std::make_unique<AlwaysExportPolicy>();
    EXPECT_TRUE(policy->shouldExport("orders", MetadataExportTrigger::SCHEMA_CREATED));
    EXPECT_EQ(policy->exportDelay("orders", MetadataExportTrigger::SCHEMA_CREATED),
              std::chrono::milliseconds{0});
}

TEST(MetadataExportPolicyPolymorphismTest, NeverViaInterface) {
    std::unique_ptr<IMetadataExportPolicy> policy =
        std::make_unique<NeverExportPolicy>();
    EXPECT_FALSE(policy->shouldExport("orders", MetadataExportTrigger::SCHEMA_CREATED));
}

TEST(MetadataExportPolicyPolymorphismTest, FilteredViaInterface) {
    auto filtered = std::make_unique<FilteredExportPolicy>(100ms);
    filtered->addExclusion("_tmp");

    std::unique_ptr<IMetadataExportPolicy> policy = std::move(filtered);
    EXPECT_TRUE(policy->shouldExport("orders", MetadataExportTrigger::SCHEMA_MODIFIED));
    EXPECT_FALSE(policy->shouldExport("_tmp",  MetadataExportTrigger::SCHEMA_MODIFIED));
    EXPECT_EQ(policy->exportDelay("orders", MetadataExportTrigger::SCHEMA_MODIFIED),
              std::chrono::milliseconds{100});
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-EP-10/11 — All trigger variants accepted by Always/Never
// ─────────────────────────────────────────────────────────────────────────────

TEST(AlwaysExportPolicyTest, AllTriggersAccepted) {
    AlwaysExportPolicy policy;
    const MetadataExportTrigger triggers[] = {
        MetadataExportTrigger::SCHEMA_CREATED,
        MetadataExportTrigger::SCHEMA_MODIFIED,
        MetadataExportTrigger::SCHEMA_DROPPED,
        MetadataExportTrigger::STATISTICS_UPDATED,
    };
    for (auto t : triggers) {
        EXPECT_TRUE(policy.shouldExport("tbl", t));
    }
}

TEST(NeverExportPolicyTest, AllTriggersBlocked) {
    NeverExportPolicy policy;
    const MetadataExportTrigger triggers[] = {
        MetadataExportTrigger::SCHEMA_CREATED,
        MetadataExportTrigger::SCHEMA_MODIFIED,
        MetadataExportTrigger::SCHEMA_DROPPED,
        MetadataExportTrigger::STATISTICS_UPDATED,
    };
    for (auto t : triggers) {
        EXPECT_FALSE(policy.shouldExport("tbl", t));
    }
}
