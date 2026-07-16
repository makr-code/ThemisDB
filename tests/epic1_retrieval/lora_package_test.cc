/**
 * @file lora_package_test.cc
 * @brief GTest coverage for LoRAPackage, PortableAdapterProduct, and
 *        LoRAManifestStore (Phase 3 manifest & serialization).
 *
 * Test coverage areas:
 *   - LoRAPackage construction, serialization, and deserialization
 *   - PortableAdapterProduct construction, serialization, and deserialization
 *   - LoRAManifestStore CRUD operations for both artifact types
 *   - Integrity: manifest-hash computation and hash-mismatch detection
 *   - Integrity: signature-verifier callback integration (sign + verify)
 *   - Status lifecycle transitions (enum → string → enum round-trip)
 *   - Policy and provenance field round-trip
 *   - Edge cases: empty IDs, missing required fields, unknown status strings
 *   - Bulk export/import round-trip
 *   - listPackagesByStatus / listProductsByStatus / listProductsByPackage
 */

#include <gtest/gtest.h>
#include "lora_package.h"

#include <string>
#include <vector>
#include <stdexcept>

using namespace themis::retrieval;

// ============================================================================
// Test helpers
// ============================================================================

namespace {

LoRAPackage makeValidPackage(const std::string& id = "pkg-001") {
    LoRAPackage p;
    p.package_id             = id;
    p.name                   = "legal-qa-lora";
    p.version                = "1.0.0";
    p.description            = "LoRA adapter for legal Q&A";
    p.supported_architectures = {"llama", "mistral"};
    p.lora_rank              = 8;
    p.lora_alpha             = 16.0f;
    p.target_modules         = {"q_proj", "v_proj"};
    p.parent_package_id      = "";
    p.weights_path           = "/adapters/legal-qa/weights.safetensors";
    p.status                 = LoRAPackageStatus::DRAFT;
    p.created_at             = "2026-07-01T00:00:00Z";
    p.updated_at             = "2026-07-01T00:00:00Z";
    p.provenance.trainer_id  = "trainer-42";
    p.provenance.training_framework = "PEFT-0.14";
    p.provenance.dataset_id  = "ds-legal-2026";
    p.provenance.dataset_hash = "aabbccdd";
    p.provenance.base_model_id = "llama-3-8b";
    p.provenance.base_model_hash = "deadbeef";
    p.provenance.training_duration_secs = 3600.0;
    p.policy.license         = "MIT";
    p.policy.restrictions    = "Research use only";
    p.policy.allowed_base_models = {"llama-3-8b", "mistral-7b"};
    return p;
}

PortableAdapterProduct makeValidProduct(const std::string& id = "prod-001",
                                         const std::string& pkg_id = "pkg-001") {
    PortableAdapterProduct pr;
    pr.product_id              = id;
    pr.name                    = "legal-qa-lora-llama-q4";
    pr.version                 = "1.0.0";
    pr.source_package_id       = pkg_id;
    pr.target_base_model_id    = "llama-3-8b";
    pr.target_model_architecture = "llama";
    pr.quantization            = "Q4_K_M";
    pr.format                  = "GGUF-ST";
    pr.file_path               = "/products/legal-qa-llama-q4.gguf";
    pr.file_size_bytes         = 4096000u;
    pr.max_context_length      = 4096;
    pr.memory_requirement_mb   = 2048u;
    pr.compatible_model_versions = {"3.8b-v1", "3.8b-v2"};
    pr.status                  = AdapterProductStatus::READY;
    pr.created_at              = "2026-07-01T01:00:00Z";
    pr.updated_at              = "2026-07-01T01:00:00Z";
    return pr;
}

} // anonymous namespace

// ============================================================================
// AdapterUsagePolicy serialization
// ============================================================================

TEST(AdapterUsagePolicyTest, RoundTrip_AllFields) {
    AdapterUsagePolicy p;
    p.license                    = "CC-BY-4.0";
    p.restrictions               = "No commercial use";
    p.allowed_base_models        = {"llama-3-8b", "mistral-7b"};
    p.max_concurrent_deployments = 5;
    p.expiry_date                = "2027-12-31";

    const auto j  = p.to_json();
    const auto p2 = AdapterUsagePolicy::from_json(j);

    EXPECT_EQ(p2.license,                    p.license);
    EXPECT_EQ(p2.restrictions,               p.restrictions);
    EXPECT_EQ(p2.allowed_base_models,        p.allowed_base_models);
    EXPECT_EQ(p2.max_concurrent_deployments, p.max_concurrent_deployments);
    EXPECT_EQ(p2.expiry_date,                p.expiry_date);
}

TEST(AdapterUsagePolicyTest, RoundTrip_Empty) {
    AdapterUsagePolicy p;
    const auto j  = p.to_json();
    const auto p2 = AdapterUsagePolicy::from_json(j);
    EXPECT_TRUE(p2.license.empty());
    EXPECT_TRUE(p2.allowed_base_models.empty());
    EXPECT_EQ(p2.max_concurrent_deployments, 0);
}

// ============================================================================
// LoRAPackageProvenance serialization
// ============================================================================

TEST(LoRAPackageProvenanceTest, RoundTrip) {
    LoRAPackageProvenance prov;
    prov.trainer_id           = "trainer-42";
    prov.training_framework   = "PEFT-0.14";
    prov.dataset_id           = "ds-2026";
    prov.dataset_hash         = "aaaa1111";
    prov.base_model_id        = "llama-3-8b";
    prov.base_model_hash      = "bbbb2222";
    prov.hyperparameter_hash  = "cccc3333";
    prov.training_duration_secs = 7200.5;
    prov.created_at           = "2026-07-01T00:00:00Z";

    const auto j     = prov.to_json();
    const auto prov2 = LoRAPackageProvenance::from_json(j);

    EXPECT_EQ(prov2.trainer_id,           prov.trainer_id);
    EXPECT_EQ(prov2.training_framework,   prov.training_framework);
    EXPECT_EQ(prov2.dataset_hash,         prov.dataset_hash);
    EXPECT_EQ(prov2.base_model_hash,      prov.base_model_hash);
    EXPECT_DOUBLE_EQ(prov2.training_duration_secs, prov.training_duration_secs);
}

// ============================================================================
// ArtifactIntegrity serialization
// ============================================================================

TEST(ArtifactIntegrityTest, RoundTrip) {
    ArtifactIntegrity i;
    i.weights_hash        = "abc123";
    i.manifest_hash       = "def456";
    i.signature           = "base64sig==";
    i.signature_algorithm = "Ed25519";
    i.signer_id           = "key-A";
    i.signed_at           = "2026-07-01T00:00:00Z";
    i.signature_verified  = true; // Must NOT be serialised

    const auto j  = i.to_json();
    const auto i2 = ArtifactIntegrity::from_json(j);

    EXPECT_EQ(i2.weights_hash,        i.weights_hash);
    EXPECT_EQ(i2.manifest_hash,       i.manifest_hash);
    EXPECT_EQ(i2.signature,           i.signature);
    EXPECT_EQ(i2.signature_algorithm, i.signature_algorithm);
    EXPECT_EQ(i2.signer_id,           i.signer_id);
    EXPECT_EQ(i2.signed_at,           i.signed_at);
    // signature_verified must default to false after deserialization
    EXPECT_FALSE(i2.signature_verified);
}

// ============================================================================
// LoRAPackage serialization
// ============================================================================

TEST(LoRAPackageTest, ToJson_ContainsRequiredFields) {
    const auto pkg = makeValidPackage();
    const auto j   = pkg.to_json();

    EXPECT_EQ(j["package_id"], "pkg-001");
    EXPECT_EQ(j["name"],       "legal-qa-lora");
    EXPECT_EQ(j["version"],    "1.0.0");
    EXPECT_EQ(j["lora_rank"],  8);
    EXPECT_FLOAT_EQ(j["lora_alpha"].get<float>(), 16.0f);
    EXPECT_EQ(j["status"], "DRAFT");
}

TEST(LoRAPackageTest, RoundTrip_AllFields) {
    const auto pkg  = makeValidPackage("pkg-round");
    const auto j    = pkg.to_json();
    const auto pkg2 = LoRAPackage::from_json(j);

    EXPECT_EQ(pkg2.package_id,  pkg.package_id);
    EXPECT_EQ(pkg2.name,        pkg.name);
    EXPECT_EQ(pkg2.version,     pkg.version);
    EXPECT_EQ(pkg2.description, pkg.description);
    EXPECT_EQ(pkg2.lora_rank,   pkg.lora_rank);
    EXPECT_FLOAT_EQ(pkg2.lora_alpha, pkg.lora_alpha);
    EXPECT_EQ(pkg2.target_modules,  pkg.target_modules);
    EXPECT_EQ(pkg2.weights_path,    pkg.weights_path);
    EXPECT_EQ(pkg2.status,          pkg.status);
    EXPECT_EQ(pkg2.created_at,      pkg.created_at);
    EXPECT_EQ(pkg2.provenance.trainer_id, pkg.provenance.trainer_id);
    EXPECT_EQ(pkg2.policy.license,        pkg.policy.license);
}

TEST(LoRAPackageTest, FromJson_ThrowsOnMissingPackageId) {
    json j;
    j["name"]    = "x";
    j["version"] = "1.0.0";
    EXPECT_THROW(LoRAPackage::from_json(j), std::invalid_argument);
}

TEST(LoRAPackageTest, FromJson_ThrowsOnMissingName) {
    json j;
    j["package_id"] = "x";
    j["version"]    = "1.0.0";
    EXPECT_THROW(LoRAPackage::from_json(j), std::invalid_argument);
}

TEST(LoRAPackageTest, FromJson_ThrowsOnMissingVersion) {
    json j;
    j["package_id"] = "x";
    j["name"]       = "y";
    EXPECT_THROW(LoRAPackage::from_json(j), std::invalid_argument);
}

// ============================================================================
// LoRAPackage status helpers
// ============================================================================

TEST(LoRAPackageTest, StatusRoundTrip_AllValues) {
    const std::vector<LoRAPackageStatus> statuses = {
        LoRAPackageStatus::DRAFT,
        LoRAPackageStatus::VALIDATED,
        LoRAPackageStatus::DEPRECATED,
        LoRAPackageStatus::REVOKED
    };
    for (auto s : statuses) {
        LoRAPackage pkg;
        pkg.package_id = "x";
        pkg.name       = "y";
        pkg.version    = "1.0.0";
        pkg.status     = s;
        const std::string str = pkg.statusToString();
        EXPECT_EQ(LoRAPackage::statusFromString(str), s);
    }
}

TEST(LoRAPackageTest, StatusFromString_ThrowsOnUnknown) {
    EXPECT_THROW(LoRAPackage::statusFromString("BOGUS"), std::invalid_argument);
}

// ============================================================================
// LoRAPackage architecture support
// ============================================================================

TEST(LoRAPackageTest, SupportsArchitecture_EmptyList_AcceptsAll) {
    auto pkg = makeValidPackage();
    pkg.supported_architectures.clear();
    EXPECT_TRUE(pkg.supportsArchitecture("llama"));
    EXPECT_TRUE(pkg.supportsArchitecture("falcon"));
}

TEST(LoRAPackageTest, SupportsArchitecture_ExplicitList) {
    const auto pkg = makeValidPackage(); // {"llama", "mistral"}
    EXPECT_TRUE(pkg.supportsArchitecture("llama"));
    EXPECT_TRUE(pkg.supportsArchitecture("LLAMA")); // case-insensitive
    EXPECT_TRUE(pkg.supportsArchitecture("mistral"));
    EXPECT_FALSE(pkg.supportsArchitecture("falcon"));
}

// ============================================================================
// LoRAPackage manifest hash
// ============================================================================

TEST(LoRAPackageTest, ComputeManifestHash_ProducesNonEmptyHash) {
    auto pkg = makeValidPackage();
    pkg.computeManifestHash();
    EXPECT_EQ(pkg.integrity.manifest_hash.size(), 64u);
}

TEST(LoRAPackageTest, ComputeManifestHash_IsDeterministic) {
    auto pkg1 = makeValidPackage();
    auto pkg2 = makeValidPackage();
    pkg1.computeManifestHash();
    pkg2.computeManifestHash();
    EXPECT_EQ(pkg1.integrity.manifest_hash, pkg2.integrity.manifest_hash);
}

TEST(LoRAPackageTest, ComputeManifestHash_ChangesWithFieldChange) {
    auto pkg1 = makeValidPackage();
    auto pkg2 = makeValidPackage();
    pkg2.description = "Different description";
    pkg1.computeManifestHash();
    pkg2.computeManifestHash();
    EXPECT_NE(pkg1.integrity.manifest_hash, pkg2.integrity.manifest_hash);
}

// ============================================================================
// PortableAdapterProduct serialization
// ============================================================================

TEST(PortableAdapterProductTest, ToJson_ContainsRequiredFields) {
    const auto prod = makeValidProduct();
    const auto j    = prod.to_json();

    EXPECT_EQ(j["product_id"],             "prod-001");
    EXPECT_EQ(j["source_package_id"],      "pkg-001");
    EXPECT_EQ(j["target_base_model_id"],   "llama-3-8b");
    EXPECT_EQ(j["quantization"],           "Q4_K_M");
    EXPECT_EQ(j["format"],                 "GGUF-ST");
    EXPECT_EQ(j["status"],                 "READY");
}

TEST(PortableAdapterProductTest, RoundTrip_AllFields) {
    const auto prod  = makeValidProduct("prod-rt", "pkg-rt");
    const auto j     = prod.to_json();
    const auto prod2 = PortableAdapterProduct::from_json(j);

    EXPECT_EQ(prod2.product_id,              prod.product_id);
    EXPECT_EQ(prod2.source_package_id,       prod.source_package_id);
    EXPECT_EQ(prod2.target_base_model_id,    prod.target_base_model_id);
    EXPECT_EQ(prod2.target_model_architecture, prod.target_model_architecture);
    EXPECT_EQ(prod2.quantization,            prod.quantization);
    EXPECT_EQ(prod2.format,                  prod.format);
    EXPECT_EQ(prod2.file_path,               prod.file_path);
    EXPECT_EQ(prod2.file_size_bytes,         prod.file_size_bytes);
    EXPECT_EQ(prod2.max_context_length,      prod.max_context_length);
    EXPECT_EQ(prod2.memory_requirement_mb,   prod.memory_requirement_mb);
    EXPECT_EQ(prod2.compatible_model_versions, prod.compatible_model_versions);
    EXPECT_EQ(prod2.status,                  prod.status);
}

TEST(PortableAdapterProductTest, FromJson_ThrowsOnMissingProductId) {
    json j;
    j["source_package_id"]    = "pkg-x";
    j["target_base_model_id"] = "llama-3-8b";
    EXPECT_THROW(PortableAdapterProduct::from_json(j), std::invalid_argument);
}

TEST(PortableAdapterProductTest, FromJson_ThrowsOnMissingSourcePackageId) {
    json j;
    j["product_id"]           = "prod-x";
    j["target_base_model_id"] = "llama-3-8b";
    EXPECT_THROW(PortableAdapterProduct::from_json(j), std::invalid_argument);
}

TEST(PortableAdapterProductTest, FromJson_ThrowsOnMissingTargetBaseModelId) {
    json j;
    j["product_id"]        = "prod-x";
    j["source_package_id"] = "pkg-x";
    EXPECT_THROW(PortableAdapterProduct::from_json(j), std::invalid_argument);
}

// ============================================================================
// PortableAdapterProduct status helpers
// ============================================================================

TEST(PortableAdapterProductTest, StatusRoundTrip_AllValues) {
    const std::vector<AdapterProductStatus> statuses = {
        AdapterProductStatus::BUILDING,
        AdapterProductStatus::READY,
        AdapterProductStatus::DEPLOYED,
        AdapterProductStatus::RETIRED,
        AdapterProductStatus::FAILED
    };
    for (auto s : statuses) {
        PortableAdapterProduct prod;
        prod.product_id         = "p";
        prod.source_package_id  = "q";
        prod.target_base_model_id = "m";
        prod.status             = s;
        const std::string str   = prod.statusToString();
        EXPECT_EQ(PortableAdapterProduct::statusFromString(str), s);
    }
}

TEST(PortableAdapterProductTest, StatusFromString_ThrowsOnUnknown) {
    EXPECT_THROW(PortableAdapterProduct::statusFromString("INVALID"), std::invalid_argument);
}

// ============================================================================
// PortableAdapterProduct manifest hash
// ============================================================================

TEST(PortableAdapterProductTest, ComputeManifestHash_ProducesNonEmptyHash) {
    auto prod = makeValidProduct();
    prod.computeManifestHash();
    EXPECT_EQ(prod.integrity.manifest_hash.size(), 64u);
}

TEST(PortableAdapterProductTest, ComputeManifestHash_IsDeterministic) {
    auto p1 = makeValidProduct();
    auto p2 = makeValidProduct();
    p1.computeManifestHash();
    p2.computeManifestHash();
    EXPECT_EQ(p1.integrity.manifest_hash, p2.integrity.manifest_hash);
}

// ============================================================================
// IntegrityHelper
// ============================================================================

TEST(IntegrityHelperTest, Sha256Hex_KnownVector) {
    // SHA-256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    const std::string empty_hash = IntegrityHelper::sha256Hex("");
    EXPECT_EQ(empty_hash,
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(IntegrityHelperTest, Sha256Hex_HelloWorld) {
    // SHA-256("hello world") = b94d27b9934d3e08a52e52d7da7dabfac484efe04294e576f18d2e655571d6d8
    // Note: canonical value is:
    // b94d27b9934d3e08a52e52d7da7dabfac484efe04294e576f18d2e655571d6d8
    // (32 bytes → 64 hex chars)
    const std::string h = IntegrityHelper::sha256Hex("hello world");
    EXPECT_EQ(h.size(), 64u);
    // Spot-check first two chars
    EXPECT_EQ(h.substr(0, 2), "b9");
}

TEST(IntegrityHelperTest, VerifyHash_CorrectHash_ReturnsTrue) {
    const std::string input = "test-data";
    const std::string hash  = IntegrityHelper::sha256Hex(input);
    EXPECT_TRUE(IntegrityHelper::verifyHash(input, hash));
}

TEST(IntegrityHelperTest, VerifyHash_WrongHash_ReturnsFalse) {
    EXPECT_FALSE(IntegrityHelper::verifyHash("data", "badhash"));
}

TEST(IntegrityHelperTest, Sha256Hex_Bytes_MatchesStringOverload) {
    const std::string s = "abc123";
    const auto h1 = IntegrityHelper::sha256Hex(s);
    const auto h2 = IntegrityHelper::sha256Hex(
        reinterpret_cast<const uint8_t*>(s.data()), s.size());
    EXPECT_EQ(h1, h2);
}

// ============================================================================
// LoRAManifestStore — package CRUD
// ============================================================================

TEST(LoRAManifestStoreTest, StoreAndLoad_Package) {
    LoRAManifestStore store;
    const auto pkg = makeValidPackage("pkg-crud-1");
    ASSERT_TRUE(store.storePackage(pkg));
    const auto loaded = store.loadPackage("pkg-crud-1");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->package_id, "pkg-crud-1");
}

TEST(LoRAManifestStoreTest, LoadPackage_NotFound_ReturnsNullopt) {
    LoRAManifestStore store;
    EXPECT_FALSE(store.loadPackage("nonexistent").has_value());
}

TEST(LoRAManifestStoreTest, DeletePackage_RemovesEntry) {
    LoRAManifestStore store;
    const auto pkg = makeValidPackage("pkg-del");
    store.storePackage(pkg);
    ASSERT_TRUE(store.deletePackage("pkg-del"));
    EXPECT_FALSE(store.loadPackage("pkg-del").has_value());
}

TEST(LoRAManifestStoreTest, DeletePackage_NotFound_ReturnsFalse) {
    LoRAManifestStore store;
    EXPECT_FALSE(store.deletePackage("ghost"));
}

TEST(LoRAManifestStoreTest, StorePackage_EmptyId_ReturnsFalse) {
    LoRAManifestStore store;
    LoRAPackage pkg;
    // package_id is empty
    EXPECT_FALSE(store.storePackage(pkg));
    EXPECT_EQ(store.packageCount(), 0u);
}

TEST(LoRAManifestStoreTest, ListPackageIds) {
    LoRAManifestStore store;
    store.storePackage(makeValidPackage("p1"));
    store.storePackage(makeValidPackage("p2"));
    const auto ids = store.listPackageIds();
    EXPECT_EQ(ids.size(), 2u);
}

TEST(LoRAManifestStoreTest, ListPackagesByStatus) {
    LoRAManifestStore store;
    auto p1 = makeValidPackage("ps1");
    p1.status = LoRAPackageStatus::DRAFT;
    auto p2 = makeValidPackage("ps2");
    p2.status = LoRAPackageStatus::VALIDATED;
    store.storePackage(p1);
    store.storePackage(p2);

    const auto drafts     = store.listPackagesByStatus(LoRAPackageStatus::DRAFT);
    const auto validated  = store.listPackagesByStatus(LoRAPackageStatus::VALIDATED);
    const auto deprecated = store.listPackagesByStatus(LoRAPackageStatus::DEPRECATED);

    EXPECT_EQ(drafts.size(),    1u);
    EXPECT_EQ(validated.size(), 1u);
    EXPECT_EQ(deprecated.size(), 0u);
}

// ============================================================================
// LoRAManifestStore — product CRUD
// ============================================================================

TEST(LoRAManifestStoreTest, StoreAndLoad_Product) {
    LoRAManifestStore store;
    const auto prod = makeValidProduct("prod-c1");
    ASSERT_TRUE(store.storeProduct(prod));
    const auto loaded = store.loadProduct("prod-c1");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->product_id, "prod-c1");
}

TEST(LoRAManifestStoreTest, DeleteProduct_RemovesEntry) {
    LoRAManifestStore store;
    store.storeProduct(makeValidProduct("prod-del"));
    ASSERT_TRUE(store.deleteProduct("prod-del"));
    EXPECT_FALSE(store.loadProduct("prod-del").has_value());
}

TEST(LoRAManifestStoreTest, StoreProduct_EmptyId_ReturnsFalse) {
    LoRAManifestStore store;
    PortableAdapterProduct prod;
    // product_id and source_package_id are both empty
    EXPECT_FALSE(store.storeProduct(prod));
}

TEST(LoRAManifestStoreTest, ListProductsByPackage) {
    LoRAManifestStore store;
    store.storeProduct(makeValidProduct("pr1", "pkg-A"));
    store.storeProduct(makeValidProduct("pr2", "pkg-A"));
    store.storeProduct(makeValidProduct("pr3", "pkg-B"));

    const auto forA = store.listProductsByPackage("pkg-A");
    const auto forB = store.listProductsByPackage("pkg-B");
    const auto forC = store.listProductsByPackage("pkg-C");

    EXPECT_EQ(forA.size(), 2u);
    EXPECT_EQ(forB.size(), 1u);
    EXPECT_EQ(forC.size(), 0u);
}

TEST(LoRAManifestStoreTest, ListProductsByStatus) {
    LoRAManifestStore store;
    auto p1 = makeValidProduct("ps1");
    p1.status = AdapterProductStatus::READY;
    auto p2 = makeValidProduct("ps2");
    p2.status = AdapterProductStatus::DEPLOYED;
    store.storeProduct(p1);
    store.storeProduct(p2);

    EXPECT_EQ(store.listProductsByStatus(AdapterProductStatus::READY).size(),    1u);
    EXPECT_EQ(store.listProductsByStatus(AdapterProductStatus::DEPLOYED).size(), 1u);
    EXPECT_EQ(store.listProductsByStatus(AdapterProductStatus::FAILED).size(),   0u);
}

// ============================================================================
// LoRAManifestStore — integrity verification
// ============================================================================

TEST(LoRAManifestStoreTest, VerifyPackageIntegrity_ValidHash_ReturnsTrue) {
    LoRAManifestStore store;
    auto pkg = makeValidPackage("pkg-vi");
    pkg.computeManifestHash();
    store.storePackage(pkg);
    EXPECT_TRUE(store.verifyPackageIntegrity("pkg-vi"));
}

TEST(LoRAManifestStoreTest, VerifyPackageIntegrity_TamperedField_ReturnsFalse) {
    LoRAManifestStore store;
    auto pkg = makeValidPackage("pkg-tamper");
    pkg.computeManifestHash();
    store.storePackage(pkg);

    // Retrieve, tamper, and re-store without recomputing hash
    auto loaded = store.loadPackage("pkg-tamper").value();
    loaded.description = "TAMPERED";
    store.storePackage(loaded);

    EXPECT_FALSE(store.verifyPackageIntegrity("pkg-tamper"));
}

TEST(LoRAManifestStoreTest, VerifyPackageIntegrity_EmptyHash_ReturnsFalse) {
    LoRAManifestStore store;
    auto pkg = makeValidPackage("pkg-nohash");
    // Do NOT call computeManifestHash()
    store.storePackage(pkg);
    EXPECT_FALSE(store.verifyPackageIntegrity("pkg-nohash"));
}

TEST(LoRAManifestStoreTest, VerifyPackageIntegrity_NotFound_ReturnsFalse) {
    LoRAManifestStore store;
    EXPECT_FALSE(store.verifyPackageIntegrity("nonexistent"));
}

TEST(LoRAManifestStoreTest, VerifyProductIntegrity_ValidHash_ReturnsTrue) {
    LoRAManifestStore store;
    auto prod = makeValidProduct("prod-vi");
    prod.computeManifestHash();
    store.storeProduct(prod);
    EXPECT_TRUE(store.verifyProductIntegrity("prod-vi"));
}

TEST(LoRAManifestStoreTest, VerifyProductIntegrity_TamperedField_ReturnsFalse) {
    LoRAManifestStore store;
    auto prod = makeValidProduct("prod-tamper");
    prod.computeManifestHash();
    store.storeProduct(prod);

    auto loaded = store.loadProduct("prod-tamper").value();
    loaded.quantization = "INT4"; // tamper
    store.storeProduct(loaded);

    EXPECT_FALSE(store.verifyProductIntegrity("prod-tamper"));
}

// ============================================================================
// LoRAManifestStore — signature verifier integration
// ============================================================================

TEST(LoRAManifestStoreTest, SignatureVerifier_CalledOnVerifyPackageIntegrity) {
    LoRAManifestStore store;
    bool verifier_called = false;
    store.setSignatureVerifier(
        [&](const std::string&, const std::string&, const std::string&) -> bool {
            verifier_called = true;
            return true;
        });

    auto pkg = makeValidPackage("pkg-sig");
    pkg.computeManifestHash();
    pkg.integrity.signature           = "fake-sig";
    pkg.integrity.signature_algorithm = "Ed25519";
    pkg.integrity.signer_id           = "key-1";
    store.storePackage(pkg);

    EXPECT_TRUE(store.verifyPackageIntegrity("pkg-sig"));
    EXPECT_TRUE(verifier_called);
}

TEST(LoRAManifestStoreTest, SignatureVerifier_ReturnsFalse_FailsVerification) {
    LoRAManifestStore store;
    store.setSignatureVerifier(
        [](const std::string&, const std::string&, const std::string&) -> bool {
            return false; // Simulate bad signature
        });

    auto pkg = makeValidPackage("pkg-badsig");
    pkg.computeManifestHash();
    pkg.integrity.signature = "bad-sig";
    store.storePackage(pkg);

    EXPECT_FALSE(store.verifyPackageIntegrity("pkg-badsig"));
}

// ============================================================================
// LoRAManifestStore — bulk export/import
// ============================================================================

TEST(LoRAManifestStoreTest, ExportImportPackages_RoundTrip) {
    LoRAManifestStore store;
    store.storePackage(makeValidPackage("exp1"));
    store.storePackage(makeValidPackage("exp2"));

    const auto exported = store.exportPackages();
    ASSERT_EQ(exported.size(), 2u);

    LoRAManifestStore store2;
    const auto imported = store2.importPackages(exported);
    EXPECT_EQ(imported, 2u);
    EXPECT_EQ(store2.packageCount(), 2u);
    EXPECT_TRUE(store2.loadPackage("exp1").has_value());
    EXPECT_TRUE(store2.loadPackage("exp2").has_value());
}

TEST(LoRAManifestStoreTest, ExportImportProducts_RoundTrip) {
    LoRAManifestStore store;
    store.storeProduct(makeValidProduct("ep1", "pkg-x"));
    store.storeProduct(makeValidProduct("ep2", "pkg-x"));

    const auto exported = store.exportProducts();
    ASSERT_EQ(exported.size(), 2u);

    LoRAManifestStore store2;
    const auto imported = store2.importProducts(exported);
    EXPECT_EQ(imported, 2u);
    EXPECT_EQ(store2.productCount(), 2u);
}

TEST(LoRAManifestStoreTest, ImportPackages_MalformedEntry_Skipped) {
    LoRAManifestStore store;
    json arr = json::array();
    arr.push_back({{"name", "missing-id-field"}, {"version", "1.0.0"}});  // malformed
    arr.push_back(makeValidPackage("good-pkg").to_json());                   // valid

    const auto imported = store.importPackages(arr);
    EXPECT_EQ(imported, 1u);
    EXPECT_EQ(store.packageCount(), 1u);
}

// ============================================================================
// LoRAManifestStore — statistics
// ============================================================================

TEST(LoRAManifestStoreTest, PackageAndProductCount) {
    LoRAManifestStore store;
    EXPECT_EQ(store.packageCount(), 0u);
    EXPECT_EQ(store.productCount(), 0u);

    store.storePackage(makeValidPackage("c1"));
    store.storePackage(makeValidPackage("c2"));
    store.storeProduct(makeValidProduct("cp1", "c1"));

    EXPECT_EQ(store.packageCount(), 2u);
    EXPECT_EQ(store.productCount(), 1u);
}

// ============================================================================
// Lifecycle integration: package → product pipeline
// ============================================================================

TEST(LoRALifecycleIntegrationTest, FullPipelinePackageToDeployedProduct) {
    LoRAManifestStore store;

    // Step 1: Create DRAFT package
    auto pkg = makeValidPackage("lifecycle-pkg");
    pkg.status = LoRAPackageStatus::DRAFT;
    pkg.computeManifestHash();
    store.storePackage(pkg);
    EXPECT_TRUE(store.verifyPackageIntegrity("lifecycle-pkg"));

    // Step 2: Validate package
    {
        auto p = store.loadPackage("lifecycle-pkg").value();
        p.status = LoRAPackageStatus::VALIDATED;
        p.computeManifestHash();
        store.storePackage(p);
    }
    EXPECT_EQ(store.loadPackage("lifecycle-pkg")->status, LoRAPackageStatus::VALIDATED);

    // Step 3: Build product from validated package
    auto prod = makeValidProduct("lifecycle-prod", "lifecycle-pkg");
    prod.status = AdapterProductStatus::BUILDING;
    store.storeProduct(prod);

    // Step 4: Mark product READY with hash
    {
        auto p = store.loadProduct("lifecycle-prod").value();
        p.status = AdapterProductStatus::READY;
        p.computeManifestHash();
        store.storeProduct(p);
    }
    EXPECT_TRUE(store.verifyProductIntegrity("lifecycle-prod"));
    EXPECT_EQ(store.loadProduct("lifecycle-prod")->status, AdapterProductStatus::READY);

    // Step 5: Deploy
    {
        auto p = store.loadProduct("lifecycle-prod").value();
        p.status      = AdapterProductStatus::DEPLOYED;
        p.deployed_at = "2026-07-01T12:00:00Z";
        p.computeManifestHash();
        store.storeProduct(p);
    }
    EXPECT_EQ(store.listProductsByStatus(AdapterProductStatus::DEPLOYED).size(), 1u);

    // Step 6: Deprecate source package (products still valid)
    {
        auto p = store.loadPackage("lifecycle-pkg").value();
        p.status = LoRAPackageStatus::DEPRECATED;
        p.computeManifestHash();
        store.storePackage(p);
    }
    EXPECT_EQ(store.listPackagesByStatus(LoRAPackageStatus::DEPRECATED).size(), 1u);
}
