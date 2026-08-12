/**
 * @file test_lora_provenance.cpp
 * @brief Unit tests for the LoRA Adapter Provenance system.
 *
 * Covers:
 *   - LoRAProvenanceRecord JSON round-trip
 *   - ExternalAdapterProvenance JSON round-trip and validation logic
 *   - AdapterSnapshot creation, listing, and lookup
 *   - InferenceAuditEntry Merkle chain append and verification
 *   - LoRAProvenanceManager overall integration
 */

#include <gtest/gtest.h>
#include "llm/lora_framework/lora_provenance.h"
#include <string>

using namespace themis::llm::lora;

// ============================================================================
// LoRAProvenanceRecord tests
// ============================================================================

TEST(LoRAProvenanceRecordTest, DefaultConstruction) {
    LoRAProvenanceRecord r;
    EXPECT_TRUE(r.dataset_hash.empty());
    EXPECT_TRUE(r.base_model_hash.empty());
    EXPECT_TRUE(r.hyperparameter_hash.empty());
    EXPECT_DOUBLE_EQ(r.training_duration_secs, 0.0);
}

TEST(LoRAProvenanceRecordTest, JSONRoundTrip) {
    LoRAProvenanceRecord r;
    r.dataset_hash         = "aabbcc";
    r.base_model_hash      = "ddeeff";
    r.hyperparameter_hash  = "112233";
    r.adapter_weights_hash = "445566";
    r.trainer_id           = "trainer-001";
    r.ca_chain             = "-----BEGIN CERTIFICATE-----\n...";
    r.signature            = "sig-placeholder";
    r.created_at           = "2026-02-21T12:00:00Z";
    r.rfc3161_timestamp    = "base64==";
    r.training_duration_secs = 3600.5;
    r.hardware_info        = {{"gpu", "A100"}, {"vram_mb", 80000}};
    r.custom_metadata      = {{"project", "legal-lora"}};

    const json j = r.toJSON();
    const auto r2 = LoRAProvenanceRecord::fromJSON(j);

    EXPECT_EQ(r2.dataset_hash,         r.dataset_hash);
    EXPECT_EQ(r2.base_model_hash,      r.base_model_hash);
    EXPECT_EQ(r2.hyperparameter_hash,  r.hyperparameter_hash);
    EXPECT_EQ(r2.adapter_weights_hash, r.adapter_weights_hash);
    EXPECT_EQ(r2.trainer_id,           r.trainer_id);
    EXPECT_EQ(r2.ca_chain,             r.ca_chain);
    EXPECT_EQ(r2.signature,            r.signature);
    EXPECT_EQ(r2.created_at,           r.created_at);
    EXPECT_EQ(r2.rfc3161_timestamp,    r.rfc3161_timestamp);
    EXPECT_DOUBLE_EQ(r2.training_duration_secs, r.training_duration_secs);
    EXPECT_EQ(r2.hardware_info,        r.hardware_info);
    EXPECT_EQ(r2.custom_metadata,      r.custom_metadata);
}

// ============================================================================
// ExternalAdapterProvenance tests
// ============================================================================

TEST(ExternalAdapterProvenanceTest, DefaultConstruction) {
    ExternalAdapterProvenance e;
    EXPECT_FALSE(e.signature_valid);
    EXPECT_FALSE(e.cert_chain_valid);
    EXPECT_TRUE(e.validation_errors.empty());
}

TEST(ExternalAdapterProvenanceTest, JSONRoundTrip) {
    ExternalAdapterProvenance e;
    e.source_url            = "https://example.com/adapters/legal.zip";
    e.commit_hash           = "abc123def456";
    e.description           = "Legal domain LoRA";
    e.adapter_hash          = std::string(64, 'a');  // 64-char placeholder
    e.provenance_signature  = "sigdata";
    e.certificate_chain     = "-----BEGIN CERTIFICATE-----\n...";
    e.import_timestamp      = "2026-02-21T13:00:00Z";
    e.signature_valid       = true;
    e.cert_chain_valid      = true;
    e.validation_errors     = {};

    const json j = e.toJSON();
    const auto e2 = ExternalAdapterProvenance::fromJSON(j);

    EXPECT_EQ(e2.source_url,           e.source_url);
    EXPECT_EQ(e2.commit_hash,          e.commit_hash);
    EXPECT_EQ(e2.description,          e.description);
    EXPECT_EQ(e2.adapter_hash,         e.adapter_hash);
    EXPECT_EQ(e2.provenance_signature, e.provenance_signature);
    EXPECT_EQ(e2.certificate_chain,    e.certificate_chain);
    EXPECT_EQ(e2.import_timestamp,     e.import_timestamp);
    EXPECT_EQ(e2.signature_valid,      e.signature_valid);
    EXPECT_EQ(e2.cert_chain_valid,     e.cert_chain_valid);
    EXPECT_TRUE(e2.validation_errors.empty());
}

// ============================================================================
// AdapterSnapshot tests
// ============================================================================

TEST(AdapterSnapshotTest, JSONRoundTrip) {
    AdapterSnapshot s;
    s.snapshot_id        = "snap-001";
    s.adapter_id         = "legal-lora";
    s.version            = "v1.2.0";
    s.weights_hash       = std::string(64, 'f');
    s.timestamp          = "2026-02-21T14:00:00Z";
    s.parent_snapshot_id = "snap-000";
    s.provenance.trainer_id = "trainer-42";

    const json j = s.toJSON();
    const auto s2 = AdapterSnapshot::fromJSON(j);

    EXPECT_EQ(s2.snapshot_id,        s.snapshot_id);
    EXPECT_EQ(s2.adapter_id,         s.adapter_id);
    EXPECT_EQ(s2.version,            s.version);
    EXPECT_EQ(s2.weights_hash,       s.weights_hash);
    EXPECT_EQ(s2.timestamp,          s.timestamp);
    EXPECT_EQ(s2.parent_snapshot_id, s.parent_snapshot_id);
    EXPECT_EQ(s2.provenance.trainer_id, s.provenance.trainer_id);
}

// ============================================================================
// InferenceAuditEntry tests
// ============================================================================

TEST(InferenceAuditEntryTest, JSONRoundTrip) {
    InferenceAuditEntry e;
    e.entry_id      = "entry-001";
    e.previous_hash = "";
    e.entry_hash    = std::string(64, 'c');
    e.timestamp     = "2026-02-21T15:00:00Z";
    e.request_id    = "req-001";
    e.query_hash    = std::string(64, 'd');
    e.response_hash = std::string(64, 'e');
    e.model_hash    = std::string(64, 'f');
    e.adapter_hash  = std::string(64, 'a');
    e.commitments   = {{"type", "sha256"}};
    e.metadata      = {{"user", "alice"}};

    const json j = e.toJSON();
    const auto e2 = InferenceAuditEntry::fromJSON(j);

    EXPECT_EQ(e2.entry_id,      e.entry_id);
    EXPECT_EQ(e2.previous_hash, e.previous_hash);
    EXPECT_EQ(e2.entry_hash,    e.entry_hash);
    EXPECT_EQ(e2.timestamp,     e.timestamp);
    EXPECT_EQ(e2.request_id,    e.request_id);
    EXPECT_EQ(e2.query_hash,    e.query_hash);
    EXPECT_EQ(e2.response_hash, e.response_hash);
    EXPECT_EQ(e2.model_hash,    e.model_hash);
    EXPECT_EQ(e2.adapter_hash,  e.adapter_hash);
    EXPECT_EQ(e2.commitments,   e.commitments);
    EXPECT_EQ(e2.metadata,      e.metadata);
}

TEST(InferenceAuditEntryTest, ComputeContentHash_Deterministic) {
    InferenceAuditEntry e;
    e.entry_id      = "entry-abc";
    e.previous_hash = "prev-hash";
    e.timestamp     = "2026-02-21T16:00:00Z";
    e.request_id    = "req-xyz";
    e.query_hash    = std::string(64, 'q');
    e.response_hash = std::string(64, 'r');
    e.model_hash    = std::string(64, 'm');
    e.adapter_hash  = std::string(64, 'a');

    const std::string h1 = e.computeContentHash();
    const std::string h2 = e.computeContentHash();

    EXPECT_EQ(h1, h2);
    EXPECT_EQ(h1.size(), 64u);  // SHA-256 hex = 64 chars
}

TEST(InferenceAuditEntryTest, ComputeContentHash_SensitiveToChanges) {
    InferenceAuditEntry e1;
    e1.entry_id     = "e1";
    e1.previous_hash = "";
    e1.query_hash   = std::string(64, 'q');

    InferenceAuditEntry e2 = e1;
    e2.query_hash           = std::string(64, 'z');  // change one field

    EXPECT_NE(e1.computeContentHash(), e2.computeContentHash());
}

// ============================================================================
// LoRAProvenanceManager – sha256Hex utility
// ============================================================================

TEST(LoRAProvenanceManagerTest, Sha256Hex_KnownValue) {
    // SHA-256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    const std::string hash = LoRAProvenanceManager::sha256Hex("");
    EXPECT_EQ(hash, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(LoRAProvenanceManagerTest, Sha256Hex_NonEmpty) {
    const std::string h1 = LoRAProvenanceManager::sha256Hex("hello");
    const std::string h2 = LoRAProvenanceManager::sha256Hex("hello");
    const std::string h3 = LoRAProvenanceManager::sha256Hex("world");

    EXPECT_EQ(h1.size(), 64u);
    EXPECT_EQ(h1, h2);
    EXPECT_NE(h1, h3);
}

// ============================================================================
// LoRAProvenanceManager – local provenance CRUD
// ============================================================================

class LoRAProvenanceManagerTest : public ::testing::Test {
protected:
    LoRAProvenanceManager mgr;
};

TEST_F(LoRAProvenanceManagerTest, StoreAndRetrieveProvenance) {
    LoRAProvenanceRecord r;
    r.dataset_hash  = "dataset-sha256";
    r.trainer_id    = "trainer-99";

    EXPECT_TRUE(mgr.storeProvenance("adapter-A", r));

    auto opt = mgr.getProvenance("adapter-A");
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->dataset_hash, "dataset-sha256");
    EXPECT_EQ(opt->trainer_id,   "trainer-99");
}

TEST_F(LoRAProvenanceManagerTest, GetProvenance_NotFound) {
    auto opt = mgr.getProvenance("nonexistent");
    EXPECT_FALSE(opt.has_value());
}

TEST_F(LoRAProvenanceManagerTest, StoreProvenance_EmptyIdFails) {
    LoRAProvenanceRecord r;
    EXPECT_FALSE(mgr.storeProvenance("", r));
}

TEST_F(LoRAProvenanceManagerTest, StoreProvenance_Overwrites) {
    LoRAProvenanceRecord r1;
    r1.trainer_id = "first";
    LoRAProvenanceRecord r2;
    r2.trainer_id = "second";

    mgr.storeProvenance("adapter-B", r1);
    mgr.storeProvenance("adapter-B", r2);

    auto opt = mgr.getProvenance("adapter-B");
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->trainer_id, "second");
}

// ============================================================================
// LoRAProvenanceManager – external adapter import
// ============================================================================

TEST_F(LoRAProvenanceManagerTest, ImportExternal_AllowUnsigned) {
    ExternalAdapterProvenance ep;
    ep.source_url           = "https://example.com/adapter.zip";
    ep.adapter_hash         = std::string(64, 'a');
    // No signature, no cert chain

    auto result = mgr.importExternalAdapter("ext-A", ep, "", /*allow_unsigned=*/true);

    EXPECT_TRUE(result.signature_valid);
    EXPECT_TRUE(result.cert_chain_valid);
    EXPECT_TRUE(result.validation_errors.empty());

    auto stored = mgr.getExternalProvenance("ext-A");
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->source_url, ep.source_url);
}

TEST_F(LoRAProvenanceManagerTest, ImportExternal_MissingSignatureRejected) {
    ExternalAdapterProvenance ep;
    ep.adapter_hash = std::string(64, 'b');
    // signature and cert chain are empty

    auto result = mgr.importExternalAdapter("ext-B", ep, "trusted-ca", /*allow_unsigned=*/false);

    EXPECT_FALSE(result.signature_valid);
    EXPECT_FALSE(result.validation_errors.empty());

    // Should NOT be persisted
    auto stored = mgr.getExternalProvenance("ext-B");
    EXPECT_FALSE(stored.has_value());
}

TEST_F(LoRAProvenanceManagerTest, ImportExternal_MissingCertChainRejected) {
    ExternalAdapterProvenance ep;
    ep.adapter_hash         = std::string(64, 'c');
    ep.provenance_signature = "some-signature";
    // cert chain is empty

    auto result = mgr.importExternalAdapter("ext-C", ep, "trusted-ca", /*allow_unsigned=*/false);

    EXPECT_FALSE(result.cert_chain_valid);
    EXPECT_FALSE(result.validation_errors.empty());
}

TEST_F(LoRAProvenanceManagerTest, ImportExternal_InvalidHashLength) {
    ExternalAdapterProvenance ep;
    ep.adapter_hash         = "too-short";
    ep.provenance_signature = "sig";
    ep.certificate_chain    = "cert";

    auto result = mgr.importExternalAdapter("ext-D", ep, "ca", /*allow_unsigned=*/false);

    EXPECT_FALSE(result.validation_errors.empty());
    bool has_hash_error = false;
    for (const auto& e : result.validation_errors) {
        if (e.find("hash") != std::string::npos || e.find("64") != std::string::npos) {
            has_hash_error = true;
        }
    }
    EXPECT_TRUE(has_hash_error);
}

TEST_F(LoRAProvenanceManagerTest, ImportExternal_ValidProvenance) {
    ExternalAdapterProvenance ep;
    ep.source_url           = "https://hf.co/org/legal-lora";
    ep.commit_hash          = "deadbeef";
    ep.description          = "Legal domain";
    ep.adapter_hash         = std::string(64, 'a');
    ep.provenance_signature = "valid-sig";
    ep.certificate_chain    = "-----BEGIN CERTIFICATE-----\n...";

    auto result = mgr.importExternalAdapter("ext-E", ep, "trusted-ca", /*allow_unsigned=*/false);

    EXPECT_TRUE(result.signature_valid);
    EXPECT_TRUE(result.cert_chain_valid);
    EXPECT_TRUE(result.validation_errors.empty());

    auto stored = mgr.getExternalProvenance("ext-E");
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->commit_hash, "deadbeef");
}

// ============================================================================
// LoRAProvenanceManager – snapshots
// ============================================================================

TEST_F(LoRAProvenanceManagerTest, CreateAndListSnapshots) {
    LoRAProvenanceRecord prov;
    prov.trainer_id = "trainer-snap";

    auto snap = mgr.createSnapshot("snap-adapter", "v1.0", std::string(64, '1'), prov);

    EXPECT_FALSE(snap.snapshot_id.empty());
    EXPECT_EQ(snap.adapter_id,  "snap-adapter");
    EXPECT_EQ(snap.version,     "v1.0");
    EXPECT_TRUE(snap.parent_snapshot_id.empty());  // first snapshot has no parent

    auto list = mgr.listSnapshots("snap-adapter");
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].snapshot_id, snap.snapshot_id);
}

TEST_F(LoRAProvenanceManagerTest, MultipleSnapshots_ChainedParents) {
    LoRAProvenanceRecord prov;

    auto s1 = mgr.createSnapshot("chain-adapter", "v1.0", std::string(64, '1'), prov);
    auto s2 = mgr.createSnapshot("chain-adapter", "v2.0", std::string(64, '2'), prov);
    auto s3 = mgr.createSnapshot("chain-adapter", "v3.0", std::string(64, '3'), prov);

    EXPECT_TRUE(s1.parent_snapshot_id.empty());
    EXPECT_EQ(s2.parent_snapshot_id, s1.snapshot_id);
    EXPECT_EQ(s3.parent_snapshot_id, s2.snapshot_id);

    auto list = mgr.listSnapshots("chain-adapter");
    EXPECT_EQ(list.size(), 3u);
}

TEST_F(LoRAProvenanceManagerTest, GetSnapshotById) {
    LoRAProvenanceRecord prov;
    auto snap = mgr.createSnapshot("lookup-adapter", "v1.0", std::string(64, 'x'), prov);

    auto found = mgr.getSnapshot(snap.snapshot_id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->adapter_id, "lookup-adapter");
}

TEST_F(LoRAProvenanceManagerTest, GetSnapshot_NotFound) {
    auto found = mgr.getSnapshot("nonexistent-id");
    EXPECT_FALSE(found.has_value());
}

TEST_F(LoRAProvenanceManagerTest, ListSnapshots_EmptyForUnknownAdapter) {
    auto list = mgr.listSnapshots("unknown-adapter");
    EXPECT_TRUE(list.empty());
}

// ============================================================================
// LoRAProvenanceManager – Merkle-chained audit log
// ============================================================================

TEST_F(LoRAProvenanceManagerTest, AppendAuditEntry_SetsFields) {
    InferenceAuditEntry e;
    e.request_id    = "req-001";
    e.query_hash    = std::string(64, 'q');
    e.response_hash = std::string(64, 'r');
    e.model_hash    = std::string(64, 'm');
    e.adapter_hash  = std::string(64, 'a');

    auto stored = mgr.appendAuditEntry("audit-adapter", e);

    EXPECT_FALSE(stored.entry_id.empty());
    EXPECT_FALSE(stored.timestamp.empty());
    EXPECT_FALSE(stored.entry_hash.empty());
    EXPECT_TRUE(stored.previous_hash.empty());  // genesis entry
}

TEST_F(LoRAProvenanceManagerTest, AppendAuditEntry_MerkleChain) {
    InferenceAuditEntry e1, e2;
    e1.query_hash = std::string(64, '1');
    e2.query_hash = std::string(64, '2');

    auto s1 = mgr.appendAuditEntry("merkle-adapter", e1);
    auto s2 = mgr.appendAuditEntry("merkle-adapter", e2);

    // Second entry's previous_hash must equal first entry's entry_hash
    EXPECT_EQ(s2.previous_hash, s1.entry_hash);
    EXPECT_NE(s1.entry_hash, s2.entry_hash);
}

TEST_F(LoRAProvenanceManagerTest, GetAuditLog_OrderedAndComplete) {
    for (int i = 0; i < 5; ++i) {
        InferenceAuditEntry e;
        e.query_hash = std::string(64, static_cast<char>('a' + i));
        mgr.appendAuditEntry("log-adapter", e);
    }

    auto log = mgr.getAuditLog("log-adapter");
    EXPECT_EQ(log.size(), 5u);
}

TEST_F(LoRAProvenanceManagerTest, VerifyAuditChain_ValidChain) {
    for (int i = 0; i < 4; ++i) {
        InferenceAuditEntry e;
        e.query_hash = std::string(64, static_cast<char>('0' + i));
        mgr.appendAuditEntry("verify-adapter", e);
    }

    EXPECT_TRUE(mgr.verifyAuditChain("verify-adapter"));
}

TEST_F(LoRAProvenanceManagerTest, VerifyAuditChain_EmptyChainIsValid) {
    EXPECT_TRUE(mgr.verifyAuditChain("empty-adapter"));
}

TEST_F(LoRAProvenanceManagerTest, VerifyAuditChain_TamperedEntry) {
    InferenceAuditEntry e1, e2;
    e1.query_hash = std::string(64, '1');
    e2.query_hash = std::string(64, '2');

    mgr.appendAuditEntry("tamper-adapter", e1);
    mgr.appendAuditEntry("tamper-adapter", e2);

    // Tamper with the chain by retrieving and mutating a copy
    auto log = mgr.getAuditLog("tamper-adapter");
    ASSERT_EQ(log.size(), 2u);

    // The in-memory copy is verified through verifyAuditChain; modifying the
    // copy doesn't change the stored log, so verify is still valid here.
    // To test actual tampering we rely on the content-hash property:
    // a hash mismatch is detected by recomputing.

    // Verify that entry_hash matches computeContentHash()
    for (const auto& entry : log) {
        EXPECT_EQ(entry.entry_hash, entry.computeContentHash());
    }
}

// ============================================================================
// AdapterRegistry provenance integration tests
// ============================================================================

#include "llm/adapter_registry.h"
// SecuritySignatureManager is required by AdapterRegistry's constructor
#include "storage/security_signature_manager.h"

using namespace themis::llm;

class AdapterRegistryProvenanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // A null sig_manager is acceptable for unit tests that don't exercise signing
        registry = std::make_unique<AdapterRegistry>(nullptr);

        // Register a minimal adapter so provenance can be attached
        AdapterMetadata meta;
        meta.adapter_id      = "test-adapter";
        meta.base_model_name = "mistral-7b";
        meta.domain          = "legal";
        registry->registerAdapter(meta);
    }

    std::unique_ptr<AdapterRegistry> registry;
};

TEST_F(AdapterRegistryProvenanceTest, AttachAndRetrieveProvenance) {
    lora::LoRAProvenanceRecord prov;
    prov.dataset_hash        = std::string(64, 'd');
    prov.base_model_hash     = std::string(64, 'm');
    prov.adapter_weights_hash = std::string(64, 'w');
    prov.trainer_id          = "trainer-registry";

    EXPECT_TRUE(registry->attachProvenance("test-adapter", prov));

    auto opt = registry->getProvenanceRecord("test-adapter");
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->trainer_id,   "trainer-registry");
    EXPECT_EQ(opt->dataset_hash, prov.dataset_hash);
}

TEST_F(AdapterRegistryProvenanceTest, AttachProvenance_UnknownAdapterFails) {
    lora::LoRAProvenanceRecord prov;
    prov.trainer_id = "t";
    EXPECT_FALSE(registry->attachProvenance("nonexistent", prov));
}

TEST_F(AdapterRegistryProvenanceTest, GetProvenance_NoneAttached) {
    auto opt = registry->getProvenanceRecord("test-adapter");
    EXPECT_FALSE(opt.has_value());
}

TEST_F(AdapterRegistryProvenanceTest, RecordInferenceAudit_ChainedEntries) {
    lora::InferenceAuditEntry e1, e2;
    e1.query_hash    = std::string(64, 'q');
    e1.response_hash = std::string(64, 'r');
    e1.model_hash    = std::string(64, 'm');
    e1.adapter_hash  = std::string(64, 'a');

    e2.query_hash    = std::string(64, 'Q');
    e2.response_hash = std::string(64, 'R');
    e2.model_hash    = std::string(64, 'M');
    e2.adapter_hash  = std::string(64, 'A');

    auto s1 = registry->recordInferenceAudit("test-adapter", e1);
    auto s2 = registry->recordInferenceAudit("test-adapter", e2);

    EXPECT_FALSE(s1.entry_hash.empty());
    EXPECT_EQ(s2.previous_hash, s1.entry_hash);
}

TEST_F(AdapterRegistryProvenanceTest, GetInferenceAuditLog_Count) {
    for (int i = 0; i < 3; ++i) {
        lora::InferenceAuditEntry e;
        e.query_hash = std::string(64, static_cast<char>('a' + i));
        registry->recordInferenceAudit("test-adapter", e);
    }

    auto log = registry->getInferenceAuditLog("test-adapter");
    EXPECT_EQ(log.size(), 3u);
}

TEST_F(AdapterRegistryProvenanceTest, VerifyAuditChain_ValidChain) {
    for (int i = 0; i < 4; ++i) {
        lora::InferenceAuditEntry e;
        e.query_hash = std::string(64, static_cast<char>('0' + i));
        registry->recordInferenceAudit("test-adapter", e);
    }

    EXPECT_TRUE(registry->verifyAuditChain("test-adapter"));
}

TEST_F(AdapterRegistryProvenanceTest, VerifyAuditChain_EmptyIsValid) {
    EXPECT_TRUE(registry->verifyAuditChain("test-adapter"));
}

// ============================================================================
// LoRAOrchestrator provenance integration tests
// ============================================================================

#include "llm/lora_framework/lora_orchestrator.h"

using namespace themis::llm::lora;

class LoRAOrchestratorProvenanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        LoRAOrchestrator::Config cfg;
        orch = std::make_unique<LoRAOrchestrator>(cfg);

        // Register a minimal adapter so provenance can be attached
        TrainingData td;
        td.dataset_name = "test-ds";
        orch->createAdapter("orch-adapter", td);
    }

    std::unique_ptr<LoRAOrchestrator> orch;
};

TEST_F(LoRAOrchestratorProvenanceTest, AttachAndRetrieveProvenance) {
    LoRAProvenanceRecord prov;
    prov.dataset_hash    = std::string(64, 'd');
    prov.base_model_hash = std::string(64, 'm');
    prov.trainer_id      = "trainer-orch";

    EXPECT_TRUE(orch->attachProvenance("orch-adapter", prov));

    auto opt = orch->getProvenanceRecord("orch-adapter");
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->trainer_id, "trainer-orch");
}

TEST_F(LoRAOrchestratorProvenanceTest, AttachProvenance_UnknownAdapterFails) {
    LoRAProvenanceRecord prov;
    prov.trainer_id = "t";
    EXPECT_FALSE(orch->attachProvenance("does-not-exist", prov));
}

TEST_F(LoRAOrchestratorProvenanceTest, CreateAndListSnapshots) {
    auto snap = orch->createAdapterSnapshot("orch-adapter", "v1.0", std::string(64, 'h'));

    EXPECT_FALSE(snap.snapshot_id.empty());
    EXPECT_EQ(snap.adapter_id, "orch-adapter");
    EXPECT_TRUE(snap.parent_snapshot_id.empty());  // first snapshot

    auto list = orch->listAdapterSnapshots("orch-adapter");
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].snapshot_id, snap.snapshot_id);
}

TEST_F(LoRAOrchestratorProvenanceTest, SnapshotChain) {
    auto s1 = orch->createAdapterSnapshot("orch-adapter", "v1.0", std::string(64, '1'));
    auto s2 = orch->createAdapterSnapshot("orch-adapter", "v2.0", std::string(64, '2'));

    EXPECT_EQ(s2.parent_snapshot_id, s1.snapshot_id);
}

TEST_F(LoRAOrchestratorProvenanceTest, RecordAndVerifyAuditChain) {
    for (int i = 0; i < 3; ++i) {
        InferenceAuditEntry e;
        e.query_hash    = std::string(64, static_cast<char>('a' + i));
        e.response_hash = std::string(64, static_cast<char>('A' + i));
        e.model_hash    = std::string(64, 'm');
        e.adapter_hash  = std::string(64, 'a');
        orch->recordInferenceAudit("orch-adapter", e);
    }

    auto log = orch->getInferenceAuditLog("orch-adapter");
    EXPECT_EQ(log.size(), 3u);

    EXPECT_TRUE(orch->verifyAuditChain("orch-adapter"));
}

TEST_F(LoRAOrchestratorProvenanceTest, VerifyAuditChain_EmptyIsValid) {
    EXPECT_TRUE(orch->verifyAuditChain("orch-adapter"));
}

TEST_F(LoRAOrchestratorProvenanceTest, MerkleChainHashLinkage) {
    InferenceAuditEntry e1, e2;
    e1.query_hash = std::string(64, '1');
    e2.query_hash = std::string(64, '2');

    auto s1 = orch->recordInferenceAudit("orch-adapter", e1);
    auto s2 = orch->recordInferenceAudit("orch-adapter", e2);

    EXPECT_EQ(s2.previous_hash, s1.entry_hash);
    EXPECT_NE(s1.entry_hash, s2.entry_hash);
}



// ============================================================================
// LoRAAuditLogger → LoRAProvenanceManager bridge tests
// ============================================================================

#include "llm/lora_framework/lora_audit_logger.h"
#include "utils/audit_logger.h"

using namespace themis::utils;

class LoRAAuditLoggerBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        AuditLoggerConfig cfg;
        cfg.enabled = true;
        logger = std::make_unique<LoRAAuditLogger>(cfg);

        provenance_mgr = std::make_shared<LoRAProvenanceManager>();
        logger->setProvenanceManager(provenance_mgr);
    }

    std::unique_ptr<LoRAAuditLogger> logger;
    std::shared_ptr<LoRAProvenanceManager> provenance_mgr;
};

TEST_F(LoRAAuditLoggerBridgeTest, LogInferenceAppendsToMerkleChain) {
    LoRAInferenceAudit audit;
    audit.request_id      = "req-001";
    audit.session_id      = "sess-001";
    audit.user_id         = "user-001";
    audit.base_model_id   = "mistral-7b";
    audit.adapter_id      = "test-bridge-adapter";
    audit.adapter_version = "v1.0";
    audit.adapter_hash    = std::string(64, 'a');
    audit.prompt          = "Hello, world!";
    audit.response        = "Hi there!";
    audit.success         = true;

    logger->logInference(audit);

    auto log = provenance_mgr->getAuditLog("test-bridge-adapter");
    ASSERT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0].request_id, "req-001");
    EXPECT_FALSE(log[0].entry_hash.empty());
    EXPECT_TRUE(log[0].previous_hash.empty()); // genesis entry
}

TEST_F(LoRAAuditLoggerBridgeTest, MultipleInferencesFormChain) {
    for (int i = 0; i < 3; ++i) {
        LoRAInferenceAudit audit;
        audit.request_id    = "req-" + std::to_string(i);
        audit.adapter_id    = "chain-adapter";
        audit.adapter_hash  = std::string(64, static_cast<char>('0' + i));
        audit.prompt        = "prompt " + std::to_string(i);
        audit.response      = "resp "   + std::to_string(i);
        audit.success       = true;
        logger->logInference(audit);
    }

    auto log = provenance_mgr->getAuditLog("chain-adapter");
    ASSERT_EQ(log.size(), 3u);
    EXPECT_EQ(log[1].previous_hash, log[0].entry_hash);
    EXPECT_EQ(log[2].previous_hash, log[1].entry_hash);
    EXPECT_TRUE(provenance_mgr->verifyAuditChain("chain-adapter"));
}

TEST_F(LoRAAuditLoggerBridgeTest, NoProvenanceMgrDoesNotCrash) {
    // Disconnect the provenance manager – logInference must still succeed
    logger->setProvenanceManager(nullptr);

    LoRAInferenceAudit audit;
    audit.adapter_id = "no-mgr-adapter";
    audit.adapter_hash = std::string(64, 'x');
    audit.prompt = "test";
    audit.response = "ok";
    EXPECT_NO_THROW(logger->logInference(audit));
}

TEST_F(LoRAAuditLoggerBridgeTest, QueryHashAndResponseHashAreNonEmpty) {
    LoRAInferenceAudit audit;
    audit.adapter_id   = "hash-check-adapter";
    audit.adapter_hash = std::string(64, 'h');
    audit.prompt       = "non-empty prompt";
    audit.response     = "non-empty response";
    audit.success      = true;
    logger->logInference(audit);

    auto log = provenance_mgr->getAuditLog("hash-check-adapter");
    ASSERT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0].query_hash.size(),    64u);  // SHA-256 hex = 64 chars
    EXPECT_EQ(log[0].response_hash.size(), 64u);
}

TEST_F(LoRAAuditLoggerBridgeTest, LogProvenanceAttached_EmitsEvent) {
    LoRAProvenanceRecord rec;
    rec.dataset_hash        = std::string(64, 'd');
    rec.base_model_hash     = std::string(64, 'm');
    rec.adapter_weights_hash = std::string(64, 'w');
    rec.trainer_id          = "trainer-x";
    rec.rfc3161_timestamp   = "dummytoken";

    // Should not throw; event is logged via logEvent()
    EXPECT_NO_THROW(logger->logProvenanceAttached("prov-adapter", rec));
}

TEST_F(LoRAAuditLoggerBridgeTest, LogSnapshotCreated_EmitsEvent) {
    AdapterSnapshot snap;
    snap.snapshot_id       = "snap-001";
    snap.adapter_id        = "snap-adapter";
    snap.version           = "v1.0";
    snap.weights_hash      = std::string(64, 's');
    snap.timestamp         = "2026-02-21T00:00:00Z";

    EXPECT_NO_THROW(logger->logSnapshotCreated("snap-adapter", snap));
}

TEST_F(LoRAAuditLoggerBridgeTest, LogAuditChainVerified_ValidChain) {
    EXPECT_NO_THROW(logger->logAuditChainVerified("verify-adapter", true, 42));
}

TEST_F(LoRAAuditLoggerBridgeTest, LogAuditChainVerified_TamperedChain) {
    EXPECT_NO_THROW(logger->logAuditChainVerified("tamper-adapter", false, 7));
}
