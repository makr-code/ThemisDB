/**
 * @file confidential_computing.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace themis {
namespace security {

/**
 * @brief TEE (Trusted Execution Environment) technology type
 *
 * Identifies the confidential computing platform detected at runtime.
 */
enum class TeeType {
    NONE,       ///< No TEE hardware detected; software-only fallback
    INTEL_TDX,  ///< Intel Trust Domain Extensions (VM-level isolation)
    AMD_SEV,    ///< AMD Secure Encrypted Virtualization (memory encryption)
    AMD_SEV_SNP ///< AMD SEV + Secure Nested Paging (memory integrity)
};

/**
 * @brief Result of TEE detection at startup
 */
struct TeeDetectionResult {
    TeeType type = TeeType::NONE;
    std::string description;    ///< Human-readable technology name
    bool hardware_attested = false; ///< True when the kernel driver confirmed TEE presence
};

/**
 * @brief Attestation report produced by the TEE
 *
 * For Intel TDX this is a TDREPORT structure (1024 bytes) returned by
 * the TDX_CMD_GET_REPORT0 ioctl on /dev/tdx_guest.
 *
 * For AMD SEV-SNP this is an snp_report_t (1184 bytes) returned by the
 * SNP_GET_REPORT ioctl on /dev/sev-guest.
 *
 * When no TEE is present a software-synthesised report (empty raw_report,
 * populated fields only) is returned so callers can always use the same API.
 */
struct TeeAttestationReport {
    TeeType tee_type = TeeType::NONE;
    std::vector<uint8_t> report_data;  ///< 64-byte user-supplied nonce / challenge
    std::vector<uint8_t> raw_report;   ///< Raw attestation bytes from the kernel driver
    std::string tee_version;           ///< e.g. "TDX 1.0", "SEV-SNP 1.51"
    bool is_genuine = false;           ///< True only when raw_report is from real HW

    bool empty() const { return raw_report.empty(); }
};

/**
 * @brief Sealed blob — data bound to the current TEE measurement
 *
 * Unsealing will fail if the enclave measurement changes (e.g. after a
 * software update), providing "measured-boot" style binding.
 *
 * In software-fallback mode the blob is AES-256-GCM encrypted with a
 * process-lifetime key derived from OpenSSL RAND_bytes; it will NOT
 * survive process restart.  Real TDX/SEV implementations should persist
 * measurement-bound key material through a remote KMS.
 */
struct SealedBlob {
    TeeType tee_type = TeeType::NONE;
    std::vector<uint8_t> iv;         ///< 12-byte AES-GCM IV
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> tag;        ///< 16-byte AES-GCM tag
    std::vector<uint8_t> measurement_binding; ///< TEE measurement at seal time
};

/**
 * @brief Confidential Computing support for Intel TDX and AMD SEV/SEV-SNP
 *
 * This class provides:
 * - Runtime detection of the active TEE technology (CPUID + kernel driver)
 * - Attestation report generation via the respective Linux kernel drivers
 * - Sealed memory: AES-256-GCM encryption bound to TEE measurements
 * - Software fallback for non-TEE environments (CI, developer machines)
 *
 * ### Linux kernel interfaces used
 * - Intel TDX: `ioctl(/dev/tdx_guest, TDX_CMD_GET_REPORT0)` (kernel ≥ 6.2)
 * - AMD SEV-SNP: `ioctl(/dev/sev-guest, SNP_GET_REPORT)` (kernel ≥ 5.19)
 *
 * ### Usage
 * ```cpp
 * auto cc = ConfidentialComputing::create();
 * auto detection = cc->detect();
 * if (detection.type != TeeType::NONE) {
 *     auto report = cc->getAttestationReport(nonce);
 *     // send report to remote verifier ...
 * }
 * auto sealed = cc->seal(sensitive_key_bytes);
 * auto recovered = cc->unseal(sealed);
 * ```
 */
class ConfidentialComputing {
public:
    virtual ~ConfidentialComputing() = default;

    // Non-copyable, movable
    ConfidentialComputing(const ConfidentialComputing&) = delete;
    ConfidentialComputing& operator=(const ConfidentialComputing&) = delete;
    ConfidentialComputing(ConfidentialComputing&&) = default;
    ConfidentialComputing& operator=(ConfidentialComputing&&) = default;

    /**
     * @brief Factory: detect the active TEE and return the appropriate
     *        implementation.  Falls back to software mode when no TEE HW
     *        is present.
     */
    static std::unique_ptr<ConfidentialComputing> create();

    /**
     * @brief Detect the active TEE technology.
     *
     * Performs CPUID enumeration followed by an optional kernel-driver
     * probe to confirm genuine hardware presence.
     *
     * @return Detection result (type == NONE on ordinary hardware)
     */
    [[nodiscard]] virtual TeeDetectionResult detect() const = 0;

    /**
     * @brief Generate a TEE attestation report.
     *
     * @param report_data  Up to 64 bytes of caller-supplied data (nonce /
     *                     challenge) to embed in the report so the verifier
     *                     can bind the quote to a specific request.
     * @return             Attestation report; raw_report is empty and
     *                     is_genuine == false in software-fallback mode.
     */
    [[nodiscard]] virtual TeeAttestationReport getAttestationReport(
        const std::vector<uint8_t>& report_data) const = 0;

    /**
     * @brief Seal (encrypt-and-bind) data to the current TEE measurement.
     *
     * @param plaintext  Data to protect.
     * @return           SealedBlob that can only be unsealed by the same
     *                   (or compatible) TEE measurement.
     * @throws std::runtime_error on encryption failure.
     */
    [[nodiscard]] virtual SealedBlob seal(const std::vector<uint8_t>& plaintext) const = 0;

    /**
     * @brief Unseal a previously sealed blob.
     *
     * @param blob  Blob produced by seal().
     * @return      Recovered plaintext.
     * @throws std::runtime_error if authentication or measurement check fails.
     */
    [[nodiscard]] virtual std::vector<uint8_t> unseal(const SealedBlob& blob) const = 0;

    /**
     * @brief Human-readable name of the active TEE (or "Software fallback").
     */
    [[nodiscard]] virtual std::string name() const = 0;

protected:
    ConfidentialComputing() = default;
};

/**
 * @brief Convert TeeType enum to a human-readable string.
 */
inline std::string teeTypeToString(TeeType t) {
    switch (t) {
        case TeeType::INTEL_TDX:  return "Intel TDX";
        case TeeType::AMD_SEV:    return "AMD SEV";
        case TeeType::AMD_SEV_SNP: return "AMD SEV-SNP";
        default:                  return "None";
    }
}

} // namespace security
} // namespace themis
