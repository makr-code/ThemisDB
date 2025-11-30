/**
 * @file function_registry.cpp
 * @brief Implementation of the AQL Function Registry
 * 
 * Registers all built-in functions at application startup.
 */

#include "query/functions/function_registry.h"
#include "query/functions/string_functions.h"
#include "query/functions/math_functions.h"
#include "query/functions/array_functions.h"
#include "query/functions/date_functions.h"
#include "query/functions/document_functions.h"

namespace themis {
namespace query {
namespace functions {

void registerBuiltinFunctions() {
    // Register all function categories
    registerStringFunctions();
    registerMathFunctions();
    registerArrayFunctions();
    registerDateFunctions();
    registerDocumentFunctions();
    
    // TODO: Register Geo functions (migrate from let_evaluator.cpp)
    // TODO: Register Vector functions
    // TODO: Register Graph functions
}

} // namespace functions
} // namespace query
} // namespace themis
