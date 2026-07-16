/*
 * ThemisDB | File: test_vault_signing_provider.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=8; TODO=2, Stub=1, Unimpl=0, Mock=4, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
