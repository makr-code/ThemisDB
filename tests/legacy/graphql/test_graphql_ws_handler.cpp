/**
 * @file test_graphql_ws_handler.cpp
 * @brief Unit tests for GraphQLWsHandler (graphql-transport-ws protocol)
 *
 * Validates:
 * - connection_init / connection_ack handshake
 * - subscribe message parsing, registration, and max_subscriptions enforcement
 * - complete (client cancel) removes the subscription
 * - ping → pong round-trip
 * - duplicate connection_init is rejected
 * - malformed JSON is handled gracefully
 * - QueryLimits::max_subscriptions field default and permissive values
 */

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_WEBSOCKET

#include "api/graphql_ws_handler.h"
#include "api/graphql.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <atomic>
#include <thread>

using namespace themis::api;
namespace gql = themis::graphql;
using json = nlohmann::json;
namespace graphql = themis::graphql;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static gql::Schema makeDefaultSchema() {
    return gql::ThemisSchemaBuilder::build();
}

// ---------------------------------------------------------------------------
// QueryLimits::max_subscriptions
// ---------------------------------------------------------------------------

TEST(QueryLimitsTest, DefaultMaxSubscriptions) {
    auto limits = gql::QueryLimits::defaults();
    EXPECT_EQ(limits.max_subscriptions, 10u);
}

TEST(QueryLimitsTest, PermissiveMaxSubscriptions) {
    auto limits = gql::QueryLimits::permissive();
    EXPECT_EQ(limits.max_subscriptions, 50u);
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class GraphQLWsHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto schema = makeDefaultSchema();
        gql::QueryLimits limits;
        limits.max_subscriptions = 3; // small limit for tests
        handler = std::make_unique<GraphQLWsHandler>(std::move(schema), limits);
    }

    // Convenience: send a frame and return parsed responses.
    std::vector<json> send(const json& msg) {
        auto frames = handler->handleFrame(msg.dump());
        std::vector<json> results = {};

        for (auto& f : frames) {
            results.push_back(json::parse(f));
        }
        return results;
    }

    // Perform connection_init handshake and assert ack.
    void doHandshake() {
        auto resp = send({{"type", "connection_init"}});
        ASSERT_EQ(resp.size(), 1u);
        ASSERT_EQ(resp[0]["type"], "connection_ack");
    }

    std::unique_ptr<GraphQLWsHandler> handler;
};

// ---------------------------------------------------------------------------
// connection_init / connection_ack
// ---------------------------------------------------------------------------

TEST_F(GraphQLWsHandlerTest, ConnectionInitReturnsAck) {
    auto resp = send({{"type", "connection_init"}});
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "connection_ack");
}

TEST_F(GraphQLWsHandlerTest, ConnectionAckAfterInitIsConnected) {
    EXPECT_FALSE(handler->isConnected());
    send({{"type", "connection_init"}});
    EXPECT_TRUE(handler->isConnected());
}

TEST_F(GraphQLWsHandlerTest, DuplicateConnectionInitIgnored) {
    // First init → ack
    auto r1 = send({{"type", "connection_init"}});
    ASSERT_EQ(r1.size(), 1u);
    EXPECT_EQ(r1[0]["type"], "connection_ack");

    // Second init → no response (protocol: close 4429)
    auto r2 = send({{"type", "connection_init"}});
    EXPECT_TRUE(r2.empty());
}

// ---------------------------------------------------------------------------
// subscribe – validation
// ---------------------------------------------------------------------------

TEST_F(GraphQLWsHandlerTest, SubscribeBeforeInitIsIgnored) {
    // No connection_init sent yet → subscribe should be ignored.
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub1"},
        {"payload", {{"query", "subscription { onChange(collection: \"orders\") { key } }"}}}
    });
    EXPECT_TRUE(resp.empty());
}

TEST_F(GraphQLWsHandlerTest, SubscribeMissingIdProducesNoResponse) {
    doHandshake();
    // Missing "id" field → should be dropped gracefully.
    auto resp = send({
        {"type",    "subscribe"},
        {"payload", {{"query", "subscription { onChange(collection: \"orders\") { key } }"}}}
    });
    EXPECT_TRUE(resp.empty());
}

TEST_F(GraphQLWsHandlerTest, SubscribeMissingQueryReturnsError) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub1"},
        {"payload", json::object()}  // empty payload, no "query" field
    });
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "error");
    EXPECT_EQ(resp[0]["id"],   "sub1");
}

TEST_F(GraphQLWsHandlerTest, SubscribeNonSubscriptionOperationReturnsError) {
    doHandshake();
    // A plain query (not a subscription) on the subscriptions endpoint
    // should return an error.
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub1"},
        {"payload", {{"query", "{ user { id } }"}}}
    });
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "error");
    EXPECT_EQ(resp[0]["id"],   "sub1");
}

TEST_F(GraphQLWsHandlerTest, SubscribeValidSubscriptionSucceeds) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub1"},
        {"payload", {{"query", "subscription { onChange(collection: \"orders\") { key type } }"}}}
    });
    // No immediate response for a valid subscription (events come later).
    EXPECT_TRUE(resp.empty());
    EXPECT_EQ(handler->activeSubscriptionCount(), 1u);
}

// ---------------------------------------------------------------------------
// subscribe – max_subscriptions enforcement
// ---------------------------------------------------------------------------

TEST_F(GraphQLWsHandlerTest, MaxSubscriptionsEnforced) {
    doHandshake();

    const std::string q =
        "subscription { onChange(collection: \"orders\") { key } }";

    // Register up to the limit (3 subscriptions).
    for (int sub_index = 0; sub_index < 3; ++sub_index) {
        auto resp = send({
            {"type",    "subscribe"},
            {"id",      "sub" + std::to_string(sub_index)},
            {"payload", {{"query", q}}}
        });
        EXPECT_TRUE(resp.empty()) << "Subscription " << sub_index << " should succeed";
    }
    EXPECT_EQ(handler->activeSubscriptionCount(), 3u);

    // 4th subscription should return an error.
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub99"},
        {"payload", {{"query", q}}}
    });
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "error");
    EXPECT_EQ(resp[0]["id"],   "sub99");
    // Still 3 active subscriptions.
    EXPECT_EQ(handler->activeSubscriptionCount(), 3u);
}

// ---------------------------------------------------------------------------
// complete (client cancel)
// ---------------------------------------------------------------------------

TEST_F(GraphQLWsHandlerTest, ClientCompleteRemovesSubscription) {
    doHandshake();

    const std::string q =
        "subscription { onChange(collection: \"orders\") { key } }";

    send({
        {"type",    "subscribe"},
        {"id",      "sub1"},
        {"payload", {{"query", q}}}
    });
    EXPECT_EQ(handler->activeSubscriptionCount(), 1u);

    // Client cancels.
    auto resp = send({{"type", "complete"}, {"id", "sub1"}});
    EXPECT_TRUE(resp.empty());
    EXPECT_EQ(handler->activeSubscriptionCount(), 0u);
}

TEST_F(GraphQLWsHandlerTest, CompleteUnknownIdIsIgnored) {
    doHandshake();
    auto resp = send({{"type", "complete"}, {"id", "nonexistent"}});
    EXPECT_TRUE(resp.empty());
}

// ---------------------------------------------------------------------------
// ping / pong
// ---------------------------------------------------------------------------

TEST_F(GraphQLWsHandlerTest, PingReturnsPong) {
    auto resp = send({{"type", "ping"}});
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "pong");
}

TEST_F(GraphQLWsHandlerTest, PingWithPayloadReturnsPongWithPayload) {
    auto resp = send({{"type", "ping"}, {"payload", {{"nonce", 42}}}});
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "pong");
    // Payload should be echoed back.
    ASSERT_TRUE(resp[0].contains("payload"));
    EXPECT_EQ(resp[0]["payload"]["nonce"], 42);
}

TEST_F(GraphQLWsHandlerTest, PongIsNoOp) {
    auto resp = send({{"type", "pong"}});
    EXPECT_TRUE(resp.empty());
}

// ---------------------------------------------------------------------------
// Malformed input
// ---------------------------------------------------------------------------

TEST_F(GraphQLWsHandlerTest, InvalidJSONFrameIsIgnored) {
    auto frames = handler->handleFrame("{not valid json!!!}");
    EXPECT_TRUE(frames.empty());
}

TEST_F(GraphQLWsHandlerTest, UnknownMessageTypeIsIgnored) {
    doHandshake();
    auto resp = send({{"type", "unknown_future_type"}, {"id", "x"}});
    EXPECT_TRUE(resp.empty());
}

// ---------------------------------------------------------------------------
// reset()
// ---------------------------------------------------------------------------

TEST_F(GraphQLWsHandlerTest, ResetClearsState) {
    doHandshake();
    const std::string q =
        "subscription { onChange(collection: \"orders\") { key } }";

    send({
        {"type",    "subscribe"},
        {"id",      "sub1"},
        {"payload", {{"query", q}}}
    });
    ASSERT_EQ(handler->activeSubscriptionCount(), 1u);

    handler->reset();
    EXPECT_EQ(handler->activeSubscriptionCount(), 0u);
    EXPECT_FALSE(handler->isConnected());
}

// reset() must not crash even when subscriptions are active (alive-flag path).
TEST_F(GraphQLWsHandlerTest, ResetWithActiveSubscriptionDoesNotCrash) {
    doHandshake();
    const std::string q =
        "subscription { onChange(collection: \"orders\") { key } }";
    send({
        {"type",    "subscribe"},
        {"id",      "sub1"},
        {"payload", {{"query", q}}}
    });
    EXPECT_EQ(handler->activeSubscriptionCount(), 1u);
    // reset() sets the alive flag to false before clearing subscriptions.
    // No crash or assertion should occur.
    EXPECT_NO_THROW(handler->reset());
    EXPECT_EQ(handler->activeSubscriptionCount(), 0u);
    EXPECT_FALSE(handler->isConnected());
}

// ---------------------------------------------------------------------------
// alive_ lifetime flag – direct mechanism tests
//
// These tests verify the shared_ptr<atomic<bool>> alive_ flag mechanism that
// guards CDC callback lambdas against use-after-free.  They use the
// aliveForTesting() accessor to simulate what a CDC callback lambda does
// (capture a shared copy of the flag, then check it with acquire ordering),
// exercising the exact memory-ordering contract that reset() must fulfil.
// ---------------------------------------------------------------------------

// The alive flag must be true after construction (before reset).
TEST_F(GraphQLWsHandlerTest, AliveFlag_InitiallyTrue) {
    EXPECT_TRUE(handler->aliveForTesting()->load(std::memory_order_acquire));
}

// reset() must set the alive flag to false with release ordering.
// A captured copy of the flag (as held by a CDC callback lambda) must observe
// false with acquire ordering — the exact guarantee that prevents the lambda
// from dereferencing the destroyed handler.
TEST_F(GraphQLWsHandlerTest, AliveFlag_FalseAfterReset) {
    // Simulate the CDC lambda: capture a shared copy of the alive flag.
    auto alive = handler->aliveForTesting();
    ASSERT_TRUE(alive->load(std::memory_order_acquire));

    handler->reset();

    // The flag must now be false. A CDC callback holding `alive` by value
    // (shared_ptr) would observe this and return early instead of touching self.
    EXPECT_FALSE(alive->load(std::memory_order_acquire));
}

// Simulate the race: a background thread checks the alive flag (as a CDC
// callback would) after reset() has already set it to false.  The thread
// must observe false — with the acquire/release ordering guaranteeing
// visibility — and must not crash.
TEST_F(GraphQLWsHandlerTest, AliveFlag_CallbackAfterResetObservesFalse) {
    // Capture shared ownership of the alive flag (mimicking the lambda capture).
    auto alive = handler->aliveForTesting();

    // Reset the handler first, setting alive_ to false with release ordering.
    handler->reset();

    // Simulate a CDC callback firing after reset() on a separate thread.
    // With acquire ordering the callback must see false.
    bool flag_value_in_callback = true;
    std::thread t([&alive, &flag_value_in_callback]() {
        flag_value_in_callback = alive->load(std::memory_order_acquire);
    });
    t.join();

    EXPECT_FALSE(flag_value_in_callback);
}

// ---------------------------------------------------------------------------
// Step-2 – schema-level variable type validation
// ---------------------------------------------------------------------------

// Subscription with a required String variable provided correctly → succeeds.
TEST_F(GraphQLWsHandlerTest, VariableValidation_RequiredStringProvided) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var1"},
        {"payload", {
            {"query",     "subscription Q($col: String!) { onChange(collection: $col) { key } }"},
            {"variables", {{"col", "orders"}}}
        }}
    });
    // Valid subscription – no error frame expected.
    EXPECT_TRUE(resp.empty());
    EXPECT_EQ(handler->activeSubscriptionCount(), 1u);
}

// Required variable absent → error.
TEST_F(GraphQLWsHandlerTest, VariableValidation_RequiredVariableMissing) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var2"},
        {"payload", {
            {"query",     "subscription Q($col: String!) { onChange(collection: $col) { key } }"},
            {"variables", json::object()}  // empty – $col not provided
        }}
    });
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "error");
    EXPECT_EQ(resp[0]["id"],   "sub_var2");
    EXPECT_EQ(handler->activeSubscriptionCount(), 0u);
}

// Wrong type – Int variable supplied a string value → error.
TEST_F(GraphQLWsHandlerTest, VariableValidation_WrongType_IntGivenString) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var3"},
        {"payload", {
            {"query",     "subscription Q($limit: Int!) { onChange(collection: \"orders\") { key } }"},
            {"variables", {{"limit", "not-an-int"}}}
        }}
    });
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "error");
    EXPECT_EQ(resp[0]["id"],   "sub_var3");
}

// Wrong type – String variable given an integer value → error.
TEST_F(GraphQLWsHandlerTest, VariableValidation_WrongType_StringGivenInt) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var4"},
        {"payload", {
            {"query",     "subscription Q($col: String!) { onChange(collection: $col) { key } }"},
            {"variables", {{"col", 42}}}
        }}
    });
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "error");
    EXPECT_EQ(resp[0]["id"],   "sub_var4");
}

// Non-null variable set to JSON null → error.
TEST_F(GraphQLWsHandlerTest, VariableValidation_NonNullSetToNull) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var5"},
        {"payload", {
            {"query",     "subscription Q($col: String!) { onChange(collection: $col) { key } }"},
            {"variables", {{"col", nullptr}}}
        }}
    });
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "error");
    EXPECT_EQ(resp[0]["id"],   "sub_var5");
}

// Nullable variable set to JSON null → succeeds (null is allowed).
TEST_F(GraphQLWsHandlerTest, VariableValidation_NullableSetToNull) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var6"},
        {"payload", {
            {"query",     "subscription Q($col: String) { onChange(collection: \"orders\") { key } }"},
            {"variables", {{"col", nullptr}}}
        }}
    });
    // Nullable variable with null value is valid.
    EXPECT_TRUE(resp.empty());
    EXPECT_EQ(handler->activeSubscriptionCount(), 1u);
}

// 'variables' field is not an object → error.
TEST_F(GraphQLWsHandlerTest, VariableValidation_VariablesNotObject) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var7"},
        {"payload", {
            {"query",     "subscription { onChange(collection: \"orders\") { key } }"},
            {"variables", json::array({1, 2, 3})}  // array, not object
        }}
    });
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "error");
    EXPECT_EQ(resp[0]["id"],   "sub_var7");
}

// Optional variable omitted entirely → succeeds (no default needed).
TEST_F(GraphQLWsHandlerTest, VariableValidation_OptionalVariableOmitted) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var8"},
        {"payload", {
            {"query",     "subscription Q($col: String) { onChange(collection: \"orders\") { key } }"},
            {"variables", json::object()}  // $col is optional, not provided
        }}
    });
    // Optional absent variable – no error.
    EXPECT_TRUE(resp.empty());
    EXPECT_EQ(handler->activeSubscriptionCount(), 1u);
}

// Boolean variable type validation.
TEST_F(GraphQLWsHandlerTest, VariableValidation_BooleanType_Valid) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var9"},
        {"payload", {
            {"query",     "subscription Q($flag: Boolean!) { onChange(collection: \"orders\") { key } }"},
            {"variables", {{"flag", true}}}
        }}
    });
    EXPECT_TRUE(resp.empty());
    EXPECT_EQ(handler->activeSubscriptionCount(), 1u);
}

TEST_F(GraphQLWsHandlerTest, VariableValidation_BooleanType_WrongType) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var10"},
        {"payload", {
            {"query",     "subscription Q($flag: Boolean!) { onChange(collection: \"orders\") { key } }"},
            {"variables", {{"flag", "yes"}}}  // string, not boolean
        }}
    });
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "error");
    EXPECT_EQ(resp[0]["id"],   "sub_var10");
}

// Float variable type validation.
TEST_F(GraphQLWsHandlerTest, VariableValidation_FloatType_Valid) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var11"},
        {"payload", {
            {"query",     "subscription Q($threshold: Float!) { onChange(collection: \"orders\") { key } }"},
            {"variables", {{"threshold", 3.14}}}
        }}
    });
    EXPECT_TRUE(resp.empty());
    EXPECT_EQ(handler->activeSubscriptionCount(), 1u);
}

// Float variable also accepts integers (GraphQL coercion rule: Int → Float).
TEST_F(GraphQLWsHandlerTest, VariableValidation_FloatType_IntCoercion) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var12"},
        {"payload", {
            {"query",     "subscription Q($threshold: Float!) { onChange(collection: \"orders\") { key } }"},
            {"variables", {{"threshold", 42}}}  // integer coercible to Float – allowed
        }}
    });
    EXPECT_TRUE(resp.empty());
    EXPECT_EQ(handler->activeSubscriptionCount(), 1u);
}

// Float variable given a string value → error.
TEST_F(GraphQLWsHandlerTest, VariableValidation_FloatType_WrongType) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var13"},
        {"payload", {
            {"query",     "subscription Q($threshold: Float!) { onChange(collection: \"orders\") { key } }"},
            {"variables", {{"threshold", "not-a-number"}}}
        }}
    });
    ASSERT_EQ(resp.size(), 1u);
    EXPECT_EQ(resp[0]["type"], "error");
    EXPECT_EQ(resp[0]["id"],   "sub_var13");
}

// Many GraphQL clients send `"variables": null` to mean "no variables".
// This must be accepted and treated identically to an empty variables object.
TEST_F(GraphQLWsHandlerTest, VariableValidation_NullVariablesField_AcceptedAsEmpty) {
    doHandshake();
    auto resp = send({
        {"type",    "subscribe"},
        {"id",      "sub_var14"},
        {"payload", {
            {"query",     "subscription { onChange(collection: \"orders\") { key } }"},
            {"variables", nullptr}  // explicit JSON null – many clients send this
        }}
    });
    // null variables must be treated as "no variables" → no error.
    EXPECT_TRUE(resp.empty());
    EXPECT_EQ(handler->activeSubscriptionCount(), 1u);
}

// ---------------------------------------------------------------------------
// Static builder helpers
// ---------------------------------------------------------------------------

TEST(GraphQLWsHandlerStaticTest, BuildConnectionAck) {
    auto s = GraphQLWsHandler::buildConnectionAck();
    auto j = json::parse(s);
    EXPECT_EQ(j["type"], "connection_ack");
}

TEST(GraphQLWsHandlerStaticTest, BuildNext) {
    auto s = GraphQLWsHandler::buildNext("sub1", R"({"key":"doc_1"})");
    auto j = json::parse(s);
    EXPECT_EQ(j["type"], "next");
    EXPECT_EQ(j["id"],   "sub1");
    EXPECT_TRUE(j.contains("payload"));
    EXPECT_TRUE(j["payload"].contains("data"));
}

TEST(GraphQLWsHandlerStaticTest, BuildError) {
    auto s = GraphQLWsHandler::buildError("sub1", "Something went wrong");
    auto j = json::parse(s);
    EXPECT_EQ(j["type"], "error");
    EXPECT_EQ(j["id"],   "sub1");
    ASSERT_TRUE(j["payload"].is_array());
    EXPECT_EQ(j["payload"][0]["message"], "Something went wrong");
}

TEST(GraphQLWsHandlerStaticTest, BuildComplete) {
    auto s = GraphQLWsHandler::buildComplete("sub1");
    auto j = json::parse(s);
    EXPECT_EQ(j["type"], "complete");
    EXPECT_EQ(j["id"],   "sub1");
}

TEST(GraphQLWsHandlerStaticTest, BuildPing) {
    auto s = GraphQLWsHandler::buildPing();
    auto j = json::parse(s);
    EXPECT_EQ(j["type"], "ping");
}

TEST(GraphQLWsHandlerStaticTest, BuildPong) {
    auto s = GraphQLWsHandler::buildPong();
    auto j = json::parse(s);
    EXPECT_EQ(j["type"], "pong");
}

// ---------------------------------------------------------------------------
// isGraphQLWsPath()
// ---------------------------------------------------------------------------

TEST(GraphQLWsHandlerStaticTest, IsGraphQLWsPath_GraphQL) {
    EXPECT_TRUE(GraphQLWsHandler::isGraphQLWsPath("/graphql"));
}

TEST(GraphQLWsHandlerStaticTest, IsGraphQLWsPath_V2Subscriptions) {
    EXPECT_TRUE(GraphQLWsHandler::isGraphQLWsPath("/v2/graphql/subscriptions"));
}

TEST(GraphQLWsHandlerStaticTest, IsGraphQLWsPath_Other) {
    EXPECT_FALSE(GraphQLWsHandler::isGraphQLWsPath("/v2/changes"));
    EXPECT_FALSE(GraphQLWsHandler::isGraphQLWsPath("/graphql/"));
    EXPECT_FALSE(GraphQLWsHandler::isGraphQLWsPath(""));
}

#else

TEST(GraphQLWsHandlerTest, WebSocketDisabled) {
    // When WebSocket support is not compiled, this test is a placeholder.
    SUCCEED() << "THEMIS_ENABLE_WEBSOCKET not set – GraphQLWsHandler tests skipped.";
}

#endif // THEMIS_ENABLE_WEBSOCKET
