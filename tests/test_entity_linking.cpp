// test_entity_linking.cpp
//
// Unit tests for EntityLinker:
//   - createLink (happy path, dry-run, invalid IDs)
//   - linkBatch
//   - getLinksForEntity (bidirectional, collection filter)
//   - exportLinkGraph
//   - linkCount / clear

#include <gtest/gtest.h>
#include "importers/entity_linker.h"

namespace ti = themis::importers;

// Helper to create a minimal valid EntityLink.
static ti::EntityLink makeLink(
    const std::string& src,
    const std::string& tgt,
    double             confidence = 0.95,
    ti::LinkType       lt = ti::LinkType::SAME_AS,
    ti::ResolutionStatus rs = ti::ResolutionStatus::RESOLVED
) {
    ti::EntityLink link;
    link.source_id  = src;
    link.target_id  = tgt;
    link.link_type  = lt;
    link.status     = rs;
    link.confidence = confidence;
    link.created_at = "2026-03-11T12:00:00Z";
    link.created_by = "test_suite";
    link.matching_evidence = {{"field", "email"}, {"value", "alice@test.com"}};
    link.matched_fields = {"email"};
    return link;
}

// Helper to make default ImportOptions with dry_run = false.
static ti::ImportOptions defaultOptions() {
    ti::ImportOptions opts;
    opts.dry_run = false;
    return opts;
}

// ============================================================================
// createLink tests
// ============================================================================

class EntityLinkerTest : public ::testing::Test {
protected:
    ti::EntityLinker linker;
};

TEST_F(EntityLinkerTest, CreateLinkSucceeds) {
    auto link = makeLink("src-1", "tgt-1");
    EXPECT_TRUE(linker.createLink(link, defaultOptions()));
    EXPECT_EQ(linker.linkCount(), 1u);
}

TEST_F(EntityLinkerTest, CreateLinkEmptySourceIdFails) {
    auto link = makeLink("", "tgt-1");
    EXPECT_FALSE(linker.createLink(link, defaultOptions()));
    EXPECT_EQ(linker.linkCount(), 0u);
}

TEST_F(EntityLinkerTest, CreateLinkEmptyTargetIdFails) {
    auto link = makeLink("src-1", "");
    EXPECT_FALSE(linker.createLink(link, defaultOptions()));
}

TEST_F(EntityLinkerTest, CreateLinkDryRunDoesNotStore) {
    ti::ImportOptions opts;
    opts.dry_run = true;
    auto link = makeLink("src-1", "tgt-1");
    EXPECT_TRUE(linker.createLink(link, opts));  // Returns true (success)
    EXPECT_EQ(linker.linkCount(), 0u);            // But nothing stored
}

TEST_F(EntityLinkerTest, MultipleLinksSameSource) {
    linker.createLink(makeLink("src-1", "tgt-1"), defaultOptions());
    linker.createLink(makeLink("src-1", "tgt-2"), defaultOptions());
    EXPECT_EQ(linker.linkCount(), 2u);
}

// ============================================================================
// linkBatch tests
// ============================================================================

TEST_F(EntityLinkerTest, LinkBatchImportsAll) {
    std::vector<ti::EntityLink> links = {};

    for (int i = 0; i < 5; ++i) {
        links.push_back(makeLink("src-" + std::to_string(i), "tgt-" + std::to_string(i)));
    }
    auto stats = linker.linkBatch("users", links, defaultOptions());
    EXPECT_EQ(stats.total_records, 5u);
    EXPECT_EQ(stats.imported_records, 5u);
    EXPECT_EQ(stats.failed_records, 0u);
    EXPECT_EQ(linker.linkCount(), 5u);
}

TEST_F(EntityLinkerTest, LinkBatchSkipsInvalidLinks) {
    std::vector<ti::EntityLink> links = {
        makeLink("src-1", "tgt-1"),
        makeLink("", "tgt-2"),     // Invalid: empty source
        makeLink("src-3", "tgt-3")
    };
    auto stats = linker.linkBatch("users", links, defaultOptions());
    EXPECT_EQ(stats.imported_records, 2u);
    EXPECT_EQ(stats.failed_records, 1u);
}

TEST_F(EntityLinkerTest, LinkBatchDryRunCountsButDoesNotStore) {
    ti::ImportOptions dry_opts;
    dry_opts.dry_run = true;
    std::vector<ti::EntityLink> links = {makeLink("s1", "t1"), makeLink("s2", "t2")};
    auto stats = linker.linkBatch("users", links, dry_opts);
    EXPECT_EQ(stats.imported_records, 2u);
    EXPECT_EQ(linker.linkCount(), 0u);
}

// ============================================================================
// getLinksForEntity tests
// ============================================================================

TEST_F(EntityLinkerTest, GetLinksForSourceEntity) {
    linker.createLink(makeLink("alice", "alice-v2"), defaultOptions());
    linker.createLink(makeLink("bob",   "bob-v2"),   defaultOptions());

    auto entries = linker.getLinksForEntity("alice", "");
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].link.source_id, "alice");
}

TEST_F(EntityLinkerTest, GetLinksForTargetEntity) {
    linker.createLink(makeLink("src-X", "tgt-Y"), defaultOptions());
    auto entries = linker.getLinksForEntity("tgt-Y", "");
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].link.target_id, "tgt-Y");
}

TEST_F(EntityLinkerTest, GetLinksFilterByCollection) {
    auto l1 = makeLink("src-1", "tgt-1");
    l1.metadata["collection"] = "users";
    auto l2 = makeLink("src-1", "tgt-2");
    l2.metadata["collection"] = "products";

    linker.createLink(l1, defaultOptions());
    linker.createLink(l2, defaultOptions());

    auto users_entries   = linker.getLinksForEntity("src-1", "users");
    auto product_entries = linker.getLinksForEntity("src-1", "products");

    EXPECT_EQ(users_entries.size(), 1u);
    EXPECT_EQ(product_entries.size(), 1u);
}

TEST_F(EntityLinkerTest, GetLinksUnknownEntityReturnsEmpty) {
    linker.createLink(makeLink("src-1", "tgt-1"), defaultOptions());
    auto entries = linker.getLinksForEntity("unknown-entity", "");
    EXPECT_TRUE(entries.empty());
}

TEST_F(EntityLinkerTest, AuditEntryEventTypeIsCreated) {
    linker.createLink(makeLink("s1", "t1"), defaultOptions());
    auto entries = linker.getLinksForEntity("s1", "");
    ASSERT_FALSE(entries.empty());
    EXPECT_EQ(entries[0].event_type, "created");
}

// ============================================================================
// exportLinkGraph tests
// ============================================================================

TEST_F(EntityLinkerTest, ExportLinkGraphContainsNodesAndEdges) {
    linker.createLink(makeLink("src-A", "tgt-B"), defaultOptions());
    auto graph = linker.exportLinkGraph("", {}, true);
    ASSERT_TRUE(graph.contains("nodes"));
    ASSERT_TRUE(graph.contains("edges"));
    EXPECT_GE(graph["edges"].size(), 1u);
}

TEST_F(EntityLinkerTest, ExportLinkGraphWithConfidenceScores) {
    linker.createLink(makeLink("src-1", "tgt-1", 0.97), defaultOptions());
    auto graph = linker.exportLinkGraph("", {}, true);
    ASSERT_FALSE(graph["edges"].empty());
    EXPECT_TRUE(graph["edges"][0].contains("confidence"));
    EXPECT_NEAR(graph["edges"][0]["confidence"].get<double>(), 0.97, 1e-6);
}

TEST_F(EntityLinkerTest, ExportLinkGraphWithoutConfidenceScores) {
    linker.createLink(makeLink("src-1", "tgt-1", 0.97), defaultOptions());
    auto graph = linker.exportLinkGraph("", {}, false);
    ASSERT_FALSE(graph["edges"].empty());
    EXPECT_FALSE(graph["edges"][0].contains("confidence"));
}

TEST_F(EntityLinkerTest, ExportLinkGraphFilterByEntityIds) {
    auto l1 = makeLink("A", "B");
    l1.metadata["collection"] = "col";
    auto l2 = makeLink("C", "D");
    l2.metadata["collection"] = "col";

    linker.createLink(l1, defaultOptions());
    linker.createLink(l2, defaultOptions());

    // Only request links involving "A" or "B".
    auto graph = linker.exportLinkGraph("", {"A", "B"}, true);
    ASSERT_EQ(graph["edges"].size(), 1u);
}

// ============================================================================
// linkCount / clear tests
// ============================================================================

TEST_F(EntityLinkerTest, ClearResetsLinkCount) {
    linker.createLink(makeLink("s1", "t1"), defaultOptions());
    linker.createLink(makeLink("s2", "t2"), defaultOptions());
    EXPECT_EQ(linker.linkCount(), 2u);
    linker.clear();
    EXPECT_EQ(linker.linkCount(), 0u);
}

// ============================================================================
// EntityLink serialisation
// ============================================================================

TEST_F(EntityLinkerTest, EntityLinkToJsonContainsAllFields) {
    auto link = makeLink("src-X", "tgt-Y", 0.88, ti::LinkType::DUPLICATE_OF);
    link.matched_fields = {"email", "phone"};
    auto j = link.toJson();
    EXPECT_EQ(j["source_id"].get<std::string>(), "src-X");
    EXPECT_EQ(j["target_id"].get<std::string>(), "tgt-Y");
    EXPECT_NEAR(j["confidence"].get<double>(), 0.88, 1e-6);
    EXPECT_EQ(j["link_type"].get<std::string>(), "duplicate_of");
    EXPECT_EQ(j["matched_fields"].size(), 2u);
}

TEST_F(EntityLinkerTest, BiDirectionalLinkingConsistency) {
    // Forward link
    auto fwd = makeLink("entity-new", "entity-existing");
    linker.createLink(fwd, defaultOptions());

    // Reverse link
    auto rev = makeLink("entity-existing", "entity-new");
    linker.createLink(rev, defaultOptions());

    auto fwd_entries = linker.getLinksForEntity("entity-new",      "");
    auto rev_entries = linker.getLinksForEntity("entity-existing", "");

    // Each entity appears in both directions.
    EXPECT_GE(fwd_entries.size(), 1u);
    EXPECT_GE(rev_entries.size(), 1u);
}
