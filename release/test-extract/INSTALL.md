# ThemisDB v1.3.0 - Production Installation

## Quick Start

### Windows
```powershell
# Extract and start
Expand-Archive -Path themisdb-v1.3.0-windows-x64-production.zip
cd themisdb-1.3.0-production\bin
.\themis_server.exe --config ..\config\config.json
```

## Directory Structure
bin/       - Executables and DLLs
config/    - Configuration files
data/      - Database storage
docs/      - Documentation
logs/      - Log files
scripts/   - Admin scripts
tools/     - Utilities

## Admin Tools
.\scripts\admin\backup.ps1                    # Backup database
.\scripts\admin\restore.ps1 -BackupPath <dir> # Restore
.\scripts\monitoring\health-check.ps1         # Health check

## REST API
http://localhost:8765/health  - Health check
http://localhost:8765/metrics - Metrics

## Support
GitHub: https://github.com/makr-code/ThemisDB
