// Copyright (c) 2025 VCC ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Unit tests for ArrowUserRegistrationPlugin (Issue #99)
// Tests bulk sync of user records via Apache Arrow RecordBatch and
// Arrow-based authentication for synced users.

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_ARROW

#include <arrow/api.h>
#include <memory>
#include <string>
#include <vector>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>

#include "security/arrow_user_registration_plugin.h"

namespace themis {
namespace security {
namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/**
 * Build an Arrow RecordBatch with the required user columns.
 *
 * @param user_ids       Vector of user_id values.
 * @param password_hashes Vector of password_hash values (pre-hashed).
 * @param roles          Vector of comma-separated roles strings.
 * @param emails         Vector of email addresses.
 */
std::shared_ptr<arrow::RecordBatch> BuildUserBatch(
    const std::vector<std::string>& user_ids,
    const std::vector<std::string>& password_hashes,
    const std::vector<std::string>& roles,
    const std::vector<std::string>& emails)
{
    arrow::StringBuilder uid_builder, pwd_builder, roles_builder, email_builder;

    for (size_t i = 0; i < user_ids.size(); ++i) {
        EXPECT_TRUE(uid_builder.Append(user_ids[i]).ok());
        EXPECT_TRUE(pwd_builder.Append(password_hashes[i]).ok());
        EXPECT_TRUE(roles_builder.Append(roles[i]).ok());
        EXPECT_TRUE(email_builder.Append(emails[i]).ok());
    }

    std::shared_ptr<arrow::Array> uid_arr, pwd_arr, roles_arr, email_arr;
    EXPECT_TRUE(uid_builder.Finish(&uid_arr).ok());
    EXPECT_TRUE(pwd_builder.Finish(&pwd_arr).ok());
    EXPECT_TRUE(roles_builder.Finish(&roles_arr).ok());
    EXPECT_TRUE(email_builder.Finish(&email_arr).ok());

    auto schema = arrow::schema({
        arrow::field("user_id",       arrow::utf8()),
        arrow::field("password_hash", arrow::utf8()),
        arrow::field("roles",         arrow::utf8()),
        arrow::field("email",         arrow::utf8())
    });

    return arrow::RecordBatch::Make(
        schema,
        static_cast<int64_t>(user_ids.size()),
        {uid_arr, pwd_arr, roles_arr, email_arr}
    );
}

// Convenience: SHA-256 hex string via OpenSSL (mirrors hashPassword in plugin)
std::string Sha256Hex(const std::string& input) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  hash_len = 0;
    EVP_MD_CTX*   ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, input.c_str(), input.size());
    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    EVP_MD_CTX_free(ctx);

    std::ostringstream ss = {};
    for (unsigned int i = 0; i < hash_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class ArrowUserRegistrationPluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        ArrowUserRegistrationPlugin::Config cfg;
        cfg.arrow_source_uri = "memory://test";
        plugin_ = std::make_unique<ArrowUserRegistrationPlugin>(cfg);
    }

    std::unique_ptr<ArrowUserRegistrationPlugin> plugin_;
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// T1: bulkSyncFromArrow – 1000-user batch is accepted and fully upserted
TEST_F(ArrowUserRegistrationPluginTest, BulkSync1000Users) {
    const int N = 1000;

    std::vector<std::string> user_ids, password_hashes, roles, emails;
    user_ids.reserve(N);
    password_hashes.reserve(N);
    roles.reserve(N);
    emails.reserve(N);

    for (int i = 0; i < N; ++i) {
        user_ids.push_back("user_" + std::to_string(i));
        password_hashes.push_back(Sha256Hex("password_" + std::to_string(i)));
        roles.push_back(i % 2 == 0 ? "admin,readonly" : "readonly");
        emails.push_back("user_" + std::to_string(i) + "@example.com");
    }

    auto batch = BuildUserBatch(user_ids, password_hashes, roles, emails);
    ASSERT_NE(batch, nullptr);
    EXPECT_EQ(batch->num_rows(), N);

    auto result = plugin_->bulkSyncFromArrow(*batch);
    ASSERT_TRUE(result.has_value()) << "bulkSyncFromArrow failed: "
                                    << result.error().message();
    EXPECT_EQ(*result, static_cast<size_t>(N));
}

// T2: syncUsers returns all users after a bulkSyncFromArrow call
TEST_F(ArrowUserRegistrationPluginTest, SyncUsersReturnsAllAfterBulkSync) {
    const int N = 50;

    std::vector<std::string> user_ids, password_hashes, roles, emails;
    for (int i = 0; i < N; ++i) {
        user_ids.push_back("sync_user_" + std::to_string(i));
        password_hashes.push_back(Sha256Hex("pw_" + std::to_string(i)));
        roles.push_back("readonly");
        emails.push_back("sync_" + std::to_string(i) + "@test.com");
    }

    auto batch = BuildUserBatch(user_ids, password_hashes, roles, emails);
    auto sync_result = plugin_->bulkSyncFromArrow(*batch);
    ASSERT_TRUE(sync_result.has_value());

    auto users_result = plugin_->syncUsers();
    ASSERT_TRUE(users_result.has_value());
    EXPECT_EQ(users_result->size(), static_cast<size_t>(N));
}

// T3: authenticateFromArrow succeeds for a user synced via bulkSyncFromArrow
TEST_F(ArrowUserRegistrationPluginTest, AuthenticateAfterBulkSync) {
    // Sync a single user with a known pre-hashed password
    const std::string uid  = "alice@example.com";
    const std::string pass = "super_secret";
    const std::string hash = Sha256Hex(pass);

    auto batch = BuildUserBatch({uid}, {hash}, {"admin,readonly"}, {"alice@example.com"});
    auto sync_result = plugin_->bulkSyncFromArrow(*batch);
    ASSERT_TRUE(sync_result.has_value());

    // authenticateFromArrow should verify plain-text password against stored hash
    auto auth_result = plugin_->authenticateFromArrow(uid, pass);
    ASSERT_TRUE(auth_result.has_value())
        << "Authentication failed: " << auth_result.error().message();

    EXPECT_EQ(auth_result->user_id, uid);
    EXPECT_EQ(auth_result->source, "arrow");
    ASSERT_EQ(auth_result->roles.size(), 2u);
    EXPECT_EQ(auth_result->roles[0], "admin");
    EXPECT_EQ(auth_result->roles[1], "readonly");
    EXPECT_EQ(auth_result->attributes.at("email"), "alice@example.com");

    // Wrong credentials must be rejected for the same user
    auto bad_auth = plugin_->authenticateFromArrow(uid, "wrong_password");
    EXPECT_FALSE(bad_auth.has_value())
        << "Expected authentication to fail for wrong credentials";
}

// T4: authenticateUser (interface method) works for 1000 bulk-synced users
TEST_F(ArrowUserRegistrationPluginTest, AuthenticateUserAfterBulkSync1000) {
    const int N = 1000;

    std::vector<std::string> user_ids, password_hashes, roles, emails;
    user_ids.reserve(N);
    password_hashes.reserve(N);
    roles.reserve(N);
    emails.reserve(N);

    for (int i = 0; i < N; ++i) {
        user_ids.push_back("bulk_user_" + std::to_string(i));
        password_hashes.push_back(Sha256Hex("bulk_pw_" + std::to_string(i)));
        roles.push_back("readonly");
        emails.push_back("bulk_" + std::to_string(i) + "@example.com");
    }

    auto batch = BuildUserBatch(user_ids, password_hashes, roles, emails);
    auto sync_result = plugin_->bulkSyncFromArrow(*batch);
    ASSERT_TRUE(sync_result.has_value());

    // Spot-check 10 users spread across the range
    for (int i = 0; i < N; i += 100) {
        const std::string uid  = "bulk_user_" + std::to_string(i);
        const std::string pass = "bulk_pw_" + std::to_string(i);

        auto auth = plugin_->authenticateUser(uid, pass);
        ASSERT_TRUE(auth.has_value())
            << "authenticateUser failed for " << uid << ": "
            << auth.error().message();
        EXPECT_EQ(auth->user_id, uid);
    }
}

// T5: authenticateFromArrow fails for an unknown user
TEST_F(ArrowUserRegistrationPluginTest, AuthenticateUnknownUserFails) {
    auto result = plugin_->authenticateFromArrow("nonexistent@example.com", "any_password");
    EXPECT_FALSE(result.has_value());
}

// T6: authenticateFromArrow fails for wrong credentials
TEST_F(ArrowUserRegistrationPluginTest, AuthenticateWrongCredentialsFails) {
    const std::string uid  = "bob@example.com";
    const std::string pass = "correct_password";
    const std::string hash = Sha256Hex(pass);

    auto batch = BuildUserBatch({uid}, {hash}, {"readonly"}, {"bob@example.com"});
    ASSERT_TRUE(plugin_->bulkSyncFromArrow(*batch).has_value());

    auto result = plugin_->authenticateFromArrow(uid, "wrong_password");
    EXPECT_FALSE(result.has_value());
}

// T7: bulkSyncFromArrow fails when user_id column is missing
TEST_F(ArrowUserRegistrationPluginTest, BulkSyncFailsMissingUserIdColumn) {
    arrow::StringBuilder pwd_builder;
    EXPECT_TRUE(pwd_builder.Append("hash1").ok());
    std::shared_ptr<arrow::Array> pwd_arr;
    EXPECT_TRUE(pwd_builder.Finish(&pwd_arr).ok());

    auto schema = arrow::schema({arrow::field("password_hash", arrow::utf8())});
    auto batch  = arrow::RecordBatch::Make(schema, 1, {pwd_arr});

    auto result = plugin_->bulkSyncFromArrow(*batch);
    EXPECT_FALSE(result.has_value());
}

// T8: bulkSyncFromArrow upserts existing user (idempotent re-sync)
TEST_F(ArrowUserRegistrationPluginTest, BulkSyncUpsertsExistingUser) {
    const std::string uid = "charlie@example.com";

    // First sync: role = readonly
    auto batch1 = BuildUserBatch({uid}, {Sha256Hex("pw")}, {"readonly"}, {uid});
    ASSERT_TRUE(plugin_->bulkSyncFromArrow(*batch1).has_value());

    // Second sync: role updated to admin
    auto batch2 = BuildUserBatch({uid}, {Sha256Hex("pw")}, {"admin"}, {uid});
    ASSERT_TRUE(plugin_->bulkSyncFromArrow(*batch2).has_value());

    // Store should reflect latest data
    auto update = plugin_->updateUser(uid);
    ASSERT_TRUE(update.has_value());
    ASSERT_EQ(update->roles.size(), 1u);
    EXPECT_EQ(update->roles[0], "admin");
}

// T9: registerUser stores user and authenticateUser can verify it
TEST_F(ArrowUserRegistrationPluginTest, RegisterUserThenAuthenticate) {
    const std::string uid  = "dave@example.com";
    const std::string pass = "dave_pass";

    auto reg = plugin_->registerUser(uid, pass, {{"department", "engineering"}});
    ASSERT_TRUE(reg.has_value()) << reg.error().message();
    EXPECT_EQ(reg->user_id, uid);
    EXPECT_EQ(reg->source, "arrow");
    EXPECT_FALSE(reg->roles.empty());

    auto auth = plugin_->authenticateUser(uid, pass);
    ASSERT_TRUE(auth.has_value()) << auth.error().message();
    EXPECT_EQ(auth->user_id, uid);
}

// T10: updateUser returns user data after registration
TEST_F(ArrowUserRegistrationPluginTest, UpdateUserReturnsStoredData) {
    const std::string uid = "eve@example.com";
    ASSERT_TRUE(plugin_->registerUser(uid, "password", {}).has_value());

    auto update = plugin_->updateUser(uid);
    ASSERT_TRUE(update.has_value());
    EXPECT_EQ(update->user_id, uid);
    EXPECT_EQ(update->source, "arrow");
}

// T11: updateUser fails for unknown user
TEST_F(ArrowUserRegistrationPluginTest, UpdateUserFailsForUnknownUser) {
    auto result = plugin_->updateUser("ghost@example.com");
    EXPECT_FALSE(result.has_value());
}

// T12: bulkSyncFromArrow – email attribute stored correctly
TEST_F(ArrowUserRegistrationPluginTest, BulkSyncEmailStoredAsAttribute) {
    const std::string uid   = "frank@example.com";
    const std::string email = "frank@example.com";

    auto batch = BuildUserBatch({uid}, {Sha256Hex("frankpass")}, {"readonly"}, {email});
    ASSERT_TRUE(plugin_->bulkSyncFromArrow(*batch).has_value());

    auto update = plugin_->updateUser(uid);
    ASSERT_TRUE(update.has_value());
    ASSERT_NE(update->attributes.count("email"), 0u);
    EXPECT_EQ(update->attributes.at("email"), email);
}

// T13: bulkSyncFromArrow – source metadata set correctly
TEST_F(ArrowUserRegistrationPluginTest, BulkSyncSourceMetadataSetCorrectly) {
    ArrowUserRegistrationPlugin::Config cfg;
    cfg.arrow_source_uri = "file:///data/users.parquet";
    auto plugin = std::make_unique<ArrowUserRegistrationPlugin>(cfg);

    auto batch = BuildUserBatch({"g@test.com"}, {Sha256Hex("pw")}, {"readonly"}, {"g@test.com"});
    ASSERT_TRUE(plugin->bulkSyncFromArrow(*batch).has_value());

    auto update = plugin->updateUser("g@test.com");
    ASSERT_TRUE(update.has_value());
    EXPECT_EQ(update->source, "arrow");
    EXPECT_EQ(update->source_uri, "file:///data/users.parquet");
}

} // namespace
} // namespace security
} // namespace themis

#else // THEMIS_ENABLE_ARROW

TEST(ArrowUserRegistrationPluginTest, ArrowNotEnabled) {
    GTEST_SKIP() << "THEMIS_ENABLE_ARROW not defined; Arrow plugin tests skipped.";
}

#endif // THEMIS_ENABLE_ARROW
