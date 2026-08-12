/**
 * @file prompt_library_io.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/prompt_library_io.h"
#include "utils/hash_util.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include <yaml-cpp/yaml.h>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

std::string toHex16(std::uint64_t v) {
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << v;
    return ss.str();
}

// Convert a system_clock time_point to a Unix timestamp (seconds).
std::int64_t toUnixTime(std::chrono::system_clock::time_point tp) {
    return static_cast<std::int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            tp.time_since_epoch())
        .count());
}

std::chrono::system_clock::time_point fromUnixTime(std::int64_t t) {
    return std::chrono::system_clock::time_point(std::chrono::seconds(t));
}

// Build a PromptTemplate from a YAML::Node.
PromptManager::PromptTemplate templateFromYaml(const YAML::Node& node) {
    PromptManager::PromptTemplate t;
    t.id          = node["id"]          ? node["id"].as<std::string>()          : "";
    t.name        = node["name"]        ? node["name"].as<std::string>()        : "";
    t.version     = node["version"]     ? node["version"].as<std::string>()     : "";
    t.content     = node["content"]     ? node["content"].as<std::string>()     : "";
    t.description = node["description"] ? node["description"].as<std::string>() : "";
    t.active      = node["active"]      ? node["active"].as<bool>()             : true;

    if (node["metadata"] && node["metadata"].IsScalar()) {
        try {
            t.metadata = nlohmann::json::parse(
                node["metadata"].as<std::string>());
        } catch (...) {
            t.metadata = nlohmann::json::object();
        }
    }

    if (node["images"] && node["images"].IsSequence()) {
        for (const auto& img_node : node["images"]) {
            PromptManager::ImageDescription img;
            img.url         = img_node["url"]         ? img_node["url"].as<std::string>()         : "";
            img.alt_text    = img_node["alt_text"]    ? img_node["alt_text"].as<std::string>()    : "";
            img.description = img_node["description"] ? img_node["description"].as<std::string>() : "";
            img.mime_type   = img_node["mime_type"]   ? img_node["mime_type"].as<std::string>()   : "image/jpeg";
            t.images.push_back(img);
        }
    }
    return t;
}

} // anonymous namespace

// ============================================================================
// PromptLibraryBundle
// ============================================================================

nlohmann::json PromptLibraryBundle::toJson() const {
    nlohmann::json j;
    j["name"]           = name;
    j["description"]    = description;
    j["version"]        = version;
    j["format_version"] = format_version;
    j["created_at"]     = toUnixTime(created_at);
    j["checksum"]       = checksum;

    nlohmann::json tpls = nlohmann::json::array();
    for (const auto& t : templates) {
        tpls.push_back(t.toJson());
    }
    j["templates"] = std::move(tpls);
    return j;
}

PromptLibraryBundle PromptLibraryBundle::fromJson(const nlohmann::json& j) {
    PromptLibraryBundle b;
    b.name           = j.value("name",           std::string{});
    b.description    = j.value("description",    std::string{});
    b.version        = j.value("version",        std::string{});
    b.format_version = j.value("format_version", std::string{"1.0"});
    b.checksum       = j.value("checksum",       std::string{});
    b.created_at     = fromUnixTime(j.value("created_at", std::int64_t{0}));

    if (j.contains("templates") && j["templates"].is_array()) {
        for (const auto& jt : j["templates"]) {
            PromptManager::PromptTemplate t;
            t.id          = jt.value("id",          std::string{});
            t.name        = jt.value("name",        std::string{});
            t.version     = jt.value("version",     std::string{});
            t.content     = jt.value("content",     std::string{});
            t.description = jt.value("description", std::string{});
            t.active      = jt.value("active",      true);
            if (jt.contains("metadata")) {
                t.metadata = jt["metadata"];
            }
            if (jt.contains("images") && jt["images"].is_array()) {
                for (const auto& ji : jt["images"]) {
                    PromptManager::ImageDescription img;
                    img.url         = ji.value("url",         std::string{});
                    img.alt_text    = ji.value("alt_text",    std::string{});
                    img.description = ji.value("description", std::string{});
                    img.mime_type   = ji.value("mime_type",   std::string{"image/jpeg"});
                    t.images.push_back(img);
                }
            }
            b.templates.push_back(std::move(t));
        }
    }
    return b;
}

// ============================================================================
// PromptLibraryIO — Checksum
// ============================================================================

std::string PromptLibraryIO::computeChecksum(
        const PromptLibraryBundle& bundle) {
    // Collect canonical JSON strings, sort by id for determinism.
    std::vector<std::string> parts;
    parts.reserve(bundle.templates.size());
    for (const auto& t : bundle.templates) {
        parts.push_back(t.toJson().dump());
    }
    std::sort(parts.begin(), parts.end());

    std::string concat;
    for (const auto& p : parts) { concat += p; }

    return toHex16(themis::hash::fnv1a64(concat));
}

bool PromptLibraryIO::verifyChecksum(const PromptLibraryBundle& bundle) {
    return bundle.checksum == computeChecksum(bundle);
}

// ============================================================================
// PromptLibraryIO — Export
// ============================================================================

std::string PromptLibraryIO::exportToJson(PromptLibraryBundle bundle) {
    if (bundle.checksum.empty()) {
        bundle.checksum = computeChecksum(bundle);
    }
    if (bundle.created_at == std::chrono::system_clock::time_point{}) {
        bundle.created_at = std::chrono::system_clock::now();
    }
    return bundle.toJson().dump(2);
}

std::string PromptLibraryIO::exportToYaml(PromptLibraryBundle bundle) {
    if (bundle.checksum.empty()) {
        bundle.checksum = computeChecksum(bundle);
    }
    if (bundle.created_at == std::chrono::system_clock::time_point{}) {
        bundle.created_at = std::chrono::system_clock::now();
    }

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "name"           << YAML::Value << bundle.name;
    out << YAML::Key << "description"    << YAML::Value << bundle.description;
    out << YAML::Key << "version"        << YAML::Value << bundle.version;
    out << YAML::Key << "format_version" << YAML::Value << bundle.format_version;
    out << YAML::Key << "created_at"     << YAML::Value << toUnixTime(bundle.created_at);
    out << YAML::Key << "checksum"       << YAML::Value << bundle.checksum;
    out << YAML::Key << "templates"      << YAML::Value << YAML::BeginSeq;

    for (const auto& t : bundle.templates) {
        out << YAML::BeginMap;
        out << YAML::Key << "id"          << YAML::Value << t.id;
        out << YAML::Key << "name"        << YAML::Value << t.name;
        out << YAML::Key << "version"     << YAML::Value << t.version;
        out << YAML::Key << "content"     << YAML::Value << t.content;
        out << YAML::Key << "description" << YAML::Value << t.description;
        out << YAML::Key << "active"      << YAML::Value << t.active;
        out << YAML::Key << "metadata"    << YAML::Value << t.metadata.dump();
        out << YAML::Key << "images"      << YAML::Value << YAML::BeginSeq;
        for (const auto& img : t.images) {
            out << YAML::BeginMap;
            out << YAML::Key << "url"         << YAML::Value << img.url;
            out << YAML::Key << "alt_text"    << YAML::Value << img.alt_text;
            out << YAML::Key << "description" << YAML::Value << img.description;
            out << YAML::Key << "mime_type"   << YAML::Value << img.mime_type;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

bool PromptLibraryIO::isYamlPath(const std::string& path) noexcept {
    try {
        std::filesystem::path p(path);
        const std::string ext = p.extension().string();
        return ext == ".yaml" || ext == ".yml";
    } catch (...) {
        return false;
    }
}

ExportResult PromptLibraryIO::exportToFile(PromptLibraryBundle bundle,
                                            const std::string&  path,
                                            ExportFormat        fmt) {
    ExportResult result;
    result.templates_written = bundle.templates.size();

    try {
        const bool use_yaml =
            (fmt == ExportFormat::YAML) || isYamlPath(path);

        const std::string content =
            use_yaml ? exportToYaml(bundle) : exportToJson(bundle);

        std::ofstream ofs(path, std::ios::out | std::ios::trunc);
        if (!ofs.is_open()) {
            result.error_message =
                "Cannot open file for writing: " + path;
            return result;
        }
        ofs << content;
        if (ofs.fail()) {
            result.error_message = "Write error: " + path;
            return result;
        }
        result.success = true;
    } catch (const std::exception& e) {
        result.error_message = std::string("exportToFile: ") + e.what();
    } catch (...) {
        result.error_message = "exportToFile: unknown error";
    }
    return result;
}

// ============================================================================
// PromptLibraryIO — Import
// ============================================================================

std::optional<PromptLibraryBundle> PromptLibraryIO::importFromJson(
        const std::string& json_str) {
    if (json_str.empty()) { return std::nullopt; }
    try {
        const auto j = nlohmann::json::parse(json_str);
        return PromptLibraryBundle::fromJson(j);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<PromptLibraryBundle> PromptLibraryIO::importFromYaml(
        const std::string& yaml_str) {
    if (yaml_str.empty()) { return std::nullopt; }
    try {
        const YAML::Node root = YAML::Load(yaml_str);
        if (!root.IsMap()) { return std::nullopt; }

        PromptLibraryBundle b;
        b.name           = root["name"]           ? root["name"].as<std::string>()           : "";
        b.description    = root["description"]    ? root["description"].as<std::string>()    : "";
        b.version        = root["version"]        ? root["version"].as<std::string>()        : "";
        b.format_version = root["format_version"] ? root["format_version"].as<std::string>() : "1.0";
        b.checksum       = root["checksum"]       ? root["checksum"].as<std::string>()       : "";
        if (root["created_at"]) {
            b.created_at = fromUnixTime(root["created_at"].as<std::int64_t>());
        }

        if (root["templates"] && root["templates"].IsSequence()) {
            for (const auto& node : root["templates"]) {
                b.templates.push_back(templateFromYaml(node));
            }
        }
        return b;
    } catch (...) {
        return std::nullopt;
    }
}

ImportResult PromptLibraryIO::importFromFile(const std::string&   path,
                                              PromptLibraryBundle& out_bundle) {
    ImportResult result;
    try {
        if (!std::filesystem::exists(path)) {
            result.error_message = "File not found: " + path;
            return result;
        }

        std::ifstream ifs(path);
        if (!ifs.is_open()) {
            result.error_message = "Cannot open file: " + path;
            return result;
        }
        const std::string content(
            (std::istreambuf_iterator<char>(ifs)),
            std::istreambuf_iterator<char>());

        std::optional<PromptLibraryBundle> opt;
        if (isYamlPath(path)) {
            opt = importFromYaml(content);
        } else {
            opt = importFromJson(content);
        }

        if (!opt.has_value()) {
            result.error_message = "Failed to parse bundle from: " + path;
            return result;
        }

        out_bundle = std::move(*opt);
        result.templates_loaded = out_bundle.templates.size();
        result.checksum_valid   = verifyChecksum(out_bundle);
        result.success          = true;
    } catch (const std::exception& e) {
        result.error_message = std::string("importFromFile: ") + e.what();
    } catch (...) {
        result.error_message = "importFromFile: unknown error";
    }
    return result;
}

// ============================================================================
// PromptLibraryIO — templateFromYamlNode (public bridge)
// ============================================================================

PromptManager::PromptTemplate PromptLibraryIO::templateFromYamlNode(
        const void* node_ptr) {
    if (!node_ptr) {
        return PromptManager::PromptTemplate{};
    }
    return templateFromYaml(
        *static_cast<const YAML::Node*>(node_ptr));
}

} // namespace prompt_engineering
} // namespace themis

