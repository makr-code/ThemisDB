# APIs Documentation

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** APIs

---

## Übersicht

API-Spezifikationen und -Dokumentation für ThemisDB.

## API-Typen

| API | Status | Beschreibung |
|-----|--------|--------------|
| REST (HTTP/1.1) | ✅ Production | HTTP/JSON API |
| REST (HTTP/2) | 🚧 In Development | HTTP/2 Support für bessere Performance |
| REST (HTTP/3) | 📋 Planned | HTTP/3 (QUIC) für Mobile/Edge Use Cases |
| GraphQL | ✅ Production | GraphQL Schema |
| OpenAPI | ✅ Production | OpenAPI 3.0 Spec |
| gRPC | 📋 Planned Q1 2025 | High-Performance RPC für Microservices |
| WebSocket | 📋 Planned Q1 2025 | Bidirektionale Real-time Kommunikation |
| PostgreSQL Wire | 📋 Planned Q3 2025 | PostgreSQL-Tool-Kompatibilität |

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [HTTP_API_REFERENCE.md](HTTP_API_REFERENCE.md) | **Vollständige HTTP API Referenz** |
| [HTTP2_HTTP3_PROTOCOL_SUPPORT.md](HTTP2_HTTP3_PROTOCOL_SUPPORT.md) | **HTTP/2 und HTTP/3 Support - Vor- und Nachteile** |
| [HTTP2_HTTP3_USAGE_GUIDE.md](HTTP2_HTTP3_USAGE_GUIDE.md) | **HTTP/2 und HTTP/3 Benutzerhandbuch** |
| [ADDITIONAL_PROTOCOLS.md](ADDITIONAL_PROTOCOLS.md) | **Weitere Protokolle (gRPC, WebSocket, MQTT, PostgreSQL Wire)** |
| [apis_contentfs.md](apis_contentfs.md) | ContentFS API |
| [apis_graphql.md](apis_graphql.md) | GraphQL API |
| [apis_hot_reload.md](apis_hot_reload.md) | Hot Reload API |
| [apis_hybrid_search.md](apis_hybrid_search.md) | Hybrid Search API |
| [apis_openapi.md](apis_openapi.md) | OpenAPI Specification |

## Verwandte Dokumentation

- [API Documentation](../api/README.md) - REST API Endpoints
- [Server Module](../server/README.md) - HTTP Server
- [OpenAPI Spec](../openapi.yaml) - OpenAPI 3.0
