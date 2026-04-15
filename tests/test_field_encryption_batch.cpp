/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_field_encryption_batch.cpp                    ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:18:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     53                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "security/encryption.h"
#include "security/mock_key_provider.h"

using namespace themis;

TEST(FieldEncryptionBatch, RoundtripEncryptDecrypt) {
    auto provider = std::make_shared<MockKeyProvider>();
    // Create a deterministic test key (create with bytes) to avoid randomness in CI
    std::vector<uint8_t> key_bytes(32, 0x42);
    provider->createKeyFromBytes("user_pii", key_bytes);

    FieldEncryption enc(provider);

    std::vector<std::pair<std::string,std::string>> items = {
        {"salt-1", "hello world"},
        {"salt-2", "The quick brown fox"},
        {"salt-3", "Lorem ipsum"}
    };

    auto out = enc.encryptEntityBatch(items, "user_pii");
    std::cerr << "debug: out.size=" << out.size() << " items.size=" << items.size() << std::endl;
    ASSERT_EQ(out.size(), items.size());

    for (size_t i = 0; i < out.size(); ++i) {
        // Debug: print encrypted blob JSON to inspect IV/tag/ciphertext
        std::cerr << out[i].toJson().dump() << std::endl;
        auto decrypted = enc.decryptToString(out[i]);
        EXPECT_EQ(decrypted, items[i].second);
    }
}
