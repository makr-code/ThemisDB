# APIs Documentation

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** APIs

---

## Übersicht

API-Spezifikationen und -Dokumentation für ThemisDB.

## API-Typen

| API | Status | Standard | Beschreibung |
|-----|--------|----------|--------------|
| REST (HTTP/1.1) | ✅ Production | **ON** | HTTP/JSON API (immer aktiviert) |
| GraphQL | ✅ Production | **ON** | GraphQL Schema |
| SSE | ✅ Production | **ON** | Server-Sent Events für CDC/Live-Updates |
| gRPC | ✅ Production | **ON** | Inter-Shard Kommunikation (v1.3.0) |
| REST (HTTP/2) | ✅ Production-Ready | **OFF** | HTTP/2 Support mit ALPN (opt-in) |
| WebSocket | ✅ Production-Ready | **OFF** | Bidirektionale Real-time Kommunikation + CDC (opt-in) |
| MCP | 📋 Planned | **OFF** | Model Context Protocol für LLM-Integration (opt-in) |
| REST (HTTP/3) | 📋 Planned | **OFF** | HTTP/3 (QUIC) für Mobile/Edge (opt-in) |
| MQTT | 📋 Implemented (Stub) | **OFF** | IoT Message Broker - siehe OPTIONAL_PROTOCOLS.md |
| PostgreSQL Wire | 📋 Implemented (Stub) | **OFF** | PostgreSQL-Tool-Kompatibilität - siehe OPTIONAL_PROTOCOLS.md |
| OpenAPI | ✅ Production | - | OpenAPI 3.0 Spec |

**Hinweis:** Alle Protokolle können über Build-Schalter aktiviert/deaktiviert werden. Siehe [PROTOCOL_BUILD_SWITCHES.md](PROTOCOL_BUILD_SWITCHES.md) für Details.

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [HTTP_API_REFERENCE.md](HTTP_API_REFERENCE.md) | **Vollständige HTTP API Referenz** |
| [PROTOCOL_BUILD_SWITCHES.md](PROTOCOL_BUILD_SWITCHES.md) | **Netzwerk-Protokoll Build-Schalter und Übersicht** |
| [HTTP2_HTTP3_PROTOCOL_SUPPORT.md](HTTP2_HTTP3_PROTOCOL_SUPPORT.md) | **HTTP/2 und HTTP/3 Support - Vor- und Nachteile** |
| [HTTP2_HTTP3_USAGE_GUIDE.md](HTTP2_HTTP3_USAGE_GUIDE.md) | **HTTP/2 und HTTP/3 Benutzerhandbuch** |
| [HTTP2_HTTP3_IMPLEMENTATION_SUMMARY.md](HTTP2_HTTP3_IMPLEMENTATION_SUMMARY.md) | **HTTP/2 und HTTP/3 Implementierung - Technische Details** |
| [ADDITIONAL_PROTOCOLS.md](ADDITIONAL_PROTOCOLS.md) | **Weitere Protokolle (gRPC, WebSocket, MQTT, PostgreSQL Wire)** |
| [OPTIONAL_PROTOCOLS.md](OPTIONAL_PROTOCOLS.md) | **🆕 Optional Protocols - MQTT und PostgreSQL Wire Implementierung** |
| [MCP_PROTOCOL_SUPPORT.md](MCP_PROTOCOL_SUPPORT.md) | **MCP (Model Context Protocol) Support für LLM-Integration** |
| [MCP_OFFICE_PLUGINS.md](MCP_OFFICE_PLUGINS.md) | **MCP Integration mit Office Plugins (Word, Excel, Outlook)** |
| [apis_contentfs.md](apis_contentfs.md) | ContentFS API |
| [apis_graphql.md](apis_graphql.md) | GraphQL API |
| [apis_hot_reload.md](apis_hot_reload.md) | Hot Reload API |
| [apis_hybrid_search.md](apis_hybrid_search.md) | Hybrid Search API |
| [apis_openapi.md](apis_openapi.md) | OpenAPI Specification |

## Verwandte Dokumentation

- [API Documentation](../api/README.md) - REST API Endpoints
- [Server Module](../server/README.md) - HTTP Server
- [OpenAPI Spec](../openapi.yaml) - OpenAPI 3.0
