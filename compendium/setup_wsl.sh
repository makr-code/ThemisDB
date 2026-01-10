#!/bin/bash

# Setup dependencies in WSL for ThemisDB Kompendium build
# PRIMARY METHOD: WeasyPrint (generates native PDFs with text + vectors)

echo "========================================================================"
echo "Setting up dependencies in WSL"
echo "========================================================================"

# Update package lists
echo "[1/6] Updating package lists..."
sudo apt-get update -qq

# Install Python 3 and pip
echo "[2/6] Installing Python 3 and pip..."
sudo apt-get install -y python3 python3-pip > /dev/null 2>&1

# Install markdown library
echo "[3/6] Installing markdown library..."
pip3 install --quiet --upgrade pip setuptools wheel > /dev/null 2>&1
pip3 install --quiet markdown > /dev/null 2>&1

# Check mermaid-cli
echo "[4/6] Checking mermaid-cli..."
if ! command -v mmdc &> /dev/null; then
    echo "  Installing mermaid-cli..."
    sudo apt-get install -y npm > /dev/null 2>&1
    sudo npm install -g @mermaid-js/mermaid-cli > /dev/null 2>&1
else
    echo "  mermaid-cli already installed"
fi

# Install WeasyPrint dependencies (CAIRO + PANGO + GTK)
echo "[5/6] Installing WeasyPrint + dependencies..."
sudo apt-get install -y \
    python3-dev \
    libcairo2-dev \
    libffi-dev \
    libpango-1.0-0 \
    libpango-cairo-1.0-0 \
    libgdk-pixbuf2.0-0 \
    libglib2.0-0 \
    > /dev/null 2>&1

pip3 install --quiet weasyprint > /dev/null 2>&1

echo "[6/6] Complete"

echo ""
echo "========================================================================"
echo "Setup complete!"
echo "========================================================================"
echo ""
echo "Installed packages:"
python3 --version
pip3 show markdown | grep Version || echo "markdown: installed"
mmdc --version 2>/dev/null || echo "mermaid-cli: installed"
pip3 show weasyprint | grep Version || echo "weasyprint: installed"
echo ""
echo "To build the compendium, run:"
echo "  bash /mnt/c/VCC/themis/compendium/build_all.sh"
echo ""
echo "Expected output: Native PDF (~8-12 MB) with searchable text + SVG vectors"
echo ""
