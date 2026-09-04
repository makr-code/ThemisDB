/**
 * @file chain_visualizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "chain_visualizer.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <unordered_set>

namespace themis {
namespace plugins {
namespace ethics {

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

const char* ChainVisualizer::dotFillColor(ArgumentType type) {
    switch (type) {
        case ArgumentType::PRO:
        [[fallthrough]];\n        case ArgumentType::SYNTHESIS:
            return "lightblue";
        case ArgumentType::CONTRA:
        [[fallthrough]];\n        case ArgumentType::REBUTTAL:
            return "lightyellow";
        case ArgumentType::QUESTION:
        [[fallthrough]];\n        case ArgumentType::CLARIFICATION:
            return "lightgrey";
        default:
            return "white";
    }
}

std::string ChainVisualizer::makeLabel(const EthicalArgument& arg) {
    const char* type_str  = argumentTypeToString(arg.argument_type);
    const char* str_str   = argumentStrengthToString(arg.strength);
    return arg.philosophy_school + "\\n" + type_str + " | " + str_str;
}

std::string ChainVisualizer::dotEscape(const std::string& s) {
    std::ostringstream out;
    for (char c : s) {
        if (c == '"') {
          out << "\\\"";
        }
        else if (c == '\\') out << "\\\\";
        else                out << c;
    }
    return out.str();
}

std::string ChainVisualizer::mermaidEscape(const std::string& s) {
    // Mermaid node labels are wrapped in quotes; replace special chars.
    std::ostringstream out;
    for (char c : s) {
        if (c == '"') {
          out << "'";
        }
        else if (c == '\n') out << "<br/>";
        else           out << c;
    }
    return out.str();
}

// ---------------------------------------------------------------------------
// Core export logic
// ---------------------------------------------------------------------------

std::string ChainVisualizer::exportDot(
    const std::vector<std::string>& argument_ids,
    ArgumentStore& store,
    const std::string& graph_name)
{
    // Build a set for fast membership tests (to restrict edges)
    std::set<std::string> id_set(argument_ids.begin(), argument_ids.end());

    std::ostringstream out;
    out << "digraph " << dotEscape(graph_name) << " {\n";
    out << "  rankdir=LR;\n";
    out << "  node [shape=box, style=filled];\n\n";

    // Emit nodes
    for (const auto& id : argument_ids) {
        auto res = store.getArgument(id);
        if (auto* arg = std::get_if<EthicalArgument>(&res)) {
            out << "  \"" << dotEscape(id) << "\""
                << " [label=\"" << dotEscape(makeLabel(*arg)) << "\""
                << ", fillcolor=" << dotFillColor(arg->argument_type) << "];\n";
        } else {
            // Unresolvable: emit a placeholder node with a comment
            out << "  \"" << dotEscape(id) << "\""
                << " [label=\"" << dotEscape(id) << "\\n(unresolved)\""
                << ", fillcolor=lightgrey];\n";
        }
    }

    out << "\n";

    // Emit edges (only between nodes in the set)
    for (const auto& id : argument_ids) {
        auto res = store.getArgument(id);
        if (auto* arg = std::get_if<EthicalArgument>(&res)) {
            for (const auto& target : arg->supports) {
                if (id_set.count(target)) {
                    out << "  \"" << dotEscape(id) << "\" -> \""
                        << dotEscape(target) << "\" [label=\"supports\"];\n";
                }
            }
            for (const auto& target : arg->counterarguments) {
                if (id_set.count(target)) {
                    out << "  \"" << dotEscape(id) << "\" -> \""
                        << dotEscape(target) << "\" [label=\"counters\", style=dashed];\n";
                }
            }
        }
    }

    out << "}\n";
    return out.str();
}

std::string ChainVisualizer::exportMermaid(
    const std::vector<std::string>& argument_ids,
    ArgumentStore& store)
{
    std::set<std::string> id_set(argument_ids.begin(), argument_ids.end());

    std::ostringstream out;
    out << "flowchart LR\n";

    // Emit nodes
    for (const auto& id : argument_ids) {
        // Mermaid node IDs cannot contain special chars; hash to safe name
        std::string safe_id = id;
        std::replace(safe_id.begin(), safe_id.end(), '-', '_');
        std::replace(safe_id.begin(), safe_id.end(), '.', '_');

        auto res = store.getArgument(id);
        if (auto* arg = std::get_if<EthicalArgument>(&res)) {
            // Replace \n with actual newline substitute for Mermaid
            std::string label = arg->philosophy_school + "\n"
                + argumentTypeToString(arg->argument_type)
                + " | " + argumentStrengthToString(arg->strength);
            out << "  " << safe_id << "[\"" << mermaidEscape(label) << "\"]\n";
        } else {
            out << "  " << safe_id << "[\"" << mermaidEscape(id) << " (unresolved)\"]\n";
        }
    }

    out << "\n";

    // Emit edges
    for (const auto& id : argument_ids) {
        std::string safe_id = id;
        std::replace(safe_id.begin(), safe_id.end(), '-', '_');
        std::replace(safe_id.begin(), safe_id.end(), '.', '_');

        auto res = store.getArgument(id);
        if (auto* arg = std::get_if<EthicalArgument>(&res)) {
            for (const auto& target : arg->supports) {
                if (id_set.count(target)) {
                    std::string safe_target = target;
                    std::replace(safe_target.begin(), safe_target.end(), '-', '_');
                    std::replace(safe_target.begin(), safe_target.end(), '.', '_');
                    out << "  " << safe_id << " -->|supports| " << safe_target << "\n";
                }
            }
            for (const auto& target : arg->counterarguments) {
                if (id_set.count(target)) {
                    std::string safe_target = target;
                    std::replace(safe_target.begin(), safe_target.end(), '-', '_');
                    std::replace(safe_target.begin(), safe_target.end(), '.', '_');
                    out << "  " << safe_id << " -.->|counters| " << safe_target << "\n";
                }
            }
        }
    }

    return out.str();
}

// ---------------------------------------------------------------------------
// Convenience wrappers
// ---------------------------------------------------------------------------

std::string ChainVisualizer::chainToDot(
    const ArgumentChain& chain,
    ArgumentStore& store,
    const std::string& graph_name)
{
    return exportDot(chain.argument_ids, store, graph_name);
}

std::string ChainVisualizer::chainToMermaid(
    const ArgumentChain& chain,
    ArgumentStore& store)
{
    return exportMermaid(chain.argument_ids, store);
}

} // namespace ethics
} // namespace plugins
} // namespace themis
