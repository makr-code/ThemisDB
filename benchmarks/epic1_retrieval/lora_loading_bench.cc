/**
 * @file lora_loading_bench.cc
 * @brief Google Benchmark suite for LoRAPackage and PortableAdapterProduct
 *        manifest operations (Phase 3 — serialization, integrity, store CRUD).
 *
 * Benchmark scenarios:
 *   BM_LoRAPackage_ToJson         — serialize a single package to JSON
 *   BM_LoRAPackage_FromJson       — deserialize a pre-built JSON object
 *   BM_LoRAPackage_ComputeHash    — compute manifest SHA-256
 *   BM_Product_ToJson             — serialize a single product to JSON
 *   BM_Product_FromJson           — deserialize a pre-built JSON object
 *   BM_Product_ComputeHash        — compute product manifest SHA-256
 *   BM_ManifestStore_Store_Pkg    — store a package into the registry
 *   BM_ManifestStore_Load_Pkg     — load a package by ID
 *   BM_ManifestStore_Verify_Pkg   — verify package manifest hash
 *   BM_ManifestStore_Export       — bulk export 100 packages to JSON
 *   BM_ManifestStore_Import       — bulk import 100 packages from JSON
 *   BM_IntegrityHelper_Sha256     — hash a 4 KiB buffer
 */

#include <benchmark/benchmark.h>
#include "lora_package.h"

#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <openssl/evp.h>
#include "plugins/pinned_ed25519_verifier.h"

using namespace themis::retrieval;

// ============================================================================
// Fixture helpers
// ============================================================================

namespace {

std::vector<uint8_t> benchmarkSeed() {
    std::vector<uint8_t> seed(32);
    for (size_t i = 0; i < seed.size(); ++i) {
        seed[i] = static_cast<uint8_t>(i + 33);
    }
    return seed;
}

themis::plugins::PinnedKey benchmarkPinnedKey() {
    const auto seed = benchmarkSeed();
    auto* key = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, seed.data(), seed.size());
    if (!key) {
        throw std::runtime_error("failed to construct benchmark Ed25519 key");
    }
    themis::plugins::PinnedKey pin;
    pin.public_key.resize(32);
    size_t size = pin.public_key.size();
    const int result =
        EVP_PKEY_get_raw_public_key(key, pin.public_key.data(), &size);
    EVP_PKEY_free(key);
    if (result != 1 || size != 32) {
        throw std::runtime_error("failed to extract benchmark Ed25519 key");
    }
    pin.public_key.resize(size);
    pin.fingerprint =
        themis::plugins::pinned_ed25519::fingerprint(pin.public_key);
    pin.label = "benchmark-only key";
    return pin;
}

std::string signHash(const std::string& hash) {
    const auto seed = benchmarkSeed();
    auto* key = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, seed.data(), seed.size());
    auto* context = EVP_MD_CTX_new();
    if (!key || !context ||
        EVP_DigestSignInit(context, nullptr, nullptr, nullptr, key) != 1) {
        EVP_MD_CTX_free(context);
        EVP_PKEY_free(key);
        throw std::runtime_error("failed to initialize benchmark signer");
    }
    std::vector<uint8_t> signature(64);
    size_t size = signature.size();
    const int result = EVP_DigestSign(
        context, signature.data(), &size,
        reinterpret_cast<const unsigned char*>(hash.data()), hash.size());
    EVP_MD_CTX_free(context);
    EVP_PKEY_free(key);
    if (result != 1 || size != 64) {
        throw std::runtime_error("failed to sign benchmark manifest");
    }
    std::string encoded(89, '\0');
    const int encoded_size = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(encoded.data()), signature.data(),
        static_cast<int>(signature.size()));
    if (encoded_size != 88) {
        throw std::runtime_error("failed to encode benchmark signature");
    }
    encoded.resize(static_cast<size_t>(encoded_size));
    return encoded;
}

void sign(LoRAPackage& package) {
    package.computeManifestHash();
    package.integrity.signature = signHash(package.integrity.manifest_hash);
    package.integrity.signature_algorithm = "Ed25519";
    package.integrity.signer_id = benchmarkPinnedKey().fingerprint;
}

void sign(PortableAdapterProduct& product) {
    product.computeManifestHash();
    product.integrity.signature = signHash(product.integrity.manifest_hash);
    product.integrity.signature_algorithm = "Ed25519";
    product.integrity.signer_id = benchmarkPinnedKey().fingerprint;
}

LoRAManifestStore::RuntimeConfig makeStoreConfig() {
    static std::atomic<uint64_t> next_id{0};
    LoRAManifestStore::RuntimeConfig config;
    config.database_path =
        (std::filesystem::temp_directory_path() /
         ("themis-epic1-retrieval-bench-" +
          std::to_string(std::chrono::steady_clock::now()
                             .time_since_epoch().count()) + "-" +
          std::to_string(next_id.fetch_add(1)))).string();
    const auto pin = benchmarkPinnedKey();
    config.pinned_keys.push_back(pin);
    return config;
}

LoRAPackage makePackage(const std::string& id = "bench-pkg") {
    LoRAPackage p;
    p.package_id              = id;
    p.name                    = "bench-lora";
    p.version                 = "1.0.0";
    p.description             = "Benchmark adapter package";
    p.supported_architectures = {"llama", "mistral"};
    p.lora_rank               = 16;
    p.lora_alpha              = 32.0f;
    p.target_modules          = {"q_proj", "v_proj", "k_proj", "o_proj"};
    p.weights_path            = "/adapters/bench/weights.safetensors";
    p.status                  = LoRAPackageStatus::VALIDATED;
    p.created_at              = "2026-07-01T00:00:00Z";
    p.updated_at              = "2026-07-01T00:00:00Z";
    p.provenance.trainer_id   = "trainer-bench";
    p.provenance.training_framework = "PEFT-0.14";
    p.provenance.dataset_id   = "ds-bench";
    p.provenance.dataset_hash = std::string(64, 'a');
    p.provenance.base_model_id  = "llama-3-8b";
    p.provenance.base_model_hash = std::string(64, 'b');
    p.policy.license          = "MIT";
    p.policy.allowed_base_models = {"llama-3-8b"};
    sign(p);
    return p;
}

PortableAdapterProduct makeProduct(const std::string& id = "bench-prod",
                                    const std::string& pkg_id = "bench-pkg") {
    PortableAdapterProduct pr;
    pr.product_id               = id;
    pr.name                     = "bench-prod";
    pr.version                  = "1.0.0";
    pr.source_package_id        = pkg_id;
    pr.target_base_model_id     = "llama-3-8b";
    pr.target_model_architecture = "llama";
    pr.quantization             = "Q4_K_M";
    pr.format                   = "GGUF-ST";
    pr.file_path                = "/products/bench.gguf";
    pr.file_size_bytes          = 4194304u;  // 4 MiB
    pr.max_context_length       = 4096;
    pr.memory_requirement_mb    = 2048u;
    pr.compatible_model_versions = {"3.8b-v1", "3.8b-v2"};
    pr.status                   = AdapterProductStatus::READY;
    pr.created_at               = "2026-07-01T01:00:00Z";
    pr.updated_at               = "2026-07-01T01:00:00Z";
    sign(pr);
    return pr;
}

// Pre-built JSON for deserialization benchmarks (avoid allocation in loop)
const json& cachedPackageJson() {
    static const json j = makePackage().to_json();
    return j;
}

const json& cachedProductJson() {
    static const json j = makeProduct().to_json();
    return j;
}

// Pre-populated store with N packages for bulk benchmarks
std::unique_ptr<LoRAManifestStore> makePopulatedStore(size_t n) {
    auto store = std::make_unique<LoRAManifestStore>(makeStoreConfig());
    for (size_t i = 0; i < n; ++i) {
        auto pkg = makePackage("pkg-" + std::to_string(i));
        pkg.computeManifestHash();
        store->storePackage(pkg);
        store->storeProduct(makeProduct("prod-" + std::to_string(i),
                                         "pkg-" + std::to_string(i)));
    }
    return store;
}

} // anonymous namespace

// ============================================================================
// Serialization benchmarks
// ============================================================================

static void BM_LoRAPackage_ToJson(benchmark::State& state) {
    const auto pkg = makePackage();
    for (auto _ : state) {
        benchmark::DoNotOptimize(pkg.to_json());
    }
}
BENCHMARK(BM_LoRAPackage_ToJson);

static void BM_LoRAPackage_FromJson(benchmark::State& state) {
    const auto& j = cachedPackageJson();
    for (auto _ : state) {
        benchmark::DoNotOptimize(LoRAPackage::from_json(j));
    }
}
BENCHMARK(BM_LoRAPackage_FromJson);

static void BM_LoRAPackage_ComputeHash(benchmark::State& state) {
    auto pkg = makePackage();
    for (auto _ : state) {
        pkg.computeManifestHash();
        benchmark::DoNotOptimize(pkg.integrity.manifest_hash);
    }
}
BENCHMARK(BM_LoRAPackage_ComputeHash);

static void BM_Product_ToJson(benchmark::State& state) {
    const auto prod = makeProduct();
    for (auto _ : state) {
        benchmark::DoNotOptimize(prod.to_json());
    }
}
BENCHMARK(BM_Product_ToJson);

static void BM_Product_FromJson(benchmark::State& state) {
    const auto& j = cachedProductJson();
    for (auto _ : state) {
        benchmark::DoNotOptimize(PortableAdapterProduct::from_json(j));
    }
}
BENCHMARK(BM_Product_FromJson);

static void BM_Product_ComputeHash(benchmark::State& state) {
    auto prod = makeProduct();
    for (auto _ : state) {
        prod.computeManifestHash();
        benchmark::DoNotOptimize(prod.integrity.manifest_hash);
    }
}
BENCHMARK(BM_Product_ComputeHash);

// ============================================================================
// Store CRUD benchmarks
// ============================================================================

static void BM_ManifestStore_Store_Pkg(benchmark::State& state) {
    LoRAManifestStore store(makeStoreConfig());
    const auto pkg = makePackage();
    int idx = 0;
    for (auto _ : state) {
        LoRAPackage p = pkg;
        p.package_id  = "pkg-" + std::to_string(idx++);
        state.PauseTiming();
        sign(p);
        state.ResumeTiming();
        benchmark::DoNotOptimize(store.storePackage(p));
    }
}
BENCHMARK(BM_ManifestStore_Store_Pkg);

static void BM_ManifestStore_Load_Pkg(benchmark::State& state) {
    LoRAManifestStore store(makeStoreConfig());
    auto pkg = makePackage("load-target");
    store.storePackage(pkg);

    for (auto _ : state) {
        benchmark::DoNotOptimize(store.loadPackage("load-target"));
    }
}
BENCHMARK(BM_ManifestStore_Load_Pkg);

static void BM_ManifestStore_Verify_Pkg(benchmark::State& state) {
    LoRAManifestStore store(makeStoreConfig());
    auto pkg = makePackage("verify-target");
    store.storePackage(pkg);

    for (auto _ : state) {
        benchmark::DoNotOptimize(store.verifyPackageIntegrity("verify-target"));
    }
}
BENCHMARK(BM_ManifestStore_Verify_Pkg);

// ============================================================================
// Bulk export / import benchmarks (100 entries)
// ============================================================================

static void BM_ManifestStore_Export(benchmark::State& state) {
    const auto store = makePopulatedStore(100);
    for (auto _ : state) {
        benchmark::DoNotOptimize(store->exportPackages());
    }
}
BENCHMARK(BM_ManifestStore_Export);

static void BM_ManifestStore_Import(benchmark::State& state) {
    const auto src = makePopulatedStore(100);
    const json exported = src->exportPackages();
    for (auto _ : state) {
        LoRAManifestStore store(makeStoreConfig());
        benchmark::DoNotOptimize(store.importPackages(exported));
    }
}
BENCHMARK(BM_ManifestStore_Import);

// ============================================================================
// IntegrityHelper: raw SHA-256 throughput (4 KiB block)
// ============================================================================

static void BM_IntegrityHelper_Sha256_4KiB(benchmark::State& state) {
    const std::string data(4096, 'x');
    for (auto _ : state) {
        benchmark::DoNotOptimize(IntegrityHelper::sha256Hex(data));
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));
}
BENCHMARK(BM_IntegrityHelper_Sha256_4KiB);

BENCHMARK_MAIN();
