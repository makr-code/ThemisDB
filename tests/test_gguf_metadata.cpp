/**
 * @file tests/test_gguf_metadata.cpp
 * @brief Unit tests for GGUFMetadata (GGUF v3 provenance store).
 *
 * Test IDs
 * --------
 * GMD-01  attach() then retrieve() returns identical record
 * GMD-02  retrieve() for unknown key returns nullopt
 * GMD-03  detach() removes the record; subsequent retrieve() returns nullopt
 * GMD-04  sign() + verify() round-trip succeeds with correct key
 * GMD-05  verify() returns false with wrong key
 * GMD-06  ProvenanceRecord::isComplete() reflects mandatory fields
 * GMD-07  serialize() + deserialize() round-trips all fields
 * GMD-08  size() and keys() reflect current store state
 */

#include "storage/gguf_metadata.h"

#include <gtest/gtest.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <string>
#include <vector>

namespace {

using namespace themis::storage;

// Helper: build a fully-populated record.
ProvenanceRecord makeRecord(const std::string& filename = "doc.pdf",
                             const std::string& doc_id  = "uuid-001",
                             const std::string& tenant  = "tenant1") {
    ProvenanceRecord rec;
    rec.source_filename  = filename;
    rec.source_page      = 3;
    rec.source_line      = 42;
    rec.source_doc_id    = doc_id;
    rec.tenant_id        = tenant;
    rec.ingest_timestamp = "2026-05-06T19:00:00Z";
    return rec;
}

std::string hmacSha256Hex(const std::string& data, const std::string& key) {
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;
    const auto* result = HMAC(
        EVP_sha256(),
        reinterpret_cast<const unsigned char*>(key.data()),
        static_cast<int>(key.size()),
        reinterpret_cast<const unsigned char*>(data.data()),
        static_cast<int>(data.size()),
        md,
        &md_len);
    EXPECT_NE(result, nullptr);
    if (!result) {
        return {};
    }

    static constexpr char kHexDigits[] = "0123456789abcdef";
    std::string hex = {};
    hex.resize(md_len * 2);
    for (unsigned int i = 0; i < md_len; ++i) {
        hex[2 * i] = kHexDigits[(md[i] >> 4) & 0x0F];
        hex[2 * i + 1] = kHexDigits[md[i] & 0x0F];
    }
    return hex;
}

// ──────────────────────────────────────────────────────────────────────────────
// GMD-01: attach then retrieve returns identical record
// ──────────────────────────────────────────────────────────────────────────────
TEST(GGUFMetadata, GMD01_attach_retrieve_roundtrip) {
    GGUFMetadata meta;
    const std::string key = "__ttcore__:tenant1:file42:chunk7";
    auto rec = makeRecord();

    meta.attach(key, rec);
    auto retrieved = meta.retrieve(key);

    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->source_filename,  rec.source_filename);
    EXPECT_EQ(retrieved->source_page,      rec.source_page);
    EXPECT_EQ(retrieved->source_line,      rec.source_line);
    EXPECT_EQ(retrieved->source_doc_id,    rec.source_doc_id);
    EXPECT_EQ(retrieved->tenant_id,        rec.tenant_id);
    EXPECT_EQ(retrieved->ingest_timestamp, rec.ingest_timestamp);
}

// ──────────────────────────────────────────────────────────────────────────────
// GMD-02: retrieve for unknown key returns nullopt
// ──────────────────────────────────────────────────────────────────────────────
TEST(GGUFMetadata, GMD02_retrieve_unknown_key_returns_nullopt) {
    GGUFMetadata meta;
    EXPECT_FALSE(meta.retrieve("does_not_exist").has_value());
}

// ──────────────────────────────────────────────────────────────────────────────
// GMD-03: detach removes the record
// ──────────────────────────────────────────────────────────────────────────────
TEST(GGUFMetadata, GMD03_detach_removes_record) {
    GGUFMetadata meta;
    const std::string key = "__adapters__:t1:legal:llama3";
    meta.attach(key, makeRecord());
    ASSERT_TRUE(meta.has(key));

    EXPECT_TRUE(meta.detach(key));
    EXPECT_FALSE(meta.has(key));
    EXPECT_FALSE(meta.retrieve(key).has_value());
    EXPECT_EQ(meta.size(), 0u);
}

// ──────────────────────────────────────────────────────────────────────────────
// GMD-04: sign + verify round-trip with correct key
// ──────────────────────────────────────────────────────────────────────────────
TEST(GGUFMetadata, GMD04_sign_verify_correct_key) {
    auto rec = makeRecord();
    const std::string hmac_key = "super_secret_key_42";
    const std::string expected =
        hmacSha256Hex(rec.canonicalBytes(), hmac_key);

    GGUFMetadata::sign(rec, hmac_key);
    EXPECT_FALSE(rec.hmac_signature.empty());
    EXPECT_EQ(rec.hmac_signature.size(), 64u);
    EXPECT_EQ(rec.hmac_signature, expected);

    EXPECT_TRUE(GGUFMetadata::verify(rec, hmac_key));
}

// ──────────────────────────────────────────────────────────────────────────────
// GMD-05: verify returns false with wrong key
// ──────────────────────────────────────────────────────────────────────────────
TEST(GGUFMetadata, GMD05_verify_wrong_key_returns_false) {
    auto rec = makeRecord();
    GGUFMetadata::sign(rec, "correct_key");
    EXPECT_FALSE(GGUFMetadata::verify(rec, "wrong_key"));
}

// ──────────────────────────────────────────────────────────────────────────────
// GMD-06: ProvenanceRecord::isComplete() reflects mandatory fields
// ──────────────────────────────────────────────────────────────────────────────
TEST(GGUFMetadata, GMD06_isComplete_reflects_mandatory_fields) {
    ProvenanceRecord empty_rec;
    EXPECT_FALSE(empty_rec.isComplete());

    auto rec = makeRecord();
    EXPECT_TRUE(rec.isComplete());

    rec.tenant_id = "";
    EXPECT_FALSE(rec.isComplete());
}

// ──────────────────────────────────────────────────────────────────────────────
// GMD-07: serialize + deserialize round-trips all fields
// ──────────────────────────────────────────────────────────────────────────────
TEST(GGUFMetadata, GMD07_serialize_deserialize_roundtrip) {
    GGUFMetadata meta;
    const std::string key1 = "__ttcore__:t1:fileA:chunk1";
    const std::string key2 = "__adapters__:t1:legal:llama3";

    auto rec1 = makeRecord("docA.pdf", "uuid-A", "tenant1");
    auto rec2 = makeRecord("docB.pdf", "uuid-B", "tenant1");
    GGUFMetadata::sign(rec1, "hmac_key1");
    GGUFMetadata::sign(rec2, "hmac_key2");

    meta.attach(key1, rec1);
    meta.attach(key2, rec2);

    const auto bytes = meta.serialize();
    ASSERT_FALSE(bytes.empty());

    GGUFMetadata meta2;
    ASSERT_TRUE(meta2.deserialize(bytes));

    ASSERT_EQ(meta2.size(), 2u);

    auto r1 = meta2.retrieve(key1);
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(r1->source_filename,  rec1.source_filename);
    EXPECT_EQ(r1->source_doc_id,    rec1.source_doc_id);
    EXPECT_EQ(r1->hmac_signature,   rec1.hmac_signature);

    auto r2 = meta2.retrieve(key2);
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r2->source_filename,  rec2.source_filename);
}

// ──────────────────────────────────────────────────────────────────────────────
// GMD-08: size() and keys() reflect current store state
// ──────────────────────────────────────────────────────────────────────────────
TEST(GGUFMetadata, GMD08_size_and_keys) {
    GGUFMetadata meta;
    EXPECT_EQ(meta.size(), 0u);
    EXPECT_TRUE(meta.keys().empty());

    meta.attach("key_a", makeRecord("a.pdf", "uuid-a", "t1"));
    meta.attach("key_b", makeRecord("b.pdf", "uuid-b", "t1"));
    meta.attach("key_c", makeRecord("c.pdf", "uuid-c", "t1"));

    EXPECT_EQ(meta.size(), 3u);

    const auto ks = meta.keys();
    ASSERT_EQ(ks.size(), 3u);
    // keys() returns sorted result.
    EXPECT_EQ(ks[0], "key_a");
    EXPECT_EQ(ks[1], "key_b");
    EXPECT_EQ(ks[2], "key_c");
}

TEST(GGUFMetadata, GMD09_injected_hmacfn_overrides_default_path) {
    auto rec = makeRecord();
    const std::string key = "custom-key";
    // constantTimeEquals requires exactly 64 lowercase hex chars (SHA-256 output length).
    const std::string signature = std::string(64, 'a');  // "aaa...a" — valid 64-char hex string

    GGUFMetadata::setHmacFn(
        [signature]([[maybe_unused]] const std::string& data,
                    [[maybe_unused]] const std::string& hmac_key) {
            return signature;
        });

    GGUFMetadata::sign(rec, key);
    EXPECT_EQ(rec.hmac_signature, signature);
    EXPECT_TRUE(GGUFMetadata::verify(rec, key));
    // Injected HMAC function fully controls verification behavior.
    EXPECT_TRUE(GGUFMetadata::verify(rec, "wrong-key"));

    GGUFMetadata::setHmacFn(nullptr);

    auto rec_default = makeRecord();
    const std::string default_key = "super_secret_key_42";
    const std::string expected =
        hmacSha256Hex(rec_default.canonicalBytes(), default_key);
    GGUFMetadata::sign(rec_default, default_key);
    EXPECT_EQ(rec_default.hmac_signature, expected);
    EXPECT_TRUE(GGUFMetadata::verify(rec_default, default_key));
}

} // anonymous namespace
