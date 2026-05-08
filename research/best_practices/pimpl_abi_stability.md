# PIMPL Idiom for ABI Stability and Compile-Time Isolation

**Metadaten:**
- Source: Herb Sutter — "Exceptional C++" (2000); Guru of the Week #24 (GotW #24)
- URL: https://herbsutter.com/gotw/_24/
- Tags: software-design, abi
- ThemisDB-Versionen: v1.9.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

The PIMPL (Pointer to IMPLementation) idiom moves all private data members and the implementation details of a class behind a forward-declared opaque struct, exposing only the public interface in the header. This achieves two goals simultaneously: (1) ABI stability — adding or changing private members does not change the size or layout of the public class, so shared libraries can be updated without recompiling all consumers; (2) compile-time isolation — changes to implementation-only headers do not trigger recompilation of every translation unit that includes the public header.

In ThemisDB, the PIMPL idiom is applied in `src/server/mqtt_client_service.cpp` (the `AsioImpl` struct hides all Boost.Asio handles), and more broadly across async server components introduced in v1.9.0 where stable plugin and adapter ABIs are required.

## 🎯 Core Principles

- **Forward-declare the Impl struct**: The public header contains only `struct Impl;` and a `std::unique_ptr<Impl> pimpl_;` member, making the header's only dependency the `<memory>` header.
- **Define Impl in the .cpp file**: All private data (sockets, timers, mutexes, internal state) live in the `.cpp`-local `Impl` struct, invisible to consumers.
- **Rule of Five compliance**: Because `unique_ptr<Impl>` requires a complete type for its destructor, the destructor of the outer class must be defined in the `.cpp` file (even if `= default`), not implicitly in the header.
- **No raw pointers**: `std::unique_ptr<Impl>` for sole-ownership scenarios; `std::shared_ptr<Impl>` when shared ownership is needed (e.g., async callbacks outliving the outer object).
- **ABI versioning via IMPL_VERSION constant**: For dynamically loaded plugins, a version constant in the `.cpp` lets the loader reject incompatible builds.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/server/mqtt_client_service.cpp` — `MqttClientService::AsioImpl` holds all `boost::asio::ip::tcp::socket`, `boost::asio::steady_timer` (keepalive_timer, reconnect_timer), and internal MQTT state machine fields.
- `src/server/http_server.cpp` — HTTP server's `Impl` hides Boost.Asio acceptor and connection pool.
- `src/server/` async server components (v1.9.0) — `WebSocketSession::Impl`, `SseSession::Impl`, and `DistributedGateway::Impl` all follow the same pattern.

### What Was Adopted?

- Public header: `class MqttClientService { struct Impl; std::unique_ptr<Impl> pimpl_; public: ... };`
- `.cpp` file: full `struct MqttClientService::Impl { boost::asio::io_context& ioc; tcp::socket socket; steady_timer keepalive_timer; ... };`
- Destructor defined in `.cpp`: `MqttClientService::~MqttClientService() = default;` — ensures `unique_ptr<Impl>` destructor sees complete type.
- Move constructor and move-assignment also defined in `.cpp` for the same reason.
- Plugin loader (`src/plugins/plugin_loader.cpp`) validates `PLUGIN_ABI_VERSION` symbol exported by each `.so` before binding function pointers.

### Deviations & Rationale

- **Shared_ptr used for async lifetime extension**: Where Boost.Asio completion handlers capture `this`, `std::shared_ptr<Impl>` (via `shared_from_this`) is used instead of `unique_ptr` to safely extend the Impl's lifetime past the outer object's destruction in async chains.
- **Some internal-only classes skip PIMPL**: Pure-internal classes (not exported from any shared library, not part of any stable API) do not use PIMPL to avoid unnecessary indirection overhead. The idiom is applied selectively to ABI-sensitive boundaries.
- **No `make_unique` helper on older compilers**: The codebase targets C++17; `std::make_unique` is used throughout, consistent with the modern idiom.

## ⚠️ Trade-offs & Limitations

- **One extra heap allocation per object**: `unique_ptr<Impl>` requires a heap allocation that a plain struct-member approach does not. For high-frequency short-lived objects this can be measurable; PIMPL is appropriate only for long-lived service objects.
- **Indirect member access**: Every access to an `Impl` field goes through a pointer dereference (`pimpl_->field`), which can prevent the compiler from inlining or keeping values in registers across function calls.
- **Debugger ergonomics**: GDB/LLDB present the object as having only the `pimpl_` pointer, requiring an extra dereference step to inspect internals. Custom pretty-printers can mitigate this.
- **Copy semantics require manual implementation**: `unique_ptr<Impl>` is move-only, so if a copyable value type is needed, a deep-copy constructor must be written by hand.

## 🔬 Validation

- [x] Code reviewed against Herb Sutter GotW #24 and "Exceptional C++" chapter 6
- [x] ABI stability verified: `nm` + `objdump` confirm public symbols unchanged across Impl modifications
- [x] Rule of Five compliance checked in code review for all PIMPL classes
- [x] Module README linked (`src/server/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [Boost.Asio Async I/O](boost_asio_async_io.md)
- [Shared Mutex Read-Write Locks](shared_mutex_read_write_locks.md)

---
**Last Updated:** 2026-04-06
