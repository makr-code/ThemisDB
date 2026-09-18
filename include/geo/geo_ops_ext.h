/**
 * @file geo_ops_ext.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>

namespace themis {
namespace geo {

// Extension interface for providing additional ST_* operations via plugins
/** @brief Extension interface for providing additional ST_* operations via plugins. */
class IGeoOpsExtension {
public:
    /**
     * @brief TBD: Describe ~IGeoOpsExtension.
     * @return Return value.
     */
    virtual ~IGeoOpsExtension() = default;
    /**
     * @brief TBD: Describe name.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    virtual const char* name() const noexcept = 0;
    /**
     * @brief TBD: Describe supports.
     * @param[in] op_name Input parameter.
     * @return True on success.
     * @note Exception safety: noexcept.
     */
    virtual bool supports(const std::string& op_name) const noexcept = 0; // e.g., "ST_Buffer"
};

} // namespace geo
} // namespace themis
