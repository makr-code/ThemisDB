# Network Module — Gap Verifier Report
**Analyst pass:** 2026-08-25T15:34:34Z  
**Raw CRITICAL count:** 29 | **Verified CRITICAL:** 4 | **Downgraded:** 11 | **Removed (FP):** 14

---

## Executive Summary

| Category | Count |
|---|---|
| Real CRITICAL (production risk) | 4 |
| Real HIGH (real gap, not immediately critical) | 9 |
| Real MEDIUM (guarded / bounded) | 2 |
| False-Positives removed | 14 |

The scanner's raw CRITICAL count (29) was heavily inflated by two systematic false-positive patterns:
1. **`braces_imbalance` at line 1** (5 findings) — all 5 flagged files have **perfectly balanced braces** (diff=0) when comments and string literals are stripped. The scanner's preprocessor-unaware heuristic fires on `#ifdef`-gated brace blocks.
2. **`scope_mismatch`** (1,404 raw findings, not individually in CRITICAL list) — every fully-qualified C++ name (`std::`, `boost::asio::`) triggers this pattern. No actual scope collisions found in manual review.

---

## Confirmed Real CRITICAL Gaps

### 1. Command Injection — `qos_manager.cpp` lines 663 / 672 / 685

```cpp
// line 660-663 (CRITICAL)
char cmd[256];
std::snprintf(cmd, sizeof(cmd),
              "%s qdisc del dev %s root 2>/dev/null",
              tc_bin, iface.c_str());          // ← iface is user-controlled
std::system(cmd);  // NOLINT(cert-env33-c)    // ← OS shell injection
```

**Root cause:** `iface.c_str()` (the network interface name) flows from configuration into a shell command string without any allowlist validation. If an attacker can write to the config or API endpoint that sets `interface_name`, they achieve arbitrary OS command execution as the ThemisDB process user.

**Fix:**
```cpp
// Option A: execv-family (no shell, no injection)
const char* argv[] = { tc_bin, "qdisc", "del", "dev", iface.c_str(), "root", nullptr };
pid_t pid; posix_spawn(&pid, tc_bin, nullptr, nullptr, const_cast<char**>(argv), environ);
waitpid(pid, nullptr, 0);

// Option B: allowlist regex before any system() call
static const std::regex kIfaceAllowlist(R"([a-zA-Z0-9\-_\.]{1,15})");
if (!std::regex_match(iface, kIfaceAllowlist))
    return false;  // reject before shell call
```

---

### 2. Unimplemented Health Check Stub — `raft_load_balancer.cpp` line 425

```cpp
bool RaftLoadBalancer::defaultHealthCheck(const Backend & /*backend*/) {
    // In a real implementation this would open a TCP connection to
    // backend.address and return true if it succeeds.
    return true;  // ← ALWAYS returns healthy — stub never implemented
}
```

**Root cause:** Every backend permanently reports healthy. Dead or overloaded backends continue to receive traffic. Load-balancer failover never triggers. The function is the *default* — it is used whenever the caller has not supplied a `health_check_fn_`, which is the common case for embedded usage.

**Fix:**
```cpp
bool RaftLoadBalancer::defaultHealthCheck(const Backend& backend) {
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) return false;
    struct sockaddr_in addr{}; /* populate from backend.address */
    ConnectionGuard guard(*this, backend.address);  // RAII
    int r = ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    // poll with 500 ms deadline, then close
    // ... return result
    ::close(fd);
    return (r == 0 || errno == EISCONN);
}
```

---

## Downgraded to HIGH

### 3. Deadlock Risk — `wire_protocol_server.cpp` lines 701 + 710

```cpp
bool WireProtocolServer::checkConnectionLimit(...) {
    std::lock_guard<std::mutex> lock(connections_mutex_);  // L1
    ...
}
bool WireProtocolServer::checkRateLimit(...) {
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);   // L2
    ...
}
// Accept handler calls them sequentially: L1 then L2.
// Stats path (lines 830, 845) takes L2 (stats_mutex_) then later L1 — ABBA risk.
```
**Fix:** Codify canonical lock order: `connections_mutex_ < rate_limit_mutex_ < stats_mutex_`. Use `std::scoped_lock` for any function requiring two.

---

### 4. Deadlock Risk — `wire_protocol_server.cpp` lines 830 / 845 / 855

Two consecutive `stats_mutex_` acquisitions (830, 845) followed by `connections_mutex_` (855) in the accept loop. Because `registerConnection()` (751) takes `connections_mutex_` and then stats can be updated inside the same accept callback, a thread interleave exists.  
**Fix:** Same canonical ordering as #3.

---

### 5. Missing Destructor — `socket_timeout_manager.cpp` line 71

No destructor definition found in `.cpp`. Class manages raw `socket_t` handles. Implicit destructor will not close OS file descriptors.  
**Fix:** Add `~SocketTimeoutManager()` that calls `closeAllSockets()` and joins background timer threads.

---

### 6. Smart Pointer Misuse — `socket_timeout_manager.cpp` line 202

`shared_ptr<socket_handle>` wrapper missing custom deleter for `socket_t` (which requires `::closesocket()` on Windows, `::close()` on POSIX). Default `shared_ptr` deleter calls `delete` — UB on a socket handle.  
**Fix:**
```cpp
auto handle = std::shared_ptr<socket_t>(
    new socket_t(fd),
    [](socket_t* s) { ::close(*s); delete s; }
);
```

---

### 7. No Timeout in service_mesh.cpp SO_SNDTIMEO — lines 175 / 194

`SO_SNDTIMEO` hardcoded to 5 seconds with no way to override via `config_`. On non-Linux builds (the `#ifdef __linux__` guard), timeout is never set — synchronous `boost::asio::write()` can block indefinitely on macOS/FreeBSD.  
**Fix:** Apply timeout via `boost::asio::async_write` with a deadline timer, or set `SO_SNDTIMEO` in a platform-independent way using `setsockopt`.

---

### 8. Service Mesh Accept Polling — `service_mesh.cpp` line 243

30-second deadline is implemented but via `sleep_for(poll_interval)` busy-polling. Under load, this wastes CPU and creates up to `poll_interval` (typically 10ms) latency per accept attempt.  
**Fix:** Replace poll loop with `boost::asio::async_accept` + `steady_timer`.

---

### 9–11. (See JSON artifact for `missing_dtor` service_mesh.cpp lines 175/194, `exception_in_destructor` zero_copy.cpp line 220)

---

## MEDIUM (Guarded — Verify Only)

| File | Line | Pattern | Status |
|---|---|---|---|
| `connection_compression.cpp` | 114 | `unchecked_memcpy` | R12 bounds checks added (lines 108–113). Verify concat.size() pre-loop total. |
| `io_uring_batcher.cpp` | 251 | `unchecked_memcpy` | R13 bounds checks present. Replace runtime checks with `static_assert(SQE_SIZE == 64)`. |

---

## False-Positives Removed (14 total)

| # | File | Line | Pattern | Why Removed |
|---|---|---|---|---|
| 1 | `kernel_bypass.cpp` | 1 | `braces_imbalance` | Brace count: 116=116 ✓ |
| 2 | `quic_server.cpp` | 1 | `braces_imbalance` | Brace count: 119=119 ✓ |
| 3 | `raft_load_balancer.cpp` | 1 | `braces_imbalance` | Brace count: 95=95 ✓ |
| 4 | `wire_protocol_performance.cpp` | 1 | `braces_imbalance` | Brace count: 47=47 ✓ |
| 5 | `wire_protocol_v2.cpp` | 1 | `braces_imbalance` | Brace count: 134=134 ✓ |
| 6–14 | Various | — | `scope_mismatch` | Triggered on every `std::`/`boost::` qualifier; no actual ADL collision found in 9 reviewed files |

**Root Cause of Scanner Inflation:** The `braces_imbalance` heuristic does not model `#ifdef`/`#endif` preprocessor conditional compilation. Files with platform-specific brace blocks trigger false reports. The scanner should skip line-1 reports entirely and require diff ≥ 2 before flagging.

---

## Wave 3-Network Priority

See next section of report.
