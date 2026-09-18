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
class IGeoOpsExtension {
public:
    /**
     * @brief IGeo Ops Extension.
     * @return Return value.
     */
    virtual ~IGeoOpsExtension() = default;
    /**
     * @brief Name.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    virtual const char* name() const noexcept = 0;
    /**
     * @brief Supports.
     * @param[in] op_name Name of the op.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    virtual bool supports(const std::string& op_name) const noexcept = 0; // e.g., "ST_Buffer"
};

} // namespace geo
} // namespace themis
