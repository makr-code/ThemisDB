# vcpkg Docker Cache Best Practices für ThemisDB

**Erstellt:** 15. Februar 2026  
**Ziel:** Minimale Builds durch optimales Caching von vcpkg Ressourcen

---

## 📊 Lokale Ressourcen (Analyse)

```
C:\VCC\themis\vcpkg\
├── downloads/       4.42 GB, 71.271 Dateien  ✅ MOUNTEN (spart Downloads)
├── buildtrees/     64.66 GB                  ❌ NICHT MOUNTEN (zu groß, temp)
├── packages/       18 GB (x64-windows)       ❌ NICHT KOMPATIBEL (Windows Symlinks)
└── installed/      (leer in vcpkg root)

C:\VCC\themis\vcpkg_installed\
└── x64-windows/    11.51 GB                  ❌ NICHT KOMPATIBEL (anderer Triplet)
```

**Problem:** 
- Lokale Packages sind `x64-windows` (Windows) 
- Docker braucht `x64-linux` (Linux)
- Windows Symlinks funktionieren nicht in Docker

**Lösung:**
- ✅ Mount `vcpkg/downloads` → spart 4.42 GB Downloads
- ✅ Mount vcpkg Binary Cache (wenn vorhanden)
- ✅ BuildKit Cache Mounts für Container-Persistenz
- ❌ Packages müssen neu kompiliert werden (x64-linux)

---

## 🎯 Best Practice: Triple-Cache-Strategie

### 1. BuildKit Container-Cache (persistent zwischen Builds)

```dockerfile
RUN --mount=type=cache,target=/opt/vcpkg/downloads,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/packages,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/buildtrees,sharing=locked
```

**Vorteil:** Docker speichert diese Verzeichnisse zwischen Builds

### 2. Host-Downloads Bind-Mount (readonly)

```dockerfile
RUN --mount=type=bind,source=vcpkg/downloads,target=/vcpkg-host-downloads,readonly
```

**Vorteil:** Nutzt Ihre 4.42 GB lokalen Downloads, keine GitHub-Downloads notwendig

### 3. vcpkg Binary Cache (optional - lokal leer)

```bash
export VCPKG_BINARY_SOURCES="clear;files,/opt/vcpkg/archives,readwrite"
```

**Vorteil:** Kompilierte Packages als .zip gespeichert, wiederverwendbar

---

## 🔧 Optimiertes Dockerfile (deps Stage)

```dockerfile
# Stage 2: deps - Install dependencies with optimal caching
FROM base AS deps

ARG THEMIS_EDITION
ARG TARGETARCH

WORKDIR /build

# Copy edition-specific vcpkg manifests
COPY docker/vcpkg-*.json ./
COPY vcpkg-configuration.json ./
COPY ports ./ports

# Select edition and detect architecture
RUN set -eux; \
    EDITION=$(echo "${THEMIS_EDITION}" | tr '[:upper:]' '[:lower:]'); \
    if [ -f "vcpkg-${EDITION}.json" ]; then \
        cp "vcpkg-${EDITION}.json" vcpkg.json; \
        echo "✓ Edition: ${THEMIS_EDITION}"; \
    else \
        echo "ERROR: vcpkg-${EDITION}.json not found"; exit 1; \
    fi; \
    case "${TARGETARCH}" in \
        amd64) TRIPLET="x64-linux" ;; \
        arm64) TRIPLET="arm64-linux" ;; \
        arm)   TRIPLET="arm-linux" ;; \
        *)     echo "ERROR: Unsupported arch ${TARGETARCH}"; exit 1 ;; \
    esac; \
    echo "${TRIPLET}" > /tmp/triplet.txt

# Install dependencies with TRIPLE CACHE:
# 1. BuildKit cache mounts (persistent Docker cache)
# 2. Host downloads bind-mount (your local 4.42 GB cache)
# 3. vcpkg binary cache (compiled packages as .zip)
RUN --mount=type=cache,target=/opt/vcpkg/downloads,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/packages,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/buildtrees,sharing=locked \
    --mount=type=bind,source=vcpkg/downloads,target=/vcpkg-host-downloads,readonly \
    set -eux; \
    TRIPLET=$(cat /tmp/triplet.txt); \
    mkdir -p /build/vcpkg_installed/${TRIPLET}; \
    \
    # Copy host downloads to container cache (one-time copy per package)
    if [ -d /vcpkg-host-downloads ] && [ "$(ls -A /vcpkg-host-downloads 2>/dev/null)" ]; then \
        echo "📦 Copying host downloads cache..."; \
        find /vcpkg-host-downloads -type f \( -name "*.tar.gz" -o -name "*.zip" -o -name "*.7z" \) \
            -exec cp -n {} /opt/vcpkg/downloads/ \; 2>/dev/null || true; \
        echo "✓ $(ls /opt/vcpkg/downloads | wc -l) files in downloads cache"; \
    fi; \
    \
    # Configure vcpkg
    export VCPKG_BINARY_SOURCES="clear;files,/opt/vcpkg/archives,readwrite"; \
    export VCPKG_BUILD_TYPE=release; \
    export VCPKG_MAX_CONCURRENCY=8; \
    \
    # Install packages (will use cache, only compile x64-linux variants)
    echo "📦 Installing packages for ${TRIPLET}..."; \
    ${VCPKG_ROOT}/vcpkg install \
        --triplet="${TRIPLET}" \
        --x-manifest-root=/build \
        --x-install-root=/build/vcpkg_installed \
        --allow-unsupported \
        --clean-after-build || { \
        echo "ERROR: vcpkg install failed"; \
        exit 1; \
    }; \
    \
    # Create symlinks for CMake
    ln -sf /build/vcpkg_installed/${TRIPLET}/include /build/include; \
    ln -sf /build/vcpkg_installed/${TRIPLET}/lib /build/lib; \
    \
    echo "✓ Dependencies complete: ${TRIPLET}"; \
    ls -lh /build/vcpkg_installed/${TRIPLET}/lib/ 2>/dev/null | head -20 || true
```

---

## 📈 Erwarteter Performance-Gewinn

### Ohne Cache
- Downloads: ~10-15 Min (4+ GB von GitHub)
- Compilation: ~30-45 Min (Boost, RocksDB, etc.)
- **Total: ~40-60 Min**

### Mit Host Downloads + BuildKit Cache
- Downloads: ~30 Sek (lokale Kopie)
- Compilation (1. Build): ~30-45 Min (x64-linux neu kompilieren)
- Compilation (2+ Builds): ~1-2 Min (BuildKit cache)
- **Total (1. Build): ~30-45 Min**
- **Total (2+ Builds): ~2-5 Min** 🚀

---

## 🚀 Build Command

```powershell
# Stelle sicher vcpkg/downloads existiert
if (-not (Test-Path "vcpkg\downloads")) {
    Write-Host "❌ vcpkg\downloads nicht gefunden!"
    exit 1
}

# Docker Build mit Cache-Mounts
docker buildx build `
    --build-arg THEMIS_EDITION=COMMUNITY `
    --progress=plain `
    -t themisdb:latest `
    -f Dockerfile `
    . 2>&1 | Tee-Object -FilePath docker-build-cached.log
```

**Wichtig:** 
- Docker BuildKit muss aktiviert sein: `$env:DOCKER_BUILDKIT=1`
- Source `vcpkg/downloads` muss relativ zum Build Context existieren

---

## ⚠️ Wichtige Hinweise

### 1. Windows vs Linux Packages
- **Problem:** Lokale `vcpkg_installed/x64-windows` Packages sind nicht kompatibel
- **Lösung:** Downloads wiederverwenden, aber Packages neu für x64-linux kompilieren

### 2. Symlinks in vcpkg/packages
- **Problem:** Windows Symlinks funktionieren nicht in Docker (WSL2 Storage)
- **Lösung:** Nicht mounten, nur downloads mounten

### 3. BuildKit Cache Persistenz
- Cache wird in Docker Desktop gespeichert
- Überlebt Container-Neustarts
- Bei Docker Desktop Neuinstallation: Cache verloren

### 4. Cache Invalidierung
- `vcpkg.json` Änderungen → vcpkg install neu
- `Dockerfile` RUN Layer Änderung → Cache ab dieser Zeile invalide
- Host downloads Änderungen → automatisch erkannt (mtime)

---

## 🔍 Debugging / Cache Verifikation

```powershell
# Zeige BuildKit Cache
docker buildx du

# Prüfe vcpkg downloads Cache
docker run --rm themisdb:latest ls -lh /opt/vcpkg/downloads | head -20

# Zeige installierte Packages
docker run --rm themisdb:latest ls /build/vcpkg_installed/x64-linux/lib
```

---

## 📚 Referenzen

- [vcpkg Binary Caching Docs](https://learn.microsoft.com/vcpkg/users/binarycaching)
- [Docker BuildKit Cache Mounts](https://docs.docker.com/build/cache/optimize/)
- [Reddit: vcpkg Docker Caching](https://www.reddit.com/r/cpp_questions/comments/1nb1cwo/)
- ThemisDB: `docker/DOCKER_BUILD_STRATEGY_QUICKREF.md`
