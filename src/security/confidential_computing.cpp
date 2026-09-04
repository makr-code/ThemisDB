/**
 * @file confidential_computing.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=1, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * Confidential Computing support — Intel TDX and AMD SEV / SEV-SNP
 *
 * Detection strategy
 * ──────────────────
 * 1. CPUID (x86-64 only, no privileges required):
 *    - Intel TDX:    leaf 0x21, sub-leaf 0 returns "IntelTDX    " in
 *                    EBX/EDX/ECX when executed inside a TD.
 *    - AMD SEV:      leaf 0x8000_001F bit 1 of EAX indicates SEV support;
 *                    MSR 0xC001_0131 bit 0 means SEV is active in this VM.
 *    - AMD SEV-SNP:  same leaf, bit 4 of EAX; MSR bit 3 for active SNP.
 *
 * 2. Kernel driver probe (Linux, optional — graceful fallback if absent):
 *    - TDX:     /dev/tdx_guest   + TDX_CMD_GET_REPORT0 ioctl
 *    - SEV-SNP: /dev/sev-guest   + SNP_GET_REPORT ioctl
 *
 * Non-Linux or non-x86 builds fall back to software mode automatically.
 *
 * Seal / Unseal
 * ─────────────
 * Data is protected with AES-256-GCM.  The sealing key is:
 *   - TEE mode:      a 32-byte key derived from RAND_bytes(), stored
 *                    only inside the TEE memory region.  In a production
 *                    deployment this key should be persisted through a
 *                    remote attestation + KMS flow so that it survives
 *                    planned VM migration while still being bound to the
 *                    TEE measurement.
 *   - Software mode: identical mechanism; survives only within the
 *                    current process lifetime.
 *
 * The measurement_binding field in SealedBlob stores a SHA-256 digest of
 * the raw attestation bytes (or zeros in software mode) so that a future
 * unseal call can detect measurement drift and reject the blob.
 */

#include "security/confidential_computing.h"
#include "utils/logger.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <array>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// ── Platform-specific includes ────────────────────────────────────────────────
#if defined(__linux__) && defined(__x86_64__)
#  include <fcntl.h>
#  include <sys/ioctl.h>
#  include <unistd.h>
#  include <cpuid.h>   // GCC / Clang built-in
#  define THEMIS_HAS_CPUID 1
#  define THEMIS_IS_LINUX  1
#elif defined(__x86_64__) || defined(_M_X64)
#  if defined(_MSC_VER)
#    include <intrin.h>
#    define THEMIS_HAS_CPUID 1
#  elif defined(__GNUC__) || defined(__clang__)
#    include <cpuid.h>
#    define THEMIS_HAS_CPUID 1
#  endif
#endif

// ── Linux kernel ioctl constants ──────────────────────────────────────────────
//
// Defined here so the file compiles even when the system headers are absent
// (older kernels, non-Linux cross-compile targets).  The _IOW/_IOR macros
// produce the same numeric values as the upstream headers.
#if defined(THEMIS_IS_LINUX)
#  ifndef TDX_CMD_GET_REPORT0
//   From linux/tdx-guest.h (kernel ≥ 6.2)
#    define TDX_REPORTDATA_LEN  64
#    define TDX_REPORT_LEN     1024
     struct tdx_report_req {
         uint8_t reportdata[TDX_REPORTDATA_LEN];
         uint8_t tdreport[TDX_REPORT_LEN];
     };
#    define TDX_CMD_GET_REPORT0  _IOWR('T', 1, struct tdx_report_req)
#  endif

#  ifndef SNP_GET_REPORT
//   From linux/sev-guest.h (kernel ≥ 5.19)
#    define SNP_REPORT_DATA_SIZE 64
#    define SNP_REPORT_SIZE     1184
     struct snp_report_req {
         uint8_t user_data[SNP_REPORT_DATA_SIZE];
         uint32_t vmpl;
         uint8_t rsvd[28];
     };
     struct snp_report_resp {
         uint8_t data[SNP_REPORT_SIZE];
     };
     struct snp_guest_request_ioctl {
         uint8_t  msg_version;
         uint64_t req_data;   // user-space pointer to snp_report_req
         uint64_t resp_data;  // user-space pointer to snp_report_resp
         uint64_t fw_err = {};
     };
#    define SNP_GET_REPORT  _IOWR('S', 0x0, struct snp_guest_request_ioctl)
#  endif
#endif // THEMIS_IS_LINUX

// ── RAII Wrappers for OpenSSL objects ─────────────────────────────────────────
struct CC_EVP_CIPHER_CTX_Deleter {
    void operator()(EVP_CIPHER_CTX* p) const { if (p) EVP_CIPHER_CTX_free(p); }
};
using CC_EVP_CIPHER_CTX_ptr = std::unique_ptr<EVP_CIPHER_CTX, CC_EVP_CIPHER_CTX_Deleter>;

#if defined(THEMIS_IS_LINUX)
// RAII guard for POSIX file descriptors — prevents fd leaks on exception paths
struct ScopedFd {
    int fd = 0;
    explicit ScopedFd([[maybe_unused]] int f) noexcept : fd(f) {}
    ~ScopedFd() { if (fd >= 0) ::close(fd); }
    ScopedFd(const ScopedFd&) = delete;
    ScopedFd& operator=(const ScopedFd&) = delete;
    bool valid() const noexcept { return fd >= 0; }
};
#endif

namespace themis {
namespace security {

// ── Helpers ───────────────────────────────────────────────────────────────────

namespace {

// Run CPUID and return {eax, ebx, ecx, edx}.
inline std::array<uint32_t, 4> cpuid(uint32_t leaf, uint32_t subleaf = 0)
{
    std::array<uint32_t, 4> r{};
#if defined(THEMIS_HAS_CPUID)
#  if defined(_MSC_VER)
    int info[4];
    __cpuidex(info, static_cast<int>(leaf), static_cast<int>(subleaf));
    r[0] = static_cast<uint32_t>(info[0]);
    r[1] = static_cast<uint32_t>(info[1]);
    r[2] = static_cast<uint32_t>(info[2]);
    r[3] = static_cast<uint32_t>(info[3]);
#  else
    __cpuid_count(leaf, subleaf, r[0], r[1], r[2], r[3]);
#  endif
#else
#endif
    return r;
}

// Check for Intel TDX via CPUID leaf 0x21 sub-leaf 0.
// The hypervisor exposes the string "IntelTDX    " split across EBX/EDX/ECX.
bool cpuid_detect_tdx()
{
#if defined(THEMIS_HAS_CPUID)
    // First, confirm leaf 0x21 is present (max leaf must be >= 0x21).
    auto max_leaf = cpuid(0);
    if (max_leaf[0] < 0x21) {
      return false;
    }

    auto r = cpuid(0x21, 0);
    // EBX = "Inte", EDX = "lTDX", ECX = "    " (four spaces)
    char sig[13]{};
    std::memcpy(sig + 0, &r[1], 4); // EBX
    std::memcpy(sig + 4, &r[3], 4); // EDX
    std::memcpy(sig + 8, &r[2], 4); // ECX
    sig[12] = '\0';
    return std::string(sig) == "IntelTDX    ";
#else
    return false;
#endif
}

// Check for AMD SEV / SEV-SNP via CPUID leaf 0x8000_001F.
// Returns {sev_active, sev_snp_active}.
std::pair<bool,bool> cpuid_detect_amd_sev()
{
#if defined(THEMIS_HAS_CPUID)
    // Check vendor is AuthenticAMD
    auto vendor = cpuid(0);
    char vend[13]{};
    std::memcpy(vend + 0, &vendor[1], 4); // EBX
    std::memcpy(vend + 4, &vendor[3], 4); // EDX
    std::memcpy(vend + 8, &vendor[2], 4); // ECX
    vend[12] = '\0';
    if (std::string(vend) != "AuthenticAMD") return {false, false};

    // Leaf 0x8000_001F: Memory Encryption Support
    auto r = cpuid(0x8000001F);
    bool sev_supported     = (r[0] >> 1) & 1;  // EAX bit 1
    bool sev_snp_supported = (r[0] >> 4) & 1;  // EAX bit 4

    if (!sev_supported) return {false, false};

    // Read MSR 0xC001_0131 to check whether SEV is *active* in this VM.
    // MSR access is privileged; we catch failure and fall back to CPUID only.
    bool sev_active     = false;
    bool sev_snp_active = false;

#  if defined(THEMIS_IS_LINUX)
    ScopedFd fd(::open("/dev/cpu/0/msr", O_RDONLY | O_CLOEXEC | O_NONBLOCK));
    if (fd.valid()) {
        uint64_t msr_val = 0;
        off_t offset = static_cast<off_t>(0xC0010131ULL);
        if (::pread(fd.fd, &msr_val, sizeof(msr_val), offset) == sizeof(msr_val)) {
            sev_active     = (msr_val & (1 << 0)) != 0; // SEV bit
            sev_snp_active = (msr_val & (1 << 3)) != 0; // SNP bit
        }
    } else {
        // MSR not accessible (no root, container, etc.); trust CPUID alone.
        sev_active     = sev_supported;
        sev_snp_active = sev_snp_supported;
    }
#  else
    sev_active     = sev_supported;
    sev_snp_active = sev_snp_supported;
#  endif

    return {sev_active, sev_snp_active};
#else
    return {false, false};
#endif
}

// SHA-256 convenience wrapper (returns 32-byte digest).
std::vector<uint8_t> sha256(const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> digest(SHA256_DIGEST_LENGTH);
    SHA256(data.data(), data.size(), digest.data());
    return digest;
}

// AES-256-GCM encrypt.  Throws on failure.
void aes256gcm_encrypt(
    const uint8_t key[32],
    const std::vector<uint8_t>& plaintext,
    std::vector<uint8_t>& iv_out,
    std::vector<uint8_t>& ciphertext_out,
    std::vector<uint8_t>& tag_out)
{
    iv_out.resize(12);
    if (RAND_bytes(iv_out.data(), 12) != 1)
        throw std::runtime_error("ConfidentialComputing: RAND_bytes failed for IV");

    CC_EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
      throw std::runtime_error("ConfidentialComputing: EVP_CIPHER_CTX_new failed");
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
        throw std::runtime_error("ConfidentialComputing: EVP_EncryptInit_ex failed");
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1)
        throw std::runtime_error("ConfidentialComputing: EVP_CTRL_GCM_SET_IVLEN failed");
    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key, iv_out.data()) != 1)
        throw std::runtime_error("ConfidentialComputing: EVP_EncryptInit_ex (key/iv) failed");

    ciphertext_out.resize(plaintext.size());
    int len = 0;
    if (!plaintext.empty()) {
        if (EVP_EncryptUpdate(ctx.get(), ciphertext_out.data(), &len,
                              plaintext.data(), static_cast<int>(plaintext.size())) != 1)
            throw std::runtime_error("ConfidentialComputing: EVP_EncryptUpdate failed");
    }
    int final_len = 0;
    if (EVP_EncryptFinal_ex(ctx.get(), ciphertext_out.data() + len, &final_len) != 1)
        throw std::runtime_error("ConfidentialComputing: EVP_EncryptFinal_ex failed");
    ciphertext_out.resize(static_cast<size_t>(len + final_len));

    tag_out.resize(16);
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, 16, tag_out.data()) != 1)
        throw std::runtime_error("ConfidentialComputing: EVP_CTRL_GCM_GET_TAG failed");
}

// AES-256-GCM decrypt.  Throws on authentication failure.
std::vector<uint8_t> aes256gcm_decrypt(
    const uint8_t key[32],
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& ciphertext,
    const std::vector<uint8_t>& tag)
{
    if (iv.size() != 12) {
      throw std::runtime_error("ConfidentialComputing: invalid IV length");
    }
    if (tag.size() != 16) {
      throw std::runtime_error("ConfidentialComputing: invalid tag length");
    }

    CC_EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
      throw std::runtime_error("ConfidentialComputing: EVP_CIPHER_CTX_new failed");
    }

    std::vector<uint8_t> plaintext = {};

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
        throw std::runtime_error("ConfidentialComputing: EVP_DecryptInit_ex failed");
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1)
        throw std::runtime_error("ConfidentialComputing: EVP_CTRL_GCM_SET_IVLEN failed");
    if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, key, iv.data()) != 1)
        throw std::runtime_error("ConfidentialComputing: EVP_DecryptInit_ex (key/iv) failed");

    plaintext.resize(ciphertext.size());
    int len = 0;
    if (!ciphertext.empty()) {
        if (EVP_DecryptUpdate(ctx.get(), plaintext.data(), &len,
                              ciphertext.data(), static_cast<int>(ciphertext.size())) != 1)
            throw std::runtime_error("ConfidentialComputing: EVP_DecryptUpdate failed");
    }
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, 16,
                             const_cast<uint8_t*>(tag.data())) != 1)
            throw std::runtime_error("ConfidentialComputing: EVP_CTRL_GCM_SET_TAG failed");

    int final_len = 0;
    int ret = EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + len, &final_len);
    if (ret <= 0)
        throw std::runtime_error("ConfidentialComputing: authentication tag verification failed"
                                 " — data may have been tampered with");
    plaintext.resize(static_cast<size_t>(len + final_len));
    return plaintext;
}

} // anonymous namespace

// ── Base implementation shared by all concrete classes ────────────────────────

/** @brief ── Base implementation shared by all concrete classes ────────────────────────. */
class ConfidentialComputingBase : public ConfidentialComputing {
public:
    // Each subclass owns a 32-byte sealing key generated at construction.
    // In a production TEE deployment this key should be provisioned through
    // a remote attestation flow and persisted in a KMS; the current
    // in-memory approach guarantees protection within a single process
    // lifetime without external dependencies.
    explicit ConfidentialComputingBase(TeeType tee_type) : tee_type_(tee_type)
    {
        if (RAND_bytes(sealing_key_.data(), static_cast<int>(sealing_key_.size())) != 1)
            throw std::runtime_error("ConfidentialComputing: failed to generate sealing key");
    }

    SealedBlob seal(const std::vector<uint8_t>& plaintext) const override
    {
        SealedBlob blob;
        blob.tee_type = tee_type_;
        blob.measurement_binding = currentMeasurement();

        aes256gcm_encrypt(sealing_key_.data(), plaintext,
                          blob.iv, blob.ciphertext, blob.tag);
        return blob;
    }

    std::vector<uint8_t> unseal(const SealedBlob& blob) const override
    {
        if (blob.tee_type != tee_type_)
            throw std::runtime_error(
                "ConfidentialComputing: blob was sealed by a different TEE type");

        auto current = currentMeasurement();
        if (!blob.measurement_binding.empty() &&
            blob.measurement_binding != current) {
            throw std::runtime_error(
                "ConfidentialComputing: measurement binding mismatch — "
                "TEE measurement has changed since seal()");
        }

        return aes256gcm_decrypt(sealing_key_.data(), blob.iv,
                                 blob.ciphertext, blob.tag);
    }

protected:
    // Subclasses override to return the current TEE measurement digest
    // (used for measurement binding in seal/unseal).
    virtual std::vector<uint8_t> currentMeasurement() const = 0;

    TeeType tee_type_;
    std::array<uint8_t, 32> sealing_key_{};
};

// ── Intel TDX implementation ──────────────────────────────────────────────────

/** @brief ── Intel TDX implementation ──────────────────────────────────────────────────. */
class TdxConfidentialComputing final : public ConfidentialComputingBase {
public:
    TdxConfidentialComputing() : ConfidentialComputingBase(TeeType::INTEL_TDX) {}

    TeeDetectionResult detect() const override
    {
        TeeDetectionResult r;
        r.type        = TeeType::INTEL_TDX;
        r.description = "Intel TDX";

#if defined(THEMIS_IS_LINUX)
        // Attempt kernel driver confirmation
        ScopedFd fd(::open("/dev/tdx_guest", O_RDWR | O_CLOEXEC | O_NONBLOCK));
        if (fd.valid()) {
            r.hardware_attested = true;
            THEMIS_INFO("ConfidentialComputing: Intel TDX confirmed via /dev/tdx_guest");
        } else {
            THEMIS_WARN("ConfidentialComputing: /dev/tdx_guest unavailable ({}); "
                        "TDX detected via CPUID only", strerror(errno));
        }
#else
        THEMIS_INFO("ConfidentialComputing: Intel TDX detected via CPUID (non-Linux)");
#endif
        return r;
    }

    TeeAttestationReport getAttestationReport(
            const std::vector<uint8_t>& report_data) const override
    {
        TeeAttestationReport report;
        report.tee_type    = TeeType::INTEL_TDX;
        report.tee_version = "TDX 1.0";

        // Clamp / pad user data to exactly 64 bytes
        report.report_data.assign(64, 0x00);
        auto copy_len = std::min(report_data.size(), size_t{64});
        std::memcpy(report.report_data.data(), report_data.data(), copy_len);

#if defined(THEMIS_IS_LINUX)
        ScopedFd fd(::open("/dev/tdx_guest", O_RDWR | O_CLOEXEC | O_NONBLOCK));
        if (fd.valid()) {
            struct tdx_report_req req{};
            std::memcpy(req.reportdata, report.report_data.data(), TDX_REPORTDATA_LEN);

            if (::ioctl(fd.fd, TDX_CMD_GET_REPORT0, &req) == 0) {
                report.raw_report.assign(req.tdreport, req.tdreport + TDX_REPORT_LEN);
                report.is_genuine = true;
                THEMIS_INFO("ConfidentialComputing: TDX TDREPORT obtained ({} bytes)",
                            TDX_REPORT_LEN);
            } else {
                THEMIS_WARN("ConfidentialComputing: TDX_CMD_GET_REPORT0 ioctl failed ({}); "
                            "returning software-mode report", strerror(errno));
            }
        } else {
            THEMIS_WARN("ConfidentialComputing: /dev/tdx_guest not accessible ({}); "
                        "returning software-mode report", strerror(errno));
        }
#endif
        return report;
    }

    std::string name() const override { return "Intel TDX"; }

protected:
    std::vector<uint8_t> currentMeasurement() const override
    {
        // Obtain TDREPORT with zeroed report_data and digest the MRTD field
        // (bytes 128-176 of the TDREPORT, the TD measurement register).
        // On non-driver systems return zeros so seal/unseal still work.
        std::vector<uint8_t> zeros(64, 0);
        auto rpt = getAttestationReport(zeros);
        if (!rpt.raw_report.empty() && rpt.raw_report.size() >= 176) {
            // MRTD is at offset 128, length 48 bytes in the TDREPORT structure
            std::vector<uint8_t> mrtd(rpt.raw_report.begin() + 128,
                                      rpt.raw_report.begin() + 176);
            return sha256(mrtd);
        }
        return std::vector<uint8_t>(32, 0);
    }
};

// ── AMD SEV / SEV-SNP implementation ─────────────────────────────────────────

/** @brief ── AMD SEV / SEV-SNP implementation ─────────────────────────────────────────. */
class SevConfidentialComputing final : public ConfidentialComputingBase {
public:
    explicit SevConfidentialComputing(TeeType type)
        : ConfidentialComputingBase(type) {}

    TeeDetectionResult detect() const override
    {
        TeeDetectionResult r;
        r.type = tee_type_;
        r.description = (tee_type_ == TeeType::AMD_SEV_SNP) ? "AMD SEV-SNP" : "AMD SEV";

#if defined(THEMIS_IS_LINUX)
        const char* dev = (tee_type_ == TeeType::AMD_SEV_SNP)
                          ? "/dev/sev-guest" : "/dev/sev";
        ScopedFd fd(::open(dev, O_RDONLY | O_CLOEXEC | O_NONBLOCK));
        if (fd.valid()) {
            r.hardware_attested = true;
            THEMIS_INFO("ConfidentialComputing: {} confirmed via {}", r.description, dev);
        } else {
            THEMIS_WARN("ConfidentialComputing: {} not accessible ({}); "
                        "detected via CPUID only", dev, strerror(errno));
        }
#else
        THEMIS_INFO("ConfidentialComputing: {} detected via CPUID (non-Linux)", r.description);
#endif
        return r;
    }

    TeeAttestationReport getAttestationReport(
            const std::vector<uint8_t>& report_data) const override
    {
        TeeAttestationReport report;
        report.tee_type    = tee_type_;
        report.tee_version = (tee_type_ == TeeType::AMD_SEV_SNP) ? "SEV-SNP 1.51" : "SEV 1.0";

        report.report_data.assign(64, 0x00);
        auto copy_len = std::min(report_data.size(), size_t{64});
        std::memcpy(report.report_data.data(), report_data.data(), copy_len);

#if defined(THEMIS_IS_LINUX)
        if (tee_type_ == TeeType::AMD_SEV_SNP) {
            ScopedFd fd(::open("/dev/sev-guest", O_RDWR | O_CLOEXEC | O_NONBLOCK));
            if (fd.valid()) {
                struct snp_report_req  req{};
                struct snp_report_resp resp{};
                struct snp_guest_request_ioctl guest_req{};

                std::memcpy(req.user_data, report.report_data.data(),
                            SNP_REPORT_DATA_SIZE);
                req.vmpl = 0;

                guest_req.msg_version = 1;
                guest_req.req_data    = reinterpret_cast<uint64_t>(&req);
                guest_req.resp_data   = reinterpret_cast<uint64_t>(&resp);

                if (::ioctl(fd.fd, SNP_GET_REPORT, &guest_req) == 0) {
                    report.raw_report.assign(resp.data, resp.data + SNP_REPORT_SIZE);
                    report.is_genuine = true;
                    THEMIS_INFO("ConfidentialComputing: SEV-SNP report obtained ({} bytes)",
                                SNP_REPORT_SIZE);
                } else {
                    THEMIS_WARN("ConfidentialComputing: SNP_GET_REPORT ioctl failed ({}); "
                                "returning software-mode report", strerror(errno));
                }
            } else {
                THEMIS_WARN("ConfidentialComputing: /dev/sev-guest not accessible ({}); "
                            "returning software-mode report", strerror(errno));
            }
        }
        // For legacy AMD SEV (non-SNP) attestation is done via the platform
        // certificate; we leave raw_report empty (software-mode).
#endif
        return report;
    }

    std::string name() const override
    {
        return (tee_type_ == TeeType::AMD_SEV_SNP) ? "AMD SEV-SNP" : "AMD SEV";
    }

protected:
    std::vector<uint8_t> currentMeasurement() const override
    {
        // SEV-SNP measurement is in the SNP report at byte offset 0x60,
        // length 48 bytes (MEASUREMENT field).
        if (tee_type_ == TeeType::AMD_SEV_SNP) {
            std::vector<uint8_t> zeros(64, 0);
            auto rpt = getAttestationReport(zeros);
            if (!rpt.raw_report.empty() && rpt.raw_report.size() >= 0x60 + 48) {
                std::vector<uint8_t> meas(rpt.raw_report.begin() + 0x60,
                                          rpt.raw_report.begin() + 0x60 + 48);
                return sha256(meas);
            }
        }
        return std::vector<uint8_t>(32, 0);
    }
};

// ── Software fallback (no TEE) ────────────────────────────────────────────────

/** @brief ── Software fallback (no TEE) ────────────────────────────────────────────────. */
class SoftwareConfidentialComputing final : public ConfidentialComputingBase {
public:
    SoftwareConfidentialComputing() : ConfidentialComputingBase(TeeType::NONE) {}

    TeeDetectionResult detect() const override
    {
        return {TeeType::NONE, "Software fallback (no TEE hardware detected)", false};
    }

    TeeAttestationReport getAttestationReport(
            const std::vector<uint8_t>& report_data) const override
    {
        TeeAttestationReport report;
        report.tee_type    = TeeType::NONE;
        report.tee_version = "software";
        report.report_data.assign(64, 0x00);
        auto copy_len = std::min(report_data.size(), size_t{64});
        std::memcpy(report.report_data.data(), report_data.data(), copy_len);
        // raw_report intentionally empty; is_genuine remains false
        return report;
    }

    std::string name() const override { return "Software fallback"; }

protected:
    std::vector<uint8_t> currentMeasurement() const override
    {
        // No TEE measurement available; use zeros so seal/unseal work within
        // the same process without binding to any hardware measurement.
        return std::vector<uint8_t>(32, 0);
    }
};

// ── Factory ───────────────────────────────────────────────────────────────────

std::unique_ptr<ConfidentialComputing> ConfidentialComputing::create()
{
    // 1. Try Intel TDX
    if (cpuid_detect_tdx()) {
        THEMIS_INFO("ConfidentialComputing: Intel TDX detected via CPUID");
        return std::make_unique<TdxConfidentialComputing>();
    }

    // 2. Try AMD SEV / SEV-SNP
    auto [sev, snp] = cpuid_detect_amd_sev();
    if (snp) {
        THEMIS_INFO("ConfidentialComputing: AMD SEV-SNP detected via CPUID");
        return std::make_unique<SevConfidentialComputing>(TeeType::AMD_SEV_SNP);
    }
    if (sev) {
        THEMIS_INFO("ConfidentialComputing: AMD SEV detected via CPUID");
        return std::make_unique<SevConfidentialComputing>(TeeType::AMD_SEV);
    }

    // 3. Software fallback
    THEMIS_INFO("ConfidentialComputing: no TEE hardware detected; using software fallback");
    return std::make_unique<SoftwareConfidentialComputing>();
}

} // namespace security
} // namespace themis


