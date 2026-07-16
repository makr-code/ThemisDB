/**
 * @file tensor_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "query/functions/function_registry.h"

namespace themis {
namespace query {
namespace functions {

/**
 * @brief Register all TENSOR_* built-in functions with the AQL registry.
 *
 * @param registry  The FunctionRegistry singleton.
 */
void registerTensorFunctions(FunctionRegistry& registry);

} // namespace functions
} // namespace query
} // namespace themis
