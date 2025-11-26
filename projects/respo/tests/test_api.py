"""
Tests for RESPO API
"""

import pytest
from fastapi.testclient import TestClient

from respo.api.app import app

client = TestClient(app)


class TestHealthEndpoint:
    """Tests for the health check endpoint."""

    def test_health_check(self) -> None:
        """Test health endpoint returns healthy status."""
        response = client.get("/health")
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "healthy"
        assert data["service"] == "respo"
        assert "version" in data


class TestRootEndpoint:
    """Tests for the root endpoint."""

    def test_root(self) -> None:
        """Test root endpoint returns API info."""
        response = client.get("/")
        assert response.status_code == 200
        data = response.json()
        assert data["name"] == "RESPO API"
        assert "endpoints" in data
        assert "chat" in data["endpoints"]


class TestPlaceholderEndpoints:
    """Tests for placeholder endpoints (not yet implemented)."""

    def test_chat_not_implemented(self) -> None:
        """Test chat endpoint returns not implemented."""
        response = client.post("/chat", json={"message": "test"})
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "not_implemented"

    def test_complete_not_implemented(self) -> None:
        """Test complete endpoint returns not implemented."""
        response = client.post("/complete", json={"code": "test"})
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "not_implemented"

    def test_explain_not_implemented(self) -> None:
        """Test explain endpoint returns not implemented."""
        response = client.post("/explain", json={"code": "test"})
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "not_implemented"

    def test_review_not_implemented(self) -> None:
        """Test review endpoint returns not implemented."""
        response = client.post("/review", json={"code": "test"})
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "not_implemented"

    def test_search_not_implemented(self) -> None:
        """Test search endpoint returns not implemented."""
        response = client.post("/search", json={"query": "test"})
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "not_implemented"

    def test_ingest_not_implemented(self) -> None:
        """Test ingest endpoint returns not implemented."""
        response = client.post("/ingest", json={"repo": "test"})
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "not_implemented"
