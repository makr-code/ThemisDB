/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            chain_visualizer.h                                 ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 18:48:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     142                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 87778519a4  2026-04-12  feat(ethics_ai): remove stubs — computed scoring, YAML fi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include "argument_store.h"

#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Argument chain visualisation in DOT and Mermaid formats.
 *
 * All methods are purely functional (stateless static).  They read argument
 * metadata from @p store and produce a self-contained graph description that
 * can be rendered by Graphviz (`dot`) or the Mermaid diagram tool.
 *
 * ### DOT (Graphviz) example
 * @code
 * digraph ethics_debate {
 *   rankdir=LR;
 *   node [shape=box, style=filled];
 *   "arg_1" [label="kant\nPRO | MODERATE", fillcolor=lightblue];
 *   "arg_2" [label="utilit…\nCONTRA | STRONG", fillcolor=lightyellow];
 *   "arg_1" -> "arg_2" [label="supports"];
 * }
 * @endcode
 *
 * ### Mermaid example
 * @code
 * flowchart LR
 *   arg_1["kant\nPRO | MODERATE"]
 *   arg_2["utilit...\nCONTRA | STRONG"]
 *   arg_1 -->|supports| arg_2
 * @endcode
 *
 * Node colours in DOT output:
 * - PRO / SYNTHESIS  → lightblue
 * - CONTRA / REBUTTAL → lightyellow
 * - QUESTION / CLARIFICATION → lightgrey
 */
class ChainVisualizer {
public:
    // ------------------------------------------------------------------
    // Primary API
    // ------------------------------------------------------------------

    /**
     * @brief Export an ordered list of argument IDs to Graphviz DOT format.
     *
     * Nodes are rendered in the order given by @p argument_ids.  Edges are
     * derived from each argument's `supports` and `counterarguments` link
     * lists, restricted to arguments that appear in @p argument_ids.
     *
     * @param argument_ids  Ordered argument IDs to include in the graph.
     * @param store         Argument store to resolve IDs.
     * @param graph_name    DOT graph identifier (default: "ethics_debate").
     * @return DOT source string.  Never throws; unresolvable IDs produce a
     *         comment node.
     */
    static std::string exportDot(
        const std::vector<std::string>& argument_ids,
        ArgumentStore& store,
        const std::string& graph_name = "ethics_debate"
    );

    /**
     * @brief Export an ordered list of argument IDs to Mermaid flowchart format.
     *
     * Same semantics as exportDot().
     *
     * @param argument_ids  Ordered argument IDs.
     * @param store         Argument store.
     * @return Mermaid source string.
     */
    static std::string exportMermaid(
        const std::vector<std::string>& argument_ids,
        ArgumentStore& store
    );

    /**
     * @brief Export a full ArgumentChain to Graphviz DOT.
     *
     * Convenience wrapper: uses `chain.argument_ids` as the ordered list.
     */
    static std::string chainToDot(
        const ArgumentChain& chain,
        ArgumentStore& store,
        const std::string& graph_name = "ethics_chain"
    );

    /**
     * @brief Export a full ArgumentChain to Mermaid flowchart.
     *
     * Convenience wrapper: uses `chain.argument_ids` as the ordered list.
     */
    static std::string chainToMermaid(
        const ArgumentChain& chain,
        ArgumentStore& store
    );

private:
    // Node colour (DOT fillcolor) based on argument type
    static const char* dotFillColor(ArgumentType type);

    // Short label string:  "<school>\n<type> | <strength>"
    static std::string makeLabel(const EthicalArgument& arg);

    // Escape a string for use inside DOT double-quoted attributes
    static std::string dotEscape(const std::string& s);

    // Escape a string for use inside Mermaid node labels
    static std::string mermaidEscape(const std::string& s);
};

} // namespace ethics
} // namespace plugins
} // namespace themis

