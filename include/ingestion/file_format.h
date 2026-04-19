/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            file_format.h                                      ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-19                                         ║
  Author:          Copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

namespace themis {
namespace ingestion {

/**
 * @brief Supported file formats for ingestion.
 */
enum class FileFormat {
    AUTO,       ///< Auto-detect format
    PDF,        ///< Portable Document Format
    DOCX,       ///< Microsoft Word
    TXT,        ///< Plain text
    HTML,       ///< HTML document
    XML,        ///< XML document
    JSON,       ///< JSON document
    MD,         ///< Markdown
    EPUB,       ///< EPUB e-book
    XLSX,       ///< Microsoft Excel (Open XML)
    CSV,        ///< Comma-separated values
    ZIP,        ///< ZIP archive (may contain nested documents)
    SHP,        ///< ESRI Shapefile (geo vector data)
    GEOJSON,    ///< GeoJSON geo vector data
    DXF,        ///< Drawing Exchange Format (CAD)
    PNG,        ///< Portable Network Graphics image
    JPG,        ///< JPEG image
    DB,         ///< Generic database / SQLite file
    UNKNOWN     ///< Unknown or undetected format
};

} // namespace ingestion
} // namespace themis
