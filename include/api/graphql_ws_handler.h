/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphql_ws_handler.h                               ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 11:22:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     278                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • e442ddfb53  2026-03-17  fix(api): address PR review — CI path, alive-flag tests, ... ║
    • 6fe3cb23d1  2026-03-17  audit(api): fix move-ctor UB, renumber steps, drop dead f... ║
    • 1812297f72  2026-03-17  Changes before error encountered        ║
    • 2317d01553  2026-03-10  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#ifdef THEMIS_ENABLE_WEBSOCKET

#include "api/graphql.h"
#include "cdc/changefeed.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace themis {
namespace api {

namespace beast = boost::beast;
namespace http  = beast::http;

/**
 * @brief GraphQL WebSocket handler implementing the `graphql-transport-ws` protocol.
 *
 * This class handles the server-side logic for the `graphql-transport-ws` protocol
 * (https://github.com/enisdenjo/graphql-ws/blob/master/PROTOCOL.md).
 *
 * ## Protocol Overview
 * 1. Client sends `connection_init`   → Server replies `connection_ack`
 * 2. Client sends `subscribe` (with id + payload={query, variables, operationName})
 *    → Server sends `next` messages and eventually `complete`
 * 3. Client sends `complete` (with id) to cancel an active subscription
 * 4. Server sends `error` if the subscription query is invalid
 *
 * ## Message types
 * - `connection_init`  (client → server)
 * - `connection_ack`   (server → client)
 * - `ping`             (both directions)
 * - `pong`             (both directions)
 * - `subscribe`        (client → server)
 * - `next`             (server → client)
 * - `error`            (server → client)
 * - `complete`         (both directions)
 *
 * ## Security
 * - The WebSocket upgrade request must carry a valid JWT in the `Authorization`
 *   header (validated before the upgrade completes, in `WsChangeHandler`).
 * - Subscriptions per connection are capped by `QueryLimits::max_subscriptions`
 *   to prevent fan-out DoS.
 *
 * ## Usage
 * ```cpp
 * GraphQLWsHandler handler(schema, limits);
 * // On each incoming WebSocket text frame:
 * auto frames = handler.handleFrame(frame_text);
 * for (auto& f : frames) ws_session.sendText(f);
 * ```
 *
 * @see src/api/graphql_ws_handler.cpp
 * @see include/api/graphql.h  (QueryLimits, Schema, Executor)
 * @see include/api/ws_handler.h  (WsChangeHandler – validates the upgrade request)
 */
class GraphQLWsHandler {
public:
    /**
     * @brief Construct a handler for a single WebSocket connection.
     *
     * @param schema   GraphQL schema to validate and execute subscriptions against.
     * @param limits   Query limits (including max_subscriptions).
     * @param changefeed Optional Changefeed instance for `onChange` subscriptions.
     *                   When non-null, GraphQL `subscription { onChange(...) }` queries
     *                   are wired to the CDC event source via `Changefeed::subscribe()`.
     */
    GraphQLWsHandler(graphql::Schema schema,
                     graphql::QueryLimits limits = graphql::QueryLimits::defaults(),
                     themis::Changefeed* changefeed = nullptr);

    ~GraphQLWsHandler();

    // Non-copyable and non-movable: each instance owns a shared alive_ flag that
    // is captured by value in CDC lambda closures.  Allowing moves would leave
    // the source's alive_ null; the source's destructor would then call reset(),
    // which dereferences alive_ → null-pointer UB.
    GraphQLWsHandler(const GraphQLWsHandler&)            = delete;
    GraphQLWsHandler& operator=(const GraphQLWsHandler&) = delete;
    GraphQLWsHandler(GraphQLWsHandler&&)                 = delete;
    GraphQLWsHandler& operator=(GraphQLWsHandler&&)      = delete;

    // -----------------------------------------------------------------------
    // Frame processing
    // -----------------------------------------------------------------------

    /**
     * @brief Process an incoming WebSocket text frame.
     *
     * Parses the JSON-encoded `graphql-transport-ws` message and returns zero
     * or more JSON-encoded response frames that should be sent to the client.
     *
     * Thread-safe: may be called from a single I/O thread; do not call
     * concurrently for the same handler instance.
     *
     * @param frame_text  Raw text content of the WebSocket frame.
     * @return Vector of response frame payloads (may be empty).
     */
    std::vector<std::string> handleFrame(std::string_view frame_text);

    // -----------------------------------------------------------------------
    // Connection lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Return true if the connection handshake is complete
     *        (i.e. `connection_ack` has been sent).
     */
    bool isConnected() const noexcept { return connected_.load(std::memory_order_relaxed); }

    /**
     * @brief Return the number of active subscriptions on this connection.
     */
    size_t activeSubscriptionCount() const;

    /**
     * @brief Cancel all active subscriptions and reset connection state.
     *
     * Should be called when the WebSocket connection is closed.
     */
    void reset();

    // -----------------------------------------------------------------------
    // Static helpers for protocol message construction
    // -----------------------------------------------------------------------

    /// Build a `connection_ack` message.
    static std::string buildConnectionAck();

    /// Build a `ping` message (optional payload).
    static std::string buildPing(const std::string& payload = "");

    /// Build a `pong` message (optional payload).
    static std::string buildPong(const std::string& payload = "");

    /// Build a `next` message carrying a GraphQL result.
    static std::string buildNext(const std::string& id,
                                 const std::string& data_json);

    /// Build an `error` message with an array of error objects.
    static std::string buildError(const std::string& id,
                                  const std::string& message);

    /// Build a `complete` message.
    static std::string buildComplete(const std::string& id);

    // -----------------------------------------------------------------------
    // Path helper
    // -----------------------------------------------------------------------

    /**
     * @brief Return true if @p path targets the GraphQL WebSocket endpoint.
     *
     * Accepts `/graphql` (standard) and `/v2/graphql/subscriptions`.
     */
    static bool isGraphQLWsPath(std::string_view path);

    // -----------------------------------------------------------------------
    // Test accessor
    // -----------------------------------------------------------------------

    /**
     * @brief Return the shared alive flag for white-box testing.
     *
     * Allows tests to verify that reset() sets the flag to false with the
     * correct memory ordering before subscription handles are cleared, and
     * that a captured copy of the flag (as held by a CDC callback lambda)
     * correctly observes false after reset().
     *
     * Not intended for use in production code.
     */
    const std::shared_ptr<std::atomic<bool>>& aliveForTesting() const noexcept {
        return alive_;
    }

private:
    // -----------------------------------------------------------------------
    // Internal message handlers
    // -----------------------------------------------------------------------

    std::vector<std::string> handleConnectionInit(const std::string& payload_json);
    std::vector<std::string> handleSubscribe(const std::string& id,
                                              const std::string& payload_json);
    std::vector<std::string> handleComplete(const std::string& id);
    std::vector<std::string> handlePing(const std::string& payload_json);

    /// Extract the `onChange` collection argument from a parsed subscription document.
    static std::string extractOnChangeCollection(const graphql::Document& doc);

    /**
     * @brief Validate that the JSON @p variables map satisfies the declared
     *        VariableDefinition types of @p op.
     *
     * Checks:
     * - Non-null variables without defaults must be present.
     * - Non-null variables must not have a null JSON value.
     * - Provided scalar variables must match their declared GraphQL type
     *   (String/ID → string, Int → integer, Float → number, Boolean → boolean).
     * - List-typed variables must be JSON arrays (or null if nullable).
     *
     * @param op        Parsed GraphQL operation whose variable definitions to
     *                  validate against.
     * @param variables JSON object of variable values supplied by the client.
     * @return Empty string on success; human-readable error message otherwise.
     */
    static std::string validateVariables(const graphql::Operation& op,
                                         const nlohmann::json& variables);

    // -----------------------------------------------------------------------
    // Members
    // -----------------------------------------------------------------------

    graphql::Schema        schema_;
    graphql::QueryLimits   limits_;
    themis::Changefeed*    changefeed_ = nullptr;  ///< Non-owning; may be null.

    /// True after connection_ack has been sent.
    std::atomic<bool> connected_{false};

    /**
     * @brief Lifetime flag shared between this handler and all CDC callback lambdas.
     *
     * Set to @c false in reset() *before* subscription handles are destroyed,
     * so any in-flight CDC callback running on another thread will observe the
     * flag and return early instead of dereferencing `this` (avoiding
     * use-after-free).  The flag is stored in a @c shared_ptr so the lambda
     * can outlive the handler without dangling.
     */
    std::shared_ptr<std::atomic<bool>> alive_;

    /// Per-subscription entry: holds the CDC subscription handle (RAII).
    struct SubscriptionEntry {
        themis::Changefeed::SubscriptionHandle cdc_handle;
    };

    /// Active subscription IDs.  Guarded by mutex_.
    std::unordered_map<std::string, SubscriptionEntry> subscriptions_;
    mutable std::mutex mutex_;

    // Frames queued by CDC callbacks for delivery.  Guarded by mutex_.
    std::vector<std::string> pending_frames_;
};

} // namespace api
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
