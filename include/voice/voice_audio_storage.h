/**
 * @file voice_audio_storage.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

    // Retrieve audio data by record ID
    std::optional<std::vector<uint8_t>> retrieve(const std::string& record_id);

    // Get record metadata without loading audio
    std::optional<AudioStorageRecord> getRecord(const std::string& record_id) const;

    // Delete record
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

    // Deduplication: compute hash and check for duplicate
    DeduplicationResult checkDuplicate(const std::vector<uint8_t>& audio_data) const;
    std::string computeHash(const std::vector<uint8_t>& data) const;

    // Tiered storage management
    size_t applyTierPolicy();  // Returns number of records moved
    StorageTier computeTier(const AudioStorageRecord& record) const;
    bool promoteTier(const std::string& record_id);   // Move up a tier
    bool demoteTier(const std::string& record_id);    // Move down a tier

    // Encryption wrapper (marks records as encrypted, stores key reference)
    bool markEncrypted(const std::string& record_id, const std::string& key_id);
    bool isEncrypted(const std::string& record_id) const;

    // Audio format detection from raw bytes (magic bytes check)
    AudioFormat detectFormat(const std::vector<uint8_t>& data) const;

    // Storage statistics
    StorageStats getStats() const;

private:
    StorageTierPolicy policy_;
    EncryptionConfig enc_config_;
    mutable std::mutex mutex_;

    std::map<std::string, AudioStorageRecord> records_;   // metadata
    std::map<std::string, std::vector<uint8_t>> data_;    // actual audio data
    std::map<std::string, std::string> hash_to_id_;       // dedup index

    std::string generateRecordId() const;
    int64_t nowMs() const;
};

}} // namespace themis::voice
