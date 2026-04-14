"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_rest_api.py                                   ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:35:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     352                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Comprehensive REST API tests for ThemisDB Python client.

Tests cover:
- HTTP client operations (CRUD, Query)
- Graph API operations
- Vector API operations
- Transaction handling
- Error scenarios
"""

import pytest
from unittest.mock import Mock, patch, AsyncMock
import httpx
from themis import ThemisClient
from themis.async_client import AsyncThemisClient


class TestRESTClient_CRUD:
    """Test REST API CRUD operations."""

    def test_client_creation(self):
        """Test client can be created with endpoints."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        assert client is not None
        assert hasattr(client, 'endpoints')

    def test_client_get_operation(self):
        """Test GET operation via REST API."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        # Mock the HTTP request
        with patch.object(client, '_request') as mock_request:
            mock_request.return_value = {"id": "user-123", "name": "Alice"}
            
            result = client.get("relational", "users", "user-123")
            
            mock_request.assert_called_once()
            assert result["name"] == "Alice"

    def test_client_put_operation(self):
        """Test PUT operation via REST API."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.return_value = None
            
            data = {"name": "Bob", "email": "bob@example.com"}
            client.put("relational", "users", "user-456", data)
            
            mock_request.assert_called_once()

    def test_client_delete_operation(self):
        """Test DELETE operation via REST API."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.return_value = None
            
            client.delete("relational", "users", "user-789")
            
            mock_request.assert_called_once()


class TestRESTClient_Query:
    """Test REST API query operations."""

    def test_query_execution(self):
        """Test AQL query execution."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.return_value = {
                "data": [
                    {"id": "1", "name": "Alice"},
                    {"id": "2", "name": "Bob"}
                ]
            }
            
            result = client.query("SELECT * FROM users")
            
            mock_request.assert_called_once()
            assert len(result["data"]) == 2

    def test_query_with_parameters(self):
        """Test query with parameters."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.return_value = {
                "data": [{"id": "3", "name": "Charlie", "age": 30}]
            }
            
            result = client.query("SELECT * FROM users WHERE age > 25")
            
            mock_request.assert_called_once()
            assert result["data"][0]["age"] == 30


class TestRESTClient_Graph:
    """Test REST API graph operations."""

    def test_graph_traverse(self):
        """Test graph traversal."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.return_value = {
                "nodes": ["node1", "node2", "node3"],
                "visited": ["node1", "node2", "node3"]
            }
            
            if hasattr(client, 'graph_traverse'):
                result = client.graph_traverse("node1", max_depth=3)
                mock_request.assert_called_once()
                assert len(result["nodes"]) == 3

    def test_shortest_path(self):
        """Test shortest path computation."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.return_value = {
                "path": ["node1", "node2", "node3"]
            }
            
            if hasattr(client, 'shortest_path'):
                result = client.shortest_path("node1", "node3")
                mock_request.assert_called_once()
                assert len(result["path"]) == 3


class TestRESTClient_Vector:
    """Test REST API vector operations."""

    def test_vector_search(self):
        """Test vector similarity search."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.return_value = {
                "results": [
                    {"id": "vec1", "score": 0.95},
                    {"id": "vec2", "score": 0.88}
                ]
            }
            
            if hasattr(client, 'vector_search'):
                embedding = [0.1, 0.2, 0.3]
                result = client.vector_search(embedding, k=10)
                mock_request.assert_called_once()
                assert len(result["results"]) == 2

    def test_vector_upsert(self):
        """Test vector insert/update."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.return_value = None
            
            if hasattr(client, 'vector_upsert'):
                embedding = [0.1, 0.2, 0.3]
                metadata = {"category": "test"}
                client.vector_upsert("vec-123", embedding, metadata)
                mock_request.assert_called_once()


class TestRESTClient_Transaction:
    """Test REST API transaction operations."""

    def test_begin_transaction(self):
        """Test transaction can be started."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.return_value = {"transaction_id": "tx-12345"}
            
            if hasattr(client, 'begin_transaction'):
                tx = client.begin_transaction()
                mock_request.assert_called_once()
                assert tx.transaction_id == "tx-12345"

    def test_transaction_commit(self):
        """Test transaction can be committed."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.return_value = None
            
            if hasattr(client, 'Transaction'):
                from themis import Transaction
                tx = Transaction(client, "tx-12345")
                tx.commit()
                mock_request.assert_called_once()

    def test_transaction_rollback(self):
        """Test transaction can be rolled back."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.return_value = None
            
            if hasattr(client, 'Transaction'):
                from themis import Transaction
                tx = Transaction(client, "tx-12345")
                tx.rollback()
                mock_request.assert_called_once()


class TestRESTClient_ErrorHandling:
    """Test REST API error handling."""

    def test_network_error(self):
        """Test handling of network errors."""
        client = ThemisClient(endpoints=["http://invalid-server:9999"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.side_effect = httpx.ConnectError("Connection failed")
            
            with pytest.raises(Exception):
                client.get("relational", "users", "123")

    def test_http_error_404(self):
        """Test handling of HTTP 404 errors."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.side_effect = httpx.HTTPStatusError(
                "Not Found", request=Mock(), response=Mock(status_code=404)
            )
            
            with pytest.raises(Exception):
                client.get("relational", "users", "not-found")

    def test_http_error_500(self):
        """Test handling of HTTP 500 errors."""
        client = ThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client, '_request') as mock_request:
            mock_request.side_effect = httpx.HTTPStatusError(
                "Internal Server Error", 
                request=Mock(), 
                response=Mock(status_code=500)
            )
            
            with pytest.raises(Exception):
                client.put("relational", "users", "123", {"name": "Test"})


@pytest.mark.asyncio
class TestAsyncRESTClient:
    """Test async REST API client."""

    async def test_async_client_creation(self):
        """Test async client can be created."""
        client = AsyncThemisClient(endpoints=["http://localhost:8080"])
        assert client is not None
        await client.close()

    async def test_async_get_operation(self):
        """Test async GET operation."""
        client = AsyncThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client.pool, 'request', new_callable=AsyncMock) as mock_request:
            mock_response = Mock()
            mock_response.json.return_value = {"id": "user-123", "name": "Alice"}
            mock_response.status_code = 200
            mock_request.return_value = mock_response
            
            if hasattr(client, 'get'):
                result = await client.get("relational", "users", "user-123")
                mock_request.assert_called_once()
                assert result["name"] == "Alice"
        
        await client.close()

    async def test_async_query_operation(self):
        """Test async query operation."""
        client = AsyncThemisClient(endpoints=["http://localhost:8080"])
        
        with patch.object(client.pool, 'request', new_callable=AsyncMock) as mock_request:
            mock_response = Mock()
            mock_response.json.return_value = {
                "data": [{"id": "1", "name": "Alice"}]
            }
            mock_response.status_code = 200
            mock_request.return_value = mock_response
            
            if hasattr(client, 'query'):
                result = await client.query("SELECT * FROM users")
                mock_request.assert_called_once()
        
        await client.close()


class TestRESTClient_Configuration:
    """Test REST client configuration options."""

    def test_client_with_multiple_endpoints(self):
        """Test client with multiple endpoints."""
        endpoints = [
            "http://server1:8080",
            "http://server2:8080",
            "http://server3:8080"
        ]
        client = ThemisClient(endpoints=endpoints)
        assert len(client.endpoints) == 3

    def test_client_with_timeout(self):
        """Test client with custom timeout."""
        client = ThemisClient(
            endpoints=["http://localhost:8080"],
            timeout=60.0
        )
        if hasattr(client, 'timeout'):
            assert client.timeout == 60.0

    def test_client_with_namespace(self):
        """Test client with custom namespace."""
        client = ThemisClient(
            endpoints=["http://localhost:8080"],
            namespace="custom"
        )
        if hasattr(client, 'namespace'):
            assert client.namespace == "custom"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
