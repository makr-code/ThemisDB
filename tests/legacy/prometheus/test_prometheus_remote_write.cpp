// Tests for Prometheus remote-write endpoint compatibility
//
// Phase 1: Unit tests for the protobuf decoder (PromWriteRequest::decode).
// Phase 2: Unit tests for snappy + protobuf combined path (decodeSnappy).
// Phase 3: Integration test for handlePrometheusRemoteWrite using a real
//          TSStore backed by an in-memory RocksDB instance.

#include <gtest/gtest.h>
#include "timeseries/prometheus_remote_write.h"
#include "timeseries/tsstore.h"
#include "server/timeseries_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include <snappy.h>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <chrono>
#include <vector>

namespace fs = std::filesystem;

using namespace themis;
using namespace themis::timeseries;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: hand-craft a minimal protobuf-encoded Prometheus WriteRequest
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Append a protobuf varint to buf.
void appendVarint(std::vector<uint8_t>& buf, uint64_t value) {
    while (value >= 0x80) {
        buf.push_back(static_cast<uint8_t>((value & 0x7F) | 0x80));
        value >>= 7;
    }
    buf.push_back(static_cast<uint8_t>(value));
}

/// Append a length-delimited byte span (wire type 2).
void appendLenDelim(std::vector<uint8_t>& buf, const std::vector<uint8_t>& span) {
    appendVarint(buf, span.size());
    buf.insert(buf.end(), span.begin(), span.end());
}

/// Append a length-delimited string field.
void appendStringField(std::vector<uint8_t>& buf, uint32_t field_number,
                        const std::string& value) {
    // tag = (field_number << 3) | 2 (wire type LEN)
    appendVarint(buf, (static_cast<uint64_t>(field_number) << 3) | 2);
    appendVarint(buf, value.size());
    buf.insert(buf.end(), value.begin(), value.end());
}

/// Encode a Label { name=n, value=v }.
std::vector<uint8_t> encodeLabel(const std::string& name, const std::string& value) {
    std::vector<uint8_t> buf;
    appendStringField(buf, 1, name);
    appendStringField(buf, 2, value);
    return buf;
}

/// Encode a Sample { value=v, timestamp=ts_ms }.
std::vector<uint8_t> encodeSample(double value, int64_t timestamp_ms) {
    std::vector<uint8_t> buf;
    // Field 1 (value): wire type 1 (I64)
    appendVarint(buf, (1ULL << 3) | 1);
    static_assert(sizeof(double) == 8, "double must be 64-bit IEEE 754");
    uint8_t raw[8];
    std::memcpy(raw, &value, 8);
    buf.insert(buf.end(), raw, raw + 8);
    // Field 2 (timestamp): wire type 0 (VARINT)
    appendVarint(buf, (2ULL << 3) | 0);
    appendVarint(buf, static_cast<uint64_t>(timestamp_ms));
    return buf;
}

/// Encode a complete Prometheus WriteRequest containing a single TimeSeries with
/// one label (__name__=metric_name) plus the supplied samples.
std::vector<uint8_t> buildWriteRequest(
        const std::string& metric_name,
        const std::vector<std::pair<double, int64_t>>& samples) {

    // Build TimeSeries
    std::vector<uint8_t> ts_buf;

    // Label __name__=<metric_name>
    auto lbl = encodeLabel("__name__", metric_name);
    appendVarint(ts_buf, (1ULL << 3) | 2); // field 1, wire LEN
    appendLenDelim(ts_buf, lbl);

    // Samples
    for (const auto& [v, ts] : samples) {
        auto s = encodeSample(v, ts);
        appendVarint(ts_buf, (2ULL << 3) | 2); // field 2, wire LEN
        appendLenDelim(ts_buf, s);
    }

    // Wrap in WriteRequest
    std::vector<uint8_t> req_buf;
    appendVarint(req_buf, (1ULL << 3) | 2); // field 1, wire LEN
    appendLenDelim(req_buf, ts_buf);
    return req_buf;
}

/// Snappy-compress a byte buffer.
std::string snappyCompress(const std::vector<uint8_t>& data) {
    std::string compressed = {};
    snappy::Compress(reinterpret_cast<const char*>(data.data()), data.size(),
                     &compressed);
    return compressed;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Unit tests: PromWriteRequest::decode (raw protobuf, no snappy)
// ─────────────────────────────────────────────────────────────────────────────

class PrometheusProtoDecodeTest : public ::testing::Test {};

TEST_F(PrometheusProtoDecodeTest, DecodeEmptyBuffer) {
    // size==0 returns an empty request regardless of the pointer value.
    auto result = PromWriteRequest::decode(nullptr, 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->timeseries.empty());

    // Same with a real pointer but zero size.
    const uint8_t dummy = 0;
    auto result2 = PromWriteRequest::decode(&dummy, 0);
    ASSERT_TRUE(result2.has_value());
    EXPECT_TRUE(result2->timeseries.empty());
}

TEST_F(PrometheusProtoDecodeTest, DecodeRejectsInvalidWireStartTag) {
    // field 1 encoded as VARINT (wire type 0) instead of LEN (wire type 2)
    // must fail fast at wire-start validation.
    std::vector<uint8_t> invalid = {
        static_cast<uint8_t>((1u << 3) | 0u),
        0x01
    };

    auto result = PromWriteRequest::decode(invalid.data(), invalid.size());
    EXPECT_FALSE(result.has_value());
}

TEST_F(PrometheusProtoDecodeTest, DecodeRejectsTruncatedFirstTimeSeriesField) {
    // field 1, wire LEN with declared length 4 but only 1 payload byte present.
    std::vector<uint8_t> invalid = {
        static_cast<uint8_t>((1u << 3) | 2u),
        0x04,
        0x08
    };

    auto result = PromWriteRequest::decode(invalid.data(), invalid.size());
    EXPECT_FALSE(result.has_value());
}

TEST_F(PrometheusProtoDecodeTest, DecodeSingleSample) {
    int64_t ts_ms = 1700000000000LL;
    auto buf = buildWriteRequest("cpu_usage", {{42.5, ts_ms}});
    auto result = PromWriteRequest::decode(buf.data(), buf.size());
    ASSERT_TRUE(result.has_value()) << result.error().message();

    ASSERT_EQ(result->timeseries.size(), 1u);
    const auto& ts = result->timeseries[0];
    EXPECT_EQ(ts.metricName(), "cpu_usage");
    ASSERT_EQ(ts.samples.size(), 1u);
    EXPECT_DOUBLE_EQ(ts.samples[0].value, 42.5);
    EXPECT_EQ(ts.samples[0].timestamp_ms, ts_ms);
}

TEST_F(PrometheusProtoDecodeTest, DecodeMultipleSamples) {
    int64_t base_ms = 1700000000000LL;
    std::vector<std::pair<double, int64_t>> samples;
    for (int i = 0; i < 10; ++i) {
        samples.push_back({static_cast<double>(i) * 1.5, base_ms + i * 15000});
    }
    auto buf = buildWriteRequest("memory_usage", samples);
    auto result = PromWriteRequest::decode(buf.data(), buf.size());
    ASSERT_TRUE(result.has_value()) << result.error().message();

    ASSERT_EQ(result->timeseries.size(), 1u);
    const auto& ts = result->timeseries[0];
    EXPECT_EQ(ts.metricName(), "memory_usage");
    ASSERT_EQ(ts.samples.size(), 10u);
    for (int i = 0; i < 10; ++i) {
        EXPECT_DOUBLE_EQ(ts.samples[i].value, static_cast<double>(i) * 1.5);
        EXPECT_EQ(ts.samples[i].timestamp_ms, base_ms + i * 15000);
    }
}

TEST_F(PrometheusProtoDecodeTest, DecodeAdditionalLabels) {
    // Build a TimeSeries with __name__ + instance + job labels
    std::vector<uint8_t> ts_buf;
    auto add_label = [&](const std::string& name, const std::string& value) {
        auto lbl = encodeLabel(name, value);
        appendVarint(ts_buf, (1ULL << 3) | 2);
        appendLenDelim(ts_buf, lbl);
    };
    add_label("__name__", "http_requests_total");
    add_label("instance",  "web01:9090");
    add_label("job",       "prometheus");

    auto s = encodeSample(100.0, 1700000000000LL);
    appendVarint(ts_buf, (2ULL << 3) | 2);
    appendLenDelim(ts_buf, s);

    std::vector<uint8_t> req_buf;
    appendVarint(req_buf, (1ULL << 3) | 2);
    appendLenDelim(req_buf, ts_buf);

    auto result = PromWriteRequest::decode(req_buf.data(), req_buf.size());
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ(result->timeseries.size(), 1u);

    const auto& ts = result->timeseries[0];
    EXPECT_EQ(ts.metricName(),             "http_requests_total");
    EXPECT_EQ(ts.labelValue("instance"),   "web01:9090");
    EXPECT_EQ(ts.labelValue("job"),        "prometheus");
}

TEST_F(PrometheusProtoDecodeTest, DecodeMultipleTimeSeries) {
    // Two time series in one WriteRequest
    int64_t ts_ms = 1700000000000LL;
    auto buf1 = buildWriteRequest("metric_a", {{1.0, ts_ms}});
    auto buf2 = buildWriteRequest("metric_b", {{2.0, ts_ms + 1000}});

    // Concatenate the two WriteRequests by merging their outer fields
    // (both are field 1, wire LEN in WriteRequest)
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), buf1.begin(), buf1.end());
    combined.insert(combined.end(), buf2.begin(), buf2.end());

    auto result = PromWriteRequest::decode(combined.data(), combined.size());
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ(result->timeseries.size(), 2u);
    EXPECT_EQ(result->timeseries[0].metricName(), "metric_a");
    EXPECT_EQ(result->timeseries[1].metricName(), "metric_b");
}

TEST_F(PrometheusProtoDecodeTest, DecodeTimeSeriesWithNoNameLabel) {
    // A TimeSeries with no __name__ label should still decode without error;
    // metricName() returns empty string.
    auto lbl = encodeLabel("host", "srv01");
    std::vector<uint8_t> ts_buf;
    appendVarint(ts_buf, (1ULL << 3) | 2);
    appendLenDelim(ts_buf, lbl);
    auto s = encodeSample(9.9, 1700000000000LL);
    appendVarint(ts_buf, (2ULL << 3) | 2);
    appendLenDelim(ts_buf, s);

    std::vector<uint8_t> req_buf;
    appendVarint(req_buf, (1ULL << 3) | 2);
    appendLenDelim(req_buf, ts_buf);

    auto result = PromWriteRequest::decode(req_buf.data(), req_buf.size());
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ(result->timeseries.size(), 1u);
    EXPECT_EQ(result->timeseries[0].metricName(), "");
}

// ─────────────────────────────────────────────────────────────────────────────
// Unit tests: PromWriteRequest::decodeSnappy
// ─────────────────────────────────────────────────────────────────────────────

class PrometheusSnappyDecodeTest : public ::testing::Test {};

TEST_F(PrometheusSnappyDecodeTest, DecodeSnappySingleSample) {
    int64_t ts_ms = 1700000000000LL;
    auto raw = buildWriteRequest("disk_io", {{77.3, ts_ms}});
    auto compressed = snappyCompress(raw);

    auto result = PromWriteRequest::decodeSnappy(
        reinterpret_cast<const uint8_t*>(compressed.data()),
        compressed.size());
    ASSERT_TRUE(result.has_value()) << result.error().message();

    ASSERT_EQ(result->timeseries.size(), 1u);
    EXPECT_EQ(result->timeseries[0].metricName(), "disk_io");
    ASSERT_EQ(result->timeseries[0].samples.size(), 1u);
    EXPECT_DOUBLE_EQ(result->timeseries[0].samples[0].value, 77.3);
    EXPECT_EQ(result->timeseries[0].samples[0].timestamp_ms, ts_ms);
}

TEST_F(PrometheusSnappyDecodeTest, DecodeSnappyInvalidData) {
    // Garbage input should fail gracefully
    const uint8_t garbage[] = {0xFF, 0xFE, 0xFD, 0x01, 0x02, 0x03};
    auto result = PromWriteRequest::decodeSnappy(garbage, sizeof(garbage));
    EXPECT_FALSE(result.has_value());
}

TEST_F(PrometheusSnappyDecodeTest, DecodeSnappyEmptyBuffer) {
    auto result = PromWriteRequest::decodeSnappy(nullptr, 0);
    EXPECT_FALSE(result.has_value()); // empty snappy stream is invalid
}

TEST_F(PrometheusSnappyDecodeTest, DecodeSnappyRejectsInvalidWireStartAfterDecompress) {
    // Build a syntactically valid snappy stream that inflates to an invalid
    // protobuf start (field 1 with VARINT wire type).
    const std::vector<uint8_t> invalid_raw = {
        static_cast<uint8_t>((1u << 3) | 0u),
        0x01
    };
    const auto compressed = snappyCompress(invalid_raw);

    auto result = PromWriteRequest::decodeSnappy(
        reinterpret_cast<const uint8_t*>(compressed.data()),
        compressed.size());
    EXPECT_FALSE(result.has_value());
}

TEST_F(PrometheusSnappyDecodeTest, DecodeSnappyDecompressionBombRejected) {
    // Craft a valid snappy frame whose header claims a very large uncompressed
    // size (> 32 MB).  We fabricate the snappy preamble manually:
    // snappy stores the uncompressed length as a varint at the start of the
    // compressed stream.  We cannot produce a valid compressed payload that
    // truly decompresses to > 32 MB in this unit test, but we can verify that
    // the 32 MB guard in decodeSnappy rejects payloads that claim such a size
    // by injecting a real snappy compressed blob and then manipulating its
    // length varint.

    // Build a small legitimate compressed buffer.
    auto raw = buildWriteRequest("bomb_metric", {{1.0, 1700000000000LL}});
    std::string compressed;
    snappy::Compress(reinterpret_cast<const char*>(raw.data()), raw.size(),
                     &compressed);

    // Build a tampered header varint claiming 64 MB (> 32 MB limit).
    // Re-use the appendVarint helper defined in this file by encoding into
    // a temporary vector<uint8_t> then converting to string.
    constexpr uint64_t fake_len = 64ULL * 1024 * 1024;
    std::vector<uint8_t> varint_buf;
    appendVarint(varint_buf, fake_len);
    std::string tampered(reinterpret_cast<const char*>(varint_buf.data()),
                         varint_buf.size());

    // Skip the original varint in the compressed header to get the raw payload.
    size_t orig_varint_bytes = 0;
    {
        size_t pos = 0;
        while (pos < compressed.size()) {
            uint8_t b = static_cast<uint8_t>(compressed[pos++]);
            orig_varint_bytes++;
            if ((b & 0x80) == 0) {
              break;
            }
        }
    }
    tampered.append(compressed.substr(orig_varint_bytes));

    auto result = PromWriteRequest::decodeSnappy(
        reinterpret_cast<const uint8_t*>(tampered.data()), tampered.size());
    // The tampered payload has an invalid claimed size → snappy itself rejects it
    // (GetUncompressedLength will return false for an inconsistent stream),
    // OR our size guard catches it.  Either way, decoding must fail.
    EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Integration test: handlePrometheusRemoteWrite → TSStore
// ─────────────────────────────────────────────────────────────────────────────

class PrometheusRemoteWriteHandlerTest : public ::testing::Test {
protected:
    std::string db_path = {};
    std::unique_ptr<RocksDBWrapper> db;
    std::shared_ptr<TSStore>        ts_store;

    void SetUp() override {
        auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        db_path = (fs::temp_directory_path()
                   / ("themis_prom_rw_" + std::to_string(ns))).string();
        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "RocksDB open failed";
        ts_store = std::make_shared<TSStore>(db->getRawDB());
    }

    void TearDown() override {
        ts_store.reset();
        db.reset();
        fs::remove_all(db_path);
    }

    /// Build a Boost.Beast HTTP request with the given body and headers.
    static boost::beast::http::request<boost::beast::http::string_body>
    makeRequest(const std::string& body,
                const std::string& content_encoding = "snappy") {
        namespace http = boost::beast::http;
        http::request<http::string_body> req{http::verb::post, "/api/v1/prom/write", 11};
        req.set(http::field::content_type, "application/x-protobuf");
        req.set("X-Prometheus-Remote-Write-Version", "0.1.0");
        if (!content_encoding.empty()) {
            req.set(http::field::content_encoding, content_encoding);
        }
        req.body() = body;
        req.prepare_payload();
        return req;
    }

    size_t queryCount(const std::string& metric, const std::string& entity,
                      int64_t from_ms, int64_t to_ms) {
        TSStore::QueryOptions q;
        q.metric            = metric;
        q.entity            = entity;
        q.from_timestamp_ms = from_ms;
        q.to_timestamp_ms   = to_ms;
        q.limit             = 1000000;
        auto r = ts_store->query(q);
        return r.has_value() ? r->size() : 0;
    }
};

TEST_F(PrometheusRemoteWriteHandlerTest, HandlerReturns204OnSuccess) {
    namespace http = boost::beast::http;

    themis::server::TimeSeriesApiHandler handler(
        nullptr, ts_store, nullptr, nullptr);

    int64_t ts_ms = 1700000000000LL;
    auto raw        = buildWriteRequest("net_bytes_recv", {{1024.0, ts_ms}});
    auto compressed = snappyCompress(raw);

    auto req = makeRequest(compressed);
    auto res = handler.handlePrometheusRemoteWrite(req);

    EXPECT_EQ(res.result(), http::status::no_content);
}

TEST_F(PrometheusRemoteWriteHandlerTest, HandlerStoresSamplesInTSStore) {
    namespace http = boost::beast::http;

    themis::server::TimeSeriesApiHandler handler(
        nullptr, ts_store, nullptr, nullptr);

    int64_t base_ms = 1700000000000LL;
    std::vector<std::pair<double, int64_t>> samples;
    for (int i = 0; i < 5; ++i) {
        samples.push_back({static_cast<double>(i), base_ms + i * 15000LL});
    }
    auto raw        = buildWriteRequest("prom_metric", samples);
    auto compressed = snappyCompress(raw);

    auto req = makeRequest(compressed);
    auto res = handler.handlePrometheusRemoteWrite(req);
    EXPECT_EQ(res.result(), http::status::no_content);

    // All 5 samples should be retrievable from TSStore
    EXPECT_EQ(queryCount("prom_metric", "default",
                          base_ms, base_ms + 5 * 15000LL), 5u);
}

TEST_F(PrometheusRemoteWriteHandlerTest, HandlerExtractsInstanceAsEntity) {
    namespace http = boost::beast::http;

    themis::server::TimeSeriesApiHandler handler(
        nullptr, ts_store, nullptr, nullptr);

    int64_t ts_ms = 1700000000000LL;

    // Build a TimeSeries with __name__, instance, and job labels
    std::vector<uint8_t> ts_buf;
    auto add_lbl = [&](const std::string& n, const std::string& v) {
        auto lbl = encodeLabel(n, v);
        appendVarint(ts_buf, (1ULL << 3) | 2);
        appendLenDelim(ts_buf, lbl);
    };
    add_lbl("__name__",  "http_requests");
    add_lbl("instance",  "app01:8080");
    add_lbl("job",       "webserver");

    auto s = encodeSample(200.0, ts_ms);
    appendVarint(ts_buf, (2ULL << 3) | 2);
    appendLenDelim(ts_buf, s);

    std::vector<uint8_t> req_buf;
    appendVarint(req_buf, (1ULL << 3) | 2);
    appendLenDelim(req_buf, ts_buf);

    auto compressed = snappyCompress(req_buf);
    auto req = makeRequest(compressed);
    auto res = handler.handlePrometheusRemoteWrite(req);
    EXPECT_EQ(res.result(), boost::beast::http::status::no_content);

    // The 'instance' label should map to the 'entity' field
    EXPECT_EQ(queryCount("http_requests", "app01:8080",
                          ts_ms - 1, ts_ms + 1), 1u);
}

TEST_F(PrometheusRemoteWriteHandlerTest, HandlerRejects400OnBadSnappy) {
    namespace http = boost::beast::http;

    themis::server::TimeSeriesApiHandler handler(
        nullptr, ts_store, nullptr, nullptr);

    // Send garbage that is not a valid snappy payload
    std::string garbage(32, '\xFF');
    auto req = makeRequest(garbage);
    auto res = handler.handlePrometheusRemoteWrite(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(PrometheusRemoteWriteHandlerTest, HandlerRejects400OnEmptyBody) {
    namespace http = boost::beast::http;

    themis::server::TimeSeriesApiHandler handler(
        nullptr, ts_store, nullptr, nullptr);

    auto req = makeRequest("");
    auto res = handler.handlePrometheusRemoteWrite(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(PrometheusRemoteWriteHandlerTest, HandlerReturns501WhenFeatureDisabled) {
    namespace http = boost::beast::http;

    // Handler constructed with null ts_store (feature disabled)
    themis::server::TimeSeriesApiHandler handler(
        nullptr, nullptr, nullptr, nullptr);

    int64_t ts_ms = 1700000000000LL;
    auto raw        = buildWriteRequest("test_metric", {{1.0, ts_ms}});
    auto compressed = snappyCompress(raw);
    auto req = makeRequest(compressed);
    auto res = handler.handlePrometheusRemoteWrite(req);

    EXPECT_EQ(res.result(), http::status::not_implemented);
}

TEST_F(PrometheusRemoteWriteHandlerTest, HandlerAcceptsIdentityEncoding) {
    namespace http = boost::beast::http;

    themis::server::TimeSeriesApiHandler handler(
        nullptr, ts_store, nullptr, nullptr);

    int64_t ts_ms = 1700000000000LL;
    auto raw = buildWriteRequest("raw_metric", {{3.14, ts_ms}});

    // Send raw (identity-encoded) protobuf
    std::string body(reinterpret_cast<const char*>(raw.data()), raw.size());
    auto req = makeRequest(body, "identity");
    auto res = handler.handlePrometheusRemoteWrite(req);

    EXPECT_EQ(res.result(), http::status::no_content);
    EXPECT_EQ(queryCount("raw_metric", "default",
                          ts_ms - 1, ts_ms + 1), 1u);
}

TEST_F(PrometheusRemoteWriteHandlerTest, HandlerRejects400OnUnsupportedEncoding) {
    namespace http = boost::beast::http;

    themis::server::TimeSeriesApiHandler handler(
        nullptr, ts_store, nullptr, nullptr);

    auto req = makeRequest("dummy", "gzip");
    auto res = handler.handlePrometheusRemoteWrite(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(PrometheusRemoteWriteHandlerTest, HandlerSkipsTimeSeriesWithoutNameLabel) {
    namespace http = boost::beast::http;

    themis::server::TimeSeriesApiHandler handler(
        nullptr, ts_store, nullptr, nullptr);

    int64_t ts_ms = 1700000000000LL;

    // TimeSeries with no __name__ label
    std::vector<uint8_t> ts_buf;
    auto lbl = encodeLabel("host", "srv01");
    appendVarint(ts_buf, (1ULL << 3) | 2);
    appendLenDelim(ts_buf, lbl);
    auto s = encodeSample(1.0, ts_ms);
    appendVarint(ts_buf, (2ULL << 3) | 2);
    appendLenDelim(ts_buf, s);

    std::vector<uint8_t> req_buf;
    appendVarint(req_buf, (1ULL << 3) | 2);
    appendLenDelim(req_buf, ts_buf);

    auto compressed = snappyCompress(req_buf);
    auto req = makeRequest(compressed);
    auto res = handler.handlePrometheusRemoteWrite(req);

    // Handler still returns 204; unnamed time series are silently skipped
    EXPECT_EQ(res.result(), http::status::no_content);
}
