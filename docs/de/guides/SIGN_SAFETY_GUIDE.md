# Sign Safety Guide - ThemisDB

## Übersicht

Dieses Dokument beschreibt Best Practices für den sicheren Umgang mit signed/unsigned Integer-Typen in ThemisDB, um häufige Fehlerquellen zu vermeiden und die Codequalität zu verbessern.

## Warum ist Sign Safety wichtig?

Signed/unsigned Mismatches können zu schwerwiegenden Bugs führen:

- **Buffer Overflows**: Negative Indices werden als große unsigned Werte interpretiert
- **Endlosschleifen**: Falsche Schleifenbedingungen durch implizite Konvertierung
- **Arithmetische Fehler**: Overflow/Underflow bei gemischten Operationen
- **Sicherheitslücken**: Unerwartetes Verhalten bei kritischen Operationen

## Compiler-Warnungen

ThemisDB aktiviert folgende Warnungen zur Erkennung von Sign-Problemen:

- **MSVC**: `/W4 /we4018` (C4018: signed/unsigned mismatch)
- **GCC/Clang**: `-Wsign-compare -Wsign-conversion`

## Häufige Problemmuster und Lösungen

### 1. Loop-Indices mit falschen Typen

#### ❌ FALSCH
```cpp
std::vector<int> data = get_data();
for (int i = 0; i < data.size(); ++i) {  // ⚠️ C4018: signed/unsigned mismatch
    process(data[i]);
}
```

#### ✅ RICHTIG
```cpp
// Option 1: Korrekte Typen
std::vector<int> data = get_data();
for (size_t i = 0; i < data.size(); ++i) {
    process(data[i]);
}

// Option 2: Modern C++ (bevorzugt)
for (const auto& item : data) {
    process(item);
}

// Option 3: Mit Index via separate counter
size_t i = 0;
for (const auto& item : data) {
    process(item, i++);
}
```

### 2. Vergleiche mit .size()

#### ❌ FALSCH
```cpp
int count = get_count();
if (count < vec.size()) {  // ⚠️ Gefährlich! Negative Werte werden zu großen unsigned
    // Problem: wenn count negativ, wird es zu großem unsigned!
}
```

#### ✅ RICHTIG
```cpp
// Option 1: Verwende Safe Arithmetic Utilities
#include "utils/safe_arithmetic.h"
using namespace themis::utils;

int count = get_count();
if (safe_less_than(count, vec.size())) {
    // Sicher - korrekt behandelt negative Werte
}

// Option 2: Explizite Prüfung
if (count >= 0 && static_cast<size_t>(count) < vec.size()) {
    // Sicher
}

// Option 3: size_t verwenden
if (auto size = safe_int_to_size(count)) {
    if (*size < vec.size()) {
        // Sicher
    }
}
```

### 3. Reverse Iteration

#### ❌ FALSCH
```cpp
std::vector<int> items = get_items();
for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
    // ⚠️ Problem: Overflow wenn items.size() > INT_MAX
    process(items[i]);
}
```

#### ✅ RICHTIG
```cpp
// Modern C++ Pattern für Reverse Iteration
std::vector<int> items = get_items();
if (!items.empty()) {
    for (size_t i = items.size(); i > 0; --i) {
        const size_t idx = i - 1;
        process(items[idx]);
    }
}

// Oder: Reverse Iterators verwenden
for (auto it = items.rbegin(); it != items.rend(); ++it) {
    process(*it);
}
```

### 4. Vector-Konstruktion mit potenziell negativen Werten

#### ❌ GEFÄHRLICH
```cpp
int error_code = -1;
std::vector<int> items(error_code);  // ⚠️ Riesiger Vector! -1 → 4294967295
```

#### ✅ RICHTIG
```cpp
#include "utils/safe_arithmetic.h"
using namespace themis::utils;

int error_code = get_error_code();
if (auto size = safe_int_to_size(error_code)) {
    std::vector<int> items(*size);
    // Sicher
} else {
    handle_error("Invalid size: negative value");
}
```

### 5. Arithmetik mit gemischten Typen

#### ❌ FALSCH
```cpp
int offset = -5;
size_t index = 10;
size_t result = index + offset;  // ⚠️ Underflow! offset wird zu großem unsigned
```

#### ✅ RICHTIG
```cpp
#include "utils/safe_arithmetic.h"
using namespace themis::utils;

int offset = -5;
size_t index = 10;

if (auto result = safe_add(index, offset)) {
    process(*result);  // result = 5
} else {
    handle_error("Arithmetic overflow/underflow");
}
```

### 6. Function Signatures

#### ❌ FALSCH
```cpp
class Buffer {
    int size_;  // ⚠️ Sollte size_t sein
public:
    int size() const { return size_; }
    void resize(int new_size) { /* ⚠️ Negative Größe möglich! */ }
};
```

#### ✅ RICHTIG
```cpp
class Buffer {
    size_t size_;
public:
    size_t size() const { return size_; }
    
    void resize(size_t new_size) {
        if (new_size > max_size()) {
            throw std::bad_alloc();
        }
        size_ = new_size;
    }
};
```

## Safe Arithmetic Utilities

ThemisDB stellt Hilfsfunktionen für sichere signed/unsigned Operationen bereit:

### Header einbinden
```cpp
#include "utils/safe_arithmetic.h"
using namespace themis::utils;
```

### Verfügbare Funktionen

#### safe_add(base, offset)
Addiert signed offset zu unsigned base mit Overflow-Erkennung.

```cpp
size_t index = 10;
int offset = -5;
auto result = safe_add(index, offset);  // Returns optional<size_t>(5)

size_t small = 3;
int large_neg = -10;
auto underflow = safe_add(small, large_neg);  // Returns nullopt
```

#### safe_sub(a, b)
Subtrahiert zwei unsigned Werte mit Underflow-Erkennung.

```cpp
size_t a = 10, b = 5;
auto result = safe_sub(a, b);  // Returns optional<size_t>(5)

auto underflow = safe_sub(5, 10);  // Returns nullopt
```

#### safe_int_to_size(value)
Konvertiert int zu size_t mit Validierung.

```cpp
int count = 100;
if (auto size = safe_int_to_size(count)) {
    std::vector<int> vec(*size);  // Sicher
}

int negative = -1;
auto invalid = safe_int_to_size(negative);  // Returns nullopt
```

#### in_range(index, size)
Prüft ob signed Index im gültigen Bereich liegt.

```cpp
std::vector<int> vec = {1, 2, 3};
int idx = get_index();
if (in_range(idx, vec.size())) {
    process(vec[idx]);  // Sicher
}
```

#### safe_less_than(signed_val, unsigned_val)
Vergleicht signed mit unsigned korrekt.

```cpp
int count = -1;
std::vector<int> vec = {1, 2, 3};
if (safe_less_than(count, vec.size())) {
    // TRUE - korrekt, negative Werte sind kleiner
}
```

#### safe_iterate(container, start_index, callback)
Sichere Iteration ab einem signed Index.

```cpp
std::vector<int> data = {10, 20, 30, 40, 50};
safe_iterate(data, 2, [&](size_t i) {
    process(data[i]);  // Iteriert über Indices 2, 3, 4
});
```

## Testen

Alle Safe Arithmetic Utilities haben umfangreiche Unit-Tests in:
```
tests/test_safe_arithmetic.cpp
```

Um die Tests auszuführen:
```bash
cd build
ctest -R test_safe_arithmetic
```

## Code-Review-Checkliste

Bei Code-Reviews auf folgende Punkte achten:

- [ ] Keine `int` Loop-Indices für Container-Iteration
- [ ] Keine direkten Vergleiche zwischen `int` und `.size()`
- [ ] Keine Vector-Konstruktion mit potenziell negativen Werten
- [ ] Keine Arithmetik zwischen signed und unsigned ohne Validierung
- [ ] API-Signaturen verwenden `size_t` für Größenangaben
- [ ] Reverse Iteration verwendet sichere Patterns
- [ ] Bei Bedarf Safe Arithmetic Utilities verwenden

## Häufige Fehlerquellen

### GPU/VRAM Allocation
```cpp
// ❌ GEFÄHRLICH
void allocate_vram(int size_mb);  // Negative Werte?

// ✅ SICHER
void allocate_vram(size_t size_bytes);
bool check_available(size_t required_bytes) const;
```

### Batch Sizes & Token Counts
```cpp
// ❌ FALSCH
for (int i = 0; i < batch.tokens.size(); ++i) {
    tokens[i] = ...;  // Potential buffer overflow
}

// ✅ RICHTIG
for (size_t i = 0; i < batch.tokens.size(); ++i) {
    tokens[i] = ...;
}
```

### Timeseries Indices
```cpp
// ❌ GEFÄHRLICH
int64_t timestamp = get_timestamp();
if (timestamp < time_series.size()) {  // ⚠️ signed/unsigned
    // ...
}

// ✅ SICHER
int64_t timestamp = get_timestamp();
if (safe_less_than(timestamp, time_series.size())) {
    // ...
}
```

## Best Practices Zusammenfassung

1. **Verwende `size_t` für alle Größen, Indices und Längen**
2. **Bevorzuge Range-based for Loops wo möglich**
3. **Nutze Safe Arithmetic Utilities bei signed/unsigned Mixing**
4. **Validiere negative Werte bevor sie zu unsigned konvertiert werden**
5. **Verwende explizite Casts nur nach Validierung**
6. **Schreibe Tests für Edge-Cases (negative Werte, Overflow)**

## Weitere Ressourcen

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#es102-use-signed-types-for-arithmetic)
- [CERT C++ Coding Standard](https://wiki.sei.cmu.edu/confluence/display/cplusplus)
- ThemisDB Safe Arithmetic Header: `include/utils/safe_arithmetic.h`
- ThemisDB Tests: `tests/test_safe_arithmetic.cpp`

## Kontakt & Support

Bei Fragen zu Sign Safety in ThemisDB:
- Öffne ein Issue im GitHub Repository
- Konsultiere die Safe Arithmetic Utility-Dokumentation
- Frage im Team-Chat nach Best Practices
