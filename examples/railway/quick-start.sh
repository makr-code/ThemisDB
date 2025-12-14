#!/bin/bash
#
# Railway Monitoring System - Quick Start Script
# Startet das komplette System mit einem Befehl
#

set -e

echo "═══════════════════════════════════════════════════════════════"
echo "   Railway Monitoring System - Quick Start"
echo "   Deutsche Bahn IoT & Energie-Management mit ThemisDB"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check prerequisites
echo -e "${BLUE}[1/6]${NC} Checking prerequisites..."

if ! command -v docker &> /dev/null; then
    echo -e "${RED}❌ Docker not found. Please install Docker first.${NC}"
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    echo -e "${RED}❌ Docker Compose not found. Please install Docker Compose first.${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Docker and Docker Compose found${NC}"

# Check if C++ compiler is available (optional)
if command -v g++ &> /dev/null; then
    HAS_CPP=true
    echo -e "${GREEN}✓ C++ compiler found (g++)${NC}"
else
    HAS_CPP=false
    echo -e "${YELLOW}⚠ C++ compiler not found - will use pre-generated data${NC}"
fi

# Generate network data
echo ""
echo -e "${BLUE}[2/6]${NC} Generating railway network data..."

if [ "$HAS_CPP" = true ]; then
    echo "   Compiling network generator..."
    g++ -std=c++17 railway_base_data_generator.cpp -o railway_generator 2>&1 | head -20
    
    if [ $? -eq 0 ]; then
        echo "   Generating network data (15 stations, ~400 segments)..."
        ./railway_generator
        echo -e "${GREEN}✓ Network data generated${NC}"
    else
        echo -e "${YELLOW}⚠ Compilation failed, using fallback data${NC}"
        HAS_CPP=false
    fi
fi

if [ "$HAS_CPP" = false ]; then
    # Create minimal sample data
    mkdir -p ../../data
    cat > ../../data/railway_network_base_germany.json <<'EOF'
{
  "metadata": {
    "name": "German Railway Network (Sample)",
    "version": "1.0",
    "generated": "2024-12-14",
    "stations": 5,
    "segments": 10
  },
  "stations": [
    {"id": "FF", "name": "Frankfurt (Main) Hbf", "lat": 50.1070, "lon": 8.6632},
    {"id": "MH", "name": "Mannheim Hbf", "lat": 49.4793, "lon": 8.4695},
    {"id": "KA", "name": "Karlsruhe Hbf", "lat": 48.9934, "lon": 8.4010},
    {"id": "HN", "name": "Heidelberg Hbf", "lat": 49.4039, "lon": 8.6752},
    {"id": "DA", "name": "Darmstadt Hbf", "lat": 49.8728, "lon": 8.6303}
  ]
}
EOF
    echo -e "${GREEN}✓ Sample network data created${NC}"
fi

# Start Docker services
echo ""
echo -e "${BLUE}[3/6]${NC} Starting Docker services..."
echo "   This will pull and start:"
echo "   - ThemisDB (port 8765)"
echo "   - Ollama LLM (port 11434)"
echo "   - Train Simulator (50 trains)"
echo "   - Web UI (port 8080)"
echo ""

docker-compose -f docker-compose.railway.yml up -d

echo -e "${GREEN}✓ Docker services started${NC}"

# Wait for services
echo ""
echo -e "${BLUE}[4/6]${NC} Waiting for services to be ready..."
echo "   This may take 1-2 minutes..."

MAX_WAIT=120
ELAPSED=0

while [ $ELAPSED -lt $MAX_WAIT ]; do
    if curl -s http://localhost:8765/health > /dev/null 2>&1; then
        echo -e "${GREEN}✓ ThemisDB ready${NC}"
        break
    fi
    sleep 5
    ELAPSED=$((ELAPSED + 5))
    echo -n "."
done

if [ $ELAPSED -ge $MAX_WAIT ]; then
    echo -e "${RED}❌ Timeout waiting for ThemisDB${NC}"
    echo "Check logs: docker-compose -f docker-compose.railway.yml logs themisdb"
    exit 1
fi

# Import network data
echo ""
echo -e "${BLUE}[5/6]${NC} Importing railway network to ThemisDB..."

cd ../../scripts/railway
python3 import_railway_network.py ../../data/railway_network_base_germany.json

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Network data imported${NC}"
else
    echo -e "${YELLOW}⚠ Import had issues, but continuing...${NC}"
fi

cd ../../examples/railway

# Show status
echo ""
echo -e "${BLUE}[6/6]${NC} System Status"
echo "═══════════════════════════════════════════════════════════════"

docker-compose -f docker-compose.railway.yml ps

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo -e "${GREEN}✓ Railway Monitoring System is running!${NC}"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "Access Points:"
echo "  🌐 Web UI:        http://localhost:8080"
echo "  🗄️  ThemisDB API:  http://localhost:8765"
echo "  🤖 Ollama LLM:    http://localhost:11434"
echo ""
echo "Quick Commands:"
echo "  📊 View logs:     docker-compose -f docker-compose.railway.yml logs -f"
echo "  📈 Simulator:     docker-compose -f docker-compose.railway.yml logs -f train-simulator"
echo "  🛑 Stop system:   docker-compose -f docker-compose.railway.yml down"
echo "  🗑️  Clean data:    docker-compose -f docker-compose.railway.yml down -v"
echo ""
echo "Next Steps:"
echo "  1. Open http://localhost:8080 to see live train map"
echo "  2. Check simulator output: docker logs railway-simulator"
echo "  3. Query API: curl http://localhost:8765/api/trains"
echo "  4. Run WPF client (Windows): cd ../../clients/RailwayMonitor.WPF && dotnet run"
echo ""
echo "Documentation:"
echo "  📖 Complete Guide: ../../RAILWAY_COMPLETE_GUIDE.md"
echo "  📖 API Docs:       ../../docs/projects/RAILWAY_MONITORING.md"
echo ""
echo "═══════════════════════════════════════════════════════════════"
