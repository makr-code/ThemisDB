/**
 * @file graphql_ws_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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
                                   graphql::QueryLimits limits,
                                   themis::Changefeed* changefeed)
    : schema_(std::move(schema))
    , limits_(limits)
    , changefeed_(changefeed)
    , alive_(std::make_shared<std::atomic<bool>>(true))
{}

GraphQLWsHandler::~GraphQLWsHandler() {
    reset();
}

// ---------------------------------------------------------------------------
// reset()
// ---------------------------------------------------------------------------

void GraphQLWsHandler::reset() {
    // Signal any in-flight CDC callbacks to stop before the subscription
    // handles (and their associated RAII teardown) are destroyed.  This
    // prevents a use-after-free should the CDC implementation fire the
    // callback concurrently with the SubscriptionHandle destructor.
    alive_->store(false, std::memory_order_release);
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.clear();
    connected_.store(false, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// activeSubscriptionCount()
// ---------------------------------------------------------------------------

size_t GraphQLWsHandler::activeSubscriptionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(subscriptions_.size());
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
        return {buildError("", std::string("Invalid JSON frame: ") + ex.what())};
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
        return {buildError(id, "connection_init required before this message type")};
    }

    std::vector<std::string> frames;

    if (type == "subscribe") {
        if (id.empty()) {
            THEMIS_WARN([[maybe_unused]] "GraphQLWsHandler: subscribe message missing 'id'");
            return {buildError("", "subscribe message missing required 'id'")};
        }
        frames = handleSubscribe(id, payload_json);
    } else if (type == "complete") {
        if (id.empty()) {
            THEMIS_WARN([[maybe_unused]] "GraphQLWsHandler: complete message missing 'id'");
            return {buildError("", "complete message missing required 'id'")};
        }
        frames = handleComplete(id);
    } else if (type == "pong") {
        // pong is a no-op on the server side.
    } else {
        THEMIS_WARN("GraphQLWsHandler: unknown message type '{}'", type);
        return {buildError(id, "unknown GraphQL WS message type: " + type)};
    }

    // Flush CDC-queued next frames accumulated since the last handleFrame call.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!pending_frames_.empty()) {
            frames.insert(frames.end(),
                          std::make_move_iterator(pending_frames_.begin()),
                          std::make_move_iterator(pending_frames_.end()));
            pending_frames_.clear();
        }
    }
    return frames;
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
        THEMIS_WARN([[maybe_unused]] "GraphQLWsHandler: duplicate connection_init – ignoring");
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
        if (static_cast<int>(subscriptions_.size()) > = limits_.max_subscriptions) {
            THEMIS_WARN("GraphQLWsHandler: max_subscriptions ({}) reached for id '{}'",
                        limits_.max_subscriptions, id);
            return {buildError(id, "Maximum concurrent subscriptions exceeded")};
        }
    }

    // ── 2. Parse the payload ─────────────────────────────────────────────────
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

    // ── 3. Parse the GraphQL query ───────────────────────────────────────────
    auto parse_result = graphql::Parser::parse(query, limits_);
    if (!parse_result.success) {
        json errors_arr = json::array();
        for (const auto& e : parse_result.errors) {
            errors_arr.push_back(json{{"message", e.toString()}});
        }
        return {buildError(id, "Parse error: " + errors_arr.dump())};
    }

    // ── 4. Validate that at least one operation is a subscription ────────────
    if (parse_result.document.operations.empty()) {
        return {buildError(id, "No operations found in document")};
    }

    const auto& op = parse_result.document.operations[0];
    if (op.type != graphql::OperationType::Subscription) {
        // Non-subscription queries/mutations are not supported over this handler.
        return {buildError(id, "Only subscription operations are supported on this endpoint")};
    }

    // ── 5. Schema-level variable type validation ──────────────────────────────
    // Verify that every variable value supplied in the payload matches the
    // declared VariableDefinition type (requires the parsed operation from step 3).
    {
        // Treat explicit `"variables": null` the same as omitted (empty object):
        // many GraphQL clients send null to mean "no variables".
        const json variables = (payload.contains("variables") && !payload["variables"].is_null())
                               ? payload["variables"]
                               : json::object();
        if (!variables.is_object()) {
            return {buildError(id, "Field 'variables' must be a JSON object when present")};
        }
        const std::string var_error = validateVariables(op, variables);
        if (!var_error.empty()) {
            return {buildError(id, var_error)};
        }
    }

    // ── 6. Register the subscription ─────────────────────────────────────────
    // If a Changefeed is available and the operation targets the `onChange` field,
    // wire a push callback so CDC events are delivered as `next` frames.
    themis::Changefeed::SubscriptionHandle cdc_handle;
    if (changefeed_) {
        const std::string collection = extractOnChangeCollection(parse_result.document);
        if (!collection.empty()) {
            themis::Changefeed::SubscriptionFilter f;
            f.key_prefix = collection + ":";  // keys are "collection:pk"

            // Capture shared state by value: the subscription ID, a raw
            // pointer to this handler so the callback can queue next frames,
            // and a shared copy of the lifetime flag.
            // The alive flag is checked at entry so that if the CDC system
            // fires this callback after reset() has set the flag to false,
            // the callback exits without touching 'self' (preventing
            // use-after-free).
            const std::string sub_id = id;
            GraphQLWsHandler* self = this;
            auto alive = alive_;  // shared ownership; survives handler destruction

            cdc_handle = changefeed_->subscribe(std::move(f),
                [self, sub_id, alive]([[maybe_unused]] const themis::Changefeed::ChangeEvent& ev) {
                    // Guard against use-after-free: reset() sets this flag to
                    // false (with release ordering) before destroying the
                    // subscription handles; we load it with acquire ordering so
                    // the check is sequenced after reset()'s store.
                    if (!alive->load(std::memory_order_acquire)) {
                        return;
                    }
                    // Build a minimal GraphQL `next` data payload from the CDC event.
                    json data = {
                        {"onChange", {
                            {"sequence",    static_cast<int64_t>(ev.sequence)},
                            {"type",        ev.type == themis::Changefeed::ChangeEventType::EVENT_PUT
                                                ? "PUT" : "DELETE"},
                            {"key",         ev.key},
                            {"document",    ev.value.has_value()
                                                ? json::parse(ev.value.value(), nullptr, false)
                                                : json(nullptr)},
                            {"timestampMs", ev.timestamp_ms}
                        }}
                    };
                    const std::string frame = buildNext(sub_id, data.dump());
                    std::lock_guard<std::mutex> lk(self->mutex_);
                    self->pending_frames_.push_back(frame);
                });

            THEMIS_INFO("GraphQLWsHandler: subscription '{}' wired to CDC collection '{}'",
                        id, collection);
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        SubscriptionEntry entry;
        entry.cdc_handle = std::move(cdc_handle);
        subscriptions_.emplace(id, std::move(entry));
    }

    THEMIS_INFO("GraphQLWsHandler: subscription '{}' registered (query: {} chars)",
                id,static_cast<int>(query.size()));

    // Flush any frames that may have been queued by the CDC callback between
    // wiring and registering (timing edge case – normally empty).
    std::vector<std::string> frames;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        frames.swap(pending_frames_);
    }
    return frames;
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
std::string GraphQLWsHandler::buildPing([[maybe_unused]] const std::string& payload) {
    json msg{{"type", "ping"}};
    if (!payload.empty()) {
        try {
            msg["payload"] = json::parse(payload);
        } catch (const json::exception& ex) {
            THEMIS_WARN([[maybe_unused]] "GraphQLWsHandler::buildPing: payload is not valid JSON ({}), omitting", ex.what());
        }
    }
    return msg.dump();
}

/*static*/
std::string GraphQLWsHandler::buildPong([[maybe_unused]] const std::string& payload) {
    json msg{{"type", "pong"}};
    if (!payload.empty()) {
        try {
            msg["payload"] = json::parse(payload);
        } catch (const json::exception& ex) {
            THEMIS_WARN([[maybe_unused]] "GraphQLWsHandler::buildPong: payload is not valid JSON ({}), omitting", ex.what());
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
        THEMIS_WARN([[maybe_unused]] "GraphQLWsHandler::buildNext: data_json is not valid JSON ({}), using raw string", ex.what());
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
std::string GraphQLWsHandler::buildComplete([[maybe_unused]] const std::string& id) {
    return json{{"type", "complete"}, {"id", id}}.dump();
}

/*static*/
bool GraphQLWsHandler::isGraphQLWsPath([[maybe_unused]] std::string_view path) {
    return path == "/graphql" || path == "/v2/graphql/subscriptions";
}

// ---------------------------------------------------------------------------
// extractOnChangeCollection()
// ---------------------------------------------------------------------------

/*static*/
std::string GraphQLWsHandler::extractOnChangeCollection(const graphql::Document& doc)
{
    // Walk the first subscription operation looking for:
    //   subscription { onChange(collection: "orders") { ... } }
    //
    // Returns the string value of the "collection" argument on the `onChange`
    // field, or an empty string if not found.
    for (const auto& op : doc.operations) {
        if (op.type != graphql::OperationType::Subscription) {
          continue;
        }
        for (const auto& field : op.selections) {
            if (field.name != "onChange") {
              continue;
            }
            const auto it = field.arguments.find("collection");
            if (it != field.arguments.end() && it->second && it->second->isString()) {
                return it->second->asString();
            }
        }
    }
    return {};
}

// ---------------------------------------------------------------------------
// validateVariables()
// ---------------------------------------------------------------------------

/*static*/
std::string GraphQLWsHandler::validateVariables(const graphql::Operation& op,
                                                  const nlohmann::json& variables)
{
    for (const auto& vdef : op.variables) {
        const bool provided = variables.contains(vdef.name);

        // 1. Non-null variables without a default value must be supplied.
        if (vdef.is_non_null && !vdef.default_value && !provided) {
            return "Variable '$" + vdef.name + "' of required type '" +
                   vdef.type_name + "!' was not provided";
        }

        if (!provided) continue;  // Optional and absent – fine.

        const auto& val = variables.at(vdef.name);

        // 2. Non-null variables must not carry a null JSON value.
        if (val.is_null()) {
            if (vdef.is_non_null) {
                return "Variable '$" + vdef.name + "' of non-null type '" +
                       vdef.type_name + "!' must not be null";
            }
            continue;  // Nullable and null – fine.
        }

        // 3. List-typed variables must be JSON arrays.
        if (vdef.is_list) {
            if (!val.is_array()) {
                return "Variable '$" + vdef.name + "' expected a list (JSON array)";
            }
            continue;  // Skip element-level type checking.
        }

        // 4. Scalar type matching for well-known GraphQL built-in scalars.
        //    Custom / schema-defined types are not validated here.
        const std::string& t = vdef.type_name;
        bool type_ok = true;
        if (t == "String" || t == "ID") {
            type_ok = val.is_string();
        } else if (t == "Int") {
            type_ok = val.is_number_integer();
        } else if (t == "Float") {
            type_ok = val.is_number();  // integers are coercible to Float
        } else if (t == "Boolean") {
            type_ok = val.is_boolean();
        }
        // For non-scalar / custom input types, we skip validation at this layer.

        if (!type_ok) {
            return "Variable '$" + vdef.name +
                   "': value type does not match declared type '" + t + "'";
        }
    }
    return {};  // No errors.
}

} // namespace api
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
