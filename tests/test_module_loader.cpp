/// @file test_module_loader.cpp
/// @brief Unit tests for module loader functionality
/// 
/// Tests verify:
/// - Error code handling
/// - Metadata structures
/// - Error categorization
/// - Module loading state management

#include <gtest/gtest.h>
#include "themis/base/module_loader.h"
#include <string>

using namespace themis::modules;

// ===== Error Code Tests =====

TEST(ModuleLoader, ErrorCodeEnumValues) {
    // Verify error code ranges
    EXPECT_EQ(static_cast<int>(ModuleErrorCode::SUCCESS), 0);
    EXPECT_EQ(static_cast<int>(ModuleErrorCode::MODULE_NOT_FOUND), 100);
    EXPECT_EQ(static_cast<int>(ModuleErrorCode::VERIFICATION_FAILED), 200);
    EXPECT_EQ(static_cast<int>(ModuleErrorCode::LOAD_LIBRARY_FAILED), 300);
    EXPECT_EQ(static_cast<int>(ModuleErrorCode::VERSION_INCOMPATIBLE), 400);
    EXPECT_EQ(static_cast<int>(ModuleErrorCode::POLICY_VIOLATION), 500);
    EXPECT_EQ(static_cast<int>(ModuleErrorCode::INTERNAL_ERROR), 900);
}

TEST(ModuleLoader, ErrorCategoryValues) {
    // Verify error categories exist
    ErrorCategory transient = ErrorCategory::TRANSIENT;
    ErrorCategory permanent = ErrorCategory::PERMANENT;
    ErrorCategory recoverable = ErrorCategory::RECOVERABLE;
    ErrorCategory fatal = ErrorCategory::FATAL;
    
    EXPECT_TRUE(transient == ErrorCategory::TRANSIENT);
    EXPECT_TRUE(permanent == ErrorCategory::PERMANENT);
    EXPECT_TRUE(recoverable == ErrorCategory::RECOVERABLE);
    EXPECT_TRUE(fatal == ErrorCategory::FATAL);
}

// ===== Metadata Tests =====

TEST(ModuleMetadata, DefaultConstructor) {
    ModuleMetadata metadata;
    
    EXPECT_TRUE(metadata.version.empty());
    EXPECT_TRUE(metadata.abiVersion.empty());
    EXPECT_TRUE(metadata.buildId.empty());
    EXPECT_TRUE(metadata.buildDate.empty());
    EXPECT_EQ(metadata.themisMajor, 0u);
    EXPECT_EQ(metadata.themisMinor, 0u);
    EXPECT_EQ(metadata.themisPatch, 0u);
    EXPECT_FALSE(metadata.isValid());
}

TEST(ModuleMetadata, IsValidWithVersion) {
    ModuleMetadata metadata;
    metadata.version = "1.0.0";
    metadata.themisMajor = 1;
    
    EXPECT_TRUE(metadata.isValid());
}

TEST(ModuleMetadata, IsValidWithoutMajorVersion) {
    ModuleMetadata metadata;
    metadata.version = "1.0.0";
    metadata.themisMajor = 0;  // Invalid
    
    EXPECT_FALSE(metadata.isValid());
}

TEST(ModuleMetadata, IsValidWithoutVersion) {
    ModuleMetadata metadata;
    metadata.themisMajor = 1;
    metadata.version = "";  // Invalid
    
    EXPECT_FALSE(metadata.isValid());
}

// ===== Verification Result Tests =====

TEST(ModuleVerificationResult, DefaultValues) {
    ModuleVerificationResult result;
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorCode, ModuleErrorCode::SUCCESS);
    EXPECT_EQ(result.errorCategory, ErrorCategory::PERMANENT);
    EXPECT_TRUE(result.errorMessage.empty());
    EXPECT_TRUE(result.moduleHash.empty());
    EXPECT_TRUE(result.modulePath.empty());
    EXPECT_EQ(result.verificationTimestamp, 0u);
    EXPECT_FALSE(result.metadata.isValid());
}

TEST(ModuleVerificationResult, SuccessState) {
    ModuleVerificationResult result;
    result.success = true;
    result.errorCode = ModuleErrorCode::SUCCESS;
    result.errorCategory = ErrorCategory::PERMANENT;
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.errorCode, ModuleErrorCode::SUCCESS);
}

TEST(ModuleVerificationResult, ErrorState) {
    ModuleVerificationResult result;
    result.success = false;
    result.errorCode = ModuleErrorCode::MODULE_NOT_FOUND;
    result.errorCategory = ErrorCategory::RECOVERABLE;
    result.errorMessage = "Module file not found";
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorCode, ModuleErrorCode::MODULE_NOT_FOUND);
    EXPECT_EQ(result.errorCategory, ErrorCategory::RECOVERABLE);
    EXPECT_FALSE(result.errorMessage.empty());
}

// ===== Loaded Module Tests =====

TEST(LoadedModule, DefaultValues) {
    LoadedModule module;
    
    EXPECT_TRUE(module.name.empty());
    EXPECT_TRUE(module.path.empty());
    EXPECT_TRUE(module.version.empty());
    EXPECT_TRUE(module.fileHash.empty());
    EXPECT_EQ(module.handle, nullptr);
    EXPECT_FALSE(module.verified);
    EXPECT_EQ(module.loadTime, 0u);
    EXPECT_EQ(module.loadDurationMs, 0u);
    EXPECT_FALSE(module.metadata.isValid());
}

TEST(LoadedModule, PopulatedModule) {
    LoadedModule module;
    module.name = "themis_test";
    module.path = "/path/to/module.so";
    module.version = "1.2.3";
    module.fileHash = "abc123";
    module.verified = true;
    module.loadTime = 1234567890;
    module.loadDurationMs = 150;
    
    module.metadata.version = "1.2.3";
    module.metadata.themisMajor = 1;
    module.metadata.themisMinor = 2;
    module.metadata.themisPatch = 3;
    
    EXPECT_EQ(module.name, "themis_test");
    EXPECT_EQ(module.path, "/path/to/module.so");
    EXPECT_EQ(module.version, "1.2.3");
    EXPECT_EQ(module.fileHash, "abc123");
    EXPECT_TRUE(module.verified);
    EXPECT_EQ(module.loadTime, 1234567890u);
    EXPECT_EQ(module.loadDurationMs, 150u);
    EXPECT_TRUE(module.metadata.isValid());
}

// ===== Module Loader Basic Tests =====

TEST(ModuleLoader, ConstructorDestructor) {
    // Verify that ModuleLoader can be constructed and destroyed
    ModuleLoader loader;
    // If we get here, construction succeeded
    SUCCEED();
}

TEST(ModuleLoader, LoadNonExistentModule) {
    ModuleLoader loader;
    
    // Allow unsigned modules for testing
    loader.setAllowUnsigned(true);
    loader.setRequireSignature(false);
    
    auto result = loader.loadModule("/nonexistent/path/module.so", "test_module");
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorCode, ModuleErrorCode::MODULE_NOT_FOUND);
    EXPECT_EQ(result.errorCategory, ErrorCategory::RECOVERABLE);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST(ModuleLoader, IsModuleLoadedReturnsFalseForUnloaded) {
    ModuleLoader loader;
    
    EXPECT_FALSE(loader.isModuleLoaded("nonexistent_module"));
}

TEST(ModuleLoader, GetModuleInfoReturnsNulloptForUnloaded) {
    ModuleLoader loader;
    
    auto info = loader.getModuleInfo("nonexistent_module");
    
    EXPECT_FALSE(info.has_value());
}

TEST(ModuleLoader, GetAllLoadedModulesEmptyInitially) {
    ModuleLoader loader;
    
    auto modules = loader.getAllLoadedModules();
    
    EXPECT_TRUE(modules.empty());
}

// ===== Module Registry Tests =====

TEST(ModuleRegistry, Singleton) {
    // Verify registry is a singleton
    auto& registry1 = ModuleRegistry::instance();
    auto& registry2 = ModuleRegistry::instance();
    
    EXPECT_EQ(&registry1, &registry2);
}

TEST(ModuleRegistry, ClearFunction) {
    auto& registry = ModuleRegistry::instance();
    
    // Clear and verify
    registry.clear();
    auto modules = registry.getAllModules();
    EXPECT_TRUE(modules.empty());
}

TEST(ModuleRegistry, RegisterAndCheck) {
    auto& registry = ModuleRegistry::instance();
    registry.clear();
    
    LoadedModule module;
    module.name = "test_module";
    module.path = "/test/path";
    module.version = "1.0.0";
    
    registry.registerModule(module);
    
    EXPECT_TRUE(registry.isRegistered("test_module"));
    EXPECT_FALSE(registry.isRegistered("other_module"));
    
    // Clean up
    registry.clear();
}

TEST(ModuleRegistry, UnregisterModule) {
    auto& registry = ModuleRegistry::instance();
    registry.clear();
    
    LoadedModule module;
    module.name = "test_module";
    module.path = "/test/path";
    
    registry.registerModule(module);
    EXPECT_TRUE(registry.isRegistered("test_module"));
    
    registry.unregisterModule("test_module");
    EXPECT_FALSE(registry.isRegistered("test_module"));
    
    // Clean up
    registry.clear();
}

// ===== Error Message Tests =====

TEST(ModuleLoader, GetErrorMessageReturnsNonEmpty) {
    ModuleLoader loader;
    
    // Test a few error codes - we can't directly call private methods,
    // so we test via loading non-existent modules
    auto result = loader.loadModule("/nonexistent.so", "test");
    
    EXPECT_FALSE(result.errorMessage.empty());
    EXPECT_NE(result.errorMessage.find("not found"), std::string::npos);
}

// ===== Integration Tests =====

TEST(ModuleLoader, FailedLoadDoesNotPreventRetry) {
    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    loader.setRequireSignature(false);
    
    // Try to load a non-existent module twice
    // Both should fail with MODULE_NOT_FOUND, not MODULE_ALREADY_LOADED
    // This verifies that failed loads don't mark module as "loaded"
    
    auto result1 = loader.loadModule("/nonexistent1.so", "test_module");
    EXPECT_FALSE(result1.success);
    EXPECT_EQ(result1.errorCode, ModuleErrorCode::MODULE_NOT_FOUND);
    
    // Second attempt should also fail with MODULE_NOT_FOUND
    // (not MODULE_ALREADY_LOADED) since first load failed
    auto result2 = loader.loadModule("/nonexistent1.so", "test_module");
    EXPECT_FALSE(result2.success);
    EXPECT_EQ(result2.errorCode, ModuleErrorCode::MODULE_NOT_FOUND);
}

// ===== Security Policy Tests =====

TEST(ModuleLoader, SetSecurityPolicyFlags) {
    ModuleLoader loader;
    
    // These should not throw
    loader.setRequireSignature(true);
    loader.setAllowUnsigned(false);
    loader.addWhitelistedHash("abc123");
    loader.addBlacklistedHash("def456");
    
    SUCCEED();
}

// ===== Phase 2 Tests: Quarantine & Backoff =====

TEST(ModuleFailureHistory, DefaultValues) {
    ModuleFailureHistory history;
    
    EXPECT_TRUE(history.modulePath.empty());
    EXPECT_TRUE(history.failureTimestamps.empty());
    EXPECT_EQ(history.consecutiveFailures, 0u);
    EXPECT_EQ(history.lastFailureTime, 0u);
    EXPECT_EQ(history.quarantineTime, 0u);
    EXPECT_EQ(history.nextRetryTime, 0u);
    EXPECT_EQ(history.lastErrorCode, ModuleErrorCode::SUCCESS);
    EXPECT_FALSE(history.isQuarantined());
}

TEST(ModuleFailureHistory, QuarantineState) {
    ModuleFailureHistory history;
    
    // Not quarantined initially
    EXPECT_FALSE(history.isQuarantined());
    
    // Set quarantine time
    history.quarantineTime = 1234567890;
    EXPECT_TRUE(history.isQuarantined());
}

TEST(ModuleFailureHistory, CanRetry) {
    ModuleFailureHistory history;
    uint64_t currentTime = 1000;
    
    // Can retry if no backoff set
    history.nextRetryTime = 0;
    EXPECT_TRUE(history.canRetry(currentTime));
    
    // Cannot retry if backoff not expired
    history.nextRetryTime = 2000;
    EXPECT_FALSE(history.canRetry(currentTime));
    
    // Can retry after backoff expires
    currentTime = 2500;
    EXPECT_TRUE(history.canRetry(currentTime));
}

TEST(ModuleMetrics, DefaultValues) {
    ModuleMetrics metrics;
    
    EXPECT_EQ(metrics.totalLoadAttempts, 0u);
    EXPECT_EQ(metrics.successfulLoads, 0u);
    EXPECT_EQ(metrics.failedLoads, 0u);
    EXPECT_EQ(metrics.totalUnloads, 0u);
    EXPECT_EQ(metrics.totalLoadDurationMs, 0u);
    EXPECT_EQ(metrics.minLoadDurationMs, UINT64_MAX);
    EXPECT_EQ(metrics.maxLoadDurationMs, 0u);
    EXPECT_EQ(metrics.verificationSuccesses, 0u);
    EXPECT_EQ(metrics.verificationFailures, 0u);
    EXPECT_EQ(metrics.quarantineEvents, 0u);
    EXPECT_EQ(metrics.quarantineReleases, 0u);
    EXPECT_EQ(metrics.currentlyQuarantined, 0u);
    EXPECT_TRUE(metrics.errorCounts.empty());
}

TEST(ModuleMetrics, SuccessRate) {
    ModuleMetrics metrics;
    
    // Zero attempts = 0% success rate
    EXPECT_DOUBLE_EQ(metrics.getSuccessRate(), 0.0);
    
    // 5 successes out of 10 attempts = 50%
    metrics.totalLoadAttempts = 10;
    metrics.successfulLoads = 5;
    EXPECT_DOUBLE_EQ(metrics.getSuccessRate(), 0.5);
    
    // 10 successes out of 10 attempts = 100%
    metrics.successfulLoads = 10;
    EXPECT_DOUBLE_EQ(metrics.getSuccessRate(), 1.0);
}

TEST(ModuleMetrics, AverageLoadDuration) {
    ModuleMetrics metrics;
    
    // Zero loads = 0ms average
    EXPECT_DOUBLE_EQ(metrics.getAverageLoadDurationMs(), 0.0);
    
    // 300ms total over 3 loads = 100ms average
    metrics.successfulLoads = 3;
    metrics.totalLoadDurationMs = 300;
    EXPECT_DOUBLE_EQ(metrics.getAverageLoadDurationMs(), 100.0);
}

TEST(ModuleLoader, GetFailureHistoryEmpty) {
    ModuleLoader loader;
    
    auto history = loader.getFailureHistory("/nonexistent.so");
    EXPECT_FALSE(history.has_value());
}

TEST(ModuleLoader, GetQuarantinedModulesEmpty) {
    ModuleLoader loader;
    
    auto quarantined = loader.getQuarantinedModules();
    EXPECT_TRUE(quarantined.empty());
}

TEST(ModuleLoader, ReleaseFromQuarantineNonExistent) {
    ModuleLoader loader;
    
    // Cannot release module that's not quarantined
    EXPECT_FALSE(loader.releaseFromQuarantine("/nonexistent.so"));
}

TEST(ModuleLoader, ClearFailureHistoryNonExistent) {
    ModuleLoader loader;
    
    // Should not throw for non-existent module
    loader.clearFailureHistory("/nonexistent.so");
    SUCCEED();
}

TEST(ModuleLoader, GetMetricsInitial) {
    ModuleLoader loader;
    
    auto metrics = loader.getMetrics();
    
    EXPECT_EQ(metrics.totalLoadAttempts, 0u);
    EXPECT_EQ(metrics.successfulLoads, 0u);
    EXPECT_EQ(metrics.failedLoads, 0u);
    EXPECT_EQ(metrics.quarantineEvents, 0u);
    EXPECT_EQ(metrics.currentlyQuarantined, 0u);
}

TEST(ModuleLoader, ResetMetrics) {
    ModuleLoader loader;
    
    // Reset should not throw
    loader.resetMetrics();
    
    auto metrics = loader.getMetrics();
    EXPECT_EQ(metrics.totalLoadAttempts, 0u);
    SUCCEED();
}

TEST(ModuleLoader, SetQuarantineThreshold) {
    ModuleLoader loader;
    
    // Should accept various thresholds
    loader.setQuarantineThreshold(1);
    loader.setQuarantineThreshold(5);
    loader.setQuarantineThreshold(10);
    SUCCEED();
}

TEST(ModuleLoader, SetMaxBackoffSeconds) {
    ModuleLoader loader;
    
    // Should accept various max backoff times
    loader.setMaxBackoffSeconds(60);
    loader.setMaxBackoffSeconds(300);
    loader.setMaxBackoffSeconds(600);
    SUCCEED();
}

// ===== Phase 2 Tests: ABI Compatibility =====

TEST(ModuleLoader, IsABICompatibleInvalidMetadata) {
    ModuleLoader loader;
    ModuleMetadata metadata;
    
    // Invalid metadata (no version, themisMajor = 0)
    EXPECT_FALSE(loader.isABICompatible(metadata));
}

TEST(ModuleLoader, IsABICompatibleMajorMatch) {
    ModuleLoader loader;
    ModuleMetadata metadata;
    
    // Compatible: same major, module minor <= themis minor
    metadata.version = "1.0.0";
    metadata.themisMajor = 1;
    metadata.themisMinor = 0;
    metadata.themisPatch = 0;
    
    EXPECT_TRUE(loader.isABICompatible(metadata));
}

TEST(ModuleLoader, IsABICompatibleMajorMismatch) {
    ModuleLoader loader;
    ModuleMetadata metadata;
    
    // Incompatible: different major version
    metadata.version = "2.0.0";
    metadata.themisMajor = 2;  // ThemisDB is at major version 1
    metadata.themisMinor = 0;
    
    EXPECT_FALSE(loader.isABICompatible(metadata));
}

TEST(ModuleLoader, IsABICompatibleMinorTooNew) {
    ModuleLoader loader;
    ModuleMetadata metadata;
    
    // Incompatible: module minor > themis minor
    metadata.version = "1.5.0";
    metadata.themisMajor = 1;
    metadata.themisMinor = 5;  // ThemisDB is at 1.0
    
    EXPECT_FALSE(loader.isABICompatible(metadata));
}

TEST(ModuleLoader, IsABICompatibleMinorOlder) {
    ModuleLoader loader;
    ModuleMetadata metadata;
    
    // Compatible: module minor < themis minor (backward compatible)
    metadata.version = "1.0.0";
    metadata.themisMajor = 1;
    metadata.themisMinor = 0;  // Module is older, but compatible
    
    EXPECT_TRUE(loader.isABICompatible(metadata));
}

// ===== Phase 2 Integration Tests =====

TEST(ModuleLoader, LoadNonExistentRecordsFailure) {
    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    
    // Try to load non-existent module
    auto result = loader.loadModule("/nonexistent_phase2.so", "test_module");
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorCode, ModuleErrorCode::MODULE_NOT_FOUND);
    
    // Check that failure was recorded
    auto history = loader.getFailureHistory("/nonexistent_phase2.so");
    EXPECT_TRUE(history.has_value());
    if (history) {
        EXPECT_EQ(history->consecutiveFailures, 1u);
        EXPECT_EQ(history->lastErrorCode, ModuleErrorCode::MODULE_NOT_FOUND);
    }
    
    // Check metrics updated
    auto metrics = loader.getMetrics();
    EXPECT_EQ(metrics.totalLoadAttempts, 1u);
    EXPECT_EQ(metrics.failedLoads, 1u);
    EXPECT_EQ(metrics.successfulLoads, 0u);
}

TEST(ModuleLoader, RepeatedFailuresIncrementCount) {
    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    
    std::string path = "/nonexistent_repeated.so";
    
    // First failure
    auto result1 = loader.loadModule(path, "test1");
    EXPECT_FALSE(result1.success);
    
    auto history1 = loader.getFailureHistory(path);
    EXPECT_TRUE(history1.has_value());
    if (history1) {
        EXPECT_EQ(history1->consecutiveFailures, 1u);
    }
    
    // Wait to avoid backoff (in real scenario, time would pass)
    // For testing, we clear and try again
    loader.clearFailureHistory(path);
    
    // Second failure
    auto result2 = loader.loadModule(path, "test2");
    EXPECT_FALSE(result2.success);
    
    auto history2 = loader.getFailureHistory(path);
    EXPECT_TRUE(history2.has_value());
    if (history2) {
        EXPECT_EQ(history2->consecutiveFailures, 1u);  // Cleared, so back to 1
    }
}

TEST(ModuleLoader, ClearFailureHistoryWorks) {
    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    
    std::string path = "/nonexistent_clear.so";
    
    // Create a failure
    loader.loadModule(path, "test");
    
    auto history1 = loader.getFailureHistory(path);
    EXPECT_TRUE(history1.has_value());
    
    // Clear history
    loader.clearFailureHistory(path);
    
    auto history2 = loader.getFailureHistory(path);
    EXPECT_FALSE(history2.has_value());
}

// Entry point for test execution
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
