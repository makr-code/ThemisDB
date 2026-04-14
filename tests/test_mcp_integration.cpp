/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_mcp_integration.cpp                           ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:16:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     623                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// MCP (Model Context Protocol) Integration Tests
// These tests validate the complete MCP integration including index management,
// schema discovery, statistics, and tool execution

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_MCP

#include "server/mcp_server.h"
#include "storage/rocksdb_wrapper.h"
#include "metadata/schema_manager.h"
#include "index/secondary_index.h"
#include <nlohmann/json.hpp>
#include <string>
#include <filesystem>
#include <boost/asio.hpp>

using namespace themis::server;
using json = nlohmann::json;

class MCPIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary test database
        test_db_path_ = "/tmp/test_mcp_integration_db_" + std::to_string(time(nullptr));
        
        // Create database
        db_ = std::make_shared<themis::RocksDBWrapper>(test_db_path_);
        ASSERT_TRUE(db_->open());
        
        // Create MCP server
        McpServer::Config config;
        config.enable_stdio = false;  // Disable for tests
        config.enable_sse = false;
        config.enable_websocket = false;
        
        server_ = std::make_shared<McpServer>(io_context_, config);
        server_->attachDatabase(db_);
        server_->start();
    }
    
    void TearDown() override {
        if (server_) {
            server_->stop();
        }
        
        if (db_) {
            db_->close();
        }
        
        // Clean up test database
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    json callTool(const std::string& tool_name, const json& arguments) {
        json request = {
            {"jsonrpc", "2.0"},
            {"method", "tools/call"},
            {"params", {
                {"name", tool_name},
                {"arguments", arguments}
            }},
            {"id", 1}
        };
        
        return server_->handleRequest(request);
    }
    
protected:
    boost::asio::io_context io_context_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<McpServer> server_;
    std::string test_db_path_;
};

// ============================================================================
// Basic MCP Server Tests
// ============================================================================

TEST_F(MCPIntegrationTest, ServerInitialization) {
    // Test initialize method
    json init_request = {
        {"jsonrpc", "2.0"},
        {"method", "initialize"},
        {"params", {
            {"protocolVersion", "2024-11-05"},
            {"capabilities", json::object()},
            {"clientInfo", {
                {"name", "test-client"},
                {"version", "1.0.0"}
            }}
        }},
        {"id", 1}
    };
    
    json response = server_->handleRequest(init_request);
    
    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_EQ(response["id"], 1);
    EXPECT_TRUE(response.contains("result"));
    EXPECT_EQ(response["result"]["protocolVersion"], "2024-11-05");
    EXPECT_TRUE(response["result"].contains("serverInfo"));
    EXPECT_TRUE(response["result"].contains("capabilities"));
}

TEST_F(MCPIntegrationTest, ToolsList) {
    // Test tools/list method
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "tools/list"},
        {"id", 2}
    };
    
    json response = server_->handleRequest(request);
    
    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_TRUE(response.contains("result"));
    EXPECT_TRUE(response["result"]["tools"].is_array());
    
    // Verify essential tools are registered
    auto tools = response["result"]["tools"];
    bool has_create_index = false;
    bool has_drop_index = false;
    bool has_list_indexes = false;
    bool has_get_schema = false;
    bool has_get_stats = false;
    
    for (const auto& tool : tools) {
        std::string name = tool["name"];
        if (name == "create_index") has_create_index = true;
        if (name == "drop_index") has_drop_index = true;
        if (name == "list_indexes") has_list_indexes = true;
        if (name == "get_schema") has_get_schema = true;
        if (name == "get_stats") has_get_stats = true;
    }
    
    EXPECT_TRUE(has_create_index) << "create_index tool not found";
    EXPECT_TRUE(has_drop_index) << "drop_index tool not found";
    EXPECT_TRUE(has_list_indexes) << "list_indexes tool not found";
    EXPECT_TRUE(has_get_schema) << "get_schema tool not found";
    EXPECT_TRUE(has_get_stats) << "get_stats tool not found";
}

// ============================================================================
// Entity Operations Tests
// ============================================================================

TEST_F(MCPIntegrationTest, EntityPutAndGet) {
    // Put an entity
    json put_result = callTool("put_entity", {
        {"key", "test:user:1"},
        {"value", {
            {"name", "Alice"},
            {"age", 30},
            {"email", "alice@example.com"}
        }}
    });
    
    // Verify put was successful
    std::string put_text = put_result["result"]["content"][0]["text"];
    EXPECT_NE(put_text.find("\"status\":\"success\""), std::string::npos) 
        << "Put operation should return success status";
    
    // Get the entity
    json get_result = callTool("get_entity", {
        {"key", "test:user:1"}
    });
    
    std::string result_text = get_result["result"]["content"][0]["text"];
    json result = json::parse(result_text);
    
    EXPECT_EQ(result["status"], "success");
    EXPECT_TRUE(result.contains("value"));
    EXPECT_EQ(result["value"]["name"], "Alice");
    EXPECT_EQ(result["value"]["age"], 30);
}

TEST_F(MCPIntegrationTest, EntityDelete) {
    // Put an entity
    callTool("put_entity", {
        {"key", "test:user:2"},
        {"value", {{"name", "Bob"}}}
    });
    
    // Delete it
    json delete_result = callTool("delete_entity", {
        {"key", "test:user:2"}
    });
    
    std::string result_text = delete_result["result"]["content"][0]["text"];
    json result = json::parse(result_text);
    EXPECT_EQ(result["status"], "success");
    
    // Verify it's gone
    json get_result = callTool("get_entity", {
        {"key", "test:user:2"}
    });
    
    result_text = get_result["result"]["content"][0]["text"];
    result = json::parse(result_text);
    EXPECT_EQ(result["status"], "error");
}

// ============================================================================
// Index Management Tests
// ============================================================================

TEST_F(MCPIntegrationTest, CreateRegularIndex) {
    json result = callTool("create_index", {
        {"table", "users"},
        {"column", "email"},
        {"type", "regular"},
        {"unique", true}
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_EQ(parsed["table"], "users");
    EXPECT_EQ(parsed["column"], "email");
    EXPECT_EQ(parsed["index_type"], "regular");
    EXPECT_EQ(parsed["unique"], true);
}

TEST_F(MCPIntegrationTest, CreateRangeIndex) {
    json result = callTool("create_index", {
        {"table", "products"},
        {"column", "price"},
        {"type", "range"}
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_EQ(parsed["index_type"], "range");
}

TEST_F(MCPIntegrationTest, CreateFulltextIndex) {
    json result = callTool("create_index", {
        {"table", "articles"},
        {"column", "content"},
        {"type", "fulltext"},
        {"fulltext_config", {
            {"stemming", true},
            {"language", "en"},
            {"stopwords", true}
        }}
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_EQ(parsed["index_type"], "fulltext");
}

TEST_F(MCPIntegrationTest, CreateGeoIndex) {
    json result = callTool("create_index", {
        {"table", "locations"},
        {"column", "coordinates"},
        {"type", "geo"}
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_EQ(parsed["index_type"], "geo");
}

TEST_F(MCPIntegrationTest, CreateTTLIndex) {
    json result = callTool("create_index", {
        {"table", "sessions"},
        {"column", "expires_at"},
        {"type", "ttl"},
        {"ttl_seconds", 3600}  // 1 hour
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_EQ(parsed["index_type"], "ttl");
}

TEST_F(MCPIntegrationTest, CreateSparseIndex) {
    json result = callTool("create_index", {
        {"table", "profiles"},
        {"column", "optional_field"},
        {"type", "sparse"}
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_EQ(parsed["index_type"], "sparse");
}

TEST_F(MCPIntegrationTest, DropIndex) {
    // Create an index first
    callTool("create_index", {
        {"table", "test_table"},
        {"column", "test_column"},
        {"type", "regular"}
    });
    
    // Drop it
    json result = callTool("drop_index", {
        {"table", "test_table"},
        {"column", "test_column"},
        {"type", "regular"}
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_EQ(parsed["table"], "test_table");
    EXPECT_EQ(parsed["column"], "test_column");
}

TEST_F(MCPIntegrationTest, ListIndexes) {
    // Create a few indexes
    callTool("create_index", {
        {"table", "users"},
        {"column", "username"},
        {"type", "regular"}
    });
    
    callTool("create_index", {
        {"table", "users"},
        {"column", "created_at"},
        {"type", "range"}
    });
    
    // List all indexes
    json result = callTool("list_indexes", json::object());
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_TRUE(parsed.contains("indexes"));
    EXPECT_TRUE(parsed["indexes"].is_array());
    EXPECT_TRUE(parsed.contains("total_count"));
}

TEST_F(MCPIntegrationTest, CreateIndexMissingParameters) {
    // Try to create index without required parameters
    json result = callTool("create_index", {
        {"table", "test"}
        // Missing "column" parameter
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "error");
    EXPECT_NE(parsed["message"].get<std::string>().find("Missing"), std::string::npos)
        << "Error message should indicate missing parameter";
    EXPECT_NE(parsed["message"].get<std::string>().find("column"), std::string::npos)
        << "Error message should specify that 'column' parameter is missing";
}

TEST_F(MCPIntegrationTest, CreateIndexUnsupportedType) {
    json result = callTool("create_index", {
        {"table", "test"},
        {"column", "test"},
        {"type", "invalid_type"}
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "error");
    EXPECT_NE(parsed["message"].get<std::string>().find("Unsupported"), std::string::npos)
        << "Error message should indicate unsupported index type";
    EXPECT_NE(parsed["message"].get<std::string>().find("invalid_type"), std::string::npos)
        << "Error message should specify which type was invalid";
}

// ============================================================================
// Query Tool Tests (AQL Integration)
// ============================================================================

TEST_F(MCPIntegrationTest, QueryToolAQLSimple) {
    // First, put some test data
    callTool("put_entity", {
        {"key", "users:1"},
        {"value", {
            {"name", "Alice"},
            {"age", 30},
            {"city", "Berlin"}
        }}
    });
    
    callTool("put_entity", {
        {"key", "users:2"},
        {"value", {
            {"name", "Bob"},
            {"age", 25},
            {"city", "Munich"}
        }}
    });
    
    // Execute simple AQL query
    json result = callTool("query", {
        {"query", "FOR user IN users RETURN user"},
        {"language", "aql"}
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_EQ(parsed["language"], "aql");
    EXPECT_TRUE(parsed.contains("results"));
}

TEST_F(MCPIntegrationTest, QueryToolAQLWithFilter) {
    // Put test data
    callTool("put_entity", {
        {"key", "products:1"},
        {"value", {
            {"name", "Laptop"},
            {"price", 1200.0},
            {"category", "electronics"}
        }}
    });
    
    callTool("put_entity", {
        {"key", "products:2"},
        {"value", {
            {"name", "Book"},
            {"price", 15.0},
            {"category", "books"}
        }}
    });
    
    // Execute AQL query with filter
    json result = callTool("query", {
        {"query", "FOR product IN products FILTER product.price > 100 RETURN product"},
        {"language", "aql"}
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_EQ(parsed["language"], "aql");
}

TEST_F(MCPIntegrationTest, QueryToolAutoDetectAQL) {
    // Test automatic language detection for AQL
    json result = callTool("query", {
        {"query", "FOR doc IN collection RETURN doc"},
        {"language", "auto"}
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    // Should detect as AQL and attempt execution
    EXPECT_EQ(parsed["language"], "aql");
}

TEST_F(MCPIntegrationTest, QueryToolUnsupportedLanguage) {
    // Test SQL (not yet implemented)
    json result = callTool("query", {
        {"query", "SELECT * FROM users"},
        {"language", "sql"}
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "error");
    EXPECT_NE(parsed["message"].get<std::string>().find("not yet implemented"), std::string::npos);
}

TEST_F(MCPIntegrationTest, QueryToolInvalidAQL) {
    // Test invalid AQL query
    json result = callTool("query", {
        {"query", "INVALID QUERY SYNTAX"},
        {"language", "aql"}
    });
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "error");
    EXPECT_TRUE(parsed.contains("message"));
}

// ============================================================================
// Schema Discovery Tests
// ============================================================================

TEST_F(MCPIntegrationTest, GetSchema) {
    json result = callTool("get_schema", json::object());
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    // Should have integration_level = "full" since SchemaManager is attached
    EXPECT_TRUE(parsed.contains("integration_level"));
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(MCPIntegrationTest, GetStats) {
    json result = callTool("get_stats", json::object());
    
    std::string result_text = result["result"]["content"][0]["text"];
    json parsed = json::parse(result_text);
    
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_TRUE(parsed.contains("database_connected"));
    EXPECT_TRUE(parsed["database_connected"]);
}

// ============================================================================
// Resource Tests
// ============================================================================

TEST_F(MCPIntegrationTest, ResourcesList) {
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "resources/list"},
        {"id", 3}
    };
    
    json response = server_->handleRequest(request);
    
    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_TRUE(response.contains("result"));
    EXPECT_TRUE(response["result"]["resources"].is_array());
}

TEST_F(MCPIntegrationTest, ResourceRead) {
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "resources/read"},
        {"params", {
            {"uri", "schema://database"}
        }},
        {"id", 4}
    };
    
    json response = server_->handleRequest(request);
    
    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_TRUE(response.contains("result"));
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(MCPIntegrationTest, InvalidMethod) {
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "invalid/method"},
        {"id", 99}
    };
    
    json response = server_->handleRequest(request);
    
    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_TRUE(response.contains("error"));
    EXPECT_EQ(response["error"]["code"], -32601);  // Method not found
}

TEST_F(MCPIntegrationTest, MissingMethod) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 100}
    };
    
    json response = server_->handleRequest(request);
    
    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_TRUE(response.contains("error"));
    EXPECT_EQ(response["error"]["code"], -32600);  // Invalid Request
}

TEST_F(MCPIntegrationTest, ToolNotFound) {
    json result = callTool("nonexistent_tool", json::object());
    
    EXPECT_TRUE(result.contains("error"));
    EXPECT_EQ(result["error"]["code"], -32602);  // Invalid params (tool not found)
}

#endif // THEMIS_ENABLE_MCP
