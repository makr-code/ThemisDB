/*
 * Tests for Schema-aware CDC with Avro/Protobuf schema registry integration.
 *
 * Validates that the schema registry components work correctly:
 *
 *  1.  SchemaFormat enum and schemaFormatString() helper
 *  2.  InMemorySchemaRegistryBackend – register, idempotent re-registration,
 *      getById, getLatest, size/clear
 *  3.  SchemaRegistryConfig – default values
 *  4.  SchemaRegistryClient – construction with implicit in-memory backend,
 *      ensureSchema (caching), getSchema, getLatestSchema, clearCache
 *  5.  CdcSchemaEncoder – default schema templates (Avro/JSON/Protobuf),
 *      encode (wire format header validation), decodeToJson (round-trip),
 *      extractSchemaId, ensureCollectionSchema auto-registration,
 *      clearLocalCache, collection-from-key derivation
 *  6.  Wire format constants
 *  7.  Multiple collections → separate schema IDs
 *  8.  DELETE event encoding
 *  9.  UPDATE event (with before_snapshot)
 * 10.  Redacted event encoding
 * 11.  Custom collection name override
 * 12.  auto_register_schemas = false throws on unknown collection
 * 13.  Cache TTL expiry (zero TTL = never expire)
 */

#include <gtest/gtest.h>
#include "cdc/schema_registry.h"
#include "cdc/changefeed.h"

using namespace themis;
using namespace themis::cdc;

// ── Helpers ──────────────────────────────────────────────────────────────────

namespace {

Changefeed::ChangeEvent makePutEvent(
    const std::string& key   = "orders:1",
    const std::string& value = R"({"qty":5})",
    std::optional<std::string> before = std::nullopt,
    std::optional<std::string> after  = std::nullopt)
{
    Changefeed::ChangeEvent ev;
    ev.sequence        = 1;
    ev.type            = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key             = key;
    ev.value           = value;
    ev.timestamp_ms    = 1740000000000LL;
    ev.before_snapshot = std::move(before);
    ev.after_snapshot  = std::move(after);
    return ev;
}

Changefeed::ChangeEvent makeDeleteEvent(const std::string& key = "orders:2")
{
    Changefeed::ChangeEvent ev;
    ev.sequence     = 2;
    ev.type         = Changefeed::ChangeEventType::EVENT_DELETE;
    ev.key          = key;
    ev.timestamp_ms = 1740000000001LL;
    return ev;
}

} // anonymous namespace

// ── 1. SchemaFormat helper ────────────────────────────────────────────────────

TEST(SchemaFormatTest, FormatStringJSON) {
    EXPECT_EQ(schemaFormatString(SchemaFormat::JSON),     "JSON");
}

TEST(SchemaFormatTest, FormatStringAVRO) {
    EXPECT_EQ(schemaFormatString(SchemaFormat::AVRO),     "AVRO");
}

TEST(SchemaFormatTest, FormatStringPROTOBUF) {
    EXPECT_EQ(schemaFormatString(SchemaFormat::PROTOBUF), "PROTOBUF");
}

// ── 2. InMemorySchemaRegistryBackend ─────────────────────────────────────────

TEST(InMemoryBackendTest, RegisterReturnsNonNegativeId) {
    InMemorySchemaRegistryBackend backend;
    const int32_t id = backend.registerSchema("orders-value", "{}", SchemaFormat::JSON);
    EXPECT_GE(id, 0);
}

TEST(InMemoryBackendTest, IdempotentRegistrationReturnsSameId) {
    InMemorySchemaRegistryBackend backend;
    const int32_t id1 = backend.registerSchema("orders-value", "{}", SchemaFormat::JSON);
    const int32_t id2 = backend.registerSchema("orders-value", "{}", SchemaFormat::JSON);
    EXPECT_EQ(id1, id2);
}

TEST(InMemoryBackendTest, DifferentSubjectsDifferentIds) {
    InMemorySchemaRegistryBackend backend;
    const int32_t id1 = backend.registerSchema("orders-value",   "{}", SchemaFormat::JSON);
    const int32_t id2 = backend.registerSchema("customers-value", "{}", SchemaFormat::JSON);
    EXPECT_NE(id1, id2);
}

TEST(InMemoryBackendTest, GetByIdReturnsRegisteredSchema) {
    InMemorySchemaRegistryBackend backend;
    const int32_t id = backend.registerSchema("orders-value", R"({"type":"string"})",
                                              SchemaFormat::AVRO);
    auto info = backend.getById(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->id,          id);
    EXPECT_EQ(info->subject,     "orders-value");
    EXPECT_EQ(info->format,      SchemaFormat::AVRO);
    EXPECT_EQ(info->schema_json, R"({"type":"string"})");
    EXPECT_EQ(info->version,     1);
}

TEST(InMemoryBackendTest, GetByIdUnknownReturnsNullopt) {
    InMemorySchemaRegistryBackend backend;
    EXPECT_FALSE(backend.getById(99999).has_value());
}

TEST(InMemoryBackendTest, GetLatestReturnsNewestSchema) {
    InMemorySchemaRegistryBackend backend;
    backend.registerSchema("orders-value", "{}", SchemaFormat::JSON);
    const int32_t id2 = backend.registerSchema("orders-value", R"({"v":2})",
                                               SchemaFormat::JSON);
    auto latest = backend.getLatest("orders-value");
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->id, id2);
}

TEST(InMemoryBackendTest, GetLatestUnknownSubjectReturnsNullopt) {
    InMemorySchemaRegistryBackend backend;
    EXPECT_FALSE(backend.getLatest("nonexistent-value").has_value());
}

TEST(InMemoryBackendTest, SizeReflectsRegisteredCount) {
    InMemorySchemaRegistryBackend backend;
    EXPECT_EQ(backend.size(), 0u);
    backend.registerSchema("a-value", "{}", SchemaFormat::JSON);
    EXPECT_EQ(backend.size(), 1u);
    backend.registerSchema("b-value", "{}", SchemaFormat::JSON);
    EXPECT_EQ(backend.size(), 2u);
}

TEST(InMemoryBackendTest, ClearRemovesAllSchemas) {
    InMemorySchemaRegistryBackend backend;
    backend.registerSchema("orders-value", "{}", SchemaFormat::JSON);
    backend.clear();
    EXPECT_EQ(backend.size(), 0u);
    EXPECT_FALSE(backend.getLatest("orders-value").has_value());
}

TEST(InMemoryBackendTest, SchemaInfoIsValidAfterRegistration) {
    InMemorySchemaRegistryBackend backend;
    const int32_t id = backend.registerSchema("x-value", "{}", SchemaFormat::JSON);
    auto info = backend.getById(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_TRUE(info->is_valid());
}

// ── 3. SchemaRegistryConfig defaults ─────────────────────────────────────────

TEST(SchemaRegistryConfigTest, Defaults) {
    SchemaRegistryConfig cfg;
    EXPECT_TRUE(cfg.url.empty());
    EXPECT_TRUE(cfg.username.empty());
    EXPECT_TRUE(cfg.password.empty());
    EXPECT_EQ(cfg.subject_name_strategy, "TopicName");
    EXPECT_EQ(cfg.topic_prefix,          "themis.cdc.");
    EXPECT_EQ(cfg.default_format,        SchemaFormat::JSON);
    EXPECT_TRUE(cfg.auto_register_schemas);
    EXPECT_EQ(cfg.cache_ttl.count(),     300);
    EXPECT_EQ(cfg.max_retries,           3u);
    EXPECT_EQ(cfg.request_timeout.count(), 5000);
    EXPECT_TRUE(cfg.ssl_ca_location.empty());
}

// ── 4. SchemaRegistryClient ───────────────────────────────────────────────────

TEST(SchemaRegistryClientTest, ConstructWithDefaultInMemoryBackend) {
    SchemaRegistryClient client;
    // Should not throw and should have a valid backend.
    EXPECT_NE(client.backend(), nullptr);
}

TEST(SchemaRegistryClientTest, EnsureSchemaReturnsId) {
    SchemaRegistryClient client;
    const int32_t id = client.ensureSchema("orders-value", "{}", SchemaFormat::JSON);
    EXPECT_GE(id, 0);
}

TEST(SchemaRegistryClientTest, EnsureSchemaIdempotent) {
    SchemaRegistryClient client;
    const int32_t id1 = client.ensureSchema("orders-value", "{}", SchemaFormat::JSON);
    const int32_t id2 = client.ensureSchema("orders-value", "{}", SchemaFormat::JSON);
    EXPECT_EQ(id1, id2);
}

TEST(SchemaRegistryClientTest, GetSchemaByIdReturnsInfo) {
    SchemaRegistryClient client;
    const int32_t id = client.ensureSchema("orders-value", R"({})", SchemaFormat::JSON);
    auto info = client.getSchema(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->id, id);
}

TEST(SchemaRegistryClientTest, GetSchemaByIdUnknownReturnsNullopt) {
    SchemaRegistryClient client;
    EXPECT_FALSE(client.getSchema(99999).has_value());
}

TEST(SchemaRegistryClientTest, GetLatestSchemaReturnsInfo) {
    SchemaRegistryClient client;
    client.ensureSchema("orders-value", "{}", SchemaFormat::JSON);
    auto latest = client.getLatestSchema("orders-value");
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->subject, "orders-value");
}

TEST(SchemaRegistryClientTest, ClearCacheDoesNotCorruptBackend) {
    SchemaRegistryClient client;
    const int32_t id = client.ensureSchema("orders-value", "{}", SchemaFormat::JSON);
    client.clearCache();
    // After cache cleared, getSchema should still reach the backend.
    auto info = client.getSchema(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->id, id);
}

TEST(SchemaRegistryClientTest, ConfigAccessor) {
    SchemaRegistryConfig cfg;
    cfg.url = "http://schema-registry:8081";
    SchemaRegistryClient client(cfg);
    EXPECT_EQ(client.config().url, "http://schema-registry:8081");
}

// ── 5. Wire format constants ──────────────────────────────────────────────────

TEST(WireFormatTest, MagicByteIsZero) {
    EXPECT_EQ(SCHEMA_REGISTRY_MAGIC_BYTE, 0x00u);
}

TEST(WireFormatTest, HeaderSizeIsFive) {
    EXPECT_EQ(SCHEMA_REGISTRY_HEADER_SIZE, 5u);
}

// ── 6. CdcSchemaEncoder – default schema templates ───────────────────────────

TEST(CdcSchemaEncoderDefaultsTest, DefaultAvroSchemaIsValidJson) {
    const auto schema = CdcSchemaEncoder::defaultAvroSchema("orders");
    auto j = nlohmann::json::parse(schema); // must not throw
    EXPECT_EQ(j["type"].get<std::string>(), "record");
    EXPECT_EQ(j["name"].get<std::string>(), "CdcEvent");
    // Namespace should contain the collection name.
    const std::string ns = j["namespace"].get<std::string>();
    EXPECT_NE(ns.find("orders"), std::string::npos);
}

TEST(CdcSchemaEncoderDefaultsTest, DefaultJsonSchemaIsValidJson) {
    const auto schema = CdcSchemaEncoder::defaultJsonSchema("customers");
    auto j = nlohmann::json::parse(schema);
    EXPECT_EQ(j["title"].get<std::string>(), "CdcEvent");
    EXPECT_TRUE(j.contains("properties"));
    EXPECT_TRUE(j["properties"].contains("sequence"));
    EXPECT_TRUE(j["properties"].contains("operation"));
    EXPECT_TRUE(j["properties"].contains("key"));
}

TEST(CdcSchemaEncoderDefaultsTest, DefaultProtobufSchemaContainsSyntax) {
    const auto schema = CdcSchemaEncoder::defaultProtobufSchema("inventory");
    EXPECT_NE(schema.find("syntax = \"proto3\""), std::string::npos);
    EXPECT_NE(schema.find("inventory"),            std::string::npos);
    EXPECT_NE(schema.find("CdcEvent"),             std::string::npos);
}

// ── 7. CdcSchemaEncoder – encode / wire format ───────────────────────────────

class CdcSchemaEncoderTest : public ::testing::Test {
protected:
    void SetUp() override {
        client = std::make_unique<SchemaRegistryClient>();
        encoder = std::make_unique<CdcSchemaEncoder>(client.get());
    }

    std::unique_ptr<SchemaRegistryClient> client;
    std::unique_ptr<CdcSchemaEncoder>     encoder;
};

TEST_F(CdcSchemaEncoderTest, EncodeProducesNonEmptyBytes) {
    auto result = encoder->encode(makePutEvent(), "orders");
    EXPECT_FALSE(result.data.empty());
}

TEST_F(CdcSchemaEncoderTest, EncodedDataStartsWithMagicByte) {
    auto result = encoder->encode(makePutEvent(), "orders");
    ASSERT_GE(result.data.size(), SCHEMA_REGISTRY_HEADER_SIZE);
    EXPECT_EQ(result.data[0], SCHEMA_REGISTRY_MAGIC_BYTE);
}

TEST_F(CdcSchemaEncoderTest, EncodedDataHasCorrectHeaderSize) {
    auto result = encoder->encode(makePutEvent(), "orders");
    EXPECT_GT(result.data.size(), SCHEMA_REGISTRY_HEADER_SIZE);
}

TEST_F(CdcSchemaEncoderTest, ExtractSchemaIdMatchesResultSchemaId) {
    auto result = encoder->encode(makePutEvent(), "orders");
    const int32_t extracted = CdcSchemaEncoder::extractSchemaId(result.data);
    EXPECT_EQ(extracted, result.schema_id);
}

TEST_F(CdcSchemaEncoderTest, SchemaIdIsNonNegative) {
    auto result = encoder->encode(makePutEvent(), "orders");
    EXPECT_GE(result.schema_id, 0);
}

TEST_F(CdcSchemaEncoderTest, SubjectContainsCollectionName) {
    auto result = encoder->encode(makePutEvent(), "orders");
    EXPECT_NE(result.subject.find("orders"), std::string::npos);
}

TEST_F(CdcSchemaEncoderTest, DecodeToJsonRoundTrip) {
    auto ev = makePutEvent("orders:42", R"({"qty":10})");
    auto result = encoder->encode(ev, "orders");

    auto decoded = encoder->decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ((*decoded)["key"].get<std::string>(),        "orders:42");
    EXPECT_EQ((*decoded)["operation"].get<std::string>(),  "PUT");
    EXPECT_EQ((*decoded)["collection"].get<std::string>(), "orders");
    EXPECT_EQ((*decoded)["sequence"].get<uint64_t>(),      1u);
    EXPECT_EQ((*decoded)["timestamp_ms"].get<int64_t>(),   1740000000000LL);
}

TEST_F(CdcSchemaEncoderTest, DecodeToJsonContainsValueField) {
    auto ev = makePutEvent("orders:1", R"({"price":9.99})");
    auto result  = encoder->encode(ev, "orders");
    auto decoded = encoder->decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ((*decoded)["value"].get<std::string>(), R"({"price":9.99})");
}

TEST_F(CdcSchemaEncoderTest, DecodeToJsonContainsSourceBlock) {
    auto result  = encoder->encode(makePutEvent(), "orders");
    auto decoded = encoder->decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());
    ASSERT_TRUE(decoded->contains("source"));
    EXPECT_EQ((*decoded)["source"]["connector"].get<std::string>(), "themisdb");
    EXPECT_EQ((*decoded)["source"]["table"].get<std::string>(), "orders");
}

// ── 8. DELETE event ───────────────────────────────────────────────────────────

TEST_F(CdcSchemaEncoderTest, DeleteEventOperationIsDelete) {
    auto result  = encoder->encode(makeDeleteEvent("orders:99"), "orders");
    auto decoded = encoder->decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ((*decoded)["operation"].get<std::string>(), "DELETE");
}

TEST_F(CdcSchemaEncoderTest, DeleteEventValueIsNull) {
    auto result  = encoder->encode(makeDeleteEvent(), "orders");
    auto decoded = encoder->decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_TRUE((*decoded)["value"].is_null());
}

// ── 9. UPDATE event (with before_snapshot) ────────────────────────────────────

TEST_F(CdcSchemaEncoderTest, UpdateEventHasBeforeAndAfterFields) {
    auto ev = makePutEvent("orders:5",
                           R"({"qty":10})",
                           R"({"qty":5})",   // before
                           R"({"qty":10})"); // after
    auto result  = encoder->encode(ev, "orders");
    auto decoded = encoder->decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ((*decoded)["before"].get<std::string>(), R"({"qty":5})");
    EXPECT_EQ((*decoded)["after"].get<std::string>(),  R"({"qty":10})");
}

// ── 10. Redacted event ────────────────────────────────────────────────────────

TEST_F(CdcSchemaEncoderTest, RedactedEventHasRedactedField) {
    auto ev = makePutEvent("users:99", "[REDACTED]");
    ev.redacted = true;
    auto result  = encoder->encode(ev, "users");
    auto decoded = encoder->decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_TRUE(decoded->value("redacted", false));
}

// ── 11. Multiple collections → separate schema IDs ───────────────────────────

TEST_F(CdcSchemaEncoderTest, DifferentCollectionsUseDifferentSchemaIds) {
    auto r1 = encoder->encode(makePutEvent("orders:1"),    "orders");
    auto r2 = encoder->encode(makePutEvent("products:1"),  "products");
    EXPECT_NE(r1.schema_id, r2.schema_id);
    EXPECT_NE(r1.subject,   r2.subject);
}

// ── 12. Collection derivation from key ───────────────────────────────────────

TEST_F(CdcSchemaEncoderTest, CollectionDerivedFromKeyWhenNotExplicit) {
    // Key "customers:42" → collection "customers"
    auto result  = encoder->encode(makePutEvent("customers:42"));
    auto decoded = encoder->decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ((*decoded)["collection"].get<std::string>(), "customers");
}

TEST_F(CdcSchemaEncoderTest, ExplicitCollectionOverridesKeyDerived) {
    auto result  = encoder->encode(makePutEvent("foo:1"), "explicit_collection");
    auto decoded = encoder->decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ((*decoded)["collection"].get<std::string>(), "explicit_collection");
}

// ── 13. auto_register_schemas = false ────────────────────────────────────────

TEST(CdcSchemaEncoderNoAutoRegTest, ThrowsWhenSchemaNotFound) {
    SchemaRegistryConfig cfg;
    cfg.auto_register_schemas = false;
    SchemaRegistryClient client(cfg);
    CdcSchemaEncoder encoder(&client);

    EXPECT_THROW(
        encoder.encode(makePutEvent("orders:1"), "orders"),
        std::runtime_error);
}

// ── 14. clearLocalCache forces re-fetch ──────────────────────────────────────

TEST_F(CdcSchemaEncoderTest, ClearLocalCacheDoesNotCorruptEncoding) {
    auto r1 = encoder->encode(makePutEvent(), "orders");
    encoder->clearLocalCache();
    auto r2 = encoder->encode(makePutEvent(), "orders");
    // Both encodes should use the same schema ID (schema is still in the backend).
    EXPECT_EQ(r1.schema_id, r2.schema_id);
}

// ── 15. extractSchemaId on malformed input ────────────────────────────────────

TEST(ExtractSchemaIdTest, TooShortReturnsNegativeOne) {
    EXPECT_EQ(CdcSchemaEncoder::extractSchemaId({}),       -1);
    EXPECT_EQ(CdcSchemaEncoder::extractSchemaId({0x00}),    -1);
}

TEST(ExtractSchemaIdTest, WrongMagicByteReturnsNegativeOne) {
    std::vector<uint8_t> bad = {0x01, 0x00, 0x00, 0x00, 0x05, 0x7B};
    EXPECT_EQ(CdcSchemaEncoder::extractSchemaId(bad), -1);
}

TEST(ExtractSchemaIdTest, CorrectHeaderDecodesId) {
    // Schema ID = 0x00000042 (66) encoded big-endian.
    std::vector<uint8_t> wire = {
        0x00,                   // magic
        0x00, 0x00, 0x00, 0x42, // schema_id = 66 BE
        0x7B, 0x7D              // payload: {}
    };
    EXPECT_EQ(CdcSchemaEncoder::extractSchemaId(wire), 66);
}

// ── 16. decodeToJson on malformed input ──────────────────────────────────────

TEST_F(CdcSchemaEncoderTest, DecodeToJsonEmptyBytesReturnsNullopt) {
    EXPECT_FALSE(encoder->decodeToJson({}).has_value());
}

TEST_F(CdcSchemaEncoderTest, DecodeToJsonTruncatedHeaderReturnsNullopt) {
    EXPECT_FALSE(encoder->decodeToJson({0x00, 0x01}).has_value());
}

TEST_F(CdcSchemaEncoderTest, DecodeToJsonInvalidPayloadReturnsNullopt) {
    std::vector<uint8_t> bad = {
        0x00, 0x00, 0x00, 0x00, 0x01,   // valid header
        0xFF, 0xFE, 0xFD                 // invalid UTF-8 / invalid JSON
    };
    // We call this through the encoder – it should return nullopt, not throw.
    EXPECT_FALSE(encoder->decodeToJson(bad).has_value());
}

// ── 17. Avro format – schema auto-registration ───────────────────────────────

TEST(CdcSchemaEncoderAvroTest, AvroFormatAutoRegistersAvroSchema) {
    SchemaRegistryConfig cfg;
    cfg.default_format = SchemaFormat::AVRO;
    SchemaRegistryClient client(cfg);
    CdcSchemaEncoder encoder(&client);

    const int32_t id = encoder.ensureCollectionSchema("inventory");
    EXPECT_GE(id, 0);

    auto info = client.getSchema(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->format, SchemaFormat::AVRO);
    // The schema should be the Avro record schema.
    auto j = nlohmann::json::parse(info->schema_json);
    EXPECT_EQ(j["type"].get<std::string>(), "record");
}

// ── 18. Protobuf format – schema auto-registration ───────────────────────────

TEST(CdcSchemaEncoderProtoTest, ProtobufFormatAutoRegistersProtoSchema) {
    SchemaRegistryConfig cfg;
    cfg.default_format = SchemaFormat::PROTOBUF;
    SchemaRegistryClient client(cfg);
    CdcSchemaEncoder encoder(&client);

    const int32_t id = encoder.ensureCollectionSchema("shipments");
    EXPECT_GE(id, 0);

    auto info = client.getSchema(id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->format, SchemaFormat::PROTOBUF);
    EXPECT_NE(info->schema_json.find("proto3"), std::string::npos);
}

// ── 19. TRANSACTION_COMMIT / TRANSACTION_ROLLBACK event encoding ─────────────

TEST_F(CdcSchemaEncoderTest, TransactionCommitOperationString) {
    Changefeed::ChangeEvent ev;
    ev.sequence     = 10;
    ev.type         = Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT;
    ev.key          = "orders:7";
    ev.timestamp_ms = 1740000000010LL;

    auto result  = encoder->encode(ev, "orders");
    auto decoded = encoder->decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ((*decoded)["operation"].get<std::string>(), "TRANSACTION_COMMIT");
}

TEST_F(CdcSchemaEncoderTest, TransactionRollbackOperationString) {
    Changefeed::ChangeEvent ev;
    ev.sequence     = 11;
    ev.type         = Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK;
    ev.key          = "orders:8";
    ev.timestamp_ms = 1740000000011LL;

    auto result  = encoder->encode(ev, "orders");
    auto decoded = encoder->decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ((*decoded)["operation"].get<std::string>(), "TRANSACTION_ROLLBACK");
}

// ── Binary encoder injection ──────────────────────────────────────────────────
//
// CDCSE-BIN-01  setAvroEncoderFn: injected fn is called; binary payload used
// CDCSE-BIN-02  setAvroEncoderFn empty return → JSON fallback
// CDCSE-BIN-03  setProtobufEncoderFn: injected fn is called; binary payload used
// CDCSE-BIN-04  setProtobufEncoderFn(nullptr) clears; JSON fallback resumes

TEST(CdcBinaryEncoderInjectionTest, CDCSEBIN01_AvroEncoderInjected) {
    SchemaRegistryConfig cfg;
    cfg.default_format = SchemaFormat::AVRO;
    SchemaRegistryClient client(cfg);
    CdcSchemaEncoder enc(&client);

    // Inject a synthetic Avro encoder that returns fixed sentinel bytes.
    const std::vector<uint8_t> sentinel = {0xAA, 0xBB, 0xCC};
    bool encoder_called = false;
    enc.setAvroEncoderFn([&](const nlohmann::json&) -> std::vector<uint8_t> {
        encoder_called = true;
        return sentinel;
    });

    auto ev = makePutEvent("col:1", "{}");
    auto result = enc.encode(ev, "col");

    EXPECT_TRUE(encoder_called);
    // Wire format = [0x00][4-byte schema id][sentinel...]
    ASSERT_GT(result.data.size(), 5u);
    const auto payload_begin = result.data.begin() + 5;
    const std::vector<uint8_t> payload(payload_begin, result.data.end());
    EXPECT_EQ(payload, sentinel);
}

TEST(CdcBinaryEncoderInjectionTest, CDCSEBIN02_AvroEncoderEmptyReturnFallsBackToJson) {
    SchemaRegistryConfig cfg;
    cfg.default_format = SchemaFormat::AVRO;
    SchemaRegistryClient client(cfg);
    CdcSchemaEncoder enc(&client);

    // Encoder returns empty → JSON fallback must be used.
    enc.setAvroEncoderFn([](const nlohmann::json&) -> std::vector<uint8_t> {
        return {};
    });

    auto ev = makePutEvent("orders:1", R"({"qty":3})");
    auto result = enc.encode(ev, "orders");

    // Payload should be valid JSON (decodable).
    auto decoded = enc.decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ((*decoded)["operation"].get<std::string>(), "PUT");
}

TEST(CdcBinaryEncoderInjectionTest, CDCSEBIN03_ProtobufEncoderInjected) {
    SchemaRegistryConfig cfg;
    cfg.default_format = SchemaFormat::PROTOBUF;
    SchemaRegistryClient client(cfg);
    CdcSchemaEncoder enc(&client);

    const std::vector<uint8_t> sentinel = {0x08, 0x01, 0x10, 0x02}; // minimal proto bytes
    bool encoder_called = false;
    enc.setProtobufEncoderFn([&](const nlohmann::json&) -> std::vector<uint8_t> {
        encoder_called = true;
        return sentinel;
    });

    auto ev = makePutEvent("users:5", "{}");
    auto result = enc.encode(ev, "users");

    EXPECT_TRUE(encoder_called);
    ASSERT_GT(result.data.size(), 5u);
    const auto payload_begin = result.data.begin() + 5;
    const std::vector<uint8_t> payload(payload_begin, result.data.end());
    EXPECT_EQ(payload, sentinel);
}

TEST(CdcBinaryEncoderInjectionTest, CDCSEBIN04_ProtobufEncoderNullptrClearsInjection) {
    SchemaRegistryConfig cfg;
    cfg.default_format = SchemaFormat::PROTOBUF;
    SchemaRegistryClient client(cfg);
    CdcSchemaEncoder enc(&client);

    // Set then clear the encoder.
    enc.setProtobufEncoderFn([](const nlohmann::json&) -> std::vector<uint8_t> {
        return {0xFF}; // any non-empty bytes
    });
    enc.setProtobufEncoderFn(nullptr); // clear

    auto ev = makePutEvent("items:9", "{}");
    auto result = enc.encode(ev, "items");

    // After clearing, JSON fallback should be used → decodable.
    auto decoded = enc.decodeToJson(result.data);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ((*decoded)["operation"].get<std::string>(), "PUT");
}
