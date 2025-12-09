# THEMIS v1.0.1 Release - December 9, 2025

## 🎉 Release Complete

All release packages for THEMIS v1.0.1 are ready for deployment.

---

## 📦 Three Package Types Available

### 🌟 **Production-Ready** (RECOMMENDED)
Extract → Run → Done! Everything included and ready to go.

- `themis-1.0.1-windows-x64-prod.zip` (12.66 MB)
- `themisdb-1.0.1-linux-x64-prod.zip` (25.10 MB)
- `themisdb-1.0.1-qnap-x64-prod.zip` (37.54 MB)

**Features:**
- ✓ Binary + configuration files ready to use
- ✓ All runtime directories created
- ✓ Startup scripts included (`scripts/start-themis.sh` / `.bat`)
- ✓ Documentation included
- ✓ Examples included
- ✓ Config templates for reference

**Start here:** See [v1.0.1-RELEASE_GUIDE.md](v1.0.1-RELEASE_GUIDE.md)

---

### 📚 **Complete** (With Documentation)
For learning, reference, and understanding the system.

- `themis-1.0.1-windows-x64-complete.zip` (20.14 MB)
- `themisdb-1.0.1-linux-x64-complete.zip` (17.81 MB)
- `themisdb-1.0.1-qnap-x64-complete.zip` (17.58 MB)

**Contains:**
- Binary executable
- Full documentation (docs/)
- Examples and sample code
- Configuration templates
- Helper scripts

---

### 📁 **Minimal** (Binary Only)
For container deployments, CI/CD, and existing setups.

- `themis-1.0.1-windows-x64.zip` (12.44 MB)
- `themisdb-1.0.1-linux-x64.zip` (10.18 MB)
- `themisdb-1.0.1-qnap-x64.zip` (12.44 MB)
- `themisdb_1.0.1_amd64.deb` (7.93 MB)
- `themisdb-1.0.1-1.x86_64.rpm` (10.17 MB)

**Contains:**
- Binary executable only

---

## ✅ Verification

All packages include SHA256 checksums. Verify integrity:

```bash
# Production packages
sha256sum -c SHA256SUMS_v1.0.1_prod.txt

# Complete packages
sha256sum -c SHA256SUMS_v1.0.1_complete.txt

# Minimal packages
sha256sum -c SHA256SUMS_v1.0.1.txt
```

---

## 🚀 Quick Start (Production Package)

```bash
# 1. Extract
unzip themisdb-1.0.1-linux-x64-prod.zip
cd themis-1.0.1-prod

# 2. Start server
chmod +x scripts/start-themis.sh          # Linux/QNAP
./scripts/start-themis.sh start

# On Windows
# .\scripts\start-themis.bat start

# 3. Verify
curl http://localhost:8080/health

# Done! Server is running
```

---

## 📖 Documentation Files

### In This Directory
- **v1.0.1-RELEASE_GUIDE.md** - Complete guide (START HERE!)
- **v1.0.1-PACKAGE_CONTENTS.md** - Detailed package contents
- **SHA256SUMS_v1.0.1_prod.txt** - Checksums for production packages
- **SHA256SUMS_v1.0.1_complete.txt** - Checksums for complete packages
- **SHA256SUMS_v1.0.1.txt** - Checksums for minimal packages

### In Production Packages
- **INSTALLATION.md** - Detailed installation guide
- **README.md** - Project information
- **CHANGELOG.md** - Version history
- **docs/** - Full documentation
- **examples/** - Sample code and configurations

---

## 📊 Release Statistics

| Category | Size | Packages |
|----------|------|----------|
| Production | 75.29 MB | 3 ZIP files |
| Complete | 55.54 MB | 3 ZIP files |
| Minimal | 53.16 MB | 5 files (3 ZIP + DEB + RPM) |
| **Total** | **183.99 MB** | **11 ZIP + 2 DEB/RPM** |

---

## 🎯 Which Package Should You Choose?

### Production-Ready ⭐ (Most Users)
- First-time setup
- Standalone deployment
- Production environment
- Team installation
- Want everything working out-of-the-box

### Complete
- Learning/evaluation
- Developer reference
- Understanding architecture
- Integration projects

### Minimal
- Container/Docker
- CI/CD pipelines
- Existing setups
- Minimal disk space

---

## ⚙️ Production Package Structure

```
themis-1.0.1-prod/
├── bin/
│   └── themis_server              ← Binary
├── data/                          ← ✓ Created (storage)
├── logs/                          ← ✓ Created (logs)
├── cache/                         ← ✓ Created (cache)
├── plugins/                       ← ✓ Created (processors)
├── backups/                       ← ✓ Created (backups)
├── temp/                          ← ✓ Created (temp files)
├── config.json                    ← ✓ Ready (main config)
├── mime_types.yaml                ← ✓ Ready (MIME types)
├── content_processors.yaml        ← ✓ Ready (processors)
├── config-templates/              ← Reference configs
├── docs/                          ← Full documentation
├── scripts/                       ← Helper & startup scripts
│   ├── start-themis.sh            ← Startup script (Linux/QNAP)
│   ├── start-themis.bat           ← Startup script (Windows)
│   ├── backup-incremental.sh
│   ├── restore.sh
│   └── ...
├── examples/                      ← Sample code
├── INSTALLATION.md                ← Setup guide
├── README.md
├── CHANGELOG.md
├── LICENSE
└── VERSION
```

**All directories are pre-created and ready to use!**

---

## 🔧 Platforms Supported

- ✓ **Windows** (x64)
- ✓ **Linux** (x64, Ubuntu/Debian compatible)
- ✓ **QNAP** (NAS optimization)
- ✓ **Raspberry Pi** (config templates included)

---

## 🛠️ Build Information

- **Version**: 1.0.1
- **Build Date**: December 9, 2025
- **Compiler**: MSVC 19.44 (Windows), GCC (Linux)
- **C++ Standard**: C++17
- **Status**: ✓ Production Ready

---

## 📝 What's New in v1.0.1

See `CHANGELOG.md` in any package for detailed changes.

Key improvements:
- ✓ All compiler errors fixed
- ✓ C4 warnings resolved
- ✓ IntelliSense diagnostics optimized
- ✓ Platform compatibility improved
- ✓ Build optimizations

---

## 🐛 Known Issues

None reported for v1.0.1

(See CHANGELOG.md for known issues in previous versions)

---

## 📞 Support

### Quick Start
1. Read: `v1.0.1-RELEASE_GUIDE.md`
2. Choose: Production, Complete, or Minimal package
3. Extract and follow instructions

### For Production Deployments
1. Extract production package
2. Read: `INSTALLATION.md` (included in package)
3. Customize: `config.json`
4. Run: `./scripts/start-themis.sh start`

### Troubleshooting
See `INSTALLATION.md` in the production package for detailed troubleshooting.

---

## ✨ Features Included

### Core
- ✓ Full database engine
- ✓ Vector search with HNSW
- ✓ Geospatial queries
- ✓ Time-series data
- ✓ Data sharding
- ✓ Replication

### Content Processing
- ✓ Document extraction (PDF, DOCX, etc.)
- ✓ Image processing
- ✓ Video processing
- ✓ Audio processing
- ✓ Geospatial data
- ✓ CAD/3D models

### Enterprise
- ✓ Role-based access control
- ✓ Audit logging
- ✓ Data governance
- ✓ Compliance tools
- ✓ Enterprise features (in config-templates)

### Observability
- ✓ Prometheus metrics
- ✓ Jaeger tracing
- ✓ Structured logging
- ✓ Performance monitoring

---

## 🚀 Getting Started

**Choose your package type and follow the appropriate guide:**

1. **Production** (Recommended)
   - Extract: `unzip themisdb-1.0.1-linux-x64-prod.zip`
   - Read: `INSTALLATION.md` inside
   - Run: `./scripts/start-themis.sh start`

2. **Complete**
   - Extract: `unzip themisdb-1.0.1-linux-x64-complete.zip`
   - Read: `INSTALLATION.md` inside
   - Create directories manually, then run binary

3. **Minimal**
   - Extract: `unzip themis-1.0.1-linux-x64.zip`
   - Provide your own config
   - Run: `./themis_server --config /path/to/config.json`

---

## 📦 Release Directory Contents

```
release/
├── README.md                              ← YOU ARE HERE
├── v1.0.1-RELEASE_GUIDE.md                ← START HERE for details
├── v1.0.1-PACKAGE_CONTENTS.md
├── v1.0.1-prod/                          ← Production package root (before ZIP)
├── v1.0.1-complete/                      ← Complete package root (before ZIP)
├── v1.0.1-linux-complete/                ← Linux complete root (before ZIP)
├── v1.0.1-qnap-complete/                 ← QNAP complete root (before ZIP)
│
├── [PRODUCTION PACKAGES] ⭐
│   ├── themis-1.0.1-windows-x64-prod.zip
│   ├── themisdb-1.0.1-linux-x64-prod.zip
│   ├── themisdb-1.0.1-qnap-x64-prod.zip
│   └── SHA256SUMS_v1.0.1_prod.txt
│
├── [COMPLETE PACKAGES]
│   ├── themis-1.0.1-windows-x64-complete.zip
│   ├── themisdb-1.0.1-linux-x64-complete.zip
│   ├── themisdb-1.0.1-qnap-x64-complete.zip
│   └── SHA256SUMS_v1.0.1_complete.txt
│
└── [MINIMAL PACKAGES]
    ├── themis-1.0.1-windows-x64.zip
    ├── themisdb-1.0.1-linux-x64.zip
    ├── themisdb-1.0.1-qnap-x64.zip
    ├── themisdb_1.0.1_amd64.deb
    ├── themisdb-1.0.1-1.x86_64.rpm
    └── SHA256SUMS_v1.0.1.txt
```

---

## 🎓 Next Steps

1. **Choose Your Package** → See comparison in `v1.0.1-RELEASE_GUIDE.md`
2. **Download** → Select from packages above
3. **Extract** → `unzip` or your archive manager
4. **Read** → Check `INSTALLATION.md` (in package) or guide
5. **Configure** → Edit `config.json` if needed
6. **Start** → Run startup script or binary
7. **Verify** → Check health endpoint
8. **Monitor** → Review logs and metrics

---

**Version**: 1.0.1  
**Release Date**: December 9, 2025  
**Status**: ✓ Ready for Production  

For complete information, see **v1.0.1-RELEASE_GUIDE.md**
