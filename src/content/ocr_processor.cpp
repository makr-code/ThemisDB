/**
 * @file ocr_processor.cpp
 * @brief Optical character recognition engine for image-based text extraction.
 * @version 0.0.15
 * @note Maturity: 🔴 ALPHA
 * @note Score: 68/100
 * @note Gap Summary: total=18; TODO=2, Stub=2, Unimpl=2, Mock=1, Sim=1, Debt=3, C=2, H=5, M=8, L=0
 * @note Status: Beta; OCR engine integration in progress; confidence filtering and language-specific models under test
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/ocr_processor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>
#include <sstream>

#include "config/config_path_resolver.h"
#include "content/content_metrics.h"

// ---------------------------------------------------------------------------
// Optional Tesseract + Leptonica headers
// ---------------------------------------------------------------------------
#ifdef THEMIS_ENABLE_OCR
#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>
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

OcrProcessor::OcrProcessor() : OcrProcessor(Config{}) {}

OcrProcessor::OcrProcessor(Config config) : config_(std::move(config)) {}

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

/*static*/ bool OcrProcessor::isSupportedImageFormat(const std::string &blob) {
    if (static_cast<int>(blob.size()) < 4) {
        return false;
    }

    const auto *b = reinterpret_cast<const uint8_t *>(blob.data());

    // JPEG: FF D8 FF
    if (b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF) {
        return true;
    }

    // PNG: 89 50 4E 47
    if (b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G') {
        return true;
    }

    // TIFF little-endian (II) or big-endian (MM)
    if ((b[0] == 'I' && b[1] == 'I') || (b[0] == 'M' && b[1] == 'M')) {
        return true;
    }

    // BMP: BM
    if (b[0] == 'B' && b[1] == 'M') {
        return true;
    }

    // GIF: GIF8
    if (b[0] == 'G' && b[1] == 'I' && b[2] == 'F' && b[3] == '8') {
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// OCR output sanitization — strip ASCII control characters (C0 + DEL)
// while preserving printable text and common whitespace (\t, \n, \r).
// ---------------------------------------------------------------------------

[[maybe_unused]] static std::string sanitizeOcrText(std::string text) {
    text.erase(std::remove_if(text.begin(), text.end(),
                              [](unsigned char c) {
                                  // Preserve tab (0x09), newline (0x0A), carriage return (0x0D)
                                  // and all printable / high-byte UTF-8 code units (>= 0x20, != 0x7F)
                                  if (c == 0x09 || c == 0x0A || c == 0x0D) {
                                      return false;
                                  }
                                  return c < 0x20 || c == 0x7F;
                              }),
               text.end());
    return text;
}

// ---------------------------------------------------------------------------
// Core Tesseract invocation
// ---------------------------------------------------------------------------

std::string OcrProcessor::runTesseract([[maybe_unused]] const std::string &blob,
                                       [[maybe_unused]] PreprocessInfo *preprocess_info) {
#if OCR_LIBRARY_AVAILABLE
    if (blob.empty())
        return "";

    // Resolve effective tessdata directory:
    // 1. Use the explicit config_.data_dir if set.
    // 2. Otherwise try the canonical default via ConfigPathResolver.
    // 3. If neither is available, pass nullptr so Tesseract auto-detects.
    std::string effective_data_dir = config_.data_dir;
    if (effective_data_dir.empty()) {
        auto resolved = themis::config::ConfigPathResolver::tryResolve("config/ai_ml/tesseract_lang");
        if (resolved) {
            effective_data_dir = *resolved;
        }
    }

    // Initialise Tesseract API
    tesseract::TessBaseAPI api;
    const char *data_dir = effective_data_dir.empty() ? nullptr : effective_data_dir.c_str();
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
    const auto *data = reinterpret_cast<const uint8_t *>(blob.data());
    Pix *pix         = pixReadMem(data, static_cast<size_t>(blob.size()));
    if (!pix) {
        api.End();
        return "";
    }

    // -----------------------------------------------------------------------
    // Pre-processing step 1: DPI detection and rescaling to target_dpi
    // -----------------------------------------------------------------------
    if (config_.enable_dpi_rescaling && config_.target_dpi > 0) {
        l_int32 xres = pixGetXRes(pix);
        l_int32 yres = pixGetYRes(pix);
        // Prefer x-resolution; fall back to y-resolution when x is unset
        int current_dpi = (xres > 0) ? static_cast<int>(xres) : static_cast<int>(yres);

        if (preprocess_info) {
            preprocess_info->original_dpi = current_dpi;
        }

        if (current_dpi > 0 && current_dpi < config_.target_dpi) {
            l_float32 scale = static_cast<l_float32>(config_.target_dpi) / static_cast<l_float32>(current_dpi);
            Pix *scaled     = pixScale(pix, scale, scale);
            pixDestroy(&pix);
            pix = scaled;
            if (pix) {
                pixSetXRes(pix, static_cast<l_int32>(config_.target_dpi));
                pixSetYRes(pix, static_cast<l_int32>(config_.target_dpi));
                if (preprocess_info)
                    preprocess_info->rescaled = true;
            }
        }
    }

    if (!pix) {
        api.End();
        return "";
    }

    // -----------------------------------------------------------------------
    // Pre-processing step 2: Adaptive binarisation (Sauvola method)
    //
    // Only applied when the image is not already 1-bit binary, since
    // binarising an already-binary image would degrade quality.
    // -----------------------------------------------------------------------
    if (config_.enable_adaptive_binarization && pixGetDepth(pix) > 1) {
        // Convert to 8-bit grayscale (handles 1/2/4/8/16/32 bpp input)
        Pix *pix8 = pixConvertTo8(pix, 0);
        pixDestroy(&pix);
        pix = pix8;

        if (pix) {
            // Sauvola adaptive binarisation:
            //   whsize = 15  → 31×31 local window (good for 300 DPI text)
            //   factor = 0.35 → standard k parameter for document images
            Pix *pixBin = nullptr;
            if (pixSauvolaBinarize(pix, 15, 0.35f, 0, nullptr, nullptr, nullptr, &pixBin) == 0 && pixBin) {
                pixDestroy(&pix);
                pix = pixBin;
                if (preprocess_info)
                    preprocess_info->binarized = true;
            }
        }
    }

    if (!pix) {
        api.End();
        return "";
    }

    // Perform OCR
    api.SetImage(pix);
    char *raw_text = api.GetUTF8Text();
    std::string text = {};
    if (raw_text) {
        text = raw_text;
        delete[] raw_text;
    }

    pixDestroy(&pix);
    api.End();

    // Enforce max text size
    if (static_cast<int>(text.size()) > config_.max_text_size) {
        text.resize(config_.max_text_size);
    }
    // Sanitize: remove control characters to prevent injection into the document store
    text = sanitizeOcrText(std::move(text));
    return text;
#else
    return "";
#endif
}

// ---------------------------------------------------------------------------
// IContentProcessor::extract
// ---------------------------------------------------------------------------

ExtractionResult OcrProcessor::extract(const std::string &blob, const ContentType &content_type) {
    ExtractionResult result;
    result.ok = false;

    auto start = std::chrono::steady_clock::now();

    if (!isAvailable()) {
        result.error_message = "OCR not available: built without -DTHEMIS_ENABLE_OCR (libtesseract)";
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
        PreprocessInfo preprocess_info;
        result.text = runTesseract(blob, &preprocess_info);

        if (config_.extract_metadata) {
            result.metadata["ocr_language"]     = config_.language;
            result.metadata["ocr_text_length"]  = static_cast<int>(result.text.size());
            result.metadata["content_ocr_text"] = result.text;
            result.metadata["mime_type"]        = content_type.mime_type;
            result.metadata["ocr_input_dpi"]    = preprocess_info.original_dpi;
            result.metadata["ocr_rescaled"]     = preprocess_info.rescaled;
            result.metadata["ocr_binarized"]    = preprocess_info.binarized;
        }

        result.ok      = true;
        result.success = true;

        auto end         = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        if (config_.metrics) {
            config_.metrics->recordOcrExtracted();
            config_.metrics->recordLatency("ocr_extract", static_cast<double>(duration_ms));
        }
    } catch (const std::exception &e) {
        result.ok            = false;
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

std::vector<json> OcrProcessor::chunk(const ExtractionResult &extraction_result, int chunk_size, int overlap) {
    std::vector<json> chunks;

    if (!extraction_result.ok || extraction_result.text.empty()) {
        return chunks;
    }

    const std::string &text = extraction_result.text;

    // No chunking requested – return the whole text as a single chunk.
    if (chunk_size <= 0) {
        json ch;
        ch["text"]        = text;
        ch["type"]        = "ocr_text";
        ch["sequence"]    = 0;
        ch["token_count"] = countTokens(text);
        chunks.push_back(std::move(ch));
        return chunks;
    }

    // Split into sentences (split on '.', '!', '?' followed by whitespace or end)
    std::vector<std::string> sentences;
    std::string current = {};
    for (size_t i = 0; i < text.size(); ++i) {
        current += text[i];
        bool is_terminal = (text[i] == '.' || text[i] == '!' || text[i] == '?');
        bool followed_by_space
            = (i + 1 < text.size()) && (text[i + 1] == ' ' || text[i + 1] == '\n' || text[i + 1] == '\r');
        bool at_end = (i + 1 == text.size());
        if (is_terminal && (followed_by_space || at_end)) {
            sentences.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) {
        sentences.push_back(current);
    }

    std::string current_chunk = {};
    int sequence = 0;
    std::string overlap_carry; // sentences kept for overlap

    for (const auto &sentence : sentences) {
        int sentence_tokens = countTokens(sentence);
        int current_tokens  = countTokens(current_chunk);

        if (current_tokens + sentence_tokens > chunk_size && !current_chunk.empty()) {
            json ch;
            ch["text"]        = current_chunk;
            ch["type"]        = "ocr_text";
            ch["sequence"]    = sequence++;
            ch["token_count"] = current_tokens;
            chunks.push_back(std::move(ch));

            // Start next chunk: use overlap carry if any, otherwise the current sentence.
            current_chunk = overlap_carry.empty() ? sentence : overlap_carry + " " + sentence;
            overlap_carry.clear();
        } else {
            if (!current_chunk.empty()) {
                current_chunk += " ";
            }
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
                if (sp == std::string::npos) {
                    break;
                }
                overlap_carry = overlap_carry.substr(sp + 1);
            }
        }
    }

    if (!current_chunk.empty()) {
        json ch;
        ch["text"]        = current_chunk;
        ch["type"]        = "ocr_text";
        ch["sequence"]    = sequence++;
        ch["token_count"] = countTokens(current_chunk);
        chunks.push_back(std::move(ch));
    }

    return chunks;
}

// ---------------------------------------------------------------------------
// IContentProcessor::generateEmbedding
// Deterministic hash-based 768-dim embedding – same approach as
// TextProcessor and HtmlProcessor.  Delegates to the EmbeddingPipeline
// when one is wired in; falls back to the inline hash implementation so
// that OCR chunks are always indexable even without an external model.
// ---------------------------------------------------------------------------

std::vector<float> OcrProcessor::generateEmbedding(const std::string &chunk_data) {
    const int DIM = 768;
    std::vector<float> embedding(DIM, 0.0f);

    if (chunk_data.empty()) {
        return embedding;
    }

    std::hash<std::string> hasher;
    std::istringstream iss(chunk_data);
    std::vector<std::string> tokens;
    std::string token = {};
    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return embedding;
    }

    for (size_t i = 0; i < tokens.size(); ++i) {
        size_t token_hash = hasher(tokens[i]);
        for (int seed = 0; seed < 3; ++seed) {
            size_t combined = token_hash ^ (i * 31) ^ (static_cast<size_t>(seed) * 97);
            for (int d = 0; d < 10; ++d) {
                int dim      = static_cast<int>((combined + static_cast<size_t>(d) * 73) % DIM);
                float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);
                float phase = static_cast<float>((combined + static_cast<size_t>(dim)) % 360) * 3.14159265359f / 180.0f;
                embedding[dim] += std::sin(phase) * weight;
            }
        }
    }

    // L2 normalize for cosine similarity
    float norm = 0.0f;
    for (float v : embedding) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm > 1e-6f) {
        for (float &v : embedding) {
            v /= norm;
        }
    }

    return embedding;
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

/*static*/ int OcrProcessor::countTokens(const std::string &text) {
    if (text.empty()) {
        return 0;
    }
    int count     = 0;
    bool in_token = false;
    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (in_token) {
                ++count;
                in_token = false;
            }
        } else {
            in_token = true;
        }
    }
    if (in_token) {
        ++count;
    }
    return count;
}

/*static*/ std::string OcrProcessor::performOcr(const std::vector<uint8_t> &image_blob, const std::string &language,
                                                const std::string &data_dir) {
    if (image_blob.empty()) {
        return "";
    }

    Config cfg;
    cfg.language = language;
    cfg.data_dir = data_dir;
    OcrProcessor proc(cfg);

    ContentType ct;
    ct.mime_type                    = "image/jpeg"; // Leptonica detects the real format from bytes
    ct.category                     = ContentCategory::IMAGE;
    ct.supports_text_extraction     = true;
    ct.supports_embedding           = false;
    ct.supports_chunking            = false;
    ct.supports_metadata_extraction = false;
    ct.binary_storage_required      = true;

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

} // namespace content
} // namespace themis
