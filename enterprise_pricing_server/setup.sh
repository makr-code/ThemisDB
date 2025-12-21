#!/bin/bash
# Quick start script for ThemisDB Enterprise Pricing Server

set -e

echo "========================================"
echo "ThemisDB Enterprise Pricing Server"
echo "========================================"
echo ""

# Check Python version
python_version=$(python3 --version 2>&1 | awk '{print $2}')
echo "✓ Python version: $python_version"

# Create virtual environment if not exists
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
fi

# Activate virtual environment
echo "Activating virtual environment..."
source venv/bin/activate

# Install dependencies
echo "Installing dependencies..."
pip install -q --upgrade pip
pip install -q -r requirements.txt

# Create .env if not exists
if [ ! -f ".env" ]; then
    echo "Creating .env file from template..."
    cp .env.example .env
    echo "⚠️  Please edit .env and update SECRET_KEY and other settings!"
fi

echo ""
echo "✓ Setup complete!"
echo ""
echo "To start the server:"
echo "  1. Activate venv: source venv/bin/activate"
echo "  2. Run server: python run_server.py"
echo "  or: python -m uvicorn app:app --reload"
echo ""
echo "To start the Tkinter admin UI:"
echo "  python tkinter_admin.py"
echo ""
echo "API Documentation will be available at:"
echo "  - http://localhost:8000/docs (Swagger UI)"
echo "  - http://localhost:8000/redoc (ReDoc)"
echo ""
