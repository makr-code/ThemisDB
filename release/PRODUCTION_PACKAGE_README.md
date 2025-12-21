# ThemisDB v1.3.0 - Production Package Documentation

**Release**: v1.3.0 "ThemisDB keeps his own llamas"  
**Build Date**: 21. Dezember 2025  
**Package Type**: Production-Ready  

## 📦 Available Packages

### Windows x64 (Production)
- **File**: `themisdb-v1.3.0-windows-x64-production.zip`
- **Size**: 12.84 MB
- **SHA256**: `a802b2269de514ad02d0ec2b0537393403c2450cc06d824f9809af2e474ad6d5`
- **Build Platform**: Windows 11, MSVC 2022 (17.14)
- **Build Type**: Release

### Linux x64 (Production)
- **File**: `themisdb-v1.3.0-linux-x64-production.tar.gz`
- **Size**: 11.66 MB
- **SHA256**: `44e96d817868ff9d5a5bf35a8215def631aaf5ef95431ae210b63a9bb24ebdfc`
- **Build Platform**: Ubuntu 24.04 LTS, GCC 13.3
- **Build Type**: Release

## 📁 Package Structure

Beide Packages enthalten eine vollständige, produktionsreife Verzeichnisstruktur:

```
themisdb-1.3.0-{platform}/
├── bin/                          # Executables & DLLs/SOs
│   ├── themis_server(.exe)       # Main server binary
│   ├── themis_demo(.exe)         # Demo application
│   ├── themis_demo_encryption    # Encryption demo
│   └── *.dll / *.so              # Dependencies (fmt, curl, ssl, zstd, etc.)
│
├── config/                       # Configuration templates
│   ├── config.json               # Main server config
│   ├── llm_config.yaml           # LLM plugin configuration
│   ├── processors/               # Custom processors
│   └── schemas/                  # Data schemas
│
├── data/                         # Database storage
│   ├── db/                       # Main database files
│   ├── wal/                      # Write-Ahead Log
│   ├── backups/                  # Backup location
│   └── temp/                     # Temporary files
│
├── docs/                         # Documentation
│   ├── README.md                 # Project overview
│   ├── LICENSE                   # Apache 2.0 License
│   ├── CHANGELOG.md              # Version history
│   ├── SECURITY.md               # Security policy
│   ├── api/                      # REST API specs (OpenAPI)
│   ├── guides/                   # Deployment guides
│   └── architecture/             # Architecture docs
│
├── examples/                     # Sample code
│   ├── api/                      # REST API examples
│   │   └── query-example.ps1/.sh # Query example
│   └── integrations/             # Integration samples
│
├── logs/                         # Application logs (empty)
│
├── models/                       # LLM models (empty, download separately)
│
├── plugins/                      # Plugin extensions (empty)
│
├── scripts/                      # Operational scripts
│   ├── admin/                    # Administration tools
│   │   ├── backup.ps1/.sh        # Database backup
│   │   ├── restore.ps1/.sh       # Database restore
│   │   └── install-service.ps1   # Windows service installer
│   ├── monitoring/               # Monitoring scripts
│   │   └── health-check.ps1/.sh  # Health check
│   └── maintenance/              # Maintenance scripts
│
├── tests/                        # Test binaries (optional)
│   ├── integration/              # Integration tests
│   └── performance/              # Performance benchmarks
│
├── tools/                        # Utilities
│   ├── themisdb.service          # systemd service file (Linux)
│   ├── cli/                      # CLI utilities
│   ├── migration/                # Migration tools
│   └── backup/                   # Backup utilities
│
└── INSTALL.md                    # Installation guide
```

## 🚀 Quick Start

### Windows

```powershell
# 1. Extract package
Expand-Archive -Path themisdb-v1.3.0-windows-x64-production.zip -DestinationPath .

# 2. Navigate to directory
cd themisdb-1.3.0-production

# 3. Start server
.\bin\themis_server.exe --config config\config.json
```

#### Als Windows Service installieren

```powershell
# Requires Administrator privileges
cd themisdb-1.3.0-production
.\scripts\admin\install-service.ps1
```

### Linux

```bash
# 1. Extract package
tar -xzf themisdb-v1.3.0-linux-x64-production.tar.gz

# 2. Navigate to directory
cd themisdb-1.3.0-linux

# 3. Start server
./bin/themis_server --config config/config.json
```

#### Als systemd Service installieren

```bash
# Copy to system location
sudo cp -r themisdb-1.3.0-linux /opt/themisdb

# Install systemd service
sudo cp /opt/themisdb/tools/themisdb.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable themisdb
sudo systemctl start themisdb

# Check status
sudo systemctl status themisdb
```

## ⚙️ Configuration

### Server Configuration (`config/config.json`)

```json
{
  "server": {
    "host": "0.0.0.0",        // Bind address
    "port": 8765,              // Listen port
    "threads": 4,              // Worker threads
    "max_connections": 1000    // Connection limit
  },
  "database": {
    "path": "./data/db",
    "wal_path": "./data/wal",
    "backup_path": "./data/backups"
  },
  "logging": {
    "level": "info",           // trace, debug, info, warn, error
    "file": "./logs/themis.log",
    "max_size_mb": 100,
    "max_files": 10
  },
  "security": {
    "enable_tls": false,       // Enable HTTPS
    "cert_file": "",
    "key_file": ""
  }
}
```

### LLM Configuration (`config/llm_config.yaml`)

```yaml
llm:
  enabled: true
  plugin: llamacpp
  model:
    path: ./models/mistral-7b-instruct-v0.2.Q4_K_M.gguf
    n_gpu_layers: 32      # 0 for CPU-only
    n_ctx: 4096
    n_batch: 512
```

**Note**: LLM-Modelle müssen separat heruntergeladen werden:
- Mistral 7B: https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF
- Phi-3: https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf

## 🛠️ Admin Tools

### Database Backup

**Windows**:
```powershell
.\scripts\admin\backup.ps1
```

**Linux**:
```bash
./scripts/admin/backup.sh
```

Erstellt automatisch ein Backup in `data/backups/backup-YYYYMMDD-HHMMSS/`

### Database Restore

**Windows**:
```powershell
.\scripts\admin\restore.ps1 -BackupPath .\data\backups\backup-20251221-120000
```

**Linux**:
```bash
./scripts/admin/restore.sh ./data/backups/backup-20251221-120000
```

### Health Check

**Windows**:
```powershell
.\scripts\monitoring\health-check.ps1
```

**Linux**:
```bash
./scripts/monitoring/health-check.sh
```

Prüft den Status des ThemisDB-Servers über `/health` Endpoint.

## 🌐 REST API

Standard Endpoint: `http://localhost:8765`

### Core API

- `GET /health` - Server health check
- `GET /metrics` - Prometheus metrics
- `POST /api/query` - Execute AQL query
- `GET /api/collections` - List collections
- `POST /api/collections/{name}` - Create collection
- `DELETE /api/collections/{name}` - Delete collection

### LLM API

- `POST /api/llm/generate` - Generate text
- `POST /api/llm/embed` - Generate embeddings
- `GET /api/llm/models` - List loaded models
- `POST /api/llm/models/load` - Load model
- `DELETE /api/llm/models/{id}` - Unload model

### Example: AQL Query

**Windows PowerShell**:
```powershell
$body = @{
    query = "FOR doc IN users FILTER doc.age > 18 RETURN doc"
} | ConvertTo-Json

$response = Invoke-RestMethod `
    -Uri "http://localhost:8765/api/query" `
    -Method POST `
    -Body $body `
    -ContentType "application/json"

$response | ConvertTo-Json -Depth 10
```

**Linux Bash**:
```bash
curl -X POST http://localhost:8765/api/query \
  -H "Content-Type: application/json" \
  -d '{"query": "FOR doc IN users FILTER doc.age > 18 RETURN doc"}'
```

## 🔒 Security

### TLS/HTTPS konfigurieren

1. Zertifikate generieren:
```bash
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes
```

2. In `config/config.json`:
```json
{
  "security": {
    "enable_tls": true,
    "cert_file": "/path/to/cert.pem",
    "key_file": "/path/to/key.pem"
  }
}
```

### Firewall-Regeln

**Windows (PowerShell als Administrator)**:
```powershell
New-NetFirewallRule -DisplayName "ThemisDB" `
    -Direction Inbound `
    -Protocol TCP `
    -LocalPort 8765 `
    -Action Allow
```

**Linux (UFW)**:
```bash
sudo ufw allow 8765/tcp
sudo ufw reload
```

## 📊 Performance Testing

Falls Test-Binaries im Package enthalten sind:

**Windows**:
```powershell
cd tests\performance
.\bench_core_performance.exe
.\bench_crud.exe
.\bench_graph_traversal.exe
```

**Linux**:
```bash
cd tests/performance
./bench_core_performance
./bench_crud
./bench_graph_traversal
```

## 🐛 Troubleshooting

### Server startet nicht

1. **Port bereits belegt**:
```bash
# Windows
netstat -ano | findstr :8765

# Linux
sudo netstat -tulpn | grep 8765
```

2. **Fehlende Berechtigungen**: 
   - Windows: Als Administrator ausführen
   - Linux: `sudo chown -R themisdb:themisdb /opt/themisdb`

3. **Logs prüfen**:
```bash
# Windows
Get-Content logs\themis.log -Tail 50

# Linux
tail -f logs/themis.log
```

### LLM-Modell lädt nicht

- Stelle sicher, dass das Modell in `models/` vorhanden ist
- Prüfe Pfad in `config/llm_config.yaml`
- Bei GPU-Nutzung: CUDA/ROCm Treiber installiert?

### Performance-Probleme

- Erhöhe `threads` in `config/config.json`
- Für SSDs: Aktiviere Direct I/O
- Überwache mit `GET /metrics`

## 📝 System Requirements

### Minimum

- **CPU**: 2 Cores, x86_64
- **RAM**: 4 GB
- **Disk**: 10 GB freier Speicher
- **OS**: Windows 10/11 64-bit oder Linux (Ubuntu 20.04+, RHEL 8+)

### Empfohlen für LLM

- **CPU**: 8+ Cores
- **RAM**: 16+ GB
- **GPU**: NVIDIA (8+ GB VRAM) oder AMD ROCm-kompatibel
- **Disk**: SSD mit 50+ GB

## 📚 Weitere Dokumentation

- **Online Docs**: https://makr-code.github.io/ThemisDB/
- **API Reference**: `docs/api/openapi.yaml`
- **Architecture**: `docs/architecture/`
- **Deployment Guide**: `docs/guides/deployment_strategy.md`

## 🆘 Support

- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Discussions**: https://github.com/makr-code/ThemisDB/discussions
- **Security**: security@themisdb.io (siehe SECURITY.md)

## 📄 License

Apache License 2.0 - siehe LICENSE Datei

---

**ThemisDB v1.3.0** - "ThemisDB keeps his own llamas"  
© 2025 ThemisDB Contributors
