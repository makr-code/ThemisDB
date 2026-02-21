/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            config_errors.h                                    ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     128                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace config {

/**
 * Base exception for all config-related errors.
 */
class ConfigException : public std::exception {
public:
    explicit ConfigException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
    
protected:
    std::string message_;
};

/**
 * Thrown when a config file is not found in any expected location.
 */
class ConfigNotFoundException : public ConfigException {
public:
    ConfigNotFoundException(const std::string& path, 
                           const std::vector<std::string>& attempted_paths)
        : ConfigException(buildMessage(path, attempted_paths)),
          requested_path_(path),
          attempted_paths_(attempted_paths) {}
    
    const std::string& requested_path() const { return requested_path_; }
    const std::vector<std::string>& attempted_paths() const { return attempted_paths_; }
    
private:
    std::string requested_path_;
    std::vector<std::string> attempted_paths_;
    
    static std::string buildMessage(const std::string& path,
                                    const std::vector<std::string>& attempted) {
        std::string msg = "Config file not found: " + path;
        if (!attempted.empty()) {
            msg += "\nAttempted paths:";
            for (const auto& p : attempted) {
                msg += "\n  - " + p;
            }
        }
        return msg;
    }
};

/**
 * Thrown when a path mapping is not found in the mapping table.
 */
class MappingNotFoundException : public ConfigException {
public:
    explicit MappingNotFoundException(const std::string& path)
        : ConfigException("No mapping found for legacy path: " + path),
          legacy_path_(path) {}
    
    const std::string& legacy_path() const { return legacy_path_; }
    
private:
    std::string legacy_path_;
};

/**
 * Thrown when a path fails validation (e.g., path traversal attempt).
 */
class InvalidPathException : public ConfigException {
public:
    InvalidPathException(const std::string& path, const std::string& reason)
        : ConfigException("Invalid config path: " + path + " (" + reason + ")"),
          invalid_path_(path),
          reason_(reason) {}
    
    const std::string& invalid_path() const { return invalid_path_; }
    const std::string& reason() const { return reason_; }
    
private:
    std::string invalid_path_;
    std::string reason_;
};

/**
 * Thrown when config file permissions are insufficient.
 */
class ConfigPermissionException : public ConfigException {
public:
    explicit ConfigPermissionException(const std::string& path)
        : ConfigException("Permission denied accessing config: " + path),
          config_path_(path) {}
    
    const std::string& config_path() const { return config_path_; }
    
private:
    std::string config_path_;
};

} // namespace config
} // namespace themis
