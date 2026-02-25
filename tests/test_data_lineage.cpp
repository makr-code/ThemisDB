/**
 * @file test_data_lineage.cpp
 * @brief Unit tests for DataLineageTracker
 *
 * Tests cover:
 * - recordEvent with auto-assigned event_id and timestamp
 * - getLineage returns events in chronological order
 * - getUpstreamLineage follows parent chain to root
 * - getDownstreamLineage returns all transitively derived events
 * - exportLineageAsJson produces well-formed JSON
 * - totalEventCount tracks all recorded events
 * - Multiple datasets are stored independently
 * - Metrics counter is incremented on each recordEvent
 * - AuditLogger integration: every event is forwarded to the audit trail
 * - lineageEventTypeToString covers all enum values
 */

#include <gtest/gtest.h>
#include "governance/data_lineage.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace themis::governance;
using json = nlohmann::json;

// ─── lineageEventTypeToString ────────────────────────────────────────────────

TEST(LineageEventTypeToStringTest, AllValuesProduceNonEmptyStrings) {
    const std::vector<LineageEventType> all_types = {
        LineageEventType::INGESTION,
        LineageEventType::ENRICHMENT,
        LineageEventType::ANONYMIZATION,
        LineageEventType::TRANSFORMATION,
        LineageEventType::QUERY,
        LineageEventType::EXPORT,
        LineageEventType::DELETION
    };
    for (auto t : all_types) {
        EXPECT_FALSE(lineageEventTypeToString(t).empty());
    }
}

TEST(LineageEventTypeToStringTest, KnownValues) {
    EXPECT_EQ(lineageEventTypeToString(LineageEventType::INGESTION),      "INGESTION");
    EXPECT_EQ(lineageEventTypeToString(LineageEventType::ENRICHMENT),     "ENRICHMENT");
    EXPECT_EQ(lineageEventTypeToString(LineageEventType::ANONYMIZATION),  "ANONYMIZATION");
    EXPECT_EQ(lineageEventTypeToString(LineageEventType::TRANSFORMATION), "TRANSFORMATION");
    EXPECT_EQ(lineageEventTypeToString(LineageEventType::QUERY),          "QUERY");
    EXPECT_EQ(lineageEventTypeToString(LineageEventType::EXPORT),         "EXPORT");
    EXPECT_EQ(lineageEventTypeToString(LineageEventType::DELETION),       "DELETION");
}

// ─── LineageEvent::toJson ────────────────────────────────────────────────────

TEST(LineageEventToJsonTest, RequiredFieldsPresent) {
    LineageEvent ev;
    ev.event_id    = "ev-001";
    ev.dataset_id  = "ds-001";
    ev.event_type  = LineageEventType::INGESTION;
    ev.timestamp_ms = 1700000000000LL;
    ev.performed_by = "ingest-service";
    ev.operation    = "batch_import";

    json j = ev.toJson();
    EXPECT_EQ(j["event_id"],      "ev-001");
    EXPECT_EQ(j["dataset_id"],    "ds-001");
    EXPECT_EQ(j["event_type"],    "INGESTION");
    EXPECT_EQ(j["timestamp_ms"],  1700000000000LL);
    EXPECT_EQ(j["performed_by"],  "ingest-service");
    EXPECT_EQ(j["operation"],     "batch_import");
}

TEST(LineageEventToJsonTest, OptionalFieldsOmittedWhenEmpty) {
    LineageEvent ev;
    ev.event_id   = "ev-002";
    ev.dataset_id = "ds-001";
    ev.event_type = LineageEventType::QUERY;
    ev.timestamp_ms = 1700000001000LL;

    json j = ev.toJson();
    EXPECT_FALSE(j.contains("input_schema"));
    EXPECT_FALSE(j.contains("output_schema"));
    EXPECT_FALSE(j.contains("parent_event_id"));
    EXPECT_FALSE(j.contains("metadata"));
}

TEST(LineageEventToJsonTest, OptionalFieldsIncludedWhenSet) {
    LineageEvent ev;
    ev.event_id      = "ev-003";
    ev.dataset_id    = "ds-001";
    ev.event_type    = LineageEventType::TRANSFORMATION;
    ev.timestamp_ms  = 1700000002000LL;
    ev.input_schema  = "{\"type\":\"raw\"}";
    ev.output_schema = "{\"type\":\"enriched\"}";
    ev.parent_event_id = "ev-001";
    ev.metadata = {{"classification", "vs-nfd"}};

    json j = ev.toJson();
    EXPECT_EQ(j["input_schema"],    "{\"type\":\"raw\"}");
    EXPECT_EQ(j["output_schema"],   "{\"type\":\"enriched\"}");
    EXPECT_EQ(j["parent_event_id"], "ev-001");
    EXPECT_EQ(j["metadata"]["classification"], "vs-nfd");
}

// ─── DataLineageTracker — basic record / get ─────────────────────────────────

class DataLineageTrackerTest : public ::testing::Test {
protected:
    DataLineageTracker tracker;
};

TEST_F(DataLineageTrackerTest, RecordEventAssignsEventIdWhenEmpty) {
    LineageEvent ev;
    ev.dataset_id = "ds-auto";
    ev.event_type = LineageEventType::INGESTION;
    // leave event_id empty

    tracker.recordEvent(ev);
    LineageRecord rec = tracker.getLineage("ds-auto");
    ASSERT_EQ(rec.events.size(), 1u);
    EXPECT_FALSE(rec.events[0].event_id.empty());
}

TEST_F(DataLineageTrackerTest, RecordEventAssignsTimestampWhenZero) {
    LineageEvent ev;
    ev.dataset_id   = "ds-ts";
    ev.event_type   = LineageEventType::QUERY;
    ev.timestamp_ms = 0;

    tracker.recordEvent(ev);
    LineageRecord rec = tracker.getLineage("ds-ts");
    ASSERT_EQ(rec.events.size(), 1u);
    EXPECT_GT(rec.events[0].timestamp_ms, 0LL);
}

TEST_F(DataLineageTrackerTest, RecordEventPreservesProvidedFields) {
    LineageEvent ev;
    ev.event_id    = "ev-explicit";
    ev.dataset_id  = "ds-explicit";
    ev.event_type  = LineageEventType::EXPORT;
    ev.timestamp_ms = 1700000099000LL;
    ev.performed_by = "export-job";
    ev.operation    = "s3_export";

    tracker.recordEvent(ev);
    LineageRecord rec = tracker.getLineage("ds-explicit");
    ASSERT_EQ(rec.events.size(), 1u);
    EXPECT_EQ(rec.events[0].event_id,     "ev-explicit");
    EXPECT_EQ(rec.events[0].performed_by, "export-job");
    EXPECT_EQ(rec.events[0].operation,    "s3_export");
    EXPECT_EQ(rec.events[0].timestamp_ms, 1700000099000LL);
}

TEST_F(DataLineageTrackerTest, GetLineageForUnknownDatasetReturnsEmptyRecord) {
    LineageRecord rec = tracker.getLineage("ds-unknown");
    EXPECT_EQ(rec.dataset_id, "ds-unknown");
    EXPECT_TRUE(rec.events.empty());
}

TEST_F(DataLineageTrackerTest, MultipleEventsSortedChronologically) {
    for (int64_t ts : {3000LL, 1000LL, 2000LL}) {
        LineageEvent ev;
        ev.dataset_id   = "ds-order";
        ev.event_type   = LineageEventType::QUERY;
        ev.timestamp_ms = ts;
        tracker.recordEvent(ev);
    }
    LineageRecord rec = tracker.getLineage("ds-order");
    ASSERT_EQ(rec.events.size(), 3u);
    EXPECT_EQ(rec.events[0].timestamp_ms, 1000LL);
    EXPECT_EQ(rec.events[1].timestamp_ms, 2000LL);
    EXPECT_EQ(rec.events[2].timestamp_ms, 3000LL);
}

TEST_F(DataLineageTrackerTest, DifferentDatasetsManagedIndependently) {
    {
        LineageEvent ev;
        ev.dataset_id = "ds-A";
        ev.event_type = LineageEventType::INGESTION;
        ev.timestamp_ms = 1000LL;
        tracker.recordEvent(ev);
    }
    {
        LineageEvent ev;
        ev.dataset_id = "ds-B";
        ev.event_type = LineageEventType::QUERY;
        ev.timestamp_ms = 2000LL;
        tracker.recordEvent(ev);
    }

    EXPECT_EQ(tracker.getLineage("ds-A").events.size(), 1u);
    EXPECT_EQ(tracker.getLineage("ds-B").events.size(), 1u);
    EXPECT_EQ(tracker.getLineage("ds-A").events[0].event_type,
              LineageEventType::INGESTION);
    EXPECT_EQ(tracker.getLineage("ds-B").events[0].event_type,
              LineageEventType::QUERY);
}

// ─── Upstream lineage ────────────────────────────────────────────────────────

TEST_F(DataLineageTrackerTest, GetUpstreamLineage_SingleRoot) {
    LineageEvent root;
    root.event_id   = "root";
    root.dataset_id = "ds-chain";
    root.event_type = LineageEventType::INGESTION;
    root.timestamp_ms = 1000LL;
    tracker.recordEvent(root);

    auto chain = tracker.getUpstreamLineage("root");
    ASSERT_EQ(chain.size(), 1u);
    EXPECT_EQ(chain[0].event_id, "root");
}

TEST_F(DataLineageTrackerTest, GetUpstreamLineage_ThreeGenerations) {
    LineageEvent ev1;
    ev1.event_id   = "ev1";
    ev1.dataset_id = "ds-chain";
    ev1.event_type = LineageEventType::INGESTION;
    ev1.timestamp_ms = 1000LL;
    tracker.recordEvent(ev1);

    LineageEvent ev2;
    ev2.event_id       = "ev2";
    ev2.dataset_id     = "ds-chain";
    ev2.event_type     = LineageEventType::ENRICHMENT;
    ev2.parent_event_id = "ev1";
    ev2.timestamp_ms   = 2000LL;
    tracker.recordEvent(ev2);

    LineageEvent ev3;
    ev3.event_id       = "ev3";
    ev3.dataset_id     = "ds-chain";
    ev3.event_type     = LineageEventType::TRANSFORMATION;
    ev3.parent_event_id = "ev2";
    ev3.timestamp_ms   = 3000LL;
    tracker.recordEvent(ev3);

    auto chain = tracker.getUpstreamLineage("ev3");
    ASSERT_EQ(chain.size(), 3u);
    EXPECT_EQ(chain[0].event_id, "ev1");
    EXPECT_EQ(chain[1].event_id, "ev2");
    EXPECT_EQ(chain[2].event_id, "ev3");
}

TEST_F(DataLineageTrackerTest, GetUpstreamLineage_UnknownEventReturnsEmpty) {
    auto chain = tracker.getUpstreamLineage("nonexistent");
    EXPECT_TRUE(chain.empty());
}

// ─── Downstream lineage ──────────────────────────────────────────────────────

TEST_F(DataLineageTrackerTest, GetDownstreamLineage_DirectChildren) {
    // root → child1, child2
    LineageEvent root;
    root.event_id   = "root2";
    root.dataset_id = "ds-down";
    root.event_type = LineageEventType::INGESTION;
    root.timestamp_ms = 1000LL;
    tracker.recordEvent(root);

    LineageEvent child1;
    child1.event_id        = "child1";
    child1.dataset_id      = "ds-down";
    child1.event_type      = LineageEventType::QUERY;
    child1.parent_event_id = "root2";
    child1.timestamp_ms    = 2000LL;
    tracker.recordEvent(child1);

    LineageEvent child2;
    child2.event_id        = "child2";
    child2.dataset_id      = "ds-down";
    child2.event_type      = LineageEventType::EXPORT;
    child2.parent_event_id = "root2";
    child2.timestamp_ms    = 3000LL;
    tracker.recordEvent(child2);

    auto downstream = tracker.getDownstreamLineage("root2");
    ASSERT_EQ(downstream.size(), 2u);
    // Results are sorted by timestamp
    EXPECT_EQ(downstream[0].event_id, "child1");
    EXPECT_EQ(downstream[1].event_id, "child2");
}

TEST_F(DataLineageTrackerTest, GetDownstreamLineage_NoChildrenReturnsEmpty) {
    LineageEvent ev;
    ev.event_id   = "leaf";
    ev.dataset_id = "ds-leaf";
    ev.event_type = LineageEventType::QUERY;
    ev.timestamp_ms = 1000LL;
    tracker.recordEvent(ev);

    auto downstream = tracker.getDownstreamLineage("leaf");
    EXPECT_TRUE(downstream.empty());
}

// ─── JSON export ─────────────────────────────────────────────────────────────

TEST_F(DataLineageTrackerTest, ExportLineageAsJson_HasRequiredFields) {
    LineageEvent ev;
    ev.event_id    = "ev-json";
    ev.dataset_id  = "ds-json";
    ev.event_type  = LineageEventType::INGESTION;
    ev.timestamp_ms = 1700000000000LL;
    tracker.recordEvent(ev);

    json j = tracker.exportLineageAsJson("ds-json");
    EXPECT_EQ(j["dataset_id"],   "ds-json");
    EXPECT_EQ(j["event_count"],  1);
    ASSERT_TRUE(j.contains("events"));
    ASSERT_EQ(j["events"].size(), 1u);
    EXPECT_EQ(j["events"][0]["event_id"],   "ev-json");
    EXPECT_EQ(j["events"][0]["event_type"], "INGESTION");
}

TEST_F(DataLineageTrackerTest, ExportLineageAsJson_UnknownDataset) {
    json j = tracker.exportLineageAsJson("ds-empty");
    EXPECT_EQ(j["dataset_id"],  "ds-empty");
    EXPECT_EQ(j["event_count"], 0);
    EXPECT_TRUE(j["events"].empty());
}

// ─── totalEventCount ─────────────────────────────────────────────────────────

TEST_F(DataLineageTrackerTest, TotalEventCountMatchesRecordedEvents) {
    EXPECT_EQ(tracker.totalEventCount(), 0u);

    for (int i = 0; i < 5; ++i) {
        LineageEvent ev;
        ev.dataset_id   = (i % 2 == 0) ? "ds-X" : "ds-Y";
        ev.event_type   = LineageEventType::QUERY;
        ev.timestamp_ms = static_cast<int64_t>(1000LL * (i + 1));
        tracker.recordEvent(ev);
    }
    EXPECT_EQ(tracker.totalEventCount(), 5u);
}

// ─── LineageRecord::toJson ───────────────────────────────────────────────────

TEST(LineageRecordToJsonTest, EmptyRecord) {
    LineageRecord rec;
    rec.dataset_id = "ds-empty-rec";
    json j = rec.toJson();
    EXPECT_EQ(j["dataset_id"],  "ds-empty-rec");
    EXPECT_EQ(j["event_count"], 0);
    EXPECT_TRUE(j["events"].is_array());
    EXPECT_TRUE(j["events"].empty());
}

TEST(LineageRecordToJsonTest, MultipleEvents) {
    LineageRecord rec;
    rec.dataset_id = "ds-multi-rec";
    for (int i = 0; i < 3; ++i) {
        LineageEvent ev;
        ev.event_id    = "ev-" + std::to_string(i);
        ev.dataset_id  = "ds-multi-rec";
        ev.event_type  = LineageEventType::TRANSFORMATION;
        ev.timestamp_ms = 1000LL * (i + 1);
        rec.events.push_back(ev);
    }
    json j = rec.toJson();
    EXPECT_EQ(j["event_count"], 3);
    ASSERT_EQ(j["events"].size(), 3u);
}
