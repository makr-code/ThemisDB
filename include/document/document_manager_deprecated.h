/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            document_manager_deprecated.h                      ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:09:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
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
