# ThemisDB TypeScript Binary Wire Protocol Client (Experimental)

⚠️ **EXPERIMENTAL** - This is an experimental binary wire protocol implementation for TypeScript/Node.js.

## Status

This directory contains a binary wire protocol client implementation that provides 5-10x better performance than HTTP/REST by using native TCP sockets.

**Current State:**
- ✅ Wire protocol v1 implementation complete
- ⚠️ No package.json or build configuration
- ⚠️ No tests
- ⚠️ Not published to npm
- ⚠️ Not covered by CI/CD

## Recommended Client

For production use, we recommend using the **HTTP/REST client** in `clients/javascript/`:

```bash
npm install @themisdb/client
```

See the [JavaScript/TypeScript SDK documentation](../javascript/README.md) for more information.

## Wire Protocol Implementation

The `src/themis-client.ts` file implements:
- Binary frame header (12 bytes)
- CRC32 checksum validation
- OpCode-based operations (30+ opcodes)
- Connection pooling
- Async I/O with Node.js `net` module

## Future Plans

This implementation may be integrated into the main JavaScript client package in a future release to provide both HTTP and wire protocol transports in a single package.

## Contributing

If you'd like to help complete this implementation:
1. Add `package.json` with dependencies
2. Add TypeScript build configuration
3. Add unit and integration tests
4. Add CI/CD workflow
5. Update documentation

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for contribution guidelines.
