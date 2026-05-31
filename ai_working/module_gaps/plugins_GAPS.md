# plugins Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: plugins
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 187
- Actionable Findings (Critical + High): 97
- Affected Files: 9

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 18 |
| High | 79 |
| Medium | 90 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 39 |
| container | 31 |
| raii | 31 |
| exception_safety | 29 |
| performance | 17 |
| reliability | 11 |
| memory | 9 |
| security | 7 |
| concurrency | 5 |
| legacy_duplication | 4 |
| determinism | 2 |
| platform | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/plugins/plugin_manager.cpp | 90 | 8 | 39 | 43 | 0 |
| src/plugins/signed_plugin_repository.cpp | 24 | 0 | 10 | 14 | 0 |
| src/plugins/plugin_hot_plug_monitor.cpp | 23 | 3 | 9 | 11 | 0 |
| src/plugins/plugin_health_monitor.cpp | 19 | 1 | 10 | 8 | 0 |
| src/plugins/oci_registry_client.cpp | 17 | 3 | 4 | 10 | 0 |
| src/plugins/wasm_plugin_loader.cpp | 7 | 1 | 4 | 2 | 0 |
| src/plugins/plugin_metrics.cpp | 3 | 1 | 0 | 2 | 0 |
| src/plugins/rpc_service_registry.cpp | 3 | 1 | 2 | 0 | 0 |
| src/plugins/plugin_system_edition.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/plugins/plugin_manager.cpp
Total findings: 90

- Line 559: severity=CRITICAL; category=double_lock
  Description: Potential double-lock on same mutex: lock
  Remediation: Ensure proper lock nesting or use recursive_mutex if needed
  Context: lock.lock();
- Line 559: severity=CRITICAL; category=missing_lock
  Description: Raw .lock() without corresponding .unlock() — use RAII instead
  Remediation: Replace with std::lock_guard<std::mutex> guard(mutex);
  Context: lock.lock();
- Line 559: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 564: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 921: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = type_index_.find(type);
- Line 924: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator plugin_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto plugin_it = plugins_.find(name);
- Line 1066: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto msg = fmt::format("Failed to load new plugin binary for '{}' from '{}'", name, plugin_path);
- Line 1435: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool started = hot_plug_monitor_->start();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 99: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: char buffer[8192];
- Line 282: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Parse type (string form and legacy integer form)
  Confidence: band=high; score=0.8
- Line 312: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy manifest compatibility: single library field
  Confidence: band=high; score=0.8
- Line 376: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(directory)) {
- Line 923: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto plugin_it = plugins_.find(name);
- Line 923: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& name : it->second) {
  Confidence: band=very_high; score=0.9
- Line 938: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& pair : plugins_) {
  Confidence: band=very_high; score=0.9
- Line 949: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& pair : plugins_) {
  Confidence: band=very_high; score=0.9
- Line 1110: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: new_instance = nullptr;
  Context: else delete new_instance;
- Line 1146: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: new_instance = nullptr;
  Context: else delete new_instance;
- Line 1179: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: old_raw = nullptr;
  Context: else delete old_raw;
- Line 1183: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(RELOAD_UNLOAD_DELAY_MS);
  Confidence: band=very_high; score=0.9
- Line 1263: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& name : topo_order) {
  Confidence: band=very_high; score=0.9
- Line 1269: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 1461: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return hot_plug_monitor_ != nullptr && hot_plug_monitor_->isRunning();
- Line 1461: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return hot_plug_monitor_ != nullptr && hot_plug_monitor_->isRunning();
- Line 1485: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [plugin_name, entry] : plugins_) {
  Confidence: band=very_high; score=0.9
- Line 1488: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& dep : entry.manifest.dependencies) {
  Confidence: band=very_high; score=0.9
- Line 1508: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& listener : listeners_copy) {
  Confidence: band=very_high; score=0.9
- Line 75: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: dlclose(handle);
- Line 95: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 102: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 110: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 113: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.dependencies.push_back(dep.get<std::string>());
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: manifest.dependencies.push_back(dep.get<std::string>());
- Line 415: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 525: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!cycle_desc.empty()) cycle_desc += "; ";
  Confidence: band=high; score=0.74
- Line 526: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!cycle_desc.empty()) cycle_desc += "; ";
- Line 527: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cycle_desc += " -> ";
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) cycle_desc += " -> ";
- Line 528: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) cycle_desc += " -> ";
- Line 743: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: type_index_[plugin->getType()].push_back(plugin_name);
- Line 824: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!dep_list.empty()) dep_list += ", ";
  Confidence: band=high; score=0.74
- Line 825: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!dep_list.empty()) dep_list += ", ";
- Line 852: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: destroyFunc(entry.instance.release());
- Line 885: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: destroyFunc(entry.instance.release());
- Line 925: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(plugin_it->second.instance.get());
  Confidence: band=high; score=0.74
- Line 926: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(plugin_it->second.instance.get());
- Line 938: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second.manifest);
  Confidence: band=high; score=0.74
- Line 939: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(pair.second.manifest);
- Line 950: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 951: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(pair.first);
- Line 988: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!dep_list.empty()) dep_list += ", ";
  Confidence: band=high; score=0.74
- Line 989: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!dep_list.empty()) dep_list += ", ";
- Line 1103: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1110: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: else delete new_instance;
- Line 1146: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: else delete new_instance;
- Line 1165: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: entry.instance.release();
- Line 1179: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: else delete old_raw;
- Line 1225: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!cycle_desc.empty()) cycle_desc += "; ";
  Confidence: band=high; score=0.74
- Line 1226: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!cycle_desc.empty()) cycle_desc += "; ";
- Line 1227: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cycle_desc += " -> ";
  Confidence: band=high; score=0.74
- Line 1228: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) cycle_desc += " -> ";
- Line 1228: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) cycle_desc += " -> ";
- Line 1241: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!missing_desc.empty()) missing_desc += "; ";
  Confidence: band=high; score=0.74
- Line 1242: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!missing_desc.empty()) missing_desc += "; ";
- Line 1489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents.push_back(plugin_name);
  Confidence: band=high; score=0.74
- Line 1489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents.push_back(plugin_name);
  Confidence: band=high; score=0.74
- Line 1490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dependents.push_back(plugin_name);
- Line 1579: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Download from https://marketplace.themisdb.io/\n"
- Line 1581: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Place in $THEMIS_HOME/plugins/\n"

### src/plugins/signed_plugin_repository.cpp
Total findings: 24

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 57: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
- Line 89: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& k : pinned_keys_) {
  Confidence: band=very_high; score=0.9
- Line 114: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return k != nullptr && k->active;
- Line 114: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return k != nullptr && k->active;
- Line 134: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& e : entries_) {
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& e : entries_) {
  Confidence: band=very_high; score=0.9
- Line 185: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& e : entries_) {
  Confidence: band=very_high; score=0.9
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pinned_keys_.push_back(std::move(key));
  Confidence: band=high; score=0.74
- Line 95: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pinned_keys_.push_back(std::move(key));
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries_.push_back(entry);
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries_.push_back(entry);
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e);
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(e);
- Line 233: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 234: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 235: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 236: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 237: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t";  break;
- Line 307: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 318: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 319: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);

### src/plugins/plugin_hot_plug_monitor.cpp
Total findings: 23

- Line 218: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ssize_t length = read(inotify_fd_, buffer, sizeof(buffer));
- Line 359: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int dir_fd = open(watch_directory_.c_str(), O_RDONLY);
- Line 367: severity=CRITICAL; category=missing_dtor
  Description: Class kevent allocates resources but has no destructor
  Remediation: Add explicit destructor: ~kevent() { /* cleanup */ }
  Context: class/struct kevent
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 133: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(500));
- Line 153: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(500));
- Line 241: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr += sizeof(struct inotify_event) + event->len;
- Line 245: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handleFileEvent(event->name, file_event);
- Line 248: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr += sizeof(struct inotify_event) + event->len;
- Line 313: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // A rename/move into the directory counts as a new file
- Line 385: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(watch_directory_)) {
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filename.push_back(static_cast<char>(wc));
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filename.push_back(static_cast<char>(wc));
- Line 362: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(kq);
- Line 375: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(dir_fd);
- Line 376: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(kq);
- Line 381: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, fs::file_time_type> known_files;
  Confidence: band=high; score=0.74
- Line 383: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, fs::file_time_type> current_files;
  Confidence: band=high; score=0.74
- Line 454: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(dir_fd);
- Line 455: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(kq);
- Line 528: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(inotify_fd_);
- Line 574: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(inotify_fd_);

### src/plugins/plugin_health_monitor.cpp
Total findings: 19

- Line 105: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = monitored_plugins_.find(name);
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3010 [base] Plugin health monitoring and auto-restart on failure (2026-03-12T06:08:36Z)
- Line 206: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [_, mp] : monitored_plugins_) {
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 230: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 279: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(200));
  Confidence: band=very_high; score=0.9
- Line 279: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(200));
- Line 289: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [n, _] : monitored_plugins_) {
  Confidence: band=very_high; score=0.9
- Line 294: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& name : names) {
  Confidence: band=very_high; score=0.9
- Line 476: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find(strategies.begin(), strategies.end(), action);
- Line 581: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("PluginHealthMonitor [ADMIN ALERT]: plugin '{}' requires attention — {}",
- Line 187: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, MonitoredPlugin> PluginHealthMonitor::getAllPluginStats() const {
  Confidence: band=medium; score=0.66
- Line 209: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: gs.healthy_plugins_count++;
- Line 213: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: gs.unhealthy_plugins_count++;
- Line 225: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: event_callbacks_.push_back(std::move(callback));
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(n);
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: names.push_back(n);
- Line 506: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 651: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/plugins/oci_registry_client.cpp
Total findings: 17

- Line 78: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator colon may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto colon = line.find(':');
- Line 533: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = auth_configs_.find(ref.registry);
- Line 670: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = auth_configs_.find(ref.registry);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3342 [plugins] Remote plugin loading from OCI registries (2026-03-12T07:06:26Z)
- Line 523: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Accepted media types (OCI + Docker v2 for compatibility).
  Confidence: band=high; score=0.8
- Line 663: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string blob_url = base + "/v2/" + ref.name + "/blobs/" + plugin_layer->digest;
- Line 101: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 108: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 116: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 119: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 259: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> response_headers;
  Confidence: band=medium; score=0.66
- Line 363: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: writer.file.close();
- Line 492: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 538: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: accept_headers.push_back("Authorization: Bearer " + auth.bearer_token);
- Line 575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.layers.push_back(std::move(l));
  Confidence: band=high; score=0.74
- Line 576: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: manifest.layers.push_back(std::move(l));

### src/plugins/wasm_plugin_loader.cpp
Total findings: 7

- Line 295: severity=CRITICAL; category=missing_dtor
  Description: Class WasmtimeBundle allocates resources but has no destructor
  Remediation: Add explicit destructor: ~WasmtimeBundle() { /* cleanup */ }
  Context: class/struct WasmtimeBundle
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 377: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: b = nullptr;
  Context: delete b;
- Line 95: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void operator()(EVP_MD_CTX* p) const noexcept { EVP_MD_CTX_free(p); }
- Line 377: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete b;

### src/plugins/plugin_metrics.cpp
Total findings: 3

- Line 129: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = stats_.find(plugin);
- Line 87: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.latency_samples.push_back(latency_ms);
- Line 120: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, PluginMetrics::PluginStats> PluginMetrics::getAllStats() const {
  Confidence: band=high; score=0.74

### src/plugins/rpc_service_registry.cpp
Total findings: 3

- Line 37: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = services_.find(name);
- Line 38: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return (it != services_.end()) ? it->second : nullptr;
- Line 38: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != services_.end()) ? it->second : nullptr;

### src/plugins/plugin_system_edition.cpp
Total findings: 1

- Line 13: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * DEPRECATED - Merged into plugin_manager.cpp / plugin_manager.h
  Confidence: band=high; score=0.8

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
