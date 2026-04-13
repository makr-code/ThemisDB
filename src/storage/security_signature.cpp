/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            security_signature.cpp                             ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:36:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     81                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "storage/security_signature.h"
#include <stdexcept>

namespace themis {
namespace storage {

using json = nlohmann::json;

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
