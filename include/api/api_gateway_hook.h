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

/**
 * @brief Execution phase for an IAPIGatewayHook.
 *
 * Hooks registered for the same phase are ordered by IAPIGatewayHook::priority()
 * (lower value = earlier execution).
 */
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

/**
 * @brief Mutable request context visible to all hooks in the pipeline.
 *
 * Hooks communicate with each other via the `metadata` key-value bag.
 * They must not modify `request_id`, `method`, `path`, or `client_ip`.
 */
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

/**
 * @brief Decision and optional response overrides returned by a hook.
 *
 * Setting `proceed = false` halts further hook processing for the current
 * phase and causes the gateway to send the response indicated by
 * `override_status_code` (defaults to 403 if 0 and proceed is false).
 */
struct GatewayHookResult {
    bool proceed              = true;   ///< false = abort request processing.
    int  override_status_code = 0;      ///< 0 = do not override HTTP status.
    std::string override_body;          ///< Empty = do not override response body.
    std::map<std::string, std::string> add_headers; ///< Headers to inject into response.
};

// ---------------------------------------------------------------------------
// IAPIGatewayHook — single plugin hook
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual interface for a single API gateway plugin hook.
 *
 * Implementations are registered with IGatewayHookRegistry and invoked by
 * the gateway framework at the declared `phase()` for every inbound request.
 *
 * ### Contract
 * - `execute()` must be thread-safe; it may be called concurrently.
 * - `execute()` must complete within the gateway's configured hook timeout.
 * - Hooks that throw propagate the exception; the gateway treats it as
 *   `GatewayHookResult{.proceed = false, .override_status_code = 500}`.
 */
class IAPIGatewayHook {
public:
    virtual ~IAPIGatewayHook() = default;

    /// Unique identifier for this hook (used for registration/deregistration).
    /// @deprecated No external callers confirmed. CANDIDATE_FOR_REMOVAL (see src/ROADMAP.md).
    [[nodiscard, deprecated("No external callers; CANDIDATE_FOR_REMOVAL – tracked in src/ROADMAP.md")]]
    virtual std::string hookId() const = 0;

    /// Phase in which this hook executes.
    [[nodiscard]] virtual GatewayHookPhase phase() const = 0;

    /// Execution priority within the phase; lower value = earlier execution.
    virtual int priority() const { return 100; }

    /**
     * @brief Execute the hook logic.
     *
     * @param ctx  Mutable request context; hooks may write to `ctx.metadata`.
     * @return GatewayHookResult indicating whether to proceed and any overrides.
     */
    [[nodiscard]] virtual GatewayHookResult execute(GatewayHookContext& ctx) = 0;

    /// Return false to skip execution of this hook without unregistering it.
    virtual bool isEnabled() const { return true; }
};

// ---------------------------------------------------------------------------
// IGatewayHookRegistry — registry for IAPIGatewayHook instances
// ---------------------------------------------------------------------------

/**
 * @brief Registry for managing lifecycle and lookup of IAPIGatewayHook plugins.
 *
 * The gateway framework calls `getHooks()` once per request phase to obtain
 * the ordered set of enabled hooks to execute.
 *
 * Thread-safety: all methods must be thread-safe.
 */
class IGatewayHookRegistry {
public:
    virtual ~IGatewayHookRegistry() = default;

    /**
     * @brief Register a hook.
     *
     * @return `false` if a hook with the same `hookId()` is already registered.
     * @deprecated No external callers confirmed. CANDIDATE_FOR_REMOVAL (see src/ROADMAP.md).
     */
    [[nodiscard, deprecated("No external callers; CANDIDATE_FOR_REMOVAL – tracked in src/ROADMAP.md")]]
    virtual bool registerHook(std::shared_ptr<IAPIGatewayHook> hook) = 0;

    /**
     * @brief Unregister a hook by ID.
     *
     * @return `false` if no hook with @p hook_id was found.
     * @deprecated No external callers confirmed. CANDIDATE_FOR_REMOVAL (see src/ROADMAP.md).
     */
    [[nodiscard, deprecated("No external callers; CANDIDATE_FOR_REMOVAL – tracked in src/ROADMAP.md")]]
    virtual bool unregisterHook(const std::string& hook_id) = 0;

    /**
     * @brief Return enabled hooks for @p phase, sorted by priority (ascending).
     * @deprecated No external callers confirmed. CANDIDATE_FOR_REMOVAL (see src/ROADMAP.md).
     */
    [[nodiscard, deprecated("No external callers; CANDIDATE_FOR_REMOVAL – tracked in src/ROADMAP.md")]]
    virtual std::vector<std::shared_ptr<IAPIGatewayHook>> getHooks(
        GatewayHookPhase phase
    ) const = 0;
};

} // namespace api
} // namespace themis
