/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tests/test_gguf_metadata.cpp                       ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 3 (Q1 2027)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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

    GGUFMetadata::sign(rec, hmac_key);
    EXPECT_FALSE(rec.hmac_signature.empty());
    EXPECT_EQ(rec.hmac_signature.size(), 64u);
    EXPECT_EQ(rec.hmac_signature,
              "1efcad679031ffd693172f2a8b16ac4524f0b9ae3d7171836b58b00a1c68f0bf");

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
    const std::string signature = "injected-signature";

    GGUFMetadata::setHmacFn(
        [signature](const std::string& data, const std::string& hmac_key) {
            (void)data;
            (void)hmac_key;
            return signature;
        });

    GGUFMetadata::sign(rec, key);
    EXPECT_EQ(rec.hmac_signature, signature);
    EXPECT_TRUE(GGUFMetadata::verify(rec, key));
    EXPECT_FALSE(GGUFMetadata::verify(rec, "wrong-key"));

    GGUFMetadata::setHmacFn(nullptr);

    auto rec_default = makeRecord();
    GGUFMetadata::sign(rec_default, "super_secret_key_42");
    EXPECT_EQ(rec_default.hmac_signature,
              "1efcad679031ffd693172f2a8b16ac4524f0b9ae3d7171836b58b00a1c68f0bf");
    EXPECT_TRUE(GGUFMetadata::verify(rec_default, "super_secret_key_42"));
}

} // anonymous namespace
