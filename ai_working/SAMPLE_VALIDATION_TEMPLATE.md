
# REPRESENTATIVE GAP VALIDATION SAMPLE
Generated: 2026-06-02
Total Sample Size: 50

For each gap, assess as:
- **TP** = True Positive (real issue to fix)
- **FP** = False Positive (code is correct)
- **?** = Uncertain

## [ 1/50] plugins - MEDIUM
**File:** `src/plugins/plugin_manager.cpp:1034`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
 1034 | >>>             if (plugin_it != plugins_.end() && plugin_it->second.loaded) {
 1035 |                     result.push_back(plugin_it->second.instance.get());
 1036 |                 }
 1037 |             }
 1038 |         }
 1039 |     
 1040 |         return result;
 1041 |     }
 1042 |     
 1043 |     std::vector<PluginManifest> PluginManager::listPlugins() const {
 1044 |         std::lock_guard<std::mutex> lock(mutex_);
 1045 |     
 1046 |         std::vector<PluginManifest> result;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 2/50] importers - MEDIUM
**File:** `src/importers/schema_inference.cpp:297`
**Category:** performance
**Message:** std::map used only for lookups (consider std::unordered_map)

### Function Context
```cpp
  276 |         switch (t) {
  277 |             case SemanticType::EMAIL:            return "EMAIL";
  278 |             case SemanticType::PHONE:            return "PHONE";
  279 |             case SemanticType::CURRENCY:         return "CURRENCY";
  280 |             case SemanticType::LOCATION_COORD:   return "LOCATION_COORD";
  281 |             case SemanticType::ISO8601_DATETIME: return "ISO8601_DATETIME";
  282 |             case SemanticType::UUID:             return "UUID";
  283 |             case SemanticType::HASH_SHA256:      return "HASH_SHA256";
  284 |             case SemanticType::IP_ADDRESS:       return "IP_ADDRESS";
  285 |             case SemanticType::URL:              return "URL";
  286 |             default:                             return "UNKNOWN";
  287 |         }
  288 |     }
  289 |     
  290 |     // ---------------------------------------------------------------------------
  291 |     // Algorithm 3: cardinality estimation
  292 |     // ---------------------------------------------------------------------------
  293 |     
  294 |     std::vector<SchemaInferenceEngine::CardinalityEstimate>
  295 |     SchemaInferenceEngine::estimateCardinalities(
  296 |         const std::vector<InferenceTableSchema>& schemas,
  297 | >>>     const std::map<std::string, ColumnStatistics>& stats)
  298 |     {
  299 |         std::vector<CardinalityEstimate> estimates;
  300 |     
  301 |         // ── I2: Bounds check ─────────────────────────────────────────────────────
  302 |         if (schemas.size() > kMaxTableCount) {
  303 |             return estimates;  // Input too large; reject defensively
  304 |         }
  305 |     
  306 |         for (const auto& schema : schemas) {
  307 |             for (const auto& fk : schema.foreign_keys) {
  308 |                 const std::string& local_col = fk.first;
  309 |                 const std::string& ref = fk.second; // "other_table.other_col"
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 3/50] base - MEDIUM
**File:** `src/base/plugin_dependency_graph.cpp:135`
**Category:** performance
**Message:** std::map used only for lookups (consider std::unordered_map)

### Function Context
```cpp
  134 |     {
  135 | >>>     std::map<std::string, std::set<std::string>> adj;
  136 |         for (const auto& kv : nodes_) {
  137 |             adj[kv.first]; // ensure all nodes are present
  138 |         }
  139 |         for (const auto& e : edges_) {
  140 |             adj[e.from].insert(e.to);
  141 |         }
  142 |         return adj;
  143 |     }
  144 |     
  145 |     void PluginDependencyGraph::dfsVisit(
  146 |         const std::string& node,
  147 |         const std::map<std::string, std::set<std::string>>& adj,
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 4/50] query - MEDIUM
**File:** `src/query/functions/process_mining_functions.cpp:204`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
  198 |         for (const auto& e : proc.edges) {
  199 |             json ej;
  200 |             ej["id"]          = e.id;
  201 |             ej["from"]        = e.from;
  202 |             ej["to"]          = e.to;
  203 |             ej["frequency"]   = e.frequency;
  204 | >>>         ej["probability"] = e.probability;
  205 |             edges.push_back(std::move(ej));
  206 |         }
  207 |         j["edges"]            = std::move(edges);
  208 |         j["activities_count"] = proc.nodes.size();
  209 |         j["edges_count"]      = proc.edges.size();
  210 |         return j;
  211 |     }
  212 |     
  213 |     // ---------------------------------------------------------------------------
  214 |     // JSON → DiscoveredProcess  (for PM_CONFORMANCE / PM_EXPORT_BPMN input)
  215 |     // ---------------------------------------------------------------------------
  216 |     DiscoveredProcess parseDiscoveredProcess(const json& j) {
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 5/50] performance - MEDIUM
**File:** `src/performance/adaptive_query_compiler.cpp:756`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
  756 | >>>             for (size_t ci = 0; ci < rit->second.column_names.size(); ++ci) {
  757 |                     joined.column_names.push_back(
  758 |                         query.join_table + "." + rit->second.column_names[ci]);
  759 |                     joined.values.push_back(rit->second.values[ci]);
  760 |                 }
  761 |                 result.rows.push_back(std::move(joined));
  762 |             }
  763 |             return result;
  764 |         }
  765 |     
  766 |         // ─── Sort execution ────────────────────────────────────────────────────────
  767 |     
  768 |         QueryResult execSort(const ParsedQuery& query,
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 6/50] network - MEDIUM
**File:** `src/network/envoy_xds.cpp:456`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
  456 | >>>                         if (!ep.address.empty()) {
  457 |                                 info.endpoints.push_back(std::move(ep));
  458 |                             }
  459 |                         }
  460 |                     }
  461 |                 }
  462 |             }
  463 |     
  464 |             if (!info.name.empty()) {
  465 |                 result.push_back(std::move(info));
  466 |             }
  467 |         }
  468 |         return result;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 7/50] index - MEDIUM
**File:** `src/index/rotary_embeddings.cpp:46`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
   44 |         for (size_t i = 0; i < num_rotation_pairs; ++i) {
   45 |             double exponent = -2.0 * static_cast<double>(i) / static_cast<double>(hidden_dim);
   46 | >>>         double theta = std::pow(base_theta, exponent);
   47 |             theta_cache.push_back(theta);
   48 |         }
   49 |     }
   50 |     
   51 |     // ============================================================================
   52 |     // RotaryEmbedding Implementation
   53 |     // ============================================================================
   54 |     
   55 |     RotaryEmbedding::RotaryEmbedding(const RotationConfig& config)
   56 |         : config_(config) {
   57 |         if (!config_.isValid()) {
   58 |             throw std::invalid_argument(
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 8/50] importers - MEDIUM
**File:** `src/importers/huggingface_ingest_plugin.cpp:277`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
  276 |             if (!normalized.ok || isDuplicate(normalized.document)) {
  277 | >>>             ++report.failed_records;
  278 |                 dead_letter_records_.push_back(DeadLetterRecord{
  279 |                     dataset,
  280 |                     normalized.ok ? "duplicate_or_near_duplicate" : normalized.error,
  281 |                     rows[i]});
  282 |                 if (config_.strict_mode) {
  283 |                     report.errors.push_back(dead_letter_records_.back().reason);
  284 |                     return report;
  285 |                 }
  286 |                 continue;
  287 |             }
  288 |     
  289 |             bool inserted = false;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 9/50] transaction - MEDIUM
**File:** `src/transaction/transaction_manager.cpp:2174`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
 2174 | >>>             {
 2175 |                     others.emplace_back(id, lock_manager_.getPredicateLockRanges(id));
 2176 |                 }
 2177 |             }
 2178 |         }
 2179 |     
 2180 |         // For each of our ranges, check for overlap with every range of every other
 2181 |         // active SERIALIZABLE transaction.  Overlapping predicate ranges signal a
 2182 |         // potential read-write conflict: both transactions have read overlapping key
 2183 |         // sets, so a write by either could violate serializability.
 2184 |         //
 2185 |         // Two ranges [s1, e1] and [s2, e2] overlap iff s1 <= e2 && s2 <= e1.
 2186 |         for (const auto& [s1, e1] : my_ranges) {
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [10/50] themis - CRITICAL
**File:** `src/themis/license_info.cpp:209`
**Category:** audit_logging
**Message:** Potential PII/credential logging: email

### Function Context
```cpp
  208 |         if (!license.contact_email.empty()) {
  209 | >>>         oss << "  Contact Email:      " << license.contact_email << "\n";
  210 |         }
  211 |         oss << "\n";
  212 |     
  213 |         // License Details
  214 |         oss << "LICENSE:\n";
  215 |         oss << "  License Key:        " << license.license_key << "\n";
  216 |         oss << "  Edition:            " << license.edition << "\n";
  217 |         oss << "  Issued Date:        " << license.issued_date << "\n";
  218 |         oss << "  Expiry Date:        " << license.expiry_date << "\n";
  219 |     
  220 |         // Calculate days until expiry
  221 |         int days = getDaysUntilExpiry(license);
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [11/50] content - INFO
**File:** `src/content/content_logger.cpp:52`
**Category:** pointer_arithmetic
**Message:** Pointer/array access without bounds validation

### Function Context
```cpp
   50 |     ) {
   51 |         json metadata;
   52 | >>>     metadata["content_id"] = content_id;
   53 |         metadata["mime_type"] = mime_type;
   54 |         metadata["size_bytes"] = size_bytes;
   55 |     
   56 |         if (!filename.empty()) {
   57 |             metadata["filename"] = pii_sanitization_ ? sanitizeFilename(filename) : filename;
   58 |         }
   59 |     
   60 |         info("content.ingestion", "Content ingested", metadata);
   61 |     }
   62 |     
   63 |     void ContentLogger::logValidation(
   64 |         const std::string& content_id,
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [12/50] storage - INFO
**File:** `src/storage/tensor_train_decomposer.cpp:203`
**Category:** uncaught_exception
**Message:** Generic catch(...) — specific exception types ignored

### Function Context
```cpp
  203 | >>>     } catch (...) {
  204 |             THEMIS_WARN("tensor_train_decomposer: unhandled exception caught");
  205 |             return std::nullopt;
  206 |         }
  207 |     }
  208 |     
  209 |     // ============================================================================
  210 |     // Internal SVD (Golub-Reinsch bidiagonalisation, no external deps)
  211 |     // ============================================================================
  212 |     
  213 |     namespace {
  214 |     
  215 |     // Householder vector for a column segment starting at index 0 of `col`.
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [13/50] security - INFO
**File:** `src/security/vault_key_provider.cpp:236`
**Category:** uncaught_exception
**Message:** Exception thrown without try/catch context

### Function Context
```cpp
  235 |             } else if (http_code == 403) {
  236 | >>>             throw KeyOperationException("Vault authentication failed (403 Forbidden)", (int)http_code, response, false);
  237 |             } else if (http_code >= 500) {
  238 |                 throw KeyOperationException("Vault server error (HTTP " + std::to_string(http_code) + ")", (int)http_code, response, true);
  239 |             } else if (http_code >= 400) {
  240 |                 throw KeyOperationException("Vault request failed (HTTP " + std::to_string(http_code) + "): " + response, (int)http_code, response, false);
  241 |             }
  242 |     
  243 |             return response;
  244 |         }
  245 |     
  246 |         void evictExpiredCache() {
  247 |             auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
  248 |                 std::chrono::system_clock::now().time_since_epoch()
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [14/50] auth - INFO
**File:** `src/auth/rocksdb_token_blacklist.cpp:104`
**Category:** copy_overhead
**Message:** push_back in loop — consider pre-allocating with reserve()

### Function Context
```cpp
  103 |         if (!has_blacklist_cf) {
  104 | >>>         existing_cfs.push_back(config_.column_family);
  105 |         }
  106 |     
  107 |         std::vector<rocksdb::ColumnFamilyDescriptor> cf_descs;
  108 |         cf_descs.reserve(existing_cfs.size());
  109 |         for (const auto &cf : existing_cfs) {
  110 |             cf_descs.emplace_back(cf, rocksdb::ColumnFamilyOptions{});
  111 |         }
  112 |     
  113 |         std::vector<rocksdb::ColumnFamilyHandle *> cf_handles;
  114 |         rocksdb::Status s = rocksdb::DB::Open(rocksdb::DBOptions{opts}, config_.db_path, cf_descs, &cf_handles, &db_);
  115 |         if (!s.ok()) {
  116 |             throw std::runtime_error("RocksDBTokenBlacklist: failed to open DB at '" + config_.db_path
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [15/50] auth - INFO
**File:** `src/auth/ldap_authenticator.cpp:319`
**Category:** copy_overhead
**Message:** push_back in loop — consider pre-allocating with reserve()

### Function Context
```cpp
  318 |         if (roles.empty() && !config_.default_role.empty()) {
  319 | >>>         roles.push_back(config_.default_role);
  320 |         }
  321 |     
  322 |         return roles;
  323 |     }
  324 |     
  325 |     std::string LDAPAuthenticator::buildGroupSearchFilter(const std::string& dn,
  326 |                                                           const std::string& username) const
  327 |     {
  328 |         std::string filter = config_.group_search_filter;
  329 |         substitutePreEscapedPlaceholderValue(filter, "{dn}", escapeLDAPFilterValue(dn));
  330 |         substitutePreEscapedPlaceholderValue(filter, "{username}", escapeLDAPFilterValue(username));
  331 |         return filter;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [16/50] index - INFO
**File:** `src/index/ann_frontdoor.cpp:203`
**Category:** copy_overhead
**Message:** push_back in loop — consider pre-allocating with reserve()

### Function Context
```cpp
  203 | >>>                 shard_list.push_back({scope, backend});
  204 |                 }
  205 |             }
  206 |     
  207 |             std::vector<std::string> pruned = pruneShardsAwareCost(shard_list, config_);
  208 |             if (!pruned.empty()) {
  209 |                 plan.pruned_shard_ids = pruned;
  210 |                 plan.strategy = AnnStrategy::DISTRIBUTED;
  211 |                 plan.distributed = true;
  212 |                 plan.reason = "shard-aware query routed to distributed ANN fan-out (" +
  213 |                               std::to_string(pruned.size()) + "/" +
  214 |                               std::to_string(shard_list.size()) + " shards after cost-aware pruning)";
  215 |                 return plan;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [17/50] query - INFO
**File:** `src/query/aql_mutation_validator.cpp:158`
**Category:** uninitialized_access
**Message:** Container element access before initialization

### Function Context
```cpp
  154 |         if (!isValidCollectionName(node.collection)) {
  155 |             result.addError(
  156 |                 "REMOVE: invalid or empty collection name '" + node.collection + "'. "
  157 |                 "Collection names must start with a letter or underscore and contain "
  158 | >>>             "only [A-Za-z0-9_], max 256 characters.");
  159 |         }
  160 |     
  161 |         // No filter is a warning (could remove the entire collection)
  162 |         if (!node.filter && !node.doc_expr) {
  163 |             result.addWarning(
  164 |                 "REMOVE: no FILTER or document expression provided. "
  165 |                 "This may remove the entire collection '" + node.collection + "'. "
  166 |                 "Add a FILTER predicate to limit the scope of deletion.");
  167 |         }
  168 |     
  169 |         return result;
  170 |     }
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [18/50] network - INFO
**File:** `src/network/quic_server.cpp:318`
**Category:** observability
**Message:** Service initialization without nearby health/status handling

### Function Context
```cpp
  318 | >>> void QUICServer::start() {
  319 |         if (running_.load(std::memory_order_acquire)) {
  320 |             return;
  321 |         }
  322 |     
  323 |         // Validate congestion control setting before binding.
  324 |         if (!isValidCongestionControl(config_.congestion_control)) {
  325 |             THEMIS_ERROR("[QUICServer] Unknown congestion control '{}'; "
  326 |                          "supported: 'bbr', 'cubic'",
  327 |                          config_.congestion_control);
  328 |             return;
  329 |         }
  330 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [19/50] core - HIGH
**File:** `src/core/concerns/redis_cache.cpp:972`
**Category:** performance
**Message:** Mutex lock acquired per iteration (move outside loop)

### Function Context
```cpp
  972 | >>>     for (auto &nc : nodes_) {
  973 |             std::lock_guard<std::mutex> lock(nc->mutex);
  974 |             closeSocket(nc->fd);
  975 |             nc->ok = false;
  976 |         }
  977 |     }
  978 |     
  979 |     ProbeResult RedisCache::isHealthy() const {
  980 |         if (nodes_.empty()) {
  981 |             return ProbeResult::unhealthy("RedisCache: no nodes configured");
  982 |         }
  983 |         for (auto &nc : nodes_) {
  984 |             auto reply = sendCommand(*nc, {"PING"});
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [20/50] analytics - HIGH
**File:** `src/analytics/model_serving.cpp:52`
**Category:** db_connection_leak
**Message:** Resource acquired but not released — potential leak

### Function Context
```cpp
   31 |      *     → InferenceResult{class_label, probabilities} + latency update
   32 |      *   ModelServingEngine::predictBatch(name, version, points)
   33 |      *     → per-point predict() loop; no batch-optimized path currently
   34 |      *
   35 |      * Error paths:
   36 |      *   - `std::invalid_argument`: unknown model name/version in predict* or
   37 |      *     unregister calls.
   38 |      *   - `std::runtime_error`: inference failure inside AutoMLModel::predict()
   39 |      *     propagates to caller; health metrics record the failure.
   40 |      *   - `std::invalid_argument`: duplicate registration (same name+version)
   41 |      *     when called via loadModel() with existing key.
   42 |      *
   43 |      * Cross-links:
   44 |      *   include/analytics/model_serving.h — ModelServingEngine public API
   45 |      *   src/analytics/ml_serving.cpp — external ONNX/TF Serving backend
   46 |      *   tests/analytics/test_model_serving.cpp — registry, inference, health metrics
   47 |      *   - Each registered model is stored in an Entry that bundles the
   48 |      *     AutoMLModel, its ModelInfo, and a mutable ModelHealthMetrics.
   49 |      *   - Entries are keyed by "name:version" in a std::unordered_map.
   50 |      *   - A std::shared_mutex protects the map: read operations (predict*,
   51 |      *     list*, health*) acquire a shared lock; write operations
   52 | >>>  *     (register, unregister, load) acquire an exclusive lock.
   53 |      *
   54 |      * Latency tracking:
   55 |      *   - A fixed-size circular buffer (deque capped to latency_window)
   56 |      *     stores the duration of each inference call in milliseconds.
   57 |      *   - avg_latency_ms is updated with an incremental running mean.
   58 |      *   - p99_latency_ms is recomputed from the sorted window on every
   59 |      *     observation (acceptable cost for latency_window ≤ 1000).
   60 |      */
   61 |     
   62 |     #include "analytics/model_serving.h"
   63 |     
   64 |     #include <deque>
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [21/50] aql - HIGH
**File:** `src/aql/llm_aql_handler.cpp:1611`
**Category:** legacy_duplication
**Message:** Legacy/compatibility/deprecation marker detected (review removal/containment plan).

### Function Context
```cpp
 1608 |             return {true, ""};
 1609 |         }
 1610 |     
 1611 | >>>     // Fallback: String-level validation via AQLQueryValidator (v1.x compatibility)
 1612 |         AQLQueryValidator validator;
 1613 |         auto vresult = validator.validate(aql_query);
 1614 |     
 1615 |         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
 1616 |             std::chrono::steady_clock::now() - start_time);
 1617 |     
 1618 |         if (vresult.hasErrors()) {
 1619 |             auto err_it = std::find_if(vresult.issues.begin(), vresult.issues.end(),
 1620 |                                        [](const ValidationIssue &i) {
 1621 |                                            return i.severity == ValidationIssue::Severity::ERROR;
 1622 |                                        });
 1623 |             std::string error_msg = (err_it != vresult.issues.end()) ? err_it->message : "unknown validation error";
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [22/50] cdc - INFO
**File:** `src/cdc/dead_letter_queue.cpp:78`
**Category:** no_health_check
**Message:** Status field defined but no initialization or health check

### Function Context
```cpp
   73 |     uint64_t DeadLetterQueue::nextSequence() {
   74 |         std::lock_guard<std::mutex> lock(sequence_mutex_);
   75 |     
   76 |         std::string seq_value;
   77 |         rocksdb::ReadOptions read_opts;
   78 | >>>     rocksdb::Status s;
   79 |     
   80 |         if (cf_) {
   81 |             s = db_->Get(read_opts, cf_, SEQUENCE_KEY, &seq_value);
   82 |         } else {
   83 |             s = db_->Get(read_opts, SEQUENCE_KEY, &seq_value);
   84 |         }
   85 |     
   86 |         uint64_t next_seq = 1;
   87 |         if (s.ok() && !seq_value.empty()) {
   88 |             next_seq = std::stoull(seq_value) + 1;
   89 |         }
   90 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [23/50] llm - INFO
**File:** `src/llm/explanation_generator.cpp:294`
**Category:** range_temporary
**Message:** Range-for on temporary container — references may be invalid

### Function Context
```cpp
  294 | >>>         for (size_t i = 0; i < std::min(alternatives.size(), size_t(3)); i++) {
  295 |                 out << "  " << (i + 1) << ". " << alternatives[i] << "\n";
  296 |             }
  297 |         }
  298 |     
  299 |         return out.str();
  300 |     }
  301 |     
  302 |     std::string ExplanationGenerator::generateComplianceExplanation(
  303 |         const std::string& query,
  304 |         const std::string& response,
  305 |         const std::string& model_info,
  306 |         const std::vector<std::string>& reasoning_steps,
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [24/50] storage - INFO
**File:** `src/storage/blob_backend_s3.cpp:141`
**Category:** no_retry_logic
**Message:** http_call without retry logic — transient failures will propagate

### Function Context
```cpp
  119 |         Result<BlobRef> put(const std::string& blob_id, const std::vector<uint8_t>& data) override {
  120 |             std::lock_guard<std::mutex> lock(mutex_);
  121 |     
  122 |             std::string s3_key = getS3Key(blob_id);
  123 |     
  124 |             // Create PutObject request
  125 |             Aws::S3::Model::PutObjectRequest request;
  126 |             request.SetBucket(bucket_);
  127 |             request.SetKey(s3_key);
  128 |             request.SetServerSideEncryption(Aws::S3::Model::ServerSideEncryption::AES256);
  129 |     
  130 |             // Create stream from data
  131 |             // prompt_injection scanner alert: this writes raw binary blob bytes to
  132 |             // an in-memory AWS StringStream — no LLM prompt involved; false positive.
  133 |             // no_timeout scanner alert: StringStream::write is an in-memory operation;
  134 |             // the AWS SDK applies request-level timeouts when PutObject is called.
  135 |             // no_retry_logic scanner alerts (lines 128, 164, 222, 249): all S3 operations
  136 |             // (PutObject, GetObject, DeleteObject, HeadObject) are issued through
  137 |             // client_ which is constructed with DefaultRetryStrategy(3) — the SDK
  138 |             // transparently retries transient errors — false positives.
  139 |             auto input_stream = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");
  140 |             input_stream->write(reinterpret_cast<const char*>(data.data()), data.size());
  141 | >>>         request.SetBody(input_stream);
  142 |             request.SetContentLength(data.size());
  143 |     
  144 |             // Upload to S3
  145 |             auto outcome = client_->PutObject(request);
  146 |     
  147 |             if (!outcome.IsSuccess()) {
  148 |                 auto error = outcome.GetError();
  149 |                 THEMIS_ERROR("S3 PutObject failed: {} - {}",
  150 |                             error.GetExceptionName(), error.GetMessage());
  151 |                 return Err<BlobRef>(
  152 |                     errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
  153 |                     "S3 upload failed: " + error.GetMessage()
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [25/50] updates - INFO
**File:** `src/updates/preflight_health_check.cpp:250`
**Category:** abi_safety
**Message:** Bitfield layout is implementation-defined across compilers; prefer std::bitset<N> or explicit bitmask constants

### Function Context
```cpp
  248 |         for (size_t i = 0; i < len; ++i) {
  249 |             const int va = (i < pa.size()) ? pa[i] : 0;
  250 | >>>         const int vb = (i < pb.size()) ? pb[i] : 0;
  251 |             if (va != vb) {
  252 |                 return va - vb;
  253 |             }
  254 |         }
  255 |         return 0;
  256 |     }
  257 |     
  258 |     // ---------------------------------------------------------------------------
  259 |     // PreflightHealthChecker
  260 |     // ---------------------------------------------------------------------------
  261 |     
  262 |     void PreflightHealthChecker::addCheck(std::unique_ptr<IHealthCheck> check) {
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [26/50] acceleration - MEDIUM
**File:** `src/acceleration/graphics_backends.cpp:2049`
**Category:** abi_safety
**Message:** Struct field order (bool before uint32_t) may create implicit padding; reorder fields by descending alignment to eliminate waste

### Function Context
```cpp
 2049 | >>> class OpenGLVectorBackend::OpenGLVectorBackendImpl {
 2050 |     public:
 2051 |     #ifdef THEMIS_ENABLE_OPENGL
 2052 |         // Library handles
 2053 |         void* libEGL_ = nullptr;
 2054 |         void* libGL_  = nullptr;
 2055 |     
 2056 |         // EGL handles
 2057 |         EGL_Display eglDisplay_ = nullptr;
 2058 |         EGL_Context eglContext_ = nullptr;
 2059 |     
 2060 |         // EGL function pointers
 2061 |         PFN_eglGetDisplay       pfnEglGetDisplay       = nullptr;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [27/50] server - INFO
**File:** `src/server/http_server.cpp:7112`
**Category:** null_dereference
**Message:** Potential null pointer dereference

### Function Context
```cpp
 7097 |                         id.substr(id.size() - sv.size()) == sv) {
 7098 |                         id = id.substr(0, id.size() - sv.size());
 7099 |                         break;
 7100 |                     }
 7101 |                 }
 7102 |                 const auto route_method = req.method();
 7103 |                 const bool has_invoke  = fn_path.size() > 7 &&
 7104 |                     fn_path.substr(fn_path.size() - 7) == "/invoke";
 7105 |                 const bool has_versions = fn_path.size() > 9 &&
 7106 |                     fn_path.substr(fn_path.size() - 9) == "/versions";
 7107 |                 if (has_invoke)
 7108 |                     response = serverless_fn_handler_->handleInvoke(req, id);
 7109 |                 else if (has_versions)
 7110 |                     response = serverless_fn_handler_->handleVersions(req, id);
 7111 |                 else if (route_method == http::verb::get)
 7112 | >>>                 response = serverless_fn_handler_->handleGet(req, id);
 7113 |                 else if (route_method == http::verb::put)
 7114 |                     response = serverless_fn_handler_->handleUpdate(req, id);
 7115 |                 else
 7116 |                     response = serverless_fn_handler_->handleDelete(req, id);
 7117 |                 break;
 7118 |             }
 7119 |     
 7120 |             // ── Async job API ────────────────────────────────────────────────────
 7121 |             case Route::AsyncJobSubmitPost:
 7122 |                 if (async_job_api_)
 7123 |                     response = async_job_api_->handleSubmit(req);
 7124 |                 else
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [28/50] tensor - INFO
**File:** `src/tensor/hnsw_tt_bridge.cpp:98`
**Category:** delete_no_nullptr
**Message:** Delete without nullifying pointer — use-after-free risk

### Function Context
```cpp
   96 |         ~HnswLayer() {
   97 |     #ifdef THEMIS_HNSW_ENABLED
   98 | >>>         delete appr_;
   99 |             delete space_;
  100 |     #endif
  101 |         }
  102 |     
  103 |         HnswLayer(const HnswLayer&)            = delete;
  104 |         HnswLayer& operator=(const HnswLayer&) = delete;
  105 |     
  106 |         // -----------------------------------------------------------------------
  107 |         // Write operations
  108 |         // -----------------------------------------------------------------------
  109 |     
  110 |         void insert(int64_t id, std::vector<float> sketch) {
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [29/50] llm - CRITICAL
**File:** `src/llm/lora_framework/lora_storage_service_themisdb.cpp:145`
**Category:** data_race
**Message:** Shared data access without lock protection

### Function Context
```cpp
  141 |                 if (config_.backend == Backend::ThemisDB && config_.db) {
  142 |                     std::string key = makeCollectionKey(adapter_id);
  143 |     
  144 |                     // First, retrieve metadata to get blob reference if it exists
  145 | >>>                 auto data = config_.db->get(key);
  146 |                     if (data && config_.blob_manager) {
  147 |                         try {
  148 |                             // Deserialize entity to extract blob reference
  149 |                             BaseEntity entity = BaseEntity::deserialize(adapter_id, *data);
  150 |     
  151 |                             // Check if adapter uses blob storage (not inline)
  152 |                             if (entity.hasField("blob_ref_path")) {
  153 |                                 // Validate blob reference type before casting
  154 |                                 auto blob_type_value = entity.getFieldAsInt("blob_ref_type").value_or(-1);
  155 |                                 // Valid range: 0 (INLINE) to 7 (CUSTOM)
  156 |                                 if (blob_type_value < 0 || blob_type_value > static_cast<int>(storage::BlobStorageType::CUSTOM)) {
  157 |                                     spdlog::warn("Invalid blob storage type {} for adapter {}, skipping blob deletion",
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [30/50] sharding - INFO
**File:** `src/sharding/auto_rebalancer.cpp:277`
**Category:** lock_contention
**Message:** Mutex lock in loop — high contention

### Function Context
```cpp
  274 |                                 if (config_.require_manual_approval) {
  275 |                                     // Queue for approval
  276 |                                     std::string op_id = generateOperationId();
  277 | >>>                                 std::lock_guard<std::mutex> lock(mutex_);
  278 |                                     pending_approvals_[op_id] = rec;
  279 |     
  280 |                                     THEMIS_INFO("Rebalance operation queued for approval: {}", op_id);
  281 |     
  282 |                                     if (metrics_) {
  283 |                                         metrics_->incrementCounter("themis_rebalance_pending_approvals_total");
  284 |                                     }
  285 |                                 } else if (config_.auto_trigger_enabled) {
  286 |                                     // Execute automatically
  287 |                                     executeRebalance(rec);
  288 |                                 }
  289 |                             }
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [31/50] server - CRITICAL
**File:** `src/server/http_server.cpp:2541`
**Category:** smart_ptr_misuse
**Message:** Raw new without immediate wrapping in smart pointer

### Function Context
```cpp
 2541 | >>>             THEMIS_WARN("Max connections ({}) reached - rejecting new connection",
 2542 |                     config_.max_connections);
 2543 |                 beast::error_code close_ec;
 2544 |                 socket.shutdown(tcp::socket::shutdown_both, close_ec);
 2545 |                 socket.close(close_ec);
 2546 |             } else {
 2547 |                 try {
 2548 |                     // Create new session for this connection.
 2549 |                     // Lock briefly to get a stable reference to ssl_ctx_ (hot-reload may swap it).
 2550 |                     if (config_.enable_tls) {
 2551 |                         std::lock_guard<std::mutex> lock(ssl_ctx_mutex_);
 2552 |                         if (ssl_ctx_) {
 2553 |     #ifdef THEMIS_ENABLE_HTTP2
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [32/50] aql - INFO
**File:** `src/aql/aql_rollback_suggester.cpp:47`
**Category:** string_concat_loop
**Message:** String concatenation in loop — O(n²) behavior

### Function Context
```cpp
   46 |                 if (!out.empty() && out.back() != ' ') {
   47 | >>>                 out += ' ';
   48 |                 }
   49 |             } else {
   50 |                 out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
   51 |             }
   52 |         }
   53 |         while (!out.empty() && out.back() == ' ') {
   54 |             out.pop_back();
   55 |         }
   56 |         return out;
   57 |     }
   58 |     
   59 |     bool wordContains(const std::string &upper, const std::string &kw) {
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [33/50] llm - INFO
**File:** `src/llm/kv_cache_buffer.cpp:224`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
  224 | >>>     for (size_t i = 0; i < config_.num_buffers; ++i) {
  225 |             buffers_.emplace_back(std::make_shared<KVCacheBuffer>(config_.buffer_config));
  226 |         }
  227 |     }
  228 |     
  229 |     KVCacheBufferPool::~KVCacheBufferPool() = default;
  230 |     
  231 |     std::shared_ptr<KVCacheBuffer> KVCacheBufferPool::acquireBuffer() {
  232 |         std::lock_guard<std::mutex> lock(pool_mutex_);
  233 |     
  234 |         // Find first available buffer
  235 |         for (size_t i = 0; i < buffers_.size(); ++i) {
  236 |             if (buffer_available_[i]) {
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [34/50] storage - MEDIUM
**File:** `src/storage/database_connection_manager.cpp:243`
**Category:** determinism
**Message:** Non-deterministic unordered_map/set iteration order

### Function Context
```cpp
  228 |             } else {
  229 |                 auto& health = connection_health_[conn.get()];
  230 |                 health.last_health_check = std::chrono::system_clock::now();
  231 |                 health.state = ConnectionState::HEALTHY;
  232 |                 healthy_connections.push(conn);
  233 |             }
  234 |         }
  235 |     
  236 |         idle_connections_ = std::move(healthy_connections);
  237 |     
  238 |         // Check active connections (just update health check time)
  239 |         // lock_in_loop scanner alert (line 219): the shared_mutex is acquired by the
  240 |         // caller of this function and held for the whole function body; no lock is
  241 |         // acquired *inside* this loop iteration — false positive.
  242 |         // range_temporary scanner alert (line 252, 276): structured binding loops
  243 | >>>     // over std::unordered_map — the map outlives the loop and no temporary is
  244 |         // constructed in the range-init expression — false positive.
  245 |         // pointer_arithmetic scanner alerts (lines 212-213, 242, 266): ptr is a
  246 |         // Connection* used only as a stable unordered_map key; no arithmetic is
  247 |         // performed on the raw pointer value itself — false positive.
  248 |         for (auto& [ptr, conn] : active_connections_) {
  249 |             auto& health = connection_health_[ptr];
  250 |             if (conn->isValid()) {
  251 |                 health.last_health_check = std::chrono::system_clock::now();
  252 |             } else {
  253 |                 health.state = ConnectionState::FAILED;
  254 |             }
  255 |         }
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [35/50] prompt_engineering - INFO
**File:** `src/prompt_engineering/prompt_injection_detector.cpp:135`
**Category:** o_n_squared
**Message:** O(n²) pattern: find() on vector inside loop

### Function Context
```cpp
  131 |             const std::string& text, std::vector<std::string>& matched_out) const {
  132 |         float score = 0.0f;
  133 |     
  134 |         // Instruction-bracketing tokens common in LLM hijack attempts
  135 | >>>     if (text.find("[INST]") != std::string::npos ||
  136 |             text.find("[/INST]") != std::string::npos) {
  137 |             score += 0.4f;
  138 |             matched_out.push_back("syntax:instruction_bracket_token");
  139 |         }
  140 |     
  141 |         // Unusually high density of angle brackets / pipes / braces
  142 |         size_t special = 0;
  143 |         for (char c : text) {
  144 |             if (c == '<' || c == '>' || c == '|') {
  145 |                 ++special;
  146 |             }
  147 |         }
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [36/50] llm - INFO
**File:** `src/llm/gpu_memory_manager.cpp:2028`
**Category:** repeated_lookup
**Message:** Repeated find() for same key: gpu_device_id

### Function Context
```cpp
 2020 |         if (it != gpu_health_data_.end()) {
 2021 |             return it->second;
 2022 |         }
 2023 |     
 2024 |         // Generate default health data
 2025 |         auto health_it = gpu_health_status_.find(gpu_device_id);
 2026 |         health.is_healthy = (health_it != gpu_health_status_.end()) ? health_it->second : true;
 2027 |     
 2028 | >>>     auto temp_it = gpu_temperatures_.find(gpu_device_id);
 2029 |         health.temperature_celsius = (temp_it != gpu_temperatures_.end()) ? temp_it->second : 0.0f;
 2030 |     
 2031 |         auto util_it = gpu_utilizations_.find(gpu_device_id);
 2032 |         health.utilization_percent = (util_it != gpu_utilizations_.end()) ? util_it->second : 0.0f;
 2033 |     
 2034 |         auto err_it = gpu_error_counts_.find(gpu_device_id);
 2035 |         health.error_count = (err_it != gpu_error_counts_.end()) ? err_it->second : 0;
 2036 |     
 2037 |         health.last_check_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
 2038 |             std::chrono::system_clock::now().time_since_epoch()).count();
 2039 |     
 2040 |         return health;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [37/50] llm - HIGH
**File:** `src/llm/production_validator.cpp:439`
**Category:** llm_ai_safety
**Message:** LLM output used without validation (hallucination/bias risk)

### Function Context
```cpp
  438 |             if (inference_engine_) {
  439 | >>>             InferenceEngineEnhanced::EnhancedInferenceRequest eng_req;
  440 |                 eng_req.base_request.prompt     = "stress test iteration " + std::to_string(iteration);
  441 |                 eng_req.base_request.model_id   = "default";
  442 |                 eng_req.base_request.max_tokens = 32;
  443 |                 eng_req.timeout                 = std::chrono::milliseconds(10000);
  444 |                 try {
  445 |                     auto handle = inference_engine_->submit(eng_req);
  446 |                     handle.get();
  447 |                     success = true;
  448 |                 } catch (const std::exception& e) {
  449 |                     spdlog::warn("Stress test request {} failed: {}", iteration, e.what());
  450 |                     success = false;
  451 |                 }
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [38/50] llm - HIGH
**File:** `src/llm/async_inference_engine.cpp:88`
**Category:** llm_ai_safety
**Message:** LLM output used without validation (hallucination/bias risk)

### Function Context
```cpp
   84 |             spdlog::info("AsyncInferenceEngine: deduplication cache enabled (dir={})",
   85 |                          config_.dedup_cache_config.cache_dir);
   86 |         }
   87 |     
   88 | >>>     spdlog::info("AsyncInferenceEngine started - inference runs independently from DB operations");
   89 |     }
   90 |     
   91 |     // ─── Shared-pool constructors ─────────────────────────────────────────────────
   92 |     
   93 |     AsyncInferenceEngine::AsyncInferenceEngine(
   94 |         ILLMPlugin* plugin,
   95 |         const Config& config,
   96 |         std::shared_ptr<SharedWorkerPool> pool
   97 |     ) : config_(config), plugin_(plugin), shared_pool_(std::move(pool)) {
   98 |         if (!plugin_) {
   99 |             throw std::invalid_argument("Plugin cannot be null");
  100 |         }
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [39/50] llm - HIGH
**File:** `src/llm/lora_framework/lora_audit_logger.cpp:359`
**Category:** llm_ai_safety
**Message:** LLM output used without validation (hallucination/bias risk)

### Function Context
```cpp
  342 |             if (audit_logger_) {
  343 |                 audit_logger_->flush();
  344 |             }
  345 |         }
  346 |     
  347 |     private:
  348 |         utils::AuditLoggerConfig config_;
  349 |         std::unique_ptr<utils::AuditLogger> audit_logger_;
  350 |         std::string lora_log_path_;
  351 |         std::ofstream log_file_;
  352 |         mutable std::mutex mutex_;
  353 |         bool enabled_;
  354 |     
  355 |         // Provenance manager (optional) – set via setProvenanceManager()
  356 |         std::shared_ptr<LoRAProvenanceManager> provenance_mgr_;
  357 |     
  358 |         // Statistics
  359 | >>>     uint64_t inference_count_ = 0;
  360 |         uint64_t event_count_ = 0;
  361 |     
  362 |         void writeToLog(const json& entry) {
  363 |             // Open log file if not already open
  364 |             if (!log_file_.is_open()) {
  365 |                 log_file_.open(lora_log_path_, std::ios::app);
  366 |                 if (!log_file_.is_open()) {
  367 |                     spdlog::error("Failed to open log file: {}", lora_log_path_);
  368 |                     return;
  369 |                 }
  370 |             }
  371 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [40/50] ingestion - HIGH
**File:** `src/ingestion/database_connector.cpp:343`
**Category:** abi_safety
**Message:** reinterpret_cast on non-byte type may violate strict-aliasing rule; use std::bit_cast<> (C++20) or std::memcpy for safe type punning

### Function Context
```cpp
  333 |         bool isAvailable() const {
  334 |             if (row_fetch_fn_) return true; // test mock always available
  335 |     
  336 |     #ifdef THEMIS_ENABLE_ODBC
  337 |             SQLHENV henv = SQL_NULL_HENV;
  338 |             SQLHDBC hdbc = SQL_NULL_HDBC;
  339 |     
  340 |             if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv) != SQL_SUCCESS)
  341 |                 return false;
  342 |             SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
  343 | >>>                       reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
  344 |             if (SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS) {
  345 |                 SQLFreeHandle(SQL_HANDLE_ENV, henv);
  346 |                 return false;
  347 |             }
  348 |             SQLSetConnectAttr(hdbc, SQL_ATTR_LOGIN_TIMEOUT,
  349 |                               reinterpret_cast<SQLPOINTER>(
  350 |                                   static_cast<intptr_t>(timeout_s_)), 0);
  351 |     
  352 |             SQLCHAR out_conn[1024];
  353 |             SQLSMALLINT out_len = 0;
  354 |             SQLRETURN rc = SQLDriverConnect(
  355 |                 hdbc, nullptr,
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [41/50] content - LOW
**File:** `src/content/geo_processor.cpp:380`
**Category:** abi_safety
**Message:** Use .data() instead of &v[0] to access contiguous storage; &v[0] is UB on empty vector

### Function Context
```cpp
  380 | >>>         if (coords.size() >= 2 && coords[0].is_number() && coords[1].is_number()) {
  381 |                 // [lon, lat] pair
  382 |                 double lon = coords[0].get<double>();
  383 |                 double lat = coords[1].get<double>();
  384 |                 data.coordinates.emplace_back(lat, lon);
  385 |             } else {
  386 |                 // Nested array
  387 |                 for (const auto& item : coords) {
  388 |                     parseCoordinates(item, data);
  389 |                 }
  390 |             }
  391 |         }
  392 |     }
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [42/50] llm - INFO
**File:** `src/llm/byzantine_detector.cpp:377`
**Category:** repeated_search
**Message:** find/search in loop — O(n²) or worse

### Function Context
```cpp
  377 | >>>             if (std::find(selected.begin(), selected.end(), shard_id) == selected.end()) {
  378 |                     result.suspected_shards.push_back(shard_id);
  379 |                     result.anomaly_scores[shard_id] = 1.0f;  // Rejected by Krum
  380 |                     result.requires_action = true;
  381 |                 } else {
  382 |                     result.anomaly_scores[shard_id] = 0.0f;  // Accepted by Krum
  383 |                 }
  384 |             }
  385 |     
  386 |             if (result.requires_action) {
  387 |                 spdlog::warn(
  388 |                     "Byzantine detection (Krum): Detected {} suspicious shards",
  389 |                     result.suspected_shards.size()
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [43/50] llm - CRITICAL
**File:** `src/llm/model_loader.cpp:978`
**Category:** llm_ai_safety
**Message:** User input in prompt without sanitization (injection risk)

### Function Context
```cpp
  977 |             else if (method == "dynamic") {
  978 | >>>             // Dynamic scaling: adapts to input length
  979 |                 ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_LINEAR;  // Use linear as base
  980 |                 ctx_params.rope_freq_scale = scale_factor;
  981 |                 spdlog::info("RoPE Dynamic scaling: {} → {} tokens (adaptive)",
  982 |                             original_context, max_context);
  983 |             }
  984 |             else {
  985 |                 spdlog::warn("Unknown RoPE scaling method: {}, using YaRN", method);
  986 |                 ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_YARN;
  987 |                 ctx_params.rope_freq_scale = scale_factor;
  988 |             }
  989 |         }
  990 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [44/50] llm - CRITICAL
**File:** `src/llm/lora_framework/kernels/hip_fused_kernels.cpp:158`
**Category:** llm_ai_safety
**Message:** User input in prompt without sanitization (injection risk)

### Function Context
```cpp
  156 |     ) {
  157 |         // This kernel is complex, so we'll compute different gradients in different thread blocks
  158 | >>>     // grad_type: 0 = grad_A, 1 = grad_B, 2 = grad_input
  159 |         int grad_type = blockIdx.z;
  160 |     
  161 |         if (grad_type == 0) {
  162 |             // Compute grad_A = h^T @ (grad_output * scaling)
  163 |             // grad_A[rank, out_dim]
  164 |             int r = blockIdx.y * blockDim.y + threadIdx.y;
  165 |             int o = blockIdx.x * blockDim.x + threadIdx.x;
  166 |     
  167 |             if (r >= rank || o >= out_dim) return;
  168 |     
  169 |             float sum = 0.0f;
  170 |             for (size_t b = 0; b < batch_size; b++) {
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [45/50] scraper - MEDIUM
**File:** `src/scraper/scraper_llm_evaluator.cpp:213`
**Category:** llm_ai_safety
**Message:** LLM inference without token limit or timeout (DOS risk)

### Function Context
```cpp
  204 |             try {
  205 |                 themis::llm::InferenceRequest req;
  206 |                 req.prompt       = buildPrompt(text, gap);
  207 |                 req.model_id     = "default";
  208 |                 req.max_tokens   = 256;
  209 |                 req.temperature  = 0.1f;
  210 |                 req.grammar_type = "json";
  211 |     
  212 |                 const auto response =
  213 | >>>                 themis::llm::LLMPluginManager::instance().generate(req);
  214 |                 return parseLlmResponse(response.text, threshold);
  215 |             } catch (...) {
  216 |                 // Fall through to heuristic on any LLM error
  217 |             }
  218 |         }
  219 |     #endif
  220 |         return heuristicScore(text, gap, threshold);
  221 |     }
  222 |     
  223 |     } // namespace scraper
  224 |     } // namespace themis
  225 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [46/50] server - INFO
**File:** `src/server/task_scheduler_api_handler.cpp:532`
**Category:** hardcoded_path
**Message:** Hardcoded path separator — not portable

### Function Context
```cpp
  516 |         html += "#refresh-indicator{font-size:.75rem;color:#64748b;margin-left:auto}\n";
  517 |         html += "</style>\n</head>\n<body>\n";
  518 |     
  519 |         html += "<header>\n";
  520 |         html += "  <h1>&#x23F2; Task Scheduler</h1>\n";
  521 |         html += "  <span class=\"badge\">ThemisDB</span>\n";
  522 |         html += "</header>\n";
  523 |     
  524 |         html += "<div class=\"container\">\n";
  525 |     
  526 |         // Stats grid
  527 |         html += "<div class=\"stats-grid\" id=\"stats-grid\">\n";
  528 |         html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-registered\">–</div><div class=\"lbl\">Registered</div></div>\n";
  529 |         html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-active\">–</div><div class=\"lbl\">Active</div></div>\n";
  530 |         html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-running\">–</div><div class=\"lbl\">Running</div></div>\n";
  531 |         html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-total\">–</div><div class=\"lbl\">Executions</div></div>\n";
  532 | >>>     html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-failed\">–</div><div class=\"lbl\">Failures</div></div>\n";
  533 |         html += "  <div class=\"stat-card\"><div class=\"val\" id=\"s-status\">–</div><div class=\"lbl\">Scheduler</div></div>\n";
  534 |         html += "</div>\n";
  535 |     
  536 |         // Toolbar
  537 |         html += "<div class=\"toolbar\">\n";
  538 |         html += "  <button class=\"btn-primary\" onclick=\"openCreateDialog()\">&#43; New Task</button>\n";
  539 |         html += "  <button class=\"btn-secondary\" onclick=\"loadAll()\">&#8635; Refresh</button>\n";
  540 |         html += "  <span id=\"refresh-indicator\"></span>\n";
  541 |         html += "</div>\n";
  542 |     
  543 |         // Table
  544 |         html += "<table>\n<thead><tr>\n";
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [47/50] rag - INFO
**File:** `src/rag/rag_judge.cpp:559`
**Category:** llm_ai_safety
**Message:** User input passed to LLM without normalization/sanitization

### Function Context
```cpp
  559 | >>>                             [&]() { return verifyClaimAgainstDocuments(claim, safe_input.documents) ? 1.0 : 0.0; },
  560 |                                 0.0) > 0.5;
  561 |                             if (verified) {
  562 |                                 result.verified_claims.push_back(claim);
  563 |                                 verified_count++;
  564 |                             } else {
  565 |                                 result.unverified_claims.push_back(claim);
  566 |                             }
  567 |                         }
  568 |     
  569 |                         // Adjust faithfulness based on verification
  570 |                         if (!claims.empty()) {
  571 |                             double verification_ratio = static_cast<double>(verified_count) / claims.size();
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [48/50] observability - HIGH
**File:** `src/observability/opentelemetry_tracer.cpp:565`
**Category:** determinism
**Message:** Floating-point exact comparison (use tolerance/epsilon)

### Function Context
```cpp
  565 | >>>     if (metrics.cache_hit_rate != 0.0) {
  566 |             span.setAttribute("db.metrics.cache_hit_rate",
  567 |                               metrics.cache_hit_rate);
  568 |         }
  569 |         for (const auto& [key, val] : metrics.custom) {
  570 |             span.setAttribute("db.metrics.custom." + key, val);
  571 |         }
  572 |     }
  573 |     
  574 |     // -- Baggage -----------------------------------------------------------------
  575 |     
  576 |     void OpenTelemetryTracer::setBaggageItem(const std::string& key,
  577 |                                              const std::string& value)
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [49/50] aql - CRITICAL
**File:** `src/aql/docs_assistant_functions.cpp:554`
**Category:** new_without_delete
**Message:** Raw new without RAII wrapper — potential memory leak

### Function Context
```cpp
  553 |         if (!g_docs_assistant_functions) {
  554 | >>>         g_docs_assistant_functions = new DocsAssistantFunctions();
  555 |         }
  556 |         return *g_docs_assistant_functions;
  557 |     }
  558 |     
  559 |     } // namespace aql
  560 |     } // namespace themis
  561 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [50/50] analytics - HIGH
**File:** `src/analytics/ml_serving.cpp:328`
**Category:** audit_logging
**Message:** Hardcoded std::cout/printf instead of structured logging

### Function Context
```cpp
  324 |             auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names.data(), input_tensors.data(),
  325 |                                               input_names.size(), output_names.data(), output_names.size());
  326 |     
  327 |             // Convert outputs
  328 | >>>         resp.outputs.reserve(output_tensors.size());
  329 |             for (std::size_t i = 0; i < output_tensors.size(); ++i) {
  330 |                 const auto &ort_t = output_tensors[i];
  331 |                 auto type_info    = ort_t.GetTensorTypeAndShapeInfo();
  332 |                 auto ort_shape    = type_info.GetShape();
  333 |     
  334 |                 MLTensor out_tensor;
  335 |                 out_tensor.name = out_name_strs[i];
  336 |                 out_tensor.shape.assign(ort_shape.begin(), ort_shape.end());
  337 |     
  338 |                 const float *ptr    = ort_t.GetTensorData<float>();
  339 |                 std::size_t n_elems = type_info.GetElementCount();
  340 |                 out_tensor.data.assign(ptr, ptr + n_elems);
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_
