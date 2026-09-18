/**
 * @file chain_visualizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include "argument_store.h"

#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

class ChainVisualizer {
public:
    // ------------------------------------------------------------------
    // Primary API
    // ------------------------------------------------------------------

    static std::string exportDot(
        const std::vector<std::string>& argument_ids,
        ArgumentStore& store,
        const std::string& graph_name = "ethics_debate"
    );

    /**
     * @brief Export Mermaid.
     * @param[in] argument_ids Input parameter.
     * @param[in,out] store Input/output parameter.
     * @return Return value.
     */
    static std::string exportMermaid(
        const std::vector<std::string>& argument_ids,
        ArgumentStore& store
    );

    static std::string chainToDot(
        const ArgumentChain& chain,
        ArgumentStore& store,
        const std::string& graph_name = "ethics_chain"
    );

    /**
     * @brief Chain To Mermaid.
     * @param[in] chain Input parameter.
     * @param[in,out] store Input/output parameter.
     * @return Return value.
     */
    static std::string chainToMermaid(
        const ArgumentChain& chain,
        ArgumentStore& store
    );

private:
    /**
     * @brief Node colour (DOT fillcolor) based on argument type
     * @param[in] type Input parameter.
     * @return Pointer to the result.
     */
    static const char* dotFillColor(ArgumentType type);

    /**
     * @brief Short label string: "<school>\n<type> | <strength>"
     * @param[in] arg Input parameter.
     * @return Return value.
     */
    static std::string makeLabel(const EthicalArgument& arg);

    /**
     * @brief Escape a string for use inside DOT double-quoted attributes
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::string dotEscape(const std::string& s);

    /**
     * @brief Escape a string for use inside Mermaid node labels
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::string mermaidEscape(const std::string& s);
};

} // namespace ethics
} // namespace plugins
} // namespace themis

