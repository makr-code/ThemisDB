#include <gtest/gtest.h>

#include "security/vault_key_provider.h"
#include "security/vcc_pki_client.h"

#include <stdexcept>

TEST(VaultTransportSecurityHardeningTest, RejectsRemoteHttpEndpoint) {
    themis::VaultKeyProvider::Config cfg;
    cfg.vault_addr = "http://vault.example.com:8200";
    cfg.vault_token = "s.test";

    EXPECT_THROW({ themis::VaultKeyProvider provider(cfg); }, themis::KeyOperationException);
}

TEST(VaultTransportSecurityHardeningTest, RejectsDisabledTlsVerificationForRemoteEndpoint) {
    themis::VaultKeyProvider::Config cfg;
    cfg.vault_addr = "https://vault.example.com:8200";
    cfg.vault_token = "s.test";
    cfg.verify_ssl = false;

    EXPECT_THROW({ themis::VaultKeyProvider provider(cfg); }, themis::KeyOperationException);
}

TEST(VaultTransportSecurityHardeningTest, AllowsLoopbackHttpForLocalDevelopment) {
    themis::VaultKeyProvider::Config cfg;
    cfg.vault_addr = "http://127.0.0.1:8200";
    cfg.vault_token = "s.test";
    cfg.verify_ssl = false;

    EXPECT_NO_THROW({ themis::VaultKeyProvider provider(cfg); });
}

TEST(VaultTransportSecurityHardeningTest, RejectsNonPositiveTimeout) {
    themis::VaultKeyProvider::Config cfg;
    cfg.vault_addr = "https://vault.example.com:8200";
    cfg.vault_token = "s.test";
    cfg.request_timeout_ms = 0;

    EXPECT_THROW({ themis::VaultKeyProvider provider(cfg); }, themis::KeyOperationException);
}

TEST(VccTransportSecurityHardeningTest, RejectsRemoteHttpEndpoint) {
    themis::TLSConfig tls_config;
    EXPECT_THROW({ themis::VCCPKIClient client("http://pki.example.com", tls_config, 5000); },
                 std::invalid_argument);
}

TEST(VccTransportSecurityHardeningTest, RejectsDisabledTlsVerificationForRemoteEndpoint) {
    themis::TLSConfig tls_config;
    tls_config.verify_server = false;
    EXPECT_THROW({ themis::VCCPKIClient client("https://pki.example.com", tls_config, 5000); },
                 std::invalid_argument);
}

TEST(VccTransportSecurityHardeningTest, AllowsLoopbackHttpForLocalDevelopment) {
    themis::TLSConfig tls_config;
    tls_config.verify_server = false;
    EXPECT_NO_THROW({ themis::VCCPKIClient client("http://127.0.0.1:9443", tls_config, 5000); });
}

TEST(VccTransportSecurityHardeningTest, RejectsNonPositiveTimeout) {
    themis::TLSConfig tls_config;
    EXPECT_THROW({ themis::VCCPKIClient client("https://pki.example.com", tls_config, 0); },
                 std::invalid_argument);
}
