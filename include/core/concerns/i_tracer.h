/**
 * @file i_tracer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 89/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/lifecycle.h"
#include <string>
#include <memory>
#include <map>

namespace themis {
namespace core {
namespace concerns {

class ITracer {
public:
    class ISpan {
    public:
        /**
         * @brief ISpan.
         * @return Return value.
         */
        virtual ~ISpan() = default;

        /**
         * @brief Set Attribute.
         * @param[in] key Input parameter.
         * @param[in] value Input parameter.
         */
        virtual void setAttribute(const std::string& key, const std::string& value) = 0;

        /**
         * @brief Set Attribute.
         * @param[in] key Input parameter.
         * @param[in] value Input parameter.
         * @details Calls: std::string().
         */
        void setAttribute(const std::string& key, const char* value) {
            setAttribute(key, std::string(value != nullptr ? value : ""));
        }

        /**
         * @brief Set Attribute.
         * @param[in] key Input parameter.
         * @param[in] value Input parameter.
         */
        virtual void setAttribute(const std::string& key, int64_t value) = 0;

        /**
         * @brief Set Attribute.
         * @param[in] key Input parameter.
         * @param[in] value Input parameter.
         */
        virtual void setAttribute(const std::string& key, double value) = 0;

        /**
         * @brief Set Attribute.
         * @param[in] key Input parameter.
         * @param[in] value Input parameter.
         */
        virtual void setAttribute(const std::string& key, bool value) = 0;

        /**
         * @brief Record Error.
         * @param[in] errorMessage Input parameter.
         */
        virtual void recordError(const std::string& errorMessage) = 0;

        virtual void setStatus(bool ok, const std::string& description = "") = 0;

        /**
         * @brief End.
         */
        virtual void end() = 0;

        [[nodiscard]] virtual bool isValid() const = 0;
    };

    /**
     * @brief ITracer.
     * @return Return value.
     */
    virtual ~ITracer() = default;

    // -----------------------------------------------------------------------
    // Span creation methods
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual std::unique_ptr<ISpan> startSpan(const std::string& name) = 0;

    [[nodiscard]] virtual std::unique_ptr<ISpan> startChildSpan(const std::string& name, const ISpan& parent) = 0;

    virtual std::unique_ptr<ISpan> startSpanFromHeaders(
            const std::string& name,
            const std::map<std::string, std::string>& headers) {
        (void)headers;
        return startSpan(name);
    }

    virtual void injectContext(std::map<std::string, std::string>& headers) {
        (void)headers;
    }

    // -----------------------------------------------------------------------
    // Initialization and cleanup
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual bool initialize(const std::string& serviceName, const std::string& endpoint) = 0;

    /**
     * @brief Shutdown.
     */
    virtual void shutdown() = 0;

    [[nodiscard]] virtual bool isInitialized() const = 0;

    // -----------------------------------------------------------------------
    // Lifecycle hooks
    // -----------------------------------------------------------------------

    virtual void flush() noexcept {}

    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

class ScopedSpan {
public:
    /**
     * @brief Scoped Span.
     * @param[in,out] tracer Input/output parameter.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    explicit ScopedSpan(ITracer& tracer, const std::string& name)
        : span_(tracer.startSpan(name)) {}

    /**
     * @brief Set Attribute.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @details Implements setAttribute without additional internal calls.
     */
    void setAttribute(const std::string& key, const std::string& value) {
        if (span_) {
          span_->setAttribute(key, value);
        }
    }

    /**
     * @brief Set Attribute.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @details Implements setAttribute without additional internal calls.
     */
    void setAttribute(const std::string& key, int64_t value) {
        if (span_) {
          span_->setAttribute(key, value);
        }
    }

    /**
     * @brief Set Attribute.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @details Implements setAttribute without additional internal calls.
     */
    void setAttribute(const std::string& key, double value) {
        if (span_) {
          span_->setAttribute(key, value);
        }
    }

    /**
     * @brief Set Attribute.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @details Implements setAttribute without additional internal calls.
     */
    void setAttribute(const std::string& key, bool value) {
        if (span_) {
          span_->setAttribute(key, value);
        }
    }

    /**
     * @brief Record Error.
     * @param[in] errorMessage Input parameter.
     * @details Implements recordError without additional internal calls.
     */
    void recordError(const std::string& errorMessage) {
        if (span_) {
          span_->recordError(errorMessage);
        }
    }

    void setStatus(bool ok, const std::string& description = "") {
        if (span_) {
          span_->setStatus(ok, description);
        }
    }

    /**
     * @brief Span.
     * @return Pointer to the result.
     * @details Calls: get().
     */
    ITracer::ISpan* span() { return span_.get(); }

    ~ScopedSpan() {
        if (span_) {
          span_->end();
        }
    }

private:
    std::unique_ptr<ITracer::ISpan> span_;
};

} // namespace concerns
} // namespace core
} // namespace themis
