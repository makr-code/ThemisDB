/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ocr_processor.cpp                                  ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:57:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     414                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 718c75097  2026-02-28  feat(content): Integrate Tesseract OCR processor (content... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file ocr_processor.cpp
 * @brief OCR Content Processor Implementation
 *
 * Extracts text from images using Tesseract OCR (libtesseract + Leptonica).
 *
 * Build with -DTHEMIS_ENABLE_OCR=ON to enable real OCR.
 * Without Tesseract, isAvailable() returns false and extract() returns a
 * skipped/unavailable result so the ingestion pipeline can continue.
 *
 * @author ThemisDB Team
 * @date February 2026
 */

#include "content/ocr_processor.h"
#include "content/content_metrics.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <sstream>

// ---------------------------------------------------------------------------
// Optional Tesseract + Leptonica headers
// ---------------------------------------------------------------------------
#ifdef THEMIS_ENABLE_OCR
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#define OCR_LIBRARY_AVAILABLE 1
#define OCR_LIBRARY_NAME "tesseract"
#else
#define OCR_LIBRARY_AVAILABLE 0
#define OCR_LIBRARY_NAME "none (built without -DTHEMIS_ENABLE_OCR)"
#endif

namespace themis {
namespace content {

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

OcrProcessor::OcrProcessor()
    : OcrProcessor(Config{})
{
}

OcrProcessor::OcrProcessor(Config config)
    : config_(std::move(config))
{
}

// ---------------------------------------------------------------------------
// Static capability queries
// ---------------------------------------------------------------------------

bool OcrProcessor::isAvailable() {
#if OCR_LIBRARY_AVAILABLE
    return true;
#else
    return false;
#endif
}

std::string OcrProcessor::getTesseractVersion() {
#if OCR_LIBRARY_AVAILABLE
    return std::string(OCR_LIBRARY_NAME " ") + tesseract::TessBaseAPI::Version();
#else
    return OCR_LIBRARY_NAME;
#endif
}

// ---------------------------------------------------------------------------
// Image format detection (magic bytes)
// ---------------------------------------------------------------------------

/*static*/ bool OcrProcessor::isSupportedImageFormat(const std::string& blob) {
    if (blob.size() < 4) return false;

    const auto* b = reinterpret_cast<const uint8_t*>(blob.data());

    // JPEG: FF D8 FF
    if (b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF) return true;

    // PNG: 89 50 4E 47
    if (b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G') return true;

    // TIFF little-endian (II) or big-endian (MM)
    if ((b[0] == 'I' && b[1] == 'I') || (b[0] == 'M' && b[1] == 'M')) return true;

    // BMP: BM
    if (b[0] == 'B' && b[1] == 'M') return true;

    // GIF: GIF8
    if (b[0] == 'G' && b[1] == 'I' && b[2] == 'F' && b[3] == '8') return true;

    return false;
}

// ---------------------------------------------------------------------------
// Core Tesseract invocation
// ---------------------------------------------------------------------------

std::string OcrProcessor::runTesseract(const std::string& blob) {
#if OCR_LIBRARY_AVAILABLE
    if (blob.empty()) return "";

    // Initialise Tesseract API
    tesseract::TessBaseAPI api;
    const char* data_dir = config_.data_dir.empty() ? nullptr : config_.data_dir.c_str();
    if (api.Init(data_dir, config_.language.c_str()) != 0) {
        return "";
    }

    // Page segmentation mode
    api.SetPageSegMode(static_cast<tesseract::PageSegMode>(config_.page_seg_mode));

    // Optional character whitelist
    if (config_.enable_char_whitelist && !config_.char_whitelist.empty()) {
        api.SetVariable("tessedit_char_whitelist", config_.char_whitelist.c_str());
    }

    // Load image from memory via Leptonica
    const auto* data = reinterpret_cast<const uint8_t*>(blob.data());
    Pix* pix = pixReadMem(data, static_cast<size_t>(blob.size()));
    if (!pix) {
        api.End();
        return "";
    }

    // Perform OCR
    api.SetImage(pix);
    char* raw_text = api.GetUTF8Text();
    std::string text;
    if (raw_text) {
        text = raw_text;
        delete[] raw_text;
    }

    pixDestroy(&pix);
    api.End();

    // Enforce max text size
    if (text.size() > config_.max_text_size) {
        text.resize(config_.max_text_size);
    }
    return text;
#else
    (void)blob;
    return "";
#endif
}

// ---------------------------------------------------------------------------
// IContentProcessor::extract
// ---------------------------------------------------------------------------

ExtractionResult OcrProcessor::extract(
    const std::string& blob,
    const ContentType& content_type
) {
    ExtractionResult result;
    result.ok = false;

    auto start = std::chrono::steady_clock::now();

    if (!isAvailable()) {
        result.error_message =
            "OCR not available: built without -DTHEMIS_ENABLE_OCR (libtesseract)";
        if (config_.metrics) {
            config_.metrics->recordExtractError();
        }
        return result;
    }

    if (blob.empty()) {
        result.error_message = "Empty image blob";
        if (config_.metrics) {
            config_.metrics->recordExtractError();
        }
        return result;
    }

    if (!isSupportedImageFormat(blob)) {
        result.error_message = "Unsupported image format for OCR";
        if (config_.metrics) {
            config_.metrics->recordExtractError();
        }
        return result;
    }

    try {
        result.text = runTesseract(blob);

        if (config_.extract_metadata) {
            result.metadata["ocr_language"] = config_.language;
            result.metadata["ocr_text_length"] = static_cast<int>(result.text.size());
            result.metadata["content_ocr_text"] = result.text;
            result.metadata["mime_type"] = content_type.mime_type;
        }

        result.ok = true;
        result.success = true;

        auto end = std::chrono::steady_clock::now();
        auto duration_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        if (config_.metrics) {
            config_.metrics->recordOcrExtracted();
            config_.metrics->recordLatency("ocr_extract", static_cast<double>(duration_ms));
        }
    } catch (const std::exception& e) {
        result.ok = false;
        result.error_message = std::string("OCR failed: ") + e.what();
        if (config_.metrics) {
            config_.metrics->recordExtractError();
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// IContentProcessor::chunk
// ---------------------------------------------------------------------------

std::vector<json> OcrProcessor::chunk(
    const ExtractionResult& extraction_result,
    int chunk_size,
    int overlap
) {
    std::vector<json> chunks;

    if (!extraction_result.ok || extraction_result.text.empty()) {
        return chunks;
    }

    const std::string& text = extraction_result.text;

    // No chunking requested – return the whole text as a single chunk.
    if (chunk_size <= 0) {
        json ch;
        ch["text"] = text;
        ch["type"] = "ocr_text";
        ch["sequence"] = 0;
        ch["token_count"] = countTokens(text);
        chunks.push_back(std::move(ch));
        return chunks;
    }

    // Split into sentences (split on '.', '!', '?' followed by whitespace or end)
    std::vector<std::string> sentences;
    std::string current;
    for (size_t i = 0; i < text.size(); ++i) {
        current += text[i];
        bool is_terminal = (text[i] == '.' || text[i] == '!' || text[i] == '?');
        bool followed_by_space =
            (i + 1 < text.size()) &&
            (text[i + 1] == ' ' || text[i + 1] == '\n' || text[i + 1] == '\r');
        bool at_end = (i + 1 == text.size());
        if (is_terminal && (followed_by_space || at_end)) {
            sentences.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) {
        sentences.push_back(current);
    }

    std::string current_chunk;
    int sequence = 0;
    std::string overlap_carry;  // sentences kept for overlap

    for (const auto& sentence : sentences) {
        int sentence_tokens = countTokens(sentence);
        int current_tokens = countTokens(current_chunk);

        if (current_tokens + sentence_tokens > chunk_size && !current_chunk.empty()) {
            json ch;
            ch["text"] = current_chunk;
            ch["type"] = "ocr_text";
            ch["sequence"] = sequence++;
            ch["token_count"] = current_tokens;
            chunks.push_back(std::move(ch));

            // Start next chunk: use overlap carry if any, otherwise the current sentence.
            current_chunk = overlap_carry.empty()
                                ? sentence
                                : overlap_carry + " " + sentence;
            overlap_carry.clear();
        } else {
            if (!current_chunk.empty()) current_chunk += " ";
            current_chunk += sentence;
        }

        // Build overlap carry: accumulate sentences until we reach 'overlap' tokens
        if (overlap > 0) {
            if (overlap_carry.empty()) {
                overlap_carry = sentence;
            } else {
                overlap_carry += " ";
                overlap_carry += sentence;
            }
            // Keep only the last 'overlap' tokens in the carry
            while (countTokens(overlap_carry) > overlap) {
                auto sp = overlap_carry.find(' ');
                if (sp == std::string::npos) break;
                overlap_carry = overlap_carry.substr(sp + 1);
            }
        }
    }

    if (!current_chunk.empty()) {
        json ch;
        ch["text"] = current_chunk;
        ch["type"] = "ocr_text";
        ch["sequence"] = sequence++;
        ch["token_count"] = countTokens(current_chunk);
        chunks.push_back(std::move(ch));
    }

    return chunks;
}

// ---------------------------------------------------------------------------
// IContentProcessor::generateEmbedding (stub – delegates to pipeline)
// ---------------------------------------------------------------------------

std::vector<float> OcrProcessor::generateEmbedding(const std::string& chunk_data) {
    (void)chunk_data;
    return {};
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

/*static*/ int OcrProcessor::countTokens(const std::string& text) {
    if (text.empty()) return 0;
    int count = 0;
    bool in_token = false;
    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (in_token) { ++count; in_token = false; }
        } else {
            in_token = true;
        }
    }
    if (in_token) ++count;
    return count;
}

/*static*/ std::string OcrProcessor::performOcr(
    const std::vector<uint8_t>& image_blob,
    const std::string& language,
    const std::string& data_dir
) {
    if (image_blob.empty()) return "";

    Config cfg;
    cfg.language = language;
    cfg.data_dir = data_dir;
    OcrProcessor proc(cfg);

    ContentType ct;
    ct.mime_type = "image/jpeg";  // Leptonica detects the real format from bytes
    ct.category = ContentCategory::IMAGE;
    ct.supports_text_extraction = true;
    ct.supports_embedding = false;
    ct.supports_chunking = false;
    ct.supports_metadata_extraction = false;
    ct.binary_storage_required = true;

    std::string blob(image_blob.begin(), image_blob.end());
    auto result = proc.extract(blob, ct);
    return result.ok ? result.text : "";
}

// ---------------------------------------------------------------------------
// Factory functions
// ---------------------------------------------------------------------------

std::unique_ptr<IContentProcessor> createOcrProcessor() {
    return std::make_unique<OcrProcessor>();
}

std::unique_ptr<IContentProcessor> createOcrProcessor(OcrProcessor::Config config) {
    return std::make_unique<OcrProcessor>(std::move(config));
}

}  // namespace content
}  // namespace themis
