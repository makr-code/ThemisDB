#include "../test_fixture.h"

#include <filesystem>
#include <fstream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis { namespace test { 

namespace {

struct AnalyticsResult {
    bool from_cache{false};
    int aggregate{0};
};

class AnalyticsExportPipeline {
public:
    explicit AnalyticsExportPipeline(std::shared_ptr<PipelineAuditLog> audit)
        : audit_(std::move(audit)) {}

    [[nodiscard]] AnalyticsResult RunOlapQuery(const std::string& query,
                                               const std::vector<int>& values) {
        const auto it = query_cache_.find(query);
        if (it != query_cache_.end()) {
            audit_->Record({"cache", "hit", query});
            return {true, it->second};
        }

        const int sum = std::accumulate(values.begin(), values.end(), 0);
        ++execution_count_;
        query_cache_[query] = sum;
        audit_->Record({"analytics", "executed", query});
        return {false, sum};
    }

    [[nodiscard]] bool ExportArrow(int aggregate, const std::filesystem::path& output_file) {
        std::ofstream out(output_file);
        if (!out.is_open()) {
            return false;
        }
        out << "arrow_sum=" << aggregate << "\n";
        return true;
    }

    [[nodiscard]] bool RunStreamingWindowAndExport(const std::vector<int>& values,
                                                   size_t window_size,
                                                   int alert_threshold,
                                                   const std::filesystem::path& output_file) {
        if (window_size == 0 || values.size() < window_size) {
            return false;
        }

        bool alert_triggered = false;
        std::ofstream out(output_file);
        if (!out.is_open()) {
            return false;
        }

        for (size_t i = 0; i + window_size <= values.size(); ++i) {
            int window_sum = 0;
            for (size_t j = 0; j < window_size; ++j) {
                window_sum += values[i + j];
            }
            out << "window_" << i << "=" << window_sum << "\n";
            if (window_sum > alert_threshold) {
                alert_triggered = true;
            }
        }

        if (alert_triggered) {
            alerts_.push_back("threshold_exceeded");
            audit_->Record({"observability", "alert", "threshold_exceeded"});
        }

        return true;
    }

    [[nodiscard]] bool ExportParquetEncrypted(int aggregate,
                                              const std::filesystem::path& output_file,
                                              const std::string& key_id) {
        std::ofstream out(output_file);
        if (!out.is_open()) {
            return false;
        }
        out << "enc(" << key_id << "):" << aggregate << "\n";
        audit_->Record({"export", "parquet_encrypted", key_id});
        return true;
    }

    [[nodiscard]] size_t ExecutionCount() const { return execution_count_; }
    [[nodiscard]] size_t AlertCount() const { return alerts_.size(); }

private:
    std::shared_ptr<PipelineAuditLog> audit_;
    std::unordered_map<std::string, int> query_cache_;
    std::vector<std::string> alerts_;
    size_t execution_count_{0};
};

} // namespace

class AnalyticsExportPipelineTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        audit_ = CreateAuditLog();
        pipeline_ = std::make_unique<AnalyticsExportPipeline>(audit_);
    }

    std::shared_ptr<PipelineAuditLog> audit_;
    std::unique_ptr<AnalyticsExportPipeline> pipeline_;
};

TEST_F(AnalyticsExportPipelineTest, AEP01_OlapAggregationArrowExportAndMetrics) {
    const auto result = pipeline_->RunOlapQuery("SELECT SUM(v)", {1, 2, 3, 4});
    ASSERT_FALSE(result.from_cache);
    ASSERT_EQ(result.aggregate, 10);

    const auto out_file = GetTempDir() / "aep01.arrow.txt";
    ASSERT_TRUE(pipeline_->ExportArrow(result.aggregate, out_file));
    EXPECT_TRUE(std::filesystem::exists(out_file));
    EXPECT_EQ(pipeline_->ExecutionCount(), 1U);
}

TEST_F(AnalyticsExportPipelineTest, AEP02_StreamingAggregationExportsAndTriggersAlert) {
    const auto out_file = GetTempDir() / "aep02.stream.txt";
    ASSERT_TRUE(pipeline_->RunStreamingWindowAndExport({5, 7, 9, 1}, 2, 10, out_file));

    EXPECT_TRUE(std::filesystem::exists(out_file));
    EXPECT_EQ(pipeline_->AlertCount(), 1U);
    EXPECT_TRUE(audit_->Contains("observability", "alert"));
}

TEST_F(AnalyticsExportPipelineTest, AEP03_ParquetExportUsesSecurityKeyContext) {
    const auto out_file = GetTempDir() / "aep03.parquet.txt";
    ASSERT_TRUE(pipeline_->ExportParquetEncrypted(42, out_file, "kms-key-1"));

    std::ifstream in(out_file);
    std::string line;
    std::getline(in, line);

    EXPECT_NE(line.find("enc(kms-key-1):42"), std::string::npos);
    EXPECT_TRUE(audit_->Contains("export", "parquet_encrypted"));
}
} } // namespace themis::test
