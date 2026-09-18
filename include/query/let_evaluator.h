/**
 * @file let_evaluator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include "query/aql_parser.h"

namespace themis {
    class SecondaryIndexManager;
}

namespace themis {
namespace query {

/**
 * @brief LET Evaluator für AQL Variable Bindings
 * 
 * Verwaltet LET-Deklarationen und evaluiert Expressions zu JSON-Werten.
 * Unterstützt:
 * - Einfache Expressions (Literale, Field Access)
 * - Arithmetische Operationen (+, -, *, /, %)
 * - String-Operationen (CONCAT, SUBSTRING, etc.)
 * - Nested Object/Array Access
 * - Referenzen zu vorherigen LETs
 * - Funktionsaufrufe (LENGTH, UPPER, LOWER, etc.)
 * 
 * Beispiel:
 * LET x = doc.age
 * LET y = x * 2
 * LET fullName = CONCAT(doc.firstName, " ", doc.lastName)
 */
class LetEvaluator {
public:
    LetEvaluator() = default;

    /**
     * @brief Wire a SecondaryIndexManager so that FULLTEXT/PHRASE/FUZZY AQL functions can call through to the real index.
     * @param[in,out] mgr Input/output parameter.
     * @details Caller retains ownership. Implements setSecondaryIndexManager without additional internal calls.
     */
    void setSecondaryIndexManager(themis::SecondaryIndexManager* mgr) {
        secondary_idx_mgr_ = mgr;
    }

    /**
     * @brief Evaluiert einen LET-Node und speichert das Binding
     * @param node Der LET-Node mit Variable und Expression
     * @param currentDoc Das aktuelle Dokument (für doc.field Zugriffe)
     * @return true wenn erfolgreich, false bei Errors (undefined vars, etc.)
     */
    bool evaluateLet(const LetNode& node, const nlohmann::json& currentDoc);

    /**
     * @brief Löst eine Variable zu ihrem JSON-Wert auf
     * @param varName Der Name der Variable (ohne $-Prefix)
     * @return Der JSON-Wert oder std::nullopt wenn Variable nicht existiert
     */
    std::optional<nlohmann::json> resolveVariable(const std::string& varName) const;

    /**
     * @brief Prüft ob eine Variable existiert
     * @param varName Der Name der Variable
     * @return true wenn Variable gebunden ist
     */
    bool hasVariable(const std::string& varName) const;

    /**
     * @brief Löscht alle Variable Bindings (für neue Query Iteration)
     */
    void clear();

    /**
     * @brief Evaluiert eine Expression zu einem JSON-Wert
     * @param expr Die zu evaluierende Expression
     * @param currentDoc Das aktuelle Dokument (für doc.field Zugriffe)
     * @return Der evaluierte JSON-Wert
     * @throws std::runtime_error bei Evaluation-Errors
     */
    nlohmann::json evaluateExpression(
        const std::shared_ptr<Expression>& expr,
        const nlohmann::json& currentDoc
    ) const;

    /**
     * @brief Alle aktuellen Bindings abrufen (für Debugging/Logging)
     * @return Map von Variable → JSON-Wert
     */
    const std::unordered_map<std::string, nlohmann::json>& getBindings() const {
        return bindings_;
    }

private:
    // Variable bindings: Variable Name → JSON Value
    std::unordered_map<std::string, nlohmann::json> bindings_;

    // Optional secondary index manager for FULLTEXT/PHRASE/FUZZY AQL functions
    themis::SecondaryIndexManager* secondary_idx_mgr_ = nullptr;

    /**
     * @brief Helper: Evaluiert Field Access (z.
     * @param[in] fieldAccess Input parameter.
     * @param[in] currentDoc Input parameter.
     * @return Return value.
     * @details B. doc.age, doc.address.city)
     */
    nlohmann::json evaluateFieldAccess(
        const FieldAccessExpr* fieldAccess,
        const nlohmann::json& currentDoc
    ) const;

    /**
     * @brief Helper: Evaluiert Binary Operations (+, -, *, /, %, ==, !
     * @param[in] binOp Input parameter.
     * @param[in] currentDoc Input parameter.
     * @return Return value.
     * @details =, <, >, etc.)
     */
    nlohmann::json evaluateBinaryOp(
        const BinaryOpExpr* binOp,
        const nlohmann::json& currentDoc
    ) const;

    /**
     * @brief Helper: Evaluiert Unary Operations (-, NOT)
     * @param[in] unaryOp Input parameter.
     * @param[in] currentDoc Input parameter.
     * @return Return value.
     */
    nlohmann::json evaluateUnaryOp(
        const UnaryOpExpr* unaryOp,
        const nlohmann::json& currentDoc
    ) const;

    /**
     * @brief Helper: Evaluiert Function Calls (LENGTH, CONCAT, SUBSTRING, etc.
     * @param[in] funcCall Input parameter.
     * @param[in] currentDoc Input parameter.
     * @return Return value.
     * @details )
     */
    nlohmann::json evaluateFunctionCall(
        const FunctionCallExpr* funcCall,
        const nlohmann::json& currentDoc
    ) const;

    /**
     * @brief Helper: Evaluiert Array/Object Literal
     * @param[in] lit Input parameter.
     * @return Return value.
     */
    nlohmann::json evaluateLiteral(const LiteralExpr* lit) const;

    /**
     * @brief Helper: Holt Wert aus nested JSON object (z.
     * @param[in] obj Input parameter.
     * @param[in] path Input parameter.
     * @return Return value.
     * @details B. ["address", "city"])
     */
    nlohmann::json getNestedValue(
        const nlohmann::json& obj,
        const std::vector<std::string>& path
    ) const;

    /**
     * @brief Helper: Arithmetische Operation auf zwei JSON-Werten
     * @param[in] op Input parameter.
     * @param[in] left Input parameter.
     * @param[in] right Input parameter.
     * @return Return value.
     */
    nlohmann::json applyArithmeticOp(
        const std::string& op,
        const nlohmann::json& left,
        const nlohmann::json& right
    ) const;

    /**
     * @brief Helper: Vergleichsoperation auf zwei JSON-Werten
     * @param[in] op Input parameter.
     * @param[in] left Input parameter.
     * @param[in] right Input parameter.
     * @return Return value.
     */
    nlohmann::json applyComparisonOp(
        const std::string& op,
        const nlohmann::json& left,
        const nlohmann::json& right
    ) const;

    /**
     * @brief Helper: Logische Operation (AND, OR)
     * @param[in] op Input parameter.
     * @param[in] left Input parameter.
     * @param[in] right Input parameter.
     * @return Return value.
     */
    nlohmann::json applyLogicalOp(
        const std::string& op,
        const nlohmann::json& left,
        const nlohmann::json& right
    ) const;

    /**
     * @brief Helper: Konvertiert JSON zu bool für Conditions
     * @param[in] value Input parameter.
     * @return True on success.
     */
    bool toBool(const nlohmann::json& value) const;

    /**
     * @brief Helper: Konvertiert JSON zu number für Arithmetik
     * @param[in] value Input parameter.
     * @return Return value.
     */
    double toNumber(const nlohmann::json& value) const;
};

} // namespace query
} // namespace themis
