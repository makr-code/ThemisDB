/**
 * @file prompt_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=9; TODO=1, Stub=7, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/prompt_manager.h"
#include <stdexcept>
#include "storage/rocksdb_wrapper.h"
#include "metadata/schema_manager.h"
#include "utils/logger.h"
#include <yaml-cpp/yaml.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstring>
#include <regex>
#include <filesystem>

namespace themis {
namespace prompt_engineering {

PromptManager::PromptManager() = default;

PromptManager::PromptManager(RocksDBWrapper* db, rocksdb::ColumnFamilyHandle* cf)
    : db_(db), cf_(cf) {}

PromptManager::ValidationResult PromptManager::validateTemplate(const PromptTemplate& t) {
    ValidationResult result;

    // Required: non-empty name
    if (t.name.empty()) {
        result.errors.push_back("Template 'name' must not be empty");
    }

    // Required: non-empty content
    if (t.content.empty()) {
        result.errors.push_back("Template 'content' must not be empty");
    }

    // Required: non-empty version
    if (t.version.empty()) {
        result.errors.push_back("Template 'version' must not be empty");
    }

    // Warn if description is missing
    if (t.description.empty()) {
        result.warnings.push_back("Template 'description' is empty – consider adding one");
    }

    // Validate image descriptions
    for (size_t i = 0; i < t.images.size(); ++i) {
        if (t.images[i].alt_text.empty()) {
            result.errors.push_back(
                "Image[" + std::to_string(i) + "] 'alt_text' must not be empty");
        }
    }

    // Validate metadata is object or null (not a raw scalar/array)
    if (!t.metadata.is_null() && !t.metadata.is_object()) {
        result.errors.push_back("Template 'metadata' must be a JSON object");
    }

    result.valid = result.errors.empty();
    return result;
}

PromptManager::PromptTemplate PromptManager::createTemplate(PromptManager::PromptTemplate t) {
    // Validate before inserting
    auto vr = validateTemplate(t);
    if (!vr.valid) {
        for (const auto& err : vr.errors) {
            THEMIS_ERROR("Template validation error: {}", err);
        }
        // Return a sentinel template with an empty id to indicate failure
        return PromptTemplate{};
    }
    for (const auto& warn : vr.warnings) {
        THEMIS_WARN("Template validation warning: {}", warn);
    }

    // v1.1.0: Lock-free concurrent hash map (no explicit lock needed)
    if (t.id.empty()) {
      t.id = generateId();
    }
    
    // Insert using TBB concurrent_hash_map (efficient single operation)
    StoreType::accessor acc;
    store_.insert(acc, {t.id, t});
    acc.release(); // Release lock

    // Persist if DB configured
    if (db_) {
        std::string key = std::string(KEY_PREFIX) + t.id;
        std::string v = t.toJson().dump();
        std::vector<uint8_t> bytes(v.begin(), v.end());
        bool ok = db_->put(key, bytes);
        if (!ok) {
            THEMIS_ERROR("Failed to persist prompt template {}", t.id);
        }
    }

    THEMIS_DEBUG("Created prompt template {} (version={})", t.id, t.version);
    return t;
}

std::optional<PromptManager::PromptTemplate> PromptManager::getTemplate(const std::string& id) const {
    // If persisted, try DB first
    if (db_) {
        std::string key = std::string(KEY_PREFIX) + id;
        auto val_opt = db_->get(key);
        if (val_opt.has_value()) {
            try {
                std::string s(reinterpret_cast<const char*>(val_opt->data()), val_opt->size());
                auto j = nlohmann::json::parse(s);
                    PromptTemplate t;
                // The PromptTemplate::fromJson isn't defined here; parse manually
                t.id = j.value("id", id);
                t.name = j.value("name", "");
                t.version = j.value("version", "");
                t.content = j.value("content", "");
                t.description = j.value("description", "");
                if (j.contains("metadata")) {
                  t.metadata = j["metadata"];
                }
                t.active = j.value("active", true);
                if (j.contains("images") && j["images"].is_array()) {
                    for (const auto& img_j : j["images"]) {
                        t.images.push_back(ImageDescription::fromJson(img_j));
                    }
                }
                return t;
            } catch (const std::exception& e) {
                THEMIS_WARN("Failed to parse persisted prompt template {}: {}", id, e.what());
            }
        }
        // fallthrough to in-memory
    }

    // v1.1.0: Lock-free read with const_accessor
    StoreType::const_accessor acc;
    if (store_.find(acc, id)) {
        return acc->second;
    }
    return std::nullopt;
}

std::vector<PromptManager::PromptTemplate> PromptManager::listTemplates() const {
    // If persisted, scan DB
    if (db_) {
        std::vector<PromptTemplate> out;
        std::string prefix = KEY_PREFIX;
        db_->scanPrefix(prefix, [&]([[maybe_unused]] std::string_view key, std::string_view value) -> bool {
            // silence unused parameter warning
            try {
                auto j = nlohmann::json::parse(std::string(value));
                PromptTemplate t;
                t.id = j.value("id", "");
                t.name = j.value("name", "");
                t.version = j.value("version", "");
                t.content = j.value("content", "");
                t.description = j.value("description", "");
                if (j.contains("metadata")) {
                  t.metadata = j["metadata"];
                }
                t.active = j.value("active", true);
                if (j.contains("images") && j["images"].is_array()) {
                    for (const auto& img_j : j["images"]) {
                        t.images.push_back(ImageDescription::fromJson(img_j));
                    }
                }
                out.push_back(t);
            } catch (const std::exception& e) {
                THEMIS_WARN("Failed to parse prompt template during scan: {}", e.what());
            }
            return true; // continue scanning
        });
        return out;
    }

    // v1.1.0: Iterate over concurrent hash map (snapshot iteration)
    std::vector<PromptTemplate> out = {};

    out.reserve(store_.size());
    for (const auto& kv : store_) {
        out.push_back(kv.second);
    }
    return out;
}

bool PromptManager::updateTemplate(const std::string& id, const nlohmann::json& metadata, bool active) {
    // v1.1.0: Update using accessor for thread-safe modification
    StoreType::accessor acc;
    if (!store_.find(acc, id)) {
        return false;
    }
    
    acc->second.metadata = metadata;
    acc->second.active = active;

    if (db_) {
        std::string key = std::string(KEY_PREFIX) + id;
        nlohmann::json j = acc->second.toJson();
        std::string s = j.dump();
        std::vector<uint8_t> bytes(s.begin(), s.end());
        if (!db_->put(key, bytes)) {
            THEMIS_ERROR("Failed to persist updated template {}", id);
        }
    }

    THEMIS_DEBUG("Updated prompt template {} active={} metadata={}", id, active, metadata.dump());
    return true;
}

bool PromptManager::assignExperiment(const std::string& id, const std::string& experiment_id) {
    // v1.1.0: Update using accessor for thread-safe modification
    StoreType::accessor acc;
    if (!store_.find(acc, id)) {
        return false;
    }
    
    acc->second.metadata["experiment_id"] = experiment_id;

    if (db_) {
        std::string key = std::string(KEY_PREFIX) + id;
        nlohmann::json j = acc->second.toJson();
        std::string s = j.dump();
        std::vector<uint8_t> bytes(s.begin(), s.end());
        if (!db_->put(key, bytes)) {
            THEMIS_ERROR("Failed to persist experiment assignment for {}", id);
        }
    }

    THEMIS_DEBUG("Assigned experiment {} to template {}", experiment_id, id);
    return true;
}

std::string PromptManager::generateId() const {
    static thread_local std::mt19937_64 gen((std::random_device())());
    static std::uniform_int_distribution<uint64_t> dis;
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0')
        << std::setw(12) << ms
        << "-"
        << std::setw(16) << dis(gen);
    return oss.str();
}

size_t PromptManager::loadFromYAML(const std::string& yaml_path) {
    try {
        if (!std::filesystem::exists(yaml_path)) {
            THEMIS_WARN("YAML prompt file not found: {}", yaml_path);
            return 0;
        }

        YAML::Node config = YAML::LoadFile(yaml_path);
        
        if (!config["prompts"]) {
            THEMIS_WARN("No 'prompts' section found in {}", yaml_path);
            return 0;
        }

        size_t loaded = 0;
        const YAML::Node& prompts = config["prompts"];
        
        for (YAML::const_iterator it = prompts.begin(); it != prompts.end(); ++it) {
            std::string prompt_id = it->first.as<std::string>();
            const YAML::Node& prompt_node = it->second;
            
            PromptTemplate pt;
            pt.id = prompt_id;
            pt.name = prompt_node["name"].as<std::string>("");
            pt.version = prompt_node["version"].as<std::string>("1.0");
            pt.content = prompt_node["content"].as<std::string>("");
            pt.description = prompt_node["description"].as<std::string>("");
            pt.active = prompt_node["active"].as<bool>(true);
            
            // Load metadata if present
            if (prompt_node["metadata"]) {
                try {
                    // Convert YAML node to JSON string then parse
                    YAML::Emitter emitter;
                    emitter << prompt_node["metadata"];
                    pt.metadata = nlohmann::json::parse(emitter.c_str());
                } catch (...) {
                    pt.metadata = nlohmann::json::object();
                }
            }

            // Load image descriptions if present
            if (prompt_node["images"] && prompt_node["images"].IsSequence()) {
                for (const auto& img_node : prompt_node["images"]) {
                    ImageDescription img;
                    img.alt_text    = img_node["alt_text"].as<std::string>("");
                    img.url         = img_node["url"].as<std::string>("");
                    img.description = img_node["description"].as<std::string>("");
                    img.mime_type   = img_node["mime_type"].as<std::string>("image/jpeg");
                    pt.images.push_back(std::move(img));
                }
            }
            
            // Validate before creating
            auto vr = validateTemplate(pt);
            if (!vr.valid) {
                for (const auto& err : vr.errors) {
                    THEMIS_ERROR("Template '{}' validation error: {}", prompt_id, err);
                }
                // Skip invalid templates
                continue;
            }
            for (const auto& warn : vr.warnings) {
                THEMIS_WARN("Template '{}' validation warning: {}", prompt_id, warn);
            }

            createTemplate(pt);
            loaded++;
            
            THEMIS_DEBUG("Loaded prompt '{}' ({})", pt.name, prompt_id);
        }
        
        THEMIS_INFO("Loaded {} prompts from {}", loaded, yaml_path);
        return loaded;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load prompts from YAML {}: {}", yaml_path, e.what());
        return 0;
    }
}

std::string PromptManager::injectContext(
    const std::string& template_str,
    const std::unordered_map<std::string, std::string>& context) const {
    
    std::string result = template_str;
    
    // Replace all {variable} patterns with context values
    for (const auto& [key, value] : context) {
        std::string placeholder = "{" + key + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }
    
    return result;
}

std::optional<std::string> PromptManager::getPromptWithContext(
    const std::string& id,
    const std::unordered_map<std::string, std::string>& context) const {
    
    auto template_opt = getTemplate(id);
    if (!template_opt.has_value()) {
        return std::nullopt;
    }
    
    if (!template_opt->active) {
        THEMIS_WARN("Prompt '{}' is inactive", id);
        return std::nullopt;
    }
    
    return injectContext(template_opt->content, context);
}

std::unordered_map<std::string, std::string> PromptManager::buildContextFromSchema(
    SchemaManager* schema_mgr,
    const std::string& edition,
    const std::string& version) {
    
    std::unordered_map<std::string, std::string> context;
    
    // Basic info
    context["version"] = version;
    context["edition"] = edition;
    
    if (!schema_mgr) {
        context["table_count"] = "0";
        context["total_rows"] = "0";
        context["tables"] = "[]";
        context["capabilities"] = "[]";
        context["schema"] = "{}";
        return context;
    }
    
    try {
        // Get database metadata
        auto metadata = schema_mgr->getDatabaseMetadata();
        context["table_count"] = std::to_string(metadata.table_count);
        context["total_rows"] = std::to_string(metadata.total_rows);
        
        // Get capabilities as JSON array string
        nlohmann::json caps_array = nlohmann::json::array();
        for (const auto& cap : metadata.capabilities) {
            caps_array.push_back(cap);
        }
        context["capabilities"] = caps_array.dump();
        
        // Get all tables
        auto tables = schema_mgr->getAllTables();
        nlohmann::json tables_array = nlohmann::json::array();
        for (const auto& table : tables) {
            nlohmann::json table_info;
            table_info["name"] = table.name;
            table_info["type"] = table.type;
            table_info["row_count"] = table.estimated_row_count;
            tables_array.push_back(table_info);
        }
        context["tables"] = tables_array.dump(2);
        
        // Get full schema (may be large, so we'll limit it)
        auto schema_json = schema_mgr->toJSON();
        std::string schema_str = schema_json.dump(2);
        
        // Limit schema size to avoid context overflow
        const size_t MAX_SCHEMA_LENGTH = 10000;
        if (schema_str.length() > MAX_SCHEMA_LENGTH) {
            schema_str = schema_str.substr(0, MAX_SCHEMA_LENGTH) + "\n... (truncated)";
        }
        context["schema"] = schema_str;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to build context from schema: {}", e.what());
    }
    
    return context;
}

std::string PromptManager::buildMultiModalPrompt(
    const PromptTemplate& t,
    const std::unordered_map<std::string, std::string>& context) {

    // Inject context variables into the template text
    std::string result = t.content;
    for (const auto& [key, value] : context) {
        std::string placeholder = "{" + key + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }

    // Append structured image-description block when images are present
    if (!t.images.empty()) {
        result += "\n\n[Images]\n";
        for (size_t i = 0; i < t.images.size(); ++i) {
            const auto& img = t.images[i];
            const std::string& mime = img.mime_type.empty() ? "image/jpeg" : img.mime_type;
            result += std::to_string(i + 1) + ". [" + mime + "] " + img.alt_text + "\n";
            if (!img.description.empty()) {
                result += "   Description: " + img.description + "\n";
            }
            if (!img.url.empty()) {
                result += "   URL: " + img.url + "\n";
            }
        }
    }

    return result;
}

} // namespace prompt_engineering
} // namespace themis

