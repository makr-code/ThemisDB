---
name: "🛑 LoRa Training Control Implementation"
about: Training Stop Logic und Production Features (Kritisch - P0)
title: "[LoRa] Implement Training Control (Stop, Checkpoint, Resume)"
labels: priority:P0, type:feature, area:llm, effort:medium, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: Implementierung der Training Control Funktionen: Graceful Stop, Checkpointing und Resume. Aktuell ist `stopTraining()` nicht implementiert und Training kann nicht kontrolliert werden.

**EN**: Implement training control functions: Graceful stop, checkpointing, and resume. Currently `stopTraining()` is not implemented and training cannot be controlled.

**Related Analysis**: `INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md` §1.1.1  
**Current Status**: `src/llm/lora_framework/lora_training_service.cpp:246` (stub with warning)  
**Blocker**: ❌ **PRODUKTIONSBLOCKER** - Training kann nicht sauber gestoppt werden → Ressourcen-Lecks

## 🎯 Ziele / Goals

- [ ] `stopTraining()` vollständig implementieren
- [ ] Checkpoint-Funktionalität (save/load)
- [ ] Resume Training from Checkpoint
- [ ] Graceful Shutdown bei SIGTERM/SIGINT
- [ ] Training Progress Reporting
- [ ] Resource Cleanup

## 📝 Aufgaben / Tasks

### 1. Stop Training Logic
**Priorität**: P0 - Kritisch

**Current Code** (Lines 246-248):
```cpp
void LoRATrainingService::stopTraining() {
    spdlog::warn("stopTraining() not yet implemented");
    // TODO: Implement training stop logic
}
```

**Implementation Required**:
- [ ] Add atomic `stop_flag_` member variable
- [ ] Check `stop_flag_` in training loop
- [ ] Wait for current batch to complete
- [ ] Save checkpoint before exit
- [ ] Clean up GPU/CPU resources
- [ ] Update training status

**File**: `src/llm/lora_framework/lora_training_service.cpp`  
**Function**: `stopTraining()`  
**Lines**: 246-248

**Implementation**:
```cpp
class LoRATrainingService {
private:
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> training_active_{false};
    std::mutex training_mutex_;
    
public:
    void stopTraining() {
        spdlog::info("Stop training requested");
        
        // 1. Set stop flag (thread-safe)
        stop_requested_.store(true, std::memory_order_release);
        
        // 2. Wait for training to complete current batch
        {
            std::unique_lock<std::mutex> lock(training_mutex_);
            // Wait up to 30 seconds for clean stop
            auto timeout = std::chrono::seconds(30);
            auto start = std::chrono::steady_clock::now();
            
            while (training_active_.load(std::memory_order_acquire)) {
                if (std::chrono::steady_clock::now() - start > timeout) {
                    spdlog::error("Training stop timeout, forcing shutdown");
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        // 3. Save checkpoint if training was active
        if (current_checkpoint_path_) {
            spdlog::info("Saving emergency checkpoint");
            saveCheckpoint(*current_checkpoint_path_);
        }
        
        // 4. Clean up resources
        cleanupResources();
        
        spdlog::info("Training stopped successfully");
    }
    
    // Modified training loop
    void train() {
        stop_requested_.store(false);
        training_active_.store(true);
        
        try {
            for (size_t epoch = 0; epoch < config_.num_epochs; ++epoch) {
                // Check stop flag at start of each epoch
                if (stop_requested_.load(std::memory_order_acquire)) {
                    spdlog::info("Training stopped at epoch {}", epoch);
                    break;
                }
                
                for (size_t step = 0; step < steps_per_epoch; ++step) {
                    // Check stop flag every 10 steps
                    if (step % 10 == 0 && stop_requested_.load(std::memory_order_acquire)) {
                        spdlog::info("Training stopped at step {}", step);
                        break;
                    }
                    
                    // Training step...
                }
            }
        } catch (...) {
            training_active_.store(false);
            throw;
        }
        
        training_active_.store(false);
    }
};
```

---

### 2. Checkpoint Save Implementation
**Priorität**: P0 - Kritisch

**Implementation**:
- [ ] Save LoRa weights (A and B matrices)
- [ ] Save optimizer state (momentum, adaptive rates)
- [ ] Save training state (epoch, step, loss)
- [ ] Save hyperparameters
- [ ] Save random state (for reproducibility)
- [ ] Atomic save (write to temp, then rename)

**File**: `src/llm/lora_framework/lora_training_service.cpp`  
**New Function**: `saveCheckpoint()`

**Checkpoint Format**:
```cpp
struct TrainingCheckpoint {
    // Model state
    std::map<std::string, LoRAWeights> lora_weights;  // Layer name → weights
    
    // Optimizer state
    std::map<std::string, OptimizerState> optimizer_state;
    
    // Training progress
    size_t current_epoch;
    size_t current_step;
    double current_loss;
    std::vector<double> loss_history;
    
    // Hyperparameters
    TrainingConfig config;
    
    // Random state (for reproducibility)
    std::vector<uint32_t> rng_state;
    
    // Metadata
    std::string model_id;
    std::chrono::system_clock::time_point saved_at;
    std::string version = "1.0";
};

bool LoRATrainingService::saveCheckpoint(const std::string& checkpoint_path) {
    spdlog::info("Saving checkpoint to {}", checkpoint_path);
    
    TrainingCheckpoint checkpoint;
    
    // 1. Collect LoRa weights
    for (auto& [layer_name, layer] : lora_layers_) {
        checkpoint.lora_weights[layer_name] = layer->getWeights();
    }
    
    // 2. Collect optimizer state
    checkpoint.optimizer_state = optimizer_->getState();
    
    // 3. Save training progress
    checkpoint.current_epoch = current_epoch_;
    checkpoint.current_step = current_step_;
    checkpoint.current_loss = current_loss_;
    checkpoint.loss_history = loss_history_;
    
    // 4. Save config
    checkpoint.config = config_;
    
    // 5. Save random state
    checkpoint.rng_state = rng_.getState();
    
    // 6. Save metadata
    checkpoint.model_id = model_id_;
    checkpoint.saved_at = std::chrono::system_clock::now();
    
    // 7. Serialize and save (atomic)
    std::string temp_path = checkpoint_path + ".tmp";
    try {
        // Serialize to temp file
        std::ofstream ofs(temp_path, std::ios::binary);
        cereal::BinaryOutputArchive archive(ofs);
        archive(checkpoint);
        ofs.close();
        
        // Atomic rename
        std::filesystem::rename(temp_path, checkpoint_path);
        
        spdlog::info("Checkpoint saved successfully ({} bytes)", 
                    std::filesystem::file_size(checkpoint_path));
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to save checkpoint: {}", e.what());
        std::filesystem::remove(temp_path);  // Cleanup temp file
        return false;
    }
}
```

---

### 3. Checkpoint Load Implementation
**Priorität**: P0 - Kritisch

**Implementation**:
- [ ] Load checkpoint file
- [ ] Validate checkpoint version
- [ ] Restore LoRa weights
- [ ] Restore optimizer state
- [ ] Restore training progress
- [ ] Restore random state
- [ ] Verify integrity (checksum)

**File**: `src/llm/lora_framework/lora_training_service.cpp`  
**New Function**: `loadCheckpoint()`

**Implementation**:
```cpp
bool LoRATrainingService::loadCheckpoint(const std::string& checkpoint_path) {
    spdlog::info("Loading checkpoint from {}", checkpoint_path);
    
    if (!std::filesystem::exists(checkpoint_path)) {
        spdlog::error("Checkpoint file not found: {}", checkpoint_path);
        return false;
    }
    
    try {
        // 1. Deserialize checkpoint
        std::ifstream ifs(checkpoint_path, std::ios::binary);
        cereal::BinaryInputArchive archive(ifs);
        TrainingCheckpoint checkpoint;
        archive(checkpoint);
        ifs.close();
        
        // 2. Validate version
        if (checkpoint.version != "1.0") {
            spdlog::error("Incompatible checkpoint version: {}", checkpoint.version);
            return false;
        }
        
        // 3. Restore LoRa weights
        for (auto& [layer_name, weights] : checkpoint.lora_weights) {
            if (lora_layers_.count(layer_name) == 0) {
                spdlog::error("Layer {} not found in current model", layer_name);
                return false;
            }
            lora_layers_[layer_name]->setWeights(weights);
        }
        
        // 4. Restore optimizer state
        optimizer_->setState(checkpoint.optimizer_state);
        
        // 5. Restore training progress
        current_epoch_ = checkpoint.current_epoch;
        current_step_ = checkpoint.current_step;
        current_loss_ = checkpoint.current_loss;
        loss_history_ = checkpoint.loss_history;
        
        // 6. Restore random state (for reproducibility)
        rng_.setState(checkpoint.rng_state);
        
        // 7. Verify config compatibility
        if (!isConfigCompatible(checkpoint.config, config_)) {
            spdlog::warn("Checkpoint config differs from current config");
            // Allow continuing with new config
        }
        
        spdlog::info("Checkpoint loaded: epoch {}, step {}, loss {:.4f}",
                    current_epoch_, current_step_, current_loss_);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to load checkpoint: {}", e.what());
        return false;
    }
}
```

---

### 4. Resume Training Implementation
**Priorität**: P0 - Kritisch

**Implementation**:
- [ ] Check for existing checkpoint
- [ ] Load checkpoint if found
- [ ] Continue from saved epoch/step
- [ ] Maintain loss history continuity
- [ ] Log resume information

**File**: `src/llm/lora_framework/lora_training_service.cpp`  
**Modified Function**: `train()`

**Implementation**:
```cpp
void LoRATrainingService::train() {
    // Check for resume
    if (config_.resume_from_checkpoint && !config_.checkpoint_path.empty()) {
        if (std::filesystem::exists(config_.checkpoint_path)) {
            spdlog::info("Resuming training from checkpoint");
            if (!loadCheckpoint(config_.checkpoint_path)) {
                spdlog::error("Failed to load checkpoint, starting from scratch");
                current_epoch_ = 0;
                current_step_ = 0;
            } else {
                spdlog::info("Resumed at epoch {}, step {}", 
                           current_epoch_, current_step_);
            }
        }
    }
    
    // Start training from current_epoch_, current_step_
    for (size_t epoch = current_epoch_; epoch < config_.num_epochs; ++epoch) {
        // Training loop...
    }
}
```

---

### 5. Periodic Checkpointing
**Priorität**: P1 - Hoch

**Implementation**:
- [ ] Save checkpoint every N steps
- [ ] Save checkpoint at end of each epoch
- [ ] Keep last K checkpoints (rotation)
- [ ] Save best checkpoint (lowest loss)

**Configuration**:
```cpp
struct CheckpointConfig {
    bool enabled = true;
    size_t save_every_n_steps = 100;  // Save every 100 steps
    size_t keep_last_k = 3;            // Keep last 3 checkpoints
    bool save_best = true;             // Save best model separately
    std::string checkpoint_dir = "./checkpoints";
};
```

**Implementation**:
```cpp
void LoRATrainingService::trainStep() {
    // ... training step ...
    
    // Check if checkpoint needed
    if (config_.checkpoint_config.enabled) {
        if (current_step_ % config_.checkpoint_config.save_every_n_steps == 0) {
            std::string checkpoint_path = fmt::format(
                "{}/checkpoint_epoch{}_step{}.ckpt",
                config_.checkpoint_config.checkpoint_dir,
                current_epoch_,
                current_step_
            );
            saveCheckpoint(checkpoint_path);
            
            // Rotate old checkpoints
            rotateCheckpoints();
        }
        
        // Save best checkpoint
        if (config_.checkpoint_config.save_best && current_loss_ < best_loss_) {
            best_loss_ = current_loss_;
            std::string best_path = config_.checkpoint_config.checkpoint_dir + "/best.ckpt";
            saveCheckpoint(best_path);
        }
    }
}
```

---

### 6. Signal Handlers (SIGTERM/SIGINT)
**Priorität**: P1 - Hoch

**Implementation**:
- [ ] Register signal handlers
- [ ] Graceful shutdown on SIGTERM
- [ ] Emergency checkpoint on SIGINT
- [ ] Resource cleanup

**File**: `src/llm/lora_framework/lora_training_service.cpp`

**Implementation**:
```cpp
namespace {
    LoRATrainingService* g_training_service = nullptr;
    
    void signalHandler(int signal) {
        if (g_training_service) {
            spdlog::warn("Received signal {}, stopping training", signal);
            g_training_service->stopTraining();
        }
    }
}

void LoRATrainingService::train() {
    // Register signal handlers
    g_training_service = this;
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);
    
    try {
        // Training loop...
    } catch (...) {
        g_training_service = nullptr;
        throw;
    }
    
    g_training_service = nullptr;
}
```

---

### 7. Resource Cleanup
**Priorität**: P0 - Kritisch

**Implementation**:
- [ ] Release GPU memory
- [ ] Close file handles
- [ ] Clear caches
- [ ] Reset state variables

**File**: `src/llm/lora_framework/lora_training_service.cpp`  
**New Function**: `cleanupResources()`

**Implementation**:
```cpp
void LoRATrainingService::cleanupResources() {
    spdlog::info("Cleaning up training resources");
    
    // 1. Release GPU memory
    if (gpu_memory_manager_) {
        gpu_memory_manager_->releaseAll();
    }
    
    // 2. Clear caches
    lora_layers_.clear();
    
    // 3. Close file handles
    if (log_file_) {
        log_file_->close();
    }
    
    // 4. Reset state
    current_epoch_ = 0;
    current_step_ = 0;
    current_loss_ = 0.0;
    
    spdlog::info("Resource cleanup complete");
}
```

---

### 8. Testing
**Priorität**: P0 - Kritisch

- [ ] Unit test for stopTraining()
- [ ] Test checkpoint save/load roundtrip
- [ ] Test resume from checkpoint
- [ ] Test signal handlers
- [ ] Test resource cleanup
- [ ] Integration test: train → stop → resume

**Test File**: `tests/test_lora_training_control.cpp`

**Test Cases**:
```cpp
TEST(LoRATrainingControl, StopTraining_Graceful) {
    // 1. Start training in background thread
    // 2. Call stopTraining() after 100 steps
    // 3. Verify training stops within 30 seconds
    // 4. Verify checkpoint saved
}

TEST(LoRATrainingControl, Checkpoint_SaveLoad) {
    // 1. Train for 100 steps
    // 2. Save checkpoint
    // 3. Create new service
    // 4. Load checkpoint
    // 5. Verify state matches
}

TEST(LoRATrainingControl, Resume_FromCheckpoint) {
    // 1. Train to step 100, save checkpoint
    // 2. Create new service, resume from checkpoint
    // 3. Verify continues from step 100
    // 4. Verify loss history preserved
}
```

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

### Functional Requirements
- [ ] `stopTraining()` stops training within 30 seconds
- [ ] Checkpoint save/load works correctly
- [ ] Resume continues from saved state
- [ ] Signal handlers work (SIGTERM/SIGINT)
- [ ] No resource leaks after stop

### Non-Functional Requirements
- [ ] Thread-safe stop operation
- [ ] Checkpoint save time <5 seconds
- [ ] All tests passing
- [ ] No memory leaks (valgrind clean)

### Production Readiness
- [ ] Graceful shutdown in production
- [ ] Emergency checkpoints on crashes
- [ ] Monitoring metrics for training state
- [ ] Documentation complete

---

## 📊 Aufwand / Effort

**Geschätzte Zeit**: 1 Woche (5 Arbeitstage)

**Breakdown**:
- Stop Training Logic: 1 Tag
- Checkpoint Save/Load: 2 Tage
- Resume Implementation: 1 Tag
- Signal Handlers & Cleanup: 0.5 Tage
- Testing & Validation: 1.5 Tage

**Complexity**: Mittel - Thread Safety, Atomicity

---

## 🏁 Definition of Done

- [ ] All tasks completed
- [ ] All tests passing
- [ ] Code review approved
- [ ] No resource leaks
- [ ] Documentation updated
- [ ] Production deployment verified

---

**Priority**: 🔴 **P0 - CRITICAL PRODUCTION BLOCKER**  
**Impact**: Enables graceful training control  
**Timeline**: 1 week  
**Dependencies**: None

---

**Erstellt**: 15. Januar 2026  
**Status**: 🚧 Ready for Implementation
