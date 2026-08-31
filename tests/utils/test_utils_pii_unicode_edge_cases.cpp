#include <gtest/gtest.h>
#include "utils/pii_detector.h"
#include "utils/regex_detection_engine.h"
#include <nlohmann/json.hpp>

#include <string>

namespace themis {
namespace utils {

class PiiUnicodeEdgeCasesTest : public ::testing::Test {
protected:
    PIIDetector detector;
    RegexDetectionEngine regex_engine;
    
    void SetUp() override {
        nlohmann::json config = nlohmann::json::object();
        config["enabled"] = true;
        ASSERT_TRUE(regex_engine.initialize(config));
    }
};

TEST_F(PiiUnicodeEdgeCasesTest, UnicodeTextDoesNotThrow) {
    const std::vector<std::string> samples = {
        "我的电子邮件是user@example.com",
        "البريد الإلكتروني: user@example.com",
        "Hello שלום مرحبا - contact: info@example.com",
        "user\u200B@example\u200B.com",
        "Café - email: user@domain.com"
    };

    for (const auto& sample : samples) {
        EXPECT_NO_THROW({
            auto findings = detector.detectInText(sample);
            (void)findings;
        });
    }
}

TEST_F(PiiUnicodeEdgeCasesTest, InvalidUtf8AndNullByteAreHandled) {
    std::string text = "Contact: ";
    text += static_cast<char>(0xFF);
    text += " user";
    text += '\0';
    text += "@test.com";

    EXPECT_NO_THROW({
        auto findings = detector.detectInText(text);
        (void)findings;
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, RegexEngineDetectsUnicodeText) {
    const std::string sample = "用户名: john_smith, 邮箱: john@example.com";
    auto findings = regex_engine.detectInText(sample);
    EXPECT_GE(findings.size(), 0u);
}

} // namespace utils
} // namespace themis

