// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_ethics_ai_ldm6_clustering_focused.cpp
 * @brief LDM-6 dynamic clustering via CrossSchoolTensionGraph (LDM6-01..LDM6-06).
 *
 * ## Test families
 *
 * ### LDM6-01..02 — CrossSchoolTensionGraph
 *   LDM6-01  tensionBetween() returns 0.0 for unknown pair
 *   LDM6-02  tensionBetween() is symmetric (a↔b == b↔a)
 *
 * ### LDM6-03..04 — ClusterAssignment helpers
 *   LDM6-03  schoolsInCluster() returns correct members
 *   LDM6-04  cluster_count matches number of distinct cluster indices
 *
 * ### LDM6-05..06 — DynamicClusteringEngine
 *   LDM6-05  Empty graph produces empty ClusterAssignment
 *   LDM6-06  High-tension pairs land in different clusters (greedy property)
 *
 * @see include/ethics_ai/ethics_ai_types.h — LDM-6 types
 * @see src/ethics_ai/ethics_ai_types.cpp   — DynamicClusteringEngine::cluster()
 */

#include <gtest/gtest.h>

#include "ethics_ai/ethics_ai_types.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace themis::plugins::ethics;

// ─────────────────────────────────────────────────────────────────────────────
// LDM6-01..02 — CrossSchoolTensionGraph
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LDM6-01: tensionBetween() returns 0.0 for an unknown pair.
 */
TEST(CrossSchoolTensionGraph, UnknownPairReturnsZero) {
    CrossSchoolTensionGraph g;
    g.schools = {"kant", "utilitarianism"};
    // No edges added.
    EXPECT_DOUBLE_EQ(g.tensionBetween("kant", "utilitarianism"), 0.0);
}

/**
 * @test LDM6-02: tensionBetween() is symmetric.
 */
TEST(CrossSchoolTensionGraph, TensionIsSymmetric) {
    CrossSchoolTensionGraph g;
    g.schools = {"kant", "utilitarianism", "virtue"};
    CrossSchoolTensionEdge e;
    e.school_a     = "kant";
    e.school_b     = "utilitarianism";
    e.tension_score = 0.8;
    g.edges.push_back(e);

    EXPECT_DOUBLE_EQ(g.tensionBetween("kant", "utilitarianism"), 0.8);
    EXPECT_DOUBLE_EQ(g.tensionBetween("utilitarianism", "kant"), 0.8);
}

// ─────────────────────────────────────────────────────────────────────────────
// LDM6-03..04 — ClusterAssignment
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LDM6-03: schoolsInCluster() returns correct members.
 */
TEST(ClusterAssignment, SchoolsInClusterReturnsCorrectMembers) {
    ClusterAssignment ca;
    ca.school_to_cluster = {{"kant", 0}, {"utilitarianism", 1}, {"virtue", 0}};
    ca.cluster_count = 2;

    const auto cluster0 = ca.schoolsInCluster(0);
    EXPECT_EQ(cluster0.size(), 2u);
    EXPECT_NE(std::find(cluster0.begin(), cluster0.end(), "kant"), cluster0.end());
    EXPECT_NE(std::find(cluster0.begin(), cluster0.end(), "virtue"), cluster0.end());

    const auto cluster1 = ca.schoolsInCluster(1);
    EXPECT_EQ(cluster1.size(), 1u);
    EXPECT_EQ(cluster1[0], "utilitarianism");
}

/**
 * @test LDM6-04: cluster_count reflects actual distinct indices.
 */
TEST(ClusterAssignment, ClusterCountIsCorrect) {
    ClusterAssignment ca;
    ca.school_to_cluster = {{"a", 0}, {"b", 1}, {"c", 2}};
    ca.cluster_count = 3;
    EXPECT_EQ(ca.cluster_count, 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// LDM6-05..06 — DynamicClusteringEngine
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LDM6-05: Empty graph produces an empty ClusterAssignment.
 */
TEST(DynamicClusteringEngine, EmptyGraphProducesEmptyAssignment) {
    DynamicClusteringEngine engine(2);
    CrossSchoolTensionGraph g;
    const auto result = engine.cluster(g);
    EXPECT_TRUE(result.school_to_cluster.empty());
    EXPECT_EQ(result.cluster_count, 0u);
}

/**
 * @test LDM6-06: High-tension pair lands in different clusters.
 *
 * Builds a 4-school graph where kant↔utilitarianism have maximum tension (1.0)
 * and all other pairs are zero.  With 2 target clusters the greedy algorithm
 * must separate them.
 */
TEST(DynamicClusteringEngine, HighTensionPairSeparatedIntoDifferentClusters) {
    CrossSchoolTensionGraph g;
    g.schools = {"kant", "utilitarianism", "virtue", "deontology"};

    CrossSchoolTensionEdge high;
    high.school_a     = "kant";
    high.school_b     = "utilitarianism";
    high.tension_score = 1.0;
    g.edges.push_back(high);

    DynamicClusteringEngine engine(2);
    const auto result = engine.cluster(g);

    // All schools must be assigned.
    EXPECT_EQ(result.school_to_cluster.size(), g.schools.size());

    // The high-tension pair must be in different clusters.
    const auto it_kant = result.school_to_cluster.find("kant");
    const auto it_util = result.school_to_cluster.find("utilitarianism");
    ASSERT_NE(it_kant, result.school_to_cluster.end());
    ASSERT_NE(it_util, result.school_to_cluster.end());
    EXPECT_NE(it_kant->second, it_util->second)
        << "kant and utilitarianism should be in different clusters due to high tension";
}
