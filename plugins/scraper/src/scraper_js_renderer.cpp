#include "scraper_js_renderer.h"
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <array>
#include <chrono>

#if defined(__unix__) || defined(__APPLE__)
#  include <sys/stat.h>
#  include <unistd.h>
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
    // Check that the first token of the command resolves to an executable.
    const std::size_t sp = renderer_cmd_.find(' ');
    const std::string exe = (sp != std::string::npos)
                          ? renderer_cmd_.substr(0, sp)
                          : renderer_cmd_;
    // If it's an absolute path check directly; otherwise rely on `which`
    if (!exe.empty() && exe.front() == '/') {
        struct stat st{};
        return ::stat(exe.c_str(), &st) == 0 && (st.st_mode & S_IXUSR);
    }
    // Use `which` to locate on PATH
    const std::string cmd = "which " + exe + " > /dev/null 2>&1";
    return std::system(cmd.c_str()) == 0; // NOLINT(cert-env33-c)
#else
    return !renderer_cmd_.empty(); // On Windows, assume available if configured
#endif
}

std::string SubprocessJSRenderer::buildCommand(const JsRenderRequest& req) const {
    std::ostringstream cmd;
    cmd << renderer_cmd_ << " \"" << req.url << "\"";
    if (req.timeout_ms > 0)
        cmd << " --timeout " << req.timeout_ms;
    if (!req.wait_selector.empty())
        cmd << " --wait-for \"" << req.wait_selector << "\"";
    for (const auto& kv : req.headers)
        cmd << " --header \"" << kv.first << ": " << kv.second << "\"";
    for (const auto& arg : req.extra_args)
        cmd << " " << arg;
    return cmd.str();
}

JsRenderResult SubprocessJSRenderer::render(const JsRenderRequest& req) {
    JsRenderResult result;

    if (!isAvailable()) {
        result.success = false;
        result.error   = "JS renderer not available: '" + renderer_cmd_ + "'";
        return result;
    }

    const std::string cmd = buildCommand(req);
    const auto t0 = std::chrono::steady_clock::now();

#if defined(__unix__) || defined(__APPLE__)
    // popen to capture stdout; stderr goes to /dev/null or is captured via 2>&1
    std::string full_cmd = cmd + " 2>/dev/null";
    FILE* pipe = ::popen(full_cmd.c_str(), "r"); // NOLINT(cert-env33-c)
    if (!pipe) {
        result.success = false;
        result.error   = "Failed to launch renderer process";
        return result;
    }

    std::string html;
    std::array<char, 4096> buf{};
    while (::fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        html += buf.data();
    }
    const int exit_code = ::pclose(pipe);

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
    // Windows: use _popen
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) {
        result.success = false;
        result.error   = "Failed to launch renderer process";
        return result;
    }
    std::string html;
    std::array<char, 4096> buf{};
    while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        html += buf.data();
    }
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
