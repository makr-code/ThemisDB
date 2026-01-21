# Cross-Compilation Requirements für ThemisDB

**Zielplattformen:** Windows (x86_64), Linux (x86_64, ARM64, ARMv7), macOS, WSL

---

## 🔴 KRITISCHE REGEL: Nur Cross-Compile-fähige Code!

Beim Code-Review oder Feature-Implementation **MUSS** sichergestellt werden, dass ALLE verwendeten Libraries, Funktionen und APIs auf **ALLEN Zielplattformen** verfügbar sind.

---

## ❌ VERBOTEN - Nicht Cross-Compile-Fähig

### Plattform-spezifische APIs
```cpp
// ❌ FALSCH - Windows only
#include <windows.h>
DWORD dwSize = GetFileSize(hFile, NULL);

// ❌ FALSCH - Linux only  
#include <unistd.h>
int fd = open("/dev/something", O_RDONLY);

// ❌ FALSCH - macOS only
#include <CoreFoundation/CoreFoundation.h>
```

### Plattform-spezifische Libraries (ausser vcpkg-Pakets)
```cpp
// ❌ FALSCH - Nur auf Windows verfügbar
#include <winsock2.h>

// ❌ FALSCH - Nur auf UNIX-Systemen
#include <sys/socket.h>
#include <netinet/in.h>

// ❌ FALSCH - X11 Graphics (nur Linux/Unix)
#include <X11/Xlib.h>
```

### Compiler-spezifische Features (ohne Fallback)
```cpp
// ❌ FALSCH - GCC/Clang specific, kein MSVC Fallback
#pragma GCC diagnostic ignored "-Wfloat-equal"

// ❌ FALSCH - MSVC only
#pragma warning(disable: 4996)  // Kein Fallback für GCC/Clang
```

### Absolut zielgerichtete Pfade
```cpp
// ❌ FALSCH - Hartcodierter Windows-Pfad
std::string config_path = "C:\\ProgramData\\ThemisDB\\config.json";

// ❌ FALSCH - Hartcodierter Linux-Pfad
const char* db_path = "/var/lib/themis/data.db";
```

### Laufzeit-Umgebungs-Annahmen
```cpp
// ❌ FALSCH - Setzt Windows-Registry voraus
// ❌ FALSCH - Setzt Linux /proc filesystem voraus
// ❌ FALSCH - Setzt macOS bundle structure voraus
```

---

## ✅ ERLAUBT - Cross-Compile-Fähig

### vcpkg-verwaltete Libraries ONLY
```cpp
// ✅ RICHTIG - Überall verfügbar via vcpkg
#include <boost/asio.hpp>              // boost-asio
#include <spdlog/spdlog.h>             // spdlog
#include <sqlite3.h>                   // sqlite3
#include <rocksdb/db.h>                // rocksdb
#include <openssl/ssl.h>               // openssl
#include <grpcpp/grpcpp.h>             // grpc
```

### Standard C++ Library + Plattform-Abstraktion
```cpp
// ✅ RICHTIG - Standard C++20 Funktionen
#include <filesystem>
#include <thread>
#include <mutex>
#include <chrono>

// ✅ RICHTIG - Boost filesystem statt raw OS APIs
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
fs::path config_dir = fs::system_complete("config");

// ✅ RICHTIG - Boost ASIO statt Sockets
#include <boost/asio.hpp>
boost::asio::ip::tcp::socket socket(io_context);
```

### Compiler-Portabilität mit Fallback
```cpp
// ✅ RICHTIG - MSVC + GCC/Clang Unterstützung
#ifdef _MSC_VER
    #pragma warning(disable: 4996)
#else
    #pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

// ✅ RICHTIG - Compiler-Features mit ifdef
#if __has_cpp_attribute(nodiscard)
    [[nodiscard]]
#endif
int getValue() { return 42; }
```

### Plattform-agnostische Pfade
```cpp
// ✅ RICHTIG - Nutze std::filesystem
#include <filesystem>
namespace fs = std::filesystem;

fs::path config_path = fs::path(getenv("HOME")) / ".themis" / "config.json";
fs::path db_path = fs::temp_directory_path() / "themis-data";
fs::create_directories(db_path);

// ✅ RICHTIG - Boost filesystem
#include <boost/filesystem.hpp>
boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
```

### CMake Conditional Compilation
```cmake
# ✅ RICHTIG - CMake abstrahiert Plattformen
if(WIN32)
    target_compile_definitions(themis_core PRIVATE WINDOWS_BUILD)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(themis_core PRIVATE LINUX_BUILD)
elseif(APPLE)
    target_compile_definitions(themis_core PRIVATE MACOS_BUILD)
endif()

# ✅ RICHTIG - Aber use präventiv statt exclusiv
if(MSVC)
    target_compile_options(themis_core PRIVATE /W4)
else()
    target_compile_options(themis_core PRIVATE -Wall -Wextra)
endif()
```

---

## 📋 Cross-Compile Checklist für Code-Reviews

Bei **JEDEM Code-Review** müssen diese Fragen mit JA beantwortet werden:

- [ ] **Alle `#include` Dateien** - Sind sie in vcpkg verfügbar ODER Standard C++?
- [ ] **Alle verwendeten APIs** - Funktionieren sie auf Windows, Linux (x86_64, ARM64, ARMv7) UND macOS?
- [ ] **Pfade** - Nutzen sie `std::filesystem` / `boost::filesystem` statt hardcodierter Pfade?
- [ ] **System Calls** - Gibt es Fallbacks für alle unterstützten OS?
- [ ] **Memory Layout** - Keine Annahmen über `sizeof()`? (ARM kann unterschiedliche Alignment haben)
- [ ] **Endianness** - Kein Bytecode-hacking das x86_64 Byte-Order voraussetzt?
- [ ] **Thread-Sicherheit** - Laufen auf ARM64 multi-threaded Tests?
- [ ] **Compiler-Flags** - Alle Optionen auch in GCC/Clang gültig (nicht nur MSVC)?
- [ ] **Conditional Compilation** - Hat jedes `#ifdef _WIN32` ein `#else` Fallback?
- [ ] **Externe Tools** - Werden externe Binaries nicht hartcodiert mit Windows-Pfaden aufgerufen?

---

## 🔧 Validierung durch Build-Tests

```bash
# Windows MSVC Build
cmake -G "Visual Studio 17 2022" -A x64 .
cmake --build . --config Release

# Linux GCC x86_64
cmake -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc .
cmake --build . -- -j8

# Linux ARM64 Cross-Compile
cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-arm64.cmake .
cmake --build . -- -j8

# macOS Clang
cmake -DCMAKE_CXX_COMPILER=clang++ .
cmake --build . -- -j8
```

---

## 🚨 Build-Fehler = Nicht Cross-Compile-fähig!

Wenn die folgenden Fehler auftreten, ist der Code NICHT Cross-Compile-fähig:

```
error: 'GetFileSize' was not declared in this scope
error: undefined reference to 'dlopen'
error: unknown type name 'HANDLE'
fatal error: windows.h: No such file or directory
fatal error: sys/socket.h: No such file or directory
```

**SOFORT**: Refactor zum plattform-agnostischen Code!

---

## 📦 Empfohlene Cross-Compile-Safe Libraries

| Funktion | ❌ Nicht Safe | ✅ Empfohlen |
|----------|------------|------------|
| **Networking** | `winsock2.h`, `unistd.h` | `boost::asio` |
| **File I/O** | `<windows.h>`, `<unistd.h>` | `std::filesystem` |
| **Threads** | `CreateThread`, `pthread_create` | `std::thread` |
| **Mutex** | `CRITICAL_SECTION`, `pthread_mutex` | `std::mutex` |
| **Path Handling** | Hardcoded `/` or `\` | `std::filesystem::path` |
| **Environment** | `GetEnv`, `getenv` | `std::getenv` |
| **Memory** | `malloc`, `new` | `std::make_unique` |
| **JSON** | Selbstgebaut | `nlohmann/json` (vcpkg) |
| **Database** | `mdb` (Windows), `SQLite` (Linux) | `rocksdb` (vcpkg) |
| **Crypto** | `CryptoAPI`, `OpenSSL raw` | `openssl` (vcpkg) |

---

## ⚠️ Häufige Cross-Compile-Fallen

### 1. Endianness-Annahmen
```cpp
// ❌ FALSCH
uint32_t value = *(uint32_t*)buffer;  // ARM Big-Endian kann anders sein!

// ✅ RICHTIG
uint32_t value = static_cast<uint32_t>(buffer[0]) << 24 |
                 static_cast<uint32_t>(buffer[1]) << 16 |
                 static_cast<uint32_t>(buffer[2]) << 8 |
                 static_cast<uint32_t>(buffer[3]);
```

### 2. Zeiger-Größe Annahmen
```cpp
// ❌ FALSCH - sizeof(void*) != 8 auf manchen ARM-Systemen
uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
if (addr & 0xFFFFFFFF00000000ULL) { }  // Assume 64-bit!

// ✅ RICHTIG
#include <cstdint>
static_assert(sizeof(void*) == 8, "This code requires 64-bit pointers");
```

### 3. Float-Vergleiche
```cpp
// ❌ FALSCH - Floating point Rounding auf ARM anders!
if (x == 3.14159) { }

// ✅ RICHTIG
const float EPSILON = 1e-6f;
if (std::abs(x - 3.14159f) < EPSILON) { }
```

---

## 🎯 Zusammenfassung

> **GOLDEN RULE:**
> 
> Wenn dein Code auf deiner lokalen Maschine funktioniert, aber du weißt nicht warum er auf ARM64 Linux nicht läuft - 
> du hast wahrscheinlich Cross-Compile Anforderungen verletzt.
>
> **IMMER** testen auf mindestens 2 verschiedenen Plattformen!

---

## 📞 Agent/Copilot Anweisung

Wenn dieser Code im Review eingereicht wird, prüfe automatisch:

1. **Alle `#include` statements** - Sind sie Standard C++ oder vcpkg-Pakete?
2. **OS-spezifische Code** - Gibt es `#ifdef _WIN32`, `#ifdef __linux__` ohne vollständige Fallbacks?
3. **Hartcodierte Pfade** - Nutzen sie `/` oder `\` statt `std::filesystem`?
4. **Externe Libraries** - Sind sie in `vcpkg.json` registriert?
5. **Thread/Mutex Code** - Nutzt es `std::thread` / `std::mutex` oder OS-APIs?

**Bei JEDEM Verstoß:** Automatisch ablehnen mit Hinweis auf dieses Dokument!

