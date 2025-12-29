# GitHub Release Anleitung für v1.3.4

## 1. Push zu GitHub

```bash
# Push commit und tag
git push origin main
git push origin v1.3.4
```

## 2. GitHub Release erstellen

Gehe zu: https://github.com/YOUR_USERNAME/themis/releases/new

**Tag:** v1.3.4  
**Release Title:** ThemisDB v1.3.4 - Insert Performance Optimization 🚀

**Description:**

```markdown
# ThemisDB v1.3.4 - Insert Performance Optimization

Major performance improvements for bulk insert operations!

## 🎯 Highlights

- **23-77x faster** bulk inserts via new Batch Insert API
- **98.2% latency reduction** for 100-entity batches (810ms → 14.5ms)  
- **60-200x faster** index metadata lookups (<10 µs vs 600-2000 µs)
- Phase 1 & 2 performance goals **dramatically exceeded**

## 📊 Performance Results

| Batch Size | Before | After | Speedup | Latency Reduction |
|------------|--------|-------|---------|-------------------|
| **100 entities** | 810ms | 14.5ms | **23.4x** | **98.2%** |
| **1000 entities** | 3744ms | 311ms | **77.5x** | **91.7%** |

## 🆕 New Features

### Batch Insert API
```cpp
// Single API call for bulk inserts
std::vector<BaseEntity> entities = /* prepare 1000 entities */;
auto status = indexMgr->putBatch("users", entities);
// 77x faster than 1000 individual inserts!
```

### Automatic Metadata Cache
- In-memory cache for index configurations
- Eliminates 6 DB scans per insert
- Thread-safe with automatic invalidation
- No code changes required!

## 📚 Documentation

- [Full Release Notes](RELEASE_NOTES_v1.3.4.md)
- [Performance Analysis](V1_3_4_PERFORMANCE_ANALYSIS.md)
- [Batch Insert Results](BATCH_INSERT_PERFORMANCE_RESULTS.md)
- [Root Cause Analysis](INSERT_PERFORMANCE_DEEP_DIVE.md)

## 🐛 Bug Fixes

- Fixed WriteBatch commit with TransactionDB
- Removed compiler warnings for shadowing variables

## 📦 Installation

**Docker:**
```bash
docker pull YOUR_USERNAME/themis:1.3.4
docker run -p 7687:7687 -p 8080:8080 YOUR_USERNAME/themis:1.3.4
```

**From Source:**
```bash
git clone https://github.com/YOUR_USERNAME/themis.git
cd themis
git checkout v1.3.4
# See BUILD.md for build instructions
```

## 🔜 What's Next (v1.3.5)

- Extended batch operations (update, delete)
- Adaptive cache TTL
- Parallel batch processing

---

**Full Changelog:** [CHANGELOG.md](CHANGELOG.md)
```

## 3. Assets zum Release hinzufügen

Nach dem Erstellen des Releases, füge diese Dateien als Assets hinzu:

### Binaries (wenn verfügbar)
- `themis_server.exe` (Windows)
- `themis_server` (Linux)
- Benchmarks (optional)

### Dokumentation
- `RELEASE_NOTES_v1.3.4.md`
- `BATCH_INSERT_PERFORMANCE_RESULTS.md`

### Source Archive
GitHub erstellt automatisch:
- Source code (zip)
- Source code (tar.gz)

## 4. Docker Image

```bash
# Build
docker build -t YOUR_USERNAME/themis:1.3.4 .
docker tag YOUR_USERNAME/themis:1.3.4 YOUR_USERNAME/themis:latest

# Push
docker push YOUR_USERNAME/themis:1.3.4
docker push YOUR_USERNAME/themis:latest
```

## 5. Announcement

Nach dem Release:
- Tweet/Social Media Post
- Update README.md mit neuem Badge
- Notify Discord/Slack community
- Update Docker Hub description
