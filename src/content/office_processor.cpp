/**
 * @file office_processor.cpp
 * @brief Office document processor (XLSX, DOCX, PPTX) with content extraction.
 * @version 0.0.47
 * @note Maturity: 🟡 BETA
 * @note Score: 81/100
 * @note Gap Summary: total=8; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=3, C=1, H=2, M=5, L=0
 * @note Status: Production Ready; XLSX/DOCX/PPTX extraction working; advanced macro handling deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/office_processor.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <map>
#include <regex>
#include <sstream>
#include <string>

#include "content/content_metrics.h"
#include "security/xxe_safe_xml_parser.h"

// POSIX subprocess support for LibreOffice headless fallback
#ifndef _WIN32
#include <cerrno>
#include <ctime>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

// ZIP handling (minizip or libzip)
#ifdef THEMIS_ENABLE_OFFICE
#include <pugixml.hpp>
#include <zip.h>
#define OFFICE_LIBRARY_AVAILABLE 1
#else
#define OFFICE_LIBRARY_AVAILABLE 0
#endif

namespace themis {
namespace content {

// ============================================================================
// Constants
// ============================================================================

// OOXML Content Types
constexpr const char *DOCX_CONTENT_TYPE = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
constexpr const char *XLSX_CONTENT_TYPE = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
constexpr const char *PPTX_CONTENT_TYPE = "application/vnd.openxmlformats-officedocument.presentationml.presentation";

// ZIP signatures
constexpr uint32_t ZIP_SIGNATURE = 0x04034b50; // PK\x03\x04
constexpr size_t MAX_OFFICE_BLOB_BYTES = 256ULL * 1024ULL * 1024ULL; // 256 MiB safety ceiling

// ============================================================================
// OfficeProcessor Implementation
// ============================================================================

OfficeProcessor::OfficeProcessor() : OfficeProcessor(Config{}) {}

OfficeProcessor::OfficeProcessor(Config config) : config_(std::move(config)) {}

bool OfficeProcessor::isAvailable() {
#if OFFICE_LIBRARY_AVAILABLE
    return true;
#else
    return false;
#endif
}

OfficeDocumentType OfficeProcessor::detectDocumentType(const std::string &blob) {
    if (blob.size() < 4) {
        return OfficeDocumentType::UNKNOWN;
    }

    // Check ZIP signature
    uint32_t sig = 0;
    std::memcpy(&sig, blob.data(), 4);
    if (sig != ZIP_SIGNATURE) {
        // Check for legacy Office formats (OLE Compound Document)
        if (blob.size() >= 8) {
            const unsigned char *data = reinterpret_cast<const unsigned char *>(blob.data());
            // Full 8-byte OLE Compound Document header: D0 CF 11 E0 A1 B1 1A E1
            if (data[0] == 0xD0 && data[1] == 0xCF && data[2] == 0x11 && data[3] == 0xE0 && data[4] == 0xA1
                && data[5] == 0xB1 && data[6] == 0x1A && data[7] == 0xE1) {
                // Legacy Office format - try to determine type
                // This is a simplified check
                if (blob.find("WordDocument") != std::string::npos) {
                    return OfficeDocumentType::DOC;
                }
                if (blob.find("Workbook") != std::string::npos) {
                    return OfficeDocumentType::XLS;
                }
                if (blob.find("PowerPoint") != std::string::npos) {
                    return OfficeDocumentType::PPT;
                }
            }
        }
        // Check for RTF
        if (blob.size() >= 5 && blob.substr(0, 5) == "{\\rtf") {
            return OfficeDocumentType::RTF;
        }
        return OfficeDocumentType::UNKNOWN;
    }

    // It's a ZIP file - check for OOXML or ODF
    // Look for [Content_Types].xml (OOXML) or mimetype (ODF)

    // Simple pattern matching for document type detection
    // DOCX: word/document.xml
    // XLSX: xl/workbook.xml
    // PPTX: ppt/presentation.xml
    // ODT: mimetype starts with "application/vnd.oasis.opendocument.text"

    if (blob.find("word/document.xml") != std::string::npos || blob.find("word/_rels") != std::string::npos) {
        return OfficeDocumentType::DOCX;
    }
    if (blob.find("xl/workbook.xml") != std::string::npos || blob.find("xl/_rels") != std::string::npos) {
        return OfficeDocumentType::XLSX;
    }
    if (blob.find("ppt/presentation.xml") != std::string::npos || blob.find("ppt/_rels") != std::string::npos) {
        return OfficeDocumentType::PPTX;
    }
    if (blob.find("application/vnd.oasis.opendocument.text") != std::string::npos) {
        return OfficeDocumentType::ODT;
    }
    if (blob.find("application/vnd.oasis.opendocument.spreadsheet") != std::string::npos) {
        return OfficeDocumentType::ODS;
    }
    if (blob.find("application/vnd.oasis.opendocument.presentation") != std::string::npos) {
        return OfficeDocumentType::ODP;
    }

    return OfficeDocumentType::UNKNOWN;
}

ExtractionResult OfficeProcessor::extract(const std::string &blob, const ContentType & /*content_type*/
) {
    ExtractionResult result;
    result.ok       = false;
    result.metadata = json::object();

    if (blob.empty()) {
        result.error_message = "Empty Office payload";
        if (config_.metrics) {
            config_.metrics->recordExtractError();
        }
        return result;
    }

    if (blob.size() > MAX_OFFICE_BLOB_BYTES) {
        result.error_message = "Office payload exceeds maximum supported size";
        result.metadata["size_bytes"] = blob.size();
        result.metadata["max_size_bytes"] = MAX_OFFICE_BLOB_BYTES;
        if (config_.metrics) {
            config_.metrics->recordExtractError();
        }
        return result;
    }

    // Detect document type
    OfficeDocumentType doc_type   = detectDocumentType(blob);
    result.metadata["size_bytes"] = blob.size();

    switch (doc_type) {
        case OfficeDocumentType::DOCX:
            result.metadata["document_type"] = "docx";
            result.metadata["mime_type"]     = DOCX_CONTENT_TYPE;
            result                           = extractDOCX(blob);
            break;

        case OfficeDocumentType::XLSX:
            result.metadata["document_type"] = "xlsx";
            result.metadata["mime_type"]     = XLSX_CONTENT_TYPE;
            result                           = extractXLSX(blob);
            break;

        case OfficeDocumentType::PPTX:
            result.metadata["document_type"] = "pptx";
            result.metadata["mime_type"]     = PPTX_CONTENT_TYPE;
            result                           = extractPPTX(blob);
            break;

        case OfficeDocumentType::ODT:
        [[fallthrough]];\n        case OfficeDocumentType::ODS:
        [[fallthrough]];\n        case OfficeDocumentType::ODP:
            result.metadata["document_type"] = "odf";
            result                           = extractODF(blob, doc_type);
            break;

        case OfficeDocumentType::DOC:
        [[fallthrough]];\n        case OfficeDocumentType::XLS:
        [[fallthrough]];\n        case OfficeDocumentType::PPT:
            result = extractLegacyViaLibreOffice(blob, doc_type);
            break;

        case OfficeDocumentType::RTF:
            result.metadata["document_type"] = "rtf";
            // Basic RTF text extraction
            {
                std::string text = {};
                std::regex rtf_text_regex("\\\\([a-z]+)\\s*([^\\\\{}]+)");
                std::sregex_iterator it(blob.begin(), blob.end(), rtf_text_regex);
                std::sregex_iterator end = {};

                for (; it != end; ++it) {
                    std::string control = (*it)[1].str();
                    std::string content = (*it)[2].str();
                    // Skip control words, keep text
                    if (control.empty() || control == "pard" || control == "par") {
                        text += content + " ";
                    }
                }
                result.text                          = text;
                result.metadata["extraction_method"] = "basic_rtf";
                result.ok                            = true;
            }
            break;

        default:
            result.error_message = "Unknown or unsupported Office document format";
            break;
    }

    // Report metrics if a ContentMetrics instance was configured
    if (config_.metrics) {
        if (result.ok) {
            config_.metrics->recordOfficeExtracted();
        } else {
            config_.metrics->recordExtractError();
        }
    }

    return result;
}

ExtractionResult OfficeProcessor::extractDOCX(const std::string &blob) {
    ExtractionResult result;
    result.ok                        = false;
    result.metadata                  = json::object();
    result.metadata["document_type"] = "docx";
    result.metadata["mime_type"]     = DOCX_CONTENT_TYPE;

#if OFFICE_LIBRARY_AVAILABLE
    try {
        // Read document.xml from ZIP
        std::string document_xml = readZipEntry(blob, "word/document.xml");

        if (document_xml.empty()) {
            result.error_message = "Failed to read word/document.xml from DOCX";
            return result;
        }

        // Extract metadata
        OfficeMetadata metadata          = extractOOXMLMetadata(blob);
        result.metadata["title"]         = metadata.title;
        result.metadata["author"]        = metadata.author;
        result.metadata["subject"]       = metadata.subject;
        result.metadata["keywords"]      = metadata.keywords;
        result.metadata["created_date"]  = metadata.created_date;
        result.metadata["modified_date"] = metadata.modified_date;
        result.metadata["application"]   = metadata.application;

        // Parse XML and extract text
        auto parse_result = themis::security::parseXmlSafe(document_xml, "DOCX document.xml");
        if (!parse_result.success) {
            result.error_message = "Failed to parse document.xml: " + parse_result.error_message;
            return result;
        }

        pugi::xml_document& doc = parse_result.document;

        // Extract paragraphs
        std::vector<std::string> paragraphs;
        std::ostringstream all_text = {};

        // Navigate to w:body/w:p elements
        for (auto p : doc.select_nodes("//w:p")) {
            std::string para_text = {};
            for (auto t : p.node().select_nodes(".//w:t")) {
                para_text += t.node().child_value();
            }
            if (!para_text.empty()) {
                paragraphs.push_back(para_text);
                all_text << para_text << "\n";
            }
        }

        result.text                        = all_text.str();
        result.metadata["paragraph_count"] = paragraphs.size();
        result.metadata["token_count"]     = countTokens(result.text);
        result.ok                          = true;

    } catch (const std::exception &e) {
        result.error_message = std::string("DOCX extraction error: ") + e.what();
    }
#else
    // Fallback: Basic extraction without libzip/pugixml
    result.metadata["note"] = "Full DOCX extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";

    // Try to find text between <w:t> tags
    std::regex text_regex("<w:t[^>]*>([^<]+)</w:t>");
    std::ostringstream extracted = {};

    auto text_begin = std::sregex_iterator(blob.begin(), blob.end(), text_regex);
    auto text_end   = std::sregex_iterator();

    for (auto it = text_begin; it != text_end; ++it) {
        extracted << (*it)[1].str() << " ";
    }

    result.text                          = extracted.str();
    result.metadata["extraction_method"] = "basic_regex";
    result.metadata["token_count"]       = countTokens(result.text);
    result.ok                            = true;
#endif

    return result;
}

ExtractionResult OfficeProcessor::extractXLSX(const std::string &blob) {
    ExtractionResult result;
    result.ok                        = false;
    result.metadata                  = json::object();
    result.metadata["document_type"] = "xlsx";
    result.metadata["mime_type"]     = XLSX_CONTENT_TYPE;

#if OFFICE_LIBRARY_AVAILABLE
    try {
        // Read shared strings
        std::string shared_strings_xml = readZipEntry(blob, "xl/sharedStrings.xml");

        // Parse shared strings
        std::vector<std::string> shared_strings = {};

        if (!shared_strings_xml.empty()) {
            auto ss_parse = themis::security::parseXmlSafe(shared_strings_xml, "XLSX sharedStrings.xml");
            if (ss_parse.success) {
                pugi::xml_document& ss_doc = ss_parse.document;
                for (auto si : ss_doc.select_nodes("//si")) {
                    std::string text = {};
                    for (auto t : si.node().select_nodes(".//t")) {
                        text += t.node().child_value();
                    }
                    shared_strings.push_back(text);
                }
            }
        }

        // Extract metadata
        OfficeMetadata metadata          = extractOOXMLMetadata(blob);
        result.metadata["title"]         = metadata.title;
        result.metadata["author"]        = metadata.author;
        result.metadata["created_date"]  = metadata.created_date;
        result.metadata["modified_date"] = metadata.modified_date;

        // Read workbook.xml to get sheet names
        std::string workbook_xml = readZipEntry(blob, "xl/workbook.xml");
        std::vector<std::string> sheet_names = {};

        if (!workbook_xml.empty()) {
            auto wb_parse = themis::security::parseXmlSafe(workbook_xml, "XLSX workbook.xml");
            if (wb_parse.success) {
                pugi::xml_document& wb_doc = wb_parse.document;
                for (auto sheet : wb_doc.select_nodes("//sheet")) {
                    const char *name = sheet.node().attribute("name").value();
                    if (name) {
                        sheet_names.push_back(name);
                    }
                }
            }
        }

        result.metadata["sheet_count"] = sheet_names.size();
        result.metadata["sheet_names"] = sheet_names;

        // Read first sheet
        std::string sheet1_xml = readZipEntry(blob, "xl/worksheets/sheet1.xml");
        std::ostringstream all_text = {};
        int row_count  = 0;
        int cell_count = 0;

        if (!sheet1_xml.empty()) {
            auto sheet_parse = themis::security::parseXmlSafe(sheet1_xml, "XLSX sheet1.xml");
            if (sheet_parse.success) {
                pugi::xml_document& sheet_doc = sheet_parse.document;
                for (auto row : sheet_doc.select_nodes("//row")) {
                    row_count++;
                    std::vector<std::string> row_values;

                    for (auto cell : row.node().select_nodes("c")) {
                        cell_count++;
                        if (cell_count > config_.max_cell_count) {
                            break;
                        }

                        std::string value = {};
                        const char *type = cell.node().attribute("t").value();
                        auto v_node      = cell.node().child("v");

                        if (v_node) {
                            if (type && std::string(type) == "s") {
                                // Shared string reference
                                int idx = std::stoi(v_node.child_value());
                                if (idx >= 0 && idx < static_cast<int>(shared_strings.size())) {
                                    value = shared_strings[idx];
                                }
                            } else {
                                value = v_node.child_value();
                            }
                        }

                        row_values.push_back(value);
                    }

                    if (cell_count > config_.max_cell_count) {
                        break;
                    }

                    // Join row values
                    for (size_t i = 0; i < row_values.size(); ++i) {
                        all_text << row_values[i];
                        if (i + 1 < row_values.size()) {
                            all_text << "\t";
                        }
                    }
                    all_text << "\n";
                }
            }
        }

        result.text                    = all_text.str();
        result.metadata["row_count"]   = row_count;
        result.metadata["cell_count"]  = cell_count;
        result.metadata["token_count"] = countTokens(result.text);
        result.ok                      = true;

    } catch (const std::exception &e) {
        result.error_message = std::string("XLSX extraction error: ") + e.what();
    }
#else
    result.metadata["note"]              = "Full XLSX extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";
    result.metadata["extraction_method"] = "not_available";
    result.text                          = "";
    result.ok                            = true;
#endif

    return result;
}

ExtractionResult OfficeProcessor::extractPPTX(const std::string &blob) {
    ExtractionResult result;
    result.ok                        = false;
    result.metadata                  = json::object();
    result.metadata["document_type"] = "pptx";
    result.metadata["mime_type"]     = PPTX_CONTENT_TYPE;

#if OFFICE_LIBRARY_AVAILABLE
    try {
        // Extract metadata
        OfficeMetadata metadata          = extractOOXMLMetadata(blob);
        result.metadata["title"]         = metadata.title;
        result.metadata["author"]        = metadata.author;
        result.metadata["created_date"]  = metadata.created_date;
        result.metadata["modified_date"] = metadata.modified_date;

        // List slides
        std::vector<std::string> slide_files = listZipEntries(blob);
        std::vector<std::string> slides;

        for (const auto &entry : slide_files) {
            if (entry.find("ppt/slides/slide") != std::string::npos && entry.find(".xml") != std::string::npos) {
                slides.push_back(entry);
            }
        }

        // Sort slides by number
        std::sort(slides.begin(), slides.end());

        result.metadata["slide_count"] = slides.size();

        std::ostringstream all_text = {};
        int slide_num = 1;

        for (const auto &slide_path : slides) {
            std::string slide_xml = readZipEntry(blob, slide_path);

            if (slide_xml.empty()) {
                continue;
            }

            auto slide_parse = themis::security::parseXmlSafe(slide_xml, "PPTX slide.xml");
            if (!slide_parse.success) {
                continue;
            }

            pugi::xml_document& slide_doc = slide_parse.document;

            all_text << "--- Slide " << slide_num << " ---\n";

            // Extract text from a:t elements
            for (auto t : slide_doc.select_nodes("//a:t")) {
                all_text << t.node().child_value() << " ";
            }

            all_text << "\n\n";
            slide_num++;
        }

        // Extract speaker notes if requested
        if (config_.extract_speaker_notes) {
            for (size_t i = 1; i <= slides.size(); ++i) {
                std::string notes_path = "ppt/notesSlides/notesSlide" + std::to_string(i) + ".xml";
                std::string notes_xml  = readZipEntry(blob, notes_path);

                if (notes_xml.empty()) {
                    continue;
                }

                auto notes_parse = themis::security::parseXmlSafe(notes_xml, "PPTX notes.xml");
                if (!notes_parse.success) {
                    continue;
                }

                pugi::xml_document& notes_doc = notes_parse.document;

                all_text << "--- Notes for Slide " << i << " ---\n";
                for (auto t : notes_doc.select_nodes("//a:t")) {
                    all_text << t.node().child_value() << " ";
                }
                all_text << "\n\n";
            }
        }

        result.text                    = all_text.str();
        result.metadata["token_count"] = countTokens(result.text);
        result.ok                      = true;

    } catch (const std::exception &e) {
        result.error_message = std::string("PPTX extraction error: ") + e.what();
    }
#else
    result.metadata["note"]              = "Full PPTX extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";
    result.metadata["extraction_method"] = "not_available";
    result.text                          = "";
    result.ok                            = true;
#endif

    return result;
}

ExtractionResult OfficeProcessor::extractODF(const std::string &blob, OfficeDocumentType type) {
    ExtractionResult result;
    result.ok       = false;
    result.metadata = json::object();

    std::string type_str = {};
    switch (type) {
        case OfficeDocumentType::ODT:
            type_str = "odt";
            break;
        case OfficeDocumentType::ODS:
            type_str = "ods";
            break;
        case OfficeDocumentType::ODP:
            type_str = "odp";
            break;
        default:
            type_str = "odf";
            break;
    }
    result.metadata["document_type"] = type_str;

#if OFFICE_LIBRARY_AVAILABLE
    try {
        // Read content.xml
        std::string content_xml = readZipEntry(blob, "content.xml");

        if (content_xml.empty()) {
            result.error_message = "Failed to read content.xml from ODF";
            return result;
        }

        // Parse and extract text
        auto content_parse = themis::security::parseXmlSafe(content_xml, "Office content.xml");
        if (!content_parse.success) {
            result.error_message = "Failed to parse content.xml: " + content_parse.error_message;
            return result;
        }

        pugi::xml_document& doc = content_parse.document;

        std::ostringstream all_text = {};

        // Extract text from text:p and text:h elements
        for (auto node : doc.select_nodes("//text:p | //text:h")) {
            std::string para_text = {};
            for (auto child : node.node().children()) {
                if (child.type() == pugi::node_pcdata) {
                    para_text += child.value();
                } else if (std::string(child.name()).find("text:") == 0) {
                    para_text += child.child_value();
                }
            }
            if (!para_text.empty()) {
                all_text << para_text << "\n";
            }
        }

        result.text                    = all_text.str();
        result.metadata["token_count"] = countTokens(result.text);
        result.ok                      = true;

    } catch (const std::exception &e) {
        result.error_message = std::string("ODF extraction error: ") + e.what();
    }
#else
    result.metadata["note"] = "Full ODF extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";
    result.text             = "";
    result.ok               = true;
#endif

    return result;
}

#if OFFICE_LIBRARY_AVAILABLE
std::string OfficeProcessor::readZipEntry(const std::string &zip_blob, const std::string &entry_path) {
    zip_error_t error;
    zip_error_init(&error);

    zip_source_t *source = zip_source_buffer_create(zip_blob.data(), zip_blob.size(), 0, &error);

    if (!source) {
        zip_error_fini(&error);
        return "";
    }

    zip_t *archive = zip_open_from_source(source, ZIP_RDONLY, &error);
    if (!archive) {
        zip_source_free(source);
        zip_error_fini(&error);
        return "";
    }

    zip_int64_t index = zip_name_locate(archive, entry_path.c_str(), 0);
    if (index < 0) {
        zip_close(archive);
        return "";
    }

    zip_stat_t stat = {};
    if (zip_stat_index(archive, index, 0, &stat) != 0) {
        zip_close(archive);
        return "";
    }

    zip_file_t *file = zip_fopen_index(archive, index, 0);
    if (!file) {
        zip_close(archive);
        return "";
    }

    std::string content(stat.size, '\0');
    zip_fread(file, content.data(), stat.size);

    zip_fclose(file);
    zip_close(archive);

    return content;
}

OfficeMetadata OfficeProcessor::extractOOXMLMetadata(const std::string &zip_blob) {
    OfficeMetadata metadata;

    // Read docProps/core.xml
    std::string core_xml = readZipEntry(zip_blob, "docProps/core.xml");
    if (core_xml.empty()) {
        return metadata;
    }

    auto core_parse = themis::security::parseXmlSafe(core_xml, "Office core.xml");
    if (!core_parse.success) {
        return metadata;
    }

    pugi::xml_document& doc = core_parse.document;

    // Extract properties
    auto get_text = [&doc](const char *xpath) -> std::string {
        auto node = doc.select_node(xpath);
        return node ? node.node().child_value() : "";
    };

    metadata.title            = get_text("//dc:title");
    metadata.author           = get_text("//dc:creator");
    metadata.subject          = get_text("//dc:subject");
    metadata.keywords         = get_text("//cp:keywords");
    metadata.last_modified_by = get_text("//cp:lastModifiedBy");
    metadata.created_date     = get_text("//dcterms:created");
    metadata.modified_date    = get_text("//dcterms:modified");

    // Read docProps/app.xml for application info
    std::string app_xml = readZipEntry(zip_blob, "docProps/app.xml");
    if (!app_xml.empty()) {
        auto app_parse = themis::security::parseXmlSafe(app_xml, "Office app.xml");
        if (app_parse.success) {
            pugi::xml_document& app_doc = app_parse.document;
            auto app_node = app_doc.select_node("//Application");
            if (app_node) {
                metadata.application = app_node.node().child_value();
            }
        }
    }

    return metadata;
}

std::vector<std::string> OfficeProcessor::listZipEntries(const std::string &zip_blob) {
    std::vector<std::string> entries;

    zip_error_t error;
    zip_error_init(&error);

    zip_source_t *source = zip_source_buffer_create(zip_blob.data(), zip_blob.size(), 0, &error);

    if (!source) {
        zip_error_fini(&error);
        return entries;
    }

    zip_t *archive = zip_open_from_source(source, ZIP_RDONLY, &error);
    if (!archive) {
        zip_source_free(source);
        zip_error_fini(&error);
        return entries;
    }

    zip_int64_t num_entries = zip_get_num_entries(archive, 0);
    for (zip_int64_t i = 0; i < num_entries; ++i) {
        const char *name = zip_get_name(archive, i, 0);
        if (name) {
            entries.push_back(name);
        }
    }

    zip_close(archive);
    return entries;
}
#else
std::string OfficeProcessor::readZipEntry(const std::string &, const std::string &) {
    return "";
}

OfficeMetadata OfficeProcessor::extractOOXMLMetadata(const std::string &) {
    return OfficeMetadata{};
}

std::vector<std::string> OfficeProcessor::listZipEntries(const std::string &) {
    return {};
}
#endif

// ============================================================================
// LibreOffice headless fallback for legacy OLE formats (DOC/XLS/PPT)
// ============================================================================

ExtractionResult OfficeProcessor::extractLegacyViaLibreOffice(const std::string &blob, OfficeDocumentType doc_type) {
    ExtractionResult result;
    result.ok       = false;
    result.metadata = json::object();

    // Map document type to file extension and metadata string
    const char *ext      = nullptr;
    const char *type_str = nullptr;
    switch (doc_type) {
        case OfficeDocumentType::DOC:
            ext      = ".doc";
            type_str = "doc";
            break;
        case OfficeDocumentType::XLS:
            ext      = ".xls";
            type_str = "xls";
            break;
        case OfficeDocumentType::PPT:
            ext      = ".ppt";
            type_str = "ppt";
            break;
        default:
            result.error_message = "extractLegacyViaLibreOffice: unsupported document type";
            return result;
    }
    result.metadata["document_type"]     = type_str;
    result.metadata["extraction_method"] = "libreoffice_headless";

    // Validate OLE Compound Document header: D0 CF 11 E0 A1 B1 1A E1 (all 8 bytes)
    if (blob.size() < 8) {
        result.error_message = "Legacy Office document too small (< 8 bytes)";
        return result;
    }
    const auto *raw = reinterpret_cast<const unsigned char *>(blob.data());
    if (!(raw[0] == 0xD0 && raw[1] == 0xCF && raw[2] == 0x11 && raw[3] == 0xE0 && raw[4] == 0xA1 && raw[5] == 0xB1
          && raw[6] == 0x1A && raw[7] == 0xE1)) {
        result.error_message = "Legacy Office document has invalid OLE header (expected D0 CF 11 E0 A1 B1 1A E1)";
        return result;
    }

#ifndef _WIN32
    // -----------------------------------------------------------------------
    // Create an isolated temp directory.
    // Use P_tmpdir (POSIX) to honour TMPDIR; fall back to /tmp.
    // -----------------------------------------------------------------------
    std::string tmp_base   = P_tmpdir ? std::string(P_tmpdir) : std::string("/tmp");
    std::string tmpdir_tpl = tmp_base + "/themisdb_lo_XXXXXX";
    std::vector<char> tmpdir_buf(tmpdir_tpl.begin(), tmpdir_tpl.end());
    tmpdir_buf.push_back('\0');
    char *tmpdir_ptr = mkdtemp(tmpdir_buf.data());
    if (!tmpdir_ptr) {
        result.error_message = std::string("Failed to create temp directory: ") + strerror(errno);
        return result;
    }
    std::string tmp_dir(tmpdir_ptr);

    // RAII guard: always remove temp dir + tracked files on scope exit
    struct TempGuard {
        std::string dir = {};
        std::string in_file = {};
        std::string out_file = {};
        ~TempGuard() {
            if (!in_file.empty())
                unlink(in_file.c_str());
            if (!out_file.empty())
                unlink(out_file.c_str());
            if (!dir.empty())
                rmdir(dir.c_str());
        }
    } guard;
    guard.dir = tmp_dir;

    // -----------------------------------------------------------------------
    // Write the blob to a temp input file.
    // mkstemps ensures the name is unpredictable and collision-free.
    // -----------------------------------------------------------------------
    std::string in_tpl = tmp_dir + "/input_XXXXXX" + ext;
    std::vector<char> in_buf(in_tpl.begin(), in_tpl.end());
    in_buf.push_back('\0');
    int in_fd = mkstemps(in_buf.data(), static_cast<int>(strlen(ext)));
    if (in_fd < 0) {
        result.error_message = std::string("Failed to create temp input file: ") + strerror(errno);
        return result;
    }
    guard.in_file = std::string(in_buf.data());

    // Restrict permissions: only the current user may read/write the temp file
    fchmod(in_fd, S_IRUSR | S_IWUSR);

    const char *bdata = blob.data();
    size_t remaining  = blob.size();
    while (remaining > 0) {
        ssize_t written = write(in_fd, bdata, remaining);
        if (written < 0) {
            if (errno == EINTR)
                continue; // retry on signal interrupt
            close(in_fd);
            result.error_message = std::string("Failed to write temp input file: ") + strerror(errno);
            return result;
        }
        bdata += written;
        remaining -= static_cast<size_t>(written);
    }
    close(in_fd);

    // -----------------------------------------------------------------------
    // Build soffice argv.
    // Command: soffice --headless --convert-to txt --outdir <tmpdir> <infile>
    // -----------------------------------------------------------------------
    const std::string lo_path
        = config_.libreoffice_path.empty() ? std::string("/usr/bin/soffice") : config_.libreoffice_path;

    // Require an absolute path to prevent PATH-hijacking attacks
    if (lo_path.empty() || lo_path[0] != '/') {
        result.error_message = "libreoffice_path must be an absolute path (got: '" + lo_path + "')";
        return result;
    }

    char *const argv[] = {const_cast<char *>(lo_path.c_str()),
                          const_cast<char *>("--headless"),
                          const_cast<char *>("--convert-to"),
                          const_cast<char *>("txt"),
                          const_cast<char *>("--outdir"),
                          const_cast<char *>(tmp_dir.c_str()),
                          const_cast<char *>(guard.in_file.c_str()),
                          nullptr};

    // Minimal, sanitized environment to limit the subprocess attack surface.
    // HOME is set to the tmp dir so soffice cannot modify the user's home dir.
    std::string home_env   = "HOME=" + tmp_dir;
    std::string tmpdir_env = "TMPDIR=" + tmp_dir;
    char *const envp[] = {const_cast<char *>("PATH=/usr/bin:/usr/local/bin:/bin"), const_cast<char *>(home_env.c_str()),
                          const_cast<char *>(tmpdir_env.c_str()), nullptr};

    // -----------------------------------------------------------------------
    // Configure posix_spawn file actions:
    //   • stdout and stderr → /dev/null (avoid polluting caller's streams)
    // -----------------------------------------------------------------------
    posix_spawn_file_actions_t file_actions;
    posix_spawn_file_actions_init(&file_actions);
    posix_spawn_file_actions_addopen(&file_actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addopen(&file_actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);

    // -----------------------------------------------------------------------
    // Configure posix_spawnattr:
    //   • POSIX_SPAWN_SETPGROUP  – new process group (enables kill(-pgid) on timeout)
    //   • POSIX_SPAWN_SETSIGDEF  – reset all signal handlers to default
    //   • POSIX_SPAWN_RESETIDS   – effective UID/GID = real UID/GID (drop SUID bits)
    // -----------------------------------------------------------------------
    posix_spawnattr_t spawnattr;
    posix_spawnattr_init(&spawnattr);

    short flags = POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_RESETIDS;
    posix_spawnattr_setflags(&spawnattr, flags);
    posix_spawnattr_setpgroup(&spawnattr, 0); // subprocess in its own process group

    sigset_t sigset_all;
    sigfillset(&sigset_all);
    posix_spawnattr_setsigdefault(&spawnattr, &sigset_all);

    // -----------------------------------------------------------------------
    // Spawn soffice.
    // -----------------------------------------------------------------------
    pid_t pid     = -1;
    int spawn_ret = posix_spawn(&pid, lo_path.c_str(), &file_actions, &spawnattr, argv, envp);
    posix_spawn_file_actions_destroy(&file_actions);
    posix_spawnattr_destroy(&spawnattr);

    if (spawn_ret != 0) {
        result.error_message = std::string("posix_spawn failed for soffice: ") + strerror(spawn_ret);
        return result;
    }

    // -----------------------------------------------------------------------
    // Wait for completion with a hard timeout.
    // Poll every 100 ms; kill the entire process group on timeout.
    // -----------------------------------------------------------------------
    int timeout_sec = (config_.libreoffice_timeout_seconds > 0) ? config_.libreoffice_timeout_seconds : 30;

    // Record the deadline as wall-clock seconds via clock_gettime
    struct timespec now_ts{};
    clock_gettime(CLOCK_MONOTONIC, &now_ts);
    long deadline_sec = now_ts.tv_sec + timeout_sec;

    int wait_status = -1;
    bool timed_out  = false;

    while (true) {
        pid_t wp = waitpid(pid, &wait_status, WNOHANG);
        if (wp == pid) {
            break; // child finished
        }
        if (wp < 0) {
            // waitpid error: terminate the group and abort
            kill(-pid, SIGTERM);
            struct timespec grace = {2, 0};
            nanosleep(&grace, nullptr);
            kill(-pid, SIGKILL);
            waitpid(pid, &wait_status, 0);
            result.error_message = std::string("waitpid error: ") + strerror(errno);
            return result;
        }

        // Check timeout
        clock_gettime(CLOCK_MONOTONIC, &now_ts);
        if (now_ts.tv_sec >= deadline_sec) {
            timed_out = true;
            // Two-phase shutdown: SIGTERM first, escalate to SIGKILL after 2 s
            kill(-pid, SIGTERM);
            {
                struct timespec grace = {2, 0};
                nanosleep(&grace, nullptr);
            }
            if (waitpid(pid, &wait_status, WNOHANG) != pid) {
                kill(-pid, SIGKILL);
                waitpid(pid, &wait_status, 0);
            }
            break;
        }

        // Sleep 100 ms before next poll
        struct timespec sleep_ts = {0, 100000000L};
        nanosleep(&sleep_ts, nullptr);
    }

    if (timed_out) {
        result.error_message = "LibreOffice conversion timed out after " + std::to_string(timeout_sec) + " seconds";
        result.metadata["timed_out"] = true;
        return result;
    }

    if (!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0) {
        int exit_code        = WIFEXITED(wait_status) ? WEXITSTATUS(wait_status) : -1;
        result.error_message = "LibreOffice conversion failed with exit code " + std::to_string(exit_code);
        return result;
    }

    // -----------------------------------------------------------------------
    // Locate the output file.
    // soffice writes <basename_without_ext>.txt into --outdir.
    // -----------------------------------------------------------------------
    std::string in_full     = guard.in_file;
    std::size_t slash_pos   = in_full.rfind('/');
    std::string in_basename = (slash_pos != std::string::npos) ? in_full.substr(slash_pos + 1) : in_full;
    // Strip the original extension and append .txt
    std::size_t ext_len      = strlen(ext);
    std::string out_basename = (in_basename.size() > ext_len)
                                   ? in_basename.substr(0, in_basename.size() - ext_len) + ".txt"
                                   : in_basename + ".txt";
    std::string out_path     = tmp_dir + "/" + out_basename;
    guard.out_file           = out_path;

    int out_fd = open(out_path.c_str(), O_RDONLY);
    if (out_fd < 0) {
        result.error_message = "LibreOffice output file not found at: " + out_path;
        return result;
    }

    // Read converted text
    std::string extracted_text = {};
    {
        char buf[4096];
        ssize_t n;
        while ((n = read(out_fd, buf, sizeof(buf))) > 0) {
            extracted_text.append(buf, static_cast<size_t>(n));
        }
        if (n < 0) {
            close(out_fd);
            result.error_message = std::string("Failed to read LibreOffice output file: ") + strerror(errno);
            return result;
        }
        close(out_fd);
    }

    result.text                    = extracted_text;
    result.metadata["token_count"] = countTokens(extracted_text);
    result.metadata["size_bytes"]  = blob.size();
    result.ok                      = true;
    return result;

#else
    // Windows: LibreOffice headless fallback via posix_spawn is not supported.
    result.error_message = "LibreOffice headless fallback is not supported on Windows";
    return result;
#endif
}

std::vector<json> OfficeProcessor::chunk(const ExtractionResult &extraction_result, int chunk_size, int /*overlap*/
) {
    std::vector<json> chunks;

    const std::string &text = extraction_result.text;
    if (text.empty()) {
        return chunks;
    }

    // Split by paragraphs first
    std::vector<std::string> paragraphs;
    std::istringstream stream(text);
    std::string line = {};
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            paragraphs.push_back(line);
        }
    }

    // Group paragraphs into chunks
    int seq_num = 0;
    std::string current_chunk = {};
    int current_tokens = 0;

    for (const auto &para : paragraphs) {
        int para_tokens = countTokens(para);

        if (current_tokens + para_tokens > chunk_size && !current_chunk.empty()) {
            json chunk = {{"text", current_chunk},
                          {"seq_num", seq_num},
                          {"token_count", current_tokens},
                          {"source_type", "office"}};
            chunks.push_back(chunk);
            seq_num++;

            current_chunk  = para;
            current_tokens = para_tokens;
        } else {
            if (!current_chunk.empty()) {
                current_chunk += "\n";
            }
            current_chunk += para;
            current_tokens += para_tokens;
        }
    }

    if (!current_chunk.empty()) {
        json chunk = {
            {"text", current_chunk}, {"seq_num", seq_num}, {"token_count", current_tokens}, {"source_type", "office"}};
        chunks.push_back(chunk);
    }

    return chunks;
}

std::vector<float> OfficeProcessor::generateEmbedding(const std::string &chunk_data) {
    // Hash-projection embedding (768-dim, L2-normalised) matching the
    // approach used by TextProcessor::generateEmbedding().  Each token
    // influences 30 dimensions via sine-phase mixing so that documents
    // with different vocabulary produce distinct vectors.  Semantically
    // identical but differently worded chunks will differ; this is expected
    // until a real IEmbeddingBackend is injected (see
    // src/content/FUTURE_ENHANCEMENTS.md §OfficeProcessor Embedding Integration).
    constexpr int kDim = 768;
    std::vector<float> embedding(kDim, 0.0f);

    if (chunk_data.empty()) {
        return embedding;
    }

    std::hash<std::string> hasher;
    std::istringstream iss(chunk_data);
    std::vector<std::string> tokens;
    std::string tok = {};
    while (iss >> tok) {
        tokens.push_back(tok);
    }
    if (tokens.empty()) {
        return embedding;
    }

    for (size_t i = 0; i < tokens.size(); ++i) {
        const size_t token_hash = hasher(tokens[i]);
        for (int seed = 0; seed < 3; ++seed) {
            const size_t combined = token_hash ^ (i * 31u) ^ (static_cast<size_t>(seed) * 97u);
            for (int d = 0; d < 10; ++d) {
                const int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73u) % static_cast<size_t>(kDim));
                const float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);
                const float phase
                    = static_cast<float>((combined + static_cast<size_t>(dim)) % 360u) * 3.14159f / 180.0f;
                embedding[dim] += std::sin(phase) * weight;
            }
        }
    }

    float norm = 0.0f;
    for (float v : embedding) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm > 1e-6f) {
        for (float &v : embedding) {
            v /= norm;
        }
    }
    return embedding;
}

int OfficeProcessor::countTokens(const std::string &text) {
    if (text.empty()) {
        return 0;
    }

    int count     = 1;
    bool in_space = true;

    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            in_space = true;
        } else if (in_space) {
            count++;
            in_space = false;
        }
    }

    return count;
}

bool OfficeProcessor::isValidOOXML(const std::string &blob) {
    return blob.find("[Content_Types].xml") != std::string::npos;
}

bool OfficeProcessor::isValidODF(const std::string &blob) {
    return blob.find("mimetype") != std::string::npos
           && blob.find("application/vnd.oasis.opendocument") != std::string::npos;
}

std::unique_ptr<IContentProcessor> createOfficeProcessor() {
    return std::make_unique<OfficeProcessor>(OfficeProcessor::Config{});
}

std::unique_ptr<IContentProcessor> createOfficeProcessor(OfficeProcessor::Config config) {
    return std::make_unique<OfficeProcessor>(std::move(config));
}

} // namespace content
} // namespace themis
