/**
 * CEP Engine unit + integration tests.
 *
 * Covers:
 *  - Event serialization round-trip
 *  - EventStream push/pull, backpressure, subscriptions
 *  - PatternMatcher: SEQUENCE, DISJUNCTION, CONJUNCTION, NEGATION, REPETITION
 *  - WindowManager: TUMBLING, SLIDING, SESSION, COUNT, GLOBAL
 *  - Aggregator: COUNT, SUM, AVG, MIN, MAX, STDDEV, VARIANCE, DISTINCT_COUNT,
 *                FIRST, LAST, PERCENTILE, COLLECT, TOPN; GROUP BY
 *  - RuleEngine: add/remove rules, filter evaluation, pattern + window rules, alerts
 *  - CEPEngine: singleton lifecycle, stream management, submitEvent, rule management,
 *               alert queue, Prometheus metrics, checkpoint I/O, EPL parsing
 */

#include <gtest/gtest.h>

#include "analytics/cep_engine.h"

#include <chrono>
#include <fstream>
#include <thread>
#include <filesystem>

using namespace themisdb::analytics;

// ============================================================================
// Helpers
// ============================================================================

static Event makeEvent(const std::string& name,
                        EventType type = EventType::CUSTOM,
                        const std::string& partition_key = "") {
    Event ev;
    ev.event_id    = "test-" + name;
    ev.event_name  = name;
    ev.type        = type;
    ev.timestamp   = std::chrono::system_clock::now();
    ev.partition_key = partition_key;
    return ev;
}

static Event makeEventWithField(const std::string& name,
                                 const std::string& field,
                                 double value) {
    auto ev = makeEvent(name);
    ev.setField(field, value);
    return ev;
}

// ============================================================================
// Event serialization
// ============================================================================

TEST(CEPEventTest, SerializeDeserializeRoundTrip) {
    Event ev;
    ev.event_id    = "evt-001";
    ev.event_name  = "DOCUMENT_INSERT";
    ev.type        = EventType::DOCUMENT_INSERT;
    ev.priority    = EventPriority::HIGH;
    ev.timestamp   = std::chrono::system_clock::now();
    ev.collection_name = "orders";
    ev.document_id = "doc-42";
    ev.partition_key = "shard-1";
    ev.sequence_number = 99;
    ev.setField("amount", 123.45);
    ev.setField("status", std::string("pending"));

    auto bytes = ev.serialize();
    EXPECT_FALSE(bytes.empty());

    auto restored = Event::deserialize(bytes);
    ASSERT_TRUE(restored.has_value());
    EXPECT_EQ(restored->event_id, ev.event_id);
    EXPECT_EQ(restored->event_name, ev.event_name);
    EXPECT_EQ(restored->collection_name, ev.collection_name);
    EXPECT_EQ(restored->document_id, ev.document_id);
    EXPECT_EQ(restored->sequence_number, ev.sequence_number);
}

TEST(CEPEventTest, DeserializeEmptyReturnsNullopt) {
    auto result = Event::deserialize({});
    EXPECT_FALSE(result.has_value());
}

TEST(CEPEventTest, GetFieldReturnsCorrectType) {
    Event ev = makeEvent("test");
    ev.setField("count", int64_t(7));
    ev.setField("value", 3.14);
    ev.setField("name",  std::string("hello"));
    ev.setField("flag",  true);

    EXPECT_EQ(*ev.getField<int64_t>("count"), 7);
    EXPECT_DOUBLE_EQ(*ev.getField<double>("value"), 3.14);
    EXPECT_EQ(*ev.getField<std::string>("name"), "hello");
    EXPECT_TRUE(*ev.getField<bool>("flag"));
    EXPECT_FALSE(ev.getField<int64_t>("missing").has_value());
}

// ============================================================================
// EventStream tests
// ============================================================================

class EventStreamTest : public ::testing::Test {
protected:
    StreamConfig makeConfig(size_t buf = 16, uint32_t parts = 2) {
        StreamConfig c;
        c.stream_id   = "test-stream";
        c.stream_name = "test-stream";
        c.buffer_size = buf;
        c.partitions  = parts;
        c.enable_backpressure = true;
        c.backpressure_threshold = 0.75f;
        return c;
    }
};

TEST_F(EventStreamTest, PushPull) {
    EventStream stream(makeConfig(64, 1));
    auto ev = makeEvent("A");
    ev.partition_id = 0;

    auto result = stream.push(ev);
    EXPECT_EQ(result, EventStream::PushResult::SUCCESS);

    auto pulled = stream.pull(0);
    ASSERT_TRUE(pulled.has_value());
    EXPECT_EQ(pulled->event_name, "A");
}

TEST_F(EventStreamTest, PullFromEmptyReturnsNullopt) {
    EventStream stream(makeConfig(8, 1));
    EXPECT_FALSE(stream.pull(0).has_value());
}

TEST_F(EventStreamTest, PeekDoesNotConsume) {
    EventStream stream(makeConfig(8, 1));
    auto ev = makeEvent("X");
    ev.partition_id = 0;
    stream.push(ev);

    auto peeked = stream.peek(0);
    ASSERT_TRUE(peeked.has_value());
    EXPECT_EQ(peeked->event_name, "X");

    // event still present after peek
    auto pulled = stream.pull(0);
    ASSERT_TRUE(pulled.has_value());
    EXPECT_EQ(pulled->event_name, "X");
}

TEST_F(EventStreamTest, DroppedWhenBufferFull) {
    // buffer_size=4, 1 partition -> max 4 events
    EventStream stream(makeConfig(4, 1));
    stream.push(makeEvent("1"));
    stream.push(makeEvent("2"));
    stream.push(makeEvent("3"));
    stream.push(makeEvent("4"));

    // 5th event should be dropped
    auto res5 = stream.push(makeEvent("5"));
    EXPECT_EQ(res5, EventStream::PushResult::DROPPED);

    auto stats = stream.getStats();
    EXPECT_GE(stats.events_dropped, 1u);
}

TEST_F(EventStreamTest, Stats) {
    EventStream stream(makeConfig(64, 2));
    auto ev1 = makeEvent("A"); ev1.partition_id = 0;
    auto ev2 = makeEvent("B"); ev2.partition_id = 1;
    stream.push(ev1);
    stream.push(ev2);
    stream.pull(0);

    auto s = stream.getStats();
    EXPECT_EQ(s.events_pushed, 2u);
    EXPECT_EQ(s.events_pulled, 1u);
    EXPECT_EQ(s.current_size, 1u);
}

TEST_F(EventStreamTest, SubscribeReceivesEvents) {
    EventStream stream(makeConfig(16, 1));

    std::vector<std::string> received;
    uint64_t sub_id = stream.subscribe([&](const Event& ev) {
        received.push_back(ev.event_name);
    });

    auto ev1 = makeEvent("hello"); ev1.partition_id = 0;
    auto ev2 = makeEvent("world"); ev2.partition_id = 0;
    stream.push(ev1);
    stream.push(ev2);

    EXPECT_EQ(received.size(), 2u);
    EXPECT_EQ(received[0], "hello");
    EXPECT_EQ(received[1], "world");

    stream.unsubscribe(sub_id);
    stream.push(makeEvent("ignored"));
    EXPECT_EQ(received.size(), 2u); // no new callback
}

// ============================================================================
// PatternMatcher tests
// ============================================================================

class PatternMatcherTest : public ::testing::Test {
protected:
    PatternConfig makeSeq(std::vector<std::string> types,
                          std::chrono::milliseconds within = std::chrono::milliseconds(0)) {
        PatternConfig pc;
        pc.pattern_id   = "seq-pattern";
        pc.type         = PatternType::SEQUENCE;
        pc.event_types  = std::move(types);
        pc.within       = within;
        pc.min_occurrences = 1;
        pc.max_occurrences = UINT32_MAX;
        return pc;
    }
};

TEST_F(PatternMatcherTest, SequenceMatch) {
    PatternMatcher pm(makeSeq({"A", "B", "C"}));

    auto evA = makeEvent("A");
    auto evB = makeEvent("B");
    auto evC = makeEvent("C");

    EXPECT_TRUE(pm.processEvent(evA).empty());
    EXPECT_TRUE(pm.processEvent(evB).empty());

    auto matches = pm.processEvent(evC);
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].matched_events.size(), 3u);
}

TEST_F(PatternMatcherTest, SequenceNoMatchWrongOrder) {
    PatternMatcher pm(makeSeq({"A", "B"}));

    auto evB = makeEvent("B");
    auto evA = makeEvent("A");

    EXPECT_TRUE(pm.processEvent(evB).empty());
    EXPECT_TRUE(pm.processEvent(evA).empty()); // A starts new partial, B already consumed
}

TEST_F(PatternMatcherTest, DisjunctionMatch) {
    PatternConfig pc;
    pc.pattern_id  = "disj";
    pc.type        = PatternType::DISJUNCTION;
    pc.event_types = {"X", "Y"};
    PatternMatcher pm(pc);

    auto matches = pm.processEvent(makeEvent("Y"));
    EXPECT_EQ(matches.size(), 1u);

    matches = pm.processEvent(makeEvent("Z"));
    EXPECT_TRUE(matches.empty());
}

TEST_F(PatternMatcherTest, ConjunctionMatch) {
    PatternConfig pc;
    pc.pattern_id  = "conj";
    pc.type        = PatternType::CONJUNCTION;
    pc.event_types = {"P", "Q"};
    pc.tolerance   = std::chrono::milliseconds(5000);
    PatternMatcher pm(pc);

    pm.processEvent(makeEvent("P"));
    auto matches = pm.processEvent(makeEvent("Q"));
    EXPECT_FALSE(matches.empty());
}

TEST_F(PatternMatcherTest, MatchCountIncremented) {
    PatternMatcher pm(makeSeq({"A"}));
    pm.processEvent(makeEvent("A"));
    pm.processEvent(makeEvent("A"));
    EXPECT_EQ(pm.getMatchCount(), 2u);
}

TEST_F(PatternMatcherTest, ResetClearsState) {
    PatternMatcher pm(makeSeq({"A", "B"}));
    pm.processEvent(makeEvent("A")); // partial match pending
    EXPECT_GT(pm.getPendingMatchCount(), 0u);

    pm.reset();
    EXPECT_EQ(pm.getPendingMatchCount(), 0u);
    EXPECT_EQ(pm.getMatchCount(), 0u);
}

TEST_F(PatternMatcherTest, SingleEventSequenceMatches) {
    PatternMatcher pm(makeSeq({"EVT"}));
    auto matches = pm.processEvent(makeEvent("EVT"));
    EXPECT_EQ(matches.size(), 1u);
}

// ============================================================================
// WindowManager tests
// ============================================================================

class WindowManagerTest : public ::testing::Test {
protected:
    WindowConfig makeTumbling(std::chrono::milliseconds size) {
        WindowConfig wc;
        wc.type = WindowType::TUMBLING;
        wc.size = size;
        wc.emit_on_close = true;
        return wc;
    }

    WindowConfig makeCount(uint64_t count) {
        WindowConfig wc;
        wc.type  = WindowType::COUNT;
        wc.count = count;
        wc.emit_on_close = true;
        return wc;
    }

    WindowConfig makeSession(std::chrono::milliseconds gap) {
        WindowConfig wc;
        wc.type = WindowType::SESSION;
        wc.gap  = gap;
        wc.emit_on_close = true;
        return wc;
    }
};

TEST_F(WindowManagerTest, TumblingWindowAccumulates) {
    WindowManager wm(makeTumbling(std::chrono::milliseconds(60000)));

    auto now = std::chrono::system_clock::now();
    auto ev1 = makeEvent("E1"); ev1.timestamp = now;
    auto ev2 = makeEvent("E2"); ev2.timestamp = now + std::chrono::milliseconds(100);

    wm.addEvent(ev1);
    wm.addEvent(ev2);

    auto events = wm.getWindowEvents();
    EXPECT_EQ(events.size(), 2u);
}

TEST_F(WindowManagerTest, CountWindowEmitsOnCount) {
    std::vector<std::vector<Event>> emitted;
    WindowManager wm(makeCount(3));
    wm.setWindowCallback([&](const std::vector<Event>& evs, auto, auto) {
        emitted.push_back(evs);
    });

    wm.addEvent(makeEvent("E1"));
    wm.addEvent(makeEvent("E2"));
    EXPECT_TRUE(emitted.empty()); // not yet
    wm.addEvent(makeEvent("E3")); // triggers close
    EXPECT_EQ(emitted.size(), 1u);
    EXPECT_EQ(emitted[0].size(), 3u);
}

TEST_F(WindowManagerTest, GlobalWindowAccumulatesAll) {
    WindowConfig wc;
    wc.type = WindowType::GLOBAL;
    wc.emit_on_close = false;
    WindowManager wm(wc);

    for (int i = 0; i < 5; ++i) {
        wm.addEvent(makeEvent("E" + std::to_string(i)));
    }
    EXPECT_EQ(wm.getWindowEvents().size(), 5u);
}

TEST_F(WindowManagerTest, TumblingWindowStats) {
    WindowManager wm(makeTumbling(std::chrono::milliseconds(60000)));
    wm.addEvent(makeEvent("E1"));
    auto stats = wm.getStats();
    EXPECT_GE(stats.windows_created, 1u);
}

TEST_F(WindowManagerTest, GetEventsByTimeRange) {
    WindowManager wm(makeTumbling(std::chrono::milliseconds(60000)));

    auto base = std::chrono::system_clock::now();
    auto ev1 = makeEvent("E1"); ev1.timestamp = base;
    auto ev2 = makeEvent("E2"); ev2.timestamp = base + std::chrono::seconds(5);
    auto ev3 = makeEvent("E3"); ev3.timestamp = base + std::chrono::seconds(10);
    wm.addEvent(ev1);
    wm.addEvent(ev2);
    wm.addEvent(ev3);

    auto selected = wm.getEvents(base, base + std::chrono::seconds(7));
    EXPECT_EQ(selected.size(), 2u);
}

TEST_F(WindowManagerTest, SessionWindowExtendedByEvents) {
    std::vector<std::vector<Event>> emitted;
    WindowManager wm(makeSession(std::chrono::milliseconds(200)));
    wm.setWindowCallback([&](const std::vector<Event>& evs, auto, auto) {
        emitted.push_back(evs);
    });

    auto base = std::chrono::system_clock::now();
    auto ev1 = makeEvent("S1"); ev1.timestamp = base; ev1.partition_key = "user1";
    auto ev2 = makeEvent("S2");
    ev2.timestamp = base + std::chrono::milliseconds(50);
    ev2.partition_key = "user1";

    wm.addEvent(ev1);
    wm.addEvent(ev2);

    // Within gap: still open
    auto events = wm.getWindowEvents();
    // Session window may not be in the main deque; check via stats
    auto stats = wm.getStats();
    EXPECT_GE(stats.windows_created, 1u);
}

// ============================================================================
// Aggregator tests
// ============================================================================

TEST(AggregatorTest, CountAggregation) {
    Aggregator agg;
    agg.addAggregation("cnt", AggregationType::COUNT, "value");
    for (int i = 0; i < 5; ++i) agg.processEvent(makeEventWithField("E", "value", i));
    auto res = agg.getResult("cnt");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(std::get<int64_t>(res->result), 5);
}

TEST(AggregatorTest, SumAggregation) {
    Aggregator agg;
    agg.addAggregation("total", AggregationType::SUM, "value");
    agg.processEvent(makeEventWithField("E", "value", 10.0));
    agg.processEvent(makeEventWithField("E", "value", 20.0));
    agg.processEvent(makeEventWithField("E", "value", 30.0));
    auto res = agg.getResult("total");
    ASSERT_TRUE(res.has_value());
    EXPECT_DOUBLE_EQ(std::get<double>(res->result), 60.0);
}

TEST(AggregatorTest, AvgAggregation) {
    Aggregator agg;
    agg.addAggregation("mean", AggregationType::AVG, "val");
    agg.processEvent(makeEventWithField("E", "val", 2.0));
    agg.processEvent(makeEventWithField("E", "val", 4.0));
    agg.processEvent(makeEventWithField("E", "val", 6.0));
    auto res = agg.getResult("mean");
    ASSERT_TRUE(res.has_value());
    EXPECT_DOUBLE_EQ(std::get<double>(res->result), 4.0);
}

TEST(AggregatorTest, MinMaxAggregation) {
    Aggregator agg;
    agg.addAggregation("mn", AggregationType::MIN, "v");
    agg.addAggregation("mx", AggregationType::MAX, "v");
    agg.processEvent(makeEventWithField("E", "v", 5.0));
    agg.processEvent(makeEventWithField("E", "v", 2.0));
    agg.processEvent(makeEventWithField("E", "v", 8.0));

    EXPECT_DOUBLE_EQ(std::get<double>(agg.getResult("mn")->result), 2.0);
    EXPECT_DOUBLE_EQ(std::get<double>(agg.getResult("mx")->result), 8.0);
}

TEST(AggregatorTest, FirstLastAggregation) {
    Aggregator agg;
    agg.addAggregation("first", AggregationType::FIRST, "v");
    agg.addAggregation("last",  AggregationType::LAST,  "v");
    agg.processEvent(makeEventWithField("E", "v", 1.0));
    agg.processEvent(makeEventWithField("E", "v", 2.0));
    agg.processEvent(makeEventWithField("E", "v", 3.0));

    EXPECT_DOUBLE_EQ(std::get<double>(agg.getResult("first")->result), 1.0);
    EXPECT_DOUBLE_EQ(std::get<double>(agg.getResult("last")->result),  3.0);
}

TEST(AggregatorTest, StddevVarianceAggregation) {
    Aggregator agg;
    agg.addAggregation("std", AggregationType::STDDEV,  "v");
    agg.addAggregation("var", AggregationType::VARIANCE, "v");
    for (double v : {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}) {
        agg.processEvent(makeEventWithField("E", "v", v));
    }
    auto std_res = agg.getResult("std");
    ASSERT_TRUE(std_res.has_value());
    double std_val = std::get<double>(std_res->result);
    EXPECT_GT(std_val, 0.0);

    auto var_res = agg.getResult("var");
    ASSERT_TRUE(var_res.has_value());
    double var_val = std::get<double>(var_res->result);
    EXPECT_NEAR(var_val, std_val * std_val, 0.001);
}

TEST(AggregatorTest, DistinctCount) {
    Aggregator agg;
    agg.addAggregation("dc", AggregationType::DISTINCT_COUNT, "category");
    for (const std::string& cat : {"A", "B", "A", "C", "B"}) {
        auto ev = makeEvent("E");
        ev.setField("category", cat);
        agg.processEvent(ev);
    }
    EXPECT_EQ(std::get<int64_t>(agg.getResult("dc")->result), 3);
}

TEST(AggregatorTest, CollectAndTopN) {
    Aggregator agg;
    agg.addAggregation("vals", AggregationType::COLLECT, "v");
    agg.addAggregation("top",  AggregationType::TOPN,    "v");
    for (double v : {1.0, 5.0, 3.0, 9.0, 2.0}) {
        agg.processEvent(makeEventWithField("E", "v", v));
    }
    auto collect_res = agg.getResult("vals");
    ASSERT_TRUE(collect_res.has_value());
    EXPECT_EQ(std::get<std::vector<std::string>>(collect_res->result).size(), 5u);

    auto topn_res = agg.getResult("top");
    ASSERT_TRUE(topn_res.has_value());
    // First element should be the largest
    auto top = std::get<std::vector<std::string>>(topn_res->result);
    EXPECT_FALSE(top.empty());
}

TEST(AggregatorTest, ResetClearsResults) {
    Aggregator agg;
    agg.addAggregation("cnt", AggregationType::COUNT, "v");
    agg.processEvent(makeEventWithField("E", "v", 1.0));
    agg.reset();
    // After reset, count should be zero (empty / no events)
    auto res = agg.getResult("cnt");
    // result exists (aggregation definition retained) but count=0
    if (res.has_value()) {
        EXPECT_EQ(std::get<int64_t>(res->result), 0);
    }
}

TEST(AggregatorTest, GroupByAggregation) {
    Aggregator agg;
    agg.addAggregation("cnt", AggregationType::COUNT, "v");
    agg.setGroupBy({"category"});

    for (const std::string& cat : {"A", "B", "A", "A", "B"}) {
        auto ev = makeEvent("E");
        ev.setField("category", cat);
        ev.setField("v", 1.0);
        agg.processEvent(ev);
    }

    auto results = agg.getResults();
    EXPECT_FALSE(results.empty());
    // Should have results for groups "A" and "B"
    bool found_a = false, found_b = false;
    for (const auto& [name, res] : results) {
        if (res.group_by_values.count("category")) {
            const auto& category_value = res.group_by_values.at("category");
            if (const auto* category = std::get_if<std::string>(&category_value)) {
                if (*category == "A") found_a = true;
                if (*category == "B") found_b = true;
            }
        }
    }
    EXPECT_TRUE(found_a);
    EXPECT_TRUE(found_b);
}

// ============================================================================
// RuleEngine tests
// ============================================================================

class RuleEngineTest : public ::testing::Test {
protected:
    CEPEngine* engine_ = nullptr;
    std::unique_ptr<RuleEngine> rule_engine_;

    void SetUp() override {
        // Use a fresh CEPEngine state – don't call getInstance to avoid
        // interfering with CEPEngine singleton tests. Instead construct manually.
        rule_engine_ = std::make_unique<RuleEngine>(nullptr);
    }

    RuleConfig makeSimpleRule(const std::string& id = "rule1") {
        RuleConfig cfg;
        cfg.rule_id   = id;
        cfg.rule_name = id;
        cfg.enabled   = true;
        ActionConfig ac;
        ac.type = ActionType::ALERT;
        ac.template_str = "Test alert";
        cfg.actions.push_back(ac);
        return cfg;
    }
};

TEST_F(RuleEngineTest, AddAndGetRule) {
    auto rule = makeSimpleRule("r1");
    EXPECT_TRUE(rule_engine_->addRule(rule));

    auto retrieved = rule_engine_->getRule("r1");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->rule_id, "r1");
}

TEST_F(RuleEngineTest, RemoveRule) {
    rule_engine_->addRule(makeSimpleRule("rm1"));
    EXPECT_TRUE(rule_engine_->removeRule("rm1"));
    EXPECT_FALSE(rule_engine_->getRule("rm1").has_value());
}

TEST_F(RuleEngineTest, GetRulesReturnsAll) {
    rule_engine_->addRule(makeSimpleRule("a"));
    rule_engine_->addRule(makeSimpleRule("b"));
    rule_engine_->addRule(makeSimpleRule("c"));
    EXPECT_EQ(rule_engine_->getRules().size(), 3u);
}

TEST_F(RuleEngineTest, EnabledRuleGeneratesAlert) {
    auto rule = makeSimpleRule("enabled-rule");
    rule_engine_->addRule(rule);

    auto ev = makeEvent("anything");
    auto alerts = rule_engine_->processEvent(ev);
    EXPECT_EQ(alerts.size(), 1u);
    EXPECT_EQ(alerts[0].rule_id, "enabled-rule");
}

TEST_F(RuleEngineTest, DisabledRuleNoAlert) {
    auto rule = makeSimpleRule("disabled-rule");
    rule.enabled = false;
    rule_engine_->addRule(rule);

    auto alerts = rule_engine_->processEvent(makeEvent("anything"));
    EXPECT_TRUE(alerts.empty());
}

TEST_F(RuleEngineTest, SetRuleEnabled) {
    auto rule = makeSimpleRule("toggle-rule");
    rule.enabled = false;
    rule_engine_->addRule(rule);

    EXPECT_TRUE(rule_engine_->processEvent(makeEvent("E")).empty());

    rule_engine_->setRuleEnabled("toggle-rule", true);
    EXPECT_FALSE(rule_engine_->processEvent(makeEvent("E")).empty());
}

TEST_F(RuleEngineTest, FilterBlocksNonMatchingEvents) {
    auto rule = makeSimpleRule("filtered-rule");
    rule.filter = "collection == orders";
    rule_engine_->addRule(rule);

    auto ev_orders = makeEvent("E");
    ev_orders.collection_name = "orders";
    EXPECT_FALSE(rule_engine_->processEvent(ev_orders).empty());

    auto ev_other = makeEvent("E");
    ev_other.collection_name = "users";
    EXPECT_TRUE(rule_engine_->processEvent(ev_other).empty());
}

TEST_F(RuleEngineTest, PatternRuleMatchesOnComplete) {
    auto rule = makeSimpleRule("seq-rule");
    PatternConfig pc;
    pc.pattern_id  = "p1";
    pc.type        = PatternType::SEQUENCE;
    pc.event_types = {"START", "END"};
    rule.pattern   = pc;
    rule_engine_->addRule(rule);

    EXPECT_TRUE(rule_engine_->processEvent(makeEvent("START")).empty());
    auto alerts = rule_engine_->processEvent(makeEvent("END"));
    EXPECT_EQ(alerts.size(), 1u);
}

TEST_F(RuleEngineTest, WindowRuleAccumulatesEvents) {
    auto rule = makeSimpleRule("win-rule");
    WindowConfig wc;
    wc.type = WindowType::GLOBAL;
    wc.emit_on_close = false;
    rule.window = wc;
    rule_engine_->addRule(rule);

    for (int i = 0; i < 5; ++i) {
        rule_engine_->processEvent(makeEvent("E" + std::to_string(i)));
    }

    auto stats = rule_engine_->getRuleStats("win-rule");
    EXPECT_GE(stats.events_processed, 5u);
}

TEST_F(RuleEngineTest, ParseEPLCreatesValidRule) {
    std::string epl = "SELECT * FROM orders_stream "
                      "PATTERN SEQUENCE (INSERT, UPDATE) WITHIN 30000ms "
                      "ON MATCH ALERT severity=warning";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_FALSE(cfg->rule_id.empty());
    ASSERT_TRUE(cfg->pattern.has_value());
    EXPECT_EQ(cfg->pattern->type, PatternType::SEQUENCE);
    EXPECT_EQ(cfg->pattern->event_types.size(), 2u);
    EXPECT_EQ(cfg->pattern->within, std::chrono::milliseconds(30000));
    EXPECT_EQ(cfg->tags.at("severity"), "warning");
}

TEST_F(RuleEngineTest, ParseEPLWithWindow) {
    std::string epl = "SELECT * FROM stream WINDOW TUMBLING 5000ms ON MATCH ALERT";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    ASSERT_TRUE(cfg->window.has_value());
    EXPECT_EQ(cfg->window->type, WindowType::TUMBLING);
    EXPECT_EQ(cfg->window->size, std::chrono::milliseconds(5000));
}

TEST_F(RuleEngineTest, ParseEPLCreateRuleAsSyntax) {
    std::string epl = "CREATE RULE fraud_detection AS SELECT * FROM PaymentEvents ON MATCH ALERT";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_EQ(cfg->rule_name, "fraud_detection");
    ASSERT_FALSE(cfg->streams.empty());
    EXPECT_EQ(cfg->streams[0], "PaymentEvents");
}

TEST_F(RuleEngineTest, ParseEPLSelectAggregations) {
    std::string epl = "SELECT COUNT(*) as cnt, SUM(amount) as total, AVG(latency_ms) as avg_lat "
                      "FROM events ON MATCH ALERT";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    ASSERT_EQ(cfg->aggregations.size(), 3u);
    EXPECT_EQ(cfg->aggregations[0].first, "cnt");
    EXPECT_EQ(cfg->aggregations[0].second, AggregationType::COUNT);
    EXPECT_EQ(cfg->aggregations[1].first, "total");
    EXPECT_EQ(cfg->aggregations[1].second, AggregationType::SUM);
    EXPECT_EQ(cfg->aggregations[2].first, "avg_lat");
    EXPECT_EQ(cfg->aggregations[2].second, AggregationType::AVG);
}

TEST_F(RuleEngineTest, ParseEPLGroupBy) {
    std::string epl = "SELECT COUNT(*) FROM events GROUP BY userId, region ON MATCH ALERT";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    ASSERT_EQ(cfg->group_by.size(), 2u);
    EXPECT_EQ(cfg->group_by[0], "userId");
    EXPECT_EQ(cfg->group_by[1], "region");
}

TEST_F(RuleEngineTest, ParseEPLWindowParenthesizedMinutes) {
    std::string epl = "SELECT * FROM events WINDOW TUMBLING(5 MINUTES) ON MATCH ALERT";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    ASSERT_TRUE(cfg->window.has_value());
    EXPECT_EQ(cfg->window->type, WindowType::TUMBLING);
    EXPECT_EQ(cfg->window->size, std::chrono::milliseconds(5 * 60 * 1000));
}

TEST_F(RuleEngineTest, ParseEPLWindowParenthesizedHours) {
    std::string epl = "SELECT * FROM events WINDOW TUMBLING(1 HOUR) ON MATCH ALERT";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    ASSERT_TRUE(cfg->window.has_value());
    EXPECT_EQ(cfg->window->type, WindowType::TUMBLING);
    EXPECT_EQ(cfg->window->size, std::chrono::milliseconds(3600 * 1000));
}

TEST_F(RuleEngineTest, ParseEPLWindowSlidingWithSlide) {
    std::string epl = "SELECT * FROM events WINDOW SLIDING(5 MINUTES, 1 MINUTE) ON MATCH ALERT";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    ASSERT_TRUE(cfg->window.has_value());
    EXPECT_EQ(cfg->window->type, WindowType::SLIDING);
    EXPECT_EQ(cfg->window->size,  std::chrono::milliseconds(5 * 60 * 1000));
    EXPECT_EQ(cfg->window->slide, std::chrono::milliseconds(1 * 60 * 1000));
}

TEST_F(RuleEngineTest, ParseEPLWindowCountEvents) {
    std::string epl = "SELECT * FROM events WINDOW COUNT(100 EVENTS) ON MATCH ALERT";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    ASSERT_TRUE(cfg->window.has_value());
    EXPECT_EQ(cfg->window->type, WindowType::COUNT);
    EXPECT_EQ(cfg->window->count, 100u);
}

TEST_F(RuleEngineTest, ParseEPLPatternWithinMinutes) {
    std::string epl = "SELECT * FROM stream "
                      "PATTERN SEQUENCE(LoginEvent, PurchaseEvent) WITHIN 1 HOUR "
                      "ON MATCH ALERT";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    ASSERT_TRUE(cfg->pattern.has_value());
    EXPECT_EQ(cfg->pattern->type, PatternType::SEQUENCE);
    EXPECT_EQ(cfg->pattern->event_types.size(), 2u);
    EXPECT_EQ(cfg->pattern->within, std::chrono::milliseconds(3600 * 1000));
}

TEST_F(RuleEngineTest, ParseEPLActionWebhook) {
    std::string epl = "SELECT * FROM events "
                      "ACTION webhook('https://api.example.com/alert', '{\"key\":\"val\"}')";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    ASSERT_FALSE(cfg->actions.empty());
    EXPECT_EQ(cfg->actions[0].type, ActionType::WEBHOOK);
    EXPECT_EQ(cfg->actions[0].target, "https://api.example.com/alert");
}

TEST_F(RuleEngineTest, ParseEPLActionDbWrite) {
    std::string epl = "SELECT * FROM events ACTION db_write('alerts_collection')";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    ASSERT_FALSE(cfg->actions.empty());
    EXPECT_EQ(cfg->actions[0].type, ActionType::DB_WRITE);
    EXPECT_EQ(cfg->actions[0].target, "alerts_collection");
}

TEST_F(RuleEngineTest, ParseEPLActionAlertWithParams) {
    std::string epl = "SELECT * FROM events "
                      "ACTION alert('security', 'critical', 'Suspicious activity detected')";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    ASSERT_FALSE(cfg->actions.empty());
    EXPECT_EQ(cfg->actions[0].type, ActionType::ALERT);
    EXPECT_EQ(cfg->actions[0].target, "security");
    EXPECT_EQ(cfg->tags.at("severity"), "critical");
    EXPECT_EQ(cfg->actions[0].template_str, "Suspicious activity detected");
}

TEST_F(RuleEngineTest, ParseEPLMultiLineCreateRule) {
    std::string epl =
        "CREATE RULE brute_force AS\n"
        "SELECT userId, COUNT(*) as attempts\n"
        "FROM AuthEvents\n"
        "WHERE success = false\n"
        "WINDOW TUMBLING(5 MINUTES)\n"
        "GROUP BY userId\n"
        "HAVING COUNT(*) >= 5\n"
        "ACTION alert('security', 'critical', 'Brute force detected');";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_EQ(cfg->rule_name, "brute_force");
    ASSERT_FALSE(cfg->streams.empty());
    EXPECT_EQ(cfg->streams[0], "AuthEvents");
    EXPECT_FALSE(cfg->filter.empty());
    ASSERT_TRUE(cfg->window.has_value());
    EXPECT_EQ(cfg->window->type, WindowType::TUMBLING);
    EXPECT_EQ(cfg->window->size, std::chrono::milliseconds(5 * 60 * 1000));
    ASSERT_EQ(cfg->group_by.size(), 1u);
    EXPECT_EQ(cfg->group_by[0], "userId");
    EXPECT_FALSE(cfg->having.empty());
    ASSERT_FALSE(cfg->actions.empty());
    EXPECT_EQ(cfg->actions[0].type, ActionType::ALERT);
    EXPECT_EQ(cfg->tags.at("severity"), "critical");
}

TEST_F(RuleEngineTest, ParseEPLHavingClause) {
    std::string epl = "SELECT COUNT(*) FROM events HAVING COUNT(*) > 10 ON MATCH ALERT";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_FALSE(cfg->having.empty());
}

TEST_F(RuleEngineTest, ParseEPLAllAggregationTypes) {
    std::string epl = "SELECT "
                      "MIN(price) as min_p, MAX(price) as max_p, "
                      "FIRST(ts) as first_ts, LAST(ts) as last_ts, "
                      "STDDEV(val) as sd, VARIANCE(val) as var, "
                      "DISTINCT_COUNT(userId) as dc, "
                      "PERCENTILE(latency, 99) as p99 "
                      "FROM events ON MATCH ALERT";
    auto cfg = RuleEngine::parseEPL(epl);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_EQ(cfg->aggregations.size(), 8u);

    std::map<std::string, AggregationType> expected = {
        {"min_p",   AggregationType::MIN},
        {"max_p",   AggregationType::MAX},
        {"first_ts",AggregationType::FIRST},
        {"last_ts", AggregationType::LAST},
        {"sd",      AggregationType::STDDEV},
        {"var",     AggregationType::VARIANCE},
        {"dc",      AggregationType::DISTINCT_COUNT},
        {"p99",     AggregationType::PERCENTILE},
    };
    for (const auto& agg : cfg->aggregations) {
        auto it = expected.find(agg.first);
        ASSERT_NE(it, expected.end()) << "Unexpected alias: " << agg.first;
        EXPECT_EQ(agg.second, it->second);
    }
}

TEST_F(RuleEngineTest, RuleStatsUpdated) {
    auto rule = makeSimpleRule("stats-rule");
    rule_engine_->addRule(rule);
    rule_engine_->processEvent(makeEvent("E1"));
    rule_engine_->processEvent(makeEvent("E2"));

    auto stats = rule_engine_->getRuleStats("stats-rule");
    EXPECT_EQ(stats.events_processed, 2u);
}

// ============================================================================
// CEPEngine integration tests
// ============================================================================

class CEPEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& engine = CEPEngine::getInstance();
        if (engine.isInitialized()) engine.shutdown();

        CEPConfig cfg;
        cfg.worker_threads  = 2;
        cfg.metrics_enabled = false;
        cfg.checkpointing_enabled = false;
        cfg.checkpoint_path = "/tmp/themis_cep_test_checkpoints";
        engine.initialize(cfg);
    }

    void TearDown() override {
        CEPEngine::getInstance().shutdown();
    }
};

TEST_F(CEPEngineTest, IsInitializedAfterInit) {
    EXPECT_TRUE(CEPEngine::getInstance().isInitialized());
}

TEST_F(CEPEngineTest, CreateAndGetStream) {
    StreamConfig sc;
    sc.stream_id   = "my-stream";
    sc.stream_name = "my-stream";
    sc.buffer_size = 1024;
    sc.partitions  = 2;
    auto stream = CEPEngine::getInstance().createStream(sc);
    ASSERT_NE(stream, nullptr);

    auto retrieved = CEPEngine::getInstance().getStream("my-stream");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getStreamId(), "my-stream");
}

TEST_F(CEPEngineTest, RemoveStream) {
    StreamConfig sc;
    sc.stream_id = "tmp-stream";
    sc.stream_name = "tmp-stream";
    CEPEngine::getInstance().createStream(sc);
    EXPECT_TRUE(CEPEngine::getInstance().removeStream("tmp-stream"));
    EXPECT_EQ(CEPEngine::getInstance().getStream("tmp-stream"), nullptr);
}

TEST_F(CEPEngineTest, GetStreamsReturnsAll) {
    StreamConfig sc1; sc1.stream_id = "s1"; sc1.stream_name = "s1";
    StreamConfig sc2; sc2.stream_id = "s2"; sc2.stream_name = "s2";
    CEPEngine::getInstance().createStream(sc1);
    CEPEngine::getInstance().createStream(sc2);
    // default stream is already there
    EXPECT_GE(CEPEngine::getInstance().getStreams().size(), 3u);
}

TEST_F(CEPEngineTest, SubmitEvent) {
    auto ev = makeEvent("test-submit", EventType::DOCUMENT_INSERT);
    bool ok = CEPEngine::getInstance().submitEvent(std::move(ev));
    EXPECT_TRUE(ok);
}

TEST_F(CEPEngineTest, AddAndGetRule) {
    RuleConfig rule;
    rule.rule_id   = "engine-rule-1";
    rule.rule_name = "engine-rule-1";
    rule.enabled   = true;
    ActionConfig ac; ac.type = ActionType::ALERT; rule.actions.push_back(ac);
    EXPECT_TRUE(CEPEngine::getInstance().addRule(rule));

    auto retrieved = CEPEngine::getInstance().getRule("engine-rule-1");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->rule_id, "engine-rule-1");
}

TEST_F(CEPEngineTest, RemoveRule) {
    RuleConfig rule;
    rule.rule_id = "to-remove"; rule.rule_name = "to-remove"; rule.enabled = true;
    CEPEngine::getInstance().addRule(rule);
    EXPECT_TRUE(CEPEngine::getInstance().removeRule("to-remove"));
    EXPECT_FALSE(CEPEngine::getInstance().getRule("to-remove").has_value());
}

TEST_F(CEPEngineTest, AddRuleFromEPL) {
    std::string epl = "SELECT * FROM events WHERE collection == logs "
                      "ON MATCH ALERT severity=info";
    EXPECT_TRUE(CEPEngine::getInstance().addRuleFromEPL(epl));
}

TEST_F(CEPEngineTest, AlertsGeneratedByRule) {
    RuleConfig rule;
    rule.rule_id   = "alert-gen";
    rule.rule_name = "alert-gen";
    rule.enabled   = true;
    ActionConfig ac; ac.type = ActionType::ALERT; ac.template_str = "fired";
    rule.actions.push_back(ac);
    CEPEngine::getInstance().addRule(rule);

    // Submit event and give workers time to process
    auto ev = makeEvent("trigger", EventType::CUSTOM);
    CEPEngine::getInstance().submitEvent(ev);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto alerts = CEPEngine::getInstance().getAlerts(10);
    EXPECT_FALSE(alerts.empty());
}

TEST_F(CEPEngineTest, AcknowledgeAlert) {
    RuleConfig rule;
    rule.rule_id = "ack-rule"; rule.rule_name = "ack-rule"; rule.enabled = true;
    ActionConfig ac; ac.type = ActionType::ALERT; rule.actions.push_back(ac);
    CEPEngine::getInstance().addRule(rule);

    CEPEngine::getInstance().submitEvent(makeEvent("E"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto alerts = CEPEngine::getInstance().getAlerts(1);
    if (!alerts.empty()) {
        EXPECT_TRUE(CEPEngine::getInstance().acknowledgeAlert(alerts[0].alert_id));
        auto updated = CEPEngine::getInstance().getAlerts(1, true); // unacked only
        EXPECT_TRUE(std::none_of(updated.begin(), updated.end(),
            [&](const Alert& a) { return a.alert_id == alerts[0].alert_id; }));
    }
}

TEST_F(CEPEngineTest, AlertCallback) {
    std::atomic<int> callback_count{0};
    CEPEngine::getInstance().setAlertCallback([&](const Alert&) {
        ++callback_count;
    });

    RuleConfig rule;
    rule.rule_id = "cb-rule"; rule.rule_name = "cb-rule"; rule.enabled = true;
    ActionConfig ac; ac.type = ActionType::ALERT; rule.actions.push_back(ac);
    CEPEngine::getInstance().addRule(rule);

    CEPEngine::getInstance().submitEvent(makeEvent("CB"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_GE(callback_count.load(), 1);
}

TEST_F(CEPEngineTest, GetStats) {
    auto s = CEPEngine::getInstance().getStats();
    EXPECT_GE(s.active_streams, 1u); // default stream
}

TEST_F(CEPEngineTest, PrometheusMetrics) {
    std::string prom = CEPEngine::getInstance().toPrometheusFormat();
    EXPECT_NE(prom.find("themisdb_cep_events_received_total"), std::string::npos);
    EXPECT_NE(prom.find("themisdb_cep_active_streams"),       std::string::npos);
    EXPECT_NE(prom.find("themisdb_cep_active_rules"),         std::string::npos);
}

TEST_F(CEPEngineTest, CreateCDCEvent) {
    auto ev = CEPEngine::createCDCEvent(
        EventType::DOCUMENT_INSERT,
        "inventory",
        "item-123",
        {{"quantity", int64_t(5)}, {"price", 9.99}});

    EXPECT_EQ(ev.type, EventType::DOCUMENT_INSERT);
    EXPECT_EQ(ev.collection_name, "inventory");
    EXPECT_EQ(ev.document_id, "item-123");
    EXPECT_TRUE(ev.getField<int64_t>("quantity").has_value());
}

TEST_F(CEPEngineTest, CheckpointLifecycle) {
    CEPConfig cfg;
    cfg.worker_threads  = 1;
    cfg.metrics_enabled = false;
    cfg.checkpointing_enabled = true;
    cfg.checkpoint_path = "/tmp/themis_cep_cp_test";
    std::filesystem::remove_all(cfg.checkpoint_path);

    CEPEngine::getInstance().shutdown();
    CEPEngine::getInstance().initialize(cfg);

    EXPECT_TRUE(CEPEngine::getInstance().createCheckpoint());
    auto cps = CEPEngine::getInstance().listCheckpoints();
    EXPECT_FALSE(cps.empty());

    EXPECT_TRUE(CEPEngine::getInstance().restoreFromCheckpoint(cps[0]));

    std::filesystem::remove_all(cfg.checkpoint_path);
}

TEST_F(CEPEngineTest, SequencePatternRuleViaEngine) {
    RuleConfig rule;
    rule.rule_id   = "seq-engine-rule";
    rule.rule_name = "seq-engine-rule";
    rule.enabled   = true;

    PatternConfig pc;
    pc.pattern_id  = "seq-p";
    pc.type        = PatternType::SEQUENCE;
    pc.event_types = {"LOGIN", "PURCHASE"};
    rule.pattern   = pc;

    ActionConfig ac; ac.type = ActionType::ALERT;
    ac.template_str = "User purchase after login";
    rule.actions.push_back(ac);

    CEPEngine::getInstance().addRule(rule);

    CEPEngine::getInstance().submitEvent(makeEvent("LOGIN"));
    CEPEngine::getInstance().submitEvent(makeEvent("PURCHASE"));
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    auto alerts = CEPEngine::getInstance().getAlerts(20);
    bool found = std::any_of(alerts.begin(), alerts.end(),
        [](const Alert& a) { return a.rule_id == "seq-engine-rule"; });
    EXPECT_TRUE(found);
}

// ============================================================================
// Engine-level backpressure tests
// ============================================================================

TEST(CEPEngineBackpressureTest, EngineDropsEventsWhenQueueFull) {
    auto& engine = CEPEngine::getInstance();
    if (engine.isInitialized()) engine.shutdown();

    CEPConfig cfg;
    cfg.worker_threads  = 0;        // No workers: queue fills up immediately
    cfg.metrics_enabled = false;
    cfg.checkpointing_enabled = false;
    cfg.backpressure_enabled  = true;
    cfg.max_queue_depth       = 4;
    cfg.global_backpressure_threshold = 0.5f;
    engine.initialize(cfg);

    // Fill up the queue beyond max_queue_depth
    bool accepted = false;
    for (int i = 0; i < 10; ++i) {
        bool ok = engine.submitEvent(makeEvent("flood-" + std::to_string(i)));
        if (!ok) { accepted = false; break; }
        accepted = true;
    }
    (void)accepted;

    auto stats = engine.getStats();
    EXPECT_GE(stats.events_dropped, 1u);
    EXPECT_GE(stats.backpressure_events, 1u);

    engine.shutdown();
}

TEST(CEPEngineBackpressureTest, BackpressureMetricInPrometheus) {
    auto& engine = CEPEngine::getInstance();
    if (engine.isInitialized()) engine.shutdown();

    CEPConfig cfg;
    cfg.worker_threads  = 0;
    cfg.metrics_enabled = false;
    cfg.checkpointing_enabled = false;
    cfg.backpressure_enabled  = true;
    cfg.max_queue_depth       = 2;
    cfg.global_backpressure_threshold = 0.5f;
    engine.initialize(cfg);

    for (int i = 0; i < 8; ++i) {
        engine.submitEvent(makeEvent("bp-" + std::to_string(i)));
    }

    std::string prom = engine.toPrometheusFormat();
    EXPECT_NE(prom.find("themisdb_cep_backpressure_events_total"), std::string::npos);

    engine.shutdown();
}

TEST(CEPEngineBackpressureTest, BackpressureDisabledStillUsesBoundedQueue) {
    auto& engine = CEPEngine::getInstance();
    if (engine.isInitialized()) engine.shutdown();

    CEPConfig cfg;
    cfg.worker_threads        = 0;
    cfg.metrics_enabled       = false;
    cfg.checkpointing_enabled = false;
    cfg.backpressure_enabled  = false;  // Disabled
    cfg.max_queue_depth       = 2;      // Would drop at 2 if enabled
    engine.initialize(cfg);

    size_t accepted = 0;
    size_t rejected = 0;
    for (int i = 0; i < 8; ++i) {
        if (engine.submitEvent(makeEvent("bounded-" + std::to_string(i)))) {
            ++accepted;
        } else {
            ++rejected;
        }
    }

    auto stats = engine.getStats();
    EXPECT_GT(accepted, 0u);
    EXPECT_GT(rejected, 0u);
    EXPECT_EQ(stats.events_dropped, rejected);
    EXPECT_EQ(stats.backpressure_events, 0u);

    engine.shutdown();
}

TEST(CEPEngineBackpressureTest, QueueDepthReflectsUnprocessedEvents) {
    auto& engine = CEPEngine::getInstance();
    if (engine.isInitialized()) engine.shutdown();

    CEPConfig cfg;
    cfg.worker_threads        = 0;   // No workers: events accumulate
    cfg.metrics_enabled       = false;
    cfg.checkpointing_enabled = false;
    cfg.backpressure_enabled  = true;
    cfg.max_queue_depth       = 64;
    engine.initialize(cfg);

    constexpr int N = 5;
    for (int i = 0; i < N; ++i) {
        engine.submitEvent(makeEvent("q-" + std::to_string(i)));
    }

    auto stats = engine.getStats();
    EXPECT_EQ(stats.queue_depth, static_cast<size_t>(N));

    // Queue depth should appear in Prometheus output
    std::string prom = engine.toPrometheusFormat();
    EXPECT_NE(prom.find("themisdb_cep_queue_depth"), std::string::npos);

    engine.shutdown();
}

// ============================================================================
// Stateful checkpoint tests
// ============================================================================

// Verifies that in-progress NFA partial match states are saved by
// createCheckpoint() and restored by restoreFromCheckpoint() so that
// a SEQUENCE pattern begun before the checkpoint can be completed after
// restore without missing the match.
TEST(CEPStatefulCheckpointTest, PartialMatchStateSurvivesCheckpointRestore) {
    const std::string cp_path = "/tmp/themis_cep_stateful_cp_test";
    std::filesystem::remove_all(cp_path);

    // ── Phase 1: submit event A, checkpoint, then restart ──────────────────
    {
        auto& engine = CEPEngine::getInstance();
        if (engine.isInitialized()) engine.shutdown();

        CEPConfig cfg;
        cfg.worker_threads        = 1;
        cfg.metrics_enabled       = false;
        cfg.checkpointing_enabled = true;
        cfg.checkpoint_path       = cp_path;
        engine.initialize(cfg);

        // Register SEQUENCE rule: A → B
        RuleConfig rule;
        rule.rule_id   = "stateful-seq";
        rule.rule_name = "stateful-seq";
        rule.enabled   = true;
        PatternConfig pc;
        pc.pattern_id  = "seq-ab";
        pc.type        = PatternType::SEQUENCE;
        pc.event_types = {"EVT_A", "EVT_B"};
        rule.pattern   = pc;
        ActionConfig ac; ac.type = ActionType::ALERT;
        rule.actions.push_back(ac);
        engine.addRule(rule);

        // Submit first event of the sequence (creates a partial match)
        engine.submitEvent(makeEvent("EVT_A"));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Checkpoint (should persist the partial match state)
        ASSERT_TRUE(engine.createCheckpoint());
        auto cps = engine.listCheckpoints();
        ASSERT_FALSE(cps.empty());

        engine.shutdown();

        // ── Phase 2: restart, restore checkpoint, submit event B ───────────
        if (engine.isInitialized()) engine.shutdown();
        engine.initialize(cfg);

        // Re-register the same rule (simulate fresh start)
        engine.addRule(rule);

        // Restore checkpoint → should reload the partial match state
        EXPECT_TRUE(engine.restoreFromCheckpoint(cps[0]));

        // Submit the second event of the sequence
        engine.submitEvent(makeEvent("EVT_B"));
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        // The partial match saved before restart should have completed
        auto alerts = engine.getAlerts(20);
        bool found = std::any_of(alerts.begin(), alerts.end(),
            [](const Alert& a) { return a.rule_id == "stateful-seq"; });
        EXPECT_TRUE(found) << "SEQUENCE pattern A→B should fire after checkpoint restore";

        engine.shutdown();
    }

    std::filesystem::remove_all(cp_path);
}

// Verifies that when no partial matches exist the checkpoint file contains no
// pm_rule blocks and restoreFromCheckpoint() succeeds cleanly.
TEST(CEPStatefulCheckpointTest, CheckpointWithNoPartialMatchesIsClean) {
    const std::string cp_path = "/tmp/themis_cep_empty_stateful_cp";
    std::filesystem::remove_all(cp_path);

    auto& engine = CEPEngine::getInstance();
    if (engine.isInitialized()) engine.shutdown();

    CEPConfig cfg;
    cfg.worker_threads        = 1;
    cfg.metrics_enabled       = false;
    cfg.checkpointing_enabled = true;
    cfg.checkpoint_path       = cp_path;
    engine.initialize(cfg);

    ASSERT_TRUE(engine.createCheckpoint());
    auto cps = engine.listCheckpoints();
    ASSERT_FALSE(cps.empty());

    // Verify checkpoint file contains no pm_rule= lines
    std::ifstream f(cp_path + "/" + cps[0] + ".txt");
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    f.close();
    EXPECT_EQ(content.find("pm_rule="), std::string::npos);

    EXPECT_TRUE(engine.restoreFromCheckpoint(cps[0]));

    engine.shutdown();
    std::filesystem::remove_all(cp_path);
}

// Regression test: CEPEngine::shutdown() must complete within 100 ms even when
// metrics_interval is set to a very long value.  This verifies that metricsLoop()
// uses condition_variable::wait_for (wakes on running_=false) instead of
// std::this_thread::sleep_for (which would block for the full interval).
TEST(CEPEngineShutdownTest, ShutdownReturnsWithin100msRegardlessOfMetricsInterval) {
    auto& engine = CEPEngine::getInstance();
    if (engine.isInitialized()) engine.shutdown();

    CEPConfig cfg;
    cfg.worker_threads        = 1;
    cfg.metrics_enabled       = true;
    cfg.metrics_interval      = std::chrono::milliseconds(60000); // 60 s - would stall old impl
    cfg.checkpointing_enabled = false;
    engine.initialize(cfg);

    auto t0 = std::chrono::steady_clock::now();
    engine.shutdown();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);

    EXPECT_LE(elapsed.count(), 100)
        << "CEPEngine::shutdown() took " << elapsed.count()
        << " ms – metricsLoop() must wake immediately on stop signal";
}

// Verifies that CEPEngine::shutdown() completes within 100 ms even when
// multiple worker threads are active and events are being submitted
// concurrently.  This is the Phase 4 stop-latency acceptance test
// (FUTURE_ENHANCEMENTS.md §2 / Production Readiness Checklist).
//
// Gated on THEMIS_RUN_PERF_TESTS=1 because it relies on wall-clock timing.
TEST(CEPEngineShutdownTest, StopLatencyWithActiveWorkers) {
#ifndef THEMIS_RUN_PERF_TESTS
    GTEST_SKIP() << "timing test – set THEMIS_RUN_PERF_TESTS=1 to run";
#endif

    auto& engine = CEPEngine::getInstance();
    if (engine.isInitialized()) engine.shutdown();

    CEPConfig cfg;
    cfg.worker_threads        = 4;
    cfg.metrics_enabled       = true;
    cfg.metrics_interval      = std::chrono::milliseconds(30000); // 30 s long interval
    cfg.checkpointing_enabled = false;
    engine.initialize(cfg);

    // Flood the engine with events from a background thread while we time
    // the shutdown – workers must drain and exit promptly.
    std::atomic<bool> keep_submitting{true};
    std::thread producer([&]() {
        while (keep_submitting.load(std::memory_order_relaxed)) {
            engine.submitEvent(makeEvent("LOAD"));
        }
    });

    // Let the producer run briefly so the worker queues are active.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    keep_submitting.store(false, std::memory_order_relaxed);
    producer.join();

    auto t0 = std::chrono::steady_clock::now();
    engine.shutdown();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);

    EXPECT_LE(elapsed.count(), 100)
        << "CEPEngine::shutdown() with 4 worker threads took " << elapsed.count()
        << " ms – workers must honour stop signal within 100 ms";
}
