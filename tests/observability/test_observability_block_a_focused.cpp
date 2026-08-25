// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "observability/alerting_engine.h"
#include "observability/alertmanager.h"
#include "observability/continuous_profiler.h"
#include "observability/distributed_flame_graph.h"
#include "observability/root_cause_analyzer.h"

using namespace themis::observability;
using namespace std::chrono_literals;

namespace {

ProfileSnapshot makeCpuSnapshot(const std::string& folded) {
    ProfileSnapshot snapshot;
    snapshot.type = ProfileType::CPU;
    snapshot.data = std::vector<uint8_t>(folded.size());
    for (size_t i = 0; i < folded.size(); ++i) {
        snapshot.data[i] = static_cast<uint8_t>(folded[i]);
    }
    return snapshot;
}

class RecordingChannel : public INotificationChannel {
public:
    explicit RecordingChannel(bool fail = false) : fail_(fail) {}

    [[nodiscard]] std::string channelType() const override { return "recording"; }

    [[nodiscard]] themis::Result<void> send(const Alert& alert) override {
        alerts.push_back(alert);
        if (fail_) {
            return tl::unexpected(themis::Error{
                themis::errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
                "channel failure"
            });
        }
        return {};
    }

    std::vector<Alert> alerts;

private:
    bool fail_;
};

class RecordingAlertmanager : public Alertmanager {
public:
    themis::Result<void> sendAlert(const Alert& alert) override {
        sent.push_back(alert);
        upsertActiveAlert(alert);
        return {};
    }

    themis::Result<void> resolveAlert(const std::string& alert_id) override {
        resolved.push_back(alert_id);
        removeActiveAlertById(alert_id);
        return {};
    }

    std::vector<Alert> sent;
    std::vector<std::string> resolved;
};

} // namespace

TEST(ObservabilityBlockAFlameGraph, MergeUsesLatestVersionPerNodeAndTracksVersions) {
    DistributedFlameGraph graph;
    graph.addNodeProfile({"node-a", "host-a", 2, makeCpuSnapshot("main;a 2\n")});
    graph.addNodeProfile({"node-a", "host-a", 1, makeCpuSnapshot("main;a 99\n")});
    graph.addNodeProfile({"node-b", "host-b", 5, makeCpuSnapshot("main;b 3\n")});

    const auto merged = graph.merge();
    ASSERT_EQ(merged.node_versions.at("node-a"), 2u);
    ASSERT_EQ(merged.node_versions.at("node-b"), 5u);
    EXPECT_EQ(merged.stacks.at("main;a"), 2u);
    EXPECT_EQ(merged.stacks.at("main;b"), 3u);
}

TEST(ObservabilityBlockAFlameGraph, FoldedAndJsonOutputAreDeterministic) {
    DistributedFlameGraph graph;
    graph.addNodeProfile({"node-b", "host-b", 1, makeCpuSnapshot("main;z 1\n")});
    graph.addNodeProfile({"node-a", "host-a", 1, makeCpuSnapshot("main;a 2\n")});

    const auto merged = graph.merge();
    EXPECT_EQ(merged.toFoldedText(), std::string("main;a 2\nmain;z 1\n"));
    const auto as_json = merged.toJSON();
    ASSERT_TRUE(as_json.contains("node_versions"));
    EXPECT_EQ(as_json["node_versions"]["node-a"], 1u);
}

TEST(ObservabilityBlockAAlerting, EngineReturnsChannelFailureExplicitly) {
    AlertingEngine engine;
    engine.addChannel(std::make_shared<RecordingChannel>(true));

    Alert alert;
    alert.alert_id = "a1";
    alert.alert_name = "Test Alert";
    alert.message = "failure";

    const auto result = engine.sendAlert(alert);
    ASSERT_FALSE(result.has_value());
}

TEST(ObservabilityBlockAAlerting, EngineDispatchesAndResolvesBackendState) {
    auto backend = std::make_shared<RecordingAlertmanager>();
    AlertingEngine engine(backend);
    auto channel = std::make_shared<RecordingChannel>();
    engine.addChannel(channel);

    Alert alert;
    alert.alert_id = "a2";
    alert.alert_name = "Backend Alert";
    alert.message = "ok";

    ASSERT_TRUE(engine.sendAlert(alert).has_value());
    ASSERT_EQ(channel->alerts.size(), 1u);
    ASSERT_EQ(backend->sent.size(), 1u);

    ASSERT_TRUE(engine.resolveAlert("a2").has_value());
    ASSERT_EQ(backend->resolved.size(), 1u);
}

TEST(ObservabilityBlockARootCause, ReportIncludesReasonCodeAndFallbackFactor) {
    RootCauseAnalyzer analyzer;
    PerformanceIssue issue;
    issue.category = IssueCategory::QUERY_OPTIMIZATION;

    SystemSnapshot before;
    before.avg_query_latency_ms = 100.0;
    SystemSnapshot after;
    after.avg_query_latency_ms = 100.0;

    const auto report = analyzer.analyzeIssue(issue, before, after);
    EXPECT_FALSE(report.primary_reason_code.empty());
    EXPECT_FALSE(report.contributing_factors.empty());
}

TEST(ObservabilityBlockARootCause, CorrelationRequiresAlignedData) {
    RootCauseAnalyzer analyzer;
    TimeSeries target;
    target.name = "target";
    target.points.push_back({std::chrono::system_clock::now(), 1.0});
    TimeSeries short_series;
    short_series.name = "other";
    short_series.points.push_back({std::chrono::system_clock::now(), 2.0});

    analyzer.addTimeSeries(target);
    analyzer.addTimeSeries(short_series);
    EXPECT_TRUE(analyzer.findCorrelations("target").empty());
}
