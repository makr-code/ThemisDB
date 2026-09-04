/**
 * @file file_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/functions/function_registry.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>



namespace themis {
namespace query {
namespace functions {

/**
 * @brief File/Path Functions for AQL
 * 
 * Provides functions for working with file paths, names, and metadata.
 * 
 * ## Categories
 * 
 * ### Path Manipulation
 * - PATH_JOIN, PATH_DIRNAME, PATH_BASENAME, PATH_EXTENSION
 * - PATH_NORMALIZE, PATH_RELATIVE, PATH_ABSOLUTE
 * - PATH_SPLIT, PATH_PARENT
 * 
 * ### Path Analysis
 * - PATH_IS_ABSOLUTE, PATH_IS_RELATIVE
 * - PATH_EXISTS (requires context), PATH_IS_FILE, PATH_IS_DIRECTORY
 * 
 * ### File Name Operations
 * - FILENAME, FILENAME_WITHOUT_EXT, FILE_EXT
 * - SANITIZE_FILENAME
 * 
 * ### MIME Types
 * - MIME_TYPE, IS_IMAGE, IS_VIDEO, IS_AUDIO, IS_DOCUMENT
 * 
 * ### Size Formatting
 * - FORMAT_FILESIZE, PARSE_FILESIZE
 * 
 * ## Note
 * These are pure path string operations. They do not access the filesystem.
 * For actual file operations, use the Storage API.
 */

// ============================================================================
// Path Manipulation Functions
// ============================================================================

/**
 * @brief PATH_JOIN(path1, path2, ...) - Join path components
 */
class PathJoinFunction : public IFunction {
public:
    ~PathJoinFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_JOIN",
            "File",
            "Join multiple path components with proper separators",
            {
                {"paths", ArgType::ANY, true, nullptr, "Path components to join (variadic)"}
            },
            ArgType::STRING,
            true,
            false,
            {"PATH_JOIN('/home', 'user', 'docs')  // '/home/user/docs'"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        if (args.empty()) {
            throw std::runtime_error("PATH_JOIN requires at least one argument");
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string result;
        
        for (const auto& arg : args) {
            std::string part = arg.is_string() ? arg.get<std::string>() : arg.dump();
            
            if (part.empty()) {
              continue;
            }
            
            // If part is absolute, start fresh
            if (!part.empty() && (part[0] == '/' || (part.length() > 1 && part[1] == ':'))) {
                result = part;
            } else {
                // Append with separator
                if (!result.empty() && result.back() != '/' && result.back() != '\\') {
                    result += '/';
                }
                result += part;
            }
        }
        
        return result;
    }
};

/**
 * @brief PATH_DIRNAME(path) - Get directory part of path
 */
class PathDirnameFunction : public IFunction {
public:
    ~PathDirnameFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_DIRNAME",
            "File",
            "Extract the directory portion of a path",
            {
                {"path", ArgType::STRING, true, nullptr, "File path"}
            },
            ArgType::STRING,
            true,
            false,
            {"PATH_DIRNAME('/home/user/file.txt')  // '/home/user'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        // Find last separator
        size_t lastSep = path.find_last_of("/\\");
        
        if (lastSep == std::string::npos) {
            return ".";  // Current directory
        }
        if (lastSep == 0) {
            return "/";  // Root
        }
        
        return path.substr(0, lastSep);
    }
};

/**
 * @brief PATH_BASENAME(path) - Get filename from path
 */
class PathBasenameFunction : public IFunction {
public:
    ~PathBasenameFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_BASENAME",
            "File",
            "Extract the filename portion of a path",
            {
                {"path", ArgType::STRING, true, nullptr, "File path"},
                {"strip_extension", ArgType::BOOLEAN, false, nlohmann::json(false), "Remove extension"}
            },
            ArgType::STRING,
            true,
            false,
            {"PATH_BASENAME('/home/user/file.txt')  // 'file.txt'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        bool stripExt = args.size() > 1 && args[1].get<bool>();
        
        // Find last separator
        size_t lastSep = path.find_last_of("/\\");
        std::string basename = (lastSep == std::string::npos) ? path : path.substr(lastSep + 1);
        
        if (stripExt) {
            size_t dotPos = basename.find_last_of('.');
            if (dotPos != std::string::npos && dotPos > 0) {
                basename = basename.substr(0, dotPos);
            }
        }
        
        return basename;
    }
};

/**
 * @brief PATH_EXTENSION(path) - Get file extension
 */
class PathExtensionFunction : public IFunction {
public:
    ~PathExtensionFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_EXTENSION",
            "File",
            "Extract the file extension (without dot)",
            {
                {"path", ArgType::STRING, true, nullptr, "File path"}
            },
            ArgType::STRING,
            true,
            false,
            {"PATH_EXTENSION('/home/user/file.txt')  // 'txt'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        // Get basename first
        size_t lastSep = path.find_last_of("/\\");
        std::string basename = (lastSep == std::string::npos) ? path : path.substr(lastSep + 1);
        
        // Find extension
        size_t dotPos = basename.find_last_of('.');
        if (dotPos == std::string::npos || dotPos == 0 || dotPos == basename.length() - 1) {
            return "";
        }
        
        return basename.substr(dotPos + 1);
    }
};

/**
 * @brief PATH_NORMALIZE(path) - Normalize path separators and resolve . and ..
 */
class PathNormalizeFunction : public IFunction {
public:
    ~PathNormalizeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_NORMALIZE",
            "File",
            "Normalize path by resolving . and .. and standardizing separators",
            {
                {"path", ArgType::STRING, true, nullptr, "Path to normalize"}
            },
            ArgType::STRING,
            true,
            false,
            {"PATH_NORMALIZE('/home/user/../admin/./docs')  // '/home/admin/docs'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        // Replace backslashes with forward slashes
        std::replace(path.begin(), path.end(), '\\', '/');
        
        // Split into components
        std::vector<std::string> parts;
        std::istringstream iss(path);
        std::string part;
        bool isAbsolute = !path.empty() && path[0] == '/';
        
        while (std::getline(iss, part, '/')) {
            if (part.empty() || part == ".") {
                continue;
            }
            if (part == "..") {
                if (!parts.empty() && parts.back() != "..") {
                    parts.pop_back();
                } else if (!isAbsolute) {
                    parts.push_back("..");
                }
            } else {
                parts.push_back(part);
            }
        }
        
        // Rebuild path
        std::string result;
        if (isAbsolute) {
          result = "/";
        }
        
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) {
              result += "/";
            }
            result += parts[i];
        }
        
        if (result.empty()) {
          result = ".";
        }
        
        return result;
    }
};

/**
 * @brief PATH_SPLIT(path) - Split path into components
 */
class PathSplitFunction : public IFunction {
public:
    ~PathSplitFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_SPLIT",
            "File",
            "Split path into array of components",
            {
                {"path", ArgType::STRING, true, nullptr, "Path to split"}
            },
            ArgType::ARRAY,
            true,
            false,
            {"PATH_SPLIT('/home/user/docs')  // ['/', 'home', 'user', 'docs']"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        nlohmann::json result = nlohmann::json::array();
        
        if (!path.empty() && (path[0] == '/' || path[0] == '\\')) {
            result.push_back("/");
        }
        
        std::replace(path.begin(), path.end(), '\\', '/');
        std::istringstream iss(path);
        std::string part;
        
        while (std::getline(iss, part, '/')) {
            if (!part.empty()) {
                result.push_back(part);
            }
        }
        
        return result;
    }
};

/**
 * @brief PATH_PARENT(path, levels) - Get parent directory
 */
class PathParentFunction : public IFunction {
public:
    ~PathParentFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_PARENT",
            "File",
            "Get parent directory, optionally multiple levels up",
            {
                {"path", ArgType::STRING, true, nullptr, "File path"},
                {"levels", ArgType::INTEGER, false, nlohmann::json(1), "Number of levels to go up"}
            },
            ArgType::STRING,
            true,
            false,
            {"PATH_PARENT('/home/user/docs/file.txt', 2)  // '/home/user'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        int levels = args.size() > 1 ? args[1].get<int>() : 1;
        
        for (int i = 0; i < levels; ++i) {
            size_t lastSep = path.find_last_of("/\\");
            if (lastSep == std::string::npos) {
                return ".";
            }
            if (lastSep == 0) {
                return "/";
            }
            path = path.substr(0, lastSep);
        }
        
        return path;
    }
};

// ============================================================================
// Path Analysis Functions
// ============================================================================

/**
 * @brief PATH_IS_ABSOLUTE(path) - Check if path is absolute
 */
class PathIsAbsoluteFunction : public IFunction {
public:
    ~PathIsAbsoluteFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_IS_ABSOLUTE",
            "File",
            "Check if path is absolute (starts with / or drive letter)",
            {
                {"path", ArgType::STRING, true, nullptr, "Path to check"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"PATH_IS_ABSOLUTE('/home/user')  // true"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        if (path.empty()) {
          return false;
        }
        
        // Unix-style absolute
        if (path[0] == '/') {
          return true;
        }
        
        // Windows-style absolute (C:\ or \\server)
        if (path.length() >= 2) {
            if (path[1] == ':') {
              return true;
            }
            if (path[0] == '\\' && path[1] == '\\') {
              return true;
            }
        }
        
        return false;
    }
};

/**
 * @brief PATH_IS_RELATIVE(path) - Check if path is relative
 */
class PathIsRelativeFunction : public IFunction {
public:
    ~PathIsRelativeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_IS_RELATIVE",
            "File",
            "Check if path is relative",
            {
                {"path", ArgType::STRING, true, nullptr, "Path to check"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"PATH_IS_RELATIVE('docs/file.txt')  // true"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        if (path.empty()) {
          return true;
        }
        if (path[0] == '/') {
          return false;
        }
        if (path.length() >= 2 && path[1] == ':') {
          return false;
        }
        if (path.length() >= 2 && path[0] == '\\' && path[1] == '\\') {
          return false;
        }
        
        return true;
    }
};

// ============================================================================
// File Name Operations
// ============================================================================

/**
 * @brief FILENAME(path) - Alias for PATH_BASENAME
 */
class FilenameFunction : public IFunction {
public:
    ~FilenameFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "FILENAME",
            "File",
            "Get filename from path (alias for PATH_BASENAME)",
            {
                {"path", ArgType::STRING, true, nullptr, "File path"}
            },
            ArgType::STRING,
            true,
            false,
            {"FILENAME('/path/to/document.pdf')  // 'document.pdf'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        size_t lastSep = path.find_last_of("/\\");
        return (lastSep == std::string::npos) ? path : path.substr(lastSep + 1);
    }
};

/**
 * @brief FILENAME_WITHOUT_EXT(path) - Get filename without extension
 */
class FilenameWithoutExtFunction : public IFunction {
public:
    ~FilenameWithoutExtFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "FILENAME_WITHOUT_EXT",
            "File",
            "Get filename without extension",
            {
                {"path", ArgType::STRING, true, nullptr, "File path"}
            },
            ArgType::STRING,
            true,
            false,
            {"FILENAME_WITHOUT_EXT('/path/to/document.pdf')  // 'document'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        size_t lastSep = path.find_last_of("/\\");
        std::string basename = (lastSep == std::string::npos) ? path : path.substr(lastSep + 1);
        
        size_t dotPos = basename.find_last_of('.');
        if (dotPos != std::string::npos && dotPos > 0) {
            return basename.substr(0, dotPos);
        }
        
        return basename;
    }
};

/**
 * @brief FILE_EXT(path) - Get file extension with dot
 */
class FileExtFunction : public IFunction {
public:
    ~FileExtFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "FILE_EXT",
            "File",
            "Get file extension including the dot",
            {
                {"path", ArgType::STRING, true, nullptr, "File path"}
            },
            ArgType::STRING,
            true,
            false,
            {"FILE_EXT('/path/to/document.pdf')  // '.pdf'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        size_t lastSep = path.find_last_of("/\\");
        std::string basename = (lastSep == std::string::npos) ? path : path.substr(lastSep + 1);
        
        size_t dotPos = basename.find_last_of('.');
        if (dotPos != std::string::npos && dotPos > 0 && dotPos < basename.length() - 1) {
            return basename.substr(dotPos);
        }
        
        return "";
    }
};

/**
 * @brief SANITIZE_FILENAME(name) - Make filename safe for filesystem
 */
class SanitizeFilenameFunction : public IFunction {
public:
    ~SanitizeFilenameFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "SANITIZE_FILENAME",
            "File",
            "Remove or replace unsafe characters from filename",
            {
                {"filename", ArgType::STRING, true, nullptr, "Filename to sanitize"},
                {"replacement", ArgType::STRING, false, nlohmann::json("_"), "Replacement character"}
            },
            ArgType::STRING,
            true,
            false,
            {"SANITIZE_FILENAME('file:name?.txt')  // 'file_name_.txt'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string filename = args[0].get<std::string>();
        std::string replacement = args.size() > 1 ? args[1].get<std::string>() : "_";
        
        // Characters not allowed in filenames on most systems
        const std::string unsafe = "<>:\"/\\|?*";
        
        std::string result;
        for (char c : filename) {
            if (unsafe.find(c) != std::string::npos || c < 32) {
                result += replacement;
            } else {
                result += c;
            }
        }
        
        // Remove leading/trailing dots and spaces
        while (!result.empty() && (result.front() == '.' || result.front() == ' ')) {
            result.erase(result.begin());
        }
        while (!result.empty() && (result.back() == '.' || result.back() == ' ')) {
            result.pop_back();
        }
        
        return result.empty() ? "unnamed" : result;
    }
};

// ============================================================================
// MIME Type Functions
// ============================================================================

namespace mime_types {

inline const std::unordered_map<std::string, std::string>& getExtensionToMime() {
    static const std::unordered_map<std::string, std::string> map = {
        // Documents
        {"pdf", "application/pdf"},
        {"doc", "application/msword"},
        {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {"xls", "application/vnd.ms-excel"},
        {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {"ppt", "application/vnd.ms-powerpoint"},
        {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
        {"odt", "application/vnd.oasis.opendocument.text"},
        {"ods", "application/vnd.oasis.opendocument.spreadsheet"},
        {"txt", "text/plain"},
        {"rtf", "application/rtf"},
        {"csv", "text/csv"},
        {"json", "application/json"},
        {"xml", "application/xml"},
        {"html", "text/html"},
        {"htm", "text/html"},
        {"md", "text/markdown"},
        
        // Images
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"png", "image/png"},
        {"gif", "image/gif"},
        {"bmp", "image/bmp"},
        {"svg", "image/svg+xml"},
        {"webp", "image/webp"},
        {"ico", "image/x-icon"},
        {"tiff", "image/tiff"},
        {"tif", "image/tiff"},
        
        // Video
        {"mp4", "video/mp4"},
        {"avi", "video/x-msvideo"},
        {"mov", "video/quicktime"},
        {"wmv", "video/x-ms-wmv"},
        {"mkv", "video/x-matroska"},
        {"webm", "video/webm"},
        {"flv", "video/x-flv"},
        
        // Audio
        {"mp3", "audio/mpeg"},
        {"wav", "audio/wav"},
        {"ogg", "audio/ogg"},
        {"flac", "audio/flac"},
        {"aac", "audio/aac"},
        {"wma", "audio/x-ms-wma"},
        {"m4a", "audio/mp4"},
        
        // Archives
        {"zip", "application/zip"},
        {"rar", "application/x-rar-compressed"},
        {"tar", "application/x-tar"},
        {"gz", "application/gzip"},
        {"7z", "application/x-7z-compressed"},
        
        // Code
        {"js", "application/javascript"},
        {"css", "text/css"},
        {"py", "text/x-python"},
        {"java", "text/x-java"},
        {"cpp", "text/x-c++"},
        {"c", "text/x-c"},
        {"h", "text/x-c"},
        {"hpp", "text/x-c++"},
        {"sh", "application/x-sh"},
        {"sql", "application/sql"},
    };
    return map;
}

} // namespace mime_types

/**
 * @brief MIME_TYPE(path) - Get MIME type from file extension
 */
class MimeTypeFunction : public IFunction {
public:
    ~MimeTypeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "MIME_TYPE",
            "File",
            "Get MIME type based on file extension",
            {
                {"path", ArgType::STRING, true, nullptr, "File path or extension"}
            },
            ArgType::STRING,
            true,
            false,
            {"MIME_TYPE('document.pdf')  // 'application/pdf'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        // Extract extension
        size_t dotPos = path.find_last_of('.');
        if (dotPos == std::string::npos || dotPos == path.length() - 1) {
            return "application/octet-stream";
        }
        
        std::string ext = path.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        const auto& mimeMap = mime_types::getExtensionToMime();
        auto it = mimeMap.find(ext);
        
        return it != mimeMap.end() ? it->second : "application/octet-stream";
    }
};

/**
 * @brief IS_IMAGE(path) - Check if file is an image
 */
class IsImageFunction : public IFunction {
public:
    ~IsImageFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "IS_IMAGE",
            "File",
            "Check if file has an image extension",
            {
                {"path", ArgType::STRING, true, nullptr, "File path"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"IS_IMAGE('photo.jpg')  // true"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        size_t dotPos = path.find_last_of('.');
        if (dotPos == std::string::npos) {
          return false;
        }
        
        std::string ext = path.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        static const std::unordered_set<std::string> imageExts = {
            "jpg", "jpeg", "png", "gif", "bmp", "svg", "webp", "ico", "tiff", "tif"
        };
        
        return imageExts.count(ext) > 0;
    }
};

/**
 * @brief IS_VIDEO(path) - Check if file is a video
 */
class IsVideoFunction : public IFunction {
public:
    ~IsVideoFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "IS_VIDEO",
            "File",
            "Check if file has a video extension",
            {
                {"path", ArgType::STRING, true, nullptr, "File path"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"IS_VIDEO('movie.mp4')  // true"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        size_t dotPos = path.find_last_of('.');
        if (dotPos == std::string::npos) {
          return false;
        }
        
        std::string ext = path.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        static const std::unordered_set<std::string> videoExts = {
            "mp4", "avi", "mov", "wmv", "mkv", "webm", "flv", "m4v", "mpeg", "mpg"
        };
        
        return videoExts.count(ext) > 0;
    }
};

/**
 * @brief IS_AUDIO(path) - Check if file is audio
 */
class IsAudioFunction : public IFunction {
public:
    ~IsAudioFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "IS_AUDIO",
            "File",
            "Check if file has an audio extension",
            {
                {"path", ArgType::STRING, true, nullptr, "File path"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"IS_AUDIO('song.mp3')  // true"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        size_t dotPos = path.find_last_of('.');
        if (dotPos == std::string::npos) {
          return false;
        }
        
        std::string ext = path.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        static const std::unordered_set<std::string> audioExts = {
            "mp3", "wav", "ogg", "flac", "aac", "wma", "m4a", "opus"
        };
        
        return audioExts.count(ext) > 0;
    }
};

/**
 * @brief IS_DOCUMENT(path) - Check if file is a document
 */
class IsDocumentFunction : public IFunction {
public:
    ~IsDocumentFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "IS_DOCUMENT",
            "File",
            "Check if file has a document extension",
            {
                {"path", ArgType::STRING, true, nullptr, "File path"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"IS_DOCUMENT('report.pdf')  // true"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string path = args[0].get<std::string>();
        
        size_t dotPos = path.find_last_of('.');
        if (dotPos == std::string::npos) {
          return false;
        }
        
        std::string ext = path.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        static const std::unordered_set<std::string> docExts = {
            "pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx",
            "odt", "ods", "odp", "txt", "rtf", "csv", "md"
        };
        
        return docExts.count(ext) > 0;
    }
};

// ============================================================================
// Size Formatting Functions
// ============================================================================

/**
 * @brief FORMAT_FILESIZE(bytes, precision) - Format bytes as human-readable
 */
class FormatFilesizeFunction : public IFunction {
public:
    ~FormatFilesizeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "FORMAT_FILESIZE",
            "File",
            "Format byte count as human-readable size (KB, MB, GB, etc.)",
            {
                {"bytes", ArgType::NUMBER, true, nullptr, "Size in bytes"},
                {"precision", ArgType::INTEGER, false, nlohmann::json(2), "Decimal places"}
            },
            ArgType::STRING,
            true,
            false,
            {"FORMAT_FILESIZE(1536000)  // '1.46 MB'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        double bytes = args[0].get<double>();
        int precision = args.size() > 1 ? args[1].get<int>() : 2;
        
        const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
        int unitIndex = 0;
        
        while (bytes >= 1024.0 && unitIndex < 5) {
            bytes /= 1024.0;
            unitIndex++;
        }
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << bytes << " " << units[unitIndex];
        return oss.str();
    }
};

/**
 * @brief PARSE_FILESIZE(size_string) - Parse human-readable size to bytes
 */
class ParseFilesizeFunction : public IFunction {
public:
    ~ParseFilesizeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PARSE_FILESIZE",
            "File",
            "Parse human-readable size string to bytes",
            {
                {"size_string", ArgType::STRING, true, nullptr, "Size like '1.5 MB' or '100 KB'"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"PARSE_FILESIZE('1.5 MB')  // 1572864"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string sizeStr = args[0].get<std::string>();
        
        // Remove whitespace
        sizeStr.erase(std::remove_if(sizeStr.begin(), sizeStr.end(), ::isspace), sizeStr.end());
        
        // Extract number and unit
        double value = 0;
        std::string unit;
        
        std::regex re("^([0-9.]+)([A-Za-z]*)$");
        std::smatch match;
        
        if (std::regex_match(sizeStr, match, re)) {
            value = std::stod(match[1]);
            unit = match[2];
            std::transform(unit.begin(), unit.end(), unit.begin(), ::toupper);
        } else {
            throw std::runtime_error("PARSE_FILESIZE: Invalid format");
        }
        
        double multiplier = 1;
        if (unit == "KB" || unit == "K") {
          multiplier = 1024;
        }
        else if (unit == "MB" || unit == "M") multiplier = 1024 * 1024;
        else if (unit == "GB" || unit == "G") multiplier = 1024 * 1024 * 1024;
        else if (unit == "TB" || unit == "T") multiplier = 1024.0 * 1024 * 1024 * 1024;
        else if (unit == "PB" || unit == "P") multiplier = 1024.0 * 1024 * 1024 * 1024 * 1024;
        
        return value * multiplier;
    }
};

// ============================================================================
// Registration Function
// ============================================================================

/**
 * @brief Register all File functions with the registry
 */
inline void registerFileFunctions(FunctionRegistry& registry) {
    // Path manipulation
    registry.registerFunction(std::make_unique<PathJoinFunction>());
    registry.registerFunction(std::make_unique<PathDirnameFunction>());
    registry.registerFunction(std::make_unique<PathBasenameFunction>());
    registry.registerFunction(std::make_unique<PathExtensionFunction>());
    registry.registerFunction(std::make_unique<PathNormalizeFunction>());
    registry.registerFunction(std::make_unique<PathSplitFunction>());
    registry.registerFunction(std::make_unique<PathParentFunction>());
    
    // Path analysis
    registry.registerFunction(std::make_unique<PathIsAbsoluteFunction>());
    registry.registerFunction(std::make_unique<PathIsRelativeFunction>());
    
    // Filename operations
    registry.registerFunction(std::make_unique<FilenameFunction>());
    registry.registerFunction(std::make_unique<FilenameWithoutExtFunction>());
    registry.registerFunction(std::make_unique<FileExtFunction>());
    registry.registerFunction(std::make_unique<SanitizeFilenameFunction>());
    
    // MIME types
    registry.registerFunction(std::make_unique<MimeTypeFunction>());
    registry.registerFunction(std::make_unique<IsImageFunction>());
    registry.registerFunction(std::make_unique<IsVideoFunction>());
    registry.registerFunction(std::make_unique<IsAudioFunction>());
    registry.registerFunction(std::make_unique<IsDocumentFunction>());
    
    // Size formatting
    registry.registerFunction(std::make_unique<FormatFilesizeFunction>());
    registry.registerFunction(std::make_unique<ParseFilesizeFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis


