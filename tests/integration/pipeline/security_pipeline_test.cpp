/*
 * ThemisDB | File: security_pipeline_test.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../test_data_generator.h"
#include "../test_fixture.h"

namespace themis { namespace test { 

namespace {

struct SecurityResponse {
    int status{500};
    std::string body;
};

class SecurityPipeline {
  public:
    SecurityPipeline(std::shared_ptr<MockPipelineAuth> auth, std::shared_ptr<PipelineAuditLog> audit)
        : auth_(std::move(auth)), audit_(std::move(audit)) {}

    void AllowRole(const std::string &role) {
        allowed_roles_.insert(role);
    }

    SecurityResponse Write(const std::string &token, const std::string &role, const std::string &key,
                           const std::string &plaintext) {
        const auto auth_result = auth_->Authorize(token);
        if (!auth_result.authorized || IsRevoked(token)) {
            audit_->Record({"security", "auth_reject", "masked"});
            return {401, ""};
        }
        if (!allowed_roles_.count(role)) {
            audit_->Record({"security", "rbac_reject", "masked"});
            return {403, ""};
        }

        encrypted_store_[key] = Encrypt(plaintext);
        audit_->Record({"security", "write_encrypted", "record_id=" + key + ";masked=***"});
        return {200, "ok"};
    }

    SecurityResponse Read(const std::string &token, const std::string &role, const std::string &key) const {
        const auto auth_result = auth_->Authorize(token);
        if (!auth_result.authorized || IsRevoked(token)) {
            return {401, ""};
        }
        if (!allowed_roles_.count(role)) {
            return {403, ""};
        }

        const auto it = encrypted_store_.find(key);
        if (it == encrypted_store_.end()) {
            return {404, ""};
        }
        return {200, Decrypt(it->second)};
    }

    void RotateEncryptionKey() {
        ++key_version_;
        for (auto &[key, encrypted] : encrypted_store_) {
            const auto plain = Decrypt(encrypted);
            encrypted        = Encrypt(plain);
        }
        audit_->Record({"security", "key_rotation", "masked"});
    }

    void RevokeToken(const std::string &token) {
        revoked_tokens_.insert(token);
        audit_->Record({"security", "token_revoked", "masked"});
    }

    [[nodiscard]] size_t KeyVersion() const {
        return key_version_;
    }

    [[nodiscard]] std::string RawEncrypted(const std::string &key) const {
        const auto it = encrypted_store_.find(key);
        return it == encrypted_store_.end() ? "" : it->second;
    }

  private:
    [[nodiscard]] bool IsRevoked(const std::string &token) const {
        return revoked_tokens_.count(token) > 0U;
    }

    [[nodiscard]] std::string Encrypt(const std::string &plaintext) const {
        std::string reversed = plaintext;
        std::reverse(reversed.begin(), reversed.end());
        return "k" + std::to_string(key_version_) + ":" + reversed;
    }

    [[nodiscard]] std::string Decrypt(const std::string &encrypted) const {
        const auto pos = encrypted.find(':');
        if (pos == std::string::npos) {
            return "";
        }
        std::string reversed = encrypted.substr(pos + 1);
        std::reverse(reversed.begin(), reversed.end());
        return reversed;
    }

    std::shared_ptr<MockPipelineAuth> auth_;
    std::shared_ptr<PipelineAuditLog> audit_;
    std::unordered_set<std::string> allowed_roles_;
    std::unordered_set<std::string> revoked_tokens_;
    std::unordered_map<std::string, std::string> encrypted_store_;
    size_t key_version_{1};
};

} // namespace

class SecurityPipelineTest : public IntegrationTestFixture {
  protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        auth_     = CreateMockAuth();
        audit_    = CreateAuditLog();
        data_gen_ = std::make_unique<TestDataGenerator>();
        pipeline_ = std::make_unique<SecurityPipeline>(auth_, audit_);
        pipeline_->AllowRole("admin");
    }

    std::shared_ptr<MockPipelineAuth> auth_;
    std::shared_ptr<PipelineAuditLog> audit_;
    std::unique_ptr<TestDataGenerator> data_gen_;
    std::unique_ptr<SecurityPipeline> pipeline_;
};

TEST_F(SecurityPipelineTest, SEC01_JwtRbacEncryptedAccessAndMaskedAudit) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);

    const auto write = pipeline_->Write(token, "admin", "record_1", "top_secret");
    const auto read  = pipeline_->Read(token, "admin", "record_1");

    ASSERT_EQ(write.status, 200);
    ASSERT_EQ(read.status, 200);
    EXPECT_EQ(read.body, "top_secret");
    EXPECT_TRUE(audit_->Contains("security", "write_encrypted"));
    const auto encrypted = pipeline_->RawEncrypted("record_1");
    EXPECT_NE(encrypted.find("k1:"), std::string::npos);
}

TEST_F(SecurityPipelineTest, SEC02_KeyRotationKeepsReadsWorkingAndUsesNewKeyForNewData) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);

    ASSERT_EQ(pipeline_->Write(token, "admin", "before", "payload_before").status, 200);
    pipeline_->RotateEncryptionKey();
    ASSERT_EQ(pipeline_->Write(token, "admin", "after", "payload_after").status, 200);

    const auto before_read = pipeline_->Read(token, "admin", "before");
    const auto after_read  = pipeline_->Read(token, "admin", "after");

    EXPECT_EQ(before_read.status, 200);
    EXPECT_EQ(after_read.status, 200);
    EXPECT_EQ(before_read.body, "payload_before");
    EXPECT_EQ(after_read.body, "payload_after");
    EXPECT_NE(pipeline_->RawEncrypted("after").find("k2:"), std::string::npos);
}

TEST_F(SecurityPipelineTest, SEC03_UnknownTokenReturns401WithoutDataLeak) {
    const auto unknown_token = data_gen_->GeneratePipelineToken(false);

    const auto write = pipeline_->Write(unknown_token, "admin", "x", "secret");
    const auto read  = pipeline_->Read(unknown_token, "admin", "x");

    EXPECT_EQ(write.status, 401);
    EXPECT_EQ(read.status, 401);
    EXPECT_TRUE(write.body.empty());
    EXPECT_TRUE(read.body.empty());
}

TEST_F(SecurityPipelineTest, SEC04_RbacRejectionReturns403WithAuditAndNoDataLeak) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);

    const auto write = pipeline_->Write(token, "guest", "record_x", "payload");
    const auto read  = pipeline_->Read(token, "guest", "record_x");

    EXPECT_EQ(write.status, 403);
    EXPECT_EQ(read.status, 403);
    EXPECT_TRUE(write.body.empty());
    EXPECT_TRUE(read.body.empty());
    EXPECT_TRUE(audit_->Contains("security", "rbac_reject"));
}

TEST_F(SecurityPipelineTest, SEC05_MultipleKeyRotationsKeepAllRecordsReadableWithFinalKeyVersion) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);

    ASSERT_EQ(pipeline_->Write(token, "admin", "rec_a", "value_a").status, 200);
    pipeline_->RotateEncryptionKey();
    ASSERT_EQ(pipeline_->Write(token, "admin", "rec_b", "value_b").status, 200);
    pipeline_->RotateEncryptionKey();
    ASSERT_EQ(pipeline_->Write(token, "admin", "rec_c", "value_c").status, 200);

    EXPECT_EQ(pipeline_->KeyVersion(), 3U);
    EXPECT_EQ(pipeline_->Read(token, "admin", "rec_a").body, "value_a");
    EXPECT_EQ(pipeline_->Read(token, "admin", "rec_b").body, "value_b");
    EXPECT_EQ(pipeline_->Read(token, "admin", "rec_c").body, "value_c");
    EXPECT_NE(pipeline_->RawEncrypted("rec_a").find("k3:"), std::string::npos);
    EXPECT_NE(pipeline_->RawEncrypted("rec_b").find("k3:"), std::string::npos);
    EXPECT_NE(pipeline_->RawEncrypted("rec_c").find("k3:"), std::string::npos);
}

TEST_F(SecurityPipelineTest, SEC06_RevokedTokenIsRejectedAndAuditedWithoutDataLeak) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);

    ASSERT_EQ(pipeline_->Write(token, "admin", "sensitive", "classified").status, 200);
    pipeline_->RevokeToken(token);

    const auto write_after = pipeline_->Write(token, "admin", "new_rec", "data");
    const auto read_after  = pipeline_->Read(token, "admin", "sensitive");

    EXPECT_EQ(write_after.status, 401);
    EXPECT_EQ(read_after.status, 401);
    EXPECT_TRUE(write_after.body.empty());
    EXPECT_TRUE(read_after.body.empty());
    EXPECT_TRUE(audit_->Contains("security", "token_revoked"));
    EXPECT_TRUE(audit_->Contains("security", "auth_reject"));
}
} } // namespace themis::test
