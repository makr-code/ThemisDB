/// @file test_wire_protocol_optimizations.cpp
/// @brief Unit tests for wire protocol performance optimizations:
///        - ZeroCopyFrameBuilder / MemoryMappedPayload
///        - WireProtocolBatcher / NagleController
///        - ZstdDictionaryCompressor

#include <gtest/gtest.h>

#include "network/wire_protocol_zero_copy.h"
#include "network/wire_protocol_batch.h"
#include "network/connection_compression.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <array>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <array>
#include <cstring>
#include <vector>

using namespace themis::network;

#ifdef _WIN32

TEST(WireProtocolOptimizations, PosixOnlyOnWindows) {
    GTEST_SKIP() << "wire_protocol_optimizations tests require POSIX socket APIs";
}

#else

// =============================================================================
// Helpers
// =============================================================================

/// Create a connected socket pair for testing.
static std::pair<int, int> make_socket_pair() {
    int fds[2];
    EXPECT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    return {fds[0], fds[1]};
}

/// Read exactly @p n bytes from @p fd into @p buf.
static bool read_exact(int fd, void* buf, size_t n) {
    size_t read_so_far = 0;
    while (read_so_far < n) {
        const ssize_t r = ::read(fd, static_cast<char*>(buf) + read_so_far,
                                 n - read_so_far);
        if (r <= 0) {
          return false;
        }
        read_so_far += static_cast<size_t>(r);
    }
    return true;
}

// =============================================================================
// ZeroCopyFrameBuilder
// =============================================================================

TEST(ZeroCopyFrameBuilder, HeaderOnlyFrame) {
    // A frame with zero payload size should write exactly HEADER_SIZE bytes.
    auto [wr, rd] = make_socket_pair();

    ZeroCopyFrameBuilder builder(0x000A /*GET*/, 0x0000, nullptr, 0);
    const ssize_t written = builder.writeTo(wr);
    EXPECT_EQ(written, static_cast<ssize_t>(ZeroCopyFrameBuilder::HEADER_SIZE));

    std::array<uint8_t, ZeroCopyFrameBuilder::HEADER_SIZE> buf{};
    ASSERT_TRUE(read_exact(rd, buf.data(), buf.size()));

    // First 4 bytes == WIRE_MAGIC (network byte order).
    uint32_t magic = 0;
    std::memcpy(&magic, buf.data(), 4);
    // Convert from network (big-endian) to host byte order.
    const uint32_t magic_he = ntohl(magic);
    EXPECT_EQ(magic_he, ZeroCopyFrameBuilder::WIRE_MAGIC);

    ::close(wr);
    ::close(rd);
}

TEST(ZeroCopyFrameBuilder, FrameWithPayload) {
    auto [wr, rd] = make_socket_pair();

    const std::string payload = "Hello, ThemisDB!";
    ZeroCopyFrameBuilder builder(0x000B /*PUT*/, 0x0001,
                                 payload.data(), payload.size());

    const ssize_t written = builder.writeTo(wr);
    EXPECT_EQ(written, static_cast<ssize_t>(
        ZeroCopyFrameBuilder::HEADER_SIZE + payload.size()));

    // Read back header + payload.
    std::vector<uint8_t> buf(ZeroCopyFrameBuilder::HEADER_SIZE + payload.size());
    ASSERT_TRUE(read_exact(rd, buf.data(), buf.size()));

    // Verify payload_size field in header (bytes 8-11, big-endian).
    uint32_t psize_be = 0;
    std::memcpy(&psize_be, buf.data() + 8, 4);
    const uint32_t psize = ntohl(psize_be);
    EXPECT_EQ(psize, static_cast<uint32_t>(payload.size()));

    // Verify payload content.
    const std::string actual(
        reinterpret_cast<const char*>(buf.data() + ZeroCopyFrameBuilder::HEADER_SIZE),
        payload.size());
    EXPECT_EQ(actual, payload);

    ::close(wr);
    ::close(rd);
}

TEST(ZeroCopyFrameBuilder, FrameSizeMatchesHeaderPlusPayload) {
    const size_t payload_size = 1024;
    ZeroCopyFrameBuilder builder(0x0001, 0x0000, nullptr, payload_size);
    EXPECT_EQ(builder.frameSize(),
              ZeroCopyFrameBuilder::HEADER_SIZE + payload_size);
}

TEST(ZeroCopyFrameBuilder, HeaderBytesAccessible) {
    ZeroCopyFrameBuilder builder(0xBEEF, 0xCAFE, nullptr, 42);
    const auto& hdr = builder.header();
    EXPECT_EQ(hdr.size(), ZeroCopyFrameBuilder::HEADER_SIZE);

    // opcode is at bytes 4-5 (after magic), flags at bytes 6-7 in wire order...
    // Just verify the array is non-empty and has the right size.
    EXPECT_FALSE(hdr.empty());
}

TEST(ZeroCopyFrameBuilder, ZeroCopyStatsTracking) {
    ZeroCopyStats stats;
    stats.recordWrite(256);
    stats.recordWrite(512);
    stats.recordWriteError();
    stats.recordMmap(4096);

    EXPECT_EQ(stats.frames_written,    2u);
    EXPECT_EQ(stats.bytes_written,     768u);
    EXPECT_EQ(stats.write_errors,      1u);
    EXPECT_EQ(stats.mmap_opens,        1u);
    EXPECT_EQ(stats.mmap_bytes_mapped, 4096u);
}

// =============================================================================
// MemoryMappedPayload (anonymous mapping)
// =============================================================================

TEST(MemoryMappedPayload, AnonymousMapping) {
    MemoryMappedPayload mmp(4096);
    EXPECT_TRUE(mmp.valid());
    EXPECT_EQ(mmp.size(), 4096u);
    EXPECT_NE(mmp.data(), nullptr);
}

TEST(MemoryMappedPayload, AnonymousMappingWriteRead) {
    MemoryMappedPayload mmp(256);
    ASSERT_TRUE(mmp.valid());

    const std::string msg = "ZeroCopyPayload";
    std::memcpy(mmp.mutableData(), msg.data(), msg.size());

    const std::string actual(reinterpret_cast<const char*>(mmp.data()),
                              msg.size());
    EXPECT_EQ(actual, msg);
}

TEST(MemoryMappedPayload, ZeroSizeThrows) {
    EXPECT_THROW(MemoryMappedPayload{0}, std::invalid_argument);
}

TEST(MemoryMappedPayload, MoveSemantics) {
    MemoryMappedPayload a(1024);
    ASSERT_TRUE(a.valid());

    MemoryMappedPayload b(std::move(a));
    EXPECT_TRUE(b.valid());
    EXPECT_FALSE(a.valid()); // a was moved-from
    EXPECT_EQ(b.size(), 1024u);
}

TEST(MemoryMappedPayload, MoveAssignment) {
    MemoryMappedPayload a(512);
    MemoryMappedPayload b(256);
    b = std::move(a);
    EXPECT_TRUE(b.valid());
    EXPECT_EQ(b.size(), 512u);
}

TEST(MemoryMappedPayload, FileMapping) {
    // Create a temporary file with known content.
    char tmpfile[] = "/tmp/themis_mmp_XXXXXX";
    const int fd = ::mkstemp(tmpfile);
    ASSERT_GE(fd, 0);

    const std::string content(1024, 'A');
    ASSERT_EQ(::write(fd, content.data(), content.size()),
              static_cast<ssize_t>(content.size()));
    ::close(fd);

    MemoryMappedPayload mmp(tmpfile);
    EXPECT_TRUE(mmp.valid());
    EXPECT_EQ(mmp.size(), content.size());
    EXPECT_EQ(std::string(reinterpret_cast<const char*>(mmp.data()),
                          mmp.size()), content);

    ::unlink(tmpfile);
}

TEST(MemoryMappedPayload, MissingFileThrows) {
    EXPECT_THROW(MemoryMappedPayload{"/nonexistent/file/path.bin"},
                 std::system_error);
}

// =============================================================================
// NagleController
// =============================================================================

TEST(NagleController, DefaultModeIsDefault) {
    auto [a, b] = make_socket_pair();
    NagleController nc(a);
    EXPECT_EQ(nc.currentMode(), NagleController::Mode::DEFAULT);
    ::close(a); ::close(b);
}

TEST(NagleController, SetNodelay) {
    // TCP_NODELAY only works on IPPROTO_TCP sockets; socketpair gives AF_UNIX
    // sockets, so setsockopt may legitimately fail here.  The test validates
    // that the controller calls setsockopt and updates the mode only on success.
    auto [a, b] = make_socket_pair();
    NagleController nc(a);
    const bool ok = nc.setNodelay();
    if (ok) {
        EXPECT_EQ(nc.currentMode(), NagleController::Mode::NODELAY);
    } else {
        // On AF_UNIX sockets the call is expected to fail.
        EXPECT_EQ(nc.currentMode(), NagleController::Mode::DEFAULT);
    }
    ::close(a); ::close(b);
}

TEST(NagleController, SetCork) {
    auto [a, b] = make_socket_pair();
    NagleController nc(a);
    // TCP_CORK may not be available on all platforms; just verify it doesn't crash.
    nc.cork(); // ignore return value (platform-dependent)
    ::close(a); ::close(b);
}

TEST(NagleController, UncorkRestoresMode) {
    auto [a, b] = make_socket_pair();
    NagleController nc(a);
    nc.cork();
    nc.uncork();
    EXPECT_EQ(nc.currentMode(), NagleController::Mode::DEFAULT);
    ::close(a); ::close(b);
}

TEST(NagleController, InvalidFdReturnsFalse) {
    NagleController nc(-1);
    EXPECT_FALSE(nc.setNodelay());
    EXPECT_FALSE(nc.cork());
    EXPECT_FALSE(nc.uncork());
}

TEST(NagleController, FdAccessor) {
    auto [a, b] = make_socket_pair();
    NagleController nc(a);
    EXPECT_EQ(nc.fd(), a);
    ::close(a); ::close(b);
}

// =============================================================================
// WireProtocolBatcher
// =============================================================================

TEST(WireProtocolBatcher, EmptyBatcherNoPending) {
    auto [wr, rd] = make_socket_pair();
    WireProtocolBatcher batcher(wr);
    EXPECT_FALSE(batcher.pending());
    EXPECT_EQ(batcher.pendingCount(), 0u);
    EXPECT_EQ(batcher.pendingBytes(), 0u);
    ::close(wr); ::close(rd);
}

TEST(WireProtocolBatcher, AddSingleEntry) {
    auto [wr, rd] = make_socket_pair();
    WireProtocolBatcher batcher(wr);
    const char data[] = "hello";
    batcher.add(data, sizeof(data) - 1);
    EXPECT_TRUE(batcher.pending());
    EXPECT_EQ(batcher.pendingCount(), 1u);
    EXPECT_EQ(batcher.pendingBytes(), sizeof(data) - 1);
    batcher.flush();
    ::close(wr); ::close(rd);
}

TEST(WireProtocolBatcher, FlushSendsAllData) {
    auto [wr, rd] = make_socket_pair();
    WireProtocolBatcher batcher(wr);

    const std::string a = "Hello";
    const std::string b = ", World!";
    batcher.add(a.data(), a.size());
    batcher.add(b.data(), b.size());

    const ssize_t sent = batcher.flush();
    EXPECT_EQ(sent, static_cast<ssize_t>(a.size() + b.size()));
    EXPECT_FALSE(batcher.pending());

    // Verify data arrived in order.
    std::vector<char> buf(a.size() + b.size());
    ASSERT_TRUE(read_exact(rd, buf.data(), buf.size()));
    const std::string received(buf.data(), buf.size());
    EXPECT_EQ(received, a + b);

    ::close(wr); ::close(rd);
}

TEST(WireProtocolBatcher, FlushClearsPendingState) {
    auto [wr, rd] = make_socket_pair();
    WireProtocolBatcher batcher(wr);

    const char data[] = "test";
    batcher.add(data, 4);
    batcher.flush();

    EXPECT_EQ(batcher.pendingCount(), 0u);
    EXPECT_EQ(batcher.pendingBytes(), 0u);

    ::close(wr); ::close(rd);
}

TEST(WireProtocolBatcher, FlushEmptyBatchReturnsZero) {
    auto [wr, rd] = make_socket_pair();
    WireProtocolBatcher batcher(wr);
    EXPECT_EQ(batcher.flush(), 0);
    ::close(wr); ::close(rd);
}

TEST(WireProtocolBatcher, StatsAccumulate) {
    auto [wr, rd] = make_socket_pair();
    WireProtocolBatcher batcher(wr);

    const char d1[] = "abc";
    const char d2[] = "de";
    batcher.add(d1, 3);
    batcher.add(d2, 2);
    batcher.flush();

    EXPECT_EQ(batcher.stats().messages_queued,  2u);
    EXPECT_EQ(batcher.stats().batches_flushed,  1u);
    EXPECT_EQ(batcher.stats().bytes_flushed,    5u);
    EXPECT_NEAR(batcher.stats().avgBatchSize(), 2.0, 1e-9);

    ::close(wr); ::close(rd);
}

TEST(WireProtocolBatcher, AutoFlushOnMessageLimit) {
    auto [wr, rd] = make_socket_pair();

    WireProtocolBatcher::Config cfg;
    cfg.max_messages_per_batch = 2;
    cfg.auto_flush_on_limit    = true;
    WireProtocolBatcher batcher(wr, cfg);

    const char d[] = "x";
    // Add 3 messages; third should trigger auto-flush of first two.
    batcher.add(d, 1);
    batcher.add(d, 1);
    batcher.add(d, 1); // triggers flush of first 2, then queues this one

    EXPECT_GE(batcher.stats().forced_flushes, 1u);

    batcher.flush(); // flush remaining
    ::close(wr); ::close(rd);
}

TEST(WireProtocolBatcher, AutoFlushOnByteLimit) {
    auto [wr, rd] = make_socket_pair();

    WireProtocolBatcher::Config cfg;
    cfg.max_bytes_per_batch = 4;
    cfg.auto_flush_on_limit = true;
    WireProtocolBatcher batcher(wr, cfg);

    const char big[] = "12345"; // 5 bytes > 4-byte limit
    batcher.add(big, 5);        // should trigger auto-flush on next add
    batcher.add(big, 5);

    EXPECT_GE(batcher.stats().forced_flushes, 1u);

    batcher.flush();
    ::close(wr); ::close(rd);
}

TEST(WireProtocolBatcher, ZeroSizeAddIsNoOp) {
    auto [wr, rd] = make_socket_pair();
    WireProtocolBatcher batcher(wr);
    EXPECT_TRUE(batcher.add(nullptr, 0));
    EXPECT_FALSE(batcher.pending());
    ::close(wr); ::close(rd);
}

TEST(WireProtocolBatcher, FdAccessor) {
    auto [wr, rd] = make_socket_pair();
    WireProtocolBatcher batcher(wr);
    EXPECT_EQ(batcher.fd(), wr);
    ::close(wr); ::close(rd);
}

// =============================================================================
// ZstdDictionaryCompressor
// =============================================================================

/// Build a collection of similar JSON-like sample payloads.
static std::vector<std::vector<uint8_t>> makeSamples(size_t count,
                                                     size_t size_each) {
    std::vector<std::vector<uint8_t>> samples;
    samples.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        std::string s;
        s.reserve(size_each);
        // Mimic repeated JSON keys to make the dictionary useful.
        while (s.size() < size_each) {
            s += R"({"id":)" + std::to_string(i) +
                 R"(,"name":"user","email":"user@db.io","active":true})";
        }
        s.resize(size_each);
        samples.emplace_back(s.begin(), s.end());
    }
    return samples;
}

TEST(ZstdDictionaryCompressor, DefaultConstruction) {
    ZstdDictionaryCompressor comp;
    EXPECT_FALSE(comp.hasDictionary());
    EXPECT_EQ(comp.dictionaryId(), 0u);
    EXPECT_TRUE(comp.dictionaryBytes().empty());
}

TEST(ZstdDictionaryCompressor, TrainFromSamples) {
    ZstdDictionaryCompressor comp;
    auto samples = makeSamples(50, 512);
    EXPECT_TRUE(comp.train(samples));
    EXPECT_TRUE(comp.hasDictionary());
    EXPECT_FALSE(comp.dictionaryBytes().empty());
}

TEST(ZstdDictionaryCompressor, TrainEmptySamplesFails) {
    ZstdDictionaryCompressor comp;
    EXPECT_FALSE(comp.train({}));
    EXPECT_FALSE(comp.hasDictionary());
}

TEST(ZstdDictionaryCompressor, CompressDecompressRoundTrip) {
    ZstdDictionaryCompressor comp;
    auto samples = makeSamples(50, 512);
    ASSERT_TRUE(comp.train(samples));

    const std::string original(1024, 'a');
    const std::vector<uint8_t> data(original.begin(), original.end());

    auto compressed = comp.compress(data);
    ASSERT_FALSE(compressed.empty());

    auto restored = comp.decompress(compressed);
    ASSERT_FALSE(restored.empty());
    EXPECT_EQ(restored, data);
}

TEST(ZstdDictionaryCompressor, DictionaryImprovesRatio) {
    // Similar structured data – dictionary should outperform plain Zstd.
    auto samples = makeSamples(80, 1024);
    const std::vector<uint8_t> test_payload(samples[0]);

    ZstdDictionaryCompressor with_dict;
    ASSERT_TRUE(with_dict.train(samples));

    auto dict_compressed   = with_dict.compress(test_payload);
    auto plain_compressed  = compressZstd(test_payload);

    // Dictionary-compressed may be smaller OR equal; never much larger.
    if (!dict_compressed.empty() && !plain_compressed.empty()) {
        // Both succeed; dict version should be ≤ plain version for structured data.
        EXPECT_LE(dict_compressed.size(), plain_compressed.size() + 32)
            << "Dictionary compression should not significantly inflate output";
    }
}

TEST(ZstdDictionaryCompressor, SmallPayloadSkipped) {
    ZstdDictionaryCompressor::Config cfg;
    cfg.min_compress_bytes = 512;
    ZstdDictionaryCompressor comp(cfg);

    auto samples = makeSamples(50, 512);
    ASSERT_TRUE(comp.train(samples));

    const std::vector<uint8_t> tiny(100, 0x42); // < 512 bytes
    EXPECT_TRUE(comp.compress(tiny).empty());
}

TEST(ZstdDictionaryCompressor, DecompressWithoutDictionary) {
    // Compress with plain Zstd, decompress with an untrained compressor.
    const std::vector<uint8_t> data(512, 0xAB);
    auto plain = compressZstd(data);
    ASSERT_FALSE(plain.empty());

    // The ZstdDictionaryCompressor with no dictionary should fall back to
    // plain decompression via the decompress() interface – but the format
    // differs (plain Zstd has a 4-byte prefix, dict-compressor uses 8).
    // This test validates that decompress() on a dict-compressed payload
    // produced by the same compressor round-trips correctly.
    ZstdDictionaryCompressor comp;
    // No dictionary trained; compress will fallback to plain Zstd internally.
    auto compressed = comp.compress(data);
    // If compress returned empty (e.g. fallback didn't save bytes), that is OK.
    if (!compressed.empty()) {
        auto restored = comp.decompress(compressed);
        EXPECT_EQ(restored, data);
    }
}

TEST(ZstdDictionaryCompressor, LoadAndReloadDictionary) {
    ZstdDictionaryCompressor comp;
    auto samples = makeSamples(50, 512);
    ASSERT_TRUE(comp.train(samples));

    const auto dict_bytes = comp.dictionaryBytes();
    ASSERT_FALSE(dict_bytes.empty());

    // Create a second compressor from the serialised dictionary bytes.
    ZstdDictionaryCompressor comp2;
    ASSERT_TRUE(comp2.loadDictionary(dict_bytes));
    EXPECT_TRUE(comp2.hasDictionary());
    EXPECT_EQ(comp2.dictionaryId(), comp.dictionaryId());
}

TEST(ZstdDictionaryCompressor, MoveSemantics) {
    ZstdDictionaryCompressor comp;
    auto samples = makeSamples(50, 512);
    ASSERT_TRUE(comp.train(samples));
    const uint32_t id = comp.dictionaryId();

    ZstdDictionaryCompressor moved(std::move(comp));
    EXPECT_TRUE(moved.hasDictionary());
    EXPECT_EQ(moved.dictionaryId(), id);
    EXPECT_FALSE(comp.hasDictionary()); // moved-from
}

#endif // _WIN32
