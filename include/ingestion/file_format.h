/**
 * @file file_format.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
