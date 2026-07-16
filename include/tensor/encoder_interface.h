/**
 * @file encoder_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace tensor {

// Forward declaration — UTRConfig is defined in utr_converter.h.
struct UTRConfig;

// ============================================================================
// EncoderQuality — quality tier for encoder outputs
// ============================================================================

/**
 * @brief Semantic quality tier reported by `ITextEncoder` and `IImageEncoder`.
 *
 * Used to detect degraded operation when a high-quality encoder backend is
 * not available and the system has fallen back to a lower-tier implementation.
 */
enum class EncoderQuality : uint8_t {
    SEMANTIC = 0, ///< Learned / neural encoder (highest quality)
    LEXICAL  = 1, ///< Statistical / n-gram feature encoder (medium quality)
    HASH     = 2, ///< Hash-projection fallback (lowest quality — degraded mode)
};

// ============================================================================
// ITextEncoder — abstract interface for text segment encoders
// ============================================================================

/**
 * @brief Abstract interface for fixed-length text segment encoders.
 *
 * Implementations must be thread-safe: `encode()` may be called concurrently
 * from multiple threads.
 *
 * ### Contract
 * - `encode()` MUST return a vector of exactly `embed_dim` elements.
 * - If the encoder cannot produce an output it MUST throw `std::runtime_error`
 *   rather than returning a wrong-sized vector.
 * - `isAvailable()` MUST be cheap (no I/O, no locking) and stable within a
 *   single `UTRConverter::fromDocument()` call.
 */
class ITextEncoder {
public:
    virtual ~ITextEncoder() = default;

    /**
     * @brief Encode a UTF-8 text segment into a dense float vector.
     *
     * @param segment   UTF-8 encoded text segment (non-empty).
     * @param embed_dim Required output dimensionality (> 0).
     * @return Float vector of exactly `embed_dim` elements.
     *
     * @throws std::runtime_error if the encoder is unavailable, the segment
     *         cannot be processed, or the implementation produces a vector
     *         of the wrong size.
     * @throws std::invalid_argument if `embed_dim` is 0 or `segment` is empty.
     */
    [[nodiscard]] virtual std::vector<float>
    encode(const std::string& segment, std::size_t embed_dim) const = 0;

    /**
     * @brief Returns true when the encoder is ready to accept requests.
     *
     * A false return causes `UTRConverter::fromDocument()` to fall back to the
     * next lower-tier encoder without throwing.
     */
    [[nodiscard]] virtual bool isAvailable() const noexcept = 0;

    /**
     * @brief Returns the semantic quality tier of this encoder.
     */
    [[nodiscard]] virtual EncoderQuality quality() const noexcept = 0;

    /**
     * @brief Human-readable description for logging and diagnostics.
     *
     * @return A stable, non-null string view (e.g. "BERT-base quantised Q4_0").
     */
    [[nodiscard]] virtual std::string_view description() const noexcept = 0;
};

// ============================================================================
// IImageEncoder — abstract interface for image encoders
// ============================================================================

/**
 * @brief Abstract interface for image-to-TTTrain encoders.
 *
 * Implementations must be thread-safe: `encode()` may be called concurrently
 * from multiple threads.
 *
 * ### Contract
 * - The returned `TTTrain` MUST be non-empty (at least one core).
 * - If the encoder cannot produce an output it MUST throw `std::runtime_error`.
 * - `isAvailable()` MUST be cheap and stable within a single
 *   `UTRConverter::fromImage()` call.
 */
class IImageEncoder {
public:
    virtual ~IImageEncoder() = default;

    /**
     * @brief Encode an HWC image into a TT-train.
     *
     * @param pixels  Flat HWC float buffer (length = h × w × c, raw not
     *                pre-normalised).
     * @param h       Image height in pixels (> 0).
     * @param w       Image width in pixels (> 0).
     * @param c       Number of channels (> 0).
     * @param cfg     UTR configuration (eps, max_rank, etc.)
     * @return A non-empty TTTrain encoding the image.
     *
     * @throws std::runtime_error if the encoder is unavailable or fails to
     *         produce a valid output.
     * @throws std::invalid_argument if h, w, or c is 0, or
     *         `pixels.size() != h * w * c`.
     */
    [[nodiscard]] virtual storage::TTTrain
    encode(const std::vector<float>& pixels,
           std::size_t h, std::size_t w, std::size_t c,
           const UTRConfig& cfg) const = 0;

    /**
     * @brief Returns true when the encoder is ready to accept requests.
     *
     * A false return causes `UTRConverter::fromImage()` to fall back to the
     * next lower-tier encoder without throwing.
     */
    [[nodiscard]] virtual bool isAvailable() const noexcept = 0;

    /**
     * @brief Returns the semantic quality tier of this encoder.
     */
    [[nodiscard]] virtual EncoderQuality quality() const noexcept = 0;

    /**
     * @brief Human-readable description for logging and diagnostics.
     *
     * @return A stable, non-null string view (e.g. "ViT-B/16 patch encoder").
     */
    [[nodiscard]] virtual std::string_view description() const noexcept = 0;
};

} // namespace tensor
} // namespace themis
