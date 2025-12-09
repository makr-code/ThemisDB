# THEMIS v1.0.1 Installation & Setup Guide

## Quick Start

### 1. Extract the Package
```bash
unzip themisdb-1.0.1-linux-x64-prod.zip
cd themisdb-1.0.1
```

### 2. Start the Server
```bash
# Linux/QNAP
./bin/themis_server

# Windows
.\bin\themis_server.exe
```

The server will:
- Load `config.json` from the current directory
- Create/use `data/` directory for storage
- Write logs to `logs/` directory
- Use `cache/` for temporary caching
- Load plugins from `plugins/` directory

### 3. Verify Installation
```bash
# Check if server is running
curl http://localhost:8080/health

# Expected response:
# {"status":"healthy","version":"1.0.1"}
```

---

## Directory Structure

```
themisdb-1.0.1/
├── bin/                          ← Binary executable
│   └── themis_server            (Main application)
│
├── data/                         ← Persistent data storage
│   ├── database/                (SQLite databases)
│   ├── indices/                 (Search indices)
│   ├── blobs/                   (Large file storage)
│   └── snapshots/               (Backup snapshots)
│
├── logs/                         ← Application logs
│   ├── application.log
│   ├── error.log
│   └── debug.log               (if enabled)
│
├── cache/                        ← Temporary cache
│   ├── query_cache/
│   ├── embedding_cache/
│   └── compression_buffers/
│
├── plugins/                      ← Content processors
│   ├── video_processor.so
│   ├── image_processor.so
│   ├── audio_processor.so
│   ├── geo_processor.so
│   └── cad_processor.so
│
├── backups/                      ← Backup location
│   └── (incremental backups)
│
├── temp/                         ← Temporary files
│   └── (working directory)
│
├── config-templates/             ← Reference configs
│   ├── config.json              (All options)
│   ├── config.qnap.json         (QNAP optimization)
│   ├── config.rpi4.json         (Raspberry Pi)
│   ├── acceleration.yaml
│   ├── content_processors.yaml
│   ├── mime_types.yaml
│   ├── policies.json
│   └── (+ 20 more config files)
│
├── scripts/                      ← Helper scripts
│   ├── backup-incremental.sh
│   ├── restore.sh
│   ├── run_server_with_jaeger.sh
│   └── (+ deployment scripts)
│
├── examples/                     ← Sample code
│   ├── sharding_demo.cpp
│   ├── hot_reload_example.cpp
│   └── geo/                     (geospatial examples)
│
├── config.json                   ← Active config (main)
├── mime_types.yaml               ← MIME type definitions
├── content_processors.yaml       ← Processor configuration
├── README.md                     ← Project information
├── CHANGELOG.md                  ← Version history
├── LICENSE                       ← License
└── VERSION                       ← Version file
```

---

## Configuration

### Primary Configuration: `config.json`

Default configuration includes:
- **Server**: HTTP port (default: 8080), gRPC port (default: 50051)
- **Database**: SQLite storage location
- **Storage**: Data directory configuration
- **Content Processing**: Enabled processors and options
- **Security**: TLS/SSL, authentication
- **Performance**: Connection pools, timeouts
- **Observability**: Logging levels, metrics

### Quick Configuration Changes

#### Change HTTP Port
```json
{
  "server": {
    "http_port": 9090,
    "grpc_port": 50052
  }
}
```

#### Enable GPU Acceleration
```json
{
  "acceleration": {
    "enable_gpu": true,
    "cuda_device": 0
  }
}
```

#### Set Data Directory
```json
{
  "storage": {
    "data_path": "/var/themis/data",
    "backup_path": "/var/themis/backups"
  }
}
```

### Configuration Files in `config-templates/`

- **acceleration.yaml** - GPU/SIMD settings
- **content_processors.yaml** - Which processors to load
- **mime_types.yaml** - MIME type mappings
- **policies.json** - Security policies
- **retention_policies.yaml** - Data retention rules
- **sharding-with-metrics.yaml** - Horizontal scaling
- **storage_redundancy.yaml** - Replication setup
- **pii_patterns.yaml** - Data masking patterns

---

## First-Time Setup

### 1. Basic Setup (Out-of-the-Box)
```bash
cd themisdb-1.0.1
./bin/themis_server
```
✓ Server starts immediately with default config
✓ Data stored in `./data/`
✓ Accessible at `http://localhost:8080`

### 2. Production Setup

#### Adjust Configuration
```bash
# Edit the configuration
nano config.json

# Or use a platform-specific template
cp config-templates/config.json config.json
```

#### Create Directories with Proper Permissions
```bash
# Linux/QNAP
mkdir -p data logs cache backups temp
chmod 755 bin/themis_server
chmod 755 data logs cache backups temp
```

#### Start Server
```bash
./bin/themis_server &
```

#### Verify Startup
```bash
sleep 2
curl http://localhost:8080/health
tail -f logs/application.log
```

### 3. Advanced Setup (with Monitoring)

#### Start Jaeger Tracing (optional)
```bash
cd scripts
./start_jaeger.ps1        # Windows
./start_jaeger.sh         # Linux
```

#### Start Server with Tracing
```bash
./scripts/run_server_with_jaeger.sh
```

#### Monitor Metrics
```bash
# Prometheus endpoint
curl http://localhost:8080/metrics
```

---

## Typical Operations

### Check Server Health
```bash
curl http://localhost:8080/health
curl http://localhost:8080/version
curl http://localhost:8080/metrics
```

### View Logs
```bash
tail -f logs/application.log
tail -f logs/error.log
```

### Backup Database
```bash
./scripts/backup-incremental.sh
```

### Restore from Backup
```bash
./scripts/restore.sh backup-2025-12-09.tar.gz
```

### Stop Server Gracefully
```bash
curl -X POST http://localhost:8080/shutdown

# Or send signal
pkill -TERM themis_server
```

### Reload Configuration
```bash
curl -X POST http://localhost:8080/config/reload
```

---

## Platform-Specific Notes

### Windows
- Binary: `bin\themis_server.exe`
- Paths use backslashes: `data\` instead of `data/`
- Logs: `logs\application.log`
- Run in PowerShell or Command Prompt

### Linux
- Binary: `bin/themis_server`
- Standard Linux directory structure
- Systemd service file available in `scripts/systemd/`
- Recommended: Install to `/opt/themis/`

### QNAP
- Use `config-templates/config.qnap.json` as base
- Binary optimized for NAS environment
- Recommended: Install to `/share/themis/`
- Data stored on shared volumes recommended

### Raspberry Pi
- Use appropriate config: `config.rpi4.json` or `config.rpi5.json`
- Reduced memory settings enabled
- ARM-optimized binary included
- Good for edge processing

---

## Troubleshooting

### Server Won't Start

**Check logs:**
```bash
tail -100 logs/error.log
```

**Common issues:**
- Port already in use: Change `http_port` in config.json
- Permission denied: Run `chmod +x bin/themis_server`
- Config file not found: Ensure `config.json` in working directory

### Port Already in Use
```bash
# Find and kill process using port 8080
lsof -i :8080
kill -9 <PID>

# Or change port in config.json
```

### High Memory Usage
- Check `config.json` for `max_connections` setting
- Review `cache/` directory size
- Enable compression in `acceleration.yaml`

### Slow Performance
- Enable GPU: Set `enable_gpu: true` in acceleration config
- Check CPU usage: `top` or `Task Manager`
- Review logs for warnings

### Cannot Connect to Server
```bash
# Verify server is listening
netstat -tuln | grep 8080

# Check firewall
sudo ufw allow 8080

# Test connectivity
curl -v http://localhost:8080/health
```

---

## Performance Tuning

### For Development
```json
{
  "logging": {"level": "debug"},
  "performance": {"max_connections": 10}
}
```

### For Production
```json
{
  "logging": {"level": "info"},
  "performance": {"max_connections": 1000},
  "cache": {"enabled": true, "ttl_seconds": 3600},
  "compression": {"enabled": true}
}
```

### For High-Throughput Systems
```json
{
  "server": {"worker_threads": 16},
  "acceleration": {"enable_gpu": true},
  "storage": {"enable_compression": true},
  "cache": {"ttl_seconds": 7200}
}
```

---

## Security

### Enable TLS
1. Generate certificates (see `scripts/generate_test_certs.sh`)
2. Update `config.json`:
```json
{
  "server": {
    "tls_enabled": true,
    "cert_path": "/path/to/cert.pem",
    "key_path": "/path/to/key.pem"
  }
}
```

### Set Authentication
```json
{
  "auth": {
    "enabled": true,
    "type": "token",
    "token_secret": "your-secret-here"
  }
}
```

### Enable Audit Logging
```json
{
  "audit": {
    "enabled": true,
    "log_path": "logs/audit.log"
  }
}
```

---

## Deployment

### Docker (from minimal package)
```dockerfile
FROM ubuntu:24.04
COPY themis_server /app/bin/
COPY config.json /app/
WORKDIR /app
RUN mkdir -p data logs cache plugins backups temp
EXPOSE 8080 50051
CMD ["./bin/themis_server"]
```

### Kubernetes
```bash
kubectl apply -f scripts/k8s/
```

### Systemd (Linux)
```bash
sudo cp scripts/systemd/themis.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable themis
sudo systemctl start themis
```

### Docker Compose
```bash
docker-compose -f scripts/docker-compose.yml up -d
```

---

## Support & Documentation

- **Full API Docs**: See `scripts/openapi.yaml`
- **Architecture**: See `scripts/architecture/`
- **Deployment Guides**: See `scripts/deployment/`
- **Plugin Development**: See `scripts/plugins/`
- **Configuration Reference**: See `config-templates/`

For issues or questions, refer to the complete documentation in the package.

---

**Version**: 1.0.1  
**Release Date**: December 9, 2025  
**Platforms**: Windows, Linux, QNAP, Raspberry Pi
