/*
 * ThemisDB | File: file_format.h | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
