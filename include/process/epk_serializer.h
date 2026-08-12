/**
 * @file epk_serializer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/process_graph.h"
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace process {

/**
 * @brief EPK (Ereignisgesteuerte Prozesskette) serializer.
 *
 * EPK (Event-driven Process Chain) is the process notation used in the ARIS
 * methodology and is widely employed in German public administration and
 * enterprise modelling (VCC-VPB).
 *
 * ## EPK element mapping to ThemisDB types
 *
 * | EPK Element            | ThemisDB EPKNodeType       |
 * |------------------------|----------------------------|
 * | Ereignis               | EPKNodeType::EVENT         |
 * | Funktion               | EPKNodeType::FUNCTION      |
 * | AND-Verknüpfung        | EPKNodeType::AND_CONNECTOR |
 * | OR-Verknüpfung         | EPKNodeType::OR_CONNECTOR  |
 * | XOR-Verknüpfung        | EPKNodeType::XOR_CONNECTOR |
 * | Organisationseinheit   | EPKNodeType::ORGANIZATIONAL_UNIT |
 * | Informationsobjekt     | EPKNodeType::INFORMATION_OBJECT  |
 * | Anwendungssystem       | EPKNodeType::APPLICATION_SYSTEM  |
 * | Prozesswegweiser       | EPKNodeType::PROCESS_PATH        |
 *
 * ## Supported text format
 *
 * ```
 * EVENT: "Antrag eingegangen"
 * FUNCTION: "Vollständigkeit prüfen" [role="Sachbearbeiter", sla=48h]
 * XOR: {
 *   -> EVENT: "Vollständig" -> FUNCTION: "Fachlich prüfen"
 *   -> FUNCTION: "Nachfordern"
 * }
 * EVENT: "Entscheidung getroffen"
 * ```
 *
 * A JSON format is also accepted as input (array of node/edge objects).
 */
class EpkSerializer {
public:
    // -------------------------------------------------------------------------
    // Import
    // -------------------------------------------------------------------------

    struct ImportResult {
        bool ok{false};
        std::string message;
        std::string process_id;
        std::string process_name;
        std::vector<ProcessNodeInfo> nodes;
        std::vector<ProcessEdgeInfo> edges;
    };

    /**
     * @brief Parse an EPK text definition into ProcessNodeInfo / ProcessEdgeInfo.
     *
     * Accepts both the simple line-based text notation and a JSON array format.
     *
     * @param epk_text  EPK definition string.
     * @param process_id    Optional override for process ID.
     * @param process_name  Optional override for process name.
     */
    static ImportResult importText(
        std::string_view epk_text,
        std::string_view process_id   = "",
        std::string_view process_name = ""
    );

    /**
     * @brief Parse an EPK JSON array (nodes + edges format).
     */
    static ImportResult importJson(const nlohmann::json& epk_json);

    // -------------------------------------------------------------------------
    // Export
    // -------------------------------------------------------------------------

    /**
     * @brief Export EPK nodes and edges to the simple text notation.
     */
    static std::string exportText(
        std::string_view                    process_name,
        const std::vector<ProcessNodeInfo>& nodes,
        const std::vector<ProcessEdgeInfo>& edges
    );

    /**
     * @brief Export EPK to a structured JSON representation.
     *
     * Useful for programmatic processing and LLM context generation.
     */
    static nlohmann::json exportJson(
        std::string_view                    process_id,
        std::string_view                    process_name,
        const std::vector<ProcessNodeInfo>& nodes,
        const std::vector<ProcessEdgeInfo>& edges
    );

private:
    static std::string epkNodeTypeToLabel_(EPKNodeType t);
    static EPKNodeType labelToEpkNodeType_(std::string_view label);
};

} // namespace process
} // namespace themis
