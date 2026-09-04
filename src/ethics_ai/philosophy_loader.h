/**
 * @file philosophy_loader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    size_t count() const { return static_cast<int>(profiles_.size()); }
    
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
