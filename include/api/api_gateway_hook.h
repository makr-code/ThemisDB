/**
 * @file api_gateway_hook.h
 * @brief API gateway lifecycle hook interface.
 *
 * @details Provides callback interfaces for initialization, request routing,
 * error handling, and shutdown events in the API gateway. Enables extensibility
 * without modifying core gateway code.
 *
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */


#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// GatewayHookPhase — execution phase of the hook in the request pipeline
// ---------------------------------------------------------------------------

enum class GatewayHookPhase {
    PRE_AUTH,      ///< Before authentication (e.g., IP allow-list, rate limiting).
    POST_AUTH,     ///< After authentication; before routing.
    PRE_HANDLER,   ///< Before the request handler executes.
    POST_HANDLER,  ///< After the handler; before the response is written.
    ON_ERROR,      ///< Invoked on the error path (handler or upstream failure).
};

// ---------------------------------------------------------------------------
// GatewayHookContext — mutable per-request context passed through hook chain
// ---------------------------------------------------------------------------

struct GatewayHookContext {
    std::string request_id;
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string client_ip;
    std::map<std::string, std::string> metadata;  ///< Mutable bag for hook-to-hook state.
};

// ---------------------------------------------------------------------------
// GatewayHookResult — decision returned by a single hook invocation
// ---------------------------------------------------------------------------

struct GatewayHookResult {
    bool proceed              = true;   ///< false = abort request processing.
    int  override_status_code = 0;      ///< 0 = do not override HTTP status.
    std::string override_body;          ///< Empty = do not override response body.
    std::map<std::string, std::string> add_headers; ///< Headers to inject into response.
};

// ---------------------------------------------------------------------------
// IAPIGatewayHook — single plugin hook
// ---------------------------------------------------------------------------

class IAPIGatewayHook {
public:
    /**
     * @brief IAPIGateway Hook.
     * @return Return value.
     */
    virtual ~IAPIGatewayHook() = default;

    [[nodiscard, deprecated("No external callers; CANDIDATE_FOR_REMOVAL – tracked in src/ROADMAP.md")]]
    /**
     * @brief Hook Id.
     * @return Return value.
     */
    virtual std::string hookId() const = 0;

    [[nodiscard]] virtual GatewayHookPhase phase() const = 0;

    virtual int priority() const { return 100; }

    [[nodiscard]] virtual GatewayHookResult execute(GatewayHookContext& ctx) = 0;

    virtual bool isEnabled() const { return true; }
};

// ---------------------------------------------------------------------------
// IGatewayHookRegistry — registry for IAPIGatewayHook instances
// ---------------------------------------------------------------------------

class IGatewayHookRegistry {
public:
    /**
     * @brief IGateway Hook Registry.
     * @return Return value.
     */
    virtual ~IGatewayHookRegistry() = default;

    [[nodiscard, deprecated("No external callers; CANDIDATE_FOR_REMOVAL – tracked in src/ROADMAP.md")]]
    /**
     * @brief Register Hook.
     * @param[in] hook Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool registerHook(std::shared_ptr<IAPIGatewayHook> hook) = 0;

    [[nodiscard, deprecated("No external callers; CANDIDATE_FOR_REMOVAL – tracked in src/ROADMAP.md")]]
    /**
     * @brief Unregister Hook.
     * @param[in] hook_id Identifier of the hook.
     * @return True when the operation succeeds.
     */
    virtual bool unregisterHook(const std::string& hook_id) = 0;

    [[nodiscard, deprecated("No external callers; CANDIDATE_FOR_REMOVAL – tracked in src/ROADMAP.md")]]
    /**
     * @brief Get Hooks.
     * @param[in] phase Input parameter.
     * @return Return value.
     */
    virtual std::vector<std::shared_ptr<IAPIGatewayHook>> getHooks(
        GatewayHookPhase phase
    ) const = 0;
};

} // namespace api
} // namespace themis
