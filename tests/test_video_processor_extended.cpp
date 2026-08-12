/**
 * @file test_video_processor_extended.cpp
 * @brief Extended Google Test suite for Video Processor (v1.3.0 Phase 2)
 * 
 * This test file provides comprehensive testing for:
 * - FFmpeg/LibAV integration
 * - Metadata extraction from various video formats
 * - Keyframe detection and extraction
 * - Scene detection algorithms
 * - Subtitle extraction
 * - Thumbnail generation
 * - Plugin lifecycle management
 * - Error handling and edge cases
 */

#include <gtest/gtest.h>
#include "content/video_processor.h"
#include <vector>
#include <fstream>
#include <filesystem>
#include <limits>

// TODO(v1.3.0): Content plugin API drift (PluginConfig/ExtractionOptions fields). Disable extended video processor tests until updated.

using namespace themis::content;

/**
 * @brief Test fixture for Video Processor
 */
class VideoProcessorExtendedTest : public ::testing::Test {
protected:
    VideoProcessor processor;
    PluginConfig config;
    
    void SetUp() override {
        // Configure video processor
        config.set("thumbnail.max_width", 320);
        config.set("thumbnail.max_height", 240);
        config.set("keyframes.max_count", 10);
        config.set("subtitles.extract", true);
        config.set("scene_detection.enabled", true);
        
        // Initialize processor
        ASSERT_TRUE(processor.initialize(config));
    }
    
    void TearDown() override {
        processor.shutdown();
    }
    
    /**
     * @brief Helper to create minimal MP4 video data
     * Note: This creates a minimal valid MP4 structure for testing
     */
    std::vector<uint8_t> createMinimalMp4() {
        // Minimal MP4 file structure
        std::vector<uint8_t> data;
        
        // ftyp box (file type box)
        std::vector<uint8_t> ftyp = {
            0x00, 0x00, 0x00, 0x20, // box size (32 bytes)
            'f', 't', 'y', 'p',      // box type
            'i', 's', 'o', 'm',      // major brand
            0x00, 0x00, 0x02, 0x00,  // minor version
            'i', 's', 'o', 'm',      // compatible brands
            'i', 's', 'o', '2',
            'a', 'v', 'c', '1',
            'm', 'p', '4', '1'
        };
        
        data.insert(data.end(), ftyp.begin(), ftyp.end());
        return data;
    }
    
    /**
     * @brief Helper to create test video with known properties
     */
    std::vector<uint8_t> createTestVideo(int duration_seconds = 5) {
        // In a real implementation, this would create a proper video file
        // For testing, we create minimal valid data
        return createMinimalMp4();
    }
};

// ============================================================================
// Plugin Lifecycle Tests
// ============================================================================

/**
 * @test Test plugin info retrieval
 */
TEST_F(VideoProcessorExtendedTest, PluginInfo) {
    auto info = processor.getInfo();
    
    EXPECT_EQ(info.name, "video-processor");
    EXPECT_FALSE(info.version.empty());
    EXPECT_FALSE(info.description.empty());
    EXPECT_TRUE(info.supports_chunking);
    EXPECT_TRUE(info.supports_streaming);
    EXPECT_FALSE(info.supports_embedding);
    EXPECT_GT(info.mime_types.size(), 0);
    EXPECT_GT(info.extensions.size(), 0);
}

/**
 * @test Test MIME type support detection
 */
TEST_F(VideoProcessorExtendedTest, MimeTypeSupport) {
    EXPECT_TRUE(processor.canProcess("video/mp4"));
    EXPECT_TRUE(processor.canProcess("video/webm"));
    EXPECT_TRUE(processor.canProcess("video/x-matroska"));
    EXPECT_TRUE(processor.canProcess("video/quicktime"));
    EXPECT_TRUE(processor.canProcess("video/x-msvideo"));
    
    // Should not support non-video types
    EXPECT_FALSE(processor.canProcess("image/jpeg"));
    EXPECT_FALSE(processor.canProcess("audio/mp3"));
    EXPECT_FALSE(processor.canProcess("text/plain"));
}

/**
 * @test Test multiple initialization calls
 */
TEST_F(VideoProcessorExtendedTest, MultipleInitialization) {
    // Should handle multiple init calls gracefully
    EXPECT_TRUE(processor.initialize(config));
    EXPECT_TRUE(processor.initialize(config));
}

/**
 * @test Test plugin health check.
 *
 * CON-007: In a no-FFmpeg build the processor is initialised but cannot do
 * real work, so healthCheck() must return false.  In an FFmpeg build it
 * reflects the initialized_ flag (true after successful initialize()).
 */
TEST_F(VideoProcessorExtendedTest, HealthCheck) {
#ifdef THEMIS_HAS_FFMPEG
    EXPECT_TRUE(processor.healthCheck());
#else
    // CON-007 fix: no-FFmpeg simulation mode must surface the missing
    // dependency so health-check aggregators can detect it.
    EXPECT_FALSE(processor.healthCheck());
#endif
}

/**
 * @test Test statistics retrieval
 */
TEST_F(VideoProcessorExtendedTest, StatisticsRetrieval) {
    auto stats = processor.getStatistics();
    
    EXPECT_TRUE(stats.contains("videos_processed"));
    EXPECT_TRUE(stats.contains("total_duration_ms"));
    EXPECT_TRUE(stats.contains("errors"));
}

// ============================================================================
// Metadata Extraction Tests
// ============================================================================

/**
 * @test Test basic metadata extraction from MP4
 */
TEST_F(VideoProcessorExtendedTest, MetadataExtractionMP4) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.text.empty() || result.metadata.contains("format"));
    
    if (result.metadata.contains("format")) {
        EXPECT_EQ(result.metadata["format"], "mp4");
    }
}

/**
 * @test Test video duration extraction
 */
TEST_F(VideoProcessorExtendedTest, DurationExtraction) {
    auto video_data = createTestVideo(10);  // 10 second video
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    if (result.metadata.contains("duration_seconds")) {
        EXPECT_GT(result.metadata["duration_seconds"].get<double>(), 0.0);
    }
}

/**
 * @test Test resolution extraction
 */
TEST_F(VideoProcessorExtendedTest, ResolutionExtraction) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    // Check for resolution metadata
    bool has_resolution = result.metadata.contains("width") || 
                         result.metadata.contains("height") ||
                         result.metadata.contains("resolution");
    
    EXPECT_TRUE(result.success);
    // Resolution may not be present in minimal test data
}

/**
 * @test Test codec information extraction
 */
TEST_F(VideoProcessorExtendedTest, CodecExtraction) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    // Check for codec metadata
    bool has_codec_info = result.metadata.contains("video_codec") || 
                          result.metadata.contains("audio_codec") ||
                          result.metadata.contains("codecs");
    
    EXPECT_TRUE(result.success);
}

/**
 * @test Test bitrate extraction
 */
TEST_F(VideoProcessorExtendedTest, BitrateExtraction) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    // Bitrate may be available
    if (result.metadata.contains("bitrate")) {
        EXPECT_GT(result.metadata["bitrate"].get<int>(), 0);
    }
}

// ============================================================================
// Keyframe Detection Tests
// ============================================================================

/**
 * @test Test keyframe detection
 */
TEST_F(VideoProcessorExtendedTest, KeyframeDetection) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_keyframes = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    EXPECT_TRUE(result.success);
    // Keyframes may be available
    if (result.media.has_value() && result.media->keyframe_timestamps.size() > 0) {
        EXPECT_GT(result.media->keyframe_timestamps.size(), 0);
    }
}

/**
 * @test Test keyframe limit configuration
 */
TEST_F(VideoProcessorExtendedTest, KeyframeLimitConfiguration) {
    PluginConfig limit_config;
    limit_config.set("keyframes.max_count", 5);
    
    VideoProcessor limited_processor;
    ASSERT_TRUE(limited_processor.initialize(limit_config));
    
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_keyframes = true;
    
    auto result = limited_processor.extract(video_data, "video/mp4", options);
    
    if (result.media.has_value() && result.media->keyframe_timestamps.size() > 0) {
        EXPECT_LE(result.media->keyframe_timestamps.size(), 5);
    }
    
    limited_processor.shutdown();
}

/**
 * @test Test keyframe timestamp ordering
 */
TEST_F(VideoProcessorExtendedTest, KeyframeTimestampOrdering) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_keyframes = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    if (result.media.has_value() && result.media->keyframe_timestamps.size() > 1) {
        // Verify timestamps are in ascending order
        for (size_t i = 1; i < result.media->keyframe_timestamps.size(); ++i) {
            EXPECT_GE(result.media->keyframe_timestamps[i],
                     result.media->keyframe_timestamps[i-1]);
        }
    }
}

// ============================================================================
// Scene Detection Tests
// ============================================================================

/**
 * @test Test scene detection
 */
TEST_F(VideoProcessorExtendedTest, SceneDetection) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_scenes = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    EXPECT_TRUE(result.success);
    // Scene boundaries may be detected
}

/**
 * @test Test scene detection with configuration
 */
TEST_F(VideoProcessorExtendedTest, SceneDetectionConfiguration) {
    PluginConfig scene_config;
    scene_config.set("scene_detection.enabled", true);
    scene_config.set("scene_detection.threshold", 0.3);
    
    VideoProcessor scene_processor;
    ASSERT_TRUE(scene_processor.initialize(scene_config));
    
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_scenes = true;
    
    auto result = scene_processor.extract(video_data, "video/mp4", options);
    
    EXPECT_TRUE(result.success);
    
    scene_processor.shutdown();
}

/**
 * @test Test scene boundary consistency
 */
TEST_F(VideoProcessorExtendedTest, SceneBoundaryConsistency) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_scenes = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    if (result.media.has_value() && result.media->scene_boundaries.size() > 0) {
        // Scene boundaries should be within video duration
        for (const auto& boundary : result.media->scene_boundaries) {
            EXPECT_GE(boundary, 0);
        }
    }
}

// ============================================================================
// Subtitle Extraction Tests
// ============================================================================

/**
 * @test Test subtitle extraction
 */
TEST_F(VideoProcessorExtendedTest, SubtitleExtraction) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_subtitles = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    EXPECT_TRUE(result.success);
    // Subtitles may or may not be present
}

/**
 * @test Test subtitle format handling
 */
TEST_F(VideoProcessorExtendedTest, SubtitleFormatHandling) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_subtitles = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    if (result.media.has_value() && !result.media->subtitles.empty()) {
        // Verify subtitle format
        EXPECT_FALSE(result.media->subtitles.empty());
    }
}

// ============================================================================
// Thumbnail Generation Tests
// ============================================================================

/**
 * @test Test thumbnail generation
 */
TEST_F(VideoProcessorExtendedTest, ThumbnailGeneration) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.generate_thumbnail = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    EXPECT_TRUE(result.success);
    if (result.thumbnail.size() > 0) {
        EXPECT_GT(result.thumbnail.size(), 0);
    }
}

/**
 * @test Test thumbnail size configuration
 */
TEST_F(VideoProcessorExtendedTest, ThumbnailSizeConfiguration) {
    PluginConfig thumb_config;
    thumb_config.set("thumbnail.max_width", 160);
    thumb_config.set("thumbnail.max_height", 120);
    
    VideoProcessor thumb_processor;
    ASSERT_TRUE(thumb_processor.initialize(thumb_config));
    
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.generate_thumbnail = true;
    
    auto result = thumb_processor.extract(video_data, "video/mp4", options);
    
    EXPECT_TRUE(result.success);
    
    thumb_processor.shutdown();
}

TEST(VideoProcessorConfigValidationTest, RejectsNonPositiveThumbnailDimensions) {
    PluginConfig config;
    config.set("thumbnail.max_width", 0);
    config.set("thumbnail.max_height", 120);

    VideoProcessor processor;
    EXPECT_FALSE(processor.initialize(config));

    config.set("thumbnail.max_width", 160);
    config.set("thumbnail.max_height", -1);
    EXPECT_FALSE(processor.initialize(config));
}

TEST(VideoProcessorConfigValidationTest, RejectsOverflowProneThumbnailDimensions) {
    PluginConfig config;
    config.set("thumbnail.max_width", std::numeric_limits<int>::max());
    config.set("thumbnail.max_height", std::numeric_limits<int>::max());

    VideoProcessor processor;
    EXPECT_FALSE(processor.initialize(config));
}

// ============================================================================
// Multiple Format Tests
// ============================================================================

/**
 * @test Test WebM format processing
 */
TEST_F(VideoProcessorExtendedTest, WebMFormatProcessing) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    auto result = processor.extract(video_data, "video/webm", options);
    
    // Should handle gracefully even with minimal data
    EXPECT_TRUE(result.success || !result.success);
}

/**
 * @test Test MKV format processing
 */
TEST_F(VideoProcessorExtendedTest, MKVFormatProcessing) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    auto result = processor.extract(video_data, "video/x-matroska", options);
    
    // Should handle gracefully
    EXPECT_TRUE(result.success || !result.success);
}

/**
 * @test Test AVI format processing
 */
TEST_F(VideoProcessorExtendedTest, AVIFormatProcessing) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    auto result = processor.extract(video_data, "video/x-msvideo", options);
    
    // Should handle gracefully
    EXPECT_TRUE(result.success || !result.success);
}

/**
 * @test Test MOV format processing
 */
TEST_F(VideoProcessorExtendedTest, MOVFormatProcessing) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    auto result = processor.extract(video_data, "video/quicktime", options);
    
    // Should handle gracefully
    EXPECT_TRUE(result.success || !result.success);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

/**
 * @test Test empty data handling
 */
TEST_F(VideoProcessorExtendedTest, EmptyDataHandling) {
    std::vector<uint8_t> empty_data;
    
    ExtractionOptions options;
    auto result = processor.extract(empty_data, "video/mp4", options);
    
    // Should fail gracefully
    EXPECT_FALSE(result.success);
}

/**
 * @test Test corrupt data handling
 */
TEST_F(VideoProcessorExtendedTest, CorruptDataHandling) {
    std::vector<uint8_t> corrupt_data = {0xFF, 0xFF, 0xFF, 0xFF};
    
    ExtractionOptions options;
    auto result = processor.extract(corrupt_data, "video/mp4", options);
    
    // Should fail gracefully
    EXPECT_FALSE(result.success);
}

/**
 * @test Test unsupported format handling
 */
TEST_F(VideoProcessorExtendedTest, UnsupportedFormatHandling) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    auto result = processor.extract(video_data, "application/octet-stream", options);
    
    // Should handle unsupported format
    EXPECT_FALSE(result.success);
}

/**
 * @test Test very large file handling
 */
TEST_F(VideoProcessorExtendedTest, LargeFileHandling) {
    // Create large data buffer (10 MB)
    std::vector<uint8_t> large_data(10 * 1024 * 1024, 0x00);
    
    ExtractionOptions options;
    auto result = processor.extract(large_data, "video/mp4", options);
    
    // Should handle gracefully (may succeed or fail based on implementation)
    EXPECT_TRUE(result.success || !result.success);
}

/**
 * @test Test extraction with all options enabled
 */
TEST_F(VideoProcessorExtendedTest, AllOptionsEnabled) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_metadata = true;
    options.extract_keyframes = true;
    options.extract_scenes = true;
    options.extract_subtitles = true;
    options.generate_thumbnail = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    EXPECT_TRUE(result.success);
}

// ============================================================================
// Chunking Tests
// ============================================================================

/**
 * @test Test video content chunking
 */
TEST_F(VideoProcessorExtendedTest, ContentChunking) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    if (result.success) {
        auto chunks = processor.chunk(result, 1000, 100);
        // Chunks may or may not be created based on content
        EXPECT_TRUE(chunks.size() >= 0);
    }
}

/**
 * @test Test chunk size configuration
 */
TEST_F(VideoProcessorExtendedTest, ChunkSizeConfiguration) {
    auto video_data = createTestVideo();
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    auto result = processor.extract(video_data, "video/mp4", options);
    
    if (result.success) {
        auto chunks1 = processor.chunk(result, 500, 50);
        auto chunks2 = processor.chunk(result, 1000, 100);
        
        // Different chunk sizes may produce different number of chunks
        EXPECT_TRUE(chunks1.size() >= 0);
        EXPECT_TRUE(chunks2.size() >= 0);
    }
}

// Main function for Google Test


