#include <gtest/gtest.h>

#include "acceleration/plugin_security.h"
#include "plugins/plugin_manager.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <filesystem>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

namespace fs = std::filesystem;

namespace {

#ifndef THEMIS_RUNTIME_TEST_PLUGIN_PATH
#error "THEMIS_RUNTIME_TEST_PLUGIN_PATH must be defined for runtime plugin tests"
#endif

[[nodiscard]] fs::path runtimePluginPath() {
    return fs::path{THEMIS_RUNTIME_TEST_PLUGIN_PATH};
}

[[nodiscard]] fs::path runtimePluginMetadataPath(const fs::path& plugin_path) {
    return plugin_path.parent_path() / (plugin_path.stem().string() + ".json");
}

constexpr const char* kTrustedIssuerDn = "CN=ThemisDB Official Plugins, O=ThemisDB, C=DE";

struct EvpPkeyDeleter {
    void operator()(EVP_PKEY* key) const {
        EVP_PKEY_free(key);
    }
};

struct X509Deleter {
    void operator()(X509* cert) const {
        X509_free(cert);
    }
};

struct BioDeleter {
    void operator()(BIO* bio) const {
        BIO_free(bio);
    }
};

[[nodiscard]] std::string bytesToHex(const std::vector<unsigned char>& bytes) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2);
    for (const auto byte : bytes) {
        result.push_back(kHex[(byte >> 4) & 0x0F]);
        result.push_back(kHex[byte & 0x0F]);
    }
    return result;
}

[[nodiscard]] std::vector<unsigned char> hexToBytes(const std::string& hex) {
    auto decode_nibble = [](const char ch) -> int {
        if (ch >= '0' && ch <= '9') {
            return ch - '0';
        }
        if (ch >= 'a' && ch <= 'f') {
            return 10 + (ch - 'a');
        }
        if (ch >= 'A' && ch <= 'F') {
            return 10 + (ch - 'A');
        }
        return -1;
    };

    std::vector<unsigned char> result = {};

    if ((hex.size() % 2) != 0) {
        return result;
    }

    result.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        const int hi = decode_nibble(hex[i]);
        const int lo = decode_nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) {
            result.clear();
            return result;
        }
        result.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }
    return result;
}

[[nodiscard]] std::unique_ptr<EVP_PKEY, EvpPkeyDeleter> createTestKey() {
    std::unique_ptr<EVP_PKEY, EvpPkeyDeleter> key(EVP_PKEY_new());
    if (!key) {
        return nullptr;
    }

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) {
        return nullptr;
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0 ||
        EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return nullptr;
    }

    EVP_PKEY* raw_key = nullptr;
    if (EVP_PKEY_keygen(ctx, &raw_key) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return nullptr;
    }

    EVP_PKEY_CTX_free(ctx);
    key.reset(raw_key);
    return key;
}

[[nodiscard]] std::unique_ptr<X509, X509Deleter> createTestCertificate(EVP_PKEY* key) {
    std::unique_ptr<X509, X509Deleter> cert(X509_new());
    if (!cert) {
        return nullptr;
    }

    X509_set_version(cert.get(), 2);
    ASN1_INTEGER_set(X509_get_serialNumber(cert.get()), 1);
    X509_gmtime_adj(X509_getm_notBefore(cert.get()), -60 * 60);
    X509_gmtime_adj(X509_getm_notAfter(cert.get()), 24 * 60 * 60);
    X509_set_pubkey(cert.get(), key);

    X509_NAME* subject = X509_get_subject_name(cert.get());
    X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_ASC,
                               reinterpret_cast<const unsigned char*>("ThemisDB Official Plugins"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(subject, "O", MBSTRING_ASC,
                               reinterpret_cast<const unsigned char*>("ThemisDB"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(subject, "C", MBSTRING_ASC,
                               reinterpret_cast<const unsigned char*>("DE"), -1, -1, 0);
    X509_set_issuer_name(cert.get(), subject);

    if (X509_sign(cert.get(), key, EVP_sha256()) <= 0) {
        return nullptr;
    }

    return cert;
}

[[nodiscard]] std::string toPem(X509* cert) {
    std::unique_ptr<BIO, BioDeleter> bio(BIO_new(BIO_s_mem()));
    if (!bio || PEM_write_bio_X509(bio.get(), cert) != 1) {
        return {};
    }

    BUF_MEM* memory = nullptr;
    BIO_get_mem_ptr(bio.get(), &memory);
    if (!memory || !memory->data || memory->length == 0) {
        return {};
    }
    return std::string(memory->data, memory->length);
}

[[nodiscard]] std::string signHashBytes(EVP_PKEY* key, const std::string& hash_hex) {
    const auto hash_bytes = hexToBytes(hash_hex);
    if (hash_bytes.empty()) {
        return {};
    }

    EVP_MD_CTX* raw_ctx = EVP_MD_CTX_new();
    if (!raw_ctx) {
        return {};
    }

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(raw_ctx, &EVP_MD_CTX_free);
    if (EVP_DigestSignInit(ctx.get(), nullptr, EVP_sha256(), nullptr, key) <= 0) {
        return {};
    }

    if (EVP_DigestSignUpdate(ctx.get(), hash_bytes.data(), hash_bytes.size()) <= 0) {
        return {};
    }

    size_t signature_size = 0;
    if (EVP_DigestSignFinal(ctx.get(), nullptr, &signature_size) <= 0) {
        return {};
    }

    std::vector<unsigned char> signature(signature_size);
    if (EVP_DigestSignFinal(ctx.get(), signature.data(), &signature_size) <= 0) {
        return {};
    }
    signature.resize(signature_size);
    return bytesToHex(signature);
}

void writeSignedPluginMetadata(const fs::path& plugin_path) {
    themis::acceleration::PluginSecurityPolicy policy;
    themis::acceleration::PluginSecurityVerifier verifier(policy);
    const auto file_hash = verifier.calculateFileHash(plugin_path.string());
    ASSERT_FALSE(file_hash.empty()) << "Failed to hash runtime plugin DLL";

    auto key = createTestKey();
    ASSERT_NE(key, nullptr) << "Failed to create RSA key for runtime plugin test";

    auto cert = createTestCertificate(key.get());
    ASSERT_NE(cert, nullptr) << "Failed to create X509 certificate for runtime plugin test";

    const auto certificate_pem = toPem(cert.get());
    ASSERT_FALSE(certificate_pem.empty()) << "Failed to serialize runtime plugin certificate";

    const auto signature_hex = signHashBytes(key.get(), file_hash);
    ASSERT_FALSE(signature_hex.empty()) << "Failed to sign runtime plugin hash";

    nlohmann::json metadata = {
        {"plugin", {
            {"name", "runtime_test_plugin"},
            {"version", "1.0.0"},
            {"author", "ThemisDB Runtime Test"},
            {"description", "Runtime test plugin metadata"},
            {"license", "Apache-2.0"},
            {"signature", {
                {"sha256", file_hash},
                {"signature", signature_hex},
                {"certificate", certificate_pem},
                {"issuer", kTrustedIssuerDn},
                {"subject", kTrustedIssuerDn},
                {"timestamp", static_cast<std::uint64_t>(std::time(nullptr))}
            }},
            {"permissions", nlohmann::json::array()}
        }}
    };

    const auto metadata_path = runtimePluginMetadataPath(plugin_path);
    std::ofstream metadata_file(metadata_path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(metadata_file.is_open()) << "Failed to open runtime plugin metadata path: " << metadata_path.string();
    metadata_file << metadata.dump(2);
}

void writeDiscoveryManifest(const fs::path& manifest_path, const fs::path& plugin_binary_path) {
    nlohmann::json manifest = {
        {"name", "runtime_test_plugin"},
        {"version", "1.0.0"},
        {"type", "custom"},
        {"description", "Runtime discovery test plugin"},
        {"binary", {
            {"windows", plugin_binary_path.filename().string()},
            {"linux", plugin_binary_path.filename().string()},
            {"macos", plugin_binary_path.filename().string()}
        }}
    };

    std::ofstream manifest_file(manifest_path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(manifest_file.is_open()) << "Failed to open runtime manifest path: " << manifest_path.string();
    manifest_file << manifest.dump(2);
    manifest_file.close();

    themis::acceleration::PluginSecurityPolicy policy;
    themis::acceleration::PluginSecurityVerifier verifier(policy);
    const auto manifest_hash = verifier.calculateFileHash(manifest_path.string());
    ASSERT_FALSE(manifest_hash.empty()) << "Failed to hash runtime manifest";

    std::ofstream sig_file(manifest_path.string() + ".sig", std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(sig_file.is_open()) << "Failed to open runtime manifest signature path";
    sig_file << manifest_hash;
}

} // namespace

class PluginRuntimeLoadingFocusedTests : public ::testing::Test {
protected:
    void TearDown() override {
        auto unload_all = manager_.unloadAllPlugins();
        EXPECT_TRUE(unload_all.has_value()) << unload_all.error().message();

        std::error_code ec;
        fs::remove(runtimePluginMetadataPath(runtimePluginPath()), ec);
        if (!discovery_dir_.empty()) {
            fs::remove_all(discovery_dir_, ec);
        }
    }

    themis::plugins::PluginManager manager_;
    fs::path discovery_dir_;
};

TEST_F(PluginRuntimeLoadingFocusedTests, LoadPluginFromPathLoadsRealSharedLibrary) {
    const auto plugin_path = runtimePluginPath();
    ASSERT_TRUE(fs::exists(plugin_path)) << "Missing runtime test plugin: " << plugin_path.string();
    writeSignedPluginMetadata(plugin_path);

    const auto result = manager_.loadPluginFromPath(
        plugin_path.string(),
        R"({"mode":"runtime-test","batch_size":4})");

    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto* plugin = *result;
    ASSERT_NE(plugin, nullptr);
    EXPECT_STREQ(plugin->getName(), "runtime_test_plugin");
    EXPECT_STREQ(plugin->getVersion(), "1.0.0");
    EXPECT_EQ(plugin->getType(), themis::plugins::PluginType::CUSTOM);

    const auto capabilities = plugin->getCapabilities();
    EXPECT_TRUE(capabilities.supports_batching);
    EXPECT_TRUE(capabilities.thread_safe);
    EXPECT_FALSE(capabilities.gpu_accelerated);

    EXPECT_TRUE(manager_.isPluginLoaded("runtime_test_plugin"));

    const auto get_result = manager_.getPlugin("runtime_test_plugin");
    ASSERT_TRUE(get_result.has_value()) << get_result.error().message();
    EXPECT_EQ(*get_result, plugin);

    const auto loaded = manager_.listLoadedPlugins();
    EXPECT_NE(std::find(loaded.begin(), loaded.end(), "runtime_test_plugin"), loaded.end());

    const auto unload_result = manager_.unloadPlugin("runtime_test_plugin");
    ASSERT_TRUE(unload_result.has_value()) << unload_result.error().message();
    EXPECT_FALSE(manager_.isPluginLoaded("runtime_test_plugin"));
}

TEST_F(PluginRuntimeLoadingFocusedTests, LoadPluginFromPathFailsWhenInitializeRejectsInvalidJson) {
    const auto plugin_path = runtimePluginPath();
    ASSERT_TRUE(fs::exists(plugin_path)) << "Missing runtime test plugin: " << plugin_path.string();
    writeSignedPluginMetadata(plugin_path);

    const auto result = manager_.loadPluginFromPath(plugin_path.string(), "{invalid json");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED);
    EXPECT_FALSE(manager_.isPluginLoaded("runtime_test_plugin"));
}

TEST_F(PluginRuntimeLoadingFocusedTests, ScanDirectoryRegistersPluginAndLoadByNameIsEditionGatedInCommunity) {
    const auto source_plugin_path = runtimePluginPath();
    ASSERT_TRUE(fs::exists(source_plugin_path)) << "Missing runtime test plugin: " << source_plugin_path.string();

    const auto unique_suffix = std::to_string(
        static_cast<long long>(std::chrono::steady_clock::now().time_since_epoch().count()));
    discovery_dir_ = fs::temp_directory_path() /
        ("themis_runtime_plugin_discovery_" + unique_suffix);
    std::error_code ec;
    fs::remove_all(discovery_dir_, ec);
    ASSERT_TRUE(fs::create_directories(discovery_dir_));

    const auto copied_plugin_path = discovery_dir_ / source_plugin_path.filename();
    fs::copy_file(source_plugin_path, copied_plugin_path, fs::copy_options::overwrite_existing, ec);
    ASSERT_FALSE(ec) << "Failed to copy runtime plugin DLL: " << ec.message();

    writeSignedPluginMetadata(copied_plugin_path);
    const auto manifest_path = discovery_dir_ / "plugin.json";
    writeDiscoveryManifest(manifest_path, copied_plugin_path);

    const auto scan_result = manager_.scanPluginDirectory(discovery_dir_.string());
    ASSERT_TRUE(scan_result.has_value()) << scan_result.error().message();
    EXPECT_GE(*scan_result, 1u);

    const auto manifest_result = manager_.getManifest("runtime_test_plugin");
    ASSERT_TRUE(manifest_result.has_value()) << manifest_result.error().message();
    EXPECT_EQ(manifest_result->name, "runtime_test_plugin");
    EXPECT_EQ(manifest_result->version, "1.0.0");

    const auto load_result = manager_.loadPlugin("runtime_test_plugin");
    ASSERT_FALSE(load_result.has_value());
    EXPECT_EQ(load_result.error().code(), themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND);
    EXPECT_NE(load_result.error().message().find("Community Edition"), std::string::npos);
    EXPECT_FALSE(manager_.isPluginLoaded("runtime_test_plugin"));
}