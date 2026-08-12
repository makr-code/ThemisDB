/**
 * @file test_prompt_manager_multimodal.cpp
 * @brief Tests for multi-modal prompt support (image descriptions alongside text)
 */

#include <gtest/gtest.h>
#include "prompt_engineering/prompt_manager.h"

using namespace themis::prompt_engineering;

// ============================================================================
// ImageDescription construction and serialization
// ============================================================================

TEST(PromptManagerMultiModalTest, ImageDescriptionToJson) {
    PromptManager::ImageDescription img;
    img.alt_text    = "A chart showing revenue growth";
    img.url         = "https://example.com/chart.png";
    img.description = "Bar chart comparing Q1 and Q2 revenue";
    img.mime_type   = "image/png";

    auto j = img.toJson();
    EXPECT_EQ(j["alt_text"],    "A chart showing revenue growth");
    EXPECT_EQ(j["url"],         "https://example.com/chart.png");
    EXPECT_EQ(j["description"], "Bar chart comparing Q1 and Q2 revenue");
    EXPECT_EQ(j["mime_type"],   "image/png");
}

TEST(PromptManagerMultiModalTest, ImageDescriptionFromJson) {
    nlohmann::json j = {
        {"alt_text",    "Contract signature page"},
        {"url",         "data:image/jpeg;base64,/9j/4AAQ"},
        {"description", "Scanned signature of John Doe"},
        {"mime_type",   "image/jpeg"}
    };

    auto img = PromptManager::ImageDescription::fromJson(j);
    EXPECT_EQ(img.alt_text,    "Contract signature page");
    EXPECT_EQ(img.url,         "data:image/jpeg;base64,/9j/4AAQ");
    EXPECT_EQ(img.description, "Scanned signature of John Doe");
    EXPECT_EQ(img.mime_type,   "image/jpeg");
}

TEST(PromptManagerMultiModalTest, ImageDescriptionDefaultMimeType) {
    PromptManager::ImageDescription img;
    img.alt_text = "some image";
    // mime_type intentionally left empty

    auto j = img.toJson();
    EXPECT_EQ(j["mime_type"], "image/jpeg");
}

TEST(PromptManagerMultiModalTest, ImageDescriptionFromJsonDefaultMimeType) {
    nlohmann::json j = {{"alt_text", "photo"}};
    auto img = PromptManager::ImageDescription::fromJson(j);
    EXPECT_EQ(img.mime_type, "image/jpeg");
    EXPECT_EQ(img.url, "");
    EXPECT_EQ(img.description, "");
}

// ============================================================================
// PromptTemplate.toJson() includes images
// ============================================================================

TEST(PromptManagerMultiModalTest, TemplateToJsonIncludesImages) {
    PromptManager::PromptTemplate t;
    t.name    = "vision-prompt";
    t.version = "v1";
    t.content = "Describe what you see: {query}";

    PromptManager::ImageDescription img;
    img.alt_text  = "courtroom diagram";
    img.mime_type = "image/png";
    t.images.push_back(img);

    auto j = t.toJson();
    ASSERT_TRUE(j.contains("images"));
    ASSERT_TRUE(j["images"].is_array());
    ASSERT_EQ(j["images"].size(), 1u);
    EXPECT_EQ(j["images"][0]["alt_text"], "courtroom diagram");
    EXPECT_EQ(j["images"][0]["mime_type"], "image/png");
}

TEST(PromptManagerMultiModalTest, TemplateToJsonEmptyImages) {
    PromptManager::PromptTemplate t;
    t.name    = "text-only";
    t.version = "v1";
    t.content = "Hello {name}";

    auto j = t.toJson();
    ASSERT_TRUE(j.contains("images"));
    EXPECT_TRUE(j["images"].empty());
}

// ============================================================================
// validateTemplate checks image alt_text
// ============================================================================

TEST(PromptManagerMultiModalTest, ValidateTemplateWithValidImage) {
    PromptManager::PromptTemplate t;
    t.name    = "vtest";
    t.version = "1";
    t.content = "Analyze: {doc}";
    t.description = "Test";

    PromptManager::ImageDescription img;
    img.alt_text = "document scan";
    t.images.push_back(img);

    auto result = PromptManager::validateTemplate(t);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST(PromptManagerMultiModalTest, ValidateTemplateRejectsEmptyAltText) {
    PromptManager::PromptTemplate t;
    t.name    = "vtest";
    t.version = "1";
    t.content = "Analyze: {doc}";
    t.description = "Test";

    PromptManager::ImageDescription img;
    img.alt_text = "";  // empty – must fail
    img.url      = "https://example.com/img.jpg";
    t.images.push_back(img);

    auto result = PromptManager::validateTemplate(t);
    EXPECT_FALSE(result.valid);
    ASSERT_EQ(result.errors.size(), 1u);
    EXPECT_NE(result.errors[0].find("Image[0]"), std::string::npos);
    EXPECT_NE(result.errors[0].find("alt_text"), std::string::npos);
}

TEST(PromptManagerMultiModalTest, ValidateTemplateMultipleImageErrors) {
    PromptManager::PromptTemplate t;
    t.name    = "multi-img";
    t.version = "1";
    t.content = "Analyze all";
    t.description = "Test";

    // Two images, both with empty alt_text
    PromptManager::ImageDescription img1, img2;
    img1.alt_text = "";
    img2.alt_text = "";
    t.images.push_back(img1);
    t.images.push_back(img2);

    auto result = PromptManager::validateTemplate(t);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.errors.size(), 2u);
}

// ============================================================================
// buildMultiModalPrompt – text-only template (no images)
// ============================================================================

TEST(PromptManagerMultiModalTest, BuildMultiModalPromptTextOnly) {
    PromptManager::PromptTemplate t;
    t.name    = "text";
    t.version = "v1";
    t.content = "Hello {name}!";

    std::string out = PromptManager::buildMultiModalPrompt(t, {{"name", "World"}});
    EXPECT_EQ(out, "Hello World!");
    EXPECT_EQ(out.find("[Images]"), std::string::npos);
}

// ============================================================================
// buildMultiModalPrompt – with image descriptions
// ============================================================================

TEST(PromptManagerMultiModalTest, BuildMultiModalPromptWithImages) {
    PromptManager::PromptTemplate t;
    t.name    = "vision";
    t.version = "v1";
    t.content = "Describe the document: {type}";

    PromptManager::ImageDescription img;
    img.alt_text    = "Contract page 1";
    img.description = "First page of the rental agreement";
    img.url         = "https://example.com/contract_p1.jpg";
    img.mime_type   = "image/jpeg";
    t.images.push_back(img);

    std::string out = PromptManager::buildMultiModalPrompt(t, {{"type", "rental contract"}});

    // Text part with context injection
    EXPECT_NE(out.find("Describe the document: rental contract"), std::string::npos);

    // Image block
    EXPECT_NE(out.find("[Images]"), std::string::npos);
    EXPECT_NE(out.find("1. [image/jpeg] Contract page 1"), std::string::npos);
    EXPECT_NE(out.find("Description: First page of the rental agreement"), std::string::npos);
    EXPECT_NE(out.find("URL: https://example.com/contract_p1.jpg"), std::string::npos);
}

TEST(PromptManagerMultiModalTest, BuildMultiModalPromptMultipleImages) {
    PromptManager::PromptTemplate t;
    t.name    = "multi";
    t.version = "v1";
    t.content = "Compare these images";

    PromptManager::ImageDescription img1, img2;
    img1.alt_text = "Before photo";
    img1.mime_type = "image/png";
    img2.alt_text = "After photo";
    img2.mime_type = "image/png";
    t.images.push_back(img1);
    t.images.push_back(img2);

    std::string out = PromptManager::buildMultiModalPrompt(t);

    EXPECT_NE(out.find("1. [image/png] Before photo"), std::string::npos);
    EXPECT_NE(out.find("2. [image/png] After photo"), std::string::npos);
}

TEST(PromptManagerMultiModalTest, BuildMultiModalPromptImageWithoutOptionalFields) {
    PromptManager::PromptTemplate t;
    t.name    = "minimal-img";
    t.version = "v1";
    t.content = "Analyze";

    PromptManager::ImageDescription img;
    img.alt_text = "exhibit A";
    // url and description deliberately omitted
    t.images.push_back(img);

    std::string out = PromptManager::buildMultiModalPrompt(t);

    EXPECT_NE(out.find("1. [image/jpeg] exhibit A"), std::string::npos);
    // Optional fields should not appear
    EXPECT_EQ(out.find("Description:"), std::string::npos);
    EXPECT_EQ(out.find("URL:"), std::string::npos);
}

// ============================================================================
// createTemplate / getTemplate round-trip with images
// ============================================================================

TEST(PromptManagerMultiModalTest, CreateAndRetrieveTemplateWithImages) {
    PromptManager pm;

    PromptManager::PromptTemplate t;
    t.name    = "multimodal-template";
    t.version = "v1";
    t.content = "Analyze this {doc_type}";
    t.description = "Multi-modal analysis prompt";

    PromptManager::ImageDescription img;
    img.alt_text    = "exhibit scan";
    img.url         = "https://example.com/exhibit.jpg";
    img.description = "Scanned court exhibit";
    img.mime_type   = "image/jpeg";
    t.images.push_back(img);

    auto created = pm.createTemplate(t);
    ASSERT_FALSE(created.id.empty());

    auto retrieved = pm.getTemplate(created.id);
    ASSERT_TRUE(retrieved.has_value());
    ASSERT_EQ(retrieved->images.size(), 1u);
    EXPECT_EQ(retrieved->images[0].alt_text,    "exhibit scan");
    EXPECT_EQ(retrieved->images[0].url,         "https://example.com/exhibit.jpg");
    EXPECT_EQ(retrieved->images[0].description, "Scanned court exhibit");
    EXPECT_EQ(retrieved->images[0].mime_type,   "image/jpeg");
}

TEST(PromptManagerMultiModalTest, CreateTemplateWithEmptyAltTextFails) {
    PromptManager pm;

    PromptManager::PromptTemplate t;
    t.name    = "bad-image";
    t.version = "v1";
    t.content = "Analyze this";
    t.description = "Test";

    PromptManager::ImageDescription img;
    img.alt_text = "";  // invalid
    t.images.push_back(img);

    auto created = pm.createTemplate(t);
    EXPECT_TRUE(created.id.empty());  // Sentinel on validation failure
}
