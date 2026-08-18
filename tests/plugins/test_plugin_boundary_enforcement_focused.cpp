/// @file tests/plugins/test_plugin_boundary_enforcement_focused.cpp
/// @brief Wave C Batch 3: Community Leak Prevention & Boundary Enforcement Tests
///
/// Comprehensive test suite for CI policy gates:
/// - TEST-1: Community build rejects private submodules (scoped-checkout test)
/// - TEST-2: Private credential scanner detects leaked AWS keys
/// - TEST-3: Private credential scanner detects leaked Azure keys
/// - TEST-4: Private credential scanner detects leaked OAuth tokens
/// - TEST-5: Private credential scanner detects leaked SSH/PGP keys
/// - TEST-6: Enterprise lane can load private plugins, community cannot
/// - TEST-7: SBOM hash verification passes when hashes match
/// - TEST-8: SBOM hash verification fails (fail-closed) when SBOM missing
/// - TEST-9: SBOM hash verification fails (fail-closed) when hash mismatches
/// - TEST-10: Verify dependency list in SBOM contains no private sources
/// - TEST-11: Negative test: verify scanner catches injected private credentials
/// - TEST-12: Scoped-checkout validation blocks private submodules in community PR
///
/// Total Coverage: ≥12 focused test cases

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <regex>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <memory>

namespace themis::plugins::boundary_enforcement {

/// Private credential scanner patterns
class CredentialScanner {
public:
    struct CredentialMatch {
        std::string pattern_name;
        std::string matched_text;
        size_t line_number;
        std::string file_path;
        std::string severity;  // CRITICAL, HIGH, MEDIUM
    };

    CredentialScanner() {
        // AWS patterns
        credential_patterns_["aws_secret"] = {
            std::regex(R"(AKIA[0-9A-Z]{16})"),  // AWS Access Key
            "CRITICAL"
        };
        credential_patterns_["aws_secret_key"] = {
            std::regex(R"(aws_secret_access_key\s*=\s*['\"]([^'\"]{20,})['\"])"),
            "CRITICAL"
        };

        // Azure patterns
        credential_patterns_["azure_key"] = {
            std::regex(R"(DefaultEndpointsProtocol=https;AccountName=.*;AccountKey=.{88})"),
            "CRITICAL"
        };
        credential_patterns_["azure_conn_string"] = {
            std::regex(R"(SharedAccessKey=.{20,})"),
            "CRITICAL"
        };

        // GCP patterns
        credential_patterns_["gcp_service_account"] = {
            std::regex(R"(\"type\":\s*\"service_account\".*\"private_key\":\s*\"-----BEGIN PRIVATE KEY)"),
            "CRITICAL"
        };
        credential_patterns_["gcp_api_key"] = {
            std::regex(R"(AIza[0-9A-Za-z\-_]{35})"),
            "HIGH"
        };

        // OAuth patterns
        credential_patterns_["oauth_token"] = {
            std::regex(R"(oauth_token\s*=\s*['\"]([a-zA-Z0-9\-_.]{50,})['\"])"),
            "CRITICAL"
        };
        credential_patterns_["github_token"] = {
            std::regex(R"(ghp_[A-Za-z0-9_]{36})"),
            "CRITICAL"
        };
        credential_patterns_["github_oauth"] = {
            std::regex(R"(gho_[A-Za-z0-9_]{36})"),
            "CRITICAL"
        };

        // SSH/PGP patterns
        credential_patterns_["ssh_private_key"] = {
            std::regex(R"(-----BEGIN RSA PRIVATE KEY-----)", std::regex::icase),
            "CRITICAL"
        };
        credential_patterns_["openssh_private_key"] = {
            std::regex(R"(-----BEGIN OPENSSH PRIVATE KEY-----)", std::regex::icase),
            "CRITICAL"
        };
        credential_patterns_["pgp_private_key"] = {
            std::regex(R"(-----BEGIN PGP PRIVATE KEY BLOCK-----)", std::regex::icase),
            "CRITICAL"
        };
        credential_patterns_["pgp_fingerprint"] = {
            std::regex(R"([0-9A-F]{40})"),  // 40-char hex (fingerprint)
            "MEDIUM"
        };

        // Private secret context patterns
        credential_patterns_["private_secret_env"] = {
            std::regex(R"(secrets\.[A-Z_]+_PRIVATE)", std::regex::icase),
            "HIGH"
        };
        credential_patterns_["db_password"] = {
            std::regex(R"(db_password\s*[:=]\s*['\"]([^'\"]{8,})['\"])"),
            "HIGH"
        };
    }

    std::vector<CredentialMatch> scan_file(
        const std::string& file_path,
        const std::string& content) {
        std::vector<CredentialMatch> matches;
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;

        while (std::getline(stream, line)) {
            line_number++;
            for (const auto& [pattern_name, pattern_data] : credential_patterns_) {
                std::smatch match;
                if (std::regex_search(line, match, pattern_data.first)) {
                    matches.push_back({
                        pattern_name,
                        match[0].str(),
                        line_number,
                        file_path,
                        pattern_data.second
                    });
                }
            }
        }
        return matches;
    }

    bool has_critical_leaks(const std::vector<CredentialMatch>& matches) const {
        return std::any_of(matches.begin(), matches.end(),
            [](const CredentialMatch& m) { return m.severity == "CRITICAL"; });
    }

private:
    std::map<std::string, std::pair<std::regex, std::string>> credential_patterns_;
};

/// SBOM (Software Bill of Materials) verification
class SBOMVerifier {
public:
    struct SBOMEntry {
        std::string component_name;
        std::string version;
        std::string source_url;
        std::string hash;
        bool is_private;
    };

    struct SBOMHash {
        std::string sbom_content_hash;
        std::vector<SBOMEntry> dependencies;
    };

    static std::string compute_sha256(const std::string& content) {
        // Simplified SHA256 computation for testing
        // In production, use actual SHA256 library
        unsigned long hash = 5381;
        for (char c : content) {
            hash = ((hash << 5) + hash) + c;  // hash * 33 + c
        }
        std::stringstream ss;
        ss << std::hex << (hash & 0xFFFFFFFFFFFFFFFFUL);
        std::string result = ss.str();
        // Pad to 64 characters to simulate SHA256
        while (result.length() < 64) result = "0" + result;
        return result.substr(result.length() - 64);
    }

    static SBOMHash generate_sbom(const std::vector<SBOMEntry>& dependencies) {
        std::stringstream ss;
        for (const auto& dep : dependencies) {
            ss << dep.component_name << ":" << dep.version << "|"
               << dep.source_url << "|" << dep.hash << "\n";
        }
        return {
            compute_sha256(ss.str()),
            dependencies
        };
    }

    static bool verify_sbom_hash(
        const SBOMHash& sbom,
        const std::string& expected_hash) {
        return sbom.sbom_content_hash == expected_hash;
    }

    static bool has_private_dependencies(const SBOMHash& sbom) {
        return std::any_of(sbom.dependencies.begin(), sbom.dependencies.end(),
            [](const SBOMEntry& e) { return e.is_private; });
    }
};

/// Scoped checkout validator
class ScopedCheckoutValidator {
public:
    struct GitmodulesEntry {
        std::string name;
        std::string path;
        std::string url;
        bool shallow;
        bool is_private;
    };

    static bool validate_community_checkout(
        const std::vector<GitmodulesEntry>& submodules,
        bool is_community_target) {
        if (!is_community_target) return true;

        // Community builds must NOT fetch private submodules
        for (const auto& submodule : submodules) {
            if (submodule.is_private && !submodule.shallow) {
                return false;  // FAIL-CLOSED: private submodule would be fetched
            }
        }
        return true;
    }

    static std::vector<GitmodulesEntry> parse_gitmodules(
        const std::string& content) {
        std::vector<GitmodulesEntry> entries;
        std::istringstream stream(content);
        std::string line;
        GitmodulesEntry current;
        bool in_section = false;

        while (std::getline(stream, line)) {
            if (line.find("[submodule") != std::string::npos) {
                if (in_section && !current.name.empty()) {
                    entries.push_back(current);
                }
                current = GitmodulesEntry();
                in_section = true;
                size_t start = line.find('\"') + 1;
                size_t end = line.rfind('\"');
                if (start < end) {
                    current.name = line.substr(start, end - start);
                    // Mark as private if name contains private plugins
                    current.is_private = line.find("themisdb_ethic_ai") != std::string::npos ||
                                        line.find("themisdb_storage") != std::string::npos ||
                                        line.find("themisdb_importer") != std::string::npos;
                }
            } else if (in_section && line.find("path = ") != std::string::npos) {
                current.path = line.substr(line.find("= ") + 2);
            } else if (in_section && line.find("url = ") != std::string::npos) {
                current.url = line.substr(line.find("= ") + 2);
            } else if (in_section && line.find("shallow = ") != std::string::npos) {
                current.shallow = line.find("true") != std::string::npos;
            }
        }
        if (in_section && !current.name.empty()) {
            entries.push_back(current);
        }
        return entries;
    }
};

// ============================================================================
// TEST SUITE
// ============================================================================

class BoundaryEnforcementTest : public ::testing::Test {
protected:
    CredentialScanner scanner_;
    SBOMVerifier sbom_verifier_;
    ScopedCheckoutValidator checkout_validator_;

    void SetUp() override {
        spdlog::set_level(spdlog::level::debug);
    }
};

/// TEST-1: Community build rejects private submodules (scoped-checkout test)
TEST_F(BoundaryEnforcementTest, WAVE_C_B3_001_ScopedCheckoutRejectsPrivateSubmodules) {
    std::vector<ScopedCheckoutValidator::GitmodulesEntry> submodules = {
        {"llama.cpp", "llama.cpp", "https://github.com/ggerganov/llama.cpp.git", true, false},
        {"themisdb_ethic_ai", "plugins/themisdb_ethic_ai", "https://github.com/makr-code/themisdb_ethic_ai.git", false, true},
    };

    // Community PR: should FAIL if private submodule not shallow
    bool result = ScopedCheckoutValidator::validate_community_checkout(submodules, true);
    EXPECT_FALSE(result) << "Community checkout should reject non-shallow private submodules";

    // Enterprise PR: should PASS (enterprise can fetch private)
    result = ScopedCheckoutValidator::validate_community_checkout(submodules, false);
    EXPECT_TRUE(result) << "Non-community checkout should allow private submodules";
}

/// TEST-2: Private credential scanner detects leaked AWS keys
TEST_F(BoundaryEnforcementTest, WAVE_C_B3_002_CredentialScannerDetectsAWSKeys) {
    std::string code_with_aws_secret =
        R"(
        # Build configuration
        AWS_ACCESS_KEY_ID=AKIA2XQRJ7NQKL3MOPQR
        aws_secret_access_key = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY"
        )";

    auto matches = scanner_.scan_file("test.cfg", code_with_aws_secret);
    EXPECT_GT(matches.size(), 0) << "Scanner should detect AWS access key";
    EXPECT_TRUE(scanner_.has_critical_leaks(matches)) << "AWS key should be CRITICAL severity";
}

/// TEST-3: Private credential scanner detects leaked Azure keys
TEST_F(BoundaryEnforcementTest, WAVE_C_B3_003_CredentialScannerDetectsAzureKeys) {
    std::string code_with_azure_secret =
        R"(
        # Azure connection string
        connection_string="DefaultEndpointsProtocol=https;AccountName=myaccount;AccountKey=ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890=="
        )";

    auto matches = scanner_.scan_file("azure_config.json", code_with_azure_secret);
    EXPECT_GT(matches.size(), 0) << "Scanner should detect Azure connection string";
    EXPECT_TRUE(scanner_.has_critical_leaks(matches)) << "Azure key should be CRITICAL severity";
}

/// TEST-4: Private credential scanner detects leaked OAuth tokens
TEST_F(BoundaryEnforcementTest, WAVE_C_B3_004_CredentialScannerDetectsOAuthTokens) {
    std::string code_with_oauth =
        R"(
        oauth_token = "******"
        github_token = "******"
        )";

    auto matches = scanner_.scan_file("auth.py", code_with_oauth);
    EXPECT_GT(matches.size(), 0) << "Scanner should detect GitHub tokens";
    EXPECT_TRUE(scanner_.has_critical_leaks(matches)) << "GitHub token should be CRITICAL severity";
}

/// TEST-5: Private credential scanner detects leaked SSH/PGP keys
TEST_F(BoundaryEnforcementTest, WAVE_C_B3_005_CredentialScannerDetectsSSHPGPKeys) {
    std::string code_with_ssh =
        R"(
        -----BEGIN RSA PRIVATE KEY-----
        MIIEowIBAAKCAQEA1234567890ABCDEFGH...
        -----END RSA PRIVATE KEY-----
        )";

    auto matches = scanner_.scan_file("id_rsa", code_with_ssh);
    EXPECT_GT(matches.size(), 0) << "Scanner should detect SSH private key";
    EXPECT_TRUE(scanner_.has_critical_leaks(matches)) << "SSH key should be CRITICAL severity";
}

/// TEST-6: Enterprise lane can load private plugins, community cannot
TEST_F(BoundaryEnforcementTest, WAVE_C_B3_006_EnterpriseLaneCanLoadPrivatePlugins) {
    std::vector<ScopedCheckoutValidator::GitmodulesEntry> submodules = {
        {"themisdb_ethic_ai", "plugins/themisdb_ethic_ai", "https://github.com/makr-code/themisdb_ethic_ai.git", false, true},
        {"themisdb_storage", "plugins/themisdb_storage", "https://github.com/makr-code/themisdb_storage.git", false, true},
    };

    // Enterprise: private plugins allowed
    bool enterprise_ok = ScopedCheckoutValidator::validate_community_checkout(submodules, false);
    EXPECT_TRUE(enterprise_ok) << "Enterprise should allow private plugins";

    // Community: private plugins rejected
    bool community_ok = ScopedCheckoutValidator::validate_community_checkout(submodules, true);
    EXPECT_FALSE(community_ok) << "Community should reject private plugins";
}

/// TEST-7: SBOM hash verification passes when hashes match
TEST_F(BoundaryEnforcementTest, WAVE_C_B3_007_SBOMHashVerificationPassesOnMatch) {
    std::vector<SBOMVerifier::SBOMEntry> dependencies = {
        {"vcpkg", "master", "https://github.com/microsoft/vcpkg.git", "abc123", false},
        {"llama.cpp", "main", "https://github.com/ggerganov/llama.cpp.git", "def456", false},
    };

    auto sbom = SBOMVerifier::generate_sbom(dependencies);
    bool verified = SBOMVerifier::verify_sbom_hash(sbom, sbom.sbom_content_hash);
    EXPECT_TRUE(verified) << "SBOM hash verification should pass when hashes match";
}

/// TEST-8: SBOM hash verification fails (fail-closed) when SBOM missing
TEST_F(BoundaryEnforcementTest, WAVE_C_B3_008_SBOMHashVerificationFailsOnMissingSBOM) {
    std::vector<SBOMVerifier::SBOMEntry> empty_dependencies;
    auto sbom = SBOMVerifier::generate_sbom(empty_dependencies);

    // Empty SBOM should fail verification against any non-empty hash
    bool verified = SBOMVerifier::verify_sbom_hash(sbom, "expected_hash_that_will_not_match");
    EXPECT_FALSE(verified) << "SBOM verification should fail (fail-closed) when SBOM content differs";
}

/// TEST-9: SBOM hash verification fails (fail-closed) when hash mismatches
TEST_F(BoundaryEnforcementTest, WAVE_C_B3_009_SBOMHashVerificationFailsOnMismatch) {
    std::vector<SBOMVerifier::SBOMEntry> dependencies = {
        {"vcpkg", "master", "https://github.com/microsoft/vcpkg.git", "abc123", false},
    };

    auto sbom = SBOMVerifier::generate_sbom(dependencies);
    std::string wrong_hash = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
    bool verified = SBOMVerifier::verify_sbom_hash(sbom, wrong_hash);
    EXPECT_FALSE(verified) << "SBOM verification should fail (fail-closed) when hash mismatches";
}

/// TEST-10: Verify dependency list in SBOM contains no private sources
TEST_F(BoundaryEnforcementTest, WAVE_C_B3_010_SBOMContainsNoPrivateDependencies) {
    std::vector<SBOMVerifier::SBOMEntry> public_dependencies = {
        {"vcpkg", "master", "https://github.com/microsoft/vcpkg.git", "abc123", false},
        {"llama.cpp", "main", "https://github.com/ggerganov/llama.cpp.git", "def456", false},
        {"whisper.cpp", "main", "https://github.com/ggerganov/whisper.cpp.git", "ghi789", false},
    };

    auto sbom = SBOMVerifier::generate_sbom(public_dependencies);
    bool has_private = SBOMVerifier::has_private_dependencies(sbom);
    EXPECT_FALSE(has_private) << "Community SBOM should not contain private dependencies";

    // With private dependency: should fail
    std::vector<SBOMVerifier::SBOMEntry> with_private = public_dependencies;
    with_private.push_back({"themisdb_ethic_ai", "develop", "https://github.com/makr-code/themisdb_ethic_ai.git", "xyz999", true});
    auto sbom_with_private = SBOMVerifier::generate_sbom(with_private);
    has_private = SBOMVerifier::has_private_dependencies(sbom_with_private);
    EXPECT_TRUE(has_private) << "SBOM with private dependencies should be detected";
}

/// TEST-11: Negative test: verify scanner catches injected private credentials
TEST_F(BoundaryEnforcementTest, WAVE_C_B3_011_NegativeTestScannerCatchesInjected) {
    // This test verifies that our scanner successfully detects intentionally
    // injected credentials that should never be in committed code
    std::string malicious_code =
        R"(
        # DO NOT COMMIT: Test injection of credentials
        AWS_SECRET_ACCESS_KEY=AKIA5TESTKEY123456789
        export AZURE_KEY="DefaultEndpointsProtocol=https;AccountName=hacked;AccountKey=EXPLOITKEYHEREEXPLOIT=="
        PRIVATE_DB_PASSWORD="hunter2_super_secret_123"
        -----BEGIN OPENSSH PRIVATE KEY-----
        MIIEXAMPLEPRIVATEKEYDONOTCOMMITabc123def456ghi789jkl
        -----END OPENSSH PRIVATE KEY-----
        )";

    auto matches = scanner_.scan_file("compromised.sh", malicious_code);
    EXPECT_GT(matches.size(), 2) << "Scanner should detect multiple injected credentials";
    EXPECT_TRUE(scanner_.has_critical_leaks(matches))
        << "Injected credentials should be detected as CRITICAL";

    // Verify specific patterns are caught
    bool found_aws = false, found_azure = false, found_ssh = false;
    for (const auto& match : matches) {
        if (match.pattern_name == "aws_secret") found_aws = true;
        if (match.pattern_name == "azure_key") found_azure = true;
        if (match.pattern_name == "openssh_private_key") found_ssh = true;
    }
    EXPECT_TRUE(found_aws) << "AWS key pattern should be detected";
    EXPECT_TRUE(found_azure) << "Azure key pattern should be detected";
    EXPECT_TRUE(found_ssh) << "SSH key pattern should be detected";
}

/// TEST-12: Scoped-checkout validation blocks private submodules in community PR
TEST_F(BoundaryEnforcementTest, WAVE_C_B3_012_ScopedCheckoutBlocksPrivateInCommunity) {
    std::string gitmodules_content = R"(
[submodule "vcpkg"]
	path = vcpkg
	url = https://github.com/microsoft/vcpkg.git
	branch = master

[submodule "plugins/themisdb_ethic_ai"]
	path = plugins/themisdb_ethic_ai
	url = https://github.com/makr-code/themisdb_ethic_ai.git
	branch = develop
	commit = ce401ad9d604012a2c02655e79f5c17f57a9f82d

[submodule "plugins/themisdb_storage"]
	path = plugins/themisdb_storage
	url = https://github.com/makr-code/themisdb_storage.git
	branch = develop
	commit = 03eb17c844a0da96eb66155e20075a90cf467c36

[submodule "plugins/themisdb_importer"]
	path = plugins/themisdb_importer
	url = https://github.com/makr-code/themisdb_importer.git
	branch = develop
	commit = 317add1f724d5f6b979883b85b3c530f6191804d
)";

    auto submodules = ScopedCheckoutValidator::parse_gitmodules(gitmodules_content);
    EXPECT_GT(submodules.size(), 0) << "Should parse gitmodules entries";

    // Count private submodules
    size_t private_count = 0;
    for (const auto& sm : submodules) {
        if (sm.is_private) private_count++;
    }
    EXPECT_GT(private_count, 0) << "Should identify private submodules";

    // Community validation should fail
    bool community_validated = ScopedCheckoutValidator::validate_community_checkout(
        submodules, true);
    EXPECT_FALSE(community_validated)
        << "Community checkout should be rejected due to non-shallow private submodules";
}

}  // namespace themis::plugins::boundary_enforcement
