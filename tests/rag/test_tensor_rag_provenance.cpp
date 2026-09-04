/**
 * @file test_tensor_rag_provenance.cpp
 * @brief Focused tests for TensorRAGPipeline provenance persistence integration.
 */

#include "rag/tensor_rag_pipeline.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using themis::rag::TensorRAGPipeline;
using themis::rag::TensorRAGPipelineConfig;
using themis::observability::IProvenanceStore;
using themis::observability::ProvenanceStepRecord;

class InMemoryProvenanceStore final : public IProvenanceStore {
public:
    bool storeRecord(const std::string& query_id,
                     int step_number,
                     const ProvenanceStepRecord& record) override {
        calls.emplace_back(query_id, step_number);
        records.push_back(record);
        return true;
    }

    std::optional<ProvenanceStepRecord> getRecord(const std::string& query_id,
                                                  int step_number) override {
        for (const auto& rec : records) {
            if (rec.query_id == query_id && rec.step_number == step_number) {
                return rec;
            }
        }
        return std::nullopt;
    }

    std::vector<ProvenanceStepRecord> getProvenanceChain(const std::string& query_id) override {
        std::vector<ProvenanceStepRecord> out = {};

        for (const auto& rec : records) {
            if (rec.query_id == query_id) {
                out.push_back(rec);
            }
        }
        return out;
    }

    std::vector<ProvenanceStepRecord> getRecordsByTimeRange(int64_t start_ts_ms,
                                                            int64_t end_ts_ms) override {
        std::vector<ProvenanceStepRecord> out = {};

        for (const auto& rec : records) {
            if (rec.timestamp_ms >= start_ts_ms && rec.timestamp_ms <= end_ts_ms) {
                out.push_back(rec);
            }
        }
        return out;
    }

    std::vector<std::string> listQueryIds() override {
        std::vector<std::string> out = {};

        for (const auto& rec : records) {
            if (std::find(out.begin(), out.end(), rec.query_id) == out.end()) {
                out.push_back(rec.query_id);
            }
        }
        return out;
    }

    bool deleteQuery(const std::string& query_id) override {
        records.erase(
            std::remove_if(records.begin(), records.end(),
                           [&query_id](const ProvenanceStepRecord& rec) {
                               return rec.query_id == query_id;
                           }),
            records.end());
        return true;
    }

    std::vector<std::pair<std::string, int>> calls;
    std::vector<ProvenanceStepRecord> records;
};

std::vector<float> uncertainLogits() {
    return {5.1f, 5.0f, 1.0f, 0.5f, 0.2f};
}

TEST(TensorRAGProvenanceIntegration, PersistsProvenanceStepWhenStoreConfigured) {
    auto store = std::make_shared<InMemoryProvenanceStore>();

    TensorRAGPipelineConfig cfg;
    cfg.session_id = "sess-provenance";
    cfg.use_flare = false;
    cfg.use_targ = true;
    cfg.targ_config.gap_threshold = 5.0f;
    cfg.targ_config.min_consecutive_uncertain = 1;
    cfg.provenance_store = store;

    TensorRAGPipeline pipeline(cfg);

    const auto d = pipeline.step("tok", -0.5f, uncertainLogits());
    ASSERT_TRUE(d.should_retrieve);

    ASSERT_EQ(store->calls.size(), 1u);
    EXPECT_EQ(store->calls[0].first, "sess-provenance");
    EXPECT_EQ(store->calls[0].second, 0);

    ASSERT_EQ(store->records.size(), 1u);
    EXPECT_EQ(store->records[0].query_id, "sess-provenance");
    EXPECT_EQ(store->records[0].step_number, 0);
    EXPECT_EQ(store->records[0].routing_reason_code, d.routing_reason_code);
}

TEST(TensorRAGProvenanceIntegration, ResetRestartsProvenanceStepSequence) {
    auto store = std::make_shared<InMemoryProvenanceStore>();

    TensorRAGPipelineConfig cfg;
    cfg.session_id = "sess-reset";
    cfg.use_flare = false;
    cfg.use_targ = true;
    cfg.targ_config.gap_threshold = 5.0f;
    cfg.targ_config.min_consecutive_uncertain = 1;
    cfg.provenance_store = store;

    TensorRAGPipeline pipeline(cfg);
    (void)pipeline.step("a", -0.5f, uncertainLogits());
    (void)pipeline.step("b", -0.5f, uncertainLogits());
    pipeline.reset();
    (void)pipeline.step("c", -0.5f, uncertainLogits());

    ASSERT_EQ(store->calls.size(), 3u);
    EXPECT_EQ(store->calls[0].second, 0);
    EXPECT_EQ(store->calls[1].second, 1);
    EXPECT_EQ(store->calls[2].second, 0);
}

}  // namespace
