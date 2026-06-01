/*
 * ThemisDB | File: constitutional_reasoning_engine.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 699
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=17, H=12, M=11, L=0
 * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file constitutional_reasoning_engine.cpp
 * @brief Implementation of constitutional reasoning engine
 */

#include "llm/constitutional_reasoning_engine.h"
#include <algorithm>
#include <sstream>
#include <regex>
#include <mutex>

namespace themis {
namespace llm {

// ═══════════════════════════════════════════════════════════
// Implementation details
// ═══════════════════════════════════════════════════════════

const ConstitutionalReasoningEngine::PromptRunner* asPromptRunner(void* llm_wrapper) {
    return static_cast<const ConstitutionalReasoningEngine::PromptRunner*>(llm_wrapper);
}

std::string invokePromptRunner(void* llm_wrapper, const std::string& prompt) {
    const auto* runner = asPromptRunner(llm_wrapper);
    if (runner == nullptr || !(*runner)) {
        return {};
    }
    return (*runner)(prompt);
}

struct ConstitutionalReasoningEngine::Impl {
    ConstitutionalReasoningConfig config;
    mutable Statistics stats;
    mutable std::mutex mutex;
    
    // Cache for critiques
    std::unordered_map<std::string, std::vector<std::string>> critique_cache;
    
    // Callback
    std::function<void(const ConstitutionalReasoningResult&)> callback;
};

// ═══════════════════════════════════════════════════════════
// Constructor / Destructor
// ═══════════════════════════════════════════════════════════

ConstitutionalReasoningEngine::ConstitutionalReasoningEngine(
    const ConstitutionalReasoningConfig& config
) : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    
    // Load default principles if none provided
    if (impl_->config.principles.empty()) {
        loadDefaultPrinciples();
    }
}

ConstitutionalReasoningEngine::~ConstitutionalReasoningEngine() = default;

// ═══════════════════════════════════════════════════════════
// Core functionality
// ═══════════════════════════════════════════════════════════

ConstitutionalReasoningResult ConstitutionalReasoningEngine::reason(
    const std::string& response,
    const std::string& query,
    void* llm_wrapper
) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->stats.total_reasonings++;
    
    auto start = std::chrono::steady_clock::now();
    
    ConstitutionalReasoningResult result;
    result.original_response = response;
    result.revised_response = response;
    result.was_revised = false;
    result.iterations = 0;
    
    // Score original response
    result.original_score = scoreResponse(response);
    result.revised_score = result.original_score;
    
    // Check for violations
    result.violated_principles = checkViolations(response);
    
    if (!impl_->config.enable_self_critique || result.violated_principles.empty()) {
        // No issues detected or critique disabled
        result.improvement = 0.0f;
        return result;
    }
    
    // Iterative critique and revision
    std::string current_response = response;
    float current_score = result.original_score;
    
    for (int iter = 0; iter < impl_->config.max_iterations; ++iter) {
        result.iterations = iter + 1;
        
        // Generate critiques for each violated principle
        auto critique_start = std::chrono::steady_clock::now();
        std::vector<std::string> critiques;
        
        for (const auto& principle_id : result.violated_principles) {
            // Find the principle
            auto it = std::find_if(
                impl_->config.principles.begin(),
                impl_->config.principles.end(),
                [&](const ConstitutionalPrinciple& p) { return p.id == principle_id; }
            );
            
            if (it != impl_->config.principles.end()) {
                std::string critique = generateCritique(
                    current_response,
                    query,
                    *it,
                    llm_wrapper
                );
                if (!critique.empty()) {
                    critiques.push_back(critique);
                }
            }
        }
        
        auto critique_end = std::chrono::steady_clock::now();
        result.critique_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            critique_end - critique_start
        );
        
        result.critiques = critiques;
        
        // Generate revision if critiques were found
        if (!critiques.empty() && impl_->config.enable_self_revision) {
            auto revision_start = std::chrono::steady_clock::now();
            
            std::string revised = generateRevision(
                current_response,
                critiques,
                query,
                llm_wrapper
            );
            
            auto revision_end = std::chrono::steady_clock::now();
            result.revision_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                revision_end - revision_start
            );
            
            // Score revised response
            float revised_score = scoreResponse(revised);
            
            // Check if improvement is sufficient
            float improvement = revised_score - current_score;
            if (improvement >= impl_->config.improvement_threshold) {
                current_response = revised;
                current_score = revised_score;
                result.revised_response = revised;
                result.was_revised = true;
                result.revised_score = revised_score;
                result.improvement = revised_score - result.original_score;
                impl_->stats.revisions_performed++;
                
                // Check new violations
                result.violated_principles = checkViolations(current_response);
                
                // Stop if no more violations
                if (result.violated_principles.empty()) {
                    break;
                }
            } else {
                // No significant improvement, stop iterating
                break;
            }
        } else {
            break;
        }
        
        // Check if we should continue
        if (!shouldContinueIterating(result, iter + 1)) {
            break;
        }
    }
    
    // Generate revision reasoning
    if (result.was_revised) {
        std::ostringstream oss;
        oss << "Revised to address " << result.critiques.size() << " critique(s). ";
        oss << "Improvement: " << (result.improvement * 100) << "%. ";
        oss << "Iterations: " << result.iterations << ".";
        result.revision_reasoning = oss.str();
    }
    
    // Track applied principles (those that were satisfied)
    for (const auto& principle : impl_->config.principles) {
        auto it = std::find(
            result.violated_principles.begin(),
            result.violated_principles.end(),
            principle.id
        );
        if (it == result.violated_principles.end()) {
            result.applied_principles.push_back(principle.id);
            impl_->stats.principle_applications[principle.id]++;
        }
    }
    
    // Update statistics
    updateStatistics(result);
    
    // Call callback if set
    if (impl_->callback) {
        impl_->callback(result);
    }
    
    return result;
}

std::string ConstitutionalReasoningEngine::generateCritique(
    const std::string& response,
    const std::string& query,
    const ConstitutionalPrinciple& principle,
    void* llm_wrapper
) {
    // Check cache
    std::string cache_key = response + "|" + principle.id;
    if (impl_->config.cache_critiques) {
        auto it = impl_->critique_cache.find(cache_key);
        if (it != impl_->critique_cache.end() && !it->second.empty()) {
            impl_->stats.cache_hits++;
            return it->second[0];
        }
        impl_->stats.cache_misses++;
    }
    
    // Build critique prompt
    std::string prompt = buildCritiquePrompt(response, query, principle);

    const std::string llm_critique = invokePromptRunner(llm_wrapper, prompt);
    if (!llm_critique.empty()) {
        if (impl_->config.cache_critiques) {
            impl_->critique_cache[cache_key] = {llm_critique};
        }
        return llm_critique;
    }
    
    // Generate critique using rule-based detection
    // This provides fast, deterministic critique generation without LLM overhead
    std::string critique;
    
    if (principle.id == "human_autonomy") {
        // Check for patronizing language
        std::string lower_response = response;
        std::transform(lower_response.begin(), lower_response.end(), 
                      lower_response.begin(), ::tolower);
        
        if (lower_response.find("you must") != std::string::npos ||
            lower_response.find("you should") != std::string::npos ||
            lower_response.find("you have to") != std::string::npos) {
            critique = "Response contains directive language that may undermine human autonomy. "
                      "Consider rephrasing to present options rather than commands.";
        }
    } else if (principle.id == "transparency") {
        // Check for uncertainty acknowledgment
        std::string lower_response = response;
        std::transform(lower_response.begin(), lower_response.end(), 
                      lower_response.begin(), ::tolower);
        
        if (lower_response.find("may") == std::string::npos &&
            lower_response.find("might") == std::string::npos &&
            lower_response.find("could") == std::string::npos &&
            lower_response.find("possibly") == std::string::npos) {
            critique = "Response presents information with absolute certainty. "
                      "Consider acknowledging potential limitations or uncertainties.";
        }
    }
    
    // Cache the critique
    if (impl_->config.cache_critiques && !critique.empty()) {
        impl_->critique_cache[cache_key] = {critique};
    }
    
    return critique;
}

std::string ConstitutionalReasoningEngine::generateRevision(
    const std::string& response,
    const std::vector<std::string>& critiques,
    const std::string& query,
    void* llm_wrapper
) {
    // Build revision prompt
    std::string prompt = buildRevisionPrompt(response, critiques, query);

    const std::string llm_revision = invokePromptRunner(llm_wrapper, prompt);
    if (!llm_revision.empty()) {
        return llm_revision;
    }
    
    // Apply rule-based revisions
    // This provides deterministic, fast revision without LLM overhead
    std::string revised = response;
    
    // Replace directive language with suggestions
    std::regex must_pattern("(you must|you have to)", std::regex::icase);
    revised = std::regex_replace(revised, must_pattern, "you might consider");
    
    std::regex should_pattern("you should", std::regex::icase);
    revised = std::regex_replace(revised, should_pattern, "you could");
    
    // Add uncertainty acknowledgment if needed
    if (revised.find("may") == std::string::npos &&
        revised.find("might") == std::string::npos &&
        revised.find("could") == std::string::npos) {
        revised += " Please note that this information may have limitations, "
                   "and you should consult relevant experts for specific guidance.";
    }
    
    return revised;
}

std::vector<std::string> ConstitutionalReasoningEngine::checkViolations(
    const std::string& response
) {
    std::vector<std::string> violations;
    
    // Check each principle
    for (const auto& principle : impl_->config.principles) {
        bool violated = false;
        
        if (principle.id == "human_autonomy") {
            violated = !checkAutonomyRespect(response);
        } else if (principle.id == "transparency") {
            violated = !checkTransparency(response);
        } else if (principle.id == "do_no_harm") {
            violated = !checkNonHarmfulness(response);
        } else if (principle.id == "fairness") {
            violated = !checkFairness(response);
        }
        
        if (violated) {
            violations.push_back(principle.id);
            impl_->stats.violations_detected++;
            impl_->stats.principle_violations[principle.id]++;
        }
    }
    
    return violations;
}

float ConstitutionalReasoningEngine::scoreResponse(const std::string& response) {
    // Score based on principle compliance
    auto violations = checkViolations(response);
    
    if (impl_->config.principles.empty()) {
        return 1.0f;
    }
    
    float compliance_rate = 1.0f - (static_cast<float>(violations.size()) / 
                                   impl_->config.principles.size());
    
    return std::max(0.0f, std::min(1.0f, compliance_rate));
}

// ═══════════════════════════════════════════════════════════
// Principle management
// ═══════════════════════════════════════════════════════════

void ConstitutionalReasoningEngine::addPrinciple(const ConstitutionalPrinciple& principle) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config.principles.push_back(principle);
}

void ConstitutionalReasoningEngine::removePrinciple(const std::string& principle_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto& principles = impl_->config.principles;
    principles.erase(
        std::remove_if(
            principles.begin(),
            principles.end(),
            [&](const ConstitutionalPrinciple& p) { return p.id == principle_id; }
        ),
        principles.end()
    );
}

std::vector<ConstitutionalPrinciple> ConstitutionalReasoningEngine::getPrinciples() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->config.principles;
}

void ConstitutionalReasoningEngine::loadDefaultPrinciples() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config.principles.clear();
    
    // Principle 1: Human Autonomy (UN HR Art. 1, Asimov's 2nd Law)
    ConstitutionalPrinciple autonomy;
    autonomy.id = "human_autonomy";
    autonomy.name = "Respects human dignity and autonomy";
    autonomy.description = "The AI supports human decisions but never replaces them. "
                          "Does not use patronizing or commanding language.";
    autonomy.priority = 1;
    autonomy.critique_prompt = "Does this response respect human autonomy and avoid "
                              "patronizing or commanding language?";
    autonomy.revision_prompt = "Revise to present options and information rather than "
                              "commands, while respecting human agency.";
    autonomy.source = "UN Human Rights Art. 1, Asimov's 2nd Law (adapted)";
    autonomy.domain_agnostic = true;
    impl_->config.principles.push_back(autonomy);
    
    // Principle 2: Non-Discrimination (UN HR Art. 2)
    ConstitutionalPrinciple fairness;
    fairness.id = "fairness";
    fairness.name = "Does not discriminate";
    fairness.description = "Treats all people equally regardless of background, identity, "
                          "or personal characteristics.";
    fairness.priority = 1;
    fairness.critique_prompt = "Does this response treat all people fairly without bias "
                              "or discrimination?";
    fairness.revision_prompt = "Revise to ensure equal and fair treatment of all people.";
    fairness.source = "UN Human Rights Art. 2";
    fairness.domain_agnostic = true;
    impl_->config.principles.push_back(fairness);
    
    // Principle 3: Transparency (Core Ethical Principle)
    ConstitutionalPrinciple transparency;
    transparency.id = "transparency";
    transparency.name = "Provides transparent reasoning";
    transparency.description = "Acknowledges limitations, uncertainties, and sources. "
                              "Makes reasoning process clear.";
    transparency.priority = 1;
    transparency.critique_prompt = "Does this response acknowledge limitations and "
                                  "provide transparent reasoning?";
    transparency.revision_prompt = "Revise to include acknowledgment of limitations "
                                  "and uncertainty where appropriate.";
    transparency.source = "Core Ethical Principle";
    transparency.domain_agnostic = true;
    impl_->config.principles.push_back(transparency);
    
    // Principle 4: Do No Harm (Asimov's 1st Law, Medical Ethics)
    ConstitutionalPrinciple harm;
    harm.id = "do_no_harm";
    harm.name = "Does no intentional harm";
    harm.description = "Avoids providing harmful information or advice that could "
                      "cause physical, emotional, or social harm.";
    harm.priority = 1;
    harm.critique_prompt = "Could this response potentially cause harm to the user "
                          "or others?";
    harm.revision_prompt = "Revise to remove potentially harmful content and add "
                          "appropriate warnings if needed.";
    harm.source = "Asimov's 1st Law, Medical Ethics (Primum non nocere)";
    harm.domain_agnostic = true;
    impl_->config.principles.push_back(harm);
}

// ═══════════════════════════════════════════════════════════
// Configuration
// ═══════════════════════════════════════════════════════════

void ConstitutionalReasoningEngine::setConfig(const ConstitutionalReasoningConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config = config;
}

ConstitutionalReasoningConfig ConstitutionalReasoningEngine::getConfig() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->config;
}

void ConstitutionalReasoningEngine::clearCache() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->critique_cache.clear();
}

// ═══════════════════════════════════════════════════════════
// Statistics
// ═══════════════════════════════════════════════════════════

ConstitutionalReasoningEngine::Statistics ConstitutionalReasoningEngine::getStatistics() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->stats;
}

void ConstitutionalReasoningEngine::resetStatistics() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->stats = Statistics();
}

void ConstitutionalReasoningEngine::setReasoningCallback(
    std::function<void(const ConstitutionalReasoningResult&)> callback
) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->callback = callback;
}

// ═══════════════════════════════════════════════════════════
// Helper methods
// ═══════════════════════════════════════════════════════════

std::string ConstitutionalReasoningEngine::buildCritiquePrompt(
    const std::string& response,
    const std::string& query,
    const ConstitutionalPrinciple& principle
) {
    std::ostringstream oss;
    oss << "Critique the following response based on the principle: " 
        << principle.name << "\n\n";
    oss << "Principle: " << principle.description << "\n\n";
    oss << "User Query: " << query << "\n\n";
    oss << "Response: " << response << "\n\n";
    oss << principle.critique_prompt << "\n";
    return oss.str();
}

std::string ConstitutionalReasoningEngine::buildRevisionPrompt(
    const std::string& response,
    const std::vector<std::string>& critiques,
    const std::string& query
) {
    std::ostringstream oss;
    oss << "Revise the following response based on these critiques:\n\n";
    oss << "Original Response: " << response << "\n\n";
    oss << "Critiques:\n";
    for (size_t i = 0; i < critiques.size(); ++i) {
        oss << (i + 1) << ". " << critiques[i] << "\n";
    }
    oss << "\nProvide a revised response that addresses these critiques while "
        << "maintaining relevance to the original query: " << query << "\n";
    return oss.str();
}

bool ConstitutionalReasoningEngine::shouldContinueIterating(
    const ConstitutionalReasoningResult& result,
    int iteration
) {
    // Stop if max iterations reached
    if (iteration >= impl_->config.max_iterations) {
        return false;
    }
    
    // Stop if no violations remain
    if (result.violated_principles.empty()) {
        return false;
    }
    
    // Stop if score is acceptable
    if (result.revised_score >= impl_->config.min_acceptable_score) {
        return false;
    }
    
    return true;
}

void ConstitutionalReasoningEngine::updateStatistics(
    const ConstitutionalReasoningResult& result
) {
    if (impl_->stats.total_reasonings == 0) {
        impl_->stats.avg_original_score = result.original_score;
        impl_->stats.avg_revised_score = result.revised_score;
        impl_->stats.avg_improvement = result.improvement;
        impl_->stats.avg_critique_time = result.critique_time;
        impl_->stats.avg_revision_time = result.revision_time;
    } else {
        float n = static_cast<float>(impl_->stats.total_reasonings);
        impl_->stats.avg_original_score = 
            (impl_->stats.avg_original_score * (n - 1) + result.original_score) / n;
        impl_->stats.avg_revised_score = 
            (impl_->stats.avg_revised_score * (n - 1) + result.revised_score) / n;
        impl_->stats.avg_improvement = 
            (impl_->stats.avg_improvement * (n - 1) + result.improvement) / n;
        
        auto critique_ms = impl_->stats.avg_critique_time.count() * (n - 1) + 
                          result.critique_time.count();
        impl_->stats.avg_critique_time = std::chrono::milliseconds(
            static_cast<int64_t>(critique_ms / n)
        );
        
        auto revision_ms = impl_->stats.avg_revision_time.count() * (n - 1) + 
                          result.revision_time.count();
        impl_->stats.avg_revision_time = std::chrono::milliseconds(
            static_cast<int64_t>(revision_ms / n)
        );
    }
}

// ═══════════════════════════════════════════════════════════
// Violation detection helpers
// ═══════════════════════════════════════════════════════════

bool ConstitutionalReasoningEngine::checkAutonomyRespect(const std::string& response) {
    std::string lower = response;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // Check for patronizing patterns
    std::vector<std::string> patronizing = {
        "you must", "you have to", "you should", "it is your duty",
        "obviously you", "clearly you must"
    };
    
    for (const auto& pattern : patronizing) {
        if (lower.find(pattern) != std::string::npos) {
            return false;
        }
    }
    
    return true;
}

bool ConstitutionalReasoningEngine::checkTransparency(const std::string& response) {
    std::string lower = response;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // Check for hedge words indicating transparency
    std::vector<std::string> hedge_words = {
        "may", "might", "could", "possibly", "perhaps", "likely",
        "it seems", "appears to", "suggests"
    };
    
    for (const auto& word : hedge_words) {
        if (lower.find(word) != std::string::npos) {
            return true;
        }
    }
    
    // Also acceptable if mentions limitations or expertise
    std::vector<std::string> limitation_phrases = {
        "consult", "expert", "specialist", "professional",
        "limited information", "based on available"
    };
    
    for (const auto& phrase : limitation_phrases) {
        if (lower.find(phrase) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool ConstitutionalReasoningEngine::checkNonHarmfulness(const std::string& response) {
    std::string lower = response;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // Check for harmful patterns (simplified)
    std::vector<std::string> harmful_patterns = {
        "self-harm", "suicide", "violence", "illegal activity",
        "dangerous", "harm yourself", "hurt yourself"
    };
    
    for (const auto& pattern : harmful_patterns) {
        if (lower.find(pattern) != std::string::npos) {
            // Found potentially harmful content
            // Check if it's in a warning context
            if (lower.find("do not") != std::string::npos ||
                lower.find("avoid") != std::string::npos ||
                lower.find("warning") != std::string::npos) {
                continue; // Warnings are okay
            }
            return false;
        }
    }
    
    return true;
}

bool ConstitutionalReasoningEngine::checkFairness(const std::string& response) {
    std::string lower = response;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // Check for discriminatory patterns (simplified)
    std::vector<std::string> bias_patterns = {
        "all [group] are", "typical [group]", "[group] always",
        "inferior", "superior race", "less intelligent"
    };
    
    for (const auto& pattern : bias_patterns) {
        if (lower.find(pattern) != std::string::npos) {
            return false;
        }
    }
    
    return true;
}

// ═══════════════════════════════════════════════════════════
// Factory methods
// ═══════════════════════════════════════════════════════════

std::unique_ptr<ConstitutionalReasoningEngine> ConstitutionalReasoningFactory::createDefault() {
    return std::make_unique<ConstitutionalReasoningEngine>();
}

std::unique_ptr<ConstitutionalReasoningEngine> ConstitutionalReasoningFactory::createStrict() {
    ConstitutionalReasoningConfig config;
    config.max_iterations = 5;
    config.improvement_threshold = 0.02f;
    config.min_acceptable_score = 0.85f;
    config.require_all_principles = true;
    return std::make_unique<ConstitutionalReasoningEngine>(config);
}

std::unique_ptr<ConstitutionalReasoningEngine> ConstitutionalReasoningFactory::createLenient() {
    ConstitutionalReasoningConfig config;
    config.max_iterations = 2;
    config.improvement_threshold = 0.1f;
    config.min_acceptable_score = 0.60f;
    config.require_all_principles = false;
    return std::make_unique<ConstitutionalReasoningEngine>(config);
}

std::unique_ptr<ConstitutionalReasoningEngine> ConstitutionalReasoningFactory::create(
    const ConstitutionalReasoningConfig& config
) {
    return std::make_unique<ConstitutionalReasoningEngine>(config);
}

} // namespace llm
} // namespace themis
