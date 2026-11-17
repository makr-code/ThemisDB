/**
 * @file skill_registry.cpp
 * @brief Filesystem-backed, progressively disclosed Agent Skill packages.
 */

#include "llm/skill_registry.h"

#include "llm/ai_orchestrator.h"
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <limits>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <system_error>

namespace themis::llm {
namespace {

namespace fs = std::filesystem;
using ::themis::errors::ErrorCode;

constexpr std::string_view kSkillFileName = "SKILL.md";

template <typename T>
[[nodiscard]] ::themis::Result<T> skillError(ErrorCode code,
                                              std::string context) {
    return ::themis::Err<T>(code, "SkillRegistry: " + std::move(context));
}

[[nodiscard]] bool isWithinRoot(const fs::path& candidate,
                                const fs::path& root) {
    auto candidate_part = candidate.begin();
    for (auto root_part = root.begin(); root_part != root.end();
         ++root_part, ++candidate_part) {
        if (candidate_part == candidate.end() || *candidate_part != *root_part) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool isMissingPathError(const std::error_code& error) {
    return error == std::errc::no_such_file_or_directory ||
           error == std::errc::not_a_directory;
}

[[nodiscard]] bool isValidSkillName(std::string_view name) {
    if (name.empty() || name.size() > 64 || name.front() == '-' ||
        name.back() == '-') {
        return false;
    }

    bool previous_hyphen = false;
    for (const unsigned char character : name) {
        const bool is_lower_alnum =
            (character >= static_cast<unsigned char>('a') &&
             character <= static_cast<unsigned char>('z')) ||
            (character >= static_cast<unsigned char>('0') &&
             character <= static_cast<unsigned char>('9'));
        if (character == static_cast<unsigned char>('-')) {
            if (previous_hyphen) {
                return false;
            }
            previous_hyphen = true;
            continue;
        }
        if (!is_lower_alnum) {
            return false;
        }
        previous_hyphen = false;
    }
    return true;
}

[[nodiscard]] bool isValidUtf8(std::string_view text) {
    std::size_t index = 0;
    while (index < text.size()) {
        const auto first = static_cast<unsigned char>(text[index]);
        if (first <= 0x7f) {
            ++index;
            continue;
        }

        std::size_t continuation_count = 0;
        unsigned char second_min = 0x80;
        unsigned char second_max = 0xbf;
        if (first >= 0xc2 && first <= 0xdf) {
            continuation_count = 1;
        } else if (first >= 0xe0 && first <= 0xef) {
            continuation_count = 2;
            if (first == 0xe0) {
                second_min = 0xa0;
            } else if (first == 0xed) {
                second_max = 0x9f;
            }
        } else if (first >= 0xf0 && first <= 0xf4) {
            continuation_count = 3;
            if (first == 0xf0) {
                second_min = 0x90;
            } else if (first == 0xf4) {
                second_max = 0x8f;
            }
        } else {
            return false;
        }

        if (index + continuation_count >= text.size()) {
            return false;
        }
        const auto second = static_cast<unsigned char>(text[index + 1]);
        if (second < second_min || second > second_max) {
            return false;
        }
        for (std::size_t offset = 2; offset <= continuation_count; ++offset) {
            const auto continuation =
                static_cast<unsigned char>(text[index + offset]);
            if (continuation < 0x80 || continuation > 0xbf) {
                return false;
            }
        }
        index += continuation_count + 1;
    }
    return true;
}

[[nodiscard]] ::themis::Result<std::string> readLimitedFile(
    const fs::path& path,
    std::size_t maximum_bytes,
    ErrorCode size_error) {
    std::error_code filesystem_error;
    const auto file_size = fs::file_size(path, filesystem_error);
    if (filesystem_error) {
        return skillError<std::string>(
            ErrorCode::ERR_LLM_SKILL_IO_FAILED, "cannot inspect a package file");
    }
    if (file_size > maximum_bytes) {
        return skillError<std::string>(size_error, "configured file size limit exceeded");
    }

    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return skillError<std::string>(
            ErrorCode::ERR_LLM_SKILL_IO_FAILED, "cannot open a package file");
    }

    std::string contents;
    contents.reserve(static_cast<std::size_t>(file_size));
    std::array<char, 4096> buffer{};
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto read_count = input.gcount();
        if (read_count <= 0) {
            break;
        }
        const auto count = static_cast<std::size_t>(read_count);
        if (count > maximum_bytes - contents.size()) {
            return skillError<std::string>(
                size_error, "configured file size limit exceeded");
        }
        contents.append(buffer.data(), count);
    }
    if (input.bad()) {
        return skillError<std::string>(
            ErrorCode::ERR_LLM_SKILL_IO_FAILED, "failed while reading a package file");
    }
    if (!isValidUtf8(contents)) {
        return skillError<std::string>(
            ErrorCode::ERR_LLM_SKILL_INVALID_CONTENT, "package content is not valid UTF-8");
    }
    return contents;
}

[[nodiscard]] ::themis::Result<std::string> readFrontmatter(
    const fs::path& path,
    std::size_t maximum_bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return skillError<std::string>(
            ErrorCode::ERR_LLM_SKILL_IO_FAILED, "cannot open SKILL.md");
    }

    std::string header;
    std::string line;
    std::size_t bytes_read = 0;
    bool first_line = true;
    char character = '\0';

    while (input.get(character)) {
        ++bytes_read;
        if (bytes_read > maximum_bytes) {
            return skillError<std::string>(
                ErrorCode::ERR_LLM_SKILL_SIZE_LIMIT, "frontmatter size limit exceeded");
        }
        if (character != '\n') {
            line.push_back(character);
            continue;
        }

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (first_line && line != "---") {
            return skillError<std::string>(
                ErrorCode::ERR_LLM_SKILL_INVALID_METADATA,
                "SKILL.md must start with YAML frontmatter");
        }
        first_line = false;
        header.append(line);
        header.push_back('\n');
        if (line == "---" && header.size() > 4) {
            return header;
        }
        line.clear();
    }

    if (input.bad()) {
        return skillError<std::string>(
            ErrorCode::ERR_LLM_SKILL_IO_FAILED, "failed while reading SKILL.md frontmatter");
    }
    if (!line.empty()) {
        if (line.back() == '\r') {
            line.pop_back();
        }
        if (first_line && line != "---") {
            return skillError<std::string>(
                ErrorCode::ERR_LLM_SKILL_INVALID_METADATA,
                "SKILL.md must start with YAML frontmatter");
        }
        if (line == "---" && !first_line) {
            header.append(line);
            return header;
        }
    }
    return skillError<std::string>(
        ErrorCode::ERR_LLM_SKILL_INVALID_METADATA,
        "SKILL.md frontmatter closing delimiter is missing");
}

struct ParsedFrontmatter {
    SkillDescriptor descriptor;
    std::size_t body_offset = 0;
};

[[nodiscard]] ::themis::Result<ParsedFrontmatter> parseFrontmatter(
    std::string_view contents,
    const fs::path& package_root,
    std::size_t maximum_bytes) {
    if (!isValidUtf8(contents)) {
        return skillError<ParsedFrontmatter>(
            ErrorCode::ERR_LLM_SKILL_INVALID_CONTENT,
            "SKILL.md frontmatter is not valid UTF-8");
    }

    std::size_t offset = 0;
    std::size_t header_bytes = 0;
    bool exceeded_limit = false;
    auto nextLine = [&contents, &offset, &header_bytes, maximum_bytes]()
        -> std::optional<std::string_view> {
        if (offset >= contents.size()) {
            return std::nullopt;
        }
        const auto newline = contents.find('\n', offset);
        const auto line_end =
            newline == std::string_view::npos ? contents.size() : newline;
        auto line = contents.substr(offset, line_end - offset);
        offset = newline == std::string_view::npos ? contents.size() : newline + 1;
        header_bytes += offset - (line_end - line.size());
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        if (header_bytes > maximum_bytes) {
            return std::nullopt;
        }
        return line;
    };

    const auto opening = nextLine();
    if (header_bytes > maximum_bytes) {
        return skillError<ParsedFrontmatter>(
            ErrorCode::ERR_LLM_SKILL_SIZE_LIMIT, "frontmatter size limit exceeded");
    }
    if (!opening || *opening != "---") {
        return skillError<ParsedFrontmatter>(
            ErrorCode::ERR_LLM_SKILL_INVALID_METADATA,
            "SKILL.md must start with YAML frontmatter");
    }

    std::string yaml_text;
    bool closed = false;
    while (true) {
        const auto line = nextLine();
        if (header_bytes > maximum_bytes) {
            exceeded_limit = true;
            break;
        }
        if (!line) {
            break;
        }
        if (*line == "---") {
            closed = true;
            break;
        }
        yaml_text.append(*line);
        yaml_text.push_back('\n');
    }
    if (exceeded_limit) {
        return skillError<ParsedFrontmatter>(
            ErrorCode::ERR_LLM_SKILL_SIZE_LIMIT, "frontmatter size limit exceeded");
    }
    if (!closed) {
        return skillError<ParsedFrontmatter>(
            ErrorCode::ERR_LLM_SKILL_INVALID_METADATA,
            "SKILL.md frontmatter closing delimiter is missing");
    }

    YAML::Node metadata;
    try {
        metadata = YAML::Load(yaml_text);
    } catch (const YAML::Exception&) {
        return skillError<ParsedFrontmatter>(
            ErrorCode::ERR_LLM_SKILL_INVALID_METADATA, "YAML frontmatter is malformed");
    }
    if (!metadata.IsMap() || !metadata["name"] || !metadata["name"].IsScalar() ||
        !metadata["description"] || !metadata["description"].IsScalar()) {
        return skillError<ParsedFrontmatter>(
            ErrorCode::ERR_LLM_SKILL_INVALID_METADATA,
            "frontmatter requires scalar name and description fields");
    }

    std::set<std::string> metadata_keys;
    try {
        for (const auto& entry : metadata) {
            if (!entry.first.IsScalar() ||
                !metadata_keys.insert(entry.first.as<std::string>()).second) {
                return skillError<ParsedFrontmatter>(
                    ErrorCode::ERR_LLM_SKILL_INVALID_METADATA,
                    "frontmatter keys must be unique scalar values");
            }
        }
    } catch (const YAML::Exception&) {
        return skillError<ParsedFrontmatter>(
            ErrorCode::ERR_LLM_SKILL_INVALID_METADATA,
            "frontmatter keys must be unique scalar values");
    }

    std::string name;
    std::string description;
    try {
        name = metadata["name"].as<std::string>();
        description = metadata["description"].as<std::string>();
    } catch (const YAML::Exception&) {
        return skillError<ParsedFrontmatter>(
            ErrorCode::ERR_LLM_SKILL_INVALID_METADATA,
            "frontmatter name and description must be strings");
    }

    if (!isValidSkillName(name) || name != package_root.filename().string()) {
        return skillError<ParsedFrontmatter>(
            ErrorCode::ERR_LLM_SKILL_INVALID_METADATA,
            "skill name must be a lowercase package name matching its directory");
    }
    if (description.empty() || description.size() > 1024 ||
        !isValidUtf8(description)) {
        return skillError<ParsedFrontmatter>(
            ErrorCode::ERR_LLM_SKILL_INVALID_METADATA,
            "description must be non-empty UTF-8 text of at most 1024 bytes");
    }

    return ParsedFrontmatter{
        SkillDescriptor{std::move(name), std::move(description), package_root},
        offset,
    };
}

[[nodiscard]] ::themis::Result<fs::path> canonicalPackageFile(
    const fs::path& package_root,
    const fs::path& relative_path) {
    if (relative_path.empty() || relative_path.is_absolute() ||
        relative_path.has_root_name() || relative_path.has_root_directory()) {
        return skillError<fs::path>(
            ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE, "resource path must be package-relative");
    }

    fs::path candidate = package_root;
    std::error_code filesystem_error;
    for (const auto& component : relative_path) {
        if (component.empty() || component == "." || component == "..") {
            return skillError<fs::path>(
                ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE,
                "resource path contains an invalid component");
        }
        candidate /= component;
        const auto status = fs::symlink_status(candidate, filesystem_error);
        if (filesystem_error) {
            if (isMissingPathError(filesystem_error)) {
                return skillError<fs::path>(
                    ErrorCode::ERR_LLM_SKILL_NOT_FOUND,
                    "package resource does not exist");
            }
            return skillError<fs::path>(
                ErrorCode::ERR_LLM_SKILL_IO_FAILED, "cannot inspect a package resource");
        }
        if (status.type() == fs::file_type::not_found) {
            return skillError<fs::path>(
                ErrorCode::ERR_LLM_SKILL_NOT_FOUND,
                "package resource does not exist");
        }
        if (fs::is_symlink(status)) {
            return skillError<fs::path>(
                ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE, "package resources cannot be symlinks");
        }
    }

    const auto canonical_candidate = fs::canonical(candidate, filesystem_error);
    if (filesystem_error) {
        if (!isMissingPathError(filesystem_error)) {
            return skillError<fs::path>(
                ErrorCode::ERR_LLM_SKILL_IO_FAILED,
                "cannot resolve a package resource");
        }
        return skillError<fs::path>(
            ErrorCode::ERR_LLM_SKILL_NOT_FOUND, "package resource does not exist");
    }
    if (!isWithinRoot(canonical_candidate, package_root)) {
        return skillError<fs::path>(
            ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE,
            "resolved resource is outside its package root");
    }
    if (!fs::is_regular_file(canonical_candidate, filesystem_error) ||
        filesystem_error) {
        return skillError<fs::path>(
            ErrorCode::ERR_LLM_SKILL_IO_FAILED, "package resource is not a regular file");
    }
    return canonical_candidate;
}

[[nodiscard]] ::themis::Result<SkillDescriptor> findSkill(
    const std::unordered_map<std::string, SkillDescriptor>& skills,
    std::string_view name) {
    const auto found = skills.find(std::string(name));
    if (found == skills.end()) {
        return skillError<SkillDescriptor>(
            ErrorCode::ERR_LLM_SKILL_NOT_FOUND, "requested skill was not discovered");
    }
    return found->second;
}

[[nodiscard]] nlohmann::json toolError(const ::themis::Error& error) {
    return {
        {"error", error.message()},
        {"error_code", static_cast<int>(error.code())},
    };
}

} // namespace

SkillRegistry::SkillRegistry(SkillRegistryOptions options)
    : options_(std::move(options)) {}

std::shared_ptr<const SkillRegistry::Snapshot> SkillRegistry::snapshot() const {
    std::shared_lock lock(mutex_);
    return snapshot_;
}

::themis::Result<std::size_t> SkillRegistry::refresh() {
    if (options_.roots.empty() || options_.max_frontmatter_bytes == 0 ||
        options_.max_instruction_bytes == 0 || options_.max_resource_bytes == 0) {
        return skillError<std::size_t>(
            ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "at least one root and positive file-size limits are required");
    }

    std::map<std::string, SkillDescriptor> discovered;
    for (const auto& configured_root : options_.roots) {
        std::error_code filesystem_error;
        const auto root = fs::canonical(configured_root, filesystem_error);
        if (filesystem_error || !fs::is_directory(root, filesystem_error) ||
            filesystem_error) {
            return skillError<std::size_t>(
                ErrorCode::ERR_LLM_SKILL_IO_FAILED,
                "a configured package root is missing or not a directory");
        }

        fs::directory_iterator iterator(root, filesystem_error);
        if (filesystem_error) {
            return skillError<std::size_t>(
                ErrorCode::ERR_LLM_SKILL_IO_FAILED, "cannot scan a configured package root");
        }
        const fs::directory_iterator end;
        while (iterator != end) {
            const auto entry = *iterator;
            const auto entry_status = entry.symlink_status(filesystem_error);
            if (filesystem_error) {
                return skillError<std::size_t>(
                    ErrorCode::ERR_LLM_SKILL_IO_FAILED,
                    "cannot inspect a package directory entry");
            }
            if (fs::is_symlink(entry_status)) {
                return skillError<std::size_t>(
                    ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE,
                    "configured roots cannot contain symlinked package entries");
            }
            if (fs::is_directory(entry_status)) {
                const auto package_root = fs::canonical(entry.path(), filesystem_error);
                if (filesystem_error || !isWithinRoot(package_root, root)) {
                    return skillError<std::size_t>(
                        ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE,
                        "package directory resolves outside its configured root");
                }

                const auto manifest = package_root / kSkillFileName;
                const auto manifest_status = fs::symlink_status(manifest, filesystem_error);
                if (filesystem_error == std::errc::no_such_file_or_directory) {
                    filesystem_error.clear();
                } else if (filesystem_error) {
                    return skillError<std::size_t>(
                        ErrorCode::ERR_LLM_SKILL_IO_FAILED,
                        "cannot inspect a package manifest");
                } else if (fs::exists(manifest_status)) {
                    if (fs::is_symlink(manifest_status) ||
                        !fs::is_regular_file(manifest_status)) {
                        return skillError<std::size_t>(
                            ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE,
                            "SKILL.md must be a regular, non-symlink file");
                    }
                    const auto canonical_manifest = fs::canonical(manifest, filesystem_error);
                    if (filesystem_error || !isWithinRoot(canonical_manifest, package_root)) {
                        return skillError<std::size_t>(
                            ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE,
                            "SKILL.md resolves outside its package directory");
                    }

                    auto header = readFrontmatter(
                        canonical_manifest, options_.max_frontmatter_bytes);
                    if (!header) {
                        return ::tl::unexpected(header.error());
                    }
                    auto parsed = parseFrontmatter(
                        *header, package_root, options_.max_frontmatter_bytes);
                    if (!parsed) {
                        return ::tl::unexpected(parsed.error());
                    }
                    const bool inserted = discovered.emplace(
                        parsed->descriptor.name, parsed->descriptor).second;
                    if (!inserted) {
                        return skillError<std::size_t>(
                            ErrorCode::ERR_LLM_SKILL_DUPLICATE_NAME,
                            "multiple packages use the same skill name");
                    }
                }
            }
            iterator.increment(filesystem_error);
            if (filesystem_error) {
                return skillError<std::size_t>(
                    ErrorCode::ERR_LLM_SKILL_IO_FAILED,
                    "failed while scanning a configured package root");
            }
        }
    }

    auto next_snapshot = std::make_shared<Snapshot>();
    next_snapshot->skills.reserve(discovered.size());
    for (auto& [name, descriptor] : discovered) {
        next_snapshot->skills.emplace(name, std::move(descriptor));
    }

    const auto count = next_snapshot->skills.size();
    {
        std::unique_lock lock(mutex_);
        snapshot_ = std::move(next_snapshot);
    }
    return count;
}

::themis::Result<std::vector<SkillDescriptor>> SkillRegistry::list() const {
    const auto current = snapshot();
    if (!current) {
        return skillError<std::vector<SkillDescriptor>>(
            ErrorCode::ERR_LLM_SKILL_REGISTRY_NOT_INITIALIZED,
            "refresh must succeed before listing skills");
    }

    std::vector<SkillDescriptor> skills;
    skills.reserve(current->skills.size());
    for (const auto& [name, descriptor] : current->skills) {
        (void)name;
        skills.push_back(descriptor);
    }
    std::sort(skills.begin(), skills.end(),
              [](const SkillDescriptor& left, const SkillDescriptor& right) {
                  return left.name < right.name;
              });
    return skills;
}

::themis::Result<SkillDocument> SkillRegistry::load(std::string_view name) const {
    const auto current = snapshot();
    if (!current) {
        return skillError<SkillDocument>(
            ErrorCode::ERR_LLM_SKILL_REGISTRY_NOT_INITIALIZED,
            "refresh must succeed before loading skills");
    }
    auto descriptor = findSkill(current->skills, name);
    if (!descriptor) {
        return ::tl::unexpected(descriptor.error());
    }

    auto manifest = canonicalPackageFile(
        descriptor->package_root, fs::path("SKILL.md"));
    if (!manifest) {
        return ::tl::unexpected(manifest.error());
    }
    if (options_.max_instruction_bytes >
        std::numeric_limits<std::size_t>::max() - options_.max_frontmatter_bytes) {
        return skillError<SkillDocument>(
            ErrorCode::ERR_UTIL_INVALID_ARGUMENT, "configured size limits overflow");
    }
    const auto maximum_bytes =
        options_.max_frontmatter_bytes + options_.max_instruction_bytes;
    auto contents = readLimitedFile(
        *manifest, maximum_bytes, ErrorCode::ERR_LLM_SKILL_SIZE_LIMIT);
    if (!contents) {
        return ::tl::unexpected(contents.error());
    }

    auto parsed = parseFrontmatter(
        *contents, descriptor->package_root, options_.max_frontmatter_bytes);
    if (!parsed) {
        return ::tl::unexpected(parsed.error());
    }
    if (parsed->descriptor.name != descriptor->name ||
        parsed->descriptor.description != descriptor->description) {
        return skillError<SkillDocument>(
            ErrorCode::ERR_LLM_SKILL_INVALID_METADATA,
            "manifest changed after the last successful refresh");
    }

    const auto body_size = contents->size() - parsed->body_offset;
    if (body_size > options_.max_instruction_bytes) {
        return skillError<SkillDocument>(
            ErrorCode::ERR_LLM_SKILL_SIZE_LIMIT, "instruction size limit exceeded");
    }
    return SkillDocument{
        std::move(*descriptor),
        contents->substr(parsed->body_offset),
    };
}

::themis::Result<std::string> SkillRegistry::readResource(
    std::string_view name,
    std::string_view relative_path) const {
    const auto current = snapshot();
    if (!current) {
        return skillError<std::string>(
            ErrorCode::ERR_LLM_SKILL_REGISTRY_NOT_INITIALIZED,
            "refresh must succeed before reading resources");
    }
    auto descriptor = findSkill(current->skills, name);
    if (!descriptor) {
        return ::tl::unexpected(descriptor.error());
    }

    if (!isValidUtf8(relative_path) ||
        relative_path.find('\0') != std::string_view::npos) {
        return skillError<std::string>(
            ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE,
            "resource path must be valid UTF-8 without NUL bytes");
    }

    fs::path requested_path;
    try {
        requested_path = fs::u8path(relative_path.begin(), relative_path.end());
    } catch (const std::range_error&) {
        return skillError<std::string>(
            ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE,
            "resource path cannot be represented by the filesystem");
    }
    auto safe_path = canonicalPackageFile(descriptor->package_root, requested_path);
    if (!safe_path) {
        return ::tl::unexpected(safe_path.error());
    }
    return readLimitedFile(
        *safe_path, options_.max_resource_bytes, ErrorCode::ERR_LLM_SKILL_SIZE_LIMIT);
}

void registerSkillTools(ToolRegistry& tools,
                        std::shared_ptr<SkillRegistry> skills) {
    if (!skills) {
        throw std::invalid_argument("registerSkillTools requires a SkillRegistry");
    }

    constexpr std::array<std::string_view, 3> names{
        "list_skills", "load_skill", "read_skill_resource"};
    for (const auto name : names) {
        if (tools.getSpec(std::string(name)).has_value()) {
            throw std::invalid_argument(
                "registerSkillTools tool name is already registered");
        }
    }

    ToolSpec list_spec;
    list_spec.name = "list_skills";
    list_spec.description =
        "List available Agent Skill names and descriptions without loading instructions.";
    list_spec.args_schema = {
        {"type", "object"},
        {"properties", json::object()},
        {"additionalProperties", false},
    };
    tools.registerTool(list_spec, [skills](const json&, const ModeSpec&) {
        auto result = skills->list();
        if (!result) {
            return toolError(result.error());
        }
        json items = json::array();
        for (const auto& skill : *result) {
            items.push_back({{"name", skill.name}, {"description", skill.description}});
        }
        return json{{"skills", std::move(items)}};
    });

    ToolSpec load_spec;
    load_spec.name = "load_skill";
    load_spec.description =
        "Load the full instructions for one discovered Agent Skill by exact name.";
    load_spec.args_schema = {
        {"type", "object"},
        {"properties", {{"name", {{"type", "string"}}}}},
        {"required", json::array({"name"})},
        {"additionalProperties", false},
    };
    tools.registerTool(load_spec, [skills](const json& args, const ModeSpec&) {
        if (!args.is_object() || !args.contains("name") ||
            !args["name"].is_string()) {
            return toolError(::themis::Error(
                ErrorCode::ERR_TOOL_INVALID_INPUT, "load_skill requires a string name"));
        }
        auto result = skills->load(args["name"].get<std::string>());
        if (!result) {
            return toolError(result.error());
        }
        return json{
            {"name", result->descriptor.name},
            {"description", result->descriptor.description},
            {"instructions", result->instructions},
        };
    });

    ToolSpec resource_spec;
    resource_spec.name = "read_skill_resource";
    resource_spec.description =
        "Read one UTF-8 resource file beneath a discovered Agent Skill package.";
    resource_spec.args_schema = {
        {"type", "object"},
        {"properties",
         {
             {"name", {{"type", "string"}}},
             {"path", {{"type", "string"}}},
         }},
        {"required", json::array({"name", "path"})},
        {"additionalProperties", false},
    };
    tools.registerTool(resource_spec, [skills](const json& args, const ModeSpec&) {
        if (!args.is_object() || !args.contains("name") ||
            !args["name"].is_string() || !args.contains("path") ||
            !args["path"].is_string()) {
            return toolError(::themis::Error(
                ErrorCode::ERR_TOOL_INVALID_INPUT,
                "read_skill_resource requires string name and path"));
        }
        const auto name = args["name"].get<std::string>();
        const auto path = args["path"].get<std::string>();
        auto result = skills->readResource(name, path);
        if (!result) {
            return toolError(result.error());
        }
        return json{{"name", name}, {"path", path}, {"content", *result}};
    });
}

} // namespace themis::llm
