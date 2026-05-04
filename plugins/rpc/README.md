# RPC Plugins

## Status: ✅ Production-ready

Remote Procedure Call (RPC) backend plugins for ThemisDB, enabling high-performance inter-shard and client-server communication.

## Available Backends

### gRPC ✅
**Path:** `grpc/`  
**Public API:** `include/plugins/rpc/grpc_plugin.h`  
**Implementation:** `src/rpc_grpc/`  
**Legacy compatibility path:** `plugins/rpc/grpc/`

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
gRPC Backend  (src/rpc_grpc/)
```

## Integration Notes

- The canonical implementation now lives in `src/rpc_grpc/`.
- Public includes should use `include/plugins/rpc/grpc_plugin.h`.
- `plugins/rpc/grpc/` is retained as a compatibility CMake entry point for older workflows.

## Development Status

| Component | Status |
|-----------|--------|
| gRPC server | ✅ Production |
| mTLS support | ✅ Implemented |
| Inter-shard transfers | ✅ Implemented |

## Research / References

- A. D. Birrell and B. J. Nelson, "Implementing remote procedure calls," *ACM Trans. Comput. Syst.*, vol. 2, no. 1, pp. 39–59, Feb. 1984. DOI: [10.1145/2080.357392](https://doi.org/10.1145/2080.357392)
- M. Belshe, R. Peon, and M. Thomson, "Hypertext Transfer Protocol Version 2 (HTTP/2)," RFC 9113, IETF, Jun. 2022. DOI: [10.17487/RFC9113](https://doi.org/10.17487/RFC9113)
- L. Lamport, "Time, clocks, and the ordering of events in a distributed system," *Commun. ACM*, vol. 21, no. 7, pp. 558–565, Jul. 1978. DOI: [10.1145/359545.359563](https://doi.org/10.1145/359545.359563)
- J. H. Saltzer, D. P. Reed, and D. D. Clark, "End-to-end arguments in system design," *ACM Trans. Comput. Syst.*, vol. 2, no. 4, pp. 277–288, Nov. 1984. DOI: [10.1145/357401.357402](https://doi.org/10.1145/357401.357402)
- E. Brewer, "Towards robust distributed systems," in *Proc. 19th ACM Symp. Principles of Distributed Computing (PODC)*, 2000, p. 7. DOI: [10.1145/343477.343502](https://doi.org/10.1145/343477.343502)

---

> Each plugin has its own documentation:
> - [`ROADMAP.md`](ROADMAP.md) – planned work
> - [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) – ideas backlog
