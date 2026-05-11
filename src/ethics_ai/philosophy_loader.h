/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            philosophy_loader.h                                ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:48:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     117                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 32f246a038  2026-04-08  feat(ethics_ai): enhance plugin configuration and overrid... ║
    • 63cde823d4  2026-04-08  Add unit tests for Ethics AI and RAG Context Engine plugins ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include <map>
#include <mutex>
#include <string>
#include <memory>
#include <variant>

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
     * @brief Hot-reload profiles from a directory without stopping the server.
     *
     * Atomically re-scans @p directory: loads all YAML profiles, then swaps
     * the internal profile map under the loader's write lock.  Profiles that
     * could not be parsed are skipped; the old map is left intact if the
     * directory is empty or does not exist.
     *
     * @param directory Path to directory containing YAML files.
     * @return Number of profiles now loaded, or Status::Error on failure.
     */
    std::variant<size_t, Status> reloadProfiles(const std::string& directory);

    /**
     * @brief Register a profile directly (used for testing / plugin registration)
     * @param profile The profile to register
     */
    void addProfile(const PhilosophyProfile& profile);

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
    mutable std::mutex mutex_;
    std::map<std::string, PhilosophyProfile> profiles_;
    
    // Helper to parse YAML content
    Status parseYAML(const std::string& content, PhilosophyProfile& profile);
};

} // namespace ethics
} // namespace plugins
} // namespace themis
