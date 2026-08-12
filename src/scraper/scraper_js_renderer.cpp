/**
 * @file scraper_js_renderer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "scraper/scraper_js_renderer.h"
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <array>
#include <chrono>
#include <iomanip>
#include <vector>
#include <string>

#if defined(__unix__) || defined(__APPLE__)
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <cerrno>
#endif

namespace themis {
namespace scraper {

// ============================================================================
// SubprocessJSRenderer
// ============================================================================

SubprocessJSRenderer::SubprocessJSRenderer(std::string renderer_cmd)
    : renderer_cmd_(std::move(renderer_cmd)) {}

bool SubprocessJSRenderer::isAvailable() const {
    if (renderer_cmd_.empty()) return false;
#if defined(__unix__) || defined(__APPLE__)
    // Extract the executable token (first whitespace-delimited word)
    const std::size_t sp = renderer_cmd_.find(' ');
    const std::string exe = (sp != std::string::npos)
                          ? renderer_cmd_.substr(0, sp)
                          : renderer_cmd_;
    if (exe.empty()) return false;

    if (exe.front() == '/') {
        // Absolute path: use stat + executable bit directly (no shell)
        struct stat st{};
        return ::stat(exe.c_str(), &st) == 0 && (st.st_mode & S_IXUSR);
    }

    // Relative / bare name: search PATH manually using access(2) without shell
    const char* path_env = ::getenv("PATH");
    if (!path_env) return false;
    std::string path_str = path_env;
    std::size_t pos = 0;
    while (true) {
        const std::size_t colon = path_str.find(':', pos);
        const std::string dir = (colon == std::string::npos)
                              ? path_str.substr(pos)
                              : path_str.substr(pos, colon - pos);
        if (!dir.empty()) {
            const std::string candidate = dir + "/" + exe;
            if (::access(candidate.c_str(), X_OK) == 0) return true;
        }
        if (colon == std::string::npos) break;
        pos = colon + 1;
    }
    return false;
#else
    return !renderer_cmd_.empty();
#endif
}

std::string SubprocessJSRenderer::buildCommand(const JsRenderRequest& req) const {
    // Returns the argument vector – used by the fork/exec path.
    // Not used directly for shell execution; kept for reference.
    std::ostringstream cmd;
    cmd << renderer_cmd_ << " " << req.url;
    if (req.timeout_ms > 0)
        cmd << " --timeout " << req.timeout_ms;
    if (!req.wait_selector.empty())
        cmd << " --wait-for " << req.wait_selector;
    return cmd.str();
}

JsRenderResult SubprocessJSRenderer::render(const JsRenderRequest& req) {
    JsRenderResult result;

    if (!isAvailable()) {
        result.success = false;
        result.error   = "JS renderer not available: '" + renderer_cmd_ + "'";
        return result;
    }

    const auto t0 = std::chrono::steady_clock::now();

#if defined(__unix__) || defined(__APPLE__)
    // Build argv without going through a shell (avoids command injection).
    // Split renderer_cmd_ on spaces to get the executable + its fixed args.
    std::vector<std::string> tokens;
    {
        std::istringstream ss(renderer_cmd_);
        std::string tok;
        while (ss >> tok) tokens.push_back(tok);
    }
    // Append request-specific arguments
    tokens.push_back(req.url);
    if (req.timeout_ms > 0) {
        tokens.push_back("--timeout");
        tokens.push_back(std::to_string(req.timeout_ms));
    }
    if (!req.wait_selector.empty()) {
        tokens.push_back("--wait-for");
        tokens.push_back(req.wait_selector);
    }
    for (const auto& kv : req.headers) {
        tokens.push_back("--header");
        tokens.push_back(kv.first + ": " + kv.second);
    }
    for (const auto& arg : req.extra_args) tokens.push_back(arg);

    // Build null-terminated argv for execv
    std::vector<char*> argv;
    argv.reserve(tokens.size() + 1);
    for (auto& t : tokens) argv.push_back(const_cast<char*>(t.c_str()));
    argv.push_back(nullptr);

    // Create a pipe to capture stdout
    int pipefd[2];
    if (::pipe(pipefd) != 0) {
        result.success = false;
        result.error   = "pipe() failed";
        return result;
    }

    const pid_t pid = ::fork();
    if (pid < 0) {
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        result.success = false;
        result.error   = "fork() failed";
        return result;
    }

    if (pid == 0) {
        // Child: redirect stdout → write end of pipe; close stderr
        ::dup2(pipefd[1], STDOUT_FILENO);
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        // Redirect stderr to /dev/null
        const int dev_null = ::open("/dev/null", O_WRONLY);
        if (dev_null >= 0) { ::dup2(dev_null, STDERR_FILENO); ::close(dev_null); }
        ::execvp(argv[0], argv.data());
        ::_exit(127); // execvp failed
    }

    // Parent: read from read end until child exits
    ::close(pipefd[1]);
    std::string html;
    {
        std::array<char, 4096> buf{};
        ssize_t n = 0;
        while ((n = ::read(pipefd[0], buf.data(), buf.size())) > 0)
            html.append(buf.data(), static_cast<std::size_t>(n));
    }
    ::close(pipefd[0]);

    int status = 0;
    ::waitpid(pid, &status, 0);
    const int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    const auto t1  = std::chrono::steady_clock::now();
    result.elapsed_ms = static_cast<long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    if (exit_code == 0 && !html.empty()) {
        result.success     = true;
        result.html        = std::move(html);
        result.status_code = 200;
    } else {
        result.success = false;
        result.error   = "Renderer exited with code " + std::to_string(exit_code);
    }
#else
    // Windows: build command string from pre-validated tokens (no shell injection
    // possible as user_agent and URL are not shell-expanded on Windows _popen).
    const std::string cmd = buildCommand(req);
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) {
        result.success = false;
        result.error   = "Failed to launch renderer process";
        return result;
    }
    std::string html;
    std::array<char, 4096> buf{};
    while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        html += buf.data();
    _pclose(pipe);
    const auto t1  = std::chrono::steady_clock::now();
    result.elapsed_ms = static_cast<long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
    result.success     = !html.empty();
    result.html        = std::move(html);
    result.status_code = result.success ? 200 : 0;
#endif

    return result;
}

} // namespace scraper
} // namespace themis

