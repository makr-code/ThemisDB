/**
 * @file kernel_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace gpu {

/**
 * @brief Kernel integrity validator.
 *
 * Provides a two-layer defence for GPU kernel blobs before they are
 * submitted for execution:
 *
 * 1. **Checksum verification** — a simple FNV-1a 64-bit hash of the kernel
 *    blob bytes is compared against a pre-registered expected value.  This
 *    detects accidental corruption and tampered blobs.
 *
 * 2. **Whitelist enforcement** — only kernel IDs that have been explicitly
 *    registered with `registerKernel()` are allowed to run.  Any unknown
 *    kernel ID is rejected regardless of its checksum.
 *
 * Design notes
 * ------------
 * - FNV-1a is used for speed and simplicity; a production deployment should
 *   replace or augment it with a cryptographic hash (SHA-256 or BLAKE3) and
 *   an HMAC/signature check.
 * - The validator holds no runtime GPU handles.  It operates purely on
 *   memory blobs passed by the caller.
 *
 * Thread safety: all methods are protected by an internal mutex.
 */
class GPUKernelValidator {
public:
    // -----------------------------------------------------------------------
    // Validation result
    // -----------------------------------------------------------------------
    enum class Status {
        OK,                     ///< Kernel is known and checksum matches
        UNKNOWN_KERNEL,         ///< kernel_id not in the whitelist
        CHECKSUM_MISMATCH,      ///< Blob does not match registered checksum
        EMPTY_BLOB,             ///< Zero-length blob rejected
    };

    struct ValidationResult {
        Status      status   = Status::UNKNOWN_KERNEL;
        std::string kernel_id;
        uint64_t    computed_checksum  = 0;
        uint64_t    expected_checksum  = 0;
        std::string message;
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static GPUKernelValidator& GetInstance() {
        static GPUKernelValidator inst;
        return inst;
    }

    // -----------------------------------------------------------------------
    // Registry management
    // -----------------------------------------------------------------------

    /**
     * @brief Register a kernel with its expected checksum.
     *
     * @param kernel_id  Unique identifier (e.g. "vector_dot_fp32").
     * @param expected_checksum  FNV-1a 64-bit hash of the canonical blob.
     */
    void registerKernel(const std::string& kernel_id,
                        uint64_t expected_checksum);

    /**
     * @brief Register a kernel by computing its checksum from the blob.
     * @param kernel_id  Unique identifier (e.g. "vector_dot_fp32").
     * @param canonical_blob Canonical byte sequence for checksum computation.
     */
    void registerKernel(const std::string& kernel_id,
                        const std::vector<uint8_t>& canonical_blob);

    /**
     * @brief Remove a kernel from the whitelist.
     */
    void unregisterKernel(const std::string& kernel_id);

    /**
     * @brief Return true when @p kernel_id is whitelisted.
     */
    bool isRegistered(const std::string& kernel_id) const;

    /**
     * @brief Return all registered kernel IDs.
     */
    std::vector<std::string> registeredKernels() const;

    // -----------------------------------------------------------------------
    // Validation
    // -----------------------------------------------------------------------

    /**
     * @brief Validate @p blob against the registered checksum for
     *        @p kernel_id.
     *
     * @return ValidationResult describing the outcome.
     */
    ValidationResult validate(const std::string& kernel_id,
                               const std::vector<uint8_t>& blob) const;

    /**
     * @brief Convenience method — returns true iff validation succeeds.
     */
    bool isValid(const std::string& kernel_id,
                  const std::vector<uint8_t>& blob) const;

    // -----------------------------------------------------------------------
    // Checksum utility (public for testing / pre-registration use)
    // -----------------------------------------------------------------------

    /**
     * @brief Compute FNV-1a 64-bit checksum of @p data.
     */
    static uint64_t computeChecksum(const std::vector<uint8_t>& data);

    /**
     * @brief Compute FNV-1a 64-bit checksum of a raw byte range.
     */
    static uint64_t computeChecksum(const uint8_t* data, size_t length);

    // -----------------------------------------------------------------------
    // Stats
    // -----------------------------------------------------------------------
    struct Stats {
        size_t registered_count      = 0;
        size_t total_validations     = 0;
        size_t ok_count              = 0;
        size_t unknown_kernel_count  = 0;
        size_t checksum_mismatch_count = 0;
        size_t empty_blob_count      = 0;
    };

    Stats getStats() const;

    /**
     * @brief Clear all registered kernels and reset stats (for testing).
     */
    void reset();

private:
    GPUKernelValidator() = default;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, uint64_t> registry_;  // id → expected checksum

    // Mutable stats counters (updated under mutex_).
    mutable size_t total_validations_      = 0;
    mutable size_t ok_count_               = 0;
    mutable size_t unknown_kernel_count_   = 0;
    mutable size_t checksum_mismatch_count_ = 0;
    mutable size_t empty_blob_count_       = 0;
};

} // namespace gpu
} // namespace themis
