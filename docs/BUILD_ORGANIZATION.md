# ThemisDB Build System Organization

**Last Updated:** 12. Dezember 2025

## 📁 Directory Structure

### 🔨 Build Scripts (`.\scripts\`)

**Primary Entry Points:**
- `build.ps1` - Unified build orchestrator (Windows, Linux, Docker)
- `build-windows.ps1` - Windows MSVC build
- `build-docker.ps1` - Docker multi-arch build
- `build-linux.sh` - Linux GCC build
- `build.sh` - Linux shell wrapper
- `update-vcpkg-cache.ps1` - Automatic dependency cache update

**Quick Start from Root:**
```powershell
.\quick-build.ps1              # Windows MSVC (simplest)
.\scripts\build.ps1 -Target windows    # Windows (full control)
.\scripts\build.ps1 -Target docker     # Docker multi-arch
.\scripts\build.ps1 -Target all        # All platforms
```

### 🧭 Versionierung

- Die Datei `VERSION` ist die einzige Quelle der Wahrheit für die Versionsnummer.
- CMake liest die Version automatisch daraus (`project(Themis VERSION ...)`).
- Alle Build-/Release-Skripte verwenden standardmäßig den Inhalt der `VERSION`-Datei.

### 🔗 Linkage/Artefakt-Form

- Standard: dynamisch (EXE/ELF + DLL/.so). `THEMIS_CORE_SHARED=ON` ist Default.
- QNAP/Static: statisch via `THEMIS_STATIC_BUILD=ON` oder `THEMIS_QNAP_BUILD=ON` (erzwingt statischen Core).
- `THEMIS_CORE_SHARED=ON` (CMake): baut `themis_core` als Shared Library (DLL/.so).
    - Windows: automatische Symbol-Exporte via `WINDOWS_EXPORT_ALL_SYMBOLS` (keine `__declspec(dllexport)` notwendig).
    - Docker-Build bleibt monolithisch (vereinfacht das Runtime-Image).

### 🐳 Docker (`.\docker\`)

**Dockerfiles:**
- `Dockerfile` (Primary) - Multi-stage production build
- `Dockerfile.runtime` - Production runtime only
- `Dockerfile.qnap` - QNAP NAS optimized
- `Dockerfile.benchmark` - Benchmark suite
- `Dockerfile.*.minimal/.fast` - Experimental variants

**Docker Compose:**
- `docker-compose.yml` (Primary) - Standard setup
- `docker-compose.qnap.yml` - QNAP configuration
- `docker-compose.benchmark.yml` - Benchmark environment

**Usage:**
```bash
# Primary
docker-compose up

# QNAP
docker-compose -f docker-compose.qnap.yml up

# Multi-arch via buildx (via scripts)
.\scripts\build-docker.ps1 -Push
```

### 📚 Documentation (`.\docs\`)

**Build & Deployment:**
- `deployment/deployment_strategy.md` - **2.0.0** Unified build system
- `BUILD-SYSTEM.md` - High-level architecture
- `BUILDGUIDE.md` - Detailed build handbook
- `IMPLEMENTATION-SUMMARY.md` - What/Why/How

**Archive (`.\docs\archive\`):**
- Old session summaries, release notes (v1.0.0), audit reports
- Historical documentation preserved for reference

### 📦 Utilities (Root Level)

**Convenience Scripts** (remain in root for easy access):
- `quick-build.ps1` - One-click Windows build
- `setup.ps1` - Development environment setup
- `security-scan.ps1` - Security audit
- `sync-wiki.ps1` - Documentation sync to GitHub Wiki
- `start_server.ps1` - Quick server launch

### 🗂️ Other Directories

| Directory | Purpose |
|-----------|---------|
| `./src` | Source code (C++) |
| `./include` | Public headers |
| `./tests` | Unit tests |
| `./benchmarks` | Performance benchmarks |
| `./cmake` | CMake modules |
| `./examples` | Usage examples |
| `./tools` | Development tools |
| `./vcpkg` | Dependency manager |
| `./vcpkg\downloads\` | **Cached dependencies (~2GB)** |

---

## 🚀 Build Workflow

### Automatic Flow

```
┌─────────────────────────────────────────┐
│  .\scripts\build.ps1 -Target <platform> │
└────────────┬────────────────────────────┘
             │
             ├─→ Auto-calls: .\scripts\update-vcpkg-cache.ps1
             │   (Updates vcpkg\downloads\ with latest archives)
             │
             ├─→ Platform-Specific Build:
             │   ├─ Windows: build-windows.ps1
             │   ├─ Linux: build-linux.sh
             │   └─ Docker: build-docker.ps1
             │
             └─→ Binary Output:
                 ├─ Windows: build-msvc\Release\themis_server.exe
                 ├─ Linux: build-linux\themis_server
                 └─ Docker: themisdb/themisdb:v$(Get-Content VERSION).Trim()
```

### Cache Architecture

**Single Source of Truth:** `.\vcpkg\downloads\` (~2GB)
- Contains 119 pre-downloaded source archives
- Covers all triplets: x64-windows, x64-linux, arm64-linux
- Enables completely offline builds

**NOT Copied to Docker:**
- `vcpkg\packages\` (compiled binaries, platform-specific)
- `vcpkg\buildtrees\` (temporary build artifacts)

---

## ✅ Cleanup Summary (12.12.2025)

### ✓ Deleted from Root (20 files)
- `build-unified.ps1`, `build.ps1`, `build.sh` (old)
- `docker-build.ps1`, `docker-build.sh` (old)
- `build-deb.sh`, `build-rpm.sh`, `build-qnap.*` (old)
- `publish-all.ps1`, `validate_fixes.ps1` (debug)

### ✓ Cleaned from scripts/ (11 files)
- `build_and_push_docker.ps1`, `release_build.ps1` (old)
- `windows_build.ps1`, `wsl_build.ps1` (old)
- `*.cmd` variants (deprecated)

### ✓ Deleted Temp/Build Artifacts (50+ files)
- Build logs: `build*.log`, `docker*.log`, `cmake_config.log`
- Temp files: `tmp_*.json`, `*.tar`, binary artifacts
- Debug output

### ✓ Moved to docker/ (14 files)
- All `Dockerfile.*` variants
- `docker-compose.*.yml` variants

### ✓ Archived Old Sessions (20+ files)
- Moved to `docs/archive/`
- Preserves historical context

### ✓ Deleted Build Directories (8 dirs, ~25GB)
- `build-msvc`, `build-ninja`, `build-vs2022`, etc.
- Recreated on next build as needed

**Result:** Clean, organized Root with only essential files

---

## 📖 Entry Points Summary

| What | Where | Command |
|------|-------|---------|
| **Quick Windows Build** | Root | `.\quick-build.ps1` |
| **Full Control Build** | Root | `.\scripts\build.ps1 -Target <target>` |
| **Windows Only** | scripts/ | `.\scripts\build.ps1 -Target windows` |
| **Docker Multi-Arch** | scripts/ | `.\scripts\build.ps1 -Target docker -Push` |
| **Cache Update Only** | scripts/ | `.\scripts\update-vcpkg-cache.ps1` |
| **Manual Windows** | scripts/ | `.\scripts\build-windows.ps1` |
| **Manual Linux** | scripts/ | `.\scripts\build-linux.sh` |

---

## 🔧 Maintenance

### Adding New Build Scripts

1. **Create in `scripts/`** - Not in root
2. **Follow naming:** `build-<platform>-<variant>.ps1|.sh`
3. **Call cache-update:** `.\scripts\update-vcpkg-cache.ps1`
4. **Update `build.ps1`** - Add new target to orchestrator

### Adding New Dockerfiles

1. **Move to `docker/`** - Not root
2. **Naming:** `Dockerfile.<purpose>` (e.g., `Dockerfile.performance`)
3. **Reference from scripts** - Via `-f` flag if non-standard

### Old Documentation

1. **Move to `docs/archive/`** - Keep for reference
2. **Don't delete** - Historical context important
3. **Date clearly** - Use YYYYMMDD format

---

## 📊 Disk Space Saved

| Category | Size Freed | Notes |
|----------|-----------|-------|
| Build Dirs | ~25 GB | Temp, auto-recreate |
| Build Logs | ~2 GB | Archived/cleaned |
| Duplicate Scripts | ~50 MB | Old variants removed |
| Docker Files | ~5 MB | Moved to organization |
| **Total** | **~27 GB** | ✓ Now organized |

---

## 🎯 Next Steps

1. ✅ Review new structure with team
2. ✅ Update CI/CD to use `.\scripts\build.ps1` entry point
3. ✅ Document custom Docker builds (if any)
4. ✅ Archive additional experimental code (if needed)

---

**Version:** 2.0.0  
**Status:** Production-Ready  
**Last Cleanup:** 12. Dezember 2025
