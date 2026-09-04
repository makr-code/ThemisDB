/**
 * @file multimodal_infer_request.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Enumerates the supported input modality types for multi-modal inference.
 *
 * Each modality carries a distinct set of valid MIME types enforced via
 * MultiModalInput::validate().
 */
enum class ModalityType {
    TEXT,   ///< Plain text or structured text (e.g. HTML, Markdown)
    IMAGE,  ///< Raster image (JPEG, PNG, GIF, WebP, BMP, TIFF)
    AUDIO,  ///< Audio recording (WAV, MP3, OGG, FLAC, AAC, M4A)
    VIDEO,  ///< Video clip (MP4, WEBM, AVI, MOV, MKV)
};

// ============================================================================
// MultiModalInput
// ============================================================================

/**
 * @brief A single modality input for a multi-modal inference request.
 *
 * Carries either inline data (text string or raw bytes) or a filesystem path
 * to a media file, together with a MIME type that is validated before any
 * execution begins.
 *
 * Security guarantees:
 *  - MIME type must be non-empty and belong to the modality's allowlist.
 *  - Byte payloads carrying an IMAGE/AUDIO/VIDEO modality must be non-empty.
 *  - validate() must pass before the input is consumed by an inference engine.
 *
 * Usage:
 * @code
 *   MultiModalInput img;
 *   img.type      = ModalityType::IMAGE;
 *   img.mime_type = "image/png";
 *   img.data      = std::vector<uint8_t>{...bytes...};
 *   img.validate();  // throws on invalid state
 * @endcode
 */
struct MultiModalInput {
    /// The modality category of this input.
    ModalityType type = ModalityType::TEXT;

    /**
     * @brief The payload, one of:
     *  - std::string              – text content or URL string
     *  - std::vector<uint8_t>     – raw media bytes (e.g. image/audio/video)
     *  - std::filesystem::path    – path to a media file on disk
     */
    std::variant<
        std::string,
        std::vector<uint8_t>,
        std::filesystem::path
    > data;

    /**
     * @brief IANA MIME type (e.g. "image/png", "audio/wav").
     *
     * Must be non-empty and must belong to the allowlist for @c type before
     * the input is submitted to an inference engine.
     */
    std::string mime_type;

    /**
     * @brief Optional human-readable label for logging and debugging.
     *
     * Not used for inference; may be empty.
     */
    std::string label;

    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------

    /**
     * @brief Validate this input against modality-specific MIME allowlists.
     *
     * @throws std::invalid_argument if mime_type is empty, if the MIME type
     *         does not belong to the allowlist for @c type, or if a binary
     *         payload is empty when a non-TEXT modality is specified.
     *
     * Text modality: accepts any non-empty MIME type starting with "text/"
     *                as well as "application/json", "application/xml", and
     *                "application/octet-stream".
     * Image modality: "image/jpeg", "image/png", "image/gif", "image/webp",
     *                  "image/bmp", "image/tiff".
     * Audio modality: "audio/wav", "audio/mpeg", "audio/ogg", "audio/flac",
     *                  "audio/aac", "audio/mp4", "audio/x-m4a".
     * Video modality: "video/mp4", "video/webm", "video/avi", "video/quicktime",
     *                  "video/x-matroska".
     */
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

    // -------------------------------------------------------------------------
    // Allowlist accessors (static so tests can inspect them)
    // -------------------------------------------------------------------------

    static const std::unordered_set<std::string>& imageMimeTypes() {
        static const std::unordered_set<std::string> kTypes = {
            "image/jpeg", "image/png", "image/gif",
            "image/webp", "image/bmp", "image/tiff",
        };
        return kTypes;
    }

    static const std::unordered_set<std::string>& audioMimeTypes() {
        static const std::unordered_set<std::string> kTypes = {
            "audio/wav", "audio/mpeg", "audio/ogg",
            "audio/flac", "audio/aac", "audio/mp4", "audio/x-m4a",
        };
        return kTypes;
    }

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

/**
 * @brief An inference request that carries one or more typed modality inputs.
 *
 * Extends @c llm::InferenceRequest so that all existing single-modality
 * parameters (prompt, model_id, temperature, stop_sequences, …) remain
 * accessible.  Multi-modal inputs are appended via the @c inputs vector and
 * merged with the base request's text prompt by the inference engine.
 *
 * All inputs **must** pass their individual validate() checks before the
 * request is dispatched to an inference engine.  Call validateInputs() to
 * validate the entire inputs vector at once.
 *
 * Usage:
 * @code
 *   MultiModalInferRequest req;
 *   req.prompt    = "Describe the image.";
 *   req.model_id  = "llava-1.5-7b";
 *
 *   MultiModalInput img;
 *   img.type      = ModalityType::IMAGE;
 *   img.mime_type = "image/png";
 *   img.data      = loadImageBytes("photo.png");
 *   req.addInput(img);
 *
 *   req.validateInputs();  // throws on invalid state
 * @endcode
 */
struct MultiModalInferRequest : public llm::InferenceRequest {
    /// Ordered list of modality inputs accompanying the text prompt.
    std::vector<MultiModalInput> inputs;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Append a validated multi-modal input to this request.
     *
     * @param input  Input to append. validate() is called before appending.
     * @throws std::invalid_argument if @p input fails validation.
     */
    void addInput(const MultiModalInput& input) {
        input.validate();
        inputs.push_back(input);
    }

    /**
     * @brief Validate every input in the @c inputs vector.
     *
     * @throws std::invalid_argument if any input fails its validate() check.
     */
    void validateInputs() const {
        for (const auto& inp : inputs) {
            inp.validate();
        }
    }

    /**
     * @brief Return true if this request contains at least one non-TEXT input.
     *
     * Useful for inference engines that need to select a vision/audio model.
     */
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
