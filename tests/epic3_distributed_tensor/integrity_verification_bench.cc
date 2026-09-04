/**
 * @file integrity_verification_bench.cc
 * @brief Performance benchmarks for integrity verification module.
 *
 * Benchmarks measure:
 * - SHA-256 computation on various payload sizes
 * - Merkle proof generation and verification
 * - Receipt chain validation
 * - Full artifact verification workflow
 *
 * Build and run with:
 *   cmake --build . --target integrity_verification_bench
 *   ./integrity_verification_bench --benchmark_min_time=1.0
 */

#include "src/distributed_tensor/include/integrity_verification.h"
#include <benchmark/benchmark.h>
#include <string>
#include <vector>

namespace themis {
namespace distributed_tensor {

// ============================================================================
// Utility Functions
// ============================================================================

static std::string createPayload(size_t size) {
    std::string payload = {};
    payload.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        payload.push_back(static_cast<char>(i % 256));
    }
    return payload;
}

static VerificationReceipt createReceipt(
    const std::string& artifact_id,
    const std::string& content_hash) {
    VerificationReceipt receipt;
    receipt.receipt_id = "receipt-" + artifact_id;
    receipt.artifact_id = artifact_id;
    receipt.content_hash = content_hash;
    receipt.timestamp = "2026-07-05T19:00:00Z";
    receipt.parent_receipt_hash = "";
    receipt.package_lineage_hash = "lineage-hash";
    receipt.shard_placement_id = "placement-id";
    receipt.receipt_hash = receipt.computeContentHash();
    return receipt;
}

// ============================================================================
// SHA-256 Benchmarks
// ============================================================================

static void BenchmarkSHA256_1KB(benchmark::State& state) {
    std::string data = createPayload(1024);
    for (auto _ : state) {
        benchmark::DoNotOptimize(computeSHA256(data));
    }
}
BENCHMARK(BenchmarkSHA256_1KB)->Unit(benchmark::kMicrosecond);

static void BenchmarkSHA256_10KB(benchmark::State& state) {
    std::string data = createPayload(10 * 1024);
    for (auto _ : state) {
        benchmark::DoNotOptimize(computeSHA256(data));
    }
}
BENCHMARK(BenchmarkSHA256_10KB)->Unit(benchmark::kMicrosecond);

static void BenchmarkSHA256_100KB(benchmark::State& state) {
    std::string data = createPayload(100 * 1024);
    for (auto _ : state) {
        benchmark::DoNotOptimize(computeSHA256(data));
    }
}
BENCHMARK(BenchmarkSHA256_100KB)->Unit(benchmark::kMicrosecond);

static void BenchmarkSHA256_1MB(benchmark::State& state) {
    std::string data = createPayload(1024 * 1024);
    for (auto _ : state) {
        benchmark::DoNotOptimize(computeSHA256(data));
    }
}
BENCHMARK(BenchmarkSHA256_1MB)->Unit(benchmark::kMillisecond);

static void BenchmarkSHA256_10MB(benchmark::State& state) {
    std::string data = createPayload(10 * 1024 * 1024);
    for (auto _ : state) {
        benchmark::DoNotOptimize(computeSHA256(data));
    }
}
BENCHMARK(BenchmarkSHA256_10MB)->Unit(benchmark::kMillisecond);

// ============================================================================
// Content Hash Benchmarks
// ============================================================================

static void BenchmarkContentHashValidation(benchmark::State& state) {
    ContentHash hash;
    hash.value = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    for (auto _ : state) {
        benchmark::DoNotOptimize(hash.isValid());
    }
}
BENCHMARK(BenchmarkContentHashValidation)->Unit(benchmark::kNanosecond);

// ============================================================================
// Receipt Chain Benchmarks
// ============================================================================

static void BenchmarkReceiptChainCreation_10Receipts(benchmark::State& state) {
    for (auto _ : state) {
        ReceiptChain chain;
        for (int i = 0; i < 10; ++i) {
            auto hash = computeSHA256("payload" + std::to_string(i));
            VerificationReceipt receipt = createReceipt("artifact1", hash);
            chain.appendReceipt(receipt);
        }
        benchmark::DoNotOptimize(chain);
    }
}
BENCHMARK(BenchmarkReceiptChainCreation_10Receipts)->Unit(benchmark::kMicrosecond);

static void BenchmarkReceiptChainCreation_100Receipts(benchmark::State& state) {
    for (auto _ : state) {
        ReceiptChain chain;
        for (int i = 0; i < 100; ++i) {
            auto hash = computeSHA256("payload" + std::to_string(i));
            VerificationReceipt receipt = createReceipt("artifact1", hash);
            chain.appendReceipt(receipt);
        }
        benchmark::DoNotOptimize(chain);
    }
}
BENCHMARK(BenchmarkReceiptChainCreation_100Receipts)->Unit(benchmark::kMillisecond);

static void BenchmarkReceiptChainVerification_10Receipts(benchmark::State& state) {
    ReceiptChain chain;
    for (int i = 0; i < 10; ++i) {
        auto hash = computeSHA256("payload" + std::to_string(i));
        VerificationReceipt receipt = createReceipt("artifact1", hash);
        chain.appendReceipt(receipt);
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(chain.verifyChainIntegrity());
    }
}
BENCHMARK(BenchmarkReceiptChainVerification_10Receipts)->Unit(benchmark::kMicrosecond);

static void BenchmarkReceiptChainVerification_100Receipts(benchmark::State& state) {
    ReceiptChain chain;
    for (int i = 0; i < 100; ++i) {
        auto hash = computeSHA256("payload" + std::to_string(i));
        VerificationReceipt receipt = createReceipt("artifact1", hash);
        chain.appendReceipt(receipt);
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(chain.verifyChainIntegrity());
    }
}
BENCHMARK(BenchmarkReceiptChainVerification_100Receipts)->Unit(benchmark::kMillisecond);

// ============================================================================
// Merkle Proof Benchmarks
// ============================================================================

static void BenchmarkMerkleProofVerification_8Levels(benchmark::State& state) {
    MerkleProof proof;
    proof.root_hash = "root";
    proof.fragment_hash = "fragment";
    proof.fragment_index = 0;
    
    for (int i = 0; i < 8; ++i) {
        MerkleProofComponent comp;
        comp.sibling_hash = "sibling" + std::to_string(i);
        comp.is_left = (i % 2 == 0);
        proof.proof_path.push_back(comp);
    }
    
    for (auto _ : state) {
        // Verify the proof depth calculation
        benchmark::DoNotOptimize(proof.getProofDepth());
    }
}
BENCHMARK(BenchmarkMerkleProofVerification_8Levels)->Unit(benchmark::kNanosecond);

static void BenchmarkMerkleProofVerification_16Levels(benchmark::State& state) {
    MerkleProof proof;
    proof.root_hash = "root";
    proof.fragment_hash = "fragment";
    proof.fragment_index = 0;
    
    for (int i = 0; i < 16; ++i) {
        MerkleProofComponent comp;
        comp.sibling_hash = "sibling" + std::to_string(i);
        comp.is_left = (i % 2 == 0);
        proof.proof_path.push_back(comp);
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(proof.getProofDepth());
    }
}
BENCHMARK(BenchmarkMerkleProofVerification_16Levels)->Unit(benchmark::kNanosecond);

// ============================================================================
// Full Verification Workflow Benchmarks
// ============================================================================

static void BenchmarkFullVerification_1KBPayload(benchmark::State& state) {
    std::string payload = createPayload(1024);
    std::string expected_hash = computeSHA256(payload);
    
    for (auto _ : state) {
        auto result = verifyArtifactIntegrity("artifact1", payload, expected_hash);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchmarkFullVerification_1KBPayload)->Unit(benchmark::kMicrosecond);

static void BenchmarkFullVerification_100KBPayload(benchmark::State& state) {
    std::string payload = createPayload(100 * 1024);
    std::string expected_hash = computeSHA256(payload);
    
    for (auto _ : state) {
        auto result = verifyArtifactIntegrity("artifact1", payload, expected_hash);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchmarkFullVerification_100KBPayload)->Unit(benchmark::kMicrosecond);

static void BenchmarkFullVerification_1MBPayload(benchmark::State& state) {
    std::string payload = createPayload(1024 * 1024);
    std::string expected_hash = computeSHA256(payload);
    
    for (auto _ : state) {
        auto result = verifyArtifactIntegrity("artifact1", payload, expected_hash);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchmarkFullVerification_1MBPayload)->Unit(benchmark::kMillisecond);

// ============================================================================
// PHASE 3 Error Handling Benchmarks
// ============================================================================

static void BenchmarkDetectReceiptChainTampering_10Receipts(benchmark::State& state) {
    ReceiptChain chain;
    for (int i = 0; i < 10; ++i) {
        auto hash = computeSHA256("payload" + std::to_string(i));
        VerificationReceipt receipt = createReceipt("artifact1", hash);
        chain.appendReceipt(receipt);
    }
    
    for (auto _ : state) {
        auto result = detectReceiptChainTampering(chain);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchmarkDetectReceiptChainTampering_10Receipts)->Unit(benchmark::kMicrosecond);

static void BenchmarkDetectReceiptChainTampering_100Receipts(benchmark::State& state) {
    ReceiptChain chain;
    for (int i = 0; i < 100; ++i) {
        auto hash = computeSHA256("payload" + std::to_string(i));
        VerificationReceipt receipt = createReceipt("artifact1", hash);
        chain.appendReceipt(receipt);
    }
    
    for (auto _ : state) {
        auto result = detectReceiptChainTampering(chain);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchmarkDetectReceiptChainTampering_100Receipts)->Unit(benchmark::kMillisecond);

static void BenchmarkHandleStaleReceipt(benchmark::State& state) {
    auto hash = computeSHA256("test");
    VerificationReceipt receipt = createReceipt("artifact1", hash);
    
    for (auto _ : state) {
        auto result = handleStaleReceipt(receipt, receipt.package_lineage_hash, hash);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchmarkHandleStaleReceipt)->Unit(benchmark::kMicrosecond);

static void BenchmarkVerifyFragmentIntegrity(benchmark::State& state) {
    std::string fragment_data = "fragment bytes";
    MerkleProof proof;
    proof.root_hash = "root";
    proof.fragment_hash = computeSHA256(fragment_data);
    proof.fragment_index = 0;
    
    MerkleProofComponent comp;
    comp.sibling_hash = "sibling";
    comp.is_left = true;
    proof.proof_path.push_back(comp);
    
    for (auto _ : state) {
        auto result = verifyFragmentIntegrity(
            "artifact1",
            fragment_data,
            0,
            proof,
            "root");
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchmarkVerifyFragmentIntegrity)->Unit(benchmark::kMicrosecond);

}  // namespace distributed_tensor
}  // namespace themis

BENCHMARK_MAIN();
