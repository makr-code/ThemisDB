/**
 * @file pii_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/pii_detector.h"
#include <stdexcept>
#include "utils/error_contracts.h"
#include "utils/pii_detection_engine.h"
#include "config/config_path_resolver.h"
#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace {

bool isBooleanLiteral(const std::string& value, bool& parsed) {
    std::string normalized;
    normalized.reserve(value.size());
    for (const unsigned char ch : value) {
        normalized.push_back(static_cast<char>(std::tolower(ch)));
    }

    if (normalized == "true") {
        parsed = true;
        return true;
    }

    if (normalized == "false") {
        parsed = false;
        return true;
    }

    return false;
}

bool tryParseInt(const std::string& value, int& parsed) {
    const char* begin = value.data();
    const char* end = begin + value.size();
    auto [ptr, ec] = std::from_chars(begin, end, parsed);
    return ec == std::errc{} && ptr == end;
}

bool tryParseDouble(const std::string& value, double& parsed) {
    char* end_ptr = nullptr;
    const double parsed_value = std::strtod(value.c_str(), &end_ptr);
    if (end_ptr != value.c_str() + value.size()) {
        return false;
    }

    parsed = parsed_value;
    return true;
}

} // namespace

namespace themis::utils {

PIIDetector::PIIDetector(std::string config_path,
                         std::shared_ptr<VCCPKIClient> pki_client)
    : config_path_(std::move(config_path))
    , pki_client_(std::move(pki_client))
    , default_redaction_mode_("strict") {
    
    // Try loading from YAML
    if (!loadFromYaml(config_path_)) {
        spdlog::warn("PIIDetector: Failed to load {}, using embedded defaults", config_path_);
        spdlog::warn("PIIDetector: Error: {}", last_error_);
        initializeDefaultEngine();
    }
    
    spdlog::info("PIIDetector: Initialized with {} engine(s)", engines_.size());
}

bool PIIDetector::reload(const std::string& config_path) {
    std::scoped_lock lock(mutex_);
    
    std::string path = config_path.empty() ? config_path_ : config_path;
    
    // Backup current engines
    auto old_engines = std::move(engines_);
    
    last_error_.clear();
    
    if (!loadFromYaml(path)) {
        // Restore old engines on failure
        engines_ = std::move(old_engines);
        spdlog::error("PIIDetector: Reload failed, retained previous engines. Error: {}", last_error_);
        return false;
    }
    
    if (!config_path.empty()) {
        config_path_ = config_path;
    }
    
    spdlog::info("PIIDetector: Reloaded {} engine(s) from {}", engines_.size(), path);
    return true;
}

void PIIDetector::setPKIClient(std::shared_ptr<VCCPKIClient> pki_client) {
    std::scoped_lock lock(mutex_);
    pki_client_ = pki_client;
}

bool PIIDetector::isPKIVerificationEnabled() const {
    std::scoped_lock lock(mutex_);
    return pki_client_ != nullptr;
}

std::string PIIDetector::getLastError() const {
    std::scoped_lock lock(mutex_);
    return last_error_;
}

std::vector<PIIFinding> PIIDetector::detectInText(const std::string& text) const {
    std::scoped_lock lock(mutex_);
    
    std::vector<PIIFinding> all_findings;
    
    // Run all enabled engines
    for (const auto& engine : engines_) {
        if (!engine->isEnabled()) {
            continue;
        }
        
        try {
            auto engine_findings = engine->detectInText(text);
            all_findings.insert(all_findings.end(), 
                              engine_findings.begin(), engine_findings.end());
        } catch (const std::exception& e) {
            spdlog::error("PIIDetector: Engine '{}' threw exception: {}", 
                         engine->getName(), e.what());
        }
    }
    
    // Deduplicate and sort
    return deduplicateFindings(std::move(all_findings));
}

std::unordered_map<std::string, std::vector<PIIFinding>> PIIDetector::detectInJson(
    const nlohmann::json& json_obj) const {
    // Wichtig: Kein globaler Lock während der Rekursion halten, da detectInText/classifyFieldName intern sperren
    std::unordered_map<std::string, std::vector<PIIFinding>> result;
    scanJsonRecursive(json_obj, "", result);
    return result;
}

PIIType PIIDetector::classifyFieldName(const std::string& field_name) const {
    std::scoped_lock lock(mutex_);
    
    // Query all engines, return first non-UNKNOWN result
    for (const auto& engine : engines_) {
        if (!engine->isEnabled()) {
            continue;
        }
        
        PIIType type = engine->classifyFieldName(field_name);
        if (type != PIIType::UNKNOWN) {
            return type;
        }
    }
    
    return PIIType::UNKNOWN;
}

std::string PIIDetector::getRedactionRecommendation(PIIType type) const {
    std::scoped_lock lock(mutex_);
    
    // Query first enabled engine
    for (const auto& engine : engines_) {
        if (!engine->isEnabled()) {
            continue;
        }
        
        std::string mode = engine->getRedactionRecommendation(type);
        if (mode != "none" && mode != default_redaction_mode_) {
            return mode;
        }
    }
    
    return default_redaction_mode_;
}

std::string PIIDetector::maskValue(PIIType type, const std::string& value) const {
    std::string mode = getRedactionRecommendation(type);
    return PIITypeUtils::maskValue(type, value, mode);
}

std::vector<std::string> PIIDetector::getEnabledEngines() const {
    std::scoped_lock lock(mutex_);
    
    std::vector<std::string> enabled;
    for (const auto& engine : engines_) {
        if (engine->isEnabled()) {
            enabled.push_back(engine->getName());
        }
    }
    
    return enabled;
}

nlohmann::json PIIDetector::getEngineMetadata() const {
    std::scoped_lock lock(mutex_);

    nlohmann::json metadata = {
        {"total_engines", engines_.size()},
        {"enabled_engines", 0},
        {"pki_verification_enabled", pki_client_ != nullptr},
        {"engines", nlohmann::json::array()}
    };

    int enabled_engines = 0;
    
    for (const auto& engine : engines_) {
        if (engine->isEnabled()) {
            ++enabled_engines;
        }

        metadata.at("engines").push_back(engine->getMetadata());
    }

    metadata.at("enabled_engines") = enabled_engines;

    return metadata;
}

bool PIIDetector::loadFromYaml(const std::string& path) {
    try {
        // Use ConfigPathResolver to handle both new and legacy paths
        std::string resolved;
        auto maybe_resolved = themis::config::ConfigPathResolver::tryResolve(path);
        if (maybe_resolved) {
            resolved = *maybe_resolved;
        } else {
            // If the provided path doesn't point to an existing file, and is
            // relative, try walking up a few parent directories to find a
            // repository-level `config/` directory (useful when running tests
            // from the `build/` directory).
            resolved = path;
            if (!std::filesystem::exists(resolved)) {
                if (!std::filesystem::path(resolved).is_absolute()) {
                    std::filesystem::path cur = std::filesystem::current_path();
                    [[maybe_unused]] bool found = false;
                    // Try up to 4 levels up
                    for (int i = 0; i < 4; ++i) {
                        std::filesystem::path candidate = cur;
                        for (int j = 0; j < i; ++j) {
                            candidate = candidate.parent_path();
                        }
                        candidate /= resolved;
                        if (std::filesystem::exists(candidate)) {
                            resolved = candidate.string();
                            found = true;
                            break;
                        }
                    }
                }
            }
        }

        YAML::Node config = YAML::LoadFile(resolved);
        
        // Load global settings
        if (config["global_settings"]) {
            auto settings = config["global_settings"];
            if (settings["default_redaction_mode"]) {
                default_redaction_mode_ = settings["default_redaction_mode"].as<std::string>();
            }
        }
        
        // Load detection engines
        if (!config["detection_engines"]) {
            last_error_ = "No 'detection_engines' section found in YAML";
            return false;
        }
        
        bool loaded_any = false;
        
        for (const auto& engine_node : config["detection_engines"]) {
            // Convert YAML to JSON for engine loading
            nlohmann::json engine_config;
            
            // Recursive YAML->JSON conversion
            std::function<void(const YAML::Node&, nlohmann::json&)> convert_yaml_to_json =
                [&](const YAML::Node& yaml_node, nlohmann::json& json_node) {
                    if (yaml_node.IsScalar()) {
                        const std::string scalar = yaml_node.Scalar();

                        bool bool_value = false;
                        if (isBooleanLiteral(scalar, bool_value)) {
                            json_node = bool_value;
                            return;
                        }

                        int int_value = 0;
                        if (tryParseInt(scalar, int_value)) {
                            json_node = int_value;
                            return;
                        }

                        double double_value = 0.0;
                        if (tryParseDouble(scalar, double_value)) {
                            json_node = double_value;
                            return;
                        }

                        json_node = scalar;
                    } else if (yaml_node.IsSequence()) {
                        json_node = nlohmann::json::array();
                        for (const auto& item : yaml_node) {
                            nlohmann::json item_json;
                            convert_yaml_to_json(item, item_json);
                            json_node.push_back(item_json);
                        }
                    } else if (yaml_node.IsMap()) {
                        json_node = nlohmann::json::object();
                        for (const auto& kv : yaml_node) {
                            auto key = kv.first.as<std::string>();
                            nlohmann::json value_json;
                            convert_yaml_to_json(kv.second, value_json);
                            json_node[key] = value_json;
                        }
                    }
                };
            
            convert_yaml_to_json(engine_node, engine_config);
            
            // Try to load and verify engine
            if (verifyAndLoadEngine(engine_config)) {
                loaded_any = true;
            }
        }
        
        if (!loaded_any) {
            last_error_ = "No engines successfully loaded from YAML";
            return false;
        }
        
        return true;
        
    } catch (const YAML::Exception& e) {
        last_error_ = std::string("YAML parse error: ") + e.what();
        return false;
    } catch (const std::exception& e) {
        last_error_ = std::string("Error loading YAML: ") + e.what();
        return false;
    }
}

void PIIDetector::initializeDefaultEngine() {
    engines_.clear();

    nlohmann::json default_config;
    default_config["enabled"] = true;

    // Create unsigned regex engine with embedded defaults
    auto regex_result = PIIDetectionEngineFactory::createUnsigned("regex");
    if (!regex_result) {
        spdlog::error("PIIDetector: Failed to create default regex engine: {}",
                     regex_result.error().message());
    } else {
        auto regex_engine = std::move(*regex_result);
        if (!regex_engine->initialize(default_config)) {
            spdlog::error("PIIDetector: Failed to initialize default regex engine: {}",
                         regex_engine->getLastError());
        } else {
            engines_.push_back(std::move(regex_engine));
        }
    }

    // Create unsigned NER engine with embedded defaults (complements regex for
    // person names, organizations, and locations that regex cannot detect)
    auto ner_result = PIIDetectionEngineFactory::createUnsigned("ner");
    if (!ner_result) {
        spdlog::error("PIIDetector: Failed to create default NER engine: {}",
                     ner_result.error().message());
    } else {
        auto ner_engine = std::move(*ner_result);
        if (!ner_engine->initialize(default_config)) {
            spdlog::error("PIIDetector: Failed to initialize default NER engine: {}",
                         ner_engine->getLastError());
        } else {
            engines_.push_back(std::move(ner_engine));
        }
    }

    spdlog::info("PIIDetector: Initialized with {} embedded unsigned engine(s)", engines_.size());
}

bool PIIDetector::verifyAndLoadEngine(const nlohmann::json& engine_config) {
    std::string engine_type;
    
    try {
        engine_type = engine_config.value("type", "");
        if (engine_type.empty()) {
            spdlog::warn("PIIDetector: Engine config missing 'type' field");
            return false;
        }
        
        bool enabled = engine_config.value("enabled", false);
        if (!enabled) {
            spdlog::info("PIIDetector: Skipping disabled engine '{}'", engine_type);
            return false;
        }
        
        std::unique_ptr<IPIIDetectionEngine> engine;
        
        // If PKI client is configured, use signed loading
        if (pki_client_) {
            auto engine_result = themis::utils::PIIDetectionEngineFactory::createSigned(
                engine_type, engine_config, *pki_client_);
            
            if (!engine_result) {
                spdlog::error("PIIDetector: PKI verification failed for '{}': {}", 
                             engine_type, engine_result.error().message());
                
                // Check if fallback to unsigned is allowed
                auto global_settings = engine_config.value("global_settings", nlohmann::json::object());
                bool allow_fallback = global_settings.value("allow_embedded_fallback", true);
                
                if (allow_fallback && engine_type == "regex") {
                    spdlog::warn("PIIDetector: Falling back to unsigned regex engine");
                    auto unsigned_result = themis::utils::PIIDetectionEngineFactory::createUnsigned(engine_type);
                    if (unsigned_result) {
                        engine = std::move(unsigned_result.value());
                        if (!engine->initialize(engine_config)) {
                            spdlog::error(
                                "PIIDetector: Fallback engine '{}' initialization failed: {}",
                                engine_type,
                                engine->getLastError());
                            return false;
                        }
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            } else {
                engine = std::move(engine_result.value());
            }
        } else {
            // No PKI client - load unsigned (expected for local/dev setups)
            spdlog::info("PIIDetector: Loading engine '{}' without PKI verification", engine_type);
            auto engine_result = themis::utils::PIIDetectionEngineFactory::createUnsigned(engine_type);
            
            if (!engine_result) {
                spdlog::error("PIIDetector: Failed to create engine '{}': {}", 
                             engine_type, engine_result.error().message());
                return false;
            }
            
            engine = std::move(engine_result.value());
            
            if (!engine->initialize(engine_config)) {
                spdlog::error("PIIDetector: Engine '{}' initialization failed: {}", 
                             engine_type, engine->getLastError());
                return false;
            }
        }
        
        if (engine) {
            spdlog::info("PIIDetector: Loaded engine '{}' v{}", 
                        engine->getName(), engine->getVersion());
            engines_.push_back(std::move(engine));
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        spdlog::error("PIIDetector: Exception loading engine '{}': {}", engine_type, e.what());
        return false;
    }
}

void PIIDetector::scanJsonRecursive(
    const nlohmann::json& obj,
    const std::string& path,
    std::unordered_map<std::string, std::vector<PIIFinding>>& findings) const {
    
    if (obj.is_object()) {
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            const auto& key = it.key();
            std::string new_path = path.empty() ? key : path + "." + key;
            
            // Check field name for PII hints
            PIIType field_type = classifyFieldName(key);
            if (field_type != PIIType::UNKNOWN && it.value().is_string()) {
                PIIFinding finding;
                finding.type = field_type;
                finding.value = it.value().get<std::string>();
                finding.start_offset = 0;
                finding.end_offset = finding.value.length();
                finding.confidence = 0.85; // Field name heuristic confidence
                finding.pattern_name = PIITypeUtils::toString(field_type) + "_FIELD_HINT";
                finding.engine_name = "field_hint";
                auto [path_it, inserted] = findings.try_emplace(new_path);
                path_it->second.push_back(std::move(finding));
            }
            
            // Recurse for nested content
            if (it.value().is_object() || it.value().is_array()) {
                scanJsonRecursive(it.value(), new_path, findings);
            } else if (it.value().is_string()) {
                // Scan string values for PII patterns
                std::string value = it.value().get<std::string>();
                auto text_findings = detectInText(value);
                if (!text_findings.empty()) {
                    auto [path_it, inserted] = findings.try_emplace(new_path);
                    path_it->second.insert(path_it->second.end(),
                                           text_findings.begin(),
                                           text_findings.end());
                }
            }
        }
    } else if (obj.is_array()) {
        for (size_t i = 0; i < obj.size(); ++i) {
            std::string new_path = path + "[" + std::to_string(i) + "]";
            scanJsonRecursive(obj.at(i), new_path, findings);
        }
    } else if (obj.is_string()) {
        std::string value = obj.get<std::string>();
        auto text_findings = detectInText(value);
        if (!text_findings.empty()) {
            auto [path_it, inserted] = findings.try_emplace(path);
            path_it->second.insert(path_it->second.end(),
                                   text_findings.begin(),
                                   text_findings.end());
        }
    }
}

std::vector<PIIFinding> PIIDetector::deduplicateFindings(
    std::vector<PIIFinding> findings) {
    
    if (findings.size() <= 1) {
        return findings;
    }
    
    // Sort by offset
    std::ranges::sort(findings,
                      [](const PIIFinding& a, const PIIFinding& b) {
                          return a.start_offset < b.start_offset;
                      });
    
    // Remove overlapping findings (keep higher confidence)
    std::vector<PIIFinding> deduplicated;
    deduplicated.push_back(findings.front());
    
    for (size_t i = 1; i < findings.size(); ++i) {
        const auto& prev = deduplicated.back();
        const auto& curr = findings.at(i);
        
        // Check for overlap
        if (curr.start_offset < prev.end_offset) {
            // Prefer non-PHONE over PHONE when overlapping
            if (prev.type == PIIType::PHONE && curr.type != PIIType::PHONE) {
                deduplicated.back() = curr;
            } else if (curr.type == PIIType::PHONE && prev.type != PIIType::PHONE) {
                // keep prev
            } else {
                // Fall back to higher confidence
                if (curr.confidence > prev.confidence) {
                    deduplicated.back() = curr;
                }
            }
        } else {
            // No overlap - add to result
            deduplicated.push_back(curr);
        }
    }
    
    return deduplicated;
}

} // namespace themis::utils

