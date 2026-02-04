# ThemisDB Operations Scripts

This directory contains automation scripts for operational tasks addressing audit findings FIND-020, FIND-028, FIND-030, FIND-032, FIND-023, and FIND-031.

## Scripts

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
