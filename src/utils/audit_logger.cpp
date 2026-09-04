/**
 * @file audit_logger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/audit_logger.h"
#include <iostream>
#include <stdexcept>
#include "utils/error_contracts.h"
#include "utils/logger.h"
#include "utils/error_contracts.h"
#include "utils/error_registry.h"
#include <fmt/format.h>

#include <filesystem>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <curl/curl.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#ifndef _WIN32
namespace {

/// @brief RAII wrapper for a POSIX file descriptor.
/// Automatically closes the fd on scope exit. The fd must be >= 0.
struct FdGuard {
    explicit FdGuard([[maybe_unused]] int fd) noexcept : fd_(fd) {}
    ~FdGuard() noexcept { if (fd_ >= 0) ::close(fd_); }
    FdGuard(const FdGuard&) = delete;
    FdGuard& operator=(const FdGuard&) = delete;
    int fd_;
};

} // anonymous namespace
#endif

namespace themis {
namespace utils {

// Version constant for CEF format.  Injected by CMake as THEMIS_VERSION_STRING
// (-DTHEMIS_VERSION_STRING=...) so it always matches the build; falls back to
// a generic sentinel string when building outside CMake (e.g., IDE or CI stub).
#ifndef THEMIS_VERSION_STRING
#define THEMIS_VERSION_STRING "0.0.0-dev"
#endif
static const char* THEMISDB_VERSION = THEMIS_VERSION_STRING;

// Local base64 (kept minimal to avoid new deps here)
static std::string base64_encode_local(const std::vector<uint8_t>& data) {
    static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= data.size()) {
        uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | (data[i + 2]);
        out.push_back(b64_table[(n >> 18) & 63]);
        out.push_back(b64_table[(n >> 12) & 63]);
        out.push_back(b64_table[(n >> 6) & 63]);
        out.push_back(b64_table[n & 63]);
        i += 3;
    }
    if (i + 1 == data.size()) {
        uint32_t n = (data[i] << 16);
        out.push_back(b64_table[(n >> 18) & 63]);
        out.push_back(b64_table[(n >> 12) & 63]);
        out.push_back('=');
        out.push_back('=');
    } else if (i + 2 == data.size()) {
        uint32_t n = (data[i] << 16) | (data[i + 1] << 8);
        out.push_back(b64_table[(n >> 18) & 63]);
        out.push_back(b64_table[(n >> 12) & 63]);
        out.push_back(b64_table[(n >> 6) & 63]);
        out.push_back('=');
    }
    return out;
}

static std::string base64_decode_local(const std::string& s) {
    static constexpr signed char kDecTable[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };

    std::string out;
    out.reserve((s.size() * 3) / 4);
    int val = 0;
    int valb = -8;
    for (unsigned char c : s) {
        if (c == '=') {
            break;
        }
        int d = kDecTable[c];
        if (d < 0) {
            continue;
        }
        val = (val << 6) + d;
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

AuditLogger::AuditLogger(std::shared_ptr<themis::FieldEncryption> enc,
                         std::shared_ptr<VCCPKIClient> pki,
                         AuditLoggerConfig cfg)
    : enc_(std::move(enc)), pki_(std::move(pki)), cfg_(std::move(cfg)) {
    
    // Load hash chain state if tamper-proofing is enabled
    if (cfg_.enable_hash_chain) {
        loadChainState();
        
        // Verify chain integrity on startup
        if (entry_count_ > 0) {
            THEMIS_INFO("Verifying audit log chain integrity...");
            if (!verifyChainIntegrity()) {
                THEMIS_ERROR("Audit log chain integrity check FAILED - possible tampering detected!");
            } else {
                THEMIS_INFO("Audit log chain integrity verified OK ({} entries)", entry_count_);
            }
        }
    }
}

std::vector<uint8_t> AuditLogger::sha256(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out(SHA256_DIGEST_LENGTH);
    ::SHA256(data.data(), data.size(), out.data());
    return out;
}

void AuditLogger::appendJsonLine(const nlohmann::json& j) {
    std::scoped_lock lk(file_mu_);
    std::filesystem::create_directories(
        std::filesystem::path(cfg_.log_path).parent_path());

    // Bounded queue check: enforce max_queued_events before the file write so
    // callers get an explicit error rather than an unbounded disk fill.
    if (cfg_.max_queued_events > 0 && entry_count_ >= cfg_.max_queued_events) {
        auto ctx = themis::utils::makeErrorContext(
            themis::utils::ErrorCode::AUDIT_BUFFER_OVERFLOW,
            "Audit event queue at capacity (" + std::to_string(cfg_.max_queued_events) +
            ") – event rejected (fail-closed); log_path=" + cfg_.log_path,
            "AuditLogger::appendJsonLine",
            themis::utils::ErrorSeverity::Critical,
            false);
        themis::utils::logErrorWithContext(ctx);
        throw std::runtime_error(
            "AuditLogger: event queue at capacity (" +
            std::to_string(cfg_.max_queued_events) +
            "); fail-closed — event rejected");
    }

    // Rotate the primary log file if it has grown beyond the configured limit
    rotateLogIfNeeded();

    const std::string line = j.dump() + "\n";

    // Write primary log file (fail-closed: surface write failures to caller)
    {
        std::ofstream ofs(cfg_.log_path, std::ios::app | std::ios::binary);
        if (!ofs) {
            auto ctx = themis::utils::makeErrorContext(
                themis::utils::ErrorCode::AUDIT_PERSISTENCE_FAILED,
                "Failed to open audit log file for append – backend unavailable; path=" + cfg_.log_path,
                "AuditLogger::appendJsonLine",
                themis::utils::ErrorSeverity::Critical,
                false);
            themis::utils::logErrorWithContext(ctx);
            throw std::runtime_error(
                "AuditLogger: cannot open log file '" + cfg_.log_path +
                "' – audit backend unavailable");
        }
        ofs << line;
        if (!ofs) {
            auto ctx = themis::utils::makeErrorContext(
                themis::utils::ErrorCode::AUDIT_WRITE_FAILED,
                "Write to audit log file failed; path=" + cfg_.log_path,
                "AuditLogger::appendJsonLine",
                themis::utils::ErrorSeverity::Critical,
                false);
            themis::utils::logErrorWithContext(ctx);
            throw std::runtime_error(
                "AuditLogger: write failure on log file '" + cfg_.log_path + "'");
        }
    }

    // Flush kernel page-cache to stable storage for crash durability
    if (cfg_.enable_fsync) {
#ifndef _WIN32
        int fd = ::open(cfg_.log_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);
        if (fd >= 0) {
            FdGuard guard(fd);
            ::fdatasync(guard.fd_);
        }
#else
        HANDLE h = CreateFileA(cfg_.log_path.c_str(), GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(h);
            CloseHandle(h);
        }
#endif
    }

    // Mirror to secondary path (redundancy against primary storage failure)
    if (!cfg_.secondary_log_path.empty()) {
        std::filesystem::create_directories(
            std::filesystem::path(cfg_.secondary_log_path).parent_path());
        {
            std::ofstream sec(cfg_.secondary_log_path, std::ios::app | std::ios::binary);
            sec << line;
        }
        if (cfg_.enable_fsync) {
#ifndef _WIN32
            int fd2 = ::open(cfg_.secondary_log_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);
            if (fd2 >= 0) {
                FdGuard guard2(fd2);
                ::fdatasync(guard2.fd_);
            }
#else
            HANDLE h2 = CreateFileA(cfg_.secondary_log_path.c_str(), GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    nullptr, OPEN_EXISTING, 0, nullptr);
            if (h2 != INVALID_HANDLE_VALUE) {
                FlushFileBuffers(h2);
                CloseHandle(h2);
            }
#endif
        }
    }
}

void AuditLogger::rotateLogIfNeeded() {
    // Caller must hold file_mu_.
    if (cfg_.max_file_size_bytes == 0) {
      return;
    }

    std::error_code ec;
    if (!std::filesystem::exists(cfg_.log_path, ec)) {
      return;
    }

    const auto file_size = std::filesystem::file_size(cfg_.log_path, ec);
    if (ec || file_size < cfg_.max_file_size_bytes) {
      return;
    }

    const auto& base = cfg_.log_path;

    if (cfg_.max_rotated_files > 0) {
        // Remove the oldest rotated file to make room
        auto oldest = base + "." + std::to_string(cfg_.max_rotated_files);
        if (std::filesystem::exists(oldest, ec)) {
            std::filesystem::remove(oldest, ec);
        }
        // Shift existing rotated files upward: file.N-1 -> file.N
        // Loop starts at max_rotated_files (>= 1) and descends to 2; size_t is safe
        // because we never reach 0 (the condition `i > 1` stops at i==1 before decrement).
        for (size_t i = cfg_.max_rotated_files; i > 1; --i) {
            auto from_path = base + "." + std::to_string(i - 1);
            auto to_path   = base + "." + std::to_string(i);
            if (std::filesystem::exists(from_path, ec)) {
                std::filesystem::rename(from_path, to_path, ec);
            }
        }
        // Rotate the current log to .1
        std::filesystem::rename(base, base + ".1", ec);
    } else {
        // No rotated files kept: discard the full log
        std::filesystem::remove(base, ec);
    }

    THEMIS_INFO("Audit log rotated: {}", base);
}

void AuditLogger::logEvent(const nlohmann::json& event) {
    if (!cfg_.enabled) {
      return;
    }

    try {
        // ─────────────────────────────────────────────────────────────────────
        // Bounded Resource Check: Event size validation (Phase 2.9)
        // ─────────────────────────────────────────────────────────────────────
        std::string plain = event.dump();
        constexpr size_t kMaxEventSize = 10 * 1024 * 1024; // 10MB limit
        
        if (plain.size() > kMaxEventSize) {
            ErrorContext err_ctx(
                themis::utils::ErrorCode::AUDIT_BUFFER_OVERFLOW,
                "Event size exceeds maximum limit",
                "AuditLogger::logEvent"
            );
            err_ctx.resource_limit = kMaxEventSize;
            err_ctx.resource_current = plain.size();
            err_ctx.severity = ErrorSeverity::Warning;
            err_ctx.is_recoverable = true;
            err_ctx.recovery_hint = "Event will be truncated to fit maximum size";
            logErrorContext(err_ctx);
            
            // Truncate event to fit within bounds (Phase 2.8: Graceful Degradation)
            plain = plain.substr(0, kMaxEventSize - 1000); // Leave room for metadata
        }

        nlohmann::json record;
        record["ts"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()).count();
        record["category"] = "AUDIT";
        
        // Add hash chain if enabled
        if (cfg_.enable_hash_chain) {
            std::lock_guard<std::mutex> lock(chain_mu_);
            record["chain_entry"] = entry_count_;
            record["prev_hash"] = last_hash_;
        }

        if (cfg_.encrypt_then_sign && enc_) {
            // Encrypt plaintext JSON with configured key
            try {
                auto blob = enc_->encrypt(plain, cfg_.key_id);

                // Build bytes for hashing: iv || ciphertext || tag
                std::vector<uint8_t> to_hash = {};

                to_hash.reserve(blob.iv.size() + blob.ciphertext.size() + blob.tag.size());
                to_hash.insert(to_hash.end(), blob.iv.begin(), blob.iv.end());
                to_hash.insert(to_hash.end(), blob.ciphertext.begin(), blob.ciphertext.end());
                to_hash.insert(to_hash.end(), blob.tag.begin(), blob.tag.end());

                auto hash = sha256(to_hash);
                SignatureResult sig;
                if (pki_) {
                    try { 
                        sig = pki_->signHash(hash); 
                    }
                    catch (const std::exception& e) {
                        sig.ok = false;
                        
                        // Log PKI failure with error context (Phase 2.3)
                        ErrorContext pki_err(
                            themis::utils::ErrorCode::AUDIT_SIGNATURE_FAILED,
                            fmt::format("PKI signing failed: {}", e.what()),
                            "AuditLogger::logEvent[encryption]"
                        );
                        pki_err.severity = ErrorSeverity::Warning;
                        pki_err.is_recoverable = true;
                        pki_err.recovery_hint = "Continuing without signature verification";
                        logErrorContext(pki_err);
                    }
                    catch (const std::string &) { 
                        sig.ok = false;
                    }
                    catch (const char *) { 
                        sig.ok = false;
                    }
                }

                auto jblob = themis::EncryptedBlob{blob}.toJson();
                // Persist encrypted payload and signature metadata
                record["payload"] = {
                    {"type", "ciphertext"},
                    {"key_id", blob.key_id},
                    {"key_version", blob.key_version},
                    {"iv_b64", jblob["iv"]},
                    {"ciphertext_b64", jblob["ciphertext"]},
                    {"tag_b64", jblob["tag"]}
                };
                record["signature"] = {
                    {"ok", sig.ok},
                    {"id", sig.signature_id},
                    {"algorithm", sig.algorithm},
                    {"sig_b64", sig.signature_b64},
                    {"cert_serial", sig.cert_serial}
                };
            }
            catch (const std::exception&) {
                // Encryption failed - degrade to unencrypted mode (Phase 2.8)
                ErrorContext enc_err(
                    themis::utils::ErrorCode::AUDIT_ENCRYPTION_FAILED,
                    "Encryption failed while preparing encrypted audit payload",
                    "AuditLogger::logEvent[encryption]"
                );
                enc_err.severity = ErrorSeverity::Warning;
                enc_err.is_recoverable = true;
                enc_err.recovery_hint = "Switching to unencrypted audit logging";
                logErrorContext(enc_err);
                
                // Fallback: write plaintext
                std::vector<uint8_t> bytes(plain.begin(), plain.end());
                auto hash = sha256(bytes);
                record["payload"] = {
                    {"type", "plaintext"},
                    {"data_b64", base64_encode_local(bytes)}
                };
                record["signature"] = {{"ok", false}};
            }
        } else {
            // No encryption: sign plaintext bytes (if PKI available)
            try {
                std::vector<uint8_t> bytes(plain.begin(), plain.end());
                auto hash = sha256(bytes);
                SignatureResult sig;
                if (pki_) {
                    try { 
                        sig = pki_->signHash(hash); 
                    }
                    catch (const std::exception&) { 
                        sig.ok = false;
                        
                        ErrorContext pki_err(
                            themis::utils::ErrorCode::AUDIT_SIGNATURE_FAILED,
                            "PKI signing failed while preparing plaintext audit payload",
                            "AuditLogger::logEvent[plaintext]"
                        );
                        pki_err.severity = ErrorSeverity::Warning;
                        logErrorContext(pki_err);
                    }
                    catch (...) { 
                        sig.ok = false; 
                    }
                }
                record["payload"] = {
                    {"type", "plaintext"},
                    {"data_b64", base64_encode_local(bytes)}
                };
                record["signature"] = {
                    {"ok", sig.ok},
                    {"id", sig.signature_id},
                    {"algorithm", sig.algorithm},
                    {"sig_b64", sig.signature_b64},
                    {"cert_serial", sig.cert_serial}
                };
            }
            catch (const std::exception&) {
                ErrorContext err(
                    themis::utils::ErrorCode::AUDIT_VALIDATION_FAILED,
                    "Plaintext serialization failed while building audit payload",
                    "AuditLogger::logEvent[plaintext_serialize]"
                );
                err.severity = ErrorSeverity::Error;
                logErrorContext(err);
                return; // Unable to proceed
            }
        }
        
        // Update hash chain
        if (cfg_.enable_hash_chain) {
            try {
                std::lock_guard<std::mutex> lock(chain_mu_);
                last_hash_ = computeEntryHash(record);
                entry_count_++;
                last_timestamp_ = std::chrono::system_clock::now();
                saveChainState();
            }
            catch (const std::exception&) {
                ErrorContext chain_err(
                    themis::utils::ErrorCode::AUDIT_WRITE_FAILED,
                    "Chain state update failed while appending audit event",
                    "AuditLogger::logEvent[chain_update]"
                );
                chain_err.severity = ErrorSeverity::Warning;
                chain_err.is_recoverable = true;
                logErrorContext(chain_err);
            }
        }

        appendJsonLine(record);
        
        // Forward to SIEM if enabled
        if (cfg_.enable_siem) {
            forwardToSiem(event);
        }
    }
    catch (const std::exception&) {
        // Final fallback: log to stderr
        ErrorContext fatal_err(
            themis::utils::ErrorCode::AUDIT_PERSISTENCE_FAILED,
            "Unexpected error in logEvent while persisting audit data",
            "AuditLogger::logEvent[fatal]"
        );
        fatal_err.severity = ErrorSeverity::Fatal;
        fatal_err.is_recoverable = false;
        std::cerr << fatal_err.toJSON() << std::endl;
    }
}

nlohmann::json AuditLogger::getEvents(int64_t, int64_t, const std::string&) const {
    return nlohmann::json::array();
}

size_t AuditLogger::getTotalEventCount() const {
    return 0;
}

void AuditLogger::clear() {
    // Default no-op; concrete audit loggers may override.
}

// ============================================================================
// Security Event Logging
// ============================================================================

std::string AuditLogger::securityEventTypeToString(SecurityEventType type) {
    switch (type) {
        case SecurityEventType::LOGIN_SUCCESS: return "LOGIN_SUCCESS";
        case SecurityEventType::LOGIN_FAILED: return "LOGIN_FAILED";
        case SecurityEventType::LOGOUT: return "LOGOUT";
        case SecurityEventType::TOKEN_CREATED: return "TOKEN_CREATED";
        case SecurityEventType::TOKEN_REVOKED: return "TOKEN_REVOKED";
        case SecurityEventType::UNAUTHORIZED_ACCESS: return "UNAUTHORIZED_ACCESS";
        case SecurityEventType::PERMISSION_DENIED: return "PERMISSION_DENIED";
        // MFA Events (Phase 3)
        case SecurityEventType::MFA_ENROLLED: return "MFA_ENROLLED";
        case SecurityEventType::MFA_ENABLED: return "MFA_ENABLED";
        case SecurityEventType::MFA_DISABLED: return "MFA_DISABLED";
        case SecurityEventType::MFA_TOTP_SUCCESS: return "MFA_TOTP_SUCCESS";
        case SecurityEventType::MFA_TOTP_FAILED: return "MFA_TOTP_FAILED";
        case SecurityEventType::MFA_RECOVERY_CODE_USED: return "MFA_RECOVERY_CODE_USED";
        case SecurityEventType::MFA_RECOVERY_CODES_REGENERATED: return "MFA_RECOVERY_CODES_REGENERATED";
        case SecurityEventType::MFA_BACKUP_CODES_VIEWED: return "MFA_BACKUP_CODES_VIEWED";
        // Privilege Escalation
        case SecurityEventType::PRIVILEGE_ESCALATION_ATTEMPT: return "PRIVILEGE_ESCALATION_ATTEMPT";
        case SecurityEventType::ROLE_CHANGED: return "ROLE_CHANGED";
        case SecurityEventType::SCOPE_GRANTED: return "SCOPE_GRANTED";
        case SecurityEventType::SCOPE_REVOKED: return "SCOPE_REVOKED";
        // Key Management
        case SecurityEventType::KEY_CREATED: return "KEY_CREATED";
        case SecurityEventType::KEY_ROTATED: return "KEY_ROTATED";
        case SecurityEventType::KEY_DELETED: return "KEY_DELETED";
        case SecurityEventType::KEY_ACCESS: return "KEY_ACCESS";
        // Data Access
        case SecurityEventType::DATA_READ: return "DATA_READ";
        case SecurityEventType::DATA_WRITE: return "DATA_WRITE";
        case SecurityEventType::DATA_DELETE: return "DATA_DELETE";
        case SecurityEventType::BULK_EXPORT: return "BULK_EXPORT";
        case SecurityEventType::EXPORT_DENIED: return "EXPORT_DENIED";
        case SecurityEventType::BULK_IMPORT: return "BULK_IMPORT";
        case SecurityEventType::BULK_IMPORT_COMPLETED: return "BULK_IMPORT_COMPLETED";
        // Graph & Vector Operations (Phase 1)
        case SecurityEventType::GRAPH_TRAVERSAL: return "GRAPH_TRAVERSAL";
        case SecurityEventType::BULK_NODE_ACCESS: return "BULK_NODE_ACCESS";
        case SecurityEventType::BULK_EDGE_ACCESS: return "BULK_EDGE_ACCESS";
        case SecurityEventType::EMBEDDING_QUERY: return "EMBEDDING_QUERY";
        case SecurityEventType::EMBEDDING_EXPORT: return "EMBEDDING_EXPORT";
        case SecurityEventType::GRAPH_EXPORT: return "GRAPH_EXPORT";
        case SecurityEventType::TEMPORAL_QUERY: return "TEMPORAL_QUERY";
        // GPU/VRAM Security (Phase 2)
        case SecurityEventType::VRAM_ALLOCATED: return "VRAM_ALLOCATED";
        case SecurityEventType::VRAM_DEALLOCATED: return "VRAM_DEALLOCATED";
        case SecurityEventType::VRAM_SECURE_CLEAR: return "VRAM_SECURE_CLEAR";
        case SecurityEventType::GPU_MEMORY_EXHAUSTION: return "GPU_MEMORY_EXHAUSTION";
        // PII
        case SecurityEventType::PII_ACCESSED: return "PII_ACCESSED";
        case SecurityEventType::PII_REVEALED: return "PII_REVEALED";
        case SecurityEventType::PII_ERASED: return "PII_ERASED";
        // Configuration
        case SecurityEventType::CONFIG_CHANGED: return "CONFIG_CHANGED";
        case SecurityEventType::POLICY_UPDATED: return "POLICY_UPDATED";
        case SecurityEventType::ENCRYPTION_SCHEMA_CHANGED: return "ENCRYPTION_SCHEMA_CHANGED";
        // Security Incidents
        case SecurityEventType::BRUTE_FORCE_DETECTED: return "BRUTE_FORCE_DETECTED";
        case SecurityEventType::RATE_LIMIT_EXCEEDED: return "RATE_LIMIT_EXCEEDED";
        case SecurityEventType::SUSPICIOUS_ACTIVITY: return "SUSPICIOUS_ACTIVITY";
        case SecurityEventType::INTEGRITY_VIOLATION: return "INTEGRITY_VIOLATION";
        // Binary Integrity (Phase 5)
        case SecurityEventType::BINARY_SIGNATURE_VERIFIED: return "BINARY_SIGNATURE_VERIFIED";
        case SecurityEventType::BINARY_SIGNATURE_FAILED: return "BINARY_SIGNATURE_FAILED";
        case SecurityEventType::MANIFEST_UPDATED: return "MANIFEST_UPDATED";
        // System Events
        case SecurityEventType::SERVER_STARTED: return "SERVER_STARTED";
        case SecurityEventType::SERVER_STOPPED: return "SERVER_STOPPED";
        case SecurityEventType::BACKUP_CREATED: return "BACKUP_CREATED";
        case SecurityEventType::RESTORE_COMPLETED: return "RESTORE_COMPLETED";
        // Task Scheduler Events
        case SecurityEventType::TASK_REGISTERED: return "TASK_REGISTERED";
        case SecurityEventType::TASK_UNREGISTERED: return "TASK_UNREGISTERED";
        case SecurityEventType::TASK_ENABLED: return "TASK_ENABLED";
        case SecurityEventType::TASK_DISABLED: return "TASK_DISABLED";
        case SecurityEventType::TASK_UPDATED: return "TASK_UPDATED";
        case SecurityEventType::TASK_EXECUTED_SUCCESS: return "TASK_EXECUTED_SUCCESS";
        case SecurityEventType::TASK_EXECUTED_FAILURE: return "TASK_EXECUTED_FAILURE";
        case SecurityEventType::TASK_CRON_TRIGGERED: return "TASK_CRON_TRIGGERED";
        case SecurityEventType::TASK_CDC_TRIGGERED: return "TASK_CDC_TRIGGERED";
        case SecurityEventType::TASK_MANUAL_TRIGGERED: return "TASK_MANUAL_TRIGGERED";
        case SecurityEventType::TASK_TIMEOUT: return "TASK_TIMEOUT";
        case SecurityEventType::TASK_RESOURCE_LIMIT_EXCEEDED: return "TASK_RESOURCE_LIMIT_EXCEEDED";
        case SecurityEventType::TASK_ANOMALY_DETECTED: return "TASK_ANOMALY_DETECTED";
        // Sharding Events
        case SecurityEventType::SHARD_SPLIT: return "SHARD_SPLIT";
        case SecurityEventType::SHARD_MERGE: return "SHARD_MERGE";
        case SecurityEventType::SHARD_LIVE_MIGRATION_STARTED: return "SHARD_LIVE_MIGRATION_STARTED";
        case SecurityEventType::SHARD_LIVE_MIGRATION_COMPLETED: return "SHARD_LIVE_MIGRATION_COMPLETED";
        case SecurityEventType::SHARD_LIVE_MIGRATION_FAILED: return "SHARD_LIVE_MIGRATION_FAILED";
        // Generic
        case SecurityEventType::CUSTOM_EVENT: return "CUSTOM_EVENT";
        // Auth Wave4B additions (A4, A5, A6)
        case SecurityEventType::PERMISSION_CHANGED: return "PERMISSION_CHANGED";
        case SecurityEventType::KEY_ROTATION_FAILED: return "KEY_ROTATION_FAILED";
        case SecurityEventType::KEY_REVOCATION_FAILED: return "KEY_REVOCATION_FAILED";
        default: return "UNKNOWN";
    }
}

void AuditLogger::logSecurityEvent(
    SecurityEventType event_type,
    const std::string& user_id,
    const std::string& resource,
    const nlohmann::json& details
) {
    nlohmann::json event = {
        {"event_type", securityEventTypeToString(event_type)},
        {"user_id", user_id},
        {"resource", resource},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()}
    };
    
    if (!details.is_null() && !details.empty()) {
        event["details"] = details;
    }
    
    // Log severity based on event type
    if (event_type == SecurityEventType::LOGIN_FAILED ||
        event_type == SecurityEventType::UNAUTHORIZED_ACCESS ||
        event_type == SecurityEventType::PRIVILEGE_ESCALATION_ATTEMPT ||
        event_type == SecurityEventType::BRUTE_FORCE_DETECTED ||
        event_type == SecurityEventType::INTEGRITY_VIOLATION) {
        event["severity"] = "HIGH";
        THEMIS_WARN("Security event: {} - User: {} - Resource: {}", 
            securityEventTypeToString(event_type), user_id, resource);
    } else if (event_type == SecurityEventType::RATE_LIMIT_EXCEEDED ||
               event_type == SecurityEventType::PERMISSION_DENIED ||
               event_type == SecurityEventType::EXPORT_DENIED ||
               event_type == SecurityEventType::SUSPICIOUS_ACTIVITY) {
        event["severity"] = "MEDIUM";
    } else {
        event["severity"] = "LOW";
    }
    
    logEvent(event);
}

// ============================================================================
// Hash Chain Methods (Tamper-Proofing)
// ============================================================================

void AuditLogger::loadChainState() {
    std::lock_guard<std::mutex> lock(chain_mu_);
    
    if (!std::filesystem::exists(cfg_.chain_state_file)) {
        // Initialize empty chain
        last_hash_ = std::string(64, '0'); // Genesis hash
        entry_count_ = 0;
        last_timestamp_ = std::chrono::system_clock::now();
        return;
    }
    
    try {
        std::ifstream ifs(cfg_.chain_state_file);
        nlohmann::json state;
        ifs >> state;
        
        last_hash_ = state.value("last_hash", std::string(64, '0'));
        entry_count_ = state.value("entry_count", 0ull);
        
        if (state.contains("last_timestamp_ms")) {
            auto ms = state["last_timestamp_ms"].get<uint64_t>();
            last_timestamp_ = std::chrono::system_clock::time_point(
                std::chrono::milliseconds(ms));
        }
        
        THEMIS_INFO("Loaded audit chain state: {} entries, last_hash={}...", 
            entry_count_, last_hash_.substr(0, 16));
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load chain state: {}", e.what());
        // Reset to genesis
        last_hash_ = std::string(64, '0');
        entry_count_ = 0;
    }
}

void AuditLogger::saveChainState() {
    // Assumes chain_mu_ is already locked by caller
    nlohmann::json state = {
        {"last_hash", last_hash_},
        {"entry_count", entry_count_},
        {"last_timestamp_ms", std::chrono::duration_cast<std::chrono::milliseconds>(
            last_timestamp_.time_since_epoch()).count()}
    };
    
    try {
        auto path = std::filesystem::path(cfg_.chain_state_file);
        std::filesystem::create_directories(path.parent_path());
        
        std::ofstream ofs(cfg_.chain_state_file);
        ofs << state.dump(2);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to save chain state: {}", e.what());
    }
}

std::string AuditLogger::computeEntryHash(const nlohmann::json& entry) const {
    // Hash the JSON entry concatenated with previous hash
    std::string to_hash = last_hash_ + entry.dump();
    std::vector<uint8_t> bytes(to_hash.begin(), to_hash.end());
    auto hash_bytes = sha256(bytes);
    
    // Convert to hex string
    std::ostringstream oss;
    for (auto b : hash_bytes) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

bool AuditLogger::verifyChainIntegrity() {
    if (!cfg_.enable_hash_chain) {
        return true; // Chain disabled, nothing to verify
    }
    
    if (!std::filesystem::exists(cfg_.log_path)) {
        return true; // No log file yet, chain is valid
    }
    
    try {
        std::ifstream ifs(cfg_.log_path);
        std::string line;
        std::string expected_prev_hash = std::string(64, '0'); // Genesis
        uint64_t line_num = 0;
        
        while (std::getline(ifs, line)) {
            if (line.empty()) {
              continue;
            }
            
            line_num++;
            nlohmann::json entry = nlohmann::json::parse(line);
            
            if (!entry.contains("chain_entry") || !entry.contains("prev_hash")) {
                THEMIS_WARN("Entry {} missing chain fields", line_num);
                continue; // Skip entries without chain data (backward compat)
            }
            
            std::string prev_hash = entry["prev_hash"];
            if (prev_hash != expected_prev_hash) {
                THEMIS_ERROR("Chain integrity violation at entry {}: expected prev_hash={}, got={}",
                    line_num, expected_prev_hash.substr(0, 16), prev_hash.substr(0, 16));
                return false;
            }
            
            // Compute hash for this entry
            std::string to_hash = prev_hash + entry.dump();
            std::vector<uint8_t> bytes(to_hash.begin(), to_hash.end());
            auto hash_bytes = sha256(bytes);
            
            std::ostringstream oss;
            for (auto b : hash_bytes) {
                oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
            }
            expected_prev_hash = oss.str();
        }
        
        THEMIS_INFO("Audit log chain verified: {} entries, last_hash={}...", 
            line_num, expected_prev_hash.substr(0, 16));
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Chain verification failed: {}", e.what());
        return false;
    }
}

nlohmann::json AuditLogger::getChainState() const {
    std::lock_guard<std::mutex> lock(chain_mu_);
    return {
        {"last_hash", last_hash_},
        {"entry_count", entry_count_},
        {"last_timestamp_ms", std::chrono::duration_cast<std::chrono::milliseconds>(
            last_timestamp_.time_since_epoch()).count()},
        {"chain_enabled", cfg_.enable_hash_chain}
    };
}

// ============================================================================
// SIEM Integration
// ============================================================================

void AuditLogger::forwardToSiem(const nlohmann::json& event) {
    if (!cfg_.enable_siem) {
        return;
    }
    
    // Determine event type if available
    SecurityEventType event_type = SecurityEventType::CUSTOM_EVENT;
    if (event.contains("event_type")) {
        std::string event_type_str = event["event_type"].get<std::string>();
        // Map string back to enum for formatting (simplified approach)
        // In production, might want a proper reverse mapping
    }
    
    // Format the message based on configured format
    std::string formatted_message;
    if (cfg_.siem_format == "cef") {
        formatted_message = formatAsCef(event, event_type);
    } else if (cfg_.siem_format == "syslog") {
        formatted_message = formatAsSyslog(event, event_type);
    } else { // json (default)
        formatted_message = formatAsJson(event);
    }
    
    if (cfg_.siem_type == "syslog") {
        // RFC 5424 Syslog format (POSIX only)
#ifndef _WIN32
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            THEMIS_ERROR("Failed to create syslog socket");
            return;
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(cfg_.siem_port);
        inet_pton(AF_INET, cfg_.siem_host.c_str(), &addr.sin_addr);

        sendto(sock, formatted_message.c_str(), formatted_message.size(), 0,
               (struct sockaddr*)&addr, sizeof(addr));

        close(sock);
#else
        THEMIS_WARN("Syslog SIEM forwarding is not supported on Windows");
#endif
        
    } else if (cfg_.siem_type == "splunk") {
        // Splunk HEC (HTTP Event Collector) via libcurl
        {
            // Wrap the formatted event in a Splunk HEC envelope
            nlohmann::json hec_payload;
            hec_payload["event"] = formatted_message;
            hec_payload["sourcetype"] = "_json";
            hec_payload["source"]     = "themisdb";
            std::string body = hec_payload.dump();

            std::string url = "https://" + cfg_.siem_host + ":" +
                              std::to_string(cfg_.siem_port) + "/services/collector/event";

            CURL* curl = curl_easy_init();
            if (!curl) {
                THEMIS_ERROR("Splunk SIEM: failed to init curl");
                return;
            }

            struct curl_slist* headers = nullptr;
            std::string auth_header = "Authorization: Splunk " + cfg_.splunk_token;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, auth_header.c_str());

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            // Use the configured CA bundle when provided; otherwise libcurl falls
            // back to the system default bundle.  Warn if neither is available.
            if (!cfg_.siem_ca_bundle_path.empty()) {
                curl_easy_setopt(curl, CURLOPT_CAINFO, cfg_.siem_ca_bundle_path.c_str());
            } else {
                THEMIS_WARN("Splunk SIEM: siem_ca_bundle_path not set – relying on system CA bundle");
            }
            // Discard response body
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                             +[](char*, size_t s, size_t n, void*) -> size_t { return s * n; });

            CURLcode rc = curl_easy_perform(curl);
            if (rc != CURLE_OK) {
                THEMIS_WARN("Splunk SIEM forward failed: {}", curl_easy_strerror(rc));
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
        
    } else if (cfg_.siem_type == "elastic") {
        // Elasticsearch _doc API via libcurl
        {
            std::string url = "http://" + cfg_.siem_host + ":" +
                              std::to_string(cfg_.siem_port) + "/" +
                              cfg_.elastic_index + "/_doc";

            CURL* curl = curl_easy_init();
            if (!curl) {
                THEMIS_ERROR("Elasticsearch SIEM: failed to init curl");
                return;
            }

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, formatted_message.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                             static_cast<long>(formatted_message.size()));
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                             +[](char*, size_t s, size_t n, void*) -> size_t { return s * n; });

            CURLcode rc = curl_easy_perform(curl);
            if (rc != CURLE_OK) {
                THEMIS_WARN("Elasticsearch SIEM forward failed: {}", curl_easy_strerror(rc));
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
    }
}

void AuditLogger::flush() {
    std::lock_guard<std::mutex> lock(file_mu_);
    // flush() is already called within appendJsonLine when file is opened
    // This method is kept for API compatibility
}

// ============================================================================
// Phase 2.3: Observability Plane Hardening - Error Context Logging
// ============================================================================

void AuditLogger::logErrorContext(const ErrorContext& ctx) {
    // Create an audit event representing the error context
    nlohmann::json error_event = nlohmann::json::parse(ctx.toJSON());
    error_event["event_type"] = "DIAGNOSTIC_ERROR";
    error_event["severity_level"] = static_cast<int>(ctx.severity);
    error_event["is_recoverable"] = ctx.is_recoverable;
    
    // Log diagnostically - use simplified path to avoid recursion
    try {
        std::lock_guard<std::mutex> lock(file_mu_);
        // Append directly without full encryption/chain processing
        // to prevent recursive failures during error logging
        appendJsonLine(error_event);
    }
    catch (const std::exception &e) {
        // Final fallback: write to stderr (Phase 2.8: Graceful Degradation)
        std::cerr << "AUDIT_ERROR_CONTEXT_FAILED: " << e.what() 
                  << " | " << ctx.toJSON() << std::endl;
    }
}


std::vector<AuditLogger::AuditLogEntry> AuditLogger::enumerateEntries() const {
    std::vector<AuditLogEntry> entries;
    
    if (!std::filesystem::exists(cfg_.log_path)) {
        return entries;
    }
    
    try {
        std::ifstream ifs(cfg_.log_path);
        std::string line;
        uint64_t entry_num = 0;
        
        while (std::getline(ifs, line)) {
            if (line.empty()) {
              continue;
            }
            
            try {
                nlohmann::json record = nlohmann::json::parse(line);
                
                AuditLogEntry entry;
                entry.entry_number = entry_num++;
                entry.record = record;
                
                // Extract timestamp from record
                if (record.contains("ts")) {
                    auto ts_ms = record["ts"].get<uint64_t>();
                    entry.timestamp = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(ts_ms));
                } else {
                    // Missing timestamp is suspicious - log warning and use current time
                    THEMIS_WARN("Audit log entry at line {} missing timestamp field - possible data corruption", entry_num + 1);
                    entry.timestamp = std::chrono::system_clock::now();
                }
                
                entries.push_back(entry);
                
            } catch (const std::exception& e) {
                THEMIS_WARN("Failed to parse audit log entry {}: {}", entry_num, e.what());
            }
        }
        
        THEMIS_INFO("Enumerated {} audit log entries from {}", entries.size(), cfg_.log_path);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to enumerate audit entries: {}", e.what());
    }
    
    return entries;
}

size_t AuditLogger::archiveOldEntries(std::chrono::system_clock::time_point older_than,
                                      const std::string& archive_path) {
    std::scoped_lock lock(file_mu_);
    
    if (!std::filesystem::exists(cfg_.log_path)) {
        return 0;
    }
    
    try {
        // Read all entries
        std::ifstream ifs(cfg_.log_path);
        std::string line;
        std::vector<std::string> kept_entries;
        std::vector<std::string> archived_entries;
        
        while (std::getline(ifs, line)) {
            if (line.empty()) {
              continue;
            }
            
            try {
                nlohmann::json record = nlohmann::json::parse(line);
                
                // Extract timestamp
                std::chrono::system_clock::time_point entry_ts;
                if (record.contains("ts")) {
                    auto ts_ms = record["ts"].get<uint64_t>();
                    entry_ts = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(ts_ms));
                } else {
                    // If no timestamp, keep the entry (don't archive unknown)
                    kept_entries.push_back(line);
                    continue;
                }
                
                // Check if entry should be archived
                if (entry_ts < older_than) {
                    archived_entries.push_back(line);
                } else {
                    kept_entries.push_back(line);
                }
                
            } catch (const nlohmann::json::exception &) {
                // Keep unparseable entries to avoid data loss
                kept_entries.push_back(line);
            } catch (const std::exception &) {
                // Keep unparseable entries to avoid data loss
                kept_entries.push_back(line);
            } catch (const std::string &) {
                // Keep unparseable entries to avoid data loss
                kept_entries.push_back(line);
            } catch (const char *) {
                // Keep unparseable entries to avoid data loss
                kept_entries.push_back(line);
            }
        }
        ifs.close();
        
        // If nothing to archive, return early
        if (archived_entries.empty()) {
            return 0;
        }
        
        // Write archived entries to archive file (append)
        auto archive_dir = std::filesystem::path(archive_path).parent_path();
        std::filesystem::create_directories(archive_dir);
        
        std::ofstream archive_ofs(archive_path, std::ios::app);
        if (!archive_ofs.is_open() || !archive_ofs.good()) {
            THEMIS_ERROR("Failed to open archive file for writing: {}", archive_path);
            return 0;
        }
        
        for (const auto& entry : archived_entries) {
            archive_ofs << entry << "\n";
        }
        archive_ofs.close();
        
        // Rewrite main log file with only kept entries
        std::ofstream main_ofs(cfg_.log_path, std::ios::trunc);
        if (!main_ofs.is_open() || !main_ofs.good()) {
            THEMIS_ERROR("Failed to open main log file for rewriting: {}", cfg_.log_path);
            return 0;
        }
        
        for (const auto& entry : kept_entries) {
            main_ofs << entry << "\n";
        }
        main_ofs.close();
        
        THEMIS_INFO("Archived {} audit log entries to {} (kept {} entries)", 
                    archived_entries.size(), archive_path, kept_entries.size());
        
        return archived_entries.size();
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to archive audit entries: {}", e.what());
        return 0;
    }
}

size_t AuditLogger::purgeOldEntries(std::chrono::system_clock::time_point older_than) {
    std::scoped_lock lock(file_mu_);
    
    if (!std::filesystem::exists(cfg_.log_path)) {
        return 0;
    }
    
    try {
        // Read all entries
        std::ifstream ifs(cfg_.log_path);
        std::string line;
        std::vector<std::string> kept_entries;
        size_t purged_count = 0;
        
        while (std::getline(ifs, line)) {
            if (line.empty()) {
              continue;
            }
            
            try {
                nlohmann::json record = nlohmann::json::parse(line);
                
                // Extract timestamp
                std::chrono::system_clock::time_point entry_ts;
                if (record.contains("ts")) {
                    auto ts_ms = record["ts"].get<uint64_t>();
                    entry_ts = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(ts_ms));
                } else {
                    // If no timestamp, keep the entry (don't purge unknown)
                    kept_entries.push_back(line);
                    continue;
                }
                
                // Check if entry should be purged
                if (entry_ts < older_than) {
                    purged_count++;
                    // Entry is purged (not added to kept_entries)
                } else {
                    kept_entries.push_back(line);
                }
                
            } catch (const nlohmann::json::exception &) {
                // Keep unparseable entries to avoid data loss
                kept_entries.push_back(line);
            } catch (const std::exception &) {
                // Keep unparseable entries to avoid data loss
                kept_entries.push_back(line);
            } catch (const std::string &) {
                // Keep unparseable entries to avoid data loss
                kept_entries.push_back(line);
            } catch (const char *) {
                // Keep unparseable entries to avoid data loss
                kept_entries.push_back(line);
            }
        }
        ifs.close();
        
        // If nothing to purge, return early
        if (purged_count == 0) {
            return 0;
        }
        
        // Rewrite main log file with only kept entries
        std::ofstream main_ofs(cfg_.log_path, std::ios::trunc);
        if (!main_ofs.is_open() || !main_ofs.good()) {
            THEMIS_ERROR("Failed to open main log file for rewriting: {}", cfg_.log_path);
            return 0;
        }
        
        for (const auto& entry : kept_entries) {
            main_ofs << entry << "\n";
        }
        main_ofs.close();
        
        THEMIS_INFO("Purged {} audit log entries (kept {} entries)", 
                    purged_count, kept_entries.size());
        
        return purged_count;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to purge audit entries: {}", e.what());
        return 0;
    }
}

// ============================================================================
// Task Scheduler SIEM Integration
// ============================================================================

void AuditLogger::logTaskSchedulerEvent(
    SecurityEventType event_type,
    const std::string& task_id,
    const std::string& user_id,
    const nlohmann::json& details
) {
    if (!cfg_.enabled || !cfg_.enable_task_scheduler_audit) {
        return;
    }
    
    nlohmann::json event = {
        {"event_type", securityEventTypeToString(event_type)},
        {"task_id", task_id},
        {"user_id", user_id},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()},
        {"category", "TASK_SCHEDULER"}
    };
    
    // Add all details
    if (!details.is_null() && !details.empty()) {
        for (auto& [key, value] : details.items()) {
            event[key] = value;
        }
    }
    
    // Calculate anomaly score if enabled and execution metrics are present
    if (cfg_.enable_anomaly_detection && 
        details.contains("execution_time_ms") && 
        event_type == SecurityEventType::TASK_EXECUTED_SUCCESS) {
        
        double execution_time = details["execution_time_ms"].get<double>();
        double anomaly_score = calculateAnomalyScore(task_id, execution_time, details);
        event["anomaly_score"] = anomaly_score;
        
        // If anomaly detected, log separate anomaly event
        if (anomaly_score > cfg_.anomaly_threshold) {
            nlohmann::json anomaly_details = details;
            anomaly_details["anomaly_score"] = anomaly_score;
            anomaly_details["threshold"] = cfg_.anomaly_threshold;
            
            logSecurityEvent(
                SecurityEventType::TASK_ANOMALY_DETECTED,
                user_id,
                task_id,
                anomaly_details
            );
        }
    }
    
    // Set severity based on event type
    if (event_type == SecurityEventType::TASK_EXECUTED_FAILURE ||
        event_type == SecurityEventType::TASK_TIMEOUT ||
        event_type == SecurityEventType::TASK_RESOURCE_LIMIT_EXCEEDED ||
        event_type == SecurityEventType::TASK_ANOMALY_DETECTED) {
        event["severity"] = "HIGH";
        THEMIS_WARN("Task scheduler event: {} - Task: {} - User: {}", 
            securityEventTypeToString(event_type), task_id, user_id);
    } else if (event_type == SecurityEventType::TASK_UNREGISTERED ||
               event_type == SecurityEventType::TASK_DISABLED ||
               event_type == SecurityEventType::TASK_UPDATED) {
        event["severity"] = "MEDIUM";
    } else {
        event["severity"] = "LOW";
    }
    
    logEvent(event);
}

double AuditLogger::calculateAnomalyScore(
    const std::string& task_id,
    double execution_time_ms,
    const nlohmann::json& /*resource_usage*/
) {
    std::lock_guard<std::mutex> lock(baselines_mu_);
    
    auto& baseline = task_baselines_[task_id];
    
    // Need at least 10 executions to establish baseline
    if (baseline.execution_count < 10) {
        updateTaskBaseline(task_id, execution_time_ms);
        return 0.0; // No anomaly for new tasks
    }
    
    // Calculate z-score for execution time
    double time_zscore = calculateZScore(
        execution_time_ms,
        baseline.avg_execution_time_ms,
        baseline.stddev_execution_time_ms
    );
    
    // Check execution frequency anomaly
    double frequency_zscore = 0.0;
    auto now = std::chrono::system_clock::now();
    if (baseline.execution_count > 1) {
        auto time_since_last = std::chrono::duration_cast<std::chrono::seconds>(
            now - baseline.last_execution
        ).count();
        
        if (baseline.avg_frequency_seconds > 0) {
            frequency_zscore = calculateZScore(
                static_cast<double>(time_since_last),
                baseline.avg_frequency_seconds,
                baseline.avg_frequency_seconds * 0.5 // Assume 50% stddev
            );
        }
    }
    
    // Update baseline
    updateTaskBaseline(task_id, execution_time_ms);
    
    // Return maximum z-score as anomaly score
    return std::max(std::abs(time_zscore), std::abs(frequency_zscore));
}

void AuditLogger::updateTaskBaseline(const std::string& task_id, double execution_time_ms) {
    auto& baseline = task_baselines_[task_id];
    
    auto now = std::chrono::system_clock::now();
    
    // Update frequency tracking
    if (baseline.execution_count > 0) {
        auto time_since_last = std::chrono::duration_cast<std::chrono::seconds>(
            now - baseline.last_execution
        ).count();
        
        // Update average frequency using exponential moving average
        if (baseline.avg_frequency_seconds == 0.0) {
            baseline.avg_frequency_seconds = static_cast<double>(time_since_last);
        } else {
            baseline.avg_frequency_seconds = 
                0.9 * baseline.avg_frequency_seconds + 0.1 * time_since_last;
        }
    }
    
    baseline.last_execution = now;
    
    // Update execution time statistics
    size_t n = baseline.execution_count;
    double old_mean = baseline.avg_execution_time_ms;
    
    // Welford's online algorithm for mean and variance
    baseline.execution_count++;
    double delta = execution_time_ms - old_mean;
    baseline.avg_execution_time_ms = old_mean + delta / baseline.execution_count;
    
    if (n > 0) {
        double delta2 = execution_time_ms - baseline.avg_execution_time_ms;
        double variance = (n * baseline.stddev_execution_time_ms * baseline.stddev_execution_time_ms + 
                          delta * delta2) / baseline.execution_count;
        baseline.stddev_execution_time_ms = std::sqrt(variance);
    }
}

double AuditLogger::calculateZScore(double value, double mean, double stddev) const {
    if (stddev < 0.001) { // Avoid division by zero
        return 0.0;
    }
    return (value - mean) / stddev;
}

// ============================================================================
// SIEM Format Converters
// ============================================================================

std::string AuditLogger::formatAsJson(const nlohmann::json& event) const {
    return event.dump();
}

std::string AuditLogger::formatAsCef(const nlohmann::json& event, SecurityEventType event_type) const {
    // CEF Format: CEF:Version|Device Vendor|Device Product|Device Version|Signature ID|Name|Severity|Extension
    std::ostringstream cef;
    
    cef << "CEF:0|ThemisDB|TaskScheduler|" << THEMISDB_VERSION << "|";
    
    // Signature ID (event type)
    cef << securityEventTypeToString(event_type) << "|";
    
    // Name (human-readable event name)
    std::string name = securityEventTypeToString(event_type);
    std::replace(name.begin(), name.end(), '_', ' ');
    cef << name << "|";
    
    // Severity (0-10 scale)
    std::string severity_str = event.value("severity", "LOW");
    int severity = 0;
    if (severity_str == "HIGH") {
      severity = 8;
    }
    else if (severity_str == "MEDIUM") severity = 5;
    else severity = 2;
    cef << severity << "|";
    
    // Extension fields
    std::vector<std::string> extensions;
    
    if (event.contains("task_id")) {
        extensions.push_back("taskId=" + event["task_id"].get<std::string>());
    }
    if (event.contains("user_id")) {
        extensions.push_back("suser=" + event["user_id"].get<std::string>());
    }
    if (event.contains("timestamp")) {
        extensions.push_back("rt=" + std::to_string(event["timestamp"].get<uint64_t>()));
    }
    if (event.contains("execution_time_ms")) {
        extensions.push_back("executionTime=" + std::to_string(event["execution_time_ms"].get<double>()));
    }
    if (event.contains("anomaly_score")) {
        extensions.push_back("anomalyScore=" + std::to_string(event["anomaly_score"].get<double>()));
    }
    if (event.contains("error_message")) {
        std::string msg = event["error_message"].get<std::string>();
        // Escape special characters in CEF
        std::replace(msg.begin(), msg.end(), '=', ':');
        std::replace(msg.begin(), msg.end(), '|', '/');
        extensions.push_back("msg=" + msg);
    }
    if (event.contains("source_ip")) {
        extensions.push_back("src=" + event["source_ip"].get<std::string>());
    }
    
    // Join extensions
    for (size_t i = 0; i < extensions.size(); ++i) {
        if (i > 0) {
          cef << " ";
        }
        cef << extensions[i];
    }
    
    return cef.str();
}

std::string AuditLogger::formatAsSyslog(const nlohmann::json& event, SecurityEventType event_type) const {
    // RFC 5424 Syslog format
    std::ostringstream syslog;
    
    // Priority: Facility (16 = local use 0) * 8 + Severity
    std::string severity_str = event.value("severity", "LOW");
    int severity = 6; // Informational
    if (severity_str == "HIGH") severity = 2; // Critical
    else if (severity_str == "MEDIUM") severity = 4; // Warning
    
    int priority = 16 * 8 + severity;
    syslog << "<" << priority << ">1 ";
    
    // Timestamp (ISO 8601)
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    #ifdef _WIN32
    gmtime_s(&tm, &time_t);
    #else
    gmtime_r(&time_t, &tm);
    #endif
    
    syslog << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S") << "Z ";
    
    // Hostname
    syslog << "themisdb-server ";
    
    // App name
    syslog << "task-scheduler ";
    
    // Process ID
    syslog << "- ";
    
    // Message ID
    syslog << securityEventTypeToString(event_type) << " ";
    
    // Structured data
    syslog << "[themis@32473";
    if (event.contains("task_id")) {
        syslog << " taskId=\"" << event["task_id"].get<std::string>() << "\"";
    }
    if (event.contains("user_id")) {
        syslog << " userId=\"" << event["user_id"].get<std::string>() << "\"";
    }
    if (event.contains("anomaly_score")) {
        syslog << " anomalyScore=\"" << event["anomaly_score"].get<double>() << "\"";
    }
    syslog << "] ";
    
    // Message
    syslog << "Task Scheduler Event: " << securityEventTypeToString(event_type);
    
    return syslog.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 3: Audit Search & Compliance Reporting
// ─────────────────────────────────────────────────────────────────────────────

std::vector<AuditLogger::AuditLogEntry> AuditLogger::searchEntries(
    const SearchQuery& query) const {

    std::vector<AuditLogEntry> results = {};

    if (!std::filesystem::exists(cfg_.log_path)) {
        return results;
    }

    std::scoped_lock lk(file_mu_);
    std::ifstream ifs(cfg_.log_path);
    std::string line;
    uint64_t entry_num = 0;

    while (std::getline(ifs, line)) {
        if (line.empty()) {
          continue;
        }
        try {
            auto record = nlohmann::json::parse(line);
            ++entry_num;

            // Timestamp filter
            std::chrono::system_clock::time_point ts;
            if (record.contains("ts")) {
                auto ms = record["ts"].get<int64_t>();
                ts = std::chrono::system_clock::time_point(
                    std::chrono::milliseconds(ms));
            }
            if (query.from && ts < *query.from) {
              continue;
            }
            if (query.to   && ts >= *query.to) {
              continue;
            }

            // Extract payload for field-based filters (only plaintext entries)
            nlohmann::json payload;
            if (record.contains("payload")) {
                const auto& p = record["payload"];
                if (p.contains("type") && p["type"] == "plaintext") {
                    if (p.contains("data")) {
                        payload = p["data"];
                    } else if (p.contains("data_b64")) {
                        auto decoded = base64_decode_local(p["data_b64"].get<std::string>());
                        payload = nlohmann::json::parse(decoded);
                    }
                }
            }

            // User filter
            if (!query.user_id.empty()) {
                std::string uid = payload.value("user", payload.value("user_id", std::string{}));
                if (uid != query.user_id) {
                  continue;
                }
            }

            // Action filter
            if (!query.action.empty()) {
                std::string action = payload.value("action",
                    payload.value("event_type", std::string{}));
                if (action.find(query.action) == std::string::npos) {
                  continue;
                }
            }

            // Resource prefix filter
            if (!query.resource_prefix.empty()) {
                std::string resource = payload.value("resource", std::string{});
                if (resource.substr(0, query.resource_prefix.size()) != query.resource_prefix) {
                    continue;
                }
            }

            AuditLogEntry entry;
            entry.entry_number = entry_num;
            entry.timestamp    = ts;
            entry.record       = std::move(record);
            results.push_back(std::move(entry));

            if (query.max_results > 0 && results.size() >= query.max_results) {
              break;
            }

        } catch (const nlohmann::json::exception &) {
            // Skip malformed lines
        } catch (const std::exception &) {
            // Skip malformed lines
        } catch (const std::string &) {
            // Skip malformed lines
        } catch (const char *) {
            // Skip malformed lines
        }
    }
    return results;
}

AuditLogger::ComplianceReport AuditLogger::generateComplianceReport(
    std::chrono::system_clock::time_point from,
    std::chrono::system_clock::time_point to) {

    ComplianceReport report;
    report.from = from;
    report.to   = to;
    report.chain_intact = verifyChainIntegrity();

    nlohmann::json type_counts = nlohmann::json::object();
    nlohmann::json user_counts = nlohmann::json::object();

    if (!std::filesystem::exists(cfg_.log_path)) {
        return report;
    }

    std::scoped_lock lk(file_mu_);
    std::ifstream ifs(cfg_.log_path);
    std::string line;

    while (std::getline(ifs, line)) {
        if (line.empty()) {
          continue;
        }
        try {
            auto record = nlohmann::json::parse(line);

            std::chrono::system_clock::time_point ts;
            if (record.contains("ts")) {
                auto ms = record["ts"].get<int64_t>();
                ts = std::chrono::system_clock::time_point(
                    std::chrono::milliseconds(ms));
            }
            if (ts < from || ts >= to) {
              continue;
            }

            ++report.total_events;

            // Extract plaintext payload for categorisation
            nlohmann::json payload;
            if (record.contains("payload")) {
                const auto& p = record["payload"];
                if (p.value("type", "") == "plaintext") {
                    if (p.contains("data")) {
                        payload = p["data"];
                    } else if (p.contains("data_b64")) {
                        auto decoded = base64_decode_local(p["data_b64"].get<std::string>());
                        payload = nlohmann::json::parse(decoded);
                    }
                }
            }

            std::string event_type = payload.value("event_type",
                payload.value("action", std::string{"unknown"}));

            // Count by type
            type_counts[event_type] = type_counts.value(event_type, 0) + 1;

            // Categorise by event-type string
            if (event_type.find("LOGIN") != std::string::npos ||
                event_type.find("TOKEN") != std::string::npos ||
                event_type.find("LOGOUT") != std::string::npos) {
                ++report.authentication_events;
            } else if (event_type.find("DATA_") != std::string::npos ||
                       event_type.find("BULK_") != std::string::npos) {
                ++report.data_access_events;
            } else if (event_type.find("KEY_") != std::string::npos ||
                       event_type.find("LEK_") != std::string::npos) {
                ++report.key_management_events;
            } else if (event_type.find("PII_") != std::string::npos) {
                ++report.pii_events;
            }

            // Anything with a "severity" field is a security event
            if (payload.contains("severity") ||
                event_type.find("UNAUTHORIZED") != std::string::npos ||
                event_type.find("DENIED") != std::string::npos) {
                ++report.security_events;
            }

            // Track per-user counts
            std::string user = payload.value("user_id", payload.value("user", std::string{"system"}));
            user_counts[user] = user_counts.value(user, 0) + 1;

        } catch (const nlohmann::json::exception &) {
            // Skip malformed lines
        } catch (const std::exception &) {
            // Skip malformed lines
        } catch (const std::string &) {
            // Skip malformed lines
        } catch (const char *) {
            // Skip malformed lines
        }
    }

    report.event_counts_by_type = std::move(type_counts);
    report.top_users            = std::move(user_counts);
    return report;
}

// ===========================================================================
// HashChainAuditWriter
// ===========================================================================

/* static */
std::vector<uint8_t> HashChainAuditWriter::sha256(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> digest(SHA256_DIGEST_LENGTH);
    SHA256(data.data(), data.size(), digest.data());
    return digest;
}

/* static */
std::string HashChainAuditWriter::bytesToHex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto b : data) {
      oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

void HashChainAuditWriter::loadOrInitChainHead(const std::string& chain_seed) {
    namespace fs = std::filesystem;

    if (fs::exists(cfg_.chain_head_path)) {
        try {
            std::ifstream ifs(cfg_.chain_head_path);
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            if (!content.empty() && content.front() == '{') {
                auto j = nlohmann::json::parse(content);
                last_hash_ = j.value("last_hash", std::string(64, '0'));
                seq_       = j.value("seq", uint64_t{0});
                return;
            }

            std::istringstream iss(content);
            std::string hash_line;
            uint64_t seq = 0;
            if (std::getline(iss, hash_line) && (iss >> seq)) {
                last_hash_ = hash_line.empty() ? std::string(64, '0') : hash_line;
                seq_ = seq;
                return;
            }
        } catch (const nlohmann::json::exception &) {
            // Fall through to re-initialise on corrupted file.
        } catch (const std::exception &) {
            // Fall through to re-initialise on corrupted file.
        } catch (const std::string &) {
            // Fall through to re-initialise on corrupted file.
        } catch (const char *) {
            // Fall through to re-initialise on corrupted file.
        }
    }

    // Initialise: genesis hash = SHA-256(chain_seed) or 64 zeros.
    if (!chain_seed.empty()) {
        std::vector<uint8_t> seed_bytes(chain_seed.begin(), chain_seed.end());
        last_hash_ = bytesToHex(sha256(seed_bytes));
    } else {
        last_hash_ = std::string(64, '0');
    }
    seq_ = 0;
}

void HashChainAuditWriter::saveChainHead() {
    try {
        if (!chain_head_stream_.is_open()) {
            logErrorWithContext(makeErrorContext(
                ErrorCode::AUDIT_PERSISTENCE_FAILED,
                "chain head stream is not open",
                "HashChainAuditWriter::saveChainHead",
                ErrorSeverity::Error,
                /*is_recoverable=*/false));
            throw std::runtime_error("chain head stream is not open");
        }

        chain_head_stream_.seekp(0);
        chain_head_stream_.clear();
        chain_head_stream_ << last_hash_ << '\n' << seq_ << '\n';
        chain_head_stream_.flush();

#ifndef _WIN32
        if (cfg_.fsync_on_write) {
            int fd = ::open(cfg_.chain_head_path.c_str(), O_RDONLY);
            if (fd >= 0) {
                FdGuard guard(fd);
                ::fdatasync(guard.fd_);
            }
        }
#endif
    } catch (const std::exception& e) {
        THEMIS_ERROR("HashChainAuditWriter: failed to save chain head to {}: {}",
                     cfg_.chain_head_path, e.what());
    }
}

HashChainAuditWriter::HashChainAuditWriter(HashChainAuditWriterConfig cfg,
                                           const std::string& chain_seed)
    : cfg_(std::move(cfg))
{
    namespace fs = std::filesystem;
    try {
        fs::create_directories(fs::path(cfg_.log_path).parent_path());
        fs::create_directories(fs::path(cfg_.chain_head_path).parent_path());
    } catch (const std::filesystem::filesystem_error &) {
    } catch (const std::exception &) {
    } catch (const std::string &) {
    } catch (const char *) {
    }

    loadOrInitChainHead(chain_seed);

    chain_head_stream_.open(cfg_.chain_head_path,
                            std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    if (!chain_head_stream_.is_open()) {
        THEMIS_ERROR("HashChainAuditWriter: failed to open chain head file {}", cfg_.chain_head_path);
    } else {
        saveChainHead();
    }

    log_stream_.open(cfg_.log_path, std::ios::app | std::ios::binary);
    if (!log_stream_.is_open()) {
        THEMIS_ERROR("HashChainAuditWriter: failed to open log file {}", cfg_.log_path);
    }
}

HashChainAuditWriter::~HashChainAuditWriter() = default;

void HashChainAuditWriter::write(nlohmann::json record) {
    std::lock_guard<std::mutex> lk(mu_);

    // Inject chain fields.
    record["chain_seq"]  = seq_;
    record["prev_hash"]  = last_hash_;

    // Compute new chain head: SHA-256(prev_hash || record_json)
    std::string record_json = record.dump();
    std::string hash_input  = last_hash_ + record_json;
    std::vector<uint8_t> hash_bytes(hash_input.begin(), hash_input.end());
    last_hash_ = bytesToHex(sha256(hash_bytes));
    ++seq_;

    // Append record to log file.
    try {
        if (!log_stream_.is_open()) {
            log_stream_.open(cfg_.log_path, std::ios::app | std::ios::binary);
        }
        if (!log_stream_.is_open()) {
            logErrorWithContext(makeErrorContext(
                ErrorCode::AUDIT_PERSISTENCE_FAILED,
                fmt::format("log stream could not be opened: {}", cfg_.log_path),
                "HashChainAuditWriter::append",
                ErrorSeverity::Error,
                /*is_recoverable=*/true));
            throw std::runtime_error("log stream is not open");
        }
        log_stream_ << record_json << '\n';
        if (cfg_.fsync_on_write) {
            log_stream_.flush();
        }
    } catch (const std::exception& e) {
        logErrorWithContext(makeErrorContext(
            ErrorCode::AUDIT_WRITE_FAILED,
            fmt::format("failed to append log to {}: {}", cfg_.log_path, e.what()),
            "HashChainAuditWriter::append",
            ErrorSeverity::Error,
            /*is_recoverable=*/true));
        THEMIS_ERROR("HashChainAuditWriter: failed to append log to {}: {}",
                     cfg_.log_path, e.what());
    }

    // Persist chain head (fsync if configured).
    saveChainHead();
}

std::string HashChainAuditWriter::headHash() const {
    std::lock_guard<std::mutex> lk(mu_);
    return last_hash_;
}

uint64_t HashChainAuditWriter::sequenceNumber() const {
    std::lock_guard<std::mutex> lk(mu_);
    return seq_;
}

// ===========================================================================
// AuditLogVerifier
// ===========================================================================

/* static */
std::string AuditLogVerifier::computeEntryHash(const std::string& prev_hash,
                                                const nlohmann::json& entry) {
    std::string record_json = entry.dump();
    std::string hash_input  = prev_hash + record_json;

    std::vector<uint8_t> bytes(hash_input.begin(), hash_input.end());
    std::vector<uint8_t> digest(SHA256_DIGEST_LENGTH);
    SHA256(bytes.data(), bytes.size(), digest.data());

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto b : digest) {
      oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

AuditVerifyResult AuditLogVerifier::verify_chain(const std::string& log_path,
                                                  const std::string& genesis_hash) const {
    AuditVerifyResult result;

    if (!std::filesystem::exists(log_path)) {
        result.ok            = true;
        result.entries_total = 0;
        return result;
    }

    std::ifstream ifs(log_path);
    if (!ifs) {
        result.ok            = false;
        result.error_message = "Cannot open log file: " + log_path;
        return result;
    }

    std::string expected_prev_hash = genesis_hash;

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) {
          continue;
        }
        ++result.entries_total;

        nlohmann::json entry = nlohmann::json::parse(line, nullptr, /*allow_exceptions=*/false);
        if (entry.is_discarded()) {
            result.ok            = false;
            result.first_bad_seq = result.entries_total - 1;
            result.error_message = "Malformed JSON at line " +
                                   std::to_string(result.entries_total);
            return result;
        }

        if (!entry.contains("prev_hash") || !entry["prev_hash"].is_string()) {
            // Entry predates hash chain — skip silently.
            ++result.entries_ok;
            continue;
        }

        const std::string& stored_prev = entry["prev_hash"].get_ref<const std::string&>();
        if (stored_prev != expected_prev_hash) {
            result.ok = false;
            // Use chain_seq field if present; otherwise fall back to 1-based line index.
            if (entry.contains("chain_seq") && entry["chain_seq"].is_number_unsigned()) {
                result.first_bad_seq = entry["chain_seq"].get<uint64_t>();
            } else {
                result.first_bad_seq = result.entries_total - 1;
            }
            result.error_message = "Hash chain broken at seq " +
                                   std::to_string(result.first_bad_seq) +
                                   ": expected prev_hash " + expected_prev_hash.substr(0, 16) +
                                   "... got " + stored_prev.substr(0, 16) + "...";
            return result;
        }

        expected_prev_hash = computeEntryHash(expected_prev_hash, entry);
        ++result.entries_ok;
    }

    return result;
}

} // namespace utils
} // namespace themis
