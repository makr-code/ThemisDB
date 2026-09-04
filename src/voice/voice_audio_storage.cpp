/**
 * @file voice_audio_storage.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "voice/voice_audio_storage.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <atomic>
#include <cctype>

namespace themis { namespace voice {

std::string storageTierToString(StorageTier tier) {
    switch (tier) {
        case StorageTier::HOT:     return "hot";
        case StorageTier::WARM:    return "warm";
        case StorageTier::COLD:    return "cold";
        case StorageTier::DELETED: return "deleted";
        default:                   return "unknown";
    }
}

VoiceAudioStorage::VoiceAudioStorage(
    const StorageTierPolicy& policy,
    const EncryptionConfig& enc_config)
    : policy_(policy), enc_config_(enc_config)
{}

int64_t VoiceAudioStorage::nowMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string VoiceAudioStorage::generateRecordId() const {
    static std::atomic<uint64_t> counter{0};
    auto ts = nowMs();
    std::ostringstream oss = {};
    oss << "rec-" << std::hex << ts << "-" << (++counter);
    return oss.str();
}

std::string VoiceAudioStorage::computeHash(const std::vector<uint8_t>& data) const {
    // FNV-1a 64-bit hash
    uint64_t hash = 14695981039346656037;
    for (uint8_t b : data) {
        hash ^= static_cast<uint64_t>(b);
        hash *= 1099511628211;
    }
    std::ostringstream oss = {};
    oss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return oss.str();
}

DeduplicationResult VoiceAudioStorage::checkDuplicate(
    const std::vector<uint8_t>& audio_data) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    DeduplicationResult result;
    result.content_hash = computeHash(audio_data);
    auto it = hash_to_id_.find(result.content_hash);
    if (it != hash_to_id_.end()) {
        result.is_duplicate = true;
        result.existing_record_id = it->second;
        result.bytes_saved = audio_data.size();
    }
    return result;
}

AudioFormat VoiceAudioStorage::detectFormat(const std::vector<uint8_t>& data) const {
    AudioFormat fmt;
    fmt.size_bytes = data.size();
    if (static_cast<int>(data.size()) >= 4) {
        // WAV: "RIFF"
        if (data[0]=='R' && data[1]=='I' && data[2]=='F' && data[3]=='F') {
            fmt.codec = "wav";
            return fmt;
        }
        // OGG: "OggS"
        if (data[0]=='O' && data[1]=='g' && data[2]=='g' && data[3]=='S') {
            // Check for OpusHead signature (8 bytes) in the first OGG page
            // Typical offset is 28 bytes into the OGG stream.
            static constexpr size_t OGG_OPUS_SIGNATURE_OFFSET = 28;
            if (static_cast<int>(data.size()) >= OGG_OPUS_SIGNATURE_OFFSET + 8) {
                bool is_opus = (data[OGG_OPUS_SIGNATURE_OFFSET  ]=='O' &&
                                data[OGG_OPUS_SIGNATURE_OFFSET+1]=='p' &&
                                data[OGG_OPUS_SIGNATURE_OFFSET+2]=='u' &&
                                data[OGG_OPUS_SIGNATURE_OFFSET+3]=='s' &&
                                data[OGG_OPUS_SIGNATURE_OFFSET+4]=='H' &&
                                data[OGG_OPUS_SIGNATURE_OFFSET+5]=='e' &&
                                data[OGG_OPUS_SIGNATURE_OFFSET+6]=='a' &&
                                data[OGG_OPUS_SIGNATURE_OFFSET+7]=='d');
                fmt.codec = is_opus ? "opus" : "ogg";
            } else {
                fmt.codec = "ogg";
            }
            return fmt;
        }
        // MP3: sync word 0xFF 0xFB or 0xFF 0xF3 or 0xFF 0xFA
        if ((data[0] == 0xFF && (data[1] == 0xFB || data[1] == 0xF3 || data[1] == 0xFA))) {
            fmt.codec = "mp3";
            return fmt;
        }
        // AAC ADTS: 0xFF 0xF1 or 0xFF 0xF9
        if ((data[0] == 0xFF && (data[1] == 0xF1 || data[1] == 0xF9))) {
            fmt.codec = "aac";
            return fmt;
        }
    }
    fmt.codec = "pcm";
    return fmt;
}

std::string VoiceAudioStorage::store(
    const std::vector<uint8_t>& audio_data,
    const AudioFormat& format,
    const std::string& transcript,
    const json& metadata)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::string hash = computeHash(audio_data);

    // Deduplication check
    auto dup_it = hash_to_id_.find(hash);
    if (dup_it != hash_to_id_.end()) {
        // Return existing ID – mark the lookup record
        const std::string& orig_id = dup_it->second;
        return orig_id;
    }

    std::string id = generateRecordId();
    AudioStorageRecord rec;
    rec.record_id = id;
    rec.content_hash = hash;
    rec.tier = StorageTier::HOT;
    rec.format = format;
    rec.format.size_bytes = audio_data.size();
    if (rec.format.codec.empty()) {
        rec.format = detectFormat(audio_data);
        rec.format.size_bytes = audio_data.size();
    }
    rec.transcript = transcript;
    rec.metadata = metadata;
    rec.created_at_ms = nowMs();
    rec.last_accessed_ms = rec.created_at_ms;

    if (enc_config_.enabled && !enc_config_.key_id.empty()) {
        rec.encrypted = true;
        rec.encryption_key_id = enc_config_.key_id;
    }

    records_.emplace(id, std::move(rec));
    data_.emplace(id, audio_data);
    hash_to_id_.emplace(hash, id);
    return id;
}

std::optional<std::vector<uint8_t>> VoiceAudioStorage::retrieve(
    const std::string& record_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto dit = data_.find(record_id);
    if (dit == data_.end()) {
      return std::nullopt;
    }

    auto rit = records_.find(record_id);
    if (rit != records_.end()) {
        rit->second.access_count++;
        rit->second.last_accessed_ms = nowMs();
    }
    return dit->second;
}

std::optional<AudioStorageRecord> VoiceAudioStorage::getRecord(
    const std::string& record_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(record_id);
    if (it == records_.end()) {
      return std::nullopt;
    }
    return it->second;
}

bool VoiceAudioStorage::deleteRecord(const std::string& record_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto rit = records_.find(record_id);
    if (rit == records_.end()) {
      return false;
    }

    // Remove from hash index
    hash_to_id_.erase(rit->second.content_hash);
    records_.erase(rit);
    data_.erase(record_id);
    return true;
}

std::vector<AudioStorageRecord> VoiceAudioStorage::listRecords(
    StorageTier tier_filter, size_t limit) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AudioStorageRecord> result = {};

    for (const auto& [id, rec] : records_) {
        if (static_cast<int>(result.size()) >= limit) {
          break;
        }
        if (rec.tier == tier_filter) {
            result.push_back(rec);
        }
    }
    return result;
}

std::vector<AudioStorageRecord> VoiceAudioStorage::searchTranscripts(
    const std::string& query, size_t limit) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AudioStorageRecord> result = {};

    if (query.empty()) {
      return result;
    }

    // Build lowercase version of query for case-insensitive matching
    std::string lower_query = query;
    for (auto& c : lower_query) {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    for (const auto& [id, rec] : records_) {
        if (static_cast<int>(result.size()) >= limit) {
          break;
        }
        if (rec.transcript.empty()) {
          continue;
        }
        std::string lower_transcript = rec.transcript;
        for (auto& c : lower_transcript) {
          c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (lower_transcript.find(lower_query) != std::string::npos) {
            result.push_back(rec);
        }
    }
    return result;
}

StorageTier VoiceAudioStorage::computeTier(const AudioStorageRecord& record) const {
    if (!policy_.enable_auto_tier) {
      return record.tier;
    }
    int64_t now = nowMs();
    int64_t age = now - record.last_accessed_ms;
    if (age >= policy_.cold_to_delete_after_ms) {
      return StorageTier::DELETED;
    }
    if (age >= policy_.warm_to_cold_after_ms) {
      return StorageTier::COLD;
    }
    if (age >= policy_.hot_to_warm_after_ms) {
      return StorageTier::WARM;
    }
    return StorageTier::HOT;
}

size_t VoiceAudioStorage::applyTierPolicy() {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t moved = 0;
    for (auto& [id, rec] : records_) {
        StorageTier new_tier = computeTier(rec);
        if (new_tier != rec.tier) {
            rec.tier = new_tier;
            ++moved;
        }
    }
    return moved;
}

bool VoiceAudioStorage::promoteTier(const std::string& record_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(record_id);
    if (it == records_.end()) {
      return false;
    }
    auto& tier = it->second.tier;
    if (tier == StorageTier::COLD)    { tier = StorageTier::WARM; return true; }
    if (tier == StorageTier::WARM)    { tier = StorageTier::HOT;  return true; }
    if (tier == StorageTier::DELETED) { tier = StorageTier::COLD; return true; }
    return false; // already HOT
}

bool VoiceAudioStorage::demoteTier(const std::string& record_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(record_id);
    if (it == records_.end()) {
      return false;
    }
    auto& tier = it->second.tier;
    if (tier == StorageTier::HOT)  { tier = StorageTier::WARM;    return true; }
    if (tier == StorageTier::WARM) { tier = StorageTier::COLD;    return true; }
    if (tier == StorageTier::COLD) { tier = StorageTier::DELETED; return true; }
    return false; // already DELETED
}

bool VoiceAudioStorage::markEncrypted(
    const std::string& record_id, const std::string& key_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(record_id);
    if (it == records_.end()) {
      return false;
    }
    it->second.encrypted = true;
    it->second.encryption_key_id = key_id;
    return true;
}

bool VoiceAudioStorage::isEncrypted(const std::string& record_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(record_id);
    if (it == records_.end()) {
      return false;
    }
    return it->second.encrypted;
}

StorageStats VoiceAudioStorage::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    StorageStats stats;
    stats.total_records = records_.size();
    for (const auto& [id, rec] : records_) {
        stats.total_bytes += rec.format.size_bytes;
        if (rec.tier == StorageTier::HOT) {
          ++stats.hot_records;
        }
        if (rec.tier == StorageTier::WARM) {
          ++stats.warm_records;
        }
        if (rec.tier == StorageTier::COLD) {
          ++stats.cold_records;
        }
        if (rec.encrypted) {
          ++stats.encryption_enabled_records;
        }
        if (rec.is_duplicate) {
            ++stats.deduplication_hits;
            stats.deduplicated_bytes += rec.format.size_bytes;
        }
    }
    return stats;
}

}} // namespace themis::voice
