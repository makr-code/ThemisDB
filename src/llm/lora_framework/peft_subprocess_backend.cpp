/**
 * @file peft_subprocess_backend.cpp
 * @brief Implementation of PeftSubprocessTrainingBackend (see header).
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "llm/lora_framework/peft_subprocess_backend.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <limits>
#include <random>
#include <sstream>
#include <system_error>
#include <thread>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mutex>
#else
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace themis {
namespace llm {
namespace lora {

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr std::chrono::milliseconds kTick{200};
constexpr std::chrono::seconds kPostExitDrain{2};

/// Shortest decimal that round-trips the float (avoids 3e-4f -> 0.000300000014).
double floatToDouble(float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.9g", static_cast<double>(v));
    for (int prec = 6; prec <= 9; ++prec) {
        char shorter[32];
        std::snprintf(shorter, sizeof(shorter), "%.*g", prec, static_cast<double>(v));
        if (std::strtof(shorter, nullptr) == v) {
            return std::strtod(shorter, nullptr);
        }
    }
    return std::strtod(buf, nullptr);
}

bool isBlank(const std::string& s) {
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c) != 0; });
}

double metricOrNaN(const json& obj, const char* key) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->is_number()) {
        return kNaN;
    }
    return it->get<double>();
}

std::size_t countOrZero(const json& obj, const char* key) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->is_number_integer()) {
        return 0;
    }
    const auto v = it->get<std::int64_t>();
    return v > 0 ? static_cast<std::size_t>(v) : 0;
}

std::string stringOrEmpty(const json& obj, const char* key) {
    auto it = obj.find(key);
    return (it != obj.end() && it->is_string()) ? it->get<std::string>() : std::string{};
}

TrainingJobResult failure(std::string error) {
    TrainingJobResult r;
    r.success = false;
    r.error = std::move(error);
    r.train_loss = kNaN;
    r.eval_loss = kNaN;
    r.eval_perplexity = kNaN;
    r.baseline_eval_loss = kNaN;
    r.rank_pattern = json::object();
    r.raw_report = json::object();
    return r;
}

void appendTail(std::string& tail, std::string_view data, std::size_t cap) {
    tail.append(data.data(), data.size());
    if (tail.size() > cap) {
        tail.erase(0, tail.size() - cap);
    }
}

std::string describeExit(const SubprocessOutcome& o) {
    if (o.term_signal != 0) {
        return "trainer terminated by signal " + std::to_string(o.term_signal);
    }
    return "trainer exited with code " + std::to_string(o.exit_code);
}

std::string withTail(std::string msg, const SubprocessOutcome& o) {
    if (!o.stderr_tail.empty()) {
        msg += "; stderr tail:\n" + o.stderr_tail;
    }
    return msg;
}

bool writeTextFile(const fs::path& path, const std::string& text, std::string* error) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        *error = "cannot open " + path.string() + " for writing";
        return false;
    }
    out << text;
    out.flush();
    if (!out) {
        *error = "write failed: " + path.string();
        return false;
    }
    return true;
}

/// Create (if needed) a directory and restrict it to the owner (0700).
bool makePrivateDir(const fs::path& dir, std::string* error) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        *error = "cannot create directory " + dir.string() + ": " + ec.message();
        return false;
    }
    fs::permissions(dir, fs::perms::owner_all, fs::perm_options::replace, ec);
    if (ec) {
        *error = "cannot restrict permissions of " + dir.string() + ": " + ec.message();
        return false;
    }
    return true;
}

/// Removes a directory tree on scope exit (job inputs contain DB-derived text).
struct ScopedDirRemoval {
    fs::path dir;
    ~ScopedDirRemoval() {
        if (!dir.empty()) {
            std::error_code ec;
            fs::remove_all(dir, ec);
        }
    }
};

std::string randomHex(std::size_t bytes) {
    std::random_device rd;
    static constexpr char kHex[] = "0123456789abcdef";
    std::string s;
    s.reserve(bytes * 2);
    for (std::size_t i = 0; i < bytes; ++i) {
        const auto b = static_cast<unsigned>(rd() & 0xFFu);
        s.push_back(kHex[b >> 4]);
        s.push_back(kHex[b & 0xF]);
    }
    return s;
}

bool isPickleName(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const char* suf : {".bin", ".pt", ".pth", ".pkl", ".pickle", ".ckpt"}) {
        const std::string s(suf);
        if (lower.size() >= s.size() && lower.compare(lower.size() - s.size(), s.size(), s) == 0) {
            return true;
        }
    }
    return false;
}

/// Splits stdout bytes into lines and dispatches protocol messages.
class LineDispatcher {
public:
    LineDispatcher(SubprocessOutcome& outcome,
                   const std::function<void(const ProtocolMessage&)>& on_line,
                   std::size_t tail_cap)
        : outcome_(outcome), on_line_(on_line), tail_cap_(tail_cap) {}

    void feed(std::string_view data) {
        for (char c : data) {
            if (c == '\n') {
                if (!discarding_) {
                    dispatch(buf_);
                }
                buf_.clear();
                discarding_ = false;
                continue;
            }
            if (discarding_) {
                continue;
            }
            if (buf_.size() >= PeftSubprocessTrainingBackend::kMaxProtocolLineBytes) {
                appendTail(outcome_.stderr_tail, "[stdout] oversized line discarded\n", tail_cap_);
                buf_.clear();
                discarding_ = true;
                continue;
            }
            buf_.push_back(c);
        }
    }

    void finish() {
        if (!buf_.empty() && !discarding_) {
            dispatch(buf_);
        }
        buf_.clear();
    }

private:
    void dispatch(const std::string& line) {
        auto msg = PeftSubprocessTrainingBackend::parseProtocolLine(line);
        if (!msg) {
            if (!line.empty()) {
                appendTail(outcome_.stderr_tail, "[stdout] " + line + "\n", tail_cap_);
            }
            return;
        }
        if (msg->kind == ProtocolMessage::Kind::Result) {
            outcome_.result = msg->payload;
        }
        if (on_line_) {
            on_line_(*msg);
        }
    }

    SubprocessOutcome& outcome_;
    const std::function<void(const ProtocolMessage&)>& on_line_;
    std::size_t tail_cap_;
    std::string buf_;
    bool discarding_ = false;
};

#ifndef _WIN32

bool makeCloexecPipe(int fds[2]) {
#if defined(__linux__)
    return ::pipe2(fds, O_CLOEXEC) == 0;
#else
    if (::pipe(fds) != 0) {
        return false;
    }
    for (int i = 0; i < 2; ++i) {
        const int f = ::fcntl(fds[i], F_GETFD);
        if (f < 0 || ::fcntl(fds[i], F_SETFD, f | FD_CLOEXEC) != 0) {
            ::close(fds[0]);
            ::close(fds[1]);
            return false;
        }
    }
    return true;
#endif
}

void setNonBlocking(int fd) {
    const int f = ::fcntl(fd, F_GETFL);
    if (f >= 0) {
        (void)::fcntl(fd, F_SETFL, f | O_NONBLOCK);
    }
}

void closeFd(int& fd) {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

/// Reads what is currently available (bounded so that a chatty child cannot
/// starve the cancel/timeout checks); returns false on EOF/error.
bool drainFd(int fd, const std::function<void(std::string_view)>& sink) {
    char buf[65536];
    for (int rounds = 0; rounds < 64; ++rounds) {
        const ssize_t n = ::read(fd, buf, sizeof(buf));
        if (n > 0) {
            sink(std::string_view(buf, static_cast<std::size_t>(n)));
            continue;
        }
        if (n == 0) {
            return false;
        }
        if (errno == EINTR) {
            continue;
        }
        return errno == EAGAIN || errno == EWOULDBLOCK;
    }
}

SubprocessOutcome runPosix(const std::vector<std::string>& args,
                           const std::function<void(const ProtocolMessage&)>& on_line,
                           const std::atomic<bool>& cancel, std::chrono::seconds timeout,
                           std::chrono::seconds kill_grace, std::size_t tail_cap) {
    SubprocessOutcome outcome;
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const auto& a : args) {
        argv.push_back(const_cast<char*>(a.c_str()));
    }
    argv.push_back(nullptr);

    int out_pipe[2] = {-1, -1};
    int err_pipe[2] = {-1, -1};
    int exec_pipe[2] = {-1, -1};
    if (!makeCloexecPipe(out_pipe)) {
        outcome.launch_error = std::string("pipe() failed: ") + std::strerror(errno);
        return outcome;
    }
    if (!makeCloexecPipe(err_pipe)) {
        outcome.launch_error = std::string("pipe() failed: ") + std::strerror(errno);
        closeFd(out_pipe[0]);
        closeFd(out_pipe[1]);
        return outcome;
    }
    if (!makeCloexecPipe(exec_pipe)) {
        outcome.launch_error = std::string("pipe() failed: ") + std::strerror(errno);
        for (int* p : {out_pipe, err_pipe}) {
            closeFd(p[0]);
            closeFd(p[1]);
        }
        return outcome;
    }

    const pid_t pid = ::fork();
    if (pid < 0) {
        outcome.launch_error = std::string("fork() failed: ") + std::strerror(errno);
        for (int* p : {out_pipe, err_pipe, exec_pipe}) {
            closeFd(p[0]);
            closeFd(p[1]);
        }
        return outcome;
    }
    if (pid == 0) {
        // Child: only async-signal-safe calls until execvp.
        (void)::setpgid(0, 0);
        const int devnull = ::open("/dev/null", O_RDONLY);
        if (devnull >= 0) {
            (void)::dup2(devnull, STDIN_FILENO);
        }
        (void)::dup2(out_pipe[1], STDOUT_FILENO);
        (void)::dup2(err_pipe[1], STDERR_FILENO);
        ::execvp(argv[0], argv.data());
        const int e = errno;
        (void)!::write(exec_pipe[1], &e, sizeof(e));
        ::_exit(127);
    }

    // Parent.
    (void)::setpgid(pid, pid);  // closes the race with the child's own setpgid
    closeFd(out_pipe[1]);
    closeFd(err_pipe[1]);
    closeFd(exec_pipe[1]);

    int exec_errno = 0;
    ssize_t got = 0;
    do {
        got = ::read(exec_pipe[0], &exec_errno, sizeof(exec_errno));
    } while (got < 0 && errno == EINTR);
    closeFd(exec_pipe[0]);
    if (got == static_cast<ssize_t>(sizeof(exec_errno))) {
        int st = 0;
        while (::waitpid(pid, &st, 0) < 0 && errno == EINTR) {
        }
        closeFd(out_pipe[0]);
        closeFd(err_pipe[0]);
        outcome.launch_error = "cannot execute '" + args.front() + "': " + std::strerror(exec_errno);
        return outcome;
    }
    outcome.launched = true;

    int out_fd = out_pipe[0];
    int err_fd = err_pipe[0];
    setNonBlocking(out_fd);
    setNonBlocking(err_fd);

    LineDispatcher dispatcher(outcome, on_line, tail_cap);
    const auto out_sink = [&](std::string_view d) { dispatcher.feed(d); };
    const auto err_sink = [&](std::string_view d) { appendTail(outcome.stderr_tail, d, tail_cap); };

    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + timeout;
    bool term_sent = false;
    std::optional<clock::time_point> kill_at;
    bool reaped = false;
    clock::time_point reaped_at{};
    int status = 0;

    for (;;) {
        pollfd fds[2];
        nfds_t nfds = 0;
        if (out_fd >= 0) {
            fds[nfds++] = pollfd{out_fd, POLLIN, 0};
        }
        if (err_fd >= 0) {
            fds[nfds++] = pollfd{err_fd, POLLIN, 0};
        }
        if (nfds > 0) {
            const int pr = ::poll(fds, nfds, static_cast<int>(kTick.count()));
            if (pr < 0 && errno != EINTR) {
                appendTail(outcome.stderr_tail, "[backend] poll() failed\n", tail_cap);
            }
            for (nfds_t i = 0; pr > 0 && i < nfds; ++i) {
                if ((fds[i].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) == 0) {
                    continue;
                }
                const bool is_out = fds[i].fd == out_fd;
                const bool open = drainFd(fds[i].fd, is_out ? std::function<void(std::string_view)>(out_sink)
                                                            : std::function<void(std::string_view)>(err_sink));
                if (!open || (fds[i].revents & POLLNVAL) != 0) {
                    closeFd(is_out ? out_fd : err_fd);
                }
            }
        } else {
            std::this_thread::sleep_for(kTick);
        }

        const auto now = clock::now();
        if (!reaped) {
            const pid_t r = ::waitpid(pid, &status, WNOHANG);
            if (r == pid) {
                reaped = true;
                reaped_at = now;
            } else if (r < 0 && errno != EINTR) {
                // ECHILD: someone else reaped it (e.g. SIGCHLD=SIG_IGN); exit status unknown.
                reaped = true;
                reaped_at = now;
                status = -1;
            }
        }
        if (reaped) {
            if ((out_fd < 0 && err_fd < 0) || now - reaped_at >= kPostExitDrain) {
                break;
            }
            continue;
        }
        if (!term_sent && (cancel.load(std::memory_order_relaxed) || now >= deadline)) {
            if (cancel.load(std::memory_order_relaxed)) {
                outcome.cancelled = true;
            } else {
                outcome.timed_out = true;
            }
            (void)::kill(-pid, SIGTERM);
            term_sent = true;
            kill_at = now + kill_grace;
        } else if (kill_at && now >= *kill_at) {
            (void)::kill(-pid, SIGKILL);
            kill_at.reset();
        }
    }

    // Descendants still holding the pipes keep the process group (and thus the
    // pgid) alive, so signalling the group cannot hit an unrelated process.
    if (out_fd >= 0 || err_fd >= 0) {
        (void)::kill(-pid, SIGKILL);
    }
    closeFd(out_fd);
    closeFd(err_fd);
    dispatcher.finish();

    if (status != -1 && WIFEXITED(status)) {
        outcome.exit_code = WEXITSTATUS(status);
    } else if (status != -1 && WIFSIGNALED(status)) {
        outcome.term_signal = WTERMSIG(status);
    }
    return outcome;
}

#else  // _WIN32

std::wstring widen(const std::string& s) {
    if (s.empty()) {
        return {};
    }
    const int n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<std::size_t>(n), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

/// Quote one argument following the CommandLineToArgvW / MSVCRT rules.
std::wstring quoteArg(const std::wstring& arg) {
    if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        return arg;
    }
    std::wstring out = L"\"";
    for (auto it = arg.begin();; ++it) {
        std::size_t backslashes = 0;
        while (it != arg.end() && *it == L'\\') {
            ++it;
            ++backslashes;
        }
        if (it == arg.end()) {
            out.append(backslashes * 2, L'\\');
            break;
        }
        if (*it == L'"') {
            out.append(backslashes * 2 + 1, L'\\');
        } else {
            out.append(backslashes, L'\\');
        }
        out.push_back(*it);
    }
    out.push_back(L'"');
    return out;
}

std::string lastErrorText(const char* what) {
    return std::string(what) + " failed (Win32 error " + std::to_string(::GetLastError()) + ")";
}

SubprocessOutcome runWindows(const std::vector<std::string>& args,
                             const std::function<void(const ProtocolMessage&)>& on_line,
                             const std::atomic<bool>& cancel, std::chrono::seconds timeout,
                             std::chrono::seconds /*kill_grace*/, std::size_t tail_cap) {
    SubprocessOutcome outcome;
    std::wstring cmdline;
    for (const auto& a : args) {
        if (!cmdline.empty()) {
            cmdline.push_back(L' ');
        }
        cmdline += quoteArg(widen(a));
    }

    SECURITY_ATTRIBUTES sa{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE out_r = nullptr, out_w = nullptr, err_r = nullptr, err_w = nullptr;
    if (!::CreatePipe(&out_r, &out_w, &sa, 0) || !::CreatePipe(&err_r, &err_w, &sa, 0)) {
        outcome.launch_error = lastErrorText("CreatePipe");
        for (HANDLE h : {out_r, out_w, err_r, err_w}) {
            if (h) ::CloseHandle(h);
        }
        return outcome;
    }
    ::SetHandleInformation(out_r, HANDLE_FLAG_INHERIT, 0);
    ::SetHandleInformation(err_r, HANDLE_FLAG_INHERIT, 0);
    HANDLE nul = ::CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                               OPEN_EXISTING, 0, nullptr);

    HANDLE job = ::CreateJobObjectW(nullptr, nullptr);
    if (job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        ::SetInformationJobObject(job, JobObjectExtendedLimitInformation, &info, sizeof(info));
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = nul != INVALID_HANDLE_VALUE ? nul : nullptr;
    si.hStdOutput = out_w;
    si.hStdError = err_w;
    PROCESS_INFORMATION pi{};
    const BOOL ok = ::CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, TRUE,
                                     CREATE_SUSPENDED | CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    ::CloseHandle(out_w);
    ::CloseHandle(err_w);
    if (nul != INVALID_HANDLE_VALUE) ::CloseHandle(nul);
    if (!ok) {
        outcome.launch_error = "cannot execute '" + args.front() + "': " + lastErrorText("CreateProcessW");
        ::CloseHandle(out_r);
        ::CloseHandle(err_r);
        if (job) ::CloseHandle(job);
        return outcome;
    }
    if (job) {
        ::AssignProcessToJobObject(job, pi.hProcess);
    }
    ::ResumeThread(pi.hThread);
    ::CloseHandle(pi.hThread);
    outcome.launched = true;

    std::mutex mu;
    std::string out_bytes;
    std::string err_bytes;
    auto reader = [&mu](HANDLE h, std::string* sink) {
        char buf[65536];
        DWORD n = 0;
        while (::ReadFile(h, buf, sizeof(buf), &n, nullptr) && n > 0) {
            std::lock_guard<std::mutex> lk(mu);
            sink->append(buf, n);
        }
    };
    std::thread t_out(reader, out_r, &out_bytes);
    std::thread t_err(reader, err_r, &err_bytes);

    LineDispatcher dispatcher(outcome, on_line, tail_cap);
    auto pump = [&]() {
        std::string o, e;
        {
            std::lock_guard<std::mutex> lk(mu);
            o.swap(out_bytes);
            e.swap(err_bytes);
        }
        if (!e.empty()) appendTail(outcome.stderr_tail, e, tail_cap);
        if (!o.empty()) dispatcher.feed(o);
    };

    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + timeout;
    bool terminated = false;
    for (;;) {
        const DWORD w = ::WaitForSingleObject(pi.hProcess, static_cast<DWORD>(kTick.count()));
        pump();
        if (w == WAIT_OBJECT_0) {
            break;
        }
        if (!terminated && (cancel.load(std::memory_order_relaxed) || clock::now() >= deadline)) {
            if (cancel.load(std::memory_order_relaxed)) {
                outcome.cancelled = true;
            } else {
                outcome.timed_out = true;
            }
            // Windows has no SIGTERM equivalent for console-less children: terminate the job tree.
            if (job) {
                ::TerminateJobObject(job, 1);
            } else {
                ::TerminateProcess(pi.hProcess, 1);
            }
            terminated = true;
        }
    }
    // Kill leftover descendants so the pipe readers see EOF.
    if (job) {
        ::TerminateJobObject(job, 1);
    }
    t_out.join();
    t_err.join();
    pump();
    dispatcher.finish();
    DWORD code = 0;
    if (::GetExitCodeProcess(pi.hProcess, &code)) {
        outcome.exit_code = static_cast<int>(code);
    }
    ::CloseHandle(pi.hProcess);
    ::CloseHandle(out_r);
    ::CloseHandle(err_r);
    if (job) ::CloseHandle(job);
    return outcome;
}

#endif  // _WIN32

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────

PeftSubprocessTrainingBackend::PeftSubprocessTrainingBackend(PeftSubprocessBackendConfig config)
    : config_(std::move(config)) {}

std::string PeftSubprocessTrainingBackend::name() const {
    return "peft-subprocess";
}

std::optional<ProtocolMessage> PeftSubprocessTrainingBackend::parseProtocolLine(std::string_view line) {
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.remove_suffix(1);
    }
    ProtocolMessage msg;
    std::string_view body;
    if (line.substr(0, kProgressPrefix.size()) == kProgressPrefix) {
        msg.kind = ProtocolMessage::Kind::Progress;
        body = line.substr(kProgressPrefix.size());
    } else if (line.substr(0, kResultPrefix.size()) == kResultPrefix) {
        msg.kind = ProtocolMessage::Kind::Result;
        body = line.substr(kResultPrefix.size());
    } else {
        return std::nullopt;
    }
    json payload = json::parse(body.begin(), body.end(), nullptr, /*allow_exceptions=*/false);
    if (payload.is_discarded() || !payload.is_object()) {
        return std::nullopt;
    }
    msg.payload = std::move(payload);
    return msg;
}

TrainingProgress PeftSubprocessTrainingBackend::progressFromJson(const json& p) {
    TrainingProgress tp;
    const double progress = metricOrNaN(p, "progress");
    tp.progress = std::isfinite(progress) ? static_cast<float>(std::clamp(progress, 0.0, 1.0)) : 0.0f;
    auto intOf = [&](const char* key) {
        auto it = p.find(key);
        if (it == p.end() || !it->is_number()) {
            return 0;
        }
        const double v = it->get<double>();
        if (!std::isfinite(v) || v < 0) {
            return 0;
        }
        return static_cast<int>(std::min(v, static_cast<double>(std::numeric_limits<int>::max())));
    };
    tp.step = intOf("step");
    tp.total_steps = intOf("total_steps");
    tp.train_loss = metricOrNaN(p, "train_loss");
    const double eval = metricOrNaN(p, "eval_loss");
    tp.eval_loss = std::isfinite(eval) ? eval : -1.0;
    tp.phase = stringOrEmpty(p, "phase");
    return tp;
}

TrainingJobResult PeftSubprocessTrainingBackend::resultFromJson(const json& p) {
    TrainingJobResult r;
    auto it = p.find("success");
    r.success = it != p.end() && it->is_boolean() && it->get<bool>();
    r.error = stringOrEmpty(p, "error");
    r.gguf_adapter_path = stringOrEmpty(p, "gguf_adapter_path");
    r.peft_adapter_dir = stringOrEmpty(p, "peft_adapter_dir");
    r.train_loss = metricOrNaN(p, "train_loss");
    r.eval_loss = metricOrNaN(p, "eval_loss");
    r.eval_perplexity = metricOrNaN(p, "eval_perplexity");
    r.baseline_eval_loss = metricOrNaN(p, "baseline_eval_loss");
    r.train_samples = countOrZero(p, "train_samples");
    r.eval_samples = countOrZero(p, "eval_samples");
    auto rp = p.find("rank_pattern");
    r.rank_pattern = (rp != p.end() && rp->is_object()) ? *rp : json::object();
    r.raw_report = p;
    if (!r.success && r.error.empty()) {
        r.error = "trainer reported failure without an error message";
    }
    return r;
}

bool PeftSubprocessTrainingBackend::isValidAdapterId(std::string_view id) noexcept {
    if (id.empty() || id.size() > 128) {
        return false;
    }
    return std::all_of(id.begin(), id.end(), [](char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
               c == '_' || c == '.' || c == '-';
    });
}

bool PeftSubprocessTrainingBackend::isWithinRoot(const fs::path& candidate, const fs::path& root) {
    if (candidate.empty() || root.empty()) {
        return false;
    }
    std::error_code ec;
    const fs::path c = fs::weakly_canonical(fs::absolute(candidate, ec), ec);
    if (ec) {
        return false;
    }
    const fs::path r = fs::weakly_canonical(fs::absolute(root, ec), ec);
    if (ec) {
        return false;
    }
    const fs::path rel = c.lexically_relative(r);
    if (rel.empty() || rel == ".") {
        return false;
    }
    const auto first = *rel.begin();
    return first != ".." && first != ".";
}

std::string PeftSubprocessTrainingBackend::validateRequest(const TrainingJobRequest& req,
                                                           const PeftSubprocessBackendConfig& cfg) {
    if (!isValidAdapterId(req.adapter_id)) {
        return "invalid adapter_id '" + req.adapter_id + "' (expected [A-Za-z0-9_.-]{1,128})";
    }
    const auto& samples = req.data.samples;
    if (samples.size() < 2) {
        return "at least 2 training samples are required (got " + std::to_string(samples.size()) + ")";
    }
    for (std::size_t i = 0; i < samples.size(); ++i) {
        if (isBlank(samples[i].input) || isBlank(samples[i].output)) {
            return "sample " + std::to_string(i) + " has an empty input or output";
        }
    }
    if (!std::isfinite(req.holdout_fraction) || req.holdout_fraction <= 0.0 || req.holdout_fraction > 0.5) {
        return "holdout_fraction must be in (0, 0.5]";
    }
    const auto& hp = req.hyperparameters;
    if (hp.rank <= 0) return "rank must be > 0";
    if (!(std::isfinite(hp.alpha) && hp.alpha > 0.0f)) return "alpha must be > 0";
    if (!(hp.dropout >= 0.0f && hp.dropout < 1.0f)) return "dropout must be in [0, 1)";
    if (!(std::isfinite(hp.learning_rate) && hp.learning_rate > 0.0f)) return "learning_rate must be > 0";
    if (hp.num_epochs <= 0) return "num_epochs must be > 0";
    if (hp.batch_size <= 0) return "batch_size must be > 0";
    if (hp.max_seq_length < 8) return "max_seq_length must be >= 8";
    if (!(std::isfinite(hp.weight_decay) && hp.weight_decay >= 0.0f)) return "weight_decay must be >= 0";
    for (const auto& m : hp.target_modules) {
        if (m.empty()) return "target_modules must not contain empty names";
    }
    if (req.method == AdapterTrainingMethod::AdaLoRA) {
        const auto& a = req.adalora;
        if (a.init_rank <= 0 || a.target_rank <= 0) return "AdaLoRA ranks must be > 0";
        if (a.target_rank > a.init_rank) return "AdaLoRA target_rank must be <= init_rank";
        if (a.warmup_steps < 0 || a.final_steps < 0) return "AdaLoRA warmup_steps/final_steps must be >= 0";
        if (a.delta_t <= 0) return "AdaLoRA delta_t must be > 0";
        if (!(a.beta1 > 0.0 && a.beta1 < 1.0 && a.beta2 > 0.0 && a.beta2 < 1.0))
            return "AdaLoRA beta1/beta2 must be in (0, 1)";
        if (!(std::isfinite(a.orth_reg_weight) && a.orth_reg_weight >= 0.0))
            return "AdaLoRA orth_reg_weight must be >= 0";
    }
    if (req.base_model.empty() && cfg.base_model.empty()) {
        return "base_model is required (request or backend config)";
    }
    if (cfg.work_root.empty()) {
        return "backend work_root is not configured";
    }
    if (req.output_dir.empty()) {
        return "output_dir is required";
    }
    if (!isWithinRoot(req.output_dir, cfg.work_root)) {
        return "output_dir '" + req.output_dir + "' is not inside work_root '" + cfg.work_root + "'";
    }
    if (cfg.python_executable.empty()) {
        return "python_executable is not configured";
    }
    std::error_code ec;
    if (cfg.script_path.empty() || !fs::is_regular_file(cfg.script_path, ec)) {
        return "trainer script not found: '" + cfg.script_path + "'";
    }
    if (cfg.convert_script_path.empty() || !fs::is_regular_file(cfg.convert_script_path, ec)) {
        return "llama.cpp convert_lora_to_gguf.py not found: '" + cfg.convert_script_path + "'";
    }
    return {};
}

TrainEvalSplit PeftSubprocessTrainingBackend::splitTrainEval(std::size_t n, double holdout,
                                                            std::uint64_t seed) {
    TrainEvalSplit split;
    if (n < 2 || !(holdout > 0.0 && holdout <= 0.5)) {
        return split;
    }
    std::vector<std::size_t> idx(n);
    for (std::size_t i = 0; i < n; ++i) {
        idx[i] = i;
    }
    std::mt19937_64 rng(seed);
    // Unbiased bounded draw in [0, bound) by rejection (portable, unlike
    // std::uniform_int_distribution whose algorithm is implementation-defined).
    auto draw = [&rng](std::uint64_t bound) {
        const std::uint64_t limit = std::numeric_limits<std::uint64_t>::max() -
                                    (std::numeric_limits<std::uint64_t>::max() % bound);
        std::uint64_t v = 0;
        do {
            v = rng();
        } while (v >= limit);
        return v % bound;
    };
    for (std::size_t i = n - 1; i > 0; --i) {
        const auto j = static_cast<std::size_t>(draw(static_cast<std::uint64_t>(i) + 1));
        std::swap(idx[i], idx[j]);
    }
    auto n_eval = static_cast<std::size_t>(std::llround(static_cast<double>(n) * holdout));
    n_eval = std::clamp<std::size_t>(n_eval, 1, n - 1);
    split.eval.assign(idx.begin(), idx.begin() + static_cast<std::ptrdiff_t>(n_eval));
    split.train.assign(idx.begin() + static_cast<std::ptrdiff_t>(n_eval), idx.end());
    std::sort(split.eval.begin(), split.eval.end());
    std::sort(split.train.begin(), split.train.end());
    return split;
}

json PeftSubprocessTrainingBackend::sampleToJson(const TrainingDataSample& s) {
    json j = {{"input", s.input}, {"output", s.output}};
    if (s.metadata.is_object()) {
        auto ctx = s.metadata.find("context");
        if (ctx != s.metadata.end() && ctx->is_array()) {
            json docs = json::array();
            bool all_strings = true;
            for (const auto& d : *ctx) {
                if (!d.is_string()) {
                    all_strings = false;
                    break;
                }
                docs.push_back(d);
            }
            if (all_strings && !docs.empty()) {
                j["context"] = std::move(docs);
            }
        }
        auto sys = s.metadata.find("system");
        if (sys != s.metadata.end() && sys->is_string() && !sys->get<std::string>().empty()) {
            j["system"] = *sys;
        }
    }
    return j;
}

bool PeftSubprocessTrainingBackend::writeJsonl(const fs::path& path,
                                               const std::vector<TrainingDataSample>& samples,
                                               const std::vector<std::size_t>& indices,
                                               std::string* error) {
    std::string local_error;
    std::string& err = error ? *error : local_error;
    std::string text;
    for (const std::size_t i : indices) {
        if (i >= samples.size()) {
            err = "sample index " + std::to_string(i) + " out of range";
            return false;
        }
        try {
            text += sampleToJson(samples[i]).dump(-1, ' ', false, json::error_handler_t::strict);
        } catch (const json::exception& e) {
            err = "sample " + std::to_string(i) + " is not valid UTF-8: " + e.what();
            return false;
        }
        text.push_back('\n');
    }
    return writeTextFile(path, text, &err);
}

json PeftSubprocessTrainingBackend::buildJobJson(const TrainingJobRequest& req,
                                                 const PeftSubprocessBackendConfig& cfg,
                                                 const fs::path& train_file, const fs::path& eval_file,
                                                 const fs::path& output_dir) {
    const auto& hp = req.hyperparameters;
    const json& md = req.data.metadata;
    auto mdString = [&md](const char* key) -> std::string {
        if (!md.is_object()) return {};
        auto it = md.find(key);
        return (it != md.end() && it->is_string()) ? it->get<std::string>() : std::string{};
    };

    json themis_md = json::object();
    if (md.is_object()) {
        for (auto it = md.begin(); it != md.end(); ++it) {
            const auto& v = it.value();
            if (it.key().rfind("themis.", 0) == 0 && (v.is_string() || v.is_number() || v.is_boolean())) {
                themis_md[it.key()] = v;
            }
        }
    }
    std::string dataset_id = mdString("themis.training.dataset_id");
    if (dataset_id.empty()) {
        dataset_id = req.data.dataset_name;
    }

    json job = {
        {"job_id", req.job_id},
        {"adapter_id", req.adapter_id},
        {"adapter_version", mdString("themis.adapter.version")},
        {"dataset_id", dataset_id},
        {"train_file", train_file.string()},
        {"eval_file", eval_file.string()},
        {"base_model", req.base_model.empty() ? cfg.base_model : req.base_model},
        {"base_model_gguf", req.base_model_gguf.empty() ? cfg.base_model_gguf : req.base_model_gguf},
        {"base_model_gguf_sha256", mdString("themis.base_model.sha256")},
        {"output_dir", output_dir.string()},
        {"method", req.method == AdapterTrainingMethod::AdaLoRA ? "adalora" : "lora"},
        {"rank", hp.rank},
        {"alpha", floatToDouble(hp.alpha)},
        {"dropout", floatToDouble(hp.dropout)},
        {"learning_rate", floatToDouble(hp.learning_rate)},
        {"weight_decay", floatToDouble(hp.weight_decay)},
        {"num_epochs", hp.num_epochs},
        {"batch_size", hp.batch_size},
        {"max_seq_length", hp.max_seq_length},
        {"seed", req.seed},
        {"convert_script", cfg.convert_script_path},
        {"gguf_outtype", cfg.gguf_outtype},
        {"require_improvement", req.require_improvement},
        {"adalora",
         {{"init_r", req.adalora.init_rank},
          {"target_r", req.adalora.target_rank},
          {"tinit", req.adalora.warmup_steps},
          {"tfinal", req.adalora.final_steps},
          {"deltaT", req.adalora.delta_t},
          {"beta1", req.adalora.beta1},
          {"beta2", req.adalora.beta2},
          {"orth_reg_weight", req.adalora.orth_reg_weight}}},
        {"themis_metadata", std::move(themis_md)},
    };
    if (!hp.target_modules.empty()) {
        job["target_modules"] = hp.target_modules;
    }
    return job;
}

std::vector<std::string> PeftSubprocessTrainingBackend::buildArgv(const PeftSubprocessBackendConfig& cfg,
                                                                  const fs::path& config_file,
                                                                  bool convert_only) {
    std::vector<std::string> argv = {cfg.python_executable, cfg.script_path, "--config", config_file.string()};
    if (convert_only) {
        argv.emplace_back("--convert-only");
    }
    return argv;
}

SubprocessOutcome PeftSubprocessTrainingBackend::runProtocolProcess(
    const std::vector<std::string>& argv, const std::function<void(const ProtocolMessage&)>& on_line,
    const std::atomic<bool>& cancel, std::chrono::seconds timeout, std::chrono::seconds kill_grace,
    std::size_t stderr_tail_bytes) {
    if (argv.empty() || argv.front().empty()) {
        SubprocessOutcome o;
        o.launch_error = "empty argv";
        return o;
    }
    const std::size_t cap = std::max<std::size_t>(stderr_tail_bytes, 256);
#ifdef _WIN32
    return runWindows(argv, on_line, cancel, timeout, kill_grace, cap);
#else
    return runPosix(argv, on_line, cancel, timeout, kill_grace, cap);
#endif
}

TrainingJobResult PeftSubprocessTrainingBackend::train(const TrainingJobRequest& request,
                                                       const ProgressCallback& on_progress,
                                                       const std::atomic<bool>& cancel) {
    try {
        return trainImpl(request, on_progress, cancel);
    } catch (const std::exception& e) {
        return failure(std::string("training backend error: ") + e.what());
    }
}

TrainingJobResult PeftSubprocessTrainingBackend::trainImpl(const TrainingJobRequest& request,
                                                           const ProgressCallback& on_progress,
                                                           const std::atomic<bool>& cancel) {
    if (std::string err = validateRequest(request, config_); !err.empty()) {
        return failure("invalid training request: " + err);
    }
    std::string err;
    const fs::path output_dir = fs::weakly_canonical(fs::absolute(request.output_dir));
    if (!makePrivateDir(output_dir, &err)) {
        return failure(err);
    }
    const fs::path input_dir = output_dir / "input";
    {
        std::error_code ec;
        fs::remove_all(input_dir, ec);
    }
    if (!makePrivateDir(input_dir, &err)) {
        return failure(err);
    }
    ScopedDirRemoval cleanup{input_dir};

    const TrainEvalSplit split =
        splitTrainEval(request.data.samples.size(), request.holdout_fraction, request.seed);
    const fs::path train_file = input_dir / "train.jsonl";
    const fs::path eval_file = input_dir / "eval.jsonl";
    const fs::path job_file = input_dir / "job.json";
    if (!writeJsonl(train_file, request.data.samples, split.train, &err) ||
        !writeJsonl(eval_file, request.data.samples, split.eval, &err)) {
        return failure("cannot write training data: " + err);
    }
    const json job = buildJobJson(request, config_, train_file, eval_file, output_dir);
    std::string job_text;
    try {
        job_text = job.dump(2, ' ', false, json::error_handler_t::strict);
    } catch (const json::exception& e) {
        return failure(std::string("job configuration is not valid UTF-8: ") + e.what());
    }
    if (!writeTextFile(job_file, job_text, &err)) {
        return failure(err);
    }

    const auto on_line = [&on_progress](const ProtocolMessage& m) {
        if (m.kind != ProtocolMessage::Kind::Progress || !on_progress) {
            return;
        }
        try {
            on_progress(progressFromJson(m.payload));
        } catch (...) {
            // A failing observer must not abort or corrupt the training run.
        }
    };
    const SubprocessOutcome out = runProtocolProcess(buildArgv(config_, job_file, false), on_line, cancel,
                                                     config_.timeout, config_.kill_grace,
                                                     config_.stderr_tail_bytes);
    if (!out.launched) {
        return failure("cannot start trainer: " + out.launch_error);
    }
    if (out.cancelled) {
        return failure("training cancelled");
    }
    if (out.timed_out) {
        return failure(withTail("training timed out after " + std::to_string(config_.timeout.count()) + " s",
                                out));
    }
    if (!out.result) {
        return failure(withTail(describeExit(out) + " without a THEMIS_RESULT line", out));
    }

    TrainingJobResult res = resultFromJson(*out.result);
    res.raw_report["backend"] = name();
    res.raw_report["exit_code"] = out.exit_code;
    res.raw_report["train_split_size"] = split.train.size();
    res.raw_report["eval_split_size"] = split.eval.size();
    if (out.exit_code != 0 || out.term_signal != 0 || !res.success) {
        res.success = false;
        std::string msg = res.error.empty() ? describeExit(out) : res.error;
        // Exit 2 (config/data) and 3 (no improvement) carry precise messages.
        if (out.exit_code != 2 && out.exit_code != 3) {
            msg = withTail(std::move(msg), out);
        }
        res.error = std::move(msg);
        return res;
    }

    std::error_code ec;
    const std::string st_path = stringOrEmpty(res.raw_report, "safetensors_path");
    if (res.gguf_adapter_path.empty() || !fs::is_regular_file(res.gguf_adapter_path, ec) ||
        !isWithinRoot(res.gguf_adapter_path, output_dir)) {
        res.success = false;
        res.error = "trainer reported success but the GGUF adapter is missing or outside output_dir: '" +
                    res.gguf_adapter_path + "'";
        return res;
    }
    if (st_path.empty() || !fs::is_regular_file(st_path, ec) || !isWithinRoot(st_path, output_dir)) {
        res.success = false;
        res.error = "trainer reported success but the safetensors package is missing or outside output_dir: '" +
                    st_path + "'";
        return res;
    }
    res.error.clear();
    return res;
}

TrainingJobResult PeftSubprocessTrainingBackend::convertSafetensorsToGguf(const std::string& peft_package_dir,
                                                                          const json& themis_metadata,
                                                                          const std::string& output_gguf_path,
                                                                          const std::atomic<bool>& cancel) {
    try {
        return convertImpl(peft_package_dir, themis_metadata, output_gguf_path, cancel);
    } catch (const std::exception& e) {
        return failure(std::string("conversion backend error: ") + e.what());
    }
}

TrainingJobResult PeftSubprocessTrainingBackend::convertImpl(const std::string& peft_package_dir,
                                                             const json& themis_metadata,
                                                             const std::string& output_gguf_path,
                                                             const std::atomic<bool>& cancel) {
    std::error_code ec;
    if (peft_package_dir.empty() || !fs::is_directory(peft_package_dir, ec)) {
        return failure("package directory not found: '" + peft_package_dir + "'");
    }
    const fs::path pkg = fs::weakly_canonical(fs::absolute(peft_package_dir));
    if (!fs::is_regular_file(pkg / "adapter_model.safetensors", ec)) {
        return failure("package lacks adapter_model.safetensors: '" + pkg.string() + "'");
    }
    if (!fs::is_regular_file(pkg / "adapter_config.json", ec)) {
        return failure("package lacks adapter_config.json: '" + pkg.string() + "'");
    }
    for (const auto& entry : fs::directory_iterator(pkg, ec)) {
        if (isPickleName(entry.path().filename().string())) {
            return failure("pickle checkpoint '" + entry.path().filename().string() +
                           "' is not accepted; use safetensors");
        }
    }
    if (!themis_metadata.is_null() && !themis_metadata.is_object()) {
        return failure("themis_metadata must be a JSON object");
    }
    json meta = json::object();
    if (themis_metadata.is_object()) {
        for (auto it = themis_metadata.begin(); it != themis_metadata.end(); ++it) {
            const auto& v = it.value();
            if (it.key().rfind("themis.", 0) != 0) {
                return failure("metadata key '" + it.key() + "' must start with 'themis.'");
            }
            if (!(v.is_string() || v.is_number() || v.is_boolean())) {
                return failure("metadata value of '" + it.key() + "' must be a string, number or bool");
            }
            meta[it.key()] = v;
        }
    }
    if (output_gguf_path.empty() || fs::path(output_gguf_path).extension() != ".gguf") {
        return failure("output path must end with .gguf: '" + output_gguf_path + "'");
    }
    const fs::path out_gguf = fs::absolute(output_gguf_path);
    if (config_.python_executable.empty() || config_.script_path.empty() ||
        !fs::is_regular_file(config_.script_path, ec)) {
        return failure("trainer script not found: '" + config_.script_path + "'");
    }
    if (config_.convert_script_path.empty() || !fs::is_regular_file(config_.convert_script_path, ec)) {
        return failure("llama.cpp convert_lora_to_gguf.py not found: '" + config_.convert_script_path + "'");
    }
    std::string base_model = config_.base_model;
    if (base_model.empty() && meta.contains("themis.base_model.name") && meta["themis.base_model.name"].is_string()) {
        base_model = meta["themis.base_model.name"].get<std::string>();
    }
    if (base_model.empty()) {
        return failure("base model (HF directory or id) unknown: configure base_model");
    }

    std::string err;
    const fs::path scratch_root =
        config_.work_root.empty() ? fs::temp_directory_path(ec) : fs::path(config_.work_root);
    if (ec) {
        return failure("no scratch directory available: " + ec.message());
    }
    fs::path scratch;
    for (int attempt = 0; attempt < 8 && scratch.empty(); ++attempt) {
        const fs::path candidate = scratch_root / (".themis-convert-" + randomHex(8));
        fs::create_directories(scratch_root, ec);
        if (fs::create_directory(candidate, ec) && !ec) {
            scratch = candidate;
        }
    }
    if (scratch.empty()) {
        return failure("cannot create scratch directory below '" + scratch_root.string() + "'");
    }
    ScopedDirRemoval cleanup{scratch};
    fs::permissions(scratch, fs::perms::owner_all, fs::perm_options::replace, ec);

    const json cfg = {{"package_dir", pkg.string()},
                      {"base_model", base_model},
                      {"convert_script", config_.convert_script_path},
                      {"output_gguf", out_gguf.string()},
                      {"gguf_outtype", config_.gguf_outtype},
                      {"themis_metadata", meta}};
    const fs::path cfg_file = scratch / "convert.json";
    std::string cfg_text;
    try {
        cfg_text = cfg.dump(2, ' ', false, json::error_handler_t::strict);
    } catch (const json::exception& e) {
        return failure(std::string("conversion configuration is not valid UTF-8: ") + e.what());
    }
    if (!writeTextFile(cfg_file, cfg_text, &err)) {
        return failure(err);
    }

    const SubprocessOutcome out = runProtocolProcess(buildArgv(config_, cfg_file, true), {}, cancel,
                                                     config_.convert_timeout, config_.kill_grace,
                                                     config_.stderr_tail_bytes);
    if (!out.launched) {
        return failure("cannot start converter: " + out.launch_error);
    }
    if (out.cancelled) {
        return failure("conversion cancelled");
    }
    if (out.timed_out) {
        return failure(withTail("conversion timed out after " + std::to_string(config_.convert_timeout.count()) +
                                    " s",
                                out));
    }
    if (!out.result) {
        return failure(withTail(describeExit(out) + " without a THEMIS_RESULT line", out));
    }
    TrainingJobResult res = resultFromJson(*out.result);
    res.raw_report["backend"] = name();
    res.raw_report["exit_code"] = out.exit_code;
    if (out.exit_code != 0 || out.term_signal != 0 || !res.success) {
        res.success = false;
        std::string msg = res.error.empty() ? describeExit(out) : res.error;
        if (out.exit_code != 2) {
            msg = withTail(std::move(msg), out);
        }
        res.error = std::move(msg);
        return res;
    }
    if (res.gguf_adapter_path.empty() || !fs::is_regular_file(res.gguf_adapter_path, ec) ||
        fs::weakly_canonical(res.gguf_adapter_path, ec) != fs::weakly_canonical(out_gguf, ec)) {
        res.success = false;
        res.error = "converter reported success but '" + out_gguf.string() + "' was not produced";
        return res;
    }
    res.error.clear();
    return res;
}

}  // namespace lora
}  // namespace llm
}  // namespace themis
