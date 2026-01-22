---
name: RAG Judge - BatchEvaluator Implementation
about: Implementierung von Batch-Processing und Async-Evaluation (3-4 Tage)
title: '[RAG-JUDGE-P6-BATCH] BatchEvaluator Implementation - Parallel & Async Processing'
labels: 'priority:P2, type:feature, area:performance, area:rag, effort:large, phase:6'
assignees: ''
---

## 📋 Übersicht

Implementierung des `BatchEvaluator` für effizientes Parallel-Batch-Processing und asynchrone Evaluation von RAG-Outputs.

**Namespace:** `themis::rag::judge`  
**Header:** `include/rag/batch_evaluator.h` (✅ bereits vorhanden)  
**Implementation:** `src/rag/batch_evaluator.cpp` (🚧 zu implementieren)  
**Dokumentation:** `docs/implementation-history/IMPLEMENTATION_PROGRESS_RAG_JUDGE_P5_P6.md`

## 🎯 Ziele

- ✅ API-Header definiert (225 LOC)
- 🚧 Worker-Thread-Pool (~500 LOC)
- 🚧 Async-Evaluation mit Futures/Promises
- 🚧 Queue-basierte Processing
- 🚧 Progress-Tracking & Callbacks
- 🚧 Timeout & Cancellation Support
- 🚧 Unit Tests (8 Tests)

## 📦 Arbeitspakete

### 1. Worker-Thread-Pool (1.5 Tage)

**Zu implementieren:**

#### Initialisierung
- [ ] Constructor: `BatchEvaluator(judge, config)`
  - [ ] Speichere shared_ptr zu RAGJudge
  - [ ] Parse config (num_workers, batch_size, etc.)
  - [ ] Starte Worker-Threads
  - [ ] Initialisiere Queue und Synchronization
- [ ] Destructor: `~BatchEvaluator()`
  - [ ] Setze stop_requested_ = true
  - [ ] Notify all worker threads
  - [ ] Join alle threads
  - [ ] Clear queue

#### Worker-Thread-Funktion
- [ ] `workerThread()` - Main worker loop
  - [ ] Warte auf Queue-Eintrag (condition_variable)
  - [ ] Dequeue QueuedEvaluation
  - [ ] Call `processEvaluation(input)`
  - [ ] Set promise value mit result
  - [ ] Call callback (falls vorhanden)
  - [ ] Update statistics (total_processed_++)
  - [ ] Handle exceptions (try-catch, set exception in promise)

**Data Structures:**
```cpp
std::vector<std::thread> workers_;
std::queue<QueuedEvaluation> eval_queue_;
std::mutex queue_mutex_;
std::condition_variable queue_cv_;
std::atomic<bool> stop_requested_;
std::atomic<bool> paused_;
```

**Acceptance Criteria:**
- Thread-Pool startet korrekt
- Workers warten idle wenn Queue leer
- Clean shutdown ohne deadlocks

---

### 2. Synchronous Batch-Evaluation (1 Tag)

**Zu implementieren:**
- [ ] `evaluateBatch(test_cases)` - Synchronous batch processing
  - [ ] Konvertiere RAGTestCase → EvaluationInput
  - [ ] Teile in Batches auf (batch_size)
  - [ ] Verteile Batches auf Worker-Threads
  - [ ] Warte auf alle Completions (futures.wait())
  - [ ] Sammle Results
  - [ ] Berechne Statistics (aggregateResults)
  - [ ] Return BatchEvaluationResult
- [ ] `evaluateBatch(inputs)` - Direkt mit EvaluationInputs
  - [ ] Ähnlich zu test_cases-Version
  - [ ] Keine Konvertierung nötig
- [ ] `aggregateResults()` - Calculate batch statistics
  - [ ] Average scores per dimension
  - [ ] Pass/fail counts
  - [ ] Total time, progress
  - [ ] Return BatchEvaluationResult

**Acceptance Criteria:**
- Batch-Processing funktioniert mit N workers
- Results korrekt aggregiert
- Progress-Tracking funktioniert
- Fail-Fast-Mode optional

**Progress-Tracking:**
```cpp
if (config_.enable_progress_tracking && config_.progress_callback) {
    BatchProgress progress;
    progress.completed_items = completed;
    progress.total_items = total;
    progress.progress_percentage = (completed / total) * 100;
    config_.progress_callback(progress);
}
```

---

### 3. Asynchronous Evaluation (1 Tag)

**Zu implementieren:**

#### Single Async
- [ ] `evaluateAsync(input)` - Return AsyncEvaluationHandle
  - [ ] Create QueuedEvaluation mit promise
  - [ ] Enqueue in eval_queue_
  - [ ] Notify one worker (queue_cv_.notify_one())
  - [ ] Create AsyncEvaluationHandle mit future
  - [ ] Return handle

#### Batch Async
- [ ] `evaluateAsync(inputs)` - Vector of handles
  - [ ] Loop über inputs, call single evaluateAsync()
  - [ ] Sammle handles in vector
  - [ ] Return vector

#### AsyncEvaluationHandle
- [ ] `isDone()` - Check future status
  - [ ] `future_.wait_for(0ms) == ready`
- [ ] `wait(timeout)` - Block bis done oder timeout
  - [ ] `future_.wait_for(timeout)`
  - [ ] Return true if completed
- [ ] `get()` - Blocking get result
  - [ ] `future_.get()` - throws if error
  - [ ] Check if cancelled
- [ ] `cancel()` - Cancel evaluation
  - [ ] Set cancelled_ = true
  - [ ] Future: Attempt to remove from queue (if not started)

**Acceptance Criteria:**
- Async-API non-blocking
- Futures funktionieren korrekt
- Exception-Propagation zu get()
- Cancellation unterstützt

---

### 4. Queue-Management & Backpressure (0.5 Tage)

**Zu implementieren:**
- [ ] `submit(input, callback)` - Non-blocking submit
  - [ ] Enqueue QueuedEvaluation mit callback
  - [ ] Notify worker
- [ ] `getQueueSize()` - Current pending count
  - [ ] Return eval_queue_.size() (with lock)
- [ ] `waitForAll(timeout)` - Wait for queue empty
  - [ ] Loop: check queue empty und alle workers idle
  - [ ] Sleep kurz zwischen checks
  - [ ] Timeout after specified duration
- [ ] Backpressure-Handling (optional)
  - [ ] Max-Queue-Size-Limit
  - [ ] Block submit() wenn Queue voll

**Acceptance Criteria:**
- Queue-Size korrekt reported
- waitForAll funktioniert
- Backpressure verhindert OOM

---

### 5. Control & Statistics (0.5 Tage)

**Zu implementieren:**

#### Control
- [ ] `stop()` - Stop processing, clear queue
  - [ ] Set stop_requested_ = true
  - [ ] Clear eval_queue_
  - [ ] Notify all workers
- [ ] `resume()` - Resume after stop
  - [ ] Set stop_requested_ = false
  - [ ] Paused_ = false

#### Statistics
- [ ] Track total_processed_, total_failed_
- [ ] Calculate throughput (items/sec)
- [ ] Track latency per item
- [ ] Progress-Percentage-Berechnung
- [ ] Estimated time remaining

**Acceptance Criteria:**
- Stop/Resume funktioniert
- Statistics akkurat
- Throughput tracking funktioniert

---

### 6. Error-Handling & Timeout (0.5 Tage)

**Zu implementieren:**
- [ ] Timeout pro Evaluation
  - [ ] `config_.timeout_per_item`
  - [ ] future.wait_for(timeout) in worker
  - [ ] Throw timeout_error wenn exceeded
- [ ] Exception-Handling in workerThread()
  - [ ] try-catch um processEvaluation()
  - [ ] Set exception in promise: `promise.set_exception()`
  - [ ] Increment total_failed_
  - [ ] Log error details
- [ ] Fail-Fast-Mode
  - [ ] `config_.fail_fast`
  - [ ] Stop alle workers bei erstem Fehler

**Acceptance Criteria:**
- Timeouts funktionieren
- Exceptions propagiert zu future.get()
- Fail-Fast stoppt alle workers

---

### 7. Unit Tests (0.5 Tage)

**Test-Suite:** `tests/test_batch_evaluator.cpp`

**Zu implementieren (8 Tests):**
1. [ ] `TEST(BatchEvaluator, BasicBatchEvaluation)`
   - 10 Inputs, verify all evaluated
2. [ ] `TEST(BatchEvaluator, ParallelProcessing)`
   - 100 Inputs, 4 workers, verify speedup
3. [ ] `TEST(BatchEvaluator, AsyncEvaluation)`
   - evaluateAsync(), verify handle works
4. [ ] `TEST(BatchEvaluator, ProgressTracking)`
   - Callback gets called with progress updates
5. [ ] `TEST(BatchEvaluator, TimeoutHandling)`
   - Slow evaluation, verify timeout
6. [ ] `TEST(BatchEvaluator, CancellationSupport)`
   - Cancel async handle, verify not processed
7. [ ] `TEST(BatchEvaluator, QueueBackpressure)`
   - Fill queue, verify waitForAll()
8. [ ] `TEST(BatchEvaluator, StopResume)`
   - Stop processing, verify queue cleared, resume

**Acceptance Criteria:**
- Alle 8 Tests bestehen
- Thread-safe unter stress
- Performance-Test zeigt N-worker-Speedup

---

## 🔗 Abhängigkeiten

**Code:**
- `include/rag/rag_judge.h` - RAGJudge, EvaluationInput
- `<thread>`, `<future>`, `<queue>` - Async primitives
- `<mutex>`, `<condition_variable>` - Synchronization

**Voraussetzungen:**
- Phase 1-4 Judge-Implementation
- C++20 für std::jthread (oder C++11 std::thread)

---

## 📊 Erfolgskriterien

- [ ] Alle Methoden in `batch_evaluator.h` implementiert
- [ ] 8 Unit Tests bestehen
- [ ] Throughput: 100+ eval/sec mit 4 workers
- [ ] Async-Overhead < 5ms per submission
- [ ] Thread-safe unter concurrent load
- [ ] Clean shutdown ohne leaks
- [ ] Dokumentation aktualisiert
- [ ] Code Review abgeschlossen

---

## 📝 Implementation Notes

**Performance-Targets:**
- Batch-Throughput: 100+ eval/sec (4 workers, ~500ms/eval)
- Async-Submit: < 5ms (enqueue + notify)
- Queue-Overhead: < 1% of eval time
- Memory: < 100 MB für 1000 queued items

**Threading Best Practices:**
- Minimize lock-hold-time
- Use condition_variable für wait, nicht busy-wait
- Proper exception-handling in threads
- Clean thread-join in destructor
- Avoid deadlocks (consistent lock ordering)

**Memory Management:**
- Shared_ptr für Judge (thread-safe ref-counting)
- Move-semantics für EvaluationInput (avoid copies)
- Clear queue on stop (free memory)
- Monitor queue-size (warn if growing unbounded)

**Konfiguration:**
```yaml
batch:
  batch_size: 8                # Items per batch
  num_workers: 4               # Worker thread count
  enable_progress_tracking: true
  fail_fast: false             # Stop on first error
  timeout_per_item_seconds: 30
```

**Usage Example:**
```cpp
// Create batch evaluator
auto judge = RAGJudgeFactory::createBalanced();
BatchEvaluatorConfig config;
config.num_workers = 4;
BatchEvaluator batch_eval(judge, config);

// Synchronous batch
auto results = batch_eval.evaluateBatch(test_cases);
std::cout << "Average score: " << results.average_overall_score << "\n";

// Asynchronous
auto handle = batch_eval.evaluateAsync(input);
// Do other work...
auto result = handle->get(); // Block until done
```

---

## 📚 Referenzen

- [C++ Thread Pool Patterns](https://en.cppreference.com/w/cpp/thread)
- [std::future and std::promise](https://en.cppreference.com/w/cpp/thread/future)
- Phase 6 Progress: `docs/implementation-history/IMPLEMENTATION_PROGRESS_RAG_JUDGE_P5_P6.md`

---

**Labels:** `priority:P2`, `type:feature`, `area:performance`, `area:rag`, `effort:large`, `phase:6`  
**Estimated Effort:** 3-4 Tage (1 Developer)  
**Dependencies:** Phase 1-4 Complete, EvaluationCache (optional, for performance)  
**Follow-up:** Performance Benchmarking, Load Testing, Production Deployment
