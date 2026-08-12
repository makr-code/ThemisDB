/**
 * @file bench_security.cpp
 * @brief Performance benchmarks for the ThemisDB security module.
 *
 * Measures the cryptographic and policy-evaluation hot paths mandated by
 * the security module FUTURE_ENHANCEMENTS performance targets:
 *
 *   - AES-256-GCM encrypt/decrypt throughput  : target ≥ 1 GB/s (AES-NI)
 *   - RBAC policy evaluation (100 roles)       : target p99 ≤ 0.5 ms
 *   - Kyber-1024 key encapsulation             : target ≥ 2 000 ops/s
 *   - Dilithium-5 signing                      : target ≥ 1 000 ops/s
 *   - FIPS algorithm-list validation           : overhead characterisation
 *   - AQL injection detection throughput       : throughput characterisation
 *   - Audit log tamper-evident append          : target p99 ≤ 2 ms per entry
 *
 * All benchmarks run in CPU-only mode; no GPU or HSM hardware is required.
 *
 * Usage:
 *   ./bench_security
 *   ./bench_security --benchmark_filter=BM_PostQuantum
 *   ./bench_security --benchmark_min_time=2
 *   ./bench_security --benchmark_format=json --benchmark_out=bench_security.json
 */

#include <benchmark/benchmark.h>

#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "security/rbac.h"
#include "security/fips_crypto_mode.h"
#include "security/aql_injection_detector.h"
#include "security/post_quantum_crypto.h"
#include "utils/audit_logger.h"

#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::vector<uint8_t> random_bytes(std::size_t n) {
    std::vector<uint8_t> buf(n);
    RAND_bytes(buf.data(), static_cast<int>(n));
    return buf;
}

std::string random_string(std::size_t n) {
    static constexpr char kAlpha[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, static_cast<int>(sizeof(kAlpha) - 2));
    std::string s(n, '\0');
    for (char& c : s)
        c = kAlpha[dist(rng)];
    return s;
}

std::filesystem::path benchmark_temp_path(const std::string& stem) {
    auto dir = std::filesystem::temp_directory_path() / "themis_benchmarks";
    std::filesystem::create_directories(dir);
    return dir / stem;
}

// ---------------------------------------------------------------------------
// AES-256-GCM encrypt/decrypt via OpenSSL EVP (AES-NI accelerated)
// ---------------------------------------------------------------------------

void aes_gcm_encrypt(const uint8_t* key32, const uint8_t* iv12,
                     const uint8_t* pt, std::size_t pt_len,
                     uint8_t* ct, uint8_t* tag16) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int len = 0;
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
    EVP_EncryptInit_ex(ctx, nullptr, nullptr, key32, iv12);
    EVP_EncryptUpdate(ctx, ct, &len, pt, static_cast<int>(pt_len));
    EVP_EncryptFinal_ex(ctx, ct + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag16);
    EVP_CIPHER_CTX_free(ctx);
}

void aes_gcm_decrypt(const uint8_t* key32, const uint8_t* iv12,
                     const uint8_t* ct, std::size_t ct_len,
                     const uint8_t* tag16, uint8_t* pt) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int len = 0;
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
    EVP_DecryptInit_ex(ctx, nullptr, nullptr, key32, iv12);
    EVP_DecryptUpdate(ctx, pt, &len, ct, static_cast<int>(ct_len));
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16,
                        const_cast<uint8_t*>(tag16));
    EVP_DecryptFinal_ex(ctx, pt + len, &len);
    EVP_CIPHER_CTX_free(ctx);
}

}  // namespace

// ============================================================================
// Section 1: AES-256-GCM throughput
// ============================================================================

static void BM_AES256GCM_Encrypt_1KB(benchmark::State& state) {
    auto key = random_bytes(32);
    auto iv  = random_bytes(12);
    auto pt  = random_bytes(1024);
    std::vector<uint8_t> ct(1024), tag(16);

    for (auto _ : state) {
        aes_gcm_encrypt(key.data(), iv.data(),
                        pt.data(), pt.size(),
                        ct.data(), tag.data());
        benchmark::DoNotOptimize(ct.data());
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(pt.size()));
    state.SetLabel("AES-256-GCM encrypt 1 KB");
}
BENCHMARK(BM_AES256GCM_Encrypt_1KB);

static void BM_AES256GCM_Encrypt_64KB(benchmark::State& state) {
    auto key = random_bytes(32);
    auto iv  = random_bytes(12);
    auto pt  = random_bytes(64 * 1024);
    std::vector<uint8_t> ct(64 * 1024), tag(16);

    for (auto _ : state) {
        aes_gcm_encrypt(key.data(), iv.data(),
                        pt.data(), pt.size(),
                        ct.data(), tag.data());
        benchmark::DoNotOptimize(ct.data());
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(pt.size()));
    state.SetLabel("AES-256-GCM encrypt 64 KB");
}
BENCHMARK(BM_AES256GCM_Encrypt_64KB);

static void BM_AES256GCM_Encrypt_1MB(benchmark::State& state) {
    auto key = random_bytes(32);
    auto iv  = random_bytes(12);
    auto pt  = random_bytes(1024 * 1024);
    std::vector<uint8_t> ct(1024 * 1024), tag(16);

    for (auto _ : state) {
        aes_gcm_encrypt(key.data(), iv.data(),
                        pt.data(), pt.size(),
                        ct.data(), tag.data());
        benchmark::DoNotOptimize(ct.data());
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(pt.size()));
    state.SetLabel("AES-256-GCM encrypt 1 MB — target ≥ 1 GB/s (AES-NI)");
}
BENCHMARK(BM_AES256GCM_Encrypt_1MB);

static void BM_AES256GCM_Decrypt_1MB(benchmark::State& state) {
    auto key = random_bytes(32);
    auto iv  = random_bytes(12);
    auto pt  = random_bytes(1024 * 1024);
    std::vector<uint8_t> ct(1024 * 1024), tag(16), out(1024 * 1024);
    aes_gcm_encrypt(key.data(), iv.data(), pt.data(), pt.size(),
                    ct.data(), tag.data());

    for (auto _ : state) {
        aes_gcm_decrypt(key.data(), iv.data(),
                        ct.data(), ct.size(),
                        tag.data(), out.data());
        benchmark::DoNotOptimize(out.data());
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(ct.size()));
    state.SetLabel("AES-256-GCM decrypt 1 MB");
}
BENCHMARK(BM_AES256GCM_Decrypt_1MB);

// ============================================================================
// Section 2: FieldEncryption (high-level API)
// ============================================================================

static void BM_FieldEncryption_SmallDocument(benchmark::State& state) {
    auto provider = std::make_shared<themis::MockKeyProvider>();
    provider->createKey("bench-doc-key", 1);
    themis::FieldEncryption enc(provider);
    const std::string key_id = "bench-doc-key";
    const std::string plaintext = random_string(256);

    auto raw_key = provider->getKey(key_id);
    auto meta    = provider->getKeyMetadata(key_id);

    for (auto _ : state) {
        auto blob = enc.encryptWithKey(plaintext, key_id, meta.version, raw_key);
        benchmark::DoNotOptimize(blob.ciphertext.data());
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(plaintext.size()));
    state.SetLabel("FieldEncryption.encrypt 256 B document");
}
BENCHMARK(BM_FieldEncryption_SmallDocument);

static void BM_FieldDecryption_SmallDocument(benchmark::State& state) {
    auto provider = std::make_shared<themis::MockKeyProvider>();
    provider->createKey("bench-doc-key", 1);
    themis::FieldEncryption enc(provider);
    const std::string key_id = "bench-doc-key";
    const std::string plaintext = random_string(256);
    auto raw_key = provider->getKey(key_id);
    auto meta    = provider->getKeyMetadata(key_id);
    auto blob    = enc.encryptWithKey(plaintext, key_id, meta.version, raw_key);

    for (auto _ : state) {
        auto recovered = enc.decryptWithKey(blob, raw_key);
        benchmark::DoNotOptimize(recovered.data());
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(plaintext.size()));
    state.SetLabel("FieldEncryption.decrypt 256 B document");
}
BENCHMARK(BM_FieldDecryption_SmallDocument);

// ============================================================================
// Section 3: RBAC policy evaluation
// ============================================================================

static void BM_RBAC_PermissionCheck_SingleRole(benchmark::State& state) {
    themis::security::RBACConfig cfg;
    cfg.use_builtin_roles      = true;
    cfg.enable_role_inheritance = true;
    themis::security::RBAC rbac(cfg);

    const std::vector<std::string> roles = {"operator"};
    for (auto _ : state) {
        bool ok = rbac.checkPermission(roles, "data", "read");
        benchmark::DoNotOptimize(ok);
    }
    state.SetLabel("RBAC single-role permission check");
}
BENCHMARK(BM_RBAC_PermissionCheck_SingleRole);

static void BM_RBAC_PermissionCheck_ManyRoles(benchmark::State& state) {
    themis::security::RBACConfig cfg;
    cfg.use_builtin_roles      = true;
    cfg.enable_role_inheritance = true;
    themis::security::RBAC rbac(cfg);

    // Add 95 synthetic roles to reach ~100 total
    for (int i = 0; i < 95; ++i) {
        themis::security::Role r;
        r.name = "synthetic_role_" + std::to_string(i);
        r.permissions.push_back({"data", "read"});
        rbac.addRole(r);
    }

    std::vector<std::string> roles;
    roles.reserve(100);
    for (int i = 0; i < 95; ++i)
        roles.push_back("synthetic_role_" + std::to_string(i));
    roles.push_back("operator");

    for (auto _ : state) {
        bool ok = rbac.checkPermission(roles, "data", "read");
        benchmark::DoNotOptimize(ok);
    }
    state.SetLabel("RBAC 100-role permission check — target p99 ≤ 0.5 ms");
}
BENCHMARK(BM_RBAC_PermissionCheck_ManyRoles);

static void BM_RBAC_RoleHierarchyValidation(benchmark::State& state) {
    themis::security::RBACConfig cfg;
    cfg.use_builtin_roles      = true;
    cfg.enable_role_inheritance = true;
    themis::security::RBAC rbac(cfg);

    for (int i = 0; i < 50; ++i) {
        themis::security::Role r;
        r.name = "hr_role_" + std::to_string(i);
        if (i > 0)
            r.inherits = {"hr_role_" + std::to_string(i - 1)};
        r.permissions.push_back({"data", "read"});
        rbac.addRole(r);
    }

    for (auto _ : state) {
        bool valid = rbac.validateRoleHierarchy();
        benchmark::DoNotOptimize(valid);
    }
    state.SetLabel("RBAC role hierarchy validation (50-level chain)");
}
BENCHMARK(BM_RBAC_RoleHierarchyValidation);

// ============================================================================
// Section 4: Post-quantum cryptography
// ============================================================================

static void BM_PostQuantum_KyberKeyGen_1024(benchmark::State& state) {
    themis::security::KyberKEM kem(themis::security::KyberKEM::SecurityLevel::KYBER_1024);
    for (auto _ : state) {
        auto kp = kem.generateKeyPair();
        benchmark::DoNotOptimize(kp.public_key.data());
    }
    state.SetLabel("Kyber-1024 key generation");
}
BENCHMARK(BM_PostQuantum_KyberKeyGen_1024);

static void BM_PostQuantum_KyberEncapsulate_1024(benchmark::State& state) {
    themis::security::KyberKEM kem(themis::security::KyberKEM::SecurityLevel::KYBER_1024);
    auto kp = kem.generateKeyPair();
    for (auto _ : state) {
        auto res = kem.encapsulate(kp.public_key);
        benchmark::DoNotOptimize(res.ciphertext.data());
    }
    state.SetLabel("Kyber-1024 encapsulate — target ≥ 2 000 ops/s");
}
BENCHMARK(BM_PostQuantum_KyberEncapsulate_1024);

static void BM_PostQuantum_KyberDecapsulate_1024(benchmark::State& state) {
    themis::security::KyberKEM kem(themis::security::KyberKEM::SecurityLevel::KYBER_1024);
    auto kp = kem.generateKeyPair();
    auto enc = kem.encapsulate(kp.public_key);
    for (auto _ : state) {
        auto ss = kem.decapsulate(enc.ciphertext, kp.secret_key);
        benchmark::DoNotOptimize(ss.data());
    }
    state.SetLabel("Kyber-1024 decapsulate");
}
BENCHMARK(BM_PostQuantum_KyberDecapsulate_1024);

static void BM_PostQuantum_DilithiumKeyGen_5(benchmark::State& state) {
    themis::security::DilithiumSigner signer(themis::security::DilithiumSigner::SecurityLevel::DILITHIUM_5);
    for (auto _ : state) {
        auto kp = signer.generateKeyPair();
        benchmark::DoNotOptimize(kp.public_key.data());
    }
    state.SetLabel("Dilithium-5 key generation");
}
BENCHMARK(BM_PostQuantum_DilithiumKeyGen_5);

static void BM_PostQuantum_DilithiumSign_5(benchmark::State& state) {
    themis::security::DilithiumSigner signer(themis::security::DilithiumSigner::SecurityLevel::DILITHIUM_5);
    auto kp = signer.generateKeyPair();
    const std::vector<uint8_t> msg(256, 0xAB);
    for (auto _ : state) {
        auto sig = signer.sign(msg, kp.secret_key);
        benchmark::DoNotOptimize(sig.data());
    }
    state.SetLabel("Dilithium-5 sign 256 B — target ≥ 1 000 ops/s");
}
BENCHMARK(BM_PostQuantum_DilithiumSign_5);

static void BM_PostQuantum_DilithiumVerify_5(benchmark::State& state) {
    themis::security::DilithiumSigner signer(themis::security::DilithiumSigner::SecurityLevel::DILITHIUM_5);
    auto kp = signer.generateKeyPair();
    const std::vector<uint8_t> msg(256, 0xAB);
    auto sig = signer.sign(msg, kp.secret_key);
    for (auto _ : state) {
        bool ok = signer.verify(msg, sig, kp.public_key);
        benchmark::DoNotOptimize(ok);
    }
    state.SetLabel("Dilithium-5 verify 256 B");
}
BENCHMARK(BM_PostQuantum_DilithiumVerify_5);

// ============================================================================
// Section 5: FIPS algorithm validation overhead
// ============================================================================

static void BM_FIPS_ValidateApprovedAlgorithm(benchmark::State& state) {
    auto& fips = themis::FipsCryptoMode::instance();
    for (auto _ : state) {
        try {
            fips.validateAlgorithm("AES-256-GCM");
        } catch (...) {}
    }
    state.SetLabel("FipsCryptoMode.validateAlgorithm (approved)");
}
BENCHMARK(BM_FIPS_ValidateApprovedAlgorithm);

static void BM_FIPS_ValidateRejectedAlgorithm(benchmark::State& state) {
    auto& fips = themis::FipsCryptoMode::instance();
    for (auto _ : state) {
        try {
            fips.validateAlgorithm("MD5");
        } catch (const themis::FipsPolicyViolation&) {}
    }
    state.SetLabel("FipsCryptoMode.validateAlgorithm (rejected — throws)");
}
BENCHMARK(BM_FIPS_ValidateRejectedAlgorithm);

// ============================================================================
// Section 6: AQL injection detection
// ============================================================================

static void BM_AQLInjection_SafeQuery(benchmark::State& state) {
    themis::security::AQLInjectionDetector detector;
    const std::string safe_query =
        "FOR u IN users FILTER u.name == 'alice' RETURN u.id";
    for (auto _ : state) {
        auto res = detector.validateAQLAST(safe_query);
        benchmark::DoNotOptimize(res.is_safe);
    }
    state.SetLabel("AQL injection detector — benign query");
}
BENCHMARK(BM_AQLInjection_SafeQuery);

static void BM_AQLInjection_MaliciousQuery(benchmark::State& state) {
    themis::security::AQLInjectionDetector detector;
    const std::string bad_query =
        "FOR u IN users FILTER u.name == 'a' OR '1'=='1' RETURN u";
    for (auto _ : state) {
        auto res = detector.validateAQLAST(bad_query);
        benchmark::DoNotOptimize(res.is_safe);
    }
    state.SetLabel("AQL injection detector — malicious query");
}
BENCHMARK(BM_AQLInjection_MaliciousQuery);

// ============================================================================
// Section 7: Audit log tamper-evident append (HashChainAuditWriter)
// ============================================================================

static void BM_AuditLog_TamperEvidentAppend(benchmark::State& state) {
    themis::utils::HashChainAuditWriterConfig cfg;
    cfg.log_path        = benchmark_temp_path("bench_security_audit.jsonl").string();
    cfg.chain_head_path = benchmark_temp_path("bench_security_audit_head.bin").string();
    cfg.fsync_on_write  = false;  // disable fsync for throughput benchmark
    std::filesystem::remove(cfg.log_path);
    std::filesystem::remove(cfg.chain_head_path);
    themis::utils::HashChainAuditWriter writer(cfg);

    nlohmann::json ev;
    ev["actor"]    = "bench_user";
    ev["action"]   = "READ";
    ev["resource"] = "collection:accounts";
    ev["outcome"]  = "SUCCESS";

    for (auto _ : state) {
        writer.write(ev);
        benchmark::DoNotOptimize(writer.sequenceNumber());
    }
    state.SetLabel("HashChainAuditWriter tamper-evident append — target p99 ≤ 2 ms");
}
BENCHMARK(BM_AuditLog_TamperEvidentAppend);

static void BM_AuditLog_BatchAppend_100(benchmark::State& state) {
    themis::utils::HashChainAuditWriterConfig cfg;
    cfg.log_path        = benchmark_temp_path("bench_security_audit_batch.jsonl").string();
    cfg.chain_head_path = benchmark_temp_path("bench_security_audit_batch_head.bin").string();
    cfg.fsync_on_write  = false;
    std::filesystem::remove(cfg.log_path);
    std::filesystem::remove(cfg.chain_head_path);
    themis::utils::HashChainAuditWriter writer(cfg);

    nlohmann::json ev;
    ev["actor"]    = "bench_user";
    ev["action"]   = "WRITE";
    ev["resource"] = "collection:transactions";

    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            writer.write(ev);
        }
        benchmark::DoNotOptimize(writer.sequenceNumber());
    }
    state.SetLabel("HashChainAuditWriter batch-append 100 entries");
}
BENCHMARK(BM_AuditLog_BatchAppend_100);

BENCHMARK_MAIN();
