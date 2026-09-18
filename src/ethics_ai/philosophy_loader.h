/**
 * @file philosophy_loader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class PhilosophyLoader {
public:
    PhilosophyLoader() = default;
    ~PhilosophyLoader() = default;
    
    std::variant<size_t, Status> loadFromDirectory(const std::string& directory);
    
    /**
     * @brief Load From File.
     * @param[in] filepath Input parameter.
     * @return Return value.
     */
    Status loadFromFile(const std::string& filepath);
    
    std::variant<PhilosophyProfile, Status> getProfile(const std::string& school_id) const;
    
    /**
     * @brief Has Profile.
     * @param[in] school_id Identifier of the school.
     * @return True when the operation succeeds.
     */
    bool hasProfile(const std::string& school_id) const;
    
    /**
     * @brief Get School Ids.
     * @return Return value.
     */
    std::vector<std::string> getSchoolIds() const;
    
    /**
     * @brief Clear.
     */
    void clear();
    

    std::variant<size_t, Status> reloadProfiles(const std::string& directory);

    /**
     * @brief Add Profile.
     * @param[in] profile Input parameter.
     */
    void addProfile(const PhilosophyProfile& profile);

    size_t count() const { return static_cast<int>(profiles_.size()); }
    
    std::map<std::string, PhilosophyProfile> getAllProfiles() const;
    
private:
    mutable std::mutex mutex_;
    std::map<std::string, PhilosophyProfile> profiles_;
    
    /**
     * @brief Helper to parse YAML content
     * @param[in] content Input parameter.
     * @param[in,out] profile Input/output parameter.
     * @return Return value.
     */
    Status parseYAML(const std::string& content, PhilosophyProfile& profile);
};

} // namespace ethics
} // namespace plugins
} // namespace themis
