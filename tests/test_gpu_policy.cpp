#include <gtest/gtest.h>
#include "themis/gpu/policy.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Default-deny
// ---------------------------------------------------------------------------

TEST(GPUPolicyTest, DefaultDeny_UnknownCaller_Rejected) {
    GPUPolicy pol;
    EXPECT_FALSE(pol.isAllowed("alice", GPUPolicy::Capability::GPU_ALLOCATE));
    EXPECT_FALSE(pol.isAllowed("alice", GPUPolicy::Capability::GPU_FREE));
    EXPECT_FALSE(pol.isAllowed("alice", GPUPolicy::Capability::GPU_ADMIN));
}

TEST(GPUPolicyTest, DefaultDeny_CheckReturnsReason) {
    GPUPolicy pol;
    auto d = pol.check("bob", GPUPolicy::Capability::GPU_ALLOCATE);
    EXPECT_FALSE(d.allowed);
    EXPECT_FALSE(d.reason.empty());
    EXPECT_NE(d.reason.find("default-deny"), std::string::npos);
    EXPECT_EQ(d.caller_id, "bob");
}

TEST(GPUPolicyTest, GrantedCount_ZeroOnConstruction) {
    GPUPolicy pol;
    EXPECT_EQ(pol.grantedCount(), 0u);
}

// ---------------------------------------------------------------------------
// Grant
// ---------------------------------------------------------------------------

TEST(GPUPolicyTest, Grant_SingleCapability_AllowsIt) {
    GPUPolicy pol;
    pol.grant("alice", GPUPolicy::Capability::GPU_ALLOCATE);
    EXPECT_TRUE(pol.isAllowed("alice", GPUPolicy::Capability::GPU_ALLOCATE));
    EXPECT_FALSE(pol.isAllowed("alice", GPUPolicy::Capability::GPU_ADMIN));
}

TEST(GPUPolicyTest, Grant_GPUAny_AllowsAllCapabilities) {
    GPUPolicy pol;
    pol.grant("svc", GPUPolicy::Capability::GPU_ANY);
    EXPECT_TRUE(pol.isAllowed("svc", GPUPolicy::Capability::GPU_ALLOCATE));
    EXPECT_TRUE(pol.isAllowed("svc", GPUPolicy::Capability::GPU_FREE));
    EXPECT_TRUE(pol.isAllowed("svc", GPUPolicy::Capability::GPU_ADMIN));
}

TEST(GPUPolicyTest, Grant_PolicyDecision_HasGrantedReason) {
    GPUPolicy pol;
    pol.grant("svc", GPUPolicy::Capability::GPU_ALLOCATE);
    const auto d = pol.check("svc", GPUPolicy::Capability::GPU_ALLOCATE);
    EXPECT_TRUE(d.allowed);
    EXPECT_EQ(d.reason, "granted");
}

TEST(GPUPolicyTest, GrantedCount_IncreasesOnGrant) {
    GPUPolicy pol;
    pol.grant("a");
    pol.grant("b");
    EXPECT_EQ(pol.grantedCount(), 2u);
}

TEST(GPUPolicyTest, GrantedCallers_ContainsGrantedIds) {
    GPUPolicy pol;
    pol.grant("x");
    pol.grant("y");
    const auto callers = pol.grantedCallers();
    EXPECT_EQ(callers.size(), 2u);
    bool has_x = false, has_y = false;
    for (const auto& c : callers) {
        if (c == "x") {
          has_x = true;
        }
        if (c == "y") {
          has_y = true;
        }
    }
    EXPECT_TRUE(has_x);
    EXPECT_TRUE(has_y);
}

TEST(GPUPolicyTest, CapabilitiesOf_ListsGrantedCaps) {
    GPUPolicy pol;
    pol.grant("svc", GPUPolicy::Capability::GPU_ALLOCATE);
    pol.grant("svc", GPUPolicy::Capability::GPU_FREE);
    const auto caps = pol.capabilitiesOf("svc");
    EXPECT_GE(caps.size(), 2u);
}

TEST(GPUPolicyTest, CapabilitiesOf_UnknownCaller_ReturnsEmpty) {
    GPUPolicy pol;
    EXPECT_TRUE(pol.capabilitiesOf("unknown").empty());
}

// ---------------------------------------------------------------------------
// Revoke
// ---------------------------------------------------------------------------

TEST(GPUPolicyTest, Revoke_SingleCapability_DeniesToThatCap) {
    GPUPolicy pol;
    pol.grant("svc", GPUPolicy::Capability::GPU_ANY);
    pol.revoke("svc", GPUPolicy::Capability::GPU_ADMIN);
    EXPECT_TRUE(pol.isAllowed("svc", GPUPolicy::Capability::GPU_ALLOCATE));
    EXPECT_FALSE(pol.isAllowed("svc", GPUPolicy::Capability::GPU_ADMIN));
}

TEST(GPUPolicyTest, RevokeAll_RemovesAllCaps) {
    GPUPolicy pol;
    pol.grant("svc");
    pol.revokeAll("svc");
    EXPECT_FALSE(pol.isAllowed("svc", GPUPolicy::Capability::GPU_ALLOCATE));
    EXPECT_EQ(pol.grantedCount(), 0u);
}

TEST(GPUPolicyTest, Revoke_UnknownCaller_NoOp) {
    GPUPolicy pol;
    EXPECT_NO_THROW(pol.revoke("nobody", GPUPolicy::Capability::GPU_ALLOCATE));
    EXPECT_NO_THROW(pol.revokeAll("nobody"));
}

// ---------------------------------------------------------------------------
// Pre-granted constructor
// ---------------------------------------------------------------------------

TEST(GPUPolicyTest, PreGrantedConstructor_AllowsListedCallers) {
    GPUPolicy pol({"svc_a", "svc_b"});
    EXPECT_TRUE(pol.isAllowed("svc_a", GPUPolicy::Capability::GPU_ALLOCATE));
    EXPECT_TRUE(pol.isAllowed("svc_b", GPUPolicy::Capability::GPU_ALLOCATE));
    EXPECT_FALSE(pol.isAllowed("svc_c", GPUPolicy::Capability::GPU_ALLOCATE));
}

// ---------------------------------------------------------------------------
// capabilityName helper
// ---------------------------------------------------------------------------

TEST(GPUPolicyTest, CapabilityName_KnownValues) {
    EXPECT_STREQ(capabilityName(GPUPolicy::Capability::GPU_ALLOCATE),
                 "GPU_ALLOCATE");
    EXPECT_STREQ(capabilityName(GPUPolicy::Capability::GPU_FREE), "GPU_FREE");
    EXPECT_STREQ(capabilityName(GPUPolicy::Capability::GPU_ADMIN), "GPU_ADMIN");
    EXPECT_STREQ(capabilityName(GPUPolicy::Capability::GPU_ANY),   "GPU_ANY");
}

// ---------------------------------------------------------------------------
// Thread safety
// ---------------------------------------------------------------------------

TEST(GPUPolicyTest, Concurrent_GrantRevoke_NoDataRace) {
    GPUPolicy pol;
    constexpr int THREADS = 8, OPS = 50;
    std::atomic<int> allowed_count{0};

    auto worker = [&](int id) {
        const std::string caller = "caller_" + std::to_string(id);
        for (int i = 0; i < OPS; ++i) {
            pol.grant(caller, GPUPolicy::Capability::GPU_ALLOCATE);
            if (pol.isAllowed(caller, GPUPolicy::Capability::GPU_ALLOCATE)) {
                allowed_count.fetch_add(1);
            }
            pol.revokeAll(caller);
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < THREADS; ++t) {
      threads.emplace_back(worker, t);
    }
    for (auto& th : threads) {
      th.join();
    }

    // After all threads revoke, no caller should hold capabilities.
    EXPECT_EQ(pol.grantedCount(), 0u);
    EXPECT_GT(allowed_count.load(), 0);  // Some grants must have been seen.
}
