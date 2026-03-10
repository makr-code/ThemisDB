#ifdef THEMIS_ENABLE_WEBSOCKET

#include "api/graphql_ws_handler.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>
#include <sstream>

namespace themis {
namespace api {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

GraphQLWsHandler::GraphQLWsHandler(graphql::Schema schema,
                                   graphql::QueryLimits limits)
    : schema_(std::move(schema))
    , limits_(limits)
{}

GraphQLWsHandler::~GraphQLWsHandler() {
    reset();
}

// ---------------------------------------------------------------------------
// reset()
// ---------------------------------------------------------------------------

void GraphQLWsHandler::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.clear();
    connected_.store(false, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// activeSubscriptionCount()
// ---------------------------------------------------------------------------

size_t GraphQLWsHandler::activeSubscriptionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return subscriptions_.size();
}

// ---------------------------------------------------------------------------
// handleFrame() – entry point
// ---------------------------------------------------------------------------

std::vector<std::string>
GraphQLWsHandler::handleFrame(std::string_view frame_text)
{
    json msg;
    try {
        msg = json::parse(frame_text);
    } catch (const json::exception& ex) {
        THEMIS_WARN("GraphQLWsHandler: invalid JSON frame: {}", ex.what());
        // Close-worthy parse error; return an error but let the transport close.
        return {};
    }

    const std::string type = msg.value("type", "");
    const std::string id   = msg.value("id",   "");

    const std::string payload_json =
        msg.contains("payload") ? msg["payload"].dump() : "{}";

    if (type == "connection_init") {
        return handleConnectionInit(payload_json);
    }

    if (type == "ping") {
        return handlePing(payload_json);
    }

    // All subsequent messages require the connection to be initialised.
    if (!connected_.load(std::memory_order_relaxed)) {
        THEMIS_WARN("GraphQLWsHandler: message '{}' received before connection_init", type);
        return {};
    }

    if (type == "subscribe") {
        if (id.empty()) {
            THEMIS_WARN("GraphQLWsHandler: subscribe message missing 'id'");
            return {};
        }
        return handleSubscribe(id, payload_json);
    }

    if (type == "complete") {
        return handleComplete(id);
    }

    if (type == "pong") {
        // pong is a no-op on the server side.
        return {};
    }

    THEMIS_WARN("GraphQLWsHandler: unknown message type '{}'", type);
    return {};
}

// ---------------------------------------------------------------------------
// handleConnectionInit()
// ---------------------------------------------------------------------------

std::vector<std::string>
GraphQLWsHandler::handleConnectionInit(const std::string& /*payload_json*/)
{
    if (connected_.load(std::memory_order_relaxed)) {
        // The protocol mandates closing 4429 "Too many initialisation requests".
        // We simply return nothing; the caller (transport) should close the
        // connection with close code 4429.
        THEMIS_WARN("GraphQLWsHandler: duplicate connection_init – ignoring");
        return {};
    }

    connected_.store(true, std::memory_order_relaxed);
    return {buildConnectionAck()};
}

// ---------------------------------------------------------------------------
// handleSubscribe()
// ---------------------------------------------------------------------------

std::vector<std::string>
GraphQLWsHandler::handleSubscribe(const std::string& id,
                                   const std::string& payload_json)
{
    // ── 1. Reject duplicate IDs AND enforce max_subscriptions under one lock ──
    // Both checks must be atomic (Time-Of-Check-Time-Of-Use / TOCTOU): a
    // concurrent subscribe from another thread could slip between two separate
    // lock acquisitions and bypass either guard.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (subscriptions_.count(id)) {
            THEMIS_WARN("GraphQLWsHandler: duplicate subscription id '{}'", id);
            return {buildError(id, "Subscriber for " + id + " already exists")};
        }
        if (subscriptions_.size() >= limits_.max_subscriptions) {
            THEMIS_WARN("GraphQLWsHandler: max_subscriptions ({}) reached for id '{}'",
                        limits_.max_subscriptions, id);
            return {buildError(id, "Maximum concurrent subscriptions exceeded")};
        }
    }

    // ── 3. Parse the payload ─────────────────────────────────────────────────
    json payload;
    try {
        payload = json::parse(payload_json);
    } catch (const json::exception& ex) {
        return {buildError(id, std::string("Invalid payload JSON: ") + ex.what())};
    }

    const std::string query = payload.value("query", "");
    if (query.empty()) {
        return {buildError(id, "Missing 'query' in subscribe payload")};
    }

    // ── 4. Parse the GraphQL query ───────────────────────────────────────────
    auto parse_result = graphql::Parser::parse(query, limits_);
    if (!parse_result.success) {
        json errors_arr = json::array();
        for (const auto& e : parse_result.errors) {
            errors_arr.push_back(json{{"message", e}});
        }
        return {buildError(id, "Parse error: " + errors_arr.dump())};
    }

    // ── 5. Validate that at least one operation is a subscription ────────────
    if (parse_result.document.operations.empty()) {
        return {buildError(id, "No operations found in document")};
    }

    const auto& op = parse_result.document.operations[0];
    if (op.type != graphql::OperationType::Subscription) {
        // Non-subscription queries/mutations are not supported over this handler.
        return {buildError(id, "Only subscription operations are supported on this endpoint")};
    }

    // ── 6. Register the subscription ─────────────────────────────────────────
    {
        std::lock_guard<std::mutex> lock(mutex_);
        subscriptions_[id] = true;
    }

    THEMIS_INFO("GraphQLWsHandler: subscription '{}' registered (query: {} chars)",
                id, query.size());

    // ── 7. Real event delivery is done externally by the transport layer,
    //       which calls buildNext() / buildComplete() as CDC events arrive.
    // Return nothing here – the subscription is now registered and active.
    return {};
}

// ---------------------------------------------------------------------------
// handleComplete()
// ---------------------------------------------------------------------------

std::vector<std::string>
GraphQLWsHandler::handleComplete(const std::string& id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = subscriptions_.find(id);
    if (it == subscriptions_.end()) {
        THEMIS_WARN("GraphQLWsHandler: complete for unknown subscription '{}'", id);
        return {};
    }
    subscriptions_.erase(it);
    THEMIS_INFO("GraphQLWsHandler: subscription '{}' cancelled", id);
    return {};
}

// ---------------------------------------------------------------------------
// handlePing()
// ---------------------------------------------------------------------------

std::vector<std::string>
GraphQLWsHandler::handlePing(const std::string& payload_json)
{
    return {buildPong(payload_json == "{}" ? "" : payload_json)};
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

/*static*/
std::string GraphQLWsHandler::buildConnectionAck() {
    return json{{"type", "connection_ack"}}.dump();
}

/*static*/
std::string GraphQLWsHandler::buildPing(const std::string& payload) {
    json msg{{"type", "ping"}};
    if (!payload.empty()) {
        try {
            msg["payload"] = json::parse(payload);
        } catch (const json::exception& ex) {
            THEMIS_WARN("GraphQLWsHandler::buildPing: payload is not valid JSON ({}), omitting", ex.what());
        }
    }
    return msg.dump();
}

/*static*/
std::string GraphQLWsHandler::buildPong(const std::string& payload) {
    json msg{{"type", "pong"}};
    if (!payload.empty()) {
        try {
            msg["payload"] = json::parse(payload);
        } catch (const json::exception& ex) {
            THEMIS_WARN("GraphQLWsHandler::buildPong: payload is not valid JSON ({}), omitting", ex.what());
        }
    }
    return msg.dump();
}

/*static*/
std::string GraphQLWsHandler::buildNext(const std::string& id,
                                         const std::string& data_json)
{
    json payload;
    try {
        payload = json::parse(data_json);
    } catch (const json::exception& ex) {
        THEMIS_WARN("GraphQLWsHandler::buildNext: data_json is not valid JSON ({}), using raw string", ex.what());
        payload = json{{"raw", data_json}};
    }
    return json{
        {"type", "next"},
        {"id",   id},
        {"payload", {{"data", payload}}}
    }.dump();
}

/*static*/
std::string GraphQLWsHandler::buildError(const std::string& id,
                                          const std::string& message)
{
    return json{
        {"type",    "error"},
        {"id",      id},
        {"payload", json::array({json{{"message", message}}})}
    }.dump();
}

/*static*/
std::string GraphQLWsHandler::buildComplete(const std::string& id) {
    return json{{"type", "complete"}, {"id", id}}.dump();
}

/*static*/
bool GraphQLWsHandler::isGraphQLWsPath(std::string_view path) {
    return path == "/graphql" || path == "/v2/graphql/subscriptions";
}

} // namespace api
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
