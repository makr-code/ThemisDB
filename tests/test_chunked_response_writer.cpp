#include <gtest/gtest.h>
#include "server/chunked_response_writer.h"
#include "query/result_stream.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <sstream>

using namespace themis::server;
using namespace themis::query;
using json = nlohmann::json;

namespace beast = boost::beast;
namespace http  = beast::http;

// ----------------------------------------------------------------------------
// Helper: build a minimal HTTP/1.1 request
// ----------------------------------------------------------------------------
static http::request<http::string_body> makeRequest(unsigned version = 11) {
    http::request<http::string_body> req{http::verb::post, "/query", version};
    req.set(http::field::host, "localhost");
    req.set(http::field::content_type, "application/json");
    req.keep_alive(true);
    req.body() = "{}";
    req.prepare_payload();
    return req;
}

// ----------------------------------------------------------------------------
// Helper: decode RFC 7230 chunked body back to plain text
// ----------------------------------------------------------------------------
static std::string decodeChunked(const std::string& body) {
    std::string result;
    size_t pos = 0;
    while (pos < body.size()) {
        // Find CRLF after hex size
        size_t crlf = body.find("\r\n", pos);
        EXPECT_NE(crlf, std::string::npos) << "Malformed chunk: missing size CRLF";
        if (crlf == std::string::npos) {
          break;
        }

        std::string size_str = body.substr(pos, crlf - pos);
        size_t chunk_size = std::stoul(size_str, nullptr, 16);
        pos = crlf + 2;

        if (chunk_size == 0) {
            // terminal chunk
            break;
        }

        EXPECT_GE(body.size(), pos + chunk_size + 2) << "Truncated chunk data";
        result += body.substr(pos, chunk_size);
        pos += chunk_size + 2; // skip data + CRLF
    }
    return result;
}

// ============================================================================
// encodeChunkedBody
// ============================================================================

TEST(ChunkedResponseWriterTest, EncodeEmptyFragmentsProducesTerminator) {
    auto body = ChunkedResponseWriter::encodeChunkedBody({});
    EXPECT_EQ(body, "0\r\n\r\n");
}

TEST(ChunkedResponseWriterTest, EncodeSingleFragment) {
    std::vector<std::string> frags = {"hello"};
    auto body = ChunkedResponseWriter::encodeChunkedBody(frags);

    // Expected: "5\r\nhello\r\n0\r\n\r\n"
    EXPECT_EQ(body, "5\r\nhello\r\n0\r\n\r\n");
    EXPECT_EQ(decodeChunked(body), "hello");
}

TEST(ChunkedResponseWriterTest, EncodeMultipleFragments) {
    std::vector<std::string> frags = {"foo", "bar", "baz"};
    auto body = ChunkedResponseWriter::encodeChunkedBody(frags);

    EXPECT_EQ(decodeChunked(body), "foobarbaz");
}

TEST(ChunkedResponseWriterTest, EmptyFragmentsAreSkipped) {
    std::vector<std::string> frags = {"a", "", "b"};
    auto body = ChunkedResponseWriter::encodeChunkedBody(frags);

    // Empty string must not produce a 0-sized intermediate chunk
    EXPECT_EQ(decodeChunked(body), "ab");
    // The body should NOT contain "0\r\n" before the terminal chunk for empty fragments
    // (all chunks before terminator must have size > 0)
    size_t first_zero = body.find("0\r\n");
    // last occurrence should be the terminal chunk
    size_t last_zero  = body.rfind("0\r\n");
    EXPECT_EQ(first_zero, last_zero);
}

TEST(ChunkedResponseWriterTest, EncodePreservesNewlines) {
    std::vector<std::string> frags = {"{\"k\":1}\n{\"k\":2}\n"};
    auto body = ChunkedResponseWriter::encodeChunkedBody(frags);

    EXPECT_EQ(decodeChunked(body), "{\"k\":1}\n{\"k\":2}\n");
}

// ============================================================================
// fromFragments
// ============================================================================

TEST(ChunkedResponseWriterTest, FromFragmentsSetsTransferEncoding) {
    auto req = makeRequest();
    auto res = ChunkedResponseWriter::fromFragments(req, http::status::ok, {"data"});

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::transfer_encoding], "chunked");
    EXPECT_FALSE(res.count(http::field::content_length));
}

TEST(ChunkedResponseWriterTest, FromFragmentsBodyIsChunkEncoded) {
    auto req = makeRequest();
    std::vector<std::string> frags = {"abc", "def"};
    auto res = ChunkedResponseWriter::fromFragments(req, http::status::ok, frags);

    EXPECT_EQ(decodeChunked(res.body()), "abcdef");
}

TEST(ChunkedResponseWriterTest, FromFragmentsRespectContentType) {
    auto req = makeRequest();
    auto res = ChunkedResponseWriter::fromFragments(
        req, http::status::ok, {"x"}, "text/plain");

    EXPECT_EQ(res[http::field::content_type], "text/plain");
}

TEST(ChunkedResponseWriterTest, FromFragmentsHttp10ReturnsResponse) {
    // Even for HTTP/1.0 requests, fromFragments still encodes – callers are
    // expected to call shouldUseChunkedTransfer() before deciding.
    auto req = makeRequest(10);
    auto res = ChunkedResponseWriter::fromFragments(req, http::status::ok, {"data"});
    EXPECT_EQ(res.result(), http::status::ok);
}

// ============================================================================
// fromJsonVector
// ============================================================================

TEST(ChunkedResponseWriterTest, FromJsonVectorEmpty) {
    auto req = makeRequest();
    std::vector<json> items;
    auto res = ChunkedResponseWriter::fromJsonVector(req, http::status::ok, items);

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::transfer_encoding], "chunked");
    // Terminal chunk only
    EXPECT_EQ(res.body(), "0\r\n\r\n");
}

TEST(ChunkedResponseWriterTest, FromJsonVectorSingleItem) {
    auto req = makeRequest();
    std::vector<json> items = {json{{"key", "value"}}};
    auto res = ChunkedResponseWriter::fromJsonVector(req, http::status::ok, items);

    std::string decoded = decodeChunked(res.body());
    // Decoded should be one NDJSON line
    EXPECT_FALSE(decoded.empty());
    json parsed = json::parse(decoded.substr(0, decoded.find('\n')));
    EXPECT_EQ(parsed["key"], "value");
}

TEST(ChunkedResponseWriterTest, FromJsonVectorChunkBatching) {
    auto req = makeRequest();

    // 10 items with chunk_size = 3 → 4 chunks (3+3+3+1)
    std::vector<json> items;
    for (int i = 0; i < 10; ++i) {
        items.push_back({{"i", i}});
    }

    ChunkedWriterConfig cfg;
    cfg.chunk_size = 3;

    auto res = ChunkedResponseWriter::fromJsonVector(req, http::status::ok, items, cfg);

    // Decode and count NDJSON lines
    std::string decoded = decodeChunked(res.body());
    std::istringstream iss(decoded);
    std::string line;
    int line_count = 0;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
          ++line_count;
        }
    }
    EXPECT_EQ(line_count, 10);
}

TEST(ChunkedResponseWriterTest, FromJsonVectorMaxItemsLimit) {
    auto req = makeRequest();

    std::vector<json> items;
    for (int i = 0; i < 20; ++i) {
        items.push_back({{"i", i}});
    }

    ChunkedWriterConfig cfg;
    cfg.max_items = 5;

    auto res = ChunkedResponseWriter::fromJsonVector(req, http::status::ok, items, cfg);
    std::string decoded = decodeChunked(res.body());

    // Only 5 items should appear
    std::istringstream iss(decoded);
    std::string line;
    int line_count = 0;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
          ++line_count;
        }
    }
    EXPECT_EQ(line_count, 5);
}

// ============================================================================
// fromStream
// ============================================================================

TEST(ChunkedResponseWriterTest, FromStreamBasic) {
    auto req = makeRequest();

    // Build a materialized ResultStream<json>
    std::vector<json> data;
    for (int i = 0; i < 5; ++i) {
        data.push_back({{"n", i}});
    }
    auto stream = std::make_shared<ResultStream<json>>(data);

    auto res = ChunkedResponseWriter::fromStream(req, http::status::ok, stream);

    EXPECT_EQ(res[http::field::transfer_encoding], "chunked");
    std::string decoded = decodeChunked(res.body());

    std::istringstream iss(decoded);
    std::string line;
    int count = 0;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
          ++count;
        }
    }
    EXPECT_EQ(count, 5);
}

TEST(ChunkedResponseWriterTest, FromStreamEmpty) {
    auto req = makeRequest();

    std::vector<json> data;
    auto stream = std::make_shared<ResultStream<json>>(data);

    auto res = ChunkedResponseWriter::fromStream(req, http::status::ok, stream);

    EXPECT_EQ(res[http::field::transfer_encoding], "chunked");
    EXPECT_EQ(res.body(), "0\r\n\r\n");
}

TEST(ChunkedResponseWriterTest, FromStreamMaxItems) {
    auto req = makeRequest();

    std::vector<json> data;
    for (int i = 0; i < 100; ++i) {
        data.push_back({{"x", i}});
    }
    auto stream = std::make_shared<ResultStream<json>>(data);

    ChunkedWriterConfig cfg;
    cfg.max_items = 10;
    cfg.chunk_size = 5;

    auto res = ChunkedResponseWriter::fromStream(req, http::status::ok, stream, cfg);
    std::string decoded = decodeChunked(res.body());

    std::istringstream iss(decoded);
    std::string line;
    int count = 0;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
          ++count;
        }
    }
    EXPECT_EQ(count, 10);
}

// ============================================================================
// shouldUseChunkedTransfer
// ============================================================================

TEST(ChunkedResponseWriterTest, ShouldUseChunkedForLargeResultHttp11) {
    auto req = makeRequest(11);
    EXPECT_TRUE(ChunkedResponseWriter::shouldUseChunkedTransfer(req, 1001, 1000));
    EXPECT_FALSE(ChunkedResponseWriter::shouldUseChunkedTransfer(req, 999, 1000));
    EXPECT_FALSE(ChunkedResponseWriter::shouldUseChunkedTransfer(req, 1000, 1001));
}

TEST(ChunkedResponseWriterTest, ShouldNotUseChunkedForHttp10) {
    auto req = makeRequest(10);
    // HTTP/1.0 clients do not support chunked transfer
    EXPECT_FALSE(ChunkedResponseWriter::shouldUseChunkedTransfer(req, 1000000, 1));
}

TEST(ChunkedResponseWriterTest, ShouldUseChunkedAtExactThreshold) {
    auto req = makeRequest(11);
    EXPECT_TRUE(ChunkedResponseWriter::shouldUseChunkedTransfer(req, 1000, 1000));
}

TEST(ChunkedResponseWriterTest, CustomThreshold) {
    auto req = makeRequest(11);
    EXPECT_TRUE(ChunkedResponseWriter::shouldUseChunkedTransfer(req, 50, 50));
    EXPECT_FALSE(ChunkedResponseWriter::shouldUseChunkedTransfer(req, 49, 50));
}

// ============================================================================
// decodeChunkedBody — round-trip tests
// ============================================================================

TEST(ChunkedResponseWriterTest, DecodeEmptyBody) {
    EXPECT_EQ(ChunkedResponseWriter::decodeChunkedBody(""), "");
}

TEST(ChunkedResponseWriterTest, DecodeTerminatorOnly) {
    EXPECT_EQ(ChunkedResponseWriter::decodeChunkedBody("0\r\n\r\n"), "");
}

TEST(ChunkedResponseWriterTest, DecodeSingleChunk) {
    // Manually encoded single chunk
    std::string encoded = "5\r\nhello\r\n0\r\n\r\n";
    EXPECT_EQ(ChunkedResponseWriter::decodeChunkedBody(encoded), "hello");
}

TEST(ChunkedResponseWriterTest, DecodeMultipleChunks) {
    std::string encoded = "3\r\nfoo\r\n3\r\nbar\r\n0\r\n\r\n";
    EXPECT_EQ(ChunkedResponseWriter::decodeChunkedBody(encoded), "foobar");
}

TEST(ChunkedResponseWriterTest, EncodeDecodeRoundTrip) {
    std::vector<std::string> frags = {"{\"a\":1}\n", "{\"b\":2}\n", "{\"c\":3}\n"};
    std::string encoded = ChunkedResponseWriter::encodeChunkedBody(frags);
    std::string decoded = ChunkedResponseWriter::decodeChunkedBody(encoded);
    EXPECT_EQ(decoded, "{\"a\":1}\n{\"b\":2}\n{\"c\":3}\n");
}

TEST(ChunkedResponseWriterTest, DecodePreservesContent) {
    // Build via fromJsonVector, then decode – validates the full pipeline
    auto req = makeRequest();
    std::vector<json> items;
    for (int i = 0; i < 5; ++i) {
        items.push_back({{"v", i}});
    }
    auto res = ChunkedResponseWriter::fromJsonVector(req, http::status::ok, items);
    std::string decoded = ChunkedResponseWriter::decodeChunkedBody(res.body());

    // Should be 5 NDJSON lines
    std::istringstream iss(decoded);
    std::string line;
    int count = 0;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            json parsed = json::parse(line);
            EXPECT_EQ(parsed["v"], count);
            ++count;
        }
    }
    EXPECT_EQ(count, 5);
}
