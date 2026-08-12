#include <gtest/gtest.h>
#include "security/binary_manifest.h"
#include "security/manifest_signer.h"
#include "security/signing.h"
#include <filesystem>
#include <fstream>

using namespace themis::security;

/**
 * @brief Test binary manifest creation and serialization
 */
TEST(BinaryManifestTest, CreateAndSerialize) {
    BinaryManifest::Metadata metadata;
    metadata.version = "1.4.0";
    metadata.build_id = "abc123";
    metadata.timestamp = std::chrono::system_clock::now();
    metadata.release_type = "release";
    metadata.platform = "linux-x64";
    
    BinaryManifest manifest(metadata);
    
    BinaryFileEntry file1;
    file1.path = "bin/themisdb";
    file1.sha256_hash = "abcdef1234567890";
    file1.size_bytes = 1024000;
    file1.version = "1.4.0";
    
    manifest.addFile(file1);
    
    // Serialize to JSON
    nlohmann::json j = manifest.to_json();
    
    EXPECT_EQ(j["metadata"]["version"], "1.4.0");
    EXPECT_EQ(j["metadata"]["build_id"], "abc123");
    EXPECT_EQ(j["files"].size(), 1);
    EXPECT_EQ(j["files"][0]["path"], "bin/themisdb");
    
    // Deserialize from JSON
    BinaryManifest manifest2 = BinaryManifest::from_json(j);
    
    EXPECT_EQ(manifest2.getMetadata().version, "1.4.0");
    EXPECT_EQ(manifest2.getFiles().size(), 1);
    EXPECT_EQ(manifest2.getFiles()[0].path, "bin/themisdb");
}

/**
 * @brief Test canonical JSON generation
 */
TEST(BinaryManifestTest, CanonicalJson) {
    BinaryManifest::Metadata metadata;
    metadata.version = "1.4.0";
    metadata.build_id = "test";
    metadata.timestamp = std::chrono::system_clock::now();
    metadata.release_type = "release";
    metadata.platform = "linux-x64";
    
    BinaryManifest manifest(metadata);
    
    // Add files in different order
    BinaryFileEntry file1{"bin/a", "hash1", 100, "1.4.0"};
    BinaryFileEntry file2{"bin/b", "hash2", 200, "1.4.0"};
    
    manifest.addFile(file1);
    manifest.addFile(file2);
    
    std::string canonical1 = manifest.getCanonicalJson();
    
    // Create another manifest with same data
    BinaryManifest manifest2(metadata);
    manifest2.addFile(file1);
    manifest2.addFile(file2);
    
    std::string canonical2 = manifest2.getCanonicalJson();
    
    // Should produce identical canonical JSON
    EXPECT_EQ(canonical1, canonical2);
}

/**
 * @brief Test signed manifest serialization
 */
TEST(SignedManifestTest, SerializeAndDeserialize) {
    BinaryManifest::Metadata metadata;
    metadata.version = "1.4.0";
    metadata.build_id = "test";
    metadata.timestamp = std::chrono::system_clock::now();
    metadata.release_type = "release";
    metadata.platform = "linux-x64";
    
    BinaryManifest manifest(metadata);
    
    SignedManifest signed_manifest;
    signed_manifest.manifest = manifest;
    signed_manifest.signature_base64 = "base64signature";
    signed_manifest.signature_algorithm = "RSA-4096-SHA256";
    signed_manifest.signer_id = "release_key";
    
    // Serialize
    nlohmann::json j = signed_manifest.to_json();
    
    EXPECT_EQ(j["signature"], "base64signature");
    EXPECT_EQ(j["signature_algorithm"], "RSA-4096-SHA256");
    EXPECT_EQ(j["signer_id"], "release_key");
    
    // Deserialize
    SignedManifest signed_manifest2 = SignedManifest::from_json(j);
    
    EXPECT_EQ(signed_manifest2.signature_base64, "base64signature");
    EXPECT_EQ(signed_manifest2.signature_algorithm, "RSA-4096-SHA256");
    EXPECT_EQ(signed_manifest2.signer_id, "release_key");
}

/**
 * @brief Test file save and load
 */
TEST(SignedManifestTest, SaveAndLoad) {
    BinaryManifest::Metadata metadata;
    metadata.version = "1.4.0";
    metadata.build_id = "test";
    metadata.timestamp = std::chrono::system_clock::now();
    metadata.release_type = "release";
    metadata.platform = "linux-x64";
    
    BinaryManifest manifest(metadata);
    
    SignedManifest signed_manifest;
    signed_manifest.manifest = manifest;
    signed_manifest.signature_base64 = "test_signature";
    signed_manifest.signature_algorithm = "RSA-4096-SHA256";
    signed_manifest.signer_id = "test_key";
    
    // Save to file
    std::string temp_path = "/tmp/test_manifest.json";
    bool saved = signed_manifest.saveToFile(temp_path);
    EXPECT_TRUE(saved);
    
    // Load from file
    SignedManifest loaded = SignedManifest::loadFromFile(temp_path);
    
    EXPECT_EQ(loaded.signature_base64, "test_signature");
    EXPECT_EQ(loaded.manifest.getMetadata().version, "1.4.0");
    
    // Clean up
    std::filesystem::remove(temp_path);
}

/**
 * @brief Test SHA-256 file hashing
 */
TEST(ManifestSignerTest, ComputeFileSHA256) {
    // Create temporary test file
    std::string temp_file = "/tmp/test_file.txt";
    {
        std::ofstream file(temp_file);
        file << "Hello, World!";
    }
    
    // Compute hash
    std::string hash = ManifestSigner::computeFileSHA256(temp_file);
    
    // Known SHA-256 of "Hello, World!"
    EXPECT_EQ(hash, "dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f");
    
    // Clean up
    std::filesystem::remove(temp_file);
}

/**
 * @brief Test manifest generation from directory
 */
TEST(ManifestSignerTest, GenerateManifest) {
    // Create temporary directory with test files
    std::string temp_dir = "/tmp/test_binaries";
    std::filesystem::create_directories(temp_dir);
    
    // Create test files
    {
        std::ofstream file(temp_dir + "/binary1.exe");
        file << "Binary 1 content";
    }
    {
        std::ofstream file(temp_dir + "/binary2.so");
        file << "Binary 2 content";
    }
    {
        std::ofstream file(temp_dir + "/readme.txt");
        file << "README";
    }
    
    // Create manifest signer with mock signing service
    auto signing_service = themis::createMockSigningService();
    ManifestSigner::Config config;
    config.key_id = "test_key";
    
    ManifestSigner signer(signing_service, config);
    
    // Generate manifest (include all files)
    BinaryManifest manifest = signer.generateManifest(
        temp_dir,
        "1.4.0",
        "test_build",
        {"*.exe", "*.so", "*.txt"}
    );
    
    EXPECT_EQ(manifest.getMetadata().version, "1.4.0");
    EXPECT_EQ(manifest.getMetadata().build_id, "test_build");
    EXPECT_EQ(manifest.getFiles().size(), 3);
    
    // Verify files are in manifest
    bool found_exe = false, found_so = false, found_txt = false;
    for (const auto& file : manifest.getFiles()) {
        if (file.path.find("binary1.exe") != std::string::npos) found_exe = true;
        if (file.path.find("binary2.so") != std::string::npos) found_so = true;
        if (file.path.find("readme.txt") != std::string::npos) found_txt = true;
    }
    
    EXPECT_TRUE(found_exe);
    EXPECT_TRUE(found_so);
    EXPECT_TRUE(found_txt);
    
    // Clean up
    std::filesystem::remove_all(temp_dir);
}

/**
 * @brief Test pattern matching
 */
TEST(ManifestSignerTest, PatternMatching) {
    auto signing_service = themis::createMockSigningService();
    ManifestSigner::Config config;
    config.key_id = "test_key";
    
    ManifestSigner signer(signing_service, config);
    
    // Create temp directory with various files
    std::string temp_dir = "/tmp/test_patterns";
    std::filesystem::create_directories(temp_dir);
    
    {
        std::ofstream(temp_dir + "/app.exe") << "exe";
        std::ofstream(temp_dir + "/lib.dll") << "dll";
        std::ofstream(temp_dir + "/readme.txt") << "txt";
        std::ofstream(temp_dir + "/config.json") << "json";
    }
    
    // Generate manifest with only *.exe and *.dll
    BinaryManifest manifest = signer.generateManifest(
        temp_dir,
        "1.0",
        "test",
        {"*.exe", "*.dll"}
    );
    
    EXPECT_EQ(manifest.getFiles().size(), 2);
    
    for (const auto& file : manifest.getFiles()) {
        EXPECT_TRUE(file.path.find(".exe") != std::string::npos || 
                   file.path.find(".dll") != std::string::npos);
    }
    
    std::filesystem::remove_all(temp_dir);
}

/**
 * @brief Test manifest signing and verification
 */
TEST(ManifestSignerTest, SignAndVerify) {
    // Create manifest
    BinaryManifest::Metadata metadata;
    metadata.version = "1.4.0";
    metadata.build_id = "test";
    metadata.timestamp = std::chrono::system_clock::now();
    metadata.release_type = "release";
    metadata.platform = "linux-x64";
    
    BinaryManifest manifest(metadata);
    
    // Create signing service
    auto signing_service = themis::createMockSigningService();
    ManifestSigner::Config config;
    config.key_id = "test_key";
    
    ManifestSigner signer(signing_service, config);
    
    // Sign manifest
    SignedManifest signed_manifest = signer.signManifest(manifest);
    
    EXPECT_FALSE(signed_manifest.signature_base64.empty());
    EXPECT_EQ(signed_manifest.signature_algorithm, "RSA-4096-SHA256");
    EXPECT_EQ(signed_manifest.signer_id, "test_key");
    
    // Verify signature
    bool valid = signer.verifySignature(signed_manifest);
    EXPECT_TRUE(valid);
}

/**
 * @brief Test binary verification
 */
TEST(ManifestSignerTest, VerifyBinaries) {
    // Create test directory with files
    std::string temp_dir = "/tmp/test_verify";
    std::filesystem::create_directories(temp_dir);
    
    std::string file_content = "Test binary content";
    {
        std::ofstream file(temp_dir + "/test.bin");
        file << file_content;
    }
    
    // Generate manifest
    auto signing_service = themis::createMockSigningService();
    ManifestSigner::Config config;
    config.key_id = "test_key";
    
    ManifestSigner signer(signing_service, config);
    
    BinaryManifest manifest = signer.generateManifest(
        temp_dir,
        "1.0",
        "test",
        {"*.bin"}
    );
    
    // Sign manifest
    SignedManifest signed_manifest = signer.signManifest(manifest);
    
    // Verify binaries (should pass)
    auto result = signer.verifyBinaries(signed_manifest, temp_dir);
    
    EXPECT_TRUE(result.signature_valid);
    EXPECT_TRUE(result.files_valid);
    EXPECT_TRUE(result.missing_files.empty());
    EXPECT_TRUE(result.modified_files.empty());
    
    // Modify file
    {
        std::ofstream file(temp_dir + "/test.bin");
        file << "Modified content";
    }
    
    // Verify again (should fail)
    result = signer.verifyBinaries(signed_manifest, temp_dir);
    
    EXPECT_TRUE(result.signature_valid);  // Signature still valid
    EXPECT_FALSE(result.files_valid);     // But files are invalid
    EXPECT_FALSE(result.modified_files.empty());
    
    std::filesystem::remove_all(temp_dir);
}

/**
 * @brief Test missing file detection
 */
TEST(ManifestSignerTest, DetectMissingFiles) {
    std::string temp_dir = "/tmp/test_missing";
    std::filesystem::create_directories(temp_dir);
    
    {
        std::ofstream(temp_dir + "/file1.bin") << "content1";
        std::ofstream(temp_dir + "/file2.bin") << "content2";
    }
    
    auto signing_service = themis::createMockSigningService();
    ManifestSigner::Config config;
    config.key_id = "test_key";
    
    ManifestSigner signer(signing_service, config);
    
    BinaryManifest manifest = signer.generateManifest(temp_dir, "1.0", "test", {"*.bin"});
    SignedManifest signed_manifest = signer.signManifest(manifest);
    
    // Remove one file
    std::filesystem::remove(temp_dir + "/file1.bin");
    
    // Verify (should detect missing file)
    auto result = signer.verifyBinaries(signed_manifest, temp_dir);
    
    EXPECT_TRUE(result.signature_valid);
    EXPECT_FALSE(result.files_valid);
    EXPECT_EQ(result.missing_files.size(), 1);
    EXPECT_TRUE(result.missing_files[0].find("file1.bin") != std::string::npos);
    
    std::filesystem::remove_all(temp_dir);
}

/**
 * @brief Test startup verifier
 */
TEST(StartupVerifierTest, VerifyOnStartup) {
    // Create test directory
    std::string temp_dir = "/tmp/test_startup";
    std::filesystem::create_directories(temp_dir);
    
    {
        std::ofstream(temp_dir + "/app.exe") << "application";
    }
    
    // Generate and sign manifest
    auto signing_service = themis::createMockSigningService();
    ManifestSigner::Config signer_config;
    signer_config.key_id = "release_key";
    
    ManifestSigner signer(signing_service, signer_config);
    BinaryManifest manifest = signer.generateManifest(temp_dir, "1.0", "test", {"*.exe"});
    SignedManifest signed_manifest = signer.signManifest(manifest);
    
    // Save manifest
    std::string manifest_path = temp_dir + "/manifest.json";
    signed_manifest.saveToFile(manifest_path);
    
    // Create startup verifier
    StartupVerifier::Config verifier_config;
    verifier_config.manifest_path = manifest_path;
    verifier_config.binaries_root = temp_dir;
    verifier_config.fail_on_invalid = false;  // Don't exit on failure in test
    
    StartupVerifier verifier(signing_service, verifier_config);
    
    // Verify (should pass)
    bool valid = verifier.verify();
    EXPECT_TRUE(valid);
    
    const auto& result = verifier.getResult();
    EXPECT_TRUE(result.signature_valid);
    EXPECT_TRUE(result.files_valid);
    
    std::filesystem::remove_all(temp_dir);
}
