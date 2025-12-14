# Release Notes v1.1.0
\n**Datum:** 2025-12-14
\n**Highlights:**
- Windows Build Fix: RocksDB-Linking angepasst; stabiler statischer Build.
- Linux Build: Erfolgreich via WSL; Artefakte und Checksums generiert.
- Packaging: ZIP-Bundles und MANIFEST erstellt; Version aus VERSION.
- Docs: Troubleshooting für RocksDB-Windows; CHANGELOG aktualisiert.
\n**Artefakte:**
- Windows: release/themisdb-v1.1.0-windows-x64.exe, release/themis-1.1.0-windows-x64.zip
- Linux: release/themisdb-v1.1.0-linux-x64, release/themisdb-1.1.0-linux-x64.zip, release/themisdb-v1.1.0-linux-x64-libthemis_core.so
- Checksums: release/SHA256SUMS.txt
\n**Upgrade Hinweise:**
- Keine DB-Migrations erforderlich; kompatibel zu 1.0.x.
- Windows: statischer Build; DLL-Build dokumentiert in docs/troubleshooting.
