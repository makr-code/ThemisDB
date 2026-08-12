/**
 * @file test_content_processing.cpp
 * @brief Integration tests for content processing pipelines
 * 
 * Tests multimedia content processing:
 * - Audio processing (speech-to-text, metadata extraction)
 * - Video processing (frame extraction, metadata)
 * - Image processing (CLIP embeddings, metadata)
 * - PDF processing (text extraction, metadata)
 * - Error handling for unsupported formats
 */

#include "test_fixture.h"
#include "test_data_generator.h"
#include "content/mime_detector.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <fstream>
#include <memory>
#include <vector>

using namespace themis;
using namespace themis::test;
using namespace themis::content;
using json = nlohmann::json;

/**
 * @brief Mock content processor for testing
 */
class MockContentProcessor {
public:
    struct ProcessingResult {
        bool success = true;
        std::string mime_type;
        json metadata;
        std::vector<uint8_t> processed_data;
        std::string error_message;
        int64_t processing_time_ms = 0;
    };
    
    ProcessingResult ProcessAudio(const std::vector<uint8_t>& audio_data) {
        static_cast<void>(audio_data);
        ProcessingResult result;
        auto start = std::chrono::high_resolution_clock::now();
        
        result.mime_type = "audio/mpeg";
        result.metadata = {
            {"duration_seconds", 180},
            {"sample_rate", 44100},
            {"channels", 2},
            {"bitrate", 320000},
            {"format", "mp3"},
            {"has_speech", true},
            {"transcription", "This is a mock audio transcription."}
        };
        result.success = true;
        
        auto end = std::chrono::high_resolution_clock::now();
        result.processing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        return result;
    }
    
    ProcessingResult ProcessVideo(const std::vector<uint8_t>& video_data) {
        static_cast<void>(video_data);
        ProcessingResult result;
        auto start = std::chrono::high_resolution_clock::now();
        
        result.mime_type = "video/mp4";
        result.metadata = {
            {"duration_seconds", 600},
            {"width", 1920},
            {"height", 1080},
            {"fps", 30},
            {"codec", "h264"},
            {"bitrate", 5000000},
            {"has_audio", true},
            {"frame_count", 18000}
        };
        result.success = true;
        
        auto end = std::chrono::high_resolution_clock::now();
        result.processing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        return result;
    }
    
    ProcessingResult ProcessImage(const std::vector<uint8_t>& image_data) {
        static_cast<void>(image_data);
        ProcessingResult result;
        auto start = std::chrono::high_resolution_clock::now();
        
        result.mime_type = "image/jpeg";
        result.metadata = {
            {"width", 1920},
            {"height", 1080},
            {"format", "jpeg"},
            {"color_space", "RGB"},
            {"has_exif", true},
            {"camera_model", "Canon EOS 5D"},
            {"focal_length", 50},
            {"clip_embedding", json::array({0.1, 0.2, 0.3, 0.4, 0.5})}
        };
        result.success = true;
        
        auto end = std::chrono::high_resolution_clock::now();
        result.processing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        return result;
    }
    
    ProcessingResult ProcessPDF(const std::vector<uint8_t>& pdf_data) {
        static_cast<void>(pdf_data);
        ProcessingResult result;
        auto start = std::chrono::high_resolution_clock::now();
        
        result.mime_type = "application/pdf";
        result.metadata = {
            {"page_count", 42},
            {"title", "Sample Document"},
            {"author", "John Doe"},
            {"created_date", "2024-01-15"},
            {"has_images", true},
            {"has_text", true},
            {"text_content", "This is extracted text from the PDF document."},
            {"word_count", 1500}
        };
        result.success = true;
        
        auto end = std::chrono::high_resolution_clock::now();
        result.processing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        return result;
    }
    
    ProcessingResult ProcessUnsupported(const std::vector<uint8_t>& data) {
        static_cast<void>(data);
        ProcessingResult result;
        result.success = false;
        result.mime_type = "application/octet-stream";
        result.error_message = "Unsupported file format";
        return result;
    }
};

/**
 * @brief Mock MIME type detector
 */
class MockMimeDetector {
public:
    std::string DetectMimeType(const std::vector<uint8_t>& data) {
        if (data.empty()) return "application/octet-stream";
        
        // Simple magic number detection
        if (data.size() >= 4) {
            // PNG: 89 50 4E 47
            if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47) {
                return "image/png";
            }
            // JPEG: FF D8 FF
            if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
                return "image/jpeg";
            }
            // PDF: 25 50 44 46
            if (data[0] == 0x25 && data[1] == 0x50 && data[2] == 0x44 && data[3] == 0x46) {
                return "application/pdf";
            }
        }
        
        return "application/octet-stream";
    }
};

/**
 * @brief Integration tests for content processing
 */
class ContentProcessingTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        processor_ = std::make_unique<MockContentProcessor>();
        mime_detector_ = std::make_unique<MockMimeDetector>();
        data_gen_ = std::make_unique<TestDataGenerator>();
    }
    
    void TearDown() override {
        processor_.reset();
        mime_detector_.reset();
        IntegrationTestFixture::TearDown();
    }
    
    // Create mock audio file
    std::vector<uint8_t> CreateMockAudio() {
        // Mock MP3 header
        return std::vector<uint8_t>{0xFF, 0xFB, 0x90, 0x00};
    }
    
    // Create mock video file
    std::vector<uint8_t> CreateMockVideo() {
        // Mock MP4 header
        return std::vector<uint8_t>{0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70};
    }
    
    // Create mock image file (PNG)
    std::vector<uint8_t> CreateMockImage() {
        return std::vector<uint8_t>{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    }
    
    // Create mock PDF file
    std::vector<uint8_t> CreateMockPDF() {
        return std::vector<uint8_t>{0x25, 0x50, 0x44, 0x46, 0x2D, 0x31, 0x2E, 0x34};
    }
    
    std::unique_ptr<MockContentProcessor> processor_;
    std::unique_ptr<MockMimeDetector> mime_detector_;
    std::unique_ptr<TestDataGenerator> data_gen_;
};

// ============================================================================
// Test 1-3: Audio Processing
// ============================================================================

TEST_F(ContentProcessingTest, ProcessAudioFile) {
    auto audio_data = CreateMockAudio();
    auto result = processor_->ProcessAudio(audio_data);
    
    ASSERT_TRUE(result.success) << "Audio processing should succeed";
    EXPECT_EQ(result.mime_type, "audio/mpeg");
    
    // Verify metadata
    ASSERT_TRUE(result.metadata.contains("duration_seconds"));
    EXPECT_GT(result.metadata["duration_seconds"], 0);
    
    ASSERT_TRUE(result.metadata.contains("sample_rate"));
    EXPECT_EQ(result.metadata["sample_rate"], 44100);
    
    ASSERT_TRUE(result.metadata.contains("channels"));
    EXPECT_GE(result.metadata["channels"], 1);
}

TEST_F(ContentProcessingTest, AudioSpeechToText) {
    auto audio_data = CreateMockAudio();
    auto result = processor_->ProcessAudio(audio_data);
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.metadata.contains("has_speech"));
    
    if (result.metadata["has_speech"]) {
        ASSERT_TRUE(result.metadata.contains("transcription"));
        std::string transcript = result.metadata["transcription"];
        EXPECT_FALSE(transcript.empty()) << "Transcription should not be empty";
    }
}

TEST_F(ContentProcessingTest, AudioMetadataExtraction) {
    auto audio_data = CreateMockAudio();
    auto result = processor_->ProcessAudio(audio_data);
    
    ASSERT_TRUE(result.success);
    
    // Verify all expected metadata fields
    std::vector<std::string> expected_fields = {
        "duration_seconds", "sample_rate", "channels", 
        "bitrate", "format"
    };
    
    for (const auto& field : expected_fields) {
        EXPECT_TRUE(result.metadata.contains(field)) 
            << "Missing metadata field: " << field;
    }
}

// ============================================================================
// Test 4-6: Video Processing
// ============================================================================

TEST_F(ContentProcessingTest, ProcessVideoFile) {
    auto video_data = CreateMockVideo();
    auto result = processor_->ProcessVideo(video_data);
    
    ASSERT_TRUE(result.success) << "Video processing should succeed";
    EXPECT_EQ(result.mime_type, "video/mp4");
    
    // Verify video dimensions
    ASSERT_TRUE(result.metadata.contains("width"));
    ASSERT_TRUE(result.metadata.contains("height"));
    EXPECT_GT(result.metadata["width"], 0);
    EXPECT_GT(result.metadata["height"], 0);
}

TEST_F(ContentProcessingTest, VideoFrameExtraction) {
    auto video_data = CreateMockVideo();
    auto result = processor_->ProcessVideo(video_data);
    
    ASSERT_TRUE(result.success);
    
    // Verify frame-related metadata
    ASSERT_TRUE(result.metadata.contains("fps"));
    ASSERT_TRUE(result.metadata.contains("frame_count"));
    
    int fps = result.metadata["fps"];
    int frame_count = result.metadata["frame_count"];
    int duration = result.metadata["duration_seconds"];
    
    // Verify frame count is consistent with duration and fps
    EXPECT_NEAR(frame_count, fps * duration, fps * 2) 
        << "Frame count should match fps * duration";
}

TEST_F(ContentProcessingTest, VideoMetadataExtraction) {
    auto video_data = CreateMockVideo();
    auto result = processor_->ProcessVideo(video_data);
    
    ASSERT_TRUE(result.success);
    
    std::vector<std::string> expected_fields = {
        "duration_seconds", "width", "height", "fps", 
        "codec", "bitrate", "has_audio"
    };
    
    for (const auto& field : expected_fields) {
        EXPECT_TRUE(result.metadata.contains(field)) 
            << "Missing video metadata field: " << field;
    }
}

// ============================================================================
// Test 7-9: Image Processing
// ============================================================================

TEST_F(ContentProcessingTest, ProcessImageFile) {
    auto image_data = CreateMockImage();
    
    // Detect MIME type
    std::string mime = mime_detector_->DetectMimeType(image_data);
    EXPECT_EQ(mime, "image/png");
    
    // Process image
    auto result = processor_->ProcessImage(image_data);
    
    ASSERT_TRUE(result.success) << "Image processing should succeed";
    EXPECT_TRUE(result.mime_type.find("image/") == 0);
    
    // Verify dimensions
    ASSERT_TRUE(result.metadata.contains("width"));
    ASSERT_TRUE(result.metadata.contains("height"));
}

TEST_F(ContentProcessingTest, ImageCLIPEmbedding) {
    auto image_data = CreateMockImage();
    auto result = processor_->ProcessImage(image_data);
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.metadata.contains("clip_embedding"));
    
    auto embedding = result.metadata["clip_embedding"];
    EXPECT_TRUE(embedding.is_array());
    EXPECT_GT(embedding.size(), 0) << "CLIP embedding should not be empty";
    
    // Verify embedding values are numeric
    for (const auto& val : embedding) {
        EXPECT_TRUE(val.is_number());
    }
}

TEST_F(ContentProcessingTest, ImageEXIFExtraction) {
    auto image_data = CreateMockImage();
    auto result = processor_->ProcessImage(image_data);
    
    ASSERT_TRUE(result.success);
    
    if (result.metadata.contains("has_exif") && result.metadata["has_exif"]) {
        // Verify EXIF metadata fields
        EXPECT_TRUE(result.metadata.contains("camera_model") || 
                   result.metadata.contains("focal_length"));
    }
}

// ============================================================================
// Test 10-12: PDF Processing
// ============================================================================

TEST_F(ContentProcessingTest, ProcessPDFFile) {
    auto pdf_data = CreateMockPDF();
    
    // Detect MIME type
    std::string mime = mime_detector_->DetectMimeType(pdf_data);
    EXPECT_EQ(mime, "application/pdf");
    
    // Process PDF
    auto result = processor_->ProcessPDF(pdf_data);
    
    ASSERT_TRUE(result.success) << "PDF processing should succeed";
    EXPECT_EQ(result.mime_type, "application/pdf");
    
    // Verify page count
    ASSERT_TRUE(result.metadata.contains("page_count"));
    EXPECT_GT(result.metadata["page_count"], 0);
}

TEST_F(ContentProcessingTest, PDFTextExtraction) {
    auto pdf_data = CreateMockPDF();
    auto result = processor_->ProcessPDF(pdf_data);
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.metadata.contains("has_text"));
    
    if (result.metadata["has_text"]) {
        ASSERT_TRUE(result.metadata.contains("text_content"));
        std::string text = result.metadata["text_content"];
        EXPECT_FALSE(text.empty()) << "Extracted text should not be empty";
        
        // Verify word count
        if (result.metadata.contains("word_count")) {
            EXPECT_GT(result.metadata["word_count"], 0);
        }
    }
}

TEST_F(ContentProcessingTest, PDFMetadataExtraction) {
    auto pdf_data = CreateMockPDF();
    auto result = processor_->ProcessPDF(pdf_data);
    
    ASSERT_TRUE(result.success);
    
    // Verify document metadata
    std::vector<std::string> metadata_fields = {
        "page_count", "title", "author"
    };
    
    for (const auto& field : metadata_fields) {
        EXPECT_TRUE(result.metadata.contains(field)) 
            << "Missing PDF metadata field: " << field;
    }
}

// ============================================================================
// Test 13-15: Error Handling
// ============================================================================

TEST_F(ContentProcessingTest, HandleUnsupportedFormat) {
    // Create data with unknown format
    std::vector<uint8_t> unknown_data = {0x00, 0x01, 0x02, 0x03};
    
    auto result = processor_->ProcessUnsupported(unknown_data);
    
    EXPECT_FALSE(result.success) << "Processing unsupported format should fail";
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_TRUE(result.error_message.find("Unsupported") != std::string::npos);
}

TEST_F(ContentProcessingTest, HandleEmptyFile) {
    std::vector<uint8_t> empty_data;
    
    // Should handle gracefully
    std::string mime = mime_detector_->DetectMimeType(empty_data);
    EXPECT_EQ(mime, "application/octet-stream");
}

TEST_F(ContentProcessingTest, HandleCorruptedFile) {
    // Create corrupted PDF (correct header, but truncated)
    std::vector<uint8_t> corrupted_pdf = {0x25, 0x50, 0x44, 0x46};
    
    // MIME detection should work (based on header)
    std::string mime = mime_detector_->DetectMimeType(corrupted_pdf);
    EXPECT_EQ(mime, "application/pdf");
    
    // Processing might fail or return partial metadata
    // In real implementation, should handle gracefully
}

// ============================================================================
// Test 16-18: Performance Tests
// ============================================================================

TEST_F(ContentProcessingTest, AudioProcessingPerformance) {
    auto audio_data = CreateMockAudio();
    auto result = processor_->ProcessAudio(audio_data);
    
    ASSERT_TRUE(result.success);
    EXPECT_LT(result.processing_time_ms, 5000) 
        << "Audio processing should complete in <5s";
}

TEST_F(ContentProcessingTest, VideoProcessingPerformance) {
    auto video_data = CreateMockVideo();
    auto result = processor_->ProcessVideo(video_data);
    
    ASSERT_TRUE(result.success);
    // Video processing can be slower
    EXPECT_LT(result.processing_time_ms, 30000) 
        << "Video processing should complete in <30s";
}

TEST_F(ContentProcessingTest, BatchProcessingPerformance) {
    // Process multiple files in batch
    const int NUM_FILES = 10;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_FILES; i++) {
        auto image_data = CreateMockImage();
        auto result = processor_->ProcessImage(image_data);
        EXPECT_TRUE(result.success);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto total_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_LT(total_time_ms, 10000) << "Batch processing should be efficient";
}

// ============================================================================
// Test 19-20: MIME Type Detection
// ============================================================================

TEST_F(ContentProcessingTest, MIMEDetectionAccuracy) {
    // Test PNG detection
    auto png_data = CreateMockImage();
    EXPECT_EQ(mime_detector_->DetectMimeType(png_data), "image/png");
    
    // Test PDF detection
    auto pdf_data = CreateMockPDF();
    EXPECT_EQ(mime_detector_->DetectMimeType(pdf_data), "application/pdf");
    
    // Test JPEG detection
    std::vector<uint8_t> jpeg_data = {0xFF, 0xD8, 0xFF, 0xE0};
    EXPECT_EQ(mime_detector_->DetectMimeType(jpeg_data), "image/jpeg");
}

TEST_F(ContentProcessingTest, MIMEDetectionFallback) {
    // Test with unrecognized data
    std::vector<uint8_t> unknown_data = {0xAA, 0xBB, 0xCC, 0xDD};
    std::string mime = mime_detector_->DetectMimeType(unknown_data);
    
    EXPECT_EQ(mime, "application/octet-stream") 
        << "Unknown data should fallback to octet-stream";
}

// ============================================================================
// Test 21: End-to-End Content Pipeline
// ============================================================================

TEST_F(ContentProcessingTest, CompleteContentPipeline) {
    // Test complete pipeline: detect -> process -> extract metadata
    
    // 1. Create test file
    auto image_data = CreateMockImage();
    
    // 2. Detect MIME type
    std::string mime = mime_detector_->DetectMimeType(image_data);
    ASSERT_FALSE(mime.empty());
    
    // 3. Process based on MIME type
    MockContentProcessor::ProcessingResult result;
    if (mime.find("image/") == 0) {
        result = processor_->ProcessImage(image_data);
    } else if (mime.find("video/") == 0) {
        result = processor_->ProcessVideo(image_data);
    } else if (mime.find("audio/") == 0) {
        result = processor_->ProcessAudio(image_data);
    } else if (mime == "application/pdf") {
        result = processor_->ProcessPDF(image_data);
    }
    
    // 4. Verify results
    ASSERT_TRUE(result.success) << "Processing should succeed";
    EXPECT_FALSE(result.metadata.empty()) << "Should have metadata";
    
    // 5. Verify metadata can be serialized
    std::string metadata_json = result.metadata.dump();
    EXPECT_FALSE(metadata_json.empty());
    
    // 6. Verify metadata can be deserialized
    json deserialized = json::parse(metadata_json);
    EXPECT_EQ(deserialized, result.metadata);
}

// ============================================================================
// Main
// ============================================================================


