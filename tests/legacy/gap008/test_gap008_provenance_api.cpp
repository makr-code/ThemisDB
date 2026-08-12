#include <gtest/gtest.h>

#include "observability/provenance_store.h"
#include "server/monitoring_api_handler.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis { namespace observability { namespace test { 

namespace {

class InMemoryProvenanceStore final : public IProvenanceStore {
public:
    bool storeRecord(const std::string& query_id,
                     int step_number,
                     const ProvenanceStepRecord& record) override {
        records_.push_back(record);
        std::sort(records_.begin(), records_.end(),
                  [](const ProvenanceStepRecord& lhs, const ProvenanceStepRecord& rhs) {
                      if (lhs.query_id != rhs.query_id) {
                          return lhs.query_id < rhs.query_id;
                      }
                      return lhs.step_number < rhs.step_number;
                  });
        (void)query_id;
        (void)step_number;
        return true;
    }

    std::optional<ProvenanceStepRecord> getRecord(const std::string& query_id,
                                                  int step_number) override {
        for (const auto& rec : records_) {
            if (rec.query_id == query_id && rec.step_number == step_number) {
                return rec;
            }
        }
        return std::nullopt;
    }

    std::vector<ProvenanceStepRecord> getProvenanceChain(const std::string& query_id) override {
        std::vector<ProvenanceStepRecord> out;
        for (const auto& rec : records_) {
            if (rec.query_id == query_id) {
                out.push_back(rec);
            }
        }
        std::sort(out.begin(), out.end(),
                  [](const ProvenanceStepRecord& lhs, const ProvenanceStepRecord& rhs) {
                      return lhs.step_number < rhs.step_number;
                  });
        return out;
    }

    std::vector<ProvenanceStepRecord> getRecordsByTimeRange(int64_t start_ts_ms,
                                                            int64_t end_ts_ms) override {
        std::vector<ProvenanceStepRecord> out;
        for (const auto& rec : records_) {
            if (rec.timestamp_ms >= start_ts_ms && rec.timestamp_ms <= end_ts_ms) {
                out.push_back(rec);
            }
        }
        std::sort(out.begin(), out.end(),
                  [](const ProvenanceStepRecord& lhs, const ProvenanceStepRecord& rhs) {
                      return lhs.timestamp_ms < rhs.timestamp_ms;
                  });
        return out;
    }

    std::vector<std::string> listQueryIds() override {
        std::vector<std::string> ids;
        for (const auto& rec : records_) {
            if (std::find(ids.begin(), ids.end(), rec.query_id) == ids.end()) {
                ids.push_back(rec.query_id);
            }
        }
        return ids;
    }

    bool deleteQuery(const std::string& query_id) override {
        records_.erase(
            std::remove_if(records_.begin(), records_.end(),
                           [&](const ProvenanceStepRecord& rec) { return rec.query_id == query_id; }),
            records_.end());
        return true;
    }

private:
    std::vector<ProvenanceStepRecord> records_;
};

ProvenanceStepRecord makeRecord(const std::string& query_id,
                                int step_number,
                                int64_t timestamp_ms) {
    ProvenanceStepRecord rec;
    rec.query_id = query_id;
    rec.step_number = step_number;
    rec.correlation_id = query_id + "-corr";
    rec.timestamp_ms = timestamp_ms;
    rec.layer_name = "TensorMidLayer";
    rec.source_layer = "AnnFrontdoor";
    rec.num_candidates = 8;
    rec.num_selected = 3;
    rec.input_vector_hash = "h";
    rec.backend_name = "TensorRAGPipeline";
    rec.routing_reason_code = "kTest";
    rec.fallback_mode = "NONE";
    rec.confidence_policy_version = "1.0";
    rec.decision_duration_us = 42;
    return rec;
}

class ObservabilityProvenanceApiTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto store = std::make_shared<InMemoryProvenanceStore>();
        ASSERT_TRUE(store->storeRecord("q1", 0, makeRecord("q1", 0, 1000)));
        ASSERT_TRUE(store->storeRecord("q1", 1, makeRecord("q1", 1, 2000)));
        ASSERT_TRUE(store->storeRecord("q2", 0, makeRecord("q2", 0, 1500)));

        store_ = std::move(store);

        handler_ = std::make_unique<server::MonitoringApiHandler>(
            nullptr,
            nullptr,
            &request_count_,
            &error_count_,
            &start_time_,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr);
        handler_->setProvenanceStore(store_);
    }

    static nlohmann::json parseJsonResponse(const server::http::response<server::http::string_body>& resp) {
        return nlohmann::json::parse(resp.body());
    }

    std::atomic<uint64_t> request_count_{0};
    std::atomic<uint64_t> error_count_{0};
    std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()};
    std::shared_ptr<InMemoryProvenanceStore> store_;
    std::unique_ptr<server::MonitoringApiHandler> handler_;
};

TEST_F(ObservabilityProvenanceApiTest, ReturnsQueryChainForQueryId) {
    server::http::request<server::http::string_body> req{
        server::http::verb::get,
        "/api/v1/observability/provenance?query_id=q1",
        11};

    const auto resp = handler_->handleObservabilityProvenance(req);
    ASSERT_EQ(resp.result(), server::http::status::ok);

    const auto body = parseJsonResponse(resp);
    ASSERT_TRUE(body.contains("records"));
    ASSERT_EQ(body["source"], "query_chain");
    ASSERT_EQ(body["count"], 2);
    ASSERT_EQ(body["records"][0]["query_id"], "q1");
    ASSERT_EQ(body["records"][1]["query_id"], "q1");
}

TEST_F(ObservabilityProvenanceApiTest, SupportsTimeRangeFilter) {
    server::http::request<server::http::string_body> req{
        server::http::verb::get,
        "/api/v1/observability/provenance?start_ts_ms=1200&end_ts_ms=1800",
        11};

    const auto resp = handler_->handleObservabilityProvenance(req);
    ASSERT_EQ(resp.result(), server::http::status::ok);

    const auto body = parseJsonResponse(resp);
    ASSERT_EQ(body["source"], "time_range");
    ASSERT_EQ(body["count"], 1);
    ASSERT_EQ(body["records"][0]["query_id"], "q2");
    ASSERT_EQ(body["records"][0]["timestamp_ms"], 1500);
}

TEST_F(ObservabilityProvenanceApiTest, RejectsInvalidTimeRangeQuery) {
    server::http::request<server::http::string_body> req{
        server::http::verb::get,
        "/api/v1/observability/provenance?start_ts_ms=2000",
        11};

    const auto resp = handler_->handleObservabilityProvenance(req);
    ASSERT_EQ(resp.result(), server::http::status::bad_request);
}

TEST(ObservabilityProvenanceApiStandaloneTest, ReturnsServiceUnavailableWithoutStore) {
    std::atomic<uint64_t> request_count{0};
    std::atomic<uint64_t> error_count{0};
    const auto start_time = std::chrono::steady_clock::now();

    server::MonitoringApiHandler handler(
        nullptr,
        nullptr,
        &request_count,
        &error_count,
        &start_time,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    server::http::request<server::http::string_body> req{
        server::http::verb::get,
        "/api/v1/observability/provenance",
        11};
    const auto resp = handler.handleObservabilityProvenance(req);
    ASSERT_EQ(resp.result(), server::http::status::service_unavailable);
}

} // namespace
} } } // namespace themis::observability::test
