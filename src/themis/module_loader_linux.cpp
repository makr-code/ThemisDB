/**
 * @file module_loader_linux.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Linux-specific module loading helpers for the ThemisDB secure module loader.
//
// This file provides:
//   - GPG detached-signature verification (posix_spawn, no shell)
//   - Extended attribute (xattr) enumeration
//   - ELF metadata extraction (build-ID, .comment section)
//
// Migrated from src/base/module_loader.cpp to src/themis/ as part of the
// v1.7.0 modular build architecture.

#include "themis/base/module_loader.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <type_traits>
#include <spdlog/spdlog.h>

#ifdef __linux__

#include <spawn.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/xattr.h>
#include <elf.h>

namespace themis {
namespace modules {

static constexpr size_t   kGpgReadBufSize      = 256;
static constexpr uint64_t kMaxCommentSectionSize = 4096;
static const char* const  kGpgBin               = "/usr/bin/gpg";

// ============================================================================
// GPG detached-signature verification (posix_spawn, no shell)
// ============================================================================

bool ModuleLoader::verifyGPGSignature(const std::string& modulePath,
                                      const std::string& signaturePath) const {
    // Auto-detect detached signature file.
    std::string sigFile = signaturePath;
    if (sigFile.empty()) {
        for (const auto& ext : {".asc", ".sig", ".gpg"}) {
            std::string candidate = modulePath + ext;
            if (std::filesystem::exists(candidate)) {
                sigFile = candidate;
                break;
            }
        }
    }

    if (sigFile.empty()) {
        spdlog::warn("verifyGPGSignature: no signature file found for: {}",
                     modulePath);
        return false;
    }

    // Build argv without going through a shell – no injection risk.
    // argv must be char* (not const char*) per POSIX, but posix_spawn does
    // not modify the strings.
    char* const argv[] = {
        const_cast<char*>("gpg"),
        const_cast<char*>("--verify"),
        const_cast<char*>(sigFile.c_str()),
        const_cast<char*>(modulePath.c_str()),
        nullptr
    };

    // Create a pipe to capture combined stdout + stderr.
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        spdlog::error("verifyGPGSignature: pipe() failed for: {}", modulePath);
        return false;
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, pipefd[0]);

    pid_t pid;
    int spawnRet = posix_spawn(&pid, kGpgBin, &actions, nullptr, argv, environ);
    posix_spawn_file_actions_destroy(&actions);
    close(pipefd[1]);  // parent closes the write end

    if (spawnRet != 0) {
        close(pipefd[0]);
        spdlog::error("verifyGPGSignature: posix_spawn failed for: {}",
                      modulePath);
        return false;
    }

    // Drain the pipe.
    char buf[kGpgReadBufSize];
    std::string output = {};
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        output += buf;
    }
    close(pipefd[0]);

    int wstatus = 0;
    waitpid(pid, &wstatus, 0);
    int exitCode = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : -1;

    if (exitCode == 0 &&
        output.find("Good signature") != std::string::npos) {
        spdlog::info("GPG signature verification PASSED for: {}", modulePath);
        return true;
    }

    spdlog::warn("GPG signature verification FAILED for: {} - {}",
                 modulePath, output);
    return false;
}

// ============================================================================
// Extended attribute (xattr) enumeration
// ============================================================================

std::map<std::string, std::string>
ModuleLoader::getExtendedAttributes(const std::string& modulePath) const {
    std::map<std::string, std::string> result;

    // First, list all attribute names
    ssize_t listSize = listxattr(modulePath.c_str(), nullptr, 0);
    if (listSize <= 0) {
        return result;
    }

    std::string namesBuf(static_cast<size_t>(listSize), '\0');
    listSize = listxattr(modulePath.c_str(), &namesBuf[0],
                         static_cast<size_t>(listSize));
    if (listSize <= 0) {
        return result;
    }

    // Parse null-separated attribute names and read each value
    size_t pos = 0;
    while (pos < static_cast<size_t>(listSize)) {
        std::string name = &namesBuf[pos];
        pos += name.size() + 1;

        ssize_t valueSize =
            getxattr(modulePath.c_str(), name.c_str(), nullptr, 0);
        if (valueSize < 0) {
            continue;
        }
        std::string value(static_cast<size_t>(valueSize), '\0');
        if (getxattr(modulePath.c_str(), name.c_str(), &value[0],
                     static_cast<size_t>(valueSize)) >= 0) {
            result[name] = value;
        }
    }

    return result;
}

// ============================================================================
// ELF metadata extraction (GNU build-ID, .comment section)
// ============================================================================

std::string ModuleLoader::readELFMetadata(const std::string& modulePath) const {
    std::ifstream file(modulePath, std::ios::binary);
    if (!file) {
        spdlog::warn("readELFMetadata: cannot open: {}", modulePath);
        return {};
    }

    // Verify ELF magic number
    unsigned char magic[4];
    file.read(reinterpret_cast<char*>(magic), 4);
    if (file.gcount() < 4 ||
        magic[0] != 0x7f || magic[1] != 'E' ||
        magic[2] != 'L'  || magic[3] != 'F') {
        return {};
    }

    file.seekg(0, std::ios::beg);

    // Read ELF class (32 or 64-bit)
    unsigned char elfClass;
    file.seekg(4);
    file.read(reinterpret_cast<char*>(&elfClass), 1);
    file.seekg(0, std::ios::beg);

    std::string metadata = {};

    auto processNoteSection = [&](uint64_t offset, uint64_t size) {
        file.seekg(static_cast<std::streamoff>(offset));
        uint64_t remaining = size;
        while (remaining >= sizeof(Elf64_Nhdr)) {
            Elf64_Nhdr nhdr = {};
            file.read(reinterpret_cast<char*>(&nhdr), sizeof(nhdr));
            if (file.gcount() < static_cast<std::streamsize>(sizeof(nhdr))) {
              break;
            }
            remaining -= sizeof(nhdr);

            uint64_t nameSize = (nhdr.n_namesz + 3) & ~3u;
            uint64_t descSize = (nhdr.n_descsz + 3) & ~3u;

            if (nameSize > remaining) {
              break;
            }
            uint32_t nameDataLen =
                nhdr.n_namesz > 0 ? nhdr.n_namesz - 1 : 0;
            std::string name(nameDataLen, '\0');
            file.read(&name[0], nameDataLen);
            file.seekg(
                static_cast<std::streamoff>(file.tellg()) +
                static_cast<std::streamoff>(nameSize - nameDataLen));
            remaining -= nameSize;

            if (descSize > remaining) {
              break;
            }
            if (nhdr.n_type == NT_GNU_BUILD_ID && name == "GNU") {
                std::string buildId(nhdr.n_descsz, '\0');
                file.read(&buildId[0],
                          static_cast<std::streamsize>(nhdr.n_descsz));
                // Convert binary build-ID to hex string
                std::string hexId = {};
                hexId.reserve(nhdr.n_descsz * 2);
                static const char kHex[] = "0123456789abcdef";
                for (unsigned char byte :
                     std::string(buildId.begin(), buildId.end())) {
                    hexId += kHex[byte >> 4];
                    hexId += kHex[byte & 0xf];
                }
                if (!metadata.empty()) {
                  metadata += "; ";
                }
                metadata += "BuildID=" + hexId;
                // Skip remaining alignment padding
                uint64_t padded = (nhdr.n_descsz + 3) & ~3u;
                if (padded > nhdr.n_descsz) {
                    file.seekg(
                        static_cast<std::streamoff>(file.tellg()) +
                        static_cast<std::streamoff>(padded - nhdr.n_descsz));
                }
            } else {
                file.seekg(
                    static_cast<std::streamoff>(file.tellg()) +
                    static_cast<std::streamoff>(descSize));
            }
            remaining -= descSize;
        }
    };

    // Template helper that iterates ELF section headers for both 32- and
    // 64-bit ELF files, calling processNoteSection for SHT_NOTE sections and
    // appending .comment section content to `metadata`.
    auto processELFSectionsForMetadata = [&]<typename Ehdr, typename Shdr>(
                                  std::type_identity<Ehdr>,
                                  std::type_identity<Shdr>) {
        Ehdr ehdr = {};
        file.seekg(0);
        file.read(reinterpret_cast<char*>(&ehdr), sizeof(ehdr));

        auto shOffset      = static_cast<uint64_t>(ehdr.e_shoff);
        uint16_t shEntSize = ehdr.e_shentsize;
        uint16_t shNum     = ehdr.e_shnum;
        uint16_t shStrIdx  = ehdr.e_shstrndx;

        if (shOffset == 0 || shNum == 0) {
          return;
        }

        file.seekg(static_cast<std::streamoff>(
            shOffset + static_cast<uint64_t>(shStrIdx) * shEntSize));
        Shdr strShdr = {};
        file.read(reinterpret_cast<char*>(&strShdr), sizeof(strShdr));
        std::string strtab(strShdr.sh_size, '\0');
        file.seekg(static_cast<std::streamoff>(strShdr.sh_offset));
        file.read(&strtab[0], static_cast<std::streamsize>(strShdr.sh_size));

        for (uint16_t i = 0; i < shNum; ++i) {
            file.seekg(static_cast<std::streamoff>(
                shOffset + static_cast<uint64_t>(i) * shEntSize));
            Shdr shdr = {};
            file.read(reinterpret_cast<char*>(&shdr), sizeof(shdr));

            std::string secName = {};
            if (shdr.sh_name < strtab.size()) {
                secName = &strtab[shdr.sh_name];
            }

            if (shdr.sh_type == SHT_NOTE) {
                processNoteSection(shdr.sh_offset, shdr.sh_size);
            } else if (secName == ".comment" &&
                       shdr.sh_size > 0 &&
                       shdr.sh_size < kMaxCommentSectionSize) {
                std::string comment(shdr.sh_size, '\0');
                file.seekg(static_cast<std::streamoff>(shdr.sh_offset));
                file.read(&comment[0],
                          static_cast<std::streamsize>(shdr.sh_size));
                for (char& c : comment) {
                    if (c == '\0') {
                      c = ' ';
                    }
                }
                while (!comment.empty() && comment.back() == ' ') {
                    comment.pop_back();
                }
                if (!comment.empty()) {
                    if (!metadata.empty()) {
                      metadata += "; ";
                    }
                    metadata += "Comment=" + comment;
                }
            }
        }
    };

    if (elfClass == ELFCLASS64) {
        processELFSectionsForMetadata(std::type_identity<Elf64_Ehdr>{},
                                      std::type_identity<Elf64_Shdr>{});
    } else if (elfClass == ELFCLASS32) {
        processELFSectionsForMetadata(std::type_identity<Elf32_Ehdr>{},
                                      std::type_identity<Elf32_Shdr>{});
    }

    return metadata;
}

} // namespace modules
} // namespace themis

#endif // __linux__
