# ThemisDB QNAP Quick Start

Schnellanleitung zur Installation von ThemisDB auf QNAP NAS.

## 1-Minuten-Installation

```bash
# Image herunterladen
docker pull themisdb/themisdb:qnap

# Container starten
docker run -d \
  --name themis \
  --restart unless-stopped \
  -p 18765:18765 \
  -v /share/Container/themis/data:/data \
  -e TZ=Europe/Berlin \
  themisdb/themisdb:qnap
```

## Zugriff

ThemisDB ist verfügbar unter:
- **API**: `http://<QNAP-IP>:18765`
- **Health**: `http://<QNAP-IP>:18765/health`

## Unterschiede zu Standard-Build

| Eigenschaft | Standard (Ubuntu 22.04) | QNAP (Ubuntu 20.04) |
|-------------|------------------------|---------------------|
| GLIBC Version | 2.35 | 2.31 |
| Kompatibilität | Neuere Systeme | Ältere QNAP-Modelle |
| Default Port | 8080 | 18765 |
| Healthcheck | Optional | Aktiviert |

## Vollständige Dokumentation

Siehe [QNAP_DEPLOYMENT.md](QNAP_DEPLOYMENT.md) für:
- Erweiterte Konfiguration
- Volume-Management
- Backup/Restore
- Troubleshooting
- ARM-Support
- Performance-Tuning

## Support

Bei Fragen oder Problemen:
- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Dokumentation**: https://makr-code.github.io/ThemisDB/

## Build

Um selbst zu builden:

```bash
# QNAP x86_64
.\build-docker-qnap.ps1

# QNAP ARM64
docker build -f Dockerfile.qnap \
  --build-arg VCPKG_TRIPLET=arm64-linux \
  -t themisdb/themisdb:qnap-arm64 .
```
