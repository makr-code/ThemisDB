/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            philosophy_loader.cpp                              ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 04:24:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     271                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 87778519a4  2026-04-12  feat(ethics_ai): remove stubs — computed scoring, YAML fi... ║
    • 63cde823d4  2026-04-08  Add unit tests for Ethics AI and RAG Context Engine plugins ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "philosophy_loader.h"
#include <fstream>
#include <functional>
#include <sstream>
#include <filesystem>

#ifdef HAVE_YAML_CPP
#include <yaml-cpp/yaml.h>
#endif

namespace themis {
namespace plugins {
namespace ethics {

std::variant<size_t, Status> PhilosophyLoader::loadFromDirectory(const std::string& directory) {
    namespace fs = std::filesystem;
    
    if (!fs::exists(directory)) {
        return Status::Error("Directory does not exist: " + directory);
    }
    
    if (!fs::is_directory(directory)) {
        return Status::Error("Path is not a directory: " + directory);
    }
    
    size_t count = 0;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            auto path = entry.path();
            auto ext = path.extension().string();
            
            if (ext == ".yaml" || ext == ".yml") {
                auto status = loadFromFile(path.string());
                if (status.isOK()) {
                    count++;
                }
            }
        }
    }
    
    return count;
}

Status PhilosophyLoader::loadFromFile(const std::string& filepath) {
#ifdef HAVE_YAML_CPP
    try {
        YAML::Node config = YAML::LoadFile(filepath);
        
        PhilosophyProfile profile;
        profile.school_id = config["school_id"].as<std::string>("");
        profile.name = config["name"].as<std::string>("");
        
        if (profile.school_id.empty()) {
            // Use filename without extension as school_id
            namespace fs = std::filesystem;
            profile.school_id = fs::path(filepath).stem().string();
        }

        // ----------------------------------------------------------------
        // Helpers to extract a single string from a potentially complex node
        // ----------------------------------------------------------------
        auto extractText = [](const YAML::Node& node) -> std::string {
            if (!node) return "";
            if (node.IsScalar()) return node.as<std::string>("");
            if (node.IsMap()) {
                // Rich thesis object: prefer "description", then "name", then "text"
                for (const char* key : {"description", "name", "text"}) {
                    if (node[key] && node[key].IsScalar())
                        return node[key].as<std::string>("");
                }
                // Fallback: join all scalar leaf values
                std::string acc;
                for (const auto& kv : node) {
                    if (kv.second.IsScalar()) {
                        if (!acc.empty()) acc += "; ";
                        acc += kv.second.as<std::string>("");
                    }
                }
                return acc;
            }
            if (node.IsSequence() && node.size() > 0 && node[0].IsScalar())
                return node[0].as<std::string>("");
            return "";
        };

        auto extractPointText = [](const YAML::Node& node) -> std::string {
            if (!node) return "";
            if (node.IsScalar()) return node.as<std::string>("");
            if (node.IsMap()) {
                // Strength/weakness entries use "point" as the headline
                for (const char* key : {"point", "description", "name"}) {
                    if (node[key] && node[key].IsScalar())
                        return node[key].as<std::string>("");
                }
            }
            return "";
        };

        // Join a potentially nested sequence/scalar YAML value to a string
        std::function<std::string(const YAML::Node&)> joinNode;
        joinNode = [&joinNode](const YAML::Node& node) -> std::string {
            if (!node) return "";
            if (node.IsScalar()) return node.as<std::string>("");
            if (node.IsSequence()) {
                std::string acc;
                for (const auto& item : node) {
                    std::string s = joinNode(item);
                    if (!s.empty()) {
                        if (!acc.empty()) acc += "; ";
                        acc += s;
                    }
                }
                return acc;
            }
            if (node.IsMap()) {
                // Try common key names first, then join all scalar leaves
                for (const char* key : {"description", "name", "text", "procedure"}) {
                    if (node[key] && node[key].IsScalar())
                        return node[key].as<std::string>("");
                }
                std::string acc;
                for (const auto& kv : node) {
                    std::string s = joinNode(kv.second);
                    if (!s.empty()) {
                        if (!acc.empty()) acc += "; ";
                        acc += s;
                    }
                }
                return acc;
            }
            return "";
        };

        // Load main theses (may be plain strings or complex objects)
        if (config["main_theses"]) {
            for (const auto& thesis : config["main_theses"]) {
                std::string text = extractText(thesis);
                if (!text.empty()) profile.main_theses.push_back(text);
            }
        }
        
        // Load secondary theses (same handling as main_theses)
        if (config["secondary_theses"]) {
            for (const auto& thesis : config["secondary_theses"]) {
                std::string text = extractText(thesis);
                if (!text.empty()) profile.secondary_theses.push_back(text);
            }
        }
        
        // Load decision framework (may be a flat map or nested structure)
        if (config["decision_framework"]) {
            const auto& df = config["decision_framework"];
            if (df.IsMap()) {
                for (const auto& kv : df) {
                    std::string key = kv.first.as<std::string>("");
                    std::string val = joinNode(kv.second);
                    if (!key.empty()) profile.decision_framework[key] = val;
                }
            }
        }
        
        // Load strengths (may be plain strings or {point, elaboration} objects)
        if (config["strengths"]) {
            for (const auto& item : config["strengths"]) {
                std::string text = extractPointText(item);
                if (!text.empty()) profile.strengths.push_back(text);
            }
        }
        
        // Load weaknesses (same handling as strengths)
        if (config["weaknesses"]) {
            for (const auto& item : config["weaknesses"]) {
                std::string text = extractPointText(item);
                if (!text.empty()) profile.weaknesses.push_back(text);
            }
        }
        
        // Load internal debate
        if (config["internal_debate"]) {
            for (const auto& kv : config["internal_debate"]) {
                profile.internal_debate[kv.first.as<std::string>()] = 
                    joinNode(kv.second);
            }
        }
        
        // Load philosophical positioning
        if (config["philosophical_positioning"]) {
            for (const auto& kv : config["philosophical_positioning"]) {
                profile.philosophical_positioning[kv.first.as<std::string>()] = 
                    joinNode(kv.second);
            }
        }
        
        profiles_[profile.school_id] = profile;
        return Status::OK();
        
    } catch (const YAML::Exception& e) {
        return Status::Error("YAML parsing error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return Status::Error("Error loading file: " + std::string(e.what()));
    }
#else
    return Status::Error("YAML support not enabled (yaml-cpp not found)");
#endif
}

std::variant<PhilosophyProfile, Status> PhilosophyLoader::getProfile(
    const std::string& school_id) const {
    
    auto it = profiles_.find(school_id);
    if (it == profiles_.end()) {
        return Status::Error("Philosophy profile not found: " + school_id);
    }
    
    return it->second;
}

bool PhilosophyLoader::hasProfile(const std::string& school_id) const {
    return profiles_.find(school_id) != profiles_.end();
}

std::vector<std::string> PhilosophyLoader::getSchoolIds() const {
    std::vector<std::string> ids;
    ids.reserve(profiles_.size());
    
    for (const auto& kv : profiles_) {
        ids.push_back(kv.first);
    }
    
    return ids;
}

void PhilosophyLoader::clear() {
    profiles_.clear();
}

void PhilosophyLoader::addProfile(const PhilosophyProfile& profile) {
    profiles_[profile.school_id] = profile;
}

std::map<std::string, PhilosophyProfile> PhilosophyLoader::getAllProfiles() const {
    return profiles_;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
