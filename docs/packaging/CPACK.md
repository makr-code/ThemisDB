# ThemisDB Local Packaging with CPack

This guide explains how to build **DEB** (Debian/Ubuntu), **MSI** (Windows via WiX), and
other distribution packages locally using CMake and CPack — no CI pipeline required.

## Deployment folder structure

The install layout is defined by `cmake/CMakeLists.txt` `install()` rules and
`cmake/ModularBuild.cmake`:

| Destination | Contents | Source |
|-------------|----------|--------|
| `bin/` | All runtime binaries: `themis_server`, `themisctl`, `themis-export`, `themis-model`, `config_migration_scanner` | `cmake/CMakeLists.txt` — `install(TARGETS themis_server …)` and CLI tool `install()` calls |
| `lib/` | Shared/static libraries: `themis_core` (`.so` / `.dll` / `.a`) | `cmake/CMakeLists.txt`, `cmake/ModularBuild.cmake` |
| `include/` | Public SDK headers | `cmake/CMakeLists.txt` |
| `data/` | Optional: `docs.db`, legal training data | `CMakeLists.txt` (controlled via `THEMIS_DOCS_DB_MODE`) |
| `models/` | Optional: LLM runtime models | `CMakeLists.txt` (controlled via `THEMIS_MODELS_MODE`) |
| `bin/benchmarks/` | Optional: benchmark binaries | `benchmarks/CMakeLists.txt` |

**During the build** (before install/packaging), `themis_server` is placed in
`<build-dir>/bin/` via the `RUNTIME_OUTPUT_DIRECTORY` target property
(search for `themis_server RUNTIME_OUTPUT_DIRECTORY` in `cmake/CMakeLists.txt`).

On **Linux DEB/RPM** packages the install prefix is `/usr`, so the package
layout becomes `/usr/bin/`, `/usr/lib/`, `/usr/include/`.  
On **ZIP/TGZ** archives the paths are relative, so unpacking anywhere gives
`bin/`, `lib/`, `include/`.

## Supported packaging formats

| Platform | Format | Generator | Tool required |
|----------|--------|-----------|---------------|
| Linux | TGZ (generic archive) | `TGZ` | — |
| Linux | DEB (Debian/Ubuntu) | `DEB` | `dpkg-dev` |
| Linux | RPM (Fedora/RHEL) | `RPM` | `rpmbuild` |
| Windows | ZIP (archive) | `ZIP` | — |
| Windows | MSI (WiX installer) | `WIX` | WiX Toolset v3 |
| Windows | NSIS installer | `NSIS` | NSIS (`makensis`) — auto-detected |
| macOS | TGZ | `TGZ` | — |
| macOS | ZIP | `ZIP` | — |
| All | Source TGZ/ZIP | `TGZ`/`ZIP` via `CPackSourceConfig.cmake` | — |

> **NSIS** is only added to the Windows generator list when `makensis` is found
> on `PATH` at configure time. It does not require any extra CMake files.

## Release artifact matrix

The project produces two layers of release artifacts:

1. the actual package artifacts from CPack, which are platform-specific installers and archives;
2. the GitHub release bundle used for publishing, validation, and downstream distribution.

### CPack package outputs

| Platform | Example artifact | Notes |
|----------|------------------|-------|
| Windows | `ThemisDB-<version>-Windows-x64.zip` | portable archive |
| Windows | `ThemisDB-<version>-Windows-x64.msi` | WiX installer |
| Windows | `ThemisDB-<version>-Windows-x64.exe` | optional NSIS installer if `makensis` is found |
| Linux | `themisdb_<version>_amd64.deb` | Debian/Ubuntu package |
| Linux | `themisdb-<version>.rpm` | RPM package |
| Linux | `themisdb-<version>.tar.gz` | generic Linux archive |
| All | `themisdb-<version>.tar.gz` | source tarball |
| All | `themisdb-<version>.zip` | source zip |

### GitHub release wrapper artifacts

The release workflow consolidates the platform packages into a publishable release bundle. In addition to the package files above, the release pipeline creates:

| Artifact | Purpose |
|----------|---------|
| `SHA256SUMS.txt` | checksum manifest for every uploaded package |
| `RELEASE_MANIFEST.txt` | machine-readable list of published artifacts |
| `themisdb-<edition>-<version>-linux-x64.tar.gz` | fallback archive created when the release bundle is assembled from raw build output |

These files are generated in the packaging job described in the release workflow and are uploaded to the GitHub Release as release assets, not as CPack-native package types.

### Distribution metadata and installer registries

The repository also keeps distribution metadata outside the core CPack step:

| Surface | Example |
|---------|---------|
| Winget manifests | `packaging/winget/manifests/.../ThemisDB.ThemisDB.installer.yaml` |
| Release metadata | `RELEASE_MANIFEST.txt`, `SHA256SUMS.txt` |
| Installer metadata | Windows WiX/MSI metadata from `CPACK_WIX_*` variables |

Winget metadata does not create a new binary package format by itself; it references real release artifacts and their hashes so that package managers can install the released ZIP/MSI outputs reliably.

## Prerequisites

### All Platforms

| Tool | Minimum version | Notes |
|------|-----------------|-------|
| CMake | 3.20 | Required |
| C++17 compiler | GCC 11 / Clang 14 / MSVC 2022 | Required |

### Linux (DEB/RPM)

| Tool | Purpose |
|------|---------|
| `dpkg-dev` / `dpkg-shlibdeps` | Automatic shared-library dependency resolution for DEB |
| `rpmbuild` | RPM package generation |

Install on Debian/Ubuntu:

```bash
sudo apt-get install -y dpkg-dev
```

### Windows (MSI via WiX)

WiX Toolset v3.x must be installed and its tools (`candle.exe`, `light.exe`) available on
`PATH`.  Download from: <https://wixtoolset.org/releases/>

CMake 3.20+ ships a built-in CPack WiX generator that calls these tools automatically.

### Windows (NSIS — optional)

NSIS (`makensis`) must be installed and available on `PATH`.
Download from: <https://nsis.sourceforge.io/>

CMake detects `makensis` at configure time and adds `NSIS` to the generator list
automatically when found.

---

## Quick-start

### 1. Configure

```bash
# Linux
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Windows (PowerShell)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Using a preset (recommended — see `CMakePresets.json`):

```bash
# Linux
cmake --preset linux-release

# Windows
cmake --preset windows-release
```

### 2. Build

```bash
cmake --build build --config Release -j
```

### 3. Package

Run from inside the build directory:

```bash
cd build
```

#### DEB (Debian/Ubuntu)

```bash
cpack -G DEB -V
```

The generated package will be named following Debian conventions, e.g.:

```
themisdb_1.9.0-alpha_amd64.deb
```

Install it with:

```bash
sudo dpkg -i themisdb_*.deb
```

#### RPM (Fedora/RHEL/CentOS)

```bash
cpack -G RPM -V
```

#### TGZ / ZIP (generic archive)

```bash
cpack -G TGZ   # Linux/macOS
cpack -G ZIP   # all platforms
```

#### MSI (Windows — WiX required)

```powershell
cpack -G WIX -V
```

The generated installer will be named, for example:

```
ThemisDB-1.9.0-alpha-Windows-x64.msi
```

#### NSIS installer (Windows — NSIS required, auto-detected)

```powershell
cpack -G NSIS -V
```

#### Source archive

```bash
cpack --config CPackSourceConfig.cmake -G TGZ
```

---

## Configuration overview

All CPack settings are defined in the root `CMakeLists.txt` **before** the
`include(CPack)` call.  Key variables:

| Variable | Description |
|----------|-------------|
| `CPACK_PACKAGE_NAME` | Package name (`themisdb`) |
| `CPACK_PACKAGE_VERSION` | Version read from `VERSION` file |
| `CPACK_PACKAGE_VENDOR` | Vendor/publisher name |
| `CPACK_PACKAGE_CONTACT` | Maintainer contact |
| `CPACK_GENERATOR` | Active generators (platform-dependent) |
| `CPACK_PACKAGING_INSTALL_PREFIX` | `/usr` on Linux (DEB/RPM standard) |

### DEB-specific

| Variable | Description |
|----------|-------------|
| `CPACK_DEBIAN_PACKAGE_MAINTAINER` | Maintainer field in `control` |
| `CPACK_DEBIAN_PACKAGE_SECTION` | Section (e.g. `database`) |
| `CPACK_DEBIAN_PACKAGE_PRIORITY` | Priority (e.g. `optional`) |
| `CPACK_DEBIAN_PACKAGE_DEPENDS` | Hard dependencies |
| `CPACK_DEBIAN_PACKAGE_SHLIBDEPS` | Auto-detect shared-library deps |
| `CPACK_DEBIAN_FILE_NAME` | `DEB-DEFAULT` → standard Debian naming |

### WiX/MSI-specific

| Variable | Description |
|----------|-------------|
| `CPACK_WIX_UPGRADE_GUID` | **Stable** GUID — must NOT change between releases |
| `CPACK_WIX_PRODUCT_GUID` | Per-build product code (`*` = auto-generate) |
| `CPACK_WIX_PRODUCT_NAME` | Display name in Add/Remove Programs |
| `CPACK_WIX_PROPERTY_ARPURLINFOABOUT` | "About" URL in ARP |

> **Important:** `CPACK_WIX_UPGRADE_GUID` is intentionally fixed.  Windows uses
> this GUID to identify upgrades.  If it changes, Windows will treat the new
> version as a different product, leaving the old one installed side-by-side.

---

## Troubleshooting

### `dpkg-shlibdeps: error: …`

`CPACK_DEBIAN_PACKAGE_SHLIBDEPS` is `ON` by default.  If `dpkg-shlibdeps` is not
available (e.g. cross-compile environment), either:

- Install `dpkg-dev`: `sudo apt-get install dpkg-dev`, or  
- Disable it temporarily: `cmake -DCPACK_DEBIAN_PACKAGE_SHLIBDEPS=OFF …`

### WiX: `candle.exe` / `light.exe` not found

Ensure WiX Toolset v3 is installed and its `bin/` directory is on `PATH`, then
re-run `cpack -G WIX`.

### NSIS not added automatically

NSIS is only enabled when `makensis` is found at CMake configure time.
Install NSIS and re-run `cmake -S . -B build …` to enable it.

### Wrong package metadata

If you see stale metadata after changing `CMakeLists.txt`, delete the build
directory and reconfigure:

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cd build && cpack -G DEB
```

---

## Relation to `debian/` directory

The repository also contains a `debian/` directory for **debhelper**-style
packaging (`dpkg-buildpackage`).  This is a separate workflow intended for
upstream Debian/Ubuntu submission and is **not** used by CPack.

Use `dpkg-buildpackage` only when preparing packages for Debian/Ubuntu
official repositories.  For all other local/CI deployment scenarios, use
CPack as described above.

---

## See also

- [DEPLOYMENT.md](DEPLOYMENT.md) — best-practice deployment guide for Windows, Linux distros, and macOS (systemd, launchd, Windows Services, security hardening, upgrades)
