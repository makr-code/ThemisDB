# Compiler Security Hardening – ThemisDB

**Audit-Referenz:** SEC-CC-4  
**Severity:** High  
**Stand:** 2026-07-15  
**Version:** 1.5.0+  

---

## Überblick

ThemisDB aktiviert für alle **Release-Builds** standardmäßig Compiler- und Linker-Security-Hardening-Flags.
Diese schützen gegen häufige Klassen von Schwachstellen:

| Kategorie | Schutz |
|---|---|
| Stack-Buffer-Overflow | `-fstack-protector-strong` / `/GS` |
| Heap/Buffer-Overflow | `-D_FORTIFY_SOURCE=3` / `/sdl` |
| Stack-Heap-Collision | `-fstack-clash-protection` (wenn unterstützt) |
| Code-Injektion / ROP | PIE + ASLR (`-fPIE/-pie`, `/DYNAMICBASE`) |
| Return-Oriented Programming | RELRO (`-Wl,-z,relro,-z,now`) |
| Non-Executable Stack | `-Wl,-z,noexecstack` |
| Control-Flow-Hijacking | Control Flow Guard `/GUARD:CF` |
| Daten-Ausführung (DEP) | `/NXCOMPAT` |

---

## Aktivierte Flags nach Plattform

### Linux (GCC ≥ 8, Clang ≥ 11) – Release-Build

| Flag | Typ | Beschreibung |
|---|---|---|
| `-fstack-protector-strong` | Compile | Stack-Canary für Funktionen mit Puffern und Zeigern |
| `-D_FORTIFY_SOURCE=3` | Macro | Bounds-checked libc-Wrapper (benötigt `-O1+`; Level 3 seit GCC 12 / glibc 2.35) |
| `-fstack-clash-protection` | Compile | Verhindert Stack-Heap-Kollisions-Angriffe (optional, wenn unterstützt) |
| `-fPIE` | Compile | Position Independent Executable – kompilieren |
| `-pie` | Link | Position Independent Executable – linken (ASLR-Aktivierung) |
| `-Wl,-z,relro` | Link | Read-only Relocations nach Startup (RELRO) – **nur Linux** |
| `-Wl,-z,now` | Link | Full RELRO: sofortige PLT-Auflösung – **nur Linux** |
| `-Wl,-z,noexecstack` | Link | Stack nicht ausführbar markieren – **nur Linux** |

> **Hinweis:** `-fstack-protector-strong` und PIE sind **Pflichtflags**.
> Sind sie nicht verfügbar, bricht der Build mit FATAL_ERROR ab.
> `-fstack-clash-protection` und die RELRO-Flags sind Best-Effort (Warnung).
> Die RELRO-Linker-Flags (`-Wl,-z,relro`, `-Wl,-z,now`, `-Wl,-z,noexecstack`) sind
> **ausschließlich auf Linux** aktiv – macOS (`ld64`) unterstützt ELF-RELRO nicht.

### macOS (Clang ≥ 11) – Release-Build

| Flag | Typ | Beschreibung |
|---|---|---|
| `-fstack-protector-strong` | Compile | Stack-Canary für Funktionen mit Puffern und Zeigern |
| `-D_FORTIFY_SOURCE=3` | Macro | Bounds-checked libc-Wrapper (benötigt `-O1+`) |
| `-fstack-clash-protection` | Compile | Verhindert Stack-Heap-Kollisions-Angriffe (wenn unterstützt) |
| `-fPIE` | Compile | Position Independent Executable – kompilieren |
| `-pie` | Link | Position Independent Executable – linken (ASLR-Aktivierung) |

> **Hinweis:** RELRO-Linker-Flags sind auf macOS nicht verfügbar. `ld64` unterstützt
> keine ELF-spezifischen `-z`-Flags. ASLR und Stack-Schutz werden durch das macOS-
> Betriebssystem und `ld64` nativ bereitgestellt.

### Windows (MSVC ≥ VS 2019 / cl.exe) – Release-Build

| Flag | Typ | Beschreibung |
|---|---|---|
| `/GS` | Compile | Buffer Security Check (Stack-Canaries) |
| `/sdl` | Compile | Additional SDL-Checks (Security Development Lifecycle) |
| `/guard:cf` | Compile | Control Flow Guard – Compile-Zeit-Instrumentierung |
| `/GUARD:CF` | Link | Control Flow Guard – Enforcement |
| `/NXCOMPAT` | Link | Data Execution Prevention (DEP/NX) |
| `/DYNAMICBASE` | Link | Address Space Layout Randomization (ASLR) |

---

## CMake-Konfiguration

### Standard (Hardening aktiviert)

```bash
cmake --preset community-release
cmake --build --preset community-release
```

Die Hardening-Flags werden automatisch für `Release`-Builds aktiviert.
In der CMake-Ausgabe erscheinen entsprechende Statusmeldungen, z. B.:

```
-- Security Hardening (GCC/Clang): -fstack-protector-strong -D_FORTIFY_SOURCE=3 -fPIE/-pie -fstack-clash-protection  [Release]
--   Linker:  Full RELRO (-z relro -z now -z noexecstack)
```

### Debug-Builds

In `Debug`-Builds werden **keine** Hardening-Flags aktiviert, da:
- `-D_FORTIFY_SOURCE=3` einen Optimierungsgrad ≥ `-O1` erfordert
- der zusätzliche Overhead den Debug-Zyklus verlangsamt

Stattdessen stehen ASAN/LSAN/UBSan für Debug-Sicherheit zur Verfügung:

```bash
cmake --preset community-debug -DTHEMIS_ENABLE_ASAN=ON
```

### Hardening deaktivieren (NICHT für Produktion)

> ⚠️ **WARNUNG:** Das Deaktivieren ist nur für spezielle Umgebungen zulässig
> (z. B. Cross-Compile-Targets, die bestimmte Flags nicht unterstützen).
> **Produktions-Builds dürfen dies nicht verwenden.**

```bash
cmake --preset community-release -DTHEMIS_DISABLE_SECURITY_HARDENING=ON
```

Beim Build erscheint eine explizite Sicherheitswarnung. Der Einsatz in der
Produktion ist ein Verstoß gegen SEC-CC-4.

---

## Fehlschlag-Verhalten

| Situation | Verhalten |
|---|---|
| Pflicht-Compilerflag nicht unterstützt | `FATAL_ERROR` – Build bricht ab |
| Linker-RELRO-Flag nicht unterstützt | `WARNING` – Build wird fortgesetzt |
| `THEMIS_DISABLE_SECURITY_HARDENING=ON` | `WARNING` – Build wird fortgesetzt |

Pflicht-Flags (`-fstack-protector-strong`, PIE):
```
FATAL_ERROR: Security hardening compile flag '-fstack-protector-strong' is not
supported by <Compiler>. Upgrade your compiler or set
THEMIS_DISABLE_SECURITY_HARDENING=ON (not recommended for production). SEC-CC-4.
```

---

## Verifikation der aktiven Flags

### Linux/macOS

```bash
# Prüfe Stack-Protector im Binary
objdump -d build-linux-release/themisdb_server | grep __stack_chk

# Prüfe PIE
file build-linux-release/themisdb_server
# Erwartete Ausgabe: "ELF 64-bit LSB pie executable"

# Prüfe RELRO
readelf -d build-linux-release/themisdb_server | grep -E "RELRO|BIND_NOW"

# Prüfe NX-Bit
readelf -l build-linux-release/themisdb_server | grep GNU_STACK
# Erwartete Ausgabe: "RW" (nicht "RWE")

# Kompakte Übersicht mit checksec (Paket: checksec / python3-pwntools)
checksec --file=build-linux-release/themisdb_server
```

Erwartete `checksec`-Ausgabe für gehärtetes Binary:
```
RELRO:    Full RELRO
STACK CANARY: Canary found
NX:       NX enabled
PIE:      PIE enabled
```

### Windows

```powershell
# Prüfe PE-Header-Flags (requires dumpbin aus MSVC)
dumpbin /headers build-windows-release\themisdb_server.exe | findstr /i "Dynamic NX CFG ASLR"

# Erwartet:
#   DYNAMIC_BASE (ASLR)
#   NX_COMPAT (DEP)
#   GUARD_CF (Control Flow Guard)
```

---

## Policy für neue Compiler und Plattformen

Bei der Portierung auf einen neuen Compiler oder eine neue Plattform:

1. **Prüfe Flag-Unterstützung** mit `cmake -DTHEMIS_DISABLE_SECURITY_HARDENING=OFF ...` und beobachte die Ausgabe.
2. **Pflicht-Flags müssen unterstützt werden.** Ist ein Pflicht-Flag nicht verfügbar, muss der Compiler aktualisiert oder ein offiziell unterstützter verwendet werden.
3. **Neue Flags:** Wenn der neue Compiler/Linker bessere Hardening-Flags bietet (z. B. `-fcf-protection` für Intel CET), sind diese in `cmake/CompilerOptions.cmake` in der jeweiligen Sektion zu ergänzen.
4. **Dokumentation:** Diese Datei muss mit der neuen Plattform und ihren Flags aktualisiert werden.
5. **Review:** Alle Änderungen an Hardening-Flags erfordern ein Security-Review (Label `security`).

### Minimale Compiler-Anforderungen für Hardening

| Compiler | Mindestversion | Grund |
|---|---|---|
| GCC | 8.0 | `-fstack-protector-strong` (4.9+), `-fstack-clash-protection` (8+) |
| Clang | 11.0 | `-fstack-clash-protection` (11+) |
| MSVC (cl.exe) | VS 2019 (16.x) | `/guard:cf`, `/sdl` |

---

## Referenzen

- [GCC Security Flags – Fedora Hardened Build](https://fedoraproject.org/wiki/Changes/Harden_All_Packages)
- [Debian Security Hardening](https://wiki.debian.org/Hardening)
- [MSVC Security Features](https://learn.microsoft.com/en-us/cpp/build/reference/gs-buffer-security-check)
- [MSVC Control Flow Guard](https://learn.microsoft.com/en-us/windows/win32/secbp/control-flow-guard)
- ThemisDB Audit Finding SEC-CC-4 (2026-05-26)
- `cmake/CompilerOptions.cmake` (kanonische Flag-Quelle)
