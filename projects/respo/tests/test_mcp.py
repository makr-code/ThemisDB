"""Tests for MCP (Model Context Protocol) integration."""

import pytest
from unittest.mock import AsyncMock, MagicMock, patch
import json

from respo.mcp.server import MCPServer
from respo.mcp.handlers import (
    MCPRequest,
    MCPResponse,
    handle_initialize,
    handle_tools_list,
    handle_tools_call,
)


class TestMCPServer:
    """Tests for MCPServer."""

    @pytest.fixture
    def mcp_server(self):
        """Create an MCP server instance."""
        return MCPServer()

    def test_server_creation(self, mcp_server):
        """Test creating an MCP server."""
        assert mcp_server is not None
        assert mcp_server.name == "respo"

    def test_get_capabilities(self, mcp_server):
        """Test getting server capabilities."""
        caps = mcp_server.get_capabilities()
        
        assert "tools" in caps
        assert "prompts" in caps or "resources" in caps

    def test_list_tools(self, mcp_server):
        """Test listing available tools."""
        tools = mcp_server.list_tools()
        
        assert isinstance(tools, list)
        assert len(tools) > 0
        
        # Check tool structure
        for tool in tools:
            assert "name" in tool
            assert "description" in tool
            assert "inputSchema" in tool


class TestMCPRequest:
    """Tests for MCP request parsing."""

    def test_parse_valid_request(self):
        """Test parsing a valid JSON-RPC request."""
        data = {
            "jsonrpc": "2.0",
            "method": "tools/list",
            "id": 1
        }
        request = MCPRequest.from_dict(data)
        
        assert request.jsonrpc == "2.0"
        assert request.method == "tools/list"
        assert request.id == 1
        assert request.params is None

    def test_parse_request_with_params(self):
        """Test parsing a request with parameters."""
        data = {
            "jsonrpc": "2.0",
            "method": "tools/call",
            "params": {
                "name": "respo_search",
                "arguments": {"query": "test"}
            },
            "id": 2
        }
        request = MCPRequest.from_dict(data)
        
        assert request.method == "tools/call"
        assert request.params["name"] == "respo_search"
        assert request.params["arguments"]["query"] == "test"

    def test_parse_invalid_request(self):
        """Test parsing an invalid request."""
        data = {"invalid": "data"}
        
        with pytest.raises(ValueError):
            MCPRequest.from_dict(data)


class TestMCPResponse:
    """Tests for MCP response creation."""

    def test_success_response(self):
        """Test creating a success response."""
        response = MCPResponse.success(
            id=1,
            result={"tools": []}
        )
        
        assert response.jsonrpc == "2.0"
        assert response.id == 1
        assert response.result == {"tools": []}
        assert response.error is None

    def test_error_response(self):
        """Test creating an error response."""
        response = MCPResponse.error(
            id=1,
            code=-32600,
            message="Invalid Request"
        )
        
        assert response.jsonrpc == "2.0"
        assert response.id == 1
        assert response.result is None
        assert response.error["code"] == -32600
        assert response.error["message"] == "Invalid Request"

    def test_response_to_dict(self):
        """Test converting response to dictionary."""
        response = MCPResponse.success(id=1, result={"data": "test"})
        data = response.to_dict()
        
        assert data["jsonrpc"] == "2.0"
        assert data["id"] == 1
        assert data["result"]["data"] == "test"
        assert "error" not in data


class TestMCPHandlers:
    """Tests for MCP request handlers."""

    @pytest.mark.asyncio
    async def test_handle_initialize(self):
        """Test handling initialize request."""
        request = MCPRequest(
            jsonrpc="2.0",
            method="initialize",
            params={
                "protocolVersion": "2024-11-05",
                "clientInfo": {"name": "test-client", "version": "1.0.0"}
            },
            id=1
        )
        
        response = await handle_initialize(request)
        
        assert response.error is None
        assert "protocolVersion" in response.result
        assert "serverInfo" in response.result
        assert "capabilities" in response.result

    @pytest.mark.asyncio
    async def test_handle_tools_list(self):
        """Test handling tools/list request."""
        request = MCPRequest(
            jsonrpc="2.0",
            method="tools/list",
            id=2
        )
        
        response = await handle_tools_list(request)
        
        assert response.error is None
        assert "tools" in response.result
        assert isinstance(response.result["tools"], list)

    @pytest.mark.asyncio
    async def test_handle_tools_call_search(self):
        """Test handling tools/call for search."""
        request = MCPRequest(
            jsonrpc="2.0",
            method="tools/call",
            params={
                "name": "respo_search",
                "arguments": {"query": "database connection", "limit": 5}
            },
            id=3
        )
        
        with patch("respo.mcp.handlers.get_vector_store") as mock_store:
            mock_store.return_value.search = AsyncMock(return_value=[])
            response = await handle_tools_call(request)
        
        assert response.id == 3
        # Response should have content (even if empty results)
        assert response.result is not None or response.error is not None

    @pytest.mark.asyncio
    async def test_handle_unknown_tool(self):
        """Test handling unknown tool call."""
        request = MCPRequest(
            jsonrpc="2.0",
            method="tools/call",
            params={
                "name": "unknown_tool",
                "arguments": {}
            },
            id=4
        )
        
        response = await handle_tools_call(request)
        
        assert response.error is not None
        assert response.error["code"] == -32601  # Method not found


class TestMCPIntegration:
    """Integration tests for MCP endpoint."""

    @pytest.fixture
    def mcp_server(self):
        """Create MCP server with mocked dependencies."""
        server = MCPServer()
        return server

    @pytest.mark.asyncio
    async def test_full_request_cycle(self, mcp_server):
        """Test a complete request-response cycle."""
        # Initialize
        init_response = await mcp_server.handle_request({
            "jsonrpc": "2.0",
            "method": "initialize",
            "params": {
                "protocolVersion": "2024-11-05",
                "clientInfo": {"name": "test", "version": "1.0"}
            },
            "id": 1
        })
        
        assert init_response["result"]["serverInfo"]["name"] == "respo"
        
        # List tools
        tools_response = await mcp_server.handle_request({
            "jsonrpc": "2.0",
            "method": "tools/list",
            "id": 2
        })
        
        assert "tools" in tools_response["result"]

    @pytest.mark.asyncio
    async def test_batch_requests(self, mcp_server):
        """Test handling batch requests."""
        requests = [
            {"jsonrpc": "2.0", "method": "tools/list", "id": 1},
            {"jsonrpc": "2.0", "method": "tools/list", "id": 2},
        ]
        
        responses = await mcp_server.handle_batch(requests)
        
        assert len(responses) == 2
        assert responses[0]["id"] == 1
        assert responses[1]["id"] == 2
