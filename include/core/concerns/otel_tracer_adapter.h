#pragma once

#include "core/concerns/i_tracer.h"
#include "utils/tracing.h"

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief OpenTelemetry adapter implementation of ITracer.
 * 
 * Wraps the existing OpenTelemetry-based tracer to implement the ITracer interface.
 */
class OpenTelemetryTracerAdapter : public ITracer {
public:
    class OtelSpanAdapter : public ISpan {
    public:
        explicit OtelSpanAdapter(themis::Tracer::Span&& span)
            : span_(std::move(span)) {}

        void setAttribute(const std::string& key, const std::string& value) override {
            span_.setAttribute(key, value);
        }

        void setAttribute(const std::string& key, int64_t value) override {
            span_.setAttribute(key, value);
        }

        void setAttribute(const std::string& key, double value) override {
            span_.setAttribute(key, value);
        }

        void setAttribute(const std::string& key, bool value) override {
            span_.setAttribute(key, value);
        }

        void recordError(const std::string& errorMessage) override {
            span_.recordError(errorMessage);
        }

        void setStatus(bool ok, const std::string& description = "") override {
            span_.setStatus(ok, description);
        }

        void end() override {
            span_.end();
        }

        bool isValid() const override {
            return span_.isValid();
        }

        themis::Tracer::Span& getSpan() { return span_; }

    private:
        themis::Tracer::Span span_;
    };

    std::unique_ptr<ISpan> startSpan(const std::string& name) override {
        return std::make_unique<OtelSpanAdapter>(themis::Tracer::startSpan(name));
    }

    std::unique_ptr<ISpan> startChildSpan(const std::string& name, const ISpan& parent) override {
        auto* otelParent = dynamic_cast<const OtelSpanAdapter*>(&parent);
        if (otelParent) {
            return std::make_unique<OtelSpanAdapter>(
                themis::Tracer::startChildSpan(name, const_cast<OtelSpanAdapter*>(otelParent)->getSpan())
            );
        }
        return std::make_unique<OtelSpanAdapter>(themis::Tracer::startSpan(name));
    }

    bool initialize(const std::string& serviceName, const std::string& endpoint) override {
        initialized_ = themis::Tracer::initialize(serviceName, endpoint);
        return initialized_;
    }

    void shutdown() override {
        themis::Tracer::shutdown();
        initialized_ = false;
    }

    bool isInitialized() const override {
        return initialized_;
    }

private:
    bool initialized_ = false;
};

} // namespace concerns
} // namespace core
} // namespace themis
