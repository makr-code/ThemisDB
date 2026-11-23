#include "tensor/dream_candidate_store.h"

#include "utils/hash_util.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <vector>

namespace themis {
namespace tensor {

namespace {

constexpr int kEnvelopeSchemaVersion = 1;
constexpr const char* kChecksumAlgorithm = "fnv1a64";
constexpr const char* kStorageKeyPrefix = "__dream_candidate__:v1:";

bool hasNonWhitespace(const std::string& value) {
    return std::any_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) == 0;
    });
}

nlohmann::json envelopeWithoutChecksum(const DreamCandidateArtifact& artifact) {
    nlohmann::json envelope = nlohmann::json::object();
    envelope["schema_version"] = kEnvelopeSchemaVersion;
    envelope["candidate_id"] = artifact.candidate_id;
    envelope["payload"] = artifact.payload;
    envelope["provenance"] = {
        {"synthetic", artifact.provenance.synthetic},
        {"dream_mode", artifact.provenance.dream_mode},
        {"run_id", artifact.provenance.run_id},
    };
    return envelope;
}

std::string checksumFor(const DreamCandidateArtifact& artifact) {
    return themis::hash::fnv1a64_hex(envelopeWithoutChecksum(artifact).dump());
}

bool hasExactKeys(const nlohmann::json& value,
                  std::initializer_list<const char*> expected_keys) {
    if (!value.is_object() || value.size() != expected_keys.size()) {
        return false;
    }
    return std::all_of(expected_keys.begin(), expected_keys.end(),
                       [&value](const char* key) { return value.contains(key); });
}

void setError(std::string* error, const std::string& message) {
    if (error != nullptr) {
        *error = message;
    }
}

std::optional<DreamCandidateArtifact> parseSerializedArtifact(
    const std::string& serialized) {
    const auto envelope = nlohmann::json::parse(serialized);
    if (!hasExactKeys(envelope, {"schema_version", "candidate_id", "payload",
                                 "provenance", "checksum_algorithm", "checksum"}) ||
        !envelope["schema_version"].is_number_integer() ||
        envelope["schema_version"].get<int>() != kEnvelopeSchemaVersion ||
        !envelope["candidate_id"].is_string() ||
        !envelope["checksum_algorithm"].is_string() ||
        envelope["checksum_algorithm"] != kChecksumAlgorithm ||
        !envelope["checksum"].is_string() ||
        !hasExactKeys(envelope["provenance"], {"synthetic", "dream_mode", "run_id"}) ||
        !envelope["provenance"]["synthetic"].is_boolean() ||
        !envelope["provenance"]["dream_mode"].is_boolean() ||
        !envelope["provenance"]["run_id"].is_string()) {
        return std::nullopt;
    }

    DreamCandidateArtifact artifact;
    artifact.candidate_id = envelope["candidate_id"].get<std::string>();
    artifact.payload = envelope["payload"];
    artifact.provenance.synthetic = envelope["provenance"]["synthetic"].get<bool>();
    artifact.provenance.dream_mode = envelope["provenance"]["dream_mode"].get<bool>();
    artifact.provenance.run_id = envelope["provenance"]["run_id"].get<std::string>();
    artifact.checksum = envelope["checksum"].get<std::string>();
    if (!DreamCandidateStore::verifyArtifact(artifact)) {
        return std::nullopt;
    }
    return artifact;
}

std::vector<uint8_t> asBytes(const std::string& value) {
    std::vector<uint8_t> bytes;
    bytes.reserve(value.size());
    for (const auto ch : value) {
        bytes.push_back(static_cast<uint8_t>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

std::string asString(const std::vector<uint8_t>& value) {
    std::string serialized;
    serialized.reserve(value.size());
    for (const auto byte : value) {
        serialized.push_back(static_cast<char>(byte));
    }
    return serialized;
}

} // namespace

nlohmann::json DreamCandidateArtifact::toJson() const {
    auto envelope = envelopeWithoutChecksum(*this);
    envelope["checksum_algorithm"] = kChecksumAlgorithm;
    envelope["checksum"] = checksum;
    return envelope;
}

DreamCandidateStore::DreamCandidateStore()
    : DreamCandidateStore(std::make_shared<storage::InMemoryTensorBackend>()) {}

DreamCandidateStore::DreamCandidateStore(
    std::shared_ptr<storage::ITensorStorageBackend> backend)
    : backend_(std::move(backend)) {
    if (!backend_) {
        throw std::invalid_argument("DreamCandidateStore backend must not be null");
    }
}

std::string DreamCandidateStore::storageKeyFor(const std::string& candidate_id) {
    constexpr char kHexDigits[] = "0123456789abcdef";
    std::string key = kStorageKeyPrefix;
    key.reserve(key.size() + candidate_id.size() * 2);
    for (const auto ch : candidate_id) {
        const auto byte = static_cast<unsigned char>(ch);
        key.push_back(kHexDigits[byte >> 4]);
        key.push_back(kHexDigits[byte & 0x0f]);
    }
    return key;
}

bool DreamCandidateStore::store(const std::string& candidate_id,
                                const nlohmann::json& payload,
                                const DreamCandidateProvenance& provenance,
                                std::string* error) {
    if (error != nullptr) {
        error->clear();
    }
    if (!hasNonWhitespace(candidate_id)) {
        setError(error, "candidate_id must not be empty or whitespace");
        return false;
    }
    if (payload.is_null() || payload.is_discarded()) {
        setError(error, "candidate payload must be valid non-null JSON");
        return false;
    }
    if (!provenance.synthetic || !provenance.dream_mode ||
        !hasNonWhitespace(provenance.run_id)) {
        setError(error, "provenance requires synthetic=true, dream_mode=true, and run_id");
        return false;
    }

    DreamCandidateArtifact artifact{candidate_id, payload, provenance, {}};
    try {
        artifact.checksum = checksumFor(artifact);
    } catch (const std::exception&) {
        setError(error, "candidate envelope cannot be serialized");
        return false;
    }

    std::vector<uint8_t> serialized;
    try {
        serialized = asBytes(artifact.toJson().dump());
    } catch (const std::exception&) {
        setError(error, "candidate envelope cannot be serialized");
        return false;
    }

    try {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto key = storageKeyFor(candidate_id);
        if (backend_->get(key).has_value()) {
            setError(error, "candidate_id already exists");
            return false;
        }
        if (!backend_->put(key, serialized)) {
            setError(error, "candidate storage backend rejected the write");
            return false;
        }
        return true;
    } catch (const std::exception&) {
        setError(error, "candidate storage backend failed");
        return false;
    } catch (...) {
        setError(error, "candidate storage backend failed");
        return false;
    }
}

std::optional<DreamCandidateArtifact> DreamCandidateStore::get(
    const std::string& candidate_id) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto serialized = backend_->get(storageKeyFor(candidate_id));
        if (!serialized) {
            return std::nullopt;
        }
        const auto artifact = parseSerializedArtifact(asString(*serialized));
        if (!artifact || artifact->candidate_id != candidate_id) {
            return std::nullopt;
        }
        return artifact;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::string> DreamCandidateStore::exportArtifact(
    const std::string& candidate_id) const {
    const auto artifact = get(candidate_id);
    if (!artifact) {
        return std::nullopt;
    }
    try {
        return artifact->toJson().dump();
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

bool DreamCandidateStore::importArtifact(const std::string& serialized,
                                         std::string* error) {
    if (error != nullptr) {
        error->clear();
    }

    std::optional<DreamCandidateArtifact> artifact;
    try {
        artifact = parseSerializedArtifact(serialized);
    } catch (const std::exception&) {
        setError(error, "candidate envelope is not valid JSON");
        return false;
    }
    if (!artifact) {
        setError(error, "candidate envelope schema, provenance, or checksum validation failed");
        return false;
    }

    try {
        const auto key = storageKeyFor(artifact->candidate_id);
        const auto bytes = asBytes(serialized);
        std::lock_guard<std::mutex> lock(mutex_);
        if (backend_->get(key).has_value()) {
            setError(error, "candidate_id already exists");
            return false;
        }
        if (!backend_->put(key, bytes)) {
            setError(error, "candidate storage backend rejected the write");
            return false;
        }
        return true;
    } catch (const std::exception&) {
        setError(error, "candidate storage backend failed");
        return false;
    } catch (...) {
        setError(error, "candidate storage backend failed");
        return false;
    }
}

bool DreamCandidateStore::verifyArtifact(
    const DreamCandidateArtifact& artifact) noexcept {
    if (!hasNonWhitespace(artifact.candidate_id) || artifact.payload.is_null() ||
        artifact.payload.is_discarded() || !artifact.provenance.synthetic ||
        !artifact.provenance.dream_mode || !hasNonWhitespace(artifact.provenance.run_id)) {
        return false;
    }
    try {
        return artifact.checksum == checksumFor(artifact);
    } catch (...) {
        return false;
    }
}

} // namespace tensor
} // namespace themis
