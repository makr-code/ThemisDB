/*
 * ThemisDB | File: application_profile_pipeline_test.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 97/100
 * Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../test_data_generator.h"
#include "../test_fixture.h"

namespace themis { namespace test { 

namespace {

struct AppProfileAskResult {
    bool ok{false};
    std::string answer;
    std::string error;
};

class ApplicationProfilePipeline {
  public:
    ApplicationProfilePipeline(std::shared_ptr<MockPipelineAuth> auth, std::shared_ptr<MockPipelineIndex> index,
                               std::shared_ptr<InMemoryPipelineStorage> storage,
                               std::shared_ptr<MockPipelineLlmBackend> llm, std::shared_ptr<PipelineAuditLog> audit)
        : auth_(std::move(auth)), index_(std::move(index)), storage_(std::move(storage)), llm_(std::move(llm)),
          audit_(std::move(audit)) {}

    [[nodiscard]] bool RegisterSession(const std::string &token, const std::string &tenant_id,
                                       const std::string &role) {
        const auto auth_result = auth_->Authorize(token);
        if (!auth_result.authorized || role != "app_user") {
            audit_->Record({"app_profile", "session_rejected", tenant_id});
            return false;
        }

        const auto existing_session = session_tenant_.find(token);
        if (existing_session != session_tenant_.end() && existing_session->second != tenant_id) {
            audit_->Record({"app_profile", "session_rejected", "token_rebind_blocked"});
            return false;
        }

        session_tenant_[token] = tenant_id;
        audit_->Record({"app_profile", "session_started", tenant_id});
        return true;
    }

    [[nodiscard]] bool IngestEvent(const std::string &token, const std::string &tenant_id, const std::string &event_id,
                                   const std::string &payload, const std::vector<std::string> &terms) {
        if (!IsAuthorizedTenant(token, tenant_id)) {
            return false;
        }

        const auto key = tenant_id + "::" + event_id;
        if (ingested_keys_.count(key) > 0U) {
            audit_->Record({"app_profile", "duplicate_ignored", key});
            return true;
        }

        ingested_keys_.insert(key);
        storage_->Write(key, payload);
        index_->IndexDocument(key, terms);
        tenant_keys_[tenant_id].push_back(key);
        cdc_events_.push_back("cdc:" + key);
        audit_->Record({"app_profile", "event_ingested", key});
        return true;
    }

    [[nodiscard]] AppProfileAskResult AskAssistant(const std::string &token, const std::string &tenant_id,
                                                   const std::string &question, const std::string &term) {
        if (!IsAuthorizedTenant(token, tenant_id)) {
            return {false, "", "unauthorized_tenant"};
        }

        if (assistant_circuit_open_) {
            audit_->Record({"app_profile", "assistant_circuit_open", tenant_id});
            return {true, "fallback:circuit-open", ""};
        }

        const auto all_hits = index_->Search(term);
        std::string context = "no_context";
        for (const auto &key : all_hits) {
            if (key.rfind(tenant_id + "::", 0) != 0U) {
                continue;
            }
            const auto payload = storage_->Read(key);
            if (!payload.has_value()) {
                continue;
            }
            context = *payload;
            break;
        }

        for (size_t attempt = 0; attempt <= assistant_retry_budget_; ++attempt) {
            const bool is_timeout = simulated_timeout_terms_.count(term) > 0U;
            if (is_timeout) {
                audit_->Record({"app_profile", "assistant_timeout", tenant_id});
            } else {
                const auto response = llm_->Infer(question, context);
                if (response.has_value()) {
                    consecutive_assistant_failures_ = 0U;
                    audit_->Record({"app_profile", "assistant_answered", tenant_id});
                    return {true, *response, ""};
                }
            }

            if (attempt < assistant_retry_budget_) {
                audit_->Record({"app_profile", "assistant_retry", tenant_id});
            }
        }

        ++consecutive_assistant_failures_;
        if (consecutive_assistant_failures_ >= circuit_breaker_threshold_) {
            assistant_circuit_open_ = true;
            audit_->Record({"app_profile", "assistant_circuit_opened", tenant_id});
        }

        audit_->Record({"app_profile", "assistant_fallback", tenant_id});
        return {true, "fallback:local-summary:" + context, ""};
    }

    [[nodiscard]] bool ExportTenantSnapshot(const std::string &tenant_id, const std::filesystem::path &output_file) {
        std::ofstream out(output_file);
        if (!out.is_open()) {
            return false;
        }

        const auto event_count = TenantEventCount(tenant_id);
        out << "tenant=" << tenant_id << "\n";
        out << "events=" << event_count << "\n";
        out << "replica_marker=" << event_count << "\n";
        replica_markers_.push_back(tenant_id + ":" + std::to_string(event_count));
        audit_->Record({"app_profile", "snapshot_exported", tenant_id});
        return true;
    }

    [[nodiscard]] size_t TenantEventCount(const std::string &tenant_id) const {
        const auto it = tenant_keys_.find(tenant_id);
        return it == tenant_keys_.end() ? 0U : it->second.size();
    }

    [[nodiscard]] size_t CdcCount() const {
        return cdc_events_.size();
    }

    [[nodiscard]] size_t ReplicaMarkerCount() const {
        return replica_markers_.size();
    }

    void SetAssistantRetryBudget(size_t retries) {
        assistant_retry_budget_ = retries;
    }

    void SetAssistantCircuitBreakerThreshold(size_t threshold) {
        circuit_breaker_threshold_ = threshold == 0U ? 1U : threshold;
    }

    void AddTimeoutTerm(std::string term) {
        simulated_timeout_terms_.insert(std::move(term));
    }

  private:
    [[nodiscard]] bool IsAuthorizedTenant(const std::string &token, const std::string &tenant_id) const {
        const auto auth_result = auth_->Authorize(token);
        if (!auth_result.authorized) {
            return false;
        }

        const auto it = session_tenant_.find(token);
        return it != session_tenant_.end() && it->second == tenant_id;
    }

    std::shared_ptr<MockPipelineAuth> auth_;
    std::shared_ptr<MockPipelineIndex> index_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineLlmBackend> llm_;
    std::shared_ptr<PipelineAuditLog> audit_;

    std::unordered_map<std::string, std::string> session_tenant_;
    std::unordered_map<std::string, std::vector<std::string>> tenant_keys_;
    std::unordered_set<std::string> ingested_keys_;
    std::vector<std::string> cdc_events_;
    std::vector<std::string> replica_markers_;
    std::unordered_set<std::string> simulated_timeout_terms_;
    size_t assistant_retry_budget_{0U};
    size_t circuit_breaker_threshold_{3U};
    size_t consecutive_assistant_failures_{0U};
    bool assistant_circuit_open_{false};
};

} // namespace

class ApplicationProfilePipelineTest : public IntegrationTestFixture {
  protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        auth_     = CreateMockAuth();
        index_    = CreateMockIndex();
        storage_  = CreateInMemoryStorage();
        llm_      = CreateMockLlmBackend();
        audit_    = CreateAuditLog();
        data_gen_ = std::make_unique<TestDataGenerator>();
        pipeline_ = std::make_unique<ApplicationProfilePipeline>(auth_, index_, storage_, llm_, audit_);
    }

    std::shared_ptr<MockPipelineAuth> auth_;
    std::shared_ptr<MockPipelineIndex> index_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineLlmBackend> llm_;
    std::shared_ptr<PipelineAuditLog> audit_;
    std::unique_ptr<TestDataGenerator> data_gen_;
    std::unique_ptr<ApplicationProfilePipeline> pipeline_;
};

TEST_F(ApplicationProfilePipelineTest, APP01_EndUserJourneyIngestAskAndExport) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);

    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_a", "app_user"));
    ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_a", "event_1", "invoice pending", {"billing"}));

    const auto ask_result = pipeline_->AskAssistant(token, "tenant_a", "Welche Rechnung ist offen?", "billing");
    ASSERT_TRUE(ask_result.ok);
    EXPECT_NE(ask_result.answer.find("Welche Rechnung ist offen?"), std::string::npos);

    const auto snapshot_file = GetTempDir() / "tenant_a.snapshot";
    ASSERT_TRUE(pipeline_->ExportTenantSnapshot("tenant_a", snapshot_file));
    EXPECT_TRUE(std::filesystem::exists(snapshot_file));
    EXPECT_TRUE(audit_->Contains("app_profile", "event_ingested"));
    EXPECT_TRUE(audit_->Contains("app_profile", "assistant_answered"));
}

TEST_F(ApplicationProfilePipelineTest, APP02_MultiTenantIsolationBlocksCrossTenantRead) {
    const auto token_a = data_gen_->GeneratePipelineToken(true);
    const auto token_b = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token_a);
    auth_->AllowToken(token_b);

    ASSERT_TRUE(pipeline_->RegisterSession(token_a, "tenant_a", "app_user"));
    ASSERT_TRUE(pipeline_->RegisterSession(token_b, "tenant_b", "app_user"));
    ASSERT_TRUE(pipeline_->IngestEvent(token_a, "tenant_a", "event_a", "private-a", {"shared"}));

    const auto result_b = pipeline_->AskAssistant(token_b, "tenant_b", "show shared", "shared");

    ASSERT_TRUE(result_b.ok);
    EXPECT_NE(result_b.answer.find("no_context"), std::string::npos);
    EXPECT_EQ(result_b.answer.find("private-a"), std::string::npos);
}

TEST_F(ApplicationProfilePipelineTest, APP03_LlmFailureFallsBackToDeterministicSummary) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    llm_->SetInferenceFailure(true);

    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_fallback", "app_user"));
    ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_fallback", "event_critical", "incident-42", {"ops"}));

    const auto result = pipeline_->AskAssistant(token, "tenant_fallback", "status incident", "ops");

    ASSERT_TRUE(result.ok);
    EXPECT_NE(result.answer.find("fallback:local-summary"), std::string::npos);
    EXPECT_NE(result.answer.find("incident-42"), std::string::npos);
    EXPECT_TRUE(audit_->Contains("app_profile", "assistant_fallback"));
}

TEST_F(ApplicationProfilePipelineTest, APP04_BurstProfileKeepsCdcAndReplicaSnapshotConsistent) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_burst", "app_user"));

    for (size_t i = 0; i < 24; ++i) {
        const auto event_id = "event_" + std::to_string(i);
        const auto payload  = "payload_" + std::to_string(i);
        ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_burst", event_id, payload, {"burst"}));
    }

    const auto snapshot_file = GetTempDir() / "tenant_burst.snapshot";
    ASSERT_TRUE(pipeline_->ExportTenantSnapshot("tenant_burst", snapshot_file));

    EXPECT_EQ(pipeline_->TenantEventCount("tenant_burst"), 24U);
    EXPECT_EQ(pipeline_->CdcCount(), 24U);
    EXPECT_EQ(pipeline_->ReplicaMarkerCount(), 1U);
}

TEST_F(ApplicationProfilePipelineTest, APP05_TokenRebindAcrossTenantsIsRejected) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);

    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_original", "app_user"));
    EXPECT_FALSE(pipeline_->RegisterSession(token, "tenant_other", "app_user"));
    EXPECT_TRUE(audit_->Contains("app_profile", "session_rejected"));
}

TEST_F(ApplicationProfilePipelineTest, APP06_DuplicateEventIsIdempotentForCdcAndSnapshotCount) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_idempotent", "app_user"));

    ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_idempotent", "event_1", "payload", {"billing"}));
    ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_idempotent", "event_1", "payload", {"billing"}));

    const auto snapshot_file = GetTempDir() / "tenant_idempotent.snapshot";
    ASSERT_TRUE(pipeline_->ExportTenantSnapshot("tenant_idempotent", snapshot_file));

    EXPECT_EQ(pipeline_->TenantEventCount("tenant_idempotent"), 1U);
    EXPECT_EQ(pipeline_->CdcCount(), 1U);
    EXPECT_TRUE(audit_->Contains("app_profile", "duplicate_ignored"));
}

TEST_F(ApplicationProfilePipelineTest, APP07_CrossTenantIngestAttemptIsBlockedWithoutArtifacts) {
    const auto token_a = data_gen_->GeneratePipelineToken(true);
    const auto token_b = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token_a);
    auth_->AllowToken(token_b);

    ASSERT_TRUE(pipeline_->RegisterSession(token_a, "tenant_a", "app_user"));
    ASSERT_TRUE(pipeline_->RegisterSession(token_b, "tenant_b", "app_user"));

    EXPECT_FALSE(pipeline_->IngestEvent(token_a, "tenant_b", "event_blocked", "secret_b", {"blocked"}));
    EXPECT_EQ(pipeline_->TenantEventCount("tenant_b"), 0U);
    EXPECT_EQ(pipeline_->CdcCount(), 0U);
}

TEST_F(ApplicationProfilePipelineTest, APP08_UnauthorizedTokenIsRejectedAcrossPipelineActions) {
    const auto token = data_gen_->GeneratePipelineToken(false);

    EXPECT_FALSE(pipeline_->RegisterSession(token, "tenant_unauth", "app_user"));
    EXPECT_FALSE(pipeline_->IngestEvent(token, "tenant_unauth", "event_1", "payload", {"billing"}));

    const auto result = pipeline_->AskAssistant(token, "tenant_unauth", "status", "billing");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, "unauthorized_tenant");
}

TEST_F(ApplicationProfilePipelineTest, APP09_SnapshotExportFailureDoesNotEmitReplicaMarker) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_export", "app_user"));
    ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_export", "event_1", "payload", {"ops"}));

    const auto invalid_snapshot_path = GetTempDir() / "non_existing_parent" / "tenant_export.snapshot";
    EXPECT_FALSE(pipeline_->ExportTenantSnapshot("tenant_export", invalid_snapshot_path));
    EXPECT_EQ(pipeline_->ReplicaMarkerCount(), 0U);
}

TEST_F(ApplicationProfilePipelineTest, APP10_TransientLlmFailureCanRecoverOnSubsequentRequest) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_recover", "app_user"));
    ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_recover", "event_1", "incident-007", {"ops"}));

    llm_->SetInferenceFailure(true);
    const auto first = pipeline_->AskAssistant(token, "tenant_recover", "status", "ops");
    ASSERT_TRUE(first.ok);
    EXPECT_NE(first.answer.find("fallback:local-summary"), std::string::npos);

    llm_->SetInferenceFailure(false);
    const auto second = pipeline_->AskAssistant(token, "tenant_recover", "status", "ops");
    ASSERT_TRUE(second.ok);
    EXPECT_EQ(second.answer.find("fallback:local-summary"), std::string::npos);
    EXPECT_TRUE(audit_->Contains("app_profile", "assistant_fallback"));
    EXPECT_TRUE(audit_->Contains("app_profile", "assistant_answered"));
}

TEST_F(ApplicationProfilePipelineTest, APP11_TimeoutTriggersRetryThenFallbackSummary) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    pipeline_->SetAssistantRetryBudget(2U);
    pipeline_->AddTimeoutTerm("slow_term");

    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_timeout", "app_user"));
    ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_timeout", "event_1", "context-timeout", {"slow_term"}));

    const auto result = pipeline_->AskAssistant(token, "tenant_timeout", "status", "slow_term");
    ASSERT_TRUE(result.ok);
    EXPECT_NE(result.answer.find("fallback:local-summary"), std::string::npos);
    EXPECT_TRUE(audit_->Contains("app_profile", "assistant_timeout"));
    EXPECT_TRUE(audit_->Contains("app_profile", "assistant_retry"));
}

TEST_F(ApplicationProfilePipelineTest, APP12_LlmFailuresRespectRetryBudgetBeforeFallback) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    pipeline_->SetAssistantRetryBudget(2U);
    llm_->SetInferenceFailure(true);

    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_retry", "app_user"));
    ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_retry", "event_1", "context-retry", {"ops"}));

    const auto result = pipeline_->AskAssistant(token, "tenant_retry", "status", "ops");
    ASSERT_TRUE(result.ok);
    EXPECT_NE(result.answer.find("fallback:local-summary"), std::string::npos);
    EXPECT_TRUE(audit_->Contains("app_profile", "assistant_retry"));
    EXPECT_TRUE(audit_->Contains("app_profile", "assistant_fallback"));
}

TEST_F(ApplicationProfilePipelineTest, APP13_CircuitBreakerBlocksAssistantAfterConsecutiveFailures) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    pipeline_->SetAssistantCircuitBreakerThreshold(2U);
    llm_->SetInferenceFailure(true);

    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_cb", "app_user"));
    ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_cb", "event_1", "context-cb", {"ops"}));

    const auto first  = pipeline_->AskAssistant(token, "tenant_cb", "status", "ops");
    const auto second = pipeline_->AskAssistant(token, "tenant_cb", "status", "ops");
    ASSERT_TRUE(first.ok);
    ASSERT_TRUE(second.ok);
    EXPECT_TRUE(audit_->Contains("app_profile", "assistant_circuit_opened"));

    llm_->SetInferenceFailure(false);
    const auto blocked = pipeline_->AskAssistant(token, "tenant_cb", "status", "ops");
    ASSERT_TRUE(blocked.ok);
    EXPECT_NE(blocked.answer.find("fallback:circuit-open"), std::string::npos);
    EXPECT_TRUE(audit_->Contains("app_profile", "assistant_circuit_open"));
}
} } // namespace themis::test
