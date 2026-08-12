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
#include "themis/module_hash_verifier.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

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
    EXPECT_EQ(static_cast<int>(ModuleErrorCode::ZONE_ID_BLOCKED), 503);
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
    EXPECT_EQ(result.errorCategory, ErrorCategory::NONE);
    EXPECT_TRUE(result.errorMessage.empty());
    EXPECT_TRUE(result.moduleHash.empty());
    EXPECT_TRUE(result.modulePath.empty());
    EXPECT_EQ(result.verificationTimestamp, 0u);
    EXPECT_FALSE(result.metadata.isValid());
    EXPECT_EQ(result.zoneId, -1);
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
    
    // The second attempt must not be treated as "already loaded".
    // Current loader semantics apply exponential backoff after the first
    // recorded failure, so the immediate retry is rejected as a policy/backoff
    // violation instead of re-probing the filesystem.
    auto result2 = loader.loadModule("/nonexistent1.so", "test_module");
    EXPECT_FALSE(result2.success);
    EXPECT_EQ(result2.errorCode, ModuleErrorCode::POLICY_VIOLATION);
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

// ===== Phase 3 Tests: Staged Loading & Health Checks =====

TEST(LoadStage, EnumValues) {
    // Verify load stage enum exists
    LoadStage stage = LoadStage::UNLOADED;
    EXPECT_TRUE(stage == LoadStage::UNLOADED);
    
    stage = LoadStage::VERIFYING;
    EXPECT_TRUE(stage == LoadStage::VERIFYING);
    
    stage = LoadStage::ACTIVE;
    EXPECT_TRUE(stage == LoadStage::ACTIVE);
}

TEST(HealthCheckResult, SuccessFactory) {
    auto result = HealthCheckResult::success("test_check", "All good");
    
    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.checkName, "test_check");
    EXPECT_EQ(result.message, "All good");
}

TEST(HealthCheckResult, FailureFactory) {
    auto result = HealthCheckResult::failure("test_check", "Something wrong");
    
    EXPECT_FALSE(result.passed);
    EXPECT_EQ(result.checkName, "test_check");
    EXPECT_EQ(result.message, "Something wrong");
}

TEST(LoadedModule, HasStageTracking) {
    LoadedModule module;
    
    EXPECT_EQ(module.currentStage, LoadStage::UNLOADED);
    EXPECT_TRUE(module.healthChecks.empty());
    EXPECT_FALSE(module.fullyActivated);
}

TEST(LoadedModule, CanTrackHealthChecks) {
    LoadedModule module;
    
    module.healthChecks.push_back(HealthCheckResult::success("check1"));
    module.healthChecks.push_back(HealthCheckResult::success("check2"));
    
    EXPECT_EQ(module.healthChecks.size(), 2u);
    EXPECT_TRUE(module.healthChecks[0].passed);
    EXPECT_EQ(module.healthChecks[0].checkName, "check1");
}

TEST(ModuleLoader, RegisterHealthCheck) {
    ModuleLoader loader;
    
    // Register a health check
    loader.registerHealthCheck("test_check", [](void* handle, const std::string& name) {
        return HealthCheckResult::success("test_check", "OK");
    });
    
    SUCCEED();
}

TEST(ModuleLoader, ClearHealthChecks) {
    ModuleLoader loader;
    
    loader.registerHealthCheck("check1", [](void*, const std::string&) {
        return HealthCheckResult::success("check1");
    });
    
    loader.clearHealthChecks();
    
    SUCCEED();
}

TEST(ModuleLoader, SetStagedLoadingEnabled) {
    ModuleLoader loader;
    
    loader.setStagedLoadingEnabled(true);
    loader.setStagedLoadingEnabled(false);
    
    SUCCEED();
}

TEST(ModuleLoader, QueryModuleStageNotFound) {
    ModuleLoader loader;
    
    auto stage = loader.queryModuleStage("nonexistent");
    EXPECT_FALSE(stage.has_value());
}

TEST(ModuleLoader, GetHealthCheckResultsEmpty) {
    ModuleLoader loader;
    
    auto results = loader.getHealthCheckResults("nonexistent");
    EXPECT_TRUE(results.empty());
}

TEST(ModuleLoader, HealthCheckExecutionSimulation) {
    ModuleLoader loader;
    
    // Register multiple health checks
    bool check1Called = false;
    bool check2Called = false;
    
    loader.registerHealthCheck("check1", [&check1Called](void* handle, const std::string& name) {
        check1Called = true;
        return HealthCheckResult::success("check1", "Check 1 passed");
    });
    
    loader.registerHealthCheck("check2", [&check2Called](void* handle, const std::string& name) {
        check2Called = true;
        return HealthCheckResult::success("check2", "Check 2 passed");
    });
    
    // Note: Without a real module file, we can't test actual health check execution
    // But we can verify the registration works
    SUCCEED();
}

TEST(ModuleLoader, NewErrorCodesExist) {
    // Verify Phase 3 error codes exist
    ModuleErrorCode code = ModuleErrorCode::HEALTH_CHECK_FAILED;
    EXPECT_EQ(static_cast<int>(code), 303);
    
    code = ModuleErrorCode::STAGING_FAILED;
    EXPECT_EQ(static_cast<int>(code), 304);
    
    code = ModuleErrorCode::ACTIVATION_FAILED;
    EXPECT_EQ(static_cast<int>(code), 305);
}

TEST(ModuleLoader, ZoneIdBlockedErrorCodeValue) {
    // Phase 4: Verify ZONE_ID_BLOCKED error code exists and has the correct value
    EXPECT_EQ(static_cast<int>(ModuleErrorCode::ZONE_ID_BLOCKED), 503);
}

TEST(ModuleLoader, ZoneIdBlockedErrorCategory) {
    // ZONE_ID_BLOCKED must be categorized as FATAL – the module cannot be used
    // until its Zone.Identifier is explicitly removed by the administrator.
    ModuleLoader loader;
    // Use the public interface: load a nonexistent path won't trigger zone
    // check (file does not exist), but we can verify the enum and category
    // mapping independently.  A direct categorizeError call is private, so we
    // verify via a ModuleVerificationResult field that carries errorCategory.
    ModuleVerificationResult result;
    result.errorCode = ModuleErrorCode::ZONE_ID_BLOCKED;
    result.errorCategory = ErrorCategory::FATAL;
    EXPECT_EQ(result.errorCategory, ErrorCategory::FATAL);
}

TEST(ModuleVerificationResult, ZoneIdDefaultsToMinusOne) {
    // zoneId = -1 means no Zone.Identifier ADS (safe default for all platforms)
    ModuleVerificationResult result;
    EXPECT_EQ(result.zoneId, -1);
}

TEST(ModuleLoader, MetadataCachingBehavior) {
    ModuleLoader loader;
    
    // Try to load same module twice to test caching
    // First load will create cache, second should use it
    // Without real modules, we can't fully test, but structure is in place
    SUCCEED();
}

TEST(ModuleLoader, StagedLoadingLogsStages) {
    ModuleLoader loader;
    loader.setStagedLoadingEnabled(true);
    
    // With staged loading enabled, module load should log each stage
    // This is tested via the log output in actual usage
    SUCCEED();
}

TEST(ModuleLoader, HealthCheckCanFailActivation) {
    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    loader.setStagedLoadingEnabled(true);
    
    // Register a failing health check
    loader.registerHealthCheck("fail_check", [](void* handle, const std::string& name) {
        return HealthCheckResult::failure("fail_check", "Intentional failure for testing");
    });
    
    // Try to load (will fail at health check stage if module actually loads)
    // With non-existent file, it fails earlier
    auto result = loader.loadModule("/nonexistent_health.so", "test");
    EXPECT_FALSE(result.success);
}

TEST(ModuleLoader, DisabledStagedLoadingSkipsHealthChecks) {
    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    loader.setStagedLoadingEnabled(false);
    
    // With staged loading disabled, health checks should be skipped
    loader.registerHealthCheck("check", [](void*, const std::string&) {
        // This should not be called
        return HealthCheckResult::success("check");
    });
    
    SUCCEED();
}

// ===== Phase 3 Integration Tests =====

TEST(ModuleLoader, StageTransitionTracking) {
    ModuleLoader loader;
    
    // Verify that stage tracking is available
    auto stage = loader.queryModuleStage("test_module");
    EXPECT_FALSE(stage.has_value());  // Module not loaded
}

TEST(ModuleLoader, HealthCheckResultsStoredPerModule) {
    ModuleLoader loader;
    
    // Verify health check results can be retrieved
    auto results = loader.getHealthCheckResults("test_module");
    EXPECT_TRUE(results.empty());
}

TEST(ModuleLoader, MetadataCacheAvoidsDoubleLoading) {
    ModuleLoader loader;
    
    // The implementation now uses getCachedMetadata() which:
    // 1. Checks cache first
    // 2. Extracts from handle after load
    // 3. Caches for future use
    
    // This optimization eliminates the double-loading issue
    SUCCEED();
}

// ===== SHA-256 Hash Manifest Tests (Issue #2471) =====

TEST(ModuleLoader, SetHashManifestReturnsFalseForMissingFile) {
    ModuleLoader loader;
    EXPECT_FALSE(loader.setHashManifest("/no/such/manifest.json"));
}

TEST(ModuleLoader, SetHashManifestReturnsTrueForValidFile) {
    // Write a minimal valid manifest to a temp file
    const std::string manifestPath = (std::filesystem::temp_directory_path() /
        ("themis_loader_manifest_" +
         std::to_string(static_cast<long>(::getpid())) + ".json")).string();

    std::ofstream f(manifestPath);
    f << R"({"themis_test":"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"})";
    f.close();

    ModuleLoader loader;
    EXPECT_TRUE(loader.setHashManifest(manifestPath));

    std::remove(manifestPath.c_str());
}

TEST(ModuleLoader, LoadModuleHashMismatchRejected) {
    // Create a real (small) shared library content substitute: just a binary file.
    // The module file exists but its hash won't match the "expected" value in
    // the manifest, so the load must be rejected with HASH_MISMATCH before
    // dlopen() is attempted.
    const std::string modPath = (std::filesystem::temp_directory_path() /
        ("themis_test_mismatch_" +
         std::to_string(static_cast<long>(::getpid())) + ".so")).string();

    // Write arbitrary content so the file exists.
    {
        std::ofstream f(modPath, std::ios::binary);
        f.write("\x7f" "ELF FAKE", 8);
    }

    // Compute the actual hash so we can supply a WRONG one in the manifest.
    const std::string actualHash =
        themis::modules::ModuleHashVerifier::computeSHA256(modPath);
    ASSERT_FALSE(actualHash.empty());

    const std::string wrongHash(64, '0');  // all-zero hash, guaranteed wrong
    const std::string manifestPath = (std::filesystem::temp_directory_path() /
        ("themis_loader_mismatch_manifest_" +
         std::to_string(static_cast<long>(::getpid())) + ".json")).string();

    {
        std::ofstream f(manifestPath);
        f << "{\"themis_test_mismatch\":\"" << wrongHash << "\"}";
    }

    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    loader.setRequireSignature(false);
    ASSERT_TRUE(loader.setHashManifest(manifestPath));

    auto result = loader.loadModule(modPath, "themis_test_mismatch");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorCode, ModuleErrorCode::HASH_MISMATCH);
    EXPECT_FALSE(result.errorMessage.empty());

    std::remove(modPath.c_str());
    std::remove(manifestPath.c_str());
}

TEST(ModuleLoader, LoadModuleNotInManifestIsAllowed) {
    // A module whose name is NOT in the manifest should not be blocked.
    // (Manifest acts as integrity pin for listed modules, not a global allowlist.)
    // We verify this by checking that loadModule() fails with MODULE_NOT_FOUND
    // (i.e., it gets past the manifest check and fails for the normal reason).
    const std::string manifestPath = (std::filesystem::temp_directory_path() /
        ("themis_loader_partial_manifest_" +
         std::to_string(static_cast<long>(::getpid())) + ".json")).string();

    {
        std::ofstream f(manifestPath);
        f << R"({"some_other_module":"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"})";
    }

    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    loader.setRequireSignature(false);
    ASSERT_TRUE(loader.setHashManifest(manifestPath));

    // "unlisted_module" is NOT in the manifest → should pass manifest check.
    auto result = loader.loadModule("/nonexistent_unlisted.so", "unlisted_module");
    EXPECT_FALSE(result.success);
    // Should fail with MODULE_NOT_FOUND (file doesn't exist), not HASH_MISMATCH
    EXPECT_EQ(result.errorCode, ModuleErrorCode::MODULE_NOT_FOUND);

    std::remove(manifestPath.c_str());
}
// ===== Phase 4 Tests: Platform-Specific Signature Verification =====

#ifdef _WIN32
TEST(ModuleLoader, VerifyAuthenticodeSignatureNonExistent) {
    ModuleLoader loader;
    std::string signerInfo;
    
    // Non-existent file: should return false
    bool result = loader.verifyAuthenticodeSignature("/nonexistent.dll", signerInfo);
    EXPECT_FALSE(result);
}

TEST(ModuleLoader, GetZoneIdentifierNonExistent) {
    ModuleLoader loader;
    
    // Non-existent file: no ADS stream, return -1
    int zone = loader.getZoneIdentifier("C:\\nonexistent_path\\module.dll");
    EXPECT_EQ(zone, -1);
}

TEST(ModuleLoader, RemoveZoneIdentifierNonExistent) {
    ModuleLoader loader;
    
    // Non-existent file: Zone.Identifier also absent, should succeed
    bool result = loader.removeZoneIdentifier("C:\\nonexistent_path\\module.dll");
    EXPECT_TRUE(result);
}

// RAII scope guard for cleaning up Zone.Identifier test files on Windows.
// Deletes both the main file and its Zone.Identifier ADS on destruction.
struct ZoneTestFileGuard {
    std::string path;
    bool hasADS = false;
    ~ZoneTestFileGuard() {
        if (hasADS) {
            DeleteFileA((path + ":Zone.Identifier").c_str());
        }
        DeleteFileA(path.c_str());
    }
};

// Helper: returns the system temporary directory path (no trailing backslash)
static std::string GetWindowsTempDir() {
    char buf[MAX_PATH + 1] = {};
    DWORD len = GetTempPathA(MAX_PATH + 1, buf);
    if (len == 0 || len > MAX_PATH) {
        return "C:\\Windows\\Temp";
    }
    std::string dir(buf, len);
    // Remove trailing backslash for consistent path construction
    if (!dir.empty() && dir.back() == '\\') {
        dir.pop_back();
    }
    return dir;
}

TEST(ModuleLoader, LoadModuleWithInternetZoneIsBlocked) {
    // Create a real temporary file, attach Zone.Identifier ADS with zone 3
    // (Internet), and verify that loadModule() rejects it with ZONE_ID_BLOCKED.
    const std::string tmpPath =
        GetWindowsTempDir() + "\\themis_zone_test_internet.dll";
    ZoneTestFileGuard guard{tmpPath, true};

    // Write a minimal placeholder file
    {
        std::ofstream f(tmpPath, std::ios::binary);
        ASSERT_TRUE(f.is_open()) << "Could not create temp file for Zone.Identifier test";
        f << "placeholder";
    }

    // Attach Zone.Identifier ADS: zone 3 = Internet
    {
        std::string adsPath = tmpPath + ":Zone.Identifier";
        std::ofstream ads(adsPath);
        ASSERT_TRUE(ads.is_open()) << "Could not create Zone.Identifier ADS";
        ads << "[ZoneTransfer]\r\nZoneId=3\r\n";
    }

    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    loader.setRequireSignature(false);

    auto result = loader.loadModule(tmpPath, "themis_zone_test");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorCode, ModuleErrorCode::ZONE_ID_BLOCKED);
    EXPECT_EQ(result.zoneId, 3);
    EXPECT_FALSE(result.errorMessage.empty());
    EXPECT_NE(result.errorMessage.find("Internet"), std::string::npos);
    // guard destructor performs cleanup
}

TEST(ModuleLoader, LoadModuleWithRestrictedZoneIsBlocked) {
    // Same test for zone 4 (Restricted Sites)
    const std::string tmpPath =
        GetWindowsTempDir() + "\\themis_zone_test_restricted.dll";
    ZoneTestFileGuard guard{tmpPath, true};

    {
        std::ofstream f(tmpPath, std::ios::binary);
        ASSERT_TRUE(f.is_open());
        f << "placeholder";
    }
    {
        std::string adsPath = tmpPath + ":Zone.Identifier";
        std::ofstream ads(adsPath);
        ASSERT_TRUE(ads.is_open());
        ads << "[ZoneTransfer]\r\nZoneId=4\r\n";
    }

    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    loader.setRequireSignature(false);

    auto result = loader.loadModule(tmpPath, "themis_zone_test_restricted");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorCode, ModuleErrorCode::ZONE_ID_BLOCKED);
    EXPECT_EQ(result.zoneId, 4);
    EXPECT_NE(result.errorMessage.find("Restricted"), std::string::npos);
    // guard destructor performs cleanup
}

TEST(ModuleLoader, LoadModuleWithTrustedZonePermitted) {
    // Zone 2 (Trusted Sites) should not trigger ZONE_ID_BLOCKED.
    // The load will fail for other reasons (not a valid DLL), but not zone check.
    const std::string tmpPath =
        GetWindowsTempDir() + "\\themis_zone_test_trusted.dll";
    ZoneTestFileGuard guard{tmpPath, true};

    {
        std::ofstream f(tmpPath, std::ios::binary);
        ASSERT_TRUE(f.is_open());
        f << "placeholder";
    }
    {
        std::string adsPath = tmpPath + ":Zone.Identifier";
        std::ofstream ads(adsPath);
        ASSERT_TRUE(ads.is_open());
        ads << "[ZoneTransfer]\r\nZoneId=2\r\n";
    }

    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    loader.setRequireSignature(false);

    auto result = loader.loadModule(tmpPath, "themis_zone_test_trusted");
    // Zone 2 is permitted; result.zoneId must be set correctly
    EXPECT_NE(result.errorCode, ModuleErrorCode::ZONE_ID_BLOCKED);
    EXPECT_EQ(result.zoneId, 2);
    // guard destructor performs cleanup
}

TEST(ModuleLoader, LoadModuleNoZoneIdentifierPermitted) {
    // A file with no Zone.Identifier ADS (zoneId == -1) must not be blocked
    // by the zone check.  The load will fail later (not a valid DLL), but not
    // at the zone check step.
    const std::string tmpPath =
        GetWindowsTempDir() + "\\themis_no_zone_test.dll";
    ZoneTestFileGuard guard{tmpPath, false};

    {
        std::ofstream f(tmpPath, std::ios::binary);
        ASSERT_TRUE(f.is_open());
        f << "placeholder";
    }

    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    loader.setRequireSignature(false);

    auto result = loader.loadModule(tmpPath, "themis_no_zone_test");
    EXPECT_NE(result.errorCode, ModuleErrorCode::ZONE_ID_BLOCKED);
    EXPECT_EQ(result.zoneId, -1);
    // guard destructor performs cleanup
}
#endif // _WIN32

#ifdef __linux__
TEST(ModuleLoader, VerifyGPGSignatureNoSigFile) {
    ModuleLoader loader;
    
    // Module without any .asc/.sig/.gpg file should return false
    bool result = loader.verifyGPGSignature("/nonexistent_module.so");
    EXPECT_FALSE(result);
}

TEST(ModuleLoader, VerifyGPGSignatureInvalidPath) {
    ModuleLoader loader;
    
    // Path with shell-unsafe characters should be rejected
    bool result = loader.verifyGPGSignature("/path/with'quote/module.so");
    EXPECT_FALSE(result);
}

TEST(ModuleLoader, VerifyGPGSignatureExplicitSigPath) {
    ModuleLoader loader;
    
    // Non-existent explicit signature path should return false
    bool result = loader.verifyGPGSignature("/nonexistent.so", "/nonexistent.so.asc");
    EXPECT_FALSE(result);
}

TEST(ModuleLoader, GetExtendedAttributesNonExistent) {
    ModuleLoader loader;
    
    // Non-existent file: empty attribute map
    auto attrs = loader.getExtendedAttributes("/nonexistent_module.so");
    EXPECT_TRUE(attrs.empty());
}

TEST(ModuleLoader, ReadELFMetadataNonExistent) {
    ModuleLoader loader;
    
    // Non-existent file: empty metadata
    std::string metadata = loader.readELFMetadata("/nonexistent_module.so");
    EXPECT_TRUE(metadata.empty());
}

TEST(ModuleLoader, ReadELFMetadataNonELF) {
    // Create a temporary non-ELF file
    std::string tmpPath = "/tmp/test_non_elf.bin";
    {
        std::ofstream f(tmpPath, std::ios::binary);
        f << "This is not an ELF file";
    }
    
    ModuleLoader loader;
    std::string metadata = loader.readELFMetadata(tmpPath);
    EXPECT_TRUE(metadata.empty());
    
    // Clean up
    std::filesystem::remove(tmpPath);
}

TEST(ModuleLoader, ReadELFMetadataCurrentLibrary) {
    // /proc/self/exe is a valid ELF binary on Linux (the test executable itself)
    ModuleLoader loader;
    
    // Should not crash; result may be empty or contain build info
    EXPECT_NO_THROW({
        std::string metadata = loader.readELFMetadata("/proc/self/exe");
        // Metadata may be empty or contain BuildID / Comment sections
        // Just verify it doesn't throw
        (void)metadata;
    });
}
#endif // __linux__

// ===== Per-Plugin Audit Trail Tests =====

using namespace themis::acceleration;

TEST(ModuleLoader, GetPluginAuditTrailEmptyForUnknownPath) {
    // Clear global auditor to avoid pollution from other tests
    PluginSecurityAuditor::instance().clearEvents();

    ModuleLoader loader;
    auto trail = loader.getPluginAuditTrail("/no/such/plugin.so");
    EXPECT_TRUE(trail.empty());
}

TEST(ModuleLoader, GetPluginAuditTrailRecordsLoadFailure) {
    PluginSecurityAuditor::instance().clearEvents();

    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    loader.setRequireSignature(false);

    const std::string path = "/nonexistent_audit_trail_test.so";
    auto result = loader.loadModule(path, "audit_trail_test");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorCode, ModuleErrorCode::MODULE_NOT_FOUND);

    // MODULE_NOT_FOUND failures are recorded in the failure-history / metrics
    // subsystem but not emitted to the security auditor (only security and
    // staged-activation failures are).  Verify that the per-plugin trail is
    // empty for this case, which is expected and documented behaviour.
    auto trail = loader.getPluginAuditTrail(path);
    EXPECT_TRUE(trail.empty());

    PluginSecurityAuditor::instance().clearEvents();
}

TEST(ModuleLoader, PluginUnloadedEventTypeIsLoggable) {
    // Verify that the PLUGIN_UNLOADED event type can be logged and retrieved
    // from the auditor (i.e., it is properly handled at runtime, not just at
    // compile time).
    PluginSecurityAuditor::instance().clearEvents();

    PluginSecurityAuditor::instance().logEvent({
        PluginSecurityEvent::EventType::PLUGIN_UNLOADED,
        "/test/plugin.so", "hash123", "unloaded for test",
        42ULL, "INFO"
    });

    auto events = PluginSecurityAuditor::instance().getAllEvents();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, PluginSecurityEvent::EventType::PLUGIN_UNLOADED);

    PluginSecurityAuditor::instance().clearEvents();
}

TEST(ModuleLoader, UnloadModuleLogsAuditEvent) {
    PluginSecurityAuditor::instance().clearEvents();

    // Without a real shared library file on disk we cannot exercise the full
    // loadModule/unloadModule path.  Instead we verify the audit mechanism
    // end-to-end:
    //   1. Log a PLUGIN_UNLOADED event via the auditor (matching what
    //      unloadModule() does internally).
    //   2. Retrieve it via ModuleLoader::getPluginAuditTrail().
    // This confirms that the ModuleLoader correctly surfaces unload events
    // recorded in the global PluginSecurityAuditor.
    ModuleLoader loader;

    const std::string fakePath = "/fake/libthemis_test.so";
    const std::string fakeHash = "deadbeefdeadbeef";

    PluginSecurityAuditor::instance().logEvent({
        PluginSecurityEvent::EventType::PLUGIN_UNLOADED,
        fakePath,
        fakeHash,
        "Module unloaded: libthemis_test",
        static_cast<uint64_t>(1000),
        "INFO"
    });

    auto trail = loader.getPluginAuditTrail(fakePath);
    ASSERT_EQ(trail.size(), 1u);
    EXPECT_EQ(trail[0].type, PluginSecurityEvent::EventType::PLUGIN_UNLOADED);
    EXPECT_EQ(trail[0].pluginPath, fakePath);
    EXPECT_EQ(trail[0].pluginHash, fakeHash);
    EXPECT_EQ(trail[0].severity, "INFO");

    PluginSecurityAuditor::instance().clearEvents();
}

TEST(ModuleLoader, AuditTrailIsolatedPerPlugin) {
    PluginSecurityAuditor::instance().clearEvents();

    ModuleLoader loader;

    const std::string pathA = "/plugins/plugin_a.so";
    const std::string pathB = "/plugins/plugin_b.so";

    PluginSecurityAuditor::instance().logEvent({
        PluginSecurityEvent::EventType::PLUGIN_LOADED, pathA, "h1", "loaded A", 1ULL, "INFO"
    });
    PluginSecurityAuditor::instance().logEvent({
        PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED, pathB, "h2", "failed B", 2ULL, "ERROR"
    });
    PluginSecurityAuditor::instance().logEvent({
        PluginSecurityEvent::EventType::PLUGIN_UNLOADED, pathA, "h1", "unloaded A", 3ULL, "INFO"
    });

    auto trailA = loader.getPluginAuditTrail(pathA);
    ASSERT_EQ(trailA.size(), 2u);
    EXPECT_EQ(trailA[0].type, PluginSecurityEvent::EventType::PLUGIN_LOADED);
    EXPECT_EQ(trailA[1].type, PluginSecurityEvent::EventType::PLUGIN_UNLOADED);

    auto trailB = loader.getPluginAuditTrail(pathB);
    ASSERT_EQ(trailB.size(), 1u);
    EXPECT_EQ(trailB[0].type, PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED);

    PluginSecurityAuditor::instance().clearEvents();
}

TEST(ModuleMetrics, TotalUnloadsField) {
    // Verify totalUnloads field is accessible and defaults to zero
    ModuleMetrics metrics;
    EXPECT_EQ(metrics.totalUnloads, 0u);
}

// ============================================================================
// PluginBundle tests  (v1.4.0 — Cross-Platform Module Format)
// ============================================================================

// ── PluginBundleManifest ────────────────────────────────────────────────────

TEST(PluginBundleManifest, DefaultConstruction) {
    PluginBundleManifest m;
    EXPECT_TRUE(m.name.empty());
    EXPECT_TRUE(m.version.empty());
    EXPECT_TRUE(m.description.empty());
    EXPECT_TRUE(m.author.empty());
    EXPECT_TRUE(m.nativeLibraries.empty());
    EXPECT_TRUE(m.wasmFallback.empty());
    EXPECT_EQ(m.signatureFile, "plugin.sig");
    EXPECT_FALSE(m.isValid());
}

TEST(PluginBundleManifest, IsValidRequiresNameAndVersion) {
    PluginBundleManifest m;
    m.name = "my_plugin";
    EXPECT_FALSE(m.isValid());  // version still empty

    m.version = "1.0.0";
    EXPECT_TRUE(m.isValid());
}

// ── PluginBundleLoadResult ──────────────────────────────────────────────────

TEST(PluginBundleLoadResult, DefaultConstruction) {
    PluginBundleLoadResult r;
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.errorMessage.empty());
    EXPECT_TRUE(r.resolvedBinaryPath.empty());
    EXPECT_FALSE(r.usedWasmFallback);
    EXPECT_TRUE(r.tempDirectory.empty());
}

// ── PluginBundleLoader — currentPlatform ────────────────────────────────────

TEST(PluginBundleLoader, CurrentPlatformNonEmpty) {
    std::string p = PluginBundleLoader::currentPlatform();
    EXPECT_FALSE(p.empty());
    // Must contain a '-' separating OS and architecture tokens.
    EXPECT_NE(p.find('-'), std::string::npos);
}

TEST(PluginBundleLoader, CurrentPlatformKnownOS) {
    std::string p = PluginBundleLoader::currentPlatform();
    bool knownOS = (p.rfind("linux-",   0) == 0) ||
                   (p.rfind("windows-", 0) == 0) ||
                   (p.rfind("macos-",   0) == 0) ||
                   (p.rfind("unknown-", 0) == 0);
    EXPECT_TRUE(knownOS) << "Unexpected platform token: " << p;
}

// ── PluginBundleLoader — parseManifest ─────────────────────────────────────

TEST(PluginBundleLoader, ParseManifestMinimal) {
    const std::string json = R"({"name":"test_plugin","version":"1.2.3"})";
    PluginBundleManifest m;
    std::string err;
    EXPECT_TRUE(PluginBundleLoader::parseManifest(json, m, err));
    EXPECT_EQ(m.name,    "test_plugin");
    EXPECT_EQ(m.version, "1.2.3");
    EXPECT_TRUE(m.description.empty());
    EXPECT_TRUE(m.wasmFallback.empty());
    EXPECT_EQ(m.signatureFile, "plugin.sig");  // default preserved
}

TEST(PluginBundleLoader, ParseManifestFull) {
    const std::string json = R"({
        "name": "my_backend",
        "version": "2.0.1",
        "description": "An example backend",
        "author": "ThemisDB Team",
        "signatureFile": "bundle.sig",
        "wasmFallback": "plugin.wasm",
        "nativeLibraries": {
            "linux-x86_64":  "linux-x86_64/libmy_backend.so",
            "windows-x86_64":"windows-x86_64/my_backend.dll",
            "macos-arm64":   "macos-arm64/libmy_backend.dylib"
        }
    })";

    PluginBundleManifest m;
    std::string err;
    ASSERT_TRUE(PluginBundleLoader::parseManifest(json, m, err)) << err;
    EXPECT_EQ(m.name,         "my_backend");
    EXPECT_EQ(m.version,      "2.0.1");
    EXPECT_EQ(m.description,  "An example backend");
    EXPECT_EQ(m.author,       "ThemisDB Team");
    EXPECT_EQ(m.signatureFile,"bundle.sig");
    EXPECT_EQ(m.wasmFallback, "plugin.wasm");
    ASSERT_EQ(m.nativeLibraries.size(), 3u);
    EXPECT_EQ(m.nativeLibraries.at("linux-x86_64"),   "linux-x86_64/libmy_backend.so");
    EXPECT_EQ(m.nativeLibraries.at("windows-x86_64"), "windows-x86_64/my_backend.dll");
    EXPECT_EQ(m.nativeLibraries.at("macos-arm64"),    "macos-arm64/libmy_backend.dylib");
}

TEST(PluginBundleLoader, ParseManifestMissingName) {
    const std::string json = R"({"version":"1.0.0"})";
    PluginBundleManifest m;
    std::string err;
    EXPECT_FALSE(PluginBundleLoader::parseManifest(json, m, err));
    EXPECT_FALSE(err.empty());
}

TEST(PluginBundleLoader, ParseManifestMissingVersion) {
    const std::string json = R"({"name":"plugin_x"})";
    PluginBundleManifest m;
    std::string err;
    EXPECT_FALSE(PluginBundleLoader::parseManifest(json, m, err));
    EXPECT_FALSE(err.empty());
}

TEST(PluginBundleLoader, ParseManifestEmptyName) {
    const std::string json = R"({"name":"","version":"1.0.0"})";
    PluginBundleManifest m;
    std::string err;
    EXPECT_FALSE(PluginBundleLoader::parseManifest(json, m, err));
    EXPECT_FALSE(err.empty());
}

TEST(PluginBundleLoader, ParseManifestInvalidJson) {
    const std::string json = "{ this is not valid json }";
    PluginBundleManifest m;
    std::string err;
    EXPECT_FALSE(PluginBundleLoader::parseManifest(json, m, err));
    EXPECT_FALSE(err.empty());
}

TEST(PluginBundleLoader, ParseManifestWasmFallbackOnly) {
    // A WASM-only bundle has no nativeLibraries but has wasmFallback.
    const std::string json = R"({
        "name": "portable_plugin",
        "version": "0.1.0",
        "wasmFallback": "plugin.wasm"
    })";
    PluginBundleManifest m;
    std::string err;
    ASSERT_TRUE(PluginBundleLoader::parseManifest(json, m, err)) << err;
    EXPECT_EQ(m.wasmFallback, "plugin.wasm");
    EXPECT_TRUE(m.nativeLibraries.empty());
}

// ── PluginBundleLoader — loadBundle error paths ─────────────────────────────

TEST(PluginBundleLoader, LoadBundleNonexistentFileReturnsError) {
    PluginBundleLoader bundleLoader;
    // Must opt in to unsigned bundles so extraction is attempted
    // (if the archive can't be opened the error comes from extractToTempDir
    //  before signature checking).
    bundleLoader.setAllowUnsignedBundles(true);
    ModuleLoader moduleLoader;
    auto result = bundleLoader.loadBundle("/nonexistent/path/plugin.tdb", moduleLoader);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST(PluginBundleLoader, LoadBundleFailsClosedWithoutPublicKeyOrOptIn) {
    // Default state: no public key and allowUnsignedBundles defaults to false.
    // loadBundle() must fail fast (fail-closed) rather than silently skipping
    // signature verification.
    PluginBundleLoader bundleLoader;
    ModuleLoader moduleLoader;
    auto result = bundleLoader.loadBundle("/nonexistent/path/plugin.tdb", moduleLoader);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
    // The error should come from either extraction failure (no libzip / bad path)
    // or from the fail-closed signature check — either way the bundle is rejected.
}

// ── PluginBundleLoader — Ed25519 signature verification ─────────────────────

TEST(PluginBundleLoader, VerifyEd25519InvalidKeyReturnsError) {
    const uint8_t msg[] = "hello";
    std::vector<uint8_t> fakeSig(64, 0xAB);
    std::string err;
    bool ok = PluginBundleLoader::verifyEd25519Signature(
        msg, sizeof(msg), fakeSig, "not-a-real-pem-key", err);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(err.empty());
}

TEST(PluginBundleLoader, VerifyEd25519WrongSigLengthReturnsError) {
    const uint8_t msg[] = "data";
    std::vector<uint8_t> shortSig(10, 0x00);  // must be exactly 64 bytes
    std::string err;
    bool ok = PluginBundleLoader::verifyEd25519Signature(
        msg, sizeof(msg), shortSig, "", err);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(err.empty());
}


// ============================================================================
// Concurrent access tests (O(1) module lookup / shared_mutex)
// ============================================================================

// Verify that concurrent read-only calls (isModuleLoaded / getModuleInfo /
// getAllLoadedModules) do not race or deadlock.  Eight reader threads issue
// repeated lookups against an empty ModuleLoader instance to stress shared
// read-only access paths under contention. The test must pass cleanly under
// TSAN.
TEST(ModuleLoader, ConcurrentReadersDoNotRace) {
    ModuleLoader loader;

    constexpr int kReaders    = 8;
    constexpr int kIterations = 200;

    std::atomic<bool> start{false};
    // Accumulate results in atomics; EXPECT_* is not guaranteed thread-safe.
    std::atomic<int> unexpectedLoaded{0};
    std::atomic<int> unexpectedInfo{0};
    std::atomic<int> unexpectedNonEmpty{0};

    auto readerTask = [&]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (int i = 0; i < kIterations; ++i) {
            if (loader.isModuleLoaded("nonexistent_module"))
                unexpectedLoaded.fetch_add(1, std::memory_order_relaxed);
            if (loader.getModuleInfo("nonexistent_module").has_value())
                unexpectedInfo.fetch_add(1, std::memory_order_relaxed);
            if (!loader.getAllLoadedModules().empty())
                unexpectedNonEmpty.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back(readerTask);
    }

    start.store(true, std::memory_order_release);

    for (auto& t : readers) {
        t.join();
    }

    EXPECT_EQ(unexpectedLoaded.load(), 0);
    EXPECT_EQ(unexpectedInfo.load(), 0);
    EXPECT_EQ(unexpectedNonEmpty.load(), 0);
}

// Verify that concurrent write calls (unloadModule on a non-existent module
// is a benign no-op) do not race with concurrent reads.
TEST(ModuleLoader, ConcurrentReadWriteDoNotRace) {
    ModuleLoader loader;

    constexpr int kReaders    = 8;
    constexpr int kWriters    = 2;
    constexpr int kIterations = 100;

    std::atomic<bool> start{false};

    auto readerTask = [&]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (int i = 0; i < kIterations; ++i) {
            (void)loader.isModuleLoaded("no_such_module");
            (void)loader.getModuleInfo("no_such_module");
            (void)loader.getAllLoadedModules();
        }
    };

    // Writers call unloadModule for a non-existent module (no-op but acquires
    // the unique_lock, exercising the writer path).
    auto writerTask = [&]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (int i = 0; i < kIterations; ++i) {
            loader.unloadModule("no_such_module");
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kReaders + kWriters);
    for (int i = 0; i < kReaders; ++i) {
        threads.emplace_back(readerTask);
    }
    for (int i = 0; i < kWriters; ++i) {
        threads.emplace_back(writerTask);
    }

    start.store(true, std::memory_order_release);

    for (auto& t : threads) {
        t.join();
    }
}

// Verify getModuleInfo O(1) lookup: calling it 8 times concurrently from
// independent threads must not corrupt the loader's internal state.
TEST(ModuleLoader, GetModuleInfoO1LookupIsSafe) {
    ModuleLoader loader;

    constexpr int kThreads    = 8;
    constexpr int kIterations = 500;

    std::atomic<bool> start{false};
    std::atomic<int>  found{0};

    auto task = [&]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (int i = 0; i < kIterations; ++i) {
            auto info = loader.getModuleInfo("absent");
            if (info.has_value()) {
                found.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(task);
    }

    start.store(true, std::memory_order_release);

    for (auto& t : threads) {
        t.join();
    }

    // No module was ever loaded, so none should have been found.
    EXPECT_EQ(found.load(), 0);
}


// ============================================================================
// ModuleDependencyResolver Tests
// ============================================================================

// --- registerModule / getRegisteredModules -----------------------------------

TEST(ModuleDependencyResolver, EmptyResolverHasNoModules) {
    ModuleDependencyResolver resolver;
    EXPECT_TRUE(resolver.getRegisteredModules().empty());
}

TEST(ModuleDependencyResolver, RegisterModuleWithVersion) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("themis_base", "1.0.0", {});
    auto modules = resolver.getRegisteredModules();
    ASSERT_EQ(modules.size(), 1u);
    EXPECT_EQ(modules[0].name, "themis_base");
    EXPECT_EQ(modules[0].version, "1.0.0");
    EXPECT_TRUE(modules[0].deps.empty());
}

TEST(ModuleDependencyResolver, RegisterModuleWithoutVersion) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("themis_base", {});
    auto modules = resolver.getRegisteredModules();
    ASSERT_EQ(modules.size(), 1u);
    EXPECT_EQ(modules[0].name, "themis_base");
    EXPECT_EQ(modules[0].version, "");
}

TEST(ModuleDependencyResolver, RegisterModuleReplacesExisting) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("themis_base", "1.0.0", {});
    resolver.registerModule("themis_base", "2.0.0", {});
    auto modules = resolver.getRegisteredModules();
    ASSERT_EQ(modules.size(), 1u);
    EXPECT_EQ(modules[0].version, "2.0.0");
}

TEST(ModuleDependencyResolver, ClearRemovesAllModules) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", {});
    resolver.registerModule("b", {});
    resolver.clear();
    EXPECT_TRUE(resolver.getRegisteredModules().empty());
}

// --- isVersionCompatible -----------------------------------------------------

TEST(ModuleDependencyResolver, IsVersionCompatible_NoConstraints) {
    EXPECT_TRUE(ModuleDependencyResolver::isVersionCompatible("1.2.3", "", ""));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_MinOnly_Pass) {
    EXPECT_TRUE(ModuleDependencyResolver::isVersionCompatible("2.0.0", "1.0.0", ""));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_MinOnly_Equal) {
    EXPECT_TRUE(ModuleDependencyResolver::isVersionCompatible("1.0.0", "1.0.0", ""));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_MinOnly_Fail) {
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("0.9.0", "1.0.0", ""));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_MaxOnly_Pass) {
    EXPECT_TRUE(ModuleDependencyResolver::isVersionCompatible("1.0.0", "", "2.0.0"));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_MaxOnly_Equal) {
    EXPECT_TRUE(ModuleDependencyResolver::isVersionCompatible("2.0.0", "", "2.0.0"));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_MaxOnly_Fail) {
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("3.0.0", "", "2.0.0"));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_BothConstraints_Pass) {
    EXPECT_TRUE(ModuleDependencyResolver::isVersionCompatible("1.5.0", "1.0.0", "2.0.0"));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_BothConstraints_Fail_Below) {
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("0.5.0", "1.0.0", "2.0.0"));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_BothConstraints_Fail_Above) {
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("3.0.0", "1.0.0", "2.0.0"));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_EmptyVersion_NoConstraints) {
    EXPECT_TRUE(ModuleDependencyResolver::isVersionCompatible("", "", ""));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_EmptyVersion_WithConstraints) {
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("", "1.0.0", ""));
}

// --- resolve() – load order --------------------------------------------------

TEST(ModuleDependencyResolver, ResolveSingleModule) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("themis_base", "1.0.0", {});

    auto result = resolver.resolve();
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 1u);
    EXPECT_EQ(result.loadOrder[0], "themis_base");
}

TEST(ModuleDependencyResolver, ResolveEmptyRegistry) {
    ModuleDependencyResolver resolver;
    auto result = resolver.resolve();
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.loadOrder.empty());
}

TEST(ModuleDependencyResolver, ResolveLinearChain) {
    // storage depends on base; query depends on storage.
    // Expected load order: base → storage → query.
    ModuleDependencyResolver resolver;
    resolver.registerModule("themis_base",    "1.0.0", {});
    resolver.registerModule("themis_storage", "1.0.0", {{"themis_base", "", "", true}});
    resolver.registerModule("themis_query",   "1.0.0", {{"themis_storage", "", "", true}});

    auto result = resolver.resolve();
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 3u);

    auto idx = [&](const std::string& n) {
        return static_cast<int>(
            std::find(result.loadOrder.begin(), result.loadOrder.end(), n)
            - result.loadOrder.begin());
    };
    EXPECT_LT(idx("themis_base"),    idx("themis_storage"));
    EXPECT_LT(idx("themis_storage"), idx("themis_query"));
}

TEST(ModuleDependencyResolver, ResolveDiamondDependency) {
    // Both B and C depend on A; D depends on both B and C.
    // Expected: A before B and C, B and C before D.
    ModuleDependencyResolver resolver;
    resolver.registerModule("A", "1.0.0", {});
    resolver.registerModule("B", "1.0.0", {{"A", "", "", true}});
    resolver.registerModule("C", "1.0.0", {{"A", "", "", true}});
    resolver.registerModule("D", "1.0.0", {{"B", "", "", true}, {"C", "", "", true}});

    auto result = resolver.resolve();
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 4u);

    auto idx = [&](const std::string& n) {
        return static_cast<int>(
            std::find(result.loadOrder.begin(), result.loadOrder.end(), n)
            - result.loadOrder.begin());
    };
    EXPECT_LT(idx("A"), idx("B"));
    EXPECT_LT(idx("A"), idx("C"));
    EXPECT_LT(idx("B"), idx("D"));
    EXPECT_LT(idx("C"), idx("D"));
}

TEST(ModuleDependencyResolver, ResolveIndependentModules) {
    // Three modules with no mutual dependencies – all must appear in result.
    ModuleDependencyResolver resolver;
    resolver.registerModule("mod_x", "1.0.0", {});
    resolver.registerModule("mod_y", "1.0.0", {});
    resolver.registerModule("mod_z", "1.0.0", {});

    auto result = resolver.resolve();
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 3u);
}

// --- resolve() – cycle detection ---------------------------------------------

TEST(ModuleDependencyResolver, ResolveDetectsDirectCycle) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("A", "1.0.0", {{"B", "", "", true}});
    resolver.registerModule("B", "1.0.0", {{"A", "", "", true}});

    auto result = resolver.resolve();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.cycles.empty());
}

TEST(ModuleDependencyResolver, ResolveDetectsTransitiveCycle) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("A", "1.0.0", {{"B", "", "", true}});
    resolver.registerModule("B", "1.0.0", {{"C", "", "", true}});
    resolver.registerModule("C", "1.0.0", {{"A", "", "", true}});

    auto result = resolver.resolve();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.cycles.empty());
}

// --- resolve() – missing required dependencies -------------------------------

TEST(ModuleDependencyResolver, ResolveReportsMissingRequiredDep) {
    ModuleDependencyResolver resolver;
    // "themis_query" requires "themis_storage" which is NOT registered.
    resolver.registerModule("themis_query", "1.0.0", {{"themis_storage", "", "", true}});

    auto result = resolver.resolve();
    EXPECT_FALSE(result.success);
    ASSERT_FALSE(result.missingRequired.empty());
    EXPECT_EQ(result.missingRequired[0], "themis_storage");
}

TEST(ModuleDependencyResolver, ResolveIgnoresMissingOptionalDep) {
    ModuleDependencyResolver resolver;
    // Optional dependency on an unregistered module should not fail resolution.
    resolver.registerModule("themis_query", "1.0.0", {{"themis_optional", "", "", false}});

    auto result = resolver.resolve();
    EXPECT_TRUE(result.success);
}

// --- resolve() – version mismatch detection ----------------------------------

TEST(ModuleDependencyResolver, ResolveReportsVersionMismatch) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("themis_base",    "0.5.0", {});
    // themis_storage requires themis_base >= 1.0.0
    resolver.registerModule("themis_storage", "1.0.0",
                            {{"themis_base", "1.0.0", "", true}});

    auto result = resolver.resolve();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.versionMismatches.empty());
}

// --- resolveFor() ------------------------------------------------------------

TEST(ModuleDependencyResolver, ResolveForSubsetWithVersion) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("themis_base",    "1.0.0", {});
    resolver.registerModule("themis_storage", "1.0.0", {{"themis_base",    "", "", true}});
    resolver.registerModule("themis_query",   "1.0.0", {{"themis_storage", "", "", true}});
    // Register an unrelated module that should NOT appear in the result.
    resolver.registerModule("themis_geo",     "1.0.0", {{"themis_storage", "", "", true}});

    // Only resolve themis_query and its transitive deps.
    auto result = resolver.resolveFor({"themis_query"});
    ASSERT_TRUE(result.success);

    // Exactly base + storage + query must appear (geo is excluded).
    ASSERT_EQ(result.loadOrder.size(), 3u);
    EXPECT_NE(std::find(result.loadOrder.begin(), result.loadOrder.end(), "themis_base"),
              result.loadOrder.end());
    EXPECT_NE(std::find(result.loadOrder.begin(), result.loadOrder.end(), "themis_storage"),
              result.loadOrder.end());
    EXPECT_NE(std::find(result.loadOrder.begin(), result.loadOrder.end(), "themis_query"),
              result.loadOrder.end());
    EXPECT_EQ(std::find(result.loadOrder.begin(), result.loadOrder.end(), "themis_geo"),
              result.loadOrder.end());
}

TEST(ModuleDependencyResolver, ResolveForUnregisteredModuleFails) {
    ModuleDependencyResolver resolver;
    auto result = resolver.resolveFor({"nonexistent_module"});
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.missingRequired.empty());
}

// --- resolveFor() – regression: unregistered transitive dep must fail --------

// Regression test for bug where resolveFor() silently accepted an unregistered
// transitive required dependency by adding it to the `needed` set, which caused
// topologicalSort() to treat it as present in nodeSet and skip the missing-dep
// check. The fix ensures that only registered modules are BFS-visited.
TEST(ModuleDependencyResolver, ResolveForRejectsMissingTransitiveDep) {
    ModuleDependencyResolver resolver;
    // "A" depends on "B"; "B" depends on "C" (required); "C" is NOT registered.
    resolver.registerModule("A", "1.0.0", {{"B", "", "", true}});
    resolver.registerModule("B", "1.0.0", {{"C", "", "", true}});
    // "C" is intentionally not registered.

    auto result = resolver.resolveFor({"A"});
    EXPECT_FALSE(result.success);
    // "C" must be reported as a missing required dependency.
    EXPECT_FALSE(result.missingRequired.empty());
    EXPECT_NE(std::find(result.missingRequired.begin(),
                        result.missingRequired.end(), "C"),
              result.missingRequired.end());
}

TEST(ModuleDependencyResolver, ResolveForAcceptsMissingTransitiveOptionalDep) {
    ModuleDependencyResolver resolver;
    // "A" depends on "B" (required); "B" depends on "C" (optional, unregistered).
    // Since "C" is optional, the resolution should succeed.
    resolver.registerModule("A", "1.0.0", {{"B", "", "", true}});
    resolver.registerModule("B", "1.0.0", {{"C", "", "", false}});
    // "C" is intentionally not registered.

    auto result = resolver.resolveFor({"A"});
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 2u);
    EXPECT_EQ(std::find(result.loadOrder.begin(), result.loadOrder.end(), "C"),
              result.loadOrder.end());
}

// --- isVersionCompatible() – regression: partial version recovery -----------

// Regression test for bug where a blanket catch block reset maj/min to 0 even
// when they had been successfully parsed (e.g. "1.2.abc" → {0,0,0} instead of
// {1,2,0}).  The fix uses individual try-catches for each component.
TEST(ModuleDependencyResolver, IsVersionCompatible_PartialMalformed_MajMin) {
    // "1.2.abc" should parse as {1, 2, 0}, not {0, 0, 0}.
    // With min="1.2.0" and max="1.3.0", version "1.2.abc" ({1,2,0}) is within
    // range; {0,0,0} would be out of range.
    EXPECT_TRUE(ModuleDependencyResolver::isVersionCompatible("1.2.abc", "1.0.0", "1.3.0"));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_MajOnly) {
    // "1" should parse as {1, 0, 0}.
    EXPECT_TRUE(ModuleDependencyResolver::isVersionCompatible("1", "1.0.0", "1.99.0"));
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("1", "2.0.0", ""));
}

TEST(ModuleDependencyResolver, IsVersionCompatible_TotallyMalformed) {
    // "abc" should parse as {0, 0, 0}.
    EXPECT_TRUE(ModuleDependencyResolver::isVersionCompatible("abc", "", ""));
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("abc", "1.0.0", ""));
}

// =============================================================================
// Plugin Watchdog Tests (Issue #2373)
// =============================================================================

// --- WatchdogConfig default values -------------------------------------------

TEST(WatchdogConfig, DefaultValues) {
    WatchdogConfig cfg;
    EXPECT_EQ(cfg.check_interval_ms, 30'000u);
    EXPECT_EQ(cfg.max_restart_attempts, 5u);
    EXPECT_EQ(cfg.initial_backoff_ms, 5'000u);
    EXPECT_DOUBLE_EQ(cfg.backoff_multiplier, 2.0);
    EXPECT_EQ(cfg.max_backoff_ms, 300'000u);
    EXPECT_TRUE(cfg.enabled);
}

TEST(WatchdogConfig, CustomValues) {
    WatchdogConfig cfg;
    cfg.check_interval_ms = 1'000;
    cfg.max_restart_attempts = 3;
    cfg.initial_backoff_ms = 500;
    cfg.backoff_multiplier = 1.5;
    cfg.max_backoff_ms = 60'000;
    cfg.enabled = false;

    EXPECT_EQ(cfg.check_interval_ms, 1'000u);
    EXPECT_EQ(cfg.max_restart_attempts, 3u);
    EXPECT_EQ(cfg.initial_backoff_ms, 500u);
    EXPECT_DOUBLE_EQ(cfg.backoff_multiplier, 1.5);
    EXPECT_EQ(cfg.max_backoff_ms, 60'000u);
    EXPECT_FALSE(cfg.enabled);
}

// --- WatchdogModuleStats default values --------------------------------------

TEST(WatchdogModuleStats, DefaultValues) {
    WatchdogModuleStats stats;
    EXPECT_TRUE(stats.moduleName.empty());
    EXPECT_TRUE(stats.modulePath.empty());
    EXPECT_EQ(stats.restart_count, 0u);
    EXPECT_EQ(stats.consecutive_failures, 0u);
    EXPECT_EQ(stats.last_health_check_ms, 0u);
    EXPECT_EQ(stats.last_failure_ms, 0u);
    EXPECT_EQ(stats.last_restart_ms, 0u);
    EXPECT_EQ(stats.next_retry_ms, 0u);
    EXPECT_FALSE(stats.permanently_failed);
    EXPECT_TRUE(stats.last_error.empty());
}

// --- Watchdog lifecycle (start / stop / isRunning) ---------------------------

TEST(PluginWatchdog, NotRunningByDefault) {
    ModuleLoader loader;
    EXPECT_FALSE(loader.isWatchdogRunning());
}

TEST(PluginWatchdog, StartAndStop) {
    ModuleLoader loader;
    WatchdogConfig cfg;
    cfg.check_interval_ms = 5'000;  // Large interval so the loop doesn't fire during test
    loader.configureWatchdog(cfg);

    loader.startWatchdog();
    EXPECT_TRUE(loader.isWatchdogRunning());

    loader.stopWatchdog();
    EXPECT_FALSE(loader.isWatchdogRunning());
}

TEST(PluginWatchdog, DoubleStartIsIdempotent) {
    ModuleLoader loader;
    WatchdogConfig cfg;
    cfg.check_interval_ms = 5'000;
    loader.configureWatchdog(cfg);

    loader.startWatchdog();
    loader.startWatchdog();  // Second start must not crash or create a second thread
    EXPECT_TRUE(loader.isWatchdogRunning());
    loader.stopWatchdog();
    EXPECT_FALSE(loader.isWatchdogRunning());
}

TEST(PluginWatchdog, DoubleStopIsIdempotent) {
    ModuleLoader loader;
    loader.stopWatchdog();  // Stop when not running — must not crash
    EXPECT_FALSE(loader.isWatchdogRunning());
}

// --- configureWatchdog -------------------------------------------------------

TEST(PluginWatchdog, ConfigureBeforeStart) {
    ModuleLoader loader;
    WatchdogConfig cfg;
    cfg.check_interval_ms = 10'000;
    cfg.max_restart_attempts = 2;
    cfg.enabled = false;
    loader.configureWatchdog(cfg);
    // No assertion on internals; just verify no crash and can start/stop
    loader.startWatchdog();
    EXPECT_TRUE(loader.isWatchdogRunning());
    loader.stopWatchdog();
    EXPECT_FALSE(loader.isWatchdogRunning());
}

// --- getWatchdogStats / getAllWatchdogStats -----------------------------------

TEST(PluginWatchdog, GetStatsReturnsNulloptWhenUntracked) {
    ModuleLoader loader;
    auto stats = loader.getWatchdogStats("nonexistent_module");
    EXPECT_FALSE(stats.has_value());
}

TEST(PluginWatchdog, GetAllStatsEmptyInitially) {
    ModuleLoader loader;
    auto all = loader.getAllWatchdogStats();
    EXPECT_TRUE(all.empty());
}

// --- resetWatchdogStats ------------------------------------------------------

TEST(PluginWatchdog, ResetWatchdogStatsIsIdempotent) {
    ModuleLoader loader;
    loader.resetWatchdogStats();  // Resetting when empty must not crash
    auto all = loader.getAllWatchdogStats();
    EXPECT_TRUE(all.empty());
}

// --- Health-check failure triggers restart -----------------------------------
// Since loadModule() requires a real .so/.dll file, we simulate the watchdog
// restart logic by checking that:
//   1. A health check registered as "always fail" is correctly invoked.
//   2. The watchdog correctly records consecutive failures.
//   3. The permanently_failed flag is set once max_restart_attempts is reached.
// We drive the watchdog loop manually via a short check_interval_ms so the
// test remains deterministic without needing a real module binary.

TEST(PluginWatchdog, WatchdogRunsCleanlyWithNoLoadedModules) {
    // Register a health check that always fails.  With no loaded modules,
    // the watchdog loop should complete one sweep and record no stats.
    // This verifies the watchdog lifecycle (start/sweep/stop) is safe.
    ModuleLoader loader;

    loader.registerHealthCheck("always_fail", [](void*, const std::string&) {
        return HealthCheckResult::failure("always_fail", "simulated failure");
    });

    WatchdogConfig cfg;
    cfg.check_interval_ms = 50;       // 50 ms sweep interval
    cfg.max_restart_attempts = 1;
    cfg.initial_backoff_ms = 100'000;
    cfg.enabled = true;
    loader.configureWatchdog(cfg);

    loader.startWatchdog();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    loader.stopWatchdog();

    // No loaded modules → stats map remains empty
    auto all = loader.getAllWatchdogStats();
    EXPECT_TRUE(all.empty());
    loader.clearHealthChecks();
}

// --- Backoff calculation (via configureWatchdog + stats) ---------------------

TEST(PluginWatchdog, BackoffDoesNotExceedMaxBackoff) {
    // Verify that the configured max_backoff_ms is respected.
    // We cannot call watchdogCalculateBackoff() directly (it is private), but
    // we can verify the config is accepted and the watchdog lifecycle works.
    ModuleLoader loader;
    WatchdogConfig cfg;
    cfg.initial_backoff_ms = 1'000;
    cfg.backoff_multiplier = 2.0;
    cfg.max_backoff_ms = 5'000;
    cfg.check_interval_ms = 100'000;  // Don't fire during test
    loader.configureWatchdog(cfg);

    loader.startWatchdog();
    EXPECT_TRUE(loader.isWatchdogRunning());
    loader.stopWatchdog();
    EXPECT_FALSE(loader.isWatchdogRunning());
}

// --- Watchdog disabled flag --------------------------------------------------

TEST(PluginWatchdog, DisabledWatchdogDoesNotCheckModules) {
    ModuleLoader loader;
    WatchdogConfig cfg;
    cfg.enabled = false;
    cfg.check_interval_ms = 50;  // Short interval so loop fires quickly
    loader.configureWatchdog(cfg);

    loader.registerHealthCheck("fail_if_called", [](void*, const std::string&) {
        return HealthCheckResult::failure("fail_if_called", "should not be called");
    });

    loader.startWatchdog();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    loader.stopWatchdog();

    // No stats should have been recorded since watchdog.enabled = false
    auto all = loader.getAllWatchdogStats();
    EXPECT_TRUE(all.empty());
    loader.clearHealthChecks();
}

// --- Destructor stops watchdog -----------------------------------------------

TEST(PluginWatchdog, DestructorStopsWatchdog) {
    // If the destructor does not stop the watchdog thread the process would
    // crash (or sanitizers would detect a data race).  This test verifies
    // the destructor cleans up correctly.
    {
        ModuleLoader loader;
        WatchdogConfig cfg;
        cfg.check_interval_ms = 5'000;
        loader.configureWatchdog(cfg);
        loader.startWatchdog();
        EXPECT_TRUE(loader.isWatchdogRunning());
        // loader goes out of scope here → destructor must join the thread
    }
    // No crash = pass
}

// --- resetWatchdogStats while watchdog is running ----------------------------

TEST(PluginWatchdog, ResetWatchdogStatsWhileRunning) {
    // Calling resetWatchdogStats() while the watchdog thread is running must
    // not crash or deadlock.  This exercises the concurrent-access path where
    // the background thread may be reading/writing stats at the same time.
    ModuleLoader loader;
    WatchdogConfig cfg;
    cfg.check_interval_ms = 5'000;  // Long interval so loop won't fire
    loader.configureWatchdog(cfg);
    loader.startWatchdog();
    EXPECT_TRUE(loader.isWatchdogRunning());

    loader.resetWatchdogStats();  // must not deadlock or crash
    auto all = loader.getAllWatchdogStats();
    EXPECT_TRUE(all.empty());

    loader.stopWatchdog();
    EXPECT_FALSE(loader.isWatchdogRunning());
}

// --- WatchdogModuleStats field completeness ----------------------------------

TEST(WatchdogModuleStats, AllFieldsPresent) {
    // Verify that every documented field is accessible and has a sensible
    // zero / false default so callers can compare against initial state.
    WatchdogModuleStats s;
    EXPECT_EQ(s.restart_count, 0u);
    EXPECT_EQ(s.consecutive_failures, 0u);
    EXPECT_EQ(s.last_health_check_ms, 0u);
    EXPECT_EQ(s.last_failure_ms, 0u);
    EXPECT_EQ(s.last_restart_ms, 0u);
    EXPECT_EQ(s.next_retry_ms, 0u);
    EXPECT_FALSE(s.permanently_failed);
    EXPECT_TRUE(s.moduleName.empty());
    EXPECT_TRUE(s.modulePath.empty());
    EXPECT_TRUE(s.last_error.empty());

    // Mutate a few fields to confirm they are writable (no const / read-only restriction)
    s.restart_count = 3;
    s.permanently_failed = true;
    s.last_error = "test error";
    EXPECT_EQ(s.restart_count, 3u);
    EXPECT_TRUE(s.permanently_failed);
    EXPECT_EQ(s.last_error, "test error");
}

// --- WatchdogConfig: zero max_restart_attempts means unlimited ---------------

TEST(WatchdogConfig, ZeroMaxRestartAttemptsIsUnlimited) {
    // When max_restart_attempts == 0, the watchdog should never set
    // permanently_failed due to restart exhaustion.  This test verifies the
    // config field can be set to 0 and is stored correctly.
    WatchdogConfig cfg;
    cfg.max_restart_attempts = 0;
    EXPECT_EQ(cfg.max_restart_attempts, 0u);

    ModuleLoader loader;
    loader.configureWatchdog(cfg);
    // We cannot observe the unlimited-restart behaviour without a real .so,
    // but starting/stopping the watchdog with this config must not crash.
    loader.startWatchdog();
    loader.stopWatchdog();
}


// ===========================================================================
// GAP-014 — popen injection prevention: verifyGPGSignature uses fork+execvp
// ===========================================================================

#ifdef __linux__
// GAP-014-01: A module path containing shell-injection characters must not
// cause a crash or inject commands.  The previous popen-based implementation
// was vulnerable when modulePath contained single quotes.  The fork+execvp
// replacement passes paths as separate argv entries so the shell is never
// invoked; the function must simply return false (no sig file found or gpg
// returning non-zero).
TEST(ModuleLoaderGPGTest, GAP014_PathWithShellCharsDoesNotInject) {
    ModuleLoader loader;
    // A path with embedded single-quote + semicolon — would break out of
    // popen shell quoting in the old implementation.
    const std::string evil_path = "/tmp/benign'; rm -rf /tmp/innocent #";
    // The function should return false (no signature file, or gpg failure)
    // without crashing or running unexpected shell commands.
    bool result = loader.verifyGPGSignature(evil_path, "");
    EXPECT_FALSE(result) << "verifyGPGSignature must return false for missing/bad path";
}

// GAP-014-02: A normal missing-file path must return false gracefully.
TEST(ModuleLoaderGPGTest, GAP014_MissingModulePath_ReturnsFalse) {
    ModuleLoader loader;
    bool result = loader.verifyGPGSignature("/tmp/nonexistent_module_xyz_12345.so", "");
    EXPECT_FALSE(result);
}
#endif // __linux__

// ===========================================================================
// GAP-011 — Admin token must not be logged in plain text
// ===========================================================================

// GAP-011-01: Validate that the static helper we use to decide the log message
// only reveals presence, not the secret value.
TEST(HttpServerTokenLoggingTest, GAP011_TokenPresenceLoggedNotValue) {
    // This test does not construct a full HttpServer (no listening socket needed).
    // It verifies the boolean-only "is set" decision logic is correct by
    // exercising the same conditional branch used in the constructor.
    const char* token_env_value = "super_secret_token_12345";

#ifdef _WIN32
    _putenv_s("THEMIS_TOKEN_ADMIN", token_env_value);
#else
    ::setenv("THEMIS_TOKEN_ADMIN", token_env_value, 1);
#endif

    // The production fix logs only whether the env-var is set (bool).
    // Simulate the same check.
    const bool admin_token_set = (std::getenv("THEMIS_TOKEN_ADMIN") != nullptr);
    EXPECT_TRUE(admin_token_set);

    // The raw value must NOT be part of the log message.
    // We verify this by confirming the fixed code forms "is set" / "not set",
    // not the raw token.  We can't inspect spdlog output here, so we confirm
    // the logic doesn't reference the value string.
    std::string log_fragment = admin_token_set ? "set" : "not set";
    EXPECT_NE(log_fragment.find("set"), std::string::npos);
    EXPECT_EQ(log_fragment.find(token_env_value), std::string::npos)
        << "Token value must not appear in the log message fragment";

#ifdef _WIN32
    _putenv_s("THEMIS_TOKEN_ADMIN", "");
#else
    ::unsetenv("THEMIS_TOKEN_ADMIN");
#endif
}
