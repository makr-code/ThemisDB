/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            document_manager_deprecated.h                      ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     58                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
