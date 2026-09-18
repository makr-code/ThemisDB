/**
 * @file graphql_ws_handler.h
 * @brief GraphQL WebSocket handler implementing the graphql-transport-ws protocol.
 *
 * @details Server-side implementation of the graphql-transport-ws protocol
 * (https://github.com/enisdenjo/graphql-ws), enabling GraphQL subscriptions
 * over WebSocket connections.
 *
 * Protocol flow:
 *  1. Client sends `connection_init` → Server replies `connection_ack`
 *  2. Client sends `subscribe` (with id + query/variables) → Server sends `next` messages
 *  3. Client may send `complete` (with id) to cancel a subscription
 *  4. Server sends `error` if subscription query is invalid, `complete` when done
 *
 * Message types (JSON-encoded):
 *  - `connection_init` (client → server)
 *  - `connection_ack` (server → client)
 *  - `ping` / `pong` (both directions)
 *  - `subscribe` (client → server)
 *  - `next` (server → client)
 *  - `error` (server → client)
 *  - `complete` (both directions)
 *
 * Core components:
 *  - `GraphQLWsHandler`: Per-connection state machine and message processor
 *  - One handler instance per active WebSocket connection
 *
 * Security:
 *  - WebSocket upgrade request must carry valid JWT in Authorization header
 *    (validated before upgrade, in WsChangeHandler)
 *  - Subscriptions per connection are capped by QueryLimits::max_subscriptions
 *    to prevent fan-out DoS
 *
 * ### Usage
 * ```cpp
 * // On WebSocket upgrade:
 * auto handler = std::make_unique<GraphQLWsHandler>(schema, limits, changefeed);
 *
 * // For each incoming text frame:
 * auto response_frames = handler->handleFrame(frame_text);
 * for (auto& frame : response_frames) {
 *     ws_session->sendText(frame);
 * }
 *
 * // When connection closes:
 * handler->reset();
 * ```
 *
 * ### Thread safety
 * - `handleFrame()` must be called from a single I/O thread (not reentrant)
 * - Safe to call concurrently from different connections' I/O threads
 * - Changefeed integration uses shared CDC event source (thread-safe)
 *
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
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

class GraphQLWsHandler {
public:
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
     * @brief Handle Frame.
     * @param[in] frame_text Input parameter.
     * @return Return value.
     */
    std::vector<std::string> handleFrame(std::string_view frame_text);

    // -----------------------------------------------------------------------
    // Connection lifecycle
    // -----------------------------------------------------------------------

    bool isConnected() const noexcept { return connected_.load(std::memory_order_relaxed); }

    /**
     * @brief Active Subscription Count.
     * @return Return value.
     */
    size_t activeSubscriptionCount() const;

    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

    /**
     * @brief ----------------------------------------------------------------------- Static helpers for protocol message construction -----------------------------------------------------------------------
     * @return Return value.
     */

    static std::string buildConnectionAck();

    static std::string buildPing(const std::string& payload = "");

    static std::string buildPong(const std::string& payload = "");

    /**
     * @brief Build Next.
     * @param[in] id Input parameter.
     * @param[in] data_json Input parameter.
     * @return Return value.
     */
    static std::string buildNext(const std::string& id,
                                 const std::string& data_json);

    /**
     * @brief Build Error.
     * @param[in] id Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    static std::string buildError(const std::string& id,
                                  const std::string& message);

    /**
     * @brief Build Complete.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    static std::string buildComplete(const std::string& id);

    // -----------------------------------------------------------------------
    // Path helper
    // -----------------------------------------------------------------------

    /**
     * @brief Is Graph QLWs Path.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isGraphQLWsPath(std::string_view path);

    // -----------------------------------------------------------------------
    // Test accessor
    // -----------------------------------------------------------------------

    const std::shared_ptr<std::atomic<bool>>& aliveForTesting() const noexcept {
        return alive_;
    }

private:
    // -----------------------------------------------------------------------
    // Internal message handlers
    // -----------------------------------------------------------------------

    /**
     * @brief Handle Connection Init.
     * @param[in] payload_json Input parameter.
     * @return Return value.
     */
    std::vector<std::string> handleConnectionInit(const std::string& payload_json);
    /**
     * @brief Handle Subscribe.
     * @param[in] id Input parameter.
     * @param[in] payload_json Input parameter.
     * @return Return value.
     */
    std::vector<std::string> handleSubscribe(const std::string& id,
                                              const std::string& payload_json);
    /**
     * @brief Handle Complete.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::vector<std::string> handleComplete(const std::string& id);
    /**
     * @brief Handle Ping.
     * @param[in] payload_json Input parameter.
     * @return Return value.
     */
    std::vector<std::string> handlePing(const std::string& payload_json);

    /**
     * @brief Extract On Change Collection.
     * @param[in] doc Input parameter.
     * @return Return value.
     */
    static std::string extractOnChangeCollection(const graphql::Document& doc);

    /**
     * @brief Validate Variables.
     * @param[in] op Input parameter.
     * @param[in] variables Input parameter.
     * @return Return value.
     */
    static std::string validateVariables(const graphql::Operation& op,
                                         const nlohmann::json& variables);

    // -----------------------------------------------------------------------
    // Members
    // -----------------------------------------------------------------------

    graphql::Schema        schema_;
    graphql::QueryLimits   limits_;
    themis::Changefeed*    changefeed_ = nullptr;  ///< Non-owning; may be null.

    std::atomic<bool> connected_{false};

    std::shared_ptr<std::atomic<bool>> alive_;

    struct SubscriptionEntry {
        themis::Changefeed::SubscriptionHandle cdc_handle;
    };

    std::unordered_map<std::string, SubscriptionEntry> subscriptions_;
    mutable std::mutex mutex_;

    // Frames queued by CDC callbacks for delivery.  Guarded by mutex_.
    std::vector<std::string> pending_frames_;
};

} // namespace api
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
