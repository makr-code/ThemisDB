# Git Repository Cleanup Guide

## Overview

ThemisDB maintains strict repository hygiene to ensure fast clones, efficient CI/CD, and clean commit history. This guide documents the cleanup practices and how to avoid common issues.

## Large Files Exclusion Policy

### Excluded File Types

The following patterns are **permanently excluded** from the repository via `.gitignore` and `.gitattributes`:

#### 1. **Test Data (> 100MB)**
- `testdata*/` - Development test datasets (often 5GB+)
- `test_data_*.json` - Generated test data files
- `test_data_*.csv` - CSV test exports
- `test_data_*.db` / `*.sst` - RocksDB test instances

**Why:** Test data is ephemeral and should be generated at runtime or fetched from external sources (e.g., Hugging Face datasets).

#### 2. **Benchmark Results**
- `benchmark_results/` - JSON/CSV benchmark outputs
- `benchmarks/benchmark_results/` - Benchmark execution results
- `benchmarks/*.json` / `benchmarks/*.csv` - Individual benchmark metrics

**Why:** Benchmark results vary by hardware and are better tracked in external dashboards (e.g., DataDog, Prometheus).

#### 3. **Build Artifacts** (already in .gitignore)
- `build*/` - CMake build directories
- `*.lib`, `*.a`, `*.o`, `*.obj` - Object files
- `*.exe`, `*.dll`, `*.so` - Binaries

### Size Limits

Repository size guidelines:
- **Uncompressed repo**: Should stay **< 500 MB** for fast clones
- **Large files**: Never commit files > 5 MB
- **Total history**: Aim for < 1 GB compressed

## Pre-Commit Checklist

Before pushing code, verify:

```powershell
# Check for large files being added
git diff --cached --name-only | ForEach-Object {
    $size = (git ls-files -s $_).Split()[3]
    if ($size -gt 5242880) { Write-Warning "Large file detected: $_ ($([math]::Round($size/1MB,2))MB)" }
}

# List all cached files with sizes
git diff --cached --name-only -z | xargs -0 -I {} sh -c 'echo "{}"; git ls-files -s "{}" | awk "{print \$4}"' | paste - - | awk '$2 > 1048576 {print $1 " - " $2 " bytes"}'
```

## Handling Test Data

### Development Workflow

For local testing and benchmarking:

```bash
# 1. Generate test data locally (NOT in repo)
./scripts/generate_test_data.sh --size 100K --output /tmp/test_data

# 2. Run benchmarks against local data
./benchmarks/run_complete_benchmarks.py --data-path /tmp/test_data

# 3. Store results in LOCAL directory (NOT committed)
mkdir -p ~/.themis_benchmark_results
cp benchmark_results/complete_benchmark_latest.json ~/.themis_benchmark_results/
```

### Docker Testing

For containerized testing:

```bash
# Mount test data as volume (NOT in image)
docker run -v /tmp/test_data:/data themis_benchmarks \
  --data-path /data \
  --output /results

# Copy results back
docker cp <container_id>:/results ./benchmark_results_tmp
```

## Git Cleanup Operations

### 1. Remove Accidentally Committed Large Files

```bash
# Option A: If file is still in staging
git reset HEAD <large-file>
git rm --cached <large-file>

# Option B: If file was committed (rewrite history - dangerous!)
git filter-branch --tree-filter 'rm -f <large-file>' HEAD
# Then: git push -f (coordinate with team!)

# Option C: Using BFG Repo Cleaner (safer alternative)
bfg --delete-files <filename> .
git reflog expire --expire=now --all && git gc --prune=now --aggressive
```

### 2. Verify .gitignore Changes Apply

```bash
# Apply new .gitignore rules
git rm --cached -r .
git add .
git commit -m "Apply gitignore rules"

# Verify large files are not tracked
git ls-files | grep -E "(testdata|benchmark|test_data)" | wc -l
# Should output: 0
```

### 3. Monitor Repository Size

```bash
# Check current size
git count-objects -v
# Output: count: 45678
#         size: 123456 (kilobytes)
#         in-pack: 34567
#         ...

# Optimize repository
git gc --aggressive --prune=now
```

## CI/CD Integration

### GitHub Actions

```yaml
name: Validate Repository

on: [push, pull_request]

jobs:
  check-large-files:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Check for large files
        run: |
          find . -size +5M ! -path './.git/*' ! -path './build*' ! -path './vcpkg*' | \
          while read file; do
            echo "Error: Large file detected: $file"
            exit 1
          done
```

### Docker Build

```dockerfile
# .dockerignore - mirrors .gitignore
benchmark_results/
testdata*/
test_data_*
build-*/
vcpkg_installed/
*.test.db
```

## Recommended Tools

### 1. **Pre-commit Framework**
```bash
# Install pre-commit hook
pip install pre-commit
pre-commit install

# Create .pre-commit-config.yaml
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.4.0
    hooks:
      - id: check-added-large-files
        args: ['--maxkb=5000']
      - id: check-json
      - id: check-merge-conflict
```

### 2. **Git LFS** (if large files become necessary)
```bash
# Install Git LFS
brew install git-lfs  # macOS
# or
apt-get install git-lfs  # Linux

# Track large files
git lfs track "*.db"
git add .gitattributes
git commit -m "Enable Git LFS for database files"
```

### 3. **Repository Analyzer**
```bash
# Find the largest files in history
git rev-list --all --objects | \
sort -k2 | \
tail -10 | \
while read sha path; do
  echo $(git cat-file -s $sha | awk '{sum+=$1}; END {print sum}') $path
done | sort -k1 -n -r | head -10
```

## Storage Statistics

### Current Repository Status (2025-12-04)

| Metric | Value | Target |
|--------|-------|--------|
| Uncompressed Size | ~400 MB | < 500 MB ✓ |
| Compressed Size (.git) | ~85 MB | < 150 MB ✓ |
| Number of Files | 8,532 | < 15,000 ✓ |
| Largest Single File | 2.1 MB (docs PDF) | < 5 MB ✓ |
| Benchmark Results | excluded | 0 MB ✓ |
| Test Data | excluded | 0 MB ✓ |

### Build Directories (Not Tracked)

| Directory | Size | Status |
|-----------|------|--------|
| `build-msvc/` | 1.2 GB | ✓ Ignored |
| `build-wsl/` | 890 MB | ✓ Ignored |
| `vcpkg_installed/` | 3.4 GB | ✓ Ignored |
| `vcpkg/packages/` | 2.1 GB | ✓ Ignored |

## Troubleshooting

### Issue: Large File Accidentally Committed

**Symptom:** Git says you're trying to push a file > LFS threshold

**Solution:**
```bash
# Check what's in staging
git diff --cached --stat | head -20

# Unstage and remove
git reset HEAD <file>
rm <file>
echo "<file>" >> .gitignore
git add .gitignore
git commit -m "Add <file> to .gitignore"
```

### Issue: Push Rejected Due to File Size

**Symptom:** `fatal: The remote end hung up unexpectedly`

**Solution:**
```bash
# Check file sizes in commit
git show --stat HEAD

# If necessary, amend commit
git reset --soft HEAD~1
# Remove large file
git commit -m "Remove large test data"
```

### Issue: Repository Clone is Slow

**Symptom:** `git clone` takes > 5 minutes

**Solution:**
```bash
# Shallow clone for faster initial download
git clone --depth 1 https://github.com/makr-code/ThemisDB.git

# Later pull full history
git fetch --unshallow
```

## Best Practices

### Do's ✓
- ✓ Keep .gitignore and .gitattributes updated
- ✓ Use external storage (S3, GCS) for test data
- ✓ Store benchmark results in time-series DB (InfluxDB, Prometheus)
- ✓ Use Git hooks to prevent large file commits
- ✓ Document ephemeral data generation in README

### Don'ts ✗
- ✗ Commit test datasets (> 100 MB)
- ✗ Commit build artifacts (use CI/CD instead)
- ✗ Commit benchmark results (use dashboards instead)
- ✗ Use `git add .` without reviewing changes first
- ✗ Force push to `main` branch without coordination

## References

- [GitHub: Removing sensitive data from repository](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository)
- [Pro Git: Git Internals](https://git-scm.com/book/en/v2/Git-Internals-Maintenance-and-Data-Recovery)
- [Git LFS Documentation](https://git-lfs.github.com/)
- [BFG Repo Cleaner](https://rtyley.github.io/bfg-repo-cleaner/)

---

**Last Updated:** 2026-04-06  
**Maintainer:** ThemisDB Development Team
