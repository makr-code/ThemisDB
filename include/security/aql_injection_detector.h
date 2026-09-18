/**
 * @file aql_injection_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <regex>
#include <unordered_set>
#include "query/aql_parser.h"
#include "utils/expected.h"

namespace themis {
namespace security {

class AQLInjectionDetector {
public:
    struct InjectionCheckResult {
        bool is_safe = true;                          // True if query is safe to execute
        std::string error_message;                     // Error message if unsafe
        std::vector<std::string> detected_patterns;    // Patterns detected during validation
        
        // Implicit conversion to bool for convenient checking
        explicit operator bool() const { return is_safe; }
    };
    
    /**
     * @brief Validate Parameterized Query.
     * @param[in] aql_template Input parameter.
     * @param[in] parameters Input parameter.
     * @return Return value.
     */
    InjectionCheckResult validateParameterizedQuery(
        const std::string& aql_template,
        const std::vector<std::string>& parameters
    );
    
    /**
     * @brief Validate AQLAST.
     * @param[in] aql Input parameter.
     * @return Return value.
     */
    InjectionCheckResult validateAQLAST(const std::string& aql);

    /**
     * @brief Validate For Read Only Context.
     * @param[in] aql Input parameter.
     * @return Return value.
     */
    InjectionCheckResult validateForReadOnlyContext(const std::string& aql);

    /**
     * @brief Validate Unbounded For Loops.
     * @param[in] aql Input parameter.
     * @return Return value.
     */
    InjectionCheckResult validateUnboundedForLoops(const std::string& aql);

private:
    // ============================================================================
    // Helper Functions
    // ============================================================================
    
    /**
     * @brief Is Valid AQLTemplate.
     * @param[in] template_str Input parameter.
     * @return True when the operation succeeds.
     */
    bool isValidAQLTemplate(const std::string& template_str);
    
    /**
     * @brief Validate Parameter.
     * @param[in] param Input parameter.
     * @return Return value.
     */
    InjectionCheckResult validateParameter(const std::string& param);
    
    /**
     * @brief Contains Suspicious Patterns.
     * @param[in] str Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsSuspiciousPatterns(const std::string& str);
    
    /**
     * @brief Extract Patterns.
     * @param[in] str Input parameter.
     * @return Return value.
     */
    std::vector<std::string> extractPatterns(const std::string& str);
    
    /**
     * @brief Contains SQLKeywords.
     * @param[in] str Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsSQLKeywords(const std::string& str);
    
    /**
     * @brief Contains Dangerous Operations.
     * @param[in] ast Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsDangerousOperations(const query::Query& ast);
    
    /**
     * @brief Extract String Literals.
     * @param[in] ast Input parameter.
     * @return Return value.
     */
    std::vector<std::string> extractStringLiterals(const query::Query& ast);
    
    /**
     * @brief Extract String Literals From Expression.
     * @param[in] expr Input parameter.
     * @param[in,out] literals Input/output parameter.
     */
    void extractStringLiteralsFromExpression(
        const std::shared_ptr<query::Expression>& expr,
        std::vector<std::string>& literals
    );
    
    /**
     * @brief Parse AQL.
     * @param[in] aql Input parameter.
     * @return Return value.
     */
    Result<std::shared_ptr<query::Query>> parseAQL(const std::string& aql);

    /**
     * @brief Scan Expression For Dangerous Ops.
     * @param[in] expr Input parameter.
     * @return True when the operation succeeds.
     */
    bool scanExpressionForDangerousOps(const std::shared_ptr<query::Expression>& expr);

};

} // namespace security
} // namespace themis
