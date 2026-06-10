/*
 * ThemisDB | File: test_vault_key_provider_retry.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 97/100
 * Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <gtest/gtest.h>
#include "security/vault_key_provider.h"
#include "security/key_provider.h"

using namespace themis;

TEST(VaultKeyProviderRetry, RetriesAndSucceeds) {
    VaultKeyProvider::Config cfg;
    cfg.vault_addr = "http://localhost:8200"; // not actually used by override
    cfg.vault_token = "s.test";
    cfg.transit_max_retries = 3;
    cfg.transit_backoff_ms = 10;

    VaultKeyProvider vp(cfg);

    int calls = 0;
    // Prepare base64 signature payload: vault:v1:BASE64
    const std::string raw_sig = "deadbeef";
    // simple base64 encode of raw_sig (manual)
    const std::string b64 = "ZGVhZGJlZWY="; // base64("deadbeef") without trailing newline
    // Build Vault response
    std::string vault_resp = std::string("{\"data\":{\"signature\":\"vault:v1:") + b64 + "\"}}";

    vp.setTestRequestOverride([&](const std::string& url, const std::string& method, const std::string& body)->std::string {
        (void)url; (void)method; (void)body;
        ++calls;
        if (calls < 3) {
            // Simulate transient vault server error by throwing KeyOperationException with transient=true
            throw KeyOperationException("simulated transient", 500, "simulated", true);
        }
        return vault_resp;
    });

    std::vector<uint8_t> data = {'h','i'};
    SigningResult res = vp.sign("mykey", data);

    // Expect that we tried at least 3 times (two failures + one success)
    EXPECT_GE(calls, 3);
    EXPECT_EQ(res.algorithm, "VAULT+TRANSIT");
    EXPECT_FALSE(res.signature.empty());
}
