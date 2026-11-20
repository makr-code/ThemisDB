/**
 * @file object_block_manifest.cpp
 * @brief Object-version block construction and integrity validation.
 */

#include "storage/object_block_manifest.h"

#include "utils/error_registry.h"
#include "utils/expected.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <sstream>
#include <utility>

namespace themis::storage {
namespace {

using DigestContext = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

[[nodiscard]] Result<std::string> sha256Hex(std::span<const std::uint8_t> bytes) {
    DigestContext context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!context || EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1) {
        return Err<std::string>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                                "unable to initialize SHA-256");
    }

    if (!bytes.empty() && EVP_DigestUpdate(context.get(), bytes.data(), bytes.size()) != 1) {
        return Err<std::string>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                                "unable to hash object block");
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_size = 0;
    if (EVP_DigestFinal_ex(context.get(), digest.data(), &digest_size) != 1 ||
        digest_size != 32) {
        return Err<std::string>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                                "unable to finalize SHA-256");
    }

    std::ostringstream result;
    result << std::hex;
    for (unsigned int index = 0; index < digest_size; ++index) {
        result.width(2);
        result.fill('0');
        result << static_cast<unsigned int>(digest[index]);
    }
    return Ok(result.str());
}

[[nodiscard]] bool isLowercaseSha256(std::string_view digest) {
    if (digest.size() != 64) {
        return false;
    }
    for (const char character : digest) {
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f'))) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] Result<void> validateManifest(const ObjectVersionBlockManifest& manifest) {
    if (manifest.format_version != OBJECT_BLOCK_MANIFEST_FORMAT_VERSION ||
        manifest.object_id.empty() || manifest.version_id.empty()) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                       "object block manifest has an unsupported version or missing identity");
    }

    std::uint64_t calculated_size = 0;
    for (std::size_t index = 0; index < manifest.blocks.size(); ++index) {
        const auto& block = manifest.blocks[index];
        if (block.ordinal != index || block.shard_id.empty() ||
            block.length_bytes == 0 || block.length_bytes > OBJECT_BLOCK_MAX_BYTES ||
            !isLowercaseSha256(block.sha256)) {
            return ErrVoid(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                           "object block manifest contains invalid block metadata");
        }
        if (calculated_size > std::numeric_limits<std::uint64_t>::max() - block.length_bytes) {
            return ErrVoid(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                           "object block manifest size overflows");
        }
        calculated_size += block.length_bytes;
    }

    if (calculated_size != manifest.total_size_bytes) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                       "object block manifest total size does not match its blocks");
    }
    return OkVoid();
}

} // namespace

Result<ObjectVersionBlockSet> buildObjectVersionBlockSet(
    std::string_view object_id,
    std::string_view version_id,
    std::uint64_t placement_map_epoch,
    std::span<const std::uint8_t> bytes,
    std::span<const std::string> block_shard_ids) {
    if (object_id.empty() || version_id.empty()) {
        return Err<ObjectVersionBlockSet>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                          "object and version identifiers must not be empty");
    }

    const auto block_count =
        bytes.size() / OBJECT_BLOCK_MAX_BYTES +
        static_cast<std::size_t>(bytes.size() % OBJECT_BLOCK_MAX_BYTES != 0);
    if (block_shard_ids.size() != block_count) {
        return Err<ObjectVersionBlockSet>(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "exactly one shard assignment is required for each object block");
    }

    ObjectVersionBlockSet block_set;
    block_set.manifest.object_id = object_id;
    block_set.manifest.version_id = version_id;
    block_set.manifest.placement_map_epoch = placement_map_epoch;
    block_set.manifest.total_size_bytes = bytes.size();
    block_set.manifest.blocks.reserve(block_count);
    block_set.payloads.reserve(block_count);

    std::size_t offset = 0;
    for (std::size_t index = 0; index < block_count; ++index) {
        if (block_shard_ids[index].empty()) {
            return Err<ObjectVersionBlockSet>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                              "object block shard assignments must not be empty");
        }

        const auto length = std::min(OBJECT_BLOCK_MAX_BYTES, bytes.size() - offset);
        const auto block_bytes = bytes.subspan(offset, length);
        auto digest = sha256Hex(block_bytes);
        if (!digest.has_value()) {
            return Err<ObjectVersionBlockSet>(digest.error().code(), digest.error().message());
        }

        ObjectBlockDescriptor descriptor;
        descriptor.ordinal = static_cast<std::uint64_t>(index);
        descriptor.shard_id = block_shard_ids[index];
        descriptor.length_bytes = static_cast<std::uint32_t>(length);
        descriptor.sha256 = std::move(digest.value());
        block_set.manifest.blocks.push_back(std::move(descriptor));

        ObjectBlockPayload payload;
        payload.ordinal = static_cast<std::uint64_t>(index);
        payload.bytes.assign(block_bytes.begin(), block_bytes.end());
        block_set.payloads.push_back(std::move(payload));
        offset += length;
    }

    return Ok(std::move(block_set));
}

Result<std::vector<std::uint8_t>> assembleObjectVersion(
    const ObjectVersionBlockManifest& manifest,
    std::span<const ObjectBlockPayload> payloads) {
    auto manifest_result = validateManifest(manifest);
    if (!manifest_result.has_value()) {
        return Err<std::vector<std::uint8_t>>(manifest_result.error().code(),
                                              manifest_result.error().message());
    }
    if (payloads.size() != manifest.blocks.size()) {
        return Err<std::vector<std::uint8_t>>(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                                              "object version is missing or has extra blocks");
    }

    std::vector<const ObjectBlockPayload*> ordered_payloads(manifest.blocks.size(), nullptr);
    for (const auto& payload : payloads) {
        if (payload.ordinal >= ordered_payloads.size()) {
            return Err<std::vector<std::uint8_t>>(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                                                  "object block ordinal is out of range");
        }
        auto& slot = ordered_payloads[static_cast<std::size_t>(payload.ordinal)];
        if (slot != nullptr) {
            return Err<std::vector<std::uint8_t>>(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                                                  "object version contains a duplicate block");
        }
        slot = &payload;
    }

    std::vector<std::uint8_t> assembled;
    for (std::size_t index = 0; index < ordered_payloads.size(); ++index) {
        const auto* payload = ordered_payloads[index];
        if (payload == nullptr) {
            return Err<std::vector<std::uint8_t>>(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                                                  "object version is missing a block");
        }
        const auto& descriptor = manifest.blocks[index];
        if (payload->bytes.size() != descriptor.length_bytes) {
            return Err<std::vector<std::uint8_t>>(errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                                                  "object block length does not match its manifest");
        }
        auto digest = sha256Hex(payload->bytes);
        if (!digest.has_value()) {
            return Err<std::vector<std::uint8_t>>(digest.error().code(), digest.error().message());
        }
        if (digest.value() != descriptor.sha256) {
            return Err<std::vector<std::uint8_t>>(errors::ErrorCode::ERR_BACKUP_CHECKSUM_MISMATCH,
                                                  "object block SHA-256 does not match its manifest");
        }
        assembled.insert(assembled.end(), payload->bytes.begin(), payload->bytes.end());
    }
    return Ok(std::move(assembled));
}

} // namespace themis::storage
