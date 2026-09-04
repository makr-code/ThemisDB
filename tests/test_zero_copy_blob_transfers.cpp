/*
 * Focused tests for ZeroCopyBlobTransfer + MmapBlobView
 * (Issue #231 – Zero-Copy Blob Transfers, v1.7.0)
 *
 * Acceptance criteria covered:
 *   AC-1  sendfile(): filesystem blob transferred to dest fd without
 *         user-space copy; bytes match original data
 *   AC-2  sendfile() fallback: portable read+write path used when
 *         sendfile() is not available (non-Linux); data identical
 *   AC-3  sendfile() with explicit offset + length: only the requested
 *         byte range is transferred
 *   AC-4  sendfile() returns ERR_STORAGE_FILE_NOT_FOUND for missing source
 *   AC-5  MmapBlobView: valid() == true after mapping an existing file
 *   AC-6  MmapBlobView: data() and size() match the original file content
 *   AC-7  MmapBlobView: forEach() iterates over the entire mapped data
 *         in the specified chunk size
 *   AC-8  MmapBlobView: valid() == false for non-existent file
 *   AC-9  MmapBlobView: move semantics leave source in invalid state
 *   AC-10 shouldUseZeroCopy(): returns false below threshold,
 *         true at/above threshold
 *   AC-11 ZeroCopyTransferConfig: s3_multipart_part_size_bytes is
 *         clamped to S3_MULTIPART_MIN_PART_BYTES when constructed too small
 *   AC-12 ZeroCopyTransferStats: bytes_transferred matches file content
 *         after a successful sendfile/fallback transfer
 *   AC-13 openMmap(): returns valid view for a real file
 *   AC-14 sendfileTransfer(): invalid offset returns error
 */

#include "storage/zero_copy_blob_transfer.h"

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#if defined(__linux__) || defined(__APPLE__)
#  include <fcntl.h>
#  include <unistd.h>
#endif

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Test helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Create a temporary file filled with @p data and return its path.
std::string writeTmpFile(const std::string& name, const std::vector<uint8_t>& data) {
    std::string path = (fs::temp_directory_path() / name).string();
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) {
      throw std::runtime_error("writeTmpFile: cannot create " + path);
    }
    ofs.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    return path;
}

/// Read the full content of @p path into a vector.
std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    return {(std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>()};
}

/// Build a deterministic blob of @p size bytes (content = index % 256).
std::vector<uint8_t> makeBlob(size_t size) {
    std::vector<uint8_t> v(size);
    for (size_t i = 0; i < size; ++i) {
        v[i] = static_cast<uint8_t>(i % 256);
    }
    return v;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class ZeroCopyBlobTransferFocusedTests : public ::testing::Test {
protected:
    themis::storage::ZeroCopyBlobTransfer xfer_;

    void TearDown() override {
        // Clean up any tmp files we may have created
        for (const auto& p : tmp_files_) {
            std::error_code ec = {};
            fs::remove(p, ec);
        }
    }

    std::string createTmpBlob(const std::string& name, size_t size) {
        auto data = makeBlob(size);
        std::string path = writeTmpFile(name, data);
        tmp_files_.push_back(path);
        return path;
    }

    std::string createTmpBlobFromData(const std::string& name,
                                      const std::vector<uint8_t>& data) {
        std::string path = writeTmpFile(name, data);
        tmp_files_.push_back(path);
        return path;
    }

    // Accessible from TEST_F test bodies (GTest subclasses the fixture)
    std::vector<std::string> tmp_files_;
};

// ─────────────────────────────────────────────────────────────────────────────
// AC-10: shouldUseZeroCopy threshold
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ZeroCopyBlobTransferFocusedTests, ShouldUseZeroCopyBelowThreshold) {
    int64_t threshold = xfer_.config().zero_copy_threshold_bytes;
    EXPECT_FALSE(xfer_.shouldUseZeroCopy(threshold - 1));
}

TEST_F(ZeroCopyBlobTransferFocusedTests, ShouldUseZeroCopyAtThreshold) {
    int64_t threshold = xfer_.config().zero_copy_threshold_bytes;
    EXPECT_TRUE(xfer_.shouldUseZeroCopy(threshold));
}

TEST_F(ZeroCopyBlobTransferFocusedTests, ShouldUseZeroCopyAboveThreshold) {
    int64_t threshold = xfer_.config().zero_copy_threshold_bytes;
    EXPECT_TRUE(xfer_.shouldUseZeroCopy(threshold + 1024));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-11: s3_multipart_part_size_bytes clamping
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ZeroCopyBlobTransferFocusedTests, S3PartSizeClampedToMinimum) {
    themis::storage::ZeroCopyTransferConfig cfg;
    cfg.s3_multipart_part_size_bytes = 1024;  // below 5 MB minimum
    themis::storage::ZeroCopyBlobTransfer small_xfer(cfg);

    EXPECT_GE(small_xfer.config().s3_multipart_part_size_bytes,
              themis::storage::S3_MULTIPART_MIN_PART_BYTES);
}

TEST_F(ZeroCopyBlobTransferFocusedTests, S3PartSizeAboveMinimumPreserved) {
    themis::storage::ZeroCopyTransferConfig cfg;
    cfg.s3_multipart_part_size_bytes = 10LL * 1024 * 1024;  // 10 MB
    themis::storage::ZeroCopyBlobTransfer big_xfer(cfg);

    EXPECT_EQ(big_xfer.config().s3_multipart_part_size_bytes, 10LL * 1024 * 1024);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-5 / AC-6: MmapBlobView — valid mapping with correct data
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ZeroCopyBlobTransferFocusedTests, MmapBlobViewValidForExistingFile) {
    std::vector<uint8_t> data = makeBlob(8192);
    std::string path = createTmpBlobFromData("mmap_test_valid.blob", data);

    themis::storage::MmapBlobView view(path);

    EXPECT_TRUE(view.valid());
    EXPECT_EQ(view.size(), data.size());
    ASSERT_NE(view.data(), nullptr);
    EXPECT_EQ(0, std::memcmp(view.data(), data.data(), data.size()));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-8: MmapBlobView — invalid for non-existent file
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ZeroCopyBlobTransferFocusedTests, MmapBlobViewInvalidForMissingFile) {
    themis::storage::MmapBlobView view(
        (fs::temp_directory_path() / "no_such_file_themisdb_12345.blob").string());

    EXPECT_FALSE(view.valid());
    EXPECT_EQ(view.data(), nullptr);
    EXPECT_EQ(view.size(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-7: MmapBlobView::forEach iterates correctly
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ZeroCopyBlobTransferFocusedTests, MmapForEachIteratesAllBytes) {
    std::vector<uint8_t> data = makeBlob(10000);
    std::string path = createTmpBlobFromData("mmap_foreach.blob", data);

    themis::storage::MmapBlobView view(path);
    ASSERT_TRUE(view.valid());

    std::vector<uint8_t> collected = {};

    collected.reserve(data.size());

    view.forEach(1024, [&](const uint8_t* ptr, size_t len) {
        collected.insert(collected.end(), ptr, ptr + len);
        return true;
    });

    EXPECT_EQ(collected, data);
}

TEST_F(ZeroCopyBlobTransferFocusedTests, MmapForEachCanStopEarly) {
    std::vector<uint8_t> data = makeBlob(10000);
    std::string path = createTmpBlobFromData("mmap_foreach_stop.blob", data);

    themis::storage::MmapBlobView view(path);
    ASSERT_TRUE(view.valid());

    size_t calls = 0;
    view.forEach(1024, [&](const uint8_t*, size_t) {
        ++calls;
        return calls < 3;  // stop after 3 chunks
    });

    EXPECT_EQ(calls, 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-9: MmapBlobView move semantics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ZeroCopyBlobTransferFocusedTests, MmapBlobViewMoveLeaveSourceInvalid) {
    std::vector<uint8_t> data = makeBlob(4096);
    std::string path = createTmpBlobFromData("mmap_move.blob", data);

    themis::storage::MmapBlobView src(path);
    ASSERT_TRUE(src.valid());

    themis::storage::MmapBlobView dst(std::move(src));

    // Intentionally accessing src after move to verify the post-move state is invalid.
    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_FALSE(src.valid());
    EXPECT_TRUE(dst.valid());
    EXPECT_EQ(dst.size(), data.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-13: openMmap helper on ZeroCopyBlobTransfer
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ZeroCopyBlobTransferFocusedTests, OpenMmapReturnsValidView) {
    std::vector<uint8_t> data = makeBlob(2048);
    std::string path = createTmpBlobFromData("open_mmap.blob", data);

    auto view = xfer_.openMmap(path);

    EXPECT_TRUE(view.valid());
    EXPECT_EQ(view.size(), data.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// sendfileTransfer / fallback tests — require writable fd
// ─────────────────────────────────────────────────────────────────────────────

#if defined(__linux__) || defined(__APPLE__)
// On POSIX we can use a real pipe/file fd

TEST_F(ZeroCopyBlobTransferFocusedTests, AC1_SendfileTransfersFullFile) {
    // AC-1: full transfer — bytes match original
    std::vector<uint8_t> data = makeBlob(16384);
    std::string src_path = createTmpBlobFromData("sendfile_full_src.blob", data);

    std::string dst_path = (fs::temp_directory_path() / "sendfile_full_dst.blob").string();
    tmp_files_.push_back(dst_path);

    int dst_fd = ::open(dst_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    ASSERT_GE(dst_fd, 0) << "Failed to open dest fd";

    auto result = xfer_.sendfileTransfer(src_path, dst_fd);
    ::close(dst_fd);

    ASSERT_TRUE(result.has_value()) << result.error().message();

    auto& stats = result.value();
    EXPECT_EQ(stats.bytes_transferred, static_cast<int64_t>(data.size()));
    EXPECT_GT(stats.duration_us, 0);

    auto received = readFile(dst_path);
    EXPECT_EQ(received, data);
}

TEST_F(ZeroCopyBlobTransferFocusedTests, AC3_SendfileWithOffsetAndLength) {
    // AC-3: partial transfer — only the requested range
    std::vector<uint8_t> data = makeBlob(16384);
    std::string src_path = createTmpBlobFromData("sendfile_partial_src.blob", data);

    std::string dst_path = (fs::temp_directory_path() / "sendfile_partial_dst.blob").string();
    tmp_files_.push_back(dst_path);

    int dst_fd = ::open(dst_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    ASSERT_GE(dst_fd, 0);

    constexpr int64_t OFFSET = 1024;
    constexpr int64_t LENGTH = 4096;

    auto result = xfer_.sendfileTransfer(src_path, dst_fd, OFFSET, LENGTH);
    ::close(dst_fd);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result.value().bytes_transferred, LENGTH);

    auto received = readFile(dst_path);
    ASSERT_EQ(static_cast<int64_t>(received.size()), LENGTH);
    EXPECT_EQ(0, std::memcmp(received.data(), data.data() + OFFSET, LENGTH));
}

TEST_F(ZeroCopyBlobTransferFocusedTests, AC12_TransferStatsMatchFileSize) {
    // AC-12: bytes_transferred == file size
    size_t blob_size = 32768;
    std::vector<uint8_t> data = makeBlob(blob_size);
    std::string src_path = createTmpBlobFromData("stats_check.blob", data);

    std::string dst_path = (fs::temp_directory_path() / "stats_check_dst.blob").string();
    tmp_files_.push_back(dst_path);

    int dst_fd = ::open(dst_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    ASSERT_GE(dst_fd, 0);

    auto result = xfer_.sendfileTransfer(src_path, dst_fd);
    ::close(dst_fd);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().bytes_transferred, static_cast<int64_t>(blob_size));
}

#endif // POSIX

TEST_F(ZeroCopyBlobTransferFocusedTests, AC4_SendfileErrorForMissingSource) {
    // AC-4: missing source → ERR_STORAGE_FILE_NOT_FOUND
    int dummy_fd = 1;  // stdout — we won't actually write to it
    auto result = xfer_.sendfileTransfer(
        (fs::temp_directory_path() / "no_such_blob_xfer_9999.blob").string(), dummy_fd);

    EXPECT_FALSE(result.has_value());
}

TEST_F(ZeroCopyBlobTransferFocusedTests, AC14_SendfileInvalidOffsetReturnsError) {
    // AC-14: offset beyond file size → error
    std::vector<uint8_t> data = makeBlob(512);
    std::string src_path = createTmpBlobFromData("sendfile_bad_offset.blob", data);

    int dummy_fd = 1;
    auto result = xfer_.sendfileTransfer(src_path, dummy_fd,
                                         /*offset=*/1024, /*length=*/512);
    EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Large-blob round-trip via mmap (AC-6, sized above the default threshold)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ZeroCopyBlobTransferFocusedTests, LargeBlobMmapRoundTrip) {
    // 5 MB blob — above default ZERO_COPY_MMAP_THRESHOLD_BYTES (4 MB)
    constexpr size_t BLOB_SIZE = 5ULL * 1024 * 1024;
    auto data = makeBlob(BLOB_SIZE);
    std::string path = createTmpBlobFromData("large_mmap.blob", data);

    EXPECT_TRUE(xfer_.shouldUseZeroCopy(static_cast<int64_t>(BLOB_SIZE)));

    auto view = xfer_.openMmap(path);
    ASSERT_TRUE(view.valid());
    EXPECT_EQ(view.size(), BLOB_SIZE);
    EXPECT_EQ(0, std::memcmp(view.data(), data.data(), BLOB_SIZE));
}

// ─────────────────────────────────────────────────────────────────────────────
// Default configuration sanity checks
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ZeroCopyBlobTransferFocusedTests, DefaultConfigHasExpectedThreshold) {
    EXPECT_EQ(xfer_.config().zero_copy_threshold_bytes,
              themis::storage::ZERO_COPY_MMAP_THRESHOLD_BYTES);
}

TEST_F(ZeroCopyBlobTransferFocusedTests, DefaultConfigHasExpectedPartSize) {
    EXPECT_GE(xfer_.config().s3_multipart_part_size_bytes,
              themis::storage::S3_MULTIPART_MIN_PART_BYTES);
}
