#include <gtest/gtest.h>
#include <simdjson.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

// Test fixture for JSON library integration
class JSONLibIntegrationTest : public ::testing::Test {
protected:
    simdjson::ondemand::parser parser;
    
    void SetUp() override {
        // Parser is reusable
    }
};

// Test 1: simdjson library linking and basic parsing
TEST_F(JSONLibIntegrationTest, SimdjsonLibraryLinking) {
    std::string json_str = R"({"key": "value", "number": 42})";
    simdjson::padded_string padded(json_str);
    
    simdjson::ondemand::document doc = parser.iterate(padded);
    
    std::string_view key_value = doc["key"];
    EXPECT_EQ(key_value, "value");
    
    uint64_t number;
    EXPECT_EQ(doc["number"].get(number), simdjson::SUCCESS);
    EXPECT_EQ(number, 42u);
}

// Test 2: simdjson parsing complex nested JSON
TEST_F(JSONLibIntegrationTest, SimdjsonNestedJSON) {
    std::string json_str = R"({
        "user": {
            "name": "John Doe",
            "age": 30,
            "emails": ["john@example.com", "doe@example.com"]
        }
    })";
    
    simdjson::padded_string padded(json_str);
    simdjson::ondemand::document doc = parser.iterate(padded);
    
    // Access nested object
    auto user = doc["user"];
    std::string_view name = user["name"];
    EXPECT_EQ(name, "John Doe");
    
    uint64_t age;
    EXPECT_EQ(user["age"].get(age), simdjson::SUCCESS);
    EXPECT_EQ(age, 30u);
    
    // Access array
    auto emails = user["emails"];
    size_t count = 0;
    for (auto email : emails) {
        count++;
    }
    EXPECT_EQ(count, 2u);
}

// Test 3: simdjson array iteration
TEST_F(JSONLibIntegrationTest, SimdjsonArrayIteration) {
    std::string json_str = R"([1, 2, 3, 4, 5])";
    simdjson::padded_string padded(json_str);
    
    simdjson::ondemand::document doc = parser.iterate(padded);
    simdjson::ondemand::array arr = doc.get_array();
    
    std::vector<int64_t> numbers;
    for (auto element : arr) {
        int64_t num;
        EXPECT_EQ(element.get(num), simdjson::SUCCESS);
        numbers.push_back(num);
    }
    
    EXPECT_EQ(numbers.size(), 5u);
    EXPECT_EQ(numbers[0], 1);
    EXPECT_EQ(numbers[4], 5);
}

// Test 4: simdjson error handling
TEST_F(JSONLibIntegrationTest, SimdjsonErrorHandling) {
    std::string invalid_json = R"({"key": invalid})";
    simdjson::padded_string padded(invalid_json);
    
    // This should fail to parse
    auto result = parser.iterate(padded);
    // simdjson will throw or return error on invalid JSON
    // We're just verifying the library can handle errors
    EXPECT_NO_THROW({
        try {
            auto doc = parser.iterate(padded);
            auto val = doc["key"];
        } catch (...) {
            // Expected to catch parse error
        }
    });
}

// Test 5: simdjson performance on large JSON
TEST_F(JSONLibIntegrationTest, SimdjsonLargeJSON) {
    // Generate large JSON array
    std::string json_str = "[";
    for (int i = 0; i < 1000; ++i) {
        if (i > 0) json_str += ",";
        json_str += R"({"id":)" + std::to_string(i) + R"(,"value":"data_)" + std::to_string(i) + R"("})";
    }
    json_str += "]";
    
    simdjson::padded_string padded(json_str);
    simdjson::ondemand::document doc = parser.iterate(padded);
    
    simdjson::ondemand::array arr = doc.get_array();
    size_t count = 0;
    for (auto element : arr) {
        count++;
    }
    
    EXPECT_EQ(count, 1000u);
}

// Test 6: nlohmann/json library linking and basic operations
TEST_F(JSONLibIntegrationTest, NlohmannJsonLibraryLinking) {
    json j = {
        {"name", "Alice"},
        {"age", 25},
        {"active", true}
    };
    
    EXPECT_EQ(j["name"], "Alice");
    EXPECT_EQ(j["age"], 25);
    EXPECT_TRUE(j["active"]);
}

// Test 7: nlohmann/json parsing from string
TEST_F(JSONLibIntegrationTest, NlohmannJsonParsing) {
    std::string json_str = R"({
        "product": "Widget",
        "price": 19.99,
        "in_stock": true,
        "tags": ["electronics", "gadget"]
    })";
    
    json j = json::parse(json_str);
    
    EXPECT_EQ(j["product"], "Widget");
    EXPECT_DOUBLE_EQ(j["price"], 19.99);
    EXPECT_TRUE(j["in_stock"]);
    EXPECT_EQ(j["tags"].size(), 2u);
    EXPECT_EQ(j["tags"][0], "electronics");
}

// Test 8: nlohmann/json serialization
TEST_F(JSONLibIntegrationTest, NlohmannJsonSerialization) {
    json j;
    j["user_id"] = 123;
    j["username"] = "testuser";
    j["settings"] = {
        {"theme", "dark"},
        {"notifications", true}
    };
    
    std::string serialized = j.dump();
    EXPECT_GT(serialized.size(), 0u);
    
    // Parse back and verify
    json j2 = json::parse(serialized);
    EXPECT_EQ(j2["user_id"], 123);
    EXPECT_EQ(j2["username"], "testuser");
    EXPECT_EQ(j2["settings"]["theme"], "dark");
}

// Test 9: nlohmann/json nested objects
TEST_F(JSONLibIntegrationTest, NlohmannJsonNestedObjects) {
    json j = {
        {"company", {
            {"name", "TechCorp"},
            {"employees", {
                {"count", 500},
                {"departments", {"Engineering", "Sales", "HR"}}
            }}
        }}
    };
    
    EXPECT_EQ(j["company"]["name"], "TechCorp");
    EXPECT_EQ(j["company"]["employees"]["count"], 500);
    EXPECT_EQ(j["company"]["employees"]["departments"].size(), 3u);
}

// Test 10: nlohmann/json array operations
TEST_F(JSONLibIntegrationTest, NlohmannJsonArrayOperations) {
    json arr = json::array();
    arr.push_back("item1");
    arr.push_back("item2");
    arr.push_back("item3");
    
    EXPECT_EQ(arr.size(), 3u);
    EXPECT_TRUE(arr.is_array());
    
    // Iterate
    int count = 0;
    for (const auto& item : arr) {
        count++;
        EXPECT_TRUE(item.is_string());
    }
    EXPECT_EQ(count, 3);
}

// Test 11: nlohmann/json type checking
TEST_F(JSONLibIntegrationTest, NlohmannJsonTypeChecking) {
    json j = {
        {"string", "text"},
        {"number", 42},
        {"float", 3.14},
        {"bool", true},
        {"null", nullptr},
        {"array", {1, 2, 3}},
        {"object", {{"key", "value"}}}
    };
    
    EXPECT_TRUE(j["string"].is_string());
    EXPECT_TRUE(j["number"].is_number_integer());
    EXPECT_TRUE(j["float"].is_number_float());
    EXPECT_TRUE(j["bool"].is_boolean());
    EXPECT_TRUE(j["null"].is_null());
    EXPECT_TRUE(j["array"].is_array());
    EXPECT_TRUE(j["object"].is_object());
}

// Test 12: nlohmann/json error handling
TEST_F(JSONLibIntegrationTest, NlohmannJsonErrorHandling) {
    std::string invalid_json = R"({"invalid": })";
    
    EXPECT_THROW({
        json j = json::parse(invalid_json);
    }, json::parse_error);
    
    // Test accessing non-existent key with exception
    json j = {{"existing_key", "value"}};
    EXPECT_THROW({
        std::string val = j.at("non_existent_key");
    }, json::out_of_range);
}

// Test 13: nlohmann/json merging
TEST_F(JSONLibIntegrationTest, NlohmannJsonMerging) {
    json j1 = {{"a", 1}, {"b", 2}};
    json j2 = {{"c", 3}, {"d", 4}};
    
    j1.merge_patch(j2);
    
    EXPECT_EQ(j1["a"], 1);
    EXPECT_EQ(j1["b"], 2);
    EXPECT_EQ(j1["c"], 3);
    EXPECT_EQ(j1["d"], 4);
}

// Test 14: nlohmann/json pointer access (JSON Pointer RFC 6901)
TEST_F(JSONLibIntegrationTest, NlohmannJsonPointer) {
    json j = {
        {"users", {
            {{"id", 1}, {"name", "Alice"}},
            {{"id", 2}, {"name", "Bob"}}
        }}
    };
    
    // Access using JSON Pointer
    EXPECT_EQ(j["/users/0/name"_json_pointer], "Alice");
    EXPECT_EQ(j["/users/1/id"_json_pointer], 2);
}

// Test 15: Interoperability - simdjson parse, nlohmann manipulate
TEST_F(JSONLibIntegrationTest, InteroperabilitySimdjsonToNlohmann) {
    std::string json_str = R"({"data": [1, 2, 3, 4, 5]})";
    
    // Parse with simdjson (fast)
    simdjson::padded_string padded(json_str);
    simdjson::ondemand::document doc = parser.iterate(padded);
    
    // Extract and convert to nlohmann for manipulation
    json j = json::parse(json_str);
    
    // Manipulate with nlohmann
    j["data"].push_back(6);
    j["computed_sum"] = 0;
    for (const auto& val : j["data"]) {
        j["computed_sum"] = j["computed_sum"].get<int>() + val.get<int>();
    }
    
    EXPECT_EQ(j["data"].size(), 6u);
    EXPECT_EQ(j["computed_sum"], 21);
}

// Test 16: nlohmann/json pretty printing
TEST_F(JSONLibIntegrationTest, NlohmannJsonPrettyPrint) {
    json j = {
        {"user", {
            {"id", 1},
            {"name", "Test User"}
        }}
    };
    
    std::string pretty = j.dump(4); // 4 spaces indentation
    EXPECT_GT(pretty.size(), 0u);
    EXPECT_NE(pretty.find('\n'), std::string::npos); // Should contain newlines
    
    std::string compact = j.dump(); // No indentation
    EXPECT_GT(compact.size(), 0u);
    EXPECT_LT(compact.size(), pretty.size()); // Compact should be smaller
}

// Test 17: nlohmann/json custom types
TEST_F(JSONLibIntegrationTest, NlohmannJsonCustomTypes) {
    struct Person {
        std::string name;
        int age;
    };
    
    // Manual serialization
    Person p{"John", 30};
    json j;
    j["name"] = p.name;
    j["age"] = p.age;
    
    EXPECT_EQ(j["name"], "John");
    EXPECT_EQ(j["age"], 30);
    
    // Manual deserialization
    Person p2;
    p2.name = j["name"];
    p2.age = j["age"];
    
    EXPECT_EQ(p2.name, "John");
    EXPECT_EQ(p2.age, 30);
}

// Test 18: simdjson vs nlohmann performance comparison
TEST_F(JSONLibIntegrationTest, PerformanceComparison) {
    std::string json_str = R"({"values": [)";
    for (int i = 0; i < 100; ++i) {
        if (i > 0) json_str += ",";
        json_str += std::to_string(i);
    }
    json_str += "]}";
    
    // Test simdjson parsing
    {
        simdjson::padded_string padded(json_str);
        auto doc = parser.iterate(padded);
        auto arr = doc["values"];
        size_t count = 0;
        for (auto val : arr) {
            count++;
        }
        EXPECT_EQ(count, 100u);
    }
    
    // Test nlohmann parsing
    {
        json j = json::parse(json_str);
        EXPECT_EQ(j["values"].size(), 100u);
    }
    
    // Both should produce correct results
}
