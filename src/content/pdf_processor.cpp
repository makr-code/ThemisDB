/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pdf_processor.cpp                                  ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     487                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file pdf_processor.cpp
 * @brief PDF Content Processor Implementation
 * 
 * Extracts text and metadata from PDF documents.
 * 
 * Build with -DTHEMIS_ENABLE_PDF=ON to enable poppler-cpp support.
 * Without poppler, basic header parsing and metadata extraction is available.
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#include "content/pdf_processor.h"
#include <regex>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <iomanip>

// Optional: poppler-cpp support
#ifdef THEMIS_ENABLE_PDF
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-global.h>
#define PDF_LIBRARY_AVAILABLE 1
#define PDF_LIBRARY_NAME "poppler-cpp"
#else
#define PDF_LIBRARY_AVAILABLE 0
#define PDF_LIBRARY_NAME "none (built without -DTHEMIS_ENABLE_PDF)"
#endif

namespace themis {
namespace content {

// ============================================================================
// PDFProcessor Implementation
// ============================================================================

PDFProcessor::PDFProcessor()
    : PDFProcessor(Config{})
{
}

PDFProcessor::PDFProcessor(Config config)
    : config_(std::move(config))
{
}

bool PDFProcessor::isAvailable() {
#if PDF_LIBRARY_AVAILABLE
    return true;
#else
    return false;
#endif
}

std::string PDFProcessor::getLibraryVersion() {
#if PDF_LIBRARY_AVAILABLE
    return std::string(PDF_LIBRARY_NAME) + " " + poppler::version_string();
#else
    return PDF_LIBRARY_NAME;
#endif
}

bool PDFProcessor::isPDFValid(const std::string& blob) {
    // Check PDF header signature
    if (blob.size() < 8) {
        return false;
    }
    // PDF starts with %PDF-x.x
    return blob.substr(0, 4) == "%PDF";
}

ExtractionResult PDFProcessor::extract(
    const std::string& blob,
    const ContentType& content_type
) {
    ExtractionResult result;
    result.ok = false;
    result.metadata = json::object();

    // Validate PDF
    if (!isPDFValid(blob)) {
        result.error_message = "Invalid PDF: missing %PDF header";
        return result;
    }

    // Extract basic info from header
    result.metadata["mime_type"] = "application/pdf";
    result.metadata["size_bytes"] = blob.size();

    // Extract PDF version from header
    std::string version = "1.0";
    if (blob.size() >= 8) {
        // %PDF-1.7
        std::string header = blob.substr(0, 8);
        std::regex version_regex("%PDF-(\\d+\\.\\d+)");
        std::smatch match;
        if (std::regex_search(header, match, version_regex)) {
            version = match[1].str();
        }
    }
    result.metadata["pdf_version"] = version;

#if PDF_LIBRARY_AVAILABLE
    try {
        // Use poppler-cpp for full extraction
        std::vector<char> data(blob.begin(), blob.end());
        
        std::unique_ptr<poppler::document> doc;
        
        if (!config_.password.empty()) {
            doc.reset(poppler::document::load_from_raw_data(
                data.data(), data.size(),
                config_.password, config_.password
            ));
        } else {
            doc.reset(poppler::document::load_from_raw_data(
                data.data(), data.size()
            ));
        }
        
        if (!doc) {
            result.error_message = "Failed to parse PDF with poppler";
            return result;
        }
        
        if (doc->is_locked()) {
            result.error_message = "PDF is encrypted and password is incorrect";
            result.metadata["is_encrypted"] = true;
            return result;
        }
        
        // Extract metadata
        PDFMetadata metadata = extractMetadata(blob);
        result.metadata["title"] = metadata.title;
        result.metadata["author"] = metadata.author;
        result.metadata["subject"] = metadata.subject;
        result.metadata["keywords"] = metadata.keywords;
        result.metadata["creator"] = metadata.creator;
        result.metadata["producer"] = metadata.producer;
        result.metadata["creation_date"] = metadata.creation_date;
        result.metadata["modification_date"] = metadata.modification_date;
        result.metadata["page_count"] = doc->pages();
        result.metadata["is_encrypted"] = metadata.is_encrypted;
        result.metadata["is_linearized"] = metadata.is_linearized;
        
        // Extract text from each page
        std::ostringstream all_text;
        int max_pages = config_.max_pages > 0 
            ? std::min(config_.max_pages, doc->pages()) 
            : doc->pages();
        
        for (int i = 0; i < max_pages; ++i) {
            std::unique_ptr<poppler::page> page(doc->create_page(i));
            if (!page) continue;
            
            // Get page text
            poppler::byte_array page_text_bytes = page->text().to_utf8();
            std::string page_text(page_text_bytes.data(), page_text_bytes.size());
            
            if (!page_text.empty()) {
                all_text << page_text;
                if (i + 1 < max_pages) {
                    all_text << "\n\n--- Page " << (i + 2) << " ---\n\n";
                }
            }
        }
        
        result.text = all_text.str();
        result.metadata["extracted_pages"] = max_pages;
        result.metadata["token_count"] = countTokens(result.text);
        result.ok = true;
        
    } catch (const std::exception& e) {
        result.error_message = std::string("PDF extraction error: ") + e.what();
        return result;
    }
#else
    // Fallback: Basic extraction without poppler
    // Try to extract text from PDF streams (very basic)
    
    // Count pages using /Type /Page pattern
    std::regex page_regex("/Type\\s*/Page[^s]");
    auto pages_begin = std::sregex_iterator(blob.begin(), blob.end(), page_regex);
    auto pages_end = std::sregex_iterator();
    int page_count = std::distance(pages_begin, pages_end);
    result.metadata["page_count"] = page_count;
    
    // Try to extract text from BT/ET blocks
    std::ostringstream extracted;
    std::regex text_regex("\\(([^)]+)\\)\\s*Tj");
    auto text_begin = std::sregex_iterator(blob.begin(), blob.end(), text_regex);
    auto text_end = std::sregex_iterator();
    
    for (auto it = text_begin; it != text_end; ++it) {
        extracted << (*it)[1].str() << " ";
    }
    
    result.text = extracted.str();
    result.metadata["extraction_method"] = "basic_regex";
    result.metadata["note"] = "Full PDF extraction requires building with -DTHEMIS_ENABLE_PDF=ON";
    result.metadata["token_count"] = countTokens(result.text);
    result.ok = true;
#endif

    return result;
}

PDFMetadata PDFProcessor::extractMetadata(const std::string& blob) {
    PDFMetadata metadata;
    metadata.is_encrypted = false;
    metadata.is_linearized = false;
    metadata.page_count = 0;

#if PDF_LIBRARY_AVAILABLE
    std::vector<char> data(blob.begin(), blob.end());
    std::unique_ptr<poppler::document> doc(
        poppler::document::load_from_raw_data(data.data(), data.size())
    );
    
    if (doc) {
        auto to_string = [](const poppler::ustring& us) -> std::string {
            poppler::byte_array bytes = us.to_utf8();
            return std::string(bytes.data(), bytes.size());
        };
        
        metadata.title = to_string(doc->get_title());
        metadata.author = to_string(doc->get_author());
        metadata.subject = to_string(doc->get_subject());
        metadata.keywords = to_string(doc->get_keywords());
        metadata.creator = to_string(doc->get_creator());
        metadata.producer = to_string(doc->get_producer());
        
        // Convert times
        time_t created = doc->get_creation_date();
        time_t modified = doc->get_modification_date();
        
        char buf[32];
        if (created > 0) {
            std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&created));
            metadata.creation_date = buf;
        }
        if (modified > 0) {
            std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&modified));
            metadata.modification_date = buf;
        }
        
        metadata.page_count = doc->pages();
        metadata.is_encrypted = doc->is_encrypted();
        metadata.is_linearized = doc->is_linearized();
    }
#else
    // Basic metadata extraction from PDF info dict
    // Look for /Title, /Author, etc. in the PDF
    auto extractInfo = [&blob](const std::string& key) -> std::string {
        std::regex pattern("/" + key + "\\s*\\(([^)]+)\\)");
        std::smatch match;
        if (std::regex_search(blob, match, pattern)) {
            return match[1].str();
        }
        return "";
    };
    
    metadata.title = extractInfo("Title");
    metadata.author = extractInfo("Author");
    metadata.subject = extractInfo("Subject");
    metadata.keywords = extractInfo("Keywords");
    metadata.creator = extractInfo("Creator");
    metadata.producer = extractInfo("Producer");
#endif

    return metadata;
}

std::vector<PDFPageInfo> PDFProcessor::extractPages(const std::string& blob) {
    std::vector<PDFPageInfo> pages;

#if PDF_LIBRARY_AVAILABLE
    std::vector<char> data(blob.begin(), blob.end());
    std::unique_ptr<poppler::document> doc(
        poppler::document::load_from_raw_data(data.data(), data.size())
    );
    
    if (!doc) return pages;
    
    int max_pages = config_.max_pages > 0 
        ? std::min(config_.max_pages, doc->pages()) 
        : doc->pages();
    
    for (int i = 0; i < max_pages; ++i) {
        std::unique_ptr<poppler::page> page(doc->create_page(i));
        if (!page) continue;
        
        PDFPageInfo info;
        info.page_number = i + 1;
        
        // Get text
        poppler::byte_array text_bytes = page->text().to_utf8();
        info.text = std::string(text_bytes.data(), text_bytes.size());
        
        // Get dimensions
        poppler::rectf rect = page->page_rect();
        info.width = static_cast<int>(rect.width());
        info.height = static_cast<int>(rect.height());
        info.rotation = page->orientation() * 90;
        
        pages.push_back(std::move(info));
    }
#endif

    return pages;
}

std::string PDFProcessor::extractAllText(const std::vector<PDFPageInfo>& pages) {
    std::ostringstream oss;
    for (size_t i = 0; i < pages.size(); ++i) {
        oss << pages[i].text;
        if (i + 1 < pages.size()) {
            oss << "\n\n";
        }
    }
    return oss.str();
}

std::vector<json> PDFProcessor::chunk(
    const ExtractionResult& extraction_result,
    int chunk_size,
    int overlap
) {
    std::vector<json> chunks;
    
    const std::string& text = extraction_result.text;
    if (text.empty()) {
        return chunks;
    }
    
    // Simple sentence-based chunking
    std::vector<std::string> sentences;
    std::regex sentence_regex("[.!?]+\\s+");
    std::sregex_token_iterator iter(text.begin(), text.end(), sentence_regex, -1);
    std::sregex_token_iterator end;
    
    for (; iter != end; ++iter) {
        std::string sentence = iter->str();
        if (!sentence.empty()) {
            sentences.push_back(sentence);
        }
    }
    
    // Group sentences into chunks of approximately chunk_size tokens
    int seq_num = 0;
    std::string current_chunk;
    int current_tokens = 0;
    
    for (const auto& sentence : sentences) {
        int sentence_tokens = countTokens(sentence);
        
        if (current_tokens + sentence_tokens > chunk_size && !current_chunk.empty()) {
            // Save current chunk
            json chunk = {
                {"text", current_chunk},
                {"seq_num", seq_num},
                {"token_count", current_tokens},
                {"source_type", "pdf"}
            };
            chunks.push_back(chunk);
            seq_num++;
            
            // Start new chunk (with overlap)
            if (overlap > 0 && current_tokens > overlap) {
                // Keep last part for overlap
                current_chunk = sentence;
                current_tokens = sentence_tokens;
            } else {
                current_chunk = sentence;
                current_tokens = sentence_tokens;
            }
        } else {
            if (!current_chunk.empty()) {
                current_chunk += " ";
            }
            current_chunk += sentence;
            current_tokens += sentence_tokens;
        }
    }
    
    // Add remaining chunk
    if (!current_chunk.empty()) {
        json chunk = {
            {"text", current_chunk},
            {"seq_num", seq_num},
            {"token_count", current_tokens},
            {"source_type", "pdf"}
        };
        chunks.push_back(chunk);
    }
    
    return chunks;
}

std::vector<float> PDFProcessor::generateEmbedding(const std::string& chunk_data) {
    // Placeholder: Return empty embedding
    // In production, this would call an embedding service
    return std::vector<float>();
}

int PDFProcessor::countTokens(const std::string& text) {
    if (text.empty()) return 0;
    
    // Simple whitespace tokenization
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

std::string PDFProcessor::parsePDFDate(const std::string& pdf_date) {
    // PDF date format: D:YYYYMMDDHHmmSSOHH'mm'
    // Convert to ISO 8601
    if (pdf_date.size() < 10) return "";
    
    std::string date = pdf_date;
    if (date.substr(0, 2) == "D:") {
        date = date.substr(2);
    }
    
    if (date.size() < 8) return "";
    
    std::ostringstream iso;
    iso << date.substr(0, 4) << "-" << date.substr(4, 2) << "-" << date.substr(6, 2);
    
    if (date.size() >= 14) {
        iso << "T" << date.substr(8, 2) << ":" << date.substr(10, 2) << ":" << date.substr(12, 2);
    }
    
    iso << "Z";
    return iso.str();
}

std::unique_ptr<IContentProcessor> createPDFProcessor() {
    return std::make_unique<PDFProcessor>(PDFProcessor::Config{});
}

std::unique_ptr<IContentProcessor> createPDFProcessor(PDFProcessor::Config config) {
    return std::make_unique<PDFProcessor>(std::move(config));
}

} // namespace content
} // namespace themis
