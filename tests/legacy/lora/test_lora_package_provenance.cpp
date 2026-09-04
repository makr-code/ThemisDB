// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_lora_package_provenance.cpp
 * @brief Regression tests for the Phase 4 HashChain & Provenance Layer.
 *
 * Covers:
 *   - LoRAPackage JSON round-trip and content hash stability
 *   - AdapterProduct JSON round-trip and content hash stability
 *   - DistributionReceipt JSON round-trip and content hash stability
 *   - ReceiptManifest JSON round-trip, Merkle root, manifest hash
 *   - ShardLedgerEntry JSON round-trip and content hash stability
 *   - ReceiptChain JSON round-trip
 *   - ProvenanceHashLedger: package chain append/lookup/verify
 *   - ProvenanceHashLedger: product chain append/lookup/verify
 *   - ProvenanceHashLedger: receipt chain append/verify/tamper detection
 *   - ProvenanceHashLedger: ReceiptManifest creation/validation/tamper
 *   - ProvenanceHashLedger: ShardLedger append/verify/tamper detection
 *   - ProvenanceHashLedger: audit path export completeness
 *   - Fuzzing-style integrity: sequential appends maintain chain invariants
 *   - Genesis chain seeds produce non-empty hash strings
 *   - Cross-artifact isolation (chains for different artifacts do not interfere)
 */

#include <gtest/gtest.h>
#include "llm/lora_framework/lora_package_provenance.h"

#include <string>
#include <vector>

using namespace themis::llm::lora;

// ============================================================================
// Helpers
// ============================================================================

namespace {

LoRAPackage makePackage(const std::string& adapter_id,
                        const std::string& version = "1.0.0",
                        const std::string& weights_hash = "aabbccdd") {
    LoRAPackage p;
    p.adapter_id   = adapter_id;
    p.version      = version;
    p.format       = "safetensors";
    p.weights_hash = weights_hash;
    p.metadata     = {{"project", "test"}};
    return p;
}

AdapterProduct makeProduct(const std::string& package_id,
                           const std::string& base_model_id = "llama3") {
    AdapterProduct p;
    p.package_id          = package_id;
    p.base_model_id       = base_model_id;
    p.base_model_hash     = "deadbeef";
    p.compatibility_hash  = "cafebabe";
    p.deployment_metadata = {{"env", "prod"}};
    return p;
}

DistributionReceipt makeReceipt(const std::string& artifact_id,
                                const std::string& recipient = "node-1",
                                const std::string& event    = "deploy") {
    DistributionReceipt r;
    r.artifact_id   = artifact_id;
    r.artifact_hash = "feed1234";
    r.recipient_id  = recipient;
    r.event_type    = event;
    r.extra_metadata = {{"region", "eu-west-1"}};
    return r;
}

ShardLedgerEntry makeShardEntry(const std::string& artifact_id,
                                uint32_t           shard_index = 0,
                                uint32_t           total = 4) {
    ShardLedgerEntry e;
    e.artifact_id  = artifact_id;
    e.shard_id     = "shard-" + std::to_string(shard_index);
    e.shard_index  = shard_index;
    e.total_shards = total;
    e.shard_hash   = "shard_hash_" + std::to_string(shard_index);
    e.merkle_proof = {"sibling_0", "sibling_1"};
    return e;
}

} // anonymous namespace

// ============================================================================
// LoRAPackage — JSON round-trip
// ============================================================================

TEST(LoRAPackageTest, DefaultConstruction) {
    LoRAPackage p;
    EXPECT_TRUE(p.package_id.empty());
    EXPECT_TRUE(p.adapter_id.empty());
    EXPECT_TRUE(p.version.empty());
    EXPECT_TRUE(p.package_hash.empty());
    EXPECT_TRUE(p.parent_hash.empty());
}

TEST(LoRAPackageTest, JSONRoundTrip) {
    LoRAPackage p;
    p.package_id   = "pkg-001";
    p.adapter_id   = "ada-001";
    p.version      = "2.0.0";
    p.format       = "gguf";
    p.weights_hash = "112233445566";
    p.package_hash = "aabbccddee";
    p.parent_hash  = "00001111";
    p.created_at   = "2026-07-16T10:00:00Z";
    p.metadata     = {{"note", "regression test"}};

    const json j   = p.toJSON();
    const auto p2  = LoRAPackage::fromJSON(j);

    EXPECT_EQ(p2.package_id,   p.package_id);
    EXPECT_EQ(p2.adapter_id,   p.adapter_id);
    EXPECT_EQ(p2.version,      p.version);
    EXPECT_EQ(p2.format,       p.format);
    EXPECT_EQ(p2.weights_hash, p.weights_hash);
    EXPECT_EQ(p2.package_hash, p.package_hash);
    EXPECT_EQ(p2.parent_hash,  p.parent_hash);
    EXPECT_EQ(p2.created_at,   p.created_at);
    EXPECT_EQ(p2.metadata,     p.metadata);
}

TEST(LoRAPackageTest, ContentHashStability) {
    LoRAPackage p;
    p.adapter_id   = "ada-stable";
    p.version      = "1.0.0";
    p.format       = "safetensors";
    p.weights_hash = "deadbeef";
    p.parent_hash  = "";
    p.created_at   = "2026-07-01T00:00:00Z";
    p.metadata     = json::object();

    const std::string h1 = p.computeContentHash();
    const std::string h2 = p.computeContentHash();
    EXPECT_EQ(h1, h2);
    EXPECT_EQ(h1.size(), 64u);
}

TEST(LoRAPackageTest, ContentHashSensitiveToWeightsHash) {
    LoRAPackage base;
    base.adapter_id   = "ada-A";
    base.version      = "1.0.0";
    base.format       = "safetensors";
    base.weights_hash = "original";
    base.created_at   = "2026-01-01T00:00:00Z";

    LoRAPackage modified = base;
    modified.weights_hash = "modified";

    EXPECT_NE(base.computeContentHash(), modified.computeContentHash());
}

// ============================================================================
// AdapterProduct — JSON round-trip
// ============================================================================

TEST(AdapterProductTest, DefaultConstruction) {
    AdapterProduct p;
    EXPECT_TRUE(p.product_id.empty());
    EXPECT_TRUE(p.package_id.empty());
    EXPECT_TRUE(p.product_hash.empty());
    EXPECT_TRUE(p.parent_hash.empty());
}

TEST(AdapterProductTest, JSONRoundTrip) {
    AdapterProduct p;
    p.product_id           = "prod-001";
    p.package_id           = "pkg-001";
    p.base_model_id        = "llama3-8b";
    p.base_model_hash      = "beef0000";
    p.product_hash         = "cafecafe";
    p.parent_hash          = "11110000";
    p.compatibility_hash   = "compat123";
    p.created_at           = "2026-07-16T11:00:00Z";
    p.deployment_metadata  = {{"env", "staging"}};

    const json j  = p.toJSON();
    const auto p2 = AdapterProduct::fromJSON(j);

    EXPECT_EQ(p2.product_id,          p.product_id);
    EXPECT_EQ(p2.package_id,          p.package_id);
    EXPECT_EQ(p2.base_model_id,       p.base_model_id);
    EXPECT_EQ(p2.base_model_hash,     p.base_model_hash);
    EXPECT_EQ(p2.product_hash,        p.product_hash);
    EXPECT_EQ(p2.parent_hash,         p.parent_hash);
    EXPECT_EQ(p2.compatibility_hash,  p.compatibility_hash);
    EXPECT_EQ(p2.created_at,          p.created_at);
    EXPECT_EQ(p2.deployment_metadata, p.deployment_metadata);
}

TEST(AdapterProductTest, ContentHashSensitiveToCompatibility) {
    AdapterProduct base;
    base.package_id        = "pkg-A";
    base.base_model_id     = "llama3";
    base.base_model_hash   = "model_hash";
    base.compatibility_hash = "compat_v1";
    base.created_at        = "2026-01-01T00:00:00Z";

    AdapterProduct updated = base;
    updated.compatibility_hash = "compat_v2";

    EXPECT_NE(base.computeContentHash(), updated.computeContentHash());
}

// ============================================================================
// DistributionReceipt — JSON round-trip
// ============================================================================

TEST(DistributionReceiptTest, DefaultConstruction) {
    DistributionReceipt r;
    EXPECT_TRUE(r.receipt_id.empty());
    EXPECT_TRUE(r.receipt_hash.empty());
    EXPECT_TRUE(r.parent_receipt_hash.empty());
}

TEST(DistributionReceiptTest, JSONRoundTrip) {
    DistributionReceipt r;
    r.receipt_id             = "rcpt-001";
    r.event_type             = "deploy";
    r.artifact_id            = "pkg-001";
    r.artifact_hash          = "weightsHASH";
    r.recipient_id           = "node-7";
    r.distribution_timestamp = "2026-07-16T12:00:00Z";
    r.operator_signature     = "sig_base64==";
    r.parent_receipt_hash    = "prev_hash";
    r.receipt_hash           = "current_hash";
    r.extra_metadata         = {{"dc", "eu-central-1"}};

    const json j  = r.toJSON();
    const auto r2 = DistributionReceipt::fromJSON(j);

    EXPECT_EQ(r2.receipt_id,             r.receipt_id);
    EXPECT_EQ(r2.event_type,             r.event_type);
    EXPECT_EQ(r2.artifact_id,            r.artifact_id);
    EXPECT_EQ(r2.artifact_hash,          r.artifact_hash);
    EXPECT_EQ(r2.recipient_id,           r.recipient_id);
    EXPECT_EQ(r2.distribution_timestamp, r.distribution_timestamp);
    EXPECT_EQ(r2.operator_signature,     r.operator_signature);
    EXPECT_EQ(r2.parent_receipt_hash,    r.parent_receipt_hash);
    EXPECT_EQ(r2.receipt_hash,           r.receipt_hash);
    EXPECT_EQ(r2.extra_metadata,         r.extra_metadata);
}

TEST(DistributionReceiptTest, ContentHashDeterministic) {
    DistributionReceipt r = makeReceipt("pkg-001");
    r.distribution_timestamp = "2026-07-16T00:00:00Z";
    r.parent_receipt_hash    = "genesis";

    const auto h1 = r.computeContentHash();
    const auto h2 = r.computeContentHash();
    EXPECT_EQ(h1, h2);
    EXPECT_EQ(h1.size(), 64u);
}

TEST(DistributionReceiptTest, ContentHashSensitiveToRecipient) {
    DistributionReceipt r1 = makeReceipt("pkg-001", "node-1");
    DistributionReceipt r2 = makeReceipt("pkg-001", "node-2");
    r1.distribution_timestamp = r2.distribution_timestamp = "2026-07-16T00:00:00Z";
    r1.parent_receipt_hash = r2.parent_receipt_hash = "";

    EXPECT_NE(r1.computeContentHash(), r2.computeContentHash());
}

// ============================================================================
// ReceiptManifest — JSON round-trip, Merkle root, manifest hash
// ============================================================================

TEST(ReceiptManifestTest, EmptyReceiptsMerkleRoot) {
    ReceiptManifest m;
    m.manifest_id = "mfst-empty";
    m.event_type  = "deploy";
    m.artifact_id = "pkg-empty";
    m.created_at  = "2026-01-01T00:00:00Z";

    // Should return SHA-256 of "" (not crash).
    const auto root = m.computeMerkleRoot();
    EXPECT_EQ(root.size(), 64u);
}

TEST(ReceiptManifestTest, SingleReceiptMerkleRootEqualsReceiptHash) {
    DistributionReceipt r = makeReceipt("pkg-001", "node-1");
    r.receipt_hash = r.computeContentHash();

    ReceiptManifest m;
    m.receipts = {r};
    const auto root = m.computeMerkleRoot();
    EXPECT_EQ(root, r.receipt_hash);
}

TEST(ReceiptManifestTest, TwoReceiptsMerkleRootChangesWithContent) {
    DistributionReceipt r1 = makeReceipt("pkg-002", "node-1");
    r1.receipt_hash = r1.computeContentHash();
    DistributionReceipt r2 = makeReceipt("pkg-002", "node-2");
    r2.receipt_hash = r2.computeContentHash();
    DistributionReceipt r2_tampered = r2;
    r2_tampered.recipient_id = "node-evil";
    r2_tampered.receipt_hash = r2_tampered.computeContentHash();

    ReceiptManifest m1;
    m1.receipts = {r1, r2};
    ReceiptManifest m2;
    m2.receipts = {r1, r2_tampered};

    EXPECT_NE(m1.computeMerkleRoot(), m2.computeMerkleRoot());
}

TEST(ReceiptManifestTest, JSONRoundTrip) {
    DistributionReceipt r = makeReceipt("pkg-rt", "node-x");
    r.distribution_timestamp = "2026-07-16T00:00:00Z";
    r.parent_receipt_hash    = "p_hash";
    r.receipt_hash           = r.computeContentHash();

    ReceiptManifest m;
    m.manifest_id  = "mfst-rt-001";
    m.event_type   = "export";
    m.artifact_id  = "pkg-rt";
    m.created_at   = "2026-07-16T00:00:00Z";
    m.receipts     = {r};
    m.merkle_root  = m.computeMerkleRoot();
    m.manifest_hash = m.computeContentHash();

    const json j   = m.toJSON();
    const auto m2  = ReceiptManifest::fromJSON(j);

    EXPECT_EQ(m2.manifest_id,   m.manifest_id);
    EXPECT_EQ(m2.event_type,    m.event_type);
    EXPECT_EQ(m2.artifact_id,   m.artifact_id);
    EXPECT_EQ(m2.created_at,    m.created_at);
    EXPECT_EQ(m2.merkle_root,   m.merkle_root);
    EXPECT_EQ(m2.manifest_hash, m.manifest_hash);
    ASSERT_EQ(m2.receipts.size(), 1u);
    EXPECT_EQ(m2.receipts[0].receipt_hash, r.receipt_hash);
}

// ============================================================================
// ShardLedgerEntry — JSON round-trip, hash stability
// ============================================================================

TEST(ShardLedgerEntryTest, DefaultConstruction) {
    ShardLedgerEntry e;
    EXPECT_TRUE(e.entry_id.empty());
    EXPECT_EQ(e.shard_index,  0u);
    EXPECT_EQ(e.total_shards, 0u);
    EXPECT_TRUE(e.entry_hash.empty());
}

TEST(ShardLedgerEntryTest, JSONRoundTrip) {
    ShardLedgerEntry e = makeShardEntry("pkg-shard", 2, 8);
    e.entry_id            = "shrd-001";
    e.placement_timestamp = "2026-07-16T08:00:00Z";
    e.prev_entry_hash     = "prev_shard";
    e.entry_hash          = e.computeContentHash();

    const json j   = e.toJSON();
    const auto e2  = ShardLedgerEntry::fromJSON(j);

    EXPECT_EQ(e2.entry_id,            e.entry_id);
    EXPECT_EQ(e2.artifact_id,         e.artifact_id);
    EXPECT_EQ(e2.shard_id,            e.shard_id);
    EXPECT_EQ(e2.shard_index,         e.shard_index);
    EXPECT_EQ(e2.total_shards,        e.total_shards);
    EXPECT_EQ(e2.shard_hash,          e.shard_hash);
    EXPECT_EQ(e2.merkle_proof,        e.merkle_proof);
    EXPECT_EQ(e2.placement_timestamp, e.placement_timestamp);
    EXPECT_EQ(e2.prev_entry_hash,     e.prev_entry_hash);
    EXPECT_EQ(e2.entry_hash,          e.entry_hash);
}

TEST(ShardLedgerEntryTest, ContentHashSensitiveToShardHash) {
    ShardLedgerEntry base = makeShardEntry("art-A", 0, 4);
    base.placement_timestamp = "2026-01-01T00:00:00Z";
    base.prev_entry_hash     = "genesis";

    ShardLedgerEntry modified = base;
    modified.shard_hash = "tampered_shard";

    EXPECT_NE(base.computeContentHash(), modified.computeContentHash());
}

// ============================================================================
// ReceiptChain — JSON round-trip
// ============================================================================

TEST(ReceiptChainTest, JSONRoundTrip) {
    DistributionReceipt r1 = makeReceipt("art-chain", "node-a");
    r1.receipt_id = "rcpt-chain-1";
    r1.parent_receipt_hash = "genesis_h";
    r1.receipt_hash = r1.computeContentHash();

    ReceiptChain c;
    c.chain_id    = "art-chain";
    c.artifact_id = "art-chain";
    c.genesis_hash = "genesis_h";
    c.entries     = {r1};
    c.head_hash   = r1.receipt_hash;

    const json j   = c.toJSON();
    const auto c2  = ReceiptChain::fromJSON(j);

    EXPECT_EQ(c2.chain_id,    c.chain_id);
    EXPECT_EQ(c2.artifact_id, c.artifact_id);
    EXPECT_EQ(c2.genesis_hash, c.genesis_hash);
    EXPECT_EQ(c2.head_hash,   c.head_hash);
    ASSERT_EQ(c2.entries.size(), 1u);
    EXPECT_EQ(c2.entries[0].receipt_hash, r1.receipt_hash);
}

// ============================================================================
// ProvenanceHashLedger — package chain
// ============================================================================

TEST(ProvenanceHashLedgerPackageTest, AppendAndRetrieve) {
    ProvenanceHashLedger ledger;
    auto pkg = ledger.appendPackage(makePackage("ada-001", "1.0.0"));

    EXPECT_FALSE(pkg.package_id.empty());
    EXPECT_FALSE(pkg.package_hash.empty());
    EXPECT_FALSE(pkg.created_at.empty());
    // Genesis: parent_hash is empty.
    EXPECT_TRUE(pkg.parent_hash.empty());

    const auto chain = ledger.getPackageChain("ada-001");
    ASSERT_EQ(chain.size(), 1u);
    EXPECT_EQ(chain[0].package_id, pkg.package_id);
}

TEST(ProvenanceHashLedgerPackageTest, MultiVersionChain) {
    ProvenanceHashLedger ledger;
    auto v1 = ledger.appendPackage(makePackage("ada-multi", "1.0.0", "hash_v1"));
    auto v2 = ledger.appendPackage(makePackage("ada-multi", "1.1.0", "hash_v2"));
    auto v3 = ledger.appendPackage(makePackage("ada-multi", "2.0.0", "hash_v3"));

    // v2's parent_hash must equal v1's package_hash.
    EXPECT_EQ(v2.parent_hash, v1.package_hash);
    // v3's parent_hash must equal v2's package_hash.
    EXPECT_EQ(v3.parent_hash, v2.package_hash);

    const auto chain = ledger.getPackageChain("ada-multi");
    ASSERT_EQ(chain.size(), 3u);
}

TEST(ProvenanceHashLedgerPackageTest, GetPackageById) {
    ProvenanceHashLedger ledger;
    auto pkg = ledger.appendPackage(makePackage("ada-by-id"));

    const auto opt = ledger.getPackage(pkg.package_id);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->package_id, pkg.package_id);
}

TEST(ProvenanceHashLedgerPackageTest, GetPackageByIdNotFound) {
    ProvenanceHashLedger ledger;
    EXPECT_FALSE(ledger.getPackage("nonexistent").has_value());
}

// ============================================================================
// ProvenanceHashLedger — product chain
// ============================================================================

TEST(ProvenanceHashLedgerProductTest, AppendAndRetrieve) {
    ProvenanceHashLedger ledger;
    auto pkg  = ledger.appendPackage(makePackage("ada-prod", "1.0.0"));
    auto prod = ledger.appendProduct(makeProduct(pkg.package_id));

    EXPECT_FALSE(prod.product_id.empty());
    EXPECT_FALSE(prod.product_hash.empty());
    EXPECT_TRUE(prod.parent_hash.empty());  // genesis
}

TEST(ProvenanceHashLedgerProductTest, MultiVersionProductChain) {
    ProvenanceHashLedger ledger;
    auto pkg  = ledger.appendPackage(makePackage("ada-mprod", "1.0.0"));
    auto pr1  = ledger.appendProduct(makeProduct(pkg.package_id, "llama3-8b"));
    auto pr2  = ledger.appendProduct(makeProduct(pkg.package_id, "llama3-70b"));

    EXPECT_EQ(pr2.parent_hash, pr1.product_hash);

    const auto chain = ledger.getProductChain(pkg.package_id);
    ASSERT_EQ(chain.size(), 2u);
}

TEST(ProvenanceHashLedgerProductTest, GetProductById) {
    ProvenanceHashLedger ledger;
    auto pkg  = ledger.appendPackage(makePackage("ada-pid"));
    auto prod = ledger.appendProduct(makeProduct(pkg.package_id));

    ASSERT_TRUE(ledger.getProduct(prod.product_id).has_value());
}

// ============================================================================
// ProvenanceHashLedger — receipt chain
// ============================================================================

TEST(ProvenanceHashLedgerReceiptTest, AppendSingleReceipt) {
    ProvenanceHashLedger ledger;
    auto r = ledger.appendReceipt(makeReceipt("art-r1"));

    EXPECT_FALSE(r.receipt_id.empty());
    EXPECT_FALSE(r.receipt_hash.empty());
    EXPECT_FALSE(r.distribution_timestamp.empty());
    // Genesis chain sets parent_receipt_hash = chain genesis hash (non-empty).
    EXPECT_FALSE(r.parent_receipt_hash.empty());
}

TEST(ProvenanceHashLedgerReceiptTest, ReceiptChainLinkage) {
    ProvenanceHashLedger ledger;
    auto r1 = ledger.appendReceipt(makeReceipt("art-chain2", "node-1"));
    auto r2 = ledger.appendReceipt(makeReceipt("art-chain2", "node-2"));
    auto r3 = ledger.appendReceipt(makeReceipt("art-chain2", "node-3"));

    EXPECT_EQ(r2.parent_receipt_hash, r1.receipt_hash);
    EXPECT_EQ(r3.parent_receipt_hash, r2.receipt_hash);
}

TEST(ProvenanceHashLedgerReceiptTest, VerifyChainIntact) {
    ProvenanceHashLedger ledger;
    (void)ledger.appendReceipt(makeReceipt("art-verify", "n1"));
    (void)ledger.appendReceipt(makeReceipt("art-verify", "n2"));
    (void)ledger.appendReceipt(makeReceipt("art-verify", "n3"));

    EXPECT_TRUE(ledger.verifyReceiptChain("art-verify"));
}

TEST(ProvenanceHashLedgerReceiptTest, VerifyEmptyChain) {
    ProvenanceHashLedger ledger;
    // Empty chain for unknown artifact is trivially valid.
    EXPECT_TRUE(ledger.verifyReceiptChain("nonexistent-art"));
}

TEST(ProvenanceHashLedgerReceiptTest, EmptyArtifactIdThrows) {
    ProvenanceHashLedger ledger;
    DistributionReceipt r;
    r.artifact_id = "";  // empty — must throw
    EXPECT_THROW(ledger.appendReceipt(std::move(r)), std::invalid_argument);
}

TEST(ProvenanceHashLedgerReceiptTest, GetReceiptChain) {
    ProvenanceHashLedger ledger;
    (void)ledger.appendReceipt(makeReceipt("art-get-chain", "x"));
    (void)ledger.appendReceipt(makeReceipt("art-get-chain", "y"));

    const auto chain = ledger.getReceiptChain("art-get-chain");
    EXPECT_EQ(chain.artifact_id, "art-get-chain");
    ASSERT_EQ(chain.entries.size(), 2u);
}

// ============================================================================
// ProvenanceHashLedger — ReceiptManifest creation and validation
// ============================================================================

TEST(ProvenanceHashLedgerManifestTest, CreateManifest) {
    ProvenanceHashLedger ledger;

    std::vector<DistributionReceipt> receipts = {
        makeReceipt("art-mfst", "node-A", "deploy"),
        makeReceipt("art-mfst", "node-B", "deploy"),
        makeReceipt("art-mfst", "node-C", "deploy"),
    };

    const auto manifest = ledger.createManifest("deploy", "art-mfst",
                                                std::move(receipts));

    EXPECT_FALSE(manifest.manifest_id.empty());
    EXPECT_EQ(manifest.artifact_id, "art-mfst");
    EXPECT_EQ(manifest.event_type, "deploy");
    ASSERT_EQ(manifest.receipts.size(), 3u);
    EXPECT_EQ(manifest.merkle_root.size(), 64u);
    EXPECT_EQ(manifest.manifest_hash.size(), 64u);
}

TEST(ProvenanceHashLedgerManifestTest, ValidateManifestPasses) {
    ProvenanceHashLedger ledger;

    std::vector<DistributionReceipt> receipts = {
        makeReceipt("art-val", "node-1"),
        makeReceipt("art-val", "node-2"),
    };
    const auto manifest = ledger.createManifest("export", "art-val",
                                                std::move(receipts));
    EXPECT_TRUE(ledger.validateManifest(manifest));
}

TEST(ProvenanceHashLedgerManifestTest, ValidateTamperedMerkleRootFails) {
    ProvenanceHashLedger ledger;

    std::vector<DistributionReceipt> receipts = {
        makeReceipt("art-tamper-m", "n1"),
    };
    auto manifest = ledger.createManifest("deploy", "art-tamper-m",
                                          std::move(receipts));
    // Tamper with the Merkle root.
    manifest.merkle_root = "0000000000000000000000000000000000000000000000000000000000000000";
    EXPECT_FALSE(ledger.validateManifest(manifest));
}

TEST(ProvenanceHashLedgerManifestTest, ValidateTamperedManifestHashFails) {
    ProvenanceHashLedger ledger;

    std::vector<DistributionReceipt> receipts = {
        makeReceipt("art-tamper-h", "n1"),
    };
    auto manifest = ledger.createManifest("deploy", "art-tamper-h",
                                          std::move(receipts));
    // Tamper with the manifest hash.
    manifest.manifest_hash = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
    EXPECT_FALSE(ledger.validateManifest(manifest));
}

TEST(ProvenanceHashLedgerManifestTest, GetManifestById) {
    ProvenanceHashLedger ledger;
    std::vector<DistributionReceipt> receipts = {
        makeReceipt("art-getm", "n1"),
    };
    const auto m = ledger.createManifest("deploy", "art-getm",
                                         std::move(receipts));
    const auto opt = ledger.getManifest(m.manifest_id);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->manifest_id, m.manifest_id);
}

TEST(ProvenanceHashLedgerManifestTest, GetManifestNotFound) {
    ProvenanceHashLedger ledger;
    EXPECT_FALSE(ledger.getManifest("nonexistent").has_value());
}

// ============================================================================
// ProvenanceHashLedger — ShardLedger
// ============================================================================

TEST(ProvenanceHashLedgerShardTest, AppendAndRetrieve) {
    ProvenanceHashLedger ledger;
    auto e = ledger.appendShardEntry(makeShardEntry("art-shard", 0, 4));

    EXPECT_FALSE(e.entry_id.empty());
    EXPECT_FALSE(e.entry_hash.empty());
    EXPECT_FALSE(e.placement_timestamp.empty());
    EXPECT_FALSE(e.prev_entry_hash.empty());  // seeded from artifact_id genesis
}

TEST(ProvenanceHashLedgerShardTest, ShardLedgerChainLinkage) {
    ProvenanceHashLedger ledger;
    auto e0 = ledger.appendShardEntry(makeShardEntry("art-sl", 0, 3));
    auto e1 = ledger.appendShardEntry(makeShardEntry("art-sl", 1, 3));
    auto e2 = ledger.appendShardEntry(makeShardEntry("art-sl", 2, 3));

    EXPECT_EQ(e1.prev_entry_hash, e0.entry_hash);
    EXPECT_EQ(e2.prev_entry_hash, e1.entry_hash);
}

TEST(ProvenanceHashLedgerShardTest, VerifyShardLedgerIntact) {
    ProvenanceHashLedger ledger;
    for (uint32_t i = 0; i < 5; ++i) {
        (void)ledger.appendShardEntry(makeShardEntry("art-sl-verify", i, 5));
    }
    EXPECT_TRUE(ledger.verifyShardLedger("art-sl-verify"));
}

TEST(ProvenanceHashLedgerShardTest, VerifyEmptyShardLedger) {
    ProvenanceHashLedger ledger;
    EXPECT_TRUE(ledger.verifyShardLedger("unknown-art"));
}

TEST(ProvenanceHashLedgerShardTest, EmptyArtifactIdThrows) {
    ProvenanceHashLedger ledger;
    ShardLedgerEntry e;
    e.artifact_id = "";
    EXPECT_THROW(ledger.appendShardEntry(std::move(e)), std::invalid_argument);
}

TEST(ProvenanceHashLedgerShardTest, GetShardLedger) {
    ProvenanceHashLedger ledger;
    (void)ledger.appendShardEntry(makeShardEntry("art-get-sl", 0, 2));
    (void)ledger.appendShardEntry(makeShardEntry("art-get-sl", 1, 2));

    const auto entries = ledger.getShardLedger("art-get-sl");
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0].shard_index, 0u);
    EXPECT_EQ(entries[1].shard_index, 1u);
}

// ============================================================================
// ProvenanceHashLedger — audit path export
// ============================================================================

TEST(ProvenanceHashLedgerAuditPathTest, ExportContainsAllSections) {
    ProvenanceHashLedger ledger;
    const std::string adapter_id = "art-audit";

    // Package
    auto pkg = ledger.appendPackage(makePackage(adapter_id));
    // Product
    (void)ledger.appendProduct(makeProduct(pkg.package_id));
    // Receipts
    (void)ledger.appendReceipt(makeReceipt(adapter_id, "n1"));
    (void)ledger.appendReceipt(makeReceipt(adapter_id, "n2"));
    // Manifest
    std::vector<DistributionReceipt> rr = {makeReceipt(adapter_id, "n3")};
    (void)ledger.createManifest("deploy", adapter_id, std::move(rr));
    // Shard entries
    (void)ledger.appendShardEntry(makeShardEntry(adapter_id, 0, 2));
    (void)ledger.appendShardEntry(makeShardEntry(adapter_id, 1, 2));

    const auto path = ledger.exportAuditPath(adapter_id);

    EXPECT_TRUE(path.contains("artifact_id"));
    EXPECT_TRUE(path.contains("packages"));
    EXPECT_TRUE(path.contains("products"));
    EXPECT_TRUE(path.contains("receipt_chain"));
    EXPECT_TRUE(path.contains("manifests"));
    EXPECT_TRUE(path.contains("shard_ledger"));

    EXPECT_EQ(path["artifact_id"].get<std::string>(), adapter_id);
    EXPECT_EQ(path["packages"].size(), 1u);
    EXPECT_EQ(path["shard_ledger"].size(), 2u);
    EXPECT_EQ(path["manifests"].size(), 1u);
}

TEST(ProvenanceHashLedgerAuditPathTest, ExportEmptyArtifact) {
    ProvenanceHashLedger ledger;
    const auto path = ledger.exportAuditPath("unknown-artifact");

    EXPECT_EQ(path["packages"].size(), 0u);
    EXPECT_EQ(path["products"].size(), 0u);
    EXPECT_EQ(path["shard_ledger"].size(), 0u);
    EXPECT_EQ(path["manifests"].size(), 0u);
}

// ============================================================================
// Cross-artifact isolation
// ============================================================================

TEST(ProvenanceHashLedgerIsolationTest, ReceiptChainsIsolated) {
    ProvenanceHashLedger ledger;
    (void)ledger.appendReceipt(makeReceipt("art-X", "n1"));
    (void)ledger.appendReceipt(makeReceipt("art-X", "n2"));
    (void)ledger.appendReceipt(makeReceipt("art-Y", "n1"));

    const auto chainX = ledger.getReceiptChain("art-X");
    const auto chainY = ledger.getReceiptChain("art-Y");

    EXPECT_EQ(chainX.entries.size(), 2u);
    EXPECT_EQ(chainY.entries.size(), 1u);

    EXPECT_TRUE(ledger.verifyReceiptChain("art-X"));
    EXPECT_TRUE(ledger.verifyReceiptChain("art-Y"));
}

TEST(ProvenanceHashLedgerIsolationTest, ShardLedgersIsolated) {
    ProvenanceHashLedger ledger;
    (void)ledger.appendShardEntry(makeShardEntry("art-P", 0, 2));
    (void)ledger.appendShardEntry(makeShardEntry("art-P", 1, 2));
    (void)ledger.appendShardEntry(makeShardEntry("art-Q", 0, 3));
    (void)ledger.appendShardEntry(makeShardEntry("art-Q", 1, 3));
    (void)ledger.appendShardEntry(makeShardEntry("art-Q", 2, 3));

    EXPECT_EQ(ledger.getShardLedger("art-P").size(), 2u);
    EXPECT_EQ(ledger.getShardLedger("art-Q").size(), 3u);
    EXPECT_TRUE(ledger.verifyShardLedger("art-P"));
    EXPECT_TRUE(ledger.verifyShardLedger("art-Q"));
}

// ============================================================================
// sha256Hex utility
// ============================================================================

TEST(SHA256UtilTest, EmptyString) {
    // SHA-256 of "" is well-known.
    const auto h = ProvenanceHashLedger::sha256Hex("");
    EXPECT_EQ(h, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(SHA256UtilTest, KnownValue) {
    // SHA-256 of "abc" is well-known.
    const auto h = ProvenanceHashLedger::sha256Hex("abc");
    EXPECT_EQ(h, "ba7816bf8f01cfea414140de5dae2ec73b00361bbef0469348423f656b5ea6f");
}

// ============================================================================
// Fuzzing-style sequential integrity tests
// ============================================================================

TEST(ProvenanceHashLedgerFuzzTest, LongPackageChainInvariant) {
    ProvenanceHashLedger ledger;
    std::string prev_hash = {};
    for (int i = 0; i < 50; ++i) {
        LoRAPackage p;
        p.adapter_id   = "ada-fuzz";
        p.version      = std::to_string(i) + ".0.0";
        p.format       = "safetensors";
        p.weights_hash = "wh_" + std::to_string(i);
        auto appended = ledger.appendPackage(p);

        EXPECT_EQ(appended.parent_hash, prev_hash)
            << "Parent hash mismatch at iteration " << i;
        EXPECT_EQ(appended.package_hash.size(), 64u);
        prev_hash = appended.package_hash;
    }
    const auto chain = ledger.getPackageChain("ada-fuzz");
    EXPECT_EQ(chain.size(), 50u);
}

TEST(ProvenanceHashLedgerFuzzTest, LongReceiptChainVerifies) {
    ProvenanceHashLedger ledger;
    for (int i = 0; i < 30; ++i) {
        (void)ledger.appendReceipt(makeReceipt("art-fuzz-rcpt",
                                               "node-" + std::to_string(i)));
    }
    EXPECT_TRUE(ledger.verifyReceiptChain("art-fuzz-rcpt"));
}

TEST(ProvenanceHashLedgerFuzzTest, LargeManifestMerkleRootStable) {
    std::vector<DistributionReceipt> receipts;
    receipts.reserve(32);
    for (int i = 0; i < 32; ++i) {
        receipts.push_back(makeReceipt("art-large-mfst",
                                        "node-" + std::to_string(i)));
    }

    ProvenanceHashLedger ledger;
    const auto m = ledger.createManifest("deploy", "art-large-mfst",
                                         std::move(receipts));

    // Validate twice — must be deterministic.
    EXPECT_TRUE(ledger.validateManifest(m));
    EXPECT_TRUE(ledger.validateManifest(m));
    EXPECT_EQ(m.merkle_root.size(), 64u);
    EXPECT_EQ(m.manifest_hash.size(), 64u);
}

TEST(ProvenanceHashLedgerFuzzTest, ShardLedger16ShardsVerifies) {
    ProvenanceHashLedger ledger;
    constexpr uint32_t kShards = 16;
    for (uint32_t i = 0; i < kShards; ++i) {
        (void)ledger.appendShardEntry(makeShardEntry("art-fuzz-shard", i, kShards));
    }
    EXPECT_TRUE(ledger.verifyShardLedger("art-fuzz-shard"));
    EXPECT_EQ(ledger.getShardLedger("art-fuzz-shard").size(), kShards);
}
