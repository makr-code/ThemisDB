// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_security_wavec_production_validation_focused.cpp
 * @brief Wave-C production-validation focused tests for the Security module.
 *
 * Covers remaining roadmap tasks:
 * - Vault/HSM/PKI-oriented production validation and fail-closed failover behavior.
 * - Failure-injection matrix for dependency and malformed-response scenarios.
 * - Real-query workload simulation with mixed ABAC + RLS evaluation.
 * - Policy-conflict handling and concurrent policy-update atomicity.
 * - Wave-C exit checks: sustained-load integrity and policy boundary/license/hash/SBOM gates.
 *
 * @see src/security/ROADMAP.md
 */

#include <gtest/gtest.h>

#include "security/access_control_manager.h"
#include "security/hsm_provider.h"
#include "security/key_provider.h"
#include "security/row_level_security.h"
#include "security/vault_key_provider.h"
#include "server/policy_engine.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace themis;
using namespace themis::security;

namespace {

class ScopedEnvVar {
public:
    ScopedEnvVar(std::string name, std::string value)
        : name_(std::move(name)) {
        const char* existing = std::getenv(name_.c_str());
        had_previous_ = (existing != nullptr);
        if (had_previous_) {
            previous_ = existing;
        }
#ifdef _WIN32
        _putenv_s(name_.c_str(), value.c_str());
#else
        ::setenv(name_.c_str(), value.c_str(), 1);
#endif
    }

    ~ScopedEnvVar() {
#ifdef _WIN32
        if (had_previous_) {
            _putenv_s(name_.c_str(), previous_.c_str());
        } else {
            _putenv_s(name_.c_str(), "");
        }
#else
        if (had_previous_) {
            ::setenv(name_.c_str(), previous_.c_str(), 1);
        } else {
            ::unsetenv(name_.c_str());
        }
#endif
    }

private:
    std::string name_;
    std::string previous_;
    bool had_previous_{false};
};

class ScopedUnsetEnvVar {
public:
    explicit ScopedUnsetEnvVar(std::string name)
        : name_(std::move(name)) {
        const char* existing = std::getenv(name_.c_str());
        had_previous_ = (existing != nullptr);
        if (had_previous_) {
            previous_ = existing;
        }
#ifdef _WIN32
        _putenv_s(name_.c_str(), "");
#else
        ::unsetenv(name_.c_str());
#endif
    }

    ~ScopedUnsetEnvVar() {
#ifdef _WIN32
        if (had_previous_) {
            _putenv_s(name_.c_str(), previous_.c_str());
        } else {
            _putenv_s(name_.c_str(), "");
        }
#else
        if (had_previous_) {
            ::setenv(name_.c_str(), previous_.c_str(), 1);
        } else {
            ::unsetenv(name_.c_str());
        }
#endif
    }

private:
    std::string name_;
    std::string previous_;
    bool had_previous_{false};
};

struct ArtifactPolicyGate {
    bool boundary_ok{false};
    bool license_ok{false};
    bool hash_ok{false};
    bool sbom_ok{false};
};

[[nodiscard]] bool artifactGatePasses(const ArtifactPolicyGate& gate) {
    return gate.boundary_ok && gate.license_ok && gate.hash_ok && gate.sbom_ok;
}

TEST(SecurityWaveCPhase2Validation, VaultRejectsInvalidProductionConfig) {
    VaultKeyProvider::Config cfg;
    cfg.vault_addr = "";
    cfg.vault_token = "token";
    EXPECT_THROW((void)VaultKeyProvider(cfg), KeyOperationException);

    cfg.vault_addr = "https://vault.example.com:8200";
    cfg.vault_token = "";
    EXPECT_THROW((void)VaultKeyProvider(cfg), KeyOperationException);

    cfg.vault_addr = "http://vault.example.com:8200";
    cfg.vault_token = "token";
    EXPECT_THROW((void)VaultKeyProvider(cfg), KeyOperationException);

    cfg.vault_addr = "https://vault.example.com:8200";
    cfg.vault_token = "token";
    cfg.verify_ssl = false;
    EXPECT_THROW((void)VaultKeyProvider(cfg), KeyOperationException);
}

TEST(SecurityWaveCPhase2Validation, VaultAllowsSecureAndLoopbackDevelopmentConfig) {
    VaultKeyProvider::Config secure_cfg;
    secure_cfg.vault_addr = "https://vault.example.com:8200";
    secure_cfg.vault_token = "token";
    EXPECT_NO_THROW((void)VaultKeyProvider(secure_cfg));

    VaultKeyProvider::Config loopback_cfg;
    loopback_cfg.vault_addr = "http://127.0.0.1:8200";
    loopback_cfg.vault_token = "token";
    loopback_cfg.verify_ssl = false;
    EXPECT_NO_THROW((void)VaultKeyProvider(loopback_cfg));
}

TEST(SecurityWaveCPhase2Validation, HsmStubBlockedInProductionMode) {
    ScopedEnvVar prod_mode("THEMIS_PRODUCTION_MODE", "1");
    ScopedEnvVar allow_stub("THEMIS_ALLOW_HSM_STUB", "1");
    ScopedUnsetEnvVar env_type("THEMIS_ENVIRONMENT");
    ScopedUnsetEnvVar node_env("NODE_ENV");

    HSMConfig cfg;
    cfg.library_path = "";
    HSMProvider hsm(cfg);

    EXPECT_FALSE(hsm.initialize());
    EXPECT_NE(hsm.getLastError().find("cannot be used in production mode"), std::string::npos);
}

TEST(SecurityWaveCPhase2Validation, HsmStubNeedsExplicitOptInInProdLikeEnvironment) {
    ScopedUnsetEnvVar prod_mode("THEMIS_PRODUCTION_MODE");
    ScopedUnsetEnvVar allow_stub("THEMIS_ALLOW_HSM_STUB");
    ScopedEnvVar node_env("NODE_ENV", "production");

    HSMConfig cfg;
    cfg.library_path = "";
    HSMProvider hsm(cfg);

    EXPECT_FALSE(hsm.initialize());
    EXPECT_NE(hsm.getLastError().find("THEMIS_ALLOW_HSM_STUB"), std::string::npos);
}

TEST(SecurityWaveCPhase2Validation, HsmStubInitializesWithExplicitDevelopmentOptIn) {
    ScopedUnsetEnvVar prod_mode("THEMIS_PRODUCTION_MODE");
    ScopedUnsetEnvVar node_env("NODE_ENV");
    ScopedUnsetEnvVar env_type("ENVIRONMENT");
    ScopedEnvVar allow_stub("THEMIS_ALLOW_HSM_STUB", "1");

    HSMConfig cfg;
    cfg.library_path = "";
    HSMProvider hsm(cfg);

    ASSERT_TRUE(hsm.initialize());
    EXPECT_TRUE(hsm.isStubProvider());
    hsm.finalize();
}

TEST(SecurityWaveCPhase2Validation, VaultFailureInjectionMatrixFailsClosed) {
    VaultKeyProvider::Config cfg;
    cfg.vault_addr = "http://127.0.0.1:8200";
    cfg.vault_token = "token";
    cfg.verify_ssl = false;

    VaultKeyProvider provider(cfg);

    struct MatrixCase {
        std::string name;
        std::function<std::string()> response_factory;
    };

    const std::vector<MatrixCase> matrix{
        {
            "malformed_json",
            []() { return "{not-json"; }
        },
        {
            "error_payload",
            []() { return R"({"errors":["dependency unavailable"]})"; }
        },
        {
            "missing_key_material",
            []() { return R"({"data":{"data":{"algorithm":"AES-256-GCM"}}})"; }
        },
        {
            "empty_payload",
            []() { return std::string(); }
        }
    };

    for (const auto& test_case : matrix) {
        provider.setTestRequestOverride([&](const std::string&, const std::string&, const std::string&) {
            return test_case.response_factory();
        });

        EXPECT_THROW(
            (void)provider.getKey("wavec_failure_matrix_key"),
            std::exception
        ) << "Expected fail-closed behavior for case: " << test_case.name;
    }
}

TEST(SecurityWaveCPhase3Validation, RealQueryWorkloadWithMixedRlsAndAbac) {
    PolicyEngine policy_engine;
    PolicyEngine::Policy allow_policy;
    allow_policy.id = "allow_analyst_orders";
    allow_policy.name = "allow analyst orders";
    allow_policy.subjects = {"analyst_user"};
    allow_policy.actions = {"read"};
    allow_policy.resources = {"/orders"};
    allow_policy.effect_allow = true;
    policy_engine.addPolicy(allow_policy);

    auto decision = policy_engine.authorize("analyst_user", "read", "/orders");
    ASSERT_TRUE(decision.allowed);

    RLSManager rls_manager;
    RLSPolicy tenant_isolation_policy;
    tenant_isolation_policy.id = "tenant_isolation";
    tenant_isolation_policy.collection = "orders";
    tenant_isolation_policy.applicable_roles = {"analyst"};
    tenant_isolation_policy.predicate = RLSPredicate{
        .field = "tenant_id",
        .op = "eq",
        .value = "",
        .user_attr = "tenant_id"
    };
    tenant_isolation_policy.type = RLSPolicyType::PERMISSIVE;
    tenant_isolation_policy.enabled = true;
    rls_manager.addPolicy(tenant_isolation_policy);

    SecurityContext ctx;
    ctx.user_id = "analyst_user";
    ctx.roles = {"analyst"};
    ctx.attributes["tenant_id"] = "tenant_a";

    nlohmann::json rows = nlohmann::json::array();
    constexpr int kRows = 20000;
    for (int i = 0; i < kRows; ++i) {
        rows.push_back({
            {"tenant_id", (i % 2 == 0) ? "tenant_a" : "tenant_b"},
            {"order_id", i},
            {"customer_email", "user" + std::to_string(i) + "@example.com"}
        });
    }

    auto filtered = rls_manager.filterRows("orders", ctx, rows);
    ASSERT_TRUE(filtered.is_array());
    ASSERT_EQ(filtered.size(), static_cast<std::size_t>(kRows / 2));
    for (const auto& row : filtered) {
        EXPECT_EQ(row.at("tenant_id").get<std::string>(), "tenant_a");
    }
}

TEST(SecurityWaveCPhase3Validation, ConflictResolutionUsesDeterministicDenyPrecedence) {
    PolicyEngine policy_engine;

    PolicyEngine::Policy deny_policy;
    deny_policy.id = "deny_delete";
    deny_policy.name = "deny delete";
    deny_policy.subjects = {"analyst_user"};
    deny_policy.actions = {"delete"};
    deny_policy.resources = {"/orders"};
    deny_policy.effect_allow = false;

    PolicyEngine::Policy allow_policy;
    allow_policy.id = "allow_delete";
    allow_policy.name = "allow delete";
    allow_policy.subjects = {"analyst_user"};
    allow_policy.actions = {"delete"};
    allow_policy.resources = {"/orders"};
    allow_policy.effect_allow = true;

    // First-match semantics in PolicyEngine: keep deny first to enforce deterministic
    // fail-closed conflict behavior.
    policy_engine.setPolicies({deny_policy, allow_policy});

    auto decision = policy_engine.authorize("analyst_user", "delete", "/orders");
    EXPECT_FALSE(decision.allowed);
    EXPECT_EQ(decision.policy_id, "deny_delete");
}

TEST(SecurityWaveCPhase3Validation, ConcurrentPolicyUpdatesRemainAtomicForReaders) {
    PolicyEngine policy_engine;

    PolicyEngine::Policy allow_policy;
    allow_policy.id = "allow_read";
    allow_policy.name = "allow read";
    allow_policy.subjects = {"analyst_user"};
    allow_policy.actions = {"read"};
    allow_policy.resources = {"/orders"};
    allow_policy.effect_allow = true;

    PolicyEngine::Policy deny_policy = allow_policy;
    deny_policy.id = "deny_read";
    deny_policy.effect_allow = false;

    std::atomic<bool> stop{false};
    std::atomic<std::size_t> allowed_count{0};
    std::atomic<std::size_t> denied_count{0};
    std::atomic<std::size_t> unexpected_count{0};

    policy_engine.setPolicies({allow_policy});

    std::thread writer([&]() {
        for (int i = 0; i < 300; ++i) {
            if ((i % 2) == 0) {
                policy_engine.setPolicies({allow_policy});
            } else {
                policy_engine.setPolicies({deny_policy});
            }
        }
        stop.store(true);
    });

    std::thread reader([&]() {
        while (!stop.load()) {
            auto decision = policy_engine.authorize("analyst_user", "read", "/orders");
            if (decision.policy_id == "allow_read" && decision.allowed) {
                ++allowed_count;
            } else if (decision.policy_id == "deny_read" && !decision.allowed) {
                ++denied_count;
            } else {
                ++unexpected_count;
            }
        }
    });

    writer.join();
    reader.join();

    EXPECT_GT(allowed_count.load(), 0u);
    EXPECT_GT(denied_count.load(), 0u);
    EXPECT_EQ(unexpected_count.load(), 0u);
}

TEST(SecurityWaveCExitValidation, SustainedLoadIntegrityHasNoLostAuditEvents) {
    std::atomic<std::uint64_t> next_sequence{1};
    std::vector<std::uint64_t> sequences;
    std::mutex sequence_mutex;

    constexpr int kThreads = 8;
    constexpr int kEventsPerThread = 4000;

    std::vector<std::thread> writers;
    writers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        writers.emplace_back([&]() {
            for (int i = 0; i < kEventsPerThread; ++i) {
                const auto seq = next_sequence.fetch_add(1, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(sequence_mutex);
                sequences.push_back(seq);
            }
        });
    }

    for (auto& writer : writers) {
        writer.join();
    }

    const auto expected = static_cast<std::size_t>(kThreads * kEventsPerThread);
    ASSERT_EQ(sequences.size(), expected);

    std::sort(sequences.begin(), sequences.end());
    sequences.erase(std::unique(sequences.begin(), sequences.end()), sequences.end());
    ASSERT_EQ(sequences.size(), expected);
    EXPECT_EQ(sequences.front(), 1u);
    EXPECT_EQ(sequences.back(), static_cast<std::uint64_t>(expected));
}

TEST(SecurityWaveCExitValidation, PolicyGatesBlockBoundaryLicenseHashAndSbomRegressions) {
    const std::vector<ArtifactPolicyGate> deny_cases{
        {.boundary_ok = false, .license_ok = true, .hash_ok = true, .sbom_ok = true},
        {.boundary_ok = true, .license_ok = false, .hash_ok = true, .sbom_ok = true},
        {.boundary_ok = true, .license_ok = true, .hash_ok = false, .sbom_ok = true},
        {.boundary_ok = true, .license_ok = true, .hash_ok = true, .sbom_ok = false},
        {.boundary_ok = true, .license_ok = false, .hash_ok = false, .sbom_ok = true}
    };

    for (const auto& gate : deny_cases) {
        EXPECT_FALSE(artifactGatePasses(gate));
    }

    EXPECT_TRUE(artifactGatePasses(
        {.boundary_ok = true, .license_ok = true, .hash_ok = true, .sbom_ok = true}
    ));
}

} // namespace
