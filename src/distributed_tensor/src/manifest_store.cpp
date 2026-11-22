/**
 * @file manifest_store.cpp
 * @brief Durable RocksDB-backed implementation of the advisory manifest registry.
 */

#include "../include/manifest_store.h"

#include "storage/rocksdb_wrapper.h"

#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include <rocksdb/utilities/transaction_db.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace themis {
namespace distributed_tensor {
namespace {

constexpr std::string_view kManifestPrefix = "distributed_tensor/manifest/";

std::string encodeHex(std::string_view value) {
    constexpr char digits[] = "0123456789abcdef";
    std::string encoded;
    encoded.reserve(value.size() * 2);
    for (const unsigned char byte : value) {
        encoded.push_back(digits[byte >> 4]);
        encoded.push_back(digits[byte & 0x0f]);
    }
    return encoded;
}

std::string tensorPrefix(std::string_view tensor_name) {
    return std::string(kManifestPrefix) + encodeHex(tensor_name) + "/";
}

std::string storageKey(const detail::ArtifactManifest& manifest) {
    constexpr char digits[] = "0123456789abcdef";
    std::array<char, 8> shard_id{};
    for (std::size_t i = 0; i < shard_id.size(); ++i) {
        const auto shift = static_cast<unsigned>((shard_id.size() - i - 1) * 4);
        shard_id[i] = digits[(manifest.shard_id >> shift) & 0x0f];
    }

    return tensorPrefix(manifest.tensor_name) +
           std::string(shard_id.data(), shard_id.size()) + "/" +
           encodeHex(manifest.artifact_id);
}

std::optional<detail::ArtifactManifest> parseStoredManifest(
    std::string_view serialized) {
    try {
        const auto encoded = nlohmann::json::parse(serialized);
        constexpr std::array<std::string_view, 13> required_fields = {
            "artifact_id", "tensor_name", "kind", "shard_id", "version",
            "created_at_unix_nanos", "integrity", "artifact_class",
            "truth_semantic", "lifecycle_state",
            "shard_placements", "backup_shard_placements", "replication_factor"};
        if (!encoded.is_object() ||
            !std::ranges::all_of(required_fields, [&encoded](std::string_view field) {
                return encoded.contains(std::string(field));
            }) ||
            !encoded["integrity"].is_object() ||
            !encoded["integrity"].contains("crc32") ||
            !encoded["integrity"].contains("payload_bytes")) {
            return std::nullopt;
        }
        return detail::ArtifactManifest::fromJSON(encoded.dump());
    } catch (...) {
        return std::nullopt;
    }
}

template <typename Visitor>
bool visitManifests(RocksDBWrapper& storage, std::string_view prefix,
                    Visitor&& visitor) {
    try {
        auto* db = storage.getRawDB();
        if (db == nullptr) {
            return false;
        }

        rocksdb::ReadOptions read_options;
        read_options.verify_checksums = true;
        std::unique_ptr<rocksdb::Iterator> iterator(db->NewIterator(read_options));
        if (!iterator) {
            return false;
        }

        iterator->Seek(rocksdb::Slice(prefix.data(), prefix.size()));
        while (iterator->Valid()) {
            const auto key = iterator->key().ToString();
            if (!std::string_view(key).starts_with(prefix)) {
                break;
            }

            const auto value = iterator->value().ToString();
            auto manifest = parseStoredManifest(value);
            if (!manifest || !manifest->validate() || storageKey(*manifest) != key ||
                !visitor(key, *manifest)) {
                return false;
            }
            iterator->Next();
        }
        return iterator->status().ok();
    } catch (...) {
        return false;
    }
}

rocksdb::Status writeBatch(RocksDBWrapper& storage, rocksdb::WriteBatch& batch) {
    auto* db = storage.getRawDB();
    if (db == nullptr) {
        return rocksdb::Status::IOError("manifest database is closed");
    }

    rocksdb::WriteOptions write_options;
    write_options.sync = true;
    write_options.disableWAL = false;
    return db->Write(write_options, &batch);
}

} // namespace

ManifestStore::ManifestStore(std::string db_path,
                             observability::MetricsCollector* metrics)
    : metrics_(metrics) {
    if (db_path.empty()) {
        throw std::invalid_argument("ManifestStore requires a non-empty RocksDB path");
    }

    RocksDBWrapper::Config config;
    config.db_path = std::move(db_path);
    config.enable_wal = true;
    config.wal_enabled = true;
    config.disable_wal_for_benchmark = false;
    config.force_sync_on_write = true;
    config.verify_checksums_on_read = true;

    storage_ = std::make_unique<RocksDBWrapper>(config);
    if (!storage_->open()) {
        throw std::runtime_error("ManifestStore failed to open RocksDB at " +
                                 config.db_path);
    }

    const bool records_valid = visitManifests(
        *storage_, kManifestPrefix,
        [](const std::string&, const detail::ArtifactManifest&) { return true; });
    if (!records_valid) {
        throw std::runtime_error(
            "ManifestStore could not read or validate persisted manifest data");
    }
}

ManifestStore::~ManifestStore() = default;

bool ManifestStore::store(const detail::ArtifactManifest& manifest) {
    if (!manifest.validate()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto key = storageKey(manifest);
    std::string previous_value;
    const rocksdb::Slice rocks_key(key);
    auto* db = storage_->getRawDB();
    if (db == nullptr) {
        return false;
    }

    const auto read_status = db->Get(rocksdb::ReadOptions(), rocks_key, &previous_value);
    if (read_status.ok()) {
        auto previous = parseStoredManifest(previous_value);
        if (!previous || !previous->validate() || storageKey(*previous) != key ||
            manifest.version <= previous->version) {
            return false;
        }
    } else if (!read_status.IsNotFound()) {
        return false;
    }

    rocksdb::WriteBatch batch;
    const auto serialized = manifest.toJSON();
    batch.Put(rocks_key, rocksdb::Slice(serialized));
    if (!writeBatch(*storage_, batch).ok()) {
        return false;
    }

    if (metrics_) {
        metrics_->addCounter("tensor_delta_log_entries_total", 1,
                             {{"tensor_name", manifest.tensor_name}});
    }
    return true;
}

std::size_t ManifestStore::evict(const std::string& artifact_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> keys;
    const bool valid = visitManifests(
        *storage_, kManifestPrefix,
        [&keys, &artifact_id](const std::string& key,
                              const detail::ArtifactManifest& manifest) {
            if (manifest.artifact_id == artifact_id) {
                keys.push_back(key);
            }
            return true;
        });
    if (!valid || keys.empty()) {
        return 0;
    }

    rocksdb::WriteBatch batch;
    for (const auto& key : keys) {
        batch.Delete(rocksdb::Slice(key));
    }
    return writeBatch(*storage_, batch).ok() ? keys.size() : 0;
}

std::size_t ManifestStore::evictStale(double max_age_s) {
    const auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> keys;
    const bool valid = visitManifests(
        *storage_, kManifestPrefix,
        [&keys, max_age_s, now](const std::string& key,
                                const detail::ArtifactManifest& manifest) {
            if (!manifest.isFresh(max_age_s, now)) {
                keys.push_back(key);
            }
            return true;
        });
    if (!valid || keys.empty()) {
        return 0;
    }

    rocksdb::WriteBatch batch;
    for (const auto& key : keys) {
        batch.Delete(rocksdb::Slice(key));
    }
    return writeBatch(*storage_, batch).ok() ? keys.size() : 0;
}

std::optional<detail::ArtifactManifest> ManifestStore::get(
    const std::string& tensor_name, uint32_t shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::optional<detail::ArtifactManifest> best;
    const bool valid = visitManifests(
        *storage_, tensorPrefix(tensor_name),
        [&best, shard_id](const std::string&,
                          const detail::ArtifactManifest& manifest) {
            if (manifest.shard_id == shard_id &&
                (!best || manifest.version > best->version)) {
                best = manifest;
            }
            return true;
        });
    return valid ? best : std::nullopt;
}

std::vector<detail::ArtifactManifest> ManifestStore::list(
    const std::string& tensor_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<detail::ArtifactManifest> result;
    const bool valid = visitManifests(
        *storage_, tensorPrefix(tensor_name),
        [&result](const std::string&,
                  const detail::ArtifactManifest& manifest) {
            result.push_back(manifest);
            return true;
        });
    if (!valid) {
        return {};
    }

    std::sort(result.begin(), result.end(),
              [](const detail::ArtifactManifest& a,
                 const detail::ArtifactManifest& b) {
                  if (a.shard_id != b.shard_id) {
                      return a.shard_id < b.shard_id;
                  }
                  return a.version > b.version;
              });
    return result;
}

std::size_t ManifestStore::size() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t count = 0;
    const bool valid = visitManifests(
        *storage_, kManifestPrefix,
        [&count](const std::string&, const detail::ArtifactManifest&) {
            ++count;
            return true;
        });
    return valid ? count : 0;
}

void ManifestStore::refreshFreshnessMetrics() const {
    if (!metrics_) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    std::unordered_map<std::string, double> oldest_age;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const bool valid = visitManifests(
            *storage_, kManifestPrefix,
            [&oldest_age, now](const std::string&,
                               const detail::ArtifactManifest& manifest) {
                const double age = manifest.freshnessAgeSeconds(now);
                auto [it, inserted] = oldest_age.emplace(manifest.tensor_name, age);
                if (!inserted && age > it->second) {
                    it->second = age;
                }
                return true;
            });
        if (!valid) {
            return;
        }
    }

    for (const auto& [name, age] : oldest_age) {
        metrics_->setGauge("tensor_freshness_age_seconds", age,
                           {{"tensor_name", name}});
    }
}

} // namespace distributed_tensor
} // namespace themis
