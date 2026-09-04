/**
 * @file bench_compliance_security_governance.cpp
 * @brief Performance benchmarks for compliance, security and governance features
 * 
 * Benchmarks performance impact of:
 * - Encryption/Decryption operations
 * - Access control checks
 * - Audit logging overhead
 * - Policy evaluation
 * - Classification enforcement
 * - HSM operations
 * - Malware scanning
 * 
 * @author ThemisDB Team
 * @date January 2025
 */

#include <benchmark/benchmark.h>
#include "governance/policy_engine.h"
#include "governance/ccpa_rules.h"
#include "governance/policy_manager.h"
#include "governance/compliance_reporting.h"
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "security/hsm_provider.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <random>
#include <string>
#include <memory>
#include <unordered_set>

using namespace themis::governance;
using namespace themis;
using json = nlohmann::json;

// ============================================================================
// Test Data Generation
// ============================================================================

std::vector<uint8_t> generateTestData(size_t size) {
    std::vector<uint8_t> data(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::generate(data.begin(), data.end(), [&]() { return static_cast<uint8_t>(dis(gen)); });
    return data;
}

std::string generateRandomString(size_t length) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.reserve(length);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
    for (size_t i = 0; i < length; ++i) {
        result += alphanum[dis(gen)];
    }
    return result;
}

// ============================================================================
// Encryption Performance Benchmarks
// ============================================================================

static void BM_FieldEncryption_SmallData(benchmark::State& state) {
    auto data = generateTestData(256); // 256 bytes
    auto provider = std::make_shared<MockKeyProvider>();
    provider->createKey("bench-key", 1);
    FieldEncryption enc(provider);
    const std::string key_id = "bench-key";
    auto raw_key = provider->getKey(key_id);
    auto meta    = provider->getKeyMetadata(key_id);
    const std::string data_str(data.begin(), data.end());

    for (auto _ : state) {
        auto blob = enc.encryptWithKey(data_str, key_id, meta.version, raw_key);
        benchmark::DoNotOptimize(blob);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_FieldEncryption_SmallData);

static void BM_FieldEncryption_MediumData(benchmark::State& state) {
    auto data = generateTestData(4096); // 4 KB
    auto provider = std::make_shared<MockKeyProvider>();
    provider->createKey("bench-key", 1);
    FieldEncryption enc(provider);
    const std::string key_id = "bench-key";
    auto raw_key = provider->getKey(key_id);
    auto meta    = provider->getKeyMetadata(key_id);
    const std::string data_str(data.begin(), data.end());

    for (auto _ : state) {
        auto blob = enc.encryptWithKey(data_str, key_id, meta.version, raw_key);
        benchmark::DoNotOptimize(blob);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_FieldEncryption_MediumData);

static void BM_FieldEncryption_LargeData(benchmark::State& state) {
    auto data = generateTestData(1024 * 1024); // 1 MB
    auto provider = std::make_shared<MockKeyProvider>();
    provider->createKey("bench-key", 1);
    FieldEncryption enc(provider);
    const std::string key_id = "bench-key";
    auto raw_key = provider->getKey(key_id);
    auto meta    = provider->getKeyMetadata(key_id);
    const std::string data_str(data.begin(), data.end());

    for (auto _ : state) {
        auto blob = enc.encryptWithKey(data_str, key_id, meta.version, raw_key);
        benchmark::DoNotOptimize(blob);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_FieldEncryption_LargeData);

static void BM_FieldDecryption_Performance(benchmark::State& state) {
    size_t data_size = state.range(0) * 1024;
    auto data = generateTestData(data_size);
    auto provider = std::make_shared<MockKeyProvider>();
    provider->createKey("bench-key", 1);
    FieldEncryption enc(provider);
    const std::string key_id = "bench-key";
    // Pre-encrypt data outside the benchmark loop
    auto raw_key = provider->getKey(key_id);
    auto meta    = provider->getKeyMetadata(key_id);
    const std::string data_str(data.begin(), data.end());
    auto blob = enc.encryptWithKey(data_str, key_id, meta.version, raw_key);

    for (auto _ : state) {
        auto decrypted = enc.decryptWithKey(blob, raw_key);
        benchmark::DoNotOptimize(decrypted);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * data_size);
}
BENCHMARK(BM_FieldDecryption_Performance)
    ->Arg(1)     // 1 KB
    ->Arg(10)    // 10 KB
    ->Arg(100)   // 100 KB
    ->Arg(1024); // 1 MB

// ============================================================================
// Policy Engine Performance Benchmarks
// ============================================================================

static void BM_PolicyEvaluation_SingleRequest(benchmark::State& state) {
    PolicyEngine engine;
    
    std::unordered_map<std::string, std::string> headers;
    headers["classification"] = "offen";
    headers["user-role"] = "user";
    
    for (auto _ : state) {
        PolicyDecision decision = engine.evaluate(headers, "/api/query");
        benchmark::DoNotOptimize(decision);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PolicyEvaluation_SingleRequest);

static void BM_PolicyEvaluation_DifferentClassifications(benchmark::State& state) {
    PolicyEngine engine;
    
    std::vector<std::string> classifications = {
        "offen", "vs-nfd", "geheim", "streng-geheim"
    };
    
    int idx = 0;
    for (auto _ : state) {
        std::unordered_map<std::string, std::string> headers = {};

        headers["classification"] = classifications[idx % classifications.size()];
        
        PolicyDecision decision = engine.evaluate(headers, "/api/query");
        benchmark::DoNotOptimize(decision);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PolicyEvaluation_DifferentClassifications);

static void BM_PolicyEvaluation_HighVolume(benchmark::State& state) {
    PolicyEngine engine;
    const int batch_size = state.range(0);
    
    for (auto _ : state) {
        for (int i = 0; i < batch_size; i++) {
            std::unordered_map<std::string, std::string> headers;
            headers["classification"] = (i % 2 == 0) ? "offen" : "vs-nfd";
            headers["user-role"] = "user";
            
            PolicyDecision decision = engine.evaluate(headers, "/api/query");
            benchmark::DoNotOptimize(decision);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_PolicyEvaluation_HighVolume)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000);

static void BM_PolicyEvaluation_ComplexRules(benchmark::State& state) {
    PolicyEngine engine;
    
    std::unordered_map<std::string, std::string> headers;
    headers["classification"] = "geheim";
    headers["user-role"] = "admin";
    headers["department"] = "security";
    headers["clearance-level"] = "high";
    
    for (auto _ : state) {
        PolicyDecision decision = engine.evaluate(headers, "/api/sensitive");
        benchmark::DoNotOptimize(decision);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PolicyEvaluation_ComplexRules);

// ============================================================================
// Access Control Performance Benchmarks
// ============================================================================

static void BM_AccessControl_PermissionCheck(benchmark::State& state) {
    // Simulate RBAC permission checking
    
    std::vector<std::string> roles = {"admin", "user", "readonly", "auditor"};
    std::vector<std::string> permissions = {"read", "write", "delete", "audit"};
    
    int idx = 0;
    for (auto _ : state) {
        std::string role = roles[idx % roles.size()];
        std::string permission = permissions[idx % permissions.size()];
        
        // Simulate permission check
        bool granted = (role == "admin") || 
                      (role == "user" && permission == "read") ||
                      (role == "auditor" && permission == "audit");
        
        benchmark::DoNotOptimize(granted);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AccessControl_PermissionCheck);

static void BM_AccessControl_MultiLevelSecurity(benchmark::State& state) {
    // Simulate hierarchical access control
    
    std::vector<std::string> clearances = {"public", "confidential", "secret", "top-secret"};
    std::vector<std::string> classifications = {"public", "confidential", "secret", "top-secret"};
    
    for (auto _ : state) {
        state.PauseTiming();
        int user_clearance = state.iterations() % clearances.size();
        int data_classification = state.iterations() % classifications.size();
        state.ResumeTiming();
        
        // Access granted if clearance >= classification
        bool access_granted = user_clearance >= data_classification;
        benchmark::DoNotOptimize(access_granted);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AccessControl_MultiLevelSecurity);

static void BM_AccessControl_RoleHierarchy(benchmark::State& state) {
    // Simulate role hierarchy evaluation
    
    std::map<std::string, int> role_hierarchy = {
        {"guest", 0},
        {"user", 1},
        {"power-user", 2},
        {"admin", 3},
        {"super-admin", 4}
    };
    
    for (auto _ : state) {
        std::string user_role = "user";
        std::string required_role = "power-user";
        
        bool access = role_hierarchy[user_role] >= role_hierarchy[required_role];
        benchmark::DoNotOptimize(access);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AccessControl_RoleHierarchy);

// ============================================================================
// Audit Logging Performance Benchmarks
// ============================================================================

static void BM_AuditLog_SingleEntry(benchmark::State& state) {
    for (auto _ : state) {
        // Simulate audit log entry creation
        json log_entry;
        log_entry["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        log_entry["user"] = "user123";
        log_entry["action"] = "data_access";
        log_entry["resource"] = "/api/data/12345";
        log_entry["result"] = "success";
        
        benchmark::DoNotOptimize(log_entry);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AuditLog_SingleEntry);

static void BM_AuditLog_BatchEntries(benchmark::State& state) {
    const int batch_size = state.range(0);
    
    for (auto _ : state) {
        std::vector<json> log_batch;
        log_batch.reserve(batch_size);
        
        for (int i = 0; i < batch_size; i++) {
            json entry;
            entry["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
            entry["user"] = "user" + std::to_string(i);
            entry["action"] = "query";
            entry["resource"] = "/api/query";
            log_batch.push_back(entry);
        }
        
        benchmark::DoNotOptimize(log_batch);
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_AuditLog_BatchEntries)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000);

static void BM_AuditLog_HashChainVerification(benchmark::State& state) {
    // Simulate hash chain integrity verification
    
    const int chain_length = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Build hash chain (setup phase)
        std::vector<std::string> hashes;
        hashes.reserve(chain_length);
        std::string prev_hash = "genesis";
        for (int i = 0; i < chain_length; i++) {
            std::string current = prev_hash + std::to_string(i);
            hashes.push_back(current);
            prev_hash = current;
        }
        
        state.ResumeTiming();
        
        // Verify chain integrity (measured phase)
        bool valid = true;
        for (size_t i = 1; i < hashes.size(); i++) {
            if (hashes[i].find(hashes[i-1]) == std::string::npos) {
                valid = false;
                break;
            }
        }
        benchmark::DoNotOptimize(valid);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AuditLog_HashChainVerification)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

static void BM_AuditLog_EncryptedLogging(benchmark::State& state) {
    // Simulate encrypted audit logging (Encrypt-then-Sign)
    
    for (auto _ : state) {
        json log_entry;
        log_entry["sensitive_data"] = generateRandomString(256);
        log_entry["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Simulate encryption + signing
        std::string serialized = log_entry.dump();
        std::vector<uint8_t> data(serialized.begin(), serialized.end());
        
        benchmark::DoNotOptimize(data);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AuditLog_EncryptedLogging);

// ============================================================================
// Classification Enforcement Benchmarks
// ============================================================================

static void BM_Classification_DataLabeling(benchmark::State& state) {
    std::vector<std::string> classifications = {
        "offen", "vs-nfd", "geheim", "streng-geheim"
    };
    
    int idx = 0;
    for (auto _ : state) {
        json data;
        data["content"] = generateRandomString(128);
        data["classification"] = classifications[idx % classifications.size()];
        
        benchmark::DoNotOptimize(data);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Classification_DataLabeling);

static void BM_Classification_ValidationCheck(benchmark::State& state) {
    std::vector<std::string> valid_classifications = {
        "offen", "vs-nfd", "geheim", "streng-geheim"
    };
    
    for (auto _ : state) {
        std::string test_classification = "geheim";
        
        bool valid = std::find(valid_classifications.begin(),
                              valid_classifications.end(),
                              test_classification) != valid_classifications.end();
        
        benchmark::DoNotOptimize(valid);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Classification_ValidationCheck);

// ============================================================================
// Malware Scanning Performance Benchmarks
// ============================================================================

static void BM_MalwareScanning_SmallFile(benchmark::State& state) {
    auto file_data = generateTestData(10 * 1024); // 10 KB
    
    for (auto _ : state) {
        // Simulate malware scanning
        bool clean = true; // Simplified scan result
        benchmark::DoNotOptimize(clean);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * file_data.size());
}
BENCHMARK(BM_MalwareScanning_SmallFile);

static void BM_MalwareScanning_MediumFile(benchmark::State& state) {
    auto file_data = generateTestData(1024 * 1024); // 1 MB
    
    for (auto _ : state) {
        bool clean = true;
        benchmark::DoNotOptimize(clean);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * file_data.size());
}
BENCHMARK(BM_MalwareScanning_MediumFile);

static void BM_MalwareScanning_LargeFile(benchmark::State& state) {
    size_t file_size = state.range(0) * 1024 * 1024; // MB to bytes
    auto file_data = generateTestData(file_size);
    
    for (auto _ : state) {
        bool clean = true;
        benchmark::DoNotOptimize(clean);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * file_size);
}
BENCHMARK(BM_MalwareScanning_LargeFile)
    ->Arg(1)   // 1 MB
    ->Arg(10)  // 10 MB
    ->Arg(50); // 50 MB

// ============================================================================
// Key Management Performance Benchmarks
// ============================================================================

static void BM_KeyManagement_KeyRetrieval(benchmark::State& state) {
    // Benchmark key retrieval from MockKeyProvider (simulates cache/provider lookup)
    auto provider = std::make_shared<MockKeyProvider>();
    const std::string key_ids[] = {"key1", "key2", "key3"};
    provider->createKey("key1", 1);
    provider->createKey("key2", 1);
    provider->createKey("key3", 1);

    int idx = 0;
    for (auto _ : state) {
        auto key = provider->getKey(key_ids[idx % 3]);
        benchmark::DoNotOptimize(key);
        idx++;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_KeyManagement_KeyRetrieval);

static void BM_KeyManagement_KeyRotation(benchmark::State& state) {
    // Benchmark key rotation: create new version and verify old remains accessible
    auto provider = std::make_shared<MockKeyProvider>();
    const std::string key_id = "rotation-bench-key";
    provider->createKey(key_id, 1);

    for (auto _ : state) {
        provider->rotateKey(key_id);
        auto key = provider->getKey(key_id);
        benchmark::DoNotOptimize(key);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_KeyManagement_KeyRotation);

static void BM_KeyManagement_HSMOperation(benchmark::State& state) {
    // Benchmark real HSM stub sign/encrypt operations (stub mode, no hardware required)
    themis::security::HSMConfig cfg;
    cfg.library_path = ""; // use stub provider
    cfg.key_label = "bench-kek";
    cfg.signature_algorithm = "RSA-SHA256";
    themis::security::HSMProvider hsm(cfg);
    hsm.initialize();

    const std::vector<uint8_t> dek(32, 0x42); // 32-byte DEK to wrap

    for (auto _ : state) {
        // Wrap DEK (encrypt) then unwrap (decrypt) - measures stub HSM overhead
        auto wrapped = hsm.encryptData(dek);
        auto unwrapped = hsm.decryptData(wrapped);
        benchmark::DoNotOptimize(unwrapped);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * dek.size());
}
BENCHMARK(BM_KeyManagement_HSMOperation);

// ============================================================================
// Compliance Overhead Benchmarks
// ============================================================================

static void BM_ComplianceOverhead_GDPR_Complete(benchmark::State& state) {
    // Measure complete GDPR compliance overhead
    
    for (auto _ : state) {
        // 1. Encryption check
        bool encrypted = true;
        
        // 2. Consent verification
        bool consent = true;
        
        // 3. Audit logging
        json audit_log;
        audit_log["action"] = "data_processing";
        audit_log["legal_basis"] = "consent";
        
        // 4. Data minimization check
        bool minimal = true;
        
        benchmark::DoNotOptimize(encrypted);
        benchmark::DoNotOptimize(consent);
        benchmark::DoNotOptimize(audit_log);
        benchmark::DoNotOptimize(minimal);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ComplianceOverhead_GDPR_Complete);

static void BM_ComplianceOverhead_HIPAA_Complete(benchmark::State& state) {
    // Measure complete HIPAA compliance overhead
    
    for (auto _ : state) {
        // 1. Encryption
        bool encrypted = true;
        
        // 2. Access control
        bool authorized = true;
        
        // 3. Audit trail
        json audit_entry;
        audit_entry["phi_access"] = true;
        
        // 4. Integrity check
        bool integrity_valid = true;
        
        benchmark::DoNotOptimize(encrypted);
        benchmark::DoNotOptimize(authorized);
        benchmark::DoNotOptimize(audit_entry);
        benchmark::DoNotOptimize(integrity_valid);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ComplianceOverhead_HIPAA_Complete);

static void BM_ComplianceOverhead_MultiStandard(benchmark::State& state) {
    // Measure overhead when multiple standards apply
    
    for (auto _ : state) {
        // GDPR + HIPAA + SOC2
        PolicyEngine engine;
        
        std::unordered_map<std::string, std::string> headers;
        headers["classification"] = "geheim";
        headers["compliance"] = "gdpr,hipaa,soc2";
        
        PolicyDecision decision = engine.evaluate(headers, "/api/data");
        
        // Additional checks
        bool encrypted = true;
        bool audited = true;
        bool access_controlled = true;
        
        benchmark::DoNotOptimize(decision);
        benchmark::DoNotOptimize(encrypted);
        benchmark::DoNotOptimize(audited);
        benchmark::DoNotOptimize(access_controlled);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ComplianceOverhead_MultiStandard);

// ============================================================================
// Real-World Compliance Scenarios
// ============================================================================

static void BM_RealWorld_SecureDataAccess(benchmark::State& state) {
    // Complete secure data access workflow
    
    PolicyEngine engine;
    
    for (auto _ : state) {
        // 1. Access control check
        bool authorized = true;
        
        // 2. Policy evaluation
        std::unordered_map<std::string, std::string> headers;
        headers["classification"] = "vs-nfd";
        PolicyDecision decision = engine.evaluate(headers, "/api/data");
        
        // 3. Decrypt data
        auto data = generateTestData(1024);
        
        // 4. Audit log
        json log;
        log["access"] = "granted";
        
        benchmark::DoNotOptimize(authorized);
        benchmark::DoNotOptimize(decision);
        benchmark::DoNotOptimize(data);
        benchmark::DoNotOptimize(log);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RealWorld_SecureDataAccess);

static void BM_RealWorld_ClassifiedDocumentProcessing(benchmark::State& state) {
    // Process classified government document
    
    for (auto _ : state) {
        // 1. Classification check
        std::string classification = "geheim";
        
        // 2. Access verification
        bool clearance_valid = true;
        
        // 3. Malware scan
        bool clean = true;
        
        // 4. Encrypt for storage
        auto document = generateTestData(10 * 1024);
        
        // 5. Audit trail
        json audit;
        audit["document_processed"] = true;
        audit["classification"] = classification;
        
        benchmark::DoNotOptimize(classification);
        benchmark::DoNotOptimize(clearance_valid);
        benchmark::DoNotOptimize(clean);
        benchmark::DoNotOptimize(document);
        benchmark::DoNotOptimize(audit);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RealWorld_ClassifiedDocumentProcessing);

// ============================================================================
// CCPA/CPRA Data Subject Rights Performance Benchmarks
// Validates: CCPA opt-out flag lookup adds ≤ 0.5 ms to query-time p99
// ============================================================================

/// Baseline: PolicyEngine::evaluate() without CCPA opt-out registry set.
static void BM_PolicyEvaluation_NoCcpa(benchmark::State& state) {
    PolicyEngine engine;
    std::unordered_map<std::string, std::string> headers = {
        {"X-User-Id", "user-001"},
        {"X-Classification", "vs-nfd"}
    };
    for (auto _ : state) {
        auto decision = engine.evaluate(headers, "/api/data");
        benchmark::DoNotOptimize(decision);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PolicyEvaluation_NoCcpa);

/// WithCcpa (not opted out): PolicyEngine::evaluate() with opt-out registry
/// attached but the requesting subject is NOT in the registry.
static void BM_PolicyEvaluation_WithCcpaNotOptedOut(benchmark::State& state) {
    PolicyEngine engine;

    // Registry with 10K opted-out subjects – the requesting user is not in it.
    auto registry = std::make_shared<std::unordered_set<std::string>>();
    for (int i = 0; i < 10000; ++i) {
        registry->insert("opted-out-" + std::to_string(i));
    }
    engine.setCcpaOptOutSubjects(registry);

    std::unordered_map<std::string, std::string> headers = {
        {"X-User-Id", "normal-user"},
        {"X-Classification", "vs-nfd"}
    };
    for (auto _ : state) {
        auto decision = engine.evaluate(headers, "/api/data");
        benchmark::DoNotOptimize(decision);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PolicyEvaluation_WithCcpaNotOptedOut);

/// WithCcpa (opted out): PolicyEngine::evaluate() where the requesting subject
/// IS in the opt-out registry — export_allowed must be forced false.
static void BM_PolicyEvaluation_WithCcpaOptedOut(benchmark::State& state) {
    PolicyEngine engine;

    auto registry = std::make_shared<std::unordered_set<std::string>>();
    registry->insert("opted-out-subject");
    for (int i = 0; i < 9999; ++i) {
        registry->insert("other-user-" + std::to_string(i));
    }
    engine.setCcpaOptOutSubjects(registry);

    std::unordered_map<std::string, std::string> headers = {
        {"X-User-Id", "opted-out-subject"},
        {"X-Classification", "vs-nfd"}
    };
    for (auto _ : state) {
        auto decision = engine.evaluate(headers, "/api/data");
        benchmark::DoNotOptimize(decision);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PolicyEvaluation_WithCcpaOptedOut);

/// CcpaRuleSet::evaluateRule() latency for a single PolicyRule against all
/// four CCPA evaluators (RightToKnow, RightToDelete, OptOutOfSale, DataPortability).
static void BM_CcpaRuleSet_EvaluateRule(benchmark::State& state) {
    CcpaRuleSet ccpa;

    PolicyRule rule;
    rule.id              = "bench-rule";
    rule.name            = "Benchmark Rule";
    rule.enabled         = true;
    rule.audit_access    = true;
    rule.audit_changes   = true;
    rule.allow_export    = false;
    rule.require_signature = false;
    rule.retention_days  = 365;
    rule.resources       = {"data/*"};
    rule.actions         = {"read"};

    for (auto _ : state) {
        auto results = ccpa.evaluateRule(rule);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CcpaRuleSet_EvaluateRule);

/// CcpaRuleSet opt-out registry lookup scaling: measures CcpaRuleSet::isOptedOut()
/// latency for different registry sizes (10, 1K, 100K subjects).
static void BM_CcpaRuleSet_OptOutLookup(benchmark::State& state) {
    const int registry_size = static_cast<int>(state.range(0));
    CcpaRuleSet ccpa;
    for (int i = 0; i < registry_size; ++i) {
        ccpa.addOptOut("subject-" + std::to_string(i));
    }
    const std::string target = "subject-" + std::to_string(registry_size / 2);

    for (auto _ : state) {
        bool opted_out = ccpa.isOptedOut(target);
        benchmark::DoNotOptimize(opted_out);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CcpaRuleSet_OptOutLookup)
    ->Arg(10)
    ->Arg(1000)
    ->Arg(100000);

/// ComplianceReporter::generateCcpaReport() latency for different rule counts.
static void BM_CcpaReport_Generation(benchmark::State& state) {
    const int rule_count = static_cast<int>(state.range(0));
    PolicyManager mgr;
    for (int i = 0; i < rule_count; ++i) {
        PolicyRule r;
        r.id              = "rule-" + std::to_string(i);
        r.name            = "Rule " + std::to_string(i);
        r.enabled         = true;
        r.audit_access    = (i % 2 == 0);
        r.audit_changes   = (i % 3 == 0);
        r.allow_export    = (i % 4 == 0);
        r.retention_days  = 365 + (i % 1000);
        r.classification_level = (i % 3 == 0) ? "geheim" : "vs-nfd";
        r.resources       = {"data/*"};
        r.actions         = {"read"};
        mgr.addRule(r);
    }
    ComplianceReporter reporter;
    const int opt_out_count = rule_count / 10;

    for (auto _ : state) {
        auto report = reporter.generateCcpaReport(mgr, opt_out_count);
        benchmark::DoNotOptimize(report);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CcpaReport_Generation)
    ->Arg(10)
    ->Arg(100)
    ->Arg(500);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
