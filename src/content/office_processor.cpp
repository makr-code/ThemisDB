/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            office_processor.cpp                               ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     833                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file office_processor.cpp
 * @brief Office Content Processor Implementation
 * 
 * Extracts text and metadata from Office documents (DOCX, XLSX, PPTX, ODF).
 * 
 * Build with -DTHEMIS_ENABLE_OFFICE=ON to enable full ZIP/XML parsing.
 * Basic extraction uses built-in minizip + pugixml.
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#include "content/office_processor.h"
#include <regex>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <map>

// ZIP handling (minizip or libzip)
#ifdef THEMIS_ENABLE_OFFICE
#include <zip.h>
#include <pugixml.hpp>
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
constexpr const char* DOCX_CONTENT_TYPE = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
constexpr const char* XLSX_CONTENT_TYPE = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
constexpr const char* PPTX_CONTENT_TYPE = "application/vnd.openxmlformats-officedocument.presentationml.presentation";

// ZIP signatures
constexpr uint32_t ZIP_SIGNATURE = 0x04034b50;  // PK\x03\x04

// ============================================================================
// OfficeProcessor Implementation
// ============================================================================

OfficeProcessor::OfficeProcessor()
    : OfficeProcessor(Config{})
{
}

OfficeProcessor::OfficeProcessor(Config config)
    : config_(std::move(config))
{
}

bool OfficeProcessor::isAvailable() {
#if OFFICE_LIBRARY_AVAILABLE
    return true;
#else
    return false;
#endif
}

OfficeDocumentType OfficeProcessor::detectDocumentType(const std::string& blob) {
    if (blob.size() < 4) {
        return OfficeDocumentType::UNKNOWN;
    }
    
    // Check ZIP signature
    uint32_t sig = 0;
    std::memcpy(&sig, blob.data(), 4);
    if (sig != ZIP_SIGNATURE) {
        // Check for legacy Office formats (OLE Compound Document)
        if (blob.size() >= 8) {
            const unsigned char* data = reinterpret_cast<const unsigned char*>(blob.data());
            // OLE header: D0 CF 11 E0 A1 B1 1A E1
            if (data[0] == 0xD0 && data[1] == 0xCF && data[2] == 0x11 && data[3] == 0xE0) {
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
    
    if (blob.find("word/document.xml") != std::string::npos ||
        blob.find("word/_rels") != std::string::npos) {
        return OfficeDocumentType::DOCX;
    }
    if (blob.find("xl/workbook.xml") != std::string::npos ||
        blob.find("xl/_rels") != std::string::npos) {
        return OfficeDocumentType::XLSX;
    }
    if (blob.find("ppt/presentation.xml") != std::string::npos ||
        blob.find("ppt/_rels") != std::string::npos) {
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

ExtractionResult OfficeProcessor::extract(
    const std::string& blob,
    const ContentType& content_type
) {
    ExtractionResult result;
    result.ok = false;
    result.metadata = json::object();
    
    // Detect document type
    OfficeDocumentType doc_type = detectDocumentType(blob);
    result.metadata["size_bytes"] = blob.size();
    
    switch (doc_type) {
        case OfficeDocumentType::DOCX:
            result.metadata["document_type"] = "docx";
            result.metadata["mime_type"] = DOCX_CONTENT_TYPE;
            return extractDOCX(blob);
            
        case OfficeDocumentType::XLSX:
            result.metadata["document_type"] = "xlsx";
            result.metadata["mime_type"] = XLSX_CONTENT_TYPE;
            return extractXLSX(blob);
            
        case OfficeDocumentType::PPTX:
            result.metadata["document_type"] = "pptx";
            result.metadata["mime_type"] = PPTX_CONTENT_TYPE;
            return extractPPTX(blob);
            
        case OfficeDocumentType::ODT:
        case OfficeDocumentType::ODS:
        case OfficeDocumentType::ODP:
            result.metadata["document_type"] = "odf";
            return extractODF(blob, doc_type);
            
        case OfficeDocumentType::DOC:
        case OfficeDocumentType::XLS:
        case OfficeDocumentType::PPT:
            result.error_message = "Legacy Office formats (DOC/XLS/PPT) not fully supported. Please convert to OOXML format.";
            result.metadata["document_type"] = "legacy_office";
            return result;
            
        case OfficeDocumentType::RTF:
            result.metadata["document_type"] = "rtf";
            // Basic RTF text extraction
            {
                std::string text;
                std::regex rtf_text_regex("\\\\([a-z]+)\\s*([^\\\\{}]+)");
                std::sregex_iterator it(blob.begin(), blob.end(), rtf_text_regex);
                std::sregex_iterator end;
                
                for (; it != end; ++it) {
                    std::string control = (*it)[1].str();
                    std::string content = (*it)[2].str();
                    // Skip control words, keep text
                    if (control.empty() || control == "pard" || control == "par") {
                        text += content + " ";
                    }
                }
                result.text = text;
                result.metadata["extraction_method"] = "basic_rtf";
                result.ok = true;
            }
            return result;
            
        default:
            result.error_message = "Unknown or unsupported Office document format";
            return result;
    }
}

ExtractionResult OfficeProcessor::extractDOCX(const std::string& blob) {
    ExtractionResult result;
    result.ok = false;
    result.metadata = json::object();
    result.metadata["document_type"] = "docx";
    result.metadata["mime_type"] = DOCX_CONTENT_TYPE;

#if OFFICE_LIBRARY_AVAILABLE
    try {
        // Read document.xml from ZIP
        std::string document_xml = readZipEntry(blob, "word/document.xml");
        
        if (document_xml.empty()) {
            result.error_message = "Failed to read word/document.xml from DOCX";
            return result;
        }
        
        // Extract metadata
        OfficeMetadata metadata = extractOOXMLMetadata(blob);
        result.metadata["title"] = metadata.title;
        result.metadata["author"] = metadata.author;
        result.metadata["subject"] = metadata.subject;
        result.metadata["keywords"] = metadata.keywords;
        result.metadata["created_date"] = metadata.created_date;
        result.metadata["modified_date"] = metadata.modified_date;
        result.metadata["application"] = metadata.application;
        
        // Parse XML and extract text
        pugi::xml_document doc;
        pugi::xml_parse_result parse_result = doc.load_string(document_xml.c_str());
        
        if (!parse_result) {
            result.error_message = "Failed to parse document.xml: " + std::string(parse_result.description());
            return result;
        }
        
        // Extract paragraphs
        std::vector<std::string> paragraphs;
        std::ostringstream all_text;
        
        // Navigate to w:body/w:p elements
        for (auto p : doc.select_nodes("//w:p")) {
            std::string para_text;
            for (auto t : p.node().select_nodes(".//w:t")) {
                para_text += t.node().child_value();
            }
            if (!para_text.empty()) {
                paragraphs.push_back(para_text);
                all_text << para_text << "\n";
            }
        }
        
        result.text = all_text.str();
        result.metadata["paragraph_count"] = paragraphs.size();
        result.metadata["token_count"] = countTokens(result.text);
        result.ok = true;
        
    } catch (const std::exception& e) {
        result.error_message = std::string("DOCX extraction error: ") + e.what();
    }
#else
    // Fallback: Basic extraction without libzip/pugixml
    result.metadata["note"] = "Full DOCX extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";
    
    // Try to find text between <w:t> tags
    std::regex text_regex("<w:t[^>]*>([^<]+)</w:t>");
    std::ostringstream extracted;
    
    auto text_begin = std::sregex_iterator(blob.begin(), blob.end(), text_regex);
    auto text_end = std::sregex_iterator();
    
    for (auto it = text_begin; it != text_end; ++it) {
        extracted << (*it)[1].str() << " ";
    }
    
    result.text = extracted.str();
    result.metadata["extraction_method"] = "basic_regex";
    result.metadata["token_count"] = countTokens(result.text);
    result.ok = true;
#endif

    return result;
}

ExtractionResult OfficeProcessor::extractXLSX(const std::string& blob) {
    ExtractionResult result;
    result.ok = false;
    result.metadata = json::object();
    result.metadata["document_type"] = "xlsx";
    result.metadata["mime_type"] = XLSX_CONTENT_TYPE;

#if OFFICE_LIBRARY_AVAILABLE
    try {
        // Read shared strings
        std::string shared_strings_xml = readZipEntry(blob, "xl/sharedStrings.xml");
        
        // Parse shared strings
        std::vector<std::string> shared_strings;
        if (!shared_strings_xml.empty()) {
            pugi::xml_document ss_doc;
            if (ss_doc.load_string(shared_strings_xml.c_str())) {
                for (auto si : ss_doc.select_nodes("//si")) {
                    std::string text;
                    for (auto t : si.node().select_nodes(".//t")) {
                        text += t.node().child_value();
                    }
                    shared_strings.push_back(text);
                }
            }
        }
        
        // Extract metadata
        OfficeMetadata metadata = extractOOXMLMetadata(blob);
        result.metadata["title"] = metadata.title;
        result.metadata["author"] = metadata.author;
        result.metadata["created_date"] = metadata.created_date;
        result.metadata["modified_date"] = metadata.modified_date;
        
        // Read workbook.xml to get sheet names
        std::string workbook_xml = readZipEntry(blob, "xl/workbook.xml");
        std::vector<std::string> sheet_names;
        if (!workbook_xml.empty()) {
            pugi::xml_document wb_doc;
            if (wb_doc.load_string(workbook_xml.c_str())) {
                for (auto sheet : wb_doc.select_nodes("//sheet")) {
                    const char* name = sheet.node().attribute("name").value();
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
        std::ostringstream all_text;
        int row_count = 0;
        int cell_count = 0;
        
        if (!sheet1_xml.empty()) {
            pugi::xml_document sheet_doc;
            if (sheet_doc.load_string(sheet1_xml.c_str())) {
                for (auto row : sheet_doc.select_nodes("//row")) {
                    row_count++;
                    std::vector<std::string> row_values;
                    
                    for (auto cell : row.node().select_nodes("c")) {
                        cell_count++;
                        if (cell_count > config_.max_cell_count) break;
                        
                        std::string value;
                        const char* type = cell.node().attribute("t").value();
                        auto v_node = cell.node().child("v");
                        
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
                    
                    if (cell_count > config_.max_cell_count) break;
                    
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
        
        result.text = all_text.str();
        result.metadata["row_count"] = row_count;
        result.metadata["cell_count"] = cell_count;
        result.metadata["token_count"] = countTokens(result.text);
        result.ok = true;
        
    } catch (const std::exception& e) {
        result.error_message = std::string("XLSX extraction error: ") + e.what();
    }
#else
    result.metadata["note"] = "Full XLSX extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";
    result.metadata["extraction_method"] = "not_available";
    result.text = "";
    result.ok = true;
#endif

    return result;
}

ExtractionResult OfficeProcessor::extractPPTX(const std::string& blob) {
    ExtractionResult result;
    result.ok = false;
    result.metadata = json::object();
    result.metadata["document_type"] = "pptx";
    result.metadata["mime_type"] = PPTX_CONTENT_TYPE;

#if OFFICE_LIBRARY_AVAILABLE
    try {
        // Extract metadata
        OfficeMetadata metadata = extractOOXMLMetadata(blob);
        result.metadata["title"] = metadata.title;
        result.metadata["author"] = metadata.author;
        result.metadata["created_date"] = metadata.created_date;
        result.metadata["modified_date"] = metadata.modified_date;
        
        // List slides
        std::vector<std::string> slide_files = listZipEntries(blob);
        std::vector<std::string> slides;
        
        for (const auto& entry : slide_files) {
            if (entry.find("ppt/slides/slide") != std::string::npos &&
                entry.find(".xml") != std::string::npos) {
                slides.push_back(entry);
            }
        }
        
        // Sort slides by number
        std::sort(slides.begin(), slides.end());
        
        result.metadata["slide_count"] = slides.size();
        
        std::ostringstream all_text;
        int slide_num = 1;
        
        for (const auto& slide_path : slides) {
            std::string slide_xml = readZipEntry(blob, slide_path);
            
            if (slide_xml.empty()) continue;
            
            pugi::xml_document slide_doc;
            if (!slide_doc.load_string(slide_xml.c_str())) continue;
            
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
                std::string notes_xml = readZipEntry(blob, notes_path);
                
                if (notes_xml.empty()) continue;
                
                pugi::xml_document notes_doc;
                if (!notes_doc.load_string(notes_xml.c_str())) continue;
                
                all_text << "--- Notes for Slide " << i << " ---\n";
                for (auto t : notes_doc.select_nodes("//a:t")) {
                    all_text << t.node().child_value() << " ";
                }
                all_text << "\n\n";
            }
        }
        
        result.text = all_text.str();
        result.metadata["token_count"] = countTokens(result.text);
        result.ok = true;
        
    } catch (const std::exception& e) {
        result.error_message = std::string("PPTX extraction error: ") + e.what();
    }
#else
    result.metadata["note"] = "Full PPTX extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";
    result.metadata["extraction_method"] = "not_available";
    result.text = "";
    result.ok = true;
#endif

    return result;
}

ExtractionResult OfficeProcessor::extractODF(const std::string& blob, OfficeDocumentType type) {
    ExtractionResult result;
    result.ok = false;
    result.metadata = json::object();
    
    std::string type_str;
    switch (type) {
        case OfficeDocumentType::ODT: type_str = "odt"; break;
        case OfficeDocumentType::ODS: type_str = "ods"; break;
        case OfficeDocumentType::ODP: type_str = "odp"; break;
        default: type_str = "odf"; break;
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
        pugi::xml_document doc;
        if (!doc.load_string(content_xml.c_str())) {
            result.error_message = "Failed to parse content.xml";
            return result;
        }
        
        std::ostringstream all_text;
        
        // Extract text from text:p and text:h elements
        for (auto node : doc.select_nodes("//text:p | //text:h")) {
            std::string para_text;
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
        
        result.text = all_text.str();
        result.metadata["token_count"] = countTokens(result.text);
        result.ok = true;
        
    } catch (const std::exception& e) {
        result.error_message = std::string("ODF extraction error: ") + e.what();
    }
#else
    result.metadata["note"] = "Full ODF extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";
    result.text = "";
    result.ok = true;
#endif

    return result;
}

#if OFFICE_LIBRARY_AVAILABLE
std::string OfficeProcessor::readZipEntry(const std::string& zip_blob, const std::string& entry_path) {
    zip_error_t error;
    zip_error_init(&error);
    
    zip_source_t* source = zip_source_buffer_create(
        zip_blob.data(), zip_blob.size(), 0, &error
    );
    
    if (!source) {
        zip_error_fini(&error);
        return "";
    }
    
    zip_t* archive = zip_open_from_source(source, ZIP_RDONLY, &error);
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
    
    zip_stat_t stat;
    if (zip_stat_index(archive, index, 0, &stat) != 0) {
        zip_close(archive);
        return "";
    }
    
    zip_file_t* file = zip_fopen_index(archive, index, 0);
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

OfficeMetadata OfficeProcessor::extractOOXMLMetadata(const std::string& zip_blob) {
    OfficeMetadata metadata;
    
    // Read docProps/core.xml
    std::string core_xml = readZipEntry(zip_blob, "docProps/core.xml");
    if (core_xml.empty()) {
        return metadata;
    }
    
    pugi::xml_document doc;
    if (!doc.load_string(core_xml.c_str())) {
        return metadata;
    }
    
    // Extract properties
    auto get_text = [&doc](const char* xpath) -> std::string {
        auto node = doc.select_node(xpath);
        return node ? node.node().child_value() : "";
    };
    
    metadata.title = get_text("//dc:title");
    metadata.author = get_text("//dc:creator");
    metadata.subject = get_text("//dc:subject");
    metadata.keywords = get_text("//cp:keywords");
    metadata.last_modified_by = get_text("//cp:lastModifiedBy");
    metadata.created_date = get_text("//dcterms:created");
    metadata.modified_date = get_text("//dcterms:modified");
    
    // Read docProps/app.xml for application info
    std::string app_xml = readZipEntry(zip_blob, "docProps/app.xml");
    if (!app_xml.empty()) {
        pugi::xml_document app_doc;
        if (app_doc.load_string(app_xml.c_str())) {
            auto app_node = app_doc.select_node("//Application");
            if (app_node) {
                metadata.application = app_node.node().child_value();
            }
        }
    }
    
    return metadata;
}

std::vector<std::string> OfficeProcessor::listZipEntries(const std::string& zip_blob) {
    std::vector<std::string> entries;
    
    zip_error_t error;
    zip_error_init(&error);
    
    zip_source_t* source = zip_source_buffer_create(
        zip_blob.data(), zip_blob.size(), 0, &error
    );
    
    if (!source) {
        zip_error_fini(&error);
        return entries;
    }
    
    zip_t* archive = zip_open_from_source(source, ZIP_RDONLY, &error);
    if (!archive) {
        zip_source_free(source);
        zip_error_fini(&error);
        return entries;
    }
    
    zip_int64_t num_entries = zip_get_num_entries(archive, 0);
    for (zip_int64_t i = 0; i < num_entries; ++i) {
        const char* name = zip_get_name(archive, i, 0);
        if (name) {
            entries.push_back(name);
        }
    }
    
    zip_close(archive);
    return entries;
}
#else
std::string OfficeProcessor::readZipEntry(const std::string&, const std::string&) {
    return "";
}

OfficeMetadata OfficeProcessor::extractOOXMLMetadata(const std::string&) {
    return OfficeMetadata{};
}

std::vector<std::string> OfficeProcessor::listZipEntries(const std::string&) {
    return {};
}
#endif

std::vector<json> OfficeProcessor::chunk(
    const ExtractionResult& extraction_result,
    int chunk_size,
    int overlap
) {
    std::vector<json> chunks;
    
    const std::string& text = extraction_result.text;
    if (text.empty()) {
        return chunks;
    }
    
    // Split by paragraphs first
    std::vector<std::string> paragraphs;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            paragraphs.push_back(line);
        }
    }
    
    // Group paragraphs into chunks
    int seq_num = 0;
    std::string current_chunk;
    int current_tokens = 0;
    
    for (const auto& para : paragraphs) {
        int para_tokens = countTokens(para);
        
        if (current_tokens + para_tokens > chunk_size && !current_chunk.empty()) {
            json chunk = {
                {"text", current_chunk},
                {"seq_num", seq_num},
                {"token_count", current_tokens},
                {"source_type", "office"}
            };
            chunks.push_back(chunk);
            seq_num++;
            
            current_chunk = para;
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
            {"text", current_chunk},
            {"seq_num", seq_num},
            {"token_count", current_tokens},
            {"source_type", "office"}
        };
        chunks.push_back(chunk);
    }
    
    return chunks;
}

std::vector<float> OfficeProcessor::generateEmbedding(const std::string& chunk_data) {
    // Placeholder
    return std::vector<float>();
}

int OfficeProcessor::countTokens(const std::string& text) {
    if (text.empty()) return 0;
    
    int count = 1;
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

bool OfficeProcessor::isValidOOXML(const std::string& blob) {
    return blob.find("[Content_Types].xml") != std::string::npos;
}

bool OfficeProcessor::isValidODF(const std::string& blob) {
    return blob.find("mimetype") != std::string::npos &&
           blob.find("application/vnd.oasis.opendocument") != std::string::npos;
}

std::unique_ptr<IContentProcessor> createOfficeProcessor() {
    return std::make_unique<OfficeProcessor>(OfficeProcessor::Config{});
}

std::unique_ptr<IContentProcessor> createOfficeProcessor(OfficeProcessor::Config config) {
    return std::make_unique<OfficeProcessor>(std::move(config));
}

} // namespace content
} // namespace themis
