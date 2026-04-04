# Windows Build Runtime Linking Best Practices (MSVC)

## Zusammenfassung

Erfolgreich implementierte Build-Strategie für ThemisDB auf Windows mit konsistentem Runtime-Linking.

## Problem: /MD vs /MT Runtime Library Mismatch

### Symptome
```
LNK2038: Mismatch detected for 'RuntimeLibrary': value 'MT_StaticRelease' doesn't match value 'MD_DynamicRelease'
```

### Root Cause
- vcpkg mit `x64-windows` kompiliert alle Dependencies mit `/MD` (dynamic CRT)
- Tests erzwangen `/MT` (static CRT) 
- **Resultat**: Linker-Fehler, inkompatible Object-Dateien

## Best Practice: Konsistentes /MD (Dynamic Runtime)

### ✅ Empfohlener Ansatz
**Alle Komponenten verwenden `/MD` (dynamic runtime)**

#### Vorteile
- ✅ Konsistenter Heap → sicheres Memory-Management über DLL-Grenzen
- ✅ Alle Komponenten teilen sich eine CRT-Instanz
- ✅ Kleinere Binary-Größe als bei `/MT`
- ✅ Kompatibel mit vcpkg `x64-windows` default
- ✅ Keine Heap-Corruption-Risiken

#### Nachteile
- ❌ Deployment erfordert MSVC Redistributables (vcredist_x64.exe)
- ❌ Externe DLL-Dependency

#### Deployment-Strategie
```powershell
# Option 1: Redistributables mit Installer ausliefern
Start-Process -Wait vcredist_x64.exe -ArgumentList "/install /quiet /norestart"

# Option 2: DLLs direkt kopieren (nur für Testing, nicht Production)
Copy-Item "C:\Windows\System32\msvcp140.dll" -Destination ".\bin\"
Copy-Item "C:\Windows\System32\vcruntime140.dll" -Destination ".\bin\"
Copy-Item "C:\Windows\System32\vcruntime140_1.dll" -Destination ".\bin\"
```

### Implementierung in CMake

#### ❌ Falsch (erzwingt /MT)
```cmake
if(MSVC)
    # Force static runtime linking
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
```

#### ✅ Richtig (verwendet vcpkg triplet)
```cmake
if(MSVC)
    # Use dynamic runtime (/MD) to match vcpkg x64-windows default
    # This prevents LNK2038 mismatch errors with RocksDB and other dependencies
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    message(STATUS "MSVC Runtime: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
endif()
```

#### ✅ Noch besser (automatisch vom Triplet)
```cmake
if(MSVC)
    # Let CMAKE_MSVC_RUNTIME_LIBRARY match the vcpkg triplet automatically
    # x64-windows → /MD (dynamic)
    # x64-windows-static → /MT (static)
    message(STATUS "MSVC Runtime: ${CMAKE_MSVC_RUNTIME_LIBRARY} (from vcpkg triplet)")
endif()
```

## Alternative: /MT (Static Runtime) für Self-Contained Deployment

### Verwendung
Nur wenn **komplett selbstständige Binaries** ohne externe Dependencies benötigt werden (z.B. USB-Stick, embedded systems).

### Implementierung
```cmake
# vcpkg.json oder Command-Line
vcpkg install --triplet x64-windows-static
```

```cmake
# CMakeLists.txt
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

### ⚠️ Nachteile
- Sehr große Binaries (alle CRT-Funktionen statisch gelinkt)
- Jede Komponente hat eigene CRT-Instanz
- **Gefahr**: Memory zwischen verschiedenen `/MT` Modulen austauschen → Heap Corruption
- Längere Build-Zeiten

## ❌ Hybrid-Ansatz (NICHT empfohlen)

### Was ist das?
Mix aus `/MD` und `/MT` Komponenten im selben Prozess.

### Warum gefährlich?
```cpp
// Modul A (kompiliert mit /MT)
void* ptr = malloc(1024);

// Modul B (kompiliert mit /MD)
free(ptr);  // ❌ CRASH! Anderer Heap!
```

### Problem
- **Jede `/MT` Komponente hat eigenen Heap**
- Memory-Freigabe über Modul-Grenzen → Heap Corruption
- Schwer zu debuggen, sporadische Crashes
- Inkonsistenter globaler State (errno, locale, etc.)

### Microsoft-Empfehlung
> "All modules passed to a given invocation of the linker must have been compiled with the same runtime library compiler option (/MD, /MT, /LD)."
> 
> — MSVC Documentation

## ThemisDB Implementierung

### Aktueller Status ✅
- **vcpkg Triplet**: `x64-windows` (dynamic runtime)
- **Alle Komponenten**: `/MD` (RocksDB, OpenSSL, gRPC, Boost, etc.)
- **Tests**: `/MD` (fixed in tests/CMakeLists.txt)
- **Benchmarks**: `/MD` (consistent from start)

### Build-Konfiguration
```cmake
# tests/CMakeLists.txt
if(MSVC)
    # Use dynamic runtime (/MD) to match vcpkg x64-windows default
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Tests linken gegen themis_core.lib
target_link_libraries(test_snapshot_manager PRIVATE
    GTest::gtest
    GTest::gtest_main
    themis_core  # Bringt alle Dependencies transitiv mit
)
```

### Vorteile dieser Strategie
1. **Single Heap**: Alle Komponenten teilen sich den CRT-Heap
2. **Sicheres Memory-Management**: `malloc` in RocksDB, `free` in ThemisDB-Code funktioniert
3. **Kleinere Binaries**: Shared CRT DLLs
4. **Konsistenz**: Kein Runtime-Mismatch

## Deployment-Checkliste

### Für /MD Builds (x64-windows)
- [ ] MSVC Redistributables installieren
- [ ] Oder: vcredist_x64.exe mit Installer ausliefern
- [ ] Oder: DLLs direkt ins bin/ kopieren (nur Testing)

### Für /MT Builds (x64-windows-static)
- [ ] Keine externe Dependencies
- [ ] Binary direkt ausführbar
- [ ] ⚠️ Größere Binary-Größe akzeptiert
- [ ] ⚠️ Kein Memory-Austausch zwischen Modulen

## Zusammenfassung

| Strategie | Runtime | vcpkg Triplet | Deployment | Empfehlung |
|-----------|---------|---------------|------------|------------|
| **Dynamic CRT** | /MD | x64-windows | Redistributables | ✅ **Standard** |
| **Static CRT** | /MT | x64-windows-static | Self-contained | ⚠️ Nur bei Bedarf |
| **Hybrid** | /MD + /MT | Mixed | Unmöglich | ❌ **Niemals** |

### Finale Empfehlung
**Verwenden Sie konsistent /MD mit vcpkg `x64-windows` triplet.**  
Dies ist der Microsoft-empfohlene, moderne Standard für Windows-Development.

Für self-contained deployment: Vollständig auf `x64-windows-static` umstellen (alle Dependencies neu bauen).

---

**Status**: ✅ Implementiert in ThemisDB v1.4.1
**Build**: Ninja + MSVC 19.44 + vcpkg
**Tests**: Alle Tests und Benchmarks kompilieren und linken erfolgreich
