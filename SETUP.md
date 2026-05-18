# ThemisDB Development Setup

> Canonical **Step 2** after [QUICKSTART.md](QUICKSTART.md).
> This page defines the authoritative local development setup.

---

## Scope and Canonical Flow

Use the root onboarding path in this order:

1. [README.md](README.md)
2. [QUICKSTART.md](QUICKSTART.md)
3. [SETUP.md](SETUP.md) *(this page)*
4. [SUPPORT.md](SUPPORT.md)
5. [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md)
6. [INDEX.md](INDEX.md)

---

## Prerequisites

- Git 2.x
- Python 3.8+
- CMake 3.20+
- C++20 compiler (GCC/Clang/MSVC)
- PowerShell 7+ (for cross-platform dependency bootstrap script)

Optional for fastest onboarding:

- Docker / Docker Desktop
- VS Code + Dev Containers extension

---

## 1) Clone Repository

```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
```

---

## 2) Install Hooks and Third-Party Dependencies

### Linux / macOS

```bash
./scripts/setup-pre-commit.sh
pwsh ./scripts/setup-third-party.ps1
```

### Windows (PowerShell)

```powershell
.\scripts\setup-pre-commit.ps1
.\scripts\setup-third-party.ps1
```

---

## 3) Configure and Build with Canonical Presets

Available canonical build/test presets are defined in [CMakePresets.json](CMakePresets.json).

### Linux x64

```bash
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release --output-on-failure
```

### Windows x64 (VS Developer Command Prompt)

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release --output-on-failure
```

---

## 4) Run Server from Local Build

```bash
./build/linux-release/themis_server --data-dir ./data
```

Health check:

```bash
curl http://localhost:8765/health
```

---

## 5) Optional: Dev Container

1. Open repository in VS Code
2. Run `Dev Containers: Reopen in Container`
3. Build and test via the same canonical presets (`linux-release` / `linux-debug`)

---

## Deprecated / Non-Canonical Instructions

Legacy setup snippets using non-canonical preset names (for example `linux-gcc-release`, `linux-ninja-release`, `windows-vs2022-release`) are deprecated. Use the canonical preset names from [CMakePresets.json](CMakePresets.json).

---

## Need Help?

- [SUPPORT.md](SUPPORT.md)
- [docs/FAQ.md](docs/FAQ.md)
