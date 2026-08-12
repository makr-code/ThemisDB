/**
 * @file test_aql_multimodal.cpp
 * @brief Unit tests for MultiModalInferRequest and MultiModalInput.
 *
 * Tests cover:
 *  - Default construction of all types
 *  - ModalityType enum values
 *  - MIME type validation per modality (valid and invalid)
 *  - Binary payload empty-byte rejection
 *  - MultiModalInferRequest helpers: addInput(), validateInputs(),
 *    hasNonTextInputs()
 *  - MultiModalInferRequest inherits llm::InferenceRequest fields
 *  - Allowlist accessors return non-empty sets
 */

#include <gtest/gtest.h>
#include "aql/multimodal_infer_request.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

using namespace themis::aql;

// ============================================================================
// ModalityType enum tests
// ============================================================================

TEST(ModalityTypeTest, EnumValuesExist) {
    ModalityType t_text  = ModalityType::TEXT;
    ModalityType t_image = ModalityType::IMAGE;
    ModalityType t_audio = ModalityType::AUDIO;
    ModalityType t_video = ModalityType::VIDEO;

    EXPECT_NE(t_text,  t_image);
    EXPECT_NE(t_image, t_audio);
    EXPECT_NE(t_audio, t_video);
}

// ============================================================================
// MultiModalInput construction
// ============================================================================

TEST(MultiModalInputTest, DefaultConstruction) {
    MultiModalInput inp;
    EXPECT_EQ(inp.type, ModalityType::TEXT);
    EXPECT_TRUE(inp.mime_type.empty());
    EXPECT_TRUE(inp.label.empty());
}

// ============================================================================
// MIME type validation — TEXT modality
// ============================================================================

TEST(MultiModalInputTest, Validate_TextPlain_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::TEXT;
    inp.mime_type = "text/plain";
    inp.data      = std::string("hello");
    EXPECT_NO_THROW(inp.validate());
}

TEST(MultiModalInputTest, Validate_TextHtml_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::TEXT;
    inp.mime_type = "text/html";
    inp.data      = std::string("<p>hi</p>");
    EXPECT_NO_THROW(inp.validate());
}

TEST(MultiModalInputTest, Validate_ApplicationJson_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::TEXT;
    inp.mime_type = "application/json";
    inp.data      = std::string("{}");
    EXPECT_NO_THROW(inp.validate());
}

TEST(MultiModalInputTest, Validate_ApplicationXml_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::TEXT;
    inp.mime_type = "application/xml";
    inp.data      = std::string("<root/>");
    EXPECT_NO_THROW(inp.validate());
}

TEST(MultiModalInputTest, Validate_ApplicationOctetStream_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::TEXT;
    inp.mime_type = "application/octet-stream";
    inp.data      = std::string("raw");
    EXPECT_NO_THROW(inp.validate());
}

TEST(MultiModalInputTest, Validate_UnknownTextMime_Throws) {
    MultiModalInput inp;
    inp.type      = ModalityType::TEXT;
    inp.mime_type = "image/png";  // wrong modality
    inp.data      = std::string("data");
    EXPECT_THROW(inp.validate(), std::invalid_argument);
}

TEST(MultiModalInputTest, Validate_EmptyMime_Throws) {
    MultiModalInput inp;
    inp.type      = ModalityType::TEXT;
    inp.mime_type = "";
    inp.data      = std::string("hello");
    EXPECT_THROW(inp.validate(), std::invalid_argument);
}

// ============================================================================
// MIME type validation — IMAGE modality
// ============================================================================

TEST(MultiModalInputTest, Validate_ImagePng_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::IMAGE;
    inp.mime_type = "image/png";
    inp.data      = std::vector<uint8_t>{0x89, 0x50, 0x4E, 0x47};  // PNG magic bytes
    EXPECT_NO_THROW(inp.validate());
}

TEST(MultiModalInputTest, Validate_ImageJpeg_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::IMAGE;
    inp.mime_type = "image/jpeg";
    inp.data      = std::vector<uint8_t>{0xFF, 0xD8, 0xFF};  // JPEG magic bytes
    EXPECT_NO_THROW(inp.validate());
}

TEST(MultiModalInputTest, Validate_ImageWebp_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::IMAGE;
    inp.mime_type = "image/webp";
    inp.data      = std::vector<uint8_t>{0x52, 0x49, 0x46, 0x46};
    EXPECT_NO_THROW(inp.validate());
}

TEST(MultiModalInputTest, Validate_ImageInvalidMime_Throws) {
    MultiModalInput inp;
    inp.type      = ModalityType::IMAGE;
    inp.mime_type = "audio/wav";  // wrong modality
    inp.data      = std::vector<uint8_t>{1, 2, 3};
    EXPECT_THROW(inp.validate(), std::invalid_argument);
}

TEST(MultiModalInputTest, Validate_ImageEmptyBytes_Throws) {
    MultiModalInput inp;
    inp.type      = ModalityType::IMAGE;
    inp.mime_type = "image/png";
    inp.data      = std::vector<uint8_t>{};  // empty payload
    EXPECT_THROW(inp.validate(), std::invalid_argument);
}

TEST(MultiModalInputTest, Validate_ImageFromFilePath_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::IMAGE;
    inp.mime_type = "image/png";
    inp.data      = std::filesystem::path("/tmp/photo.png");
    EXPECT_NO_THROW(inp.validate());  // file paths are not checked for existence on disk
}

// ============================================================================
// MIME type validation — AUDIO modality
// ============================================================================

TEST(MultiModalInputTest, Validate_AudioWav_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::AUDIO;
    inp.mime_type = "audio/wav";
    inp.data      = std::vector<uint8_t>{0x52, 0x49, 0x46, 0x46};  // RIFF header
    EXPECT_NO_THROW(inp.validate());
}

TEST(MultiModalInputTest, Validate_AudioMpeg_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::AUDIO;
    inp.mime_type = "audio/mpeg";
    inp.data      = std::vector<uint8_t>{0xFF, 0xFB};
    EXPECT_NO_THROW(inp.validate());
}

TEST(MultiModalInputTest, Validate_AudioInvalidMime_Throws) {
    MultiModalInput inp;
    inp.type      = ModalityType::AUDIO;
    inp.mime_type = "video/mp4";  // wrong modality
    inp.data      = std::vector<uint8_t>{1, 2, 3};
    EXPECT_THROW(inp.validate(), std::invalid_argument);
}

// ============================================================================
// MIME type validation — VIDEO modality
// ============================================================================

TEST(MultiModalInputTest, Validate_VideoMp4_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::VIDEO;
    inp.mime_type = "video/mp4";
    inp.data      = std::vector<uint8_t>{0x00, 0x00, 0x00, 0x18};
    EXPECT_NO_THROW(inp.validate());
}

TEST(MultiModalInputTest, Validate_VideoWebm_Passes) {
    MultiModalInput inp;
    inp.type      = ModalityType::VIDEO;
    inp.mime_type = "video/webm";
    inp.data      = std::vector<uint8_t>{0x1A, 0x45, 0xDF, 0xA3};  // EBML header
    EXPECT_NO_THROW(inp.validate());
}

TEST(MultiModalInputTest, Validate_VideoInvalidMime_Throws) {
    MultiModalInput inp;
    inp.type      = ModalityType::VIDEO;
    inp.mime_type = "image/gif";  // wrong modality
    inp.data      = std::vector<uint8_t>{1, 2, 3};
    EXPECT_THROW(inp.validate(), std::invalid_argument);
}

// ============================================================================
// Allowlist accessors
// ============================================================================

TEST(MultiModalInputTest, ImageMimeTypesNonEmpty) {
    EXPECT_FALSE(MultiModalInput::imageMimeTypes().empty());
    EXPECT_TRUE(MultiModalInput::imageMimeTypes().count("image/png"));
    EXPECT_TRUE(MultiModalInput::imageMimeTypes().count("image/jpeg"));
}

TEST(MultiModalInputTest, AudioMimeTypesNonEmpty) {
    EXPECT_FALSE(MultiModalInput::audioMimeTypes().empty());
    EXPECT_TRUE(MultiModalInput::audioMimeTypes().count("audio/wav"));
}

TEST(MultiModalInputTest, VideoMimeTypesNonEmpty) {
    EXPECT_FALSE(MultiModalInput::videoMimeTypes().empty());
    EXPECT_TRUE(MultiModalInput::videoMimeTypes().count("video/mp4"));
}

// ============================================================================
// MultiModalInferRequest construction and helpers
// ============================================================================

TEST(MultiModalInferRequestTest, DefaultConstruction) {
    MultiModalInferRequest req;
    EXPECT_TRUE(req.inputs.empty());
}

TEST(MultiModalInferRequestTest, InheritsInferenceRequestFields) {
    MultiModalInferRequest req;
    req.prompt   = "Describe this image.";
    req.model_id = "llava-1.5-7b";

    EXPECT_EQ(req.prompt, "Describe this image.");
    EXPECT_EQ(req.model_id, "llava-1.5-7b");
}

TEST(MultiModalInferRequestTest, AddInput_ValidInput_Succeeds) {
    MultiModalInferRequest req;

    MultiModalInput img;
    img.type      = ModalityType::IMAGE;
    img.mime_type = "image/png";
    img.data      = std::vector<uint8_t>{0x89, 0x50};

    EXPECT_NO_THROW(req.addInput(img));
    EXPECT_EQ(req.inputs.size(), std::size_t(1));
}

TEST(MultiModalInferRequestTest, AddInput_InvalidInput_ThrowsAndDoesNotAppend) {
    MultiModalInferRequest req;

    MultiModalInput bad;
    bad.type      = ModalityType::IMAGE;
    bad.mime_type = "";  // invalid: empty MIME

    EXPECT_THROW(req.addInput(bad), std::invalid_argument);
    EXPECT_TRUE(req.inputs.empty());
}

TEST(MultiModalInferRequestTest, ValidateInputs_AllValid_Succeeds) {
    MultiModalInferRequest req;

    MultiModalInput text_inp;
    text_inp.type      = ModalityType::TEXT;
    text_inp.mime_type = "text/plain";
    text_inp.data      = std::string("caption");

    MultiModalInput img;
    img.type      = ModalityType::IMAGE;
    img.mime_type = "image/jpeg";
    img.data      = std::vector<uint8_t>{0xFF, 0xD8, 0xFF};

    req.inputs.push_back(text_inp);  // bypass addInput for raw insertion
    req.inputs.push_back(img);

    EXPECT_NO_THROW(req.validateInputs());
}

TEST(MultiModalInferRequestTest, ValidateInputs_OneInvalid_Throws) {
    MultiModalInferRequest req;

    MultiModalInput bad;
    bad.type      = ModalityType::IMAGE;
    bad.mime_type = "text/plain";  // wrong modality
    bad.data      = std::string("not an image");

    req.inputs.push_back(bad);  // raw insert to bypass validation in addInput

    EXPECT_THROW(req.validateInputs(), std::invalid_argument);
}

TEST(MultiModalInferRequestTest, HasNonTextInputs_TextOnly_ReturnsFalse) {
    MultiModalInferRequest req;

    MultiModalInput txt;
    txt.type      = ModalityType::TEXT;
    txt.mime_type = "text/plain";
    txt.data      = std::string("hello");
    req.inputs.push_back(txt);

    EXPECT_FALSE(req.hasNonTextInputs());
}

TEST(MultiModalInferRequestTest, HasNonTextInputs_WithImage_ReturnsTrue) {
    MultiModalInferRequest req;

    MultiModalInput img;
    img.type      = ModalityType::IMAGE;
    img.mime_type = "image/png";
    img.data      = std::vector<uint8_t>{1, 2, 3};
    req.inputs.push_back(img);

    EXPECT_TRUE(req.hasNonTextInputs());
}

TEST(MultiModalInferRequestTest, HasNonTextInputs_EmptyInputs_ReturnsFalse) {
    MultiModalInferRequest req;
    EXPECT_FALSE(req.hasNonTextInputs());
}

TEST(MultiModalInferRequestTest, MultipleInputTypes) {
    MultiModalInferRequest req;

    // Add a text input
    MultiModalInput txt;
    txt.type      = ModalityType::TEXT;
    txt.mime_type = "text/plain";
    txt.data      = std::string("What is in the video?");
    req.addInput(txt);

    // Add a video input via file path
    MultiModalInput vid;
    vid.type      = ModalityType::VIDEO;
    vid.mime_type = "video/mp4";
    vid.data      = std::filesystem::path("/tmp/clip.mp4");
    req.addInput(vid);

    EXPECT_EQ(req.inputs.size(), std::size_t(2));
    EXPECT_TRUE(req.hasNonTextInputs());
    EXPECT_NO_THROW(req.validateInputs());
}
