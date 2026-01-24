#pragma once

#include <string>
#include <memory>
#include <map>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Abstract tracer interface for distributed tracing.
 * 
 * Provides a unified interface for distributed tracing that can be
 * implemented by various tracing backends (OpenTelemetry, Jaeger, no-op, etc.).
 * Enables testing with mock tracers and runtime switching of implementations.
 */
class ITracer {
public:
    /**
     * @brief Represents an active trace span with RAII lifetime.
     */
    class ISpan {
    public:
        virtual ~ISpan() = default;

        virtual void setAttribute(const std::string& key, const std::string& value) = 0;
        virtual void setAttribute(const std::string& key, int64_t value) = 0;
        virtual void setAttribute(const std::string& key, double value) = 0;
        virtual void setAttribute(const std::string& key, bool value) = 0;
        
        virtual void recordError(const std::string& errorMessage) = 0;
        virtual void setStatus(bool ok, const std::string& description = "") = 0;
        virtual void end() = 0;
        virtual bool isValid() const = 0;
    };

    virtual ~ITracer() = default;

    // Span creation methods
    virtual std::unique_ptr<ISpan> startSpan(const std::string& name) = 0;
    virtual std::unique_ptr<ISpan> startChildSpan(const std::string& name, const ISpan& parent) = 0;

    // Initialization and cleanup
    virtual bool initialize(const std::string& serviceName, const std::string& endpoint) = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;
};

/**
 * @brief RAII helper for scoped spans.
 */
class ScopedSpan {
public:
    explicit ScopedSpan(ITracer& tracer, const std::string& name)
        : span_(tracer.startSpan(name)) {}

    void setAttribute(const std::string& key, const std::string& value) {
        if (span_) span_->setAttribute(key, value);
    }

    void setAttribute(const std::string& key, int64_t value) {
        if (span_) span_->setAttribute(key, value);
    }

    void setAttribute(const std::string& key, double value) {
        if (span_) span_->setAttribute(key, value);
    }

    void setAttribute(const std::string& key, bool value) {
        if (span_) span_->setAttribute(key, value);
    }

    void recordError(const std::string& errorMessage) {
        if (span_) span_->recordError(errorMessage);
    }

    void setStatus(bool ok, const std::string& description = "") {
        if (span_) span_->setStatus(ok, description);
    }

    ITracer::ISpan* span() { return span_.get(); }

private:
    std::unique_ptr<ITracer::ISpan> span_;
};

} // namespace concerns
} // namespace core
} // namespace themis
