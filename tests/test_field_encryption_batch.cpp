/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_field_encryption_batch.cpp                    ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:35:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
