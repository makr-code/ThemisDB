/**
 * @file lora_package.cc
 * @brief Implementation of LoRAPackage, PortableAdapterProduct, and
 *        LoRAManifestStore artifact classes (Phase 3 manifest & serialization).
 */

#include "lora_package.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <cmath>
#include <set>
#include <type_traits>
#include <utility>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <yaml-cpp/yaml.h>
#include "plugins/pinned_ed25519_verifier.h"
#include "utils/rocksdb_open_compat.h"

namespace themis {
namespace retrieval {

// ============================================================================
// Internal: portable SHA-256 (FIPS 180-4)
// No third-party crypto dependency; suitable for manifest hashing.
// ============================================================================

namespace {

constexpr const char* kPackagePrefix = "retrieval:package:";
constexpr const char* kProductPrefix = "retrieval:product:";

std::string artifactKey(const char* prefix, const std::string& id) {
    return std::string(prefix) + id;
}

rocksdb::WriteOptions durableWriteOptions() {
    rocksdb::WriteOptions options;
    options.sync = true;
    return options;
}

bool validQualityMetrics(
    const themis::rag::quality::AggregatedMetrics& candidate,
    const themis::rag::quality::AggregatedMetrics& baseline) {
    if (candidate.sample_count == 0 || baseline.sample_count == 0) {
        return false;
    }
    const double values[] = {
        candidate.mean_recall_10, candidate.mean_ndcg_10, candidate.mean_mrr,
        baseline.mean_recall_10, baseline.mean_ndcg_10, baseline.mean_mrr};
    for (const double value : values) {
        if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
            return false;
        }
    }
    // The existing controller calculates relative changes. A zero baseline
    // would otherwise produce NaN/Inf and can be mistakenly treated as allow.
    return baseline.mean_recall_10 > 0.0 &&
           baseline.mean_ndcg_10 > 0.0 && baseline.mean_mrr > 0.0;
}

bool verifyPersistedLifecycleState(
    const std::string& artifact_type,
    const json& artifact,
    const std::string& status,
    const std::string& initial_status,
    const LoRAManifestStore::PersistedLifecycleVerifier& verifier) {
    if (status == initial_status) {
        return true;
    }
    if (!verifier) {
        return false;
    }
    try {
        return verifier(artifact_type, artifact, status);
    } catch (...) {
        return false;
    }
}

bool authorizeLifecycleTransition(
    const std::string& artifact_type,
    const json& artifact,
    const std::string& current_status,
    const std::string& requested_status,
    const LoRAManifestStore::LifecycleTransitionAuthorizer& authorizer) {
    if (!authorizer) {
        return false;
    }
    try {
        return authorizer(
            artifact_type, artifact, current_status, requested_status);
    } catch (...) {
        return false;
    }
}

bool isLegalPackageTransition(
    LoRAPackageStatus current, LoRAPackageStatus requested) {
    switch (current) {
        case LoRAPackageStatus::DRAFT:
            return requested == LoRAPackageStatus::VALIDATED ||
                   requested == LoRAPackageStatus::DEPRECATED ||
                   requested == LoRAPackageStatus::REVOKED;
        case LoRAPackageStatus::VALIDATED:
            return requested == LoRAPackageStatus::DEPRECATED ||
                   requested == LoRAPackageStatus::REVOKED;
        case LoRAPackageStatus::DEPRECATED:
            return requested == LoRAPackageStatus::REVOKED;
        case LoRAPackageStatus::REVOKED:
            return false;
    }
    return false;
}

bool isLegalProductTransition(
    AdapterProductStatus current, AdapterProductStatus requested) {
    switch (current) {
        case AdapterProductStatus::BUILDING:
            return requested == AdapterProductStatus::READY ||
                   requested == AdapterProductStatus::FAILED;
        case AdapterProductStatus::READY:
            return requested == AdapterProductStatus::RETIRED ||
                   requested == AdapterProductStatus::FAILED;
        case AdapterProductStatus::DEPLOYED:
            return requested == AdapterProductStatus::RETIRED;
        case AdapterProductStatus::RETIRED:
        case AdapterProductStatus::FAILED:
            return false;
    }
    return false;
}

std::string utcTimestampNow() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_MSC_VER)
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    std::ostringstream timestamp;
    timestamp << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return timestamp.str();
}

template <typename Artifact>
bool verifySignedArtifact(
    const Artifact& artifact,
    const std::vector<themis::plugins::PinnedKey>& pinned_keys,
    const LoRAManifestStore::SignatureVerifier& verifier) {
    const auto& integrity = artifact.integrity;
    if (integrity.manifest_hash.empty() || integrity.signature.empty() ||
        integrity.signature_algorithm != "Ed25519" ||
        integrity.signer_id.empty() || !verifier) {
        return false;
    }

    const bool has_valid_pin = std::any_of(
        pinned_keys.begin(), pinned_keys.end(), [&](const auto& key) {
            return key.active && key.fingerprint == integrity.signer_id &&
                   themis::plugins::pinned_ed25519::fingerprint(key.public_key) ==
                       key.fingerprint;
        });
    if (!has_valid_pin) {
        return false;
    }

    Artifact unsigned_artifact = artifact;
    unsigned_artifact.integrity.manifest_hash.clear();
    unsigned_artifact.integrity.signature.clear();
    unsigned_artifact.integrity.signature_algorithm.clear();
    unsigned_artifact.integrity.signer_id.clear();
    unsigned_artifact.integrity.signed_at.clear();
    if constexpr (std::is_same_v<Artifact, LoRAPackage>) {
        unsigned_artifact.status = LoRAPackageStatus::DRAFT;
        unsigned_artifact.updated_at.clear();
    } else {
        unsigned_artifact.status = AdapterProductStatus::BUILDING;
        unsigned_artifact.updated_at.clear();
        unsigned_artifact.deployed_at.clear();
    }
    if (IntegrityHelper::sha256Hex(unsigned_artifact.to_json().dump()) !=
        integrity.manifest_hash) {
        return false;
    }

    try {
        return verifier(integrity.manifest_hash, integrity.signature,
                        integrity.signer_id);
    } catch (const std::exception&) {
        return false;
    } catch (...) {
        return false;
    }
}

// --- SHA-256 constants -------------------------------------------------------
static constexpr std::array<uint32_t, 64> kK = {{
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
}};

inline uint32_t rotr32(uint32_t x, unsigned n) noexcept {
    return (x >> n) | (x << (32u - n));
}

struct Sha256State {
    uint32_t h[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
    };
    uint64_t bitlen = 0;
    uint8_t  buf[64]{};
    uint32_t buflen = 0;

    void processBlock(const uint8_t block[64]) noexcept {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(block[i * 4])     << 24u) |
                   (static_cast<uint32_t>(block[i * 4 + 1]) << 16u) |
                   (static_cast<uint32_t>(block[i * 4 + 2]) <<  8u) |
                    static_cast<uint32_t>(block[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr32(w[i-15], 7) ^ rotr32(w[i-15], 18) ^ (w[i-15] >> 3u);
            uint32_t s1 = rotr32(w[i-2],  17) ^ rotr32(w[i-2],  19) ^ (w[i-2] >> 10u);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            uint32_t S1  = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            uint32_t ch  = (e & f) ^ (~e & g);
            uint32_t tmp1 = hh + S1 + ch + kK[static_cast<size_t>(i)] + w[i];
            uint32_t S0  = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t tmp2 = S0 + maj;
            hh = g; g = f; f = e;
            e  = d + tmp1;
            d  = c; c = b; b = a;
            a  = tmp1 + tmp2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    void update(const uint8_t* data, size_t len) noexcept {
        bitlen += static_cast<uint64_t>(len) * 8u;
        size_t off = 0;
        if (buflen > 0) {
            size_t fill = std::min(static_cast<size_t>(64u - buflen), len);
            std::memcpy(buf + buflen, data, fill);
            buflen += static_cast<uint32_t>(fill);
            off    += fill;
            if (buflen == 64u) { processBlock(buf); buflen = 0; }
        }
        while (off + 64 <= len) {
            processBlock(data + off);
            off += 64;
        }
        if (off < len) {
            std::memcpy(buf, data + off, len - off);
            buflen = static_cast<uint32_t>(len - off);
        }
    }

    std::array<uint8_t, 32> finalise() noexcept {
        buf[buflen++] = 0x80u;
        if (buflen > 56) {
            while (buflen < 64) {
              buf[buflen++] = 0;
            }
            processBlock(buf);
            buflen = 0;
        }
        while (buflen < 56) {
          buf[buflen++] = 0;
        }
        for (int i = 7; i >= 0; --i) {
            buf[buflen++] = static_cast<uint8_t>((bitlen >> (static_cast<unsigned>(i) * 8u)) & 0xffu);
        }
        processBlock(buf);
        std::array<uint8_t, 32> digest{};
        for (int i = 0; i < 8; ++i) {
            digest[static_cast<size_t>(i) * 4 + 0] = static_cast<uint8_t>(h[i] >> 24u);
            digest[static_cast<size_t>(i) * 4 + 1] = static_cast<uint8_t>(h[i] >> 16u);
            digest[static_cast<size_t>(i) * 4 + 2] = static_cast<uint8_t>(h[i] >>  8u);
            digest[static_cast<size_t>(i) * 4 + 3] = static_cast<uint8_t>(h[i]        );
        }
        return digest;
    }
};

std::string bytesToHex(const std::array<uint8_t, 32>& bytes) {
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    for (auto b : bytes) {
        oss << std::setw(2) << static_cast<unsigned>(b);
    }
    return oss.str();
}

std::vector<uint8_t> decodeHexPublicKey(const std::string& encoded) {
    if (encoded.size() != 64) {
        return {};
    }
    auto decodeNibble = [](unsigned char c) -> int {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return -1;
    };
    std::vector<uint8_t> key;
    key.reserve(32);
    for (size_t i = 0; i < encoded.size(); i += 2) {
        const int high = decodeNibble(static_cast<unsigned char>(encoded[i]));
        const int low = decodeNibble(static_cast<unsigned char>(encoded[i + 1]));
        if (high < 0 || low < 0) {
            return {};
        }
        key.push_back(static_cast<uint8_t>((high << 4) | low));
    }
    return key;
}

} // anonymous namespace

LoRAManifestStore::RuntimeConfig
LoRAManifestStore::RuntimeConfig::loadFromYaml(const std::string& yaml_path) {
    if (yaml_path.empty()) {
        throw std::invalid_argument("retrieval YAML config path must not be empty");
    }

    RuntimeConfig result;
    YAML::Node document;
    try {
        document = YAML::LoadFile(yaml_path);
    } catch (const YAML::Exception& error) {
        throw std::runtime_error(
            std::string("failed to load retrieval YAML config: ") + error.what());
    }

    if (!document.IsMap() || document.size() != 1 ||
        !document["retrieval"].IsMap() || document["retrieval"].size() != 1 ||
        !document["retrieval"]["artifact_registry"].IsMap()) {
        throw std::invalid_argument(
            "retrieval YAML must contain only retrieval.artifact_registry");
    }
    const YAML::Node registry = document["retrieval"]["artifact_registry"];
    if (registry.size() != 2 || !registry["database_path"] ||
        !registry["pinned_keys"] || !registry["database_path"].IsScalar() ||
        !registry["pinned_keys"].IsSequence()) {
        throw std::invalid_argument(
            "artifact_registry requires only database_path and pinned_keys");
    }
    result.database_path = registry["database_path"].as<std::string>();
    if (result.database_path.empty()) {
        throw std::invalid_argument("retrieval database_path must not be empty");
    }
    if (registry["pinned_keys"].size() == 0) {
        throw std::invalid_argument("retrieval requires at least one pinned Ed25519 key");
    }

    std::set<std::string> seen_fingerprints;
    for (const auto& yaml_key : registry["pinned_keys"]) {
        if (!yaml_key.IsMap() || yaml_key.size() != 3 ||
            !yaml_key["fingerprint"] || !yaml_key["public_key_hex"] ||
            !yaml_key["label"] || !yaml_key["fingerprint"].IsScalar() ||
            !yaml_key["public_key_hex"].IsScalar() || !yaml_key["label"].IsScalar()) {
            throw std::invalid_argument(
                "each pinned key requires fingerprint, public_key_hex, and label");
        }
        themis::plugins::PinnedKey key;
        key.fingerprint = yaml_key["fingerprint"].as<std::string>();
        key.public_key = decodeHexPublicKey(
            yaml_key["public_key_hex"].as<std::string>());
        key.label = yaml_key["label"].as<std::string>();
        if (key.public_key.size() != 32 || key.label.empty() ||
            themis::plugins::pinned_ed25519::fingerprint(key.public_key) !=
                key.fingerprint ||
            !seen_fingerprints.insert(key.fingerprint).second) {
            throw std::invalid_argument(
                "pinned key fingerprint/key mismatch, invalid key, or duplicate fingerprint");
        }
        result.pinned_keys.push_back(std::move(key));
    }

    return result;
}

LoRAManifestStore::LoRAManifestStore() = default;

LoRAManifestStore::LoRAManifestStore(RuntimeConfig config)
    : config_(std::move(config)) {
    if (config_.database_path.empty()) {
        throw std::invalid_argument("retrieval database_path must not be empty");
    }
    if (config_.pinned_keys.empty()) {
        throw std::invalid_argument(
            "retrieval production config requires at least one pinned Ed25519 key");
    }
    std::set<std::string> seen_fingerprints;
    for (const auto& key : config_.pinned_keys) {
        if (!key.active || key.label.empty() || key.fingerprint.empty() ||
            key.public_key.size() != 32 ||
            themis::plugins::pinned_ed25519::fingerprint(key.public_key) !=
                key.fingerprint ||
            !seen_fingerprints.insert(key.fingerprint).second) {
            throw std::invalid_argument(
                "retrieval pinned keys must be active, valid, and unique");
        }
    }
    const auto verifier_keys = config_.pinned_keys;
    signature_verifier_ =
        [verifier_keys](const std::string& hash, const std::string& signature,
                        const std::string& signer_id) {
            const auto key = std::find_if(
                verifier_keys.begin(), verifier_keys.end(),
                [&signer_id](const auto& candidate) {
                    return candidate.active &&
                           candidate.fingerprint == signer_id;
                });
            return key != verifier_keys.end() &&
                   themis::plugins::pinned_ed25519::verify(
                       key->fingerprint, key->public_key, hash, signature);
        };
    const std::filesystem::path db_path(config_.database_path);
    try {
        if (db_path.has_parent_path()) {
            std::filesystem::create_directories(db_path.parent_path());
        }
    } catch (const std::filesystem::filesystem_error& error) {
        throw std::runtime_error(
            std::string("Failed to prepare retrieval artifact database path: ") +
            error.what());
    }
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::DB* raw_db = nullptr;
    const auto status =
        themis::storage::detail::openDbCompat(options, config_.database_path, &raw_db);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open retrieval artifact RocksDB: " +
                                 status.ToString());
    }
    db_.reset(raw_db);

    std::unique_ptr<rocksdb::Iterator> iterator(
        db_->NewIterator(rocksdb::ReadOptions()));
    for (iterator->Seek(kPackagePrefix);
         iterator->Valid() &&
         iterator->key().ToString().rfind(kPackagePrefix, 0) == 0;
         iterator->Next()) {
        try {
            const auto package = LoRAPackage::from_json(
                json::parse(iterator->value().ToString()));
            if (artifactKey(kPackagePrefix, package.package_id) !=
                iterator->key().ToString()) {
                throw std::runtime_error("retrieval package key/id mismatch");
            }
            if (!verifySignedArtifact(package, config_.pinned_keys,
                                      signature_verifier_)) {
                throw std::runtime_error(
                    "persisted retrieval package signature verification failed");
            }
            if (!verifyPersistedLifecycleState(
                    "package", package.to_json(), package.statusToString(),
                    "DRAFT", config_.persisted_lifecycle_verifier)) {
                throw std::runtime_error(
                    "persisted package lifecycle state is not owner-verified");
            }
            packages_.emplace(package.package_id, package);
        } catch (const std::exception& error) {
            throw std::runtime_error(
                std::string("Invalid persisted retrieval package: ") + error.what());
        }
    }
    if (!iterator->status().ok()) {
        throw std::runtime_error("Failed to scan retrieval packages: " +
                                 iterator->status().ToString());
    }
    iterator.reset(db_->NewIterator(rocksdb::ReadOptions()));
    for (iterator->Seek(kProductPrefix);
         iterator->Valid() &&
         iterator->key().ToString().rfind(kProductPrefix, 0) == 0;
         iterator->Next()) {
        try {
            const auto product = PortableAdapterProduct::from_json(
                json::parse(iterator->value().ToString()));
            if (artifactKey(kProductPrefix, product.product_id) !=
                iterator->key().ToString()) {
                throw std::runtime_error("retrieval product key/id mismatch");
            }
            if (!verifySignedArtifact(product, config_.pinned_keys,
                                      signature_verifier_)) {
                throw std::runtime_error(
                    "persisted retrieval product signature verification failed");
            }
            if (!verifyPersistedLifecycleState(
                    "product", product.to_json(), product.statusToString(),
                    "BUILDING", config_.persisted_lifecycle_verifier)) {
                throw std::runtime_error(
                    "persisted product lifecycle state is not owner-verified");
            }
            products_.emplace(product.product_id, product);
        } catch (const std::exception& error) {
            throw std::runtime_error(
                std::string("Invalid persisted retrieval product: ") + error.what());
        }
    }
    if (!iterator->status().ok()) {
        throw std::runtime_error("Failed to scan retrieval products: " +
                                 iterator->status().ToString());
    }
}

LoRAManifestStore::~LoRAManifestStore() {
    if (db_) {
        rocksdb::FlushOptions options;
        options.wait = true;
        db_->Flush(options);
        db_->Close();
    }
}

// ============================================================================
// IntegrityHelper
// ============================================================================

/**
 * @brief Sha256 Hex.
 * @param[in] data Input parameter.
 * @param[in] size Input parameter.
 * @return Return value.
 * @details Calls: update(), bytesToHex(), finalise().
 */
std::string IntegrityHelper::sha256Hex(const uint8_t* data, size_t size) {
    Sha256State s;
    s.update(data, size);
    return bytesToHex(s.finalise());
}

/**
 * @brief Sha256 Hex.
 * @param[in] input Input parameter.
 * @return Return value.
 * @details Calls: data(), size().
 */
std::string IntegrityHelper::sha256Hex(const std::string& input) {
    return sha256Hex(reinterpret_cast<const uint8_t*>(input.data()), input.size());
}

/**
 * @brief Verify Hash.
 * @param[in] data Input parameter.
 * @param[in] size Input parameter.
 * @param[in] expected_hex Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: sha256Hex().
 */
bool IntegrityHelper::verifyHash(const uint8_t* data, size_t size,
                                  const std::string& expected_hex) {
    return sha256Hex(data, size) == expected_hex;
}

/**
 * @brief Verify Hash.
 * @param[in] input Input parameter.
 * @param[in] expected_hex Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: sha256Hex().
 */
bool IntegrityHelper::verifyHash(const std::string& input,
                                  const std::string& expected_hex) {
    return sha256Hex(input) == expected_hex;
}

// ============================================================================
// AdapterUsagePolicy
// ============================================================================

json AdapterUsagePolicy::to_json() const {
    json j;
    j["license"]                    = license;
    j["restrictions"]               = restrictions;
    j["allowed_base_models"]        = allowed_base_models;
    j["max_concurrent_deployments"] = max_concurrent_deployments;
    j["expiry_date"]                = expiry_date;
    return j;
}

/**
 * @brief From json.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: contains(), at().
 */
AdapterUsagePolicy AdapterUsagePolicy::from_json(const json& j) {
    AdapterUsagePolicy p = {};
    if (j.contains("license")) {
      p.license = j.at("license").get<std::string>();
    }
    if (j.contains("restrictions")) {
      p.restrictions = j.at("restrictions").get<std::string>();
    }
    if (j.contains("allowed_base_models")) {
      p.allowed_base_models = j.at("allowed_base_models").get<std::vector<std::string>>();
    }
    if (j.contains("max_concurrent_deployments")) {
      p.max_concurrent_deployments = j.at("max_concurrent_deployments").get<int>();
    }
    if (j.contains("expiry_date")) {
      p.expiry_date = j.at("expiry_date").get<std::string>();
    }
    return p;
}

// ============================================================================
// LoRAPackageProvenance
// ============================================================================

json LoRAPackageProvenance::to_json() const {
    json j;
    j["trainer_id"]           = trainer_id;
    j["training_framework"]   = training_framework;
    j["dataset_id"]           = dataset_id;
    j["dataset_hash"]         = dataset_hash;
    j["base_model_id"]        = base_model_id;
    j["base_model_hash"]      = base_model_hash;
    j["hyperparameter_hash"]  = hyperparameter_hash;
    j["training_duration_secs"] = training_duration_secs;
    j["created_at"]           = created_at;
    j["hardware_info"]        = hardware_info.is_null() ? json::object() : hardware_info;
    j["custom_metadata"]      = custom_metadata.is_null() ? json::object() : custom_metadata;
    return j;
}

/**
 * @brief From json.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: contains(), at().
 */
LoRAPackageProvenance LoRAPackageProvenance::from_json(const json& j) {
    LoRAPackageProvenance p = {};
    if (j.contains("trainer_id")) {
      p.trainer_id           = j.at("trainer_id").get<std::string>();
    }
    if (j.contains("training_framework")) {
      p.training_framework   = j.at("training_framework").get<std::string>();
    }
    if (j.contains("dataset_id")) {
      p.dataset_id           = j.at("dataset_id").get<std::string>();
    }
    if (j.contains("dataset_hash")) {
      p.dataset_hash         = j.at("dataset_hash").get<std::string>();
    }
    if (j.contains("base_model_id")) {
      p.base_model_id        = j.at("base_model_id").get<std::string>();
    }
    if (j.contains("base_model_hash")) {
      p.base_model_hash      = j.at("base_model_hash").get<std::string>();
    }
    if (j.contains("hyperparameter_hash")) {
      p.hyperparameter_hash  = j.at("hyperparameter_hash").get<std::string>();
    }
    if (j.contains("training_duration_secs")) {
      p.training_duration_secs = j.at("training_duration_secs").get<double>();
    }
    if (j.contains("created_at")) {
      p.created_at           = j.at("created_at").get<std::string>();
    }
    if (j.contains("hardware_info")) {
      p.hardware_info        = j.at("hardware_info");
    }
    if (j.contains("custom_metadata")) {
      p.custom_metadata      = j.at("custom_metadata");
    }
    return p;
}

// ============================================================================
// ArtifactIntegrity
// ============================================================================

json ArtifactIntegrity::to_json() const {
    json j;
    j["weights_hash"]         = weights_hash;
    j["manifest_hash"]        = manifest_hash;
    j["signature"]            = signature;
    j["signature_algorithm"]  = signature_algorithm;
    j["signer_id"]            = signer_id;
    j["signed_at"]            = signed_at;
    // Note: signature_verified is intentionally omitted — runtime-only field
    return j;
}

/**
 * @brief From json.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: contains(), at().
 */
ArtifactIntegrity ArtifactIntegrity::from_json(const json& j) {
    ArtifactIntegrity i = {};
    if (j.contains("weights_hash")) {
      i.weights_hash        = j.at("weights_hash").get<std::string>();
    }
    if (j.contains("manifest_hash")) {
      i.manifest_hash       = j.at("manifest_hash").get<std::string>();
    }
    if (j.contains("signature")) {
      i.signature           = j.at("signature").get<std::string>();
    }
    if (j.contains("signature_algorithm")) {
      i.signature_algorithm = j.at("signature_algorithm").get<std::string>();
    }
    if (j.contains("signer_id")) {
      i.signer_id           = j.at("signer_id").get<std::string>();
    }
    if (j.contains("signed_at")) {
      i.signed_at           = j.at("signed_at").get<std::string>();
    }
    return i;
}

// ============================================================================
// LoRAPackage
// ============================================================================

json LoRAPackage::to_json() const {
    json j;
    // Identity — fixed order for deterministic hashing
    j["package_id"]               = package_id;
    j["name"]                     = name;
    j["version"]                  = version;
    j["description"]              = description;
    j["supported_architectures"]  = supported_architectures;
    j["lora_rank"]                = lora_rank;
    j["lora_alpha"]               = lora_alpha;
    j["target_modules"]           = target_modules;
    j["parent_package_id"]        = parent_package_id;
    j["provenance"]               = provenance.to_json();
    j["policy"]                   = policy.to_json();
    j["weights_path"]             = weights_path;
    j["integrity"]                = integrity.to_json();
    j["status"]                   = statusToString();
    j["created_at"]               = created_at;
    j["updated_at"]               = updated_at;
    return j;
}

/**
 * @brief From json.
 * @param[in] j Input parameter.
 * @return Return value.
 * @throws std::invalid_argument if an error occurs.
 * @details Calls: contains(), at(), statusFromString().
 */
LoRAPackage LoRAPackage::from_json(const json& j) {
    // Validate required fields
    if (!j.contains("package_id") || !j.contains("name") || !j.contains("version")) {
        throw std::invalid_argument(
            "LoRAPackage::from_json: required fields missing "
            "(package_id, name, version)");
    }
    LoRAPackage p;
    p.package_id              = j.at("package_id").get<std::string>();
    p.name                    = j.at("name").get<std::string>();
    p.version                 = j.at("version").get<std::string>();
    if (j.contains("description")) {
      p.description           = j.at("description").get<std::string>();
    }
    if (j.contains("supported_architectures")) {
      p.supported_architectures = j.at("supported_architectures").get<std::vector<std::string>>();
    }
    if (j.contains("lora_rank")) {
      p.lora_rank             = j.at("lora_rank").get<int>();
    }
    if (j.contains("lora_alpha")) {
      p.lora_alpha            = j.at("lora_alpha").get<float>();
    }
    if (j.contains("target_modules")) {
      p.target_modules        = j.at("target_modules").get<std::vector<std::string>>();
    }
    if (j.contains("parent_package_id")) {
      p.parent_package_id     = j.at("parent_package_id").get<std::string>();
    }
    if (j.contains("provenance")) {
      p.provenance            = LoRAPackageProvenance::from_json(j.at("provenance"));
    }
    if (j.contains("policy")) {
      p.policy                = AdapterUsagePolicy::from_json(j.at("policy"));
    }
    if (j.contains("weights_path")) {
      p.weights_path          = j.at("weights_path").get<std::string>();
    }
    if (j.contains("integrity")) {
      p.integrity             = ArtifactIntegrity::from_json(j.at("integrity"));
    }
    if (j.contains("status")) {
      p.status                = statusFromString(j.at("status").get<std::string>());
    }
    if (j.contains("created_at")) {
      p.created_at            = j.at("created_at").get<std::string>();
    }
    if (j.contains("updated_at")) {
      p.updated_at            = j.at("updated_at").get<std::string>();
    }
    return p;
}

/**
 * @brief Compute Manifest Hash.
 * @details Calls: to_json(), dump(), IntegrityHelper::sha256Hex().
 */
void LoRAPackage::computeManifestHash() {
    // Lifecycle state is mutable by the registry; the signed payload covers
    // artifact identity/content, not status or update timestamps.
    LoRAPackage unsigned_package = *this;
    unsigned_package.integrity.manifest_hash.clear();
    unsigned_package.integrity.signature.clear();
    unsigned_package.integrity.signature_algorithm.clear();
    unsigned_package.integrity.signer_id.clear();
    unsigned_package.integrity.signed_at.clear();
    unsigned_package.status = LoRAPackageStatus::DRAFT;
    unsigned_package.updated_at.clear();
    integrity.manifest_hash =
        IntegrityHelper::sha256Hex(unsigned_package.to_json().dump());
}

bool LoRAPackage::supportsArchitecture(const std::string& arch) const {
    if (supported_architectures.empty()) {
      return true;
    }
    const std::string arch_lower = [&] {
        std::string s = arch;
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }();
    return std::any_of(supported_architectures.begin(), supported_architectures.end(),
                       [&arch_lower](const std::string& a) {
                           std::string al = a;
                           std::transform(al.begin(), al.end(), al.begin(),
                                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                           return al == arch_lower;
                       });
}

std::string LoRAPackage::statusToString() const {
    switch (status) {
        case LoRAPackageStatus::DRAFT:       return "DRAFT";
        case LoRAPackageStatus::VALIDATED:   return "VALIDATED";
        case LoRAPackageStatus::DEPRECATED:  return "DEPRECATED";
        case LoRAPackageStatus::REVOKED:     return "REVOKED";
    }
    return "DRAFT";
}

/**
 * @brief Status From String.
 * @param[in] s Input parameter.
 * @return Return value.
 * @throws std::invalid_argument if an error occurs.
 * @details Implements statusFromString without additional internal calls.
 */
LoRAPackageStatus LoRAPackage::statusFromString(const std::string& s) {
    if (s == "DRAFT") {
      return LoRAPackageStatus::DRAFT;
    }
    if (s == "VALIDATED") {
      return LoRAPackageStatus::VALIDATED;
    }
    if (s == "DEPRECATED") {
      return LoRAPackageStatus::DEPRECATED;
    }
    if (s == "REVOKED") {
      return LoRAPackageStatus::REVOKED;
    }
    throw std::invalid_argument("LoRAPackage::statusFromString: unknown status '" + s + "'");
}

// ============================================================================
// PortableAdapterProduct
// ============================================================================

json PortableAdapterProduct::to_json() const {
    json j;
    j["product_id"]                  = product_id;
    j["name"]                        = name;
    j["version"]                     = version;
    j["source_package_id"]           = source_package_id;
    j["target_base_model_id"]        = target_base_model_id;
    j["target_model_architecture"]   = target_model_architecture;
    j["quantization"]                = quantization;
    j["format"]                      = format;
    j["file_path"]                   = file_path;
    j["file_size_bytes"]             = file_size_bytes;
    j["max_context_length"]          = max_context_length;
    j["memory_requirement_mb"]       = memory_requirement_mb;
    j["compatible_model_versions"]   = compatible_model_versions;
    j["integrity"]                   = integrity.to_json();
    j["status"]                      = statusToString();
    j["created_at"]                  = created_at;
    j["updated_at"]                  = updated_at;
    j["deployed_at"]                 = deployed_at;
    return j;
}

/**
 * @brief From json.
 * @param[in] j Input parameter.
 * @return Return value.
 * @throws std::invalid_argument if an error occurs.
 * @details Calls: contains(), at(), statusFromString().
 */
PortableAdapterProduct PortableAdapterProduct::from_json(const json& j) {
    if (!j.contains("product_id") || !j.contains("source_package_id") ||
        !j.contains("target_base_model_id")) {
        throw std::invalid_argument(
            "PortableAdapterProduct::from_json: required fields missing "
            "(product_id, source_package_id, target_base_model_id)");
    }
    PortableAdapterProduct p;
    p.product_id                = j.at("product_id").get<std::string>();
    p.source_package_id         = j.at("source_package_id").get<std::string>();
    p.target_base_model_id      = j.at("target_base_model_id").get<std::string>();
    if (j.contains("name")) {
      p.name = j.at("name").get<std::string>();
    }
    if (j.contains("version")) {
      p.version = j.at("version").get<std::string>();
    }
    if (j.contains("target_model_architecture")) {
      p.target_model_architecture = j.at("target_model_architecture").get<std::string>();
    }
    if (j.contains("quantization")) {
      p.quantization = j.at("quantization").get<std::string>();
    }
    if (j.contains("format")) {
      p.format = j.at("format").get<std::string>();
    }
    if (j.contains("file_path")) {
      p.file_path = j.at("file_path").get<std::string>();
    }
    if (j.contains("file_size_bytes")) {
      p.file_size_bytes = j.at("file_size_bytes").get<size_t>();
    }
    if (j.contains("max_context_length")) {
      p.max_context_length = j.at("max_context_length").get<int>();
    }
    if (j.contains("memory_requirement_mb")) {
      p.memory_requirement_mb = j.at("memory_requirement_mb").get<size_t>();
    }
    if (j.contains("compatible_model_versions")) {
      p.compatible_model_versions = j.at("compatible_model_versions").get<std::vector<std::string>>();
    }
    if (j.contains("integrity")) {
      p.integrity = ArtifactIntegrity::from_json(j.at("integrity"));
    }
    if (j.contains("status")) {
      p.status = statusFromString(j.at("status").get<std::string>());
    }
    if (j.contains("created_at")) {
      p.created_at = j.at("created_at").get<std::string>();
    }
    if (j.contains("updated_at")) {
      p.updated_at = j.at("updated_at").get<std::string>();
    }
    if (j.contains("deployed_at")) {
      p.deployed_at = j.at("deployed_at").get<std::string>();
    }
    return p;
}

/**
 * @brief Compute Manifest Hash.
 * @details Calls: to_json(), dump(), IntegrityHelper::sha256Hex().
 */
void PortableAdapterProduct::computeManifestHash() {
    PortableAdapterProduct unsigned_product = *this;
    unsigned_product.integrity.manifest_hash.clear();
    unsigned_product.integrity.signature.clear();
    unsigned_product.integrity.signature_algorithm.clear();
    unsigned_product.integrity.signer_id.clear();
    unsigned_product.integrity.signed_at.clear();
    unsigned_product.status = AdapterProductStatus::BUILDING;
    unsigned_product.updated_at.clear();
    unsigned_product.deployed_at.clear();
    integrity.manifest_hash =
        IntegrityHelper::sha256Hex(unsigned_product.to_json().dump());
}

std::string PortableAdapterProduct::statusToString() const {
    switch (status) {
        case AdapterProductStatus::BUILDING:  return "BUILDING";
        case AdapterProductStatus::READY:     return "READY";
        case AdapterProductStatus::DEPLOYED:  return "DEPLOYED";
        case AdapterProductStatus::RETIRED:   return "RETIRED";
        case AdapterProductStatus::FAILED:    return "FAILED";
    }
    return "BUILDING";
}

/**
 * @brief Status From String.
 * @param[in] s Input parameter.
 * @return Return value.
 * @throws std::invalid_argument if an error occurs.
 * @details Implements statusFromString without additional internal calls.
 */
AdapterProductStatus PortableAdapterProduct::statusFromString(const std::string& s) {
    if (s == "BUILDING") {
      return AdapterProductStatus::BUILDING;
    }
    if (s == "READY") {
      return AdapterProductStatus::READY;
    }
    if (s == "DEPLOYED") {
      return AdapterProductStatus::DEPLOYED;
    }
    if (s == "RETIRED") {
      return AdapterProductStatus::RETIRED;
    }
    if (s == "FAILED") {
      return AdapterProductStatus::FAILED;
    }
    throw std::invalid_argument(
        "PortableAdapterProduct::statusFromString: unknown status '" + s + "'");
}

// ============================================================================
// LoRAManifestStore
// ============================================================================

/**
 * @brief ── LoRAPackage CRUD ──────────────────────────────────────────────────────────
 * @param[in] pkg Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: empty(), lk().
 */

bool LoRAManifestStore::storePackage(const LoRAPackage& pkg) {
    if (!db_ || pkg.package_id.empty() ||
        pkg.status != LoRAPackageStatus::DRAFT ||
        !verifySignedArtifact(pkg, config_.pinned_keys,
                              signature_verifier_)) {
      return false;
    }
    std::lock_guard<std::mutex> lk(mutex_);
    const auto existing = packages_.find(pkg.package_id);
    if (existing != packages_.end() &&
        existing->second.status != LoRAPackageStatus::DRAFT) {
        return false;
    }
    const auto status = db_->Put(
        durableWriteOptions(), artifactKey(kPackagePrefix, pkg.package_id),
        pkg.to_json().dump());
    if (!status.ok()) {
        return false;
    }
    packages_[pkg.package_id] = pkg;
    return true;
}

std::optional<LoRAPackage> LoRAManifestStore::loadPackage(
    const std::string& package_id) const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = packages_.find(package_id);
    if (it == packages_.end()) {
      return std::nullopt;
    }
    return it->second;
}

/**
 * @brief Delete Package.
 * @param[in] package_id Identifier of the package.
 * @return True when the operation succeeds.
 * @details Calls: lk(), erase().
 */
bool LoRAManifestStore::deletePackage(const std::string& package_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (packages_.find(package_id) == packages_.end()) {
        return false;
    }
    if (db_) {
        const auto status = db_->Delete(
            durableWriteOptions(), artifactKey(kPackagePrefix, package_id));
        if (!status.ok()) {
            return false;
        }
    }
    packages_.erase(package_id);
    return true;
}

bool LoRAManifestStore::transitionPackageStatus(
    const std::string& package_id,
    LoRAPackageStatus expected_status,
    LoRAPackageStatus requested_status) {
    if (!db_ || !isLegalPackageTransition(expected_status, requested_status)) {
        return false;
    }

    std::optional<LoRAPackage> current;
    LifecycleTransitionAuthorizer authorizer;
    PersistedLifecycleVerifier state_verifier;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = packages_.find(package_id);
        if (it == packages_.end() || it->second.status != expected_status) {
            return false;
        }
        current = it->second;
        authorizer = config_.lifecycle_transition_authorizer;
        state_verifier = config_.persisted_lifecycle_verifier;
    }

    if (!verifySignedArtifact(*current, config_.pinned_keys,
                              signature_verifier_) ||
        !authorizeLifecycleTransition(
            "package", current->to_json(), current->statusToString(),
            LoRAPackage{.status = requested_status}.statusToString(), authorizer)) {
        return false;
    }

    auto updated = *current;
    updated.status = requested_status;
    updated.updated_at = utcTimestampNow();
    const auto requested_status_string = updated.statusToString();
    if (!verifyPersistedLifecycleState(
            "package", updated.to_json(), requested_status_string,
            "DRAFT", state_verifier)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = packages_.find(package_id);
    if (it == packages_.end() || it->second.to_json() != current->to_json()) {
        return false;
    }
    const auto status = db_->Put(
        durableWriteOptions(), artifactKey(kPackagePrefix, package_id),
        updated.to_json().dump());
    if (!status.ok()) {
        return false;
    }
    it->second = std::move(updated);
    return true;
}

std::vector<std::string> LoRAManifestStore::listPackageIds() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<std::string> ids = {};

    ids.reserve(packages_.size());
    for (const auto& kv : packages_) {
      ids.push_back(kv.first);
    }
    return ids;
}

std::vector<LoRAPackage> LoRAManifestStore::listPackagesByStatus(
    LoRAPackageStatus status) const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<LoRAPackage> out = {};

    for (const auto& kv : packages_) {
        if (kv.second.status == status) {
          out.push_back(kv.second);
        }
    }
    return out;
}

/**
 * @brief ── PortableAdapterProduct CRUD ───────────────────────────────────────────────
 * @param[in] product Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: empty(), lk().
 */

bool LoRAManifestStore::storeProduct(const PortableAdapterProduct& product) {
    if (!db_ || product.product_id.empty() || product.source_package_id.empty() ||
        product.status != AdapterProductStatus::BUILDING) {
      return false;
    }
    if (!verifySignedArtifact(product, config_.pinned_keys,
                              signature_verifier_)) {
        return false;
    }
    std::lock_guard<std::mutex> lk(mutex_);
    const auto existing = products_.find(product.product_id);
    if (existing != products_.end() &&
        existing->second.status != AdapterProductStatus::BUILDING) {
        return false;
    }
    const auto status = db_->Put(
        durableWriteOptions(), artifactKey(kProductPrefix, product.product_id),
        product.to_json().dump());
    if (!status.ok()) {
        return false;
    }
    products_[product.product_id] = product;
    return true;
}

std::optional<PortableAdapterProduct> LoRAManifestStore::loadProduct(
    const std::string& product_id) const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = products_.find(product_id);
    if (it == products_.end()) {
      return std::nullopt;
    }
    return it->second;
}

/**
 * @brief Delete Product.
 * @param[in] product_id Identifier of the product.
 * @return True when the operation succeeds.
 * @details Calls: lk(), erase().
 */
bool LoRAManifestStore::deleteProduct(const std::string& product_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (products_.find(product_id) == products_.end()) {
        return false;
    }
    if (db_) {
        const auto status = db_->Delete(
            durableWriteOptions(), artifactKey(kProductPrefix, product_id));
        if (!status.ok()) {
            return false;
        }
    }
    products_.erase(product_id);
    return true;
}

bool LoRAManifestStore::transitionProductStatus(
    const std::string& product_id,
    AdapterProductStatus expected_status,
    AdapterProductStatus requested_status) {
    if (!db_ || requested_status == AdapterProductStatus::DEPLOYED ||
        !isLegalProductTransition(expected_status, requested_status)) {
        return false;
    }

    std::optional<PortableAdapterProduct> current;
    LifecycleTransitionAuthorizer authorizer;
    PersistedLifecycleVerifier state_verifier;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = products_.find(product_id);
        if (it == products_.end() || it->second.status != expected_status) {
            return false;
        }
        current = it->second;
        authorizer = config_.lifecycle_transition_authorizer;
        state_verifier = config_.persisted_lifecycle_verifier;
    }

    PortableAdapterProduct target_status;
    target_status.status = requested_status;
    if (!verifySignedArtifact(*current, config_.pinned_keys,
                              signature_verifier_) ||
        !authorizeLifecycleTransition(
            "product", current->to_json(), current->statusToString(),
            target_status.statusToString(), authorizer)) {
        return false;
    }

    auto updated = *current;
    updated.status = requested_status;
    updated.updated_at = utcTimestampNow();
    if (!verifyPersistedLifecycleState(
            "product", updated.to_json(), updated.statusToString(),
            "BUILDING", state_verifier)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = products_.find(product_id);
    if (it == products_.end() || it->second.to_json() != current->to_json()) {
        return false;
    }
    const auto status = db_->Put(
        durableWriteOptions(), artifactKey(kProductPrefix, product_id),
        updated.to_json().dump());
    if (!status.ok()) {
        return false;
    }
    it->second = std::move(updated);
    return true;
}

std::vector<std::string> LoRAManifestStore::listProductIds() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<std::string> ids = {};

    ids.reserve(products_.size());
    for (const auto& kv : products_) {
      ids.push_back(kv.first);
    }
    return ids;
}

std::vector<PortableAdapterProduct> LoRAManifestStore::listProductsByPackage(
    const std::string& package_id) const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<PortableAdapterProduct> out = {};

    for (const auto& kv : products_) {
        if (kv.second.source_package_id == package_id) {
          out.push_back(kv.second);
        }
    }
    return out;
}

std::vector<PortableAdapterProduct> LoRAManifestStore::listProductsByStatus(
    AdapterProductStatus status) const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<PortableAdapterProduct> out = {};

    for (const auto& kv : products_) {
        if (kv.second.status == status) {
          out.push_back(kv.second);
        }
    }
    return out;
}

// ── Integrity verification ────────────────────────────────────────────────────

bool LoRAManifestStore::verifyPackageIntegrity(
    const std::string& package_id) const {
    std::optional<LoRAPackage> maybe;
    SignatureVerifier verifier;
    std::vector<themis::plugins::PinnedKey> pinned_keys;
    {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = packages_.find(package_id);
        if (it == packages_.end()) {
          return false;
        }
        maybe    = it->second;
        verifier = signature_verifier_;
        pinned_keys = config_.pinned_keys;
    }
    return verifySignedArtifact(*maybe, pinned_keys, verifier);
}

bool LoRAManifestStore::verifyProductIntegrity(
    const std::string& product_id) const {
    std::optional<PortableAdapterProduct> maybe;
    SignatureVerifier verifier;
    std::vector<themis::plugins::PinnedKey> pinned_keys;
    {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = products_.find(product_id);
        if (it == products_.end()) {
          return false;
        }
        maybe    = it->second;
        verifier = signature_verifier_;
        pinned_keys = config_.pinned_keys;
    }
    return verifySignedArtifact(*maybe, pinned_keys, verifier);
}

LoRAManifestStore::DeploymentResult LoRAManifestStore::deployProduct(
    const std::string& product_id,
    uint32_t candidate_model_version,
    uint32_t baseline_model_version,
    const themis::rag::quality::AggregatedMetrics& candidate_metrics,
    const themis::rag::quality::AggregatedMetrics& baseline_metrics) {
    DeploymentResult result;
    if (!db_) {
        result.reason =
            "deployment requires a persistent RocksDB store; in-memory mode is test-only";
        return result;
    }
    std::optional<PortableAdapterProduct> product;
    std::optional<LoRAPackage> package;
    SignatureVerifier verifier;
    DeploymentAuthorization authorizer;
    DeploymentEvidenceValidator evidence_validator;
    PersistedLifecycleVerifier lifecycle_state_verifier;
    std::shared_ptr<themis::rag::quality::DeploymentGateController> quality_gate;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto product_it = products_.find(product_id);
        if (product_it == products_.end() ||
            product_it->second.status != AdapterProductStatus::READY) {
            result.reason = "product is missing or not READY";
            return result;
        }
        const auto package_it =
            packages_.find(product_it->second.source_package_id);
        if (package_it == packages_.end() ||
            package_it->second.status != LoRAPackageStatus::VALIDATED) {
            result.reason = "source package is missing or not VALIDATED";
            return result;
        }
        product = product_it->second;
        package = package_it->second;
        verifier = signature_verifier_;
        authorizer = config_.deployment_authorizer;
        evidence_validator = config_.deployment_evidence_validator;
        lifecycle_state_verifier = config_.persisted_lifecycle_verifier;
        quality_gate = config_.quality_gate;
    }

    if (!verifySignedArtifact(*package, config_.pinned_keys, verifier) ||
        !verifySignedArtifact(*product, config_.pinned_keys, verifier)) {
        result.reason = "artifact signature or pinned signer verification failed";
        return result;
    }
    if (!quality_gate) {
        result.reason = "quality gate controller is not configured";
        return result;
    }
    if (!validQualityMetrics(candidate_metrics, baseline_metrics)) {
        result.reason = "candidate or baseline quality metrics are missing or invalid";
        return result;
    }
    if (!evidence_validator) {
        result.reason = "deployment evidence validator is not configured";
        return result;
    }
    if (!lifecycle_state_verifier) {
        result.reason = "owner lifecycle-state verifier is not configured";
        return result;
    }
    try {
        if (!evidence_validator(*product, candidate_model_version,
                                baseline_model_version, candidate_metrics,
                                baseline_metrics)) {
            result.reason =
                "deployment evidence is not authenticated or product-bound";
            return result;
        }
    } catch (...) {
        result.reason = "deployment evidence validation failed";
        return result;
    }

    try {
        result.quality_decision = quality_gate->EvaluateCandidate(
            candidate_model_version, baseline_model_version, candidate_metrics,
            baseline_metrics);
    } catch (const std::exception&) {
        result.quality_decision.decision =
            themis::rag::quality::GateDecision::kDeny;
        result.reason = "quality gate evaluation failed";
        return result;
    } catch (...) {
        result.quality_decision.decision =
            themis::rag::quality::GateDecision::kDeny;
        result.reason = "quality gate evaluation failed";
        return result;
    }
    if (result.quality_decision.decision !=
        themis::rag::quality::GateDecision::kAllow) {
        result.reason = "quality gate did not return kAllow";
        return result;
    }
    if (!authorizer) {
        result.reason = "deployment authorization callback is not configured";
        return result;
    }
    try {
        if (!authorizer(*product, *package)) {
            result.reason = "deployment authorization callback denied the request";
            return result;
        }
    } catch (const std::exception&) {
        result.reason = "deployment authorization callback failed";
        return result;
    } catch (...) {
        result.reason = "deployment authorization callback failed";
        return result;
    }

    PortableAdapterProduct deployed = *product;
    deployed.status = AdapterProductStatus::DEPLOYED;
    deployed.deployed_at = utcTimestampNow();
    deployed.updated_at = deployed.deployed_at;
    if (!verifyPersistedLifecycleState(
            "product", deployed.to_json(), "DEPLOYED", "BUILDING",
            lifecycle_state_verifier)) {
        result.reason = "owner service did not verify DEPLOYED lifecycle state";
        return result;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto product_it = products_.find(product_id);
    const auto package_it = packages_.find(product->source_package_id);
    if (product_it == products_.end() || package_it == packages_.end() ||
        product_it->second.to_json() != product->to_json() ||
        package_it->second.to_json() != package->to_json()) {
        result.reason = "artifact changed during deployment authorization";
        return result;
    }
    const auto write_status = db_->Put(
        durableWriteOptions(), artifactKey(kProductPrefix, product_id),
        deployed.to_json().dump());
    if (!write_status.ok()) {
        result.reason = "failed to persist deployment status: " +
                        write_status.ToString();
        return result;
    }
    product_it->second = std::move(deployed);
    result.deployed = true;
    result.reason = "deployment quality and authorization checks passed";
    return result;
}

// ── Bulk export / import ──────────────────────────────────────────────────────

json LoRAManifestStore::exportPackages() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    // Collect keys and sort for stable, deterministic output order.
    std::vector<std::string> keys = {};

    keys.reserve(packages_.size());
    for (const auto& kv : packages_) {
      keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());
    json arr = json::array();
    for (const auto& k : keys) {
      arr.push_back(packages_.at(k).to_json());
    }
    return arr;
}

/**
 * @brief Import Packages.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: is_array(), LoRAPackage::from_json(), lk(), std::move().
 */
size_t LoRAManifestStore::importPackages(const json& j) {
    if (!j.is_array()) {
      return 0;
    }
    size_t count = 0;
    for (const auto& item : j) {
        try {
            LoRAPackage pkg = LoRAPackage::from_json(item);
            if (!db_ || pkg.package_id.empty() ||
                pkg.status != LoRAPackageStatus::DRAFT ||
                !verifySignedArtifact(pkg, config_.pinned_keys,
                                      signature_verifier_)) {
                continue;
            }
            std::lock_guard<std::mutex> lk(mutex_);
            const auto existing = packages_.find(pkg.package_id);
            if (existing != packages_.end() &&
                existing->second.status != LoRAPackageStatus::DRAFT) {
                continue;
            }
            const auto status = db_->Put(
                durableWriteOptions(),
                artifactKey(kPackagePrefix, pkg.package_id), pkg.to_json().dump());
            if (!status.ok()) {
                continue;
            }
            packages_[pkg.package_id] = std::move(pkg);
            ++count;
        } catch (const std::exception&) {
            // Skip malformed entries; caller may inspect the array for details.
        }
    }
    return count;
}

json LoRAManifestStore::exportProducts() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    // Collect keys and sort for stable, deterministic output order.
    std::vector<std::string> keys = {};

    keys.reserve(products_.size());
    for (const auto& kv : products_) {
      keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());
    json arr = json::array();
    for (const auto& k : keys) {
      arr.push_back(products_.at(k).to_json());
    }
    return arr;
}

/**
 * @brief Import Products.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: is_array(), PortableAdapterProduct::from_json(), lk(), std::move().
 */
size_t LoRAManifestStore::importProducts(const json& j) {
    if (!j.is_array()) {
      return 0;
    }
    size_t count = 0;
    for (const auto& item : j) {
        try {
            PortableAdapterProduct prod = PortableAdapterProduct::from_json(item);
            if (!db_ || prod.product_id.empty() || prod.source_package_id.empty() ||
                prod.status != AdapterProductStatus::BUILDING ||
                !verifySignedArtifact(prod, config_.pinned_keys,
                                      signature_verifier_)) {
                continue;
            }
            std::lock_guard<std::mutex> lk(mutex_);
            const auto existing = products_.find(prod.product_id);
            if (existing != products_.end() &&
                existing->second.status != AdapterProductStatus::BUILDING) {
                continue;
            }
            const auto status = db_->Put(
                durableWriteOptions(),
                artifactKey(kProductPrefix, prod.product_id), prod.to_json().dump());
            if (!status.ok()) {
                continue;
            }
            products_[prod.product_id] = std::move(prod);
            ++count;
        } catch (const std::exception&) {
        }
    }
    return count;
}

// ── Statistics ────────────────────────────────────────────────────────────────

size_t LoRAManifestStore::packageCount() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    return packages_.size();
}

size_t LoRAManifestStore::productCount() const {
    /**
     * @brief Lk.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mutex_);
    return products_.size();
}

} // namespace retrieval
} // namespace themis
