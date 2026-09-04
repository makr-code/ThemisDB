// Copyright (c) 2025 VCC ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0
//
// YAML-cpp Library Advanced Integration Tests
// Tests comprehensive YAML parsing, serialization, and configuration validation
// Use Case: Configuration validation, schema verification, policy management

#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <chrono>

namespace fs = std::filesystem;

class YAMLLibAdvancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_yaml_dir_ = "./data/test_lib_yaml_" + 
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        fs::create_directories(test_yaml_dir_);
    }

    void TearDown() override {
        if (fs::exists(test_yaml_dir_)) {
            std::error_code ec;
            fs::remove_all(test_yaml_dir_, ec);
        }
    }

    std::string test_yaml_dir_;
    
    // Helper to write YAML to file
    void writeYAMLFile(const std::string& filename, const std::string& content) {
        std::ofstream ofs(test_yaml_dir_ + "/" + filename);
        ofs << content;
        ofs.close();
    }
};

// Test 1: Library linking and basic parsing
TEST_F(YAMLLibAdvancedTest, LibraryLinking) {
    std::string yaml_str = "key: value\nnumber: 42";
    YAML::Node node = YAML::Load(yaml_str);
    
    EXPECT_TRUE(node.IsMap());
    EXPECT_EQ(node["key"].as<std::string>(), "value");
    EXPECT_EQ(node["number"].as<int>(), 42);
}

// Test 2: Scalar types
TEST_F(YAMLLibAdvancedTest, ScalarTypes) {
    std::string yaml_str = R"(
string: "hello"
integer: 42
float: 3.14
boolean_true: true
boolean_false: false
null_value: null
)";
    
    YAML::Node node = YAML::Load(yaml_str);
    
    EXPECT_EQ(node["string"].as<std::string>(), "hello");
    EXPECT_EQ(node["integer"].as<int>(), 42);
    EXPECT_DOUBLE_EQ(node["float"].as<double>(), 3.14);
    EXPECT_TRUE(node["boolean_true"].as<bool>());
    EXPECT_FALSE(node["boolean_false"].as<bool>());
    EXPECT_TRUE(node["null_value"].IsNull());
}

// Test 3: Sequences (arrays)
TEST_F(YAMLLibAdvancedTest, Sequences) {
    std::string yaml_str = R"(
fruits:
  - apple
  - banana
  - orange
numbers:
  - 1
  - 2
  - 3
)";
    
    YAML::Node node = YAML::Load(yaml_str);
    
    EXPECT_TRUE(node["fruits"].IsSequence());
    EXPECT_EQ(node["fruits"].size(), 3u);
    EXPECT_EQ(node["fruits"][0].as<std::string>(), "apple");
    EXPECT_EQ(node["fruits"][1].as<std::string>(), "banana");
    EXPECT_EQ(node["fruits"][2].as<std::string>(), "orange");
    
    EXPECT_TRUE(node["numbers"].IsSequence());
    EXPECT_EQ(node["numbers"][0].as<int>(), 1);
    EXPECT_EQ(node["numbers"][2].as<int>(), 3);
}

// Test 4: Nested structures
TEST_F(YAMLLibAdvancedTest, NestedStructures) {
    std::string yaml_str = R"(
database:
  name: themisdb
  version: 1.3.0
  connection:
    host: localhost
    port: 8080
    ssl: true
)";
    
    YAML::Node node = YAML::Load(yaml_str);
    
    EXPECT_EQ(node["database"]["name"].as<std::string>(), "themisdb");
    EXPECT_EQ(node["database"]["version"].as<std::string>(), "1.3.0");
    EXPECT_EQ(node["database"]["connection"]["host"].as<std::string>(), "localhost");
    EXPECT_EQ(node["database"]["connection"]["port"].as<int>(), 8080);
    EXPECT_TRUE(node["database"]["connection"]["ssl"].as<bool>());
}

// Test 5: Complex nested arrays
TEST_F(YAMLLibAdvancedTest, ComplexNestedArrays) {
    std::string yaml_str = R"(
users:
  - name: Alice
    age: 30
    roles:
      - admin
      - developer
  - name: Bob
    age: 25
    roles:
      - developer
)";
    
    YAML::Node node = YAML::Load(yaml_str);
    
    EXPECT_EQ(node["users"].size(), 2u);
    EXPECT_EQ(node["users"][0]["name"].as<std::string>(), "Alice");
    EXPECT_EQ(node["users"][0]["age"].as<int>(), 30);
    EXPECT_EQ(node["users"][0]["roles"].size(), 2u);
    EXPECT_EQ(node["users"][0]["roles"][0].as<std::string>(), "admin");
    EXPECT_EQ(node["users"][1]["name"].as<std::string>(), "Bob");
}

// Test 6: YAML serialization (emission)
TEST_F(YAMLLibAdvancedTest, Serialization) {
    YAML::Node node;
    node["name"] = "ThemisDB";
    node["version"] = 1.3;
    node["features"] = YAML::Node(YAML::NodeType::Sequence);
    node["features"].push_back("vector");
    node["features"].push_back("graph");
    node["features"].push_back("document");
    
    YAML::Emitter emitter;
    emitter << node;
    
    std::string yaml_str = emitter.c_str();
    
    EXPECT_NE(yaml_str.find("name"), std::string::npos);
    EXPECT_NE(yaml_str.find("ThemisDB"), std::string::npos);
    EXPECT_NE(yaml_str.find("features"), std::string::npos);
}

// Test 7: File I/O
TEST_F(YAMLLibAdvancedTest, FileIO) {
    std::string yaml_content = R"(
database: themisdb
port: 8080
enabled: true
)";
    
    writeYAMLFile("config.yaml", yaml_content);
    
    YAML::Node node = YAML::LoadFile(test_yaml_dir_ + "/config.yaml");
    
    EXPECT_EQ(node["database"].as<std::string>(), "themisdb");
    EXPECT_EQ(node["port"].as<int>(), 8080);
    EXPECT_TRUE(node["enabled"].as<bool>());
}

// Test 8: Multiline strings
TEST_F(YAMLLibAdvancedTest, MultilineStrings) {
    std::string yaml_str = R"(
literal: |
  This is a literal
  multiline string
  with preserved newlines
folded: >
  This is a folded
  multiline string
  where newlines are folded
)";
    
    YAML::Node node = YAML::Load(yaml_str);
    
    std::string literal = node["literal"].as<std::string>();
    std::string folded = node["folded"].as<std::string>();
    
    EXPECT_NE(literal.find("literal\n"), std::string::npos);
    EXPECT_NE(literal.find("multiline string\n"), std::string::npos);
    
    // Folded strings have newlines converted to spaces
    EXPECT_NE(folded.find("This is a folded"), std::string::npos);
}

// Test 9: Anchors and aliases
TEST_F(YAMLLibAdvancedTest, AnchorsAndAliases) {
    std::string yaml_str = R"(
defaults: &defaults
  timeout: 30
  retries: 3

service1:
  <<: *defaults
  name: api

service2:
  <<: *defaults
  name: worker
  timeout: 60
)";
    
    YAML::Node node = YAML::Load(yaml_str);
    
    EXPECT_EQ(node["service1"]["name"].as<std::string>(), "api");
    EXPECT_EQ(node["service1"]["timeout"].as<int>(), 30);
    EXPECT_EQ(node["service1"]["retries"].as<int>(), 3);
    
    EXPECT_EQ(node["service2"]["name"].as<std::string>(), "worker");
    EXPECT_EQ(node["service2"]["timeout"].as<int>(), 60); // Overridden
    EXPECT_EQ(node["service2"]["retries"].as<int>(), 3);
}

// Test 10: Type checking and node types
TEST_F(YAMLLibAdvancedTest, TypeChecking) {
    std::string yaml_str = R"(
map_value:
  key: value
sequence_value:
  - item1
  - item2
scalar_value: text
null_value: null
)";
    
    YAML::Node node = YAML::Load(yaml_str);
    
    EXPECT_TRUE(node["map_value"].IsMap());
    EXPECT_FALSE(node["map_value"].IsSequence());
    EXPECT_FALSE(node["map_value"].IsScalar());
    
    EXPECT_TRUE(node["sequence_value"].IsSequence());
    EXPECT_FALSE(node["sequence_value"].IsMap());
    
    EXPECT_TRUE(node["scalar_value"].IsScalar());
    EXPECT_FALSE(node["scalar_value"].IsMap());
    
    EXPECT_TRUE(node["null_value"].IsNull());
}

// Test 11: Error handling - invalid YAML
TEST_F(YAMLLibAdvancedTest, ErrorHandlingInvalidYAML) {
    std::string invalid_yaml = R"(
key: value
  invalid indentation
another_key: value
)";
    
    EXPECT_THROW(
        YAML::Load(invalid_yaml),
        YAML::ParserException
    );
}

// Test 12: Missing keys
TEST_F(YAMLLibAdvancedTest, MissingKeys) {
    std::string yaml_str = "key1: value1\nkey2: value2";
    YAML::Node node = YAML::Load(yaml_str);
    
    EXPECT_TRUE(node["key1"]);
    EXPECT_TRUE(node["key2"]);
    EXPECT_FALSE(node["non_existent"]);
    EXPECT_TRUE(node["non_existent"].IsNull());
}

// Test 13: ThemisDB configuration structure
TEST_F(YAMLLibAdvancedTest, ThemisDBConfiguration) {
    std::string yaml_str = R"(
server:
  host: 0.0.0.0
  port: 8080
  threads: 4
  
storage:
  path: /data/themisdb
  cache_size_mb: 512
  compression: zstd
  
security:
  tls_enabled: true
  tls_cert: /etc/themis/cert.pem
  tls_key: /etc/themis/key.pem
  
logging:
  level: info
  file: /var/log/themisdb.log
  rotate_size_mb: 100
)";
    
    YAML::Node config = YAML::Load(yaml_str);
    
    EXPECT_EQ(config["server"]["host"].as<std::string>(), "0.0.0.0");
    EXPECT_EQ(config["server"]["port"].as<int>(), 8080);
    EXPECT_EQ(config["storage"]["path"].as<std::string>(), "/data/themisdb");
    EXPECT_EQ(config["storage"]["cache_size_mb"].as<int>(), 512);
    EXPECT_TRUE(config["security"]["tls_enabled"].as<bool>());
    EXPECT_EQ(config["logging"]["level"].as<std::string>(), "info");
}

// Test 14: Policy configuration (from test_policy_yaml.cpp)
TEST_F(YAMLLibAdvancedTest, PolicyConfiguration) {
    std::string yaml_str = R"(
- id: allow-metrics-readonly
  name: readonly can access metrics
  subjects: ["readonly"]
  actions: ["metrics.read"]
  resources: ["/metrics"]
  effect: allow

- id: hr-allow-internal-read
  name: HR data read only from internal network
  subjects: ["*"]
  actions: ["read"]
  resources: ["/entities/hr:"]
  allowed_ip_prefixes: ["10.", "192.168."]
  effect: allow

- id: hr-deny-external-read
  name: HR data read denied externally
  subjects: ["*"]
  actions: ["read"]
  resources: ["/entities/hr:"]
  effect: deny
)";
    
    YAML::Node policies = YAML::Load(yaml_str);
    
    EXPECT_TRUE(policies.IsSequence());
    EXPECT_EQ(policies.size(), 3u);
    
    EXPECT_EQ(policies[0]["id"].as<std::string>(), "allow-metrics-readonly");
    EXPECT_EQ(policies[0]["effect"].as<std::string>(), "allow");
    EXPECT_EQ(policies[0]["subjects"].size(), 1u);
    
    EXPECT_EQ(policies[1]["allowed_ip_prefixes"].size(), 2u);
    EXPECT_EQ(policies[1]["allowed_ip_prefixes"][0].as<std::string>(), "10.");
}

// Test 15: Iteration over maps
TEST_F(YAMLLibAdvancedTest, MapIteration) {
    std::string yaml_str = R"(
features:
  vector: enabled
  graph: enabled
  document: enabled
  timeseries: disabled
)";
    
    YAML::Node node = YAML::Load(yaml_str);
    YAML::Node features = node["features"];
    
    std::map<std::string, std::string> feature_map = {};

    for (YAML::const_iterator it = features.begin(); it != features.end(); ++it) {
        std::string key = it->first.as<std::string>();
        std::string value = it->second.as<std::string>();
        feature_map[key] = value;
    }
    
    EXPECT_EQ(feature_map.size(), 4u);
    EXPECT_EQ(feature_map["vector"], "enabled");
    EXPECT_EQ(feature_map["timeseries"], "disabled");
}

// Test 16: Iteration over sequences
TEST_F(YAMLLibAdvancedTest, SequenceIteration) {
    std::string yaml_str = R"(
databases:
  - name: db1
    size: 100
  - name: db2
    size: 200
  - name: db3
    size: 300
)";
    
    YAML::Node node = YAML::Load(yaml_str);
    YAML::Node databases = node["databases"];
    
    int total_size = 0;
    for (size_t i = 0; i < databases.size(); ++i) {
        total_size += databases[i]["size"].as<int>();
    }
    
    EXPECT_EQ(total_size, 600);
}

// Test 17: Deep cloning
TEST_F(YAMLLibAdvancedTest, DeepCloning) {
    std::string yaml_str = R"(
original:
  data: value
  nested:
    key: original_value
)";
    
    YAML::Node original = YAML::Load(yaml_str);
    YAML::Node clone = YAML::Clone(original);
    
    // Modify clone
    clone["original"]["nested"]["key"] = "modified_value";
    
    // Original should be unchanged
    EXPECT_EQ(original["original"]["nested"]["key"].as<std::string>(), "original_value");
    EXPECT_EQ(clone["original"]["nested"]["key"].as<std::string>(), "modified_value");
}

// Test 18: Custom tags (advanced)
TEST_F(YAMLLibAdvancedTest, CustomTags) {
    std::string yaml_str = R"(
binary_data: !!binary |
  R0lGODlhDAAMAIQAAP//9/X17unp5WZmZgAAAOfn515eXvPz7Y6OjuDg4J+fn5
  OTk6enp56enmlpaWNjY6Ojo4SEhP/++f/++f/++f/++f/++f/++f/++f/++f/+
  +f/++f/++f/++f/++f/++SH+Dk1hZGUgd2l0aCBHSU1QACwAAAAADAAMAAAFLC
  AgjoEwnuNAFOhpEMTRiggcz4BNJHrv/zCFcLiwMWYNG84BwwEeECcgggoBADs=
)";
    
    YAML::Node node = YAML::Load(yaml_str);
    
    // yaml-cpp recognizes !!binary tag
    EXPECT_TRUE(node["binary_data"]);
}

// Test 19: Default values
TEST_F(YAMLLibAdvancedTest, DefaultValues) {
    std::string yaml_str = R"(
configured_value: 42
)";
    
    YAML::Node node = YAML::Load(yaml_str);
    
    // Existing key
    int value1 = node["configured_value"].as<int>(100);
    EXPECT_EQ(value1, 42);
    
    // Non-existent key with default
    int value2 = node["non_existent"].as<int>(100);
    EXPECT_EQ(value2, 100);
    
    std::string value3 = node["missing_string"].as<std::string>("default");
    EXPECT_EQ(value3, "default");
}

// Test 20: Complex schema validation scenario
TEST_F(YAMLLibAdvancedTest, ComplexSchemaValidation) {
    std::string yaml_str = R"(
schema_version: 2.0
collections:
  users:
    fields:
      - name: id
        type: string
        required: true
        primary_key: true
      - name: email
        type: string
        required: true
        indexed: true
      - name: age
        type: integer
        required: false
    indexes:
      - name: email_idx
        fields: [email]
        unique: true
      - name: age_idx
        fields: [age]
        unique: false
)";
    
    YAML::Node schema = YAML::Load(yaml_str);
    
    EXPECT_DOUBLE_EQ(schema["schema_version"].as<double>(), 2.0);
    
    YAML::Node users = schema["collections"]["users"];
    EXPECT_TRUE(users.IsMap());
    
    YAML::Node fields = users["fields"];
    EXPECT_EQ(fields.size(), 3u);
    
    // Validate first field
    EXPECT_EQ(fields[0]["name"].as<std::string>(), "id");
    EXPECT_EQ(fields[0]["type"].as<std::string>(), "string");
    EXPECT_TRUE(fields[0]["required"].as<bool>());
    EXPECT_TRUE(fields[0]["primary_key"].as<bool>());
    
    // Validate indexes
    YAML::Node indexes = users["indexes"];
    EXPECT_EQ(indexes.size(), 2u);
    EXPECT_TRUE(indexes[0]["unique"].as<bool>());
    EXPECT_FALSE(indexes[1]["unique"].as<bool>());
}
