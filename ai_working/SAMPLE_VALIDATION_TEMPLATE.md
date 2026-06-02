
# REPRESENTATIVE GAP VALIDATION SAMPLE
Generated: 2026-06-02
Total Sample Size: 50

For each gap, assess as:
- **TP** = True Positive (real issue to fix)
- **FP** = False Positive (code is correct)
- **?** = Uncertain

## [ 1/50] server - MEDIUM
**File:** `src\server\voice_api_handler.cpp:308`
**Category:** no_health_check
**Message:** Status field defined but no initialization or health check

### Function Context
```cpp
  306 |                 if (!isValidVoicePathIdentifier(session_id)) {
  307 |                     return createErrorResponse(
  308 | >>>                     http::status::bad_request, "Bad Request", "Invalid session ID");
  309 |                 }
  310 |     
  311 |                 if (action == "context" && method == http::verb::post) {
  312 |                     return handleUpdateSessionContext(req, session_id);
  313 |                 }
  314 |     
  315 |                 return createErrorResponse(
  316 |                     http::status::bad_request, "Bad Request", "Invalid session path");
  317 |             }
  318 |     
  319 |             if (!isValidVoicePathIdentifier(session_id)) {
  320 |                 return createErrorResponse(
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 2/50] query - CRITICAL
**File:** `src\query\query_engine.cpp:4558`
**Category:** no_timeout
**Message:** semaphore_wait without timeout — can block indefinitely

### Function Context
```cpp
 4547 |     					bool ok = true; for (auto& ef : q.extra_filters) { if (!evaluateCondition(ef, ctx)) { ok=false; break; } }
 4548 |     					if (!ok) continue;
 4549 |     				}
 4550 |     				std::vector<float> vec = entity[q.vector_field].get<std::vector<float>>();
 4551 |     				if (vec.size() != q.query_vector.size()) continue;
 4552 |     				float d = simd::l2_distance(vec.data(), q.query_vector.data(), vec.size());
 4553 |     				buf.emplace_back(pk, d);
 4554 |     			}
 4555 |     			buckets[bi] = std::move(buf);
 4556 |     		});
 4557 |     	}
 4558 | >>> 	tg2.wait();
 4559 |     	for (auto& b : buckets) {
 4560 |     		vectorResults.insert(vectorResults.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));
 4561 |     	}
 4562 |     
 4563 |     	// Sort by distance and take top-k
 4564 |     	tbb::parallel_sort(vectorResults.begin(), vectorResults.end(),
 4565 |     	          [](const auto& a, const auto& b) {
 4566 |     			  if (a.second == b.second) {
 4567 |     				  return a.first < b.first;
 4568 |     			  }
 4569 |     			  return a.second < b.second;
 4570 |     		  });
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 3/50] utils - HIGH
**File:** `src\utils\memory\pool_allocator.cpp:810`
**Category:** db_connection_leak
**Message:** Resource acquired but not released — potential leak

### Function Context
```cpp
  810 | >>> Result<void*> PoolAllocator::allocate(size_t size, AllocationHint hint) {
  811 |         IAllocator* allocator = impl_->selectAllocator(size, hint);
  812 |         auto result = allocator->allocate(size, hint);
  813 |     
  814 |         if (result) {
  815 |             void* ptr = *result;
  816 |             std::lock_guard<std::mutex> lock(impl_->ownership_mutex);
  817 |             impl_->ownership[reinterpret_cast<uintptr_t>(ptr)] = allocator;
  818 |         }
  819 |     
  820 |         return result;
  821 |     }
  822 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 4/50] network - HIGH
**File:** `src\network\envoy_xds.cpp:380`
**Category:** range_temporary
**Message:** Range-for on temporary container — references may be invalid

### Function Context
```cpp
  380 | >>>     for (const auto& item : splitJsonArray(body)) {
  381 |             ListenerInfo info;
  382 |             info.name     = extractString(item, "name");
  383 |     
  384 |             // Address is nested: address.socket_address.address / port_value
  385 |             const std::string addr_obj = extractRawValue(item, "address");
  386 |             if (!addr_obj.empty()) {
  387 |                 const std::string sa = extractRawValue(addr_obj, "socket_address");
  388 |                 if (!sa.empty()) {
  389 |                     info.address  = extractString(sa, "address");
  390 |                     info.port     = parsePort(extractRawValue(sa, "port_value"));
  391 |                     info.protocol = extractString(sa, "protocol");
  392 |                     if (info.protocol.empty()) info.protocol = "TCP";
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 5/50] index - HIGH
**File:** `src\index\vector_auto_buffer.cpp:384`
**Category:** lock_contention
**Message:** Mutex lock in loop — high contention

### Function Context
```cpp
  383 |         while (running_.load()) {
  384 | >>>         std::unique_lock<std::mutex> lock(flush_mutex_);
  385 |     
  386 |             // Wait for flush interval or notification
  387 |             flush_cv_.wait_for(lock, config_.flush_interval, [this] {
  388 |                 return !running_.load() || shouldFlushGlobal();
  389 |             });
  390 |     
  391 |             if (!running_.load()) {
  392 |                 break;
  393 |             }
  394 |     
  395 |             // Check if we need to flush
  396 |             if (shouldFlushGlobal()) {
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 6/50] server - HIGH
**File:** `src\server\branch_api_handler.cpp:229`
**Category:** delete_no_nullptr
**Message:** Delete without nullifying pointer — use-after-free risk

### Function Context
```cpp
  228 |         if (!success) {
  229 | >>>         sendError(res, 400, "Failed to delete branch. Branch may be active or not fully merged.");
  230 |             return;
  231 |         }
  232 |     
  233 |         json result = {
  234 |             {"success", true},
  235 |             {"message", "Branch deleted: " + branch_name}
  236 |         };
  237 |         sendJson(res, result);
  238 |     }
  239 |     
  240 |     void BranchApiHandler::handleGetStats(const httplib::Request& /*req*/, httplib::Response& res) {
  241 |         auto stats = branch_manager_.getStats();
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 7/50] security - MEDIUM
**File:** `src\security\vault_signing_provider.cpp:44`
**Category:** copy_overhead
**Message:** push_back in loop — consider pre-allocating with reserve()

### Function Context
```cpp
   38 |             while (valb >= 0) {
   39 |                 ret.push_back(b64_chars[(val >> valb) & 0x3F]);
   40 |                 valb -= 6;
   41 |             }
   42 |         }
   43 |         if (valb > -6) ret.push_back(b64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
   44 | >>>     while (ret.size() % 4) ret.push_back('=');
   45 |         return ret;
   46 |     }
   47 |     
   48 |     static std::vector<uint8_t> vaultBase64Decode(const std::string& encoded) {
   49 |         std::vector<int> T(256, -1);
   50 |         for (int i = 0; i < 64; i++) T[(unsigned char)b64_chars[i]] = i;
   51 |     
   52 |         std::vector<uint8_t> out;
   53 |         int val = 0, valb = -8;
   54 |         for (unsigned char c : encoded) {
   55 |             if (T[c] == -1) break;
   56 |             val = (val << 6) + T[c];
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 8/50] server - MEDIUM
**File:** `src\server\profiling_api_handler.cpp:150`
**Category:** copy_overhead
**Message:** push_back in loop — consider pre-allocating with reserve()

### Function Context
```cpp
  149 |         for (const auto& profile : slow_queries) {
  150 | >>>         result.push_back(profile->toJSON());
  151 |         }
  152 |     
  153 |         return make_response(http::status::ok, result);
  154 |     }
  155 |     
  156 |     http::response<http::string_body> ProfilingApiHandler::handle_get_storage(
  157 |         const http::request<http::string_body>& /*req*/) {
  158 |         auto span = Tracer::startSpan("handle_get_storage");
  159 |     
  160 |         json result = {
  161 |             {"operation_summary", storage_profiler_->get_operation_summary()},
  162 |             {"cache_metrics", storage_profiler_->get_cache_metrics()},
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [ 9/50] server - MEDIUM
**File:** `src\server\postgres_session.cpp:730`
**Category:** copy_overhead
**Message:** push_back in loop — consider pre-allocating with reserve()

### Function Context
```cpp
  730 | >>>                         fields.push_back({colName, 0, 0, 25, -1, -1, 0}); // text type
  731 |                         }
  732 |                     }
  733 |     
  734 |                     if (fields.empty()) {
  735 |                         fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
  736 |                     }
  737 |     
  738 |                     sendRowDescription(fields);
  739 |                 } else {
  740 |                     // Non-SELECT query - send NoData
  741 |                     sendNoData();
  742 |                 }
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [10/50] metadata - MEDIUM
**File:** `src\metadata\information_schema.cpp:314`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
  314 | >>>     for (const auto& row : getColumns()) {
  315 |             cols_arr.push_back(row.toJSON());
  316 |         }
  317 |         j["columns"] = cols_arr;
  318 |     
  319 |         json stats_arr = json::array();
  320 |         for (const auto& row : getStatistics()) {
  321 |             stats_arr.push_back(row.toJSON());
  322 |         }
  323 |         j["statistics"] = stats_arr;
  324 |     
  325 |         json kcu_arr = json::array();
  326 |         for (const auto& row : getKeyColumnUsage()) {
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [11/50] llm - MEDIUM
**File:** `src\llm\gguf_loader.cpp:404`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
  399 |             for (auto dim : tensor.shape) {
  400 |                 num_elements *= dim;
  401 |             }
  402 |             tensor.size = num_elements * getGGMLTypeSize(tensor.type);
  403 |             tensor.offset = tensor_offset;
  404 | >>> 
  405 |             metadata_.tensors.push_back(tensor);
  406 |         }
  407 |     
  408 |         // Store data offset (aligned to 32 bytes)
  409 |         metadata_.data_offset = alignOffset(offset, 32);
  410 |     
  411 |         return true;
  412 |     }
  413 |     
  414 |     size_t GGUFLoader::getGGMLTypeSize(GGMLType type) const {
  415 |         switch (type) {
  416 |             case GGMLType::F32: return 4;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [12/50] server - MEDIUM
**File:** `src\server\http_server.cpp:6858`
**Category:** performance
**Message:** Unnecessary copy: use auto& for container element access

### Function Context
```cpp
 6855 |                     for (const auto& g : auth_ctx.groups) {
 6856 |                         scheduler_ctx.roles.insert(g);
 6857 |                     }
 6858 | >>>                 auto auth_header = req[http::field::authorization];
 6859 |                     if (!auth_header.empty()) {
 6860 |                         auto token = themis::AuthMiddleware::extractBearerToken(
 6861 |                             std::string_view(auth_header.data(), auth_header.size()));
 6862 |                         if (token) {
 6863 |                             auto authz = auth_->authorize(*token, "task:register");
 6864 |                             if (authz.authorized) {
 6865 |                                 scheduler_ctx.granted_permissions.insert("task:register");
 6866 |                             }
 6867 |                         }
 6868 |                     }
 6869 |                     TaskScheduler::setRequestContext(scheduler_ctx);
 6870 |                     scheduler_ctx_set = true;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [13/50] security - MEDIUM
**File:** `src\security\zero_trust_policy_enforcer.cpp:62`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
   62 | >>>     for (const auto& kv : policies_) {
   63 |             result.push_back(kv.second);
   64 |         }
   65 |         return result;
   66 |     }
   67 |     
   68 |     // ============================================================================
   69 |     // Core: per-request verification
   70 |     // ============================================================================
   71 |     
   72 |     VerificationResult ZeroTrustPolicyEnforcer::verify(const ZeroTrustContext& context) {
   73 |         metrics_.requests_total.fetch_add(1, std::memory_order_relaxed);
   74 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [14/50] index - MEDIUM
**File:** `src\index\ann_index.cpp:84`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
   83 |                 if (cumsum >= threshold) { chosen = i; break; }
   84 | >>>         }
   85 |             centroids.emplace_back(data + chosen * d, data + chosen * d + d);
   86 |         }
   87 |     
   88 |         assignments.assign(n, 0);
   89 |     
   90 |         for (size_t iter = 0; iter < iters; ++iter) {
   91 |             // Assignment step
   92 |             for (size_t i = 0; i < n; ++i) {
   93 |                 float best = std::numeric_limits<float>::max();
   94 |                 size_t best_c = 0;
   95 |                 for (size_t c = 0; c < k; ++c) {
   96 |                     float dist = l2sq(data + i * d, centroids[c].data(), d);
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [15/50] storage - MEDIUM
**File:** `src\storage\columnar_format.cpp:343`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
  337 |             if (idx >= dictionary.size()) {
  338 |                 return tl::unexpected(Error(
  339 |                     errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
  340 |                     "Dictionary decode: invalid index"
  341 |                 ));
  342 |             }
  343 | >>> 
  344 |             decoded.push_back(dictionary[idx]);
  345 |         }
  346 |     
  347 |         return decoded;
  348 |     }
  349 |     
  350 |     bool DictionaryCodec::shouldUseDictionary(const std::vector<std::string>& data,
  351 |                                              [[maybe_unused]] double min_compression_ratio) {
  352 |         if (data.empty()) return false;
  353 |     
  354 |         // Calculate unique strings
  355 |         std::unordered_set<std::string> unique_strings(data.begin(), data.end());
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [16/50] analytics - MEDIUM
**File:** `src\analytics\anomaly_detection.cpp:966`
**Category:** performance
**Message:** vector::push_back in loop without prior reserve()

### Function Context
```cpp
  966 | >>>     for (size_t i = 0; i < contrib.size() && i < impl_->feature_names.size(); ++i) {
  967 |             exp.feature_contributions.emplace_back(impl_->feature_names[i], contrib[i]);
  968 |         }
  969 |     
  970 |         // Sort by descending contribution
  971 |         std::sort(exp.feature_contributions.begin(), exp.feature_contributions.end(),
  972 |                   [](const auto &a, const auto &b) { return a.second > b.second; });
  973 |     
  974 |         std::ostringstream ss;
  975 |         ss << "Anomaly score " << exp.score << " via " << anomalyMethodName(impl_->cfg.method) << ". ";
  976 |         if (!exp.feature_contributions.empty()) {
  977 |             ss << "Top driver: " << exp.feature_contributions[0].first
  978 |                << " (contribution=" << exp.feature_contributions[0].second << ")";
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [17/50] llm - MEDIUM
**File:** `src\llm\lora_security_validator.cpp:781`
**Category:** performance
**Message:** Unnecessary copy: use auto& for container element access

### Function Context
```cpp
  776 |                 if (!tensor_info.contains("data_offsets") || !tensor_info.contains("dtype")) {
  777 |                     continue;
  778 |                 }
  779 |     
  780 |                 auto dtype = tensor_info["dtype"].get<std::string>();
  781 | >>>             auto offsets = tensor_info["data_offsets"].get<std::vector<uint64_t>>();
  782 |     
  783 |                 if (offsets.size() != 2) continue;
  784 |     
  785 |                 // Validate offsets to prevent overflow and out-of-bounds access
  786 |                 if (offsets[0] > offsets[1]) {
  787 |                     spdlog::warn("Invalid tensor offsets: start {} > end {}", offsets[0], offsets[1]);
  788 |                     continue;
  789 |                 }
  790 |     
  791 |                 // Check for overflow when adding data_offset
  792 |                 if (offsets[0] > UINT64_MAX - data_offset || offsets[1] > UINT64_MAX - data_offset) {
  793 |                     spdlog::warn("Tensor offset would overflow: data_offset={}, offsets=[{}, {}]",
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [18/50] server - MEDIUM
**File:** `src\server\import_wizard_builder.cpp:268`
**Category:** performance
**Message:** String concatenation in loop (use std::stringstream)

### Function Context
```cpp
  268 | >>>     html += "  for(var i=1;i<=5;i++){\n";
  269 |         html += "    var p=document.getElementById('panel-'+i);\n";
  270 |         html += "    var t=document.getElementById('step-tab-'+i);\n";
  271 |         html += "    if(p) p.style.display=(i===n)?'':'none';\n";
  272 |         html += "    if(t){\n";
  273 |         html += "      t.className='step'+(i===n?' active':i<n?' done':'');\n";
  274 |         html += "    }\n";
  275 |         html += "  }\n";
  276 |         html += "  currentStep=n;\n";
  277 |         html += "}\n";
  278 |     
  279 |         // buildReview
  280 |         html += "function buildReview(){\n";
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [19/50] auth - HIGH
**File:** `src\auth\rate_limiter_backend.cpp:209`
**Category:** null_dereference
**Message:** Potential null pointer dereference

### Function Context
```cpp
  209 | >>>         THEMIS_WARN("RedisRateLimiterBackend::increment: command failed: {}", ctx_->errstr);
  210 |             disconnect();
  211 |             return 0; // fail-open
  212 |         }
  213 |     
  214 |         int64_t count = 0;
  215 |         if (reply->type == REDIS_REPLY_INTEGER) {
  216 |             count = reply->integer;
  217 |         } else if (reply->type == REDIS_REPLY_ERROR) {
  218 |             THEMIS_WARN("RedisRateLimiterBackend::increment: Lua error: {}",
  219 |                         reply->str ? reply->str : "unknown");
  220 |         }
  221 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [20/50] importers - HIGH
**File:** `src\importers\mdm_engine.cpp:183`
**Category:** pointer_arithmetic
**Message:** Pointer/array access without bounds validation

### Function Context
```cpp
  168 |             for (const auto& match : matches) {
  169 |                 EntityLink link;
  170 |                 link.source_id           = src_id;
  171 |                 link.target_id           = match.entity_id;
  172 |                 link.link_type           = config.preferred_link_type;
  173 |                 link.status              = (match.hybrid_score >= config.deterministic_threshold)
  174 |                                            ? ResolutionStatus::RESOLVED
  175 |                                            : (config.auto_resolve_conflicts
  176 |                                               ? ResolutionStatus::RESOLVED
  177 |                                               : ResolutionStatus::MANUAL_REVIEW);
  178 |                 link.confidence          = match.hybrid_score;
  179 |                 link.matching_evidence   = match.confidence_evidence;
  180 |                 link.created_at          = now;
  181 |                 link.created_by          = config.initiated_by;
  182 |                 link.metadata["collection"] = collection_name;
  183 | >>>             link.metadata["match_method"] = match.match_method;
  184 |     
  185 |                 if (linker_.createLink(link, options)) {
  186 |                     if (!options.dry_run) {
  187 |                         created.push_back(link);
  188 |                     }
  189 |                 }
  190 |     
  191 |                 // Optionally create the reverse link.
  192 |                 if (config.create_reverse_links) {
  193 |                     EntityLink reverse = link;
  194 |                     std::swap(reverse.source_id, reverse.target_id);
  195 |                     reverse.metadata["reverse"] = true;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [21/50] query - HIGH
**File:** `src\query\vectorized_execution.cpp:225`
**Category:** no_retry_logic
**Message:** database_query without retry logic — transient failures will propagate

### Function Context
```cpp
  221 |         std::vector<VectorizedAggregation>  aggregations) {
  222 |     
  223 |         VectorizedQueryPlan plan;
  224 |         plan.addAggregate(std::move(aggregations));
  225 | >>>     return execute(rows, plan);
  226 |     }
  227 |     
  228 |     Result<std::vector<nlohmann::json>> VectorizedExecutionEngine::project(
  229 |         const std::vector<nlohmann::json>& rows,
  230 |         std::vector<std::string>           fields) {
  231 |     
  232 |         VectorizedQueryPlan plan;
  233 |         plan.addProject(std::move(fields));
  234 |         return execute(rows, plan);
  235 |     }
  236 |     
  237 |     Result<std::vector<nlohmann::json>> VectorizedExecutionEngine::sort(
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [22/50] query - CRITICAL
**File:** `src\query\aql_translator.cpp:1618`
**Category:** data_race
**Message:** Shared data access without lock protection

### Function Context
```cpp
 1617 |                     if (binOp->op == BinaryOperator::Or) {
 1618 | >>>                     auto notLeft = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->left);
 1619 |                         auto notRight = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->right);
 1620 |                         auto andExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::And, notLeft, notRight);
 1621 |     
 1622 |                         return convertToDNF(andExpr, table, error);
 1623 |                     }
 1624 |     
 1625 |                     // NOT (A AND B) = (NOT A) OR (NOT B)
 1626 |                     if (binOp->op == BinaryOperator::And) {
 1627 |                         auto notLeft = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->left);
 1628 |                         auto notRight = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->right);
 1629 |                         auto orExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Or, notLeft, notRight);
 1630 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [23/50] performance - CRITICAL
**File:** `src\performance\prometheus_exporter.cpp:85`
**Category:** data_race
**Message:** Shared data access without lock protection

### Function Context
```cpp
   77 |                 for (const auto* m : metrics_vec) {
   78 |                     avg_hnsw += m->hnsw_search_cycles;
   79 |                     avg_pointer += m->pointer_passing_cycles;
   80 |                     avg_llm += m->llm_inference_cycles;
   81 |                     avg_cache += m->cache_miss_cycles;
   82 |                     avg_pcie_h2d += m->pcie_host_to_device_cycles;
   83 |                     avg_pcie_d2h += m->pcie_device_to_host_cycles;
   84 |                     avg_total += m->total_cycles;
   85 | >>>                 avg_cpu_eff += m->cpu_efficiency_ratio;
   86 |                 }
   87 |     
   88 |                 size_t count = metrics_vec.size();
   89 |                 avg_hnsw /= count;
   90 |                 avg_pointer /= count;
   91 |                 avg_llm /= count;
   92 |                 avg_cache /= count;
   93 |                 avg_pcie_h2d /= count;
   94 |                 avg_pcie_d2h /= count;
   95 |                 avg_total /= count;
   96 |                 avg_cpu_eff /= count;
   97 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [24/50] analytics - MEDIUM
**File:** `src\analytics\anomaly_detection.cpp:1169`
**Category:** observability
**Message:** No latency measurement for operation

### Function Context
```cpp
 1169 | >>> std::optional<AnomalyResult> StreamingAnomalyDetector::process(const DataPoint &point) {
 1170 |         // ── Phase 0: read trained state under a brief shared detector lock ────────
 1171 |         bool is_trained = false;
 1172 |         {
 1173 |             std::shared_lock<std::shared_mutex> dl(detector_mu_);
 1174 |             is_trained = detector_.isTrained();
 1175 |         }
 1176 |     
 1177 |         // ── Phase 1: update window under window lock only (≤ 50 µs) ──────────────
 1178 |         bool need_initial_train = false;
 1179 |         bool need_retrain       = false;
 1180 |         {
 1181 |             std::unique_lock<std::shared_mutex> lk(window_mu_);
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [25/50] server - HIGH
**File:** `src\server\http_server.cpp:11693`
**Category:** legacy_duplication
**Message:** Legacy/compatibility/deprecation marker detected (review removal/containment plan).

### Function Context
```cpp
11685 |                                 "(user={}, from_seq={}, prefix='{}')",
11686 |                                 decision.user_id, decision.from_sequence,
11687 |                                 decision.key_prefix);
11688 |     
11689 |                     auto ws_session = std::make_shared<WebSocketSession>(
11690 |                         std::move(socket_), server_);
11691 |                     ws_session->setRequestPath(ws_path);
11692 |                     // Pre-configure CDC subscription from URL parameters for the
11693 | >>>                 // legacy /v2/changes protocol only.  The new /v2/cdc/stream
11694 |                     // endpoint receives subscriptions via JSON frames after connect.
11695 |                     if (ws_path == "/v2/changes") {
11696 |                         ws_session->subscribeToCDC(decision.from_sequence,
11697 |                                                    decision.key_prefix);
11698 |                     }
11699 |                     if (server_->websocket_manager_) {
11700 |                         server_->websocket_manager_->addSession(ws_session);
11701 |                     }
11702 |                     ws_session->run(std::move(request_));
11703 |                     return;
11704 |                 }
11705 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [26/50] governance - CRITICAL
**File:** `src\governance\policy_manager.cpp:653`
**Category:** smart_ptr_misuse
**Message:** Raw new without immediate wrapping in smart pointer

### Function Context
```cpp
  653 | >>>     THEMIS_INFO("Rolled back rule {} to version {} (new version: {})", rule_id, version, restored.version);
  654 |         return true;
  655 |     }
  656 |     
  657 |     bool PolicyManager::rollbackToPreviousVersion(const std::string &rule_id, const std::string &modified_by) {
  658 |         std::string latest = version_history_.getLastRecordedVersion(rule_id);
  659 |         if (latest.empty()) {
  660 |             THEMIS_WARN("No version history found for rule {}", rule_id);
  661 |             return false;
  662 |         }
  663 |         return rollbackToVersion(rule_id, latest, modified_by);
  664 |     }
  665 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [27/50] rag - MEDIUM
**File:** `src\rag\continuous_learning_orchestrator.cpp:1122`
**Category:** string_concat_loop
**Message:** String concatenation in loop — O(n²) behavior

### Function Context
```cpp
 1121 |             for (const char c : s) {
 1122 | >>>             if      (c == '"')  out += "\\\"";
 1123 |                 else if (c == '\\') out += "\\\\";
 1124 |                 else if (c == '\n') out += "\\n";
 1125 |                 else                out += c;
 1126 |             }
 1127 |             return out;
 1128 |         };
 1129 |     
 1130 |         static const std::unordered_map<int, std::string> kPhaseNames{
 1131 |             {static_cast<int>(LoopPhase::LOOP_1_HNSW_QUERY),   "LOOP_1_HNSW_QUERY"},
 1132 |             {static_cast<int>(LoopPhase::LOOP_2_WORKLOAD),      "LOOP_2_WORKLOAD"},
 1133 |             {static_cast<int>(LoopPhase::LOOP_3_SCHEMA_INDEX),  "LOOP_3_SCHEMA_INDEX"},
 1134 |             {static_cast<int>(LoopPhase::LOOP_4_RLAIF),         "LOOP_4_RLAIF"},
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [28/50] auth - MEDIUM
**File:** `src\auth\rate_limiter_backend.cpp:350`
**Category:** uncaught_exception
**Message:** Generic catch(...) — specific exception types ignored

### Function Context
```cpp
  350 | >>>         } catch (...) {
  351 |             }
  352 |         }
  353 |         return redisFallbackBackend().getCount(key, window_seconds);
  354 |     }
  355 |     
  356 |     void RedisRateLimiterBackend::reset(const std::string& key)
  357 |     {
  358 |         ResetFn fn;
  359 |         {
  360 |             std::lock_guard<std::mutex> lk(s_redis_rate_bridge_mutex);
  361 |             fn = s_reset_fn;
  362 |         }
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [29/50] server - MEDIUM
**File:** `src\server\llm_api_handler.cpp:2127`
**Category:** uncaught_exception
**Message:** Generic catch(...) — specific exception types ignored

### Function Context
```cpp
 2127 | >>>             } catch (...) {
 2128 |                     spdlog::warn(
 2129 |                         "LLMApiHandler::handleOpenAIChatCompletions non-stream failed with unknown error: model='{}'",
 2130 |                         model_id.empty() ? std::string{"default"} : model_id);
 2131 |                     logCurrentException("LLMApiHandler::handleOpenAIChatCompletions non-streaming");
 2132 |                     auto err = llm::OpenAICompatAdapter::buildError("Inference failed", "server_error");
 2133 |                     return createJsonResponse(err, http::status::internal_server_error);
 2134 |                 }
 2135 |     
 2136 |                 json response_json = llm::OpenAICompatAdapter::buildResponse(
 2137 |                     llm_response, model_id, completion_id);
 2138 |     
 2139 |                 spdlog::info(
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [30/50] llm - HIGH
**File:** `src\llm\ethics_aware_confidence_detector.cpp:404`
**Category:** o_n_squared
**Message:** O(n²) pattern: find() on vector inside loop

### Function Context
```cpp
  404 | >>>     for (const auto& word : impl_->hedge_words_en) {
  405 |             if (text_lower.find(word) != std::string::npos) {
  406 |                 detected.push_back(word);
  407 |             }
  408 |         }
  409 |     
  410 |         // Check German hedge words
  411 |         for (const auto& word : impl_->hedge_words_de) {
  412 |             if (text_lower.find(word) != std::string::npos) {
  413 |                 detected.push_back(word);
  414 |             }
  415 |         }
  416 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [31/50] training - MEDIUM
**File:** `src\training\provenance_tracker.cpp:342`
**Category:** determinism
**Message:** Non-deterministic unordered_map/set iteration order

### Function Context
```cpp
  334 |         const std::vector<std::string>& auditLog() const {
  335 |             return audit_log_;
  336 |         }
  337 |     
  338 |     private:
  339 |         ProvenanceTrackerConfig                           config_;
  340 |         std::string                                       db_connection_;
  341 |         QueryEngine*                                      query_engine_;   ///< non-owning; nullptr = offline/test
  342 | >>>     std::unordered_map<std::string, ProvenanceRecord> store_;
  343 |         std::vector<std::string>                          audit_log_;
  344 |     
  345 |         // Build an AQL query string from a template by substituting @placeholder tokens.
  346 |         // Matches the pattern used in auto_labeler.cpp::buildQuery().
  347 |         static std::string buildQuery(
  348 |             const std::string& tmpl,
  349 |             const std::vector<std::pair<std::string, std::string>>& bindings)
  350 |         {
  351 |             std::string query = tmpl;
  352 |             for (const auto& [placeholder, value] : bindings) {
  353 |                 std::string token = "@" + placeholder;
  354 |                 size_t pos = 0;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [32/50] llm - MEDIUM
**File:** `src\llm\embedded_llm.cpp:238`
**Category:** llm_ai_safety
**Message:** LLM inference without token limit or timeout (DOS risk)

### Function Context
```cpp
  234 |             if (generate_full_fn_) {
  235 |                 return generate_full_fn_(request);
  236 |             }
  237 |         }
  238 | >>>     return wrapper_->generate(request);
  239 |     }
  240 |     
  241 |     // ═══════════════════════════════════════════════════════════
  242 |     // Utility methods
  243 |     // ═══════════════════════════════════════════════════════════
  244 |     
  245 |     bool EmbeddedLLM::isReady() const {
  246 |         {
  247 |             std::lock_guard<std::mutex> lock(callback_mutex_);
  248 |             if (generate_full_fn_ || embed_fn_) {
  249 |                 return true;
  250 |             }
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [33/50] importers - HIGH
**File:** `src\importers\s3_importer.cpp:613`
**Category:** uninitialized_access
**Message:** Container element access before initialization

### Function Context
```cpp
  613 | >>>         THEMIS_WARN("S3 import error [{}]: {} ({})", location, message,
  614 |                         static_cast<uint32_t>(code));
  615 |         }
  616 |     }
  617 |     
  618 |     void S3Importer::emitMetric(const ImportOptions& options,
  619 |                                   const std::string& metric,
  620 |                                   const std::map<std::string, std::string>& labels,
  621 |                                   double value) const {
  622 |         if (options.metrics_callback)
  623 |             options.metrics_callback(metric, labels, value);
  624 |     }
  625 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [34/50] server - MEDIUM
**File:** `src\server\shard_repair_api_handler.cpp:160`
**Category:** hardcoded_path
**Message:** Hardcoded path separator — not portable

### Function Context
```cpp
  160 | >>>          << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json();raw.textContent=JSON.stringify(data,null,2);summary.innerHTML='';const cards=[['Status',data.status],['Engine',data.engine_running?'running':'stopped'],['Scans',data.metrics?.total_scans ?? 0],['Repairs OK',data.metrics?.total_repairs_successful ?? 0],['Repairs Failed',data.metrics?.total_repairs_failed ?? 0],['Active Jobs',(data.active_jobs||[]).length]];cards.forEach(([label,val])=>{const el=document.createElement('div');el.className='card';el.innerHTML=`<div>${label}</div><div class=\"metric\">${val}</div>`;summary.appendChild(el);});shards.innerHTML=(data.shards||[]).map(s=>`<tr><td>${s.shard_id||'-'}</td><td>${badge(s.status||'healthy')}</td><td>${s.documents_scanned}</td><td>${s.documents_healthy}</td><td>${s.documents_degraded}</td><td>${s.documents_unrecoverable}</td><td>${s.last_error||''}</td></tr>`).join('');jobs.innerHTML=(data.active_jobs||[]).map(j=>`<tr><td>${j.job_id}</td><td>${j.shard_id||'-'}</td><td>${j.document_id||'-'}</td><td>${j.is_full_scan?'yes':'no'}</td><td>${fmtTime(j.submitted_at_unix_ms)}</td><td>${j.completed?'yes':'no'}</td><td>${j.success?'yes':'no'}</td></tr>`).join('');}"
  161 |              << "async function post(url,body){const res=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body||{})});const data=await res.json();if(!res.ok){throw new Error(data.message||data.error||'request failed');}return data;}"
  162 |              << "document.getElementById('refreshBtn').onclick=()=>load().catch(e=>setFlash(e.message));"
  163 |              << "document.getElementById('scanBtn').onclick=async()=>{const r=await post('/v1/admin/repair/scan',{});setFlash(`Full scan queued: ${r.job_id}`);load();};"
  164 |              << "document.getElementById('repairBtn').onclick=async()=>{const shardId=document.getElementById('shardId').value.trim();const r=await post('/v1/admin/repair',{shard_id:shardId});setFlash(`Repair queued: ${r.job_id}`);load();};"
  165 |              << "load().catch(e=>{raw.textContent=e.message;setFlash(e.message);});setInterval(()=>load().catch(()=>{}),10000);"
  166 |              << "</script></main></body></html>";
  167 |         return html.str();
  168 |     }
  169 |     
  170 |     } // namespace
  171 |     
  172 |     ShardRepairApiHandler::ShardRepairApiHandler(
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [35/50] llm - CRITICAL
**File:** `src\llm\lora_framework\gpu_lora_layers.cpp:431`
**Category:** llm_ai_safety
**Message:** User input in prompt without sanitization (injection risk)

### Function Context
```cpp
  402 |                 GPUTensor grad_input({batch_size, in_dim_}, device_);
  403 |     
  404 |                 // Get raw pointers for GPU kernel
  405 |                 // Safety: GPUTensor guarantees proper float alignment for GPU memory
  406 |                 // All GPU tensors (including gradients) are allocated with hipMalloc
  407 |                 assert(performance::is_aligned<alignof(float)>(input_for_backward.data()) &&
  408 |                        "Input tensor must be float-aligned for GPU operations");
  409 |                 assert(performance::is_aligned<alignof(float)>(B_->data()) &&
  410 |                        "B tensor must be float-aligned for GPU operations");
  411 |                 assert(performance::is_aligned<alignof(float)>(A_->data()) &&
  412 |                        "A tensor must be float-aligned for GPU operations");
  413 |                 assert(performance::is_aligned<alignof(float)>(grad_output.data()) &&
  414 |                        "Grad output tensor must be float-aligned for GPU operations");
  415 |                 assert(performance::is_aligned<alignof(float)>(A_->grad->data()) &&
  416 |                        "Grad A tensor must be float-aligned for GPU operations");
  417 |                 assert(performance::is_aligned<alignof(float)>(B_->grad->data()) &&
  418 |                        "Grad B tensor must be float-aligned for GPU operations");
  419 |                 assert(performance::is_aligned<alignof(float)>(grad_input.data()) &&
  420 |                        "Grad input tensor must be float-aligned for GPU operations");
  421 |     
  422 |                 const float* input_ptr = reinterpret_cast<const float*>(input_for_backward.data());
  423 |                 const float* B_ptr = reinterpret_cast<const float*>(B_->data());
  424 |                 const float* A_ptr = reinterpret_cast<const float*>(A_->data());
  425 |                 const float* grad_output_ptr = reinterpret_cast<const float*>(grad_output.data());
  426 |                 float* grad_A_ptr = reinterpret_cast<float*>(A_->grad->data());
  427 |                 float* grad_B_ptr = reinterpret_cast<float*>(B_->grad->data());
  428 |                 float* grad_input_ptr = reinterpret_cast<float*>(grad_input.data());
  429 |     
  430 |                 hipError_t err = hip::fused::launch_fused_lora_backward(
  431 | >>>                 input_ptr, B_ptr, A_ptr, grad_output_ptr,
  432 |                     grad_A_ptr, grad_B_ptr, grad_input_ptr,
  433 |                     batch_size, in_dim_, rank_, out_dim_, scaling_);
  434 |     
  435 |                 if (err == hipSuccess) {
  436 |                     return grad_input;
  437 |                 }
  438 |                 // Fall back to unfused on error
  439 |                 spdlog::warn("Fused HIP backward kernel failed, falling back to unfused");
  440 |             }
  441 |     #endif
  442 |         }
  443 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [36/50] llm - CRITICAL
**File:** `src\llm\lora_framework\gpu_embedding_layer.cpp:49`
**Category:** llm_ai_safety
**Message:** Model loading without integrity verification (poisoning risk)

### Function Context
```cpp
   46 |         spdlog::info("Creating GPUEmbeddingLayer: vocab_size={}, hidden_dim={}, device={}",
   47 |                      vocab_size, hidden_dim, static_cast<int>(device.type));
   48 |     
   49 | >>>     // Upload embedding weights to GPU
   50 |         std::vector<float> weights_vec(embedding_weights, embedding_weights + vocab_size * hidden_dim);
   51 |         embedding_weights_.upload(weights_vec);
   52 |     
   53 |         spdlog::debug("GPUEmbeddingLayer: Uploaded {} MB to GPU",
   54 |                       (vocab_size * hidden_dim * sizeof(float)) / (1024.0 * 1024.0));
   55 |     }
   56 |     
   57 |     GPUEmbeddingLayer::~GPUEmbeddingLayer() = default;
   58 |     
   59 |     GPUEmbeddingLayer::GPUEmbeddingLayer(GPUEmbeddingLayer&& other) noexcept
   60 |         : embedding_weights_(std::move(other.embedding_weights_))
   61 |         , vocab_size_(other.vocab_size_)
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [37/50] llm - HIGH
**File:** `src\llm\lora_framework\kernels\hip_kernels.cpp:546`
**Category:** llm_ai_safety
**Message:** User input passed to LLM without normalization/sanitization

### Function Context
```cpp
  544 |         if (stream != nullptr) {
  545 |             hipLaunchKernelGGL(lora_backward_B_kernel, gridDim, blockDim, 0, stream,
  546 | >>>             input, A, grad_output, grad_B, batch_size, in_dim, rank, out_dim, scaling);
  547 |         } else {
  548 |             hipLaunchKernelGGL(lora_backward_B_kernel, gridDim, blockDim, 0, 0,
  549 |                 input, A, grad_output, grad_B, batch_size, in_dim, rank, out_dim, scaling);
  550 |         }
  551 |     
  552 |         return hipGetLastError();
  553 |     }
  554 |     
  555 |     hipError_t launch_mse_loss_reduction_kernel(
  556 |         const float* predictions,
  557 |         const float* targets,
  558 |         float* partial_sums,
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [38/50] query - HIGH
**File:** `src\query\optimizer_cost_model.cpp:159`
**Category:** llm_ai_safety
**Message:** User input passed to LLM without normalization/sanitization

### Function Context
```cpp
  148 |             if (it != columnStats.end()) {
  149 |                 const auto& colStats = it->second;
  150 |                 predSelectivity = estimateSelectivity(pred, colStats);
  151 |             }
  152 |     
  153 |             combinedSelectivity *= predSelectivity;
  154 |         }
  155 |     
  156 |         cost.selectivity = combinedSelectivity;
  157 |         cost.outputRows = static_cast<size_t>(inputRows * combinedSelectivity);
  158 |     
  159 | >>>     // CPU cost: evaluate predicates for each input row
  160 |         double predicateCost = static_cast<double>(predicates.size()) *
  161 |                               constants_.cpuCostPerPredicate;
  162 |         cost.cpuCost = calculateCpuCost(inputRows, predicateCost);
  163 |     
  164 |         return cost;
  165 |     }
  166 |     
  167 |     // =============================
  168 |     // Join Cost Estimation
  169 |     // =============================
  170 |     
  171 |     OptimizerCostModel::JoinCost OptimizerCostModel::estimateNestedLoopJoin(
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [39/50] importers - HIGH
**File:** `src\importers\schema_inference.cpp:46`
**Category:** llm_ai_safety
**Message:** LLM output used without validation (hallucination/bias risk)

### Function Context
```cpp
   37 |                     s.compare(s.size() - std::strlen(suf), std::strlen(suf), suf) == 0) {
   38 |                     s.resize(s.size() - std::strlen(suf));
   39 |                 }
   40 |             }
   41 |             return s;
   42 |         };
   43 |         return stripSuffix(a) == stripSuffix(b);
   44 |     }
   45 |     
   46 | >>> double SchemaInferenceEngine::jaccardSimilarity(const std::vector<std::string>& a,
   47 |                                                      const std::vector<std::string>& b) const {
   48 |         if (a.empty() && b.empty()) return 1.0;
   49 |         if (a.empty() || b.empty()) return 0.0;
   50 |     
   51 |         std::unordered_set<std::string> setA(a.begin(), a.end());
   52 |         std::unordered_set<std::string> setB(b.begin(), b.end());
   53 |     
   54 |         size_t intersection = 0;
   55 |         for (const auto& v : setA) {
   56 |             if (setB.count(v)) ++intersection;
   57 |         }
   58 |         size_t union_size = setA.size() + setB.size() - intersection;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [40/50] utils - HIGH
**File:** `src\utils\memory\pool_allocator.cpp:230`
**Category:** observability
**Message:** Critical function allocate without trace point

### Function Context
```cpp
  230 | >>> Result<void*> BuddyAllocator::allocate(size_t size, AllocationHint hint) {
  231 |         if (size == 0) {
  232 |             return Err<void*>(errors::ErrorCode::ERR_MEMORY_INVALID_SIZE,
  233 |                              "Allocation size must be greater than 0");
  234 |         }
  235 |     
  236 |         std::lock_guard<std::mutex> lock(impl_->mutex);
  237 |     
  238 |         // Align to cache line if requested
  239 |         if (hint == AllocationHint::CACHE_LINE_ALIGNED) {
  240 |             size = alignSize(size, CACHE_LINE_SIZE);
  241 |         }
  242 |     
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [41/50] utils - MEDIUM
**File:** `src\utils\cron_parser.cpp:283`
**Category:** repeated_lookup
**Message:** Repeated find() for same key: tm

### Function Context
```cpp
  278 |             if (!years_.empty()) {
  279 |                 int year = tm.tm_year + 1900;
  280 |                 if (years_.find(year) == years_.end()) return false;
  281 |             }
  282 |             if (minutes_.find(tm.tm_min) == minutes_.end())      return false;
  283 | >>>         if (hours_.find(tm.tm_hour) == hours_.end())         return false;
  284 |     
  285 |             bool day_matches     = days_.find(tm.tm_mday) != days_.end();
  286 |             bool weekday_matches = weekdays_.find(tm.tm_wday) != weekdays_.end();
  287 |             bool day_is_wildcard     = days_.size() == 31;
  288 |             bool weekday_is_wildcard = weekdays_.size() == 7;
  289 |     
  290 |             if (day_is_wildcard && weekday_is_wildcard) {
  291 |                 // ok
  292 |             } else if (!day_is_wildcard && weekday_is_wildcard) {
  293 |                 if (!day_matches) return false;
  294 |             } else if (day_is_wildcard && !weekday_is_wildcard) {
  295 |                 if (!weekday_matches) return false;
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [42/50] security - HIGH
**File:** `src\security\encrypted_field.cpp:147`
**Category:** uncaught_exception
**Message:** Exception thrown without try/catch context

### Function Context
```cpp
  146 |         if (str.size() < sizeof(uint32_t)) {
  147 | >>>         throw DecryptionException("Invalid vector serialization: too short");
  148 |         }
  149 |     
  150 |         // Read size
  151 |         uint32_t size;
  152 |         std::memcpy(&size, str.data(), sizeof(size));
  153 |     
  154 |         // Validate size
  155 |         size_t expected_bytes = sizeof(uint32_t) + size * sizeof(float);
  156 |         if (str.size() != expected_bytes) {
  157 |             throw DecryptionException(
  158 |                 "Invalid vector serialization: size mismatch (expected " +
  159 |                 std::to_string(expected_bytes) + " bytes, got " +
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [43/50] temporal - HIGH
**File:** `src\temporal\temporal_tier_manager.cpp:423`
**Category:** repeated_search
**Message:** find/search in loop — O(n²) or worse

### Function Context
```cpp
  422 |                 for (const auto& [t, _] : warm_) {
  423 | >>>                 if (std::find(tables.begin(), tables.end(), t) == tables.end())
  424 |                         tables.push_back(t);
  425 |                 }
  426 |             }
  427 |             for (const auto& t : tables) {
  428 |                 if (!compact_stop_) compactTable(t);
  429 |             }
  430 |         }
  431 |     }
  432 |     
  433 |     // ============================================================================
  434 |     // Observability
  435 |     // ============================================================================
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [44/50] llm - HIGH
**File:** `src\llm\lora_framework\multi_gpu_lora_layer.cpp:115`
**Category:** audit_logging
**Message:** Hardcoded std::cout/printf instead of structured logging

### Function Context
```cpp
  115 | >>> std::vector<GPUTensor> MultiGPULoRALayer::forward(const std::vector<GPUTensor>& inputs) {
  116 |         if (inputs.size() != static_cast<size_t>(ctx_.num_gpus())) {
  117 |             throw std::invalid_argument(
  118 |                 "Number of input tensors must match number of GPUs");
  119 |         }
  120 |     
  121 |         auto start = std::chrono::high_resolution_clock::now();
  122 |     
  123 |         std::vector<GPUTensor> outputs;
  124 |         outputs.reserve(inputs.size());
  125 |     
  126 |         // Forward pass on each GPU independently
  127 |         for (int device_index = 0; device_index < ctx_.num_gpus(); ++device_index) {
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [45/50] llm - HIGH
**File:** `src\llm\model_loader.cpp:596`
**Category:** memory_order
**Message:** memory_order_relaxed used — potential visibility issue

### Function Context
```cpp
  591 |     json LazyModelLoader::getCacheStats() const {
  592 |         std::lock_guard<std::mutex> lock(mutex_);
  593 |     
  594 |         // Load atomic counters once to ensure consistency
  595 |         size_t hits = cache_hits_.load(std::memory_order_relaxed);
  596 | >>>     size_t misses = cache_misses_.load(std::memory_order_relaxed);
  597 |         size_t evict = evictions_.load(std::memory_order_relaxed);
  598 |         size_t loaded = models_loaded_.load(std::memory_order_relaxed);
  599 |     
  600 |         json stats;
  601 |         stats["cache_hits"] = hits;
  602 |         stats["cache_misses"] = misses;
  603 |         stats["evictions"] = evict;
  604 |         stats["models_loaded"] = loaded;
  605 |     
  606 |         if ((hits + misses) > 0) {
  607 |             stats["hit_rate"] = static_cast<double>(hits) / (hits + misses);
  608 |         } else {
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [46/50] storage - HIGH
**File:** `src\storage\columnar_format.cpp:706`
**Category:** size_assumption
**Message:** Hardcoded size assumption — pointer/int size may differ on platforms

### Function Context
```cpp
  703 |         for (size_t i = 1; i < data.size(); ++i) {
  704 |             int64_t delta = data[i] - reference;
  705 |             const uint8_t* delta_bytes = reinterpret_cast<const uint8_t*>(&delta);
  706 | >>>         encoded.insert(encoded.end(), delta_bytes, delta_bytes + sizeof(int64_t));
  707 |         }
  708 |     
  709 |         return encoded;
  710 |     }
  711 |     
  712 |     Result<std::vector<int32_t>> FrameOfReferenceCodec::decodeInt32(const std::vector<uint8_t>& encoded) {
  713 |         if (encoded.size() < sizeof(int32_t)) {
  714 |             return tl::unexpected(Error(
  715 |                 errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
  716 |                 "Frame-of-reference decode: no reference value"
  717 |             ));
  718 |         }
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [47/50] plugins - MEDIUM
**File:** `src\plugins\plugin_hot_plug_monitor.cpp:373`
**Category:** manual_cleanup
**Message:** Manual cleanup outside exception handler — not exception-safe

### Function Context
```cpp
  372 |             THEMIS_ERROR("Failed to add kevent: {}", strerror(errno));
  373 | >>>         close(dir_fd);
  374 |             close(kq);
  375 |             return;
  376 |         }
  377 |     
  378 |         // Track files we've seen along with their last-write timestamps
  379 |         std::map<std::string, fs::file_time_type> known_files;
  380 |         auto scan_directory = [&]() {
  381 |             std::map<std::string, fs::file_time_type> current_files;
  382 |             try {
  383 |                 for (const auto& entry : fs::directory_iterator(watch_directory_)) {
  384 |                     // Skip symlinks pointing to non-existent targets
  385 |                     if (entry.is_symlink()) {
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [48/50] security - CRITICAL
**File:** `src\security\access_control.cpp:557`
**Category:** audit_logging
**Message:** Security function "authorize" without audit log

### Function Context
```cpp
  544 |         if (!session_opt.has_value()) {
  545 |             return false;
  546 |         }
  547 |     
  548 |         auto& session = session_opt.value();
  549 |     
  550 |         AuthorizationContext context;
  551 |         context.user_id = session.user_id;
  552 |         context.roles = session.roles;
  553 |         context.resource = resource;
  554 |         context.action = action;
  555 |         context.timestamp = std::chrono::system_clock::now();
  556 |     
  557 | >>>     return authorize(context);
  558 |     }
  559 |     
  560 |     std::vector<Permission> AccessControl::getUserPermissions(const std::string& user_id) const {
  561 |         std::lock_guard<std::mutex> lock(mutex_);
  562 |     
  563 |         auto roles = user_role_store_->getUserRoles(user_id);
  564 |         return rbac_->getUserPermissions(roles);
  565 |     }
  566 |     
  567 |     // ============================================================================
  568 |     // Role Management
  569 |     // ============================================================================
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [49/50] observability - HIGH
**File:** `src\observability\metric_aggregator.cpp:394`
**Category:** performance
**Message:** O(n²) pattern: linear search inside nested loop

### Function Context
```cpp
  393 |                     for (const auto& gl : rule.group_by_labels) {
  394 | >>>                     auto it = effective_labels.find(gl);
  395 |                         if (it != effective_labels.end()) {
  396 |                             group_labels[gl] = it->second;
  397 |                         }
  398 |                     }
  399 |     
  400 |                     std::string gk = makeLabelFingerprint(group_labels);
  401 |                     glabels[gk] = group_labels;
  402 |                     for (double v : snap.values) {
  403 |                         grouped[gk].push_back(v);
  404 |                     }
  405 |                 }
  406 |             }
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_

## [50/50] config - CRITICAL
**File:** `src\config\config_metrics_exporter.cpp:29`
**Category:** missing_dtor
**Message:** Class RegisteredMetrics allocates resources but has no destructor

### Function Context
```cpp
   29 | >>> struct RegisteredMetrics {
   30 |         prometheus::Counter* resolution_hits{nullptr};
   31 |         prometheus::Counter* resolution_misses{nullptr};
   32 |         prometheus::Counter* legacy_fallbacks{nullptr};
   33 |         prometheus::Counter* new_path_hits{nullptr};
   34 |         prometheus::Counter* cache_hits{nullptr};
   35 |         prometheus::Counter* cache_misses{nullptr};
   36 |         prometheus::Counter* unmapped_requests{nullptr};
   37 |         prometheus::Family<prometheus::Counter>* legacy_family{nullptr};
   38 |         prometheus::Gauge* cache_hit_ratio{nullptr};
   39 |         prometheus::Gauge* cache_size{nullptr};
   40 |         prometheus::Gauge* cache_capacity{nullptr};
   41 |         prometheus::Gauge* cache_ttl_seconds{nullptr};
```

### Assessment
- [ ] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _[Add your reasoning here]_
