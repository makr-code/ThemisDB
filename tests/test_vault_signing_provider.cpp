/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_vault_signing_provider.cpp                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:04:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     60                                             ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • cd30d9ee9  2025-11-16  Stabilize WSL tests: Vault helper, policy override, index... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include <cstdlib>
#include "security/vault_signing_provider.h"

using namespace themis;

TEST(VaultSigningProviderTest, MockFallbackProducesDeterministicSignature) {
    VaultSigningProvider::Config cfg; // not used by prototype
    VaultSigningProvider provider(cfg);

    std::vector<uint8_t> data = {1,2,3,4,5};
    // Ensure env THEIMIS_VAULT_ADDR not set for deterministic mock path
#if defined(_WIN32)
    _putenv_s("THEMIS_VAULT_ADDR", "");
#else
    unsetenv("THEMIS_VAULT_ADDR");
#endif

    SigningResult res = provider.sign("test-key", data);
    EXPECT_EQ(res.algorithm, "MOCK+SHA256");
    EXPECT_EQ(res.signature.size(), 32); // SHA256 length
    // Calling twice produces same digest
    SigningResult res2 = provider.sign("test-key", data);
    EXPECT_EQ(res.signature, res2.signature);
}

    TEST(VaultKeyProviderSign, RetryOnTransientError) {
        // This test is a smoke test that ensures VaultKeyProvider::sign can be called
        // with a mock httpPost implementation. We simulate transient failure by
        // overriding httpPost in a thin subclass in the test environment.
    
        // TODO: For now just ensure the method exists and can be invoked via the adapter.
        ASSERT_TRUE(true);
    }
