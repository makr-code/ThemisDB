/**
 * @file model_switch_workflow.cpp
 * @brief Model switching workflow with semantic version parsing, quantization detection, and deployment coordination.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=3; TODO=0, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/model_switch_workflow.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace themis {
namespace llm {

namespace {

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string extractModelFamily(const std::string& model_name,
                               const std::string& fallback_family) {
    if (!fallback_family.empty()) {
        return toLowerAscii(fallback_family);
    }
    const auto dash = model_name.find('-');
    const auto prefix = (dash == std::string::npos) ? model_name : model_name.substr(0, dash);
    return toLowerAscii(prefix);
}

std::string detectQuantizationToken(const std::string& text) {
    const auto lower = toLowerAscii(text);
    if (lower.find("nf4") != std::string::npos) {
        return "nf4";
    }
    if (lower.find("q8") != std::string::npos || lower.find("int8") != std::string::npos) {
        return "q8";
    }
    if (lower.find("q5") != std::string::npos) {
        return "q5";
    }
    if (lower.find("q4") != std::string::npos || lower.find("int4") != std::string::npos) {
        return "q4";
    }
    if (lower.find("fp16") != std::string::npos) {
        return "fp16";
    }
    if (lower.find("fp32") != std::string::npos) {
        return "fp32";
    }
    return "";
}

} // namespace

// ---------------------------------------------------------------------------
// SemVer
// ---------------------------------------------------------------------------

SemVer SemVer::parse(const std::string& s) {
    SemVer v;
    if (s.empty()) {
        return v;
    }

    auto parse_int = [](std::string_view sv, int& out) -> bool {
        if (sv.empty()) {
            return false;
        }
        if (!std::all_of(sv.begin(), sv.end(), [](unsigned char c) { return std::isdigit(c); })) {
            return false;
        }
        int result = 0;
        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);
        if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
            return false;
        }
        out = result;
        return true;
    };

    std::istringstream ss(s);
    std::string token;
    int part = 0;
    while (std::getline(ss, token, '.') && part < 3) {
        int val = 0;
        if (!parse_int(token, val)) {
            return SemVer{};
        }
        if (part == 0) {
            v.major = val;
        } else if (part == 1) {
            v.minor = val;
        } else {
            v.patch = val;
        }
        ++part;
    }
    if (part == 0 || part > 3) {
        return SemVer{};
    }
    if (ss.good()) {
        return SemVer{};
    }
    if (!s.empty() && s.back() == '.') {
        return SemVer{};
    }
    return v;
}

std::string SemVer::toString() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

bool SemVer::operator<(const SemVer& o) const noexcept {
    if (major != o.major) {
        return major < o.major;
    }
    if (minor != o.minor) {
        return minor < o.minor;
    }
    return patch < o.patch;
}

bool SemVer::operator==(const SemVer& o) const noexcept {
    return major == o.major && minor == o.minor && patch == o.patch;
}

nlohmann::json SemVer::toJson() const {
    return nlohmann::json{{"major", major}, {"minor", minor}, {"patch", patch}};
}

SemVer SemVer::fromJson(const nlohmann::json& j) {
    SemVer v;
    v.major = j.value("major", 0);
    v.minor = j.value("minor", 0);
    v.patch = j.value("patch", 0);
    return v;
}

// ---------------------------------------------------------------------------
// RatchetCompatibilityEntry
// ---------------------------------------------------------------------------

bool RatchetCompatibilityEntry::isSatisfiedBy(const SemVer& version) const noexcept {
    if (version < min_model_version) {
        return false;
    }
    // max_model_version_excl == {0,0,0} means unbounded upper limit
    const SemVer zero{};
    if (!(max_model_version_excl == zero) && !(version < max_model_version_excl)) {
        return false;
    }
    return true;
}

nlohmann::json RatchetCompatibilityEntry::toJson() const {
    return nlohmann::json{
        {"adapter_id", adapter_id},
        {"model_family", model_family},
        {"min_model_version", min_model_version.toJson()},
        {"max_model_version_excl", max_model_version_excl.toJson()},
    };
}

RatchetCompatibilityEntry RatchetCompatibilityEntry::fromJson(const nlohmann::json& j) {
    RatchetCompatibilityEntry e;
    e.adapter_id = j.at("adapter_id").get<std::string>();
    e.model_family = j.at("model_family").get<std::string>();
    e.min_model_version = SemVer::fromJson(j.at("min_model_version"));
    e.max_model_version_excl = SemVer::fromJson(j.at("max_model_version_excl"));
    return e;
}

// ---------------------------------------------------------------------------
// RatchetCompatibilityMatrix
// ---------------------------------------------------------------------------

RatchetCompatibilityMatrix::RatchetCompatibilityMatrix(std::string schema_version)
    : schema_version_(std::move(schema_version)) {}

bool RatchetCompatibilityMatrix::registerEntry(const std::string& adapter_id,
                                               const std::string& model_family,
                                               const SemVer& min_version,
                                               const SemVer& max_version_excl,
                                               bool allow_downgrade) {
    for (auto& e : entries_) {
        if (e.adapter_id == adapter_id && e.model_family == model_family) {
            // Entry exists — enforce ratchet
            if (min_version < e.min_model_version && !allow_downgrade) {
                // Attempted to lower the floor without explicit override
                return false;
            }
            e.min_model_version = min_version;
            e.max_model_version_excl = max_version_excl;
            return true;
        }
    }
    // New entry
    entries_.push_back(RatchetCompatibilityEntry{
        adapter_id, model_family, min_version, max_version_excl});
    return true;
}

std::optional<RatchetCompatibilityEntry>
RatchetCompatibilityMatrix::findEntry(const std::string& adapter_id,
                                      const std::string& model_family) const {
    for (const auto& e : entries_) {
        if (e.adapter_id == adapter_id && e.model_family == model_family) {
            return e;
        }
    }
    return std::nullopt;
}

bool RatchetCompatibilityMatrix::isCompatible(const std::string& adapter_id,
                                              const std::string& model_family,
                                              const std::string& model_version) const {
    const auto entry = findEntry(adapter_id, model_family);
    if (!entry.has_value()) {
        // Open policy: no entry means any version is allowed
        return true;
    }
    const auto parsed = SemVer::parse(model_version);
    return entry->isSatisfiedBy(parsed);
}

const std::vector<RatchetCompatibilityEntry>&
RatchetCompatibilityMatrix::entries() const noexcept {
    return entries_;
}

const std::string& RatchetCompatibilityMatrix::schemaVersion() const noexcept {
    return schema_version_;
}

nlohmann::json RatchetCompatibilityMatrix::toJson() const {
    auto arr = nlohmann::json::array();
    for (const auto& e : entries_) {
        arr.push_back(e.toJson());
    }
    return nlohmann::json{{"schema_version", schema_version_}, {"entries", arr}};
}

RatchetCompatibilityMatrix
RatchetCompatibilityMatrix::fromJson(const nlohmann::json& j) {
    if (!j.contains("schema_version") || !j.contains("entries")) {
        throw std::invalid_argument(
            "RatchetCompatibilityMatrix::fromJson: missing required fields");
    }
    RatchetCompatibilityMatrix m(j["schema_version"].get<std::string>());
    for (const auto& elem : j["entries"]) {
        m.entries_.push_back(RatchetCompatibilityEntry::fromJson(elem));
    }
    return m;
}

// ---------------------------------------------------------------------------
// RebuildPolicy
// ---------------------------------------------------------------------------

bool RebuildPolicy::isTriggerActive(RebuildTrigger trigger) const noexcept {
    return std::find(triggers.begin(), triggers.end(), trigger) != triggers.end();
}

nlohmann::json RebuildPolicy::toJson() const {
    auto trigger_arr = nlohmann::json::array();
    for (const auto t : triggers) {
        trigger_arr.push_back(static_cast<int>(t));
    }
    return nlohmann::json{
        {"fail_closed_on_rebuild", fail_closed_on_rebuild},
        {"triggers", trigger_arr},
    };
}

RebuildPolicy RebuildPolicy::fromJson(const nlohmann::json& j) {
    RebuildPolicy p;
    p.fail_closed_on_rebuild = j.value("fail_closed_on_rebuild", false);
    p.triggers.clear();
    for (const auto& t : j.value("triggers", nlohmann::json::array())) {
        p.triggers.push_back(static_cast<RebuildTrigger>(t.get<int>()));
    }
    return p;
}

// ---------------------------------------------------------------------------
// ModelSwitchResult
// ---------------------------------------------------------------------------

nlohmann::json ModelSwitchResult::toJson() const {
    auto check_arr = nlohmann::json::array();
    for (const auto& c : checks) {
        check_arr.push_back(nlohmann::json{
            {"kind", static_cast<int>(c.kind)},
            {"passed", c.passed},
            {"rebuild_required", c.rebuild_required},
            {"message", c.message},
        });
    }
    auto trigger_arr = nlohmann::json::array();
    for (const auto t : active_rebuild_triggers) {
        trigger_arr.push_back(static_cast<int>(t));
    }
    return nlohmann::json{
        {"outcome", static_cast<int>(outcome)},
        {"can_serve", canServe()},
        {"needs_rebuild", needsRebuild()},
        {"correlation_id", correlation_id},
        {"checks", check_arr},
        {"active_rebuild_triggers", trigger_arr},
        {"errors", errors},
        {"warnings", warnings},
    };
}

// ---------------------------------------------------------------------------
// ModelSwitchWorkflow — construction
// ---------------------------------------------------------------------------

ModelSwitchWorkflow::ModelSwitchWorkflow(std::shared_ptr<AdapterRegistry> registry,
                                         std::shared_ptr<FinalLayerOrchestrator> orchestrator,
                                         RatchetCompatibilityMatrix matrix,
                                         RebuildPolicy policy)
    : registry_(std::move(registry))
    , orchestrator_(std::move(orchestrator))
    , matrix_(std::move(matrix))
    , policy_(std::move(policy)) {
    if (!registry_) {
        throw std::invalid_argument("ModelSwitchWorkflow: registry must not be null");
    }
    if (!orchestrator_) {
        throw std::invalid_argument("ModelSwitchWorkflow: orchestrator must not be null");
    }
}

// ---------------------------------------------------------------------------
// ModelSwitchWorkflow — configuration
// ---------------------------------------------------------------------------

void ModelSwitchWorkflow::setCompatibilityMatrix(RatchetCompatibilityMatrix matrix) {
    matrix_ = std::move(matrix);
}

const RatchetCompatibilityMatrix& ModelSwitchWorkflow::compatibilityMatrix() const noexcept {
    return matrix_;
}

void ModelSwitchWorkflow::setRebuildPolicy(RebuildPolicy policy) {
    policy_ = std::move(policy);
}

const RebuildPolicy& ModelSwitchWorkflow::rebuildPolicy() const noexcept {
    return policy_;
}

// ---------------------------------------------------------------------------
// ModelSwitchWorkflow::isSwitchRequired
// ---------------------------------------------------------------------------

bool ModelSwitchWorkflow::isSwitchRequired(const ModelSwitchRequest& request) noexcept {
    if (request.force_revalidation) {
        return true;
    }
    return !(request.source_model_name == request.target_model_name &&
             request.source_model_version == request.target_model_version);
}

// ---------------------------------------------------------------------------
// Per-check helpers
// ---------------------------------------------------------------------------

ModelSwitchCheckResult ModelSwitchWorkflow::checkRatchetMatrix(
    const std::string& adapter_id,
    const std::string& target_model_family,
    const std::string& target_model_version) const {
    ModelSwitchCheckResult result;
    result.kind = ModelSwitchCheckResult::CheckKind::RATCHET_MATRIX;

    if (!matrix_.isCompatible(adapter_id, target_model_family, target_model_version)) {
        result.passed = false;
        const auto entry = matrix_.findEntry(adapter_id, target_model_family);
        result.message = "Ratchet matrix: adapter '" + adapter_id +
                         "' requires model-family '" + target_model_family +
                         "' >= " + (entry ? entry->min_model_version.toString() : "?") +
                         "; requested " + target_model_version;
    } else {
        result.passed = true;
        result.message = "Ratchet matrix: compatible";
    }
    return result;
}

ModelSwitchCheckResult ModelSwitchWorkflow::checkArchitectureCompatibility(
    const std::string& adapter_id,
    const std::string& target_model_name,
    const std::string& target_model_family,
    const std::string& target_model_version) const {
    ModelSwitchCheckResult result;
    result.kind = ModelSwitchCheckResult::CheckKind::ARCHITECTURE;

    const auto adapter_opt = registry_->getAdapter(adapter_id);
    if (!adapter_opt.has_value()) {
        result.passed = false;
        result.message = "Architecture: adapter '" + adapter_id + "' not found in registry";
        return result;
    }

    const auto& meta = adapter_opt.value();
    const auto target_family = extractModelFamily(target_model_name, target_model_family);
    const auto adapter_arch = toLowerAscii(meta.architecture);
    const auto has_arch = !adapter_arch.empty();
    const bool same_family = has_arch && (adapter_arch == target_family);

    if (!same_family) {
        result.passed = true;
        result.rebuild_required = true;
        result.message = "Architecture: adapter architecture '" + meta.architecture +
                         "' differs from target family '" + target_family +
                         "' — rebuild required before serving";
    } else {
        result.passed = true;
        result.message = "Architecture: compatible";
    }

    const auto validation =
        registry_->validateCompatibility(adapter_id, target_model_name, target_model_version);
    if (!validation.warnings.empty()) {
        result.message += " (" + validation.warnings.front() + ")";
    }

    return result;
}

ModelSwitchCheckResult ModelSwitchWorkflow::checkTokenizerCompatibility(
    const std::string& adapter_id,
    const std::string& target_model_name,
    const std::string& target_model_family) const {
    ModelSwitchCheckResult result;
    result.kind = ModelSwitchCheckResult::CheckKind::TOKENIZER;

    const auto adapter_opt = registry_->getAdapter(adapter_id);
    if (!adapter_opt.has_value()) {
        result.passed = false;
        result.message = "Tokenizer: adapter '" + adapter_id + "' not found in registry";
        return result;
    }

    const auto& meta = adapter_opt.value();
    const auto target_family = extractModelFamily(target_model_name, target_model_family);
    const auto adapter_arch = toLowerAscii(meta.architecture);
    const bool same_family = !adapter_arch.empty() && adapter_arch == target_family;

    if (!same_family) {
        result.passed = true;
        result.rebuild_required = true;
        result.message = "Tokenizer: adapter architecture '" + meta.architecture +
                         "' differs from target family '" + target_family +
                         "' — rebuild recommended to verify tokenizer alignment";
    } else {
        result.passed = true;
        result.message = "Tokenizer: compatible (same architectural family)";
    }
    return result;
}

ModelSwitchCheckResult ModelSwitchWorkflow::checkLayerDimensions(
    const std::string& adapter_id,
    const std::string& target_model_name) const {
    ModelSwitchCheckResult result;
    result.kind = ModelSwitchCheckResult::CheckKind::LAYER_DIMENSIONS;

    const auto adapter_opt = registry_->getAdapter(adapter_id);
    if (!adapter_opt.has_value()) {
        result.passed = false;
        result.message = "LayerDimensions: adapter '" + adapter_id + "' not found";
        return result;
    }

    const auto& meta = adapter_opt.value();
    // When the adapter was trained on a model and the target model name matches
    // the adapter's base_model_name family, assume dimensions are stable.
    // A mismatch in base model name (different size, e.g. 7b vs 13b) triggers
    // a rebuild recommendation.
    const bool name_mismatch = !meta.base_model_name.empty() &&
                               meta.base_model_name != target_model_name;

    if (name_mismatch) {
        result.passed = true; // Not a blocking failure — recommend rebuild
        result.rebuild_required = true;
        result.message = "LayerDimensions: adapter was trained on '" +
                         meta.base_model_name + "', target is '" + target_model_name +
                         "' — layer dimension verification recommended";
    } else {
        result.passed = true;
        result.message = "LayerDimensions: no dimension change detected";
    }
    return result;
}

ModelSwitchCheckResult ModelSwitchWorkflow::checkQuantizationCompatibility(
    const std::string& adapter_id,
    const std::string& target_model_name,
    const std::string& target_model_family,
    const std::string& target_model_version) const {
    ModelSwitchCheckResult result;
    result.kind = ModelSwitchCheckResult::CheckKind::QUANTIZATION;

    const auto adapter_opt = registry_->getAdapter(adapter_id);
    if (!adapter_opt.has_value()) {
        result.passed = false;
        result.message = "Quantization: adapter '" + adapter_id + "' not found";
        return result;
    }

    const auto& meta = adapter_opt.value();
    const auto adapter_quant = detectQuantizationToken(meta.quantization);
    const auto target_quant = detectQuantizationToken(target_model_name);

    result.passed = true;
    if (!target_quant.empty() && !adapter_quant.empty() && target_quant != adapter_quant) {
        result.rebuild_required = true;
        result.message = "Quantization: adapter uses '" + meta.quantization +
                         "' but target model hints '" + target_quant +
                         "' quantization — rebuild/validation required";
    } else if (target_quant.empty()) {
        const auto target_family = extractModelFamily(target_model_name, target_model_family);
        result.message = "Quantization: target model '" + target_model_name + "' (family '" +
                         target_family + "') does not encode quantization; no known conflicts";
    } else {
        result.message = "Quantization: no known conflicts";
    }

    const auto validation =
        registry_->validateCompatibility(adapter_id, target_model_name, target_model_version);
    if (!validation.warnings.empty()) {
        result.message += " (" + validation.warnings.front() + ")";
    }

    return result;
}

ModelSwitchCheckResult ModelSwitchWorkflow::checkPromptFormat(
    const std::string& adapter_id,
    const std::string& target_model_name) const {
    ModelSwitchCheckResult result;
    result.kind = ModelSwitchCheckResult::CheckKind::PROMPT_FORMAT;

    const auto adapter_opt = registry_->getAdapter(adapter_id);
    if (!adapter_opt.has_value()) {
        result.passed = false;
        result.message = "PromptFormat: adapter '" + adapter_id + "' not found";
        return result;
    }

    const auto& meta = adapter_opt.value();
    // Prompt/chat template is tied to the model family.  If the adapter was
    // trained using an instruction-tuned variant (e.g., "instruct", "chat"),
    // switching families requires verifying the prompt template.
    const bool is_instruction_tuned =
        (meta.task_type.find("instruct") != std::string::npos ||
         meta.task_type.find("chat") != std::string::npos ||
         target_model_name.find("instruct") != std::string::npos ||
         target_model_name.find("chat") != std::string::npos);

    if (is_instruction_tuned &&
        meta.base_model_name.find(target_model_name) == std::string::npos &&
        target_model_name.find(meta.base_model_name) == std::string::npos) {
        result.passed = true;
        result.rebuild_required = true;
        result.message = "PromptFormat: instruction-tuned adapter may require "
                         "prompt-template adjustment for '" + target_model_name + "'";
    } else {
        result.passed = true;
        result.message = "PromptFormat: no prompt-format mismatch detected";
    }
    return result;
}

// ---------------------------------------------------------------------------
// Rebuild policy evaluation
// ---------------------------------------------------------------------------

ModelSwitchOutcome ModelSwitchWorkflow::evaluateRebuildPolicy(
    const std::vector<ModelSwitchCheckResult>& checks,
    const RebuildPolicy& policy,
    std::vector<RebuildTrigger>& active_triggers) {
    active_triggers.clear();

    bool any_hard_failure = false;
    bool any_rebuild_needed = false;

    for (const auto& check : checks) {
        if (!check.passed) {
            any_hard_failure = true;
        }
        if (check.rebuild_required) {
            any_rebuild_needed = true;

            // Map check kind to rebuild trigger
            switch (check.kind) {
                case ModelSwitchCheckResult::CheckKind::ARCHITECTURE:
                    if (policy.isTriggerActive(RebuildTrigger::ARCHITECTURE_CHANGE)) {
                        active_triggers.push_back(RebuildTrigger::ARCHITECTURE_CHANGE);
                    }
                    break;
                case ModelSwitchCheckResult::CheckKind::TOKENIZER:
                    if (policy.isTriggerActive(RebuildTrigger::TOKENIZER_CHANGE)) {
                        active_triggers.push_back(RebuildTrigger::TOKENIZER_CHANGE);
                    }
                    break;
                case ModelSwitchCheckResult::CheckKind::LAYER_DIMENSIONS:
                    if (policy.isTriggerActive(RebuildTrigger::LAYER_DIMENSION_CHANGE)) {
                        active_triggers.push_back(RebuildTrigger::LAYER_DIMENSION_CHANGE);
                    }
                    break;
                case ModelSwitchCheckResult::CheckKind::RATCHET_MATRIX:
                    if (policy.isTriggerActive(RebuildTrigger::VERSION_OUT_OF_RANGE)) {
                        active_triggers.push_back(RebuildTrigger::VERSION_OUT_OF_RANGE);
                    }
                    break;
                default:
                    break;
            }
        }
    }

    // Deduplicate triggers (preserve insertion order)
    {
        std::vector<RebuildTrigger> seen;
        std::vector<RebuildTrigger> deduped;
        for (const auto t : active_triggers) {
            if (std::find(seen.begin(), seen.end(), t) == seen.end()) {
                seen.push_back(t);
                deduped.push_back(t);
            }
        }
        active_triggers = std::move(deduped);
    }

    if (any_hard_failure) {
        return ModelSwitchOutcome::INCOMPATIBLE;
    }
    if (any_rebuild_needed && !active_triggers.empty()) {
        return policy.fail_closed_on_rebuild ? ModelSwitchOutcome::BLOCKED
                                             : ModelSwitchOutcome::REBUILD_REQUIRED;
    }
    return ModelSwitchOutcome::COMPATIBLE;
}

// ---------------------------------------------------------------------------
// ModelSwitchWorkflow::executeSwitch
// ---------------------------------------------------------------------------

ModelSwitchResult ModelSwitchWorkflow::executeSwitch(const ModelSwitchRequest& request) const {
    ModelSwitchResult result;
    result.correlation_id = request.correlation_id;

    if (!isSwitchRequired(request)) {
        result.outcome = ModelSwitchOutcome::COMPATIBLE;
        result.warnings.push_back(
            "Source and target model are identical; no switch required "
            "(us[[maybe_unused]] e force_revalidatio[[maybe_unused]] n=tru[[maybe_unused]] e t[[maybe_unused]] o overrid[[maybe_unused]] e)");
        return result;
    }

    // Resolve the package to get adapter IDs
    const auto packages = orchestrator_->listPackages();
    const FinalLayerPackage* pkg = nullptr;
    for (const auto& p : packages) {
        if (p.package_id == request.package_id) {
            pkg = &p;
            break;
        }
    }

    if (!pkg) {
        result.outcome = ModelSwitchOutcome::INCOMPATIBLE;
        result.errors.push_back(
            "Package '" + request.package_id + "' not found in orchestrator");
        return result;
    }

    const std::string& primary_id = pkg->primary_adapter_id;

    // --- Step 1: Ratchet matrix check
    result.checks.push_back(checkRatchetMatrix(
        primary_id, request.target_model_family, request.target_model_version));

    // If ratchet check fails it is a hard incompatibility — skip remaining checks
    if (!result.checks.back().passed) {
        result.outcome = ModelSwitchOutcome::INCOMPATIBLE;
        result.errors.push_back(result.checks.back().message);
        return result;
    }

    // --- Step 2: Architecture compatibility
    result.checks.push_back(checkArchitectureCompatibility(
        primary_id, request.target_model_name, request.target_model_family,
        request.target_model_version));

    // --- Step 3: Tokenizer
    result.checks.push_back(
        checkTokenizerCompatibility(
            primary_id, request.target_model_name, request.target_model_family));

    // --- Step 4: Layer dimensions
    result.checks.push_back(
        checkLayerDimensions(primary_id, request.target_model_name));

    // --- Step 5: Quantization
    result.checks.push_back(checkQuantizationCompatibility(
        primary_id, request.target_model_name, request.target_model_family,
        request.target_model_version));

    // --- Step 6: Prompt format
    result.checks.push_back(
        checkPromptFormat(primary_id, request.target_model_name));

    // --- Step 7: Draft adapter checks (best-effort, warnings only)
    if (!pkg->draft_adapter_id.empty()) {
        auto draft_arch = checkArchitectureCompatibility(
            pkg->draft_adapter_id, request.target_model_name,
            request.target_model_family, request.target_model_version);
        if (!draft_arch.passed) {
            result.warnings.push_back(
                "Draft adapter '" + pkg->draft_adapter_id +
                "' may be incompatible with target model — rebuild recommended");
        }
    }

    // --- Step 8: Evaluate rebuild policy and determine outcome
    result.outcome = evaluateRebuildPolicy(
        result.checks, policy_, result.active_rebuild_triggers);

    // Collect errors from hard failures
    for (const auto& c : result.checks) {
        if (!c.passed) {
            result.errors.push_back(c.message);
        }
    }

    // --- Step 9: On COMPATIBLE outcome, trigger orchestrator promotion
    if (result.outcome == ModelSwitchOutcome::COMPATIBLE) {
        const bool promoted = orchestrator_->promotePackage(
            request.package_id,
            FinalLayerDeploymentStage::STAGING,
            request.target_model_name,
            request.target_model_version);

        if (!promoted) {
            // Promotion failure is a warning, not a blocking error; the
            // compatibility verdict still stands.  The package may already be
            // in a non-promotable state (e.g., already in PRODUCTION).
            result.warnings.push_back(
                "Compatibility confirmed but orchestrator promotion to STAGING failed — "
                "package may already be at target stage or DISABLED");
        }
    }

    return result;
}

} // namespace llm
} // namespace themis
