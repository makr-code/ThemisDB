/*
 * ThemisDB | File: document_manager_deprecated.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 40
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #65 Move DocumentManager from document to projects module (2026-03-11T17:04:32Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

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

// Provide type aliases for backward compatibility
namespace themis {
namespace document {
    using DocumentManager = themis::projects::DocumentManager;
    using DocumentMeta = themis::projects::DocumentMeta;
    using ChunkMeta = themis::projects::ChunkMeta;
    using ChunkingConfig = themis::projects::ChunkingConfig;
    using UploadResult = themis::projects::UploadResult;
    using Status = themis::projects::Status;
} // namespace document
} // namespace themis
