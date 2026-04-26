# RPC-gRPC-Plugin

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/rpc_grpc/README.md -->

**Stand:** 6. April 2026  
**Version:** 0.0.1  
**Kategorie:** RPC / gRPC-Plugin  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das RPC-gRPC-Plugin stellt einen dedizierten gRPC-Kommunikations-Plugin für ThemisDB bereit, der als eigenständige Plugin-Architektur neben dem Kern-API-Modul läuft.

**Primäre Quelle:** [`src/rpc_grpc/`](../../../src/rpc_grpc/)

---

## Kernkomponenten

| Komponente | Source | Beschreibung |
|------------|--------|--------------|
| GrpcPlugin | `grpc_plugin.cpp` | gRPC-Plugin-Einstiegspunkt und Registrierung |

---

## Hinweis

Das primäre gRPC-Interface (GrpcApiServer, ThemisDBGrpcService) befindet sich im API-Modul (`include/api/`, `src/api/`). Dieses Plugin-Modul ist die Plugin-Wrapper-Implementierung.

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/rpc_grpc/README.md`](../../../src/rpc_grpc/README.md) | Modulübersicht |
