#pragma once

/**
 * @file document_manager_deprecated.h
 * @brief Deprecated forwarding header for DocumentManager
 * 
 * @deprecated This header is deprecated. Please use "projects/DocumentManager/document_manager.h" instead.
 * 
 * DocumentManager has been moved from the document module to the projects module.
 * This forwarding header is provided for backward compatibility and will be removed in a future version.
 * 
 * Migration:
 * - Old include: #include "document/document_manager.h"
 * - New include: #include "projects/DocumentManager/document_manager.h"
 * 
 * - Old namespace: themis::document::DocumentManager
 * - New namespace: themis::projects::DocumentManager
 */

#include "projects/DocumentManager/document_manager.h"

// Provide a namespace alias for backward compatibility
namespace themis {
namespace document {
    using namespace themis::projects;
} // namespace document
} // namespace themis
