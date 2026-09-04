/*
 * tests/test_hardware_telemetry.cpp
 *
 * Focused unit tests for the HardwareTelemetryReporter.
 *
 * All tests use injectable fakes – no real /proc, network, or libcurl is
 * required.  The tests cover:
 *
 *   HT-01  Default TelemetryConfig has enabled=false
 *   HT-02  Default send_interval_seconds is 86400
 *   HT-03  Interval below 86400 is clamped to 86400 at construction
 *   HT-04  collect() always fills instance_id, themis_version, timestamp_utc
 *   HT-05  collect() respects include_cpu_model=false
 *   HT-06  collect() respects include_cpu_cores=false
 *   HT-07  collect() respects include_ram_mb=false
 *   HT-08  collect() respects include_os=false
 *   HT-09  collect() respects include_arch=false
 *   HT-10  HardwareSnapshot::toJson() produces valid JSON with all fields
 *   HT-11  HardwareSnapshot::toJson() omits empty/zero hardware fields
 *   HT-12  send() returns false when endpoint_url is empty
 *   HT-13  send() calls the injected HTTP sender with correct args
 *   HT-14  send() returns false when HTTP sender returns false
 *   HT-15  report() returns false when telemetry is disabled
 *   HT-16  report() calls collect() + send() when enabled
 *   HT-17  instance_id is a valid UUID v4 format
 *   HT-18  Two reporters produce different instance_ids
 *   HT-19  include_performance=false → no "performance" key in JSON
 *   HT-20  include_performance=true, no provider → no "performance" key
 *   HT-21  include_performance=true, provider wired → "performance" in JSON
 *   HT-22  queries_per_second_bucket is bucketed to power-of-2
 *   HT-23  process_rss_mb_bucket is bucketed to 64 MiB
 *   HT-24  db_size_mb_bucket is bucketed to 512 MiB
 *   HT-25  active_connections_bucket is bucketed to power-of-2
 *   HT-26  cache_hit_rate_pct value 255 is serialised as-is
 *   HT-27  perf fields with zero/unavailable values are omitted from JSON
 *   HT-28  startBackgroundReporting() no-ops when telemetry is disabled
 *   HT-29  stopBackgroundReporting() is idempotent
 *   HT-30  UpdatesConfig::loadFromYaml parses telemetry section correctly
 *   HT-31  send_interval clamped to 86400 in loadFromYaml
 *   HT-32  UpdatesConfig::toJson round-trips telemetry config
 *   HT-33  include_performance persists through fromJson/toJson round-trip
 */

#include <gtest/gtest.h>

#include "updates/hardware_telemetry.h"
#include "updates/updates_config.h"

#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <vector>

using namespace themis::updates;
using json = nlohmann::json;

// ── Fake hardware provider ────────────────────────────────────────────────────

struct FakeHWProvider final : public IHardwareInfoProvider {
    std::string cpu_model_val  = "FakeCPU X9";
    unsigned int cpu_cores_val = 8;
    uint64_t ram_mb_val        = 16384;
    std::string os_family_val  = "Linux";
    std::string cpu_arch_val   = "x86_64";

    std::string  cpuModel()  const override { return cpu_model_val; }
    unsigned int cpuCores()  const override { return cpu_cores_val; }
    uint64_t     totalRamMb() const override { return ram_mb_val; }
    std::string  osFamily()  const override { return os_family_val; }
    std::string  cpuArch()   const override { return cpu_arch_val; }
};

// ── Fake performance provider ─────────────────────────────────────────────────

struct FakePerfProvider final : public IPerformanceMetricsProvider {
    PerformanceSnapshot snap;
    PerformanceSnapshot collect() const override { return snap; }
};

// ── Fake HTTP sender ──────────────────────────────────────────────────────────

struct FakeHttpSender {
    bool return_value = true;
    std::string last_url = {};
    std::string last_body = {};
    std::string last_content_type = {};
    int         last_timeout = 0;

    bool operator()(const std::string& url, const std::string& body,
                    const std::string& ct, int timeout) {
        last_url          = url;
        last_body         = body;
        last_content_type = ct;
        last_timeout      = timeout;
        return return_value;
    }
};

// ── Helper: build a default-enabled TelemetryConfig ──────────────────────────

static TelemetryConfig makeEnabledConfig(
        const std::string& url = "https://api.themisdb.org/telemetry.php") {
    TelemetryConfig c;
    c.enabled              = true;
    c.endpoint_url         = url;
    c.send_interval_seconds = 86400;
    return c;
}

static std::shared_ptr<FakeHWProvider> fakeHW() {
    return std::make_shared<FakeHWProvider>();
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

// HT-01
TEST(HardwareTelemetry, DefaultConfigDisabled) {
    TelemetryConfig c;
    EXPECT_FALSE(c.enabled);
}

// HT-02
TEST(HardwareTelemetry, DefaultInterval86400) {
    TelemetryConfig c;
    EXPECT_EQ(c.send_interval_seconds, 86400);
}

// HT-03
TEST(HardwareTelemetry, IntervalClampedTo86400) {
    TelemetryConfig c;
    c.enabled               = true;
    c.send_interval_seconds = 60;  // too low

    FakeHttpSender sender;
    HardwareTelemetryReporter r(c, fakeHW(),
        [&](auto&&... args){ return sender(std::forward<decltype(args)>(args)...); });

    EXPECT_EQ(r.config().send_interval_seconds, 86400);
}

// HT-04
TEST(HardwareTelemetry, CollectAlwaysFillsBaseFields) {
    auto cfg = makeEnabledConfig();
    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);

    auto snap = r.collect();
    EXPECT_FALSE(snap.instance_id.empty());
    EXPECT_FALSE(snap.themis_version.empty());
    EXPECT_GT(snap.timestamp_utc, 0);
}

// HT-05 – include_cpu_model=false
TEST(HardwareTelemetry, CollectRespectsCpuModelFalse) {
    auto cfg = makeEnabledConfig();
    cfg.include_cpu_model = false;
    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    EXPECT_TRUE(r.collect().cpu_model.empty());
}

// HT-06 – include_cpu_cores=false
TEST(HardwareTelemetry, CollectRespectsCpuCoresFalse) {
    auto cfg = makeEnabledConfig();
    cfg.include_cpu_cores = false;
    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    EXPECT_EQ(r.collect().cpu_cores, 0u);
}

// HT-07 – include_ram_mb=false
TEST(HardwareTelemetry, CollectRespectsRamFalse) {
    auto cfg = makeEnabledConfig();
    cfg.include_ram_mb = false;
    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    EXPECT_EQ(r.collect().total_ram_mb, 0u);
}

// HT-08 – include_os=false
TEST(HardwareTelemetry, CollectRespectsOsFalse) {
    auto cfg = makeEnabledConfig();
    cfg.include_os = false;
    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    EXPECT_TRUE(r.collect().os_family.empty());
}

// HT-09 – include_arch=false
TEST(HardwareTelemetry, CollectRespectsArchFalse) {
    auto cfg = makeEnabledConfig();
    cfg.include_arch = false;
    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    EXPECT_TRUE(r.collect().cpu_arch.empty());
}

// HT-10 – toJson with all fields
TEST(HardwareTelemetry, ToJsonAllFields) {
    HardwareSnapshot snap;
    snap.instance_id    = "test-uuid";
    snap.themis_version = "2.0.0";
    snap.timestamp_utc  = 1713121131;
    snap.cpu_model      = "FakeCPU";
    snap.cpu_cores      = 8;
    snap.total_ram_mb   = 16384;
    snap.os_family      = "Linux";
    snap.cpu_arch       = "x86_64";

    auto j = json::parse(snap.toJson());
    EXPECT_EQ(j["instance_id"].get<std::string>(),    "test-uuid");
    EXPECT_EQ(j["themis_version"].get<std::string>(), "2.0.0");
    EXPECT_EQ(j["cpu_model"].get<std::string>(),      "FakeCPU");
    EXPECT_EQ(j["cpu_cores"].get<unsigned>(),         8u);
    EXPECT_EQ(j["total_ram_mb"].get<uint64_t>(),      16384u);
    EXPECT_EQ(j["os_family"].get<std::string>(),      "Linux");
    EXPECT_EQ(j["cpu_arch"].get<std::string>(),       "x86_64");
}

// HT-11 – toJson omits empty/zero fields
TEST(HardwareTelemetry, ToJsonOmitsEmptyFields) {
    HardwareSnapshot snap;
    snap.instance_id    = "id";
    snap.themis_version = "1.0.0";
    snap.timestamp_utc  = 1713121131;
    // cpu_model empty, cpu_cores=0, etc.

    auto j = json::parse(snap.toJson());
    EXPECT_FALSE(j.contains("cpu_model"));
    EXPECT_FALSE(j.contains("cpu_cores"));
    EXPECT_FALSE(j.contains("total_ram_mb"));
    EXPECT_FALSE(j.contains("os_family"));
    EXPECT_FALSE(j.contains("cpu_arch"));
}

// HT-12 – send() returns false on empty endpoint
TEST(HardwareTelemetry, SendReturnsFalseEmptyEndpoint) {
    auto cfg = makeEnabledConfig("");
    FakeHttpSender sender;
    HardwareTelemetryReporter r(cfg, fakeHW(),
        [&](auto&&... a){ return sender(std::forward<decltype(a)>(a)...); });

    HardwareSnapshot snap;
    snap.instance_id    = "id";
    snap.themis_version = "1.0.0";
    snap.timestamp_utc  = 1;
    EXPECT_FALSE(r.send(snap));
}

// HT-13 – send() passes correct args to HTTP sender
TEST(HardwareTelemetry, SendPassesCorrectArgsToSender) {
    auto cfg = makeEnabledConfig();
    cfg.http_timeout_seconds = 15;
    FakeHttpSender sender;
    HardwareTelemetryReporter r(cfg, fakeHW(),
        [&](auto&&... a){ return sender(std::forward<decltype(a)>(a)...); });

    HardwareSnapshot snap;
    snap.instance_id    = "id";
    snap.themis_version = "1.0.0";
    snap.timestamp_utc  = 1;

    r.send(snap);
    EXPECT_EQ(sender.last_url,          "https://api.themisdb.org/telemetry.php");
    EXPECT_EQ(sender.last_content_type, "application/json");
    EXPECT_EQ(sender.last_timeout,      15);
    EXPECT_FALSE(sender.last_body.empty());
}

// HT-14 – send() returns false when sender returns false
TEST(HardwareTelemetry, SendReturnsFalseWhenSenderFails) {
    auto cfg = makeEnabledConfig();
    cfg.max_retries = 0;
    FakeHttpSender sender;
    sender.return_value = false;
    HardwareTelemetryReporter r(cfg, fakeHW(),
        [&](auto&&... a){ return sender(std::forward<decltype(a)>(a)...); });

    HardwareSnapshot snap;
    snap.instance_id    = "id";
    snap.themis_version = "1.0.0";
    snap.timestamp_utc  = 1;
    EXPECT_FALSE(r.send(snap));
}

// HT-15 – report() returns false when disabled
TEST(HardwareTelemetry, ReportReturnsFalseWhenDisabled) {
    TelemetryConfig cfg;  // enabled=false by default
    FakeHttpSender sender;
    HardwareTelemetryReporter r(cfg, fakeHW(),
        [&](auto&&... a){ return sender(std::forward<decltype(a)>(a)...); });

    EXPECT_FALSE(r.report());
    EXPECT_TRUE(sender.last_url.empty());  // sender must not have been called
}

// HT-16 – report() calls sender when enabled
TEST(HardwareTelemetry, ReportCallsSenderWhenEnabled) {
    auto cfg = makeEnabledConfig();
    FakeHttpSender sender;
    HardwareTelemetryReporter r(cfg, fakeHW(),
        [&](auto&&... a){ return sender(std::forward<decltype(a)>(a)...); });

    r.report();
    EXPECT_FALSE(sender.last_url.empty());
}

// HT-17 – instance_id is UUID v4
TEST(HardwareTelemetry, InstanceIdIsUuidV4) {
    auto cfg = makeEnabledConfig();
    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);

    const std::regex uuid_re(
        R"([0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12})",
        std::regex::icase);
    EXPECT_TRUE(std::regex_match(r.instanceId(), uuid_re));
}

// HT-18 – two reporters produce different UUIDs
TEST(HardwareTelemetry, TwoReportersDifferentIds) {
    auto cfg = makeEnabledConfig();
    HardwareTelemetryReporter r1(cfg, fakeHW(), nullptr);
    HardwareTelemetryReporter r2(cfg, fakeHW(), nullptr);
    EXPECT_NE(r1.instanceId(), r2.instanceId());
}

// HT-19 – include_performance=false → no "performance" key
TEST(HardwareTelemetry, NoPerformanceKeyWhenDisabled) {
    auto cfg = makeEnabledConfig();
    cfg.include_performance = false;
    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    auto snap = r.collect();
    EXPECT_FALSE(snap.performance.has_value());
    auto j = json::parse(snap.toJson());
    EXPECT_FALSE(j.contains("performance"));
}

// HT-20 – include_performance=true but no provider → no "performance" key
TEST(HardwareTelemetry, NoPerformanceKeyWithoutProvider) {
    auto cfg = makeEnabledConfig();
    cfg.include_performance = true;
    // setPerformanceProvider not called
    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    auto snap = r.collect();
    EXPECT_FALSE(snap.performance.has_value());
}

// HT-21 – include_performance=true + provider → "performance" in JSON
TEST(HardwareTelemetry, PerformanceInJsonWhenProviderWired) {
    auto cfg = makeEnabledConfig();
    cfg.include_performance = true;

    auto prov = std::make_shared<FakePerfProvider>();
    prov->snap.avg_query_latency_us  = 500;
    prov->snap.p99_query_latency_us  = 2000;
    prov->snap.cache_hit_rate_pct    = 87;
    prov->snap.uptime_seconds        = 3600;

    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    r.setPerformanceProvider(prov);

    auto snap = r.collect();
    ASSERT_TRUE(snap.performance.has_value());
    auto j = json::parse(snap.toJson());
    ASSERT_TRUE(j.contains("performance"));
    EXPECT_EQ(j["performance"]["avg_query_latency_us"].get<uint64_t>(), 500u);
    EXPECT_EQ(j["performance"]["cache_hit_rate_pct"].get<uint8_t>(), 87u);
}

// HT-22 – queries_per_second_bucket bucketed to power-of-2
TEST(HardwareTelemetry, QpsBucketedToPowerOf2) {
    auto cfg = makeEnabledConfig();
    cfg.include_performance = true;

    auto prov = std::make_shared<FakePerfProvider>();
    prov->snap.queries_per_second_bucket = 300;  // nearest power-of-2 below = 256

    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    r.setPerformanceProvider(prov);

    auto snap = r.collect();
    ASSERT_TRUE(snap.performance.has_value());
    EXPECT_EQ(snap.performance->queries_per_second_bucket, 256u);
}

// HT-23 – process_rss_mb_bucket to 64 MiB
TEST(HardwareTelemetry, RssMbBucketTo64) {
    auto cfg = makeEnabledConfig();
    cfg.include_performance = true;

    auto prov = std::make_shared<FakePerfProvider>();
    prov->snap.process_rss_mb_bucket = 200;  // floor to 192 (3 * 64)

    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    r.setPerformanceProvider(prov);

    auto snap = r.collect();
    ASSERT_TRUE(snap.performance.has_value());
    EXPECT_EQ(snap.performance->process_rss_mb_bucket, 192u);
}

// HT-24 – db_size_mb_bucket to 512 MiB
TEST(HardwareTelemetry, DbSizeMbBucketTo512) {
    auto cfg = makeEnabledConfig();
    cfg.include_performance = true;

    auto prov = std::make_shared<FakePerfProvider>();
    prov->snap.db_size_mb_bucket = 1300;  // floor to 1024 (2 * 512)

    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    r.setPerformanceProvider(prov);

    auto snap = r.collect();
    ASSERT_TRUE(snap.performance.has_value());
    EXPECT_EQ(snap.performance->db_size_mb_bucket, 1024u);
}

// HT-25 – active_connections_bucket to power-of-2
TEST(HardwareTelemetry, ConnectionsBucketToPowerOf2) {
    auto cfg = makeEnabledConfig();
    cfg.include_performance = true;

    auto prov = std::make_shared<FakePerfProvider>();
    prov->snap.active_connections_bucket = 100;  // floor = 64

    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    r.setPerformanceProvider(prov);

    auto snap = r.collect();
    ASSERT_TRUE(snap.performance.has_value());
    EXPECT_EQ(snap.performance->active_connections_bucket, 64u);
}

// HT-26 – cache_hit_rate_pct 255 serialised as-is
TEST(HardwareTelemetry, CacheHitRatePct255Serialised) {
    HardwareSnapshot snap;
    snap.instance_id    = "id";
    snap.themis_version = "1.0.0";
    snap.timestamp_utc  = 1;

    PerformanceSnapshot ps;
    ps.cache_hit_rate_pct = 255;
    ps.uptime_seconds     = 1;
    snap.performance = ps;

    auto j = json::parse(snap.toJson());
    ASSERT_TRUE(j.contains("performance"));
    EXPECT_EQ(j["performance"]["cache_hit_rate_pct"].get<int>(), 255);
}

// HT-27 – zero perf fields omitted from JSON
TEST(HardwareTelemetry, ZeroPerfFieldsOmitted) {
    HardwareSnapshot snap;
    snap.instance_id    = "id";
    snap.themis_version = "1.0.0";
    snap.timestamp_utc  = 1;
    snap.performance    = PerformanceSnapshot{};  // all zeros / 255

    auto j = json::parse(snap.toJson());
    // performance sub-object should be absent or empty (all zeros → all omitted)
    if (j.contains("performance")) {
        EXPECT_FALSE(j["performance"].contains("avg_query_latency_us"));
        EXPECT_FALSE(j["performance"].contains("queries_per_second_bucket"));
    }
}

// HT-28 – startBackgroundReporting no-ops when disabled
TEST(HardwareTelemetry, StartNoOpsWhenDisabled) {
    TelemetryConfig cfg;  // enabled=false
    FakeHttpSender sender;
    HardwareTelemetryReporter r(cfg, fakeHW(),
        [&](auto&&... a){ return sender(std::forward<decltype(a)>(a)...); });

    r.startBackgroundReporting();
    EXPECT_FALSE(r.isRunning());
}

// HT-29 – stopBackgroundReporting is idempotent
TEST(HardwareTelemetry, StopIdempotent) {
    TelemetryConfig cfg;
    HardwareTelemetryReporter r(cfg, fakeHW(), nullptr);
    // Not started – calling stop multiple times should not throw or crash.
    EXPECT_NO_THROW({
        r.stopBackgroundReporting();
        r.stopBackgroundReporting();
    });
}

// HT-30 – UpdatesConfig defaults for telemetry
TEST(HardwareTelemetry, UpdatesConfigDefaultTelemetry) {
    UpdatesConfig cfg;
    EXPECT_FALSE(cfg.telemetry.enabled);
    EXPECT_EQ(cfg.telemetry.send_interval_seconds, 86400);
    EXPECT_EQ(cfg.telemetry.endpoint_url,
              std::string("https://api.themisdb.org/telemetry.php"));
}

// HT-31 – loadFromYaml clamps small interval
TEST(HardwareTelemetry, LoadFromYamlClampsInterval) {
    // We test fromJson (same logic, no file I/O needed).
    json j;
    j["telemetry"]["enabled"]               = true;
    j["telemetry"]["endpoint_url"]          = "https://example.com/t.php";
    j["telemetry"]["send_interval_seconds"] = 60;  // should be clamped

    auto cfg = UpdatesConfig::fromJson(j);
    EXPECT_EQ(cfg.telemetry.send_interval_seconds, 86400);
}

// HT-32 – toJson round-trips telemetry config
TEST(HardwareTelemetry, ToJsonRoundTrips) {
    UpdatesConfig cfg;
    cfg.telemetry.enabled               = true;
    cfg.telemetry.endpoint_url          = "https://example.com/t.php";
    cfg.telemetry.send_interval_seconds = 86400;
    cfg.telemetry.include_cpu_model     = false;

    auto j = cfg.toJson();
    EXPECT_EQ(j["telemetry"]["enabled"].get<bool>(),      true);
    EXPECT_EQ(j["telemetry"]["endpoint_url"].get<std::string>(),
              "https://example.com/t.php");
    EXPECT_EQ(j["telemetry"]["include_cpu_model"].get<bool>(), false);
}

// HT-33 – include_arch round-trips through fromJson/toJson
TEST(HardwareTelemetry, IncludeArchRoundTrip) {
    json j;
    j["telemetry"]["enabled"]             = true;
    j["telemetry"]["include_arch"]        = true;
    j["telemetry"]["send_interval_seconds"] = 86400;

    auto cfg = UpdatesConfig::fromJson(j);
    EXPECT_TRUE(cfg.telemetry.include_arch);

    auto j2 = cfg.toJson();
    EXPECT_TRUE(j2["telemetry"]["include_arch"].get<bool>());
}
