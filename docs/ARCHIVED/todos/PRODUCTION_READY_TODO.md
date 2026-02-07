# Production-Ready LLM/LoRA Integration - Detaillierter Umsetzungsplan

**Erstellt**: 15. Januar 2026  
**Version**: 1.0  
**Ziel**: Integration eines production-ready LLM/LoRA-Systems (llama.cpp) in ThemisDB  
**Basis**: Vorhandene Code-Struktur in `/src/llm` und `/include/llm`  
**Gesamtaufwand**: 6-12 Monate (2-3 Vollzeit-Entwickler)

---

## Überblick

Dieser Plan beschreibt die notwendigen Schritte zur Integration eines produktionsreifen LLM/LoRA-Systems in ThemisDB, basierend auf der bestehenden Architektur. Die aktuelle Implementation ist zu 20-40% vollständig und enthält hauptsächlich Stubs und Simulationen.

---

## Phase 1: Kritische Blocker beheben (3-4 Monate)

### 1.1 llama.cpp Integration vollständig implementieren

**Priorität**: ⛔ KRITISCH - BLOCKER  
**Dateien**: `src/llm/llama_wrapper.cpp`, `src/llm/llamacpp_inference_engine.cpp`  
**Aufwand**: 2-3 Monate  
**Verantwortlich**: Senior ML Engineer

#### Aufgaben

- [ ] **1.1.1 llama.cpp Bibliothek vollständig einbinden**
  ```cpp
  // Zu implementieren in llama_wrapper.cpp
  - llama_backend_init()
  - llama_model_params mit korrekten Defaults
  - llama_context_params mit GPU-Support
  ```
  - Prüfen: Welche llama.cpp Version wird verwendet?
  - CMake-Integration überprüfen und anpassen
  - Compiler-Flags für GPU-Support aktivieren
  - **Aufwand**: 1 Woche

- [ ] **1.1.2 Modell-Laden implementieren**
  ```cpp
  bool LlamaWrapper::loadModel(const std::string& model_path) {
      // AKTUELL: Gibt nullptr zurück und logged Warning
      // NEU: Echtes Laden mit llama_model_load_from_file()
      
      impl_->model_handle = llama_model_load_from_file(
          model_path.c_str(), 
          model_params
      );
      
      if (!impl_->model_handle) {
          throw std::runtime_error("Model loading failed");
      }
      
      // Context erstellen
      impl_->context_handle = llama_new_context_with_model(
          impl_->model_handle,
          context_params
      );
      
      return impl_->context_handle != nullptr;
  }
  ```
  - Fehlerbehandlung für fehlende Modelle
  - Fortschrittsanzeige während des Ladens (große Modelle!)
  - Speicherprüfung vor dem Laden
  - **Aufwand**: 1 Woche

- [ ] **1.1.3 Token-Generierung implementieren**
  ```cpp
  std::string LlamaWrapper::generate(const GenerationRequest& request) {
      // AKTUELL: Gibt "[Generated response placeholder...]" zurück
      // NEU: Echte Token-by-Token Generierung
      
      // 1. Prompt tokenisieren
      auto tokens = llama_tokenize(context, request.prompt, true);
      
      // 2. Batch erstellen und evaluieren
      llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
      llama_decode(context, batch);
      
      // 3. Token-Loop für Generierung
      std::string output;
      for (int i = 0; i < request.max_tokens; ++i) {
          // Sampling mit Parametern
          llama_token new_token = llama_sampler_sample(sampler, context, -1);
          
          if (llama_token_is_eog(model, new_token)) break;
          
          output += llama_token_to_piece(context, new_token);
          
          // Nächster Decode-Step
          llama_batch next_batch = llama_batch_get_one(&new_token, 1);
          llama_decode(context, next_batch);
      }
      
      return output;
  }
  ```
  - Sampling-Parameter implementieren (temperature, top_k, top_p)
  - Stopping-Kriterien (max_tokens, EOS, stop_sequences)
  - Streaming-Support für lange Antworten
  - **Aufwand**: 2 Wochen

- [ ] **1.1.4 LoRA-Adapter-Loading in llama.cpp**
  ```cpp
  bool LlamaWrapper::loadLoRAAdapter(const std::string& adapter_path) {
      // AKTUELL: Nicht implementiert, llama_lora_adapter_free() auskommentiert!
      // NEU: Echtes LoRA-Adapter-Loading
      
      llama_lora_adapter* adapter = llama_lora_adapter_init(
          impl_->model_handle,
          adapter_path.c_str()
      );
      
      if (!adapter) {
          throw std::runtime_error("LoRA adapter loading failed");
      }
      
      // Adapter auf Kontext anwenden
      llama_lora_adapter_set(
          impl_->context_handle,
          adapter,
          1.0f  // scale
      );
      
      // Adapter-Handle speichern für spätere Freigabe
      impl_->active_adapters.push_back(adapter);
      
      return true;
  }
  ```
  - Multi-LoRA-Support (mehrere Adapter gleichzeitig)
  - Adapter-Scaling-Parameter
  - Adapter-Unload und Memory-Cleanup
  - **Aufwand**: 2 Wochen

- [ ] **1.1.5 GPU-Memory-Management**
  - VRAM-Nutzung überwachen
  - Layer-Offloading konfigurierbar machen (`n_gpu_layers`)
  - OOM-Fehler abfangen und behandeln
  - **Aufwand**: 1 Woche

- [ ] **1.1.6 Batch-Processing implementieren**
  ```cpp
  // llamacpp_inference_engine.cpp
  std::vector<std::string> processBatch(
      const std::vector<std::string>& prompts) {
      // Continuous Batching für höheren Durchsatz
      // Multi-Sequence-Support in llama.cpp nutzen
  }
  ```
  - **Aufwand**: 2 Wochen

- [ ] **1.1.7 Resource-Cleanup korrigieren**
  ```cpp
  // lora_adapter_manager.cpp:150
  // AKTUELL: Auskommentiert! Memory Leak!
  if (entry->adapter_handle) {
      llama_lora_adapter_free(entry->adapter_handle);  // EINKOMMENTIEREN!
      entry->adapter_handle = nullptr;
  }
  
  // Destruktor implementieren
  ~LlamaWrapper() {
      for (auto* adapter : impl_->active_adapters) {
          llama_lora_adapter_free(adapter);
      }
      if (impl_->context_handle) {
          llama_free(impl_->context_handle);
      }
      if (impl_->model_handle) {
          llama_free_model(impl_->model_handle);
      }
      llama_backend_free();
  }
  ```
  - RAII-Wrapper für llama-Ressourcen
  - Smart Pointers mit Custom Deleters
  - **Aufwand**: 3 Tage

- [ ] **1.1.8 Integration-Tests mit echten Modellen**
  - Testmodell herunterladen (z.B. TinyLlama 1.1B)
  - End-to-End-Test: Modell laden → Prompt → Response
  - Performance-Baseline messen
  - **Aufwand**: 1 Woche

**Meilenstein 1.1**: llama.cpp vollständig funktional, echte Inferenz möglich

---

### 1.2 LoRA-Training-System implementieren

**Priorität**: ⛔ KRITISCH - BLOCKER  
**Dateien**: `src/llm/lora_framework/lora_training_service.cpp`, `include/llm/llamacpp_training_backend.h`  
**Aufwand**: 3-4 Monate  
**Verantwortlich**: ML Training Engineer

#### Aufgaben

- [ ] **1.2.1 Technologie-Entscheidung treffen**
  
  **Option A: llama.cpp native Training (falls verfügbar)**
  - Prüfen: Hat llama.cpp LoRA-Training-Support?
  - Wenn ja: Direkt nutzen (bevorzugt, da konsistent)
  
  **Option B: PyTorch C++ API (libtorch)**
  - PEFT/LoRA aus HuggingFace Transformers portieren
  - C++ Bindings erstellen
  - Integration aufwändiger aber flexibler
  
  **Option C: Custom CUDA Kernels**
  - Nur für Performance-kritische Komponenten
  - Hohe Komplexität, schwer zu warten
  
  **Empfehlung**: Option A (llama.cpp) oder Option B (libtorch)  
  **Aufwand**: 1 Woche (Evaluation + Entscheidung)

- [ ] **1.2.2 Training-Loop implementieren (Basis)**
  ```cpp
  TrainingResult LoRATrainingService::trainOnTheFly(
      const std::string& base_model_id,
      const std::vector<TrainingData>& data,
      const LoRAConfig& config) {
      
      // AKTUELL: std::this_thread::sleep_for(std::chrono::milliseconds(10));
      // NEU: Echtes Training
      
      // 1. Base Model laden
      auto model = loadBaseModel(base_model_id);
      
      // 2. LoRA-Layer initialisieren
      auto lora_layers = initializeLoRALayers(model, config.rank);
      
      // 3. Optimizer erstellen
      auto optimizer = createOptimizer(lora_layers, config.learning_rate);
      
      // 4. Training Loop
      for (size_t epoch = 0; epoch < config.epochs; ++epoch) {
          float epoch_loss = 0.0f;
          
          for (const auto& batch : createBatches(data, config.batch_size)) {
              // Forward Pass
              auto logits = model->forward(batch.inputs, lora_layers);
              
              // Loss berechnen
              float loss = computeLoss(logits, batch.targets);
              epoch_loss += loss;
              
              // Backward Pass (Gradienten)
              auto gradients = computeGradients(loss, lora_layers);
              
              // Optimizer Step
              optimizer->step(gradients);
              
              // Metriken aktualisieren
              updateMetrics(epoch, loss);
              
              // Callback für Progress
              if (progress_callback_) {
                  progress_callback_(getCurrentProgress());
              }
          }
          
          spdlog::info("Epoch {}: Loss = {}", epoch, epoch_loss / data.size());
      }
      
      // 5. Trainierte LoRA-Weights extrahieren
      auto trained_adapter = extractLoRAWeights(lora_layers);
      
      // 6. Adapter speichern
      storage_service_->saveAdapter(base_model_id + "_finetuned", trained_adapter);
      
      return TrainingResult{/* metrics */};
  }
  ```
  - **Aufwand**: 4-6 Wochen

- [ ] **1.2.3 LoRA-Layer-Implementierung**
  ```cpp
  // Low-Rank Decomposition: W' = W + BA
  // B: (d, r), A: (r, k)
  // Rank r << min(d, k)
  
  class LoRALayer {
  public:
      LoRALayer(size_t in_dim, size_t out_dim, size_t rank);
      
      Tensor forward(const Tensor& input) {
          // Original: out = input @ W
          // Mit LoRA: out = input @ W + input @ (B @ A) * scaling
          auto lora_out = input.matmul(B_).matmul(A_) * scaling_;
          return original_output + lora_out;
      }
      
      std::pair<Tensor, Tensor> getWeights() {
          return {B_, A_};
      }
  
  private:
      Tensor B_;  // (in_dim, rank)
      Tensor A_;  // (rank, out_dim)
      float scaling_;
  };
  ```
  - LoRA für Attention-Weights (Q, K, V, O)
  - LoRA für Feed-Forward-Weights
  - Initialisierung (Kaiming/Xavier)
  - **Aufwand**: 2 Wochen

- [ ] **1.2.4 Optimizer implementieren**
  ```cpp
  class AdamOptimizer {
  public:
      void step(const std::vector<Gradient>& gradients) {
          for (size_t i = 0; i < parameters_.size(); ++i) {
              // Adam: m_t = β1*m_{t-1} + (1-β1)*g_t
              //       v_t = β2*v_{t-1} + (1-β2)*g_t^2
              //       θ_t = θ_{t-1} - α * m_t / (sqrt(v_t) + ε)
              
              m_[i] = beta1_ * m_[i] + (1 - beta1_) * gradients[i];
              v_[i] = beta2_ * v_[i] + (1 - beta2_) * gradients[i].pow(2);
              
              auto m_hat = m_[i] / (1 - std::pow(beta1_, step_));
              auto v_hat = v_[i] / (1 - std::pow(beta2_, step_));
              
              parameters_[i] -= learning_rate_ * m_hat / (v_hat.sqrt() + epsilon_);
          }
          step_++;
      }
  };
  ```
  - AdamW (Adam mit Weight Decay)
  - Learning Rate Scheduling
  - Gradient Clipping
  - **Aufwand**: 1 Woche

- [ ] **1.2.5 Loss-Funktionen implementieren**
  ```cpp
  float computeCrossEntropyLoss(const Tensor& logits, const Tensor& targets) {
      // Cross-Entropy für Language Modeling
      // L = -1/N * Σ log(p(y_i | x_i))
      auto log_probs = logSoftmax(logits);
      return -log_probs.gather(targets).mean();
  }
  ```
  - Cross-Entropy Loss
  - Perplexity-Metrik
  - **Aufwand**: 3 Tage

- [ ] **1.2.6 Data Pipeline implementieren**
  ```cpp
  class TrainingDataLoader {
  public:
      std::vector<Batch> createBatches(
          const std::vector<TrainingData>& data,
          size_t batch_size) {
          
          // Tokenisierung
          auto tokenized = tokenizeDataset(data);
          
          // Padding/Truncation
          auto padded = padSequences(tokenized, max_seq_length_);
          
          // Batching
          return createBatchesFromTokens(padded, batch_size);
      }
  };
  ```
  - Tokenisierung mit llama.cpp tokenizer
  - Dynamic Batching (Sequenzen ähnlicher Länge)
  - Data Augmentation (optional)
  - **Aufwand**: 1 Woche

- [ ] **1.2.7 GPU-Training-Support**
  - CUDA-Memory-Management für Training
  - Mixed Precision Training (FP16)
  - Gradient Checkpointing (Memory-Einsparung)
  - **Aufwand**: 2 Wochen

- [ ] **1.2.8 Training-Checkpoint-System**
  ```cpp
  void LoRATrainingService::saveCheckpoint(
      const std::string& checkpoint_path,
      size_t epoch,
      const TrainingState& state) {
      
      // LoRA-Weights speichern
      // Optimizer-State speichern
      // Training-Metriken speichern
      // Ermöglicht Resume nach Crash
  }
  ```
  - **Aufwand**: 1 Woche

- [ ] **1.2.9 Evaluation während Training**
  - Validation Loss berechnen
  - Early Stopping implementieren
  - Best Model Tracking
  - **Aufwand**: 1 Woche

- [ ] **1.2.10 Training-Tests**
  - Overfit-Test (kleiner Datensatz)
  - Performance-Test (Throughput messen)
  - Multi-GPU-Test (falls unterstützt)
  - **Aufwand**: 2 Wochen

**Meilenstein 1.2**: LoRA-Training funktional, echtes Fine-Tuning möglich

---

### 1.3 Security: Kryptographische Validierung

**Priorität**: ⛔ KRITISCH - BLOCKER  
**Dateien**: `src/llm/lora_security_validator.cpp`  
**Aufwand**: 2-3 Wochen  
**Verantwortlich**: Security Engineer

#### Aufgaben

- [ ] **1.3.1 OpenSSL Integration**
  ```cpp
  // CMakeLists.txt
  find_package(OpenSSL REQUIRED)
  target_link_libraries(themis_llm OpenSSL::SSL OpenSSL::Crypto)
  ```
  - **Aufwand**: 1 Tag

- [ ] **1.3.2 RSA Signatur-Verifikation implementieren**
  ```cpp
  ValidationResult validateSignature(const std::string& lora_file_path) {
      // AKTUELL: Nur Format-Validierung
      // NEU: Kryptographische Verifikation
      
      // 1. Datei-Hash berechnen
      unsigned char file_hash[SHA256_DIGEST_LENGTH];
      SHA256_File(lora_file_path.c_str(), file_hash);
      
      // 2. Signatur aus Datei lesen
      auto signature = readSignatureFromFile(lora_file_path);
      
      // 3. Öffentlichen Schlüssel des Signers laden
      auto public_key = loadPublicKey(signature.signer_cert_fingerprint);
      
      // 4. Signatur verifizieren
      EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(public_key, nullptr);
      EVP_PKEY_verify_init(ctx);
      EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);
      EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256());
      
      int verify_result = EVP_PKEY_verify(
          ctx,
          signature.data.data(),
          signature.data.size(),
          file_hash,
          SHA256_DIGEST_LENGTH
      );
      
      EVP_PKEY_CTX_free(ctx);
      
      ValidationResult result;
      result.is_valid = (verify_result == 1);
      result.signature_algorithm = "RSA-SHA256";
      
      if (!result.is_valid) {
          result.error_message = "Signature verification failed";
          spdlog::error("Invalid signature for {}", lora_file_path);
      }
      
      return result;
  }
  ```
  - **Aufwand**: 3-4 Tage

- [ ] **1.3.3 Zertifikatsketten-Validierung**
  ```cpp
  bool validateCertificateChain(const X509* cert) {
      // Root CA prüfen
      // Intermediate CAs prüfen
      // Zertifikat nicht abgelaufen?
      // Zertifikat nicht widerrufen (CRL)?
  }
  ```
  - **Aufwand**: 1 Woche

- [ ] **1.3.4 Weight-Anomalie-Detektion**
  ```cpp
  std::vector<WeightAnomaly> detectWeightAnomalies(
      const std::vector<float>& weights) {
      
      // AKTUELL: return {}; (leer)
      // NEU: Statistische Analyse
      
      std::vector<WeightAnomaly> anomalies;
      
      // 1. Deskriptive Statistik
      float mean = computeMean(weights);
      float stddev = computeStdDev(weights, mean);
      
      // 2. Outlier Detection (Z-Score)
      for (size_t i = 0; i < weights.size(); ++i) {
          float z_score = (weights[i] - mean) / stddev;
          if (std::abs(z_score) > 3.0f) {
              anomalies.push_back({
                  .weight_index = i,
                  .value = weights[i],
                  .z_score = z_score,
                  .anomaly_type = "outlier"
              });
          }
      }
      
      // 3. Verteilungsprüfung (Kolmogorov-Smirnov-Test)
      if (!isNormalDistributed(weights)) {
          anomalies.push_back({
              .anomaly_type = "non_normal_distribution"
          });
      }
      
      // 4. NaN/Inf-Prüfung
      for (size_t i = 0; i < weights.size(); ++i) {
          if (std::isnan(weights[i]) || std::isinf(weights[i])) {
              anomalies.push_back({
                  .weight_index = i,
                  .value = weights[i],
                  .anomaly_type = "invalid_value"
              });
          }
      }
      
      return anomalies;
  }
  ```
  - **Aufwand**: 1 Woche

- [ ] **1.3.5 Prompt-Injection-Filter populieren**
  ```cpp
  void initializePatterns() {
      // AKTUELL: Methode existiert, aber Patterns leer
      // NEU: Patterns laden
      
      filter_patterns_ = {
          // System Prompt Overrides
          R"(ignore (previous|all) instructions?)",
          R"(you are now)",
          R"(new instructions:)",
          
          // Jailbreak Attempts
          R"(DAN mode)",
          R"(developer mode)",
          R"(sudo mode)",
          
          // Data Exfiltration
          R"(print (system|config|password))",
          R"(show me your (instructions|rules))",
          
          // Code Injection
          R"(<script>.*</script>)",
          R"(\$\{.*\})",  // Template injection
      };
      
      // Patterns aus Konfiguration laden (erweiterbar)
      loadPatternsFromConfig();
  }
  ```
  - **Aufwand**: 2-3 Tage

- [ ] **1.3.6 Security-Audit-Logging**
  ```cpp
  void logSecurityEvent(const SecurityEvent& event) {
      // Alle Security-relevanten Events loggen
      // Separate Log-Datei für Security-Audit
      // Format: Timestamp | Event Type | Severity | Details
      
      audit_logger_->log(
          spdlog::level::warn,
          "SECURITY: {} | Adapter: {} | Details: {}",
          event.type,
          event.adapter_id,
          event.details
      );
  }
  ```
  - **Aufwand**: 2 Tage

**Meilenstein 1.3**: Security validiert, Produktion-sicher

---

### 1.4 Orchestrator: Bugs fixen

**Priorität**: ⛔ KRITISCH - BUG  
**Dateien**: `src/llm/lora_framework/lora_orchestrator.cpp`  
**Aufwand**: 1-2 Wochen

#### Aufgaben

- [ ] **1.4.1 Method Signature Mismatch beheben**
  ```cpp
  // AKTUELL (lora_orchestrator.cpp:111):
  auto result = impl_->training_service->trainBatch(training_data.samples);
  // ERROR: training_data ist vector, hat kein .samples member
  
  // FIX:
  auto result = impl_->training_service->trainBatch(
      adapter_id,
      training_data,  // Ganze Liste übergeben
      config
  );
  ```
  - **Aufwand**: 1 Tag

- [ ] **1.4.2 Job Queue implementieren**
  ```cpp
  class JobQueue {
  public:
      std::string enqueue(Job job) {
          std::string job_id = generateJobId();
          
          std::unique_lock lock(mutex_);
          jobs_[job_id] = {
              .id = job_id,
              .status = JobStatus::Pending,
              .progress = 0.0f,
              .created_at = std::chrono::system_clock::now()
          };
          
          pending_jobs_.push(std::move(job));
          cv_.notify_one();
          
          return job_id;
      }
      
      std::optional<JobInfo> getJobInfo(const std::string& job_id) {
          std::shared_lock lock(mutex_);
          auto it = jobs_.find(job_id);
          if (it == jobs_.end()) return std::nullopt;
          return it->second;
      }
  
  private:
      std::unordered_map<std::string, JobInfo> jobs_;
      std::queue<Job> pending_jobs_;
      std::shared_mutex mutex_;
      std::condition_variable_any cv_;
  };
  ```
  - **Aufwand**: 3-4 Tage

- [ ] **1.4.3 Async Job Execution**
  ```cpp
  class JobExecutor {
  public:
      void start() {
          for (size_t i = 0; i < config_.max_concurrent_jobs; ++i) {
              worker_threads_.emplace_back([this]() { workerLoop(); });
          }
      }
      
      void workerLoop() {
          while (!shutdown_) {
              auto job = job_queue_->dequeue();  // Blocks bis Job verfügbar
              
              try {
                  executeJob(job);
                  job_queue_->markCompleted(job.id);
              } catch (const std::exception& e) {
                  job_queue_->markFailed(job.id, e.what());
              }
          }
      }
  
  private:
      std::vector<std::thread> worker_threads_;
  };
  ```
  - **Aufwand**: 1 Woche

- [ ] **1.4.4 Progress Callbacks verdrahten**
  ```cpp
  void LoRAOrchestrator::createAdapter(..., ProgressCallback callback) {
      auto job_id = job_queue_->enqueue({
          .type = JobType::Training,
          .callback = callback  // Speichern
      });
      
      // Im Training-Loop:
      training_service_->trainOnTheFly(..., [this, job_id](float progress) {
          job_queue_->updateProgress(job_id, progress);
          
          // User-Callback aufrufen
          if (auto job = job_queue_->getJob(job_id)) {
              if (job->callback) {
                  job->callback(progress);
              }
          }
      });
  }
  ```
  - **Aufwand**: 2 Tage

- [ ] **1.4.5 Resource Quota Enforcement**
  ```cpp
  bool JobExecutor::canAcceptJob(const Job& job) {
      // max_concurrent_training_jobs prüfen
      if (active_training_jobs_.size() >= config_.max_concurrent_training_jobs) {
          return false;
      }
      
      // GPU Memory prüfen
      if (job.requires_gpu) {
          auto available_vram = queryAvailableVRAM();
          if (available_vram < job.estimated_vram) {
              return false;
          }
      }
      
      return true;
  }
  ```
  - **Aufwand**: 3 Tage

**Meilenstein 1.4**: Orchestrator funktional und robust

---

## Phase 2: Storage & Infrastructure (1-2 Monate)

### 2.1 ThemisDB Storage Backend vollständig implementieren

**Priorität**: 🔴 HOCH  
**Dateien**: `src/llm/lora_framework/lora_storage_service_themisdb.cpp`  
**Aufwand**: 3-4 Wochen

#### Aufgaben

- [ ] **2.1.1 BLOB Storage für Adapter-Weights**
  ```cpp
  bool LoRAStorageService::saveAdapter(
      const std::string& adapter_id,
      const LoRAAdapter& adapter) {
      
      if (config_.storage_backend == "themisdb") {
          // AKTUELL: TODO
          // NEU: ThemisDB Integration
          
          // 1. Weights serialisieren
          auto weights_blob = serializeWeights(adapter.weights);
          
          // 2. In ThemisDB BLOB-Spalte speichern
          auto query = "INSERT INTO lora_adapters "
                      "(adapter_id, weights_blob, metadata) "
                      "VALUES (?, ?, ?)";
          
          db_->execute(query, adapter_id, weights_blob, 
                      adapter.metadata.toJson());
          
          return true;
      }
      // ... FileSystem/S3 Code bleibt
  }
  ```
  - Schema-Definition für `lora_adapters` Tabelle
  - BLOB-Column für Weights
  - JSON-Column für Metadata
  - **Aufwand**: 1 Woche

- [ ] **2.1.2 Versionierung mit Collision Detection**
  ```cpp
  std::string saveAdapterVersion(
      const std::string& adapter_id,
      const LoRAAdapter& adapter) {
      
      // Transaktions-Start
      auto tx = db_->beginTransaction();
      
      try {
          // Nächste Version ermitteln (atomar)
          auto version = db_->query(
              "SELECT COALESCE(MAX(version), 0) + 1 "
              "FROM lora_adapter_versions "
              "WHERE adapter_id = ?",
              adapter_id
          ).getSingleValue<int>();
          
          // Version einfügen
          db_->execute(
              "INSERT INTO lora_adapter_versions "
              "(adapter_id, version, weights_blob, created_at) "
              "VALUES (?, ?, ?, NOW())",
              adapter_id, version, serializeWeights(adapter.weights)
          );
          
          tx.commit();
          return std::to_string(version);
          
      } catch (const std::exception& e) {
          tx.rollback();
          throw;
      }
  }
  ```
  - **Aufwand**: 3-4 Tage

- [ ] **2.1.3 Concurrent Access mit Locking**
  ```cpp
  class LoRAStorageService {
      // Row-Level-Locks für Adapter-Updates
      bool lockAdapter(const std::string& adapter_id) {
          return db_->execute(
              "SELECT * FROM lora_adapters "
              "WHERE adapter_id = ? FOR UPDATE",
              adapter_id
          );
      }
  };
  ```
  - **Aufwand**: 2-3 Tage

- [ ] **2.1.4 Duplicate Storage Service Consolidation**
  - `lora_storage_service.cpp` und `lora_storage_service_themisdb.cpp` zusammenführen
  - Strategy Pattern für Backends:
    ```cpp
    class IStorageBackend {
    public:
        virtual bool save(const std::string& id, const LoRAAdapter& adapter) = 0;
        virtual LoRAAdapter load(const std::string& id) = 0;
    };
    
    class FileSystemBackend : public IStorageBackend { /*...*/ };
    class ThemisDBBackend : public IStorageBackend { /*...*/ };
    class S3Backend : public IStorageBackend { /*...*/ };
    
    class LoRAStorageService {
        std::unique_ptr<IStorageBackend> backend_;
    };
    ```
  - **Aufwand**: 1 Woche

**Meilenstein 2.1**: Storage vollständig implementiert, alle Backends funktional

---

### 2.2 S3 Storage Backend (Optional, Cloud Deployment)

**Priorität**: 🟡 MITTEL  
**Aufwand**: 2 Wochen

#### Aufgaben

- [ ] **2.2.1 AWS SDK C++ Integration**
  ```cmake
  find_package(AWSSDK REQUIRED COMPONENTS s3)
  target_link_libraries(themis_llm aws-cpp-sdk-s3)
  ```

- [ ] **2.2.2 S3Backend implementieren**
  ```cpp
  class S3Backend : public IStorageBackend {
      bool save(const std::string& id, const LoRAAdapter& adapter) override {
          Aws::S3::Model::PutObjectRequest request;
          request.SetBucket(bucket_name_);
          request.SetKey("lora_adapters/" + id + ".bin");
          
          auto stream = std::make_shared<std::stringstream>();
          serializeAdapter(adapter, *stream);
          request.SetBody(stream);
          
          auto outcome = s3_client_->PutObject(request);
          return outcome.IsSuccess();
      }
  };
  ```

**Meilenstein 2.2**: Cloud-Deployment möglich

---

## Phase 3: Qualitätssicherung & Testing (1-2 Monate)

### 3.1 Comprehensive Test Suite

**Priorität**: 🔴 HOCH  
**Aufwand**: 4-6 Wochen

#### Aufgaben

- [ ] **3.1.1 Unit Tests für alle Komponenten**
  ```cpp
  // test_lora_training_service.cpp
  TEST(LoRATrainingService, TrainSmallDataset) {
      LoRATrainingService service;
      
      std::vector<TrainingData> data = createTestDataset(10);
      LoRAConfig config{.epochs = 1, .batch_size = 2};
      
      auto result = service.trainOnTheFly("test_model", data, config);
      
      EXPECT_TRUE(result.success);
      EXPECT_GT(result.final_loss, 0.0f);
      EXPECT_LT(result.final_loss, 10.0f);  // Sanity check
  }
  
  TEST(LoRATrainingService, OverfitSingleExample) {
      // Overfitting-Test: Einzelnes Beispiel, viele Epochen
      // Loss sollte gegen 0 gehen
  }
  ```
  - **Test Coverage Ziel**: >80%
  - **Aufwand**: 3 Wochen

- [ ] **3.1.2 Integration Tests**
  ```cpp
  TEST(EndToEnd, TrainAndInferWithLoRA) {
      // 1. Base Model laden
      LlamaWrapper wrapper;
      wrapper.loadModel("models/test_model.gguf");
      
      // 2. LoRA-Training
      LoRATrainingService training;
      auto result = training.trainOnTheFly("test_model", test_data, config);
      ASSERT_TRUE(result.success);
      
      // 3. LoRA-Adapter laden
      wrapper.loadLoRAAdapter(result.adapter_path);
      
      // 4. Inferenz mit Adapter
      auto response = wrapper.generate({"test prompt", 100});
      
      // 5. Validierung
      EXPECT_FALSE(response.empty());
      EXPECT_NE(response.find("placeholder"), std::string::npos);  // Kein Stub!
  }
  ```
  - **Aufwand**: 2 Wochen

- [ ] **3.1.3 Performance Benchmarks**
  ```cpp
  // bench_lora_training.cpp
  static void BM_LoRATraining(benchmark::State& state) {
      for (auto _ : state) {
          auto result = training_service.trainOnTheFly(...);
      }
      
      state.SetLabel("samples_per_sec=" + std::to_string(
          total_samples / state.elapsed_time()));
  }
  BENCHMARK(BM_LoRATraining)->Range(8, 8<<10);
  ```
  - Throughput-Messung (Samples/Sekunde)
  - Latenz-Messung (P50, P95, P99)
  - GPU-Auslastung
  - **Aufwand**: 1 Woche

- [ ] **3.1.4 Memory Profiling**
  - Valgrind/AddressSanitizer für Leak-Detection
  - CUDA Memory Profiling
  - Baseline-Speichernutzung dokumentieren
  - **Aufwand**: 1 Woche

- [ ] **3.1.5 Load Testing**
  ```cpp
  TEST(LoadTest, Concurrent100Training) {
      std::vector<std::future<TrainingResult>> futures;
      
      for (int i = 0; i < 100; ++i) {
          futures.push_back(std::async(std::launch::async, [&]() {
              return training_service.trainOnTheFly(...);
          }));
      }
      
      for (auto& f : futures) {
          auto result = f.get();
          EXPECT_TRUE(result.success);
      }
  }
  ```
  - **Aufwand**: 3 Tage

**Meilenstein 3.1**: Comprehensive Test Coverage, Performance-Baseline etabliert

---

### 3.2 Security Audit & Penetration Testing

**Priorität**: 🔴 HOCH  
**Aufwand**: 2 Wochen

#### Aufgaben

- [ ] **3.2.1 Automated Security Scanning**
  - Cppcheck (statische Analyse)
  - Clang Static Analyzer
  - SonarQube
  - **Aufwand**: 3 Tage

- [ ] **3.2.2 Manual Code Review (Security Focus)**
  - Input Validation überprüfen
  - SQL Injection Risks
  - Buffer Overflow Risks
  - **Aufwand**: 1 Woche

- [ ] **3.2.3 Penetration Testing**
  - Malicious Adapter Upload
  - Prompt Injection Attacks
  - DoS Attacks (Resource Exhaustion)
  - **Aufwand**: 3-4 Tage

**Meilenstein 3.2**: Security-Audit bestanden

---

## Phase 4: Performance-Optimierung (1 Monat)

### 4.1 Inference Performance

**Aufwand**: 2 Wochen

#### Aufgaben

- [ ] **4.1.1 KV-Cache Optimierung**
  - Prefix-Cache effizient nutzen
  - Cache-Hit-Rate messen und optimieren

- [ ] **4.1.2 Batch Processing Tuning**
  - Continuous Batching implementieren
  - Dynamic Batching für variable Sequence-Längen

- [ ] **4.1.3 GPU Kernel Optimierung**
  - Flash Attention (falls llama.cpp unterstützt)
  - Tensor Cores nutzen (falls verfügbar)

**Meilenstein 4.1**: Inferenz-Latenz <200ms (P95) für 7B Modelle

---

### 4.2 Training Performance

**Aufwand**: 2 Wochen

#### Aufgaben

- [ ] **4.2.1 Mixed Precision Training**
  - FP16 für Forward/Backward Pass
  - FP32 für Optimizer State

- [ ] **4.2.2 Gradient Accumulation**
  - Effektive Batch Size erhöhen ohne Memory-Overhead

- [ ] **4.2.3 Multi-GPU Training (Optional)**
  - Data Parallelism
  - NCCL für Gradient Synchronization

**Meilenstein 4.2**: Training-Throughput >1000 Samples/Sekunde (7B Modell, A100 GPU)

---

## Phase 5: Dokumentation & Produktionsreife (1 Monat)

### 5.1 API-Dokumentation

**Aufwand**: 1 Woche

- [ ] Doxygen-Kommentare für alle öffentlichen APIs
- [ ] Beispiel-Code für häufige Use-Cases
- [ ] API-Referenz generieren

---

### 5.2 User-Dokumentation

**Aufwand**: 1 Woche

- [ ] Schnellstart-Guide
- [ ] Training-Tutorial
- [ ] Deployment-Guide
- [ ] Troubleshooting-Sektion

---

### 5.3 Operations-Dokumentation

**Aufwand**: 1 Woche

- [ ] Monitoring-Setup (Grafana Dashboards)
- [ ] Alerting-Regeln
- [ ] Backup/Restore-Prozeduren
- [ ] Skalierungs-Guidelines

---

### 5.4 Production Readiness Review

**Aufwand**: 1 Woche

- [ ] **Checkliste durchgehen**:
  - [ ] Alle Critical/High Priority TODOs abgeschlossen
  - [ ] Test Coverage >80%
  - [ ] Security Audit bestanden
  - [ ] Performance-Benchmarks erfüllt
  - [ ] Dokumentation vollständig
  - [ ] Monitoring aktiv
  - [ ] Rollback-Plan vorhanden

**Meilenstein 5**: PRODUKTIONSREIFE

---

## Risikomanagement

### Technische Risiken

| Risiko | Wahrscheinlichkeit | Impact | Mitigation |
|--------|-------------------|--------|------------|
| llama.cpp API-Änderungen | HOCH | HOCH | Version pinnen, Wrapper-Layer |
| Training-Performance unzureichend | MITTEL | HOCH | Frühzeitiges Profiling, Optimierung |
| Memory-Limits | MITTEL | HOCH | Memory-Pooling, Gradient Checkpointing |
| Security-Lücken | HOCH | KRITISCH | Security Audit, Penetration Testing |

### Zeitplan-Risiken

| Risiko | Mitigation |
|--------|------------|
| Unterschätzter Aufwand | 20% Buffer einplanen |
| Abhängigkeiten blockieren | Parallele Work Streams |
| Team-Verfügbarkeit | Backup-Plan für kritische Komponenten |

---

## Ressourcen-Bedarf

### Team

- **1x Senior ML Engineer** (llama.cpp Integration, Training)
- **1x Backend Engineer** (Storage, Orchestration)
- **1x Security Engineer** (Kryptographie, Audits)
- **Optional: 1x DevOps Engineer** (Deployment, Monitoring)

### Hardware

- **Development**: 1x NVIDIA RTX 4090 (24GB VRAM) oder vergleichbar
- **Testing**: Cloud GPU (A100 80GB für große Modelle)
- **CI/CD**: CPU-Nodes für Tests ohne GPU

### Software/Services

- llama.cpp (neueste stabile Version)
- OpenSSL (für Kryptographie)
- AWS SDK (für S3, optional)
- Grafana/Prometheus (Monitoring)

---

## Erfolgsmetriken

### Phase 1 (Blocker)
- ✅ llama.cpp gibt echte Antworten zurück (keine Placeholders)
- ✅ LoRA-Training reduziert Loss messbar
- ✅ Signatur-Verifikation funktioniert kryptographisch

### Phase 2 (Infrastructure)
- ✅ ThemisDB Storage speichert/lädt Adapter korrekt
- ✅ Job Queue verarbeitet >100 Jobs parallel

### Phase 3 (Quality)
- ✅ Test Coverage >80%
- ✅ Keine kritischen Security-Issues
- ✅ Performance-Benchmarks erfüllt

### Phase 4 (Performance)
- ✅ Inferenz P95 Latenz <200ms (7B Modell)
- ✅ Training Throughput >1000 Samples/Sekunde

### Phase 5 (Production)
- ✅ Dokumentation vollständig
- ✅ Monitoring aktiv
- ✅ Production Readiness Review bestanden

---

## Zeitplan (Übersicht)

```
Monat 1-3: Phase 1 (Blocker)
├── Woche 1-8:  llama.cpp Integration
├── Woche 9-16: LoRA Training
├── Woche 17-18: Security
└── Woche 19-20: Orchestrator Fixes

Monat 4-5: Phase 2 (Infrastructure)
├── Woche 21-24: ThemisDB Storage
└── Woche 25-26: S3 Storage (optional)

Monat 6-7: Phase 3 (Quality)
├── Woche 27-32: Testing
└── Woche 33-34: Security Audit

Monat 8: Phase 4 (Performance)
├── Woche 35-36: Inference Optimization
└── Woche 37-38: Training Optimization

Monat 9: Phase 5 (Production)
├── Woche 39-40: Dokumentation
├── Woche 41: Operations Setup
└── Woche 42: Production Readiness Review

Optional Buffer: Monat 10-12
```

**Kritischer Pfad**: Phase 1 (llama.cpp + Training) → Phase 3 (Testing) → Phase 5 (Production Review)

---

## Nächste Schritte

1. **Sofort**:
   - [ ] Diesen Plan mit Team reviewen
   - [ ] Ressourcen allokieren (Entwickler, Hardware)
   - [ ] Priorisierung bestätigen

2. **Woche 1**:
   - [ ] Phase 1.1.1 starten: llama.cpp Bibliothek einbinden
   - [ ] Entwicklungsumgebung aufsetzen (GPU-Node)
   - [ ] Testmodell herunterladen (TinyLlama 1.1B)

3. **Woche 2**:
   - [ ] Phase 1.1.2 starten: Modell-Laden implementieren
   - [ ] Erste Integration-Tests schreiben

4. **Fortlaufend**:
   - [ ] Wöchentliche Progress-Reviews
   - [ ] Risiken tracken und mitigieren
   - [ ] Scope-Anpassungen dokumentieren

---

## Anhang: Code-Struktur (Sollzustand)

```
src/llm/
├── llama_wrapper.cpp              [VOLLSTÄNDIG IMPLEMENTIERT]
├── llamacpp_inference_engine.cpp  [VOLLSTÄNDIG IMPLEMENTIERT]
├── llamacpp_training_backend.cpp  [NEU: VOLLSTÄNDIG IMPLEMENTIERT]
├── lora_framework/
│   ├── lora_training_service.cpp  [VOLLSTÄNDIG IMPLEMENTIERT, kein sleep()]
│   ├── lora_orchestrator.cpp      [BUGS GEFIXT, Job Queue]
│   ├── lora_adapter_manager.cpp   [Memory-Leaks gefixt]
│   ├── lora_storage_service.cpp   [Alle Backends implementiert]
│   └── lora_training_config.cpp
├── lora_security_validator.cpp    [Kryptographie implementiert]
├── production_validator.cpp       [Echte Validierung, keine TODOs]
└── multi_lora_manager.cpp

tests/
├── test_llama_wrapper.cpp         [NEU]
├── test_lora_training.cpp         [NEU]
├── test_lora_security.cpp         [ERWEITERT]
├── integration/
│   └── test_end_to_end.cpp        [NEU]
└── benchmarks/
    ├── bench_inference.cpp        [NEU]
    └── bench_training.cpp         [NEU]
```

---

**Version**: 1.0  
**Letzte Aktualisierung**: 15. Januar 2026  
**Status**: GENEHMIGUNG AUSSTEHEND
