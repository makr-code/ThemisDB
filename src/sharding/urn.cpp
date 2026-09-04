/**
 * @file urn.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/urn.h"
#include <sstream>
#include <algorithm>
#include <cctype>

// Use xxHash for fast consistent hashing
// If xxHash is not available, we'll use std::hash as fallback
#ifdef __has_include
  #if __has_include(<xxhash.h>)
    #include <xxhash.h>
    #define HAS_XXHASH
  #endif
#endif

namespace themis::sharding {

namespace {
    /** @brief Supported model names accepted by URN::isValidModel. */
    // Valid model types
    const std::vector<std::string> VALID_MODELS = {
        "relational", "graph", "vector", "timeseries", "document"
    };
    
    /** @brief RFC-4122 style UUID regex used for syntax validation. */
    // UUID validation regex: 8-4-4-4-12 hex pattern
    const std::regex UUID_PATTERN(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
}

/**
 * @brief Parse textual URN and validate model/namespace/collection/uuid.
 * @param urn_str Candidate URN string.
 * @return Parsed URN on success, std::nullopt for malformed or unsupported input.
 */
std::optional<URN> URN::parse(std::string_view urn_str) {
    // Expected format: urn:themis:{model}:{namespace}:{collection}:{uuid}
    // Minimum length check: "urn:themis:a:b:c:d" = 18 chars minimum
    if (urn_str.size() < 18) {
        return std::nullopt;
    }
    
    // Must start with "urn:themis:"
    if (!urn_str.starts_with("urn:themis:")) {
        return std::nullopt;
    }
    
    // Split by colons
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = 0;
    
    while (end != std::string_view::npos) {
        end = urn_str.find(':', start);
        if (end != std::string_view::npos) {
            parts.emplace_back(urn_str.substr(start, end - start));
            start = end + 1;
        } else {
            parts.emplace_back(urn_str.substr(start));
        }
    }
    
    // Expected: ["urn", "themis", model, namespace, collection, uuid]
    if (parts.size() != 6) {
        return std::nullopt;
    }
    
    // Validate prefix
    if (parts[0] != "urn" || parts[1] != "themis") {
        return std::nullopt;
    }
    
    URN urn;
    urn.model = parts[2];
    urn.namespace_ = parts[3];
    urn.collection = parts[4];
    urn.uuid = parts[5];
    
    // Validate components
    if (!urn.isValidModel()) {
        return std::nullopt;
    }
    
    if (urn.namespace_.empty() || urn.collection.empty()) {
        return std::nullopt;
    }
    
    if (!urn.isValidUUID()) {
        return std::nullopt;
    }
    
    return urn;
}

/** @brief Serialize URN components into canonical urn:themis:... format. */
std::string URN::toString() const {
    std::ostringstream oss = {};
    oss << "urn:themis:" << model << ":" << namespace_ << ":" << collection << ":" << uuid;
    return oss.str();
}

/**
 * @brief Compute deterministic routing hash from UUID component.
 * @return 64-bit hash value.
 */
uint64_t URN::hash() const {
#ifdef HAS_XXHASH
    // Use xxHash for fast, high-quality hashing
    return XXH64(uuid.data(), uuid.size(), 0);
#else
    // Fallback to std::hash
    std::hash<std::string> hasher;
    return hasher(uuid);
#endif
}

/** @brief Validate uuid field against the RFC-4122 textual pattern. */
bool URN::isValidUUID() const {
    return std::regex_match(uuid, UUID_PATTERN);
}

/** @brief Validate model against the statically supported model list. */
bool URN::isValidModel() const {
    return std::find(VALID_MODELS.begin(), VALID_MODELS.end(), model) != VALID_MODELS.end();
}

} // namespace themis::sharding
