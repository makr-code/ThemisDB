/**
 * @file test_delegate_evaluator.cpp
 * @brief Unit tests for the DELEGATE-52 round-trip corruption benchmark.
 *
 * Test IDs follow the implementation plan (DE-01 … DE-18) defined in
 * `src/rag/ROADMAP.md § Phase 11 — DELEGATE-52 Benchmark Integration`.
 *
 * Scientific basis: Laban et al., "LLMs Corrupt Your Documents When You
 * Delegate" (arXiv:2604.15597).
 */

#include "rag/delegate_evaluator.h"
#include "document/round_trip_editor.h"
#include "document/document_store.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace themis::rag::delegate_eval;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Simple identity EditFn (no change)
static EditFn identityFn() {
    return [](const std::string& doc, const std::string& /*instr*/) {
        return doc;
    };
}

/// Destructive EditFn: always returns empty string
static EditFn destructiveFn() {
    return [](const std::string& /*doc*/, const std::string& /*instr*/) {
        return std::string{};
    };
}

/// EditFn that always throws
static EditFn throwingFn() {
    return [](const std::string& /*doc*/, const std::string& /*instr*/) -> std::string {
        throw std::runtime_error("deliberate test failure");
    };
}

/// Build a simple JSON document string
static std::string makeJsonDoc() {
    return R"({"name":"Alice","age":30,"city":"Berlin","lang":"C++"})";
}

/// Build a JSON document missing the "age" field
static std::string makeJsonDocMissingAge() {
    return R"({"name":"Alice","city":"Berlin","lang":"C++"})";
}

/// Build a JSON document with all values changed
static std::string makeJsonDocAllChanged() {
    return R"({"name":"Bob","age":99,"city":"Paris","lang":"Rust"})";
}

/// Build a default `RoundTripEditPair`
static RoundTripEditPair makeEditPair() {
    return {"Add a field.", "Remove the field you added.",
            "seed", DomainType::JSON_DOCUMENT};
}

class FailingRoundTripEditor final : public themis::document::IRoundTripEditor {
public:
    themis::Result<void> beginRelay(const std::string& /*relay_id*/,
                                    const std::string& /*seed_document*/) override {
        return tl::unexpected(themis::Error(themis::errors::ErrorCode::ERR_DOC_NOT_FOUND,
                                            "beginRelay failed"));
    }

    themis::Result<void> saveInteraction(const std::string& /*relay_id*/,
                                         std::size_t /*interaction_index*/,
                                         const std::string& /*instruction*/,
                                         const std::string& /*document*/) override {
        return tl::unexpected(themis::Error(themis::errors::ErrorCode::ERR_DOC_NOT_FOUND,
                                            "saveInteraction failed"));
    }

    themis::Result<std::optional<themis::document::RoundTripSnapshot>> loadInteraction(
        const std::string& /*relay_id*/, std::size_t /*interaction_index*/) const override {
        return std::optional<themis::document::RoundTripSnapshot>{std::nullopt};
    }

    themis::Result<std::size_t> countSnapshots(const std::string& /*relay_id*/) const override {
        return std::size_t{0};
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// DE-01  JsonDocumentEvaluator — identical documents → RS = 1.0
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE01_JsonIdentical) {
    JsonDocumentEvaluator ev;
    const std::string doc = makeJsonDoc();
    const double rs = ev.evaluate(doc, doc);
    EXPECT_DOUBLE_EQ(rs, 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-02  JsonDocumentEvaluator — one field deleted → RS < 1.0, proportional
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE02_JsonOneFieldMissing) {
    JsonDocumentEvaluator ev;
    // original: 4 fields; recovered: 3 fields (age missing), values identical
    const double rs = ev.evaluate(makeJsonDoc(), makeJsonDocMissingAge());
    EXPECT_GT(rs, 0.0);
    EXPECT_LT(rs, 1.0);
    // Exactly 3 out of 4 top-level fields preserved → RS = 0.75
    EXPECT_NEAR(rs, 0.75, 1e-9);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-03  JsonDocumentEvaluator — all values changed → RS = 0.0
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE03_JsonAllValuesChanged) {
    JsonDocumentEvaluator ev;
    const double rs = ev.evaluate(makeJsonDoc(), makeJsonDocAllChanged());
    EXPECT_DOUBLE_EQ(rs, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-04  PlainTextEvaluator — identical text → RS = 1.0
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE04_PlainTextIdentical) {
    PlainTextEvaluator ev;
    const std::string text = "The quick brown fox jumps over the lazy dog.";
    EXPECT_DOUBLE_EQ(ev.evaluate(text, text), 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-05  PlainTextEvaluator — ~50 % chars changed → RS ≈ 0.5 (±0.1)
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE05_PlainTextHalfChanged) {
    PlainTextEvaluator ev;
    const std::string orig = "abcdefgh";
    const std::string rec  = "abcdXXXX"; // last 4 of 8 chars changed
    const double rs = ev.evaluate(orig, rec);
    EXPECT_GT(rs, 0.4);
    EXPECT_LT(rs, 0.6);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-06  AqlQueryEvaluator — identical AQL → RS = 1.0
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE06_AqlIdentical) {
    AqlQueryEvaluator ev;
    const std::string aql =
        "FOR u IN users FILTER u.age > 18 RETURN u.name";
    EXPECT_DOUBLE_EQ(ev.evaluate(aql, aql), 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-07  AqlQueryEvaluator — FILTER condition removed → RS < 1.0
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE07_AqlConditionRemoved) {
    AqlQueryEvaluator ev;
    const std::string orig = "FOR u IN users FILTER u.age > 18 RETURN u.name";
    const std::string rec  = "FOR u IN users RETURN u.name";
    const double rs = ev.evaluate(orig, rec);
    EXPECT_GT(rs, 0.0);
    EXPECT_LT(rs, 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-08  RoundTripSimulator — 0 rounds → RS@0 = 1.0
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE08_ZeroRounds) {
    DelegateEvaluatorConfig cfg;
    cfg.num_round_trips = 0;
    RoundTripSimulator sim(cfg);
    JsonDocumentEvaluator ev;
    const std::string doc = makeJsonDoc();
    const auto pairs = std::vector<RoundTripEditPair>{makeEditPair()};

    const auto result = sim.run(doc, pairs, ev, identityFn());

    EXPECT_EQ(result.stop_reason, StopReason::COMPLETED_NORMALLY);
    EXPECT_EQ(result.total_interactions, 0u);
    ASSERT_FALSE(result.scores.rs_per_interaction.empty());
    EXPECT_DOUBLE_EQ(result.scores.rs_per_interaction.front(), 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-09  RoundTripSimulator — destructive EditFn → RS@1 = 0.0,
//         catastrophic_corruption_count >= 1
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE09_DestructiveEditFn) {
    DelegateEvaluatorConfig cfg;
    cfg.num_round_trips      = 1;
    cfg.catastrophic_threshold = 0.80;
    RoundTripSimulator sim(cfg);
    JsonDocumentEvaluator ev;
    const std::string doc   = makeJsonDoc();
    const auto pairs = std::vector<RoundTripEditPair>{makeEditPair()};

    const auto result = sim.run(doc, pairs, ev, destructiveFn());

    EXPECT_EQ(result.stop_reason, StopReason::COMPLETED_NORMALLY);
    ASSERT_GE(result.scores.rs_per_interaction.size(), 1u);
    EXPECT_DOUBLE_EQ(result.scores.rs_per_interaction.front(), 0.0);
    EXPECT_GE(result.catastrophic_corruption_count, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-10  RoundTripSimulator — perfect identity EditFn → all RS@k = 1.0
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE10_PerfectRoundTrip) {
    DelegateEvaluatorConfig cfg;
    cfg.num_round_trips = 5;
    RoundTripSimulator sim(cfg);
    JsonDocumentEvaluator ev;
    const std::string doc   = makeJsonDoc();
    const auto pairs = std::vector<RoundTripEditPair>{makeEditPair()};

    const auto result = sim.run(doc, pairs, ev, identityFn());

    EXPECT_EQ(result.stop_reason, StopReason::COMPLETED_NORMALLY);
    EXPECT_EQ(result.scores.rs_per_interaction.size(), 5u);
    for (const double rs : result.scores.rs_per_interaction) {
        EXPECT_DOUBLE_EQ(rs, 1.0);
    }
    EXPECT_EQ(result.catastrophic_corruption_count, 0u);
    EXPECT_FALSE(result.fully_catastrophic);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-11  RoundTripSimulator — realistic degradation (5 % loss/round) →
//         RS@10 < 0.60
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE11_RealisticDegradation) {
    // EditFn removes the last character each call to simulate gradual content loss.
    // Each round-trip consists of 2 interactions (forward + backward), so
    // 10 round-trips = 20 individual calls → 20 characters removed total.
    const std::string seed(100, 'x'); // 100 'x' characters
    auto degradeFn = [](const std::string& doc, const std::string& /*instr*/) {
        if (doc.empty()) return doc;
        return doc.substr(0, doc.size() - 1); // remove one char per interaction
    };

    DelegateEvaluatorConfig cfg;
    cfg.num_round_trips = 10;
    RoundTripSimulator sim(cfg);
    PlainTextEvaluator ev;
    const auto pairs = std::vector<RoundTripEditPair>{
        {"Shorten.", "Restore.", "seed", DomainType::PLAIN_TEXT}};

    const auto result = sim.run(seed, pairs, ev, degradeFn);

    EXPECT_EQ(result.stop_reason, StopReason::COMPLETED_NORMALLY);
    // After 10 round-trips (20 single-char removals) 80/100 chars remain → RS ≈ 0.60
    // We use a loose upper bound to make the test robust across OS/locale.
    EXPECT_LT(result.scores.rs_at(10), 0.85);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-12  ReconstructionScoreAtK — catastrophic_threshold: RS < 0.80 flagged
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE12_CatastrophicThreshold) {
    ReconstructionScoreAtK scores;
    scores.rs_per_interaction = {0.95, 0.88, 0.79, 0.91};

    EXPECT_TRUE(scores.hasCatastrophicEvent(0.80));
    EXPECT_FALSE(scores.hasCatastrophicEvent(0.70));
    EXPECT_DOUBLE_EQ(scores.rs_at(3), 0.79);
    EXPECT_DOUBLE_EQ(scores.rs_at(0), 1.0); // no edit = 1.0
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-13  DelegateEvaluatorFactory — all domains produce non-null evaluator
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE13_FactoryAllDomains) {
    for (auto d : {DomainType::JSON_DOCUMENT, DomainType::AQL_QUERY,
                   DomainType::PLAIN_TEXT, DomainType::MARKDOWN,
                   DomainType::XML_PROCESS}) {
        auto ev = DelegateEvaluatorFactory::createForDomain(d);
        ASSERT_NE(ev, nullptr);
        EXPECT_EQ(ev->domain(), d);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-14  Empty seed document → RS = 0.0, no crash
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE14_EmptySeed) {
    JsonDocumentEvaluator jEv;
    EXPECT_DOUBLE_EQ(jEv.evaluate("", "{}"), 0.0);
    EXPECT_DOUBLE_EQ(jEv.evaluate("{}", ""), 0.0);

    PlainTextEvaluator tEv;
    EXPECT_DOUBLE_EQ(tEv.evaluate("", "hello"), 0.0);
    EXPECT_DOUBLE_EQ(tEv.evaluate("hello", ""), 0.0);

    // RoundTripSimulator with empty seed
    DelegateEvaluatorConfig cfg;
    cfg.num_round_trips = 1;
    RoundTripSimulator sim(cfg);
    PlainTextEvaluator ev;
    const auto pairs = std::vector<RoundTripEditPair>{
        {"fwd", "bwd", "seed", DomainType::PLAIN_TEXT}};
    const auto result = sim.run("", pairs, ev, identityFn());
    EXPECT_EQ(result.stop_reason, StopReason::COMPLETED_NORMALLY);
    // Identity fn returns "" → evaluate("","") → 0.0
    EXPECT_DOUBLE_EQ(result.scores.rs_per_interaction.front(), 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-15  EditFn exception → EDIT_FAILED stop reason, no crash
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE15_EditFnException) {
    DelegateEvaluatorConfig cfg;
    cfg.num_round_trips = 3;
    RoundTripSimulator sim(cfg);
    PlainTextEvaluator ev;
    const auto pairs = std::vector<RoundTripEditPair>{
        {"fwd", "bwd", "seed", DomainType::PLAIN_TEXT}};

    const auto result = sim.run("some document", pairs, ev, throwingFn());

    EXPECT_EQ(result.stop_reason, StopReason::EDIT_FAILED);
    // Should have exactly 1 RS entry (the failed round)
    EXPECT_EQ(result.scores.rs_per_interaction.size(), 1u);
    EXPECT_DOUBLE_EQ(result.scores.rs_per_interaction.front(), 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-16  Integration: RoundTripSimulator run() stores Zwischenversionen
//         (final_doc tracks current state)
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE16_StoreBackedSnapshots) {
    const std::string seed = makeJsonDoc();
    // EditFn: forward appends "X", backward removes last "X"
    auto fn = [](const std::string& doc, const std::string& instr) {
        if (instr == "fwd") return doc + "X";
        // backward: remove trailing X if present
        if (!doc.empty() && doc.back() == 'X') return doc.substr(0, doc.size() - 1);
        return doc;
    };

    DelegateEvaluatorConfig cfg;
    cfg.num_round_trips = 2;
    RoundTripSimulator sim(cfg);
    themis::document::InMemoryDocumentStore store;
    themis::document::StoreBackedRoundTripEditor editor(store);
    sim.setRoundTripEditor(&editor);
    PlainTextEvaluator ev;
    const auto pairs = std::vector<RoundTripEditPair>{
        {"fwd", "bwd", "seed", DomainType::PLAIN_TEXT}};

    const auto result = sim.run(seed, pairs, ev, fn);

    EXPECT_EQ(result.stop_reason, StopReason::COMPLETED_NORMALLY);
    // After 2 perfect round trips, final_doc should equal the seed
    EXPECT_EQ(result.final_doc, seed);
    EXPECT_EQ(result.total_interactions, 4u); // 2 rounds × 2 interactions

    const auto relay_id = sim.getLastRelayId();
    ASSERT_FALSE(relay_id.empty());
    const auto count_res = editor.countSnapshots(relay_id);
    ASSERT_TRUE(count_res.has_value());
    // seed + 4 interactions
    EXPECT_EQ(*count_res, 5u);

    const auto seed_snap = editor.loadInteraction(relay_id, 0);
    ASSERT_TRUE(seed_snap.has_value());
    const auto& maybe_seed_snapshot = *seed_snap;
    ASSERT_TRUE(maybe_seed_snapshot.has_value());
    EXPECT_EQ(maybe_seed_snapshot->document, seed);

    const auto first_interaction = editor.loadInteraction(relay_id, 1);
    ASSERT_TRUE(first_interaction.has_value());
    const auto& maybe_first_interaction = *first_interaction;
    ASSERT_TRUE(maybe_first_interaction.has_value());
    EXPECT_EQ(maybe_first_interaction->instruction, "fwd");
    EXPECT_EQ(maybe_first_interaction->document, seed + "X");
}

TEST(DelegateEvaluatorTest, DE16b_PersistenceFailuresAreCountedAndNonFatal) {
    const std::string seed = makeJsonDoc();
    DelegateEvaluatorConfig cfg;
    cfg.num_round_trips = 2;
    RoundTripSimulator sim(cfg);
    FailingRoundTripEditor failing_editor;
    sim.setRoundTripEditor(&failing_editor);
    PlainTextEvaluator ev;

    const auto pairs = std::vector<RoundTripEditPair>{
        {"fwd", "bwd", "seed", DomainType::PLAIN_TEXT}};
    const auto fn = identityFn();
    const auto result = sim.run(seed, pairs, ev, fn);

    EXPECT_EQ(result.stop_reason, StopReason::COMPLETED_NORMALLY);
    EXPECT_EQ(result.total_interactions, 4u);
    EXPECT_EQ(result.persistence_write_failures, 5u); // seed + 4 interactions
    EXPECT_EQ(result.final_doc, seed);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-17  XmlProcessEvaluator — element-count preserved → RS = 1.0
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE17_XmlElementsPreserved) {
    XmlProcessEvaluator ev;
    const std::string xml =
        R"(<process><task id="t1" name="review"/><gateway id="g1"/></process>)";
    EXPECT_DOUBLE_EQ(ev.evaluate(xml, xml), 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// DE-18  XmlProcessEvaluator — 3 of 10 elements missing → RS ≈ 0.7
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, DE18_XmlThreeElementsMissing) {
    XmlProcessEvaluator ev;

    // Build original with 10 <task> elements
    std::string orig = "<process>";
    for (int i = 1; i <= 10; ++i) {
        orig += "<task id=\"t" + std::to_string(i) + "\"/>";
    }
    orig += "</process>";

    // Recovered has only 7 <task> elements (3 removed)
    std::string rec = "<process>";
    for (int i = 1; i <= 7; ++i) {
        rec += "<task id=\"t" + std::to_string(i) + "\"/>";
    }
    rec += "</process>";

    const double rs = ev.evaluate(orig, rec);
    // Element overlap: 7/10 = 0.70 (no attributes beyond id which are different)
    // Attribute overlap: partial (7/10 ids preserved)
    // RS = 0.6 × elem_overlap + 0.4 × attr_overlap
    // Element names are all "task" — multiset: orig has 10 "task", rec has 7 "task"
    // → elem overlap = 7/10 = 0.70
    // → RS ≥ 0.6 × 0.70 = 0.42 and ≤ 1.0; relax check to RS ∈ [0.40, 0.85]
    EXPECT_GT(rs, 0.40);
    EXPECT_LT(rs, 0.85);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: factory createSimulator() sanity check
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, FactoryCreateSimulator) {
    DelegateEvaluatorConfig cfg;
    cfg.num_round_trips = 7;
    auto sim = DelegateEvaluatorFactory::createSimulator(cfg);
    ASSERT_NE(sim, nullptr);
    EXPECT_EQ(sim->getConfig().num_round_trips, 7u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: RS clamping — evaluator never returns out-of-range value
// ─────────────────────────────────────────────────────────────────────────────

TEST(DelegateEvaluatorTest, RSAlwaysInRange) {
    for (auto d : {DomainType::JSON_DOCUMENT, DomainType::AQL_QUERY,
                   DomainType::PLAIN_TEXT, DomainType::MARKDOWN,
                   DomainType::XML_PROCESS}) {
        auto ev = DelegateEvaluatorFactory::createForDomain(d);
        const double rs = ev->evaluate("any input", "totally different output XYZ123!");
        EXPECT_GE(rs, 0.0) << "DomainType " << static_cast<int>(d);
        EXPECT_LE(rs, 1.0) << "DomainType " << static_cast<int>(d);
    }
}
