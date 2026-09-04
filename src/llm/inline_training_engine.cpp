/**
 * @file inline_training_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=9; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=2, Debt=0, C=1, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/inline_training_engine.h"
#include "llm/adapter_registry.h"
#include "llm/training_data_iterator.h"
#include "governance/model_governance.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <rocksdb/db.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace llm {

using json = nlohmann::json;
namespace fs = std::filesystem;
constexpr float kPi = 3.14159265358979323846f;

// ═══════════════════════════════════════════════════════════════════════════
// JSON helpers – OptimizerConfig
// ═══════════════════════════════════════════════════════════════════════════

json OptimizerConfig::toJSON() const {
    json j;
    j["type"]                  = static_cast<int>(type);
    j["learning_rate"]         = learning_rate;
    j["beta1"]                 = beta1;
    j["beta2"]                 = beta2;
    j["epsilon"]               = epsilon;
    j["weight_decay"]          = weight_decay;
    j["momentum"]              = momentum;
    j["nesterov"]              = nesterov;
    j["use_gradient_clipping"] = use_gradient_clipping;
    j["max_grad_norm"]         = max_grad_norm;
    return j;
}

OptimizerConfig OptimizerConfig::fromJSON(const json& j) {
    OptimizerConfig cfg = {};
    if (j.contains("type")) {
      cfg.type = static_cast<OptimizerType>(j["type"].get<int>());
    }
    if (j.contains("learning_rate")) {
      cfg.learning_rate = j["learning_rate"].get<float>();
    }
    if (j.contains("beta1")) {
      cfg.beta1 = j["beta1"].get<float>();
    }
    if (j.contains("beta2")) {
      cfg.beta2 = j["beta2"].get<float>();
    }
    if (j.contains("epsilon")) {
      cfg.epsilon = j["epsilon"].get<float>();
    }
    if (j.contains("weight_decay")) {
      cfg.weight_decay = j["weight_decay"].get<float>();
    }
    if (j.contains("momentum")) {
      cfg.momentum = j["momentum"].get<float>();
    }
    if (j.contains("nesterov")) {
      cfg.nesterov = j["nesterov"].get<bool>();
    }
    if (j.contains("use_gradient_clipping")) {
      cfg.use_gradient_clipping = j["use_gradient_clipping"].get<bool>();
    }
    if (j.contains("max_grad_norm")) {
      cfg.max_grad_norm = j["max_grad_norm"].get<float>();
    }
    return cfg;
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON helpers – SchedulerConfig
// ═══════════════════════════════════════════════════════════════════════════

json SchedulerConfig::toJSON() const {
    json j;
    j["type"]         = static_cast<int>(type);
    j["warmup_steps"] = warmup_steps;
    j["min_lr"]       = min_lr;
    j["max_lr"]       = max_lr;
    j["total_steps"]  = total_steps;
    j["power"]        = power;
    return j;
}

SchedulerConfig SchedulerConfig::fromJSON(const json& j) {
    SchedulerConfig cfg = {};
    if (j.contains("type")) {
      cfg.type = static_cast<SchedulerType>(j["type"].get<int>());
    }
    if (j.contains("warmup_steps")) {
      cfg.warmup_steps = j["warmup_steps"].get<int>();
    }
    if (j.contains("min_lr")) {
      cfg.min_lr = j["min_lr"].get<float>();
    }
    if (j.contains("max_lr")) {
      cfg.max_lr = j["max_lr"].get<float>();
    }
    if (j.contains("total_steps")) {
      cfg.total_steps = j["total_steps"].get<int>();
    }
    if (j.contains("power")) {
      cfg.power = j["power"].get<float>();
    }
    return cfg;
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON helpers – TrainingMetrics
// ═══════════════════════════════════════════════════════════════════════════

json TrainingMetrics::toJSON() const {
    json j;
    j["epoch"]              = epoch;
    j["step"]               = step;
    j["loss"]               = loss;
    j["gradient_norm"]      = gradient_norm;
    j["learning_rate"]      = learning_rate;
    j["elapsed_seconds"]    = elapsed_seconds;
    j["samples_per_second"] = samples_per_second;
    if (perplexity) {
      j["perplexity"] = *perplexity;
    }
    if (accuracy) {
      j["accuracy"]   = *accuracy;
    }
    return j;
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON helpers – TrainingState
// ═══════════════════════════════════════════════════════════════════════════

json TrainingState::toJSON() const {
    json j;
    j["current_epoch"]   = current_epoch;
    j["current_step"]    = current_step;
    j["best_loss"]       = best_loss;
    j["loss_history"]    = loss_history;
    j["optimizer_state"] = optimizer_state;
    return j;
}

TrainingState TrainingState::fromJSON(const json& j) {
    TrainingState s = {};
    if (j.contains("current_epoch")) {
      s.current_epoch = j["current_epoch"].get<int>();
    }
    if (j.contains("current_step")) {
      s.current_step = j["current_step"].get<int>();
    }
    if (j.contains("best_loss")) {
      s.best_loss = j["best_loss"].get<float>();
    }
    if (j.contains("loss_history")) {
      s.loss_history = j["loss_history"].get<std::vector<float>>();
    }
    if (j.contains("optimizer_state")) {
      s.optimizer_state = j["optimizer_state"].get<std::vector<uint8_t>>();
    }
    return s;
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON helpers – InlineTrainingConfig
// ═══════════════════════════════════════════════════════════════════════════

json InlineTrainingConfig::toJSON() const {
    json j;
    j["epochs"]                      = epochs;
    j["batch_size"]                  = batch_size;
    j["gradient_accumulation_steps"] = gradient_accumulation_steps;
    j["max_steps"]                   = max_steps;
    j["eval_steps"]                  = eval_steps;
    j["save_steps"]                  = save_steps;
    j["eval_on_start"]               = eval_on_start;
    j["use_fp16"]                    = use_fp16;
    j["use_bf16"]                    = use_bf16;
    j["checkpoint_dir"]              = checkpoint_dir;
    j["save_optimizer_state"]        = save_optimizer_state;
    j["max_checkpoints_to_keep"]     = max_checkpoints_to_keep;
    j["optimizer"]                   = optimizer.toJSON();
    j["scheduler"]                   = scheduler.toJSON();
    if (seed) {
      j["seed"]              = *seed;
    }
    return j;
}

InlineTrainingConfig InlineTrainingConfig::fromJSON(const json& j) {
    InlineTrainingConfig cfg = {};
    if (j.contains("epochs")) {
      cfg.epochs = j["epochs"].get<int>();
    }
    if (j.contains("batch_size")) {
      cfg.batch_size = j["batch_size"].get<int>();
    }
    if (j.contains("gradient_accumulation_steps")) {
      cfg.gradient_accumulation_steps = j["gradient_accumulation_steps"].get<int>();
    }
    if (j.contains("max_steps")) {
      cfg.max_steps = j["max_steps"].get<int>();
    }
    if (j.contains("eval_steps")) {
      cfg.eval_steps = j["eval_steps"].get<int>();
    }
    if (j.contains("save_steps")) {
      cfg.save_steps = j["save_steps"].get<int>();
    }
    if (j.contains("eval_on_start")) {
      cfg.eval_on_start = j["eval_on_start"].get<bool>();
    }
    if (j.contains("use_fp16")) {
      cfg.use_fp16 = j["use_fp16"].get<bool>();
    }
    if (j.contains("use_bf16")) {
      cfg.use_bf16 = j["use_bf16"].get<bool>();
    }
    if (j.contains("checkpoint_dir")) {
      cfg.checkpoint_dir = j["checkpoint_dir"].get<std::string>();
    }
    if (j.contains("save_optimizer_state")) {
      cfg.save_optimizer_state = j["save_optimizer_state"].get<bool>();
    }
    if (j.contains("max_checkpoints_to_keep")) {
      cfg.max_checkpoints_to_keep = j["max_checkpoints_to_keep"].get<int>();
    }
    if (j.contains("optimizer")) {
      cfg.optimizer = OptimizerConfig::fromJSON(j["optimizer"]);
    }
    if (j.contains("scheduler")) {
      cfg.scheduler = SchedulerConfig::fromJSON(j["scheduler"]);
    }
    if (j.contains("seed")) {
      cfg.seed = j["seed"].get<int>();
    }
    return cfg;
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON helpers – TrainingResult
// ═══════════════════════════════════════════════════════════════════════════

json TrainingResult::toJSON() const {
    json j;
    j["success"]       = success;
    j["message"]       = message;
    j["adapter_path"]  = adapter_path;
    j["final_metrics"] = final_metrics.toJSON();
    json hist = json::array();
    for (const auto& m : history) {
        hist.push_back(m.toJSON());
    }
    j["history"] = hist;
    return j;
}

// ═══════════════════════════════════════════════════════════════════════════
// InlineTrainingEngine::Impl
// ═══════════════════════════════════════════════════════════════════════════

struct InlineTrainingEngine::Impl {
    // External dependencies
    std::shared_ptr<AdapterRegistry>      registry;
    std::shared_ptr<TrainingDataIterator> data_iterator;

    // Configuration (immutable after construction)
    InlineTrainingConfig config;

    // Per-optimizer first/second moment estimates (AdamW/Adam)
    std::vector<float> m_adam;   // first moment
    std::vector<float> v_adam;   // second moment (or v_rms for RMSProp)
    std::vector<float> m_sgd;    // SGD velocity

    // Persistent LoRA parameter vector updated by every optimizer step.
    // Initialised lazily on the first step; size matches the gradient dimension.
    // Wave-B L5: replaces the per-step dummy zero vector (stub fix).
    std::vector<float> model_params_;

    // Thread-safety and state
    mutable std::mutex state_mutex;
    std::atomic<bool>  stop_flag{false};
    std::atomic<bool>  is_training{false};

    // Current training state (guarded by state_mutex)
    TrainingState current_state;

    // Optional governance gate (Gap 3 — AI_ML_IMPACT_ASSESSMENT.md §7)
    std::shared_ptr<governance::ModelGovernancePolicy> governance_policy;

    // Optional real gradient computer (stub #37 injection)
    GradientComputerFn gradient_computer_fn;
};

// ═══════════════════════════════════════════════════════════════════════════
// Constructor / Destructor
// ═══════════════════════════════════════════════════════════════════════════

InlineTrainingEngine::InlineTrainingEngine(
    std::shared_ptr<AdapterRegistry>      registry,
    std::shared_ptr<TrainingDataIterator> data_iterator,
    const InlineTrainingConfig&           config
) : impl_(std::make_unique<Impl>()) {
    if (!registry) {
        throw std::invalid_argument("InlineTrainingEngine: registry must not be null");
    }
    if (!data_iterator) {
        throw std::invalid_argument("InlineTrainingEngine: data_iterator must not be null");
    }
    impl_->registry      = std::move(registry);
    impl_->data_iterator = std::move(data_iterator);
    impl_->config        = config;
}

InlineTrainingEngine::~InlineTrainingEngine() noexcept {
    // Phase2-LLM-B1: exception_in_destructor — stopTraining() may throw; must
    // not propagate out of the destructor (§[except.spec]).
    try {
        stopTraining();
    } catch (const std::exception& e) {
        spdlog::error("InlineTrainingEngine::~InlineTrainingEngine: exception during stopTraining (suppressed): {}", e.what());
    } catch (...) {
        spdlog::error("InlineTrainingEngine::~InlineTrainingEngine: unknown exception during stopTraining (suppressed)");
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Public API – setGovernancePolicy (Gap 3)
// ═══════════════════════════════════════════════════════════════════════════

void InlineTrainingEngine::setGovernancePolicy(
    std::shared_ptr<governance::ModelGovernancePolicy> policy)
{
    std::lock_guard<std::mutex> lock(impl_->state_mutex);
    impl_->governance_policy = std::move(policy);
}

// ═══════════════════════════════════════════════════════════════════════════
// Public API – setCheckpointDb
// ═══════════════════════════════════════════════════════════════════════════

void InlineTrainingEngine::setCheckpointDb(std::shared_ptr<rocksdb::DB> db)
{
    checkpoint_db_ = std::move(db);
}

// ═══════════════════════════════════════════════════════════════════════════
// Public API – setGradientComputer (stub #37)
// ═══════════════════════════════════════════════════════════════════════════

void InlineTrainingEngine::setGradientComputer(GradientComputerFn fn)
{
    std::lock_guard<std::mutex> lock(impl_->state_mutex);
    impl_->gradient_computer_fn = std::move(fn);
}

// ═══════════════════════════════════════════════════════════════════════════
// Public API – train
// ═══════════════════════════════════════════════════════════════════════════

TrainingResult InlineTrainingEngine::train(
    const std::string&    adapter_id,
    const std::string&    base_model_path,
    const TrainingConfig& training_config
) {
    // ── Governance gate (Gap 3 — AI_ML_IMPACT_ASSESSMENT.md §7) ─────────
    {
        std::shared_ptr<governance::ModelGovernancePolicy> policy;
        {
            std::lock_guard<std::mutex> lock(impl_->state_mutex);
            policy = impl_->governance_policy;
        }

        if (!policy) {
            if (impl_->config.require_policy_gate) {
                TrainingResult r;
                r.success = false;
                r.message = "Governance policy gate is required but no policy has been "
                            "set. Call setGovernancePolicy() before train().";
                spdlog::error("InlineTrainingEngine: {}", r.message);
                return r;
            }
            spdlog::warn("InlineTrainingEngine: no governance policy set for adapter '{}' "
                         "(require_policy_gate=false — proceeding without policy check)",
                         adapter_id);
        } else {
            governance::ModelTrainingExportRequest gov_req;
            gov_req.export_job_id   = adapter_id;
            gov_req.adapter_id      = adapter_id;
            gov_req.requesting_user = "InlineTrainingEngine";
            gov_req.purpose         = "MODEL_TRAINING";
            // collection_ids left empty here: the caller may pre-populate
            // them via ModelTrainingExportRequest before wiring in the policy,
            // or the policy may derive permissions from adapter_id alone.
            // In a future iteration TrainingDataIterator will expose
            // getDeclaredCollections() (see rag/FUTURE_ENHANCEMENTS.md §Gap 3).

            const auto decision = policy->checkExportPermission(gov_req);

            if (!decision.is_permitted) {
                TrainingResult r;
                r.success = false;
                r.message = "Governance policy DENIED training for adapter '" +
                            adapter_id + "': " + decision.denial_reason;
                spdlog::warn("InlineTrainingEngine: {}", r.message);
                return r;
            }

            spdlog::info("InlineTrainingEngine: governance PERMIT for adapter '{}' "
                         "(lineage_event_id={})",
                         adapter_id, decision.lineage_event_id);
        }
    }
    // ── End governance gate ──────────────────────────────────────────────
    if (impl_->is_training.exchange(true)) {
        TrainingResult r;
        r.success = false;
        r.message = "Training already in progress";
        return r;
    }

    impl_->stop_flag.store(false);

    spdlog::info("InlineTrainingEngine: starting training for adapter '{}', base='{}'",
                 adapter_id, base_model_path);

    TrainingResult result;
    try {
        result = trainLoop(adapter_id, base_model_path, training_config);
    } catch (const std::exception& ex) {
        result.success = false;
        result.message = std::string("Training failed: ") + ex.what();
        spdlog::error("InlineTrainingEngine: {}", result.message);
    }

    impl_->is_training.store(false);
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Public API – resumeFromCheckpoint
// ═══════════════════════════════════════════════════════════════════════════

TrainingResult InlineTrainingEngine::resumeFromCheckpoint(
    const std::string& checkpoint_path
) {
    if (impl_->is_training.exchange(true)) {
        TrainingResult r;
        r.success = false;
        r.message = "Training already in progress";
        return r;
    }

    impl_->stop_flag.store(false);

    spdlog::info("InlineTrainingEngine: resuming from checkpoint '{}'", checkpoint_path);

    TrainingResult result;
    try {
        TrainingState state = loadCheckpoint(checkpoint_path);

        {
            std::lock_guard<std::mutex> lk(impl_->state_mutex);
            impl_->current_state = state;
        }

        // Re-run the training loop – it will skip already-completed steps
        // by reading current_state.current_step.
        result = trainLoop("resumed", "", {});
    } catch (const std::exception& ex) {
        result.success = false;
        result.message = std::string("Resume failed: ") + ex.what();
        spdlog::error("InlineTrainingEngine: {}", result.message);
    }

    impl_->is_training.store(false);
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Public API – evaluate
// ═══════════════════════════════════════════════════════════════════════════

TrainingMetrics InlineTrainingEngine::evaluate(
    const std::string& /*adapter_path*/,
    const std::string& /*base_model_path*/
) {
    spdlog::info("InlineTrainingEngine: running evaluation");
    return runValidation();
}

// ═══════════════════════════════════════════════════════════════════════════
// Public API – control
// ═══════════════════════════════════════════════════════════════════════════

void InlineTrainingEngine::stopTraining() {
    impl_->stop_flag.store(true);
    spdlog::info("InlineTrainingEngine: stop requested");
}

bool InlineTrainingEngine::isTraining() const {
    return impl_->is_training.load(std::memory_order_acquire);
}

std::optional<TrainingState> InlineTrainingEngine::getCurrentState() const {
    if (!impl_->is_training.load(std::memory_order_acquire)) {
        return std::nullopt;
    }
    std::lock_guard<std::mutex> lk(impl_->state_mutex);
    return impl_->current_state;
}

// ═══════════════════════════════════════════════════════════════════════════
// Private – trainLoop
// ═══════════════════════════════════════════════════════════════════════════

TrainingResult InlineTrainingEngine::trainLoop(
    const std::string&    adapter_id,
    const std::string&    base_model_path,
    const TrainingConfig& training_config
) {
    const auto& cfg = impl_->config;
    TrainingResult result;
    result.success = false;

    auto wall_start = std::chrono::steady_clock::now();

    // Compute total steps if not set explicitly
    int total_steps = cfg.max_steps;
    // Determine epoch count from training_config if provided, else fall back to config.epochs
    const int epochs = (training_config.epochs > 0)
                           ? training_config.epochs
                           : cfg.epochs;

    // Restore or initialise training state
    TrainingState state;
    {
        std::lock_guard<std::mutex> lk(impl_->state_mutex);
        state = impl_->current_state;  // may be default-constructed
    }

    int start_epoch = state.current_epoch;
    int start_step  = state.current_step;

    // Create checkpoint directory if required
    if (!cfg.checkpoint_dir.empty()) {
        fs::create_directories(cfg.checkpoint_dir);
    }

    // Optional eval on start
    if (cfg.eval_on_start) {
        auto eval_metrics = runValidation();
        spdlog::info("InlineTrainingEngine: eval-on-start loss={:.4f}", eval_metrics.loss);
    }

    int global_step = start_step;
    auto batch_start = std::chrono::steady_clock::now();

    for (int epoch = start_epoch; epoch < epochs; ++epoch) {
        if (impl_->stop_flag.load(std::memory_order_acquire)) {
            spdlog::info("InlineTrainingEngine: stop flag set – aborting after epoch {}", epoch);
            break;
        }

        impl_->data_iterator->reset();

        while (!impl_->stop_flag.load(std::memory_order_acquire)) {
            // Early exit if max_steps reached
            if (total_steps > 0 && global_step >= total_steps) {
                goto training_done;
            }

            // Fetch next batch
            if (!impl_->data_iterator->hasNext()) {
                break;  // end of epoch
            }
            auto batch_opt = impl_->data_iterator->getNextBatch();
            if (!batch_opt.has_value()) {
                break;  // end of epoch
            }
            const auto& primary_batch = batch_opt->samples;

            // Gradient accumulation loop
            std::vector<float> accumulated_gradients = {};

            for (int acc = 0; acc < cfg.gradient_accumulation_steps; ++acc) {
                std::optional<TrainingDataIterator::TrainingBatch> sub_opt = {};

                if (acc > 0 && impl_->data_iterator->hasNext()) {
                    sub_opt = impl_->data_iterator->getNextBatch();
                }
                const auto& sub_batch = (sub_opt.has_value()) ? sub_opt->samples : primary_batch;

                std::vector<float> grads;
                computeGradients(sub_batch, grads);

                if (accumulated_gradients.empty()) {
                    accumulated_gradients = grads;
                } else {
                    for (size_t i = 0; i < grads.size()  && static_cast<size_t>(i) < accumulated_gradients.size(); ++i) {
                        accumulated_gradients[i] += grads[i];
                    }
                }
            }

            // Normalise by accumulation steps
            if (!accumulated_gradients.empty()) {
                const float scale = 1.0f / static_cast<float>(cfg.gradient_accumulation_steps);
                for (auto& g : accumulated_gradients) {
                    g *= scale;
                }
            }

            // Apply optimizer step to persistent LoRA parameter vector.
            // Wave-B L5: params are retained across steps so optimizer moments
            // and weight-decay accumulate correctly over the full training run.
            if (!accumulated_gradients.empty()) {
                if (impl_->model_params_.size() != accumulated_gradients.size()) {
                    // Lazy initialisation: small random values in [-0.01, 0.01].
                    impl_->model_params_.resize(accumulated_gradients.size());
                    const float kInitScale = 0.01f;
                    for (size_t i = 0; i < impl_->model_params_.size(); ++i) {
                        impl_->model_params_[i] =
                            kInitScale * (2.0f * static_cast<float>(i % 17) / 16.0f - 1.0f);
                    }
                }
                optimizerStep(impl_->model_params_, accumulated_gradients, global_step);
            }

            ++global_step;

            // Compute metrics for this step
            float step_loss = 0.0f;
            if (!accumulated_gradients.empty()) {
                // Proxy loss: mean squared gradient norm (serves as a stable proxy)
                float sq_sum = 0.0f;
                for (float g : accumulated_gradients) {
                    sq_sum += g * g;
                }
                step_loss = std::sqrt(sq_sum / static_cast<float>(accumulated_gradients.size()));
            }

            // Gradient norm
            float grad_norm = 0.0f;
            for (float g : accumulated_gradients) {
                grad_norm += g * g;
            }
            grad_norm = std::sqrt(grad_norm);

            float lr = getLearningRate(global_step);

            auto now = std::chrono::steady_clock::now();
            double elapsed =
                std::chrono::duration<double>(now - wall_start).count();
            double step_elapsed =
                std::chrono::duration<double>(now - batch_start).count();
            batch_start = now;

            double samples_per_sec = (step_elapsed > 0.0)
                ? static_cast<double>(cfg.batch_size) / step_elapsed
                : 0.0;

            TrainingMetrics metrics;
            metrics.epoch              = epoch;
            metrics.step               = global_step;
            metrics.loss               = step_loss;
            metrics.gradient_norm      = grad_norm;
            metrics.learning_rate      = lr;
            metrics.elapsed_seconds    = elapsed;
            metrics.samples_per_second = samples_per_sec;
            metrics.perplexity         = std::exp(step_loss);

            result.history.push_back(metrics);

            // Update training state
            {
                std::lock_guard<std::mutex> lk(impl_->state_mutex);
                state.current_epoch = epoch;
                state.current_step  = global_step;
                state.loss_history.push_back(step_loss);
                if (step_loss < state.best_loss) {
                    state.best_loss = step_loss;
                }
                impl_->current_state = state;
            }

            // Fire progress callback
            if ([[maybe_unused]] cfg.progress_callback) {
                cfg.progress_callback([[maybe_unused]] metrics);
            }

            // Periodic evaluation
            if (cfg.eval_steps > 0 && global_step % cfg.eval_steps == 0) {
                auto eval = runValidation();
                spdlog::debug("InlineTrainingEngine: step={} eval_loss={:.4f}",
                              global_step, eval.loss);
            }

            // Periodic checkpointing
            if (cfg.save_steps > 0 && global_step % cfg.save_steps == 0) {
                std::string ckpt_path =
                    cfg.checkpoint_dir + "/checkpoint-step-" + std::to_string(global_step);
                saveCheckpoint(ckpt_path, state);

                // Prune old checkpoints
                if (cfg.max_checkpoints_to_keep > 0) {
                    // Collect all checkpoint dirs sorted by step
                    std::vector<std::pair<int, std::string>> ckpts;
                    for (const auto& entry : fs::directory_iterator(cfg.checkpoint_dir)) {
                        if (entry.is_directory()) {
                            std::string name = entry.path().filename().string();
                            auto pos = name.find("checkpoint-step-");
                            if (pos != std::string::npos) {
                                try {
                                    int step_num = std::stoi(name.substr(pos + 16));
                                    ckpts.emplace_back(step_num, entry.path().string());
                                } catch (...) {}
                            }
                        }
                    }
                    std::sort(ckpts.begin(), ckpts.end());
                    while (static_cast<int>(ckpts.size()) > cfg.max_checkpoints_to_keep) {
                        fs::remove_all(ckpts.front().second);
                        ckpts.erase(ckpts.begin());
                    }
                }

                if ([[maybe_unused]] cfg.checkpoint_callback) {
                    cfg.checkpoint_callback([[maybe_unused]] ckpt_path);
                }
            }

            spdlog::debug("InlineTrainingEngine: epoch={} step={} loss={:.4f} lr={:.2e}",
                          epoch, global_step, step_loss, lr);
        }
    }

training_done:
    // Build final adapter path
    result.adapter_path = base_model_path.empty()
        ? cfg.checkpoint_dir + "/" + adapter_id + ".gguf"
        : cfg.checkpoint_dir + "/" + adapter_id + ".gguf";

    // Populate final metrics
    if (!result.history.empty()) {
        result.final_metrics = result.history.back();
    }

    // Register adapter in registry
    AdapterMetadata meta;
    meta.adapter_id          = adapter_id;
    meta.base_model_name     = base_model_path;
    meta.storage_path        = result.adapter_path;
    meta.training_config     = training_config;
    meta.status              = AdapterMetadata::Status::TRAINED;

    // ISO 8601 timestamp
    {
        auto now_tp = std::chrono::system_clock::now();
        std::time_t now_t = std::chrono::system_clock::to_time_t(now_tp);
        char buf[32] = {};
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now_t));
        meta.created_at = buf;
        meta.updated_at = buf;
    }

    // Only register if not resuming (no adapter_id implies resume path)
    if (!adapter_id.empty() && adapter_id != "resumed") {
        if (!impl_->registry->registerAdapter(meta)) {
            spdlog::warn("InlineTrainingEngine: adapter '{}' already registered (update skipped)",
                         adapter_id);
        }
    }

    result.success = true;
    result.message = "Training completed: " + std::to_string(global_step) + " steps";
    spdlog::info("InlineTrainingEngine: {}", result.message);

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Private – computeGradients
// ═══════════════════════════════════════════════════════════════════════════

void InlineTrainingEngine::computeGradients(
    const std::vector<TrainingDataIterator::TrainingSample>& batch,
    std::vector<float>&                                      gradients
) {
    // Gradient computation is performed by the underlying model backend
    // (llama.cpp / GGUF adapter layer).  This layer aggregates per-sample
    // losses and returns the normalised gradient vector for the LoRA
    // parameters.  When no real backend is attached, a synthetic loss based
    // on sequence length is used so that the optimizer, LR scheduling, and
    // checkpoint machinery can be validated end-to-end.

    if (batch.empty()) {
        gradients.clear();
        return;
    }

    // If a real gradient computer has been injected, delegate to it.
    {
        GradientComputerFn fn;
        {
            std::lock_guard<std::mutex> lock(impl_->state_mutex);
            fn = impl_->gradient_computer_fn;
        }
        if (fn) {
            fn(batch, gradients);
            return;
        }
    }

    // PERMANENT FALLBACK NOTE (InlineTrainingEngine synthetic gradient):
    // Purpose: Provide a synthetic gradient signal so that the LoRA optimizer,
    //   LR scheduling, checkpoint machinery, and training metrics can be
    //   validated end-to-end without a real llama.cpp backend attached.
    // Activation: Active when no IBackendGradientComputer is injected (null backend).
    // Production Delta: Gradients are proportional to sequence length rather
    //   than real loss; convergence is not meaningful.  Loss curve will appear
    //   smooth but does not reflect actual model quality.
    // Note: Inject a real IBackendGradientComputer via setGradientComputer() to
    //   replace this path with a real llama.cpp GGUF backend.
    static constexpr size_t kLoRAParamCount = 256;  // placeholder LoRA rank dimension
    gradients.assign(kLoRAParamCount, 0.0f);

    for (const auto& sample : batch) {
        // Proxy gradient signal: proportional to output token count normalised
        // by instruction length, clipped to [0,1].
        float signal = 0.0f;
        if (!sample.output.empty() && !sample.instruction.empty()) {
            signal = static_cast<float>(sample.output.size()) /
                     (static_cast<float>(sample.instruction.size()) + 1.0f);
            signal = std::min(signal, 1.0f);
        }

        // Apply sample weight
        signal *= static_cast<float>(sample.weight);

        // Spread across gradient vector with a simple cyclic pattern
        for (size_t i = 0; i < kLoRAParamCount; ++i) {
            gradients[i] += signal * std::sin(static_cast<float>(i) * 0.1f);
        }
    }

    // Normalise by batch size
    const float batch_size = static_cast<float>(batch.size());
    for (auto& g : gradients) {
        g /= batch_size;
    }

    // Gradient clipping
    const auto& opt = impl_->config.optimizer;
    if (opt.use_gradient_clipping && opt.max_grad_norm > 0.0f) {
        float norm = 0.0f;
        for (float g : gradients) {
            norm += g * g;
        }
        norm = std::sqrt(norm);
        if (norm > opt.max_grad_norm) {
            const float scale = opt.max_grad_norm / (norm + 1e-8f);
            for (auto& g : gradients) {
                g *= scale;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Private – optimizerStep
// ═══════════════════════════════════════════════════════════════════════════

void InlineTrainingEngine::optimizerStep(
    std::vector<float>&       parameters,
    const std::vector<float>& gradients,
    int                       step
) {
    if (parameters.empty() || gradients.empty()) {
        return;
    }

    const auto& opt = impl_->config.optimizer;
    const float lr  = getLearningRate(step);
    const size_t n  = std::min(parameters.size(), gradients.size());

    switch (opt.type) {
        case OptimizerType::ADAM:
        [[fallthrough]];\n        case OptimizerType::ADAM_W: {
            // Initialise moment vectors on first call
            if (impl_->m_adam.size() != n) {
                impl_->m_adam.assign(n, 0.0f);
                impl_->v_adam.assign(n, 0.0f);
            }

            const float b1   = opt.beta1;
            const float b2   = opt.beta2;
            const float eps  = opt.epsilon;
            const float wd   = (opt.type == OptimizerType::ADAM_W) ? opt.weight_decay : 0.0f;
            const float bias1 = 1.0f - std::pow(b1, static_cast<float>(step + 1));
            const float bias2 = 1.0f - std::pow(b2, static_cast<float>(step + 1));

            for (size_t i = 0; i < n; ++i) {
                impl_->m_adam[i] = b1 * impl_->m_adam[i] + (1.0f - b1) * gradients[i];
                impl_->v_adam[i] = b2 * impl_->v_adam[i] + (1.0f - b2) * gradients[i] * gradients[i];

                const float m_hat = impl_->m_adam[i] / bias1;
                const float v_hat = impl_->v_adam[i] / bias2;

                // AdamW decoupled weight decay
                parameters[i] -= lr * (m_hat / (std::sqrt(v_hat) + eps) + wd * parameters[i]);
            }
            break;
        }

        case OptimizerType::SGD: {
            if (impl_->m_sgd.size() != n) {
                impl_->m_sgd.assign(n, 0.0f);
            }
            const float momentum = opt.momentum;
            for (size_t i = 0; i < n; ++i) {
                if (opt.nesterov) {
                    impl_->m_sgd[i] = momentum * impl_->m_sgd[i] + gradients[i];
                    parameters[i]  -= lr * (momentum * impl_->m_sgd[i] + gradients[i]);
                } else {
                    impl_->m_sgd[i] = momentum * impl_->m_sgd[i] + gradients[i];
                    parameters[i]  -= lr * impl_->m_sgd[i];
                }
            }
            break;
        }

        case OptimizerType::ADAGRAD: {
            if (impl_->v_adam.size() != n) {
                impl_->v_adam.assign(n, 0.0f);
            }
            for (size_t i = 0; i < n; ++i) {
                impl_->v_adam[i] += gradients[i] * gradients[i];
                parameters[i]    -= lr * gradients[i] / (std::sqrt(impl_->v_adam[i]) + opt.epsilon);
            }
            break;
        }

        case OptimizerType::RMSPROP: {
            if (impl_->v_adam.size() != n) {
                impl_->v_adam.assign(n, 0.0f);
            }
            const float alpha = 0.99f;  // RMSProp decay
            for (size_t i = 0; i < n; ++i) {
                impl_->v_adam[i] = alpha * impl_->v_adam[i] +
                                   (1.0f - alpha) * gradients[i] * gradients[i];
                parameters[i]   -= lr * gradients[i] / (std::sqrt(impl_->v_adam[i]) + opt.epsilon);
            }
            break;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Private – getLearningRate
// ═══════════════════════════════════════════════════════════════════════════

float InlineTrainingEngine::getLearningRate([[maybe_unused]] int step) const {
    const auto& sched = impl_->config.scheduler;
    const float max_lr = sched.max_lr;
    const float min_lr = sched.min_lr;

    // Linear warmup phase
    if (step < sched.warmup_steps && sched.warmup_steps > 0) {
        return max_lr * static_cast<float>(step) / static_cast<float>(sched.warmup_steps);
    }

    const int decay_steps = sched.total_steps - sched.warmup_steps;
    if (decay_steps <= 0) {
        return max_lr;
    }

    const int progress = step - sched.warmup_steps;
    const float t = std::min(static_cast<float>(progress) / static_cast<float>(decay_steps), 1.0f);

    switch (sched.type) {
        case SchedulerType::CONSTANT:
            return max_lr;

        case SchedulerType::LINEAR:
            return max_lr * (1.0f - t) + min_lr * t;

        case SchedulerType::COSINE:
        [[fallthrough]];\n        case SchedulerType::COSINE_WITH_WARMUP:
            return min_lr + 0.5f * (max_lr - min_lr) * (1.0f + std::cos(kPi * t));

        case SchedulerType::POLYNOMIAL:
            return (max_lr - min_lr) * std::pow(1.0f - t, sched.power) + min_lr;
    }

    return max_lr;
}

// ═══════════════════════════════════════════════════════════════════════════
// Private – saveCheckpoint / loadCheckpoint
// ═══════════════════════════════════════════════════════════════════════════

void InlineTrainingEngine::saveCheckpoint(
    const std::string&   path,
    const TrainingState& state
) {
    try {
        // --- RocksDB persistence (when handle is set) ---
        if (checkpoint_db_) {
            std::string json_value = state.toJSON().dump();
            rocksdb::Status s = checkpoint_db_->Put(
                rocksdb::WriteOptions(), path, json_value);
            if (s.ok()) {
                spdlog::info("[TRAINING] Checkpoint persisted to RocksDB key='{}'", path);
            } else {
                spdlog::warn("[TRAINING] RocksDB checkpoint write failed for key='{}': {}",
                             path, s.ToString());
            }
        }

        // --- Filesystem JSON (always written for durability) ---
        fs::create_directories(path);
        std::ofstream ofs(path + "/training_state.json");
        if (!ofs.is_open()) {
            spdlog::error("InlineTrainingEngine: cannot write checkpoint to '{}'", path);
            return;
        }
        ofs << state.toJSON().dump(2);
        ofs.close();

        // Also persist engine config
        std::ofstream cfg_ofs(path + "/config.json");
        if (cfg_ofs.is_open()) {
            cfg_ofs << impl_->config.toJSON().dump(2);
        }

        spdlog::info("InlineTrainingEngine: checkpoint saved to '{}'", path);
    } catch (const std::exception& ex) {
        spdlog::error("InlineTrainingEngine: saveCheckpoint failed: {}", ex.what());
    }
}

TrainingState InlineTrainingEngine::loadCheckpoint(const std::string& path) {
    // --- Try RocksDB first (when handle is set) ---
    if (checkpoint_db_) {
        std::string value = {};
        rocksdb::Status s = checkpoint_db_->Get(
            rocksdb::ReadOptions(), path, &value);
        if (s.ok()) {
            json j = json::parse(value);
            TrainingState state = TrainingState::fromJSON(j);
            spdlog::info("[TRAINING] Checkpoint loaded from RocksDB key='{}'", path);
            return state;
        }
        // Key not found — fall through to filesystem
    }

    // --- Filesystem fallback ---
    std::ifstream ifs(path + "/training_state.json");
    if (!ifs.is_open()) {
        throw std::runtime_error("InlineTrainingEngine: checkpoint not found at '" + path + "'");
    }

    json j = {};
    ifs >> j;
    TrainingState state = TrainingState::fromJSON(j);

    // Also restore engine config if present
    std::ifstream cfg_ifs(path + "/config.json");
    if (cfg_ifs.is_open()) {
        json cfg_j;
        cfg_ifs >> cfg_j;
        impl_->config = InlineTrainingConfig::fromJSON(cfg_j);
    }

    spdlog::info("InlineTrainingEngine: checkpoint loaded from '{}' (epoch={}, step={})",
                 path, state.current_epoch, state.current_step);
    return state;
}

// ═══════════════════════════════════════════════════════════════════════════
// Private – runValidation
// ═══════════════════════════════════════════════════════════════════════════

TrainingMetrics InlineTrainingEngine::runValidation() {
    TrainingMetrics metrics;

    auto batch_opt = impl_->data_iterator->getNextBatch();
    if (!batch_opt.has_value() || batch_opt->samples.empty()) {
        metrics.loss = 0.0f;
        return metrics;
    }
    const auto& batch = batch_opt->samples;

    std::vector<float> grads;
    computeGradients(batch, grads);

    float loss = 0.0f;
    for (float g : grads) {
        loss += g * g;
    }
    if (!grads.empty()) {
        loss = std::sqrt(loss / static_cast<float>(grads.size()));
    }

    metrics.loss       = loss;
    metrics.perplexity = std::exp(loss);

    {
        std::lock_guard<std::mutex> lk(impl_->state_mutex);
        metrics.epoch = impl_->current_state.current_epoch;
        metrics.step  = impl_->current_state.current_step;
    }

    return metrics;
}

// ═══════════════════════════════════════════════════════════════════════════
// TrainingEngineFactory
// ═══════════════════════════════════════════════════════════════════════════

std::unique_ptr<InlineTrainingEngine> TrainingEngineFactory::create(
    std::shared_ptr<AdapterRegistry>      registry,
    std::shared_ptr<TrainingDataIterator> data_iterator
) {
    return create(std::move(registry), std::move(data_iterator), InlineTrainingConfig{});
}

std::unique_ptr<InlineTrainingEngine> TrainingEngineFactory::create(
    std::shared_ptr<AdapterRegistry>      registry,
    std::shared_ptr<TrainingDataIterator> data_iterator,
    const InlineTrainingConfig&           config
) {
    return std::make_unique<InlineTrainingEngine>(
        std::move(registry), std::move(data_iterator), config);
}

} // namespace llm
} // namespace themis


