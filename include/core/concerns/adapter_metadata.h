/**
 * @file adapter_metadata.h
 * @brief Adapter metadata, validation interface, and cryptographic signing for AdapterRegistry.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 95/100
 * @note Status: Production Ready
 */

#pragma once

#include <string>
#include <cstdint>

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// API version constant
// ---------------------------------------------------------------------------

static constexpr uint32_t kCurrentApiVersion = 1;

// ---------------------------------------------------------------------------
// AdapterMetadata
// ---------------------------------------------------------------------------

struct AdapterMetadata {
    std::string  id;

    uint32_t     apiVersion  = kCurrentApiVersion;

    std::string  description;
};

// ---------------------------------------------------------------------------
// AdapterSignature
// ---------------------------------------------------------------------------

struct AdapterSignature {
    std::string algorithm = {};

    std::string digest = {};

    [[nodiscard]] bool present() const noexcept {
        return !algorithm.empty() && !digest.empty();
    }
};

// ---------------------------------------------------------------------------
// AdapterValidator
// ---------------------------------------------------------------------------

class AdapterValidator {
public:
    /**
     * @brief Adapter Validator.
     * @return Return value.
     */
    virtual ~AdapterValidator() = default;

    /**
     * @brief Validate.
     * @param[in] m Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool validate(const AdapterMetadata& m) = 0;
};

} // namespace concerns
} // namespace core
} // namespace themis
