> **Status:** 2026-06-01 – mit aktuellem Query-Code (`aql_parser.cpp`, `query_federation.cpp`, `cross_cluster_federator.cpp`, `query_canceller.cpp`) abgeglichen.

# ThemisDB Query Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Query-Moduls.
Es definiert verbindliche Anforderungen für Query-Limits, SSRF-Schutz, Tenant-Isolation, Ressourcenbudgetierung und sichere Cluster-Registrierung.

## Dokumentabgrenzung (Canonical Split)

- **`src/query/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/query/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/query/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/query/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Query-Limit-Anforderungen

- **MUST:** Per-Query-Limits (max-rows, max-memory, timeout) in allen Produktionspfaden erzwungen; keine Caller-Option zum Deaktivieren.
- **MUST:** `ContinuousQueryEngineImpl` Registry-Cap auf ≤1 000 gleichzeitige Queries konfiguriert.
- **MUST:** Injection-Queue des ContinuousQueryEngineImpl auf ≤100 000 Einträge begrenzt.
- **MUST:** `CrossClusterFederator` HTTP-Response-Größe auf ≤64 MiB begrenzt.
- **MUST NOT:** Unbegrenzte Query-Execution ohne Timeout-Guardrail betreiben.

## Verbindliche Sicherheitsanforderungen

### 1) SSRF- und Header-Injection-Schutz (CrossClusterFederator)

- **MUST:** `registerCluster()` lehnt `base_url`-Werte ab, die nicht mit `http://` oder `https://` beginnen.
- **MUST:** `auth_token` mit CR/LF-Zeichen wird bei Cluster-Registrierung abgewiesen.
- **MUST:** Redirect-Hops auf ≤3 begrenzt; libcurl `CURLOPT_PROTOCOLS` und `CURLOPT_REDIR_PROTOCOLS` auf HTTP/HTTPS beschränkt.
- **MUST:** SSL-Peer-Verification aktiviert (`CURLOPT_SSL_VERIFYPEER`); Deaktivierung nur in deklarierten Nicht-Produktionsumgebungen.

### 2) Tenant-Isolation

- **MUST:** `collection_access_checker_` in allen Query-Execution-Pfaden aktiv; `ERR_QUERY_ACCESS_DENIED` bei Denial.
- **MUST NOT:** Query-Pfade ohne Tenant-Namespace-Isolation betreiben.

### 3) Query-Cancellation

- **MUST:** `query_canceller.cpp` in Produktionspfaden aktiv; hängende Queries müssen über Request-ID abbrechbar sein.

## Betriebsgrenzen (aktuelles Query-Verhalten)

- `AQLParser` ist stateless (KL-01, geschlossen); keine threadlocal Query-State-Leaks zwischen parallelen Requests.
- SPARQL/SQL-Inputs werden vollständig in AST übersetzt; Raw-Dialect-Strings werden nicht direkt ausgeführt.
- Parameterized-Query-API trennt Struktur von Werten; direkte String-Interpolation durch Caller umgeht AQL-Injection-Schutz.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Per-Query-Limits (max-rows, max-memory, timeout) erzwungen
- [ ] ContinuousQueryEngine Registry-Cap ≤1 000 konfiguriert
- [ ] CrossClusterFederator Response-Size-Cap ≤64 MiB aktiv
- [ ] SSRF-Schutz: base_url-Validierung und SSL-Peer-Verify aktiv
- [ ] auth_token-CR/LF-Rejection aktiv
- [ ] collection_access_checker_ in Query-Execution-Pfaden aktiv
- [ ] Query-Canceller aktiv (hängende Queries abbrechbar)
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/query/PRODUCTION_REQUIREMENTS.md`
- `src/query/aql_parser.cpp`
- `src/query/query_federation.cpp`
- `src/query/cross_cluster_federator.cpp` (falls vorhanden)
- `src/query/continuous_query_engine.cpp` (falls vorhanden)
- `src/query/query_canceller.cpp`
- `src/query/aql_safety_validator.cpp`
