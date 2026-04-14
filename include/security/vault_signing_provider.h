/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vault_signing_provider.h                           ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:26:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     142                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security/signing_provider.h"
#include <string>
#include <vector>
#include <memory>

namespace themis {

class VaultSigningProvider : public SigningProvider {
public:
    struct Config {
        std::string vault_addr;    // e.g. "http://localhost:8200"
        std::string vault_token;   // Vault token for API calls
        std::string transit_mount; // transit mount path (default: "transit")
        int request_timeout_ms = 5000;
        bool verify_ssl = true;

        Config() : transit_mount("transit") {}
    };

    explicit VaultSigningProvider(const Config& cfg);
    ~VaultSigningProvider() override;

    SigningResult sign(const std::string& key_id, const std::vector<uint8_t>& data) override;

    // =========================================================================
    // KeyProvider interface - SIGNING-ONLY LIMITATION
    // =========================================================================
    // 
    // ⚠️  IMPORTANT: VaultSigningProvider is designed for signing operations
    //     only (using HashiCorp Vault Transit Engine). It does NOT support
    //     key management operations like getKey, rotateKey, listKeys, etc.
    //
    // PURPOSE:
    //   - Sign data using Vault Transit Engine keys
    //   - Verify signatures (if implemented)
    //   - Does NOT extract or manage encryption keys
    //
    // FEATURE FLAG:
    //   Set THEMIS_VAULT_SIGNING_ONLY=1 to explicitly acknowledge this
    //   limitation and suppress key operation errors in non-signing contexts.
    //
    // MIGRATION PATH:
    //   For full key management, use:
    //   - VaultKeyProvider (for Transit Engine key operations)
    //   - HSMProvider with PKCS#11
    //   - Cloud KMS providers (AWS KMS, Azure Key Vault, GCP KMS)
    //
    // See: docs/security/VAULT_SIGNING_PROVIDER.md
    // =========================================================================
    
    std::vector<uint8_t> getKey([[maybe_unused]] const std::string& key_id) override {
        throw KeyOperationException(
            "VaultSigningProvider: getKey() not implemented - signing-only provider. "
            "Use VaultKeyProvider for key management operations. "
            "See: docs/security/VAULT_SIGNING_PROVIDER.md");
    }
    
    std::vector<uint8_t> getKey([[maybe_unused]] const std::string& key_id, uint32_t version) override {
        throw KeyOperationException(
            "VaultSigningProvider: getKey(version) not implemented - signing-only provider. "
            "Use VaultKeyProvider for key management operations. "
            "See: docs/security/VAULT_SIGNING_PROVIDER.md");
    }
    
    uint32_t rotateKey([[maybe_unused]] const std::string& key_id) override {
        throw KeyOperationException(
            "VaultSigningProvider: rotateKey() not implemented - signing-only provider. "
            "Use VaultKeyProvider or Vault CLI for key rotation. "
            "See: docs/security/VAULT_SIGNING_PROVIDER.md");
    }
    
    std::vector<KeyMetadata> listKeys() override {
        throw KeyOperationException(
            "VaultSigningProvider: listKeys() not implemented - signing-only provider. "
            "Use VaultKeyProvider or Vault API for key listing. "
            "See: docs/security/VAULT_SIGNING_PROVIDER.md");
    }
    
    KeyMetadata getKeyMetadata([[maybe_unused]] const std::string& key_id, uint32_t version = 0) override {
        throw KeyOperationException(
            "VaultSigningProvider: getKeyMetadata() not implemented - signing-only provider. "
            "Use VaultKeyProvider for key metadata operations. "
            "See: docs/security/VAULT_SIGNING_PROVIDER.md");
    }
    
    void deleteKey([[maybe_unused]] const std::string& key_id, uint32_t version) override {
        throw KeyOperationException(
            "VaultSigningProvider: deleteKey() not implemented - signing-only provider. "
            "Use VaultKeyProvider or Vault API for key deletion. "
            "See: docs/security/VAULT_SIGNING_PROVIDER.md");
    }
    
    bool hasKey([[maybe_unused]] const std::string& key_id, uint32_t version = 0) override {
        throw KeyOperationException(
            "VaultSigningProvider: hasKey() not implemented - signing-only provider. "
            "Use VaultKeyProvider for key existence checks. "
            "See: docs/security/VAULT_SIGNING_PROVIDER.md");
    }
    
    uint32_t createKeyFromBytes(
        [[maybe_unused]] const std::string& key_id,
        [[maybe_unused]] const std::vector<uint8_t>& key_bytes,
        [[maybe_unused]] const KeyMetadata& metadata = KeyMetadata()) override {
        throw KeyOperationException(
            "VaultSigningProvider: createKeyFromBytes() not implemented - signing-only provider. "
            "Use VaultKeyProvider or Vault API for key creation. "
            "See: docs/security/VAULT_SIGNING_PROVIDER.md");
    }
};

} // namespace themis
