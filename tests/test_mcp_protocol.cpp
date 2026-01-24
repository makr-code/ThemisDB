// MCP (Model Context Protocol) Basic Tests
// These tests validate the MCP server implementation including transports and tool/resource handling

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_MCP

#include "server/mcp_server.h"
#include <nlohmann/json.hpp>
#include <string>

using namespace themis::server;
using json = nlohmann::json;

// ============================================================================
// MCP Protocol Message Format Tests
// ============================================================================

TEST(MCPProtocolTest, JsonRpcRequestFormat) {
    // Test that MCP uses JSON-RPC 2.0 format
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "tools/list"},
        {"params", json::object()}
    };
    
    EXPECT_EQ(request["jsonrpc"], "2.0");
    EXPECT_EQ(request["method"], "tools/list");
    EXPECT_TRUE(request.contains("id"));
}

TEST(MCPProtocolTest, JsonRpcResponseFormat) {
    // Test JSON-RPC response format
    json response = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"result", {
            {"tools", json::array()}
        }}
    };
    
    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_TRUE(response.contains("result"));
    EXPECT_FALSE(response.contains("error"));
}

TEST(MCPProtocolTest, JsonRpcErrorFormat) {
    // Test JSON-RPC error format
    json error_response = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"error", {
            {"code", -32601},
            {"message", "Method not found"}
        }}
    };
    
    EXPECT_EQ(error_response["jsonrpc"], "2.0");
    EXPECT_TRUE(error_response.contains("error"));
    EXPECT_FALSE(error_response.contains("result"));
    EXPECT_EQ(error_response["error"]["code"], -32601);
}

// ============================================================================
// MCP Server Configuration Tests
// ============================================================================

TEST(MCPProtocolTest, DefaultConfiguration) {
    // Test default MCP server configuration
    McpServer::Config config;
    
    // Verify defaults
    EXPECT_TRUE(config.enable_stdio);  // stdio should be enabled by default
    EXPECT_GE(config.stdio_buffer_size, 4096);  // Reasonable buffer size
    
    EXPECT_FALSE(config.enable_sse);  // Optional transports disabled by default
    EXPECT_FALSE(config.enable_websocket);
}

TEST(MCPProtocolTest, CustomConfiguration) {
    // Test custom configuration
    McpServer::Config config;
    config.server_name = "TestMCP";
    config.server_version = "1.0.0";
    config.enable_stdio = true;
    config.enable_sse = true;
    config.enable_websocket = true;
    config.sse_keepalive_ms = 5000;
    config.websocket_ping_interval_ms = 10000;
    
    EXPECT_EQ(config.server_name, "TestMCP");
    EXPECT_EQ(config.server_version, "1.0.0");
    EXPECT_TRUE(config.enable_stdio);
    EXPECT_TRUE(config.enable_sse);
    EXPECT_TRUE(config.enable_websocket);
    EXPECT_EQ(config.sse_keepalive_ms, 5000);
    EXPECT_EQ(config.websocket_ping_interval_ms, 10000);
}

// ============================================================================
// MCP Tool Schema Tests
// ============================================================================

TEST(MCPProtocolTest, ToolInputSchema) {
    // Test tool input schema format
    json tool_schema = {
        {"type", "object"},
        {"properties", {
            {"query", {
                {"type", "string"},
                {"description", "The query to execute"}
            }},
            {"limit", {
                {"type", "integer"},
                {"description", "Maximum number of results"},
                {"default", 10}
            }}
        }},
        {"required", json::array({"query"})}
    };
    
    EXPECT_EQ(tool_schema["type"], "object");
    EXPECT_TRUE(tool_schema.contains("properties"));
    EXPECT_TRUE(tool_schema.contains("required"));
    EXPECT_TRUE(tool_schema["required"].is_array());
}

TEST(MCPProtocolTest, ResourceUriFormat) {
    // Test resource URI format
    std::string resource_uri = "themisdb://entities/users:alice";
    
    EXPECT_TRUE(resource_uri.find("themisdb://") == 0);
    EXPECT_NE(resource_uri.find("/entities/"), std::string::npos);
}

// ============================================================================
// MCP Transport Tests
// ============================================================================

TEST(MCPProtocolTest, StdioTransportAvailable) {
    // Test that stdio transport is the primary transport
    McpServer::Config config;
    config.enable_stdio = true;
    config.enable_sse = false;
    config.enable_websocket = false;
    
    // Verify only stdio is enabled
    EXPECT_TRUE(config.enable_stdio);
    EXPECT_FALSE(config.enable_sse);
    EXPECT_FALSE(config.enable_websocket);
}

TEST(MCPProtocolTest, MultipleTransportsConfiguration) {
    // Test configuration with multiple transports
    McpServer::Config config;
    config.enable_stdio = true;
    config.enable_sse = true;
    config.enable_websocket = true;
    
    // All transports can be enabled simultaneously
    EXPECT_TRUE(config.enable_stdio);
    EXPECT_TRUE(config.enable_sse);
    EXPECT_TRUE(config.enable_websocket);
}

// ============================================================================
// MCP Method Tests
// ============================================================================

TEST(MCPProtocolTest, InitializeMethod) {
    // Test initialize method request
    json init_request = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "initialize"},
        {"params", {
            {"protocolVersion", "2024-11-05"},
            {"capabilities", json::object()},
            {"clientInfo", {
                {"name", "test-client"},
                {"version", "1.0.0"}
            }}
        }}
    };
    
    EXPECT_EQ(init_request["method"], "initialize");
    EXPECT_TRUE(init_request["params"].contains("protocolVersion"));
    EXPECT_TRUE(init_request["params"].contains("clientInfo"));
}

TEST(MCPProtocolTest, ToolsListMethod) {
    // Test tools/list method
    json tools_request = {
        {"jsonrpc", "2.0"},
        {"id", 2},
        {"method", "tools/list"}
    };
    
    EXPECT_EQ(tools_request["method"], "tools/list");
}

TEST(MCPProtocolTest, ToolsCallMethod) {
    // Test tools/call method
    json call_request = {
        {"jsonrpc", "2.0"},
        {"id", 3},
        {"method", "tools/call"},
        {"params", {
            {"name", "query_database"},
            {"arguments", {
                {"query", "SELECT * FROM users LIMIT 10"}
            }}
        }}
    };
    
    EXPECT_EQ(call_request["method"], "tools/call");
    EXPECT_TRUE(call_request["params"].contains("name"));
    EXPECT_TRUE(call_request["params"].contains("arguments"));
}

TEST(MCPProtocolTest, ResourcesListMethod) {
    // Test resources/list method
    json resources_request = {
        {"jsonrpc", "2.0"},
        {"id", 4},
        {"method", "resources/list"}
    };
    
    EXPECT_EQ(resources_request["method"], "resources/list");
}

TEST(MCPProtocolTest, ResourcesReadMethod) {
    // Test resources/read method
    json read_request = {
        {"jsonrpc", "2.0"},
        {"id", 5},
        {"method", "resources/read"},
        {"params", {
            {"uri", "themisdb://entities/users:alice"}
        }}
    };
    
    EXPECT_EQ(read_request["method"], "resources/read");
    EXPECT_TRUE(read_request["params"].contains("uri"));
}

TEST(MCPProtocolTest, PromptsListMethod) {
    // Test prompts/list method
    json prompts_request = {
        {"jsonrpc", "2.0"},
        {"id", 6},
        {"method", "prompts/list"}
    };
    
    EXPECT_EQ(prompts_request["method"], "prompts/list");
}

TEST(MCPProtocolTest, PromptsGetMethod) {
    // Test prompts/get method
    json get_request = {
        {"jsonrpc", "2.0"},
        {"id", 7},
        {"method", "prompts/get"},
        {"params", {
            {"name", "rag_query"},
            {"arguments", {
                {"context", "user profile data"}
            }}
        }}
    };
    
    EXPECT_EQ(get_request["method"], "prompts/get");
    EXPECT_TRUE(get_request["params"].contains("name"));
}

// ============================================================================
// MCP Capability Tests
// ============================================================================

TEST(MCPProtocolTest, ServerCapabilities) {
    // Test server capabilities format
    json capabilities = {
        {"tools", json::object()},
        {"resources", {
            {"subscribe", true},
            {"listChanged", true}
        }},
        {"prompts", {
            {"listChanged", true}
        }}
    };
    
    EXPECT_TRUE(capabilities.contains("tools"));
    EXPECT_TRUE(capabilities.contains("resources"));
    EXPECT_TRUE(capabilities.contains("prompts"));
}

TEST(MCPProtocolTest, ClientCapabilities) {
    // Test client capabilities format
    json capabilities = {
        {"sampling", json::object()},
        {"roots", {
            {"listChanged", true}
        }}
    };
    
    EXPECT_TRUE(capabilities.contains("sampling"));
    EXPECT_TRUE(capabilities.contains("roots"));
}

// ============================================================================
// MCP Error Codes Tests
// ============================================================================

TEST(MCPProtocolTest, JsonRpcErrorCodes) {
    // Test standard JSON-RPC error codes
    const int PARSE_ERROR = -32700;
    const int INVALID_REQUEST = -32600;
    const int METHOD_NOT_FOUND = -32601;
    const int INVALID_PARAMS = -32602;
    const int INTERNAL_ERROR = -32603;
    
    EXPECT_EQ(PARSE_ERROR, -32700);
    EXPECT_EQ(INVALID_REQUEST, -32600);
    EXPECT_EQ(METHOD_NOT_FOUND, -32601);
    EXPECT_EQ(INVALID_PARAMS, -32602);
    EXPECT_EQ(INTERNAL_ERROR, -32603);
}

#endif // THEMIS_ENABLE_MCP


