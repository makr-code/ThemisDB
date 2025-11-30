/**
 * @file function_registry.cpp
 * @brief Implementation of the AQL Function Registry
 * 
 * Registers all built-in functions at application startup.
 * 
 * ## Function Categories
 * 
 * | Category | Description                              | Count |
 * |----------|------------------------------------------|-------|
 * | String   | Text manipulation and pattern matching   | ~15   |
 * | Math     | Arithmetic, trigonometry, statistics     | ~25   |
 * | Array    | List operations and transformations      | ~20   |
 * | Date     | Date/time parsing, formatting, arithmetic| ~15   |
 * | Document | Object manipulation and type checking    | ~20   |
 * | Geo      | Spatial/GIS functions (OGC compatible)   | ~25   |
 * | Vector   | Embeddings, similarity, ML operations    | ~20   |
 * | Graph    | Traversal, centrality, path finding      | ~15   |
 * 
 * Total: ~155 functions
 */

#include "query/functions/function_registry.h"
#include "query/functions/string_functions.h"
#include "query/functions/math_functions.h"
#include "query/functions/array_functions.h"
#include "query/functions/date_functions.h"
#include "query/functions/document_functions.h"
#include "query/functions/geo_functions.h"
#include "query/functions/vector_functions.h"
#include "query/functions/graph_functions.h"

namespace themis {
namespace query {
namespace functions {

void registerBuiltinFunctions() {
    auto& registry = FunctionRegistry::instance();
    
    // Core data manipulation functions
    registerStringFunctions(registry);
    registerMathFunctions(registry);
    registerArrayFunctions(registry);
    registerDateFunctions(registry);
    registerDocumentFunctions(registry);
    
    // Multi-model functions
    registerGeoFunctions(registry);      // Spatial/GIS operations
    registerVectorFunctions(registry);   // ML embeddings & similarity
    registerGraphFunctions(registry);    // Graph traversal & analysis
}

} // namespace functions
} // namespace query
} // namespace themis
