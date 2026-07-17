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

using namespace themis::retrieval;

// ============================================================================
// Fixture helpers
// ============================================================================

namespace {

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
LoRAManifestStore makePopulatedStore(size_t n) {
    LoRAManifestStore store;
    for (size_t i = 0; i < n; ++i) {
        auto pkg = makePackage("pkg-" + std::to_string(i));
        pkg.computeManifestHash();
        store.storePackage(pkg);
        store.storeProduct(makeProduct("prod-" + std::to_string(i),
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
    LoRAManifestStore store;
    const auto pkg = makePackage();
    int idx = 0;
    for (auto _ : state) {
        LoRAPackage p = pkg;
        p.package_id  = "pkg-" + std::to_string(idx++);
        benchmark::DoNotOptimize(store.storePackage(p));
    }
}
BENCHMARK(BM_ManifestStore_Store_Pkg);

static void BM_ManifestStore_Load_Pkg(benchmark::State& state) {
    LoRAManifestStore store;
    auto pkg = makePackage("load-target");
    pkg.computeManifestHash();
    store.storePackage(pkg);

    for (auto _ : state) {
        benchmark::DoNotOptimize(store.loadPackage("load-target"));
    }
}
BENCHMARK(BM_ManifestStore_Load_Pkg);

static void BM_ManifestStore_Verify_Pkg(benchmark::State& state) {
    LoRAManifestStore store;
    auto pkg = makePackage("verify-target");
    pkg.computeManifestHash();
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
    const LoRAManifestStore store = makePopulatedStore(100);
    for (auto _ : state) {
        benchmark::DoNotOptimize(store.exportPackages());
    }
}
BENCHMARK(BM_ManifestStore_Export);

static void BM_ManifestStore_Import(benchmark::State& state) {
    const LoRAManifestStore src = makePopulatedStore(100);
    const json exported = src.exportPackages();
    for (auto _ : state) {
        LoRAManifestStore store;
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
