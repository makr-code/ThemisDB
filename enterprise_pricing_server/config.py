"""Configuration management for the pricing server."""

from pydantic_settings import BaseSettings
from typing import Optional


class Settings(BaseSettings):
    """Application settings."""
    
    # Application
    app_name: str = "ThemisDB Enterprise Pricing Server"
    version: str = "1.0.0"
    debug: bool = False
    
    # Server
    host: str = "0.0.0.0"
    port: int = 8000
    
    # Database
    database_url: str = "sqlite+aiosqlite:///./pricing_server.db"
    
    # Security
    # SECURITY WARNING: Change this secret key in production!
    # Generate with: openssl rand -hex 32
    secret_key: str = "CHANGE-THIS-SECRET-KEY-IN-PRODUCTION"
    algorithm: str = "HS256"
    access_token_expire_minutes: int = 60 * 24  # 24 hours
    
    # Payment Provider (Stripe)
    stripe_api_key: Optional[str] = None
    stripe_webhook_secret: Optional[str] = None
    
    # Banking Interface
    banking_api_url: Optional[str] = None
    banking_api_key: Optional[str] = None
    
    # Pricing Tiers
    community_price: float = 0.0
    enterprise_price: float = 5000.0  # per month
    hyperscaler_price: float = 25000.0  # per month
    reseller_price: float = 15000.0  # per month
    
    class Config:
        env_file = ".env"
        env_file_encoding = "utf-8"


settings = Settings()
