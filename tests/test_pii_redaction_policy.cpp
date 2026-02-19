#include <gtest/gtest.h>
#include "security/pii_redaction_policy.h"
#include <map>
#include <string>

using namespace themis::security;

// ---------------------------------------------------------------------------
// Helper: ensure each test starts with strict_mode = false (the default).
// ---------------------------------------------------------------------------
class PIIRedactionPolicyTest : public ::testing::Test {
protected:
    void SetUp() override {
        PIIRedactionPolicy::get().setStrictMode(false);
    }
    void TearDown() override {
        PIIRedactionPolicy::get().setStrictMode(false);
    }
};

// ---------------------------------------------------------------------------
// redactForLog
// ---------------------------------------------------------------------------

TEST_F(PIIRedactionPolicyTest, RedactEmailInLog) {
    std::string msg = "User login: alice@example.com succeeded";
    std::string redacted = PIIRedactionPolicy::get().redactForLog(msg);

    // Original email must not appear verbatim.
    EXPECT_EQ(redacted.find("alice@example.com"), std::string::npos)
        << "PII email was not redacted in log message";

    // Non-PII parts of the message should survive.
    EXPECT_NE(redacted.find("User login:"), std::string::npos);
    EXPECT_NE(redacted.find("succeeded"), std::string::npos);
}

TEST_F(PIIRedactionPolicyTest, RedactSSNInLog) {
    std::string msg = "Processing record for SSN 123-45-6789";
    std::string redacted = PIIRedactionPolicy::get().redactForLog(msg);

    EXPECT_EQ(redacted.find("123-45-6789"), std::string::npos)
        << "SSN was not redacted";
}

TEST_F(PIIRedactionPolicyTest, RedactCreditCardInLog) {
    // Valid Visa test number (passes Luhn).
    std::string msg = "Payment card: 4242-4242-4242-4242 accepted";
    std::string redacted = PIIRedactionPolicy::get().redactForLog(msg);

    EXPECT_EQ(redacted.find("4242-4242-4242-4242"), std::string::npos)
        << "Credit card was not redacted";
}

TEST_F(PIIRedactionPolicyTest, RedactIBANInLog) {
    std::string msg = "Transfer from IBAN DE89370400440532013000 complete";
    std::string redacted = PIIRedactionPolicy::get().redactForLog(msg);

    EXPECT_EQ(redacted.find("DE89370400440532013000"), std::string::npos)
        << "IBAN was not redacted";
}

TEST_F(PIIRedactionPolicyTest, NoPIIPassesThrough) {
    std::string msg = "Query completed in 42 ms, 7 rows returned";
    std::string redacted = PIIRedactionPolicy::get().redactForLog(msg);

    // Message should be unchanged when no PII is detected.
    EXPECT_EQ(msg, redacted);
}

TEST_F(PIIRedactionPolicyTest, MultipleTypesRedacted) {
    std::string msg = "Contact bob@test.de via 123-45-6789 or card 4242-4242-4242-4242";
    std::string redacted = PIIRedactionPolicy::get().redactForLog(msg);

    EXPECT_EQ(redacted.find("bob@test.de"), std::string::npos)
        << "Email was not redacted";
    EXPECT_EQ(redacted.find("123-45-6789"), std::string::npos)
        << "SSN was not redacted";
    EXPECT_EQ(redacted.find("4242-4242-4242-4242"), std::string::npos)
        << "Credit card was not redacted";
}

// ---------------------------------------------------------------------------
// redactAttributes  (trace span attributes)
// ---------------------------------------------------------------------------

TEST_F(PIIRedactionPolicyTest, RedactEmailAttribute) {
    std::map<std::string, std::string> attrs = {
        {"user.email", "alice@example.com"},
        {"http.method", "POST"}
    };
    auto safe = PIIRedactionPolicy::get().redactAttributes(attrs);

    // The email field value must be masked.
    EXPECT_EQ(safe.at("user.email").find("alice@example.com"), std::string::npos)
        << "Email attribute was not redacted";

    // Non-PII attribute must survive unchanged.
    EXPECT_EQ(safe.at("http.method"), "POST");
}

TEST_F(PIIRedactionPolicyTest, RedactAttributeByFieldName) {
    // Key "email" is a PII field hint – the entire value should be masked.
    std::map<std::string, std::string> attrs = {
        {"email", "charlie@corp.org"}
    };
    auto safe = PIIRedactionPolicy::get().redactAttributes(attrs);

    EXPECT_EQ(safe.at("email").find("charlie@corp.org"), std::string::npos)
        << "PII field-name key did not trigger value redaction";
}

TEST_F(PIIRedactionPolicyTest, RedactAttributeInlineSSN) {
    // Value contains PII embedded in a longer string.
    std::map<std::string, std::string> attrs = {
        {"debug.note", "SSN found: 123-45-6789 in record"}
    };
    auto safe = PIIRedactionPolicy::get().redactAttributes(attrs);

    EXPECT_EQ(safe.at("debug.note").find("123-45-6789"), std::string::npos)
        << "Inline SSN in attribute value was not redacted";
}

// ---------------------------------------------------------------------------
// redactLabels  (Prometheus metric labels)
// ---------------------------------------------------------------------------

TEST_F(PIIRedactionPolicyTest, RedactLabels_EmailLabel) {
    std::map<std::string, std::string> labels = {
        {"user_email", "dave@example.com"},
        {"region", "eu-central-1"}
    };
    auto safe = PIIRedactionPolicy::get().redactLabels(labels);

    EXPECT_EQ(safe.at("user_email").find("dave@example.com"), std::string::npos)
        << "Email in metric label was not redacted";

    EXPECT_EQ(safe.at("region"), "eu-central-1");
}

TEST_F(PIIRedactionPolicyTest, RedactLabels_NoPII) {
    std::map<std::string, std::string> labels = {
        {"operation", "tsstore_write"},
        {"status", "ok"}
    };
    auto safe = PIIRedactionPolicy::get().redactLabels(labels);

    EXPECT_EQ(safe.at("operation"), "tsstore_write");
    EXPECT_EQ(safe.at("status"), "ok");
}

// ---------------------------------------------------------------------------
// Strict-mode enforcement
// ---------------------------------------------------------------------------

TEST_F(PIIRedactionPolicyTest, StrictModeFullReplace) {
    PIIRedactionPolicy::get().setStrictMode(true);

    // In partial mode an email like "alice@example.com" becomes "a***@example.com".
    // In strict mode the entire value must be fully replaced (no original chars).
    std::string msg = "User: alice@example.com";
    std::string redacted = PIIRedactionPolicy::get().redactForLog(msg);

    EXPECT_EQ(redacted.find("alice@example.com"), std::string::npos);
    // Even the local part should not appear in strict mode.
    EXPECT_EQ(redacted.find("alice"), std::string::npos)
        << "Strict mode should fully replace – no original chars should remain";
}

// ---------------------------------------------------------------------------
// Reload (smoke-test – YAML not present, should retain embedded defaults)
// ---------------------------------------------------------------------------

TEST_F(PIIRedactionPolicyTest, ReloadRetainsDefaults) {
    // Reloading with a non-existent path should fail gracefully and continue
    // to function using the previously loaded configuration.
    bool ok = PIIRedactionPolicy::get().reload("/nonexistent/pii_patterns.yaml");
    // Return value is false on failure – that is acceptable.
    (void)ok;

    // Detection should still work after a failed reload.
    std::string msg = "Contact alice@example.com";
    std::string redacted = PIIRedactionPolicy::get().redactForLog(msg);
    EXPECT_EQ(redacted.find("alice@example.com"), std::string::npos)
        << "Redaction stopped working after a failed reload";
}
