/**
 * @file skill_registry.h
 * @brief Read-only discovery and loading of portable Agent Skill packages.
 */

#pragma once

#include "utils/expected.h"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace themis::llm {

class ToolRegistry;

/**
 * @brief Resource limits and explicitly configured roots for skill packages.
 *
 * A package is a child directory containing a `SKILL.md`. Discovery parses
 * bounded YAML frontmatter only; buffered file reads may read ahead into the
 * instruction body. Limits apply before content is returned to callers.
 */
struct SkillRegistryOptions {
    std::vector<std::filesystem::path> roots;
    std::size_t max_frontmatter_bytes = 16 * 1024;
    std::size_t max_instruction_bytes = 1024 * 1024;
    std::size_t max_resource_bytes = 1024 * 1024;
};

/**
 * @brief Metadata available without loading a skill's instructions.
 */
struct SkillDescriptor {
    std::string name;
    std::string description;
    std::filesystem::path package_root;
};

/**
 * @brief A selected skill's metadata and Markdown instruction body.
 */
struct SkillDocument {
    SkillDescriptor descriptor;
    std::string instructions;
};

/**
 * @brief Discovers Agent Skill packages and loads their content on demand.
 *
 * @note Call refresh() before list(), load(), or readResource(). Refresh is
 *       atomic: on failure, readers continue to see the last successful
 *       snapshot. Concurrent readers are supported.
 * @note Package content is untrusted text. This class never executes scripts
 *       and does not grant tool or database permissions.
 * @note Path checks and file opens are not atomic against concurrent filesystem
 *       replacement. Configured roots, package contents, and their parents
 *       must not be writable by untrusted users while this registry is in use.
 */
class SkillRegistry {
public:
    /**
     * @brief Construct a registry with explicit package roots and read limits.
     * @param options Configured roots and maximum frontmatter/instruction/
     *        resource sizes. Roots are not scanned until refresh() is called.
     */
    explicit SkillRegistry(SkillRegistryOptions options);

    /**
     * @brief Scan configured roots and atomically publish discovered metadata.
     * @return Number of discovered packages, or a typed error for invalid roots,
     *         invalid limits, malformed/ambiguous metadata, invalid UTF-8
     *         frontmatter, size limits, duplicate names, unsafe paths, or
     *         filesystem failures.
     */
    [[nodiscard]] ::themis::Result<std::size_t> refresh();

    /**
     * @brief Return the current metadata snapshot in name-sorted order.
     * @return Descriptors without instruction bodies, or an error if refresh()
     *         has not completed successfully.
     */
    [[nodiscard]] ::themis::Result<std::vector<SkillDescriptor>> list() const;

    /**
     * @brief Load one skill's instructions after validating its current manifest.
     * @param name Exact, case-sensitive skill name from the metadata snapshot.
     * @return The selected skill document, or a typed error if it is missing,
     *         the registry is uninitialized, changed since discovery,
     *         malformed, invalid UTF-8, unsafe/unreadable, or exceeds configured
     *         limits.
     */
    [[nodiscard]] ::themis::Result<SkillDocument> load(
        std::string_view name) const;

    /**
     * @brief Read a resource beneath a selected package directory.
     * @param name Exact skill name from the metadata snapshot.
     * @param relative_path Non-empty UTF-8 package-relative path without NUL
     *        bytes; absolute paths, traversal components, symlinks, and
     *        out-of-root targets are rejected.
     * @return UTF-8 resource contents, or a typed error on invalid paths,
     *         missing files, encoding errors, or size-limit violations.
     */
    [[nodiscard]] ::themis::Result<std::string> readResource(
        std::string_view name,
        std::string_view relative_path) const;

private:
    struct Snapshot {
        std::unordered_map<std::string, SkillDescriptor> skills;
    };

    [[nodiscard]] std::shared_ptr<const Snapshot> snapshot() const;

    SkillRegistryOptions options_;
    mutable std::shared_mutex mutex_;
    std::shared_ptr<const Snapshot> snapshot_;
};

/**
 * @brief Register optional skill discovery/loading tools with a ToolRegistry.
 * @param tools Registry that applies the active ModeSpec allow/deny lists when
 *        invoking tools.
 * @param skills Shared registry that must outlive registered tool callbacks.
 * @throws std::invalid_argument if skills is null or a skill tool name is
 *         already registered. No tools are registered on a detected conflict.
 * @note The tools are only callable when the active mode allowlist permits them.
 *       Skill content cannot grant additional tool permissions.
 */
void registerSkillTools(ToolRegistry& tools,
                        std::shared_ptr<SkillRegistry> skills);

} // namespace themis::llm
