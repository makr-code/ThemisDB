/**
 * @file constitutional_reasoning_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=14, H=7, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    mutable std::recursive_mutex mutex;
    
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
    std::lock_guard<std::recursive_mutex> lock(impl_->mutex);
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
                [&]([[maybe_unused]] const ConstitutionalPrinciple& p) { return p.id == principle_id; }
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
    if ([[maybe_unused]] impl_->callback) {
        impl_->callback([[maybe_unused]] result);
    }
    
    return result;
}

std::string ConstitutionalReasoningEngine::generateCritique(
    const std::string& response,
    const std::string& query,
    const ConstitutionalPrinciple& principle,
    void* llm_wrapper
) {
    // Lock guards both the direct public-API path and the re-entrant path
    // from reason() (which also holds this mutex). std::recursive_mutex allows
    // the same thread to acquire the lock multiple times without deadlock.
    std::lock_guard<std::recursive_mutex> lock(impl_->mutex);
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
    std::lock_guard<std::recursive_mutex> lock(impl_->mutex);
    impl_->config.principles.push_back(principle);
}

void ConstitutionalReasoningEngine::removePrinciple(const std::string& principle_id) {
    std::lock_guard<std::recursive_mutex> lock(impl_->mutex);
    auto& principles = impl_->config.principles;
    principles.erase(
        std::remove_if(
            principles.begin(),
            principles.end(),
            [&]([[maybe_unused]] const ConstitutionalPrinciple& p) { return p.id == principle_id; }
        ),
        principles.end()
    );
}

std::vector<ConstitutionalPrinciple> ConstitutionalReasoningEngine::getPrinciples() const {
    std::lock_guard<std::recursive_mutex> lock(impl_->mutex);
    return impl_->config.principles;
}

void ConstitutionalReasoningEngine::loadDefaultPrinciples() {
    std::lock_guard<std::recursive_mutex> lock(impl_->mutex);
    impl_->config.principles.clear();

    const auto add = [&](const char* id,
                         const char* name,
                         const char* description,
                         const char* critique_prompt,
                         const char* revision_prompt,
                         const char* source,
                         int priority) {
        ConstitutionalPrinciple principle;
        principle.id = id;
        principle.name = name;
        principle.description = description;
        principle.priority = priority;
        principle.critique_prompt = critique_prompt;
        principle.revision_prompt = revision_prompt;
        principle.source = source;
        principle.domain_agnostic = true;
        impl_->config.principles.push_back(principle);
    };

    // Core safety principles used by deterministic checks.
    add("human_autonomy",
        "Respects human dignity and autonomy",
        "Supports human decision-making without coercive directives.",
        "Does this response respect human autonomy and avoid commanding language?",
        "Revise to present options and preserve user agency.",
        "UN Human Rights Art. 1, Asimov's 2nd Law (adapted)",
        3);

    add("fairness",
        "Does not discriminate",
        "Avoids discriminatory language and treats groups equitably.",
        "Does this response avoid biased or discriminatory treatment?",
        "Revise to ensure fair and equal treatment across groups.",
        "UN Human Rights Art. 2",
        3);

    add("transparency",
        "Provides transparent reasoning",
        "Acknowledges uncertainty and clarifies limitations where relevant.",
        "Does this response communicate limitations and uncertainty transparently?",
        "Revise to include transparent caveats and known limits.",
        "Core Ethical Principle",
        3);

    add("do_no_harm",
        "Does no intentional harm",
        "Avoids instructions or advice that can cause direct harm.",
        "Could this response plausibly cause physical, emotional, or social harm?",
        "Revise to remove harmful guidance and include safer alternatives.",
        "Asimov's 1st Law, Medical Ethics (Primum non nocere)",
        3);

    // Additional constitutional principles to satisfy Wave C C1 registry scope.
    add("privacy_protection",
        "Protects privacy",
        "Avoids exposing, inferring, or encouraging misuse of private data.",
        "Does this response protect personal and confidential information?",
        "Revise to remove private-data exposure and add privacy-safe guidance.",
        "OECD Privacy Guidelines",
        2);

    add("consent_and_agency",
        "Requires informed consent",
        "Encourages explicit consent before sensitive actions or data use.",
        "Does this response respect informed consent for sensitive operations?",
        "Revise to require explicit consent before sensitive processing.",
        "Belmont Report",
        2);

    add("lawful_compliance",
        "Promotes lawful behavior",
        "Avoids facilitating illegal activity and recommends compliant paths.",
        "Does this response avoid enabling illegal conduct?",
        "Revise to refuse illegal assistance and offer lawful alternatives.",
        "General Legal Compliance",
        2);

    add("security_hardening",
        "Supports secure practices",
        "Encourages secure defaults and discourages exploitative behavior.",
        "Does this response avoid weakening security posture?",
        "Revise to prefer secure defaults and safe operational controls.",
        "NIST Secure Design Principles",
        2);

    add("misuse_resistance",
        "Resists misuse",
        "Avoids dual-use escalation and reduces abuse potential.",
        "Could this response be easily repurposed for misuse?",
        "Revise to reduce misuse potential and include defensive framing.",
        "Responsible AI Safety",
        2);

    add("factual_reliability",
        "Prioritizes factual reliability",
        "Distinguishes facts from assumptions and avoids fabricated claims.",
        "Does this response avoid unverifiable or fabricated statements?",
        "Revise to separate verified facts from assumptions.",
        "Scientific Integrity",
        2);

    add("source_traceability",
        "Encourages source traceability",
        "Provides provenance cues when factual claims are made.",
        "Does this response provide source traceability for key claims?",
        "Revise to add provenance cues or confidence qualifiers.",
        "Research Reproducibility Norms",
        1);

    add("uncertainty_calibration",
        "Calibrates uncertainty",
        "Avoids overconfidence and calibrates confidence to evidence strength.",
        "Is confidence level calibrated to available evidence?",
        "Revise to calibrate certainty to evidence quality.",
        "Probabilistic Reasoning Best Practices",
        2);

    add("non_manipulation",
        "Avoids manipulation",
        "Does not exploit vulnerabilities, fear, or deception.",
        "Does this response avoid manipulative framing or coercion?",
        "Revise to remove manipulative language and preserve autonomy.",
        "Ethics of Persuasion",
        2);

    add("respectful_tone",
        "Maintains respectful tone",
        "Avoids demeaning, harassing, or inflammatory language.",
        "Does this response remain respectful and non-abusive?",
        "Revise to maintain respectful, non-hostile language.",
        "Professional Conduct Standards",
        1);

    add("vulnerability_protection",
        "Protects vulnerable populations",
        "Avoids content that targets or harms vulnerable groups.",
        "Does this response safeguard vulnerable users and populations?",
        "Revise to avoid harmful targeting of vulnerable groups.",
        "UN Human Rights Protection Norms",
        2);

    add("age_appropriate_safety",
        "Supports age-appropriate safety",
        "Avoids unsafe guidance for minors and sensitive audiences.",
        "Is this response safe and appropriate for younger audiences?",
        "Revise to enforce age-appropriate safeguards.",
        "Child Safety Guidelines",
        1);

    add("medical_caution",
        "Uses medical caution",
        "Avoids definitive medical diagnosis/treatment claims without caveats.",
        "Does this response avoid unsafe medical certainty?",
        "Revise to include caution and professional-care recommendations.",
        "Medical Ethics",
        1);

    add("financial_caution",
        "Uses financial caution",
        "Avoids guaranteed financial outcomes or reckless investment advice.",
        "Does this response avoid unsafe or guaranteed financial claims?",
        "Revise to include risk disclosure and avoid guarantees.",
        "Consumer Financial Protection Principles",
        1);

    add("escalation_prevention",
        "Prevents harmful escalation",
        "Avoids advice that escalates conflict or harm.",
        "Could this response escalate conflict or dangerous behavior?",
        "Revise to de-escalate and propose safer alternatives.",
        "Conflict De-escalation Principles",
        2);

    add("accountability",
        "Supports accountability",
        "Makes constraints explicit and avoids hiding policy boundaries.",
        "Does this response communicate boundaries and accountability clearly?",
        "Revise to state boundaries and rationale transparently.",
        "AI Governance Principles",
        1);

    add("robustness_under_ambiguity",
        "Handles ambiguity robustly",
        "Requests clarification when user intent is unsafe or unclear.",
        "Does this response request clarification for ambiguous risky intent?",
        "Revise to ask clarifying questions before proceeding.",
        "Safety-by-Design",
        1);
}

// ═══════════════════════════════════════════════════════════
// Configuration
// ═══════════════════════════════════════════════════════════

void ConstitutionalReasoningEngine::setConfig(const ConstitutionalReasoningConfig& config) {
    std::lock_guard<std::recursive_mutex> lock(impl_->mutex);
    impl_->config = config;
}

ConstitutionalReasoningConfig ConstitutionalReasoningEngine::getConfig() const {
    std::lock_guard<std::recursive_mutex> lock(impl_->mutex);
    return impl_->config;
}

void ConstitutionalReasoningEngine::clearCache() {
    std::lock_guard<std::recursive_mutex> lock(impl_->mutex);
    impl_->critique_cache.clear();
}

// ═══════════════════════════════════════════════════════════
// Statistics
// ═══════════════════════════════════════════════════════════

ConstitutionalReasoningEngine::Statistics ConstitutionalReasoningEngine::getStatistics() const {
    std::lock_guard<std::recursive_mutex> lock(impl_->mutex);
    return impl_->stats;
}

void ConstitutionalReasoningEngine::resetStatistics() {
    std::lock_guard<std::recursive_mutex> lock(impl_->mutex);
    impl_->stats = Statistics();
}

void ConstitutionalReasoningEngine::setReasoningCallback(
    std::function<void(const ConstitutionalReasoningResult&)> callback
) {
    std::lock_guard<std::recursive_mutex> lock(impl_->mutex);
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

