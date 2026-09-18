/**
 * @file fips_crypto_mode.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <stdexcept>

namespace themis {

class FipsPolicyViolation : public std::runtime_error {
public:
    /**
     * @brief Fips Policy Violation.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    explicit FipsPolicyViolation(const std::string& message)
        : std::runtime_error("FIPS policy violation: " + message)
    {}
};

class FipsCryptoMode {
public:
    /**
     * @brief Instance.
     * @return Return value.
     */
    static FipsCryptoMode& instance();

    /**
     * @brief Enable.
     * @return True when the operation succeeds.
     */
    bool enable();

    /**
     * @brief Disable.
     */
    void disable();

    /**
     * @brief Is Enabled.
     * @return True when the operation succeeds.
     */
    bool isEnabled() const;

    /**
     * @brief Is Available.
     * @return True when the operation succeeds.
     */
    bool isAvailable() const;

    /**
     * @brief Validate Algorithm.
     * @param[in] algorithm Input parameter.
     */
    void validateAlgorithm(const std::string& algorithm) const;

    /**
     * @brief Approved Algorithms.
     * @return Return value.
     */
    const std::unordered_set<std::string>& approvedAlgorithms() const;

    /**
     * @brief Run Self Tests.
     * @return True when the operation succeeds.
     */
    bool runSelfTests() const;

    /**
     * @brief Zeroize.
     * @param[in,out] ptr Input/output parameter.
     * @param[in] len Input parameter.
     * @note Exception safety: noexcept.
     */
    static void zeroize(void* ptr, std::size_t len) noexcept;

    // Non-copyable / non-movable singleton
    FipsCryptoMode(const FipsCryptoMode&)            = delete;
    FipsCryptoMode& operator=(const FipsCryptoMode&) = delete;
    FipsCryptoMode(FipsCryptoMode&&)                 = delete;
    FipsCryptoMode& operator=(FipsCryptoMode&&)      = delete;

private:
    FipsCryptoMode();
    ~FipsCryptoMode();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace themis
