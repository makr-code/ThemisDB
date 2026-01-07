#!/usr/bin/env python3
"""Run the ThemisDB Enterprise Pricing Server."""

import sys
import os

# Add parent directory to path so imports work
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import uvicorn
from app import app
from config import settings

if __name__ == "__main__":
    print("=" * 60)
    print("ThemisDB Enterprise Pricing Server")
    print("=" * 60)
    print(f"Version: {settings.version}")
    print(f"Host: {settings.host}:{settings.port}")
    print(f"Debug: {settings.debug}")
    print(f"Database: {settings.database_url}")
    print("=" * 60)
    print(f"API Documentation: http://{settings.host}:{settings.port}/docs")
    print(f"ReDoc: http://{settings.host}:{settings.port}/redoc")
    print("=" * 60)
    print("")
    
    uvicorn.run(
        "app:app",
        host=settings.host,
        port=settings.port,
        reload=settings.debug
    )
