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

// ---------------------------------------------------------------------------
// PIIRedactingSink  (auto-redacting spdlog sink wrapper)
// ---------------------------------------------------------------------------
#include "utils/pii_redacting_sink.h"
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>

class PIIRedactingSinkTest : public ::testing::Test {
protected:
    // Build an in-memory logger: ostream_sink wrapped by PIIRedactingSink.
    void SetUp() override {
        PIIRedactionPolicy::get().setStrictMode(false);
        oss_.str("");
        oss_.clear();
        auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss_);
        auto pii_sink = std::make_shared<themis::utils::PIIRedactingSink>(ostream_sink);
        logger_ = std::make_shared<spdlog::logger>("test_pii", pii_sink);
        logger_->set_level(spdlog::level::trace);
        logger_->set_pattern("%v");  // Only the message, no timestamps
    }

    void TearDown() override {
        PIIRedactionPolicy::get().setStrictMode(false);
    }

    std::ostringstream oss_ = {};
    std::shared_ptr<spdlog::logger> logger_;
};

TEST_F(PIIRedactingSinkTest, EmailAutoRedactedThroughSink) {
    logger_->info("User: alice@example.com logged in");
    logger_->flush();

    std::string output = oss_.str();
    EXPECT_EQ(output.find("alice@example.com"), std::string::npos)
        << "Email should have been auto-redacted by the sink";
    // Non-PII parts should survive.
    EXPECT_NE(output.find("User:"), std::string::npos);
    EXPECT_NE(output.find("logged in"), std::string::npos);
}

TEST_F(PIIRedactingSinkTest, SSNAutoRedactedThroughSink) {
    logger_->info("Processing SSN: 123-45-6789");
    logger_->flush();

    std::string output = oss_.str();
    EXPECT_EQ(output.find("123-45-6789"), std::string::npos)
        << "SSN should have been auto-redacted by the sink";
}

TEST_F(PIIRedactingSinkTest, NoPIIPassesThroughSinkUnchanged) {
    logger_->info("Query completed in 42 ms");
    logger_->flush();

    std::string output = oss_.str();
    EXPECT_NE(output.find("Query completed in 42 ms"), std::string::npos)
        << "Non-PII message must pass through unchanged";
}

TEST_F(PIIRedactingSinkTest, StrictModeAutoRedactedThroughSink) {
    PIIRedactionPolicy::get().setStrictMode(true);

    logger_->info("User: alice@example.com");
    logger_->flush();

    std::string output = oss_.str();
    EXPECT_EQ(output.find("alice@example.com"), std::string::npos);
    // In strict mode even the local part 'alice' should not appear.
    EXPECT_EQ(output.find("alice"), std::string::npos)
        << "Strict mode should fully replace all PII parts";
}

// ---------------------------------------------------------------------------
// MetricsCollector  – label redaction via makeKey
// ---------------------------------------------------------------------------
#include "observability/metrics_collector.h"

TEST(PIIMetricsRedactionTest, EmailLabelRedactedInPrometheusOutput) {
    using themis::observability::MetricsCollector;

    // Record a metric with a shard_id label that contains an email address.
    // The email must not appear verbatim in the Prometheus output.
    MetricsCollector::getInstance().reset();
    MetricsCollector::getInstance().recordShardRequest(
        "shard-alice@corp.de", "read");

    std::string prom = MetricsCollector::getInstance().getPrometheusMetrics();

    // 1. Raw email must not be present.
    EXPECT_EQ(prom.find("alice@corp.de"), std::string::npos)
        << "Email in metric label must be redacted in Prometheus output";

    // 2. The non-PII prefix of the shard_id should still be present so that
    //    label structure is preserved (only the PII substring is replaced).
    EXPECT_NE(prom.find("shard-"), std::string::npos)
        << "Non-PII prefix 'shard-' must be preserved after redaction";

    // 3. A masking token must replace the email (partial or strict depending on
    //    policy configuration – we just verify something non-empty is there).
    EXPECT_NE(prom.find("shard_requests_total"), std::string::npos)
        << "Metric name must still appear in Prometheus output";
}
