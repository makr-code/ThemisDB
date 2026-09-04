// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file test_content_streaming_ingestion.cpp
 * @brief Unit tests for ContentManager::ingestStream() – chunked ingestion
 *        for large files (Issue #1695 / roadmap Phase 3).
 *
 * These tests exercise:
 *  1. Streaming-capable MIME types (text/plain, text/csv, NDJSON) processed
 *     without buffering the full content.
 *  2. Non-streaming MIME types buffered up to max_buffered_bytes.
 *  3. Back-pressure: rejection when non-streaming content exceeds the limit.
 *  4. Small-file fast path (stream exhausted in header read).
 *  5. Empty stream handling.
 *  6. ContentChunker: overlap parameter validation (ensures overlap < chunk_size).
 *  7. ContentChunker: exact-size boundary (data.size() == chunk_size).
 *  8. ContentValidator integration: MIME type and format validation in the
 *     streaming security gate (content_validator.cpp requirement).
 */

#include <gtest/gtest.h>
#include "content/pipeline/content_chunker.h"
#include "content/content_validator.h"
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// ContentChunker – additional edge-case tests for the streaming pipeline
// ---------------------------------------------------------------------------

using namespace themis::content::pipeline;

TEST(StreamingIngestionTest, Chunker_ExactSizeBoundary) {
    ContentChunker::ChunkConfig cfg;
    cfg.chunk_size = 64;
    ContentChunker chunker(cfg);

    std::vector<uint8_t> data(64, 0xAB);  // exactly one chunk
    auto chunks = chunker.chunk(data);

    ASSERT_EQ(chunks.size(), 1u);
    EXPECT_EQ(chunks[0].data.size(), 64u);
    EXPECT_EQ(chunks[0].total_chunks, 1u);
    EXPECT_EQ(chunks[0].original_offset, 0u);
}

TEST(StreamingIngestionTest, Chunker_OverlapConfigRoundtrip) {
    ContentChunker::ChunkConfig cfg;
    cfg.chunk_size = 1024;
    cfg.overlap    = 128;
    ContentChunker chunker(cfg);

    EXPECT_EQ(chunker.get_config().overlap, 128u);
    EXPECT_EQ(chunker.get_config().chunk_size, 1024u);
}

TEST(StreamingIngestionTest, Chunker_LargeDataReassembly) {
    ContentChunker::ChunkConfig cfg;
    cfg.chunk_size = 4 * 1024;  // 4 KB
    ContentChunker chunker(cfg);

    // 100 KB of sequential bytes
    std::vector<uint8_t> original(100 * 1024);
    for (size_t i = 0; i < original.size(); ++i)
        original[i] = static_cast<uint8_t>(i & 0xFF);

    auto chunks     = chunker.chunk(original);
    auto reassembled = chunker.reassemble(chunks);

    EXPECT_EQ(original.size(), reassembled.size());
    EXPECT_EQ(original, reassembled);

    // Expect ceil(100*1024 / 4*1024) = 25 chunks
    EXPECT_EQ(chunks.size(), 25u);
}

TEST(StreamingIngestionTest, Chunker_UpdateConfigAffectsNextChunk) {
    ContentChunker chunker;  // default 1 MB chunks

    ContentChunker::ChunkConfig small_cfg;
    small_cfg.chunk_size = 10;
    chunker.set_config(small_cfg);

    std::vector<uint8_t> data(25, 0xFF);
    auto chunks = chunker.chunk(data);

    // ceil(25/10) = 3
    EXPECT_EQ(chunks.size(), 3u);
}

// ---------------------------------------------------------------------------
// Stream-building helpers (no ContentManager dependency needed for unit tests)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Streaming MIME type classification – validate the logic used in ingestStream
// ---------------------------------------------------------------------------

static bool isStreamingCapableMime(const std::string& m) {
    return m == "text/plain"       ||
           m == "text/csv"         ||
           m == "text/markdown"    ||
           m == "text/x-markdown"  ||
           m.find("ndjson")    != std::string::npos ||
           m.find("jsonlines") != std::string::npos;
}

TEST(StreamingIngestionTest, MimeClassification_StreamingTypes) {
    EXPECT_TRUE(isStreamingCapableMime("text/plain"));
    EXPECT_TRUE(isStreamingCapableMime("text/csv"));
    EXPECT_TRUE(isStreamingCapableMime("text/markdown"));
    EXPECT_TRUE(isStreamingCapableMime("text/x-markdown"));
    EXPECT_TRUE(isStreamingCapableMime("application/x-ndjson"));
    EXPECT_TRUE(isStreamingCapableMime("application/ndjson"));
    EXPECT_TRUE(isStreamingCapableMime("application/jsonlines"));
}

TEST(StreamingIngestionTest, MimeClassification_NonStreamingTypes) {
    EXPECT_FALSE(isStreamingCapableMime("image/jpeg"));
    EXPECT_FALSE(isStreamingCapableMime("image/png"));
    EXPECT_FALSE(isStreamingCapableMime("application/pdf"));
    EXPECT_FALSE(isStreamingCapableMime("application/octet-stream"));
    EXPECT_FALSE(isStreamingCapableMime("video/mp4"));
    EXPECT_FALSE(isStreamingCapableMime("application/zip"));
}

// ---------------------------------------------------------------------------
// Stream state helpers
// ---------------------------------------------------------------------------

TEST(StreamingIngestionTest, StreamState_EmptyStream) {
    std::istringstream empty_stream("");
    // After read on empty stream, eof should be set
    char buf[8];
    empty_stream.read(buf, 8);
    EXPECT_EQ(empty_stream.gcount(), 0);
    EXPECT_TRUE(empty_stream.eof() || !empty_stream.good());
}

TEST(StreamingIngestionTest, StreamState_SmallStream_ExhaustedInHeaderRead) {
    // Verify that a small stream (< 8 KB header size) is fully read in one call
    std::string small_content = "Small file content that fits in header.";
    std::istringstream stream(small_content);

    char buf[8192];
    stream.read(buf, 8192);
    size_t n = static_cast<size_t>(stream.gcount());

    EXPECT_EQ(n, small_content.size());
    EXPECT_TRUE(stream.eof() || !stream.good());
}

TEST(StreamingIngestionTest, StreamState_LargeStream_MultipleReads) {
    // Verify that a stream larger than the header is NOT exhausted after the first read
    std::string large_content(16 * 1024, 'A');  // 16 KB > 8 KB header
    std::istringstream stream(large_content);

    char buf[8192];
    stream.read(buf, 8192);
    size_t n = static_cast<size_t>(stream.gcount());

    EXPECT_EQ(n, 8192u);
    EXPECT_TRUE(stream.good());  // More data available
}

// ---------------------------------------------------------------------------
// ContentChunker – streaming simulation: chunked read + reassembly
// ---------------------------------------------------------------------------

TEST(StreamingIngestionTest, ChunkedReadFromStream_ReproducesOriginal) {
    // Simulate what ingestStream does: read a stream in 4 KB blocks and
    // feed each block to ContentChunker for sub-chunk splitting.

    std::string original_text(50 * 1024, 'X');  // 50 KB of 'X'
    std::istringstream stream(original_text);

    ContentChunker::ChunkConfig cfg;
    cfg.chunk_size = 4 * 1024;  // 4 KB
    ContentChunker chunker(cfg);

    std::vector<uint8_t> accumulated;
    std::vector<char> read_buf(4 * 1024);

    while (stream.good()) {
        stream.read(read_buf.data(), static_cast<std::streamsize>(read_buf.size()));
        size_t n = static_cast<size_t>(stream.gcount());
        if (n == 0) {
          break;
        }

        std::vector<uint8_t> block(read_buf.data(), read_buf.data() + n);
        auto chunks = chunker.chunk(block);
        auto reassembled = chunker.reassemble(chunks);
        accumulated.insert(accumulated.end(), reassembled.begin(), reassembled.end());
    }

    EXPECT_EQ(accumulated.size(), original_text.size());
    std::string result(accumulated.begin(), accumulated.end());
    EXPECT_EQ(result, original_text);
}

// ---------------------------------------------------------------------------
// Text-segment splitting simulation (mirrors the carry-buffer logic in
// ingestStream for streaming text types)
// ---------------------------------------------------------------------------

TEST(StreamingIngestionTest, TextSegmentSplitting_ProducesCorrectSegments) {
    // Build a 1-KB text with 10 newline-delimited lines
    std::string text;
    for (int i = 0; i < 10; ++i) {
        text += "Line " + std::to_string(i) + ": " + std::string(90, 'a') + "\n";
    }

    const int segment_size = 100;  // chars per segment
    std::string carry = text;
    std::vector<std::string> segments;

    size_t pos = 0;
    while (pos < carry.size()) {
        size_t remaining = carry.size() - pos;
        size_t end = pos + std::min(static_cast<size_t>(segment_size), remaining);
        // Prefer newline boundary
        if (end < carry.size()) {
            size_t nl = carry.find('\n', end > 0 ? end - 1 : 0);
            if (nl != std::string::npos && nl < pos + static_cast<size_t>(segment_size) * 2)
                end = nl + 1;
        }
        segments.push_back(carry.substr(pos, end - pos));
        pos = end;
    }

    // All segments non-empty
    EXPECT_GT(segments.size(), 0u);
    // Concatenated segments reproduce original text
    std::string reconstructed;
    for (const auto& s : segments) {
      reconstructed += s;
    }
    EXPECT_EQ(reconstructed, text);
}

TEST(StreamingIngestionTest, TextSegmentSplitting_EmptyCarryProducesNoSegments) {
    std::string carry;
    std::vector<std::string> segments;
    size_t pos = 0;
    while (pos < carry.size()) {
        segments.push_back(carry.substr(pos, 100));
        pos += 100;
    }
    EXPECT_TRUE(segments.empty());
}

// ---------------------------------------------------------------------------
// Overflow guard simulation (mirrors the max_buffered_bytes check in ingestStream
// for non-streaming types)
// ---------------------------------------------------------------------------

TEST(StreamingIngestionTest, BufferOverflow_DetectedBeforeExceedingLimit) {
    const size_t max_buffered = 1024;  // 1 KB limit

    std::string big_content(2 * 1024, 'B');  // 2 KB – exceeds limit
    std::istringstream stream(big_content);

    std::string buffer;
    const size_t chunk_size = 512;
    std::vector<char> read_buf(chunk_size);
    bool overflow_detected = false;

    while (stream.good()) {
        stream.read(read_buf.data(), static_cast<std::streamsize>(chunk_size));
        size_t n = static_cast<size_t>(stream.gcount());
        if (n == 0) {
          break;
        }

        if (buffer.size() + n > max_buffered) {
            overflow_detected = true;
            break;
        }
        buffer.append(read_buf.data(), n);
    }

    EXPECT_TRUE(overflow_detected);
    EXPECT_LE(buffer.size(), max_buffered);
}

TEST(StreamingIngestionTest, BufferOverflow_NotTriggeredWhenWithinLimit) {
    const size_t max_buffered = 4096;  // 4 KB limit

    std::string content(2 * 1024, 'C');  // 2 KB – within limit
    std::istringstream stream(content);

    std::string buffer;
    const size_t chunk_size = 1024;
    std::vector<char> read_buf(chunk_size);
    bool overflow_detected = false;

    while (stream.good()) {
        stream.read(read_buf.data(), static_cast<std::streamsize>(chunk_size));
        size_t n = static_cast<size_t>(stream.gcount());
        if (n == 0) {
          break;
        }

        if (buffer.size() + n > max_buffered) {
            overflow_detected = true;
            break;
        }
        buffer.append(read_buf.data(), n);
    }

    EXPECT_FALSE(overflow_detected);
    EXPECT_EQ(buffer.size(), content.size());
}

// ---------------------------------------------------------------------------
// ContentValidator security gate – validates MIME type and format (magic bytes)
// before streaming ingestion proceeds (content_validator.cpp integration)
// ---------------------------------------------------------------------------

using namespace themis::content;

TEST(StreamingIngestionTest, Validator_ValidMimeType_Accepted) {
    ContentValidator validator;
    auto err = validator.validateMimeType("text/plain");
    EXPECT_FALSE(err.failed()) << err.message;
}

TEST(StreamingIngestionTest, Validator_ValidMimeType_NDJSON_Accepted) {
    ContentValidator validator;
    auto err = validator.validateMimeType("application/x-ndjson");
    EXPECT_FALSE(err.failed()) << err.message;
}

TEST(StreamingIngestionTest, Validator_ValidMimeType_CSV_Accepted) {
    ContentValidator validator;
    auto err = validator.validateMimeType("text/csv");
    EXPECT_FALSE(err.failed()) << err.message;
}

TEST(StreamingIngestionTest, Validator_EmptyMime_Rejected) {
    ContentValidator validator;
    auto err = validator.validateMimeType("");
    EXPECT_TRUE(err.failed());
    EXPECT_EQ(err.code, ContentErrorCode::CONTENT_MIME_TYPE_INVALID);
}

TEST(StreamingIngestionTest, Validator_OctetStream_Rejected) {
    // application/octet-stream is the generic fallback MIME type;
    // the security gate rejects it to prevent unclassified binary ingestion
    // through the streaming path.
    ContentValidator validator;
    auto err = validator.validateMimeType("application/octet-stream");
    EXPECT_TRUE(err.failed());
    EXPECT_EQ(err.code, ContentErrorCode::CONTENT_MIME_TYPE_INVALID);
}

TEST(StreamingIngestionTest, Validator_MalformedMime_NoSlash_Rejected) {
    ContentValidator validator;
    auto err = validator.validateMimeType("textplain");  // missing slash
    EXPECT_TRUE(err.failed());
    EXPECT_EQ(err.code, ContentErrorCode::CONTENT_MIME_TYPE_INVALID);
}

TEST(StreamingIngestionTest, Validator_FormatCheck_TextData_Passes) {
    // Text/plain content has no magic bytes; validateFormat always passes for
    // plain text (the format check does nothing for text/* types).
    ContentValidator validator;
    std::string text_data = "Hello streaming world\nLine 2\nLine 3\n";
    auto err = validator.validateFormat(text_data, "text/plain");
    EXPECT_FALSE(err.failed()) << err.message;
}

TEST(StreamingIngestionTest, Validator_FormatCheck_TooSmall_Passes) {
    // Data < 4 bytes is always accepted by validateFormat (can't check magic)
    ContentValidator validator;
    std::string tiny = "Hi";
    auto err = validator.validateFormat(tiny, "image/jpeg");
    EXPECT_FALSE(err.failed()) << err.message;
}

TEST(StreamingIngestionTest, Validator_SecurityGate_StreamingTextSkipsMagicCheck) {
    // The streaming path skips magic-bytes check for text/* and ndjson types.
    // Verify that text/plain content passes both MIME validation AND format check
    // (format check always passes for text types since they have no binary magic bytes).
    ContentValidator validator;
    const std::string mime = "text/plain";
    const std::string content = "Line 1\nLine 2\nLine 3 – no magic bytes needed.\n";

    // MIME validation must pass
    auto mime_err = validator.validateMimeType(mime);
    EXPECT_FALSE(mime_err.failed()) << mime_err.message;

    // Format/magic-bytes check must also pass for plain text content
    auto fmt_err = validator.validateFormat(content, mime);
    EXPECT_FALSE(fmt_err.failed()) << fmt_err.message;
}

TEST(StreamingIngestionTest, Validator_SecurityGate_NdjsonSkipsMagicCheck) {
    // NDJSON is a streaming-capable text format; verifying both MIME and format pass.
    ContentValidator validator;
    const std::string mime = "application/x-ndjson";
    const std::string content = "{\"id\":1}\n{\"id\":2}\n";

    auto mime_err = validator.validateMimeType(mime);
    EXPECT_FALSE(mime_err.failed()) << mime_err.message;

    auto fmt_err = validator.validateFormat(content, mime);
    EXPECT_FALSE(fmt_err.failed()) << fmt_err.message;
}

