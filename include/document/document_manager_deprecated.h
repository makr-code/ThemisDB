/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            document_manager_deprecated.h                      ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:36:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f25844823  2025-12-08  Address code review feedback: improve Status initializati... ║
    • 5b3899a75  2025-12-08  Move DocumentManager from document to projects module ║
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
