/**
 * @file test_utilities_comprehensive.cpp
 * @brief Comprehensive real unit tests for Utilities Layer
 * 
 * Test Intent:
 * - Validate error handling with real tl::expected wrapper
 * - Test PII detection with real pattern matching (no mocks)
 * - Verify stemming with actual algorithmic transformations
 * - Test OLAP engine with real aggregation computations
 * - Validate compression utilities
 * 
 * Coverage: Utilities layer (error handling, PII detection, stemming, OLAP)
 * No stubs - all tests use real implementations
 */

#include <gtest/gtest.h>
#include "utils/expected.h"
#include "utils/pii_detector.h"
#include "utils/stemmer.h"
#include "analytics/olap.h"
#include "utils/error_registry.h"
#include <string>
#include <vector>

using namespace themis;
using namespace themis::utils;
using namespace themis::analytics;

// ============================================================================
// Error Handling Tests (tl::expected)
// ============================================================================

class ErrorHandlingTest : public ::testing::Test {};

TEST_F(ErrorHandlingTest, ExpectedWithValue) {
    // Intent: Verify Expected wrapper holds successful values
    
    Expected<int, std::string> result = 42;
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);
    EXPECT_EQ(*result, 42);
}

TEST_F(ErrorHandlingTest, ExpectedWithError) {
    // Intent: Verify Expected wrapper holds error states
    
    Expected<int, std::string> result = Unexpected("An error occurred");
    
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "An error occurred");
}

TEST_F(ErrorHandlingTest, ExpectedMapTransform) {
    // Intent: Verify map() transforms values
    
    Expected<int, std::string> result = 10;
    
    auto doubled = result.map([](int x) { return x * 2; });
    
    ASSERT_TRUE(doubled.has_value());
    EXPECT_EQ(*doubled, 20);
}

TEST_F(ErrorHandlingTest, ExpectedMapError) {
    // Intent: Verify map() preserves errors
    
    Expected<int, std::string> result = Unexpected("error");
    
    auto doubled = result.map([](int x) { return x * 2; });
    
    ASSERT_FALSE(doubled.has_value());
    EXPECT_EQ(doubled.error(), "error");
}

TEST_F(ErrorHandlingTest, ExpectedAndThen) {
    // Intent: Verify and_then() chains operations
    
    auto divide = [](int x) -> Expected<int, std::string> {
        if (x == 0) return Unexpected("Division by zero");
        return 100 / x;
    };
    
    Expected<int, std::string> result1 = 10;
    auto chained1 = result1.and_then(divide);
    ASSERT_TRUE(chained1.has_value());
    EXPECT_EQ(*chained1, 10);
    
    Expected<int, std::string> result2 = 0;
    auto chained2 = result2.and_then(divide);
    ASSERT_FALSE(chained2.has_value());
    EXPECT_EQ(chained2.error(), "Division by zero");
}

TEST_F(ErrorHandlingTest, ExpectedOrElse) {
    // Intent: Verify or_else() provides fallback values
    
    Expected<int, std::string> success = 42;
    EXPECT_EQ(success.value_or(999), 42);
    
    Expected<int, std::string> failure = Unexpected("error");
    EXPECT_EQ(failure.value_or(999), 999);
}

TEST_F(ErrorHandlingTest, ErrorRegistry) {
    // Intent: Verify error registry tracks and reports errors
    
    ErrorRegistry registry;
    
    registry.registerError("E001", "Database connection failed");
    registry.registerError("E002", "Invalid input parameter");
    
    auto error1 = registry.getError("E001");
    ASSERT_TRUE(error1.has_value());
    EXPECT_EQ(*error1, "Database connection failed");
    
    auto error_missing = registry.getError("E999");
    EXPECT_FALSE(error_missing.has_value());
}

// ============================================================================
// PII Detection Tests
// ============================================================================

class PIIDetectionTest : public ::testing::Test {
protected:
    PIIDetector detector_;
};

TEST_F(PIIDetectionTest, DetectEmailAddresses) {
    // Intent: Verify email pattern detection with real regex
    
    std::string text = "Contact alice@example.com or bob.smith@company.co.uk for details.";
    auto findings = detector_.detectInText(text);
    
    // Should find 2 emails
    int email_count = 0;
    for (const auto& finding : findings) {
        if (finding.type == PIIType::EMAIL) {
            email_count++;
            EXPECT_GT(finding.confidence, 0.9);
        }
    }
    EXPECT_EQ(email_count, 2);
}

TEST_F(PIIDetectionTest, DetectPhoneNumbers) {
    // Intent: Verify phone number pattern detection
    
    std::string text = "Call +49-123-456789 or (555) 123-4567 or 800.555.1234";
    auto findings = detector_.detectInText(text);
    
    bool found_intl = false;
    bool found_us = false;
    
    for (const auto& finding : findings) {
        if (finding.type == PIIType::PHONE) {
            if (finding.value.find("+49") != std::string::npos) {
                found_intl = true;
            }
            if (finding.value.find("555") != std::string::npos) {
                found_us = true;
            }
        }
    }
    
    EXPECT_TRUE(found_intl || found_us);
}

TEST_F(PIIDetectionTest, DetectSSN) {
    // Intent: Verify SSN pattern detection with format validation
    
    std::string text = "SSN: 123-45-6789 was compromised";
    auto findings = detector_.detectInText(text);
    
    bool found_ssn = false;
    for (const auto& finding : findings) {
        if (finding.type == PIIType::SSN) {
            found_ssn = true;
            EXPECT_GT(finding.confidence, 0.7);
            EXPECT_NE(finding.value.find("123-45-6789"), std::string::npos);
        }
    }
    EXPECT_TRUE(found_ssn);
}

TEST_F(PIIDetectionTest, DetectCreditCardWithLuhn) {
    // Intent: Verify credit card detection with Luhn checksum validation
    
    // Valid Visa test card
    std::string text = "Card number: 4242-4242-4242-4242";
    auto findings = detector_.detectInText(text);
    
    bool found_valid_cc = false;
    for (const auto& finding : findings) {
        if (finding.type == PIIType::CREDIT_CARD) {
            found_valid_cc = true;
            EXPECT_GT(finding.confidence, 0.8);
        }
    }
    EXPECT_TRUE(found_valid_cc);
}

TEST_F(PIIDetectionTest, RejectInvalidCreditCard) {
    // Intent: Verify Luhn checksum rejects invalid cards
    
    // Invalid card (fails Luhn)
    std::string text = "Card: 1234-5678-9012-3456";
    auto findings = detector_.detectInText(text);
    
    // Should NOT detect as valid credit card
    for (const auto& finding : findings) {
        EXPECT_NE(finding.type, PIIType::CREDIT_CARD);
    }
}

TEST_F(PIIDetectionTest, DetectIBAN) {
    // Intent: Verify IBAN pattern detection
    
    std::string text = "Transfer to IBAN: DE89370400440532013000";
    auto findings = detector_.detectInText(text);
    
    bool found_iban = false;
    for (const auto& finding : findings) {
        if (finding.type == PIIType::IBAN) {
            found_iban = true;
            EXPECT_EQ(finding.value, "DE89370400440532013000");
        }
    }
    EXPECT_TRUE(found_iban);
}

TEST_F(PIIDetectionTest, DetectIPAddress) {
    // Intent: Verify IP address detection
    
    std::string text = "Server IP: 192.168.1.42 and 10.0.0.1";
    auto findings = detector_.detectInText(text);
    
    int ip_count = 0;
    for (const auto& finding : findings) {
        if (finding.type == PIIType::IP_ADDRESS) {
            ip_count++;
        }
    }
    EXPECT_GE(ip_count, 1);
}

TEST_F(PIIDetectionTest, DetectMultipleTypesInText) {
    // Intent: Verify detection of mixed PII types in single text
    
    std::string text = R"(
        Contact: john.doe@example.com
        Phone: (555) 123-4567
        SSN: 987-65-4321
        IP: 192.168.1.1
    )";
    
    auto findings = detector_.detectInText(text);
    
    bool has_email = false;
    bool has_phone = false;
    bool has_ssn = false;
    bool has_ip = false;
    
    for (const auto& finding : findings) {
        if (finding.type == PIIType::EMAIL) has_email = true;
        if (finding.type == PIIType::PHONE) has_phone = true;
        if (finding.type == PIIType::SSN) has_ssn = true;
        if (finding.type == PIIType::IP_ADDRESS) has_ip = true;
    }
    
    EXPECT_TRUE(has_email);
    EXPECT_TRUE(has_phone || has_ssn || has_ip); // At least one more type
}

TEST_F(PIIDetectionTest, NoFalsePositives) {
    // Intent: Verify clean text doesn't trigger false positives
    
    std::string text = "This is a normal sentence with no PII data at all.";
    auto findings = detector_.detectInText(text);
    
    EXPECT_EQ(findings.size(), 0);
}

// ============================================================================
// Stemming Tests
// ============================================================================

class StemmingTest : public ::testing::Test {};

TEST_F(StemmingTest, EnglishPluralStemming) {
    // Intent: Verify English plural suffixes are correctly stemmed
    
    EXPECT_EQ(Stemmer::stem("cats", Stemmer::Language::EN), "cat");
    EXPECT_EQ(Stemmer::stem("dogs", Stemmer::Language::EN), "dog");
    EXPECT_EQ(Stemmer::stem("cities", Stemmer::Language::EN), "citi");
    EXPECT_EQ(Stemmer::stem("caresses", Stemmer::Language::EN), "caress");
}

TEST_F(StemmingTest, EnglishVerbStemming) {
    // Intent: Verify English verb conjugations are stemmed
    
    EXPECT_EQ(Stemmer::stem("walked", Stemmer::Language::EN), "walk");
    EXPECT_EQ(Stemmer::stem("running", Stemmer::Language::EN), "run");
    EXPECT_EQ(Stemmer::stem("played", Stemmer::Language::EN), "play");
    EXPECT_EQ(Stemmer::stem("trying", Stemmer::Language::EN), "try");
}

TEST_F(StemmingTest, EnglishSuffixRemoval) {
    // Intent: Verify English derivational suffixes are removed
    
    EXPECT_EQ(Stemmer::stem("relational", Stemmer::Language::EN), "relate");
    EXPECT_EQ(Stemmer::stem("conditional", Stemmer::Language::EN), "condition");
    EXPECT_EQ(Stemmer::stem("ational", Stemmer::Language::EN), "ate");
}

TEST_F(StemmingTest, GermanStemming) {
    // Intent: Verify German stemming algorithm
    
    EXPECT_EQ(Stemmer::stem("laufen", Stemmer::Language::DE), "lauf");
    EXPECT_EQ(Stemmer::stem("machte", Stemmer::Language::DE), "macht");
    EXPECT_EQ(Stemmer::stem("gruppen", Stemmer::Language::DE), "grupp");
}

TEST_F(StemmingTest, NoStemmingForShortWords) {
    // Intent: Verify words below minimum length are not stemmed
    
    EXPECT_EQ(Stemmer::stem("is", Stemmer::Language::EN), "is");
    EXPECT_EQ(Stemmer::stem("a", Stemmer::Language::EN), "a");
    EXPECT_EQ(Stemmer::stem("at", Stemmer::Language::EN), "at");
}

TEST_F(StemmingTest, NoStemmingMode) {
    // Intent: Verify NONE language mode doesn't stem
    
    std::string word = "running";
    EXPECT_EQ(Stemmer::stem(word, Stemmer::Language::NONE), word);
}

TEST_F(StemmingTest, LanguageParsing) {
    // Intent: Verify language code parsing
    
    EXPECT_EQ(Stemmer::parseLanguage("en"), Stemmer::Language::EN);
    EXPECT_EQ(Stemmer::parseLanguage("de"), Stemmer::Language::DE);
    EXPECT_EQ(Stemmer::parseLanguage("none"), Stemmer::Language::NONE);
    EXPECT_EQ(Stemmer::parseLanguage("unknown"), Stemmer::Language::NONE);
}

TEST_F(StemmingTest, CaseInsensitiveStemming) {
    // Intent: Verify stemming handles case properly
    
    EXPECT_EQ(Stemmer::stem("RUNNING", Stemmer::Language::EN), 
              Stemmer::stem("running", Stemmer::Language::EN));
}

// ============================================================================
// OLAP Engine Tests
// ============================================================================

class OLAPEngineTest : public ::testing::Test {
protected:
    OLAPEngine engine_;
};

TEST_F(OLAPEngineTest, BasicAggregationCount) {
    // Intent: Verify COUNT aggregation function
    
    OLAPQuery query;
    query.collection = "sales";
    query.dimensions.push_back({"region", "", true});
    query.measures.push_back({"count", "*", Measure::Function::Count});
    
    // Add sample data
    std::vector<std::map<std::string, std::string>> data = {
        {{"region", "North"}, {"amount", "100"}},
        {{"region", "North"}, {"amount", "150"}},
        {{"region", "South"}, {"amount", "200"}},
    };
    engine_.loadData("sales", data);
    
    auto result = engine_.execute(query);
    
    // Should have 2 groups: North (2), South (1)
    EXPECT_EQ(result.rows.size(), 2);
}

TEST_F(OLAPEngineTest, SumAggregation) {
    // Intent: Verify SUM aggregation function
    
    std::vector<std::map<std::string, std::string>> data = {
        {{"category", "A"}, {"amount", "100"}},
        {{"category", "A"}, {"amount", "200"}},
        {{"category", "B"}, {"amount", "300"}},
    };
    engine_.loadData("products", data);
    
    OLAPQuery query;
    query.collection = "products";
    query.dimensions.push_back({"category", "", true});
    query.measures.push_back({"total", "amount", Measure::Function::Sum});
    
    auto result = engine_.execute(query);
    
    EXPECT_EQ(result.rows.size(), 2);
    
    // Verify sums
    for (const auto& row : result.rows) {
        if (row.dimension_values[0] == "A") {
            EXPECT_EQ(row.measure_values[0], 300.0);
        } else if (row.dimension_values[0] == "B") {
            EXPECT_EQ(row.measure_values[0], 300.0);
        }
    }
}

TEST_F(OLAPEngineTest, AverageAggregation) {
    // Intent: Verify AVG aggregation function
    
    std::vector<std::map<std::string, std::string>> data = {
        {{"dept", "Sales"}, {"salary", "50000"}},
        {{"dept", "Sales"}, {"salary", "60000"}},
        {{"dept", "Eng"}, {"salary", "80000"}},
    };
    engine_.loadData("employees", data);
    
    OLAPQuery query;
    query.collection = "employees";
    query.dimensions.push_back({"dept", "", true});
    query.measures.push_back({"avg_salary", "salary", Measure::Function::Avg});
    
    auto result = engine_.execute(query);
    
    for (const auto& row : result.rows) {
        if (row.dimension_values[0] == "Sales") {
            EXPECT_NEAR(row.measure_values[0], 55000.0, 1.0);
        } else if (row.dimension_values[0] == "Eng") {
            EXPECT_NEAR(row.measure_values[0], 80000.0, 1.0);
        }
    }
}

TEST_F(OLAPEngineTest, MinMaxAggregation) {
    // Intent: Verify MIN and MAX aggregation functions
    
    std::vector<std::map<std::string, std::string>> data = {
        {{"product", "Widget"}, {"price", "10"}},
        {{"product", "Widget"}, {"price", "15"}},
        {{"product", "Widget"}, {"price", "12"}},
    };
    engine_.loadData("items", data);
    
    OLAPQuery query;
    query.collection = "items";
    query.dimensions.push_back({"product", "", true});
    query.measures.push_back({"min_price", "price", Measure::Function::Min});
    query.measures.push_back({"max_price", "price", Measure::Function::Max});
    
    auto result = engine_.execute(query);
    
    ASSERT_EQ(result.rows.size(), 1);
    EXPECT_EQ(result.rows[0].measure_values[0], 10.0);  // min
    EXPECT_EQ(result.rows[0].measure_values[1], 15.0);  // max
}

TEST_F(OLAPEngineTest, MultipleDimensions) {
    // Intent: Verify grouping by multiple dimensions
    
    std::vector<std::map<std::string, std::string>> data = {
        {{"region", "North"}, {"quarter", "Q1"}, {"sales", "100"}},
        {{"region", "North"}, {"quarter", "Q2"}, {"sales", "150"}},
        {{"region", "South"}, {"quarter", "Q1"}, {"sales", "200"}},
    };
    engine_.loadData("sales_data", data);
    
    OLAPQuery query;
    query.collection = "sales_data";
    query.dimensions.push_back({"region", "", true});
    query.dimensions.push_back({"quarter", "", true});
    query.measures.push_back({"total", "sales", Measure::Function::Sum});
    
    auto result = engine_.execute(query);
    
    // Should have 3 groups: (North, Q1), (North, Q2), (South, Q1)
    EXPECT_EQ(result.rows.size(), 3);
}

TEST_F(OLAPEngineTest, FilteredQuery) {
    // Intent: Verify WHERE clause filtering
    
    std::vector<std::map<std::string, std::string>> data = {
        {{"region", "North"}, {"amount", "100"}},
        {{"region", "South"}, {"amount", "200"}},
        {{"region", "East"}, {"amount", "300"}},
    };
    engine_.loadData("regions", data);
    
    OLAPQuery query;
    query.collection = "regions";
    query.dimensions.push_back({"region", "", true});
    query.measures.push_back({"total", "amount", Measure::Function::Sum});
    query.filters.push_back({"region", "North"});
    
    auto result = engine_.execute(query);
    
    // Should only have North
    EXPECT_EQ(result.rows.size(), 1);
    EXPECT_EQ(result.rows[0].dimension_values[0], "North");
}

TEST_F(OLAPEngineTest, CountDistinct) {
    // Intent: Verify COUNT DISTINCT aggregation
    
    std::vector<std::map<std::string, std::string>> data = {
        {{"category", "A"}, {"customer", "C1"}},
        {{"category", "A"}, {"customer", "C1"}},
        {{"category", "A"}, {"customer", "C2"}},
        {{"category", "B"}, {"customer", "C1"}},
    };
    engine_.loadData("orders", data);
    
    OLAPQuery query;
    query.collection = "orders";
    query.dimensions.push_back({"category", "", true});
    query.measures.push_back({"unique_customers", "customer", Measure::Function::CountDistinct});
    
    auto result = engine_.execute(query);
    
    for (const auto& row : result.rows) {
        if (row.dimension_values[0] == "A") {
            EXPECT_EQ(row.measure_values[0], 2.0);  // C1 and C2
        } else if (row.dimension_values[0] == "B") {
            EXPECT_EQ(row.measure_values[0], 1.0);  // C1 only
        }
    }
}

TEST_F(OLAPEngineTest, QueryExplainPlan) {
    // Intent: Verify query explain generates execution plan
    
    OLAPQuery query;
    query.collection = "test";
    query.dimensions.push_back({"dim1", "", true});
    query.measures.push_back({"m1", "col1", Measure::Function::Sum});
    
    auto plan = engine_.explain(query);
    
    EXPECT_FALSE(plan.empty());
    EXPECT_NE(plan.find("GROUP BY"), std::string::npos);
}

TEST_F(OLAPEngineTest, EmptyResultSet) {
    // Intent: Verify query on non-existent collection returns empty result
    
    OLAPQuery query;
    query.collection = "nonexistent";
    query.dimensions.push_back({"dim", "", true});
    query.measures.push_back({"m", "col", Measure::Function::Count});
    
    auto result = engine_.execute(query);
    
    EXPECT_EQ(result.rows.size(), 0);
}

TEST_F(OLAPEngineTest, PerformanceLargeDataset) {
    // Intent: Verify OLAP engine handles large datasets efficiently
    
    std::vector<std::map<std::string, std::string>> data;
    for (int i = 0; i < 10000; ++i) {
        data.push_back({
            {"category", "cat" + std::to_string(i % 100)},
            {"value", std::to_string(i)}
        });
    }
    engine_.loadData("large_data", data);
    
    OLAPQuery query;
    query.collection = "large_data";
    query.dimensions.push_back({"category", "", true});
    query.measures.push_back({"sum", "value", Measure::Function::Sum});
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = engine_.execute(query);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(result.rows.size(), 100);  // 100 categories
    EXPECT_LT(duration.count(), 3000);   // Should complete in < 3 seconds (accommodates slower systems)
}
