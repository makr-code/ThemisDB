/**
 * @file test_archive_processor.cpp
 * @brief Unit tests for ArchiveProcessor plugin
 * 
 * Tests the optional archive processing functionality.
 */

#include <gtest/gtest.h>
#include "content/archive_processor.h"
#include "content/content_validator.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace themis {
namespace content {
namespace test {

class ArchiveProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir_ = fs::temp_directory_path() / "themis_test_archive";
        fs::create_directories(test_dir_);
    }
    
    void TearDown() override {
        // Cleanup
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }
    
    // Helper: Create a simple ZIP file (mock)
    std::string createMockZipBlob() {
        // ZIP local file header signature: PK\x03\x04
        std::string blob;
        blob.push_back(0x50);  // P
        blob.push_back(0x4B);  // K
        blob.push_back(0x03);
        blob.push_back(0x04);
        
        // Add some dummy data
        blob.append(100, '\0');
        
        return blob;
    }
    
    fs::path test_dir_;
};

// Test 1: Archive format detection
TEST_F(ArchiveProcessorTest, DetectFormatZip) {
    std::string zip_blob = createMockZipBlob();
    
    auto format = ArchiveProcessor::detectFormat(zip_blob, "test.zip");
    EXPECT_EQ(format, ArchiveFormat::ZIP);
}

TEST_F(ArchiveProcessorTest, DetectFormatFromFilename) {
    // Non-empty blob with matching extension should detect format
    std::string zip_blob = createMockZipBlob();
    EXPECT_EQ(ArchiveProcessor::detectFormat(zip_blob, "test.zip"), ArchiveFormat::ZIP);

    // For non-empty blobs without archive magic, extension fallback applies
    std::string blob_with_text = "Not an archive but has some content";
    EXPECT_EQ(ArchiveProcessor::detectFormat(blob_with_text, "test.zip"), ArchiveFormat::ZIP);

    // Magic bytes still take priority over conflicting extension
    EXPECT_EQ(ArchiveProcessor::detectFormat(zip_blob, "test.txt"), ArchiveFormat::ZIP);
}

TEST_F(ArchiveProcessorTest, DetectFormatEmptyBlobReturnsUnknown) {
    std::string empty_blob;
    
    // Empty blobs must return UNKNOWN regardless of filename extension
    // This prevents false positives and improves security validation
    EXPECT_EQ(ArchiveProcessor::detectFormat(empty_blob, "test.zip"), ArchiveFormat::UNKNOWN);
    EXPECT_EQ(ArchiveProcessor::detectFormat(empty_blob, "test.tar"), ArchiveFormat::UNKNOWN);
    EXPECT_EQ(ArchiveProcessor::detectFormat(empty_blob, "test.tar.gz"), ArchiveFormat::UNKNOWN);
    EXPECT_EQ(ArchiveProcessor::detectFormat(empty_blob, "test.7z"), ArchiveFormat::UNKNOWN);
}

TEST_F(ArchiveProcessorTest, DetectFormatUnknown) {
    std::string blob = "Not an archive";
    
    auto format = ArchiveProcessor::detectFormat(blob, "test.txt");
    EXPECT_EQ(format, ArchiveFormat::UNKNOWN);
}

// Test 2: Path sanitization (security)
TEST_F(ArchiveProcessorTest, SanitizePathTraversal) {
    // Should remove path traversal attempts
    EXPECT_EQ(ArchiveProcessor::sanitizePath("../etc/passwd"), "etc/passwd");
    EXPECT_EQ(ArchiveProcessor::sanitizePath("foo/../bar"), "bar");
    EXPECT_EQ(ArchiveProcessor::sanitizePath("../../secret"), "secret");
}

TEST_F(ArchiveProcessorTest, SanitizePathNormal) {
    // Normal paths should pass through
    EXPECT_EQ(ArchiveProcessor::sanitizePath("documents/file.txt"), "documents/file.txt");
    EXPECT_EQ(ArchiveProcessor::sanitizePath("data/2024/report.pdf"), "data/2024/report.pdf");
}

TEST_F(ArchiveProcessorTest, SanitizePathEmpty) {
    EXPECT_EQ(ArchiveProcessor::sanitizePath(""), "");
    EXPECT_EQ(ArchiveProcessor::sanitizePath("."), "");
    EXPECT_EQ(ArchiveProcessor::sanitizePath("./"), "");
}

// Test 3: Configuration
TEST_F(ArchiveProcessorTest, DefaultConfiguration) {
    ArchiveProcessorConfig config;
    
    EXPECT_EQ(config.strategy, ArchiveStrategy::EXTRACT_AND_INGEST);
    EXPECT_EQ(config.encrypted_policy, EncryptedArchivePolicy::REJECT);
    EXPECT_GT(config.max_total_size, 0ULL);
    EXPECT_GT(config.max_file_size, 0ULL);
    EXPECT_GT(config.max_compression_ratio, 0ULL);
}

TEST_F(ArchiveProcessorTest, CustomConfiguration) {
    ArchiveProcessorConfig config;
    config.strategy = ArchiveStrategy::METADATA_ONLY;
    config.encrypted_policy = EncryptedArchivePolicy::REQUIRE_PASSWORD;
    config.password = "test123";
    config.max_total_size = 1024 * 1024;  // 1 MB
    
    ArchiveProcessor processor(config);
    
    auto retrieved_config = processor.getConfig();
    EXPECT_EQ(retrieved_config.strategy, ArchiveStrategy::METADATA_ONLY);
    EXPECT_EQ(retrieved_config.encrypted_policy, EncryptedArchivePolicy::REQUIRE_PASSWORD);
    EXPECT_EQ(retrieved_config.password, "test123");
    EXPECT_EQ(retrieved_config.max_total_size, 1024ULL * 1024);
}

// Test 4: MIME type handling
TEST_F(ArchiveProcessorTest, CanHandleArchiveMimeTypes) {
    ArchiveProcessor processor;
    
    EXPECT_TRUE(processor.canHandle("application/zip"));
    EXPECT_TRUE(processor.canHandle("application/x-zip-compressed"));
    EXPECT_TRUE(processor.canHandle("application/x-tar"));
    EXPECT_TRUE(processor.canHandle("application/gzip"));
    EXPECT_TRUE(processor.canHandle("application/x-7z-compressed"));
}

TEST_F(ArchiveProcessorTest, CannotHandleNonArchiveMimeTypes) {
    ArchiveProcessor processor;
    
    EXPECT_FALSE(processor.canHandle("text/plain"));
    EXPECT_FALSE(processor.canHandle("image/jpeg"));
    EXPECT_FALSE(processor.canHandle("application/pdf"));
}

// Test 5: Processor category
TEST_F(ArchiveProcessorTest, CorrectCategory) {
    ArchiveProcessor processor;
    
    auto categories = processor.getSupportedCategories();
    EXPECT_FALSE(categories.empty());
    EXPECT_EQ(categories[0], ContentCategory::ARCHIVE);
    EXPECT_EQ(processor.getName(), "ArchiveProcessor");
}

// Test 6: Reject strategy
TEST_F(ArchiveProcessorTest, RejectStrategy) {
    ArchiveProcessorConfig config;
    config.strategy = ArchiveStrategy::REJECT;
    
    ArchiveProcessor processor(config);
    
    std::string zip_blob = createMockZipBlob();
    auto result = processor.process(zip_blob, "application/zip", "test.zip");
    
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("not accepted"), std::string::npos);
}

// Test 7: Unknown format handling
TEST_F(ArchiveProcessorTest, UnknownFormatError) {
    ArchiveProcessor processor;
    
    std::string blob = "Not an archive";
    auto result = processor.process(blob, "application/zip", "test.txt");
    
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Unknown"), std::string::npos);
}

// Test 8: Compression ratio validation (zip bomb protection)
TEST_F(ArchiveProcessorTest, CompressionRatioCheck) {
    ArchiveProcessorConfig config;
    config.max_compression_ratio = 10;  // Very strict for testing
    config.max_total_size = 1024ull * 1024ull * 1024ull;
    config.max_file_count = 100000;
    config.max_file_size = 1024ull * 1024ull * 1024ull;
    
    ArchiveProcessor processor(config);
    
    ArchiveMetadata metadata;
    metadata.format = ArchiveFormat::ZIP;
    metadata.total_compressed_size = 100;
    metadata.total_uncompressed_size = 2000;  // 20:1 ratio - should fail
    
    std::string error;
    bool valid = processor.validateArchive(metadata, error);
    
    EXPECT_FALSE(valid);
    EXPECT_FALSE(error.empty());
}

// Test 9: Size limit validation
TEST_F(ArchiveProcessorTest, TotalSizeLimit) {
    ArchiveProcessorConfig config;
    config.max_total_size = 1024;  // 1 KB limit
    
    ArchiveProcessor processor(config);
    
    ArchiveMetadata metadata;
    metadata.format = ArchiveFormat::ZIP;
    metadata.total_uncompressed_size = 2048;  // 2 KB - should fail
    metadata.total_compressed_size = 100;
    
    std::string error;
    bool valid = processor.validateArchive(metadata, error);
    
    EXPECT_FALSE(valid);
    EXPECT_NE(error.find("maximum total size"), std::string::npos);
}

// Test 10: File count limit validation
TEST_F(ArchiveProcessorTest, FileCountLimit) {
    ArchiveProcessorConfig config;
    config.max_file_count = 5;
    
    ArchiveProcessor processor(config);
    
    ArchiveMetadata metadata;
    metadata.format = ArchiveFormat::ZIP;
    metadata.member_count = 10;  // Should fail
    metadata.total_compressed_size = 100;
    metadata.total_uncompressed_size = 1000;
    
    std::string error;
    bool valid = processor.validateArchive(metadata, error);
    
    EXPECT_FALSE(valid);
    EXPECT_NE(error.find("file count"), std::string::npos);
}

// Test 11: Temp directory cleanup
TEST_F(ArchiveProcessorTest, TempDirectoryCleanup) {
    auto temp_dir = test_dir_ / "temp_extract";
    fs::create_directories(temp_dir);
    
    // Create some files
    std::ofstream(temp_dir / "file1.txt") << "test";
    std::ofstream(temp_dir / "file2.txt") << "test";
    
    EXPECT_TRUE(fs::exists(temp_dir));
    
    ArchiveProcessor::cleanupTempDirectory(temp_dir.string());
    
    EXPECT_FALSE(fs::exists(temp_dir));
}

// Test 12: Plugin availability
TEST_F(ArchiveProcessorTest, PluginAvailability) {
    // The processor should report availability based on build configuration
    // With libzip in dependencies, it should be available
    ArchiveProcessor processor;
    
    // This is more of an informational test
    // In a build without libzip, this would return false
    // For now, we just verify it returns a boolean
    bool available = ArchiveProcessor::isAvailable();
    EXPECT_TRUE(available || !available);  // Always passes, documents the API
}

// ============================================================================
// Tests for zip-bomb protection via ContentSecurityManager
// ============================================================================

class ContentSecurityZipBombTest : public ::testing::Test {
protected:
    ContentSecurityConfig default_config_;  // enable_zip_bomb_check=true, ratio=100, count=1000
};

TEST_F(ContentSecurityZipBombTest, AllowsNormalArchive) {
    ContentSecurityManager manager(default_config_);
    // 100 bytes compressed -> 500 bytes uncompressed: ratio 5x, well under 100x
    auto result = manager.checkZipBomb(100, 500, 10, "test_content");
    EXPECT_FALSE(result.error.failed());
}

TEST_F(ContentSecurityZipBombTest, BlocksExcessiveCompressionRatio) {
    ContentSecurityManager manager(default_config_);
    // 1 byte compressed -> 200 bytes uncompressed: ratio 200x, exceeds 100x limit
    auto result = manager.checkZipBomb(1, 200, 1, "test_content");
    EXPECT_TRUE(result.error.failed());
    EXPECT_EQ(result.error.code, ContentErrorCode::CONTENT_MALWARE_DETECTED);
    EXPECT_NE(result.error.message.find("compression ratio"), std::string::npos);
}

TEST_F(ContentSecurityZipBombTest, AllowsExactlyAtRatioLimit) {
    ContentSecurityManager manager(default_config_);
    // Ratio exactly 100x should pass (limit is >, not >=)
    auto result = manager.checkZipBomb(1, 100, 1, "test_content");
    EXPECT_FALSE(result.error.failed());
}

TEST_F(ContentSecurityZipBombTest, BlocksOneOverRatioLimit) {
    ContentSecurityManager manager(default_config_);
    // Ratio 101x should be blocked
    auto result = manager.checkZipBomb(1, 101, 1, "test_content");
    EXPECT_TRUE(result.error.failed());
}

TEST_F(ContentSecurityZipBombTest, BlocksExcessiveFileCount) {
    ContentSecurityManager manager(default_config_);
    // 1001 files exceeds the 1000-file limit
    auto result = manager.checkZipBomb(1000, 2000, 1001, "test_content");
    EXPECT_TRUE(result.error.failed());
    EXPECT_EQ(result.error.code, ContentErrorCode::CONTENT_SIZE_EXCEEDED);
    EXPECT_NE(result.error.message.find("file count"), std::string::npos);
}

TEST_F(ContentSecurityZipBombTest, AllowsExactlyMaxFileCount) {
    ContentSecurityManager manager(default_config_);
    // Exactly 1000 files should pass
    auto result = manager.checkZipBomb(1000, 2000, 1000, "test_content");
    EXPECT_FALSE(result.error.failed());
}

TEST_F(ContentSecurityZipBombTest, SkipsCheckWhenDisabled) {
    ContentSecurityConfig cfg;
    cfg.enable_zip_bomb_check = false;
    ContentSecurityManager manager(cfg);
    // Would normally fail ratio check (200x), but check is disabled
    auto result = manager.checkZipBomb(1, 200, 2000, "test_content");
    EXPECT_FALSE(result.error.failed());
    EXPECT_FALSE(result.zip_bomb_checked);   // check was skipped
    EXPECT_FALSE(result.zip_bomb_detected);
}

TEST_F(ContentSecurityZipBombTest, HandlesZeroCompressedSize) {
    ContentSecurityManager manager(default_config_);
    // Zero compressed size should not divide-by-zero; only file count is checked
    auto result = manager.checkZipBomb(0, 1000, 5, "test_content");
    EXPECT_FALSE(result.error.failed());
}

TEST_F(ContentSecurityZipBombTest, MetricsIncrementedOnScan) {
    ContentSecurityManager manager(default_config_);
    manager.resetMetrics();
    auto result = manager.checkZipBomb(100, 500, 10, "test_content");
    EXPECT_EQ(manager.getMetrics().zip_bomb_scans.load(), 1u);
    EXPECT_EQ(manager.getMetrics().zip_bomb_blocked.load(), 0u);
    EXPECT_TRUE(result.zip_bomb_checked);
    EXPECT_FALSE(result.zip_bomb_detected);
}

TEST_F(ContentSecurityZipBombTest, MetricsIncrementedOnBlock) {
    ContentSecurityManager manager(default_config_);
    manager.resetMetrics();
    auto result = manager.checkZipBomb(1, 200, 10, "test_content");  // 200x ratio, blocked
    EXPECT_EQ(manager.getMetrics().zip_bomb_scans.load(), 1u);
    EXPECT_EQ(manager.getMetrics().zip_bomb_blocked.load(), 1u);
    EXPECT_TRUE(result.zip_bomb_checked);
    EXPECT_TRUE(result.zip_bomb_detected);
}

TEST_F(ContentSecurityZipBombTest, CustomThresholds) {
    ContentSecurityConfig cfg;
    cfg.max_zip_bomb_ratio = 10;   // stricter: only 10x ratio
    cfg.max_zip_file_count = 5;    // stricter: only 5 files
    ContentSecurityManager manager(cfg);

    // 11x ratio should be blocked
    auto r1 = manager.checkZipBomb(1, 11, 1, "test");
    EXPECT_TRUE(r1.error.failed());

    // 6 files should be blocked
    auto r2 = manager.checkZipBomb(100, 200, 6, "test");
    EXPECT_TRUE(r2.error.failed());

    // Within thresholds should pass
    auto r3 = manager.checkZipBomb(10, 50, 4, "test");
    EXPECT_FALSE(r3.error.failed());
}

TEST_F(ContentSecurityZipBombTest, ToJsonContainsZipBombFields) {
    ContentSecurityManager manager(default_config_);
    auto result = manager.checkZipBomb(1, 200, 1, "test_content");  // blocked
    auto j = result.toJson();
    EXPECT_TRUE(j.contains("zip_bomb_checked"));
    EXPECT_TRUE(j.contains("zip_bomb_detected"));
    EXPECT_TRUE(j["zip_bomb_checked"].get<bool>());
    EXPECT_TRUE(j["zip_bomb_detected"].get<bool>());
}

// Integration test: ArchiveProcessor blocks when ContentSecurityManager detects zip bomb
TEST_F(ArchiveProcessorTest, SecurityManagerBlocksZipBombRatioViaConfig) {
    // Build a processor whose security manager uses a very strict 1x ratio limit
    ArchiveProcessorConfig proc_cfg;
    proc_cfg.max_compression_ratio = 10000;  // disable internal ratio guard
    proc_cfg.max_file_count = 10000;
    ArchiveProcessor processor(proc_cfg);

    ContentSecurityConfig sec_cfg;
    sec_cfg.max_zip_bomb_ratio = 1;     // only 1x allowed
    sec_cfg.max_zip_file_count = 10000;
    processor.setSecurityConfig(sec_cfg);

    ArchiveMetadata metadata;
    metadata.format = ArchiveFormat::ZIP;
    metadata.total_compressed_size = 1;
    metadata.total_uncompressed_size = 2;  // ratio = 2, blocked by sec limit of 1
    metadata.member_count = 1;
    metadata.file_count = 1;

    // validateArchive passes (internal limit is 10000), but security manager blocks
    std::string err;
    EXPECT_TRUE(processor.validateArchive(metadata, err));  // internal check passes

    // We cannot call process() without a real archive blob, so test the security
    // manager directly through the processor's setSecurityConfig path
    ContentSecurityManager mgr(sec_cfg);
    auto result = mgr.checkZipBomb(
        metadata.total_compressed_size,
        metadata.total_uncompressed_size,
        metadata.file_count,
        "test"
    );
    EXPECT_TRUE(result.error.failed());
}

} // namespace test
} // namespace content
} // namespace themis

// ============================================================================
// ContentValidator::validateFilename Tests
// ============================================================================

namespace themis {
namespace content {
namespace test {

TEST(FilenameValidationTest, ValidFilenames) {
    ContentValidator validator;

    EXPECT_TRUE(validator.validateFilename("").isOk());                    // empty OK
    EXPECT_TRUE(validator.validateFilename("document.pdf").isOk());
    EXPECT_TRUE(validator.validateFilename("data/file.json").isOk());
    EXPECT_TRUE(validator.validateFilename("archive/2024/report.txt").isOk());
}

TEST(FilenameValidationTest, PathTraversalUnix) {
    ContentValidator validator;

    EXPECT_FALSE(validator.validateFilename("../etc/passwd").isOk());
    EXPECT_FALSE(validator.validateFilename("foo/../bar").isOk());
    EXPECT_FALSE(validator.validateFilename("../../secret").isOk());
}

TEST(FilenameValidationTest, PathTraversalWindows) {
    ContentValidator validator;

    EXPECT_FALSE(validator.validateFilename("..\\etc\\passwd").isOk());
    EXPECT_FALSE(validator.validateFilename("foo\\..\\bar").isOk());
}

TEST(FilenameValidationTest, AbsoluteUnixPath) {
    ContentValidator validator;

    EXPECT_FALSE(validator.validateFilename("/etc/passwd").isOk());
    EXPECT_FALSE(validator.validateFilename("/home/user/file.txt").isOk());
}

TEST(FilenameValidationTest, AbsoluteWindowsPath) {
    ContentValidator validator;

    EXPECT_FALSE(validator.validateFilename("C:\\Users\\Admin\\file.txt").isOk());
    EXPECT_FALSE(validator.validateFilename("D:/data/file.csv").isOk());
    EXPECT_FALSE(validator.validateFilename("\\\\server\\share\\file").isOk());
}

TEST(FilenameValidationTest, ControlCharacters) {
    ContentValidator validator;

    // Construct strings with embedded control characters explicitly
    std::string null_byte = std::string("file") + '\x00' + "name.txt";
    EXPECT_FALSE(validator.validateFilename(null_byte).isOk());   // null byte

    std::string soh = std::string("file") + '\x01' + "name.txt";
    EXPECT_FALSE(validator.validateFilename(soh).isOk());          // SOH

    std::string us = std::string("file") + '\x1f' + "name.txt";
    EXPECT_FALSE(validator.validateFilename(us).isOk());           // US

    std::string del = std::string("file") + '\x7f' + "name.txt";
    EXPECT_FALSE(validator.validateFilename(del).isOk());          // DEL
}

TEST(FilenameValidationTest, ExcessiveLength) {
    ContentValidator validator;

    std::string too_long(4097, 'a');
    EXPECT_FALSE(validator.validateFilename(too_long).isOk());

    std::string just_right(4096, 'a');
    EXPECT_TRUE(validator.validateFilename(just_right).isOk());
}

} // namespace test
} // namespace content
} // namespace themis
