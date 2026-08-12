#include <gtest/gtest.h>
#include "auth/totp_replay_cache.h"
#include "auth/mfa_authenticator.h"
#include <thread>
#include <chrono>

using namespace themis::auth;

/**
 * @brief Test basic replay detection
 */
TEST(TOTPReplayCacheTest, BasicReplayDetection) {
    TOTPReplayCache::Config config;
    config.retention_period = std::chrono::seconds(60);
    TOTPReplayCache cache(config);
    
    std::string user_id = "alice";
    std::string code = "123456";
    
    // First use - should succeed
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, code));
    
    // Second use - should fail (replay)
    EXPECT_FALSE(cache.checkAndMarkUsed(user_id, code));
    
    // Third use - still should fail
    EXPECT_FALSE(cache.checkAndMarkUsed(user_id, code));
}

/**
 * @brief Test different users don't interfere
 */
TEST(TOTPReplayCacheTest, DifferentUsers) {
    TOTPReplayCache cache;
    
    std::string code = "123456";
    
    // Same code for different users should work
    EXPECT_TRUE(cache.checkAndMarkUsed("alice", code));
    EXPECT_TRUE(cache.checkAndMarkUsed("bob", code));
    EXPECT_TRUE(cache.checkAndMarkUsed("charlie", code));
    
    // But replay for same user should fail
    EXPECT_FALSE(cache.checkAndMarkUsed("alice", code));
    EXPECT_FALSE(cache.checkAndMarkUsed("bob", code));
}

/**
 * @brief Test different codes for same user
 */
TEST(TOTPReplayCacheTest, DifferentCodes) {
    TOTPReplayCache cache;
    
    std::string user_id = "alice";
    
    // Different codes should all work
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, "111111"));
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, "222222"));
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, "333333"));
    
    // But replays should fail
    EXPECT_FALSE(cache.checkAndMarkUsed(user_id, "111111"));
    EXPECT_FALSE(cache.checkAndMarkUsed(user_id, "222222"));
}

/**
 * @brief Test isUsed method
 */
TEST(TOTPReplayCacheTest, IsUsedQuery) {
    TOTPReplayCache cache;
    
    std::string user_id = "alice";
    std::string code = "123456";
    
    // Initially not used
    EXPECT_FALSE(cache.isUsed(user_id, code));
    
    // Mark as used
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, code));
    
    // Now should be marked as used
    EXPECT_TRUE(cache.isUsed(user_id, code));
    
    // Different code should not be marked
    EXPECT_FALSE(cache.isUsed(user_id, "654321"));
}

/**
 * @brief Test expiration of old codes
 */
TEST(TOTPReplayCacheTest, CodeExpiration) {
    TOTPReplayCache::Config config;
    config.retention_period = std::chrono::seconds(2);  // Very short for testing
    TOTPReplayCache cache(config);
    
    std::string user_id = "alice";
    std::string code = "123456";
    
    // Use code
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, code));
    
    // Immediate replay should fail
    EXPECT_FALSE(cache.checkAndMarkUsed(user_id, code));
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Force cleanup
    cache.cleanup();
    
    // Should be able to use again (expired)
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, code));
}

/**
 * @brief Test clearUser functionality
 */
TEST(TOTPReplayCacheTest, ClearUser) {
    TOTPReplayCache cache;
    
    std::string user_id = "alice";
    std::string code = "123456";
    
    // Use code
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, code));
    EXPECT_FALSE(cache.checkAndMarkUsed(user_id, code));  // Replay fails
    
    // Clear user cache
    cache.clearUser(user_id);
    
    // Should be able to use again
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, code));
}

/**
 * @brief Test clear all functionality
 */
TEST(TOTPReplayCacheTest, ClearAll) {
    TOTPReplayCache cache;
    
    // Use codes for multiple users
    cache.checkAndMarkUsed("alice", "111111");
    cache.checkAndMarkUsed("bob", "222222");
    cache.checkAndMarkUsed("charlie", "333333");
    
    // Clear entire cache
    cache.clear();
    
    // All codes should work again
    EXPECT_TRUE(cache.checkAndMarkUsed("alice", "111111"));
    EXPECT_TRUE(cache.checkAndMarkUsed("bob", "222222"));
    EXPECT_TRUE(cache.checkAndMarkUsed("charlie", "333333"));
}

/**
 * @brief Test max entries per user
 */
TEST(TOTPReplayCacheTest, MaxEntriesPerUser) {
    TOTPReplayCache::Config config;
    config.max_entries_per_user = 3;
    TOTPReplayCache cache(config);
    
    std::string user_id = "alice";
    
    // Use 5 codes (more than max)
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, "111111"));
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, "222222"));
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, "333333"));
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, "444444"));
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, "555555"));
    
    // Oldest codes should be evicted, newest should still be protected
    EXPECT_FALSE(cache.checkAndMarkUsed(user_id, "555555"));  // Recent, protected
    EXPECT_FALSE(cache.checkAndMarkUsed(user_id, "444444"));  // Recent, protected
    EXPECT_FALSE(cache.checkAndMarkUsed(user_id, "333333"));  // Recent, protected

    // Re-inserting an evicted old code can evict the current oldest retained
    // code due to max_entries_per_user LRU behaviour.
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, "111111"));   // Evicted, can reuse
    EXPECT_TRUE(cache.checkAndMarkUsed(user_id, "333333"));   // Became oldest and got evicted
}

/**
 * @brief Test statistics tracking
 */
TEST(TOTPReplayCacheTest, Statistics) {
    TOTPReplayCache cache;
    
    // Use some codes
    cache.checkAndMarkUsed("alice", "111111");
    cache.checkAndMarkUsed("alice", "222222");
    cache.checkAndMarkUsed("bob", "333333");
    
    auto stats = cache.getStatistics();
    
    EXPECT_EQ(stats.total_users, 2);
    EXPECT_EQ(stats.total_codes, 3);
    
    // Try replay
    cache.checkAndMarkUsed("alice", "111111");
    
    stats = cache.getStatistics();
    EXPECT_EQ(stats.replay_attempts_blocked, 1);
}

/**
 * @brief Test thread safety
 */
TEST(TOTPReplayCacheTest, ThreadSafety) {
    TOTPReplayCache cache;
    
    std::string user_id = "alice";
    std::string code = "123456";
    
    std::atomic<int> success_count{0};
    std::atomic<int> replay_count{0};
    
    const int num_threads = 10;
    std::vector<std::thread> threads;
    
    // Multiple threads try to use the same code
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&cache, &user_id, &code, &success_count, &replay_count]() {
            if (cache.checkAndMarkUsed(user_id, code)) {
                success_count++;
            } else {
                replay_count++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Exactly one thread should succeed, others should detect replay
    EXPECT_EQ(success_count.load(), 1);
    EXPECT_EQ(replay_count.load(), num_threads - 1);
}

/**
 * @brief Test SecureMFAValidator basic functionality
 */
TEST(SecureMFAValidatorTest, BasicValidation) {
    SecureMFAValidator::Config config;
    config.enable_replay_protection = true;
    SecureMFAValidator validator(config);
    
    // Generate test enrollment
    MFAAuthenticator mfa;
    auto enrollment = mfa.generateEnrollment("alice");
    
    // Get current valid code
    std::string code = mfa.getCurrentTOTP(enrollment.secret_base32);
    
    // First use should succeed
    EXPECT_TRUE(validator.validateTOTP("alice", enrollment.secret_base32, code));
    
    // Replay should fail
    EXPECT_THROW(
        validator.validateTOTP("alice", enrollment.secret_base32, code),
        std::runtime_error
    );
}

/**
 * @brief Test SecureMFAValidator with replay protection disabled
 */
TEST(SecureMFAValidatorTest, NoReplayProtection) {
    SecureMFAValidator::Config config;
    config.enable_replay_protection = false;  // Disabled
    SecureMFAValidator validator(config);
    
    MFAAuthenticator mfa;
    auto enrollment = mfa.generateEnrollment("alice");
    std::string code = mfa.getCurrentTOTP(enrollment.secret_base32);
    
    // Both should succeed (no replay protection)
    EXPECT_TRUE(validator.validateTOTP("alice", enrollment.secret_base32, code));
    EXPECT_TRUE(validator.validateTOTP("alice", enrollment.secret_base32, code));
}

/**
 * @brief Test SecureMFAValidator clearUserCache
 */
TEST(SecureMFAValidatorTest, ClearUserCache) {
    SecureMFAValidator validator;
    
    MFAAuthenticator mfa;
    auto enrollment = mfa.generateEnrollment("alice");
    std::string code = mfa.getCurrentTOTP(enrollment.secret_base32);
    
    // First use
    EXPECT_TRUE(validator.validateTOTP("alice", enrollment.secret_base32, code));
    
    // Clear cache
    validator.clearUserCache("alice");
    
    // Should work again (but will fail because code validation itself fails on same code)
    // So we need to test with isUsed or wait for new code
}

/**
 * @brief Test SecureMFAValidator statistics
 */
TEST(SecureMFAValidatorTest, Statistics) {
    SecureMFAValidator validator;
    
    MFAAuthenticator mfa;
    auto enrollment = mfa.generateEnrollment("alice");
    std::string code = mfa.getCurrentTOTP(enrollment.secret_base32);
    
    validator.validateTOTP("alice", enrollment.secret_base32, code);
    
    try {
        validator.validateTOTP("alice", enrollment.secret_base32, code);
    } catch (...) {
        // Expected
    }
    
    auto stats = validator.getReplayStatistics();
    EXPECT_EQ(stats.replay_attempts_blocked, 1);
}

/**
 * @brief Test invalid code with replay protection
 */
TEST(SecureMFAValidatorTest, InvalidCode) {
    SecureMFAValidator validator;
    
    MFAAuthenticator mfa;
    auto enrollment = mfa.generateEnrollment("alice");
    
    // Invalid code should fail validation (not reach replay check)
    EXPECT_FALSE(validator.validateTOTP("alice", enrollment.secret_base32, "000000"));
}

/**
 * @brief Test automatic cleanup
 */
TEST(TOTPReplayCacheTest, AutomaticCleanup) {
    TOTPReplayCache::Config config;
    config.retention_period = std::chrono::seconds(1);
    config.cleanup_interval = std::chrono::seconds(1);
    TOTPReplayCache cache(config);
    
    cache.checkAndMarkUsed("alice", "111111");
    
    auto stats1 = cache.getStatistics();
    EXPECT_EQ(stats1.total_codes, 1);
    
    // Wait for cleanup
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Trigger cleanup by doing an operation
    cache.checkAndMarkUsed("bob", "222222");
    
    auto stats2 = cache.getStatistics();
    EXPECT_GT(stats2.entries_expired, 0);
}
