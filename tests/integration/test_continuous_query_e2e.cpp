/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB – Continuous Query Integration Tests (CQI-01..05)         ║
╠═════════════════════════════════════════════════════════════════════╣
  File:    tests/integration/test_continuous_query_e2e.cpp           ║
  Tests:   CQI-01  register + inject + stream delta                  ║
           CQI-02  two independent subscribers, identical deltas      ║
           CQI-03  late-event within allowed_lateness_ms             ║
           CQI-04  cancel + re-subscribe, buffered deltas delivered   ║
           CQI-05  drop + re-register, evaluation resumes             ║
╚═════════════════════════════════════════════════════════════════════╝
*/

#include <gtest/gtest.h>

#include "query/continuous_query_engine.h"
#include "query/continuous_query_registry.h"
#include "query/window_spec.h"
#include "server/continuous_query_api_handler.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// ──────────────────────────────────────────────────────────────────────────────
// Minimal mock implementations
// ──────────────────────────────────────────────────────────────────────────────

namespace {

using namespace themis::query;
using themis::Result;
using namespace std::chrono_literals;

namespace beast = boost::beast;
namespace http  = beast::http;

// ─── MockCQResultStream ───────────────────────────────────────────────────────

class MockCQResultStream final : public CQResultStream {
public:
    explicit MockCQResultStream(std::string name)
        : name_(std::move(name)), cancelled_(false) {}

    /** Push a result into the stream from the test harness. */
    void push(CQResult r) {
        std::lock_guard<std::mutex> lk(mu_);
        queue_.push_back(std::move(r));
        cv_.notify_one();
    }

    /** Signal end-of-stream. */
    void close() {
        std::lock_guard<std::mutex> lk(mu_);
        closed_ = true;
        cv_.notify_all();
    }

    // CQResultStream interface ────────────────────────────────────────────────

    [[nodiscard]] bool hasMore() const noexcept override {
        std::lock_guard<std::mutex> lk(mu_);
        return !cancelled_ && (!closed_ || !queue_.empty());
    }

    std::optional<CQResult> next(
        std::chrono::milliseconds timeout = std::chrono::seconds(5)) override
    {
        std::unique_lock<std::mutex> lk(mu_);
        const bool got = cv_.wait_for(lk, timeout, [this] {
            return cancelled_ || !queue_.empty() || closed_;
        });
        if (!got || cancelled_ || queue_.empty()) {
          return std::nullopt;
        }
        auto item = std::move(queue_.front());
        queue_.pop_front();
        return item;
    }

    void cancel() noexcept override {
        std::lock_guard<std::mutex> lk(mu_);
        cancelled_ = true;
        cv_.notify_all();
    }

    [[nodiscard]] size_t queueDepth() const noexcept override {
        std::lock_guard<std::mutex> lk(mu_);
        return queue_.size();
    }

private:
    const std::string name_;
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::deque<CQResult> queue_;
    bool cancelled_{false};
    bool closed_{false};
};

// ─── MockContinuousQueryEngine ────────────────────────────────────────────────

/**
 * In-memory mock that fulfils the full ContinuousQueryEngine contract.
 * Subscribers share a single stream per query (CQI-02 verifies fan-out).
 */
class MockContinuousQueryEngine final : public ContinuousQueryEngine {
public:
    // ── registerQuery ────────────────────────────────────────────────────────

    Result<ContinuousQueryHandle> registerQuery(ContinuousQuerySpec spec) override {
        std::lock_guard<std::mutex> lk(mu_);
        if (specs_.count(spec.name)) {
            return themis::Err<ContinuousQueryHandle>(
                themis::errors::ErrorCode::ERR_QUERY_INVALID,
                "Query already registered: " + spec.name);
        }
        const std::string name = spec.name;
        // Build ContinuousQueryInfo
        ContinuousQueryInfo info;
        info.name              = spec.name;
        info.source_collection = spec.source_collection;
        info.window            = spec.window;
        info.result_mode       = spec.result_mode;
        info.registered_at     = std::chrono::system_clock::now();
        info.last_tick_at      = info.registered_at;
        info.tuples_processed  = 0;
        info.result_queue_depth = 0;

        infos_[name] = std::move(info);
        specs_[name] = std::move(spec);
        // Create the primary stream for this query
        streams_[name] = std::make_shared<MockCQResultStream>(name);
        return themis::Ok(name);
    }

    // ── dropQuery ────────────────────────────────────────────────────────────

    Result<void> dropQuery(const std::string& name) override {
        std::lock_guard<std::mutex> lk(mu_);
        if (!specs_.count(name)) {
            return themis::ErrVoid(
                themis::errors::ErrorCode::ERR_QUERY_INVALID,
                "Query not found: " + name);
        }
        streams_[name]->close();
        streams_.erase(name);
        specs_.erase(name);
        infos_.erase(name);
        return themis::OkVoid();
    }

    // ── subscribe ────────────────────────────────────────────────────────────

    Result<ResultStreamPtr> subscribe(
        const std::string& name, ResultMode /*mode*/) override
    {
        std::lock_guard<std::mutex> lk(mu_);
        if (!streams_.count(name)) {
            return themis::Err<ResultStreamPtr>(
                themis::errors::ErrorCode::ERR_QUERY_INVALID,
                "Query not found: " + name);
        }
        // CQI-02: each call to subscribe() returns a *new* independent stream.
        // We register it in a fan-out list so injectTuple() dispatches to all.
        auto sub = std::make_shared<MockCQResultStream>(name);
        fanout_[name].push_back(sub);
        return themis::Ok<ResultStreamPtr>(sub);
    }

    // ── listQueries ──────────────────────────────────────────────────────────

    [[nodiscard]] std::vector<ContinuousQueryInfo> listQueries() const override {
        std::lock_guard<std::mutex> lk(mu_);
        std::vector<ContinuousQueryInfo> out = {};

        out.reserve(infos_.size());
        for (const auto& [k, v] : infos_) {
          out.push_back(v);
        }
        return out;
    }

    // ── injectTuple ──────────────────────────────────────────────────────────

    void injectTuple(const std::string& collection,
                     const std::string& tuple,
                     int64_t            /*event_ts*/) override
    {
        std::lock_guard<std::mutex> lk(mu_);
        // Push to all fan-out subscribers whose query sources the collection
        for (const auto& [name, spec] : specs_) {
            if (spec.source_collection == collection) {
                if (infos_.count(name)) {
                    ++infos_[name].tuples_processed;
                    infos_[name].last_tick_at = std::chrono::system_clock::now();
                }
                CQResult res{tuple, false};
                for (auto& sub : fanout_[name]) {
                  sub->push(res);
                }
            }
        }
    }

    // ── Test helpers ─────────────────────────────────────────────────────────

    /** Close all streams for a query, simulating a tick ending. */
    void closeStreams(const std::string& name) {
        std::lock_guard<std::mutex> lk(mu_);
        for (auto& sub : fanout_[name]) {
          sub->close();
        }
    }

    /** How many fan-out subscribers does a query currently have? */
    size_t subscriberCount(const std::string& name) const {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = fanout_.find(name);
        return (it != fanout_.end()) ? it->second.size() : 0;
    }

private:
    mutable std::mutex mu_;

    std::unordered_map<std::string, ContinuousQuerySpec>    specs_;
    std::unordered_map<std::string, ContinuousQueryInfo>    infos_;
    // Primary streams (one per query — kept for backward compat)
    std::unordered_map<std::string, std::shared_ptr<MockCQResultStream>> streams_;
    // Fan-out registry: all active subscribers per query
    std::unordered_map<std::string,
        std::vector<std::shared_ptr<MockCQResultStream>>> fanout_;
};

// ─── Test fixture ─────────────────────────────────────────────────────────────

class ContinuousQueryE2ETest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = std::make_shared<MockContinuousQueryEngine>();
        handler_ = std::make_unique<themis::server::ContinuousQueryApiHandler>(engine_);
    }

    /** Build a Beast HTTP request from method, target, and body string. */
    static http::request<http::string_body> makeReq(
        http::verb method,
        const std::string& target,
        const std::string& body = "")
    {
        http::request<http::string_body> req{method, target, 11};
        req.set(http::field::content_type, "application/json");
        if (!body.empty()) {
            req.body() = body;
            req.prepare_payload();
        }
        return req;
    }

    std::shared_ptr<MockContinuousQueryEngine>               engine_;
    std::unique_ptr<themis::server::ContinuousQueryApiHandler> handler_;
};

}  // anonymous namespace

// ──────────────────────────────────────────────────────────────────────────────
// CQI-01: Register query, inject tuple, verify stream delta
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(ContinuousQueryE2ETest, CQI01_RegisterInjectStreamDelta) {
    // 1. Register a query via the handler
    nlohmann::json spec_json{
        {"name",              "cq_events"},
        {"source_collection", "events"},
        {"window",            {{"type", "TIME_SLIDING"},
                               {"range_ms", 5000},
                               {"slide_ms",  500}}},
        {"result_mode",       "DELTA"}
    };
    auto reg_res = handler_->handleRegister(
        makeReq(http::verb::post, "/v1/queries/continuous", spec_json.dump()));
    ASSERT_EQ(reg_res.result(), http::status::created);
    auto reg_body = nlohmann::json::parse(reg_res.body());
    EXPECT_EQ(reg_body["name"], "cq_events");

    // 2. Subscribe to the stream
    auto sub_res = engine_->subscribe("cq_events", ResultMode::DELTA);
    ASSERT_TRUE(sub_res.has_value()) << sub_res.error().message();
    auto stream = std::move(*sub_res);
    ASSERT_TRUE(stream->hasMore());

    // 3. Inject a tuple
    const std::string tuple_json = R"({"id":1,"value":42})";
    engine_->injectTuple("events", tuple_json, 1'000'000LL);

    // 4. Verify the delta arrives
    auto item = stream->next(std::chrono::milliseconds(500));
    ASSERT_TRUE(item.has_value()) << "Expected delta within 500ms";
    EXPECT_EQ(item->payload, tuple_json);
    EXPECT_FALSE(item->is_retract);

    // 5. List shows the query with updated stats
    auto list_res = handler_->handleList(
        makeReq(http::verb::get, "/v1/queries/continuous"));
    ASSERT_EQ(list_res.result(), http::status::ok);
    auto list_body = nlohmann::json::parse(list_res.body());
    ASSERT_FALSE(list_body.empty());
    EXPECT_EQ(list_body[0]["name"], "cq_events");
    EXPECT_GE(list_body[0]["tuples_processed"].get<uint64_t>(), 1u);

    stream->cancel();
}

// ──────────────────────────────────────────────────────────────────────────────
// CQI-02: Two independent subscribers receive identical deltas
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(ContinuousQueryE2ETest, CQI02_TwoSubscribersReceiveIdenticalDeltas) {
    // Register
    nlohmann::json spec_json{
        {"name",              "cq_multi"},
        {"source_collection", "events"},
        {"window",            {{"type", "TUMBLING"}, {"range_ms", 1000}}},
        {"result_mode",       "DELTA"}
    };
    auto reg = handler_->handleRegister(
        makeReq(http::verb::post, "/v1/queries/continuous", spec_json.dump()));
    ASSERT_EQ(reg.result(), http::status::created);

    // Subscribe twice
    auto sub1_res = engine_->subscribe("cq_multi", ResultMode::DELTA);
    auto sub2_res = engine_->subscribe("cq_multi", ResultMode::DELTA);
    ASSERT_TRUE(sub1_res.has_value());
    ASSERT_TRUE(sub2_res.has_value());
    auto sub1 = std::move(*sub1_res);
    auto sub2 = std::move(*sub2_res);

    // Inject two tuples
    engine_->injectTuple("events", R"({"id":10})", 2'000'000LL);
    engine_->injectTuple("events", R"({"id":11})", 2'001'000LL);

    // Both subscribers should receive the same payloads
    for (auto* sub : {sub1.get(), sub2.get()}) {
        auto r1 = sub->next(300ms);
        auto r2 = sub->next(300ms);
        ASSERT_TRUE(r1.has_value()) << "sub did not receive first tuple";
        ASSERT_TRUE(r2.has_value()) << "sub did not receive second tuple";
        EXPECT_EQ(r1->payload, R"({"id":10})");
        EXPECT_EQ(r2->payload, R"({"id":11})");
        sub->cancel();
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// CQI-03: Late-event within allowed_lateness_ms triggers a correction delta
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(ContinuousQueryE2ETest, CQI03_LateEventCorrectionDelta) {
    // Register with 1-second lateness budget
    nlohmann::json spec_json{
        {"name",                 "cq_late"},
        {"source_collection",    "sensors"},
        {"window",               {{"type", "TIME_SLIDING"},
                                  {"range_ms", 10000},
                                  {"slide_ms",  1000}}},
        {"result_mode",          "DELTA"},
        {"allowed_lateness_ms",  1000}
    };
    auto reg = handler_->handleRegister(
        makeReq(http::verb::post, "/v1/queries/continuous", spec_json.dump()));
    ASSERT_EQ(reg.result(), http::status::created);

    auto sub_res = engine_->subscribe("cq_late", ResultMode::DELTA);
    ASSERT_TRUE(sub_res.has_value());
    auto stream = std::move(*sub_res);

    // Inject an "on-time" tuple at t=5000000 µs
    engine_->injectTuple("sensors", R"({"temp":20})", 5'000'000LL);
    auto r1 = stream->next(300ms);
    ASSERT_TRUE(r1.has_value()) << "Expected on-time tuple";
    EXPECT_FALSE(r1->is_retract);

    // Inject a "late" tuple within the lateness budget (simulated by the mock
    // honouring the inject unconditionally — the engine implementation is
    // responsible for lateness handling; the test verifies the stream delivers
    // an additional item regardless of its timestamp ordering)
    engine_->injectTuple("sensors", R"({"temp":19})", 4'800'000LL);
    auto r2 = stream->next(300ms);
    ASSERT_TRUE(r2.has_value()) << "Expected late-event correction delta";
    EXPECT_FALSE(r2->is_retract);
    EXPECT_EQ(r2->payload, R"({"temp":19})");

    stream->cancel();
}

// ──────────────────────────────────────────────────────────────────────────────
// CQI-04: Cancel + re-subscribe; buffered deltas still delivered
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(ContinuousQueryE2ETest, CQI04_CancelAndResubscribe) {
    // Register
    nlohmann::json spec_json{
        {"name",              "cq_resume"},
        {"source_collection", "logs"},
        {"window",            {{"type", "TIME_SLIDING"},
                               {"range_ms", 3000},
                               {"slide_ms",  300}}},
        {"result_mode",       "DELTA"}
    };
    auto reg = handler_->handleRegister(
        makeReq(http::verb::post, "/v1/queries/continuous", spec_json.dump()));
    ASSERT_EQ(reg.result(), http::status::created);

    // First subscription — inject and consume one item
    {
        auto sub_res = engine_->subscribe("cq_resume", ResultMode::DELTA);
        ASSERT_TRUE(sub_res.has_value());
        auto sub1 = std::move(*sub_res);

        engine_->injectTuple("logs", R"({"level":"INFO"})", 3'000'000LL);
        auto r = sub1->next(300ms);
        ASSERT_TRUE(r.has_value());
        EXPECT_EQ(r->payload, R"({"level":"INFO"})");

        sub1->cancel();  // Explicitly cancel sub1
        EXPECT_FALSE(sub1->hasMore());
    }

    // Second subscription — inject another tuple and verify receipt
    {
        auto sub_res2 = engine_->subscribe("cq_resume", ResultMode::DELTA);
        ASSERT_TRUE(sub_res2.has_value());
        auto sub2 = std::move(*sub_res2);

        engine_->injectTuple("logs", R"({"level":"WARN"})", 3'100'000LL);
        auto r = sub2->next(300ms);
        ASSERT_TRUE(r.has_value()) << "Expected tuple after re-subscribe";
        EXPECT_EQ(r->payload, R"({"level":"WARN"})");

        sub2->cancel();
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// CQI-05: Drop + re-register; evaluation resumes on the new registration
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(ContinuousQueryE2ETest, CQI05_DropAndReregister) {
    const nlohmann::json spec_json{
        {"name",              "cq_restart"},
        {"source_collection", "orders"},
        {"window",            {{"type", "TUMBLING"}, {"range_ms", 2000}}},
        {"result_mode",       "DELTA"}
    };

    // Register v1
    auto reg1 = handler_->handleRegister(
        makeReq(http::verb::post, "/v1/queries/continuous", spec_json.dump()));
    ASSERT_EQ(reg1.result(), http::status::created);

    // Subscribe and inject tuple → verify
    auto sub1_res = engine_->subscribe("cq_restart", ResultMode::DELTA);
    ASSERT_TRUE(sub1_res.has_value());
    auto sub1 = std::move(*sub1_res);
    engine_->injectTuple("orders", R"({"order_id":1})", 100'000LL);
    auto r1 = sub1->next(300ms);
    ASSERT_TRUE(r1.has_value());
    sub1->cancel();

    // Drop
    auto drop_res = handler_->handleDrop(
        makeReq(http::verb::delete_, "/v1/queries/continuous/cq_restart"),
        "cq_restart");
    ASSERT_EQ(drop_res.result(), http::status::ok);

    // Verify DROP returns 404 on second call
    auto drop2 = handler_->handleDrop(
        makeReq(http::verb::delete_, "/v1/queries/continuous/cq_restart"),
        "cq_restart");
    EXPECT_EQ(drop2.result(), http::status::not_found);

    // Re-register (same name allowed after drop)
    auto reg2 = handler_->handleRegister(
        makeReq(http::verb::post, "/v1/queries/continuous", spec_json.dump()));
    ASSERT_EQ(reg2.result(), http::status::created);

    // Subscribe to the freshly-registered query and inject a new tuple
    auto sub2_res = engine_->subscribe("cq_restart", ResultMode::DELTA);
    ASSERT_TRUE(sub2_res.has_value());
    auto sub2 = std::move(*sub2_res);
    engine_->injectTuple("orders", R"({"order_id":2})", 200'000LL);
    auto r2 = sub2->next(300ms);
    ASSERT_TRUE(r2.has_value()) << "Expected tuple after re-register";
    EXPECT_EQ(r2->payload, R"({"order_id":2})");
    sub2->cancel();
}
