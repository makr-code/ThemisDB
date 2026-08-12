/**
 * @file test_streaming_handler.cpp
 * @brief Unit tests for StreamingHandler (SSE / chunked-response formatting)
 *
 * Tests:
 * - formatSseEvent  — correct SSE wire format and JSON payload
 * - formatDoneEvent — correct [DONE] sentinel
 * - formatChunkedData — HTTP chunked-transfer encoding
 * - makeStreamCallback — token delivery via sink, index incrementing,
 *                        null-sink rejection
 *
 * @author ThemisDB Team
 * @date February 2026
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <atomic>
#include <stdexcept>

#include "llm/streaming_handler.h"

using themis::llm::StreamingHandler;

// ═══════════════════════════════════════════════════════════
// Fixture
// ═══════════════════════════════════════════════════════════

class StreamingHandlerTest : public ::testing::Test {
protected:
    const std::string kRequestId{"req-test-001"};
};

// ═══════════════════════════════════════════════════════════
// formatSseEvent
// ═══════════════════════════════════════════════════════════

/**
 * Basic SSE event must start with "data: " and end with double newline.
 */
TEST_F(StreamingHandlerTest, FormatSseEvent_WireFormat) {
    std::string event = StreamingHandler::formatSseEvent("Hello", kRequestId, 0);

    EXPECT_EQ(event.substr(0, 6), "data: ")
        << "SSE events must begin with 'data: '";
    EXPECT_EQ(event.substr(event.size() - 2), "\n\n")
        << "SSE events must end with double newline";
}

/**
 * The JSON payload must contain the expected fields.
 */
TEST_F(StreamingHandlerTest, FormatSseEvent_JsonFields) {
    std::string event = StreamingHandler::formatSseEvent("world", kRequestId, 3);

    EXPECT_NE(event.find("\"id\":\"" + kRequestId + "\""), std::string::npos)
        << "Payload must contain the request id";
    EXPECT_NE(event.find("\"token\":\"world\""), std::string::npos)
        << "Payload must contain the token text";
    EXPECT_NE(event.find("\"index\":3"), std::string::npos)
        << "Payload must contain the zero-based token index";
    EXPECT_NE(event.find("\"done\":false"), std::string::npos)
        << "Non-terminal event must have done=false";
}

/**
 * When done=true the payload must reflect that.
 */
TEST_F(StreamingHandlerTest, FormatSseEvent_DoneFlag) {
    std::string event = StreamingHandler::formatSseEvent("", kRequestId, 5, true);

    EXPECT_NE(event.find("\"done\":true"), std::string::npos)
        << "Terminal token event must have done=true";
}

/**
 * Tokens containing JSON-special characters must be escaped.
 */
TEST_F(StreamingHandlerTest, FormatSseEvent_JsonEscaping) {
    std::string event = StreamingHandler::formatSseEvent(
        "say \"hello\"\nworld\\end", kRequestId, 0);

    EXPECT_NE(event.find("\\\""), std::string::npos)
        << "Double-quote must be escaped as \\\"";
    EXPECT_NE(event.find("\\n"), std::string::npos)
        << "Newline must be escaped as \\n";
    EXPECT_NE(event.find("\\\\"), std::string::npos)
        << "Backslash must be escaped as \\\\";
}

/**
 * Carriage return and tab characters must use their named JSON escapes.
 */
TEST_F(StreamingHandlerTest, FormatSseEvent_JsonEscaping_RAndT) {
    std::string event = StreamingHandler::formatSseEvent(
        "line\r\nnext\tcol", kRequestId, 0);

    EXPECT_NE(event.find("\\r"), std::string::npos)
        << "Carriage return must be escaped as \\r";
    EXPECT_NE(event.find("\\t"), std::string::npos)
        << "Tab must be escaped as \\t";
}

/**
 * Backspace (\\b) and form-feed (\\f) must use their named JSON escapes.
 */
TEST_F(StreamingHandlerTest, FormatSseEvent_JsonEscaping_BAndF) {
    std::string token;
    token += '\x08'; // backspace
    token += '\x0C'; // form feed

    std::string event = StreamingHandler::formatSseEvent(token, kRequestId, 0);

    EXPECT_NE(event.find("\\b"), std::string::npos)
        << "Backspace (0x08) must be escaped as \\b";
    EXPECT_NE(event.find("\\f"), std::string::npos)
        << "Form feed (0x0C) must be escaped as \\f";
}

/**
 * Other C0 control characters (< 0x20 and not named) must be \\uXXXX encoded.
 */
TEST_F(StreamingHandlerTest, FormatSseEvent_JsonEscaping_ControlChars) {
    std::string token;
    token += '\x01'; // SOH — not a named escape

    std::string event = StreamingHandler::formatSseEvent(token, kRequestId, 0);

    EXPECT_NE(event.find("\\u0001"), std::string::npos)
        << "SOH (0x01) must be encoded as \\u0001";
}

/**
 * Empty token produces a valid event without errors.
 */
TEST_F(StreamingHandlerTest, FormatSseEvent_EmptyToken) {
    EXPECT_NO_THROW({
        std::string event = StreamingHandler::formatSseEvent("", kRequestId, 0);
        EXPECT_NE(event.find("\"token\":\"\""), std::string::npos);
    });
}

// ═══════════════════════════════════════════════════════════
// formatDoneEvent
// ═══════════════════════════════════════════════════════════

/**
 * The done sentinel must be the canonical OpenAI-compatible [DONE] line.
 */
TEST_F(StreamingHandlerTest, FormatDoneEvent_SentinelFormat) {
    std::string done = StreamingHandler::formatDoneEvent(kRequestId);

    EXPECT_EQ(done, "data: [DONE]\n\n")
        << "Done event must be exactly 'data: [DONE]\\n\\n'";
}

/**
 * Works with an empty request_id (should not crash).
 */
TEST_F(StreamingHandlerTest, FormatDoneEvent_EmptyRequestId) {
    EXPECT_NO_THROW({
        std::string done = StreamingHandler::formatDoneEvent("");
        EXPECT_EQ(done, "data: [DONE]\n\n");
    });
}

// ═══════════════════════════════════════════════════════════
// formatChunkedData
// ═══════════════════════════════════════════════════════════

/**
 * Non-empty data chunk must follow hex-len\r\ndata\r\n pattern.
 */
TEST_F(StreamingHandlerTest, FormatChunkedData_NonEmpty) {
    std::string chunk = StreamingHandler::formatChunkedData("hello");

    // "hello" is 5 bytes → hex "5"
    EXPECT_EQ(chunk, "5\r\nhello\r\n");
}

/**
 * Empty string produces the terminal zero-length chunk.
 */
TEST_F(StreamingHandlerTest, FormatChunkedData_TerminalChunk) {
    std::string terminal = StreamingHandler::formatChunkedData("");

    EXPECT_EQ(terminal, "0\r\n\r\n")
        << "Empty input must produce the HTTP terminal chunk '0\\r\\n\\r\\n'";
}

/**
 * Length field must be the correct hex representation.
 */
TEST_F(StreamingHandlerTest, FormatChunkedData_HexLength) {
    // 16 bytes → hex "10"
    std::string data(16, 'x');
    std::string chunk = StreamingHandler::formatChunkedData(data);

    EXPECT_EQ(chunk.substr(0, 4), "10\r\n")
        << "Chunk length for 16 bytes must be '10' in hex";
}

// ═══════════════════════════════════════════════════════════
// makeStreamCallback
// ═══════════════════════════════════════════════════════════

/**
 * Each token is forwarded to the sink as a valid SSE event.
 */
TEST_F(StreamingHandlerTest, MakeStreamCallback_DeliverTokens) {
    std::vector<std::string> received;

    auto cb = StreamingHandler::makeStreamCallback(
        [&received](const std::string& evt) { received.push_back(evt); },
        kRequestId);

    cb("token_a");
    cb("token_b");
    cb("token_c");

    ASSERT_EQ(received.size(), 3u);
    for (const auto& evt : received) {
        EXPECT_EQ(evt.substr(0, 6), "data: ");
        EXPECT_EQ(evt.substr(evt.size() - 2), "\n\n");
    }
}

/**
 * Token index increments monotonically starting from zero.
 */
TEST_F(StreamingHandlerTest, MakeStreamCallback_IndexIncrements) {
    std::vector<std::string> received;

    auto cb = StreamingHandler::makeStreamCallback(
        [&received](const std::string& evt) { received.push_back(evt); },
        kRequestId);

    cb("a");
    cb("b");
    cb("c");

    ASSERT_EQ(received.size(), 3u);
    EXPECT_NE(received[0].find("\"index\":0"), std::string::npos);
    EXPECT_NE(received[1].find("\"index\":1"), std::string::npos);
    EXPECT_NE(received[2].find("\"index\":2"), std::string::npos);
}

/**
 * Events embed the correct request_id.
 */
TEST_F(StreamingHandlerTest, MakeStreamCallback_RequestIdPropagated) {
    std::string captured;

    auto cb = StreamingHandler::makeStreamCallback(
        [&captured](const std::string& evt) { captured = evt; },
        "my-req-42");

    cb("tok");

    EXPECT_NE(captured.find("\"id\":\"my-req-42\""), std::string::npos)
        << "Request id must appear in every SSE event";
}

/**
 * A null sink must be rejected immediately at construction time.
 */
TEST_F(StreamingHandlerTest, MakeStreamCallback_NullSinkThrows) {
    EXPECT_THROW(
        StreamingHandler::makeStreamCallback(nullptr, kRequestId),
        std::invalid_argument);
}

/**
 * Callback with empty request_id still functions correctly.
 */
TEST_F(StreamingHandlerTest, MakeStreamCallback_EmptyRequestId) {
    std::string captured;

    auto cb = StreamingHandler::makeStreamCallback(
        [&captured](const std::string& evt) { captured = evt; },
        "");

    EXPECT_NO_THROW(cb("tok"));
    EXPECT_NE(captured.find("\"id\":\"\""), std::string::npos);
}
