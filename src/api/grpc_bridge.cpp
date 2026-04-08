/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_bridge.cpp                                    ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-08                                         ║
  Author:          Copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "api/grpc_bridge.h"
#include "utils/logger.h"

#include <shared_mutex>
#include <stdexcept>
#include <atomic>

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// GrpcBridgeImpl — concrete implementation of IGRPCBridge
// ---------------------------------------------------------------------------

/**
 * @brief Concrete gRPC bridge that routes inbound gRPC requests to
 *        `IHttpHandler` implementations.
 *
 * ### Thread safety
 * `registerService()` is guarded by an exclusive lock; it is expected to be
 * called only during server initialisation before any `dispatch()` calls.
 * `dispatch()` acquires a shared lock so that multiple threads can route
 * requests concurrently without blocking each other.
 */
class GrpcBridgeImpl final : public IGRPCBridge {
public:
    GrpcBridgeImpl() = default;
    ~GrpcBridgeImpl() override = default;

    // ── IGRPCBridge ────────────────────────────────────────────────────────

    void registerService(ServiceDescriptor service, IHttpHandler& handler) override {
        // Once dispatch() has been invoked, post-init registration is disallowed
        // to prevent race conditions with in-flight request routing.
        if (dispatched_.load(std::memory_order_acquire)) {
            throw std::logic_error(
                "GrpcBridgeImpl::registerService: cannot register service '" +
                service.service_name + "' after dispatch() has been called");
        }

        std::unique_lock<std::shared_mutex> lk(mutex_);
        const auto& name = service.service_name;
        if (services_.count(name)) {
            throw std::invalid_argument(
                "GrpcBridgeImpl::registerService: service '" + name +
                "' is already registered");
        }
        services_.emplace(name, Entry{std::move(service), &handler});
        THEMIS_INFO("GrpcBridgeImpl: registered service '{}'", name);
    }

    themis::Result<HttpResponse> dispatch(const GRPCRequest& request) override {
        dispatched_.store(true, std::memory_order_release);

        std::shared_lock<std::shared_mutex> lk(mutex_);
        auto it = services_.find(request.service_name);
        if (it == services_.end()) {
            THEMIS_WARN("GrpcBridgeImpl: unknown service '{}' — returning 404",
                        request.service_name);
            return themis::Ok(HttpResponse::notFound());
        }

        // Convert gRPC request → HttpRequest.
        HttpRequest http_req;
        http_req.method = "POST";
        http_req.path   = "/" + request.service_name + "/" + request.method_name;
        http_req.body   = request.request_bytes;

        // Propagate gRPC metadata as HTTP headers.
        if (!request.metadata.authority.empty())
            http_req.headers["Host"] = request.metadata.authority;
        if (!request.metadata.content_type.empty())
            http_req.headers["Content-Type"] = request.metadata.content_type;
        if (!request.metadata.deadline.empty())
            http_req.headers["grpc-deadline"] = request.metadata.deadline;
        for (const auto& [k, v] : request.metadata.user_metadata) {
            http_req.headers[k] = v;
        }

        THEMIS_DEBUG("GrpcBridgeImpl: dispatching {}/{}", request.service_name,
                     request.method_name);
        return it->second.handler->handle(http_req);
    }

    std::vector<ServiceDescriptor> registeredServices() const override {
        std::shared_lock<std::shared_mutex> lk(mutex_);
        std::vector<ServiceDescriptor> result;
        result.reserve(services_.size());
        for (const auto& [name, entry] : services_) {
            result.push_back(entry.descriptor);
        }
        return result;
    }

private:
    struct Entry {
        ServiceDescriptor descriptor;
        IHttpHandler*     handler{nullptr}; // non-owning; caller manages lifetime
    };

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Entry> services_;
    std::atomic<bool> dispatched_{false};
};

// ---------------------------------------------------------------------------
// Factory function
// ---------------------------------------------------------------------------

std::unique_ptr<IGRPCBridge> makeGrpcBridge() {
    return std::make_unique<GrpcBridgeImpl>();
}

} // namespace api
} // namespace themis
