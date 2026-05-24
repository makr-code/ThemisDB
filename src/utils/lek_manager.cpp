/*
 * ThemisDB | File: lek_manager.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 93/100 | Lines: 365
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=92 | delta=89 | status=divergent
 * External Severity (v3): C=1, H=77, M=14
 * PR: #4216 feat(timeseries): Chunk-Level AES-256-GCM Encryption at Rest (v1.7.0) (2026-03-14T17:34:38Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "utils/lek_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/audit_logger.h"
#include "utils/hkdf_helper.h"

#include <openssl/rand.h>
#include <openssl/evp.h>
#include <spdlog/spdlog.h>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace themis {
namespace utils {

LEKManager::LEKManager(std::shared_ptr<themis::RocksDBWrapper> db,
                       std::shared_ptr<VCCPKIClient> pki,
                       std::shared_ptr<KeyProvider> key_provider)
    : db_(std::move(db))
    , pki_(std::move(pki))
    , key_provider_(std::move(key_provider)) {
    
    // Ensure KEK exists
    if (!key_provider_->hasKey(kek_key_id_)) {
        auto kek = deriveKEK();
        const uint32_t version = key_provider_->createKeyFromBytes(kek_key_id_, kek);
        if (version == 0) {
            throw std::runtime_error("Failed to create KEK in key provider");
        }
    }
}

LEKManager::~LEKManager() {
    stopAutoRotation();
}

std::string LEKManager::getCurrentDateString() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

std::string LEKManager::lekKeyId(const std::string& date_str) const {
    return "lek_" + date_str;
}

std::string LEKManager::dbKey(const std::string& date_str) const {
    return "lek:encrypted:" + date_str;
}

std::vector<uint8_t> LEKManager::deriveKEK() {
    // Derive KEK from PKI certificate using HKDF
    // For now, use a deterministic derivation from service ID
    // In production: use actual certificate's public key material
    
    std::string service_id = "themis-lek-kek";
    std::string info = "KEK for ThemisDB LEK";
    
    return HKDFHelper::deriveFromString(service_id, info, 32);
}

void LEKManager::ensureLEKExists(const std::string& date_str) {
    auto key_id = lekKeyId(date_str);
    
    // Check if already in KeyProvider
    if (key_provider_->hasKey(key_id)) {
        return;
    }
    
    // Check if encrypted LEK exists in DB
    auto db_key_str = dbKey(date_str);
    auto encrypted_lek_opt = db_->get(db_key_str);
    
    if (encrypted_lek_opt) {
        // Decrypt and load into KeyProvider
        try {
            auto encrypted_lek_json = nlohmann::json::parse(*encrypted_lek_opt);
            auto blob = themis::EncryptedBlob::fromJson(encrypted_lek_json);
            
            FieldEncryption enc(key_provider_);
            auto lek_bytes = enc.decrypt(blob);
            
            const uint32_t version = key_provider_->createKeyFromBytes(
                key_id,
                std::vector<uint8_t>(lek_bytes.begin(), lek_bytes.end()));
            if (version == 0) {
                throw std::runtime_error("Failed to load LEK into key provider");
            }
            
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to decrypt LEK for " + date_str + ": " + e.what());
        }
        
    } else {
        // Generate new LEK
        std::vector<uint8_t> lek(32); // 256-bit AES key
        if (RAND_bytes(lek.data(), static_cast<int>(lek.size())) != 1) {
            throw std::runtime_error("Failed to generate random LEK");
        }
        
        // Encrypt with KEK
        FieldEncryption enc(key_provider_);
        std::string lek_plaintext(lek.begin(), lek.end());
        auto encrypted_lek = enc.encrypt(lek_plaintext, kek_key_id_);
        
        // Store in DB
        auto encrypted_json = themis::EncryptedBlob{encrypted_lek}.toJson();
        std::string json_str = encrypted_json.dump();
        std::vector<uint8_t> json_bytes(json_str.begin(), json_str.end());
        if (!db_->put(db_key_str, json_bytes)) {
            throw std::runtime_error("Failed to persist encrypted LEK in RocksDB");
        }
        
        // Load into KeyProvider
        const uint32_t version = key_provider_->createKeyFromBytes(key_id, lek);
        if (version == 0) {
            throw std::runtime_error("Failed to register generated LEK in key provider");
        }
    }
}

std::string LEKManager::getCurrentLEK() {
    std::scoped_lock lk(mu_);
    auto date_str = getCurrentDateString();
    
    // Check cache
    auto it = lek_cache_.find(date_str);
    if (it != lek_cache_.end()) {
        return it->second;
    }
    
    // Ensure exists and load
    ensureLEKExists(date_str);
    auto key_id = lekKeyId(date_str);
    lek_cache_[date_str] = key_id;
    
    return key_id;
}

std::string LEKManager::getLEKForDate(const std::string& date_str) {
    std::scoped_lock lk(mu_);
    
    // Check cache
    auto it = lek_cache_.find(date_str);
    if (it != lek_cache_.end()) {
        return it->second;
    }
    
    // Try to load from DB
    try {
        ensureLEKExists(date_str);
        auto key_id = lekKeyId(date_str);
        lek_cache_[date_str] = key_id;
        return key_id;
    } catch (...) {
        return ""; // LEK not found for this date
    }
}

void LEKManager::rotate() {
    std::scoped_lock lk(mu_);
    auto date_str = getCurrentDateString();
    
    // Remove from cache to force regeneration
    lek_cache_.erase(date_str);
    
    // Delete from DB
    if (!db_->del(dbKey(date_str))) {
        throw std::runtime_error("Failed to delete rotated LEK from RocksDB");
    }
    
    // Regenerate
    ensureLEKExists(date_str);
    lek_cache_[date_str] = lekKeyId(date_str);
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 5: Key Lifecycle – Revocation & Expiry
// ─────────────────────────────────────────────────────────────────────────────

bool LEKManager::revokeKey(const std::string& date_str) {
    // Mark in-memory
    {
        std::lock_guard<std::mutex> rlk(revocation_mu_);
        revoked_keys_.insert(date_str);
    }

    // Persist revocation flag to RocksDB
    if (db_) {
        try {
            static_cast<void>(db_->put("lek_revoked:" + date_str, "1"));
        } catch (...) {
            // Persistence is best-effort; revocation is already in-memory
        }
    }
    return true;
}

bool LEKManager::isRevoked(const std::string& date_str) const {
    std::lock_guard<std::mutex> rlk(revocation_mu_);
    return revoked_keys_.count(date_str) > 0;
}

std::vector<std::string> LEKManager::getRevokedKeys() const {
    std::lock_guard<std::mutex> rlk(revocation_mu_);
    return std::vector<std::string>(revoked_keys_.begin(), revoked_keys_.end());
}

bool LEKManager::isExpired(const std::string& date_str, int max_age_days) {
    // Parse date_str "YYYY-MM-DD"
    if (date_str.size() != 10) return false;
    try {
        int year  = std::stoi(date_str.substr(0, 4));
        int month = std::stoi(date_str.substr(5, 2));
        int day   = std::stoi(date_str.substr(8, 2));

        std::tm key_tm{};
        key_tm.tm_year = year - 1900;
        key_tm.tm_mon  = month - 1;
        key_tm.tm_mday = day;
        auto key_time  = std::chrono::system_clock::from_time_t(std::mktime(&key_tm));

        auto age_days = std::chrono::duration_cast<std::chrono::hours>(
            std::chrono::system_clock::now() - key_time).count() / 24;
        return age_days > max_age_days;
    } catch (...) {
        return false;
    }
}

bool LEKManager::migrateKey(const std::string& old_date, const std::string& new_date) {
    if (!db_) return false;
    try {
        // Copy the encrypted blob
        auto old_db_key = dbKey(old_date);
        auto new_db_key = dbKey(new_date);
        std::string blob;
        if (!db_->get(old_db_key, blob)) return false;
        if (!db_->put(new_db_key, blob)) return false;

        // Update in-memory cache
        {
            std::scoped_lock lk(mu_);
            if (lek_cache_.count(old_date)) {
                lek_cache_[new_date] = lekKeyId(new_date);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Automated Key Rotation
// ─────────────────────────────────────────────────────────────────────────────

void LEKManager::setAuditLogger(std::shared_ptr<AuditLogger> logger) {
    std::lock_guard<std::mutex> lk(audit_mu_);
    audit_logger_ = std::move(logger);
}

void LEKManager::startAutoRotation(std::chrono::seconds check_interval,
                                   int max_age_days) {
    if (max_age_days < 1) {
        throw std::invalid_argument("max_age_days must be >= 1");
    }
    if (rotation_running_.exchange(true)) {
        // Already running – no-op
        return;
    }

    {
        std::lock_guard<std::mutex> lk(rotation_cv_mu_);
        rotation_stop_ = false;
    }

    rotation_thread_ = std::thread(
        &LEKManager::autoRotationLoop, this, check_interval, max_age_days);
}

void LEKManager::stopAutoRotation() {
    {
        std::lock_guard<std::mutex> lk(rotation_cv_mu_);
        rotation_stop_ = true;
    }
    rotation_cv_.notify_all();
    if (rotation_thread_.joinable()) {
        rotation_thread_.join();
    }
    rotation_running_.store(false);
}

bool LEKManager::isAutoRotationRunning() const noexcept {
    return rotation_running_.load();
}

void LEKManager::autoRotationLoop(std::chrono::seconds check_interval,
                                  int max_age_days) {
    while (true) {
        // Sleep for the configured interval or until stopped
        {
            std::unique_lock<std::mutex> lk(rotation_cv_mu_);
            bool stopped = rotation_cv_.wait_for(
                lk, check_interval, [this] { return rotation_stop_; });
            if (stopped) break;
        }

        try {
            auto date_str = getCurrentDateString();

            // Ensure today's LEK exists; this handles midnight transitions
            // automatically – when the calendar date changes a new key is
            // created without any operator intervention.
            getCurrentLEK();

            // Collect any cached keys that have exceeded max_age_days
            std::vector<std::string> to_revoke;
            {
                std::scoped_lock lk(mu_);
                for (const auto& [cached_date, key_id] : lek_cache_) {
                    if (cached_date != date_str &&
                        isExpired(cached_date, max_age_days)) {
                        to_revoke.push_back(cached_date);
                    }
                }
            }

            // Revoke expired keys and emit audit events
            for (const auto& expired_date : to_revoke) {
                auto old_key_id = lekKeyId(expired_date);
                revokeKey(expired_date);

                std::shared_ptr<AuditLogger> logger;
                {
                    std::lock_guard<std::mutex> alk(audit_mu_);
                    logger = audit_logger_;
                }
                if (logger) {
                    logger->logSecurityEvent(
                        SecurityEventType::KEY_ROTATED,
                        "lek_manager",
                        "lek:" + expired_date,
                        {{"old_key_id", old_key_id},
                         {"reason", "max_age_exceeded"},
                         {"max_age_days", max_age_days}});
                }
            }
        } catch (const std::exception& e) {
            // Errors are non-fatal; the worker continues to the next interval
            spdlog::error("LEKManager auto-rotation error: {}", e.what());
        } catch (...) {
            spdlog::error("LEKManager auto-rotation: unknown error");
        }
    }
    rotation_running_.store(false);
}

} // namespace utils
} // namespace themis

