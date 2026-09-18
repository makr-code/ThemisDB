/**
 * @file voice_audio_storage.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Audio storage & retrieval – Phase 5 production readiness
#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <mutex>
#include <functional>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis { namespace voice {
using json = nlohmann::json;

// Audio storage tier
enum class StorageTier {
    HOT,    // Frequent access: in-memory or fast disk
    WARM,   // Recent but not active: compressed on disk
    COLD,   // Archive: heavily compressed, slow access
    DELETED // Marked for deletion / purged
};
/**
 * @brief TBD: Describe storageTierToString.
 * @param[in] tier Input parameter.
 * @return Return value.
 */
std::string storageTierToString(StorageTier tier);

// Audio format descriptor
struct AudioFormat {
    std::string codec;       // "pcm", "wav", "ogg", "mp3", "opus", "aac"
    int sample_rate = 16000;
    int channels = 1;
    int bit_depth = 16;
    float duration_seconds = 0.0f;
    size_t size_bytes = 0;
};

// Audio storage record (metadata + pointer to data)
struct AudioStorageRecord {
    std::string record_id;         // Unique ID
    std::string content_hash;      // FNV-1a hash for deduplication
    StorageTier tier = StorageTier::HOT;
    AudioFormat format;
    std::string transcript;
    json metadata;
    int64_t created_at_ms = 0;
    int64_t last_accessed_ms = 0;
    int64_t expires_at_ms = 0;    // 0 = no expiry
    bool encrypted = false;
    std::string encryption_key_id; // Key reference (not the key itself)
    size_t access_count = 0;
    bool is_duplicate = false;
    std::string duplicate_of;     // Points to original if duplicate
};

// Tiered storage policy
struct StorageTierPolicy {
    int64_t hot_to_warm_after_ms = 7LL * 24 * 3600 * 1000;      // 7 days
    int64_t warm_to_cold_after_ms = 30LL * 24 * 3600 * 1000;    // 30 days
    int64_t cold_to_delete_after_ms = 365LL * 24 * 3600 * 1000; // 1 year
    size_t max_hot_bytes = 1024ULL * 1024 * 1024;                // 1 GB
    size_t max_warm_bytes = 10ULL * 1024 * 1024 * 1024;          // 10 GB
    bool enable_auto_tier = true;
};

// AES-256 encryption config (key management only - no actual crypto dependency)
struct EncryptionConfig {
    bool enabled = false;
    std::string key_provider = "local"; // "local", "vault", "hsm"
    std::string key_id;                 // Key reference
    std::string algorithm = "AES-256-GCM";
};

// Deduplication result
struct DeduplicationResult {
    bool is_duplicate = false;
    std::string existing_record_id;  // If duplicate, the original's ID
    std::string content_hash;
    size_t bytes_saved = 0;
};

// Storage statistics
struct StorageStats {
    size_t total_records = 0;
    size_t hot_records = 0;
    size_t warm_records = 0;
    size_t cold_records = 0;
    size_t total_bytes = 0;
    size_t deduplicated_bytes = 0;
    size_t deduplication_hits = 0;
    size_t encryption_enabled_records = 0;
};

// VoiceAudioStorage: Phase 5 production component
/** @brief VoiceAudioStorage: Phase 5 production component. */
class VoiceAudioStorage {
public:
    explicit VoiceAudioStorage(
        const StorageTierPolicy& policy = {},
        const EncryptionConfig& enc_config = {}
    );
    ~VoiceAudioStorage() = default;

    // Store audio with metadata, returns record ID
    std::string store(
        const std::vector<uint8_t>& audio_data,
        const AudioFormat& format,
        const std::string& transcript = "",
        const json& metadata = {}
    );

    /**
     * @brief Retrieve audio data by record ID
     * @param[in] record_id Input parameter.
     * @return Return value.
     */
    std::optional<std::vector<uint8_t>> retrieve(const std::string& record_id);

    /**
     * @brief Get record metadata without loading audio
     * @param[in] record_id Input parameter.
     * @return Return value.
     */
    std::optional<AudioStorageRecord> getRecord(const std::string& record_id) const;

    /**
     * @brief Delete record
     * @param[in] record_id Input parameter.
     * @return True on success.
     */
    bool deleteRecord(const std::string& record_id);

    // List records matching filters
    std::vector<AudioStorageRecord> listRecords(
        StorageTier tier_filter = StorageTier::HOT,
        size_t limit = 100
    ) const;

    // Search transcripts by keyword (case-insensitive substring match)
    // Returns records whose transcript field contains the query string.
    std::vector<AudioStorageRecord> searchTranscripts(
        const std::string& query,
        size_t limit = 100
    ) const;

    /**
     * @brief Deduplication: compute hash and check for duplicate
     * @param[in] audio_data Input parameter.
     * @return Return value.
     */
    DeduplicationResult checkDuplicate(const std::vector<uint8_t>& audio_data) const;
    /**
     * @brief TBD: Describe computeHash.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::string computeHash(const std::vector<uint8_t>& data) const;

    /**
     * @brief Tiered storage management
     * @return Return value.
     */
    size_t applyTierPolicy();  // Returns number of records moved
    /**
     * @brief TBD: Describe computeTier.
     * @param[in] record Input parameter.
     * @return Return value.
     */
    StorageTier computeTier(const AudioStorageRecord& record) const;
    /**
     * @brief TBD: Describe promoteTier.
     * @param[in] record_id Input parameter.
     * @return True on success.
     */
    bool promoteTier(const std::string& record_id);   // Move up a tier
    /**
     * @brief TBD: Describe demoteTier.
     * @param[in] record_id Input parameter.
     * @return True on success.
     */
    bool demoteTier(const std::string& record_id);    // Move down a tier

    /**
     * @brief Encryption wrapper (marks records as encrypted, stores key reference)
     * @param[in] record_id Input parameter.
     * @param[in] key_id Input parameter.
     * @return True on success.
     */
    bool markEncrypted(const std::string& record_id, const std::string& key_id);
    /**
     * @brief TBD: Describe isEncrypted.
     * @param[in] record_id Input parameter.
     * @return True on success.
     */
    bool isEncrypted(const std::string& record_id) const;

    /**
     * @brief Audio format detection from raw bytes (magic bytes check)
     * @param[in] data Input parameter.
     * @return Return value.
     */
    AudioFormat detectFormat(const std::vector<uint8_t>& data) const;

    /**
     * @brief Storage statistics
     * @return Return value.
     */
    StorageStats getStats() const;

private:
    StorageTierPolicy policy_;
    EncryptionConfig enc_config_;
    mutable std::mutex mutex_;

    std::map<std::string, AudioStorageRecord> records_;   // metadata
    std::map<std::string, std::vector<uint8_t>> data_;    // actual audio data
    std::map<std::string, std::string> hash_to_id_;       // dedup index

    /**
     * @brief TBD: Describe generateRecordId.
     * @return Return value.
     */
    std::string generateRecordId() const;
    /**
     * @brief TBD: Describe nowMs.
     * @return Return value.
     */
    int64_t nowMs() const;
};

}} // namespace themis::voice
