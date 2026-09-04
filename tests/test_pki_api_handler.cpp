#include <gtest/gtest.h>
#include "server/pki_api_handler.h"
#include "security/signing.h"
#include <openssl/sha.h>

using namespace themis;

// Simple mock SigningService for tests
class MockSigningService : public SigningService {
public:
    SigningResult sign(const std::vector<uint8_t>& data, const std::string& /*key_id*/) override {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(data.data(), data.size(), hash);
        SigningResult r;
        r.signature.assign(hash, hash + SHA256_DIGEST_LENGTH);
        r.algorithm = "MOCK+SHA256";
        return r;
    }

    bool verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature, const std::string& /*key_id*/) override {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(data.data(), data.size(), hash);
        return signature == std::vector<uint8_t>(hash, hash + SHA256_DIGEST_LENGTH);
    }
};

static std::string b64_encode(const std::vector<uint8_t>& data) {
    static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out = {};
    int val=0, valb=-6;
    for (uint8_t c : data) {
        val = (val<<8) + c;
        valb += 8;
        while (valb>=0) {
            out.push_back(chars[(val>>valb)&0x3F]);
            valb -= 6;
        }
    }
    if (valb>-6) {
      out.push_back(chars[((val<<8)>>(valb+8))&0x3F]);
    }
    while (out.size()%4) {
      out.push_back('=');
    }
    return out;
}

TEST(PkiApiHandlerTest, SignAndVerify) {
    auto svc = std::make_shared<MockSigningService>();
    themis::server::PkiApiHandler handler(svc);

    std::vector<uint8_t> data = {'h','e','l','l','o'};
    std::string data_b64 = b64_encode(data);

    nlohmann::json req = { {"data_b64", data_b64} };
    auto res = handler.sign("test-key", req);

    ASSERT_TRUE(res.contains("signature_b64"));
    ASSERT_TRUE(res.contains("algorithm"));
    EXPECT_EQ(res["algorithm"].get<std::string>(), "MOCK+SHA256");

    std::string sig_b64 = res["signature_b64"].get<std::string>();
    auto verify_req = nlohmann::json{{"data_b64", data_b64}, {"signature_b64", sig_b64}};
    auto vres = handler.verify("test-key", verify_req);
    ASSERT_TRUE(vres.contains("valid"));
    EXPECT_TRUE(vres["valid"].get<bool>());

    // Negative case: altered data
    std::vector<uint8_t> data2 = {'b','a','d'};
    std::string data2_b64 = b64_encode(data2);
    auto vres2 = handler.verify("test-key", nlohmann::json{{"data_b64", data2_b64}, {"signature_b64", sig_b64}});
    ASSERT_TRUE(vres2.contains("valid"));
    EXPECT_FALSE(vres2["valid"].get<bool>());
}

TEST(PkiApiHandlerTest, SignRejectsInvalidKeyId) {
    auto svc = std::make_shared<MockSigningService>();
    themis::server::PkiApiHandler handler(svc);

    std::vector<uint8_t> data = {'h','e','l','l','o'};
    std::string data_b64 = b64_encode(data);

    auto res = handler.sign("../bad-key", nlohmann::json{{"data_b64", data_b64}});
    ASSERT_TRUE(res.contains("status_code"));
    EXPECT_EQ(res["status_code"].get<int>(), 400);
}

TEST(PkiApiHandlerTest, VerifyRejectsInvalidBase64Payload) {
    auto svc = std::make_shared<MockSigningService>();
    themis::server::PkiApiHandler handler(svc);

    auto res = handler.verify(
        "test-key",
        nlohmann::json{{"data_b64", "%%%not-base64%%%"}, {"signature_b64", "Zm9v"}}
    );

    ASSERT_TRUE(res.contains("status_code"));
    EXPECT_EQ(res["status_code"].get<int>(), 400);
}
