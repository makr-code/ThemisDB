"""
RESPO MCP Server Implementation

Model Context Protocol server for VS Code and other MCP-compatible clients.
"""

import asyncio
import json
from dataclasses import dataclass
from typing import Any, Callable, Optional

import structlog

logger = structlog.get_logger(__name__)


@dataclass
class MCPTool:
    """MCP Tool definition."""
    name: str
    description: str
    input_schema: dict[str, Any]
    handler: Callable


@dataclass
class MCPResource:
    """MCP Resource definition."""
    uri: str
    name: str
    description: str
    mime_type: str = "text/plain"


class MCPServer:
    """MCP Server for VS Code integration."""

    def __init__(self, name: str = "respo", version: str = "1.0.0"):
        self.name = name
        self.version = version
        self._tools: dict[str, MCPTool] = {}
        self._resources: dict[str, MCPResource] = {}
        self._prompts: dict[str, dict] = {}

    def register_tool(self, tool: MCPTool) -> None:
        self._tools[tool.name] = tool

    def register_resource(self, resource: MCPResource) -> None:
        self._resources[resource.uri] = resource

    def register_prompt(self, name: str, template: dict) -> None:
        self._prompts[name] = template

    async def handle_request(self, request: dict) -> dict:
        """Handle an MCP JSON-RPC request."""
        method = request.get("method", "")
        params = request.get("params", {})
        request_id = request.get("id")

        handlers = {
            "initialize": self._handle_initialize,
            "tools/list": self._handle_tools_list,
            "tools/call": self._handle_tools_call,
            "resources/list": self._handle_resources_list,
            "prompts/list": self._handle_prompts_list,
            "ping": lambda p: {},
        }

        handler = handlers.get(method)
        if not handler:
            return {"jsonrpc": "2.0", "id": request_id, "error": {"code": -32601, "message": f"Method not found: {method}"}}

        try:
            result = await handler(params) if asyncio.iscoroutinefunction(handler) else handler(params)
            return {"jsonrpc": "2.0", "id": request_id, "result": result}
        except Exception as e:
            return {"jsonrpc": "2.0", "id": request_id, "error": {"code": -32603, "message": str(e)}}

    async def _handle_initialize(self, params: dict) -> dict:
        return {
            "protocolVersion": "2024-11-05",
            "capabilities": {"tools": {"listChanged": True}, "resources": {"subscribe": True}, "prompts": {"listChanged": True}},
            "serverInfo": {"name": self.name, "version": self.version},
        }

    async def _handle_tools_list(self, params: dict) -> dict:
        return {"tools": [{"name": t.name, "description": t.description, "inputSchema": t.input_schema} for t in self._tools.values()]}

    async def _handle_tools_call(self, params: dict) -> dict:
        tool = self._tools.get(params.get("name"))
        if not tool:
            raise ValueError(f"Tool not found: {params.get('name')}")
        result = await tool.handler(**params.get("arguments", {})) if asyncio.iscoroutinefunction(tool.handler) else tool.handler(**params.get("arguments", {}))
        return {"content": [{"type": "text", "text": json.dumps(result) if isinstance(result, dict) else str(result)}]}

    async def _handle_resources_list(self, params: dict) -> dict:
        return {"resources": [{"uri": r.uri, "name": r.name, "description": r.description} for r in self._resources.values()]}

    async def _handle_prompts_list(self, params: dict) -> dict:
        return {"prompts": [{"name": n, "description": t.get("description", "")} for n, t in self._prompts.items()]}
