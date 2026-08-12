## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


# CPack Integration Analysis für ThemisDB

> **Stand:** 2026-06-23 · **Scope:** Evaluate CPack utility for multi-edition, multi-platform distribution

## Zusammenfassung

ThemisDB hat bereits ein **install()-Framework** in der CMakeLists.txt, aber **keine CPack-Integration**. Das Packaging erfolgt derzeit über **manuelle PowerShell/Batch-Skripte** (`package-*.ps1`, `package-*.bat`). 

**Fazit:** CPack würde helfen bei:
- ✅ **Konsistent Installer für Multiple Plattformen** (ZIP, MSI, TGZ, RPM, DEB, Docker)
- ✅ **Automatisierter Edition-Varianten** (minimal, community, enterprise, hyperscaler)
- ✅ **License & Signaturübergabe** (digitale Signaturen für Releases)
- ❌ **Aber:** Zusätzliche Komplexität in `CMakeLists.txt` und `CPackConfig.cmake`
- ❌ **Und:** Nicht ideal für Docker-Builds (hier ist separate Multi-Stage besser)

---

## 1. Aktueller Stand: Manuales Packaging

### 1.1 Existing Install() Rules

Die Root `CMakeLists.txt` hat bereits:

```cmake
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME runtime)

install(TARGETS themis_server themis_core
    DESTINATION bin
    COMPONENT runtime)

install(DIRECTORY ${THEMIS_ROOT_DIR}/include/
    DESTINATION include
    COMPONENT development)

# Service + shell completions
install(FILES "${THEMIS_ROOT_DIR}/debian/themisdb.service"
    DESTINATION lib/systemd/system
    COMPONENT runtime)

install(FILES ${THEMIS_ROOT_DIR}/tools/completion/themisctl.bash
    DESTINATION share/bash-completion/completions
    COMPONENT runtime)
```

**Problem:** Diese `install()`-Rules sind **nicht via CPack aktiviert**, sondern würden nur bei manuellem `cmake --install` Aufruf wirken.

### 1.2 Manuele Skripte (`package-*.ps1`, `package-*.bat`)

Aktueller Ansatz:
- Kopiert Binäre, DLLs, Docs manuell in ein Staging-Verzeichnis
- Erstellt ZIP, checksums (SHA256)
- Keine Konsistenz zwischen Edition-Varianten
- Keine automatisierte MSI/WiX-Generierung

**Beispiel aus `package-production-simple.ps1`:**
```powershell
$releaseDir = Join-Path $repo "release"
$stage = Join-Path $releaseDir "themisdb-$Version-$suffix"

foreach ($dir in $dirs) {
    New-Item -ItemType Directory -Force -Path (Join-Path $stage $dir) | Out-Null
}

# Manuelle Kopien...
Copy-Item "$buildDir/themis_server.exe" "$stage/bin/"
Copy-Item "$buildDir/themis_core.dll" "$stage/bin/"
# ... etc
```

---

## 2. CPack-Integrations-Szenarien

### 2.1 Szenario A: **Basis-CPack für ZIP/TGZ + Multi-Edition**

**Was:** Standardisierte ZIP- und TGZ-Archive pro Edition mittels CPack statt PowerShell-Skripten.

**Nutzen:**
- Automatisierte `cpack -G ZIP` / `cpack -G TGZ` pro Konfiguration
- Konsistente Dateistrukturen über Plattformen
- Integrierte Checksummen/Hash-Verifikation
- Versionierung aus CMake `VERSION` Variable

**Aufwand:**
- `CPackConfig.cmake` mit Edition-Varianten (minimal, community, enterprise, hyperscaler)
- Environment-Variable oder Preset zur Edition-Auswahl
- Entfernung manueller `package-*.ps1` (optional)

**Beispiel-Integration:**

```cmake
# In root CMakeLists.txt nach install()-Regeln

# === CPack Configuration ===
set(CPACK_PACKAGE_NAME "ThemisDB")
set(CPACK_PACKAGE_VERSION "${THEMIS_VERSION_STRING}")
set(CPACK_PACKAGE_VENDOR "ThemisDB Contributors")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://themisdb.io")

# Component-basiertes Packaging
set(CPACK_COMPONENTS_ALL runtime development documentation)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Runtime")
set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Development Headers & Libraries")
set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")

# Edition-spezifische Filter (z.B. "enterprise" enthält nicht "minimal" Tooling)
# TODO: Dynamisch basierend auf ${THEMIS_EDITION}

# Archive Format
set(CPACK_GENERATOR "ZIP;TGZ")  # Könnte auch "NSIS;WIX;DEB;RPM" sein

# Installer-Metadaten
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Enterprise distributed database with GPU acceleration")
set(CPACK_PACKAGE_CONTACT "ThemisDB <support@themisdb.io>")

# Windows MSI (optional, wenn WIX vorhanden)
if(WIN32)
    set(CPACK_WIX_UPGRADE_GUID "01234567-89AB-CDEF-0123-456789ABCDEF")
    set(CPACK_WIX_PRODUCT_ICON "${THEMIS_ROOT_DIR}/assets/themis-icon-64x64.ico")
    list(APPEND CPACK_GENERATOR "WIX")
endif()

include(CPack)
```

**Aktivierung:**
```bash
# Nach dem Build:
cmake --build --preset windows-release
cpack -G ZIP --config CPackConfig.cmake -C Release
# → themisdb-1.9.0-windows-x64.zip entsteht in build/ directory
```

---

### 2.2 Szenario B: **CPack mit Multi-Edition Varianten**

**Was:** Separate Pakete pro Edition (minimal, community, enterprise) mit unterschiedlichen Komponenten.

**Nutzen:**
- Single source tree für alle Editionen
- Automatisierte Edition-spezifische Konfigurationen
- Reduzierte manuelle Verwaltung bei Releases
- Konsistente Lizenzen/Signaturen in Paketen

**Aufwand:**
- `CPackConfig.cmake` mit Edition-Logik
- CMake Presets für "community-release", "enterprise-release", etc.
- License-Textdateien pro Edition (LICENSE.minimal, LICENSE.enterprise, ...)
- Optional: Digitale Signaturen der Pakete (GPG/Authenticode)

**Beispiel-Logik:**

```cmake
# In CPackConfig.cmake

set(THEMIS_EDITION "${THEMIS_EDITION_STRING}" CACHE STRING "Edition for packaging")

if(THEMIS_EDITION STREQUAL "minimal")
    # Minimal: nur themis_server, core lib, minimalistische Tooling
    set(CPACK_PACKAGE_FILE_NAME "themisdb-${CPACK_PACKAGE_VERSION}-minimal-windows-x64")
    set(CPACK_COMPONENTS_ALL runtime)
    set(CPACK_COMPONENT_RUNTIME_INSTALL_TYPES Minimal Full)

elseif(THEMIS_EDITION STREQUAL "community")
    # Community: full runtime + tools + docs
    set(CPACK_PACKAGE_FILE_NAME "themisdb-${CPACK_PACKAGE_VERSION}-community-windows-x64")
    set(CPACK_COMPONENTS_ALL runtime documentation tools)

elseif(THEMIS_EDITION STREQUAL "enterprise")
    # Enterprise: everything + license file
    set(CPACK_PACKAGE_FILE_NAME "themisdb-${CPACK_PACKAGE_VERSION}-enterprise-windows-x64")
    set(CPACK_COMPONENTS_ALL runtime development documentation tools enterprise-extras)
    set(CPACK_PACKAGE_LICENSE_FILE "${THEMIS_ROOT_DIR}/LICENSE.enterprise")

endif()

include(CPack)
```

**Aktivierung:**

```bash
# Community Release:
cmake -S . -B build-community-release \
  -G Ninja \
  -DTHEMIS_EDITION=community \
  --preset windows-release

cpack -G ZIP --config build-community-release/CPackConfig.cmake
# → themisdb-1.9.0-community-windows-x64.zip

# Enterprise Release:
cmake -S . -B build-enterprise-release \
  -G Ninja \
  -DTHEMIS_EDITION=enterprise \
  --preset windows-release

cpack -G ZIP --config build-enterprise-release/CPackConfig.cmake
# → themisdb-1.9.0-enterprise-windows-x64.zip
```

---

### 2.3 Szenario C: **CPack + MSI/WIX für Windows (Optional)**

**Was:** Windows MSI-Installer mittels WIX + CPack (alternativ zu aktuellen Batch-Skripten).

**Nutzen:**
- Native Windows Installer (`.msi`)
- Registry-Integration für Deinstallation
- Add/Remove Programs Eintrag
- Systemd-Service-Auto-Installation (Optional)

**Aufwand:**
- WIX Toolset Installation erforderlich
- `CPackWIX.cmake` Konfiguration
- Windows-spezifische Komponenten (Service, Shortcuts)
- Authentifizierung des MSI (Signatur-Zertifikat)

**Hinweis:** Aktueller Kommentar in `CMakeLists.txt` ist vielversprechend:
```cmake
# Treat uncomponentized install() rules as runtime payload by default.
# This prevents third-party runtime DLLs from leaking into a separate
# 'Unspecified' package component and colliding with the explicit runtime
# package during WiX/MSI generation.
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME runtime)
```

**→ Bedeutet:** Irgendwann war ein WIX/MSI-Szenario geplant!

**Beispiel-Konfig:**

```cmake
set(CPACK_WIX_UPGRADE_GUID "01234567-89AB-CDEF-0123-456789ABCDEF")
set(CPACK_WIX_PRODUCT_ICON "${THEMIS_ROOT_DIR}/assets/themis-icon-64x64.ico")
set(CPACK_WIX_LICENSE_RTF "${THEMIS_ROOT_DIR}/LICENSE.rtf")
set(CPACK_WIX_UI_BANNER "${THEMIS_ROOT_DIR}/assets/banner.png")

# Programm-Startmenü-Links
set(CPACK_WIX_PROGRAM_MENU_FOLDER "ThemisDB")
set(CPACK_WIX_FEATURES_DISPLAY_OPTIONS "INSTALLTYPE=InstallationType,IsTypical")

list(APPEND CPACK_GENERATOR "WIX")
```

---

### 2.4 Szenario D: **CPack für Linux (DEB/RPM) — Advanced**

**Was:** Native Linux Packages (`.deb` für Debian/Ubuntu, `.rpm` für RHEL/CentOS).

**Nutzen:**
- APT/YUM Integration (`apt install themisdb` / `yum install themisdb`)
- Automatisierte Service-Installation (`systemctl start themisdb`)
- Dependency Resolution
- Security Updates über Package Manager

**Aufwand:**
- `CPackDEB.cmake` + `CPackRPM.cmake` Konfiguration
- Systemd service file (bereits in `debian/themisdb.service`)
- Pre/Post-Install Scripts (z.B. Benutzer erstellen, Directories vorbereiten)
- GPG-Signing für Repositories

**Hinweis:** Basis scheint vorbereitet:
```cmake
install(FILES "${THEMIS_ROOT_DIR}/debian/themisdb.service"
    DESTINATION lib/systemd/system
    COMPONENT runtime)
```

**Beispiel-Konfig:**

```cmake
# Linux DEB
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl3, libcurl4, libboost-system1.81.0")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ThemisDB <support@themisdb.io>")
set(CPACK_DEBIAN_PACKAGE_SECTION "database")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "standard")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${THEMIS_ROOT_DIR}/debian/preinst;${THEMIS_ROOT_DIR}/debian/postinst")

# Linux RPM
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_RPM_PACKAGE_REQUIRES "openssl-libs, libcurl")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_URL "https://themisdb.io")

list(APPEND CPACK_GENERATOR "DEB" "RPM")
```

---

## 3. CPack vs. Alternativen: Vergleich

| Aspekt | CPack | Manuele Skripte | Docker | Hybrid |
|---|---|---|---|---|
| **Plattform-Konsistenz** | ✅ Hoch | ❌ Niedrig | ✅ Nativ | ✅ Mittel |
| **Edition-Automatisierung** | ✅ Via Komponenten | ❌ Manuell | ✅ Multi-Stage | ✅ Mittel |
| **Windows MSI** | ✅ Native WIX | ❌ Aufwendig | ❌ Nicht möglich | ✅ CPack für MSI |
| **Linux DEB/RPM** | ✅ Native | ❌ Keine | ❌ Kann packagen | ✅ CPack für native |
| **Docker-Integration** | ❌ Nicht ideal | ❌ Nicht ideal | ✅ Native | ✅ Docker separat |
| **Maintenance-Burden** | ✅ Niedrig | ❌ Hoch | ✅ Mittel | ✅ Mittel |
| **CI/CD-Automation** | ✅ Gut | ❌ Komplex | ✅ Gut | ✅ Gut |

---

## 4. Empfehlung für ThemisDB

### 4.1 **Kurz (6-12 Monate):** Basis-CPack Implementierung

**Minimale Änderungen:**

1. Erstelle `CPackConfig.cmake` im `cmake/`-Verzeichnis:
   - ZIP/TGZ für alle Plattformen
   - Community-Edition-Komponenten
   - Einfache Versionierung

2. Integriere in Root `CMakeLists.txt`:
   ```cmake
   include(cmake/CPackConfig.cmake)
   ```

3. Ersetze manuelle `package-*.ps1` durch:
   ```bash
   cpack -G ZIP --preset windows-release
   ```

4. Dokumentiere in `RELEASE_STRATEGY.md`:
   - CPack-Kommandos pro Edition
   - Komponenten-Mapping
   - Signatur-Prozess

**Aufwand:** ~8-16 Stunden (eine Person, ein Iteration)  
**Gewinn:** Konsistente ZIP/TGZ, weniger manuelle Skripte, CI/CD-Vorbereitung

---

### 4.2 **Mittelfristig (12-24 Monate):** Multi-Edition + MSI

1. Erweitere `CPackConfig.cmake` um Edition-Logik
2. Füge Windows MSI (WIX) hinzu (optional, abhängig von Windows-Kundenbase)
3. Integriere Linux DEB/RPM (für Server-Deployments)

**Aufwand:** ~24-40 Stunden  
**Gewinn:** Native Installer pro Plattform, Package-Manager Integration

---

### 4.3 **Langfristig (24+ Monate):** Full Automation (Hybrid CPack + Docker)

1. CPack für ZIP/MSI/DEB/RPM
2. Separate Docker Multi-Stage Builds für Images
3. Repository-Signing (GPG für Linux, Authenticode für Windows)
4. Automated CI/CD Pipeline (GitHub Actions)

**Aufwand:** ~40-60 Stunden  
**Gewinn:** Vollständig automatisierte, sichere Release-Pipeline

---

## 5. Risiken & Mitigationen

| Risiko | Wahrscheinlichkeit | Mitigation |
|---|---|---|
| **CPack-Verstecktheit** | Mittel | Dokumentation in RELEASE_STRATEGY.md, Beispiele in cmake/ |
| **Kompatibilität mit bestehenden Skripten** | Mittel | Parallel einführen, alte Skripte zunächst behalten |
| **WIX/MSI-Komplexität** | Hoch | Optional starten; ZIP ist ausreichend für MVP |
| **Multi-Edition-Fehler** | Mittel | Tests pro Edition im CI (schnell durchführbar) |
| **Signatur/License-Management** | Mittel | Separater Prozess außerhalb CPack (manuelle Signatur danach) |

---

## 6. Umsetzungsplan (Falls zustimmig)

### Phase 1: Grundkonfiguration (Week 1-2)

```markdown
- [ ] Erstelle cmake/CPackConfig.cmake (Basis-Konfiguration)
- [ ] Integriere CPack in root CMakeLists.txt
- [ ] Teste cpack -G ZIP in windows-release Preset
- [ ] Dokumentiere neue Packag-Befehle in RELEASE_STRATEGY.md
```

### Phase 2: Edition-Varianten (Week 3-4)

```markdown
- [ ] Erweitere CPackConfig.cmake um Edition-Filter
- [ ] Erstelle Edition-spezifische CMake Presets
- [ ] Teste ZIP-Pakete pro Edition (minimal, community, enterprise)
- [ ] Validiere Komponenten in Paketen
```

### Phase 3: Plattform-Erweiterung (Week 5-6, optional)

```markdown
- [ ] MSI/WIX Konfiguration (Windows)
- [ ] DEB/RPM Konfiguration (Linux)
- [ ] Systemd integration
- [ ] Test auf Linux-Plattformen
```

---

## 7. Referenzen

- **CMake CPack:** https://cmake.org/cmake/help/latest/module/CPack.html
- **CPack Generators:** https://cmake.org/cmake/help/latest/manual/cpack-generators.7.html
- **WIX Toolset:** https://wixtoolset.org/
- **RPM Packaging:** https://rpm.org/
- **Debian Packaging:** https://www.debian.org/doc/debian-policy/

---

## Entscheidungsmatrix

Soll ThemisDB CPack einführen?

| Kriterium | Relevant? | Priorität |
|---|---|---|
| ZIP/TGZ-Konsistenz → Ja | **Hoch** | Starten mit Szenario A |
| Multi-Edition-Pakete → Ja | **Mittel** | Später (Szenario B) |
| Native MSI-Installer | **Optional** | Abhängig von Windows-Kundenbase |
| Native DEB/RPM | **Mittel** | Für Hyperscaler-Edition relevant |
| Docker-Integration | Separat | Nicht via CPack, sondern Dockerfile |

**Fazit:** ✅ **CPack ist für ThemisDB sinnvoll**, insbesondere für **ZIP/TGZ-Standardisierung** und später für **Edition-Varianten + Linux Packages**. Start: Basis-CPack (Szenario A), dann iterativ ausbauen.
