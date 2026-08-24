/**
 * @file test_cmt_fin_doxygen_headers_validation.cpp
 * @brief CMT-FIN-01..06: Doxygen header validation for CMT-7500 compliance.
 * @version 0.0.1
 * 
 * Tests validate that all 35 content processor files have:
 * - Complete @file, @brief, @version headers
 * - All @note fields (Maturity, Score, Gap Summary, Status)
 * - Correct maturity classification per CMT-7500 standard
 * - Valid Gap Summary format
 * 
 * CMT-7500 Standard:
 * - Score >= 85 → PRODUCTION-READY (🟢)
 * - Score 70-84 → BETA (🟡)
 * - Score < 70  → ALPHA (🔴)
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

namespace themis::content::test {

class DoxygenHeadersValidation : public ::testing::Test {
 protected:
    static constexpr const char* CONTENT_DIR = "src/content";
    static constexpr const char* REQUIRED_FILES[] = {
        "abuse_detector.cpp",
        "archive_processor.cpp",
        "archive_processor_enhancements.cpp",
        "async_ingestion_worker.cpp",
        "audio_processor.cpp",
        "cad_processor.cpp",
        "content_errors.cpp",
        "content_fs.cpp",
        "content_logger.cpp",
        "content_manager.cpp",
        "content_manager_embedding.cpp",
        "content_manager_llm.cpp",
        "content_metrics.cpp",
        "content_policy.cpp",
        "content_security.cpp",
        "content_type.cpp",
        "content_validator.cpp",
        "deduplication_checker.cpp",
        "embedding_pipeline.cpp",
        "geo_processor.cpp",
        "html_processor.cpp",
        "image_processor.cpp",
        "ingestion_plugin.cpp",
        "language_detector.cpp",
        "markdown_processor.cpp",
        "mime_detector.cpp",
        "mock_clip_processor.cpp",
        "ocr_processor.cpp",
        "office_processor.cpp",
        "pdf_processor.cpp",
        "stt_processor.cpp",
        "text_processor.cpp",
        "tts_processor.cpp",
        "version_manager.cpp",
        "video_processor.cpp",
    };

    struct HeaderFields {
        std::string file;
        std::string brief;
        std::string version;
        std::string maturity;
        int score = -1;
        std::string gap_summary;
        std::string status;
        bool has_all_required = false;
    };

    HeaderFields ParseHeader(const std::string& content) {
        HeaderFields fields;
        
        // Extract header block /** ... */
        std::regex header_regex(R"(/\*\*(.*?)\*/)");
        std::smatch header_match;
        
        if (!std::regex_search(content, header_match, header_regex)) {
            return fields;
        }
        
        std::string header = header_match[1];
        
        // Extract individual @fields
        std::regex file_regex(R"(@file\s+(\S+))");
        std::regex brief_regex(R"(@brief\s+(.+?)(?=\n|@))");
        std::regex version_regex(R"(@version\s+(\S+))");
        std::regex maturity_regex(R"(@note Maturity:\s*(.+?)(?=\n|@))");
        std::regex score_regex(R"(@note Score:\s*(\d+)/100)");
        std::regex gap_regex(R"(@note Gap Summary:\s*(.+?)(?=\n|@))");
        std::regex status_regex(R"(@note Status:\s*(.+?)(?=\n|@))");
        
        std::smatch match;
        if (std::regex_search(header, match, file_regex))
            fields.file = match[1];
        if (std::regex_search(header, match, brief_regex))
            fields.brief = match[1];
        if (std::regex_search(header, match, version_regex))
            fields.version = match[1];
        if (std::regex_search(header, match, maturity_regex))
            fields.maturity = match[1];
        if (std::regex_search(header, match, score_regex))
            fields.score = std::stoi(match[1]);
        if (std::regex_search(header, match, gap_regex))
            fields.gap_summary = match[1];
        if (std::regex_search(header, match, status_regex))
            fields.status = match[1];
        
        fields.has_all_required = !fields.file.empty() && !fields.brief.empty() &&
                                 !fields.version.empty() && !fields.maturity.empty() &&
                                 fields.score >= 0 && !fields.gap_summary.empty() &&
                                 !fields.status.empty();
        
        return fields;
    }

    std::string ClassifyMaturity(int score) {
        if (score >= 85) return "PRODUCTION-READY";
        if (score >= 70) return "BETA";
        return "ALPHA";
    }
};

// CMT-FIN-01: All 35 files have Doxygen headers
TEST_F(DoxygenHeadersValidation, CMT_FIN_01_AllFilesHaveHeaders) {
    int files_with_headers = 0;
    int files_without_headers = 0;
    
    for (const auto& filename : REQUIRED_FILES) {
        std::string filepath = std::string(CONTENT_DIR) + "/" + filename;
        std::ifstream file(filepath);
        
        ASSERT_TRUE(file.is_open()) << "File not found: " << filepath;
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        HeaderFields fields = ParseHeader(content);
        if (!fields.file.empty()) {
            files_with_headers++;
        } else {
            files_without_headers++;
            FAIL() << "No header found in " << filename;
        }
    }
    
    EXPECT_EQ(files_with_headers, 35) << "Expected 35 files with headers";
    EXPECT_EQ(files_without_headers, 0) << "Expected 0 files without headers";
}

// CMT-FIN-02: All files have required @note fields
TEST_F(DoxygenHeadersValidation, CMT_FIN_02_AllRequiredNotesPresent) {
    for (const auto& filename : REQUIRED_FILES) {
        std::string filepath = std::string(CONTENT_DIR) + "/" + filename;
        std::ifstream file(filepath);
        ASSERT_TRUE(file.is_open());
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        HeaderFields fields = ParseHeader(content);
        
        EXPECT_FALSE(fields.file.empty()) << filename << ": Missing @file";
        EXPECT_FALSE(fields.brief.empty()) << filename << ": Missing @brief";
        EXPECT_FALSE(fields.version.empty()) << filename << ": Missing @version";
        EXPECT_FALSE(fields.maturity.empty()) << filename << ": Missing @note Maturity";
        EXPECT_GE(fields.score, 0) << filename << ": Missing @note Score";
        EXPECT_FALSE(fields.gap_summary.empty()) << filename << ": Missing @note Gap Summary";
        EXPECT_FALSE(fields.status.empty()) << filename << ": Missing @note Status";
        
        EXPECT_TRUE(fields.has_all_required) << filename << ": Not all required fields present";
    }
}

// CMT-FIN-03: Maturity classification matches score band
TEST_F(DoxygenHeadersValidation, CMT_FIN_03_MaturityClassificationVerified) {
    std::vector<std::string> mismatches;
    
    for (const auto& filename : REQUIRED_FILES) {
        std::string filepath = std::string(CONTENT_DIR) + "/" + filename;
        std::ifstream file(filepath);
        ASSERT_TRUE(file.is_open());
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        HeaderFields fields = ParseHeader(content);
        
        if (fields.score < 0) continue;
        
        std::string expected = ClassifyMaturity(fields.score);
        bool matches = false;
        
        if ((fields.score >= 85 && fields.maturity.find("PRODUCTION-READY") != std::string::npos) ||
            (fields.score >= 70 && fields.score < 85 && fields.maturity.find("BETA") != std::string::npos) ||
            (fields.score < 70 && fields.maturity.find("ALPHA") != std::string::npos)) {
            matches = true;
        }
        
        if (!matches) {
            mismatches.push_back(std::string(filename) + " (score=" + std::to_string(fields.score) + 
                               ", maturity=" + fields.maturity + ")");
        }
        
        EXPECT_TRUE(matches) << filename << " score/maturity mismatch";
    }
    
    EXPECT_TRUE(mismatches.empty()) << "Maturity mismatches found: " 
                                     << mismatches.size();
}

// CMT-FIN-04: Gap Summary format is valid
TEST_F(DoxygenHeadersValidation, CMT_FIN_04_GapSummaryFormatValid) {
    std::regex gap_format_regex(R"(total=\d+;)");
    
    for (const auto& filename : REQUIRED_FILES) {
        std::string filepath = std::string(CONTENT_DIR) + "/" + filename;
        std::ifstream file(filepath);
        ASSERT_TRUE(file.is_open());
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        HeaderFields fields = ParseHeader(content);
        
        if (!fields.gap_summary.empty()) {
            EXPECT_TRUE(std::regex_search(fields.gap_summary, gap_format_regex))
                << filename << ": Gap Summary format invalid: " << fields.gap_summary;
            
            // Check for expected fields
            EXPECT_NE(fields.gap_summary.find("TODO="), std::string::npos) 
                << filename << ": Missing TODO count";
            EXPECT_NE(fields.gap_summary.find("Stub="), std::string::npos)
                << filename << ": Missing Stub count";
            EXPECT_NE(fields.gap_summary.find("C="), std::string::npos)
                << filename << ": Missing CRITICAL severity count";
            EXPECT_NE(fields.gap_summary.find("H="), std::string::npos)
                << filename << ": Missing HIGH severity count";
        }
    }
}

// CMT-FIN-05: Score field is in valid range
TEST_F(DoxygenHeadersValidation, CMT_FIN_05_ScoreInValidRange) {
    for (const auto& filename : REQUIRED_FILES) {
        std::string filepath = std::string(CONTENT_DIR) + "/" + filename;
        std::ifstream file(filepath);
        ASSERT_TRUE(file.is_open());
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        HeaderFields fields = ParseHeader(content);
        
        EXPECT_GE(fields.score, 0) << filename << ": Score cannot be negative";
        EXPECT_LE(fields.score, 100) << filename << ": Score cannot exceed 100";
    }
}

// CMT-FIN-06: Maturity distribution matches expected bands
TEST_F(DoxygenHeadersValidation, CMT_FIN_06_MaturityDistribution) {
    int production_ready = 0;
    int beta = 0;
    int alpha = 0;
    
    for (const auto& filename : REQUIRED_FILES) {
        std::string filepath = std::string(CONTENT_DIR) + "/" + filename;
        std::ifstream file(filepath);
        ASSERT_TRUE(file.is_open());
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        HeaderFields fields = ParseHeader(content);
        
        if (fields.score >= 85) production_ready++;
        else if (fields.score >= 70) beta++;
        else alpha++;
    }
    
    // Verify distribution (from audit: 14 PROD, 18 BETA, 3 ALPHA)
    EXPECT_EQ(production_ready, 14) << "Expected 14 PRODUCTION-READY files";
    EXPECT_EQ(beta, 18) << "Expected 18 BETA files";
    EXPECT_EQ(alpha, 3) << "Expected 3 ALPHA files";
}

}  // namespace themis::content::test
