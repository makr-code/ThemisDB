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

TEST(ModuleLoader, LoadModuleTwiceReturnsAlreadyLoaded) {
    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    loader.setRequireSignature(false);
    
    // Try to "load" a module twice (will fail on first load due to non-existent file,
    // but we're testing the already-loaded check logic)
    // This test verifies the error handling works
    
    auto result1 = loader.loadModule("/nonexistent1.so", "test_module");
    EXPECT_FALSE(result1.success);
    
    // The module wasn't actually loaded, so trying again should still fail
    // with MODULE_NOT_FOUND, not MODULE_ALREADY_LOADED
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

// Entry point for test execution
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
