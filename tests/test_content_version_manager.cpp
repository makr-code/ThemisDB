// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file test_content_version_manager.cpp
 * @brief Unit tests for VersionManager – content versioning and delta storage
 *        (Issue #1692 / content module ROADMAP).
 *
 * Tests cover:
 *  1. Metadata-only version creation via createVersion()
 *  2. Content-aware version creation via createVersionWithContent()
 *  3. Content retrieval via getContent()
 *  4. Delta computation and storage
 *  5. Version history, getLatestVersion(), hasVersions()
 *  6. deleteVersion() single and nonexistent version
 *  7. Static helpers: computeHash(), computeDelta()
 *  8. Multiple independent content IDs
 */

#include <gtest/gtest.h>
#include "content/version_manager.h"
#include <string>

using namespace themis::content;

// ── Metadata-only path ───────────────────────────────────────────────────────

TEST(VersionManager, CreateVersionMetadataOnly) {
    VersionManager vm;

    int v = vm.createVersion("doc1", "abc123", 42, "alice", "initial");
    EXPECT_EQ(v, 1);
    EXPECT_TRUE(vm.hasVersions("doc1"));

    auto info = vm.getVersion("doc1", 1);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->version_number, 1);
    EXPECT_EQ(info->content_hash, "abc123");
    EXPECT_EQ(info->size_bytes, 42u);
    EXPECT_EQ(info->author, "alice");
    EXPECT_EQ(info->comment, "initial");
    EXPECT_GT(info->timestamp, 0);
    EXPECT_TRUE(info->content.empty());
    EXPECT_TRUE(info->delta.empty());
}

TEST(VersionManager, MetadataOnlyGetContentReturnsNullopt) {
    VersionManager vm;
    vm.createVersion("doc1", "abc", 10);

    auto content = vm.getContent("doc1", 1);
    EXPECT_FALSE(content.has_value());
}

// ── Content-aware path ────────────────────────────────────────────────────────

TEST(VersionManager, CreateVersionWithContentStoresContent) {
    VersionManager vm;

    int v = vm.createVersionWithContent("doc1", "Hello World", "bob", "first version");
    EXPECT_EQ(v, 1);

    auto info = vm.getVersion("doc1", 1);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->content, "Hello World");
    EXPECT_EQ(info->size_bytes, 11u);
    EXPECT_FALSE(info->content_hash.empty());
    EXPECT_EQ(info->author, "bob");
    EXPECT_EQ(info->comment, "first version");
}

TEST(VersionManager, GetContentReturnsCorrectContent) {
    VersionManager vm;
    vm.createVersionWithContent("doc1", "Version one");
    vm.createVersionWithContent("doc1", "Version two");

    auto c1 = vm.getContent("doc1", 1);
    auto c2 = vm.getContent("doc1", 2);

    ASSERT_TRUE(c1.has_value());
    ASSERT_TRUE(c2.has_value());
    EXPECT_EQ(*c1, "Version one");
    EXPECT_EQ(*c2, "Version two");
}

TEST(VersionManager, GetContentNonexistentVersionReturnsNullopt) {
    VersionManager vm;
    EXPECT_FALSE(vm.getContent("doc1", 1).has_value());

    vm.createVersionWithContent("doc1", "some content");
    EXPECT_FALSE(vm.getContent("doc1", 99).has_value());
}

// ── Delta storage ─────────────────────────────────────────────────────────────

TEST(VersionManager, FirstVersionHasEmptyDelta) {
    VersionManager vm;
    vm.createVersionWithContent("doc1", "line1\nline2\n");

    auto v1 = vm.getVersion("doc1", 1);
    ASSERT_TRUE(v1.has_value());
    EXPECT_TRUE(v1->delta.empty());
}

TEST(VersionManager, SecondVersionHasNonEmptyDelta) {
    VersionManager vm;
    vm.createVersionWithContent("doc1", "line1\nline2\n");
    vm.createVersionWithContent("doc1", "line1\nline3\n");

    auto v2 = vm.getVersion("doc1", 2);
    ASSERT_TRUE(v2.has_value());
    EXPECT_FALSE(v2->delta.empty());
}

TEST(VersionManager, IdenticalContentHasEmptyDelta) {
    VersionManager vm;
    vm.createVersionWithContent("doc1", "same content");
    vm.createVersionWithContent("doc1", "same content");

    auto v2 = vm.getVersion("doc1", 2);
    ASSERT_TRUE(v2.has_value());
    EXPECT_TRUE(v2->delta.empty());
}

// ── Hash computation ──────────────────────────────────────────────────────────

TEST(VersionManager, ComputeHashIsDeterministic) {
    std::string h1 = VersionManager::computeHash("hello");
    std::string h2 = VersionManager::computeHash("hello");
    EXPECT_EQ(h1, h2);
    EXPECT_EQ(h1.size(), 64u);  // 32 bytes × 2 hex chars
}

TEST(VersionManager, ComputeHashDiffersForDifferentInput) {
    EXPECT_NE(VersionManager::computeHash("hello"),
              VersionManager::computeHash("world"));
}

TEST(VersionManager, ComputeHashKnownValue) {
    // SHA-256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    std::string empty_hash = VersionManager::computeHash("");
    EXPECT_EQ(empty_hash,
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(VersionManager, HashStoredInVersionMatchesComputedHash) {
    VersionManager vm;
    const std::string content = "test content for hashing";
    vm.createVersionWithContent("doc1", content);

    auto v = vm.getVersion("doc1", 1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->content_hash, VersionManager::computeHash(content));
}

// ── Delta helper ──────────────────────────────────────────────────────────────

TEST(VersionManager, ComputeDeltaEmptyToEmpty) {
    EXPECT_EQ(VersionManager::computeDelta("", ""), "");
}

TEST(VersionManager, ComputeDeltaAddedLines) {
    std::string delta = VersionManager::computeDelta("", "new line\n");
    EXPECT_NE(delta.find('+'), std::string::npos);
}

TEST(VersionManager, ComputeDeltaRemovedLines) {
    std::string delta = VersionManager::computeDelta("old line\n", "");
    EXPECT_NE(delta.find('-'), std::string::npos);
}

TEST(VersionManager, ComputeDeltaUnchangedLinesNotInDelta) {
    std::string delta = VersionManager::computeDelta("same\n", "same\n");
    EXPECT_TRUE(delta.empty());
}

// ── Version history ───────────────────────────────────────────────────────────

TEST(VersionManager, GetVersionHistoryEmpty) {
    VersionManager vm;
    EXPECT_TRUE(vm.getVersionHistory("no_such_id").empty());
}

TEST(VersionManager, GetVersionHistoryMultipleVersions) {
    VersionManager vm;
    vm.createVersionWithContent("doc1", "v1");
    vm.createVersionWithContent("doc1", "v2");
    vm.createVersionWithContent("doc1", "v3");

    auto history = vm.getVersionHistory("doc1");
    ASSERT_EQ(history.size(), 3u);
    EXPECT_EQ(history[0].version_number, 1);
    EXPECT_EQ(history[1].version_number, 2);
    EXPECT_EQ(history[2].version_number, 3);
}

TEST(VersionManager, GetLatestVersion) {
    VersionManager vm;
    EXPECT_EQ(vm.getLatestVersion("doc1"), 0);

    vm.createVersionWithContent("doc1", "a");
    EXPECT_EQ(vm.getLatestVersion("doc1"), 1);

    vm.createVersionWithContent("doc1", "b");
    EXPECT_EQ(vm.getLatestVersion("doc1"), 2);
}

TEST(VersionManager, HasVersions) {
    VersionManager vm;
    EXPECT_FALSE(vm.hasVersions("doc1"));

    vm.createVersionWithContent("doc1", "content");
    EXPECT_TRUE(vm.hasVersions("doc1"));
}

// ── deleteVersion ─────────────────────────────────────────────────────────────

TEST(VersionManager, DeleteVersionRemovesIt) {
    VersionManager vm;
    vm.createVersionWithContent("doc1", "v1");
    vm.createVersionWithContent("doc1", "v2");

    EXPECT_TRUE(vm.deleteVersion("doc1", 1));
    EXPECT_FALSE(vm.getVersion("doc1", 1).has_value());
    EXPECT_EQ(vm.getVersionHistory("doc1").size(), 1u);
}

TEST(VersionManager, DeleteNonexistentVersionReturnsFalse) {
    VersionManager vm;
    EXPECT_FALSE(vm.deleteVersion("doc1", 99));

    vm.createVersionWithContent("doc1", "content");
    EXPECT_FALSE(vm.deleteVersion("doc1", 99));
}

TEST(VersionManager, DeleteVersionFromNonexistentDocReturnsFalse) {
    VersionManager vm;
    EXPECT_FALSE(vm.deleteVersion("no_doc", 1));
}

// ── Multiple content IDs ──────────────────────────────────────────────────────

TEST(VersionManager, IndependentContentIds) {
    VersionManager vm;
    vm.createVersionWithContent("doc_a", "content A v1");
    vm.createVersionWithContent("doc_b", "content B v1");
    vm.createVersionWithContent("doc_a", "content A v2");

    EXPECT_EQ(vm.getLatestVersion("doc_a"), 2);
    EXPECT_EQ(vm.getLatestVersion("doc_b"), 1);

    auto a2 = vm.getContent("doc_a", 2);
    ASSERT_TRUE(a2.has_value());
    EXPECT_EQ(*a2, "content A v2");

    auto b1 = vm.getContent("doc_b", 1);
    ASSERT_TRUE(b1.has_value());
    EXPECT_EQ(*b1, "content B v1");
}

TEST(VersionManager, VersionNumbersArePerDocumentId) {
    VersionManager vm;
    vm.createVersionWithContent("doc_a", "a1");
    vm.createVersionWithContent("doc_a", "a2");
    vm.createVersionWithContent("doc_b", "b1");

    // doc_b's first version should be v1, not v3
    EXPECT_EQ(vm.getLatestVersion("doc_b"), 1);
    auto b1 = vm.getVersion("doc_b", 1);
    ASSERT_TRUE(b1.has_value());
    EXPECT_EQ(b1->version_number, 1);
}
