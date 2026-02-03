---
name: 🎨 AI Review - API Design
about: Systematisches API-Design-Review für Konsistenz, Benutzerfreundlichkeit und Best Practices / Systematic API design review
title: '[API-REVIEW] '
labels: ['type:systematic-review', 'area:api', 'needs-triage']
assignees: ''
---

<!-- 
Wiederholbare Template für API-Design-Reviews
Repeatable template for API design reviews
Empfohlene Häufigkeit: Quartalsweise oder vor Major Releases / Recommended: Quarterly or before major releases
-->

## 🎯 API Component / API-Komponente

**API Name:** <!-- z.B. REST API, gRPC Service, GraphQL Schema -->
**API Type:** <!-- REST, gRPC, GraphQL, PostgreSQL Wire Protocol, MQTT, WebSocket -->
**Component Path:** <!-- z.B. src/api/, src/plugins/graphql/ -->
**API Version:** <!-- z.B. v1, v2, v1.4.x -->
**Review Period:** <!-- z.B. Q1 2026 -->
**Reviewer(s):** <!-- Namen der Reviewer -->
**Previous Review:** <!-- Datum des letzten Reviews -->

---

## 📊 API Overview / API-Übersicht

### API Scope / API-Umfang
- **Total Endpoints/Methods:** 
- **Public Endpoints:** 
- **Internal Endpoints:** 
- **Deprecated Endpoints:** 
- **New in this version:** 

### API Categories / API-Kategorien
- [ ] CRUD operations
- [ ] Query/Search
- [ ] Batch operations
- [ ] Streaming
- [ ] Admin/Management
- [ ] Authentication/Authorization
- [ ] Other: _______

---

## 🏗️ API Design Principles / API-Design-Prinzipien

### RESTful Design (if applicable)
- [ ] **Resource-oriented** URLs
- [ ] **HTTP verbs** used correctly (GET, POST, PUT, DELETE, PATCH)
- [ ] **Status codes** appropriate (2xx, 4xx, 5xx)
- [ ] **Idempotency** considered for PUT/DELETE
- [ ] **HATEOAS** principles (if relevant)

### Naming Conventions / Namenskonventionen
- [ ] **Consistent naming** across endpoints
- [ ] **Clear and descriptive** names
- [ ] **Proper case** usage (camelCase, snake_case, kebab-case)
- [ ] **Verb/noun** clarity in method names
- [ ] **Plural vs singular** consistency

**Observations:**


### Versioning Strategy / Versionierungsstrategie
- [ ] API versioning implemented?
- [ ] Version strategy: <!-- URL path, Header, Query parameter? -->
- [ ] Backward compatibility maintained?
- [ ] Deprecation policy defined?
- [ ] Migration path clear?

**Current Versioning:**


---

## 📝 API Consistency / API-Konsistenz

### Request/Response Format
- [ ] **JSON** format consistent
- [ ] **Field naming** consistent
- [ ] **Date/time** format standardized
- [ ] **Pagination** pattern consistent
- [ ] **Filtering** pattern consistent
- [ ] **Sorting** pattern consistent

**Inconsistencies Found:**
1. 
2. 
3. 

### Error Handling / Fehlerbehandlung
- [ ] **Error format** standardized
- [ ] **Error codes** well-defined
- [ ] **Error messages** helpful and clear
- [ ] **Error details** provided (when appropriate)
- [ ] **Stack traces** handled securely

**Error Response Example:**
```json
{
  "error": {
    "code": "",
    "message": "",
    "details": {}
  }
}
```

**Issues Found:**


### Parameter Handling / Parameter-Behandlung
- [ ] **Query parameters** documented
- [ ] **Path parameters** validated
- [ ] **Request body** validation
- [ ] **Default values** defined
- [ ] **Optional vs required** clear
- [ ] **Parameter limits** enforced

**Issues Found:**


---

## 📚 API Documentation / API-Dokumentation

### Documentation Completeness / Dokumentationsvollständigkeit
- [ ] **OpenAPI/Swagger** specification exists
- [ ] **All endpoints** documented
- [ ] **Parameters** fully described
- [ ] **Request/response examples** provided
- [ ] **Error codes** documented
- [ ] **Authentication** documented
- [ ] **Rate limiting** documented

**Documentation Gaps:**
1. 
2. 
3. 

### Developer Experience / Entwicklererfahrung
- [ ] **Quick start guide** available
- [ ] **Code examples** in multiple languages
- [ ] **Interactive API explorer** (Swagger UI, GraphiQL)
- [ ] **Postman collection** or equivalent
- [ ] **SDK/Client libraries** available
- [ ] **Changelog** maintained

**Missing Developer Resources:**
1. 
2. 
3. 

---

## 🔒 API Security / API-Sicherheit

### Authentication & Authorization
- [ ] **Authentication** method clear (JWT, OAuth2, API Key)
- [ ] **Authorization** checks at every endpoint
- [ ] **Role-based access control** (RBAC) implemented
- [ ] **Token expiration** handled properly
- [ ] **Refresh token** mechanism (if applicable)

**Security Issues:**


### OWASP API Security Top 10 (2023)
- [ ] **API1:2023** - Broken Object Level Authorization (BOLA)
- [ ] **API2:2023** - Broken Authentication
- [ ] **API3:2023** - Broken Object Property Level Authorization
- [ ] **API4:2023** - Unrestricted Resource Consumption
- [ ] **API5:2023** - Broken Function Level Authorization (BFLA)
- [ ] **API6:2023** - Unrestricted Access to Sensitive Business Flows
- [ ] **API7:2023** - Server Side Request Forgery (SSRF)
- [ ] **API8:2023** - Security Misconfiguration
- [ ] **API9:2023** - Improper Inventory Management
- [ ] **API10:2023** - Unsafe Consumption of APIs

**Findings:**


### Input Validation / Eingabevalidierung
- [ ] **SQL/NoSQL injection** protection
- [ ] **XSS** protection
- [ ] **Input sanitization** implemented
- [ ] **File upload** validation (if applicable)
- [ ] **Rate limiting** per endpoint
- [ ] **Request size limits** enforced

**Validation Gaps:**


---

## ⚡ API Performance / API-Performance

### Performance Metrics / Performance-Metriken
- **Average response time:** 
- **p95 response time:** 
- **p99 response time:** 
- **Throughput (requests/sec):** 
- **Error rate:** 

### Performance Optimization / Performance-Optimierung
- [ ] **Caching** implemented where appropriate
- [ ] **Pagination** for large result sets
- [ ] **Field filtering** (sparse fieldsets)
- [ ] **Batch endpoints** available
- [ ] **Streaming** for large data
- [ ] **Compression** enabled (gzip, brotli)

**Performance Issues:**


---

## 🧪 API Testing / API-Testing

### Test Coverage / Test-Abdeckung
- [ ] **Unit tests** for API handlers
- [ ] **Integration tests** for API flows
- [ ] **Contract tests** (Pact, etc.)
- [ ] **Performance tests** / load tests
- [ ] **Security tests** (OWASP ZAP, etc.)
- [ ] **API schema validation** tests

**Test Coverage:** <!-- Percentage, if available -->

**Testing Gaps:**
1. 
2. 
3. 

---

## 🌐 API Standards & Best Practices

### Industry Standards Compliance
- [ ] **REST** principles (if REST API)
- [ ] **gRPC** best practices (if gRPC)
- [ ] **GraphQL** best practices (if GraphQL)
- [ ] **JSON:API** specification (if applicable)
- [ ] **HAL** specification (if applicable)
- [ ] **Problem Details (RFC 7807)** for errors

### Monitoring & Observability
- [ ] **Metrics** collected (latency, throughput, errors)
- [ ] **Logging** standardized
- [ ] **Tracing** implemented (OpenTelemetry, Jaeger)
- [ ] **Health check** endpoint exists
- [ ] **Status page** integration

---

## 🔄 API Evolution / API-Evolution

### Backward Compatibility / Rückwärtskompatibilität
- [ ] Breaking changes identified?
- [ ] Deprecation warnings in place?
- [ ] Migration guide available?
- [ ] Sunset dates announced?

**Breaking Changes in Review Period:**
1. 
2. 
3. 

### Future Enhancements / Zukünftige Verbesserungen
**Short-Term (Next 3 Months):**
- [ ] 
- [ ] 
- [ ] 

**Medium-Term (3-6 Months):**
- [ ] 
- [ ] 
- [ ] 

**Long-Term (6-12 Months):**
- [ ] 
- [ ] 
- [ ] 

---

## ✅ Action Items / Aktionspunkte

### Critical (P0) - Breaking Issues
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - Impact: 

### High Priority (P1) - Important Improvements
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - Impact: 

2. [ ] **Action 2:**
   - Owner: 
   - Due Date: 
   - Impact: 

### Medium Priority (P2) - Nice-to-Have
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - Impact: 

---

## 📚 References / Referenzen

### Internal Documentation
- [API Documentation](docs/api/)
- [OpenAPI Specification](openapi/)
- [Authentication Guide](docs/security/authentication.md)

### External Resources
- [REST API Best Practices](https://restfulapi.net/)
- [OWASP API Security Top 10](https://owasp.org/www-project-api-security/)
- [gRPC Best Practices](https://grpc.io/docs/guides/best-practices/)
- [GraphQL Best Practices](https://graphql.org/learn/best-practices/)

---

## 📋 Review Checklist / Review-Checkliste

- [ ] All endpoints reviewed for consistency
- [ ] Documentation completeness verified
- [ ] Security checklist completed (OWASP API Top 10)
- [ ] Performance metrics collected
- [ ] Error handling standardized
- [ ] Breaking changes identified
- [ ] Action items created and assigned
- [ ] Sign-offs obtained from API team and security team

---

**Review Date:** <!-- YYYY-MM-DD -->
**Next Review:** <!-- YYYY-MM-DD (empfohlen: +3 Monate oder vor Major Release) -->
**Sign-Off:** <!-- API Team Lead, Security Team, Product Owner -->

---

**Template Version:** 1.0.0  
**Created:** 2026-02-02  
**Maintained by:** ThemisDB API Team
