/**
 * @file multimodal_infer_request.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/llm_plugin_interface.h"
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace themis {
namespace aql {

// ============================================================================
// ModalityType
// ============================================================================

enum class ModalityType {
    TEXT,   ///< Plain text or structured text (e.g. HTML, Markdown)
    IMAGE,  ///< Raster image (JPEG, PNG, GIF, WebP, BMP, TIFF)
    AUDIO,  ///< Audio recording (WAV, MP3, OGG, FLAC, AAC, M4A)
    VIDEO,  ///< Video clip (MP4, WEBM, AVI, MOV, MKV)
};

// ============================================================================
// MultiModalInput
// ============================================================================

struct MultiModalInput {
    ModalityType type = ModalityType::TEXT;

    std::variant<
        std::string,
        std::vector<uint8_t>,
        std::filesystem::path
    > data;

    std::string mime_type;

    std::string label;

    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------

    void validate() const {
        if (mime_type.empty()) {
            throw std::invalid_argument("MultiModalInput: mime_type must not be empty");
        }

        switch (type) {
            case ModalityType::TEXT:
                validateTextMime();
                break;
            case ModalityType::IMAGE:
                validateMimeInSet(imageMimeTypes(), "IMAGE");
                validateNonEmptyBinaryIfBytes();
                break;
            case ModalityType::AUDIO:
                validateMimeInSet(audioMimeTypes(), "AUDIO");
                validateNonEmptyBinaryIfBytes();
                break;
            case ModalityType::VIDEO:
                validateMimeInSet(videoMimeTypes(), "VIDEO");
                validateNonEmptyBinaryIfBytes();
                break;
        }
    }

    /**
     * @brief ------------------------------------------------------------------------- Allowlist accessors (static so tests can inspect them) -------------------------------------------------------------------------
     * @return Return value.
     * @details Implements imageMimeTypes without additional internal calls.
     */

    static const std::unordered_set<std::string>& imageMimeTypes() {
        static const std::unordered_set<std::string> kTypes = {
            "image/jpeg", "image/png", "image/gif",
            "image/webp", "image/bmp", "image/tiff",
        };
        return kTypes;
    }

    /**
     * @brief Audio Mime Types.
     * @return Return value.
     * @details Implements audioMimeTypes without additional internal calls.
     */
    static const std::unordered_set<std::string>& audioMimeTypes() {
        static const std::unordered_set<std::string> kTypes = {
            "audio/wav", "audio/mpeg", "audio/ogg",
            "audio/flac", "audio/aac", "audio/mp4", "audio/x-m4a",
        };
        return kTypes;
    }

    /**
     * @brief Video Mime Types.
     * @return Return value.
     * @details Implements videoMimeTypes without additional internal calls.
     */
    static const std::unordered_set<std::string>& videoMimeTypes() {
        static const std::unordered_set<std::string> kTypes = {
            "video/mp4", "video/webm", "video/avi",
            "video/quicktime", "video/x-matroska",
        };
        return kTypes;
    }

private:
    void validateTextMime() const {
        // Accept any "text/*", "application/json", "application/xml",
        // or "application/octet-stream".
        static const std::unordered_set<std::string> kExtras = {
            "application/json", "application/xml", "application/octet-stream",
        };
        if (mime_type.rfind("text/", 0) == 0) {
          return;
        }
        if (kExtras.count(mime_type)) {
          return;
        }
        throw std::invalid_argument(
            "MultiModalInput: unsupported TEXT mime_type '" + mime_type + "'"
        );
    }

    void validateMimeInSet(const std::unordered_set<std::string>& allowed,
                           const char* modality_name) const {
        if (!allowed.count(mime_type)) {
            throw std::invalid_argument(
                std::string("MultiModalInput: unsupported ") + modality_name +
                " mime_type '" + mime_type + "'"
            );
        }
    }

    void validateNonEmptyBinaryIfBytes() const {
        if (const auto* bytes = std::get_if<std::vector<uint8_t>>(&data)) {
            if (bytes->empty()) {
                throw std::invalid_argument(
                    "MultiModalInput: binary payload must not be empty"
                );
            }
        }
    }
};

// ============================================================================
// MultiModalInferRequest
// ============================================================================

struct MultiModalInferRequest : public llm::InferenceRequest {
    std::vector<MultiModalInput> inputs;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Add Input.
     * @param[in] input Input parameter.
     * @details Calls: validate(), push_back().
     */
    void addInput(const MultiModalInput& input) {
        input.validate();
        inputs.push_back(input);
    }

    void validateInputs() const {
        for (const auto& inp : inputs) {
            inp.validate();
        }
    }

    bool hasNonTextInputs() const {
        for (const auto& inp : inputs) {
            if (inp.type != ModalityType::TEXT) {
              return true;
            }
        }
        return false;
    }
};

} // namespace aql
} // namespace themis
