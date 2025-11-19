# ThemisDB QNAP Deployment Guide

## Problem: GLIBC/GLIBCXX Version Mismatch

### Symptom
```
/usr/local/bin/themis_server: /lib/x86_64-linux-gnu/libc.so.6: version `GLIBC_2.38' not found
/usr/local/bin/themis_server: /lib/x86_64-linux-gnu/libstdc++.so.6: version `GLIBCXX_3.4.31' not found
```

### Root Cause
Binary was compiled against newer system libraries (Ubuntu 24.04+) than available on QNAP's runtime environment.

### Solution
Use **Ubuntu 20.04 LTS** as build base to ensure compatibility with older systems:
- GLIBC 2.31 (vs. 2.38 required previously)
- GLIBCXX 3.4.28 (vs. 3.4.31/32 required previously)

---

## Deployment Steps

### 1. Rebuild Docker Image (on Development Machine)

```powershell
# Windows (PowerShell)
.\rebuild-qnap.ps1 -Tag "qnap-v1.0"
```

```bash
# Linux/macOS
docker build --build-arg VCPKG_TRIPLET=x64-linux -t themis:qnap-v1.0 -f Dockerfile .
```

### 2. Export Image

```bash
docker save themis:qnap-v1.0 | gzip > themis-qnap-v1.0.tar.gz
```

### 3. Transfer to QNAP

Via SCP:
```bash
scp themis-qnap-v1.0.tar.gz admin@qnap-ip:/share/Container/themis/
```

Or use FileStation/WinSCP/Cyberduck.

### 4. Load Image on QNAP

SSH into QNAP:
```bash
ssh admin@qnap-ip
cd /share/Container/themis
docker load < themis-qnap-v1.0.tar.gz
```

### 5. Deploy with Docker Compose

```bash
cd /share/Container/themis
docker-compose -f docker-compose.qnap.yml up -d
```

### 6. Verify Deployment

```bash
docker logs themis
docker exec -it themis /usr/local/bin/themis_server --version
curl http://localhost:18765/health
```

---

## Configuration

### Persistent Data Volume
Ensure the volume mapping in `docker-compose.qnap.yml` points to a QNAP share:
```yaml
volumes:
  - /share/Container/themis/data:/data
```

### Custom Configuration
To override default config:
```yaml
volumes:
  - /share/Container/themis/config/config.qnap.json:/etc/vccdb/config.json:ro
```

### Environment Variables
- `THEMIS_PORT`: Server port (default: 18765)
- `TZ`: Timezone (default: Europe/Berlin)

---

## Troubleshooting

### Check GLIBC/GLIBCXX Versions in Container
```bash
docker run --rm themis:qnap-v1.0 ldd --version
docker run --rm themis:qnap-v1.0 strings /usr/lib/x86_64-linux-gnu/libstdc++.so.6 | grep GLIBCXX
```

### Check Runtime Dependencies
```bash
docker run --rm themis:qnap-v1.0 ldd /usr/local/bin/themis_server
```

### Rebuild with Static Linking (Alternative)
If compatibility issues persist, consider static linking:
```dockerfile
RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
    && cmake --build build -j
```

---

## Version Compatibility Matrix

| Build Base      | GLIBC | GLIBCXX | QNAP Compatible |
|-----------------|-------|---------|-----------------|
| Ubuntu 24.04    | 2.39  | 3.4.32  | ❌ No           |
| Ubuntu 22.04    | 2.35  | 3.4.30  | ⚠️ Maybe        |
| Ubuntu 20.04    | 2.31  | 3.4.28  | ✅ Yes          |
| Debian 11       | 2.31  | 3.4.28  | ✅ Yes          |
| Alpine (musl)   | N/A   | N/A     | ⚠️ Requires musl build |

---

## Update Strategy

1. Rebuild image with updated tag
2. Test locally: `docker run --rm -p 18765:18765 themis:new-tag`
3. Export and transfer to QNAP
4. Stop current container: `docker-compose -f docker-compose.qnap.yml down`
5. Load new image
6. Update `docker-compose.qnap.yml` image tag if needed
7. Start: `docker-compose -f docker-compose.qnap.yml up -d`

---

## Automated CI/CD (Future)

Consider GitHub Actions workflow to:
1. Build for `x64-linux` with Ubuntu 20.04 base
2. Push to container registry (Docker Hub/GHCR)
3. QNAP pulls image directly (no manual transfer)

Example registry pull:
```yaml
services:
  themis:
    image: ghcr.io/makr-code/themisdb:qnap-latest
    # ... rest of config
```
