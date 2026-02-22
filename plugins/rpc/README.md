# RPC Plugins

## Status: ✅ Production-ready

Remote Procedure Call (RPC) backend plugins for ThemisDB, enabling high-performance inter-shard and client-server communication.

## Available Backends

### gRPC ✅
**Path:** `grpc/`  
**Entry-point:** `plugins/rpc/grpc/grpc_plugin.cpp`

HTTP/2-based, Protocol Buffers serialisation, mutual TLS (mTLS) support.

See [grpc/README.md](grpc/README.md) for full documentation.

## Architecture / Integration

```
ThemisDB Core
    ↓
PluginManager  (include/plugins/plugin_manager.h)
    ↓
IRPCPlugin  (include/plugins/rpc_plugin_interface.h)
    ↓
gRPC Backend  (plugins/rpc/grpc/)
```

## Development Status

| Component | Status |
|-----------|--------|
| gRPC server | ✅ Production |
| mTLS support | ✅ Implemented |
| Inter-shard transfers | ✅ Implemented |

## Research / References

- [ ] TODO: Add reference – *gRPC: A High-Performance, Open Source Universal RPC Framework* (URL placeholder)
- [ ] TODO: Add reference – *Protocol Buffers: Google's Data Interchange Format* (URL placeholder)
- [ ] TODO: Add reference – *HTTP/2 – RFC 9113* (URL placeholder)

---

> Each plugin has its own documentation:
> - [`roadmap.md`](roadmap.md) – planned work
> - [`future_enhancements.md`](future_enhancements.md) – ideas backlog
