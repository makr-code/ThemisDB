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

# Embedding the gRPC Plugin

## Meaning and scope

Here, “embedding” means integrating the gRPC transport plugin into a ThemisDB
host process. It does not mean vector embeddings or embedding-model inference:
`src/rpc_grpc` transports registered gRPC services and contains no embedding
model implementation.

## Build and load boundary

The module is an optional shared-library plugin. Its
[`CMakeLists.txt`](CMakeLists.txt) returns without creating the target when
`THEMIS_PLUGIN_RPC_GRPC` is off or gRPC is not found; Protobuf is required once
the module is built. The output is placed in the build tree's
`plugins/rpc` directory and installed under `lib/themis/plugins/rpc`.
The shared library exports `createPlugin` and `destroyPlugin`; the plugin host
owns discovery, loading, and destruction.

The built-in API gRPC server in `src/api/grpc_server.cpp` is a separate
subsystem. Enabling or configuring that server does not, by itself, enable this
RPC plugin.

## Host integration sequence

1. Enable the plugin target and satisfy the gRPC and Protobuf dependencies.
2. Load the plugin through the host's plugin mechanism and create an
   `IRPCServer` with `GRPCPlugin::createServer()`.
3. Populate `RPCServerConfig` explicitly. The interface defaults
   `tls_enabled` to false and `port` to zero; do not rely on the plugin
   manifest's JSON defaults because `GRPCPlugin::initialize(config_json)`
   currently ignores that JSON argument.
4. Call `initialize(config)`. When TLS is enabled, the server process must
   read all three configured PEM paths: server certificate, private key, and
   CA certificate.
5. Keep each `grpc::Service` object alive for the lifetime of the server,
   register it before `start()`, then check the boolean start result.
6. On host shutdown, call `stop()` before destroying the service objects and
   plugin server instance.

The plugin accepts service implementations via the base interface's
`void*` parameter and casts them to `grpc::Service*`. Pass only a valid gRPC
service pointer; the interface does not transfer ownership.

## Configuration and exposure constraints

The current implementation applies the listener address, port, TLS paths,
`auth_required`, and two `extra_config` values for keepalive. It also supports
an optional `extra_config["admin_port"]`, which creates an insecure listener
on the same configured host. Do not expose that listener to untrusted networks.

Other fields in `RPCServerConfig`—including connection limits, thread-pool
size, request/connection timeouts, rate limiting, and compression—are declared
by the shared interface but are not consumed by `grpc_plugin.cpp`. Do not
assume those controls are active in this plugin.

## Validation before rollout

- Verify the plugin is present in the host's actual load path and that the
  module was built with `THEMIS_PLUGIN_RPC_GRPC` enabled.
- Exercise the service with a client using the intended TLS or mTLS policy.
- Confirm registration happened before start and that the process can read the
  configured key material.
- Confirm which listener addresses are reachable from outside the host network.
- Run the module's focused integration tests in a build configured with gRPC.

For exact interface and failure details see [`API.md`](API.md),
[`MODULE_GUIDE.md`](MODULE_GUIDE.md), and [`TRUST.md`](TRUST.md).
