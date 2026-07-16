# ThemisDB Operations Scripts

This directory contains automation scripts for operational tasks addressing audit findings FIND-020, FIND-028, FIND-030, FIND-032, FIND-023, and FIND-031.

## Scripts

### Invoke-LocalProductionReadiness.ps1

**Purpose:** Execute production-readiness gates locally without CI integration  
**Addresses:** System-wide readiness gates in `ROADMAP.md` (chaos/fault-injection, SLA validation proxy, penetration-test evidence)  
**Schedule:** On-demand before release cut

**Usage:**
```powershell
# Local gate run (no CI)
pwsh -File scripts/operations/Invoke-LocalProductionReadiness.ps1 \
	-BuildPreset windows-release \
	-RepeatCount 20

# Optional pentest execution
pwsh -File scripts/operations/Invoke-LocalProductionReadiness.ps1 \
	-BuildPreset windows-release \
	-RepeatCount 20 \
	-RunPentest \
	-PentestTarget "127.0.0.1:8080"
```

**Outputs:**
- `artifacts/production-readiness/<timestamp>/openapi-completeness.json`
- `artifacts/production-readiness/<timestamp>/content_focused_ctest.log`
- `artifacts/production-readiness/<timestamp>/content_focused_ctest.junit.xml`
- `artifacts/production-readiness/<timestamp>/content_processor_coverage.json`
- `artifacts/production-readiness/<timestamp>/content_bench_build.log`
- `artifacts/production-readiness/<timestamp>/content_bench_run.log`
- `artifacts/production-readiness/<timestamp>/content_bench_content_versioning.json`
- `artifacts/production-readiness/<timestamp>/content_bench_text_run.log`
- `artifacts/production-readiness/<timestamp>/content_bench_text_extraction.json`
- `artifacts/production-readiness/<timestamp>/content_bench_processor_run.log`
- `artifacts/production-readiness/<timestamp>/content_bench_processor_paths.json`
- `artifacts/production-readiness/<timestamp>/geo_bench_build.log`
- `artifacts/production-readiness/<timestamp>/geo_bench_run.log`
- `artifacts/production-readiness/<timestamp>/geo_cpu_gpu_parity.json`
- `artifacts/production-readiness/<timestamp>/phase4_ctest.log`
- `artifacts/production-readiness/<timestamp>/phase4_ctest.junit.xml`
- `artifacts/production-readiness/<timestamp>/beta_modules.txt`
- `artifacts/production-readiness/<timestamp>/readiness-summary.json`
- `artifacts/production-readiness/<timestamp>/readiness-summary.md`

Optional flags:
- `-SkipOpenApiGate` to skip OpenAPI completeness check
- `-SkipContentFocusedTests` to skip focused content evidence gate
- `-SkipContentCoverage` to skip content processor coverage gate
- `-SkipContentBenchmarks` to skip local content benchmark gate
- `-SkipGeoGate` to skip geo readiness/parity gate
- `-SkipPhase4Tests` to skip phase4 stress gate
- `-SkipPentest` to skip penetration-report gate

Optional content benchmark thresholds:
- `-ContentProcessorCoverageMinPercent` minimum processor coverage percentage based on dedicated test files (default: `80.0`)
- `-ContentBenchmarkMinCount` minimum benchmark entry count in JSON (default: `18`)
- `-ContentFormatBenchmarkMinCount` minimum benchmark entry count for text extraction JSON (default: `20`)
- `-ContentVersionCreationMaxMs` max `BM_VersionCreation/1048576` real time in ms (default: `50.0`)
- `-ContentDiffComputationMaxMs` max `BM_DiffComputation/1048576` real time in ms (default: `50.0`)
- `-ContentVersionRetrievalMaxUs` max `BM_VersionRetrieval` real time in microseconds (default: `5.0`)
- `-ContentPdfExtractionMaxMs` max `BM_PDFExtraction/1048576` real time in ms (default: `500.0`)
- `-ContentDocxExtractionMaxMs` max `BM_DOCXExtraction/1048576` real time in ms (default: `500.0`)
- `-ContentHtmlExtractionMaxMs` max `BM_HTMLExtraction/1048576` real time in ms (default: `500.0`)
- `-ContentPlainTextExtractionMaxMs` max `BM_PlainTextExtraction/1048576` real time in ms (default: `500.0`)
- `-ContentProcessorBenchmarkMinCount` minimum benchmark entry count for processor-path JSON (default: `12`)
- `-ContentOfficeProcessorPathMaxMs` max `BM_OfficeProcessorPath/1048576` real time in ms (default: `750.0`)
- `-ContentOcrProcessorPathMaxMs` max `BM_OcrProcessorPath/1048576` real time in ms (default: `750.0`)
- `-ContentArchiveProcessorPathMaxMs` max `BM_ArchiveProcessorPath/1048576` real time in ms (default: `750.0`)

Optional geo benchmark thresholds:
- `-GeoBenchmarkMinCount` minimum benchmark entry count for geo parity JSON (default: `25`)

### check_openapi_completeness.py

**Purpose:** Compares route hints (`// GET /...`) in `src/server/**/*.cpp` against documented paths in `openapi/openapi.yaml`  
**Usage:** Called by `Invoke-LocalProductionReadiness.ps1` and writes `openapi-completeness.json`

### PRODUCTION_READINESS_TODO.md

**Purpose:** Release-manager checklist for local (non-CI) production-readiness sign-off  
**Usage:** `scripts/operations/PRODUCTION_READINESS_TODO.md` as execution board + artifact checklist

### BETA_MODULE_GRADUATION_TODO.md

**Purpose:** Source-based exit checklist to graduate currently beta-marked modules to production-ready  
**Usage:** `scripts/operations/BETA_MODULE_GRADUATION_TODO.md` as per-module release gate board

### access-review.sh

**Purpose:** Automate access reviews and generate compliance reports  
**Addresses:** FIND-020 - Manual Access Reviews  
**Schedule:** Monthly (via GitHub Actions)

**Usage:**
```bash
# Generate monthly report
./access-review.sh --report

# Generate quarterly compliance report
./access-review.sh --compliance-report

# Export user access matrix
./access-review.sh --export-matrix

# Review specific user
./access-review.sh --user john.doe
```

**Requirements:**
- Bash 4.0+
- Standard Unix utilities (date, cat, grep)
- Write permissions to reports directory

**Portability:** Cross-platform compatible (Linux, macOS, BSD)

## GitHub Actions Workflows

The following workflows automate operational processes:

### access-review.yml

- **Schedule:** Monthly on 1st at 09:00 UTC
- **Purpose:** Automated access reviews
- **Outputs:** Monthly/quarterly reports, CSV exports
- **Platform:** Ubuntu (GitHub Actions)

### dr-testing.yml

- **Schedule:** Weekly on Sunday at 03:00 UTC
- **Purpose:** Automated DR testing with RTO/RPO measurement
- **Outputs:** DR test reports, metrics
- **Platform:** Ubuntu (GitHub Actions)

### incident-drill.yml

- **Schedule:** Monthly on 15th at 10:00 UTC
- **Purpose:** Automated incident response drills
- **Outputs:** Drill reports, performance metrics
- **Platform:** Ubuntu (GitHub Actions)

## Platform Compatibility

### Local Execution

All shell scripts in this directory use portable POSIX-compliant commands and work on:
- ✅ Linux (any distribution)
- ✅ macOS (BSD userland)
- ✅ BSDs (FreeBSD, OpenBSD, NetBSD)
- ✅ WSL (Windows Subsystem for Linux)

### CI/CD Execution

GitHub Actions workflows run on Ubuntu and use GNU coreutils. They may use GNU-specific features like:
- `date` with `%N` (nanoseconds) and `-d` (date arithmetic)
- These are acceptable as workflows always run on Ubuntu runners

## Configuration

Scripts use configuration from:
- `config/operations/logging.yaml` - Logging configuration
- `config/operations/access-review.yaml` - Access review settings (if exists)
- Environment variables for runtime overrides

## Reports

All scripts generate reports in:
- `reports/access-reviews/` - Access review reports
- `reports/dr-tests/` - DR test reports  
- `reports/incident-drills/` - Incident drill reports

## Metrics

Scripts export Prometheus metrics:
- `access_review_*` - Access review metrics
- `dr_*` - DR testing metrics (RTO/RPO)
- `incident_drill_*` - Incident drill metrics

## Logging

Scripts log to:
- `/var/log/themisdb/access-review.log`
- Console output (always)

Create log directory before first run:
```bash
sudo mkdir -p /var/log/themisdb
sudo chown $USER:$USER /var/log/themisdb
```

## Testing

Test scripts in dry-run mode:
```bash
./access-review.sh --report --dry-run
```

## Documentation

See comprehensive documentation in:
- `docs/operations/OPERATIONS_HANDBOOK.md` - Main operations guide
- `docs/operations/access-management/` - Access management docs
- `docs/operations/incident-response/` - Incident response docs
- `docs/operations/disaster-recovery/` - DR docs
- `docs/operations/logging/` - Logging docs

## Compliance

All scripts implement controls for:
- **ISO 27001:** A.9.2.5, A.9.2.6, A.12.4, A.16, A.17.1
- **BSI C5:** OIS-03, OIS-04, OIS-01 to OIS-04, BCR-01, LOG-01 to LOG-03
- **GDPR:** Article 32 (Security of Processing)
- **SOC 2:** CC6.3, CC7.1, CC9.1

## Support

For issues or questions:
1. Check documentation in `docs/operations/`
2. Review script comments and usage information
3. Check GitHub Actions workflow runs for CI/CD issues
4. Contact operations team

---

**Version:** 1.5.0  
**Last Updated:** 2026-02-03
