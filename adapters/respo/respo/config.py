"""
RESPO Configuration Module

Handles all configuration via environment variables with sensible defaults.
"""

from functools import lru_cache
from typing import Optional

from pydantic import Field
from pydantic_settings import BaseSettings, SettingsConfigDict


class ThemisSettings(BaseSettings):
    """ThemisDB connection settings."""

    url: str = Field(default="http://localhost:8765", description="ThemisDB server URL")
    auth_token: Optional[str] = Field(default=None, description="Optional JWT auth token")

    model_config = SettingsConfigDict(env_prefix="THEMIS_")


class VLLMSettings(BaseSettings):
    """vLLM server settings."""

    url: str = Field(default="http://localhost:8000", description="vLLM server URL")
    model: str = Field(
        default="codellama/CodeLlama-13b-Instruct-hf", description="Base model name"
    )
    api_key: Optional[str] = Field(default=None, description="Optional API key")
    lora_modules: str = Field(default="", description="LoRA modules (name=path,name=path)")

    # Generation settings
    max_tokens: int = Field(default=2048, description="Maximum tokens to generate")
    temperature: float = Field(default=0.1, description="Sampling temperature")
    top_p: float = Field(default=0.95, description="Top-p sampling")

    model_config = SettingsConfigDict(env_prefix="VLLM_")

    @property
    def lora_module_dict(self) -> dict[str, str]:
        """Parse LoRA modules string into dictionary."""
        if not self.lora_modules:
            return {}
        modules = {}
        for module in self.lora_modules.split(","):
            if "=" in module:
                name, path = module.split("=", 1)
                modules[name.strip()] = path.strip()
        return modules


class EmbeddingSettings(BaseSettings):
    """Embedding model settings."""

    model: str = Field(default="microsoft/codebert-base", description="Embedding model name")
    device: str = Field(default="cuda", description="Device (cuda, cpu, mps)")
    batch_size: int = Field(default=32, description="Batch size for embedding")

    model_config = SettingsConfigDict(env_prefix="EMBEDDING_")


class RAGSettings(BaseSettings):
    """RAG pipeline settings."""

    top_k: int = Field(default=50, description="Number of documents to retrieve")
    rerank_top_k: int = Field(default=10, description="Number of documents after reranking")
    min_score: float = Field(default=0.5, description="Minimum similarity score")

    model_config = SettingsConfigDict(env_prefix="RAG_")


class RerankSettings(BaseSettings):
    """Reranking model settings."""

    model: str = Field(
        default="cross-encoder/ms-marco-MiniLM-L-6-v2", description="Reranking model"
    )

    model_config = SettingsConfigDict(env_prefix="RERANK_")


class APISettings(BaseSettings):
    """API server settings."""

    host: str = Field(default="0.0.0.0", description="API server host")
    port: int = Field(default=8080, description="API server port")
    workers: int = Field(default=1, description="Number of workers")

    model_config = SettingsConfigDict(env_prefix="API_")


class CORSSettings(BaseSettings):
    """CORS settings."""

    origins: str = Field(default="*", description="Allowed origins (comma-separated)")

    model_config = SettingsConfigDict(env_prefix="CORS_")

    @property
    def origin_list(self) -> list[str]:
        """Parse origins string into list."""
        if self.origins == "*":
            return ["*"]
        return [origin.strip() for origin in self.origins.split(",")]


class CacheSettings(BaseSettings):
    """Cache settings."""

    ttl: int = Field(default=3600, description="Cache TTL in seconds")
    max_size: int = Field(default=1000, description="Maximum cache entries")

    model_config = SettingsConfigDict(env_prefix="CACHE_")


class FeatureFlags(BaseSettings):
    """Feature flags."""

    streaming: bool = Field(default=True, alias="ENABLE_STREAMING")
    reranking: bool = Field(default=True, alias="ENABLE_RERANKING")
    graph_retrieval: bool = Field(default=True, alias="ENABLE_GRAPH_RETRIEVAL")
    cache: bool = Field(default=True, alias="ENABLE_CACHE")

    model_config = SettingsConfigDict(populate_by_name=True)


class LogSettings(BaseSettings):
    """Logging settings."""

    level: str = Field(default="INFO", description="Log level")
    format: str = Field(default="json", description="Log format (json or text)")

    model_config = SettingsConfigDict(env_prefix="LOG_")


class Settings(BaseSettings):
    """Main settings class combining all sub-settings."""

    # Sub-settings
    themis: ThemisSettings = Field(default_factory=ThemisSettings)
    vllm: VLLMSettings = Field(default_factory=VLLMSettings)
    embedding: EmbeddingSettings = Field(default_factory=EmbeddingSettings)
    rag: RAGSettings = Field(default_factory=RAGSettings)
    rerank: RerankSettings = Field(default_factory=RerankSettings)
    api: APISettings = Field(default_factory=APISettings)
    cors: CORSSettings = Field(default_factory=CORSSettings)
    cache: CacheSettings = Field(default_factory=CacheSettings)
    features: FeatureFlags = Field(default_factory=FeatureFlags)
    log: LogSettings = Field(default_factory=LogSettings)

    model_config = SettingsConfigDict(
        env_file=".env",
        env_file_encoding="utf-8",
        extra="ignore",
    )


@lru_cache()
def get_settings() -> Settings:
    """Get cached settings instance."""
    return Settings()
