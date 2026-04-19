> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Railway Monitoring System - Deployment & Quick Start 🚂

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

Vollständiges Produktions-Setup für das Railway Monitoring System mit Docker, ThemisDB, Ollama LLM und WPF Client.

## 📋 Inhaltsverzeichnis

- [Systemanforderungen](#systemanforderungen)
- [Quick Start (5 Minuten)](#quick-start-5-minuten)
- [Manuelle Installation](#manuelle-installation)
- [Verwendung](#verwendung)
- [Konfiguration](#konfiguration)
- [Troubleshooting](#troubleshooting)

---

> **Note:** Current Docker image: `docker run -d -p 8080:8080 themisdb/themisdb:latest`
> <!-- TODO: verify against current source -->

## Systemanforderungen

### Minimal (nur Backend & Simulator)
- **OS**: Linux, macOS oder Windows 10+
- **RAM**: 4 GB
- **CPU**: 2 Cores
- **Disk**: 10 GB freier Speicher
- **Software**: Docker + Docker Compose

### Empfohlen (inkl. WPF Client)
- **OS**: Windows 10+ (für WPF), Linux/macOS (für Backend)
- **RAM**: 8 GB
- **CPU**: 4 Cores
- **Disk**: 20 GB freier Speicher
- **Software**: 
  - Docker Desktop
  - .NET 8.0 SDK (für WPF)
  - Python 3.8+
  - Git

---

## Quick Start (5 Minuten)

### Option A: Automatisch mit Quick-Start Script

#### Linux / macOS
```bash
cd examples/railway
./quick-start.sh
```

#### Windows PowerShell
```powershell
cd examples\railway
.\quick-start.ps1
```

Das Script:
1. ✅ Prüft alle Voraussetzungen
2. ✅ Generiert Netzwerk-Daten (445 km, 10 Bahnhöfe)
3. ✅ Startet Docker Services (ThemisDB, Ollama, Simulator, Web UI)
4. ✅ Importiert Daten in ThemisDB
5. ✅ Zeigt Zugriffspunkte an

**Nach 1-2 Minuten:**
- 🌐 Web UI: http://localhost:8080
- 🗄️ ThemisDB API: http://localhost:8765
- 🤖 Ollama LLM: http://localhost:11434

### Option B: Manuell (Schritt für Schritt)

#### 1. Netzwerk-Daten generieren

**Python (einfachste Methode)**:
```bash
cd scripts/railway
python3 simple_network_generator.py
```

**Oder C++ (mehr Daten)**:
```bash
cd examples/railway
g++ -std=c++17 railway_base_data_generator.cpp -o railway_generator
./railway_generator
```

#### 2. Docker Services starten
```bash
cd examples/railway
docker-compose -f docker-compose.railway.yml up -d
```

#### 3. Auf Services warten
```bash
# Warte bis ThemisDB bereit ist
while ! curl -s http://localhost:8765/health > /dev/null; do
    echo "Waiting for ThemisDB..."
    sleep 5
done
echo "✓ ThemisDB ready!"
```

#### 4. Daten importieren
```bash
cd ../../scripts/railway
python3 import_railway_network.py ../../data/railway_network_base_germany.json
```

#### 5. Zugriff testen
```bash
# Web UI öffnen
open http://localhost:8080  # macOS
xdg-open http://localhost:8080  # Linux
start http://localhost:8080  # Windows

# API testen
curl http://localhost:8765/api/trains
```

---

## Manuelle Installation

### 1. Docker Installation

#### Linux (Ubuntu/Debian)
```bash
# Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Docker Compose
sudo apt install docker-compose

# User zu Docker-Gruppe hinzufügen
sudo usermod -aG docker $USER
newgrp docker
```

#### macOS
```bash
# Homebrew
brew install --cask docker

# Oder von https://www.docker.com/products/docker-desktop/
```

#### Windows
```powershell
# Mit winget
winget install Docker.DockerDesktop

# Oder von https://www.docker.com/products/docker-desktop/
```

### 2. Python Dependencies
```bash
cd scripts/railway
pip install -r requirements.txt
```

### 3. .NET SDK (für WPF Client)

#### Windows
```powershell
winget install Microsoft.DotNet.SDK.8
```

#### Linux
```bash
wget https://dot.net/v1/dotnet-install.sh -O dotnet-install.sh
chmod +x dotnet-install.sh
./dotnet-install.sh --channel 8.0
```

#### macOS
```bash
brew install dotnet@8
```

### 4. Ollama (optional, für KI-Analysen)

#### Linux/macOS
```bash
curl https://ollama.ai/install.sh | sh
ollama pull llama3.2
```

#### Windows
Download von https://ollama.ai/download

---

## Verwendung

### Backend Kontrolle

#### Services starten
```bash
cd examples/railway
docker-compose -f docker-compose.railway.yml up -d
```

#### Status prüfen
```bash
docker-compose -f docker-compose.railway.yml ps
```

#### Logs anzeigen
```bash
# Alle Services
docker-compose -f docker-compose.railway.yml logs -f

# Nur Simulator
docker-compose -f docker-compose.railway.yml logs -f train-simulator

# Nur ThemisDB
docker-compose -f docker-compose.railway.yml logs -f themisdb
```

#### Services stoppen
```bash
docker-compose -f docker-compose.railway.yml down
```

#### Daten löschen (Neustart)
```bash
docker-compose -f docker-compose.railway.yml down -v
```

### WPF Desktop Client (Windows)

```powershell
cd clients\RailwayMonitor.WPF
dotnet restore
dotnet run
```

Oder in Visual Studio 2022:
1. Lösung öffnen: `RailwayMonitor.WPF.csproj`
2. F5 drücken

### Simulator Kontrolle

#### Anzahl Züge ändern
```bash
# In docker-compose.railway.yml anpassen:
NUM_TRAINS=100  # Statt 50

# Services neu starten
docker-compose -f docker-compose.railway.yml restart train-simulator
```

#### Simulator manuell starten (ohne Docker)
```bash
cd scripts/railway
python3 train_simulator.py --trains 50 --themisdb-url http://localhost:8765
```

### API Verwendung

#### Züge abfragen
```bash
# Alle aktiven Züge
curl http://localhost:8765/api/trains

# Spezifischer Zug
curl http://localhost:8765/api/trains/ICE508

# Verspätungen
curl http://localhost:8765/api/trains?delay_gt=5
```

#### Bahnhöfe abfragen
```bash
curl http://localhost:8765/api/stations
```

#### Energie-Daten
```bash
# Kraftwerks-Mix
curl http://localhost:8765/api/energy/power-sources

# Unterwerke
curl http://localhost:8765/api/energy/substations

# Lastprognose (24h)
curl http://localhost:8765/api/energy/forecast
```

#### LLM Analysen
```bash
curl -X POST http://localhost:11434/api/generate \
  -H "Content-Type: application/json" \
  -d '{
    "model": "llama3.2",
    "prompt": "Warum hat ICE 508 Verspätung?",
    "stream": false
  }'
```

---

## Konfiguration

### ThemisDB Konfiguration

Datei: `examples/railway/docker-compose.railway.yml`

```yaml
environment:
  - THEMIS_LOG_LEVEL=INFO          # DEBUG, INFO, WARN, ERROR
  - THEMIS_ENABLE_CEP=true         # Complex Event Processing
  - THEMIS_ENABLE_LLM=true         # LLM Integration
  - THEMIS_MAX_CONNECTIONS=100     # Max. gleichzeitige Verbindungen
```

### Simulator Konfiguration

```yaml
environment:
  - NUM_TRAINS=50                  # Anzahl simulierter Züge
  - UPDATE_INTERVAL=1.0            # Update-Frequenz (Sekunden)
  - ENABLE_ENERGY_CALC=true        # Energieverbrauch berechnen
  - LOG_LEVEL=INFO                 # Logging-Level
```

### WPF Client Konfiguration

Datei: `clients/RailwayMonitor.WPF/appsettings.json`

```json
{
  "ThemisDb": {
    "BaseUrl": "http://localhost:8765",
    "Timeout": 30
  },
  "Ollama": {
    "BaseUrl": "http://localhost:11434",
    "Model": "llama3.2"
  },
  "Simulator": {
    "ScriptPath": "../../scripts/railway/train_simulator.py",
    "DefaultTrainCount": 50
  }
}
```

---

## Troubleshooting

### Problem: Docker Compose startet nicht

**Lösung**:
```bash
# Docker Daemon Status prüfen
systemctl status docker  # Linux
# Oder Docker Desktop öffnen (Windows/macOS)

# Docker neu starten
sudo systemctl restart docker  # Linux
```

### Problem: ThemisDB nicht erreichbar

**Lösung**:
```bash
# Logs prüfen
docker logs railway-themisdb

# Port 8765 belegt?
netstat -tuln | grep 8765  # Linux/macOS
netstat -an | findstr 8765  # Windows

# Container neu starten
docker restart railway-themisdb
```

### Problem: Simulator startet nicht

**Lösung**:
```bash
# Logs prüfen
docker logs railway-simulator

# Python Dependencies fehlen?
cd scripts/railway
pip install -r requirements.txt

# Netzwerk-Daten vorhanden?
ls -lh ../../data/railway_network_base_germany.json
```

### Problem: WPF Client kompiliert nicht

**Lösung**:
```powershell
# .NET SDK Version prüfen
dotnet --version  # Sollte 8.0.x sein

# Dependencies wiederherstellen
cd clients\RailwayMonitor.WPF
dotnet restore --force

# NuGet Cache leeren
dotnet nuget locals all --clear
```

### Problem: Ollama LLM reagiert nicht

**Lösung**:
```bash
# Modell geladen?
curl http://localhost:11434/api/tags

# Modell manuell laden
docker exec railway-ollama ollama pull llama3.2

# Container neu starten
docker restart railway-ollama
```

### Problem: Web UI zeigt keine Züge

**Lösung**:
```bash
# 1. Simulator läuft?
docker logs railway-simulator --tail 20

# 2. ThemisDB erreichbar?
curl http://localhost:8765/api/trains

# 3. Browser-Konsole prüfen (F12)
# CORS-Fehler? Nginx-Config prüfen

# 4. Daten importiert?
cd scripts/railway
python3 import_railway_network.py ../../data/railway_network_base_germany.json
```

---

## Performance Tuning

### Für mehr Züge (100+)

```yaml
# docker-compose.railway.yml anpassen:
train-simulator:
  environment:
    - NUM_TRAINS=100
  deploy:
    resources:
      limits:
        cpus: '2.0'
        memory: 2G
```

### Für Produktions-Setup

```yaml
themisdb:
  environment:
    - THEMIS_CACHE_SIZE=1G
    - THEMIS_MAX_CONNECTIONS=500
    - THEMIS_WORKER_THREADS=8
  deploy:
    resources:
      limits:
        cpus: '4.0'
        memory: 8G
```

---

## Weiterführende Dokumentation

- **Vollständiges Guide**: `../../docs/guides/RAILWAY_COMPLETE_GUIDE.md`
- **System-Architektur**: `docs/projects/RAILWAY_MONITORING.md`
- **Energie-Management**: `docs/projects/RAILWAY_ENERGY_MANAGEMENT.md`
- **Asset-Management**: `docs/projects/RAILWAY_ASSET_MANAGEMENT.md`
- **Datenmodelle**: `docs/projects/RAILWAY_TRAIN_DATA_MODEL.md`
- **API Integration**: `docs/projects/RAILWAY_REAL_DATA.md`

---

## Support & Lizenz

- **Repository**: https://github.com/makr-code/ThemisDB
- **Issues**: https://github.com/makr-code/ThemisDB/issues
- **Lizenz**: Siehe `LICENSE` Datei

---

**Ready for Production! 🚀**
