/**
 * @file expert_system_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=13, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "analytics/expert_system_engine.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace themisdb {
namespace analytics {

// ──────────────────────────────────────────────────────────────────────────────
// helpers
// ──────────────────────────────────────────────────────────────────────────────

static bool isVariable(const std::string &s) {
    return !s.empty() && s.front() == '?';
}

static std::string jsonEscape(const std::string &s) {
    std::string out = {};
    out.reserve(s.size() + 4);
    for (char c : s) {
        if (c == '"') {
            out += "\\\"";
        } else if (c == '\\') {
            out += "\\\\";
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else {
            out += c;
        }
    }
    return out;
}

static std::string factToJson(const Fact &f) {
    std::ostringstream oss = {};
    oss << "{"
        << "\"id\":\"" << jsonEscape(f.id) << "\","
        << "\"subject\":\"" << jsonEscape(f.subject) << "\","
        << "\"predicate\":\"" << jsonEscape(f.predicate) << "\","
        << "\"object\":\"" << jsonEscape(f.object) << "\""
        << "}";
    return oss.str();
}

static std::string proofStepToJson(const ProofStep &ps) {
    std::ostringstream oss = {};
    oss << "{\"rule_id\":\"" << jsonEscape(ps.rule_id) << "\","
        << "\"matched_facts\":[";
    bool first = true;
    for (const auto &f : ps.matched_facts) {
        if (!first) {
            oss << ",";
        }
        oss << factToJson(f);
        first = false;
    }
    oss << "],\"derived_fact\":" << factToJson(ps.derived_fact) << "}";
    return oss.str();
}

// ──────────────────────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────────────────────

ExpertSystemEngine::ExpertSystemEngine(Config cfg) : cfg_(cfg), kb_(std::make_shared<KnowledgeBase>()) {}

// ──────────────────────────────────────────────────────────────────────────────
// KnowledgeBase access
// ──────────────────────────────────────────────────────────────────────────────

void ExpertSystemEngine::setKnowledgeBase(std::shared_ptr<KnowledgeBase> kb) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    kb_ = std::move(kb);
}

KnowledgeBase &ExpertSystemEngine::knowledgeBase() {
    return *kb_;
}

// ──────────────────────────────────────────────────────────────────────────────
// Working Memory
// ──────────────────────────────────────────────────────────────────────────────

std::string ExpertSystemEngine::assertFact(const std::string &subject, const std::string &predicate,
                                           const std::string &object) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return kb_->assertFact(subject, predicate, object);
}

bool ExpertSystemEngine::retractFact(const std::string &fact_id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return kb_->retractFact(fact_id);
}

// ──────────────────────────────────────────────────────────────────────────────
// applyBinding
// ──────────────────────────────────────────────────────────────────────────────

/*static*/ std::string ExpertSystemEngine::applyBinding(const std::string &elem, const Bindings &b) {
    if (!isVariable(elem)) {
        return elem;
    }
    const auto it = b.find(elem);
    return (it != b.end()) ? it->second : elem;
}

// ──────────────────────────────────────────────────────────────────────────────
// factExists (called under mutex)
// ──────────────────────────────────────────────────────────────────────────────

bool ExpertSystemEngine::factExists(const std::string &s, const std::string &p, const std::string &o) const {
    for (const auto &f : kb_->getFacts(p)) {
        if (f.subject == s && f.object == o) {
            return true;
        }
    }
    return false;
}

// ──────────────────────────────────────────────────────────────────────────────
// matchConditions — recursive backtracking
// ──────────────────────────────────────────────────────────────────────────────

bool ExpertSystemEngine::matchConditionsRec(const std::vector<TriplePattern> &conditions, std::size_t cond_idx,
                                            const std::vector<Fact> &all_facts, Bindings &bindings) const {
    if (cond_idx == conditions.size()) {
        return true;
    }

    const auto &cond = conditions[cond_idx];

    for (const auto &fact : all_facts) {
        Bindings local = bindings;
        bool match     = true;

        auto tryBind = [&](const std::string &pattern, const std::string &value) -> bool {
            if (isVariable(pattern)) {
                const auto it = local.find(pattern);
                if (it == local.end()) {
                    local[pattern] = value;
                    return true;
                }
                return it->second == value;
            }
            return pattern == value;
        };

        if (!tryBind(cond.subject, fact.subject)) {
            match = false;
        }
        if (match && !tryBind(cond.predicate, fact.predicate)) {
            match = false;
        }
        if (match && !tryBind(cond.object, fact.object)) {
            match = false;
        }

        if (match) {
            if (matchConditionsRec(conditions, cond_idx + 1, all_facts, local)) {
                bindings = std::move(local);
                return true;
            }
        }
    }
    return false;
}

std::optional<ExpertSystemEngine::Bindings>
ExpertSystemEngine::matchConditions(const HornClause &rule, const std::vector<Fact> &all_facts) const {
    Bindings bindings = {};
    if (matchConditionsRec(rule.conditions, 0, all_facts, bindings)) {
        return bindings;
    }
    return std::nullopt;
}

// ──────────────────────────────────────────────────────────────────────────────
// matchAllBindingsRec / matchAllConditions
// ──────────────────────────────────────────────────────────────────────────────

void ExpertSystemEngine::matchAllBindingsRec(const std::vector<TriplePattern> &conditions, std::size_t cond_idx,
                                             const std::vector<Fact> &all_facts, Bindings &current,
                                             std::vector<Bindings> &results) const {
    if (cond_idx == conditions.size()) {
        results.push_back(current);
        return;
    }

    const auto &cond = conditions[cond_idx];

    for (const auto &fact : all_facts) {
        Bindings local = current;
        bool match     = true;

        auto tryBind = [&](const std::string &pattern, const std::string &value) -> bool {
            if (isVariable(pattern)) {
                const auto it = local.find(pattern);
                if (it == local.end()) {
                    local[pattern] = value;
                    return true;
                }
                return it->second == value;
            }
            return pattern == value;
        };

        if (!tryBind(cond.subject, fact.subject)) {
            match = false;
        }
        if (match && !tryBind(cond.predicate, fact.predicate)) {
            match = false;
        }
        if (match && !tryBind(cond.object, fact.object)) {
            match = false;
        }

        if (match) {
            matchAllBindingsRec(conditions, cond_idx + 1, all_facts, local, results);
        }
    }
}

std::vector<ExpertSystemEngine::Bindings>
ExpertSystemEngine::matchAllConditions(const HornClause &rule, const std::vector<Fact> &all_facts) const {
    std::vector<Bindings> results;
    Bindings current;
    matchAllBindingsRec(rule.conditions, 0, all_facts, current, results);
    return results;
}

// ──────────────────────────────────────────────────────────────────────────────
// ML confidence (static, called WITHOUT holding mutex_)
// ──────────────────────────────────────────────────────────────────────────────

/*static*/ double ExpertSystemEngine::mlConfidenceNoLock(
    ModelServingEngine*      scorer,
    const ScorerFn&          scorer_fn,
    const std::string&       model_name,
    const std::string&       model_ver,
    const HornClause&        rule,
    const std::vector<Fact>& matched
) {
    if (scorer_fn) {
        try {
            return scorer_fn(rule, matched);
        } catch (...) {
            return 1.0;
        }
    }

    if (scorer && rule.ml_confidence_threshold > 0.0) {
        try {
            DataPoint dp;
            dp.id = rule.id;
            dp.set("condition_count", static_cast<double>(rule.conditions.size()));
            dp.set("match_count", static_cast<double>(matched.size()));
            dp.set("priority", static_cast<double>(rule.priority));
            const std::string label = scorer->predict(model_name, model_ver, dp);
            try {
                return std::stod(label);
            } catch (...) {
                return 1.0;
            }
        } catch (...) {
            return 1.0;
        }
    }

    return 1.0;
}

// ──────────────────────────────────────────────────────────────────────────────
// forwardChain
// ──────────────────────────────────────────────────────────────────────────────

int ExpertSystemEngine::forwardChain(int max_cycles) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    // Snapshot scorer state while holding the lock so the main loop can call
    // the scorer outside the lock (lock-under-callback fix, items #41–45).
    ModelServingEngine *const scorer_snap = ml_scorer_;
    const ScorerFn scorer_fn_snap         = ml_scorer_fn_;
    const std::string model_name_snap     = ml_model_name_;
    const std::string model_ver_snap      = ml_model_version_;

    int total_fired = 0;

    for (int cycle = 0; cycle < max_cycles; ++cycle) {
        int fired_this_cycle = 0;
        const auto all_facts = kb_->getFacts();
        const auto rules     = kb_->getRules(); // sorted by priority desc

        for (const auto &rule : rules) {
            // Collect ALL binding sets so that every matching entity is processed.
            const auto all_bindings = matchAllConditions(rule, all_facts);
            for (const auto &bindings : all_bindings) {
                // Gather matched facts for ML scoring.
                std::vector<Fact> matched = {};

                for (const auto &cond : rule.conditions) {
                    const auto s = applyBinding(cond.subject, bindings);
                    const auto p = applyBinding(cond.predicate, bindings);
                    const auto o = applyBinding(cond.object, bindings);
                    for (const auto &f : kb_->getFacts(p)) {
                        if (f.subject == s && f.object == o) {
                            matched.push_back(f);
                            break;
                        }
                    }
                }

                // ML confidence gate — release lock to avoid re-entrancy deadlock.
                double conf = 1.0;
                if (scorer_fn_snap || (scorer_snap && rule.ml_confidence_threshold > 0.0)) {
                    lock.unlock();
                    conf = mlConfidenceNoLock(scorer_snap, scorer_fn_snap, model_name_snap, model_ver_snap, rule,
                                              matched);
                    // NOTE: Re-acquiring lock without timeout is intentional. The timeout
                    // would need to bound the entire mlConfidenceNoLock() operation (which is
                    // executed outside the lock), not just the lock acquisition itself.
                    // For an expert reasoning engine, inference time is unpredictable and
                    // timeout-based locking is not appropriate here.
                    lock.lock();
                }
                if (conf < rule.ml_confidence_threshold) {
                    continue;
                }

                // Derive consequents.
                for (const auto &cons : rule.consequents) {
                    const auto s = applyBinding(cons.subject, bindings);
                    const auto p = applyBinding(cons.predicate, bindings);
                    const auto o = applyBinding(cons.object, bindings);
                    if (isVariable(s) || isVariable(p) || isVariable(o)) {
                        continue;
                    }
                    if (factExists(s, p, o)) {
                        continue;
                    }

                    const std::string new_id = kb_->assertFact(s, p, o);
                    ++fired_this_cycle;
                    ++total_fired;

                    // Record in decision log.
                    Fact derived;
                    derived.id        = new_id;
                    derived.subject   = s;
                    derived.predicate = p;
                    derived.object    = o;

                    ProofStep step;
                    step.rule_id       = rule.id;
                    step.matched_facts = matched;
                    step.derived_fact  = derived;
                    decision_log_[new_id].push_back(step);
                }
            } // end for all_bindings
        }

        if (fired_this_cycle == 0) {
            break; // Fixpoint reached.
        }
    }
    return total_fired;
}

// ──────────────────────────────────────────────────────────────────────────────
// queryGoal / backwardChainDLS
// ──────────────────────────────────────────────────────────────────────────────

bool ExpertSystemEngine::tripleInWM(const std::string &s, const std::string &p, const std::string &o) const {
    for (const auto &f : kb_->getFacts(p)) {
        if (f.subject == s && f.object == o) {
            return true;
        }
    }
    return false;
}

bool ExpertSystemEngine::backwardChainDLS(const TriplePattern &goal, std::vector<ProofStep> &trace, int depth,
                                          int max_depth) const {
    // Base case 1: goal is a concrete triple present in WM.
    if (!isVariable(goal.subject) && !isVariable(goal.predicate) && !isVariable(goal.object)) {
        if (tripleInWM(goal.subject, goal.predicate, goal.object)) {
            return true;
        }
    }

    if (depth >= max_depth) {
        return false;
    }

    // Try each rule whose consequent could unify with the goal.
    for (const auto &rule : kb_->getRules()) {
        for (const auto &cons : rule.consequents) {
            // Unify the consequent pattern with the concrete goal, collecting
            // variable bindings (e.g., ?x → "Alice") so we can ground the
            // rule's conditions before recursing.
            Bindings goal_bindings;
            bool unification_ok = true;

            auto tryUnify = [&](const std::string &pattern, const std::string &goal_elem) -> bool {
                if (isVariable(goal_elem)) {
                    return true; // wildcard goal element
                }
                if (isVariable(pattern)) {
                    auto it = goal_bindings.find(pattern);
                    if (it == goal_bindings.end()) {
                        goal_bindings[pattern] = goal_elem;
                        return true;
                    }
                    return it->second == goal_elem;
                }
                return pattern == goal_elem;
            };

            if (!tryUnify(cons.subject, goal.subject)) {
                unification_ok = false;
            }
            if (!tryUnify(cons.predicate, goal.predicate)) {
                unification_ok = false;
            }
            if (!tryUnify(cons.object, goal.object)) {
                unification_ok = false;
            }
            if (!unification_ok) {
                continue;
            }

            // Apply the goal bindings to each condition so that variables
            // shared with the consequent (e.g., ?x) are grounded before
            // the recursive check.
            std::vector<ProofStep> sub_trace;
            bool all_ok = true;
            for (const auto &cond : rule.conditions) {
                TriplePattern bound_cond;
                bound_cond.subject   = applyBinding(cond.subject, goal_bindings);
                bound_cond.predicate = applyBinding(cond.predicate, goal_bindings);
                bound_cond.object    = applyBinding(cond.object, goal_bindings);
                if (!backwardChainDLS(bound_cond, sub_trace, depth + 1, max_depth)) {
                    all_ok = false;
                    break;
                }
            }
            if (all_ok) {
                ProofStep step;
                step.rule_id = rule.id;
                // Build derived fact from goal (use literals; variables → goal value).
                auto resolve = [](const std::string &p, const std::string &g) { return isVariable(p) ? g : p; };
                Fact derived;
                derived.subject   = resolve(cons.subject, goal.subject);
                derived.predicate = resolve(cons.predicate, goal.predicate);
                derived.object    = resolve(cons.object, goal.object);
                step.derived_fact = derived;
                sub_trace.push_back(step);
                trace.insert(trace.end(), sub_trace.begin(), sub_trace.end());
                return true;
            }
        }
    }
    return false;
}

GoalResult ExpertSystemEngine::queryGoal(const TriplePattern &goal) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    GoalResult result;
    result.success    = backwardChainDLS(goal, result.proof_trace, 0, cfg_.max_backward_chain_depth);
    result.depth_used = static_cast<int>(result.proof_trace.size());
    return result;
}

// ──────────────────────────────────────────────────────────────────────────────
// explain
// ──────────────────────────────────────────────────────────────────────────────

std::string ExpertSystemEngine::explain(const std::string &fact_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    const auto it = decision_log_.find(fact_id);
    if (it == decision_log_.end()) {
        return "[]";
    }

    std::ostringstream oss = {};
    oss << "[";
    bool first = true;
    for (const auto &step : it->second) {
        if (!first) {
            oss << ",";
        }
        oss << proofStepToJson(step);
        first = false;
    }
    oss << "]";
    return oss.str();
}

// ──────────────────────────────────────────────────────────────────────────────
// ML Scorer injection
// ──────────────────────────────────────────────────────────────────────────────

void ExpertSystemEngine::setMLScorer(ModelServingEngine *scorer, const std::string &model_name,
                                     const std::string &model_version) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    ml_scorer_        = scorer;
    ml_model_name_    = model_name;
    ml_model_version_ = model_version;
    ml_scorer_fn_     = nullptr; // Clear function override.
}

void ExpertSystemEngine::setMLScorerFn(ScorerFn fn) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    ml_scorer_fn_ = std::move(fn);
    ml_scorer_    = nullptr; // Clear ModelServingEngine pointer.
}

// ──────────────────────────────────────────────────────────────────────────────
// State queries
// ──────────────────────────────────────────────────────────────────────────────

std::size_t ExpertSystemEngine::factCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return kb_->factCount();
}

std::size_t ExpertSystemEngine::ruleCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return kb_->ruleCount();
}

} // namespace analytics
} // namespace themisdb

