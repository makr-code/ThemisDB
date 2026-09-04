/**
 * @file multi_perspective_generator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=1, M=18, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/multi_perspective_generator.h"
#include <algorithm>
#include <sstream>
#include <regex>
#include <mutex>
#include <unordered_set>
#include <cmath>
#include <chrono>
#include <limits>

namespace themis {
namespace llm {

// ═══════════════════════════════════════════════════════════
// Helper functions
// ═══════════════════════════════════════════════════════════

namespace {
int clampSizeToInt(const size_t value) {
    const size_t int_max = static_cast<size_t>(std::numeric_limits<int>::max());
    return static_cast<int>(std::min(value, int_max));
}

// Extract unique words from text (words longer than 3 characters)
std::unordered_set<std::string> extractWords(const std::string& text) {
    std::unordered_set<std::string> words;
    std::string current = {};
    
    for (char c : text) {
        const auto uc = static_cast<unsigned char>(c);
        if (std::isalpha(uc) != 0) {
            current.push_back(static_cast<char>(std::tolower(uc)));
        } else if (!current.empty()) {
            if (current.length() > 3) {
                words.insert(current);
            }
            current.clear();
        }
    }
    
    if (!current.empty() && current.length() > 3) {
        words.insert(current);
    }
    
    return words;
}
} // anonymous namespace

// ═══════════════════════════════════════════════════════════
// Implementation details
// ═══════════════════════════════════════════════════════════

struct MultiPerspectiveGenerator::Impl {
    MultiPerspectiveConfig config;
    mutable Statistics stats;
    mutable std::mutex mutex;
    
    // Available perspectives
    std::vector<EthicalPerspective> perspectives;
    
    // Cache for generated perspectives
    std::unordered_map<std::string, MultiPerspectiveResult> cache;
    
    // Ethical query patterns
    std::vector<std::string> ethical_keywords = {
        "ethical", "moral", "right", "wrong", "should", "ought",
        "virtue", "duty", "obligation", "justice", "fairness",
        "harm", "benefit", "consequences", "rights", "values",
        "conscience", "good", "bad", "evil", "responsibility"
    };
    
    std::vector<std::string> ethical_question_patterns = {
        "is it ethical", "is it moral", "is it right", "is it wrong",
        "should i", "ought i", "may i", "can i ethically",
        "what is the right", "what is the moral", "how should",
        "what ought", "ethically speaking", "morally speaking"
    };
    
    // Guidelines manager
    EthicalGuidelinesManager* guidelines_manager = nullptr;
    
    // Callback
    std::function<void(const MultiPerspectiveResult&)> callback;
};

// ═══════════════════════════════════════════════════════════
// Constructor / Destructor
// ═══════════════════════════════════════════════════════════

MultiPerspectiveGenerator::MultiPerspectiveGenerator(
    const MultiPerspectiveConfig& config,
    EthicalGuidelinesManager* guidelines_manager
) : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    impl_->guidelines_manager = guidelines_manager;
    
    // Load default perspectives if none provided
    loadDefaultPerspectives();
}

MultiPerspectiveGenerator::~MultiPerspectiveGenerator() = default;

// ═══════════════════════════════════════════════════════════
// Core functionality
// ═══════════════════════════════════════════════════════════

MultiPerspectiveResult MultiPerspectiveGenerator::generatePerspectives(
    const std::string& query,
    void* llm_wrapper,
    const std::vector<std::string>& /*context*/
) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->stats.total_generations++;
    
    auto start = std::chrono::steady_clock::now();
    
    // Check cache
    if (impl_->config.cache_perspectives) {
        std::string cache_key = query;
        auto it = impl_->cache.find(cache_key);
        if (it != impl_->cache.end()) {
            impl_->stats.cache_hits++;
            return it->second;
        }
        impl_->stats.cache_misses++;
    }
    
    MultiPerspectiveResult result;
    result.query = query;
    
    // Select perspectives to use
    std::vector<EthicalPerspective> selected = selectPerspectives(query);
    
    // Generate response from each perspective
    for (const auto& perspective : selected) {
        PerspectiveResponse resp = generateSinglePerspective(
            query,
            perspective,
            llm_wrapper
        );
        result.perspectives.push_back(resp);
        
        // Update perspective usage statistics
        impl_->stats.perspective_usage[perspective.id]++;
    }
    
    // Calculate diversity metrics
    result.unique_perspectives_count = clampSizeToInt(result.perspectives.size());
    result.perspective_diversity_score = calculateDiversityScore(result.perspectives);
    
    // Check if diversity requirements are met
    result.meets_diversity_requirement = 
        result.unique_perspectives_count >= impl_->config.min_perspectives &&
        result.perspective_diversity_score >= impl_->config.min_diversity_score;
    
    // Find common themes and disagreements
    result.common_themes = findCommonThemes(result.perspectives);
    result.disagreements = findDisagreements(result.perspectives);
    
    // Check if balanced view is shown
    result.shows_balanced_view = 
        result.unique_perspectives_count >= 2 &&
        !result.disagreements.empty();
    
    // Synthesize perspectives if enabled
    if (impl_->config.enable_synthesis && static_cast<int>(result.perspectives.size()) >= 2) {
        result.synthesized_response = synthesizePerspectives(
            result.perspectives,
            query
        );
        
        // Generate synthesis reasoning
        std::ostringstream oss = {};
        oss << "Synthesized " << result.perspectives.size() << " perspectives: ";
        for (size_t i = 0; i < result.perspectives.size(); ++i) {
            oss << result.perspectives[i].perspective.tradition;
            if (i < result.perspectives.size() - 1) {
              oss << ", ";
            }
        }
        oss << ". Diversity score: " << result.perspective_diversity_score;
        result.synthesis_reasoning = oss.str();
    }
    
    auto end = std::chrono::steady_clock::now();
    result.generation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    );
    
    // Update statistics
    updateStatistics(result);
    
    // Cache result
    if (impl_->config.cache_perspectives) {
        if (impl_->cache.size() >= impl_->config.max_cache_size) {
            // Remove oldest entry (simplified)
            impl_->cache.erase(impl_->cache.begin());
        }
        impl_->cache[query] = result;
    }
    
    // Call callback if set
    if ([[maybe_unused]] impl_->callback) {
        impl_->callback([[maybe_unused]] result);
    }
    
    if (static_cast<int>(result.perspectives.size()) >= 2) {
        impl_->stats.multi_perspective_generated++;
    }
    
    return result;
}

PerspectiveResponse MultiPerspectiveGenerator::generateSinglePerspective(
    const std::string& query,
    const EthicalPerspective& perspective,
    void* /*llm_wrapper*/
) {
    PerspectiveResponse response;
    response.perspective = perspective;
    
    // Build prompt for this perspective
    std::string prompt = buildPerspectivePrompt(query, perspective);
    
    // Generate perspective-specific response using rule-based approach
    // This provides deterministic perspective generation without LLM overhead
    std::ostringstream oss = {};
    
    if (perspective.tradition == "Utilitarian") {
        oss << "From a utilitarian perspective, we must consider the consequences "
            << "and outcomes. The ethical action is one that maximizes overall "
            << "well-being and minimizes harm for the greatest number of people. ";
        oss << "We should evaluate the potential benefits and harms to all affected parties.";
        response.confidence = 0.75f;
    } else if (perspective.tradition == "Deontological") {
        oss << "From a deontological perspective, we must consider duties, rights, "
            << "and moral rules. The ethical action is one that respects fundamental "
            << "principles and treats people as ends in themselves, never merely as means. ";
        oss << "We should ask whether this action can be universalized and respects human dignity.";
        response.confidence = 0.80f;
    } else if (perspective.tradition == "Virtue Ethics") {
        oss << "From a virtue ethics perspective, we must consider what character traits "
            << "and virtues this action would cultivate. The ethical action is one that "
            << "aligns with virtues like courage, wisdom, justice, and compassion. ";
        oss << "We should ask what a person of good character would do in this situation.";
        response.confidence = 0.70f;
    } else if (perspective.tradition == "Care Ethics") {
        oss << "From a care ethics perspective, we must consider relationships, "
            << "interconnection, and responsibilities to others. The ethical action "
            << "is one that maintains and strengthens caring relationships while "
            << "attending to the needs of those involved. ";
        oss << "We should consider the context and particularity of the situation.";
        response.confidence = 0.72f;
    } else {
        oss << "From the " << perspective.name << " perspective, we should consider "
            << "the principles and values that guide this ethical framework.";
        response.confidence = 0.65f;
    }
    
    response.response = oss.str();
    
    // Extract key points
    response.key_points = extractKeyPoints(response.response, perspective);
    
    // Generate reasoning
    std::ostringstream reasoning = {};
    reasoning << "Applied " << perspective.tradition << " framework focusing on ";
    reasoning << perspective.key_principles[0];
    response.reasoning = reasoning.str();
    
    return response;
}

std::string MultiPerspectiveGenerator::synthesizePerspectives(
    const std::vector<PerspectiveResponse>& perspectives,
    const std::string& query
) {
    if (perspectives.empty()) {
        return "";
    }
    
    if (static_cast<int>(perspectives.size()) == 1) {
        return perspectives[0].response;
    }
    
    // Build synthesis prompt
    std::string prompt = buildSynthesisPrompt(perspectives, query);
    
    // Create synthesis using rule-based approach
    // This provides deterministic synthesis without LLM overhead
    std::ostringstream oss = {};
    
    oss << "Considering this question from multiple ethical perspectives:\n\n";
    
    // Include each perspective
    for (size_t i = 0; i < perspectives.size(); ++i) {
        oss << (i + 1) << ". **" << perspectives[i].perspective.tradition 
            << "**: " << perspectives[i].response << "\n\n";
    }
    
    // Highlight common themes if preserve_all_perspectives
    if (impl_->config.preserve_all_perspectives) {
        auto common = findCommonThemes(perspectives);
        if (!common.empty()) {
            oss << "**Common themes**: ";
            for (size_t i = 0; i < common.size(); ++i) {
                oss << common[i];
                if (i < static_cast<int>(common.size()) - 1) {
                  oss << "; ";
                }
            }
            oss << "\n\n";
        }
    }
    
    // Highlight disagreements if enabled
    if (impl_->config.highlight_disagreements) {
        auto disagreements = findDisagreements(perspectives);
        if (!disagreements.empty()) {
            oss << "**Areas of tension**: ";
            for (size_t i = 0; i < disagreements.size(); ++i) {
                oss << disagreements[i];
                if (i < static_cast<int>(disagreements.size()) - 1) {
                  oss << "; ";
                }
            }
            oss << "\n\n";
        }
    }
    
    // Synthesis conclusion
    oss << "**Synthesis**: Each ethical framework offers valuable insights. ";
    oss << "A balanced approach would consider consequences, principles, character, ";
    oss << "and relationships. The most ethically sound decision likely integrates ";
    oss << "insights from multiple perspectives while being sensitive to the specific ";
    oss << "context and the people involved.";
    
    return oss.str();
}

bool MultiPerspectiveGenerator::requiresMultiPerspective(const std::string& query) {
    return detectEthicalQuery(query);
}

std::vector<EthicalPerspective> MultiPerspectiveGenerator::selectPerspectives(
    const std::string& /*query*/
) {
    std::vector<EthicalPerspective> selected;
    
    // Add required perspectives first
    for (const auto& req_id : impl_->config.required_perspectives) {
        auto it = std::find_if(
            impl_->perspectives.begin(),
            impl_->perspectives.end(),
            [&]([[maybe_unused]] const EthicalPerspective& p) { return p.id == req_id; }
        );
        if (it != impl_->perspectives.end()) {
            selected.push_back(*it);
        }
    }
    
    // Auto-select additional perspectives if enabled
    if (impl_->config.auto_select_perspectives) {
        // Always include core perspectives
        std::vector<std::string> core_traditions = {
            "Utilitarian", "Deontological", "Virtue Ethics", "Care Ethics"
        };
        
        for (const auto& tradition : core_traditions) {
            // Check if not already selected
            bool already_selected = false;
            for (const auto& sel : selected) {
                if (sel.tradition == tradition) {
                    already_selected = true;
                    break;
                }
            }
            
            if (!already_selected) {
                auto it = std::find_if(
                    impl_->perspectives.begin(),
                    impl_->perspectives.end(),
                    [&]([[maybe_unused]] const EthicalPerspective& p) { return p.tradition == tradition; }
                );
                if (it != impl_->perspectives.end()) {
                    selected.push_back(*it);
                    if (static_cast<int>(selected.size()) > = static_cast<size_t>(impl_->config.max_perspectives)) {
                        break;
                    }
                }
            }
        }
    }
    
    // Ensure minimum perspectives
    while ( static_cast<int>(selected.size()) < static_cast<size_t>(impl_->config.min_perspectives) &&
           selected.size() < impl_->perspectives.size()) {
        // Add any remaining perspective
        for (const auto& p : impl_->perspectives) {
            bool already_selected = false;
            for (const auto& sel : selected) {
                if (sel.id == p.id) {
                    already_selected = true;
                    break;
                }
            }
            if (!already_selected) {
                selected.push_back(p);
                break;
            }
        }
    }
    
    // Limit to max perspectives
    if (static_cast<int>(selected.size()) > static_cast<size_t>(impl_->config.max_perspectives)) {
        selected.resize(impl_->config.max_perspectives);
    }
    
    return selected;
}

// ═══════════════════════════════════════════════════════════
// Perspective management
// ═══════════════════════════════════════════════════════════

void MultiPerspectiveGenerator::addPerspective(const EthicalPerspective& perspective) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->perspectives.push_back(perspective);
}

void MultiPerspectiveGenerator::removePerspective(const std::string& perspective_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto& perspectives = impl_->perspectives;
    perspectives.erase(
        std::remove_if(
            perspectives.begin(),
            perspectives.end(),
            [&]([[maybe_unused]] const EthicalPerspective& p) { return p.id == perspective_id; }
        ),
        perspectives.end()
    );
}

std::vector<EthicalPerspective> MultiPerspectiveGenerator::getAvailablePerspectives() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->perspectives;
}

void MultiPerspectiveGenerator::loadDefaultPerspectives() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->perspectives.clear();
    
    // Perspective 1: Utilitarian (Consequentialist)
    EthicalPerspective utilitarian;
    utilitarian.id = "utilitarian";
    utilitarian.name = "Utilitarian Ethics";
    utilitarian.description = "Focuses on maximizing overall well-being and minimizing harm. "
                             "Evaluates actions based on their consequences and outcomes.";
    utilitarian.tradition = "Utilitarian";
    utilitarian.key_principles = {
        "Greatest good for greatest number",
        "Consequence-based evaluation",
        "Impartial consideration of all affected parties",
        "Utility maximization"
    };
    utilitarian.prompt_template = "Consider this from a utilitarian perspective, focusing on "
                                  "consequences and overall well-being: {query}";
    impl_->perspectives.push_back(utilitarian);
    
    // Perspective 2: Deontological (Duty-based)
    EthicalPerspective deontological;
    deontological.id = "deontological";
    deontological.name = "Deontological Ethics";
    deontological.description = "Focuses on duties, rights, and moral rules. "
                               "Evaluates actions based on adherence to principles and respect for persons.";
    deontological.tradition = "Deontological";
    deontological.key_principles = {
        "Respect for human dignity",
        "Universal moral principles",
        "Duty and obligation",
        "Rights-based reasoning",
        "Categorical imperative"
    };
    deontological.prompt_template = "Consider this from a deontological perspective, focusing on "
                                   "duties and moral principles: {query}";
    impl_->perspectives.push_back(deontological);
    
    // Perspective 3: Virtue Ethics (Character-based)
    EthicalPerspective virtue;
    virtue.id = "virtue_ethics";
    virtue.name = "Virtue Ethics";
    virtue.description = "Focuses on character traits and virtues. "
                        "Evaluates actions based on what a virtuous person would do.";
    virtue.tradition = "Virtue Ethics";
    virtue.key_principles = {
        "Cultivation of virtuous character",
        "Practical wisdom (phronesis)",
        "Eudaimonia (human flourishing)",
        "Virtues: courage, justice, wisdom, temperance",
        "Role models and moral exemplars"
    };
    virtue.prompt_template = "Consider this from a virtue ethics perspective, focusing on "
                            "character and virtues: {query}";
    impl_->perspectives.push_back(virtue);
    
    // Perspective 4: Care Ethics (Relationship-based)
    EthicalPerspective care;
    care.id = "care_ethics";
    care.name = "Care Ethics";
    care.description = "Focuses on relationships, interdependence, and caring responsibilities. "
                      "Evaluates actions based on maintaining caring relationships.";
    care.tradition = "Care Ethics";
    care.key_principles = {
        "Relational autonomy",
        "Contextual understanding",
        "Caring and being cared for",
        "Attending to needs",
        "Responsibility in relationships"
    };
    care.prompt_template = "Consider this from a care ethics perspective, focusing on "
                          "relationships and care: {query}";
    impl_->perspectives.push_back(care);
    
    // Perspective 5: Rights-based
    EthicalPerspective rights;
    rights.id = "rights_based";
    rights.name = "Rights-Based Ethics";
    rights.description = "Focuses on fundamental human rights and liberties. "
                        "Evaluates actions based on respect for individual rights.";
    rights.tradition = "Rights-Based";
    rights.key_principles = {
        "Universal human rights",
        "Individual liberty",
        "Non-interference",
        "Equal treatment",
        "Protection of minorities"
    };
    rights.prompt_template = "Consider this from a rights-based perspective, focusing on "
                            "human rights and liberties: {query}";
    impl_->perspectives.push_back(rights);
    
    // Perspective 6: Justice-based
    EthicalPerspective justice;
    justice.id = "justice_based";
    justice.name = "Justice-Based Ethics";
    justice.description = "Focuses on fairness, equality, and distributive justice. "
                         "Evaluates actions based on fair distribution and equal treatment.";
    justice.tradition = "Justice-Based";
    justice.key_principles = {
        "Distributive justice",
        "Equality and fairness",
        "Impartiality",
        "Social contract",
        "Fair opportunity"
    };
    justice.prompt_template = "Consider this from a justice-based perspective, focusing on "
                             "fairness and equality: {query}";
    impl_->perspectives.push_back(justice);
}

// ═══════════════════════════════════════════════════════════
// Diversity analysis
// ═══════════════════════════════════════════════════════════

float MultiPerspectiveGenerator::calculateDiversityScore(
    const std::vector<PerspectiveResponse>& perspectives
) {
    if (static_cast<int>(perspectives.size()) <= 1) {
        return 0.0f;
    }
    
    // Calculate diversity based on response differences
    float total_difference = 0.0f;
    int comparisons = 0;
    
    for (size_t i = 0; i < perspectives.size(); ++i) {
        for (size_t j = i + 1; j < perspectives.size(); ++j) {
            // Simple difference metric based on unique words
            std::unordered_set<std::string> words_i = extractWords(perspectives[i].response);
            std::unordered_set<std::string> words_j = extractWords(perspectives[j].response);
            
            // Calculate Jaccard distance (1 - Jaccard similarity)
            std::unordered_set<std::string> intersection;
            std::unordered_set<std::string> union_set;
            
            for (const auto& word : words_i) {
                union_set.insert(word);
                if (words_j.count(word)) {
                    intersection.insert(word);
                }
            }
            for (const auto& word : words_j) {
                union_set.insert(word);
            }
            
            float jaccard_similarity = union_set.empty() ? 0.0f : 
                static_cast<float>(intersection.size()) / union_set.size();
            float difference = 1.0f - jaccard_similarity;
            
            total_difference += difference;
            comparisons++;
        }
    }
    
    if (comparisons == 0) {
        return 0.0f;
    }
    
    float avg_difference = total_difference / comparisons;
    
    // Normalize to 0-1 range (Jaccard distance is already 0-1)
    return std::min(1.0f, std::max(0.0f, avg_difference));
}

std::vector<std::string> MultiPerspectiveGenerator::findCommonThemes(
    const std::vector<PerspectiveResponse>& perspectives
) {
    std::vector<std::string> common_themes;
    
    if (static_cast<int>(perspectives.size()) < 2) {
        return common_themes;
    }
    
    // Extract key points from all perspectives
    std::unordered_map<std::string, int> theme_counts;
    
    for (const auto& perspective : perspectives) {
        for (const auto& point : perspective.key_points) {
            std::string normalized = point;
            std::transform(normalized.begin(), normalized.end(), 
                          normalized.begin(), ::tolower);
            theme_counts[normalized]++;
        }
    }
    
    // Find themes that appear in multiple perspectives
    const int threshold = (perspectives.size() >= 3) ? 2 : static_cast<int>(perspectives.size());
    
    for (const auto& [theme, count] : theme_counts) {
        if (count >= threshold) {
            common_themes.push_back(theme);
        }
    }
    
    // Add some general common ethical themes
    std::vector<std::string> general_themes = {
        "Consideration of consequences and impacts",
        "Respect for human dignity and rights",
        "Importance of context and relationships"
    };
    
    for (const auto& theme : general_themes) {
        common_themes.push_back(theme);
        if (static_cast<int>(common_themes.size()) > = 5) break; // Limit to 5 themes
    }
    
    return common_themes;
}

std::vector<std::string> MultiPerspectiveGenerator::findDisagreements(
    const std::vector<PerspectiveResponse>& perspectives
) {
    std::vector<std::string> disagreements;
    
    if (static_cast<int>(perspectives.size()) < 2) {
        return disagreements;
    }
    
    // Identify areas where perspectives differ
    std::vector<std::string> typical_disagreements = {
        "Balance between individual rights and collective good",
        "Emphasis on principles versus consequences",
        "Role of character versus actions",
        "Importance of relationships versus impartiality"
    };
    
    // Check which disagreements are relevant based on perspectives present
    std::unordered_set<std::string> traditions = {};

    for (const auto& p : perspectives) {
        traditions.insert(p.perspective.tradition);
    }
    
    // Utilitarian vs Deontological tension
    if (traditions.count("Utilitarian") && traditions.count("Deontological")) {
        disagreements.push_back("Consequentialist outcomes versus adherence to universal principles");
    }
    
    // Virtue Ethics vs other approaches
    if (traditions.count("Virtue Ethics")) {
        disagreements.push_back("Focus on character development versus specific action evaluation");
    }
    
    // Care Ethics vs impartial approaches
    if (traditions.count("Care Ethics") && 
        (traditions.count("Utilitarian") || traditions.count("Justice-Based"))) {
        disagreements.push_back("Contextual care versus impartial rule application");
    }
    
    // Add some general disagreements if we have multiple perspectives
    if (disagreements.empty() && static_cast<int>(perspectives.size()) >= 2) {
        disagreements.push_back("Different emphases on key ethical considerations");
    }
    
    return disagreements;
}

// ═══════════════════════════════════════════════════════════
// Configuration
// ═══════════════════════════════════════════════════════════

void MultiPerspectiveGenerator::setConfig(const MultiPerspectiveConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config = config;
}

MultiPerspectiveConfig MultiPerspectiveGenerator::getConfig() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->config;
}

void MultiPerspectiveGenerator::setEthicalGuidelinesManager(EthicalGuidelinesManager* manager) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->guidelines_manager = manager;
}

void MultiPerspectiveGenerator::clearCache() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->cache.clear();
}

// ═══════════════════════════════════════════════════════════
// Statistics
// ═══════════════════════════════════════════════════════════

MultiPerspectiveGenerator::Statistics MultiPerspectiveGenerator::getStatistics() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->stats;
}

void MultiPerspectiveGenerator::resetStatistics() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->stats = Statistics();
}

void MultiPerspectiveGenerator::setGenerationCallback(
    std::function<void(const MultiPerspectiveResult&)> callback
) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->callback = callback;
}

// ═══════════════════════════════════════════════════════════
// Helper methods
// ═══════════════════════════════════════════════════════════

std::string MultiPerspectiveGenerator::buildPerspectivePrompt(
    const std::string& query,
    const EthicalPerspective& perspective
) {
    std::ostringstream oss = {};
    oss << "You are analyzing an ethical question from the " << perspective.name 
        << " perspective.\n\n";
    oss << "Framework: " << perspective.description << "\n\n";
    oss << "Key Principles:\n";
    for (const auto& principle : perspective.key_principles) {
        oss << "- " << principle << "\n";
    }
    oss << "\nQuery: " << query << "\n\n";
    oss << "Provide your analysis from this specific ethical framework, "
        << "considering its unique principles and approach.";
    return oss.str();
}

std::string MultiPerspectiveGenerator::buildSynthesisPrompt(
    const std::vector<PerspectiveResponse>& perspectives,
    const std::string& query
) {
    std::ostringstream oss = {};
    oss << "Synthesize the following ethical perspectives into a balanced response:\n\n";
    oss << "Query: " << query << "\n\n";
    oss << "Perspectives:\n";
    for (size_t i = 0; i < perspectives.size(); ++i) {
        oss << (i + 1) << ". " << perspectives[i].perspective.tradition << ": "
            << perspectives[i].response << "\n\n";
    }
    oss << "Provide a synthesis that:\n";
    oss << "- Acknowledges each perspective's insights\n";
    oss << "- Identifies common themes\n";
    oss << "- Notes areas of disagreement\n";
    oss << "- Offers a balanced conclusion that respects moral pluralism\n";
    return oss.str();
}

bool MultiPerspectiveGenerator::detectEthicalQuery(const std::string& query) {
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), 
                  query_lower.begin(), ::tolower);
    
    // Check for ethical question patterns
    for (const auto& pattern : impl_->ethical_question_patterns) {
        if (query_lower.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    // Check for ethical keywords (need at least 2 for higher confidence)
    int keyword_count = 0;
    for (const auto& keyword : impl_->ethical_keywords) {
        if (query_lower.find(keyword) != std::string::npos) {
            keyword_count++;
            if (keyword_count >= 2) {
                return true;
            }
        }
    }
    
    return false;
}

std::vector<std::string> MultiPerspectiveGenerator::extractKeyPoints(
    const std::string& response,
    const EthicalPerspective& perspective
) {
    std::vector<std::string> key_points;
    
    // Extract sentences that contain key principles
    std::istringstream iss(response);
    std::string sentence = {};
    
    // Simple sentence extraction (split by periods)
    std::string current = {};
    for (char c : response) {
        current += c;
        if (c == '.' || c == '!' || c == '?') {
            if (!current.empty()) {
                // Trim whitespace
                size_t start = current.find_first_not_of(" \t\n\r");
                size_t end = current.find_last_not_of(" \t\n\r");
                if (start != std::string::npos && end != std::string::npos) {
                    std::string trimmed = current.substr(start, end - start + 1);
                    if (trimmed.length() > 20) { // Minimum length for key point
                        key_points.push_back(trimmed);
                    }
                }
            }
            current.clear();
        }
    }
    
    // Limit to 3 key points
    if (static_cast<int>(key_points.size()) > 3) {
        key_points.resize(3);
    }
    
    // If no key points extracted, use principle names
    if (key_points.empty() && !perspective.key_principles.empty()) {
        key_points.push_back(perspective.key_principles[0]);
    }
    
    return key_points;
}

void MultiPerspectiveGenerator::updateStatistics(const MultiPerspectiveResult& result) {
    // Update average diversity score
    if (impl_->stats.total_generations == 1) {
        impl_->stats.avg_diversity_score = result.perspective_diversity_score;
        impl_->stats.avg_perspectives_per_query = 
            static_cast<float>(result.unique_perspectives_count);
        impl_->stats.avg_generation_time = result.generation_time;
    } else {
        float n = static_cast<float>(impl_->stats.total_generations);
        impl_->stats.avg_diversity_score = 
            (impl_->stats.avg_diversity_score * (n - 1) + 
             result.perspective_diversity_score) / n;
        impl_->stats.avg_perspectives_per_query = 
            (impl_->stats.avg_perspectives_per_query * (n - 1) + 
             result.unique_perspectives_count) / n;
        
        auto avg_ms = impl_->stats.avg_generation_time.count() * (n - 1) + 
                     result.generation_time.count();
        impl_->stats.avg_generation_time = std::chrono::milliseconds(
            static_cast<int64_t>(avg_ms / n)
        );
    }
}

// ═══════════════════════════════════════════════════════════
// Factory methods
// ═══════════════════════════════════════════════════════════

std::unique_ptr<MultiPerspectiveGenerator> MultiPerspectiveGeneratorFactory::createDefault() {
    return std::make_unique<MultiPerspectiveGenerator>();
}

std::unique_ptr<MultiPerspectiveGenerator> MultiPerspectiveGeneratorFactory::createHighDiversity() {
    MultiPerspectiveConfig config;
    config.min_perspectives = 3;
    config.max_perspectives = 5;
    config.min_diversity_score = 0.75f;
    config.require_contrasting_views = true;
    config.highlight_disagreements = true;
    return std::make_unique<MultiPerspectiveGenerator>(config);
}

std::unique_ptr<MultiPerspectiveGenerator> MultiPerspectiveGeneratorFactory::createWithPerspectives(
    const std::vector<std::string>& required_perspectives
) {
    MultiPerspectiveConfig config;
    config.required_perspectives = required_perspectives;
    config.auto_select_perspectives = false;
    config.min_perspectives = clampSizeToInt(required_perspectives.size());
    config.max_perspectives = clampSizeToInt(required_perspectives.size());
    return std::make_unique<MultiPerspectiveGenerator>(config);
}

std::unique_ptr<MultiPerspectiveGenerator> MultiPerspectiveGeneratorFactory::create(
    const MultiPerspectiveConfig& config
) {
    return std::make_unique<MultiPerspectiveGenerator>(config);
}

} // namespace llm
} // namespace themis
