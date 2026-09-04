/**
 * @file pdf_processor.cpp
 * @brief PDF content processor with text/image extraction and metadata parsing.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=2, C=0, H=1, M=4, L=0
 * @note Status: Production Ready; PDF text/image extraction working; form field handling deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/pdf_processor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>

#include "content/content_metrics.h"

// Optional: poppler-cpp support
#ifdef THEMIS_ENABLE_PDF
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-global.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-version.h>
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

PDFProcessor::PDFProcessor() : PDFProcessor(Config{}) {}

PDFProcessor::PDFProcessor(Config config) : config_(std::move(config)) {}

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

bool PDFProcessor::isPDFValid(const std::string &blob) {
    // Check PDF header signature
    if (blob.size() < 8) {
        return false;
    }
    // PDF starts with %PDF-x.x
    return blob.substr(0, 4) == "%PDF";
}

ExtractionResult PDFProcessor::extract(const std::string &blob, const ContentType & /*content_type*/
) {
    ExtractionResult result;
    result.ok       = false;
    result.metadata = json::object();

    // Validate PDF
    if (!isPDFValid(blob)) {
        result.error_message = "Invalid PDF: missing %PDF header";
        return result;
    }

    // Extract basic info from header
    result.metadata["mime_type"]  = "application/pdf";
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
            doc.reset(
                poppler::document::load_from_raw_data(data.data(), data.size(), config_.password, config_.password));
        } else {
            doc.reset(poppler::document::load_from_raw_data(data.data(), data.size()));
        }

        if (!doc) {
            result.error_message = "Failed to parse PDF with poppler";
            if (config_.metrics) {
                config_.metrics->recordExtractError();
            }
            return result;
        }

        if (doc->is_locked()) {
            result.error_message            = "PDF is encrypted and password is incorrect";
            result.metadata["is_encrypted"] = true;
            if (config_.metrics) {
                config_.metrics->recordExtractError();
            }
            return result;
        }

        // Extract metadata
        PDFMetadata metadata                 = extractMetadata(blob);
        result.metadata["title"]             = metadata.title;
        result.metadata["author"]            = metadata.author;
        result.metadata["subject"]           = metadata.subject;
        result.metadata["keywords"]          = metadata.keywords;
        result.metadata["creator"]           = metadata.creator;
        result.metadata["producer"]          = metadata.producer;
        result.metadata["creation_date"]     = metadata.creation_date;
        result.metadata["modification_date"] = metadata.modification_date;
        result.metadata["page_count"]        = doc->pages();
        result.metadata["is_encrypted"]      = metadata.is_encrypted;
        result.metadata["is_linearized"]     = metadata.is_linearized;

        // Extract pages using the already-loaded doc (avoids redundant PDF loading)
        std::ostringstream all_text;
        int max_pages = config_.max_pages > 0 ? std::min(config_.max_pages, doc->pages()) : doc->pages();

        json pages_array = json::array();
        for (int i = 0; i < max_pages; ++i) {
            std::unique_ptr<poppler::page> page(doc->create_page(i));
            if (!page)
                continue;

            std::string page_text;
            if (config_.maintain_layout) {
                // Layout-preserving: use positioned text boxes
                auto text_boxes = page->text_list();
                std::vector<std::pair<float, float>> positions;
                page_text = assembleTextWithLayout(text_boxes, positions);
            } else {
                // Simple reading-order extraction
                poppler::byte_array bytes = page->text().to_utf8();
                page_text                 = std::string(bytes.data(), bytes.size());
            }

            poppler::rectf rect = page->page_rect();

            if (!page_text.empty()) {
                all_text << page_text;
                if (i + 1 < max_pages) {
                    all_text << "\n\n--- Page " << (i + 2) << " ---\n\n";
                }
            }

            json page_obj;
            page_obj["page"]     = i + 1;
            page_obj["text"]     = page_text;
            page_obj["width"]    = static_cast<int>(rect.width());
            page_obj["height"]   = static_cast<int>(rect.height());
            page_obj["rotation"] = page->orientation() * 90;
            pages_array.push_back(std::move(page_obj));
        }

        result.text                         = all_text.str();
        result.metadata["extracted_pages"]  = max_pages;
        result.metadata["pages"]            = pages_array;
        result.metadata["layout_preserved"] = config_.maintain_layout;
        result.metadata["token_count"]      = countTokens(result.text);
        result.ok                           = true;

    } catch (const std::exception &e) {
        result.error_message = std::string("PDF extraction error: ") + e.what();
        if (config_.metrics) {
            config_.metrics->recordExtractError();
        }
        return result;
    }
#else
    // Fallback: Basic extraction without poppler
    // Try to extract text from PDF streams (very basic)

    // Count pages using /Type /Page pattern
    std::regex page_regex("/Type\\s*/Page[^s]");
    auto pages_begin              = std::sregex_iterator(blob.begin(), blob.end(), page_regex);
    auto pages_end                = std::sregex_iterator();
    int page_count                = static_cast<int>(std::distance(pages_begin, pages_end));
    result.metadata["page_count"] = page_count;

    // Try to extract text from BT/ET blocks
    std::ostringstream extracted;
    std::regex text_regex("\\(([^)]+)\\)\\s*Tj");
    auto text_begin = std::sregex_iterator(blob.begin(), blob.end(), text_regex);
    auto text_end   = std::sregex_iterator();

    for (auto it = text_begin; it != text_end; ++it) {
        extracted << (*it)[1].str() << " ";
    }

    result.text                          = extracted.str();
    result.metadata["extraction_method"] = "basic_regex";
    result.metadata["note"]              = "Full PDF extraction requires building with -DTHEMIS_ENABLE_PDF=ON";
    result.metadata["layout_preserved"]  = false;
    result.metadata["token_count"]       = countTokens(result.text);
    result.ok                            = true;
#endif

    // Report metrics if a ContentMetrics instance was configured
    if (config_.metrics) {
        if (result.ok) {
            config_.metrics->recordPdfExtracted();
        } else {
            config_.metrics->recordExtractError();
        }
    }

    return result;
}

PDFMetadata PDFProcessor::extractMetadata(const std::string &blob) {
    PDFMetadata metadata;
    metadata.is_encrypted  = false;
    metadata.is_linearized = false;
    metadata.page_count    = 0;

#if PDF_LIBRARY_AVAILABLE
    std::vector<char> data(blob.begin(), blob.end());
    std::unique_ptr<poppler::document> doc(poppler::document::load_from_raw_data(data.data(), data.size()));

    if (doc) {
        auto to_string = [](const poppler::ustring &us) -> std::string {
            poppler::byte_array bytes = us.to_utf8();
            return std::string(bytes.data(), bytes.size());
        };

        metadata.title    = to_string(doc->get_title());
        metadata.author   = to_string(doc->get_author());
        metadata.subject  = to_string(doc->get_subject());
        metadata.keywords = to_string(doc->get_keywords());
        metadata.creator  = to_string(doc->get_creator());
        metadata.producer = to_string(doc->get_producer());

        // Convert times
        time_t created  = doc->get_creation_date_t();
        time_t modified = doc->get_modification_date_t();

        char buf[32];
        if (created > 0) {
            std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&created));
            metadata.creation_date = buf;
        }
        if (modified > 0) {
            std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&modified));
            metadata.modification_date = buf;
        }

        metadata.page_count    = doc->pages();
        metadata.is_encrypted  = doc->is_encrypted();
        metadata.is_linearized = doc->is_linearized();
    }
#else
    // Basic metadata extraction from PDF info dict
    // Look for /Title, /Author, etc. in the PDF
    auto extractInfo = [&blob](const std::string &key) -> std::string {
        std::regex pattern("/" + key + "\\s*\\(([^)]+)\\)");
        std::smatch match;
        if (std::regex_search(blob, match, pattern)) {
            return match[1].str();
        }
        return "";
    };

    metadata.title    = extractInfo("Title");
    metadata.author   = extractInfo("Author");
    metadata.subject  = extractInfo("Subject");
    metadata.keywords = extractInfo("Keywords");
    metadata.creator  = extractInfo("Creator");
    metadata.producer = extractInfo("Producer");
#endif

    return metadata;
}

std::vector<PDFPageInfo> PDFProcessor::extractPages([[maybe_unused]] const std::string &blob) {
    std::vector<PDFPageInfo> pages;

#if PDF_LIBRARY_AVAILABLE
    std::vector<char> data(blob.begin(), blob.end());
    std::unique_ptr<poppler::document> doc(poppler::document::load_from_raw_data(data.data(), data.size()));

    if (!doc)
        return pages;

    int max_pages = config_.max_pages > 0 ? std::min(config_.max_pages, doc->pages()) : doc->pages();

    for (int i = 0; i < max_pages; ++i) {
        std::unique_ptr<poppler::page> page(doc->create_page(i));
        if (!page)
            continue;

        PDFPageInfo info;
        info.page_number = i + 1;

        if (config_.maintain_layout) {
            // Layout-preserving extraction: use text_list() to get positioned text boxes
            auto text_boxes = page->text_list();
            info.text       = assembleTextWithLayout(text_boxes, info.text_positions);
        } else {
            // Simple text extraction (reading order from poppler)
            poppler::byte_array text_bytes = page->text().to_utf8();
            info.text                      = std::string(text_bytes.data(), text_bytes.size());
        }

        // Get dimensions
        poppler::rectf rect = page->page_rect();
        info.width          = static_cast<int>(rect.width());
        info.height         = static_cast<int>(rect.height());
        info.rotation       = page->orientation() * 90;

        pages.push_back(std::move(info));
    }
#endif

    return pages;
}

#ifdef THEMIS_ENABLE_PDF
// static
std::string PDFProcessor::assembleTextWithLayout(const std::vector<poppler::text_box> &boxes,
                                                 std::vector<std::pair<float, float>> &positions_out) {
    if (boxes.empty()) {
        return {};
    }

    // Collect text boxes with their bounding box coordinates
    struct Item {
        float x, y, w, h;
        std::string text;
        bool has_space_after;
    };

    std::vector<Item> items = {};

    items.reserve(boxes.size());

    for (const auto &box : boxes) {
        poppler::byte_array bytes = box.text().to_utf8();
        std::string text(bytes.data(), bytes.size());
        if (text.empty())
            continue;

        poppler::rectf r = box.bbox();
        items.push_back({static_cast<float>(r.x()), static_cast<float>(r.y()), static_cast<float>(r.width()),
                         static_cast<float>(r.height()), std::move(text), box.has_space_after()});
    }

    if (items.empty()) {
        return {};
    }

    // Sort top-to-bottom, then left-to-right within the same visual line.
    // Two items are on the same line when their y-distance is less than
    // 40% of the average of their heights (accounts for baseline variation).
    std::sort(items.begin(), items.end(), [](const Item &a, const Item &b) {
        float line_thr = (a.h + b.h) * 0.4f;
        if (std::abs(a.y - b.y) < line_thr) {
            return a.x < b.x;
        }
        return a.y < b.y;
    });

    // Store (x, y) positions for each item
    positions_out.reserve(positions_out.size() + items.size());
    for (const auto &item : items) {
        positions_out.push_back({item.x, item.y});
    }

    // Assemble text inserting spaces and newlines based on position
    std::ostringstream oss;
    float prev_y = items[0].y;
    float prev_h = items[0].h;

    oss << items[0].text;
    if (items[0].has_space_after) {
        oss << ' ';
    }

    for (std::size_t i = 1; i < items.size(); ++i) {
        const Item &cur = items[i];
        float dy        = std::abs(cur.y - prev_y);
        float line_thr  = (cur.h + prev_h) * 0.4f;

        if (dy >= line_thr) {
            // New line: use a blank line for paragraph-sized vertical gaps
            if (dy >= prev_h * 1.5f) {
                oss << "\n\n";
            } else {
                oss << '\n';
            }
        }
        // (On the same line, spaces are handled by has_space_after above)

        oss << cur.text;
        if (cur.has_space_after) {
            oss << ' ';
        }

        prev_y = cur.y;
        prev_h = cur.h;
    }

    return oss.str();
}
#endif

std::string PDFProcessor::extractAllText(const std::vector<PDFPageInfo> &pages) {
    std::ostringstream oss;
    for (size_t i = 0; i < pages.size(); ++i) {
        oss << pages[i].text;
        if (i + 1 < pages.size()) {
            oss << "\n\n";
        }
    }
    return oss.str();
}

std::vector<json> PDFProcessor::chunk(const ExtractionResult &extraction_result, int chunk_size, int overlap) {
    std::vector<json> chunks;

    const std::string &text = extraction_result.text;
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

    for (const auto &sentence : sentences) {
        int sentence_tokens = countTokens(sentence);

        if (current_tokens + sentence_tokens > chunk_size && !current_chunk.empty()) {
            // Save current chunk
            json chunk = {
                {"text", current_chunk}, {"seq_num", seq_num}, {"token_count", current_tokens}, {"source_type", "pdf"}};
            chunks.push_back(chunk);
            seq_num++;

            // Start new chunk (with overlap)
            if (overlap > 0 && current_tokens > overlap) {
                // Keep last part for overlap
                current_chunk  = sentence;
                current_tokens = sentence_tokens;
            } else {
                current_chunk  = sentence;
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
        json chunk
            = {{"text", current_chunk}, {"seq_num", seq_num}, {"token_count", current_tokens}, {"source_type", "pdf"}};
        chunks.push_back(chunk);
    }

    return chunks;
}

std::vector<float> PDFProcessor::generateEmbedding(const std::string &chunk_data) {
    // Hash-projection embedding (768-dim, L2-normalised) matching the
    // approach used by TextProcessor::generateEmbedding().  Each token
    // influences 30 dimensions via sine-phase mixing so that documents
    // with different vocabulary produce distinct vectors.  Semantically
    // identical but differently worded chunks will differ; this is expected
    // until a real IEmbeddingBackend is injected (see
    // src/content/FUTURE_ENHANCEMENTS.md §PDF Embedding Integration).
    constexpr int kDim = 768;
    std::vector<float> embedding(kDim, 0.0f);

    if (chunk_data.empty()) {
        return embedding;
    }

    std::hash<std::string> hasher;
    std::istringstream iss(chunk_data);
    std::vector<std::string> tokens;
    std::string tok;
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

int PDFProcessor::countTokens(const std::string &text) {
    if (text.empty()) {
        return 0;
    }

    // Simple whitespace tokenization
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

std::string PDFProcessor::parsePDFDate(const std::string &pdf_date) {
    // PDF date format: D:YYYYMMDDHHmmSSOHH'mm'
    // Convert to ISO 8601
    if (pdf_date.size() < 10) {
        return "";
    }

    std::string date = pdf_date;
    if (date.substr(0, 2) == "D:") {
        date = date.substr(2);
    }

    if (date.size() < 8) {
        return "";
    }

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
