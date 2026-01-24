#pragma once

#include <string>
#include <memory>
#include <utility>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Abstract logger interface for dependency injection.
 * 
 * Provides a unified logging interface that can be implemented by various
 * logging backends (spdlog, console, no-op, etc.). Enables testing with
 * mock loggers and runtime switching of logging implementations.
 */
class ILogger {
public:
    enum class Level {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        CRITICAL
    };

    virtual ~ILogger() = default;

    // Core logging methods
    virtual void log(Level level, const std::string& message) = 0;
    
    virtual void trace(const std::string& message) = 0;
    virtual void debug(const std::string& message) = 0;
    virtual void info(const std::string& message) = 0;
    virtual void warn(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
    virtual void critical(const std::string& message) = 0;

    // Configuration methods
    virtual void setLevel(Level level) = 0;
    virtual Level getLevel() const = 0;
    virtual void setPattern(const std::string& pattern) = 0;

    // Helper methods
    static Level levelFromString(const std::string& level);
    static const char* levelToString(Level level);
};

} // namespace concerns
} // namespace core
} // namespace themis
