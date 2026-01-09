# Docker Build Guide - Debug vs Release

## Best Practice: Stage-basierte Builds

Das Dockerfile unterstützt zwei Build-Modi:

### 1. Release Build (Production)
**Eigenschaften:**
- Minimales Image (~400-600 MB)
- Nur Runtime-Dependencies
- Non-root User
- Optimized Binary (Release-Flags)
- Health Checks

**Build:**
```bash
# Standard (ohne --target = release)
docker build \
  --platform linux/amd64 \
  --build-arg THEMIS_ENABLE_LLM=ON \
  --build-arg THEMIS_ENABLE_GPU=ON \
  -f docker/Dockerfile.themis-server \
  -t themis-server:release .

# Explizit mit --target
docker build \
  --target release \
  --platform linux/amd64 \
  -f docker/Dockerfile.themis-server \
  -t themis-server:release .
```

**Run:**
```bash
docker run -d \
  -p 18765:18765 \
  -p 8080:8080 \
  -v themis-data:/var/lib/themisdb \
  themis-server:release
```

---

### 2. Debug Build (Development)
**Eigenschaften:**
- Größeres Image (~2-3 GB)
- Alle Build-Tools enthalten (gcc, cmake, gdb, valgrind)
- Source-Code und Build-Artifacts im Image
- Debug-Symbols
- Debugging-Tools (strace, ltrace, tcpdump)
- Root-Zugriff möglich

**Build:**
```bash
docker build \
  --target debug \
  --platform linux/amd64 \
  --build-arg THEMIS_ENABLE_LLM=ON \
  --build-arg THEMIS_ENABLE_GPU=ON \
  -f docker/Dockerfile.themis-server \
  -t themis-server:debug .
```

**Run (mit privilegiertem Zugriff für Debugging):**
```bash
docker run -it \
  --cap-add=SYS_PTRACE \
  --security-opt seccomp=unconfined \
  -p 18765:18765 \
  -p 8080:8080 \
  -v themis-data:/var/lib/themisdb \
  themis-server:debug \
  /bin/bash

# Im Container:
gdb /build/build-deps/cmake/themis_server
valgrind --leak-check=full /build/build-deps/cmake/themis_server
```

---

## Vergleich

| Feature | Release | Debug |
|---------|---------|-------|
| Image Size | ~400-600 MB | ~2-3 GB |
| Build Tools | ❌ | ✅ gcc, cmake, make |
| Debugging Tools | ❌ | ✅ gdb, valgrind, strace |
| Source Code | ❌ | ✅ /build |
| Debug Symbols | ❌ | ✅ |
| User | themis (non-root) | root |
| Log Level | info | debug |
| Healthcheck | ✅ | ❌ |
| Sicherheit | Gehärtet | Entwicklungsmodus |

---

## BuildKit Cache Optimierungen

Beide Builds nutzen BuildKit Cache Mounts:

```bash
export DOCKER_BUILDKIT=1

# Mit externem Cache (schnellere Rebuilds)
docker build \
  --cache-from type=local,src=/tmp/docker-cache \
  --cache-to type=local,dest=/tmp/docker-cache \
  --target release \
  -f docker/Dockerfile.themis-server \
  -t themis-server:release .
```

---

## Multi-Arch Builds

```bash
# AMD64 + ARM64 gleichzeitig
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  --target release \
  -f docker/Dockerfile.themis-server \
  -t themis-server:release \
  --push .
```

---

## Empfehlungen

**Entwicklung (lokal):**
```bash
docker build --target debug -t themis:dev .
docker run -it --rm -v $(pwd):/build themis:dev bash
```

**CI/CD (Tests):**
```bash
docker build --target debug -t themis:test .
docker run --rm themis:test ctest --output-on-failure
```

**Production (Deployment):**
```bash
docker build --target release -t themis:1.4.0 .
docker tag themis:1.4.0 themis:latest
docker push themis:1.4.0
docker push themis:latest
```

---

## Troubleshooting

**Debug-Build schlägt fehl:**
- Prüfe BuildKit: `docker buildx version`
- Cache löschen: `docker builder prune`
- Logs: `docker build --progress=plain ...`

**Release-Image zu groß:**
- Prüfe: `docker images themis-server:release`
- Layer analysieren: `dive themis-server:release`
- Erwartete Größe: 400-600 MB

**Performance-Probleme:**
- Release nutzt `-O3` Optimierung
- Debug nutzt `-Og -g` für bessere Debuggability
- Für Profiling: `--target debug` + `perf`, `gprof`
