---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
Source_Level: "L1"
SOT_Domain: "module-behavior"
Issue: "makr-code/ThemisDB#7145"
Milestone: "DOC-WEEKLY-2026-W44"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# RPC gRPC Implementation Notes

## Scope and build

This document is for developers modifying the runtime in
[`grpc_plugin.cpp`](grpc_plugin.cpp), the declarations in
[`grpc_plugin.h`](grpc_plugin.h), and the stream helper in
[`bidi_stream_adapter.h`](bidi_stream_adapter.h).

`src/rpc_grpc/CMakeLists.txt` adds `themis_rpc_grpc` only when
`THEMIS_PLUGIN_RPC_GRPC` is enabled and gRPC is found. Protobuf is then
required. The target links `gRPC::grpc++` and `protobuf::libprotobuf`; gRPC
reflection is optional. Do not infer build availability from the plugin
manifest alone.

## Server construction and lifecycle

`GRPCServer::initialize` validates the host and, if TLS is enabled, requires
three readable regular files for the certificate, private key, and CA. It
stores the configuration and builds `host:port`; port zero is accepted to
request an OS-selected port. The method does not validate all generic
`RPCServerConfig` fields.

`start` configures the main listener's credentials, adds supported keepalive
arguments, optionally adds the insecure `admin_port` listener, registers
service pointers, and applies the shared 100 MiB send/receive message-size
limits. With no
registered services it creates an idle completion queue, allowing the server
builder to start without a service. A null service pointer causes start to
fail if one is already present in the internal service list; the public
`registerService` method rejects null pointers before adding them. A second
start returns false. Errors are reported through diagnostic messages and a
false result.

`stop` first marks the server's tracked health entries not-serving and invokes
`grpc::Server::Shutdown()`. There is no module-level configurable shutdown
deadline. It drains the idle completion queue when present and clears the
running flag. Serialize start, stop, and registration calls in the host.

## Credentials and reload

When TLS is enabled, startup loads the configured files and builds gRPC SSL
credentials. `auth_required` selects whether the credential requires a client
certificate; application-level authorization is outside this class. If TLS is
disabled, the code deliberately returns insecure credentials and logs a
warning—this is not a TLS configuration fallback.

`reloadTls` requires a running TLS server and non-empty new paths. It reads all
three files and constructs credentials before stopping the listener, so
validation failures leave the active listener untouched. It then drains and
stops that listener and starts a replacement using the new file paths. A true
return means the new `BuildAndStart()` completed successfully. If the new start
fails, the method restores the old configuration and attempts to restart it;
if that rollback also fails, the server remains stopped. This is a disruptive
restart, not an in-place or zero-downtime rotation. Hosts must serialize
`reloadTls` with `initialize`, `start`, `stop`, and service registration.

The plugin stores service pointers without owning them, rejects null, late, and
duplicate-pointer registrations, and does not impose a service-count limit.

## Configuration applied by this implementation

`grpc_plugin.cpp` reads these values from `RPCServerConfig`:

- `host`, `port`, `tls_enabled`
- `tls_cert_path`, `tls_key_path`, `tls_ca_cert_path`
- `auth_required`
- `extra_config["keepalive_time_ms"]`
- `extra_config["keepalive_timeout_ms"]`
- `extra_config["admin_port"]`

The keepalive and admin values are parsed as integers. Invalid keepalive values
are ignored; an admin port outside `1..65535` is ignored. The admin listener
uses insecure credentials and must be isolated at the network boundary.
Generic config fields for rate limiting, request deadlines, connection limits,
compression, and worker counts are not applied here.

## Stream adapter behavior and limits

`BidiStreamAdapter<Req, Resp>` stores a non-owning pointer to a stream; passing
null throws `std::invalid_argument`. The default queue depth is 100. Copy and
move are disabled. `run()` calls `Read()` until EOF or the finished flag is
observed and invokes the registered handler if one exists. Setting the flag
does not interrupt a `Read()` already blocked inside gRPC.

`write()` waits while the response queue is full, pushes a response, and
synchronously drains the queue using `Stream::Write`. It returns false when
the adapter is finished; it does not propagate the return value of
`Stream::Write`. There is no internal write deadline. A zero queue depth is not
rejected and makes a writer wait until `finish()` releases it. `finish(status)`
stores the status and wakes queue waiters; it does not interrupt an in-flight
gRPC read/write, join a reader, or own the underlying stream. Callers must
coordinate stream lifetime and cancellation.

## Focused validation

Use the existing test cases rather than adding a second behavioral test layer:

- lifecycle and TLS startup: [`tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp`](../../tests/rpc_grpc/test_rpc_grpc_contract_hardening_focused.cpp)
- TLS listener restart and integration paths: [`tests/rpc_grpc/test_rpc_grpc_integration_focused.cpp`](../../tests/rpc_grpc/test_rpc_grpc_integration_focused.cpp)
- local registry and stream stress harness: [`tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp`](../../tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp); it does not test `GRPCServer::registerService`
- stream adapter: [`tests/test_bidi_stream_adapter.cpp`](../../tests/test_bidi_stream_adapter.cpp)
- metrics/access log: [`tests/test_grpc_observability.cpp`](../../tests/test_grpc_observability.cpp)

The test build is gRPC-conditional. Consult the generated CTest list for the
target names enabled by the local build.
