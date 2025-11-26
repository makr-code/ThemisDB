"""
Tests for RESPO Configuration
"""

import os

import pytest

from respo.config import Settings, get_settings


class TestSettings:
    """Tests for the Settings class."""

    def test_default_settings(self) -> None:
        """Test that default settings are applied."""
        settings = Settings()
        
        # ThemisDB defaults
        assert settings.themis.url == "http://localhost:8765"
        assert settings.themis.auth_token is None
        
        # vLLM defaults
        assert settings.vllm.url == "http://localhost:8000"
        assert "codellama" in settings.vllm.model.lower()
        
        # Embedding defaults
        assert "codebert" in settings.embedding.model.lower()
        
        # API defaults
        assert settings.api.host == "0.0.0.0"
        assert settings.api.port == 8080

    def test_lora_module_parsing(self) -> None:
        """Test LoRA module string parsing."""
        settings = Settings()
        settings.vllm.lora_modules = "python=/models/python,typescript=/models/ts"
        
        modules = settings.vllm.lora_module_dict
        assert "python" in modules
        assert "typescript" in modules
        assert modules["python"] == "/models/python"
        assert modules["typescript"] == "/models/ts"

    def test_empty_lora_modules(self) -> None:
        """Test empty LoRA modules."""
        settings = Settings()
        settings.vllm.lora_modules = ""
        
        modules = settings.vllm.lora_module_dict
        assert modules == {}

    def test_cors_origin_parsing(self) -> None:
        """Test CORS origin parsing."""
        settings = Settings()
        
        # Default wildcard
        assert settings.cors.origin_list == ["*"]
        
        # Multiple origins
        settings.cors.origins = "http://localhost:3000,http://localhost:8080"
        origins = settings.cors.origin_list
        assert len(origins) == 2
        assert "http://localhost:3000" in origins
        assert "http://localhost:8080" in origins

    def test_feature_flags_defaults(self) -> None:
        """Test feature flag defaults."""
        settings = Settings()
        
        assert settings.features.streaming is True
        assert settings.features.reranking is True
        assert settings.features.graph_retrieval is True
        assert settings.features.cache is True


class TestGetSettings:
    """Tests for the get_settings function."""

    def test_get_settings_cached(self) -> None:
        """Test that get_settings returns cached instance."""
        settings1 = get_settings()
        settings2 = get_settings()
        
        # Should be the same instance (cached)
        assert settings1 is settings2

    def test_get_settings_returns_settings(self) -> None:
        """Test that get_settings returns Settings instance."""
        settings = get_settings()
        assert isinstance(settings, Settings)
