do# 🐳 ThemisDB Docker Build - Konsolidierte Version

## ✅ Ein Dockerfile für alle Editionen

**Datei:** `Dockerfile.themis-server`

Unterstützt alle Editionen über Build-Args:
- Community (LLM + Prometheus + GPU)
- Professional (LLM + Prometheus + GPU)
- Enterprise (LLM + Prometheus + GPU)
- Hyperscaler (LLM + Prometheus + GPU)

**WICHTIG:** 
- ✅ **LLM (llama.cpp)** ist in ALLEN Editionen enthalten
- ✅ **Prometheus Metriken** sind in ALLEN Editionen enthalten
- ✅ **GPU Support** ist in ALLEN Editionen enthalten

**Alle Editionen sind identisch - nur der Name unterscheidet sich!**

---

## 🚀 Docker Build Befehle

### Community Edition
```bash
docker build -f Dockerfile.themis-server -t themisdb/themisdb:community .
```

### Professional Edition
```bash
docker build -f Dockerfile.themis-server -t themisdb/themisdb:professional .
```

### Enterprise Edition
```bash
docker build -f Dockerfile.themis-server -t themisdb/themisdb:enterprise .
```

### Hyperscaler Edition
```bash
docker build -f Dockerfile.themis-server -t themisdb/themisdb:hyperscaler .
```

**Alle Builds sind identisch und enthalten:**
- ✅ Prometheus Metriken
- ✅ LLM (llama.cpp)
- ✅ GPU/CUDA Support
- ✅ REST API auf Port 8080
- ✅ Wire Protocol auf Port 18765

**Dauer:** ~25-30 Minuten (inkl. GPU-Libraries)

**Hinweis:** Für GPU-Nutzung NVIDIA Docker Runtime erforderlich:
```bash
docker run --gpus all themisdb/themisdb:latest
```

---

## 🔧 Build-Args Übersicht

| Argument | Werte | Default | Beschreibung |
|----------|-------|---------|-------------|
| `THEMIS_ENABLE_LLM` | ON/OFF | **ON** | LLM-Funktionen (in allen Editionen) |
| `THEMIS_ENABLE_GPU` | ON/OFF | **ON** | GPU/CUDA Support (in allen Editionen) |
| `CMAKE_BUILD_TYPE` | Release/Debug | Release | Build-Modus |

**WICHTIG:** Alle Features sind standardmäßig aktiviert!

---

## 📊 Prometheus Metriken

**Wichtig:** Prometheus Metriken sind in **ALLEN Editionen** automatisch aktiviert!

### Verfügbare Endpunkte
- `http://localhost:8080/metrics` - Prometheus Format
- `http://localhost:8080/health` - Health Check
- `http://localhost:9090` - Optional (gleiche Metriken wie 8080)

### Nach dem Build
```bash
# Container starten
docker run -d -p 18765:18765 -p 8080:8080 themisdb/themisdb:latest

# Metriken abrufen
curl http://localhost:8080/metrics

# Sollte zeigen:
# # HELP themis_raid_io_bytes_total ...
# # TYPE themis_raid_io_bytes_total counter
# themis_raid_io_bytes_total{...} 0
```

---

## 🐳 Docker Compose Integration

### Standard Setup
```yaml
services:
  themis:
    image: themisdb/themisdb:latest  # Community
    # ODER
    # image: themisdb/themisdb:enterprise  # Enterprise
    # ODER
    # image: themisdb/themisdb:hyperscaler  # Hyperscaler
    ports:
      - "18765:18765"
      - "8080:8080"
    environment:
      THEMIS_PORT: "18765"
      THEMIS_ENABLE_METRICS: "true"
```

### Hyperscaler mit GPU
```yaml
services:
  themis-hyperscaler:
    image: themisdb/themisdb:hyperscaler
    runtime: nvidia
    environment:
      NVIDIA_VISIBLE_DEVICES: all
    ports:
      - "18765:18765"
      - "8080:8080"
```

---

## ✅ Vorteile der Konsolidierung

| Vorher ❌ | Nachher ✅ |
|----------|----------|
| 2+ separate Dockerfiles | 1 Dockerfile |
| `Dockerfile.themis-server` (kaputt) | `Dockerfile.themis-server` (funktioniert) |
| `Dockerfile.themis-metrics-enabled` | → konsolidiert |
| Windows Binary | Linux Binary |
| Features nur in bestimmten Editionen | **Alle Features in ALLEN Editionen** |
| Unterschiedliche Build-Befehle | Identische Builds (nur Name unterscheidet sich) |
| GPU nur in Hyperscaler | **GPU in ALLEN Editionen** |

---

## 🔍 Verifizierung

### Nach dem Build
```bash
# Image prüfen
docker image ls | grep themisdb

# Sollte zeigen:
# themisdb/themisdb   latest   ...   15 minutes ago   500MB

# Binary-Architektur prüfen
docker run --rm themisdb/themisdb:latest file /usr/local/bin/themis_server

# Sollte zeigen:
# /usr/local/bin/themis_server: ELF 64-bit LSB executable, x86-64
# (NICHT: PE32+ executable for MS Windows)
```

### Metriken testen
```bash
# Container starten
docker run -d --name themis-test -p 8080:8080 themisdb/themisdb:latest

# Warte 10 Sekunden
sleep 10

# Metriken abrufen
curl http://localhost:8080/metrics | head -20

# Cleanup
docker stop themis-test && docker rm themis-test
```

---

## 📚 Weitere Dokumentation

- [SETUP_COMPLETE.md](../../ARCHIVED/implementation-summaries/SETUP_COMPLETE.md) - Vollständiges Setup
- [docker/compose/README.md](docker/compose/README.md) - Docker Compose Setup
- [docker/compose/QUICK_START.md](docker/compose/QUICK_START.md) - Schnellanleitung

---

**Stand:** 3. April 2026  
**Dockerfile:** `Dockerfile.themis-server` (konsolidiert)  
**Editionen:** Alle über Build-Args  
**Metriken:** In allen Editionen aktiviert ✅
