#include "tensor/dream_candidate_store.h"

#include "utils/hash_util.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <initializer_list>
#include <utility>

namespace themis {
namespace tensor {

namespace {

constexpr int kEnvelopeSchemaVersion = 1;
constexpr const char* kChecksumAlgorithm = "fnv1a64";

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

} // namespace

nlohmann::json DreamCandidateArtifact::toJson() const {
    auto envelope = envelopeWithoutChecksum(*this);
    envelope["checksum_algorithm"] = kChecksumAlgorithm;
    envelope["checksum"] = checksum;
    return envelope;
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

    std::lock_guard<std::mutex> lock(mutex_);
    if (!artifacts_.emplace(candidate_id, std::move(artifact)).second) {
        setError(error, "candidate_id already exists");
        return false;
    }
    return true;
}

std::optional<DreamCandidateArtifact> DreamCandidateStore::get(
    const std::string& candidate_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = artifacts_.find(candidate_id);
    if (it == artifacts_.end() || !verifyArtifact(it->second)) {
        return std::nullopt;
    }
    return it->second;
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

    try {
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
            setError(error, "candidate envelope has an invalid schema or provenance");
            return false;
        }

        DreamCandidateArtifact artifact;
        artifact.candidate_id = envelope["candidate_id"].get<std::string>();
        artifact.payload = envelope["payload"];
        artifact.provenance.synthetic = envelope["provenance"]["synthetic"].get<bool>();
        artifact.provenance.dream_mode = envelope["provenance"]["dream_mode"].get<bool>();
        artifact.provenance.run_id = envelope["provenance"]["run_id"].get<std::string>();
        artifact.checksum = envelope["checksum"].get<std::string>();
        if (!verifyArtifact(artifact)) {
            setError(error, "candidate envelope provenance or checksum validation failed");
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (!artifacts_.emplace(artifact.candidate_id, std::move(artifact)).second) {
            setError(error, "candidate_id already exists");
            return false;
        }
        return true;
    } catch (const std::exception&) {
        setError(error, "candidate envelope is not valid JSON");
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
