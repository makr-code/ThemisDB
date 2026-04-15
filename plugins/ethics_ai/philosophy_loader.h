/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            philosophy_loader.h                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     108                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "plugins/ethics_ai/ethics_ai_types.h"
#include <map>
#include <string>
#include <memory>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Philosophy Profile Loader
 * 
 * Loads and manages philosophy profiles from YAML files.
 * Provides caching and validation of philosophy definitions.
 */
class PhilosophyLoader {
public:
    PhilosophyLoader() = default;
    ~PhilosophyLoader() = default;
    
    /**
     * @brief Load philosophy profiles from a directory
     * @param directory Path to directory containing YAML files
     * @return Number of profiles loaded or error
     */
    std::variant<size_t, Status> loadFromDirectory(const std::string& directory);
    
    /**
     * @brief Load a single philosophy profile from file
     * @param filepath Path to YAML file
     * @return Status indicating success/failure
     */
    Status loadFromFile(const std::string& filepath);
    
    /**
     * @brief Get a philosophy profile by ID
     * @param school_id School identifier
     * @return Profile or error
     */
    std::variant<PhilosophyProfile, Status> getProfile(const std::string& school_id) const;
    
    /**
     * @brief Check if a profile is loaded
     * @param school_id School identifier
     * @return true if profile exists
     */
    bool hasProfile(const std::string& school_id) const;
    
    /**
     * @brief Get list of all loaded school IDs
     * @return Vector of school IDs
     */
    std::vector<std::string> getSchoolIds() const;
    
    /**
     * @brief Clear all loaded profiles
     */
    void clear();
    
    /**
     * @brief Get count of loaded profiles
     * @return Number of profiles
     */
    size_t count() const { return profiles_.size(); }
    
    /**
     * @brief Get all loaded philosophy profiles
     * @return Map of school_id -> PhilosophyProfile
     * 
     * Returns all philosophy profiles loaded by this loader.
     * Used by plugin to register profiles with EthicalGuidelinesManager.
     */
    std::map<std::string, PhilosophyProfile> getAllProfiles() const;
    
private:
    std::map<std::string, PhilosophyProfile> profiles_;
    
    // Helper to parse YAML content
    Status parseYAML(const std::string& content, PhilosophyProfile& profile);
};

} // namespace ethics
} // namespace plugins
} // namespace themis
