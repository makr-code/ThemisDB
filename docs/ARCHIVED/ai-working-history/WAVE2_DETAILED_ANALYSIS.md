# Wave 2: Category B Detailed Analysis (20 patterns found)

## Summary
- **Total patterns found:** 20
- **TRUE gaps requiring fix:** 9
- **FALSE POSITIVES:** 11

## Pattern Categorization

### FALSE POSITIVES (11 patterns)

These appear to be moved-from usage but are actually accessing different variables:

#### FP1: ingestion_manager.cpp:1978 - cfg.options vs options
```cpp
cfg.options = std::move(options);           // options moved INTO cfg.options
cfg.options["topic"] = topic;               // Accessing cfg.options, NOT options ✓
```
**Verdict:** FALSE POSITIVE - accessing `cfg.options`, not moved-from `options`

#### FP2: ingestion_manager.cpp:2060 - cfg.options vs options  
```cpp
cfg.options = std::move(options);
cfg.options["plugin_name"] = plugin_name;   // Accessing cfg.options ✓
```
**Verdict:** FALSE POSITIVE - same pattern as FP1

#### FP3: gpu/launcher.cpp:131 - lambda capture
```cpp
[this, items = std::move(items)]() mutable {
    results.reserve(items.size());          // items still accessible in lambda ✓
```
**Verdict:** FALSE POSITIVE - lambda copies/captures items, accessible in closure

#### FP4: aql/llm_aql_handler.cpp:850 - lambda capture
```cpp
= [orig_cb = std::move(orig_cb), cancel_token](...) {
    orig_cb(token);                         // orig_cb accessible in lambda capture ✓
```
**Verdict:** FALSE POSITIVE - lambda moves orig_cb into capture, accessible in closure

#### FP5: aql/llm_aql_handler.cpp:1185 - lambda capture
```cpp
= [orig_cb = std::move(orig_cb), cancel_token](...) {
    orig_cb(token);
```
**Verdict:** FALSE POSITIVE - same as FP4

#### FP6: llm/streaming_handler.cpp:126 - lambda capture
```cpp
return [sink = std::move(sink), counter, req_id](...) {
    sink(StreamingHandler::formatSseEvent(...));  // sink captured ✓
```
**Verdict:** FALSE POSITIVE - lambda captures sink

#### FP7: analytics/process_mining.cpp:328 - accessing member
```cpp
trace.events = std::move(events);
if (!trace.events.empty()) {                // Accessing trace.events, NOT events ✓
```
**Verdict:** FALSE POSITIVE - accessing `trace.events`, not moved-from `events`

#### FP8: distributed_knowledge/federated_distillation_coordinator.cpp:130
```cpp
round.labels = std::move(labels);
round.label_count = round.labels.size();     // Accessing round.labels ✓
```
**Verdict:** FALSE POSITIVE - accessing `round.labels`, not moved-from `labels`

#### FP9: llm/inference_engine_enhanced.cpp:154
```cpp
shared_pool_ = std::move(pool);
spdlog::info("...", shared_pool_.pool_size); // Accessing shared_pool_, not pool ✓
```
**Verdict:** FALSE POSITIVE - accessing `shared_pool_`, not moved-from `pool`

#### FP10: query_engine.cpp:4253,4295,4326 - first iteration
```cpp
if (first) { current = std::move(keys); first=false; }
// keys accessed later in same line after the move within the if statement
```
**Verdict:** FALSE POSITIVE - `keys` accessed after first=false, different iteration

#### FP11: transaction/transaction_manager.cpp:1225,1279 - lambda capture
```cpp
[this, old_entity = std::move(old_entity), ...]() {
    THEMIS_DEBUG("...", old_entity.getPrimaryKey());  // old_entity captured ✓
```
**Verdict:** FALSE POSITIVE - lambda captures old_entity

---

### TRUE GAPS (9 patterns)

#### TRUE 1: changefeed_api_handler.cpp:571-572
```cpp
consumer_id = std::move(cid_str);
} else if (!cid_str.empty() && !isValidChangefeedIdentifier(cid_str)) {
```
**Issue:** `cid_str` moved on line 571, accessed in condition on line 572
**Fix:** Check condition BEFORE the move, or restructure with is-moved flag

#### TRUE 2: cross_shard_transaction.cpp:3472-3473
```cpp
retries = std::move(deferred_precommits_);
deferred_precommits_.clear();
```
**Issue:** `deferred_precommits_` moved, then `.clear()` called (valid but poor style)
**Fix:** Remove `.clear()` - it's redundant after move; or move after clear

#### TRUE 3: replication_manager.cpp:4795
```cpp
batch.sequences = std::move(pending_);
pending_.clear();
```
**Issue:** Same pattern as TRUE 2 - member moved, then cleared
**Fix:** Remove `.clear()` or restructure

#### TRUE 4: training/auto_labeler.cpp:291
```cpp
modalities = std::move(fallback_modalities);
} else if (!fallback_modalities.empty()) {
```
**Issue:** `fallback_modalities` moved on previous line, accessed in condition
**Fix:** Check condition BEFORE the move

#### TRUE 5: transaction/saga_orchestrator.cpp:437
```cpp
std::vector<std::string> wave = std::move(ready);
ready.clear();
```
**Issue:** `ready` moved into `wave`, then `.clear()` called
**Fix:** Remove `.clear()` or restructure

#### TRUE 6: utils/pii_detector.cpp:101
```cpp
auto old_engines = std::move(engines_);
engines_.clear();
```
**Issue:** `engines_` moved, then `.clear()` called
**Fix:** Remove `.clear()` after move

#### TRUE 7: query_engine.cpp:4253
```cpp
if (first) { current = std::move(keys); first=false; }
// Next line: std::vector<std::string> intersected; 
// intersected.reserve(std::min(current.size(), keys.size()));
```
**Issue:** On first iteration, `keys` is moved into `current`, but then `keys.size()` is called
**Fix:** Track which iteration it is, or use `current.size()` instead

#### TRUE 8: query_engine.cpp:4295 
```cpp
if (first) { current = std::move(keys); first=false; }
// Same issue as TRUE 7
```
**Fix:** Same as TRUE 7

#### TRUE 9: query_engine.cpp:4326
```cpp
if (first) { current = std::move(keys); first=false; }
// Same issue as TRUE 7 and 8
```
**Fix:** Same as TRUE 7

---

## Fix Priority

### High Priority (5 fixes)
1. query_engine.cpp:4253, 4295, 4326 (3 similar patterns in intersection logic)
2. changefeed_api_handler.cpp:571-572
3. training/auto_labeler.cpp:291

### Medium Priority (4 fixes)
4. cross_shard_transaction.cpp:3472-3473
5. replication_manager.cpp:4795
6. transaction/saga_orchestrator.cpp:437
7. utils/pii_detector.cpp:101

---

## Next Steps
1. Fix TRUE 1-9 patterns
2. Document fixes with commit message
3. Run tests to verify no regressions
4. Generate Wave 2 completion report

