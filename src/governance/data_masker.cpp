/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            data_masker.cpp                                    ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-14 11:33:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     230                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • dcf7b458fc  2026-02-27  feat(cmake): add transaction_retry_manager and other sour... ║
    • 8d2569bddf  2026-02-26  fix(governance): code audit – data race, maskFieldsArray ... ║
    • ffc2b43f84  2026-02-26  feat(governance): automated data masking for sensitive fi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "governance/data_masker.h"

#include <iomanip>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>

#include "observability/metrics_collector.h"
#include "utils/logger.h"

namespace themis {
namespace governance {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Convert a raw byte buffer to a lowercase hex string.
std::string toHex(const unsigned char *data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(data[i]);
    }
    return oss.str();
}

/// Compute SHA-256 hex digest of @p input.
std::string sha256Hex(const std::string &input) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(input.data()), input.size(), digest);
    return toHex(digest, SHA256_DIGEST_LENGTH);
}

/// Compute HMAC-SHA256 of @p input keyed by @p key; return hex string.
/// Returns empty string on error (key must be non-empty).
std::string hmacSha256Hex(const std::string &key, const std::string &input) {
    if (key.empty()) {
        THEMIS_WARN("DataMasker::TOKENIZE strategy called with empty collection_secret; "
                    "falling back to SHA-256 (no HMAC) - pseudonym stability across collections is lost");
        return sha256Hex(input); // Fallback: unkeyed hash when secret is absent
    }
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len     = 0;
    const unsigned char *result = HMAC(
        EVP_sha256(), reinterpret_cast<const unsigned char *>(key.data()), static_cast<int>(key.size()),
        reinterpret_cast<const unsigned char *>(input.data()), static_cast<int>(input.size()), digest, &digest_len);
    if (!result) {
        THEMIS_ERROR("DataMasker: OpenSSL HMAC-SHA256 failed; falling back to SHA-256 (no HMAC) - "
                     "pseudonym stability across collections is lost");
        return sha256Hex(input); // Fallback on OpenSSL error
    }
    return toHex(digest, digest_len);
}

/// Return the Prometheus strategy label string for a given strategy enum.
const char *strategyLabel(MaskingStrategy s) {
    switch (s) {
        case MaskingStrategy::REDACT:
            return "redact";
        case MaskingStrategy::TOKENIZE:
            return "tokenize";
        case MaskingStrategy::TRUNCATE:
            return "truncate";
        case MaskingStrategy::HASH:
            return "hash";
    }
    return "unknown";
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// DataMasker::applyStrategy
// ---------------------------------------------------------------------------

std::string DataMasker::applyStrategy(const std::string &value, const FieldMaskingRule &rule) {
    switch (rule.strategy) {
        case MaskingStrategy::REDACT:
            return "[REDACTED]";

        case MaskingStrategy::TOKENIZE: {
            const std::string hex = hmacSha256Hex(rule.collection_secret, value);
            return "tkn_" + hex;
        }

        case MaskingStrategy::TRUNCATE: {
            const int n = (rule.truncate_length > 0) ? rule.truncate_length : 4;
            if (static_cast<int>(value.size()) <= n) {
                return value; // Value is already short – no truncation needed
            }
            return value.substr(0, static_cast<size_t>(n)) + "...";
        }

        case MaskingStrategy::HASH: {
            const std::string hex = sha256Hex(value);
            return "sha_" + hex;
        }
    }
    return "[REDACTED]"; // Defensive fallback
}

// ---------------------------------------------------------------------------
// DataMasker::maskNode  (private)
// ---------------------------------------------------------------------------

nlohmann::json DataMasker::maskNode(const nlohmann::json &node, const std::string &key,
                                    const RuleIndex &rule_index) const {
    if (node.is_object()) {
        nlohmann::json out = nlohmann::json::object();
        for (auto it = node.begin(); it != node.end(); ++it) {
            out[it.key()] = maskNode(it.value(), it.key(), rule_index);
        }
        return out;
    }

    if (node.is_array()) {
        nlohmann::json out = nlohmann::json::array();
        for (const auto &elem : node) {
            out.push_back(maskNode(elem, key, rule_index));
        }
        return out;
    }

    if (node.is_string()) {
        auto it = rule_index.find(key);
        if (it != rule_index.end()) {
            const FieldMaskingRule &rule = *(it->second);
            const std::string original   = node.get<std::string>();
            const std::string masked     = applyStrategy(original, rule);

            // Emit Prometheus counter (governance_fields_masked_total)
            observability::MetricsCollector::getInstance().addCounter("governance_fields_masked_total", 1,
                                                                      {{"strategy", strategyLabel(rule.strategy)}});

            THEMIS_DEBUG("DataMasker: masked field '{}' with strategy '{}'", key, strategyLabel(rule.strategy));

            return masked;
        }
    }

    // Non-string scalars and unmatched string fields pass through unchanged.
    return node;
}

// ---------------------------------------------------------------------------
// DataMasker::maskFields
// ---------------------------------------------------------------------------

nlohmann::json DataMasker::maskFields(const nlohmann::json &doc, const FieldMaskingPolicy &policy) const {
    if (!policy.enabled || policy.rules.empty()) {
        return doc;
    }

    // Build a field-name → rule pointer index for O(1) per-key lookup.
    // If multiple rules declare the same field_name, the first one wins.
    RuleIndex rule_index;
    rule_index.reserve(policy.rules.size());
    for (const auto &rule : policy.rules) {
        if (!rule.field_name.empty()) {
            rule_index.emplace(rule.field_name, &rule); // first entry wins
        }
    }

    return maskNode(doc, "", rule_index);
}

// ---------------------------------------------------------------------------
// DataMasker::maskFieldsArray
// ---------------------------------------------------------------------------

nlohmann::json DataMasker::maskFieldsArray(const nlohmann::json &docs, const FieldMaskingPolicy &policy) const {
    if (!policy.enabled || policy.rules.empty()) {
        return docs;
    }

    if (!docs.is_array()) {
        return maskFields(docs, policy);
    }

    // Build the field-name → rule index once for the entire array so that the
    // O(rules) hash-map construction is not repeated for every document.
    // rule_index stores pointers into policy.rules; policy is valid for the
    // entire call so all pointers remain live throughout the loop.
    RuleIndex rule_index;
    rule_index.reserve(policy.rules.size());
    for (const auto &rule : policy.rules) {
        if (!rule.field_name.empty()) {
            rule_index.emplace(rule.field_name, &rule); // first entry wins
        }
    }

    nlohmann::json out = nlohmann::json::array();
    for (const auto &doc : docs) {
        out.push_back(maskNode(doc, "", rule_index));
    }
    return out;
}

} // namespace governance
} // namespace themis
