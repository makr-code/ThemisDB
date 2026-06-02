/*
 * ThemisDB | File: security_signature.cpp | Version: 0.0.47 | Last Modified: 2026-05-24 14:31:17
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 68
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "storage/security_signature.h"
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <string_view>

namespace themis {
namespace storage {

using json = nlohmann::json;

namespace {

constexpr std::size_t kSha256HexLength = 64;

bool isHexLowerString(std::string_view value) {
    if (value.size() != kSha256HexLength) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0 || (ch >= 'a' && ch <= 'f');
    });
}

bool isSupportedAlgorithm(const std::string& algorithm) {
    return algorithm == "sha256";
}

bool isValidResourceId(const std::string& resource_id) {
    return !resource_id.empty() &&
           resource_id.find('\0') == std::string::npos;
}

} // namespace

nlohmann::json SecuritySignature::toJson() const {
    json j;
    j["resource_id"] = resource_id;
    j["hash"] = hash;
    j["algorithm"] = algorithm;
    j["created_at"] = created_at;
    if (!created_by.empty()) {
        j["created_by"] = created_by;
    }
    if (!comment.empty()) {
        j["comment"] = comment;
    }
    return j;
}

std::optional<SecuritySignature> SecuritySignature::fromJson(const nlohmann::json& j) {
    try {
        SecuritySignature sig;
        sig.resource_id = j.at("resource_id").get<std::string>();
        sig.hash = j.at("hash").get<std::string>();
        sig.algorithm = j.at("algorithm").get<std::string>();
        sig.created_at = j.at("created_at").get<uint64_t>();

        if (!isValidResourceId(sig.resource_id) ||
            !isHexLowerString(sig.hash) ||
            !isSupportedAlgorithm(sig.algorithm)) {
            return std::nullopt;
        }
        
        if (j.contains("created_by")) {
            sig.created_by = j["created_by"].get<std::string>();
        }
        if (j.contains("comment")) {
            sig.comment = j["comment"].get<std::string>();
        }
        
        return sig;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::string SecuritySignature::serialize() const {
    return toJson().dump();
}

std::optional<SecuritySignature> SecuritySignature::deserialize(const std::string& data) {
    try {
        json j = json::parse(data);
        return fromJson(j);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

} // namespace storage
} // namespace themis
