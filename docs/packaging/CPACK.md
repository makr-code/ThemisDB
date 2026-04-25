# ThemisDB Local Packaging with CPack

This guide explains how to build **DEB** (Debian/Ubuntu), **MSI** (Windows via WiX), and
other distribution packages locally using CMake and CPack — no CI pipeline required.

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
