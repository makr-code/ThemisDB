/**
 * @file test_retrieval_llm_reranking_focused.cpp
 * @brief Persistence, signature, and deployment-boundary tests for LoRAManifestStore.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <thread>

#include <openssl/evp.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>

#include "plugins/pinned_ed25519_verifier.h"
#include "retrieval/include/lora_package.h"

using namespace themis::retrieval;

namespace {

std::filesystem::path NewTestDirectory() {
    static std::atomic<uint64_t> next_id{0};
    auto path = std::filesystem::temp_directory_path() /
        ("themis-retrieval-store-" +
         std::to_string(next_id.fetch_add(1)) + "-" +
         std::to_string(std::chrono::steady_clock::now()
                            .time_since_epoch().count()));
    std::filesystem::create_directories(path);
    return path;
}

LoRAManifestStore::RuntimeConfig MakeConfig(
    const std::filesystem::path& path,
    bool authorize = false) {
    static const std::vector<uint8_t> private_seed = [] {
        std::vector<uint8_t> seed(32);
        for (size_t i = 0; i < seed.size(); ++i) {
            seed[i] = static_cast<uint8_t>(i + 1);
        }
        return seed;
    }();
    auto* private_key = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, private_seed.data(), private_seed.size());
    if (!private_key) {
        throw std::runtime_error("failed to construct test Ed25519 key");
    }
    std::vector<uint8_t> public_key(32);
    size_t public_key_size = public_key.size();
    const int public_key_ok = EVP_PKEY_get_raw_public_key(
        private_key, public_key.data(), &public_key_size);
    EVP_PKEY_free(private_key);
    if (public_key_ok != 1 || public_key_size != 32) {
        throw std::runtime_error("failed to extract test Ed25519 public key");
    }
    public_key.resize(public_key_size);
    const auto signer_id =
        themis::plugins::pinned_ed25519::fingerprint(public_key);

    LoRAManifestStore::RuntimeConfig config;
    config.database_path = path.string();
    themis::plugins::PinnedKey pinned_key;
    pinned_key.fingerprint = signer_id;
    pinned_key.public_key = public_key;
    pinned_key.label = "retrieval unit test key";
    config.pinned_keys.push_back(pinned_key);
    if (authorize) {
        config.deployment_authorizer =
            [](const PortableAdapterProduct&, const LoRAPackage&) { return true; };
    }
    config.quality_gate =
        std::make_shared<themis::rag::quality::DeploymentGateController>(5.0, 2.0);
    return config;
}

std::vector<uint8_t> TestPrivateSeed() {
    std::vector<uint8_t> seed(32);
    for (size_t i = 0; i < seed.size(); ++i) {
        seed[i] = static_cast<uint8_t>(i + 1);
    }
    return seed;
}

std::pair<std::string, std::vector<uint8_t>> TestPublicKey() {
    const auto seed = TestPrivateSeed();
    auto* key = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, seed.data(), seed.size());
    if (!key) {
        throw std::runtime_error("failed to construct test Ed25519 key");
    }
    std::vector<uint8_t> public_key(32);
    size_t size = public_key.size();
    const int result =
        EVP_PKEY_get_raw_public_key(key, public_key.data(), &size);
    EVP_PKEY_free(key);
    if (result != 1 || size != 32) {
        throw std::runtime_error("failed to extract test Ed25519 public key");
    }
    public_key.resize(size);
    return {themis::plugins::pinned_ed25519::fingerprint(public_key),
            std::move(public_key)};
}

std::string Hex(const std::vector<uint8_t>& bytes) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2);
    for (const auto byte : bytes) {
        result.push_back(kHex[byte >> 4]);
        result.push_back(kHex[byte & 0x0f]);
    }
    return result;
}

std::string SignHash(const std::string& hash) {
    const auto seed = TestPrivateSeed();
    auto* key = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, seed.data(), seed.size());
    auto* context = EVP_MD_CTX_new();
    if (!key || !context ||
        EVP_DigestSignInit(context, nullptr, nullptr, nullptr, key) != 1) {
        EVP_MD_CTX_free(context);
        EVP_PKEY_free(key);
        throw std::runtime_error("failed to initialize test Ed25519 signer");
    }
    std::vector<uint8_t> signature(64);
    size_t signature_size = signature.size();
    const int sign_result = EVP_DigestSign(
        context, signature.data(), &signature_size,
        reinterpret_cast<const unsigned char*>(hash.data()), hash.size());
    EVP_MD_CTX_free(context);
    EVP_PKEY_free(key);
    if (sign_result != 1 || signature_size != 64) {
        throw std::runtime_error("failed to sign retrieval test manifest");
    }
    std::string encoded(89, '\0');
    const int encoded_size = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(encoded.data()), signature.data(),
        static_cast<int>(signature.size()));
    if (encoded_size != 88) {
        throw std::runtime_error("failed to encode test signature");
    }
    encoded.resize(static_cast<size_t>(encoded_size));
    return encoded;
}

void Sign(LoRAPackage& package) {
    package.computeManifestHash();
    package.integrity.signature = SignHash(package.integrity.manifest_hash);
    package.integrity.signature_algorithm = "Ed25519";
    package.integrity.signer_id = TestPublicKey().first;
    package.integrity.signed_at = "2026-11-01T00:00:00Z";
}

void Sign(PortableAdapterProduct& product) {
    product.computeManifestHash();
    product.integrity.signature = SignHash(product.integrity.manifest_hash);
    product.integrity.signature_algorithm = "Ed25519";
    product.integrity.signer_id = TestPublicKey().first;
    product.integrity.signed_at = "2026-11-01T00:00:00Z";
}

LoRAPackage MakeValidatedPackage() {
    LoRAPackage package;
    package.package_id = "package-1";
    package.name = "retrieval-adapter";
    package.version = "1.0.0";
    package.status = LoRAPackageStatus::VALIDATED;
    Sign(package);
    return package;
}

PortableAdapterProduct MakeReadyProduct() {
    PortableAdapterProduct product;
    product.product_id = "product-1";
    product.source_package_id = "package-1";
    product.target_base_model_id = "base-model";
    product.status = AdapterProductStatus::READY;
    Sign(product);
    return product;
}

themis::rag::quality::AggregatedMetrics Metrics(double score) {
    themis::rag::quality::AggregatedMetrics metrics;
    metrics.mean_recall_10 = score;
    metrics.mean_ndcg_10 = score;
    metrics.mean_mrr = score;
    metrics.sample_count = 10;
    return metrics;
}

}  // namespace

TEST(RetrievalArtifactStore, DefaultConstructorIsIsolatedAndRejectsWrites) {
    {
        LoRAManifestStore store;
        const auto package = MakeValidatedPackage();
        EXPECT_FALSE(store.storePackage(package));
        EXPECT_FALSE(store.loadPackage(package.package_id).has_value());
        EXPECT_FALSE(store.verifyPackageIntegrity(package.package_id));

        EXPECT_FALSE(store.storeProduct(MakeReadyProduct()));
        const auto result =
            store.deployProduct("product-1", 2, 1, Metrics(0.8), Metrics(0.8));
        EXPECT_FALSE(result.deployed);
        EXPECT_NE(result.reason.find("test-only"), std::string::npos);
    }
    {
        LoRAManifestStore fresh_store;
        EXPECT_EQ(fresh_store.packageCount(), 0u);
        EXPECT_EQ(fresh_store.productCount(), 0u);
    }
}

TEST(RetrievalArtifactStore, PersistsPackagesAndProductsAcrossReopen) {
    const auto root = NewTestDirectory();
    const auto db_path = root / "artifacts";
    {
        LoRAManifestStore store(MakeConfig(db_path));
        auto package = MakeValidatedPackage();
        auto product = MakeReadyProduct();
        ASSERT_TRUE(store.storePackage(package));
        ASSERT_TRUE(store.storeProduct(product));
    }
    {
        LoRAManifestStore store(MakeConfig(db_path));
        const auto package = store.loadPackage("package-1");
        const auto product = store.loadProduct("product-1");
        ASSERT_TRUE(package.has_value());
        ASSERT_TRUE(product.has_value());
        EXPECT_EQ(package->version, "1.0.0");
        EXPECT_EQ(product->status, AdapterProductStatus::READY);
        EXPECT_EQ(store.packageCount(), 1u);
        EXPECT_EQ(store.productCount(), 1u);
    }
    std::filesystem::remove_all(root);
}

TEST(RetrievalArtifactStore, RejectsUnsignedRecordsAlreadyPersistedInRocksDB) {
    const auto root = NewTestDirectory();
    const auto db_path = root / "artifacts";
    {
        LoRAManifestStore store(MakeConfig(db_path));
        ASSERT_TRUE(store.storePackage(MakeValidatedPackage()));
    }

    rocksdb::Options options;
    rocksdb::DB* raw_db = nullptr;
    ASSERT_TRUE(rocksdb::DB::Open(options, db_path.string(), &raw_db).ok());
    std::unique_ptr<rocksdb::DB> db(raw_db);
    auto unsigned_package = MakeValidatedPackage();
    unsigned_package.integrity.signature.clear();
    ASSERT_TRUE(db->Put(rocksdb::WriteOptions(),
                        "retrieval:package:package-1",
                        unsigned_package.to_json().dump()).ok());
    db.reset();

    EXPECT_THROW(LoRAManifestStore(MakeConfig(db_path)), std::runtime_error);
    std::filesystem::remove_all(root);
}

TEST(RetrievalArtifactStore, RejectsMissingMalformedAndUnknownConfiguration) {
    EXPECT_THROW(static_cast<void>(
                     LoRAManifestStore::RuntimeConfig::loadFromYaml("")),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(LoRAManifestStore::RuntimeConfig::loadFromYaml(
                     "/definitely/missing/retrieval.yaml")),
                 std::runtime_error);

    LoRAManifestStore::RuntimeConfig empty_path;
    EXPECT_THROW(LoRAManifestStore(std::move(empty_path)), std::invalid_argument);

    const auto root = NewTestDirectory();
    const auto no_pins = root / "no-pins.yaml";
    std::ofstream(no_pins)
        << "retrieval:\n"
           "  artifact_registry:\n"
           "    database_path: /tmp/retrieval\n"
           "    pinned_keys: []\n";
    EXPECT_THROW(static_cast<void>(
                     LoRAManifestStore::RuntimeConfig::loadFromYaml(
                         no_pins.string())),
                 std::invalid_argument);
    const auto malformed = root / "malformed.yaml";
    std::ofstream(malformed) << "retrieval: [broken\n";
    EXPECT_THROW(static_cast<void>(
                     LoRAManifestStore::RuntimeConfig::loadFromYaml(
                         malformed.string())),
                 std::runtime_error);
    std::filesystem::remove_all(root);
}

TEST(RetrievalArtifactStore, LoadsThemisYamlAndVerifiesPinnedEd25519Signature) {
    const auto root = NewTestDirectory();
    const auto [fingerprint, public_key] = TestPublicKey();
    const auto yaml_path = root / "retrieval.yaml";
    std::ofstream(yaml_path)
        << "retrieval:\n"
           "  artifact_registry:\n"
           "    database_path: \"" << (root / "artifacts").string() << "\"\n"
           "    pinned_keys:\n"
           "      - fingerprint: \"" << fingerprint << "\"\n"
           "        public_key_hex: \"" << Hex(public_key) << "\"\n"
           "        label: \"test release key\"\n";

    auto config =
        LoRAManifestStore::RuntimeConfig::loadFromYaml(yaml_path.string());
    LoRAManifestStore store(std::move(config));
    const auto package = MakeValidatedPackage();
    ASSERT_TRUE(store.storePackage(package));
    EXPECT_TRUE(store.verifyPackageIntegrity(package.package_id));
    std::filesystem::remove_all(root);
}

TEST(RetrievalArtifactStore, RequiresSignatureAndPinnedSigner) {
    const auto root = NewTestDirectory();
    {
        auto config = MakeConfig(root / "missing-signature");
        LoRAManifestStore store(std::move(config));
        auto package = MakeValidatedPackage();
        package.integrity.signature.clear();
        EXPECT_FALSE(store.storePackage(package));
        EXPECT_FALSE(store.verifyPackageIntegrity(package.package_id));
    }
    {
        auto config = MakeConfig(root / "bad-signature");
        LoRAManifestStore store(std::move(config));
        auto package = MakeValidatedPackage();
        package.integrity.signature = "not-a-valid-signature";
        EXPECT_FALSE(store.storePackage(package));
        EXPECT_FALSE(store.verifyPackageIntegrity(package.package_id));
    }
    {
        auto config = MakeConfig(root / "unpinned-signer");
        LoRAManifestStore store(std::move(config));
        auto package = MakeValidatedPackage();
        package.integrity.signer_id = "untrusted-key";
        EXPECT_FALSE(store.storePackage(package));
        EXPECT_FALSE(store.verifyPackageIntegrity(package.package_id));
    }
    {
        auto config = MakeConfig(root / "no-pins");
        config.pinned_keys.clear();
        EXPECT_THROW(LoRAManifestStore(std::move(config)), std::invalid_argument);
    }
    {
        auto config = MakeConfig(root / "malformed-signature");
        LoRAManifestStore store(std::move(config));
        auto package = MakeValidatedPackage();
        package.integrity.signature = "broken-base64";
        EXPECT_FALSE(store.storePackage(package));
        EXPECT_FALSE(store.verifyPackageIntegrity(package.package_id));
    }
    std::filesystem::remove_all(root);
}

TEST(RetrievalArtifactStore, AcceptsValidEd25519SignatureFromPinnedSigner) {
    const auto root = NewTestDirectory();
    {
        LoRAManifestStore store(MakeConfig(root / "signed"));
        const auto package = MakeValidatedPackage();
        ASSERT_TRUE(store.storePackage(package));
        EXPECT_TRUE(store.verifyPackageIntegrity(package.package_id));
    }
    std::filesystem::remove_all(root);
}

TEST(RetrievalArtifactStore, DeploymentFailsClosedWithoutAuthorizationCallback) {
    const auto root = NewTestDirectory();
    {
        auto config = MakeConfig(root / "no-auth");
        config.deployment_authorizer = {};
        LoRAManifestStore store(std::move(config));
        ASSERT_TRUE(store.storePackage(MakeValidatedPackage()));
        ASSERT_TRUE(store.storeProduct(MakeReadyProduct()));
        const auto result =
            store.deployProduct("product-1", 2, 1, Metrics(0.8), Metrics(0.8));
        EXPECT_FALSE(result.deployed);
        EXPECT_NE(result.reason.find("authorization"), std::string::npos);
        EXPECT_EQ(store.loadProduct("product-1")->status, AdapterProductStatus::READY);
    }
    {
        auto config = MakeConfig(root / "no-quality-gate", true);
        config.quality_gate.reset();
        LoRAManifestStore store(std::move(config));
        ASSERT_TRUE(store.storePackage(MakeValidatedPackage()));
        ASSERT_TRUE(store.storeProduct(MakeReadyProduct()));
        const auto result =
            store.deployProduct("product-1", 2, 1, Metrics(0.8), Metrics(0.8));
        EXPECT_FALSE(result.deployed);
        EXPECT_NE(result.reason.find("quality gate"), std::string::npos);
    }
    std::filesystem::remove_all(root);
}

TEST(RetrievalArtifactStore, DeploymentRequiresQualityAllowAndAuthorizationApproval) {
    const auto root = NewTestDirectory();
    {
        LoRAManifestStore store(MakeConfig(root / "gate"));
        ASSERT_TRUE(store.storePackage(MakeValidatedPackage()));
        ASSERT_TRUE(store.storeProduct(MakeReadyProduct()));

        auto denied =
            store.deployProduct("product-1", 2, 1, Metrics(0.6), Metrics(0.8));
        EXPECT_FALSE(denied.deployed);
        EXPECT_EQ(denied.quality_decision.decision,
                  themis::rag::quality::GateDecision::kDeny);
        EXPECT_EQ(store.loadProduct("product-1")->status, AdapterProductStatus::READY);
    }
    {
        auto config = MakeConfig(root / "authorized", true);
        LoRAManifestStore store(std::move(config));
        ASSERT_TRUE(store.storePackage(MakeValidatedPackage()));
        ASSERT_TRUE(store.storeProduct(MakeReadyProduct()));
        const auto allowed =
            store.deployProduct("product-1", 2, 1, Metrics(0.8), Metrics(0.8));
        ASSERT_TRUE(allowed.deployed) << allowed.reason;
        EXPECT_EQ(store.loadProduct("product-1")->status,
                  AdapterProductStatus::DEPLOYED);
    }
    {
        LoRAManifestStore reopened(MakeConfig(root / "authorized", true));
        const auto product = reopened.loadProduct("product-1");
        ASSERT_TRUE(product.has_value());
        EXPECT_EQ(product->status, AdapterProductStatus::DEPLOYED);
        EXPECT_TRUE(reopened.verifyProductIntegrity("product-1"));
    }
    std::filesystem::remove_all(root);
}

TEST(RetrievalArtifactStore, RejectsEmptyQualityEvaluationAndAuthorizationDenial) {
    const auto root = NewTestDirectory();
    {
        auto config = MakeConfig(root / "empty-metrics", true);
        LoRAManifestStore store(std::move(config));
        ASSERT_TRUE(store.storePackage(MakeValidatedPackage()));
        ASSERT_TRUE(store.storeProduct(MakeReadyProduct()));
        const auto result =
            store.deployProduct("product-1", 2, 1, Metrics(0.8), Metrics(0.0));
        EXPECT_FALSE(result.deployed);
        EXPECT_NE(result.reason.find("metrics"), std::string::npos);
    }
    {
        auto config = MakeConfig(root / "denied", true);
        config.deployment_authorizer =
            [](const PortableAdapterProduct&, const LoRAPackage&) { return false; };
        LoRAManifestStore store(std::move(config));
        ASSERT_TRUE(store.storePackage(MakeValidatedPackage()));
        ASSERT_TRUE(store.storeProduct(MakeReadyProduct()));
        const auto result =
            store.deployProduct("product-1", 2, 1, Metrics(0.8), Metrics(0.8));
        EXPECT_FALSE(result.deployed);
        EXPECT_NE(result.reason.find("denied"), std::string::npos);
    }
    {
        auto config = MakeConfig(root / "throwing-auth", true);
        config.deployment_authorizer =
            [](const PortableAdapterProduct&, const LoRAPackage&) -> bool {
                throw std::runtime_error("authorization service unavailable");
            };
        LoRAManifestStore store(std::move(config));
        ASSERT_TRUE(store.storePackage(MakeValidatedPackage()));
        ASSERT_TRUE(store.storeProduct(MakeReadyProduct()));
        const auto result =
            store.deployProduct("product-1", 2, 1, Metrics(0.8), Metrics(0.8));
        EXPECT_FALSE(result.deployed);
        EXPECT_NE(result.reason.find("failed"), std::string::npos);
    }
    std::filesystem::remove_all(root);
}

TEST(RetrievalArtifactStore, ConcurrentWritesRemainReadable) {
    const auto root = NewTestDirectory();
    {
        LoRAManifestStore store(MakeConfig(root / "concurrent"));
        std::vector<std::thread> writers;
        for (int worker = 0; worker < 4; ++worker) {
            writers.emplace_back([worker, &store]() {
                for (int item = 0; item < 20; ++item) {
                    LoRAPackage package;
                    package.package_id = "package-" + std::to_string(worker) +
                                          "-" + std::to_string(item);
                    package.name = "concurrent";
                    package.version = "1.0.0";
                    Sign(package);
                    EXPECT_TRUE(store.storePackage(package));
                    EXPECT_TRUE(store.loadPackage(package.package_id).has_value());
                }
            });
        }
        for (auto& writer : writers) {
            writer.join();
        }
        EXPECT_EQ(store.packageCount(), 80u);
    }
    std::filesystem::remove_all(root);
}

TEST(RetrievalArtifactStore, ReportsDatabaseOpenErrors) {
    const auto root = NewTestDirectory();
    const auto parent_file = root / "not-a-directory";
    std::ofstream(parent_file) << "not a directory";
    auto config = MakeConfig(parent_file / "database");
    EXPECT_THROW(LoRAManifestStore(std::move(config)), std::exception);
    std::filesystem::remove_all(root);
}
