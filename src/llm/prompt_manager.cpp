/**
 * @file prompt_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/prompt_manager.h"
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

PromptManager::PromptManager() = default;

PromptManager::PromptManager(RocksDBWrapper* db, rocksdb::ColumnFamilyHandle* cf)
    : db_(db), cf_(cf) {}

PromptManager::PromptTemplate PromptManager::createTemplate(PromptManager::PromptTemplate t) {
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
                if (j.contains("metadata")) {
                  t.metadata = j["metadata"];
                }
                t.active = j.value("active", true);
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
        db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) -> bool {
            try {
                auto j = nlohmann::json::parse(std::string(value));
                PromptTemplate t;
                t.id = j.value("id", "");
                t.name = j.value("name", "");
                t.version = j.value("version", "");
                t.content = j.value("content", "");
                if (j.contains("metadata")) {
                  t.metadata = j["metadata"];
                }
                t.active = j.value("active", true);
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
                    THEMIS_DEBUG("prompt_manager: unhandled exception caught");
                    pt.metadata = nlohmann::json::object();
                }
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

} // namespace themis

