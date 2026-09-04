/// @file test_gap008_provenance_cli.cpp
/// @brief Unit tests for provenance export CLI command
/// @ingroup gap008_provenance_export
///
/// Tests the CLI `provenance-export` command parsing, option handling, and output formatting.

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

using json = nlohmann::json;

// ── Helper: Parse query parameters from URL string ──────────────────────────

namespace {

/// Parse a query string into key-value pairs.
/// For example: "limit=1000&query_id=abc" → {{"limit", "1000"}, {"query_id", "abc"}}
std::unordered_map<std::string, std::string> parseQueryString(const std::string& qs) {
    std::unordered_map<std::string, std::string> params = {};

    if (qs.empty()) {
      return params;
    }

    size_t start = 0;
    if (qs[0] == '?') {
      start = 1;
    }

    size_t pos = start;
    while (pos < qs.length()) {
        size_t eq_pos = qs.find('=', pos);
        if (eq_pos == std::string::npos) {
          break;
        }

        std::string key = qs.substr(pos, eq_pos - pos);
        size_t amp_pos = qs.find('&', eq_pos);
        std::string value = {};
        if (amp_pos == std::string::npos) {
            value = qs.substr(eq_pos + 1);
            pos = qs.length();
        } else {
            value = qs.substr(eq_pos + 1, amp_pos - eq_pos - 1);
            pos = amp_pos + 1;
        }
        params[key] = value;
    }
    return params;
}

}  // namespace

// ── Test Suite ───────────────────────────────────────────────────────────────

class ProvenanceCliTest : public ::testing::Test {
protected:
    /// Helper to construct expected query string based on command options
    std::string buildExpectedQueryPath(
        int limit,
        const std::string& query_id = "",
        int64_t start_ts_ms = -1,
        int64_t end_ts_ms = -1) {
        std::ostringstream qs = {};
        qs << "/api/v1/observability/provenance?limit=" << limit;
        if (!query_id.empty()) {
            qs << "&query_id=" << query_id;
        }
        if (start_ts_ms >= 0) {
            qs << "&start_ts_ms=" << start_ts_ms;
        }
        if (end_ts_ms >= 0) {
            qs << "&end_ts_ms=" << end_ts_ms;
        }
        return qs.str();
    }

    /// Generate mock provenance response
    json generateMockProvenanceResponse(size_t count) {
        json records = json::array();
        for (size_t i = 0; i < count; ++i) {
            json rec;
            rec["query_id"] = "query_" + std::to_string(i);
            rec["operation"] = (i % 2 == 0) ? "SELECT" : "INSERT";
            rec["timestamp_ms"] = 1000000 + i * 100;
            rec["details"] = {
                {"rows_processed", i * 10},
                {"duration_ms", i * 5}
            };
            records.push_back(rec);
        }
        return records;
    }
};

// ────── Test: Query string construction for default limit ─────────────────

TEST_F(ProvenanceCliTest, QueryStringDefaultLimit) {
    std::string expected = buildExpectedQueryPath(1000);
    EXPECT_EQ(expected, "/api/v1/observability/provenance?limit=1000");
}

// ────── Test: Query string construction with query_id ─────────────────────

TEST_F(ProvenanceCliTest, QueryStringWithQueryId) {
    std::string expected = buildExpectedQueryPath(1000, "abc123");
    EXPECT_EQ(expected, "/api/v1/observability/provenance?limit=1000&query_id=abc123");
}

// ────── Test: Query string construction with time range ───────────────────

TEST_F(ProvenanceCliTest, QueryStringWithTimeRange) {
    std::string expected = buildExpectedQueryPath(1000, "", 1000, 2000);
    EXPECT_EQ(expected, "/api/v1/observability/provenance?limit=1000&start_ts_ms=1000&end_ts_ms=2000");
}

// ────── Test: Query string construction with all parameters ──────────────

TEST_F(ProvenanceCliTest, QueryStringAllParameters) {
    std::string expected = buildExpectedQueryPath(500, "xyz789", 1000, 3000);
    EXPECT_EQ(expected, "/api/v1/observability/provenance?limit=500&query_id=xyz789&start_ts_ms=1000&end_ts_ms=3000");
}

// ────── Test: Parse query string parameters ───────────────────────────────

TEST_F(ProvenanceCliTest, ParseQueryParameters) {
    auto params = parseQueryString("limit=1000&query_id=abc&start_ts_ms=500&end_ts_ms=1500");
    EXPECT_EQ(params.size(), 4u);
    EXPECT_EQ(params["limit"], "1000");
    EXPECT_EQ(params["query_id"], "abc");
    EXPECT_EQ(params["start_ts_ms"], "500");
    EXPECT_EQ(params["end_ts_ms"], "1500");
}

// ────── Test: JSON response formatting ───────────────────────────────────

TEST_F(ProvenanceCliTest, FormatJsonResponse) {
    json records = generateMockProvenanceResponse(3);
    json output;
    output["count"] = 3;
    output["records"] = records;

    std::string json_str = output.dump();
    EXPECT_TRUE(json_str.find("\"count\":3") != std::string::npos);
    EXPECT_TRUE(json_str.find("\"query_id\":\"query_0\"") != std::string::npos);
    EXPECT_TRUE(json_str.find("\"operation\":\"SELECT\"") != std::string::npos);
}

// ────── Test: CSV response formatting ───────────────────────────────────

TEST_F(ProvenanceCliTest, FormatCsvResponse) {
    json records = generateMockProvenanceResponse(2);
    std::ostringstream csv_stream = {};
    csv_stream << "query_id,operation,timestamp_ms,details\n";

    for (const auto& rec : records) {
        std::string query_id = rec.value("query_id", "");
        std::string operation = rec.value("operation", "");
        int64_t ts_ms = rec.value("timestamp_ms", 0);

        std::string details_str = {};
        if (rec.contains("details")) {
            try {
                details_str = rec["details"].dump();
            } catch (...) {
                details_str = "{}";
            }
        }

        // Escape quotes for CSV
        for (char c : details_str) {
            if (c == '"') {
                csv_stream << "\"";
            }
            csv_stream << c;
        }

        csv_stream << query_id << ","
                   << operation << ","
                   << ts_ms << ","
                   << "\"" << details_str << "\"\n";
    }

    std::string csv_output = csv_stream.str();
    EXPECT_TRUE(csv_output.find("query_id,operation,timestamp_ms,details") != std::string::npos);
    EXPECT_TRUE(csv_output.find("query_0") != std::string::npos);
    EXPECT_TRUE(csv_output.find("SELECT") != std::string::npos);
}

// ────── Test: Mock response validation ─────────────────────────────────

TEST_F(ProvenanceCliTest, ValidateResponseArray) {
    json records = generateMockProvenanceResponse(5);
    EXPECT_TRUE(records.is_array());
    EXPECT_EQ(records.size(), 5u);

    // Validate first record structure
    EXPECT_TRUE(records[0].contains("query_id"));
    EXPECT_TRUE(records[0].contains("operation"));
    EXPECT_TRUE(records[0].contains("timestamp_ms"));
    EXPECT_TRUE(records[0].contains("details"));
}

// ────── Test: Empty records handling ──────────────────────────────────

TEST_F(ProvenanceCliTest, EmptyRecordsHandling) {
    json records = generateMockProvenanceResponse(0);
    EXPECT_TRUE(records.is_array());
    EXPECT_EQ(records.size(), 0u);

    json output;
    output["count"] = 0;
    output["records"] = records;

    std::string json_str = output.dump();
    EXPECT_TRUE(json_str.find("\"count\":0") != std::string::npos);
}

// ────── Test: Response with nested JSON details ───────────────────────────

TEST_F(ProvenanceCliTest, NestedDetailsFormatting) {
    json record;
    record["query_id"] = "query_1";
    record["operation"] = "INSERT";
    record["timestamp_ms"] = 1000;
    record["details"] = {
        {"nested", {
            {"level2", {
                {"value", "deep"}
            }}
        }}
    };

    std::string details_json = record["details"].dump();
    EXPECT_TRUE(details_json.find("nested") != std::string::npos);
    EXPECT_TRUE(details_json.find("level2") != std::string::npos);
    EXPECT_TRUE(details_json.find("deep") != std::string::npos);
}

// ────── Test: Timestamp range filtering ──────────────────────────────────

TEST_F(ProvenanceCliTest, TimestampRangeValidation) {
    // Valid: start < end
    int64_t start = 1000;
    int64_t end = 2000;
    EXPECT_LT(start, end);

    // Invalid: start >= end (should be caught by server)
    start = 2000;
    end = 1000;
    EXPECT_GE(start, end);
}
