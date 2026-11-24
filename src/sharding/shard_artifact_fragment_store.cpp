/**
 * @file shard_artifact_fragment_store.cpp
 * @brief RocksDB-backed immutable tensor artifact fragment storage.
 */
#include "sharding/shard_artifact_fragment_store.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace themis::sharding {
namespace {

constexpr std::string_view kKeyspace = "themis/shard-artifact-fragment/v1/";
constexpr std::string_view kMetadataSuffix = "/metadata";
constexpr std::string_view kPayloadSuffix = "/payload";
constexpr std::size_t kMaxIdentifierBytes = 1024;
constexpr uint64_t kMaxFragmentBytes = 1024ULL * 1024ULL * 1024ULL;
constexpr std::size_t kMetadataBytes = 4 + sizeof(uint64_t) + 64;

struct FragmentMetadata {
    uint64_t size = 0;
    std::string sha256;
};

std::string encodeKeyComponent(std::string_view value) {
    constexpr char hex[] = "0123456789abcdef";
    std::string encoded;
    encoded.reserve(value.size() * 2);
    for (const unsigned char byte : value) {
        encoded.push_back(hex[byte >> 4]);
        encoded.push_back(hex[byte & 0x0f]);
    }
    return encoded;
}

std::string sha256Hex(const std::vector<uint8_t>& bytes) {
    static constexpr unsigned char empty_input = 0;
    const auto* input = bytes.empty() ? &empty_input : bytes.data();
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_size = 0;
    if (EVP_Digest(input, bytes.size(), digest.data(), &digest_size, EVP_sha256(), nullptr) != 1 ||
        digest_size != 32) {
        throw std::runtime_error("Unable to compute tensor fragment SHA-256");
    }

    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(64);
    for (const auto byte : std::span<const unsigned char>(digest.data(), digest_size)) {
        result.push_back(hex[byte >> 4]);
        result.push_back(hex[byte & 0x0f]);
    }
    return result;
}

bool isLowercaseSha256(std::string_view digest) noexcept {
    return digest.size() == 64 &&
           std::all_of(digest.begin(), digest.end(), [](unsigned char ch) {
               return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
           });
}

std::vector<uint8_t> encodeMetadata(uint64_t size, std::string_view digest) {
    std::vector<uint8_t> metadata;
    metadata.reserve(kMetadataBytes);
    metadata.insert(metadata.end(), {'S', 'A', 'F', '1'});
    for (int shift = 56; shift >= 0; shift -= 8) {
        metadata.push_back(static_cast<uint8_t>((size >> shift) & 0xff));
    }
    metadata.insert(metadata.end(), digest.begin(), digest.end());
    return metadata;
}

std::optional<FragmentMetadata> decodeMetadata(const std::vector<uint8_t>& bytes) {
    if (bytes.size() != kMetadataBytes ||
        bytes[0] != 'S' || bytes[1] != 'A' || bytes[2] != 'F' || bytes[3] != '1') {
        return std::nullopt;
    }

    FragmentMetadata metadata;
    for (std::size_t index = 4; index < 12; ++index) {
        metadata.size = (metadata.size << 8) | bytes[index];
    }
    metadata.sha256.assign(bytes.begin() + 12, bytes.end());
    if (!isLowercaseSha256(metadata.sha256)) {
        return std::nullopt;
    }
    return metadata;
}

} // namespace

ShardArtifactFragmentStore::ShardArtifactFragmentStore(
    std::shared_ptr<RocksDBWrapper> storage,
    uint64_t max_fragment_bytes)
    : storage_(std::move(storage)),
      max_fragment_bytes_(max_fragment_bytes) {
    if (!storage_ || !storage_->isOpen()) {
        throw std::invalid_argument("ShardArtifactFragmentStore requires an open RocksDB instance");
    }
    if (max_fragment_bytes_ == 0 || max_fragment_bytes_ > kMaxFragmentBytes) {
        throw std::invalid_argument("Tensor fragment storage limit must be in [1, 1 GiB]");
    }
}

bool ShardArtifactFragmentStore::validIdentifier(std::string_view value) noexcept {
    return !value.empty() && value.size() <= kMaxIdentifierBytes;
}

std::string ShardArtifactFragmentStore::keyPrefix(
    std::string_view principal_id,
    std::string_view shard_id,
    std::string_view artifact_id) {
    std::string prefix(kKeyspace);
    prefix += encodeKeyComponent(principal_id);
    prefix.push_back('/');
    prefix += encodeKeyComponent(shard_id);
    prefix.push_back('/');
    prefix += encodeKeyComponent(artifact_id);
    return prefix;
}

ShardArtifactFragmentStore::PutResult ShardArtifactFragmentStore::put(
    std::string_view principal_id,
    std::string_view shard_id,
    std::string_view artifact_id,
    const std::vector<uint8_t>& payload) {
    if (!validIdentifier(principal_id) || !validIdentifier(shard_id) ||
        !validIdentifier(artifact_id) || payload.size() > max_fragment_bytes_) {
        return PutResult::InvalidInput;
    }

    const auto digest = sha256Hex(payload);
    const auto prefix = keyPrefix(principal_id, shard_id, artifact_id);
    const auto metadata_key = prefix + std::string(kMetadataSuffix);
    const auto payload_key = prefix + std::string(kPayloadSuffix);
    auto transaction = storage_->beginDurableTransaction(
        RocksDBWrapper::TransactionIsolationLevel::Snapshot);
    if (!transaction || !transaction->isActive() ||
        !transaction->getForUpdate(metadata_key)) {
        if (transaction) {
            transaction->rollback();
        }
        return PutResult::StorageError;
    }

    const auto metadata_read = transaction->getChecked(metadata_key);
    if (!metadata_read) {
        transaction->rollback();
        return PutResult::StorageError;
    }
    if (const auto& existing_metadata_bytes = *metadata_read) {
        const auto existing_metadata = decodeMetadata(*existing_metadata_bytes);
        const auto payload_read = transaction->getChecked(payload_key);
        if (!payload_read || !existing_metadata || !*payload_read ||
            existing_metadata->size != (*payload_read)->size() ||
            existing_metadata->size > max_fragment_bytes_ ||
            sha256Hex(**payload_read) != existing_metadata->sha256) {
            transaction->rollback();
            return PutResult::StorageError;
        }
        if (**payload_read != payload) {
            transaction->rollback();
            return PutResult::Conflict;
        }
        if (!transaction->put(payload_key, payload) ||
            !transaction->put(metadata_key, *existing_metadata_bytes) ||
            !transaction->commit()) {
            transaction->rollback();
            return PutResult::StorageError;
        }
        return PutResult::Unchanged;
    }

    if (!transaction->getForUpdate(payload_key)) {
        transaction->rollback();
        return PutResult::StorageError;
    }
    const auto payload_read = transaction->getChecked(payload_key);
    if (!payload_read || *payload_read) {
        transaction->rollback();
        return PutResult::StorageError;
    }

    const auto metadata = encodeMetadata(static_cast<uint64_t>(payload.size()), digest);
    if (!transaction->put(payload_key, payload) ||
        !transaction->put(metadata_key, metadata) ||
        !transaction->commit()) {
        transaction->rollback();
        return PutResult::StorageError;
    }
    return PutResult::Stored;
}

std::optional<TensorFragmentPayload> ShardArtifactFragmentStore::read(
    std::string_view principal_id,
    std::string_view shard_id,
    std::string_view artifact_id,
    uint64_t max_bytes) const {
    if (!validIdentifier(principal_id) || !validIdentifier(shard_id) ||
        !validIdentifier(artifact_id)) {
        throw std::invalid_argument("Tensor fragment identifiers must be non-empty and bounded");
    }
    if (max_bytes == 0 || max_bytes > max_fragment_bytes_) {
        throw std::invalid_argument("Tensor fragment read limit is outside the configured range");
    }
    if (!storage_ || !storage_->isOpen()) {
        throw std::runtime_error("Tensor fragment database is not open");
    }

    const auto prefix = keyPrefix(principal_id, shard_id, artifact_id);
    const auto metadata_key = prefix + std::string(kMetadataSuffix);
    const auto payload_key = prefix + std::string(kPayloadSuffix);
    auto transaction = storage_->beginTransaction(
        RocksDBWrapper::TransactionIsolationLevel::Snapshot);
    if (!transaction || !transaction->isActive()) {
        throw std::runtime_error("Unable to begin tensor fragment read transaction");
    }

    const auto metadata_read = transaction->getChecked(metadata_key);
    if (!metadata_read) {
        transaction->rollback();
        throw std::runtime_error("Unable to read tensor fragment metadata: " +
                                 metadata_read.error().message());
    }
    const auto& metadata_bytes = *metadata_read;
    if (!metadata_bytes) {
        transaction->rollback();
        return std::nullopt;
    }
    const auto metadata = decodeMetadata(*metadata_bytes);
    if (!metadata || metadata->size > max_fragment_bytes_) {
        transaction->rollback();
        throw std::runtime_error("Tensor fragment metadata is corrupt or exceeds the store limit");
    }
    if (metadata->size > max_bytes) {
        transaction->rollback();
        return std::nullopt;
    }

    auto payload_read = transaction->getChecked(payload_key);
    transaction->rollback();
    if (!payload_read) {
        throw std::runtime_error("Unable to read tensor fragment payload: " +
                                 payload_read.error().message());
    }
    auto& payload = *payload_read;
    if (!payload || payload->size() != metadata->size ||
        sha256Hex(*payload) != metadata->sha256) {
        throw std::runtime_error("Tensor fragment payload is missing or failed SHA-256 verification");
    }
    return TensorFragmentPayload{std::move(*payload), metadata->sha256};
}

} // namespace themis::sharding
