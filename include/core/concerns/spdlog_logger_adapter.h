#pragma once

#include "core/concerns/i_logger.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <memory>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Spdlog adapter implementation of ILogger.
 * 
 * Wraps the existing spdlog-based logger to implement the ILogger interface.
 */
class SpdlogLoggerAdapter : public ILogger {
public:
    explicit SpdlogLoggerAdapter(std::shared_ptr<spdlog::logger> logger = nullptr)
        : logger_(logger ? logger : utils::Logger::get()) {}

    void log(Level level, const std::string& message) override {
        switch (level) {
            case Level::TRACE: trace(message); break;
            case Level::DEBUG: debug(message); break;
            case Level::INFO: info(message); break;
            case Level::WARN: warn(message); break;
            case Level::ERROR: error(message); break;
            case Level::CRITICAL: critical(message); break;
        }
    }

    void trace(const std::string& message) override {
        if (logger_) logger_->trace(message);
    }

    void debug(const std::string& message) override {
        if (logger_) logger_->debug(message);
    }

    void info(const std::string& message) override {
        if (logger_) logger_->info(message);
    }

    void warn(const std::string& message) override {
        if (logger_) logger_->warn(message);
    }

    void error(const std::string& message) override {
        if (logger_) logger_->error(message);
    }

    void critical(const std::string& message) override {
        if (logger_) logger_->critical(message);
    }

    void setLevel(Level level) override {
        if (!logger_) return;
        switch (level) {
            case Level::TRACE: logger_->set_level(spdlog::level::trace); break;
            case Level::DEBUG: logger_->set_level(spdlog::level::debug); break;
            case Level::INFO: logger_->set_level(spdlog::level::info); break;
            case Level::WARN: logger_->set_level(spdlog::level::warn); break;
            case Level::ERROR: logger_->set_level(spdlog::level::err); break;
            case Level::CRITICAL: logger_->set_level(spdlog::level::critical); break;
        }
    }

    Level getLevel() const override {
        if (!logger_) return Level::INFO;
        switch (logger_->level()) {
            case spdlog::level::trace: return Level::TRACE;
            case spdlog::level::debug: return Level::DEBUG;
            case spdlog::level::info: return Level::INFO;
            case spdlog::level::warn: return Level::WARN;
            case spdlog::level::err: return Level::ERROR;
            case spdlog::level::critical: return Level::CRITICAL;
            default: return Level::INFO;
        }
    }

    void setPattern(const std::string& pattern) override {
        if (logger_) logger_->set_pattern(pattern);
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};

} // namespace concerns
} // namespace core
} // namespace themis
