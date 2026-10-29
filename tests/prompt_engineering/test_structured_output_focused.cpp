#include <gtest/gtest.h>

#include "prompt_engineering/structured_output.h"

namespace themis::prompt_engineering {
namespace {

StructuredOutputConfig payloadSchema() {
    StructuredOutputConfig config;
    config.type = OutputConstraintType::JSON_SCHEMA;
    config.json_schema.schema_json =
        R"({"type":"object","properties":{"payload":{"type":"string"},"nested":{"type":"object"}},"required":["payload"]})";
    return config;
}

TEST(StructuredOutputEnforcer, AcceptsEscapedDelimitersAndNestedKeys) {
    StructuredOutputEnforcer enforcer;
    const std::string output =
        R"({"payload":"a closing brace } and escaped quote: \"","nested":{"payload":"not top-level"}})";
    std::vector<std::string> errors;

    EXPECT_TRUE(enforcer.validate(output, payloadSchema(), errors));
    EXPECT_TRUE(errors.empty());
}

TEST(StructuredOutputEnforcer, NestedKeyDoesNotSatisfyTopLevelRequiredField) {
    StructuredOutputEnforcer enforcer;
    std::vector<std::string> errors;

    EXPECT_FALSE(enforcer.validate(
        R"({"nested":{"payload":"present"}})", payloadSchema(), errors));
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_NE(errors.front().find("Missing required field: \"payload\""),
              std::string::npos);
}

} // namespace
} // namespace themis::prompt_engineering
