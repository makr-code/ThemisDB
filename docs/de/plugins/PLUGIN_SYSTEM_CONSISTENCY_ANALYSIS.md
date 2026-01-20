# Plugin-System: Konsistenz-Analyse und Hot-Reload-Implementierung

**Datum:** 20. Januar 2026  
**Kategorie:** 🔍 Analyse & Verbesserungen  
**Status:** Identifizierte Probleme

---

## Zusammenfassung

Diese Analyse untersucht die Konsistenz der Plugin-System-Implementierung in ThemisDB und identifiziert Verbesserungspotenziale, insbesondere im Bereich Hot-Reload/Hot-Plug-Funktionalität.

---

## Identifizierte Probleme

### 1. ❌ **KRITISCH: Race Condition in `autoLoadPlugins()`**

**Datei:** `src/plugins/plugin_manager.cpp`, Zeilen 635-664

**Problem:**

```cpp
size_t PluginManager::autoLoadPlugins() {
    std::lock_guard<std::mutex> lock(mutex_);  // Lock erworben
    
    // ... Code ...
    
    for (const auto& [priority, name] : to_load) {
        // ⚠️ PROBLEM: Manuelles unlock/lock während std::lock_guard aktiv ist!
        mutex_.unlock();                      // Manuelles Unlock
        auto* plugin = loadPlugin(name);      // loadPlugin() versucht Lock zu erwerben
        mutex_.lock();                        // Manuelles Lock
        
        if (plugin) {
            loaded++;
        }
    }
    
    return loaded;  // std::lock_guard destructor ruft unlock() erneut auf
}
```

**Warum problematisch:**

1. **Undefined Behavior**: `std::lock_guard` verwaltet den Mutex-Lifecycle automatisch
2. **Doppeltes Unlock**: Am Ende der Funktion ruft der `lock_guard` Destruktor `unlock()` auf, obwohl der Mutex bereits manuell entsperrt wurde
3. **Exception-Unsicherheit**: Bei Exception zwischen `unlock()` und `lock()` bleibt Mutex entsperrt

**Auswirkung:**
- Potenzielle Race Conditions
- Undefined Behavior gemäß C++ Standard
- Deadlock-Gefahr bei Exceptions

**Lösung:**

```cpp
size_t PluginManager::autoLoadPlugins() {
    std::vector<std::pair<int, std::string>> to_load;
    
    // Scope für Lock 1: Sammle Plugins
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (const auto& pair : plugins_) {
            if (pair.second.manifest.auto_load && !pair.second.loaded) {
                to_load.push_back({pair.second.manifest.load_priority, pair.first});
            }
        }
        
        std::sort(to_load.begin(), to_load.end());
    }  // Lock wird hier automatisch freigegeben
    
    // Laden ohne Lock (loadPlugin hat eigenen Lock)
    size_t loaded = 0;
    for (const auto& [priority, name] : to_load) {
        auto* plugin = loadPlugin(name);
        if (plugin) {
            loaded++;
        }
    }
    
    THEMIS_INFO("Auto-loaded {} plugins", loaded);
    return loaded;
}
```

---

### 2. ⚠️ **Unvollständige Hot-Reload-Implementierung**

**Datei:** `src/plugins/plugin_manager.cpp`, Zeilen 627-633

**Aktueller Code:**

```cpp
bool PluginManager::reloadPlugin(const std::string& name) {
    // Unload first
    unloadPlugin(name);
    
    // Then reload
    return loadPlugin(name) != nullptr;
}
```

**Probleme:**

1. **Keine Zustandssicherung**: Plugin-Zustand geht verloren
2. **Keine Dependency-Prüfung**: Abhängige Plugins könnten abstürzen
3. **Keine atomare Operation**: Wenn Reload fehlschlägt, ist Plugin nicht mehr verfügbar
4. **Keine Rollback-Möglichkeit**: Bei Fehler kein Zurückrollen auf alte Version
5. **Keine Event-Benachrichtigung**: Andere Komponenten wissen nicht über Reload
6. **Keine Konfigurationsmigration**: Neue Konfigurationsparameter fehlen

**Verbesserte Implementierung:**

```cpp
bool PluginManager::reloadPlugin(const std::string& name) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        THEMIS_ERROR("Plugin not found: {}", name);
        return false;
    }
    
    // 1. Prüfe Dependencies
    std::vector<std::string> dependents = findDependentPlugins(name);
    if (!dependents.empty()) {
        THEMIS_ERROR("Cannot reload plugin {} - {} plugins depend on it", 
            name, dependents.size());
        return false;
    }
    
    auto& entry = it->second;
    
    // 2. Benachrichtige über bevorstehendes Reload
    notifyPluginReload(name, PluginReloadPhase::BEFORE_UNLOAD);
    
    // 3. Speichere Plugin-Zustand
    std::string saved_state;
    if (entry.instance) {
        try {
            // Plugin muss IStatefulPlugin implementieren für State-Persistierung
            auto* stateful = dynamic_cast<IStatefulPlugin*>(entry.instance.get());
            if (stateful) {
                saved_state = stateful->saveState();
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to save plugin state: {}", e.what());
        }
    }
    
    // 4. Berechne aktuellen Hash als Backup
    std::string old_hash = entry.file_hash;
    std::string old_path = entry.path;
    
    // 5. Unload (mit Lock)
    if (entry.instance) {
        entry.instance->shutdown();
        
        auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(
            getSymbol(entry.library_handle, "destroyPlugin")
        );
        
        if (destroyFunc) {
            destroyFunc(entry.instance.release());
        } else {
            entry.instance.reset();
        }
    }
    
    unloadLibrary(entry.library_handle);
    entry.library_handle = nullptr;
    entry.instance = nullptr;
    entry.loaded = false;
    
    // 6. Benachrichtige über Unload-Abschluss
    notifyPluginReload(name, PluginReloadPhase::AFTER_UNLOAD);
    
    // 7. Warte kurz (ermöglicht Cleanup)
    lock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    lock.lock();
    
    // 8. Reload (verifiziert automatisch neue Version)
    std::string error_message;
    if (!verifyPlugin(entry.path, error_message)) {
        THEMIS_ERROR("Plugin verification failed after reload: {}", error_message);
        
        // Rollback-Versuch (falls alte Version noch verfügbar)
        // In Produktionsumgebung sollte hier ein Backup-System greifen
        return false;
    }
    
    // 9. Lade neue Version
    void* handle = loadLibrary(entry.path);
    if (!handle) {
        THEMIS_ERROR("Failed to reload plugin library: {}", entry.path);
        return false;
    }
    
    auto createFunc = reinterpret_cast<CreatePluginFunc>(
        getSymbol(handle, "createPlugin")
    );
    if (!createFunc) {
        THEMIS_ERROR("Plugin does not export createPlugin: {}", entry.path);
        unloadLibrary(handle);
        return false;
    }
    
    IThemisPlugin* plugin = createFunc();
    if (!plugin) {
        THEMIS_ERROR("Failed to create plugin instance: {}", name);
        unloadLibrary(handle);
        return false;
    }
    
    // 10. Initialisiere mit gespeichertem Zustand
    std::string init_config = "{}";
    if (!saved_state.empty()) {
        nlohmann::json config;
        config["restored_state"] = saved_state;
        init_config = config.dump();
    }
    
    if (!plugin->initialize(init_config.c_str())) {
        THEMIS_ERROR("Failed to initialize reloaded plugin: {}", name);
        
        auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(
            getSymbol(handle, "destroyPlugin")
        );
        if (destroyFunc) {
            destroyFunc(plugin);
        }
        unloadLibrary(handle);
        return false;
    }
    
    // 11. Aktualisiere Entry
    entry.library_handle = handle;
    entry.instance.reset(plugin);
    entry.loaded = true;
    entry.file_hash = calculateFileHash(entry.path);
    
    // 12. Benachrichtige über erfolgreichen Reload
    notifyPluginReload(name, PluginReloadPhase::AFTER_LOAD);
    
    THEMIS_INFO("Successfully reloaded plugin: {} (old hash: {}..., new hash: {}...)",
        name, old_hash.substr(0, 16), entry.file_hash.substr(0, 16));
    
    return true;
}
```

**Zusätzliche Hilfsfunktionen:**

```cpp
enum class PluginReloadPhase {
    BEFORE_UNLOAD,
    AFTER_UNLOAD,
    AFTER_LOAD
};

std::vector<std::string> PluginManager::findDependentPlugins(const std::string& name) const {
    std::vector<std::string> dependents;
    
    for (const auto& [plugin_name, entry] : plugins_) {
        if (plugin_name == name || !entry.loaded) continue;
        
        for (const auto& dep : entry.manifest.dependencies) {
            if (dep == name) {
                dependents.push_back(plugin_name);
                break;
            }
        }
    }
    
    return dependents;
}

void PluginManager::notifyPluginReload(
    const std::string& name,
    PluginReloadPhase phase
) {
    // Event an registrierte Listener senden
    for (auto& listener : reload_listeners_) {
        try {
            listener(name, phase);
        } catch (const std::exception& e) {
            THEMIS_ERROR("Reload listener failed: {}", e.what());
        }
    }
}
```

**Erweitertes Interface für Stateful Plugins:**

```cpp
// include/plugins/plugin_interface.h

class IStatefulPlugin {
public:
    virtual ~IStatefulPlugin() = default;
    
    /**
     * @brief Save plugin state before reload
     * @return Serialized state as JSON string
     */
    virtual std::string saveState() = 0;
    
    /**
     * @brief Restore plugin state after reload
     * @param state Previously saved state
     * @return true if restored successfully
     */
    virtual bool restoreState(const std::string& state) = 0;
};
```

---

### 3. ⚠️ **Fehlende Hot-Plug-Funktionalität**

**Was ist Hot-Plug?**

Hot-Plug ermöglicht das Hinzufügen und Entfernen von Plugins zur Laufzeit ohne Neustart, inklusive:
- Automatische Erkennung neuer Plugin-Dateien
- Filesystem-Monitoring für Plugin-Verzeichnis
- Event-basierte Plugin-Registrierung

**Aktueller Stand:**

- ✅ Plugins können zur Laufzeit geladen werden (`loadPlugin()`)
- ✅ Plugins können zur Laufzeit entladen werden (`unloadPlugin()`)
- ❌ Keine automatische Erkennung neuer Plugins
- ❌ Kein Filesystem-Monitoring
- ❌ Keine Watch-Funktionalität für Plugin-Verzeichnis

**Empfohlene Implementierung:**

```cpp
class PluginHotPlugMonitor {
private:
    std::string watch_directory_;
    std::thread monitor_thread_;
    std::atomic<bool> running_{false};
    PluginManager* plugin_manager_;
    
#ifdef _WIN32
    // Windows: ReadDirectoryChangesW
    HANDLE dir_handle_;
#else
    // Linux: inotify
    int inotify_fd_;
    int watch_descriptor_;
#endif
    
public:
    PluginHotPlugMonitor(
        PluginManager* manager,
        const std::string& directory
    ) : plugin_manager_(manager), watch_directory_(directory) {}
    
    void start() {
        running_ = true;
        
#ifdef _WIN32
        // Windows-Implementierung
        dir_handle_ = CreateFileA(
            watch_directory_.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
        );
        
        if (dir_handle_ == INVALID_HANDLE_VALUE) {
            THEMIS_ERROR("Failed to watch directory: {}", watch_directory_);
            return;
        }
        
        monitor_thread_ = std::thread([this]() {
            watchDirectoryWindows();
        });
#else
        // Linux-Implementierung
        inotify_fd_ = inotify_init();
        if (inotify_fd_ < 0) {
            THEMIS_ERROR("Failed to initialize inotify");
            return;
        }
        
        watch_descriptor_ = inotify_add_watch(
            inotify_fd_,
            watch_directory_.c_str(),
            IN_CREATE | IN_MODIFY | IN_DELETE
        );
        
        if (watch_descriptor_ < 0) {
            THEMIS_ERROR("Failed to watch directory: {}", watch_directory_);
            return;
        }
        
        monitor_thread_ = std::thread([this]() {
            watchDirectoryLinux();
        });
#endif
    }
    
    void stop() {
        running_ = false;
        
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        
#ifdef _WIN32
        if (dir_handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(dir_handle_);
        }
#else
        if (watch_descriptor_ >= 0) {
            inotify_rm_watch(inotify_fd_, watch_descriptor_);
        }
        if (inotify_fd_ >= 0) {
            close(inotify_fd_);
        }
#endif
    }
    
private:
    void watchDirectoryLinux() {
        char buffer[4096];
        
        while (running_) {
            ssize_t length = read(inotify_fd_, buffer, sizeof(buffer));
            if (length < 0) {
                if (errno != EINTR) {
                    THEMIS_ERROR("inotify read error: {}", strerror(errno));
                }
                continue;
            }
            
            size_t i = 0;
            while (i < length) {
                struct inotify_event* event = 
                    reinterpret_cast<struct inotify_event*>(&buffer[i]);
                
                if (event->len > 0) {
                    handleFileEvent(event->name, event->mask);
                }
                
                i += sizeof(struct inotify_event) + event->len;
            }
        }
    }
    
    void watchDirectoryWindows() {
        // Windows-Implementierung mit ReadDirectoryChangesW
        // ... (analog zu Linux-Version)
    }
    
    void handleFileEvent(const std::string& filename, uint32_t mask) {
        // Nur Plugin-Dateien beachten
        if (!isPluginFile(filename)) {
            return;
        }
        
        std::string full_path = watch_directory_ + "/" + filename;
        
        if (mask & IN_CREATE) {
            THEMIS_INFO("New plugin detected: {}", filename);
            
            // Warte kurz, bis Datei vollständig geschrieben
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            // Versuche Plugin zu laden
            plugin_manager_->scanPluginDirectory(watch_directory_);
            
            // Extrahiere Plugin-Namen aus Manifest
            auto manifest_path = full_path + "/plugin.json";
            if (std::filesystem::exists(manifest_path)) {
                auto manifest = plugin_manager_->loadManifest(manifest_path);
                if (manifest) {
                    plugin_manager_->loadPlugin(manifest->name);
                }
            }
        } else if (mask & IN_MODIFY) {
            THEMIS_INFO("Plugin modified: {}", filename);
            
            // Hot-Reload wenn bereits geladen
            // ... (Plugin-Namen ermitteln und reloadPlugin aufrufen)
        } else if (mask & IN_DELETE) {
            THEMIS_INFO("Plugin removed: {}", filename);
            
            // Plugin entladen
            // ... (Plugin-Namen ermitteln und unloadPlugin aufrufen)
        }
    }
    
    bool isPluginFile(const std::string& filename) {
        return filename.ends_with(".dll") ||
               filename.ends_with(".so") ||
               filename.ends_with(".dylib") ||
               filename == "plugin.json";
    }
};
```

**Integration in PluginManager:**

```cpp
class PluginManager {
private:
    std::unique_ptr<PluginHotPlugMonitor> hot_plug_monitor_;
    
public:
    void enableHotPlug(const std::string& directory) {
        hot_plug_monitor_ = std::make_unique<PluginHotPlugMonitor>(this, directory);
        hot_plug_monitor_->start();
        THEMIS_INFO("Hot-plug monitoring enabled for: {}", directory);
    }
    
    void disableHotPlug() {
        if (hot_plug_monitor_) {
            hot_plug_monitor_->stop();
            hot_plug_monitor_.reset();
            THEMIS_INFO("Hot-plug monitoring disabled");
        }
    }
};
```

---

### 4. ⚠️ **Fehlende Dependency-Verwaltung**

**Problem:** Plugin-Dependencies werden im Manifest definiert, aber nicht aktiv verwaltet.

**Aktueller Code prüft nur beim Laden:**

```cpp
// In loadManifest():
if (j.contains("dependencies") && j["dependencies"].is_array()) {
    for (const auto& dep : j["dependencies"]) {
        manifest.dependencies.push_back(dep.get<std::string>());
    }
}
```

**Aber:**
- ❌ Keine Überprüfung, ob Dependencies verfügbar sind
- ❌ Keine automatische Dependency-Auflösung
- ❌ Keine Reihenfolge beim Laden (topologische Sortierung)
- ❌ Keine Zirkuläre-Dependency-Erkennung

**Empfohlene Lösung:**

```cpp
class PluginDependencyResolver {
public:
    struct DependencyGraph {
        std::map<std::string, std::vector<std::string>> dependencies;
        std::map<std::string, std::vector<std::string>> dependents;
    };
    
    /**
     * @brief Build dependency graph from manifests
     */
    static DependencyGraph buildGraph(
        const std::map<std::string, PluginEntry>& plugins
    ) {
        DependencyGraph graph;
        
        for (const auto& [name, entry] : plugins) {
            graph.dependencies[name] = entry.manifest.dependencies;
            
            for (const auto& dep : entry.manifest.dependencies) {
                graph.dependents[dep].push_back(name);
            }
        }
        
        return graph;
    }
    
    /**
     * @brief Detect circular dependencies
     */
    static std::vector<std::vector<std::string>> detectCircularDependencies(
        const DependencyGraph& graph
    ) {
        std::vector<std::vector<std::string>> cycles;
        std::set<std::string> visited;
        std::set<std::string> recursion_stack;
        std::vector<std::string> current_path;
        
        for (const auto& [name, _] : graph.dependencies) {
            if (visited.find(name) == visited.end()) {
                detectCyclesRecursive(name, graph, visited, 
                    recursion_stack, current_path, cycles);
            }
        }
        
        return cycles;
    }
    
    /**
     * @brief Compute load order (topological sort)
     */
    static std::vector<std::string> computeLoadOrder(
        const DependencyGraph& graph
    ) {
        std::vector<std::string> load_order;
        std::set<std::string> loaded;
        std::map<std::string, int> in_degree;
        
        // Berechne In-Degree für jeden Knoten
        for (const auto& [name, deps] : graph.dependencies) {
            in_degree[name] = deps.size();
        }
        
        // Finde Knoten ohne Dependencies
        std::queue<std::string> ready;
        for (const auto& [name, degree] : in_degree) {
            if (degree == 0) {
                ready.push(name);
            }
        }
        
        // Topologische Sortierung
        while (!ready.empty()) {
            std::string current = ready.front();
            ready.pop();
            
            load_order.push_back(current);
            loaded.insert(current);
            
            // Reduziere In-Degree von Dependents
            if (graph.dependents.count(current) > 0) {
                for (const auto& dependent : graph.dependents.at(current)) {
                    in_degree[dependent]--;
                    if (in_degree[dependent] == 0) {
                        ready.push(dependent);
                    }
                }
            }
        }
        
        // Wenn nicht alle Plugins geladen wurden, gibt es Zyklen
        if (load_order.size() != graph.dependencies.size()) {
            throw std::runtime_error("Circular dependency detected");
        }
        
        return load_order;
    }
    
private:
    static void detectCyclesRecursive(
        const std::string& node,
        const DependencyGraph& graph,
        std::set<std::string>& visited,
        std::set<std::string>& recursion_stack,
        std::vector<std::string>& current_path,
        std::vector<std::vector<std::string>>& cycles
    ) {
        visited.insert(node);
        recursion_stack.insert(node);
        current_path.push_back(node);
        
        if (graph.dependencies.count(node) > 0) {
            for (const auto& dep : graph.dependencies.at(node)) {
                if (recursion_stack.find(dep) != recursion_stack.end()) {
                    // Zyklus gefunden
                    auto cycle_start = std::find(
                        current_path.begin(), 
                        current_path.end(), 
                        dep
                    );
                    std::vector<std::string> cycle(
                        cycle_start, 
                        current_path.end()
                    );
                    cycle.push_back(dep);
                    cycles.push_back(cycle);
                } else if (visited.find(dep) == visited.end()) {
                    detectCyclesRecursive(dep, graph, visited, 
                        recursion_stack, current_path, cycles);
                }
            }
        }
        
        current_path.pop_back();
        recursion_stack.erase(node);
    }
};
```

**Verwendung:**

```cpp
size_t PluginManager::autoLoadPlugins() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 1. Dependency-Graph erstellen
    auto graph = PluginDependencyResolver::buildGraph(plugins_);
    
    // 2. Zirkuläre Dependencies prüfen
    auto cycles = PluginDependencyResolver::detectCircularDependencies(graph);
    if (!cycles.empty()) {
        THEMIS_ERROR("Circular dependencies detected:");
        for (const auto& cycle : cycles) {
            std::string cycle_str;
            for (const auto& node : cycle) {
                cycle_str += node + " -> ";
            }
            THEMIS_ERROR("  {}", cycle_str);
        }
        return 0;
    }
    
    // 3. Load-Order berechnen
    auto load_order = PluginDependencyResolver::computeLoadOrder(graph);
    
    // 4. Plugins in korrekter Reihenfolge laden
    size_t loaded = 0;
    for (const auto& name : load_order) {
        if (plugins_[name].manifest.auto_load && !plugins_[name].loaded) {
            mutex_.unlock();
            auto* plugin = loadPlugin(name);
            mutex_.lock();
            
            if (plugin) {
                loaded++;
            } else {
                THEMIS_ERROR("Failed to load plugin: {}", name);
                // Optional: Abbrechen wenn kritische Dependency fehlt
            }
        }
    }
    
    THEMIS_INFO("Auto-loaded {} plugins in dependency order", loaded);
    return loaded;
}
```

---

### 5. ℹ️ **Fehlende Metriken und Monitoring**

**Problem:** Keine Instrumentierung für Plugin-Operationen.

**Empfohlene Metriken:**

```cpp
class PluginMetrics {
public:
    struct PluginStats {
        // Timing
        std::chrono::milliseconds load_time{0};
        std::chrono::milliseconds last_reload_time{0};
        std::chrono::system_clock::time_point loaded_at;
        
        // Counts
        uint64_t reload_count = 0;
        uint64_t function_calls = 0;
        uint64_t errors = 0;
        
        // Resource usage
        size_t memory_bytes = 0;
        
        // Performance
        double avg_call_latency_ms = 0.0;
        double p95_call_latency_ms = 0.0;
        double p99_call_latency_ms = 0.0;
    };
    
    void recordLoad(const std::string& plugin, std::chrono::milliseconds duration) {
        stats_[plugin].load_time = duration;
        stats_[plugin].loaded_at = std::chrono::system_clock::now();
    }
    
    void recordReload(const std::string& plugin, std::chrono::milliseconds duration) {
        stats_[plugin].last_reload_time = duration;
        stats_[plugin].reload_count++;
    }
    
    void recordCall(const std::string& plugin, std::chrono::microseconds latency) {
        auto& stat = stats_[plugin];
        stat.function_calls++;
        
        // Update rolling average
        stat.avg_call_latency_ms = 
            (stat.avg_call_latency_ms * (stat.function_calls - 1) + 
             latency.count() / 1000.0) / stat.function_calls;
    }
    
    void recordError(const std::string& plugin) {
        stats_[plugin].errors++;
    }
    
    const PluginStats& getStats(const std::string& plugin) const {
        static PluginStats empty;
        auto it = stats_.find(plugin);
        return it != stats_.end() ? it->second : empty;
    }
    
    std::map<std::string, PluginStats> getAllStats() const {
        return stats_;
    }
    
private:
    std::map<std::string, PluginStats> stats_;
    mutable std::mutex mutex_;
};
```

---

## Empfohlene Verbesserungen (Priorisiert)

### Priorität 1: KRITISCH

1. ✅ **Race Condition in `autoLoadPlugins()` beheben**
   - Umschreiben mit korrekt geschachtelten Lock-Scopes
   - Test mit ThreadSanitizer

2. ✅ **Hot-Reload robuster machen**
   - Zustandssicherung implementieren
   - Dependency-Check vor Reload
   - Atomare Operation mit Rollback
   - Event-Benachrichtigungen

### Priorität 2: HOCH

3. ✅ **Dependency-Management implementieren**
   - Zirkuläre Dependency-Erkennung
   - Topologische Sortierung für Load-Order
   - Automatische Dependency-Auflösung

4. ✅ **Plugin-Metriken hinzufügen**
   - Load-Time, Reload-Count, Error-Count
   - OpenTelemetry Integration
   - Prometheus Exporter

### Priorität 3: MITTEL

5. ✅ **Hot-Plug-Monitoring implementieren**
   - Filesystem-Watcher (inotify/ReadDirectoryChangesW)
   - Automatische Plugin-Erkennung
   - Event-basierte Registrierung

6. ✅ **Plugin-Versioning verbessern**
   - Semantic Versioning Checks
   - Kompatibilitätsprüfung
   - Upgrade-/Downgrade-Pfade

### Priorität 4: NIEDRIG

7. ✅ **Testing erweitern**
   - Unit-Tests für alle kritischen Pfade
   - Integration-Tests für Hot-Reload
   - Stress-Tests für Parallelität

8. ✅ **Dokumentation verbessern**
   - API-Dokumentation vervollständigen
   - Best Practices Guide
   - Troubleshooting Guide

---

## Test-Plan

### Unit-Tests

```cpp
// Test: Race Condition Fix
TEST(PluginManagerTest, AutoLoadPluginsThreadSafe) {
    PluginManager manager;
    
    // Concurrent auto-load und manual load
    std::thread t1([&]() { manager.autoLoadPlugins(); });
    std::thread t2([&]() { manager.loadPlugin("test_plugin"); });
    
    t1.join();
    t2.join();
    
    // Keine Crashes, keine Race Conditions
}

// Test: Hot-Reload mit State
TEST(PluginManagerTest, HotReloadPreservesState) {
    PluginManager manager;
    manager.loadPlugin("stateful_plugin");
    
    auto* plugin = manager.getPlugin("stateful_plugin");
    // ... State setzen ...
    
    EXPECT_TRUE(manager.reloadPlugin("stateful_plugin"));
    
    auto* reloaded = manager.getPlugin("stateful_plugin");
    // ... State prüfen ...
}

// Test: Circular Dependency Detection
TEST(PluginManagerTest, DetectsCircularDependencies) {
    PluginManager manager;
    
    // Plugin A depends on B, B depends on C, C depends on A
    // Should fail to load
    EXPECT_EQ(manager.autoLoadPlugins(), 0);
}

// Test: Dependency Load Order
TEST(PluginManagerTest, LoadsInDependencyOrder) {
    PluginManager manager;
    manager.scanPluginDirectory("./test_plugins");
    
    manager.autoLoadPlugins();
    
    auto load_order = manager.getLoadOrder();
    // Verify dependencies are loaded before dependents
}
```

### Integration-Tests

```cpp
TEST(PluginIntegrationTest, HotPlugDetectsNewPlugins) {
    PluginManager manager;
    manager.enableHotPlug("./test_plugins");
    
    // Copy plugin file to watch directory
    std::filesystem::copy("./fixtures/new_plugin.so", 
                          "./test_plugins/new_plugin.so");
    
    // Wait for detection
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Plugin should be loaded automatically
    EXPECT_TRUE(manager.isPluginLoaded("new_plugin"));
}
```

---

## Zusammenfassung

Die Plugin-System-Implementierung in ThemisDB ist grundsätzlich gut strukturiert, weist aber einige kritische Inkonsistenzen und fehlende Features auf:

**Kritische Probleme:**
- Race Condition in `autoLoadPlugins()` - **MUSS behoben werden**
- Unvollständiger Hot-Reload ohne Zustandssicherung

**Fehlende Features:**
- Hot-Plug-Monitoring (automatische Plugin-Erkennung)
- Robuste Dependency-Verwaltung
- Metriken und Monitoring

**Empfehlung:**
1. Kritische Probleme sofort beheben (Priorität 1)
2. Dependency-Management und Metriken hinzufügen (Priorität 2)
3. Hot-Plug als nächste Feature-Iteration (Priorität 3)

---

**Nächste Schritte:**
1. ✅ Bugfixes implementieren (Race Condition)
2. ✅ Tests schreiben und ausführen
3. ✅ Code Review durchführen
4. ✅ Features gemäß Priorisierung umsetzen

---

**Autor:** ThemisDB Development Team  
**Reviewer:** [TBD]  
**Letzte Aktualisierung:** 20. Januar 2026
