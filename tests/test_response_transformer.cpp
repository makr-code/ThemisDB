/**
 * @file test_response_transformer.cpp
 * @brief Unit tests for ResponseTransformer - version-specific serializers,
 *        field renaming, default values, and request transformation layer.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include "server/response_transformer.h"

using namespace themis::server;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static APIVersion v(uint32_t major, uint32_t minor = 0, uint32_t patch = 0) {
    return APIVersion{major, minor, patch};
}

// ---------------------------------------------------------------------------
// registerVersion / transform
// ---------------------------------------------------------------------------

TEST(ResponseTransformerTest, NoTransformRegistered_ReturnsPayloadUnchanged) {
    ResponseTransformer transformer;
    nlohmann::json payload = {{"id", 123}, {"type", "user"}};

    auto result = transformer.transform(payload, v(1));
    EXPECT_EQ(result, payload);
}

TEST(ResponseTransformerTest, RegisteredTransformIsApplied) {
    ResponseTransformer transformer;
    // v1 renames "id" to "user_id"
    transformer.registerVersion("v1", [](nlohmann::json res) {
        if (res.contains("id")) {
            res["user_id"] = res["id"];
            res.erase("id");
        }
        return res;
    });

    nlohmann::json payload = {{"id", 123}, {"type", "user"}};
    auto result = transformer.transform(payload, v(1));

    EXPECT_FALSE(result.contains("id"));
    EXPECT_TRUE(result.contains("user_id"));
    EXPECT_EQ(result["user_id"], 123);
    EXPECT_EQ(result["type"], "user");
}

TEST(ResponseTransformerTest, V2TransformReturnsNativeFormat) {
    ResponseTransformer transformer;
    transformer.registerVersion("v1", [](nlohmann::json res) {
        if (res.contains("id")) {
            res["user_id"] = res["id"];
            res.erase("id");
        }
        return res;
    });
    transformer.registerVersion("v2", [](nlohmann::json res) {
        return res; // native format
    });

    nlohmann::json payload = {{"id", 123}, {"type", "user"}};

    auto v2_result = transformer.transform(payload, v(2));
    EXPECT_EQ(v2_result, payload);
}

TEST(ResponseTransformerTest, OriginalPayloadIsNotMutated) {
    ResponseTransformer transformer;
    transformer.registerVersion("v1", [](nlohmann::json res) {
        res["extra"] = "added";
        return res;
    });

    nlohmann::json payload = {{"id", 1}};
    transformer.transform(payload, v(1));

    EXPECT_FALSE(payload.contains("extra")) << "Original payload must not be mutated";
}

// ---------------------------------------------------------------------------
// Version key resolution (exact, major.minor, major-only)
// ---------------------------------------------------------------------------

TEST(ResponseTransformerTest, ExactSemverKeyTakesPrecedence) {
    ResponseTransformer transformer;
    transformer.registerVersion("v1", [](nlohmann::json res) {
        res["key"] = "major_only";
        return res;
    });
    transformer.registerVersion("v1.4", [](nlohmann::json res) {
        res["key"] = "major_minor";
        return res;
    });
    transformer.registerVersion("v1.4.1", [](nlohmann::json res) {
        res["key"] = "exact";
        return res;
    });

    nlohmann::json payload = {};
    auto result = transformer.transform(payload, v(1, 4, 1));
    EXPECT_EQ(result["key"], "exact");
}

TEST(ResponseTransformerTest, MajorMinorKeyFallback) {
    ResponseTransformer transformer;
    transformer.registerVersion("v1", [](nlohmann::json res) {
        res["key"] = "major_only";
        return res;
    });
    transformer.registerVersion("v1.4", [](nlohmann::json res) {
        res["key"] = "major_minor";
        return res;
    });

    nlohmann::json payload = {};
    auto result = transformer.transform(payload, v(1, 4, 2));
    EXPECT_EQ(result["key"], "major_minor");
}

TEST(ResponseTransformerTest, MajorOnlyKeyFallback) {
    ResponseTransformer transformer;
    transformer.registerVersion("v1", [](nlohmann::json res) {
        res["key"] = "major_only";
        return res;
    });

    nlohmann::json payload = {};
    auto result = transformer.transform(payload, v(1, 3, 0));
    EXPECT_EQ(result["key"], "major_only");
}

// ---------------------------------------------------------------------------
// addFieldRename
// ---------------------------------------------------------------------------

TEST(ResponseTransformerTest, FieldRenameOldToNew) {
    ResponseTransformer transformer;
    transformer.addFieldRename("v1", "id", "user_id");

    nlohmann::json payload = {{"id", 42}, {"name", "Alice"}};
    auto result = transformer.transform(payload, v(1));

    EXPECT_FALSE(result.contains("id"));
    EXPECT_EQ(result["user_id"], 42);
    EXPECT_EQ(result["name"], "Alice");
}

TEST(ResponseTransformerTest, FieldRenameAbsentFieldIsNoOp) {
    ResponseTransformer transformer;
    transformer.addFieldRename("v1", "nonexistent", "new_name");

    nlohmann::json payload = {{"id", 1}};
    auto result = transformer.transform(payload, v(1));

    EXPECT_FALSE(result.contains("new_name"));
    EXPECT_EQ(result["id"], 1);
}

TEST(ResponseTransformerTest, MultipleFieldRenamesAppliedInOrder) {
    ResponseTransformer transformer;
    transformer.addFieldRename("v1", "a", "b");
    transformer.addFieldRename("v1", "c", "d");

    nlohmann::json payload = {{"a", 1}, {"c", 2}};
    auto result = transformer.transform(payload, v(1));

    EXPECT_EQ(result["b"], 1);
    EXPECT_EQ(result["d"], 2);
    EXPECT_FALSE(result.contains("a"));
    EXPECT_FALSE(result.contains("c"));
}

// ---------------------------------------------------------------------------
// addDefaultValue
// ---------------------------------------------------------------------------

TEST(ResponseTransformerTest, DefaultValueAddedForMissingField) {
    ResponseTransformer transformer;
    transformer.addDefaultValue("v2", "type", "user");

    nlohmann::json payload = {{"id", 123}};
    auto result = transformer.transform(payload, v(2));

    EXPECT_EQ(result["type"], "user");
    EXPECT_EQ(result["id"], 123);
}

TEST(ResponseTransformerTest, DefaultValueNotOverwrittenWhenFieldPresent) {
    ResponseTransformer transformer;
    transformer.addDefaultValue("v2", "type", "default_type");

    nlohmann::json payload = {{"id", 1}, {"type", "admin"}};
    auto result = transformer.transform(payload, v(2));

    EXPECT_EQ(result["type"], "admin"); // Existing value preserved
}

TEST(ResponseTransformerTest, DefaultValueWithJsonObject) {
    ResponseTransformer transformer;
    transformer.addDefaultValue("v2", "meta", nlohmann::json{{"version", 2}});

    nlohmann::json payload = {{"id", 1}};
    auto result = transformer.transform(payload, v(2));

    ASSERT_TRUE(result["meta"].is_object());
    EXPECT_EQ(result["meta"]["version"], 2);
}

// ---------------------------------------------------------------------------
// Combined field rename + default value + transform function
// ---------------------------------------------------------------------------

TEST(ResponseTransformerTest, FieldMappingsAppliedBeforeTransformFn) {
    ResponseTransformer transformer;
    // Rename "id" to "user_id" first, then append "_processed"
    transformer.addFieldRename("v1", "id", "user_id");
    transformer.registerVersion("v1", [](nlohmann::json res) {
        // At this point "id" should already be "user_id"
        if (res.contains("user_id")) {
            res["processed"] = true;
        }
        return res;
    });

    nlohmann::json payload = {{"id", 7}};
    auto result = transformer.transform(payload, v(1));

    EXPECT_FALSE(result.contains("id"));
    EXPECT_EQ(result["user_id"], 7);
    EXPECT_EQ(result["processed"], true);
}

TEST(ResponseTransformerTest, DefaultValueAvailableInTransformFn) {
    ResponseTransformer transformer;
    transformer.addDefaultValue("v2", "type", "user");
    transformer.registerVersion("v2", [](nlohmann::json res) {
        // type should already be populated by default value
        if (res["type"] == "user") {
            res["role"] = "member";
        }
        return res;
    });

    nlohmann::json payload = {{"id", 5}};
    auto result = transformer.transform(payload, v(2));

    EXPECT_EQ(result["type"], "user");
    EXPECT_EQ(result["role"], "member");
}

// ---------------------------------------------------------------------------
// hasVersion / registeredVersions
// ---------------------------------------------------------------------------

TEST(ResponseTransformerTest, HasVersionReturnsTrueWhenRegistered) {
    ResponseTransformer transformer;
    transformer.registerVersion("v1", [](nlohmann::json r) { return r; });

    EXPECT_TRUE(transformer.hasVersion(v(1)));
    EXPECT_FALSE(transformer.hasVersion(v(2)));
}

TEST(ResponseTransformerTest, HasVersionWithFieldRenameOnly) {
    ResponseTransformer transformer;
    transformer.addFieldRename("v1", "a", "b");

    EXPECT_TRUE(transformer.hasVersion(v(1)));
}

TEST(ResponseTransformerTest, RegisteredVersionsContainsAllKeys) {
    ResponseTransformer transformer;
    transformer.registerVersion("v1", [](nlohmann::json r) { return r; });
    transformer.registerVersion("v2", [](nlohmann::json r) { return r; });

    auto versions = transformer.registeredVersions();
    EXPECT_GE(versions.size(), 2u);

    bool has_v1 = std::find(versions.begin(), versions.end(), "v1") != versions.end();
    bool has_v2 = std::find(versions.begin(), versions.end(), "v2") != versions.end();
    EXPECT_TRUE(has_v1);
    EXPECT_TRUE(has_v2);
}

// ---------------------------------------------------------------------------
// Issue example: v1 / v2 field restructuring
// ---------------------------------------------------------------------------

TEST(ResponseTransformerTest, IssueExampleV1V2Restructuring) {
    // From the issue:
    // v1 API: {"user_id": 123}
    // v2 API: {"id": 123, "type": "user"}

    ResponseTransformer transformer;
    transformer.registerVersion("v1", [](nlohmann::json res) {
        nlohmann::json v1;
        if (res.contains("id")) {
            v1["user_id"] = res["id"];
        }
        return v1;
    });
    transformer.registerVersion("v2", [](nlohmann::json res) {
        return res; // Native format
    });

    nlohmann::json native = {{"id", 123}, {"type", "user"}};

    auto v1_result = transformer.transform(native, v(1));
    EXPECT_EQ(v1_result["user_id"], 123);
    EXPECT_FALSE(v1_result.contains("id"));
    EXPECT_FALSE(v1_result.contains("type"));

    auto v2_result = transformer.transform(native, v(2));
    EXPECT_EQ(v2_result["id"], 123);
    EXPECT_EQ(v2_result["type"], "user");
}
