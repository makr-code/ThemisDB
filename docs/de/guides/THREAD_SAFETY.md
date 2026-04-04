# Thread-Safety Guide für ThemisDB

## Überblick

ThemisDB ist ein hochleistungsfähiges Datenbanksystem mit mehreren parallelen Komponenten. Dieser Leitfaden beschreibt die Thread-Safety-Patterns, Best Practices und Testing-Strategien für die Entwicklung von Thread-sicherem Code.

## Inhaltsverzeichnis

1. [Thread-Safety Patterns](#thread-safety-patterns)
2. [Best Practices](#best-practices)
3. [Testing mit ThreadSanitizer](#testing-mit-threadsanitizer)
4. [Code Review Checkliste](#code-review-checkliste)
5. [Häufige Probleme und Lösungen](#häufige-probleme-und-lösungen)

## Thread-Safety Patterns

### Pattern 1: Mutex-geschützter Shared State

**Problem:** Race Conditions bei gleichzeitigem Zugriff auf Container

```cpp
// ❌ FALSCH - Race condition
class Scheduler {
    std::vector<Task> queue_;  // Nicht thread-safe!
    
public:
    void add_task(Task t) {
        queue_.push_back(t);  // Race wenn parallel aufgerufen
    }
    
    Task get_next() {
        if (queue_.empty()) return {};
        Task t = queue_.front();
        queue_.erase(queue_.begin());  // Race
        return t;
    }
};
```

**Lösung:** Mutex schützt kritische Sektion

```cpp
// ✅ RICHTIG - Mutex-geschützt
class Scheduler {
    std::vector<Task> queue_;
    mutable std::mutex mutex_;
    
public:
    void add_task(Task t) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(t);
    }
    
    std::optional<Task> get_next() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return std::nullopt;
        Task t = std::move(queue_.front());
        queue_.erase(queue_.begin());
        return t;
    }
};
```

**Alternative mit Utility-Klasse:**

```cpp
#include "utils/thread_safety.h"

class Scheduler {
    themis::utils::threading::Synchronized<std::vector<Task>> queue_;
    
public:
    void add_task(Task t) {
        queue_.with_lock([&](auto& q) {
            q.push_back(std::move(t));
        });
    }
    
    std::optional<Task> get_next() {
        return queue_.with_lock([](auto& q) -> std::optional<Task> {
            if (q.empty()) return std::nullopt;
            Task t = std::move(q.front());
            q.erase(q.begin());
            return t;
        });
    }
};
```

### Pattern 2: Atomics für Flags und Counter

**Problem:** Data Race bei nicht-atomaren Read-Modify-Write Operationen

```cpp
// ❌ FALSCH - Data race
class Statistics {
    size_t request_count_ = 0;  // Data race
    bool is_running_ = false;   // Data race
    
public:
    void increment() {
        ++request_count_;  // Nicht atomar: read-modify-write
    }
    
    void start() {
        is_running_ = true;  // Non-atomic write
    }
    
    bool is_running() const {
        return is_running_;  // Non-atomic read
    }
};
```

**Lösung:** std::atomic verwenden

```cpp
// ✅ RICHTIG - Atomics
class Statistics {
    std::atomic<size_t> request_count_{0};
    std::atomic<bool> is_running_{false};
    
public:
    void increment() {
        // relaxed ist ausreichend für einfache Counter
        request_count_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void start() {
        is_running_.store(true, std::memory_order_release);
    }
    
    bool is_running() const {
        return is_running_.load(std::memory_order_acquire);
    }
    
    size_t get_count() const {
        return request_count_.load(std::memory_order_relaxed);
    }
};
```

**Wann welche Memory Order verwenden:**

- `memory_order_relaxed`: Für einfache Counter ohne Synchronisationsbedarf
- `memory_order_acquire/release`: Für Flags die Synchronisation etablieren
- `memory_order_seq_cst`: Wenn unsicher (default, aber langsamer)

### Pattern 3: Reader-Writer Locks

**Problem:** Mutex blockiert auch lesende Threads gegenseitig

```cpp
// ❌ INEFFIZIENT - Reads blockieren sich gegenseitig
class Cache {
    std::unordered_map<std::string, Value> data_;
    std::mutex mutex_;  // Nur ein Mutex
    
public:
    std::optional<Value> get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);  // Read blockiert andere Reads!
        auto it = data_.find(key);
        return it != data_.end() ? std::optional(it->second) : std::nullopt;
    }
    
    void put(const std::string& key, Value val) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_[key] = std::move(val);
    }
};
```

**Lösung:** std::shared_mutex für Read-Heavy Workloads

```cpp
// ✅ RICHTIG - Mehrere Reads parallel möglich
class Cache {
    std::unordered_map<std::string, Value> data_;
    mutable std::shared_mutex mutex_;  // Reader-Writer-Lock
    
public:
    std::optional<Value> get(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);  // Shared read lock
        auto it = data_.find(key);
        return it != data_.end() ? std::optional(it->second) : std::nullopt;
    }
    
    void put(const std::string& key, Value val) {
        std::unique_lock<std::shared_mutex> lock(mutex_);  // Exclusive write lock
        data_[key] = std::move(val);
    }
};
```

**Alternative mit Utility-Klasse:**

```cpp
#include "utils/thread_safety.h"

class Cache {
    themis::utils::threading::SharedSynchronized<
        std::unordered_map<std::string, Value>
    > data_;
    
public:
    std::optional<Value> get(const std::string& key) const {
        return data_.with_shared_lock([&](const auto& map) -> std::optional<Value> {
            auto it = map.find(key);
            return it != map.end() ? std::optional(it->second) : std::nullopt;
        });
    }
    
    void put(const std::string& key, Value val) {
        data_.with_unique_lock([&](auto& map) {
            map[key] = std::move(val);
        });
    }
};
```

### Pattern 4: Lock-freie Datenstrukturen vermeiden Deadlocks

**Problem:** Deadlock durch falsche Lock-Reihenfolge

```cpp
// ❌ GEFÄHRLICH - Deadlock möglich
class ResourceManager {
    std::mutex mutex_a_;
    std::mutex mutex_b_;
    
    void transfer_a_to_b() {
        std::lock_guard<std::mutex> lock_a(mutex_a_);  // ⚠️
        std::lock_guard<std::mutex> lock_b(mutex_b_);  // ⚠️
        // ...
    }
    
    void transfer_b_to_a() {
        std::lock_guard<std::mutex> lock_b(mutex_b_);  // ⚠️ Umgekehrte Reihenfolge!
        std::lock_guard<std::mutex> lock_a(mutex_a_);  // ⚠️ DEADLOCK
        // ...
    }
};
```

**Lösung:** std::scoped_lock (C++17) oder konsistente Lock-Reihenfolge

```cpp
// ✅ RICHTIG - Deadlock-free mit scoped_lock
class ResourceManager {
    std::mutex mutex_a_;
    std::mutex mutex_b_;
    
    void transfer_a_to_b() {
        std::scoped_lock lock(mutex_a_, mutex_b_);  // ✅ Atomare Multi-Lock
        // ...
    }
    
    void transfer_b_to_a() {
        std::scoped_lock lock(mutex_a_, mutex_b_);  // ✅ Gleiche Reihenfolge
        // ...
    }
};
```

### Pattern 5: Thread-sichere Lazy Initialization

**Problem:** Broken Double-Checked Locking

```cpp
// ❌ FALSCH - Broken double-checked locking
class Singleton {
    static Singleton* instance_;  // Nicht atomic!
    static std::mutex mutex_;
    
public:
    static Singleton* get() {
        if (!instance_) {  // ⚠️ Race - 1st check
            std::lock_guard<std::mutex> lock(mutex_);
            if (!instance_) {  // 2nd check
                instance_ = new Singleton();  // ⚠️ Reordering möglich!
            }
        }
        return instance_;
    }
};
```

**Lösung Option 1:** Meyer's Singleton (C++11 thread-safe)

```cpp
// ✅ RICHTIG - Meyer's Singleton
class Singleton {
public:
    static Singleton& get() {
        static Singleton instance;  // ✅ Thread-safe seit C++11
        return instance;
    }
    
private:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};
```

**Lösung Option 2:** std::call_once

```cpp
// ✅ RICHTIG - std::call_once
class Singleton {
    static std::unique_ptr<Singleton> instance_;
    static std::once_flag init_flag_;
    
public:
    static Singleton& get() {
        std::call_once(init_flag_, []() {
            instance_ = std::make_unique<Singleton>();
        });
        return *instance_;
    }
};
```

## Best Practices

### 1. Minimale Lock-Bereiche

**Prinzip:** Halte Locks so kurz wie möglich

```cpp
// ❌ SCHLECHT - Lock zu lange gehalten
void process_request(Request req) {
    std::lock_guard<std::mutex> lock(mutex_);  // Lock zu früh
    
    // Teure I/O-Operation unter Lock!
    auto data = load_from_disk(req.file);
    
    // Nur dieser Teil braucht Lock
    cache_[req.id] = data;
}

// ✅ GUT - Lock minimiert
void process_request(Request req) {
    // I/O ohne Lock
    auto data = load_from_disk(req.file);
    
    // Lock nur für kritische Sektion
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_[req.id] = data;
    }
}
```

### 2. Lock-Hierarchie dokumentieren

Dokumentiere Lock-Reihenfolge um Deadlocks zu vermeiden:

```cpp
class Database {
    // Lock-Hierarchie (IMMER in dieser Reihenfolge locken!):
    // 1. connection_pool_mutex_
    // 2. transaction_mutex_
    // 3. cache_mutex_
    
    std::mutex connection_pool_mutex_;   // Level 1
    std::mutex transaction_mutex_;       // Level 2
    std::mutex cache_mutex_;             // Level 3
};
```

### 3. const-correctness für Thread-Safety

```cpp
class ThreadSafeCache {
    mutable std::shared_mutex mutex_;  // mutable für const-Methoden
    std::unordered_map<std::string, Value> data_;
    
public:
    // const-Methode mit shared_lock
    std::optional<Value> get(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        // ...
    }
    
    // Non-const-Methode mit unique_lock
    void put(const std::string& key, Value val) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        // ...
    }
};
```

### 4. RAII für Lock-Management

```cpp
// ❌ FEHLERANFÄLLIG - Manuelle Locks
void process() {
    mutex_.lock();
    
    if (error_condition) {
        // ⚠️ Vergessen zu unlocked! Deadlock!
        return;
    }
    
    // ... Processing ...
    
    mutex_.unlock();
}

// ✅ RICHTIG - RAII
void process() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (error_condition) {
        return;  // ✅ Lock wird automatisch freigegeben
    }
    
    // ... Processing ...
    
}  // ✅ Lock wird automatisch freigegeben
```

## Testing mit ThreadSanitizer

### ThreadSanitizer (TSan) einrichten

**1. CMake-Build mit TSan:**

```bash
# Build mit ThreadSanitizer
cmake -B build -DTHEMIS_ENABLE_TSAN=ON
cmake --build build

# Tests ausführen
cd build
ctest --output-on-failure
```

**2. TSan-Umgebungsvariablen:**

```bash
# TSan-Optionen konfigurieren
export TSAN_OPTIONS="suppressions=../tsan_suppressions.txt:halt_on_error=0:log_path=tsan"

# Tests ausführen
./build/tests/all_tests
```

**3. TSan-Suppressions erstellen:**

Datei: `tsan_suppressions.txt`

```
# ThreadSanitizer Suppressions

# Suppress known third-party races
race:^rocksdb::
race:^spdlog::

# Suppress benign races (mit Vorsicht!)
# race:benign_function_name
```

### Stress-Tests für Concurrency

**Beispiel: Concurrent Model Loader Test**

```cpp
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "llm/model_loader.h"

TEST(ThreadSafety, ConcurrentModelLoading) {
    LazyModelLoader loader(/* config */);
    
    const int NUM_THREADS = 10;
    const int LOADS_PER_THREAD = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    // Starte mehrere Threads die gleichzeitig Modelle laden
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&loader, &success_count]() {
            for (int j = 0; j < LOADS_PER_THREAD; ++j) {
                auto* model = loader.getOrLoadModel(
                    "test-model",
                    "/path/to/model.gguf"
                );
                if (model) {
                    success_count.fetch_add(1);
                }
            }
        });
    }
    
    // Warte auf Completion
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify: Alle Loads sollten erfolgreich sein
    EXPECT_EQ(success_count.load(), NUM_THREADS * LOADS_PER_THREAD);
    
    // Verify: Statistiken sind konsistent
    auto stats = loader.getStatistics();
    EXPECT_GT(stats.cache_hits + stats.cache_misses, 0);
}
```

**Beispiel: Concurrent Scheduler Test**

```cpp
TEST(ThreadSafety, ConcurrentSchedulerAccess) {
    ContinuousBatchScheduler scheduler(/* config */, /* kv_cache */);
    
    std::atomic<int> submitted{0};
    std::atomic<int> completed{0};
    
    // Producer threads
    std::vector<std::thread> producers;
    for (int i = 0; i < 5; ++i) {
        producers.emplace_back([&]() {
            for (int j = 0; j < 200; ++j) {
                InferenceRequest req;
                req.prompt = "Test prompt " + std::to_string(j);
                
                auto callback = [&](const InferenceResponse& resp) {
                    completed.fetch_add(1);
                };
                
                scheduler.submitRequest(
                    req,
                    RequestPriority::NORMAL,
                    callback
                );
                submitted.fetch_add(1);
            }
        });
    }
    
    // Wait for completion
    for (auto& t : producers) {
        t.join();
    }
    
    EXPECT_EQ(submitted.load(), 1000);
    
    // Scheduler state should be consistent
    auto stats = scheduler.getStats();
    EXPECT_GT(stats.total_requests, 0);
}
```

### TSan-Violations interpretieren

**Beispiel TSan-Output:**

```
==================
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 8 at 0x7b0400001234 by thread T2:
    #0 LazyModelLoader::loadModelInternal() src/llm/model_loader.cpp:785
    #1 LazyModelLoader::getOrLoadModel() src/llm/model_loader.cpp:144
    
  Previous read of size 8 at 0x7b0400001234 by thread T1:
    #0 LazyModelLoader::getCacheStats() src/llm/model_loader.cpp:515
    #1 main() main.cpp:42
    
  Location is heap block of size 64 at 0x7b0400001200 allocated by main thread:
    #0 operator new() <null>
    #1 LazyModelLoader::LazyModelLoader() src/llm/model_loader.cpp:18
    
SUMMARY: ThreadSanitizer: data race src/llm/model_loader.cpp:785
==================
```

**Analyse:**
1. **Write**: Thread T2 schreibt in `loadModelInternal()` Zeile 785
2. **Read**: Thread T1 liest in `getCacheStats()` Zeile 515
3. **Problem**: Gleichzeitiger Zugriff ohne Synchronisation
4. **Lösung**: Atomic verwenden oder unter Mutex schützen

## Code Review Checkliste

### Thread-Safety Checklist

Verwende diese Checkliste bei Code-Reviews:

- [ ] **Shared State identifiziert?**
  - Werden Variablen von mehreren Threads gelesen/geschrieben?
  - Sind alle Container thread-safe geschützt?

- [ ] **Synchronisation vorhanden?**
  - Sind Mutexes/Atomics für Shared State verwendet?
  - Sind Lock-Bereiche minimal gehalten?
  - Ist Lock-Reihenfolge konsistent dokumentiert?

- [ ] **Counter/Flags korrekt?**
  - Sind einfache Counter als `std::atomic` implementiert?
  - Sind Status-Flags atomar oder mutex-geschützt?

- [ ] **Reader-Writer Pattern?**
  - Sind Read-Heavy Workloads mit `std::shared_mutex` optimiert?
  - Wird `shared_lock` für Reads und `unique_lock` für Writes verwendet?

- [ ] **Deadlock-Prävention?**
  - Ist Lock-Reihenfolge dokumentiert und konsistent?
  - Wird `std::scoped_lock` für Multiple Locks verwendet?
  - Werden Locks vor I/O-Operationen freigegeben?

- [ ] **RAII verwendet?**
  - Werden `lock_guard` oder `unique_lock` statt manueller Locks verwendet?
  - Sind Exception-Safety garantiert?

- [ ] **Tests vorhanden?**
  - Gibt es Concurrency-Stress-Tests?
  - Läuft der Code unter ThreadSanitizer ohne Fehler?

## Häufige Probleme und Lösungen

### Problem 1: "It works on my machine"

**Symptom:** Code funktioniert im Debug-Build, crasht aber im Release

**Ursache:** Compiler-Optimierungen können Race Conditions aufdecken

**Lösung:**
1. Mit TSan testen: `cmake -DTHEMIS_ENABLE_TSAN=ON`
2. Atomics mit korrekter Memory Order verwenden
3. Alle Shared State schützen

### Problem 2: Sporadische Crashes

**Symptom:** Seltene Crashes die nicht reproduzierbar sind

**Ursache:** Race Conditions oder Use-After-Free

**Lösung:**
1. TSan-Build erstellen und ausführen
2. Stress-Tests mit hoher Concurrency
3. Valgrind Helgrind verwenden: `valgrind --tool=helgrind ./program`

### Problem 3: Performance-Degradation

**Symptom:** Multi-threaded Code ist langsamer als Single-threaded

**Ursache:** Lock-Contention oder False Sharing

**Lösung:**
1. Lock-Bereiche minimieren
2. Reader-Writer Locks für Read-Heavy Workloads
3. Lock-free Algorithmen erwägen
4. Cache-Line Padding für Hot Variables:
   ```cpp
   struct alignas(64) Counter {  // Cache-line aligned
       std::atomic<size_t> value{0};
   };
   ```

### Problem 4: Deadlock

**Symptom:** Programm hängt, Threads warten aufeinander

**Ursache:** Zirkuläre Lock-Abhängigkeiten

**Lösung:**
1. Lock-Hierarchie definieren und dokumentieren
2. `std::scoped_lock` für Multiple Locks verwenden
3. Timeout-basierte Locks erwägen:
   ```cpp
   std::unique_lock<std::mutex> lock(mutex_, std::chrono::seconds(5));
   if (!lock.owns_lock()) {
       // Timeout handling
   }
   ```

## Weiterführende Ressourcen

### Bücher
- C++ Concurrency in Action (Anthony Williams)
- The Art of Multiprocessor Programming (Herlihy & Shavit)

### Online
- [C++ Reference: Thread Support Library](https://en.cppreference.com/w/cpp/thread)
- [ThreadSanitizer Documentation](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)
- [Lock-free Programming](https://preshing.com/20120612/an-introduction-to-lock-free-programming/)

### ThemisDB-spezifisch
- `include/utils/thread_safety.h` - Utility-Klassen
- `tests/test_thread_pool_manager.cpp` - Beispiel-Tests
- `tsan_suppressions.txt` - TSan-Konfiguration

## Zusammenfassung

**Schlüsselpunkte:**

1. ✅ **Immer schützen:** Shared State immer mit Mutex oder Atomics schützen
2. ✅ **RAII verwenden:** Locks automatisch verwalten mit `lock_guard`
3. ✅ **Minimale Locks:** Lock-Bereiche so klein wie möglich halten
4. ✅ **Reader-Writer:** `shared_mutex` für Read-Heavy Workloads
5. ✅ **Atomics für Counter:** Einfache Counter als `std::atomic` implementieren
6. ✅ **TSan testen:** Regelmäßig mit ThreadSanitizer testen
7. ✅ **Deadlock vermeiden:** Lock-Hierarchie dokumentieren und einhalten
8. ✅ **Stress-Tests:** Concurrency-Tests in CI/CD einbauen

**Bei Unsicherheit:**
- Im Zweifel: **Mutex verwenden** (lieber langsam aber korrekt)
- Code-Review: **Thread-Safety Checklist** verwenden
- Testing: **TSan + Stress-Tests** sind Pflicht

---

*Letzte Aktualisierung: 2026-02-12*
*Version: 1.0*
