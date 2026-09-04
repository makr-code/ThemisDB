/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB — GDPR Subject Rights + Cross-Border Transfer Tests        ║
║ Phase 5.1 + Phase 5.2                                               ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "governance/gdpr_subject_rights.h"
#include "governance/cross_border_transfer.h"

#include <mutex>
#include <atomic>
#include <thread>

using namespace themis::governance;

// ============================================================================
// Mock IGdprEraseTarget
// ============================================================================

class MockEraseTarget : public IGdprEraseTarget {
public:
    explicit MockEraseTarget(const std::string& id, bool should_succeed = true,
                              uint64_t records = 3)
        : id_(id), should_succeed_(should_succeed), records_(records) {}

    std::string storeId() const override { return id_; }

    StoreErasureResult eraseSubject(const std::string& subject_id,
                                     Regulation) override {
        erase_calls_.fetch_add(1);
        last_subject_ = subject_id;
        StoreErasureResult r;
        r.store_id       = id_;
        r.success        = should_succeed_;
        r.records_erased = should_succeed_ ? records_ : 0;
        r.error_message  = should_succeed_ ? "" : "simulated failure";
        return r;
    }

    std::vector<uint8_t> exportSubjectData(const std::string&,
                                            const std::string& format) override {
        std::string data;
        if (format == "json") {
            data = R"({"store":")" + id_ + R"(","data":"test"})";
        } else {
            data = id_ + ",test\n";
        }
        return {data.begin(), data.end()};
    }

    std::atomic<int> erase_calls_{0};
    std::string last_subject_;

private:
    std::string id_;
    bool should_succeed_;
    uint64_t records_;
};

// ============================================================================
// GdprSubjectRightsManager Tests
// ============================================================================

TEST(GdprSubjectRightsManager, RegisterAndCountTargets) {
    GdprSubjectRightsManager mgr;
    EXPECT_EQ(mgr.targetCount(), 0u);
    mgr.registerEraseTarget(std::make_shared<MockEraseTarget>("store_a"));
    mgr.registerEraseTarget(std::make_shared<MockEraseTarget>("store_b"));
    EXPECT_EQ(mgr.targetCount(), 2u);
}

TEST(GdprSubjectRightsManager, NullTargetThrows) {
    GdprSubjectRightsManager mgr;
    EXPECT_THROW(mgr.registerEraseTarget(nullptr), std::invalid_argument);
}

TEST(GdprSubjectRightsManager, FullErasureAcrossMultipleStores) {
    GdprSubjectRightsManager mgr;
    auto t1 = std::make_shared<MockEraseTarget>("vector_index");
    auto t2 = std::make_shared<MockEraseTarget>("graph_store");
    auto t3 = std::make_shared<MockEraseTarget>("doc_store");
    mgr.registerEraseTarget(t1);
    mgr.registerEraseTarget(t2);
    mgr.registerEraseTarget(t3);

    auto report = mgr.requestErasure("user-1234", Regulation::GDPR,
                                      "user_request", "admin");

    EXPECT_EQ(report.subject_id, "user-1234");
    EXPECT_TRUE(report.fully_erased);
    EXPECT_EQ(report.store_results.size(), 3u);
    for (const auto& r : report.store_results) {
        EXPECT_TRUE(r.success);
    }
    EXPECT_EQ(t1->erase_calls_.load(), 1);
    EXPECT_EQ(t2->erase_calls_.load(), 1);
    EXPECT_EQ(t3->erase_calls_.load(), 1);
}

TEST(GdprSubjectRightsManager, PartialFailureReportedCorrectly) {
    GdprSubjectRightsManager mgr;
    mgr.registerEraseTarget(std::make_shared<MockEraseTarget>("store_ok",  true));
    mgr.registerEraseTarget(std::make_shared<MockEraseTarget>("store_fail", false));

    auto report = mgr.requestErasure("user-5678", Regulation::CCPA, "ccpa_request");

    EXPECT_FALSE(report.fully_erased);
    EXPECT_EQ(report.store_results.size(), 2u);
    int ok = 0, fail = 0;
    for (const auto& r : report.store_results) {
        if (r.success) {
          ++ok; else ++fail;
        }
    }
    EXPECT_EQ(ok, 1);
    EXPECT_EQ(fail, 1);
}

TEST(GdprSubjectRightsManager, EmptySubjectIdThrows) {
    GdprSubjectRightsManager mgr;
    EXPECT_THROW(mgr.requestErasure("", Regulation::GDPR, "reason"),
                 std::invalid_argument);
}

TEST(GdprSubjectRightsManager, PortabilityJsonExport) {
    GdprSubjectRightsManager mgr;
    mgr.registerEraseTarget(std::make_shared<MockEraseTarget>("store_a"));
    mgr.registerEraseTarget(std::make_shared<MockEraseTarget>("store_b"));

    auto pkg = mgr.requestPortability("user-1234", "json");
    EXPECT_EQ(pkg.subject_id, "user-1234");
    EXPECT_EQ(pkg.format, "json");
    EXPECT_FALSE(pkg.payload.empty());
    // Payload should start with '[' for JSON array wrapper
    EXPECT_EQ(pkg.payload.front(), '[');
}

TEST(GdprSubjectRightsManager, PortabilityCsvExport) {
    GdprSubjectRightsManager mgr;
    mgr.registerEraseTarget(std::make_shared<MockEraseTarget>("store_csv"));

    auto pkg = mgr.requestPortability("user-9999", "csv");
    EXPECT_FALSE(pkg.payload.empty());
    std::string content(pkg.payload.begin(), pkg.payload.end());
    EXPECT_NE(content.find("store_csv"), std::string::npos);
}

TEST(GdprSubjectRightsManager, PortabilityWithTsaSigning) {
    bool signer_called = false;
    GdprSubjectRightsManager::TsaSigner signer = [&](const std::vector<uint8_t>&) {
        signer_called = true;
        return std::vector<uint8_t>{0xDE, 0xAD, 0xBE, 0xEF};
    };

    GdprSubjectRightsManager mgr(signer);
    mgr.registerEraseTarget(std::make_shared<MockEraseTarget>("store_ts"));

    auto pkg = mgr.requestPortability("user-tsa", "json");
    EXPECT_TRUE(signer_called);
    EXPECT_FALSE(pkg.tsa_signature.empty());
    EXPECT_EQ(pkg.tsa_signature[0], 0xDE);
}

TEST(GdprSubjectRightsManager, ConcurrentErasureForSameSubjectSerialized) {
    GdprSubjectRightsManager mgr;
    auto target = std::make_shared<MockEraseTarget>("concurrent_store");
    mgr.registerEraseTarget(target);

    constexpr int kThreads = 5;
    std::vector<std::thread> threads;
    std::atomic<int> completed{0};
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&]() {
            mgr.requestErasure("shared-subject", Regulation::GDPR, "test");
            completed.fetch_add(1);
        });
    }
    for (auto& t : threads) {
      t.join();
    }
    EXPECT_EQ(completed.load(), kThreads);
    EXPECT_EQ(target->erase_calls_.load(), kThreads);
}

TEST(GdprSubjectRightsManager, PseudonymisationInAuditLog) {
    // Verify that erasure reports pseudonymisation (records_erased > 0) even
    // when the store is the audit log type (success=true, records=1 simulated).
    GdprSubjectRightsManager mgr;
    auto audit_target = std::make_shared<MockEraseTarget>("audit_log", true, 1);
    mgr.registerEraseTarget(audit_target);

    auto report = mgr.requestErasure("user-audit", Regulation::GDPR, "user_request");
    EXPECT_TRUE(report.fully_erased);
    EXPECT_GT(report.store_results[0].records_erased, 0u);
}

TEST(GdprSubjectRightsManager, ReportSummaryMapContainsExpectedKeys) {
    GdprSubjectRightsManager mgr;
    mgr.registerEraseTarget(std::make_shared<MockEraseTarget>("s1"));

    auto report = mgr.requestErasure("user-map", Regulation::GDPR, "test");
    auto m = report.toSummaryMap();
    EXPECT_NE(m.find("subject_id"), m.end());
    EXPECT_NE(m.find("regulation"), m.end());
    EXPECT_NE(m.find("fully_erased"), m.end());
    EXPECT_EQ(m.at("regulation"), "GDPR");
}

TEST(GdprSubjectRightsManager, CcpaRegulationTaggedCorrectly) {
    GdprSubjectRightsManager mgr;
    mgr.registerEraseTarget(std::make_shared<MockEraseTarget>("store"));

    auto report = mgr.requestErasure("user-ccpa", Regulation::CCPA, "ccpa");
    auto m = report.toSummaryMap();
    EXPECT_EQ(m.at("regulation"), "CCPA");
}

TEST(GdprSubjectRightsManager, ZeroTargetsErasureFullyErased) {
    GdprSubjectRightsManager mgr;
    // No targets registered — vacuously fully erased
    auto report = mgr.requestErasure("no-stores", Regulation::GDPR, "test");
    EXPECT_TRUE(report.fully_erased);
    EXPECT_TRUE(report.store_results.empty());
}

// ============================================================================
// CrossBorderTransferPolicy Tests
// ============================================================================

TEST(CrossBorderTransferPolicyTest, AdequateCountryAllowed) {
    CrossBorderTransferPolicy policy;
    auto decision = policy.checkTransfer("JP");
    EXPECT_TRUE(decision.allowed);
    EXPECT_EQ(decision.mechanism, TransferMechanism::ADEQUACY_DECISION);
    EXPECT_EQ(decision.transfer_mechanism_header, "ADEQUACY_DECISION");
}

TEST(CrossBorderTransferPolicyTest, ProhibitedCountryDenied) {
    CrossBorderTransferPolicy policy;
    auto decision = policy.checkTransfer("CN");
    EXPECT_FALSE(decision.allowed);
    EXPECT_EQ(decision.mechanism, TransferMechanism::PROHIBITED);
}

TEST(CrossBorderTransferPolicyTest, SccMechanismAttached) {
    CrossBorderTransferPolicy policy;
    auto decision = policy.checkTransfer("US");
    EXPECT_TRUE(decision.allowed);
    EXPECT_EQ(decision.mechanism, TransferMechanism::STANDARD_CONTRACTUAL_CLAUSES);
    EXPECT_EQ(decision.transfer_mechanism_header, "SCC");
}

TEST(CrossBorderTransferPolicyTest, UnknownRegionDefaultsToProhibited) {
    CrossBorderTransferPolicy policy;
    auto decision = policy.checkTransfer("XX");
    EXPECT_FALSE(decision.allowed);
    EXPECT_EQ(decision.mechanism, TransferMechanism::PROHIBITED);
}

TEST(CrossBorderTransferPolicyTest, EmptyRegionIsProhibited) {
    CrossBorderTransferPolicy policy;
    auto decision = policy.checkTransfer("");
    EXPECT_FALSE(decision.allowed);
}

TEST(CrossBorderTransferPolicyTest, AdequacyListHotReload) {
    CrossBorderTransferPolicy policy;

    // Initially CN is PROHIBITED
    EXPECT_EQ(policy.getMechanism("CN"), TransferMechanism::PROHIBITED);

    // Hot-reload with a new list that includes CN as BCR
    auto new_list = CrossBorderTransferPolicy::defaultEuAdequacyList();
    new_list["CN"] = TransferMechanism::BINDING_CORPORATE_RULES;
    policy.loadAdequacyList(new_list);

    EXPECT_EQ(policy.getMechanism("CN"), TransferMechanism::BINDING_CORPORATE_RULES);
    auto d = policy.checkTransfer("CN");
    EXPECT_TRUE(d.allowed);
}

TEST(CrossBorderTransferPolicyTest, SetRegionMechanismUpdate) {
    CrossBorderTransferPolicy policy;
    policy.setRegionMechanism("ZZ", TransferMechanism::DEROGATION);
    auto d = policy.checkTransfer("ZZ");
    EXPECT_TRUE(d.allowed);
    EXPECT_EQ(d.mechanism, TransferMechanism::DEROGATION);
}

TEST(CrossBorderTransferPolicyTest, CaseInsensitiveRegionLookup) {
    CrossBorderTransferPolicy policy;
    // "jp" (lowercase) should map to ADEQUACY_DECISION same as "JP"
    auto d = policy.checkTransfer("jp");
    EXPECT_TRUE(d.allowed);
    EXPECT_EQ(d.mechanism, TransferMechanism::ADEQUACY_DECISION);
}

TEST(CrossBorderTransferPolicyTest, MechanismDescriptionNonEmpty) {
    for (auto m : {TransferMechanism::ADEQUACY_DECISION,
                   TransferMechanism::STANDARD_CONTRACTUAL_CLAUSES,
                   TransferMechanism::BINDING_CORPORATE_RULES,
                   TransferMechanism::DEROGATION,
                   TransferMechanism::PROHIBITED}) {
        EXPECT_FALSE(CrossBorderTransferPolicy::mechanismDescription(m).empty());
        EXPECT_FALSE(CrossBorderTransferPolicy::mechanismToHeaderValue(m).empty());
    }
}

TEST(CrossBorderTransferPolicyTest, GetAdequacyListReturnsSnapshot) {
    CrossBorderTransferPolicy policy;
    auto list = policy.getAdequacyList();
    EXPECT_FALSE(list.empty());
    EXPECT_NE(list.find("JP"), list.end());
}
