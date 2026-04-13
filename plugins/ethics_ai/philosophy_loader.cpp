/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            philosophy_loader.cpp                              ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:28:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     181                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "philosophy_loader.h"
#include <fstream>
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
        
        // Load main theses
        if (config["main_theses"]) {
            for (const auto& thesis : config["main_theses"]) {
                profile.main_theses.push_back(thesis.as<std::string>());
            }
        }
        
        // Load secondary theses
        if (config["secondary_theses"]) {
            for (const auto& thesis : config["secondary_theses"]) {
                profile.secondary_theses.push_back(thesis.as<std::string>());
            }
        }
        
        // Load decision framework
        if (config["decision_framework"]) {
            for (const auto& kv : config["decision_framework"]) {
                profile.decision_framework[kv.first.as<std::string>()] = 
                    kv.second.as<std::string>();
            }
        }
        
        // Load strengths
        if (config["strengths"]) {
            for (const auto& strength : config["strengths"]) {
                profile.strengths.push_back(strength.as<std::string>());
            }
        }
        
        // Load weaknesses
        if (config["weaknesses"]) {
            for (const auto& weakness : config["weaknesses"]) {
                profile.weaknesses.push_back(weakness.as<std::string>());
            }
        }
        
        // Load internal debate
        if (config["internal_debate"]) {
            for (const auto& kv : config["internal_debate"]) {
                profile.internal_debate[kv.first.as<std::string>()] = 
                    kv.second.as<std::string>();
            }
        }
        
        // Load philosophical positioning
        if (config["philosophical_positioning"]) {
            for (const auto& kv : config["philosophical_positioning"]) {
                profile.philosophical_positioning[kv.first.as<std::string>()] = 
                    kv.second.as<std::string>();
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

std::map<std::string, PhilosophyProfile> PhilosophyLoader::getAllProfiles() const {
    return profiles_;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
