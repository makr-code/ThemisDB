#include <gtest/gtest.h>
#include "auth/auth_rate_limiter.h"
#include <thread>
#include <chrono>

using namespace themis::auth;

/**
 * @brief Test basic rate limiting for IP addresses
 */
TEST(AuthRateLimiterTest, IPRateLimiting) {
    AuthRateLimitConfig config;
    config.max_attempts_per_ip_per_minute = 3;  // Very low for testing
    config.enable_account_lockout = false;       // Disable lockout for this test
    
    AuthRateLimiter limiter(config);
    
    std::string ip = "192.168.1.100";
    
    // First 3 attempts should succeed
    EXPECT_TRUE(limiter.allowAuthAttempt(ip));
    EXPECT_TRUE(limiter.allowAuthAttempt(ip));
    EXPECT_TRUE(limiter.allowAuthAttempt(ip));
    
    // 4th attempt should be rate limited
    EXPECT_FALSE(limiter.allowAuthAttempt(ip));
    EXPECT_FALSE(limiter.allowAuthAttempt(ip));
    
    // Different IP should not be affected
    EXPECT_TRUE(limiter.allowAuthAttempt("192.168.1.101"));
}

/**
 * @brief Test per-user rate limiting
 */
TEST(AuthRateLimiterTest, UserRateLimiting) {
    AuthRateLimitConfig config;
    config.max_attempts_per_user_per_minute = 3;  // Very low for testing
    config.enable_ip_rate_limiting = false;        // Disable IP limiting
    config.enable_account_lockout = false;         // Disable lockout
    
    AuthRateLimiter limiter(config);
    
    std::string user = "alice";
    std::string ip = "192.168.1.100";
    
    // First 3 attempts should succeed
    EXPECT_TRUE(limiter.allowAuthAttempt(ip, user));
    EXPECT_TRUE(limiter.allowAuthAttempt(ip, user));
    EXPECT_TRUE(limiter.allowAuthAttempt(ip, user));
    
    // 4th attempt should be rate limited
    EXPECT_FALSE(limiter.allowAuthAttempt(ip, user));
    
    // Different user should not be affected
    EXPECT_TRUE(limiter.allowAuthAttempt(ip, "bob"));
}

/**
 * @brief Test account lockout after failed attempts
 */
TEST(AuthRateLimiterTest, AccountLockout) {
    AuthRateLimitConfig config;
    config.lockout_failed_attempts = 3;           // Lock after 3 failures
    config.lockout_window = std::chrono::minutes(5);
    config.lockout_duration = std::chrono::minutes(10);
    config.enable_ip_rate_limiting = false;        // Disable for this test
    config.enable_user_rate_limiting = false;
    
    AuthRateLimiter limiter(config);
    
    std::string user = "alice";
    std::string ip = "192.168.1.100";
    
    // Account should not be locked initially
    EXPECT_FALSE(limiter.isAccountLocked(user));
    
    // Record 3 failed attempts
    limiter.recordFailedAuth(user, ip, "invalid_password");
    limiter.recordFailedAuth(user, ip, "invalid_password");
    limiter.recordFailedAuth(user, ip, "invalid_password");
    
    // Account should now be locked
    EXPECT_TRUE(limiter.isAccountLocked(user));
    
    // Authentication attempts should be blocked
    EXPECT_FALSE(limiter.allowAuthAttempt(ip, user));
    
    // Check lockout info
    auto info = limiter.getLockoutInfo(user);
    ASSERT_TRUE(info.has_value());
    EXPECT_TRUE(info->is_locked);
    EXPECT_EQ(info->failed_attempts, 3);
}

/**
 * @brief Test manual account unlock
 */
TEST(AuthRateLimiterTest, ManualUnlock) {
    AuthRateLimitConfig config;
    config.lockout_failed_attempts = 2;
    config.enable_ip_rate_limiting = false;
    config.enable_user_rate_limiting = false;
    
    AuthRateLimiter limiter(config);
    
    std::string user = "alice";
    std::string ip = "192.168.1.100";
    
    // Lock the account
    limiter.recordFailedAuth(user, ip, "invalid_password");
    limiter.recordFailedAuth(user, ip, "invalid_password");
    
    EXPECT_TRUE(limiter.isAccountLocked(user));
    
    // Unlock the account
    bool unlocked = limiter.unlockAccount(user);
    EXPECT_TRUE(unlocked);
    
    // Account should no longer be locked
    EXPECT_FALSE(limiter.isAccountLocked(user));
    
    // Authentication should now be allowed
    EXPECT_TRUE(limiter.allowAuthAttempt(ip, user));
}

/**
 * @brief Test successful auth resets failure count
 */
TEST(AuthRateLimiterTest, SuccessfulAuthResetFailures) {
    AuthRateLimitConfig config;
    config.lockout_failed_attempts = 3;
    config.enable_ip_rate_limiting = false;
    config.enable_user_rate_limiting = false;
    
    AuthRateLimiter limiter(config);
    
    std::string user = "alice";
    std::string ip = "192.168.1.100";
    
    // Record 2 failed attempts
    limiter.recordFailedAuth(user, ip, "invalid_password");
    limiter.recordFailedAuth(user, ip, "invalid_password");
    
    // Account should not be locked yet (needs 3)
    EXPECT_FALSE(limiter.isAccountLocked(user));
    
    // Record successful auth
    limiter.recordSuccessfulAuth(user, ip);
    
    // Now record 2 more failures
    limiter.recordFailedAuth(user, ip, "invalid_password");
    limiter.recordFailedAuth(user, ip, "invalid_password");
    
    // Account should still not be locked (counter was reset)
    EXPECT_FALSE(limiter.isAccountLocked(user));
}

/**
 * @brief Test IP whitelist bypasses rate limiting
 */
TEST(AuthRateLimiterTest, IPWhitelist) {
    AuthRateLimitConfig config;
    config.max_attempts_per_ip_per_minute = 2;
    config.whitelist_ips = {"192.168.1.100", "10.0.0.1"};
    config.enable_account_lockout = false;
    
    AuthRateLimiter limiter(config);
    
    std::string whitelisted_ip = "192.168.1.100";
    std::string normal_ip = "192.168.1.200";
    
    // Whitelisted IP should never be rate limited
    EXPECT_TRUE(limiter.isWhitelisted(whitelisted_ip));
    for (int i = 0; i < 10; i++) {
        EXPECT_TRUE(limiter.allowAuthAttempt(whitelisted_ip));
    }
    
    // Normal IP should be rate limited
    EXPECT_FALSE(limiter.isWhitelisted(normal_ip));
    EXPECT_TRUE(limiter.allowAuthAttempt(normal_ip));
    EXPECT_TRUE(limiter.allowAuthAttempt(normal_ip));
    EXPECT_FALSE(limiter.allowAuthAttempt(normal_ip));  // 3rd attempt blocked
}

/**
 * @brief Test statistics tracking
 */
TEST(AuthRateLimiterTest, Statistics) {
    AuthRateLimitConfig config;
    config.max_attempts_per_ip_per_minute = 3;
    config.lockout_failed_attempts = 2;
    
    AuthRateLimiter limiter(config);
    
    std::string ip = "192.168.1.100";
    std::string user = "alice";
    
    // Make some auth attempts
    limiter.allowAuthAttempt(ip, user);
    limiter.allowAuthAttempt(ip, user);
    limiter.allowAuthAttempt(ip, user);
    limiter.allowAuthAttempt(ip, user);  // This should be rate limited
    
    // Record some auth results
    limiter.recordSuccessfulAuth(user, ip);
    limiter.recordFailedAuth(user, ip, "invalid_password");
    limiter.recordFailedAuth(user, ip, "invalid_password");  // This locks account
    
    auto stats = limiter.getStatistics();
    
    EXPECT_EQ(stats.total_auth_attempts, 4);
    EXPECT_EQ(stats.allowed_attempts, 3);
    EXPECT_EQ(stats.rate_limited_attempts, 1);
    EXPECT_EQ(stats.successful_auths, 1);
    EXPECT_EQ(stats.failed_auths, 2);
    EXPECT_EQ(stats.currently_locked_accounts, 1);
}

/**
 * @brief Test lockout window behavior
 */
TEST(AuthRateLimiterTest, LockoutWindow) {
    AuthRateLimitConfig config;
    config.lockout_failed_attempts = 3;
    config.lockout_window = std::chrono::minutes(0);  // immediate expiry window for testing
    config.enable_ip_rate_limiting = false;
    config.enable_user_rate_limiting = false;
    
    AuthRateLimiter limiter(config);
    
    std::string user = "alice";
    std::string ip = "192.168.1.100";
    
    // Record 2 failures
    limiter.recordFailedAuth(user, ip, "invalid_password");
    limiter.recordFailedAuth(user, ip, "invalid_password");
    
    // Wait for window to expire
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Next failure should not cause lockout (old failures outside window)
    limiter.recordFailedAuth(user, ip, "invalid_password");
    
    // Account should not be locked (only 1 failure in current window)
    EXPECT_FALSE(limiter.isAccountLocked(user));
}

/**
 * @brief Test retry-after header value
 */
TEST(AuthRateLimiterTest, RetryAfter) {
    AuthRateLimitConfig config;
    config.max_attempts_per_ip_per_minute = 2;
    config.enable_account_lockout = false;
    
    AuthRateLimiter limiter(config);
    
    std::string ip = "192.168.1.100";
    
    // Exhaust rate limit
    limiter.allowAuthAttempt(ip);
    limiter.allowAuthAttempt(ip);
    limiter.allowAuthAttempt(ip);  // Rate limited
    
    // Should get a retry-after value
    uint32_t retry_after = limiter.getRetryAfter(ip);
    EXPECT_GT(retry_after, 0);
}

/**
 * @brief Test reset functionality
 */
TEST(AuthRateLimiterTest, Reset) {
    AuthRateLimitConfig config;
    config.lockout_failed_attempts = 2;
    
    AuthRateLimiter limiter(config);
    
    std::string user = "alice";
    std::string ip = "192.168.1.100";
    
    // Lock account
    limiter.recordFailedAuth(user, ip, "invalid_password");
    limiter.recordFailedAuth(user, ip, "invalid_password");
    EXPECT_TRUE(limiter.isAccountLocked(user));
    
    // Reset
    limiter.reset();
    
    // Account should no longer be locked
    EXPECT_FALSE(limiter.isAccountLocked(user));
    
    // Statistics should be reset
    auto stats = limiter.getStatistics();
    EXPECT_EQ(stats.total_auth_attempts, 0);
    EXPECT_EQ(stats.failed_auths, 0);
}

/**
 * @brief Test account lockout manager directly
 */
TEST(AccountLockoutManagerTest, BasicFunctionality) {
    AuthRateLimitConfig config;
    config.lockout_failed_attempts = 3;
    config.lockout_window = std::chrono::minutes(10);
    config.lockout_duration = std::chrono::minutes(15);
    
    AccountLockoutManager manager(config);
    
    std::string user = "bob";
    std::string ip = "10.0.0.1";
    
    // Record failures
    EXPECT_FALSE(manager.recordFailedAttempt(user, ip, "reason1"));
    EXPECT_FALSE(manager.recordFailedAttempt(user, ip, "reason2"));
    EXPECT_TRUE(manager.recordFailedAttempt(user, ip, "reason3"));  // Locks account
    
    // Check lockout
    EXPECT_TRUE(manager.isAccountLocked(user));
    EXPECT_EQ(manager.getLockedAccountCount(), 1);
    
    // Get lockout info
    auto info = manager.getLockoutInfo(user);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->failed_attempts, 3);
    EXPECT_EQ(info->recent_failures.size(), 3);
}

/**
 * @brief Test combined IP and user rate limiting
 */
TEST(AuthRateLimiterTest, CombinedLimits) {
    AuthRateLimitConfig config;
    config.max_attempts_per_ip_per_minute = 5;
    config.max_attempts_per_user_per_minute = 3;
    config.enable_account_lockout = false;
    
    AuthRateLimiter limiter(config);
    
    std::string ip = "192.168.1.100";
    std::string user = "alice";
    
    // First 3 attempts succeed (within both limits)
    EXPECT_TRUE(limiter.allowAuthAttempt(ip, user));
    EXPECT_TRUE(limiter.allowAuthAttempt(ip, user));
    EXPECT_TRUE(limiter.allowAuthAttempt(ip, user));
    
    // 4th attempt blocked by user rate limit (even though IP limit not reached)
    EXPECT_FALSE(limiter.allowAuthAttempt(ip, user));
    
    // Different user from same IP should still work (IP has capacity)
    EXPECT_TRUE(limiter.allowAuthAttempt(ip, "bob"));
}
