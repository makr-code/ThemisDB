# API DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\api\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\api\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 57
- Compounds: 192
- Classes/Structs: 101
- Namespaces: 25
- File Compounds: 57

## Namespaces
- @013166054137260065331272061365140172175247000067
- @056016377065004074217277155321144017364216266343
- @112005004346367211230114071103212073102327227147
- @152047334047241147326340346102311243244361027371
- @167111040355276364274143321371140333155304235206
- @216272156133270174214220347310333063105152165211
- @225204144111052343010135254031321311001324167171
- @272025310350240050144270052350101266203210077067
- @305077063170374030137034050073140214342131063304
- @310230072370013212102216156170250052374324312001
- @333067054346374133170362000024134227105305105230
- @377247222244162004265045036140270171362023015166
- std
- testing
- themis
- themis::api
- themis::api::@161235063250017223205041261046035003125364166346
- themis::api::@221140247254077056060203363235103375206061050177
- themis::auth
- themis::errors
- themis::graphql
- themis::graphql::@220224305067110044252171250305110031310040063175
- themis::index
- themis::query
- themis::server

## Types
### Classes
- APIVersionComparisonTest
- APIVersionManagerRoutingTest
- APIVersionManagerTest
- APIVersionParsingTest
- APIVersionRangeResolutionTest
- APIVersionTest
- ApiAuthConfigTest
- ApiKeyMgmtHandlerTest
- ApiSecurityAuditTest
- BreakingChangeTest
- themis::api::ApiErrorTaxonomy
- themis::api::CorrelationId
- themis::api::FederationAdminHandler
- themis::api::GeoIndexHooks
- themis::api::IAPIGatewayHook
- themis::api::IAPIVersionRouter
- themis::api::ICorrelationIDProvider
- themis::api::IGRPCBridge
- themis::api::IGatewayHookRegistry
- themis::api::IGraphQLSchemaBuilder
- themis::api::IHttpHandler
- themis::api::ISubscriptionMultiplexer
- themis::api::ITransportContract
- themis::api::IWebSocketFrameCallback
- themis::api::IWebSocketHandler
- themis::api::MiddlewareChain
- themis::api::OtlpExporter
- themis::api::ThemisDBGrpcService
- themis::api::ThemisDBGrpcService::Impl
- themis::api::ThemisDBGrpcServiceFactory
- themis::api::TracingMiddleware
- themis::api::TransportContractValidator
- themis::api::TransportPolicyMiddleware
- themis::api::WebSocketSession
- themis::graphql::AuditLogBuilder
- themis::graphql::AuditLogger
- themis::graphql::Cache
- themis::graphql::Executor
- themis::graphql::FileAuditLogHandler
- themis::graphql::GraphQLAqlResolverFactory
- themis::graphql::GraphQLComplexityEstimator
- themis::graphql::Metrics
- themis::graphql::OperationRateLimiter
- themis::graphql::Parser
- themis::graphql::PersistedQueryRegistry
- themis::graphql::QueryAllowList
- themis::graphql::QueryHasher
- themis::graphql::QueryPlanCache
- themis::graphql::QueryTimer
- themis::graphql::RateLimiter
- themis::graphql::ResponseCache
- themis::graphql::Schema
- themis::graphql::ThemisSchemaBuilder
- themis::query::expected

### Structs
- std::hash< themis::api::CorrelationId >
- themis::api::GRPCMetadata
- themis::api::GRPCRequest
- themis::api::GatewayHookContext
- themis::api::GatewayHookResult
- themis::api::GraphQLFieldDescriptor
- themis::api::GraphQLTypeDescriptor
- themis::api::HttpError
- themis::api::HttpRequest
- themis::api::HttpResponse
- themis::api::OtlpExporterConfig
- themis::api::RouteEntry
- themis::api::SchemaValidationError
- themis::api::SchemaValidationResult
- themis::api::ServiceDescriptor
- themis::api::SpanData
- themis::api::SubscriptionEvent
- themis::api::SubscriptionFilter
- themis::api::TransportPolicyConfig
- themis::api::VersionDescriptor
- themis::api::WebSocketFrame
- themis::graphql::AuditLogEntry
- themis::graphql::AuditLogger::Stats
- themis::graphql::Cache::CacheEntry
- themis::graphql::Cache::CacheStats
- themis::graphql::Document
- themis::graphql::ExecutionContext
- themis::graphql::Executor::Result
- themis::graphql::Field
- themis::graphql::FieldDefinition
- themis::graphql::MaskedError
- themis::graphql::Metrics::QueryMetrics
- themis::graphql::Operation
- themis::graphql::ParseError
- themis::graphql::Parser::Result
- themis::graphql::PersistedQueryRegistry::PersistedQuery
- themis::graphql::QueryLimits
- themis::graphql::QueryPlanCache::QueryPlan
- themis::graphql::RateLimitHeaders
- themis::graphql::RateLimiter::Bucket
- themis::graphql::RateLimiter::Config
- themis::graphql::RateLimiter::Stats
- themis::graphql::ResponseCache::CachedResponse
- themis::graphql::TypeDefinition
- themis::graphql::TypeRef
- themis::graphql::Value
- themis::graphql::VariableDefinition

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 797

### APIVersionTest

#### `void SetUp() override`
- Source: `tests/api/test_api_version.cpp`:19
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/api/test_api_version.cpp`:23
- Brief: n/a
- Parameters: none

### ApiAuthConfigTest

#### `void SetUp() override`
- Source: `tests/api/test_api_auth_config.cpp`:19
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/api/test_api_auth_config.cpp`:23
- Brief: n/a
- Parameters: none

### ApiKeyMgmtHandlerTest

#### `void SetUp() override`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:32
- Brief: n/a
- Parameters: none

### ApiSecurityAuditTest

#### `void SetUp() override`
- Source: `tests/api/test_api_security_audit.cpp`:53
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/api/test_api_security_audit.cpp`:54
- Brief: n/a
- Parameters: none

### bench_api_release_gates.cpp

#### `BENCHMARK(BM_ContractValidateGetValid) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:298
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ContractValidateGetValid): n/a

#### `BENCHMARK(BM_ContractValidatePostValid) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ContractValidatePostValid): n/a

#### `BENCHMARK(BM_ContractValidateUnsupportedVersion) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ContractValidateUnsupportedVersion): n/a

#### `BENCHMARK(BM_ErrorTaxonomyToErrorCode) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ErrorTaxonomyToErrorCode): n/a

#### `BENCHMARK(BM_ErrorTaxonomyToHttpStatus) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:281
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ErrorTaxonomyToHttpStatus): n/a

#### `BENCHMARK(BM_PolicyGetRequestHappyPath) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PolicyGetRequestHappyPath): n/a

#### `BENCHMARK(BM_PolicyGetRequestWithVersionHeader) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PolicyGetRequestWithVersionHeader): n/a

#### `BENCHMARK(BM_PolicyMixedRequestTypes) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:406
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PolicyMixedRequestTypes): n/a

#### `BENCHMARK(BM_PolicyPayloadRejection) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PolicyPayloadRejection): n/a

#### `BENCHMARK(BM_PolicyPostMediumBody) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PolicyPostMediumBody): n/a

#### `BENCHMARK(BM_PolicyPostSmallBody) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PolicyPostSmallBody): n/a

#### `BENCHMARK(BM_PolicySustainedGetThroughput) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:351
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PolicySustainedGetThroughput): n/a

#### `BENCHMARK(BM_PolicySustainedPostThroughput) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:375
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PolicySustainedPostThroughput): n/a

#### `BENCHMARK(BM_PolicyVersionRejection) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PolicyVersionRejection): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:408
- Brief: n/a
- Parameters: none

#### `void BM_ContractValidateGetValid(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:291
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark GATE-API-06: TransportContractValidator::validate — valid GET

#### `void BM_ContractValidatePostValid(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:303
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark GATE-API-06: TransportContractValidator::validate — valid POST

#### `void BM_ContractValidateUnsupportedVersion(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:317
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark GATE-API-06: TransportContractValidator::validate — invalid version

#### `void BM_ErrorTaxonomyToErrorCode(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:242
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark GATE-API-05: ApiErrorTaxonomy::toErrorCode — all failure classes Taxonomy mapping must be O(1) per call (switch statement). This benchmark cycles through all failure classes to ensure the hot path has no branches that add surprising overhead.

#### `void BM_ErrorTaxonomyToHttpStatus(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:264
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark GATE-API-05: ApiErrorTaxonomy::toHttpStatus — all failure classes

#### `void BM_PolicyGetRequestHappyPath(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:91
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark GATE-API-01: TransportPolicyMiddleware — valid GET (no version header) Measures the overhead introduced by the policy enforcement layer for the simplest possible valid request: a GET with no special headers.

#### `void BM_PolicyGetRequestWithVersionHeader(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:110
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark GATE-API-01: TransportPolicyMiddleware — valid GET with v1 version header Adds version-header lookup cost to the GET happy path.

#### `void BM_PolicyMixedRequestTypes(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:382
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API policy: mixed request types — GET / POST / DELETE interleaved Models realistic API traffic with a mix of read, write, and delete operations.

#### `void BM_PolicyPayloadRejection(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:185
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark GATE-API-03: TransportPolicyMiddleware — payload rejection (oversized body) Validates that payload-size enforcement exits early without examining the body contents. The rejection path must be as cheap as the happy path.

#### `void BM_PolicyPostMediumBody(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:157
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark GATE-API-02: TransportPolicyMiddleware — valid POST 64 KiB payload Representative of a batch-write or import operation.

#### `void BM_PolicyPostSmallBody(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:136
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark GATE-API-02: TransportPolicyMiddleware — valid POST 1 KiB JSON payload Exercises payload-size check, Content-Type enforcement, and delegation for a small but realistic POST body.

#### `void BM_PolicySustainedGetThroughput(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:336
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API policy: sustained throughput — 100 sequential requests Measures wall-clock time for 100 sequential GET requests through the policy middleware. Used to establish the p95/p99 baseline latency per request.

#### `void BM_PolicySustainedPostThroughput(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:358
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API policy: sustained throughput — 100 sequential POST requests Representative batch-ingest scenario with JSON bodies.

#### `void BM_PolicyVersionRejection(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_release_gates.cpp`:215
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark GATE-API-04: TransportPolicyMiddleware — version rejection Measures the overhead of rejecting a request with an unsupported X-API-Version header. Should be at most as expensive as accepting a valid version (the loop over kSupportedApiVersions is O(\|versions\|) = O(2)).

### bench_api_transport.cpp

#### `BENCHMARK(BM_TransportCorrelationIdPropagation) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportCorrelationIdPropagation): n/a

#### `BENCHMARK(BM_TransportErrorMessageConstruction) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportErrorMessageConstruction): n/a

#### `BENCHMARK(BM_TransportHandleGetRequest) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportHandleGetRequest): n/a

#### `BENCHMARK(BM_TransportHandlePostMedium) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportHandlePostMedium): n/a

#### `BENCHMARK(BM_TransportHandlePostSmall) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportHandlePostSmall): n/a

#### `BENCHMARK(BM_TransportHandleWithManyHeaders) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportHandleWithManyHeaders): n/a

#### `BENCHMARK(BM_TransportHeaderLookup) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportHeaderLookup): n/a

#### `BENCHMARK(BM_TransportLongPath) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:306
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportLongPath): n/a

#### `BENCHMARK(BM_TransportResponseHeaderConstruction) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportResponseHeaderConstruction): n/a

#### `BENCHMARK(BM_TransportSequentialRequests) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:283
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportSequentialRequests): n/a

#### `BENCHMARK(BM_TransportSerializeSmallResponse) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportSerializeSmallResponse): n/a

#### `BENCHMARK(BM_TransportValidateMalformedRequest) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportValidateMalformedRequest): n/a

#### `BENCHMARK(BM_TransportVersionNegotiation) -> UseRealTime()`
- Source: `benchmarks/api/bench_api_transport.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransportVersionNegotiation): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/api/bench_api_transport.cpp`:326
- Brief: n/a
- Parameters: none

#### `void BM_TransportCorrelationIdPropagation(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:223
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: Correlation ID propagation Measures overhead of propagating correlation IDs through request/response cycle

#### `void BM_TransportErrorMessageConstruction(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:204
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: Error message construction Measures overhead of generating error responses

#### `void BM_TransportHandleGetRequest(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:75
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: GET request handling Measures time to validate and process a simple GET request

#### `void BM_TransportHandlePostMedium(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:107
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: POST request with medium payload Measures time to validate and process a POST request with 100KB payload

#### `void BM_TransportHandlePostSmall(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:90
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: POST request with small payload Measures time to validate and process a POST request with 1KB payload

#### `void BM_TransportHandleWithManyHeaders(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:124
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: Request with extended headers Measures overhead of processing requests with many custom headers

#### `void BM_TransportHeaderLookup(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:246
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: Header lookup performance Measures performance of common header lookups

#### `void BM_TransportLongPath(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:293
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: Long path handling Measures performance with very long request paths

#### `void BM_TransportResponseHeaderConstruction(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:165
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: Response header construction Measures time to build and populate response headers

#### `void BM_TransportSequentialRequests(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:272
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: Sequential request handling Measures baseline throughput of sequential request processing

#### `void BM_TransportSerializeSmallResponse(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:148
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: Small JSON response serialization Measures time to construct and return a small JSON response

#### `void BM_TransportValidateMalformedRequest(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:187
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: Validation of malformed request Measures time to detect and reject a malformed request

#### `void BM_TransportVersionNegotiation(benchmark::State &state)`
- Source: `benchmarks/api/bench_api_transport.cpp`:312
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: @benchmark API transport: Version negotiation performance Measures performance of version header processing and validation

### std::hash< themis::api::CorrelationId >

#### `std::size_t operator()(const themis::api::CorrelationId &id) const noexcept`
- Source: `include/api/correlation_id.h`:198
- Brief: n/a
- Parameters:
  - `id` (const themis::api::CorrelationId &): n/a

### test_api_auth_config.cpp

#### `TEST_F(ApiAuthConfigTest, AllSecuritySensitiveEndpointsRequireAuth)`
- Source: `tests/api/test_api_auth_config.cpp`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (AllSecuritySensitiveEndpointsRequireAuth): n/a

#### `TEST_F(ApiAuthConfigTest, CustomEndpointConfiguration)`
- Source: `tests/api/test_api_auth_config.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (CustomEndpointConfiguration): n/a

#### `TEST_F(ApiAuthConfigTest, DevDefaultsHaveAuthDisabled)`
- Source: `tests/api/test_api_auth_config.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (DevDefaultsHaveAuthDisabled): n/a

#### `TEST_F(ApiAuthConfigTest, EmptyPatternIsHandledSafely)`
- Source: `tests/api/test_api_auth_config.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (EmptyPatternIsHandledSafely): n/a

#### `TEST_F(ApiAuthConfigTest, ExactMatchTakesPrecedenceOverWildcard)`
- Source: `tests/api/test_api_auth_config.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (ExactMatchTakesPrecedenceOverWildcard): n/a

#### `TEST_F(ApiAuthConfigTest, GetEndpointConfigExactMatch)`
- Source: `tests/api/test_api_auth_config.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (GetEndpointConfigExactMatch): n/a

#### `TEST_F(ApiAuthConfigTest, GetEndpointConfigMultiLevelWildcard)`
- Source: `tests/api/test_api_auth_config.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (GetEndpointConfigMultiLevelWildcard): n/a

#### `TEST_F(ApiAuthConfigTest, GetEndpointConfigNoMatch)`
- Source: `tests/api/test_api_auth_config.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (GetEndpointConfigNoMatch): n/a

#### `TEST_F(ApiAuthConfigTest, GetEndpointConfigWildcardMatch)`
- Source: `tests/api/test_api_auth_config.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (GetEndpointConfigWildcardMatch): n/a

#### `TEST_F(ApiAuthConfigTest, HighTrafficEndpointsHaveHigherLimits)`
- Source: `tests/api/test_api_auth_config.cpp`:222
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (HighTrafficEndpointsHaveHigherLimits): n/a

#### `TEST_F(ApiAuthConfigTest, HttpMethodDifferentiation)`
- Source: `tests/api/test_api_auth_config.cpp`:369
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (HttpMethodDifferentiation): n/a

#### `TEST_F(ApiAuthConfigTest, MostSpecificWildcardMatching)`
- Source: `tests/api/test_api_auth_config.cpp`:396
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (MostSpecificWildcardMatching): n/a

#### `TEST_F(ApiAuthConfigTest, PublicEndpointsDoNotRequireAuth)`
- Source: `tests/api/test_api_auth_config.cpp`:266
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (PublicEndpointsDoNotRequireAuth): n/a

#### `TEST_F(ApiAuthConfigTest, RateLimitsAreReasonable)`
- Source: `tests/api/test_api_auth_config.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (RateLimitsAreReasonable): n/a

#### `TEST_F(ApiAuthConfigTest, SecureDefaultsHaveAuthEnabled)`
- Source: `tests/api/test_api_auth_config.cpp`:28
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (SecureDefaultsHaveAuthEnabled): n/a

#### `TEST_F(ApiAuthConfigTest, SecureDefaultsIncludeEndpointConfigs)`
- Source: `tests/api/test_api_auth_config.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (SecureDefaultsIncludeEndpointConfigs): n/a

#### `TEST_F(ApiAuthConfigTest, SensitiveEndpointsHaveRestrictiveLimits)`
- Source: `tests/api/test_api_auth_config.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (SensitiveEndpointsHaveRestrictiveLimits): n/a

#### `TEST_F(ApiAuthConfigTest, WildcardMethodMatching)`
- Source: `tests/api/test_api_auth_config.cpp`:428
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiAuthConfigTest): n/a
  - `<unnamed>` (WildcardMethodMatching): n/a

### test_api_contracts.cpp

#### `TEST(APIContractsTest, AuthenticationRequiredByDefault)`
- Source: `tests/api/test_api_contracts.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (AuthenticationRequiredByDefault): n/a
- Details: TestAuthentication Contract All transport entry points enforce authentication requirement

#### `TEST(APIContractsTest, BackwardCompatibilityV1Supported)`
- Source: `tests/api/test_api_contracts.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (BackwardCompatibilityV1Supported): n/a
- Details: TestBackward Compatibility Contract Transport-facing contracts remain backward compatible within major release line

#### `TEST(APIContractsTest, BackwardCompatibilityV2Supported)`
- Source: `tests/api/test_api_contracts.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (BackwardCompatibilityV2Supported): n/a
- Details: TestBackward Compatibility Contract New versions remain supported during major line

#### `TEST(APIContractsTest, ErrorSemanticsBadRequest)`
- Source: `tests/api/test_api_contracts.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (ErrorSemanticsBadRequest): n/a
- Details: TestError Contract: Consistent error semantics Error responses follow standard HTTP status code semantics

#### `TEST(APIContractsTest, ErrorSemanticsForbidden)`
- Source: `tests/api/test_api_contracts.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (ErrorSemanticsForbidden): n/a
- Details: TestError Contract: Consistent error semantics Forbidden responses must have 403 status

#### `TEST(APIContractsTest, ErrorSemanticsInternalError)`
- Source: `tests/api/test_api_contracts.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (ErrorSemanticsInternalError): n/a
- Details: TestError Contract: Consistent error semantics Server errors must have 500 status

#### `TEST(APIContractsTest, ErrorSemanticsNotFound)`
- Source: `tests/api/test_api_contracts.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (ErrorSemanticsNotFound): n/a
- Details: TestError Contract: Consistent error semantics Not found responses must have 404 status

#### `TEST(APIContractsTest, ErrorSemanticsUnauthorized)`
- Source: `tests/api/test_api_contracts.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (ErrorSemanticsUnauthorized): n/a
- Details: TestError Contract: Consistent error semantics Unauthorized responses must have 401 status

#### `TEST(APIContractsTest, FailClosedOnEmptyMethod)`
- Source: `tests/api/test_api_contracts.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (FailClosedOnEmptyMethod): n/a
- Details: TestFail-Closed Behavior Contract Adapter behavior remains fail-closed on invalid or unsupported protocol input

#### `TEST(APIContractsTest, FailClosedOnUnsupportedVersion)`
- Source: `tests/api/test_api_contracts.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (FailClosedOnUnsupportedVersion): n/a
- Details: TestFail-Closed Behavior Contract Adapter behavior remains fail-closed on unsupported protocol states

#### `TEST(APIContractsTest, FutureEnhancementBackwardCompatibility)`
- Source: `tests/api/test_api_contracts.cpp`:520
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (FutureEnhancementBackwardCompatibility): n/a
- Details: TestFuture Enhancement: Transport-facing contracts remain backward compatible API surfaces maintain stability within major version line

#### `TEST(APIContractsTest, FutureEnhancementConcurrencyBounding)`
- Source: `tests/api/test_api_contracts.cpp`:573
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (FutureEnhancementConcurrencyBounding): n/a
- Details: TestFuture Enhancement: High-concurrency paths remain bounded Middleware chains support bounded configuration

#### `TEST(APIContractsTest, FutureEnhancementFailClosedBehavior)`
- Source: `tests/api/test_api_contracts.cpp`:545
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (FutureEnhancementFailClosedBehavior): n/a
- Details: TestFuture Enhancement: Adapter behavior remains fail-closed on invalid input Invalid protocol states are rejected before processing

#### `TEST(APIContractsTest, FutureEnhancementObservabilityNonIntrusive)`
- Source: `tests/api/test_api_contracts.cpp`:590
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (FutureEnhancementObservabilityNonIntrusive): n/a
- Details: TestFuture Enhancement: Observability integration non-intrusive Correlation IDs and observability don't compromise request path

#### `TEST(APIContractsTest, GRPCServiceDescriptorContractMet)`
- Source: `tests/api/test_api_contracts.cpp`:331
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (GRPCServiceDescriptorContractMet): n/a
- Details: TestgRPC Service Contract Service descriptors must properly reflect method metadata

#### `TEST(APIContractsTest, GraphQLSchemaValidationContractMet)`
- Source: `tests/api/test_api_contracts.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (GraphQLSchemaValidationContractMet): n/a
- Details: TestGraphQL Schema Validation Contract Schemas must validate correctly for all standard types

#### `TEST(APIContractsTest, HTTPHandlerInterfaceContractMet)`
- Source: `tests/api/test_api_contracts.cpp`:400
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (HTTPHandlerInterfaceContractMet): n/a
- Details: TestHTTP Handler Interface Contract All HTTP handlers must implement required interface methods

#### `TEST(APIContractsTest, MiddlewareConcurrencyBounding)`
- Source: `tests/api/test_api_contracts.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (MiddlewareConcurrencyBounding): n/a
- Details: TestResource Bounding: Middleware chain size High-concurrency paths remain bounded by explicit runtime controls

#### `TEST(APIContractsTest, MiddlewareErrorHandlingUnderLoad)`
- Source: `tests/api/test_api_contracts.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (MiddlewareErrorHandlingUnderLoad): n/a
- Details: TestResource Bounding: Error handling under load Middleware chain must fail-closed even under degraded conditions

#### `TEST(APIContractsTest, ObservabilityCorrelationIDContract)`
- Source: `tests/api/test_api_contracts.cpp`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (ObservabilityCorrelationIDContract): n/a
- Details: TestObservability Contract: Correlation ID Correlation IDs must be deterministic and parseable

#### `TEST(APIContractsTest, ObservabilityCorrelationIDRoundTrip)`
- Source: `tests/api/test_api_contracts.cpp`:300
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (ObservabilityCorrelationIDRoundTrip): n/a
- Details: TestObservability Contract: Correlation ID can be round-tripped Correlation IDs maintain semantic identity across serialization

#### `TEST(APIContractsTest, RoadmapAcceptanceAdapterSurfacesVerified)`
- Source: `tests/api/test_api_contracts.cpp`:447
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (RoadmapAcceptanceAdapterSurfacesVerified): n/a
- Details: TestRoadmap Acceptance: Transport adapter surfaces documented and source-verified All adapter error semantics are properly defined

#### `TEST(APIContractsTest, RoadmapAcceptanceCoreDocsAligned)`
- Source: `tests/api/test_api_contracts.cpp`:424
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (RoadmapAcceptanceCoreDocsAligned): n/a
- Details: TestRoadmap Acceptance: Core API docs aligned to source-verifiable behavior All public transport adapter surfaces have documented behavior

#### `TEST(APIContractsTest, RoadmapAcceptanceSecurityAndFailureHandling)`
- Source: `tests/api/test_api_contracts.cpp`:487
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (RoadmapAcceptanceSecurityAndFailureHandling): n/a
- Details: TestRoadmap Acceptance: Security and failure handling documented at module level All required failure handling contracts are implemented

#### `TEST(APIContractsTest, SuccessCreatedResponseContractMet)`
- Source: `tests/api/test_api_contracts.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (SuccessCreatedResponseContractMet): n/a
- Details: TestSuccess Contract: Standard response codes Created responses must have 201 status with JSON content type

#### `TEST(APIContractsTest, SuccessNoContentResponseContractMet)`
- Source: `tests/api/test_api_contracts.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (SuccessNoContentResponseContractMet): n/a
- Details: TestSuccess Contract: Standard response codes NoContent responses must have 204 status with empty body

#### `TEST(APIContractsTest, SuccessOkResponseContractMet)`
- Source: `tests/api/test_api_contracts.cpp`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (SuccessOkResponseContractMet): n/a
- Details: TestSuccess Contract: Standard response codes OK responses must have 200 status with JSON content type

#### `TEST(APIContractsTest, VersionDescriptorCurrentContractMet)`
- Source: `tests/api/test_api_contracts.cpp`:368
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (VersionDescriptorCurrentContractMet): n/a
- Details: TestAPI Version Contract Current versions must be properly described

#### `TEST(APIContractsTest, VersionDescriptorDeprecatedContractMet)`
- Source: `tests/api/test_api_contracts.cpp`:381
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (VersionDescriptorDeprecatedContractMet): n/a
- Details: TestAPI Version Contract Deprecated versions must include sunset and migration information

#### `TEST(APIContractsTest, WebSocketFrameContractMet)`
- Source: `tests/api/test_api_contracts.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIContractsTest): n/a
  - `<unnamed>` (WebSocketFrameContractMet): n/a
- Details: TestWebSocket Frame Contract WebSocket frames must support both text and binary payloads

### test_api_degraded_mode.cpp

#### `TEST(DegradedModeDiagnosticsTest, AllFailureClassesHaveActionableMessages)`
- Source: `tests/api/test_api_degraded_mode.cpp`:378
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedModeDiagnosticsTest): n/a
  - `<unnamed>` (AllFailureClassesHaveActionableMessages): n/a
- Details: TestEvery TransportFailureClass produces a non-empty structured message Validates the Q4 2026 "extend integration diagnostics for protocol-level failure classes" requirement: every failure class must produce an actionable, non-empty message string that operators can triage.

#### `TEST(DegradedModeTest, CapabilityFlagsReflectDeploymentState)`
- Source: `tests/api/test_api_degraded_mode.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedModeTest): n/a
  - `<unnamed>` (CapabilityFlagsReflectDeploymentState): n/a
- Details: TestCapability flags are correct when capability is enabled vs disabled

#### `TEST(DegradedModeTest, CapabilityUnavailableRejectsGatedPaths)`
- Source: `tests/api/test_api_degraded_mode.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedModeTest): n/a
  - `<unnamed>` (CapabilityUnavailableRejectsGatedPaths): n/a
- Details: TestRequests to capability-gated paths are rejected when capability is disabled Models gRPC streaming or WebSocket subscription unavailability in a deployment where the feature has not been enabled via configuration.

#### `TEST(DegradedModeTest, ConcurrentMixedRequestsDegradedAdapter)`
- Source: `tests/api/test_api_degraded_mode.cpp`:318
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedModeTest): n/a
  - `<unnamed>` (ConcurrentMixedRequestsDegradedAdapter): n/a
- Details: Test8 threads mix non-gated (success) and gated (failure) requests concurrently Validates that the degraded adapter handles mixed concurrent traffic without data races and produces the correct error vs success split.

#### `TEST(DegradedModeTest, NonGatedPathsSucceedWhenCapabilityDisabled)`
- Source: `tests/api/test_api_degraded_mode.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedModeTest): n/a
  - `<unnamed>` (NonGatedPathsSucceedWhenCapabilityDisabled): n/a
- Details: TestNon-gated paths succeed even when the optional capability is disabled

#### `TEST(DegradedModeTest, PolicyComposesWithDegradedAdapter)`
- Source: `tests/api/test_api_degraded_mode.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedModeTest): n/a
  - `<unnamed>` (PolicyComposesWithDegradedAdapter): n/a
- Details: TestTransportPolicyMiddleware composes correctly with a degraded inner adapter Valid requests pass the policy layer and hit the degraded inner adapter. Invalid requests are rejected by the policy layer without touching the adapter.

#### `TEST(DegradedModeTest, SubscribePathRejectedWhenCapabilityDisabled)`
- Source: `tests/api/test_api_degraded_mode.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedModeTest): n/a
  - `<unnamed>` (SubscribePathRejectedWhenCapabilityDisabled): n/a
- Details: TestSubscribe path is also rejected when capability is disabled

#### `TEST(DegradedModeTest, TransientFailureAdapterDegradationBehavior)`
- Source: `tests/api/test_api_degraded_mode.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedModeTest): n/a
  - `<unnamed>` (TransientFailureAdapterDegradationBehavior): n/a
- Details: TestRequests succeed before degradation threshold; fail after threshold Verifies that callers can detect degradation via error codes and that the adapter provides a well-formed error response for every post-threshold request.

### test_api_error_handling.cpp

#### `TEST(DegradedModeTest, PartialCapabilityAvailability)`
- Source: `tests/api/test_api_error_handling.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedModeTest): n/a
  - `<unnamed>` (PartialCapabilityAvailability): n/a
- Details: TestGraceful degradation: partial capability availability System continues functioning with reduced capabilities during degraded mode

#### `TEST(DegradedModeTest, RestRequestDuringWebSocketDegradation)`
- Source: `tests/api/test_api_error_handling.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedModeTest): n/a
  - `<unnamed>` (RestRequestDuringWebSocketDegradation): n/a
- Details: TestREST request in WebSocket-degraded mode Non-WebSocket requests succeed even when WebSocket feature is unavailable

#### `TEST(DegradedModeTest, WebSocketFeatureUnavailable)`
- Source: `tests/api/test_api_error_handling.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (DegradedModeTest): n/a
  - `<unnamed>` (WebSocketFeatureUnavailable): n/a
- Details: TestUnsupported WebSocket feature Adapter gracefully rejects WebSocket requests when feature is unavailable

#### `TEST(ErrorHandlingTest, MalformedRequestError)`
- Source: `tests/api/test_api_error_handling.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorHandlingTest): n/a
  - `<unnamed>` (MalformedRequestError): n/a
- Details: TestMalformed request error Degraded mode adapter returns ERR_DEGRADED_MALFORMED_REQUEST for malformed requests

#### `TEST(ErrorHandlingTest, QuotaExceededError)`
- Source: `tests/api/test_api_error_handling.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorHandlingTest): n/a
  - `<unnamed>` (QuotaExceededError): n/a
- Details: TestQuota exceeded error Adapter returns ERR_DEGRADED_QUOTA_EXCEEDED when limits are reached

#### `TEST(ErrorHandlingTest, ServiceUnavailableError)`
- Source: `tests/api/test_api_error_handling.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorHandlingTest): n/a
  - `<unnamed>` (ServiceUnavailableError): n/a
- Details: TestService unavailable error Adapter returns ERR_DEGRADED_SERVICE_UNAVAILABLE for backend failures

#### `TEST(ErrorHandlingTest, TimeoutError)`
- Source: `tests/api/test_api_error_handling.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorHandlingTest): n/a
  - `<unnamed>` (TimeoutError): n/a
- Details: TestTimeout error Adapter returns ERR_DEGRADED_TIMEOUT with bounded timeout value

#### `TEST(ErrorMessageQualityTest, DescriptiveErrorMessages)`
- Source: `tests/api/test_api_error_handling.cpp`:346
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorMessageQualityTest): n/a
  - `<unnamed>` (DescriptiveErrorMessages): n/a
- Details: TestError messages are descriptive All error messages include error code, description, and helpful context

#### `TEST(ErrorMessageQualityTest, ErrorMessagesSafety)`
- Source: `tests/api/test_api_error_handling.cpp`:375
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorMessageQualityTest): n/a
  - `<unnamed>` (ErrorMessagesSafety): n/a
- Details: TestError messages do not leak sensitive information Error messages include only public-safe information

#### `TEST(ErrorRecoveryTest, PermanentUnsupportedFeatureError)`
- Source: `tests/api/test_api_error_handling.cpp`:293
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorRecoveryTest): n/a
  - `<unnamed>` (PermanentUnsupportedFeatureError): n/a
- Details: TestPermanent error: Unsupported feature Client must not retry unsupported feature errors

#### `TEST(ErrorRecoveryTest, QuotaErrorRecovery)`
- Source: `tests/api/test_api_error_handling.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorRecoveryTest): n/a
  - `<unnamed>` (QuotaErrorRecovery): n/a
- Details: TestQuota error recovery After quota reset, requests should succeed

#### `TEST(ErrorRecoveryTest, TransientServiceUnavailableRecovery)`
- Source: `tests/api/test_api_error_handling.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErrorRecoveryTest): n/a
  - `<unnamed>` (TransientServiceUnavailableRecovery): n/a
- Details: TestTransient error: Service unavailable recovery Client can retry after ERR_DEGRADED_SERVICE_UNAVAILABLE

### test_api_grpc_server.cpp

#### `TEST(GrpcApiServerTest, GrpcDisabledSkip)`
- Source: `tests/api/test_api_grpc_server.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcApiServerTest): n/a
  - `<unnamed>` (GrpcDisabledSkip): n/a

### test_api_interfaces.cpp

#### `TEST(CorrelationIdTest, ByteSize)`
- Source: `tests/api/test_api_interfaces.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (CorrelationIdTest): n/a
  - `<unnamed>` (ByteSize): n/a

#### `TEST(CorrelationIdTest, EqualityAndInequality)`
- Source: `tests/api/test_api_interfaces.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (CorrelationIdTest): n/a
  - `<unnamed>` (EqualityAndInequality): n/a

#### `TEST(CorrelationIdTest, Hashable)`
- Source: `tests/api/test_api_interfaces.cpp`:339
- Brief: n/a
- Parameters:
  - `<unnamed>` (CorrelationIdTest): n/a
  - `<unnamed>` (Hashable): n/a

#### `TEST(CorrelationIdTest, NilDefault)`
- Source: `tests/api/test_api_interfaces.cpp`:295
- Brief: n/a
- Parameters:
  - `<unnamed>` (CorrelationIdTest): n/a
  - `<unnamed>` (NilDefault): n/a

#### `TEST(CorrelationIdTest, ParseAndSerialiseRoundTrip)`
- Source: `tests/api/test_api_interfaces.cpp`:301
- Brief: n/a
- Parameters:
  - `<unnamed>` (CorrelationIdTest): n/a
  - `<unnamed>` (ParseAndSerialiseRoundTrip): n/a

#### `TEST(CorrelationIdTest, ParseInvalidThrows)`
- Source: `tests/api/test_api_interfaces.cpp`:325
- Brief: n/a
- Parameters:
  - `<unnamed>` (CorrelationIdTest): n/a
  - `<unnamed>` (ParseInvalidThrows): n/a

#### `TEST(CorrelationIdTest, ParseUppercase)`
- Source: `tests/api/test_api_interfaces.cpp`:309
- Brief: n/a
- Parameters:
  - `<unnamed>` (CorrelationIdTest): n/a
  - `<unnamed>` (ParseUppercase): n/a

#### `TEST(CorrelationIdTest, ParseWithoutDashes)`
- Source: `tests/api/test_api_interfaces.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (CorrelationIdTest): n/a
  - `<unnamed>` (ParseWithoutDashes): n/a

#### `TEST(GRPCMetadataTest, DeadlineDetection)`
- Source: `tests/api/test_api_interfaces.cpp`:367
- Brief: n/a
- Parameters:
  - `<unnamed>` (GRPCMetadataTest): n/a
  - `<unnamed>` (DeadlineDetection): n/a

#### `TEST(GRPCMetadataTest, UserMetadata)`
- Source: `tests/api/test_api_interfaces.cpp`:375
- Brief: n/a
- Parameters:
  - `<unnamed>` (GRPCMetadataTest): n/a
  - `<unnamed>` (UserMetadata): n/a

#### `TEST(GraphQLTypeDescriptorTest, FieldConstruction)`
- Source: `tests/api/test_api_interfaces.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (GraphQLTypeDescriptorTest): n/a
  - `<unnamed>` (FieldConstruction): n/a

#### `TEST(GraphQLTypeDescriptorTest, TypeConstruction)`
- Source: `tests/api/test_api_interfaces.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (GraphQLTypeDescriptorTest): n/a
  - `<unnamed>` (TypeConstruction): n/a

#### `TEST(HttpRequestTest, DefaultConstruct)`
- Source: `tests/api/test_api_interfaces.cpp`:26
- Brief: n/a
- Parameters:
  - `<unnamed>` (HttpRequestTest): n/a
  - `<unnamed>` (DefaultConstruct): n/a

#### `TEST(HttpRequestTest, HasAuthFromHeader)`
- Source: `tests/api/test_api_interfaces.cpp`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (HttpRequestTest): n/a
  - `<unnamed>` (HasAuthFromHeader): n/a

#### `TEST(HttpRequestTest, HeaderLookup)`
- Source: `tests/api/test_api_interfaces.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (HttpRequestTest): n/a
  - `<unnamed>` (HeaderLookup): n/a

#### `TEST(HttpResponseTest, BadRequestFactory)`
- Source: `tests/api/test_api_interfaces.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (HttpResponseTest): n/a
  - `<unnamed>` (BadRequestFactory): n/a

#### `TEST(HttpResponseTest, CreatedFactory)`
- Source: `tests/api/test_api_interfaces.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (HttpResponseTest): n/a
  - `<unnamed>` (CreatedFactory): n/a

#### `TEST(HttpResponseTest, ForbiddenFactory)`
- Source: `tests/api/test_api_interfaces.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (HttpResponseTest): n/a
  - `<unnamed>` (ForbiddenFactory): n/a

#### `TEST(HttpResponseTest, InternalErrorFactory)`
- Source: `tests/api/test_api_interfaces.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (HttpResponseTest): n/a
  - `<unnamed>` (InternalErrorFactory): n/a

#### `TEST(HttpResponseTest, NoContentFactory)`
- Source: `tests/api/test_api_interfaces.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (HttpResponseTest): n/a
  - `<unnamed>` (NoContentFactory): n/a

#### `TEST(HttpResponseTest, NotFoundFactory)`
- Source: `tests/api/test_api_interfaces.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (HttpResponseTest): n/a
  - `<unnamed>` (NotFoundFactory): n/a

#### `TEST(HttpResponseTest, OkFactory)`
- Source: `tests/api/test_api_interfaces.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (HttpResponseTest): n/a
  - `<unnamed>` (OkFactory): n/a

#### `TEST(HttpResponseTest, UnauthorizedFactory)`
- Source: `tests/api/test_api_interfaces.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (HttpResponseTest): n/a
  - `<unnamed>` (UnauthorizedFactory): n/a

#### `TEST(MiddlewareChainTest, EmptyChainReturnsError)`
- Source: `tests/api/test_api_interfaces.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (MiddlewareChainTest): n/a
  - `<unnamed>` (EmptyChainReturnsError): n/a

#### `TEST(MiddlewareChainTest, NoAuthIfAllLinksOptOut)`
- Source: `tests/api/test_api_interfaces.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (MiddlewareChainTest): n/a
  - `<unnamed>` (NoAuthIfAllLinksOptOut): n/a

#### `TEST(MiddlewareChainTest, PassThroughLeadsToTerminalHandlerResponse)`
- Source: `tests/api/test_api_interfaces.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (MiddlewareChainTest): n/a
  - `<unnamed>` (PassThroughLeadsToTerminalHandlerResponse): n/a

#### `TEST(MiddlewareChainTest, RejectingFirstHandlerShortCircuits)`
- Source: `tests/api/test_api_interfaces.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (MiddlewareChainTest): n/a
  - `<unnamed>` (RejectingFirstHandlerShortCircuits): n/a

#### `TEST(MiddlewareChainTest, RequiresAuthIfAnyLinkRequiresIt)`
- Source: `tests/api/test_api_interfaces.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (MiddlewareChainTest): n/a
  - `<unnamed>` (RequiresAuthIfAnyLinkRequiresIt): n/a

#### `TEST(MiddlewareChainTest, SingleHandlerReturnsResponse)`
- Source: `tests/api/test_api_interfaces.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (MiddlewareChainTest): n/a
  - `<unnamed>` (SingleHandlerReturnsResponse): n/a

#### `TEST(MiddlewareChainTest, SizeReflectsAppends)`
- Source: `tests/api/test_api_interfaces.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (MiddlewareChainTest): n/a
  - `<unnamed>` (SizeReflectsAppends): n/a

#### `TEST(SchemaValidationResultTest, FailResult)`
- Source: `tests/api/test_api_interfaces.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidationResultTest): n/a
  - `<unnamed>` (FailResult): n/a

#### `TEST(SchemaValidationResultTest, OkResult)`
- Source: `tests/api/test_api_interfaces.cpp`:203
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidationResultTest): n/a
  - `<unnamed>` (OkResult): n/a

#### `TEST(ServiceDescriptorTest, DefaultSerialization)`
- Source: `tests/api/test_api_interfaces.cpp`:357
- Brief: n/a
- Parameters:
  - `<unnamed>` (ServiceDescriptorTest): n/a
  - `<unnamed>` (DefaultSerialization): n/a

#### `TEST(VersionDescriptorTest, CurrentVersion)`
- Source: `tests/api/test_api_interfaces.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (VersionDescriptorTest): n/a
  - `<unnamed>` (CurrentVersion): n/a

#### `TEST(VersionDescriptorTest, DeprecatedVersion)`
- Source: `tests/api/test_api_interfaces.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (VersionDescriptorTest): n/a
  - `<unnamed>` (DeprecatedVersion): n/a

#### `TEST(WebSocketCloseCodeTest, Values)`
- Source: `tests/api/test_api_interfaces.cpp`:245
- Brief: n/a
- Parameters:
  - `<unnamed>` (WebSocketCloseCodeTest): n/a
  - `<unnamed>` (Values): n/a

#### `TEST(WebSocketFrameTest, BinaryFactory)`
- Source: `tests/api/test_api_interfaces.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (WebSocketFrameTest): n/a
  - `<unnamed>` (BinaryFactory): n/a

#### `TEST(WebSocketFrameTest, TextFactory)`
- Source: `tests/api/test_api_interfaces.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (WebSocketFrameTest): n/a
  - `<unnamed>` (TextFactory): n/a

### test_api_key_authenticator.cpp

#### `TEST(ApiKeyAuthenticatorTest, AddCredential_AcceptsValidCredential)`
- Source: `tests/api/test_api_key_authenticator.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (AddCredential_AcceptsValidCredential): n/a

#### `TEST(ApiKeyAuthenticatorTest, AddCredential_RejectsBadHashLength)`
- Source: `tests/api/test_api_key_authenticator.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (AddCredential_RejectsBadHashLength): n/a

#### `TEST(ApiKeyAuthenticatorTest, AddCredential_RejectsEmptyKeyId)`
- Source: `tests/api/test_api_key_authenticator.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (AddCredential_RejectsEmptyKeyId): n/a

#### `TEST(ApiKeyAuthenticatorTest, AddCredential_ReplacesExisting)`
- Source: `tests/api/test_api_key_authenticator.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (AddCredential_ReplacesExisting): n/a

#### `TEST(ApiKeyAuthenticatorTest, AuthenticateCombined_LeadingDotThrows)`
- Source: `tests/api/test_api_key_authenticator.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (AuthenticateCombined_LeadingDotThrows): n/a

#### `TEST(ApiKeyAuthenticatorTest, AuthenticateCombined_NoDotThrows)`
- Source: `tests/api/test_api_key_authenticator.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (AuthenticateCombined_NoDotThrows): n/a

#### `TEST(ApiKeyAuthenticatorTest, AuthenticateCombined_SplitsOnFirstDot)`
- Source: `tests/api/test_api_key_authenticator.cpp`:301
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (AuthenticateCombined_SplitsOnFirstDot): n/a

#### `TEST(ApiKeyAuthenticatorTest, AuthenticateCombined_Success)`
- Source: `tests/api/test_api_key_authenticator.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (AuthenticateCombined_Success): n/a

#### `TEST(ApiKeyAuthenticatorTest, AuthenticateCombined_TrailingDotThrows)`
- Source: `tests/api/test_api_key_authenticator.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (AuthenticateCombined_TrailingDotThrows): n/a

#### `TEST(ApiKeyAuthenticatorTest, Authenticate_EmptyKeyIdThrows)`
- Source: `tests/api/test_api_key_authenticator.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (Authenticate_EmptyKeyIdThrows): n/a

#### `TEST(ApiKeyAuthenticatorTest, Authenticate_EmptySecretThrows)`
- Source: `tests/api/test_api_key_authenticator.cpp`:178
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (Authenticate_EmptySecretThrows): n/a

#### `TEST(ApiKeyAuthenticatorTest, Authenticate_ExpiredKeyThrows)`
- Source: `tests/api/test_api_key_authenticator.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (Authenticate_ExpiredKeyThrows): n/a

#### `TEST(ApiKeyAuthenticatorTest, Authenticate_ExpiryCheckDisabled_ExpiredKeySucceeds)`
- Source: `tests/api/test_api_key_authenticator.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (Authenticate_ExpiryCheckDisabled_ExpiredKeySucceeds): n/a

#### `TEST(ApiKeyAuthenticatorTest, Authenticate_FutureKeyNotExpired)`
- Source: `tests/api/test_api_key_authenticator.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (Authenticate_FutureKeyNotExpired): n/a

#### `TEST(ApiKeyAuthenticatorTest, Authenticate_InactiveKeyThrows)`
- Source: `tests/api/test_api_key_authenticator.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (Authenticate_InactiveKeyThrows): n/a

#### `TEST(ApiKeyAuthenticatorTest, Authenticate_KeyIdTooLongThrows)`
- Source: `tests/api/test_api_key_authenticator.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (Authenticate_KeyIdTooLongThrows): n/a

#### `TEST(ApiKeyAuthenticatorTest, Authenticate_NoScopesOrRoles)`
- Source: `tests/api/test_api_key_authenticator.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (Authenticate_NoScopesOrRoles): n/a

#### `TEST(ApiKeyAuthenticatorTest, Authenticate_SecretTooLongThrows)`
- Source: `tests/api/test_api_key_authenticator.cpp`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (Authenticate_SecretTooLongThrows): n/a

#### `TEST(ApiKeyAuthenticatorTest, Authenticate_SuccessReturnsCorrectClaims)`
- Source: `tests/api/test_api_key_authenticator.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (Authenticate_SuccessReturnsCorrectClaims): n/a

#### `TEST(ApiKeyAuthenticatorTest, Authenticate_UnknownKeyIdThrows)`
- Source: `tests/api/test_api_key_authenticator.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (Authenticate_UnknownKeyIdThrows): n/a

#### `TEST(ApiKeyAuthenticatorTest, Authenticate_WrongSecretThrows)`
- Source: `tests/api/test_api_key_authenticator.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (Authenticate_WrongSecretThrows): n/a

#### `TEST(ApiKeyAuthenticatorTest, CreateCredential_DefaultNoExpiry)`
- Source: `tests/api/test_api_key_authenticator.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (CreateCredential_DefaultNoExpiry): n/a

#### `TEST(ApiKeyAuthenticatorTest, CreateCredential_SetsAllFields)`
- Source: `tests/api/test_api_key_authenticator.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (CreateCredential_SetsAllFields): n/a

#### `TEST(ApiKeyAuthenticatorTest, HashSecret_DifferentSecretsDifferentHashes)`
- Source: `tests/api/test_api_key_authenticator.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (HashSecret_DifferentSecretsDifferentHashes): n/a

#### `TEST(ApiKeyAuthenticatorTest, HashSecret_EmptyStringIsValid)`
- Source: `tests/api/test_api_key_authenticator.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (HashSecret_EmptyStringIsValid): n/a

#### `TEST(ApiKeyAuthenticatorTest, HashSecret_ProducesSHA256Hex)`
- Source: `tests/api/test_api_key_authenticator.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (HashSecret_ProducesSHA256Hex): n/a

#### `TEST(ApiKeyAuthenticatorTest, RemoveCredential_NoopForUnknown)`
- Source: `tests/api/test_api_key_authenticator.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (RemoveCredential_NoopForUnknown): n/a

#### `TEST(ApiKeyAuthenticatorTest, RemoveCredential_RemovesExisting)`
- Source: `tests/api/test_api_key_authenticator.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyAuthenticatorTest): n/a
  - `<unnamed>` (RemoveCredential_RemovesExisting): n/a

#### `TEST(ApiKeyClaimsTest, HasScope_Absent)`
- Source: `tests/api/test_api_key_authenticator.cpp`:339
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyClaimsTest): n/a
  - `<unnamed>` (HasScope_Absent): n/a

#### `TEST(ApiKeyClaimsTest, HasScope_EmptyScopes)`
- Source: `tests/api/test_api_key_authenticator.cpp`:345
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyClaimsTest): n/a
  - `<unnamed>` (HasScope_EmptyScopes): n/a

#### `TEST(ApiKeyClaimsTest, HasScope_Present)`
- Source: `tests/api/test_api_key_authenticator.cpp`:332
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyClaimsTest): n/a
  - `<unnamed>` (HasScope_Present): n/a

#### `TEST(ApiKeyClaimsTest, IsExpired_FutureExpiry)`
- Source: `tests/api/test_api_key_authenticator.cpp`:320
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyClaimsTest): n/a
  - `<unnamed>` (IsExpired_FutureExpiry): n/a

#### `TEST(ApiKeyClaimsTest, IsExpired_NoExpiry)`
- Source: `tests/api/test_api_key_authenticator.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyClaimsTest): n/a
  - `<unnamed>` (IsExpired_NoExpiry): n/a

#### `TEST(ApiKeyClaimsTest, IsExpired_PastExpiry)`
- Source: `tests/api/test_api_key_authenticator.cpp`:326
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyClaimsTest): n/a
  - `<unnamed>` (IsExpired_PastExpiry): n/a

### test_api_key_mgmt_handler.cpp

#### `TEST_F(ApiKeyMgmtHandlerTest, CreateKey_ActivatesInAuthMiddleware)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (CreateKey_ActivatesInAuthMiddleware): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, CreateKey_EmptyName_ReturnsError)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (CreateKey_EmptyName_ReturnsError): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, CreateKey_ImmediatelyVisibleInList)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (CreateKey_ImmediatelyVisibleInList): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, CreateKey_MissingName_ReturnsError)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (CreateKey_MissingName_ReturnsError): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, CreateKey_NoExpiry)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (CreateKey_NoExpiry): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, CreateKey_Success)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (CreateKey_Success): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, CreateKey_TokenFormat)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (CreateKey_TokenFormat): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, DeleteKey_NotFound)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (DeleteKey_NotFound): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, DeleteKey_RemovedFromListAndAuth)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (DeleteKey_RemovedFromListAndAuth): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, DeleteKey_Success)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (DeleteKey_Success): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, GetKey_NotFound)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (GetKey_NotFound): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, GetKey_Success)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (GetKey_Success): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, ListKeys_EmptyInitially)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (ListKeys_EmptyInitially): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, ListKeys_ReflectsCreatedKeys)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (ListKeys_ReflectsCreatedKeys): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, MultipleKeys_HaveUniqueIds)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (MultipleKeys_HaveUniqueIds): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, NullAuthMiddleware_CreateKeyStillWorks)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (NullAuthMiddleware_CreateKeyStillWorks): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, UpdateKey_ChangeName)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (UpdateKey_ChangeName): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, UpdateKey_ChangePermissions)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (UpdateKey_ChangePermissions): n/a

#### `TEST_F(ApiKeyMgmtHandlerTest, UpdateKey_NotFound)`
- Source: `tests/api/test_api_key_mgmt_handler.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMgmtHandlerTest): n/a
  - `<unnamed>` (UpdateKey_NotFound): n/a

### test_api_llm_grpc_focused.cpp

#### `TEST(ApiLlmGrpcFocused, AG1_LlmInferenceTimeout_CorrectCode)`
- Source: `tests/api/test_api_llm_grpc_focused.cpp`:19
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiLlmGrpcFocused): n/a
  - `<unnamed>` (AG1_LlmInferenceTimeout_CorrectCode): n/a

#### `TEST(ApiLlmGrpcFocused, AG2_LlmOomErrors_RegisteredAsLlm)`
- Source: `tests/api/test_api_llm_grpc_focused.cpp`:29
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiLlmGrpcFocused): n/a
  - `<unnamed>` (AG2_LlmOomErrors_RegisteredAsLlm): n/a

#### `TEST(ApiLlmGrpcFocused, AG3_LlmErrors_InCodeRange)`
- Source: `tests/api/test_api_llm_grpc_focused.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiLlmGrpcFocused): n/a
  - `<unnamed>` (AG3_LlmErrors_InCodeRange): n/a

#### `TEST(ApiLlmGrpcFocused, AG4_LlmBatchSizeExceeded_Registered)`
- Source: `tests/api/test_api_llm_grpc_focused.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiLlmGrpcFocused): n/a
  - `<unnamed>` (AG4_LlmBatchSizeExceeded_Registered): n/a

#### `TEST(ApiLlmGrpcFocused, AG5_LlmOomErrors_SolutionNonEmpty)`
- Source: `tests/api/test_api_llm_grpc_focused.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiLlmGrpcFocused): n/a
  - `<unnamed>` (AG5_LlmOomErrors_SolutionNonEmpty): n/a

### test_api_observability.cpp

#### `TEST(BoundedResourceTest, BoundedMemoryFootprint)`
- Source: `tests/api/test_api_observability.cpp`:384
- Brief: n/a
- Parameters:
  - `<unnamed>` (BoundedResourceTest): n/a
  - `<unnamed>` (BoundedMemoryFootprint): n/a
- Details: TestMemory footprint remains bounded Adapter maintains bounded memory usage even with many metrics tracked

#### `TEST(BoundedResourceTest, QueueSizeLimit)`
- Source: `tests/api/test_api_observability.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (BoundedResourceTest): n/a
  - `<unnamed>` (QueueSizeLimit): n/a
- Details: TestQueue size limit enforcement Adapter rejects requests when queue is at capacity. Uses a small-limit variant (kMaxQueueSize = 3) so concurrent threads can fill the queue reliably without launching thousands of threads.

#### `TEST(BoundedResourceTest, SessionLimitEnforcement)`
- Source: `tests/api/test_api_observability.cpp`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (BoundedResourceTest): n/a
  - `<unnamed>` (SessionLimitEnforcement): n/a
- Details: TestSession limit enforcement Adapter rejects requests when the active-session limit is reached. Uses a small-limit variant (kMaxActiveSessions = 3) so concurrent threads can exhaust the session budget without thousands of goroutines.

#### `TEST(ObservabilityTest, ObservabilityOverheadBounded)`
- Source: `tests/api/test_api_observability.cpp`:551
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityTest): n/a
  - `<unnamed>` (ObservabilityOverheadBounded): n/a
- Details: TestObservability doesn't impact request performance Adding observability infrastructure doesn't significantly impact latency

#### `TEST(ObservabilityTest, QueueDepthTracking)`
- Source: `tests/api/test_api_observability.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityTest): n/a
  - `<unnamed>` (QueueDepthTracking): n/a
- Details: TestQueue depth tracking Adapter tracks current queue depth

#### `TEST(ObservabilityTest, RequestMetricTracking)`
- Source: `tests/api/test_api_observability.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityTest): n/a
  - `<unnamed>` (RequestMetricTracking): n/a
- Details: TestRequest metric tracking Adapter tracks total, successful, and failed request counts

#### `TEST(ObservabilityTest, ResponseMetadataHeaders)`
- Source: `tests/api/test_api_observability.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityTest): n/a
  - `<unnamed>` (ResponseMetadataHeaders): n/a
- Details: TestResponse headers include processing metadata Responses include X-Processing-Time-Ms header for observability

#### `TEST(ObservabilityTest, SessionCountTracking)`
- Source: `tests/api/test_api_observability.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityTest): n/a
  - `<unnamed>` (SessionCountTracking): n/a
- Details: TestSession count tracking Adapter tracks number of active sessions

#### `TEST(ObservabilityTest, TracingIdNonIntrusive)`
- Source: `tests/api/test_api_observability.cpp`:580
- Brief: n/a
- Parameters:
  - `<unnamed>` (ObservabilityTest): n/a
  - `<unnamed>` (TracingIdNonIntrusive): n/a
- Details: TestTracing metadata is non-intrusive Adding tracing IDs doesn't prevent successful request processing

#### `TEST(ReliabilityTest, ConcurrentReadWriteConsistency)`
- Source: `tests/api/test_api_observability.cpp`:455
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReliabilityTest): n/a
  - `<unnamed>` (ConcurrentReadWriteConsistency): n/a
- Details: TestConcurrent access doesn't cause data corruption Adapter remains functional with concurrent reads and writes to metrics

#### `TEST(ReliabilityTest, ConsistentErrorHandlingConcurrent)`
- Source: `tests/api/test_api_observability.cpp`:509
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReliabilityTest): n/a
  - `<unnamed>` (ConsistentErrorHandlingConcurrent): n/a
- Details: TestConsistent error handling under concurrent load Errors are handled consistently when multiple threads make requests

#### `TEST(ReliabilityTest, ThreadSafeMetricUpdates)`
- Source: `tests/api/test_api_observability.cpp`:412
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReliabilityTest): n/a
  - `<unnamed>` (ThreadSafeMetricUpdates): n/a
- Details: TestThread-safe metric updates Multiple threads updating metrics concurrently produces consistent results

### test_api_phase4_concurrency.cpp

#### `TEST(Phase4ConcurrencyTest, PolicyMiddlewareHighConcurrency32x50)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4ConcurrencyTest): n/a
  - `<unnamed>` (PolicyMiddlewareHighConcurrency32x50): n/a
- Details: Test32 threads × 50 requests — all succeed through policy middleware Validates that TransportPolicyMiddleware produces no spurious failures under high-concurrency load when all requests are well-formed.

#### `TEST(Phase4ConcurrencyTest, PolicyRejectsInvalidVersionAcrossAllThreads)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4ConcurrencyTest): n/a
  - `<unnamed>` (PolicyRejectsInvalidVersionAcrossAllThreads): n/a
- Details: TestPolicy rejects all threads uniformly on a shared invalid version Ensures that all 16 concurrent threads get a well-formed UnsupportedVersion error; no thread slips through with the bad X-API-Version header.

#### `TEST(Phase4ContractsTest, ContentTypeRequirements)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:525
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4ContractsTest): n/a
  - `<unnamed>` (ContentTypeRequirements): n/a
- Details: TestTransportContractValidator correctly identifies methods requiring Content-Type

#### `TEST(Phase4ContractsTest, PolicyConfigNormalizationClampsOversize)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:539
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4ContractsTest): n/a
  - `<unnamed>` (PolicyConfigNormalizationClampsOversize): n/a
- Details: TestTransportPolicyConfig normalization clamps values to global limits

#### `TEST(Phase4ContractsTest, SupportedVersionValidation)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:511
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4ContractsTest): n/a
  - `<unnamed>` (SupportedVersionValidation): n/a
- Details: TestTransportContractValidator correctly validates all supported versions

#### `TEST(Phase4EdgeCasesTest, MalformedRequestVariants)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4EdgeCasesTest): n/a
  - `<unnamed>` (MalformedRequestVariants): n/a
- Details: TestEmpty method is rejected; empty path is rejected; both empty is rejected

#### `TEST(Phase4EdgeCasesTest, PathLengthBoundary)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4EdgeCasesTest): n/a
  - `<unnamed>` (PathLengthBoundary): n/a
- Details: TestPath length boundary: kMaxPathBytes accepted, kMaxPathBytes+1 rejected

#### `TEST(Phase4EdgeCasesTest, PayloadBoundaryExactLimit)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4EdgeCasesTest): n/a
  - `<unnamed>` (PayloadBoundaryExactLimit): n/a
- Details: TestExact-boundary payload: kMaxPayloadBytes accepted, kMaxPayloadBytes+1 rejected

#### `TEST(Phase4EdgeCasesTest, PostWithBodyRequiresContentType)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4EdgeCasesTest): n/a
  - `<unnamed>` (PostWithBodyRequiresContentType): n/a
- Details: TestContent-Type enforcement: POST with body and no Content-Type is rejected

#### `TEST(Phase4EdgeCasesTest, PutAndPatchWithBodyRequireContentType)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:351
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4EdgeCasesTest): n/a
  - `<unnamed>` (PutAndPatchWithBodyRequireContentType): n/a
- Details: TestPUT and PATCH also require Content-Type when carrying a body

#### `TEST(Phase4EdgeCasesTest, ReadMethodsNeverRequireContentType)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:372
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4EdgeCasesTest): n/a
  - `<unnamed>` (ReadMethodsNeverRequireContentType): n/a
- Details: TestGET / DELETE / HEAD / OPTIONS do not require Content-Type even with a body

#### `TEST(Phase4MatrixTest, ValidProtocolCombinationsSucceed)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:398
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4MatrixTest): n/a
  - `<unnamed>` (ValidProtocolCombinationsSucceed): n/a
- Details: TestProtocol combination matrix — all valid combinations succeed Tests 6 HTTP methods × 3 version states × 2 payload scenarios = 36 combinations.

#### `TEST(Phase4TaxonomyTest, AllFailureClassesHaveCorrectHttpStatus)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:449
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4TaxonomyTest): n/a
  - `<unnamed>` (AllFailureClassesHaveCorrectHttpStatus): n/a
- Details: TestApiErrorTaxonomy maps all failure classes to correct HTTP status codes

#### `TEST(Phase4TaxonomyTest, ClientVsServerErrorClassification)`
- Source: `tests/api/test_api_phase4_concurrency.cpp`:478
- Brief: n/a
- Parameters:
  - `<unnamed>` (Phase4TaxonomyTest): n/a
  - `<unnamed>` (ClientVsServerErrorClassification): n/a
- Details: TestApiErrorTaxonomy correctly classifies client vs server errors

### test_api_routing.cpp

#### `TEST(APIDeprecationTest, CanRegisterAndRetrieveDeprecation)`
- Source: `tests/api/test_api_routing.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIDeprecationTest): n/a
  - `<unnamed>` (CanRegisterAndRetrieveDeprecation): n/a

#### `TEST(APIDeprecationTest, DeprecationInfoReturnedBeforeRemoval)`
- Source: `tests/api/test_api_routing.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIDeprecationTest): n/a
  - `<unnamed>` (DeprecationInfoReturnedBeforeRemoval): n/a

#### `TEST(APIDeprecationTest, NoDeprecationInfoAfterRemoval)`
- Source: `tests/api/test_api_routing.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIDeprecationTest): n/a
  - `<unnamed>` (NoDeprecationInfoAfterRemoval): n/a

#### `TEST(APIDeprecationTest, NoDeprecationInfoForUnregisteredEndpoint)`
- Source: `tests/api/test_api_routing.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIDeprecationTest): n/a
  - `<unnamed>` (NoDeprecationInfoForUnregisteredEndpoint): n/a

#### `TEST(APIRoutingConventions, HealthEndpointsAreNotVersioned)`
- Source: `tests/api/test_api_routing.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIRoutingConventions): n/a
  - `<unnamed>` (HealthEndpointsAreNotVersioned): n/a

#### `TEST(APIRoutingConventions, VersionedPathPrefixUsesV1)`
- Source: `tests/api/test_api_routing.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIRoutingConventions): n/a
  - `<unnamed>` (VersionedPathPrefixUsesV1): n/a

#### `TEST(APIVersionStringTest, ToStringFormat)`
- Source: `tests/api/test_api_routing.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionStringTest): n/a
  - `<unnamed>` (ToStringFormat): n/a

#### `TEST(APIVersionStringTest, ToStringIncludesVPrefix)`
- Source: `tests/api/test_api_routing.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionStringTest): n/a
  - `<unnamed>` (ToStringIncludesVPrefix): n/a

#### `TEST(APIVersionStringTest, ToStringZeroVersion)`
- Source: `tests/api/test_api_routing.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionStringTest): n/a
  - `<unnamed>` (ToStringZeroVersion): n/a

#### `TEST(VersionedRoutingConventions, V1PathStartsWithSlashV1Slash)`
- Source: `tests/api/test_api_routing.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (VersionedRoutingConventions): n/a
  - `<unnamed>` (V1PathStartsWithSlashV1Slash): n/a

#### `TEST(VersionedRoutingConventions, V2PathStartsWithSlashV2Slash)`
- Source: `tests/api/test_api_routing.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (VersionedRoutingConventions): n/a
  - `<unnamed>` (V2PathStartsWithSlashV2Slash): n/a

#### `TEST_F(APIVersionComparisonTest, EqualVersionsAreEqual)`
- Source: `tests/api/test_api_routing.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionComparisonTest): n/a
  - `<unnamed>` (EqualVersionsAreEqual): n/a

#### `TEST_F(APIVersionComparisonTest, LessOrEqualSameVersion)`
- Source: `tests/api/test_api_routing.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionComparisonTest): n/a
  - `<unnamed>` (LessOrEqualSameVersion): n/a

#### `TEST_F(APIVersionComparisonTest, LowerMajorIsLess)`
- Source: `tests/api/test_api_routing.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionComparisonTest): n/a
  - `<unnamed>` (LowerMajorIsLess): n/a

#### `TEST_F(APIVersionComparisonTest, LowerMinorIsLess)`
- Source: `tests/api/test_api_routing.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionComparisonTest): n/a
  - `<unnamed>` (LowerMinorIsLess): n/a

#### `TEST_F(APIVersionComparisonTest, LowerPatchIsLess)`
- Source: `tests/api/test_api_routing.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionComparisonTest): n/a
  - `<unnamed>` (LowerPatchIsLess): n/a

#### `TEST_F(APIVersionManagerRoutingTest, CurrentVersionIsSupported)`
- Source: `tests/api/test_api_routing.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerRoutingTest): n/a
  - `<unnamed>` (CurrentVersionIsSupported): n/a

#### `TEST_F(APIVersionManagerRoutingTest, HasCurrentVersion)`
- Source: `tests/api/test_api_routing.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerRoutingTest): n/a
  - `<unnamed>` (HasCurrentVersion): n/a

#### `TEST_F(APIVersionManagerRoutingTest, HasMinimumVersion)`
- Source: `tests/api/test_api_routing.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerRoutingTest): n/a
  - `<unnamed>` (HasMinimumVersion): n/a

#### `TEST_F(APIVersionManagerRoutingTest, ReturnsCurrentVersionForEmptyHeader)`
- Source: `tests/api/test_api_routing.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerRoutingTest): n/a
  - `<unnamed>` (ReturnsCurrentVersionForEmptyHeader): n/a

#### `TEST_F(APIVersionManagerRoutingTest, ReturnsCurrentVersionForInvalidHeader)`
- Source: `tests/api/test_api_routing.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerRoutingTest): n/a
  - `<unnamed>` (ReturnsCurrentVersionForInvalidHeader): n/a

#### `TEST_F(APIVersionManagerRoutingTest, ReturnsCurrentVersionForLatestKeyword)`
- Source: `tests/api/test_api_routing.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerRoutingTest): n/a
  - `<unnamed>` (ReturnsCurrentVersionForLatestKeyword): n/a

#### `TEST_F(APIVersionManagerRoutingTest, SupportedVersionsListNotEmpty)`
- Source: `tests/api/test_api_routing.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerRoutingTest): n/a
  - `<unnamed>` (SupportedVersionsListNotEmpty): n/a

#### `TEST_F(APIVersionParsingTest, LatestKeywordIsCaseSensitive)`
- Source: `tests/api/test_api_routing.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionParsingTest): n/a
  - `<unnamed>` (LatestKeywordIsCaseSensitive): n/a

#### `TEST_F(APIVersionParsingTest, ParseInvalidString)`
- Source: `tests/api/test_api_routing.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionParsingTest): n/a
  - `<unnamed>` (ParseInvalidString): n/a

#### `TEST_F(APIVersionParsingTest, ParseLatestKeyword)`
- Source: `tests/api/test_api_routing.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionParsingTest): n/a
  - `<unnamed>` (ParseLatestKeyword): n/a

#### `TEST_F(APIVersionParsingTest, ParseV1)`
- Source: `tests/api/test_api_routing.cpp`:23
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionParsingTest): n/a
  - `<unnamed>` (ParseV1): n/a

#### `TEST_F(APIVersionParsingTest, ParseV1Dot0)`
- Source: `tests/api/test_api_routing.cpp`:31
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionParsingTest): n/a
  - `<unnamed>` (ParseV1Dot0): n/a

#### `TEST_F(APIVersionParsingTest, ParseV1Dot0Dot0)`
- Source: `tests/api/test_api_routing.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionParsingTest): n/a
  - `<unnamed>` (ParseV1Dot0Dot0): n/a

#### `TEST_F(APIVersionParsingTest, ParseWithoutVPrefix)`
- Source: `tests/api/test_api_routing.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionParsingTest): n/a
  - `<unnamed>` (ParseWithoutVPrefix): n/a

### test_api_security_audit.cpp

#### `TEST_F(ApiSecurityAuditTest, AllFindingsHaveNonEmptyMessages)`
- Source: `tests/api/test_api_security_audit.cpp`:392
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (AllFindingsHaveNonEmptyMessages): n/a

#### `TEST_F(ApiSecurityAuditTest, AllSensitivePrefixesWithoutAuthAreHigh)`
- Source: `tests/api/test_api_security_audit.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (AllSensitivePrefixesWithoutAuthAreHigh): n/a

#### `TEST_F(ApiSecurityAuditTest, AuthRequiredWithoutScopeIsHigh)`
- Source: `tests/api/test_api_security_audit.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (AuthRequiredWithoutScopeIsHigh): n/a

#### `TEST_F(ApiSecurityAuditTest, DevDefaultsFailAuditDueToAuthDisabled)`
- Source: `tests/api/test_api_security_audit.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (DevDefaultsFailAuditDueToAuthDisabled): n/a

#### `TEST_F(ApiSecurityAuditTest, EmptyEndpointListWithSecureGlobalsPassesAudit)`
- Source: `tests/api/test_api_security_audit.cpp`:341
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (EmptyEndpointListWithSecureGlobalsPassesAudit): n/a

#### `TEST_F(ApiSecurityAuditTest, ExcessiveBurstCapacityIsLow)`
- Source: `tests/api/test_api_security_audit.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (ExcessiveBurstCapacityIsLow): n/a

#### `TEST_F(ApiSecurityAuditTest, GlobalAuthDisabledIsCritical)`
- Source: `tests/api/test_api_security_audit.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (GlobalAuthDisabledIsCritical): n/a

#### `TEST_F(ApiSecurityAuditTest, MissingRateLimitOnAuthEndpointIsMedium)`
- Source: `tests/api/test_api_security_audit.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (MissingRateLimitOnAuthEndpointIsMedium): n/a

#### `TEST_F(ApiSecurityAuditTest, NonSensitivePublicEndpointDoesNotTriggerHighFinding)`
- Source: `tests/api/test_api_security_audit.cpp`:360
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (NonSensitivePublicEndpointDoesNotTriggerHighFinding): n/a

#### `TEST_F(ApiSecurityAuditTest, RateLimitingDisabledIsHigh)`
- Source: `tests/api/test_api_security_audit.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (RateLimitingDisabledIsHigh): n/a

#### `TEST_F(ApiSecurityAuditTest, ReasonableBurstCapacityIsClean)`
- Source: `tests/api/test_api_security_audit.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (ReasonableBurstCapacityIsClean): n/a

#### `TEST_F(ApiSecurityAuditTest, ReportCountersMatchFindings)`
- Source: `tests/api/test_api_security_audit.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (ReportCountersMatchFindings): n/a

#### `TEST_F(ApiSecurityAuditTest, SecureDefaultsPassAudit)`
- Source: `tests/api/test_api_security_audit.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (SecureDefaultsPassAudit): n/a

#### `TEST_F(ApiSecurityAuditTest, SensitiveEndpointWithoutAuthIsHigh)`
- Source: `tests/api/test_api_security_audit.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiSecurityAuditTest): n/a
  - `<unnamed>` (SensitiveEndpointWithoutAuthIsHigh): n/a

#### `uint32_t count_severity(const ApiSecurityAuditReport &report, AuditSeverity severity)`
- Source: `tests/api/test_api_security_audit.cpp`:35
- Brief: n/a
- Parameters:
  - `report` (const ApiSecurityAuditReport &): n/a
  - `severity` (AuditSeverity): n/a

#### `bool has_finding_containing(const ApiSecurityAuditReport &report, const std::string &substring)`
- Source: `tests/api/test_api_security_audit.cpp`:23
- Brief: n/a
- Parameters:
  - `report` (const ApiSecurityAuditReport &): n/a
  - `substring` (const std::string &): n/a

### test_api_transport_hardening.cpp

#### `TEST(TransportHardeningTest, BoundedResourceBehaviorUnderLoad)`
- Source: `tests/api/test_api_transport_hardening.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (BoundedResourceBehaviorUnderLoad): n/a
- Details: TestBounded resource behavior under load Adapter gracefully handles rapid request/response cycles

#### `TEST(TransportHardeningTest, ConcurrentRequestHandling)`
- Source: `tests/api/test_api_transport_hardening.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (ConcurrentRequestHandling): n/a
- Details: TestConcurrent request handling Multiple threads can safely call handle() concurrently

#### `TEST(TransportHardeningTest, CorrelationIdPropagation)`
- Source: `tests/api/test_api_transport_hardening.cpp`:319
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (CorrelationIdPropagation): n/a
- Details: TestCorrelation ID propagation Requests with X-Correlation-ID header are echoed in response X-Correlation-ID

#### `TEST(TransportHardeningTest, DefaultRequestIdWhenNoCorrelation)`
- Source: `tests/api/test_api_transport_hardening.cpp`:339
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (DefaultRequestIdWhenNoCorrelation): n/a
- Details: TestRequest without correlation ID Requests without X-Correlation-ID result in no X-Correlation-ID in the response (X-Request-ID is always present as a separately generated per-request identifier)

#### `TEST(TransportHardeningTest, EmptyPayloadAccepted)`
- Source: `tests/api/test_api_transport_hardening.cpp`:399
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (EmptyPayloadAccepted): n/a
- Details: TestEmpty payload handling POST/PUT requests with empty body are accepted (no Content-Type enforcement for empty body)

#### `TEST(TransportHardeningTest, EnforceContentTypeForPost)`
- Source: `tests/api/test_api_transport_hardening.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (EnforceContentTypeForPost): n/a
- Details: TestContent-Type validation Transport adapter enforces Content-Type header for POST requests

#### `TEST(TransportHardeningTest, GetRequestWithBodyIgnored)`
- Source: `tests/api/test_api_transport_hardening.cpp`:382
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (GetRequestWithBodyIgnored): n/a
- Details: TestGET requests with no body GET requests are processed correctly even if body is provided (ignored)

#### `TEST(TransportHardeningTest, RejectEmptyMethod)`
- Source: `tests/api/test_api_transport_hardening.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (RejectEmptyMethod): n/a
- Details: TestMalformed request rejection Transport adapter rejects empty method with ERR_TRANSPORT_MALFORMED_REQUEST

#### `TEST(TransportHardeningTest, RejectEmptyPath)`
- Source: `tests/api/test_api_transport_hardening.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (RejectEmptyPath): n/a
- Details: TestMalformed request rejection Transport adapter rejects empty path with ERR_TRANSPORT_MALFORMED_REQUEST

#### `TEST(TransportHardeningTest, RejectOversizedPayload)`
- Source: `tests/api/test_api_transport_hardening.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (RejectOversizedPayload): n/a
- Details: TestBounded payload enforcement Transport adapter rejects oversized payloads with ERR_TRANSPORT_PAYLOAD_TOO_LARGE

#### `TEST(TransportHardeningTest, ResponseHeaderNormalization)`
- Source: `tests/api/test_api_transport_hardening.cpp`:358
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (ResponseHeaderNormalization): n/a
- Details: TestResponse header normalization All responses include Content-Type and X-Request-ID headers

#### `TEST(TransportHardeningTest, VersionNegotiationRejectsUnsupported)`
- Source: `tests/api/test_api_transport_hardening.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (VersionNegotiationRejectsUnsupported): n/a
- Details: TestVersion negotiation Transport adapter rejects unsupported API versions with ERR_TRANSPORT_UNSUPPORTED_VERSION

#### `TEST(TransportHardeningTest, VersionNegotiationSupported)`
- Source: `tests/api/test_api_transport_hardening.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportHardeningTest): n/a
  - `<unnamed>` (VersionNegotiationSupported): n/a
- Details: TestVersion negotiation Transport adapter supports v1, v2 and rejects unsupported versions

### test_api_version.cpp

#### `TEST(APIVersionEdgeCases, LargeVersionNumbers)`
- Source: `tests/api/test_api_version.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionEdgeCases): n/a
  - `<unnamed>` (LargeVersionNumbers): n/a

#### `TEST(APIVersionEdgeCases, PartialVersionComparison)`
- Source: `tests/api/test_api_version.cpp`:295
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionEdgeCases): n/a
  - `<unnamed>` (PartialVersionComparison): n/a

#### `TEST(APIVersionEdgeCases, ZeroVersion)`
- Source: `tests/api/test_api_version.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionEdgeCases): n/a
  - `<unnamed>` (ZeroVersion): n/a

#### `TEST(APIVersionEndpointContract, CurrentVersionMatchesConfig)`
- Source: `tests/api/test_api_version.cpp`:308
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionEndpointContract): n/a
  - `<unnamed>` (CurrentVersionMatchesConfig): n/a

#### `TEST(APIVersionEndpointContract, CurrentVersionToStringHasVPrefix)`
- Source: `tests/api/test_api_version.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionEndpointContract): n/a
  - `<unnamed>` (CurrentVersionToStringHasVPrefix): n/a

#### `TEST(APIVersionEndpointContract, MinimumVersionMatchesConfig)`
- Source: `tests/api/test_api_version.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionEndpointContract): n/a
  - `<unnamed>` (MinimumVersionMatchesConfig): n/a

#### `TEST(APIVersionEndpointContract, SupportedVersionsContainCurrentAndMinimum)`
- Source: `tests/api/test_api_version.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionEndpointContract): n/a
  - `<unnamed>` (SupportedVersionsContainCurrentAndMinimum): n/a

#### `TEST(APIVersionIntegrationTest, FullVersionNegotiationFlow)`
- Source: `tests/api/test_api_version.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionIntegrationTest): n/a
  - `<unnamed>` (FullVersionNegotiationFlow): n/a

#### `TEST(APIVersionRangeTest, Contains)`
- Source: `tests/api/test_api_version.cpp`:566
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionRangeTest): n/a
  - `<unnamed>` (Contains): n/a

#### `TEST(APIVersionRangeTest, ParseInvalidRange_BadVersion)`
- Source: `tests/api/test_api_version.cpp`:543
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionRangeTest): n/a
  - `<unnamed>` (ParseInvalidRange_BadVersion): n/a

#### `TEST(APIVersionRangeTest, ParseInvalidRange_MinGreaterThanMax)`
- Source: `tests/api/test_api_version.cpp`:547
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionRangeTest): n/a
  - `<unnamed>` (ParseInvalidRange_MinGreaterThanMax): n/a

#### `TEST(APIVersionRangeTest, ParseInvalidRange_NoDash)`
- Source: `tests/api/test_api_version.cpp`:539
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionRangeTest): n/a
  - `<unnamed>` (ParseInvalidRange_NoDash): n/a

#### `TEST(APIVersionRangeTest, ParseRangeWithLeadingTrailingWhitespace)`
- Source: `tests/api/test_api_version.cpp`:559
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionRangeTest): n/a
  - `<unnamed>` (ParseRangeWithLeadingTrailingWhitespace): n/a

#### `TEST(APIVersionRangeTest, ParseRangeWithSpacesAroundDash)`
- Source: `tests/api/test_api_version.cpp`:551
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionRangeTest): n/a
  - `<unnamed>` (ParseRangeWithSpacesAroundDash): n/a

#### `TEST(APIVersionRangeTest, ParseValidRange)`
- Source: `tests/api/test_api_version.cpp`:525
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionRangeTest): n/a
  - `<unnamed>` (ParseValidRange): n/a

#### `TEST(APIVersionRangeTest, ParseValidRangeSemver)`
- Source: `tests/api/test_api_version.cpp`:532
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionRangeTest): n/a
  - `<unnamed>` (ParseValidRangeSemver): n/a

#### `TEST(RouteVersionRouter, ExtractVersion_ApiNested)`
- Source: `tests/api/test_api_version.cpp`:399
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (ExtractVersion_ApiNested): n/a

#### `TEST(RouteVersionRouter, ExtractVersion_Unversioned)`
- Source: `tests/api/test_api_version.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (ExtractVersion_Unversioned): n/a

#### `TEST(RouteVersionRouter, ExtractVersion_V1)`
- Source: `tests/api/test_api_version.cpp`:384
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (ExtractVersion_V1): n/a

#### `TEST(RouteVersionRouter, ExtractVersion_V2)`
- Source: `tests/api/test_api_version.cpp`:389
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (ExtractVersion_V2): n/a

#### `TEST(RouteVersionRouter, IsVersioned_ApiPrefix)`
- Source: `tests/api/test_api_version.cpp`:367
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (IsVersioned_ApiPrefix): n/a

#### `TEST(RouteVersionRouter, IsVersioned_Unversioned)`
- Source: `tests/api/test_api_version.cpp`:372
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (IsVersioned_Unversioned): n/a

#### `TEST(RouteVersionRouter, IsVersioned_V1)`
- Source: `tests/api/test_api_version.cpp`:356
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (IsVersioned_V1): n/a

#### `TEST(RouteVersionRouter, IsVersioned_V2)`
- Source: `tests/api/test_api_version.cpp`:362
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (IsVersioned_V2): n/a

#### `TEST(RouteVersionRouter, NoRedirect_AlreadyVersionedV1)`
- Source: `tests/api/test_api_version.cpp`:491
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (NoRedirect_AlreadyVersionedV1): n/a

#### `TEST(RouteVersionRouter, NoRedirect_AlreadyVersionedV2)`
- Source: `tests/api/test_api_version.cpp`:496
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (NoRedirect_AlreadyVersionedV2): n/a

#### `TEST(RouteVersionRouter, NoRedirect_GraphQLWebSocket)`
- Source: `tests/api/test_api_version.cpp`:511
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (NoRedirect_GraphQLWebSocket): n/a

#### `TEST(RouteVersionRouter, NoRedirect_HealthEndpoint)`
- Source: `tests/api/test_api_version.cpp`:501
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (NoRedirect_HealthEndpoint): n/a

#### `TEST(RouteVersionRouter, NoRedirect_MetricsEndpoint)`
- Source: `tests/api/test_api_version.cpp`:506
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (NoRedirect_MetricsEndpoint): n/a

#### `TEST(RouteVersionRouter, NoRedirect_RootPath)`
- Source: `tests/api/test_api_version.cpp`:516
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (NoRedirect_RootPath): n/a

#### `TEST(RouteVersionRouter, Normalize_ApiPrefixedPath)`
- Source: `tests/api/test_api_version.cpp`:456
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (Normalize_ApiPrefixedPath): n/a

#### `TEST(RouteVersionRouter, Normalize_UnversionedPath)`
- Source: `tests/api/test_api_version.cpp`:449
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (Normalize_UnversionedPath): n/a

#### `TEST(RouteVersionRouter, Normalize_V1Path)`
- Source: `tests/api/test_api_version.cpp`:435
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (Normalize_V1Path): n/a

#### `TEST(RouteVersionRouter, Normalize_V2Path)`
- Source: `tests/api/test_api_version.cpp`:442
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (Normalize_V2Path): n/a

#### `TEST(RouteVersionRouter, Redirect_UnversionedDocumentPath)`
- Source: `tests/api/test_api_version.cpp`:483
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (Redirect_UnversionedDocumentPath): n/a

#### `TEST(RouteVersionRouter, Redirect_UnversionedPath)`
- Source: `tests/api/test_api_version.cpp`:468
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (Redirect_UnversionedPath): n/a

#### `TEST(RouteVersionRouter, Redirect_UnversionedPathWithQuery)`
- Source: `tests/api/test_api_version.cpp`:475
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (Redirect_UnversionedPathWithQuery): n/a

#### `TEST(RouteVersionRouter, StripPrefix_ApiNested_NotStripped)`
- Source: `tests/api/test_api_version.cpp`:425
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (StripPrefix_ApiNested_NotStripped): n/a

#### `TEST(RouteVersionRouter, StripPrefix_Unversioned)`
- Source: `tests/api/test_api_version.cpp`:418
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (StripPrefix_Unversioned): n/a

#### `TEST(RouteVersionRouter, StripPrefix_V1)`
- Source: `tests/api/test_api_version.cpp`:408
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (StripPrefix_V1): n/a

#### `TEST(RouteVersionRouter, StripPrefix_V2)`
- Source: `tests/api/test_api_version.cpp`:413
- Brief: n/a
- Parameters:
  - `<unnamed>` (RouteVersionRouter): n/a
  - `<unnamed>` (StripPrefix_V2): n/a

#### `TEST_F(APIVersionManagerTest, DeprecationVersionRange)`
- Source: `tests/api/test_api_version.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (DeprecationVersionRange): n/a

#### `TEST_F(APIVersionManagerTest, GetCurrentVersion)`
- Source: `tests/api/test_api_version.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (GetCurrentVersion): n/a

#### `TEST_F(APIVersionManagerTest, GetDeprecationInfoNonExistent)`
- Source: `tests/api/test_api_version.cpp`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (GetDeprecationInfoNonExistent): n/a

#### `TEST_F(APIVersionManagerTest, GetMinimumVersion)`
- Source: `tests/api/test_api_version.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (GetMinimumVersion): n/a

#### `TEST_F(APIVersionManagerTest, GetSupportedVersions)`
- Source: `tests/api/test_api_version.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (GetSupportedVersions): n/a

#### `TEST_F(APIVersionManagerTest, IsVersionSupported)`
- Source: `tests/api/test_api_version.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (IsVersionSupported): n/a

#### `TEST_F(APIVersionManagerTest, RegisterAndGetDeprecation)`
- Source: `tests/api/test_api_version.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (RegisterAndGetDeprecation): n/a

#### `TEST_F(APIVersionManagerTest, ResolveVersionEmpty)`
- Source: `tests/api/test_api_version.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (ResolveVersionEmpty): n/a

#### `TEST_F(APIVersionManagerTest, ResolveVersionInvalid)`
- Source: `tests/api/test_api_version.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (ResolveVersionInvalid): n/a

#### `TEST_F(APIVersionManagerTest, ResolveVersionMajorMinorOnly)`
- Source: `tests/api/test_api_version.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (ResolveVersionMajorMinorOnly): n/a

#### `TEST_F(APIVersionManagerTest, ResolveVersionMajorOnly)`
- Source: `tests/api/test_api_version.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (ResolveVersionMajorOnly): n/a

#### `TEST_F(APIVersionManagerTest, ResolveVersionUnknownMajorFallsBack)`
- Source: `tests/api/test_api_version.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (ResolveVersionUnknownMajorFallsBack): n/a

#### `TEST_F(APIVersionManagerTest, ResolveVersionUnsupported)`
- Source: `tests/api/test_api_version.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (ResolveVersionUnsupported): n/a

#### `TEST_F(APIVersionManagerTest, ResolveVersionValid)`
- Source: `tests/api/test_api_version.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (ResolveVersionValid): n/a

#### `TEST_F(APIVersionManagerTest, VersionOrdering)`
- Source: `tests/api/test_api_version.cpp`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionManagerTest): n/a
  - `<unnamed>` (VersionOrdering): n/a

#### `TEST_F(APIVersionRangeResolutionTest, ExactVersionRange)`
- Source: `tests/api/test_api_version.cpp`:605
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionRangeResolutionTest): n/a
  - `<unnamed>` (ExactVersionRange): n/a

#### `TEST_F(APIVersionRangeResolutionTest, FallsBackToCurrentWhenNoMatch)`
- Source: `tests/api/test_api_version.cpp`:598
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionRangeResolutionTest): n/a
  - `<unnamed>` (FallsBackToCurrentWhenNoMatch): n/a

#### `TEST_F(APIVersionRangeResolutionTest, ResolvesHighestVersionInRange)`
- Source: `tests/api/test_api_version.cpp`:584
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionRangeResolutionTest): n/a
  - `<unnamed>` (ResolvesHighestVersionInRange): n/a

#### `TEST_F(APIVersionRangeResolutionTest, ResolvesToCurrentWhenRangeCoversCurrent)`
- Source: `tests/api/test_api_version.cpp`:591
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionRangeResolutionTest): n/a
  - `<unnamed>` (ResolvesToCurrentWhenRangeCoversCurrent): n/a

#### `TEST_F(APIVersionTest, ParseEmptyString)`
- Source: `tests/api/test_api_version.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionTest): n/a
  - `<unnamed>` (ParseEmptyString): n/a

#### `TEST_F(APIVersionTest, ParseInvalidVersion)`
- Source: `tests/api/test_api_version.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionTest): n/a
  - `<unnamed>` (ParseInvalidVersion): n/a

#### `TEST_F(APIVersionTest, ParseLatestKeyword)`
- Source: `tests/api/test_api_version.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionTest): n/a
  - `<unnamed>` (ParseLatestKeyword): n/a

#### `TEST_F(APIVersionTest, ParseValidVersion)`
- Source: `tests/api/test_api_version.cpp`:29
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionTest): n/a
  - `<unnamed>` (ParseValidVersion): n/a

#### `TEST_F(APIVersionTest, ParseVersionMajorOnly)`
- Source: `tests/api/test_api_version.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionTest): n/a
  - `<unnamed>` (ParseVersionMajorOnly): n/a

#### `TEST_F(APIVersionTest, ParseVersionMinorOnly)`
- Source: `tests/api/test_api_version.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionTest): n/a
  - `<unnamed>` (ParseVersionMinorOnly): n/a

#### `TEST_F(APIVersionTest, ParseVersionWithoutV)`
- Source: `tests/api/test_api_version.cpp`:37
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionTest): n/a
  - `<unnamed>` (ParseVersionWithoutV): n/a

#### `TEST_F(APIVersionTest, ToStringFormatted)`
- Source: `tests/api/test_api_version.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionTest): n/a
  - `<unnamed>` (ToStringFormatted): n/a

#### `TEST_F(APIVersionTest, VersionComparison)`
- Source: `tests/api/test_api_version.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionTest): n/a
  - `<unnamed>` (VersionComparison): n/a

#### `TEST_F(APIVersionTest, VersionEquality)`
- Source: `tests/api/test_api_version.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (APIVersionTest): n/a
  - `<unnamed>` (VersionEquality): n/a

#### `TEST_F(BreakingChangeTest, BreakingChangeMigrationGuideUrl)`
- Source: `tests/api/test_api_version.cpp`:683
- Brief: n/a
- Parameters:
  - `<unnamed>` (BreakingChangeTest): n/a
  - `<unnamed>` (BreakingChangeMigrationGuideUrl): n/a

#### `TEST_F(BreakingChangeTest, BreakingChangeNotDetectedWhenFromVersionExceedsIt)`
- Source: `tests/api/test_api_version.cpp`:661
- Brief: n/a
- Parameters:
  - `<unnamed>` (BreakingChangeTest): n/a
  - `<unnamed>` (BreakingChangeNotDetectedWhenFromVersionExceedsIt): n/a

#### `TEST_F(BreakingChangeTest, BreakingChangeNotDetectedWhenStrictlyBeforeRange)`
- Source: `tests/api/test_api_version.cpp`:672
- Brief: n/a
- Parameters:
  - `<unnamed>` (BreakingChangeTest): n/a
  - `<unnamed>` (BreakingChangeNotDetectedWhenStrictlyBeforeRange): n/a

#### `TEST_F(BreakingChangeTest, EndpointBreakingChangeDetected)`
- Source: `tests/api/test_api_version.cpp`:637
- Brief: n/a
- Parameters:
  - `<unnamed>` (BreakingChangeTest): n/a
  - `<unnamed>` (EndpointBreakingChangeDetected): n/a

#### `TEST_F(BreakingChangeTest, EndpointBreakingChangeNotAffectsOtherEndpoint)`
- Source: `tests/api/test_api_version.cpp`:649
- Brief: n/a
- Parameters:
  - `<unnamed>` (BreakingChangeTest): n/a
  - `<unnamed>` (EndpointBreakingChangeNotAffectsOtherEndpoint): n/a

#### `TEST_F(BreakingChangeTest, GlobalBreakingChangeDetected)`
- Source: `tests/api/test_api_version.cpp`:626
- Brief: n/a
- Parameters:
  - `<unnamed>` (BreakingChangeTest): n/a
  - `<unnamed>` (GlobalBreakingChangeDetected): n/a

#### `TEST_F(BreakingChangeTest, NoBreakingChangeRegistered)`
- Source: `tests/api/test_api_version.cpp`:621
- Brief: n/a
- Parameters:
  - `<unnamed>` (BreakingChangeTest): n/a
  - `<unnamed>` (NoBreakingChangeRegistered): n/a

### test_api_wave_d_stress.cpp

#### `TEST(WaveDOperatorHintTest, OperatorHints_AllErrorsUseOStreamNotConcat)`
- Source: `tests/api/test_api_wave_d_stress.cpp`:588
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveDOperatorHintTest): n/a
  - `<unnamed>` (OperatorHints_AllErrorsUseOStreamNotConcat): n/a
- Details: TestOperatorHints_AllErrorsUseOStreamNotConcat Regression guard: error messages must be constructed via std::ostringstream (or string literals), not via in-loop string concatenation. This test exercises every error path in OperatorHintAdapter and confirms the messages are non-empty and well-formed — the compilation of this file (which uses ostringstream throughout) is itself the primary guard. Roadmap reference: Wave D code quality — string_concat_loop replacement

#### `TEST(WaveDOperatorHintTest, OperatorHints_InvalidRequest_ContainsErrCode)`
- Source: `tests/api/test_api_wave_d_stress.cpp`:479
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveDOperatorHintTest): n/a
  - `<unnamed>` (OperatorHints_InvalidRequest_ContainsErrCode): n/a
- Details: TestOperatorHints_InvalidRequest_ContainsErrCode An ill-formed request (empty method) must produce an error whose message carries an ERR_-prefixed code and actionable operator guidance. Roadmap reference: Wave D — "operator remediation hints in diagnostic messages"

#### `TEST(WaveDOperatorHintTest, OperatorHints_OtlpExportFailed_ContainsErrCode)`
- Source: `tests/api/test_api_wave_d_stress.cpp`:558
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveDOperatorHintTest): n/a
  - `<unnamed>` (OperatorHints_OtlpExportFailed_ContainsErrCode): n/a
- Details: TestOperatorHints_OtlpExportFailed_ContainsErrCode An OTLP export failure must surface an ERR_OTLP_-prefixed code and include a hint for diagnosing the collector endpoint. Roadmap reference: Wave D — "operator remediation hints in diagnostic messages"

#### `TEST(WaveDOperatorHintTest, OperatorHints_RateLimit_ContainsErrCode)`
- Source: `tests/api/test_api_wave_d_stress.cpp`:534
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveDOperatorHintTest): n/a
  - `<unnamed>` (OperatorHints_RateLimit_ContainsErrCode): n/a
- Details: TestOperatorHints_RateLimit_ContainsErrCode A rate-limit rejection must produce ERR_API_RATE_LIMIT with guidance on how to resolve the issue (reduce rate or contact support). Roadmap reference: Wave D — "operator remediation hints in diagnostic messages"

#### `TEST(WaveDOperatorHintTest, OperatorHints_Unauthorized_ContainsErrCode)`
- Source: `tests/api/test_api_wave_d_stress.cpp`:510
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveDOperatorHintTest): n/a
  - `<unnamed>` (OperatorHints_Unauthorized_ContainsErrCode): n/a
- Details: TestOperatorHints_Unauthorized_ContainsErrCode An authentication failure must produce ERR_API_UNAUTHORIZED with a remediation instruction (e.g., token renewal endpoint). Roadmap reference: Wave D — "operator remediation hints in diagnostic messages"

#### `TEST(WaveDSoakTest, SoakSimulation_100kRequests_NoBoundedResourceLeak)`
- Source: `tests/api/test_api_wave_d_stress.cpp`:368
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveDSoakTest): n/a
  - `<unnamed>` (SoakSimulation_100kRequests_NoBoundedResourceLeak): n/a
- Details: TestSoakSimulation_100kRequests_NoBoundedResourceLeak Issues 100 000 requests in a tight single-threaded loop through the OperatorHintAdapter. After the loop completes: Total processed == 100 000 (no silent drops) Queue depth returns to zero (no resource accumulation) No heap-use-after-free or lock-order violations (TSAN/ASAN will catch these; the test itself asserts count-based invariants). Roadmap reference: Wave D — "long-duration soak test coverage"

#### `TEST(WaveDSoakTest, SoakSimulation_ConcurrentMixedLoad_NoLeak)`
- Source: `tests/api/test_api_wave_d_stress.cpp`:411
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveDSoakTest): n/a
  - `<unnamed>` (SoakSimulation_ConcurrentMixedLoad_NoLeak): n/a
- Details: TestSoakSimulation_ConcurrentMixedLoad_NoLeak 16 threads each issue 10 000 requests (160 000 total), with a mix of success, auth-failure, and rate-limit paths. Validates: success + auth_fail + rate_limit == total (accounting invariant) No deadlock or data race (TSAN catches races; test asserts counts) Roadmap reference: Wave D — "long-duration soak test coverage"

#### `TEST(WaveDStressTest, ExporterAllRetriesExhausted)`
- Source: `tests/api/test_api_wave_d_stress.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveDStressTest): n/a
  - `<unnamed>` (ExporterAllRetriesExhausted): n/a
- Details: TestExporterAllRetriesExhausted When every attempt fails the batch must be counted as dropped, not silently discarded. Roadmap reference: Wave D — "exporter reliability hardening"

#### `TEST(WaveDStressTest, ExporterRetryBackoffSimulation)`
- Source: `tests/api/test_api_wave_d_stress.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveDStressTest): n/a
  - `<unnamed>` (ExporterRetryBackoffSimulation): n/a
- Details: TestExporterRetryBackoffSimulation Simulates the retry-with-exponential-back-off logic documented in OtlpExporter::flushBatch(). A mock "transport" fails on the first two attempts and succeeds on the third. The test verifies: Exactly max_attempts calls are made before giving up (or fewer on success) Back-off delay grows by 2x each retry A success on attempt N records all spans as exported (not dropped) Roadmap reference: Wave D — "exporter reliability hardening"

#### `TEST(WaveDStressTest, HighCardinalitySpanIngestion)`
- Source: `tests/api/test_api_wave_d_stress.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveDStressTest): n/a
  - `<unnamed>` (HighCardinalitySpanIngestion): n/a
- Details: TestHighCardinalitySpanIngestion Submits 1000+ unique spans concurrently across 8 worker threads. Validates: All spans are either accepted or cleanly dropped (no assertion failure, no UB, no memory growth — the sink's in-process queue is the bound). accepted + dropped == total submitted (accounting invariant holds). Peak queue depth never exceeds the declared capacity. Roadmap reference: Wave D — "high-cardinality stress coverage for tracing paths"

### themis::api

#### `std::string aqlEscapeLiteral(const std::string &raw)`
- Source: `include/api/aql_utils.h`:34
- Brief: Aql Escape Literal.
- Parameters:
  - `raw` (const std::string &): Input parameter.
- Return: Return value.
- Details: raw Input parameter. Return value. Calls: reserve(), size().

#### `bool hasCapability(TransportCapability set, TransportCapability flag) noexcept`
- Source: `include/api/api_transport_contracts.h`:72
- Brief: n/a
- Parameters:
  - `set` (TransportCapability): n/a
  - `flag` (TransportCapability): n/a

#### `bool isValidAqlIdentifier(const std::string &name)`
- Source: `include/api/aql_utils.h`:51
- Brief: Is Valid Aql Identifier.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: name Input parameter. True when the operation succeeds. Calls: empty(), std::isalpha(), std::isalnum().

#### `TransportCapability operator&(TransportCapability a, TransportCapability b) noexcept`
- Source: `include/api/api_transport_contracts.h`:66
- Brief: n/a
- Parameters:
  - `a` (TransportCapability): n/a
  - `b` (TransportCapability): n/a

#### `TransportCapability operator\|(TransportCapability a, TransportCapability b) noexcept`
- Source: `include/api/api_transport_contracts.h`:60
- Brief: n/a
- Parameters:
  - `a` (TransportCapability): n/a
  - `b` (TransportCapability): n/a

#### `bool validateCoordinatePair(const json &coord, double &lon, double &lat)`
- Source: `src/api/geo_index_hooks.cpp`:75
- Brief: n/a
- Parameters:
  - `coord` (const json &): n/a
  - `lon` (double &): n/a
  - `lat` (double &): n/a

#### `bool validateGeoJSONBasic(const json &geojson)`
- Source: `src/api/geo_index_hooks.cpp`:32
- Brief: Helper function to validate GeoJSON before parsing.
- Parameters:
  - `geojson` (const json &): Input parameter.
- Return: True when the operation succeeds.
- Details: geojson Input parameter. True when the operation succeeds. Calls: is_object(), contains(), is_string(), is_array(), size(), THEMIS_WARN().

### themis::api::ApiErrorTaxonomy

#### `ApiErrorTaxonomy()=delete`
- Source: `include/api/api_error_taxonomy.h`:41
- Brief: n/a
- Parameters: none

#### `bool isClientError(TransportFailureClass fc) noexcept`
- Source: `include/api/api_error_taxonomy.h`:124
- Brief: n/a
- Parameters:
  - `fc` (TransportFailureClass): n/a

#### `themis::errors::ErrorCode toErrorCode(TransportFailureClass fc) noexcept`
- Source: `include/api/api_error_taxonomy.h`:43
- Brief: n/a
- Parameters:
  - `fc` (TransportFailureClass): n/a

#### `int toHttpStatus(TransportFailureClass fc) noexcept`
- Source: `include/api/api_error_taxonomy.h`:65
- Brief: n/a
- Parameters:
  - `fc` (TransportFailureClass): n/a

#### `std::string toMessage(TransportFailureClass fc, std::string_view adapter_name)`
- Source: `include/api/api_error_taxonomy.h`:90
- Brief: n/a
- Parameters:
  - `fc` (TransportFailureClass): n/a
  - `adapter_name` (std::string_view): n/a

### themis::api::CorrelationId

#### `CorrelationId() noexcept`
- Source: `include/api/correlation_id.h`:72
- Brief: n/a
- Parameters: none

#### `CorrelationId(const std::array< uint8_t, kByteSize > &bytes) noexcept`
- Source: `include/api/correlation_id.h`:74
- Brief: n/a
- Parameters:
  - `bytes` (const std::array< uint8_t, kByteSize > &): n/a

#### `CorrelationId(const uint8_t *bytes) noexcept`
- Source: `include/api/correlation_id.h`:77
- Brief: n/a
- Parameters:
  - `bytes` (const uint8_t *): n/a

#### `const std::array< uint8_t, kByteSize > & bytes() const noexcept`
- Source: `include/api/correlation_id.h`:94
- Brief: n/a
- Parameters: none

#### `bool isNil() const noexcept`
- Source: `include/api/correlation_id.h`:96
- Brief: n/a
- Parameters: none

#### `bool operator!=(const CorrelationId &other) const noexcept`
- Source: `include/api/correlation_id.h`:107
- Brief: n/a
- Parameters:
  - `other` (const CorrelationId &): n/a

#### `bool operator==(const CorrelationId &other) const noexcept`
- Source: `include/api/correlation_id.h`:103
- Brief: n/a
- Parameters:
  - `other` (const CorrelationId &): n/a

#### `CorrelationId parse(std::string_view s)`
- Source: `include/api/correlation_id.h`:86
- Brief: Parse.
- Parameters:
  - `s` (std::string_view): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value.

#### `std::string toString() const`
- Source: `include/api/correlation_id.h`:92
- Brief: To String.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::api::FederationAdminHandler

#### `FederationAdminHandler(std::shared_ptr< distributed_knowledge::LoRAFederationCoordinator > coordinator, std::shared_ptr< distributed_knowledge::FederatedRAGMerger > merger=nullptr)`
- Source: `include/api/federation_admin_handler.h`:35
- Brief: Construct with required dependencies.
- Parameters:
  - `coordinator` (std::shared_ptr< distributed_knowledge::LoRAFederationCoordinator >): Active federation coordinator (must be non-null).
  - `merger` (std::shared_ptr< distributed_knowledge::FederatedRAGMerger >): Optional RAG merger for rag-stats endpoint.
- Details: coordinator Active federation coordinator (must be non-null). merger Optional RAG merger for rag-stats endpoint.

#### `nlohmann::json getRagStats() const`
- Source: `include/api/federation_admin_handler.h`:61
- Brief: Return RAG merge statistics as JSON.
- Parameters: none
- Return: JSON object suitable for HTTP response body.
- Details: Returns merge stats from FederatedRAGMerger::getStats() when a merger is available, or {"available": false} otherwise. JSON object suitable for HTTP response body.

#### `nlohmann::json getStats() const`
- Source: `include/api/federation_admin_handler.h`:49
- Brief: Return federation statistics as JSON.
- Parameters: none
- Return: JSON object suitable for HTTP response body.
- Details: Includes current_round, pending_gradients, privacy_budget_remaining, dp_epsilon_total, and all fields from coordinator.getStats(). JSON object suitable for HTTP response body.

#### `nlohmann::json triggerRound(const std::string &algorithm="")`
- Source: `include/api/federation_admin_handler.h`:80
- Brief: Manually trigger a federation round and return the result.
- Parameters:
  - `algorithm` (const std::string &): Input parameter.
- Return: JSON object: {"round", "participants", "delta_version", "epsilon_spent", "status": "success"}.
- Throws:
  - std::runtime_error("DP: budget exhausted") when <tt>verifyPrivacyBudget()</tt> is false. @throws std::runtime_error("Cross-border transfer blocked: ...") when GDPR policy blocks a participant's region. @throws std::runtime_error("Insufficient participants: ...") when fewer than min_participants gradients were submitted.
  - std::runtime_error: if an error occurs.
- Details: Trigger Round. algorithm Optional aggregation algorithm override (e.g. "FedAvg"). Ignored if empty — config default is used. JSON object: {"round", "participants", "delta_version", "epsilon_spent", "status": "success"}. std::runtime_error("DP budget exhausted") when <tt>verifyPrivacyBudget()</tt> is false. @throws std::runtime_error("Cross-border transfer blocked: ...") when GDPR policy blocks a participant's region. @throws std::runtime_error("Insufficient participants: ...") when fewer than min_participants gradients were submitted. algorithm Input parameter. Return value. std::runtime_error if an error occurs. Calls: verifyPrivacyBudget(), triggerAggregation().

### themis::api::GRPCMetadata

#### `bool hasDeadline() const noexcept`
- Source: `include/api/grpc_bridge.h`:56
- Brief: n/a
- Parameters: none

### themis::api::GeoIndexHooks

#### `void onEntityDelete(RocksDBWrapper &db, index::SpatialIndexManager *spatial_mgr, const std::string &table, const std::string &pk, const std::vector< uint8_t > &old_blob)`
- Source: `include/api/geo_index_hooks.h`:88
- Brief: On Entity Delete.
- Parameters:
  - `db` (RocksDBWrapper &): Input/output parameter.
  - `spatial_mgr` (index::SpatialIndexManager *): Input/output parameter.
  - `table` (const std::string &): Input parameter.
  - `pk` (const std::string &): Input parameter.
  - `old_blob` (const std::vector< uint8_t > &): Input parameter.
- Details: db Input/output parameter. spatial_mgr Input/output parameter. table Input parameter. pk Input parameter. old_blob Input parameter. db Input/output parameter. spatial_mgr Input/output parameter. table Input parameter. pk Input parameter. old_blob Input parameter. Calls: hasSpatialIndex(), blob_str(), data(), size(), json::parse(), contains(), is_string(), reserve().

#### `bool onEntityDeleteAtomic(RocksDBWrapper::WriteBatchWrapper &batch, index::SpatialIndexManager *spatial_mgr, const std::string &table, const std::string &pk, const std::vector< uint8_t > &old_blob)`
- Source: `include/api/geo_index_hooks.h`:72
- Brief: On Entity Delete Atomic.
- Parameters:
  - `batch` (RocksDBWrapper::WriteBatchWrapper &): Input/output parameter.
  - `spatial_mgr` (index::SpatialIndexManager *): Input/output parameter.
  - `table` (const std::string &): Input parameter.
  - `pk` (const std::string &): Input parameter.
  - `old_blob` (const std::vector< uint8_t > &): Input parameter.
- Return: True when the operation succeeds.
- Details: Phase 2: Atomic entity DELETE with spatial index update via WriteBatch. batch Input/output parameter. spatial_mgr Input/output parameter. table Input parameter. pk Input parameter. old_blob Input parameter. True when the operation succeeds. batch Input/output parameter. spatial_mgr Input/output parameter. table Input parameter. pk Input parameter. old_blob Input parameter. True when the operation succeeds. Calls: hasSpatialIndex(), blob_str(), data(), size(), json::parse(), contains(), is_string(), reserve().

#### `void onEntityPut(RocksDBWrapper &db, index::SpatialIndexManager *spatial_mgr, const std::string &table, const std::string &pk, const std::vector< uint8_t > &blob)`
- Source: `include/api/geo_index_hooks.h`:38
- Brief: On Entity Put.
- Parameters:
  - `db` (RocksDBWrapper &): Input/output parameter.
  - `spatial_mgr` (index::SpatialIndexManager *): Input/output parameter.
  - `table` (const std::string &): Input parameter.
  - `pk` (const std::string &): Input parameter.
  - `blob` (const std::vector< uint8_t > &): Input parameter.
- Details: db Input/output parameter. spatial_mgr Input/output parameter. table Input parameter. pk Input parameter. blob Input parameter. db Input/output parameter. spatial_mgr Input/output parameter. table Input parameter. pk Input parameter. blob Input parameter. Calls: hasSpatialIndex(), empty(), blob_str(), data(), size(), nlohmann::json::parse(), BaseEntity::deserialize(), toJson().

#### `bool onEntityPutAtomic(RocksDBWrapper::WriteBatchWrapper &batch, index::SpatialIndexManager *spatial_mgr, const std::string &table, const std::string &pk, const std::vector< uint8_t > &blob)`
- Source: `include/api/geo_index_hooks.h`:55
- Brief: On Entity Put Atomic.
- Parameters:
  - `batch` (RocksDBWrapper::WriteBatchWrapper &): Input/output parameter.
  - `spatial_mgr` (index::SpatialIndexManager *): Input/output parameter.
  - `table` (const std::string &): Input parameter.
  - `pk` (const std::string &): Input parameter.
  - `blob` (const std::vector< uint8_t > &): Input parameter.
- Return: True when the operation succeeds.
- Details: Phase 2: Atomic entity PUT with spatial index update via WriteBatch. batch Input/output parameter. spatial_mgr Input/output parameter. table Input parameter. pk Input parameter. blob Input parameter. True when the operation succeeds. batch Input/output parameter. spatial_mgr Input/output parameter. table Input parameter. pk Input parameter. blob Input parameter. True when the operation succeeds. Calls: hasSpatialIndex(), blob_str(), data(), size(), json::parse(), contains(), is_string(), reserve().

### themis::api::HttpRequest

#### `bool hasAuth() const noexcept`
- Source: `include/api/http_handler.h`:101
- Brief: n/a
- Parameters: none

#### `std::string_view header(std::string_view name) const noexcept`
- Source: `include/api/http_handler.h`:105
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

### themis::api::HttpResponse

#### `HttpResponse badRequest(std::string message="Bad Request")`
- Source: `include/api/http_handler.h`:154
- Brief: n/a
- Parameters:
  - `message` (std::string): n/a

#### `HttpResponse created(std::string body={})`
- Source: `include/api/http_handler.h`:135
- Brief: n/a
- Parameters:
  - `body` (std::string): n/a

#### `HttpResponse forbidden()`
- Source: `include/api/http_handler.h`:180
- Brief: Forbidden.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements forbidden without additional internal calls.

#### `HttpResponse internalError(std::string message="Internal Server Error")`
- Source: `include/api/http_handler.h`:201
- Brief: n/a
- Parameters:
  - `message` (std::string): n/a

#### `HttpResponse noContent()`
- Source: `include/api/http_handler.h`:148
- Brief: No Content.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements noContent without additional internal calls.

#### `HttpResponse notFound()`
- Source: `include/api/http_handler.h`:193
- Brief: Not Found.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements notFound without additional internal calls.

#### `HttpResponse ok(std::string body={}, std::string content_type="application/json")`
- Source: `include/api/http_handler.h`:127
- Brief: n/a
- Parameters:
  - `body` (std::string): n/a
  - `content_type` (std::string): n/a

#### `HttpResponse unauthorized()`
- Source: `include/api/http_handler.h`:167
- Brief: Unauthorized.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements unauthorized without additional internal calls.

### themis::api::IAPIGatewayHook

#### `GatewayHookResult execute(GatewayHookContext &ctx)=0`
- Source: `include/api/api_gateway_hook.h`:85
- Brief: n/a
- Parameters:
  - `ctx` (GatewayHookContext &): n/a

#### `std::string hookId() const =0`
- Source: `include/api/api_gateway_hook.h`:79
- Brief: Hook Id.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool isEnabled() const`
- Source: `include/api/api_gateway_hook.h`:87
- Brief: n/a
- Parameters: none

#### `GatewayHookPhase phase() const =0`
- Source: `include/api/api_gateway_hook.h`:81
- Brief: n/a
- Parameters: none

#### `int priority() const`
- Source: `include/api/api_gateway_hook.h`:83
- Brief: n/a
- Parameters: none

#### `~IAPIGatewayHook()=default`
- Source: `include/api/api_gateway_hook.h`:72
- Brief: IAPIGateway Hook.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::api::IAPIVersionRouter

#### `void registerVersion(VersionDescriptor version, HandlerSet handlers)=0`
- Source: `include/api/api_version_router.h`:97
- Brief: Register Version.
- Parameters:
  - `version` (VersionDescriptor): Input parameter.
  - `handlers` (HandlerSet): Input parameter.
- Details: version Input parameter. handlers Input parameter.

#### `std::vector< VersionDescriptor > registeredVersions() const =0`
- Source: `include/api/api_version_router.h`:107
- Brief: Registered Versions.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `IHttpHandler & route(std::string_view method, std::string_view path, std::unordered_map< std::string, std::string > *out_deprecation_headers=nullptr)=0`
- Source: `include/api/api_version_router.h`:99
- Brief: n/a
- Parameters:
  - `method` (std::string_view): n/a
  - `path` (std::string_view): n/a
  - `out_deprecation_headers` (std::unordered_map< std::string, std::string > *): n/a

#### `~IAPIVersionRouter()=default`
- Source: `include/api/api_version_router.h`:90
- Brief: IAPIVersion Router.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::api::ICorrelationIDProvider

#### `CorrelationId extract(const std::unordered_map< std::string, std::string > &headers) const =0`
- Source: `include/api/correlation_id.h`:233
- Brief: n/a
- Parameters:
  - `headers` (const std::unordered_map< std::string, std::string > &): n/a

#### `CorrelationId generate() const =0`
- Source: `include/api/correlation_id.h`:231
- Brief: Generate.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string serialize(const CorrelationId &id)`
- Source: `include/api/correlation_id.h`:241
- Brief: Serialize.
- Parameters:
  - `id` (const CorrelationId &): Input parameter.
- Return: Return value.
- Details: id Input parameter. Return value. Calls: toString().

#### `~ICorrelationIDProvider()=default`
- Source: `include/api/correlation_id.h`:223
- Brief: ICorrelation IDProvider.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::api::IGRPCBridge

#### `themis::Result< HttpResponse > dispatch(const GRPCRequest &request)=0`
- Source: `include/api/grpc_bridge.h`:97
- Brief: Dispatch.
- Parameters:
  - `request` (const GRPCRequest &): Input parameter.
- Return: Return value.
- Details: request Input parameter. Return value.

#### `void registerService(ServiceDescriptor service, IHttpHandler &handler)=0`
- Source: `include/api/grpc_bridge.h`:90
- Brief: Register Service.
- Parameters:
  - `service` (ServiceDescriptor): Input parameter.
  - `handler` (IHttpHandler &): Input/output parameter.
- Details: service Input parameter. handler Input/output parameter.

#### `std::vector< ServiceDescriptor > registeredServices() const =0`
- Source: `include/api/grpc_bridge.h`:103
- Brief: Registered Services.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `~IGRPCBridge()=default`
- Source: `include/api/grpc_bridge.h`:83
- Brief: IGRPCBridge.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::api::IGatewayHookRegistry

#### `std::vector< std::shared_ptr< IAPIGatewayHook > > getHooks(GatewayHookPhase phase) const =0`
- Source: `include/api/api_gateway_hook.h`:124
- Brief: Get Hooks.
- Parameters:
  - `phase` (GatewayHookPhase): Input parameter.
- Return: Return value.
- Details: phase Input parameter. Return value.

#### `bool registerHook(std::shared_ptr< IAPIGatewayHook > hook)=0`
- Source: `include/api/api_gateway_hook.h`:108
- Brief: Register Hook.
- Parameters:
  - `hook` (std::shared_ptr< IAPIGatewayHook >): Input parameter.
- Return: True when the operation succeeds.
- Details: hook Input parameter. True when the operation succeeds.

#### `bool unregisterHook(const std::string &hook_id)=0`
- Source: `include/api/api_gateway_hook.h`:116
- Brief: Unregister Hook.
- Parameters:
  - `hook_id` (const std::string &): Identifier of the hook.
- Return: True when the operation succeeds.
- Details: hook_id Identifier of the hook. True when the operation succeeds.

#### `~IGatewayHookRegistry()=default`
- Source: `include/api/api_gateway_hook.h`:100
- Brief: IGateway Hook Registry.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::api::IGraphQLSchemaBuilder

#### `IGraphQLSchemaBuilder & addMutation(GraphQLFieldDescriptor field)=0`
- Source: `include/api/graphql_schema_builder.h`:170
- Brief: Add Mutation.
- Parameters:
  - `field` (GraphQLFieldDescriptor): Input parameter.
- Return: Return value.
- Details: field Input parameter. Return value.

#### `IGraphQLSchemaBuilder & addQuery(GraphQLFieldDescriptor field)=0`
- Source: `include/api/graphql_schema_builder.h`:163
- Brief: Add Query.
- Parameters:
  - `field` (GraphQLFieldDescriptor): Input parameter.
- Return: Return value.
- Details: field Input parameter. Return value.

#### `IGraphQLSchemaBuilder & addType(GraphQLTypeDescriptor descriptor)=0`
- Source: `include/api/graphql_schema_builder.h`:156
- Brief: Add Type.
- Parameters:
  - `descriptor` (GraphQLTypeDescriptor): Input parameter.
- Return: Return value.
- Details: descriptor Input parameter. Return value.

#### `SchemaValidationResult build()=0`
- Source: `include/api/graphql_schema_builder.h`:176
- Brief: Build.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool isBuilt() const noexcept=0`
- Source: `include/api/graphql_schema_builder.h`:183
- Brief: Is Built.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Exception safety: noexcept.

#### `~IGraphQLSchemaBuilder()=default`
- Source: `include/api/graphql_schema_builder.h`:149
- Brief: IGraph QLSchema Builder.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::api::IHttpHandler

#### `themis::Result< HttpResponse > handle(const HttpRequest &request)=0`
- Source: `include/api/http_handler.h`:222
- Brief: n/a
- Parameters:
  - `request` (const HttpRequest &): n/a

#### `std::string_view handlerName() const noexcept=0`
- Source: `include/api/http_handler.h`:226
- Brief: n/a
- Parameters: none

#### `bool requiresAuthentication() const noexcept`
- Source: `include/api/http_handler.h`:224
- Brief: n/a
- Parameters: none

#### `~IHttpHandler()=default`
- Source: `include/api/http_handler.h`:220
- Brief: IHttp Handler.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::api::ISubscriptionMultiplexer

#### `std::vector< std::string > activeTopics() const =0`
- Source: `include/api/subscription_multiplexer.h`:120
- Brief: Active Topics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t connectionCount() const =0`
- Source: `include/api/subscription_multiplexer.h`:126
- Brief: Connection Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t publish(const SubscriptionEvent &event)=0`
- Source: `include/api/subscription_multiplexer.h`:107
- Brief: Publish.
- Parameters:
  - `event` (const SubscriptionEvent &): Input parameter.
- Return: Return value.
- Details: event Input parameter. Return value.

#### `bool subscribe(const std::string &connection_id, const std::vector< SubscriptionFilter > &filters)=0`
- Source: `include/api/subscription_multiplexer.h`:92
- Brief: Subscribe.
- Parameters:
  - `connection_id` (const std::string &): Identifier of the connection.
  - `filters` (const std::vector< SubscriptionFilter > &): Input parameter.
- Return: True when the operation succeeds.
- Details: connection_id Identifier of the connection. filters Input parameter. True when the operation succeeds.

#### `size_t subscriberCount(const std::string &topic) const =0`
- Source: `include/api/subscription_multiplexer.h`:114
- Brief: Subscriber Count.
- Parameters:
  - `topic` (const std::string &): Input parameter.
- Return: Return value.
- Details: topic Input parameter. Return value.

#### `bool unsubscribe(const std::string &connection_id, const std::vector< std::string > &topics={})=0`
- Source: `include/api/subscription_multiplexer.h`:97
- Brief: n/a
- Parameters:
  - `connection_id` (const std::string &): n/a
  - `topics` (const std::vector< std::string > &): n/a

#### `~ISubscriptionMultiplexer()=default`
- Source: `include/api/subscription_multiplexer.h`:84
- Brief: ISubscription Multiplexer.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::api::ITransportContract

#### `std::string_view adapterName() const noexcept=0`
- Source: `include/api/api_transport_contracts.h`:133
- Brief: n/a
- Parameters: none

#### `TransportCapability capabilities() const noexcept`
- Source: `include/api/api_transport_contracts.h`:106
- Brief: n/a
- Parameters: none

#### `TransportFailureClass classifyFailure(std::string_view method, std::string_view path, std::string_view content_type, std::size_t payload_bytes, std::string_view api_version) const noexcept`
- Source: `include/api/api_transport_contracts.h`:119
- Brief: n/a
- Parameters:
  - `method` (std::string_view): n/a
  - `path` (std::string_view): n/a
  - `content_type` (std::string_view): n/a
  - `payload_bytes` (std::size_t): n/a
  - `api_version` (std::string_view): n/a

#### `std::vector< std::string > supportedVersions() const`
- Source: `include/api/api_transport_contracts.h`:110
- Brief: n/a
- Parameters: none

#### `~ITransportContract()=default`
- Source: `include/api/api_transport_contracts.h`:104
- Brief: ITransport Contract.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::api::IWebSocketFrameCallback

#### `void onClose(WebSocketSession &session, WebSocketCloseCode code, std::string_view reason) noexcept=0`
- Source: `include/api/websocket_handler.h`:139
- Brief: On Close.
- Parameters:
  - `session` (WebSocketSession &): Input/output parameter.
  - `code` (WebSocketCloseCode): Input parameter.
  - `reason` (std::string_view): Input parameter.
- Details: session Input/output parameter. code Input parameter. reason Input parameter. Exception safety: noexcept.

#### `void onFrame(WebSocketSession &session, const WebSocketFrame &frame) noexcept=0`
- Source: `include/api/websocket_handler.h`:130
- Brief: On Frame.
- Parameters:
  - `session` (WebSocketSession &): Input/output parameter.
  - `frame` (const WebSocketFrame &): Input parameter.
- Details: session Input/output parameter. frame Input parameter. Exception safety: noexcept.

#### `~IWebSocketFrameCallback()=default`
- Source: `include/api/websocket_handler.h`:122
- Brief: IWeb Socket Frame Callback.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::api::IWebSocketHandler

#### `std::string_view handlerName() const noexcept=0`
- Source: `include/api/websocket_handler.h`:192
- Brief: n/a
- Parameters: none

#### `themis::Result< WebSocketSession * > upgrade(std::string_view method, std::string_view path, const std::unordered_map< std::string, std::string > &headers, IWebSocketFrameCallback &callback)=0`
- Source: `include/api/websocket_handler.h`:186
- Brief: n/a
- Parameters:
  - `method` (std::string_view): n/a
  - `path` (std::string_view): n/a
  - `headers` (const std::unordered_map< std::string, std::string > &): n/a
  - `callback` (IWebSocketFrameCallback &): n/a

#### `~IWebSocketHandler()=default`
- Source: `include/api/websocket_handler.h`:184
- Brief: IWeb Socket Handler.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::api::MiddlewareChain

#### `MiddlewareChain & append(std::shared_ptr< IHttpHandler > handler)`
- Source: `include/api/http_handler.h`:241
- Brief: Append.
- Parameters:
  - `handler` (std::shared_ptr< IHttpHandler >): Input parameter.
- Return: Return value.
- Details: handler Input parameter. Return value. Calls: push_back(), std::move().

#### `themis::Result< HttpResponse > handle(const HttpRequest &request) override`
- Source: `include/api/http_handler.h`:246
- Brief: n/a
- Parameters:
  - `request` (const HttpRequest &): n/a

#### `std::string_view handlerName() const noexcept override`
- Source: `include/api/http_handler.h`:260
- Brief: n/a
- Parameters: none

#### `themis::Result< HttpResponse > invokeAt(const HttpRequest &request, std::size_t idx)`
- Source: `include/api/http_handler.h`:272
- Brief: Invoke At.
- Parameters:
  - `request` (const HttpRequest &): Input parameter.
  - `idx` (std::size_t): Input parameter.
- Return: Return value.
- Details: request Input parameter. idx Input parameter. Return value. Calls: size(), tl::unexpected(), themis::Error(), handle(), has_value().

#### `bool requiresAuthentication() const noexcept override`
- Source: `include/api/http_handler.h`:250
- Brief: n/a
- Parameters: none

#### `std::size_t size() const noexcept`
- Source: `include/api/http_handler.h`:262
- Brief: n/a
- Parameters: none

### themis::api::OtlpExporter

#### `OtlpExporter(OtlpExporter &&)=delete`
- Source: `include/api/otlp_exporter.h`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (OtlpExporter &&): n/a

#### `OtlpExporter(OtlpExporterConfig config={})`
- Source: `include/api/otlp_exporter.h`:132
- Brief: n/a
- Parameters:
  - `config` (OtlpExporterConfig): n/a

#### `OtlpExporter(const OtlpExporter &)=delete`
- Source: `include/api/otlp_exporter.h`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (const OtlpExporter &): n/a

#### `std::string buildOtlpJson(const OtlpExporterConfig &cfg, const std::vector< SpanData > &spans)`
- Source: `include/api/otlp_exporter.h`:197
- Brief: Build Otlp Json.
- Parameters:
  - `cfg` (const OtlpExporterConfig &): Input parameter.
  - `spans` (const std::vector< SpanData > &): Input parameter.
- Return: Return value.
- Details: static cfg Input parameter. spans Input parameter. Return value. cfg Input parameter. spans Input parameter. Return value. Calls: json::array(), push_back(), addAttr(), empty(), normaliseTraceId(), deriveSpanId(), substr(), std::to_string().

#### `const OtlpExporterConfig & config() const noexcept`
- Source: `include/api/otlp_exporter.h`:170
- Brief: n/a
- Parameters: none

#### `uint64_t droppedSpanCount() const noexcept`
- Source: `include/api/otlp_exporter.h`:168
- Brief: Dropped Span Count.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `void enqueue(SpanData span)`
- Source: `include/api/otlp_exporter.h`:154
- Brief: Enqueue.
- Parameters:
  - `span` (SpanData): Input parameter.
- Details: span Input parameter. span Input parameter. Calls: lk(), size(), pop_front(), fetch_add(), Increment(), THEMIS_WARN(), push_back(), std::move().

#### `uint64_t exportedSpanCount() const noexcept`
- Source: `include/api/otlp_exporter.h`:161
- Brief: Exported Span Count.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `void flushBatch(std::vector< SpanData > &batch)`
- Source: `include/api/otlp_exporter.h`:189
- Brief: Flush Batch.
- Parameters:
  - `batch` (std::vector< SpanData > &): Input/output parameter.
- Details: batch Input/output parameter. batch Input/output parameter. Calls: buildOtlpJson(), std::max(), curl_easy_init(), THEMIS_ERROR(), size(), fetch_add(), Increment(), curl_easy_setopt().

#### `void flushLoop()`
- Source: `include/api/otlp_exporter.h`:184
- Brief: Flush Loop.
- Parameters: none
- Details: Calls: std::chrono::milliseconds(), reserve(), lk(), wait_for(), load(), size(), std::min(), assign().

#### `OtlpExporter & operator=(OtlpExporter &&)=delete`
- Source: `include/api/otlp_exporter.h`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (OtlpExporter &&): n/a

#### `OtlpExporter & operator=(const OtlpExporter &)=delete`
- Source: `include/api/otlp_exporter.h`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (const OtlpExporter &): n/a

#### `void start()`
- Source: `include/api/otlp_exporter.h`:143
- Brief: Start.
- Parameters: none
- Details: Calls: joinable(), curl_easy_init(), curl_easy_setopt(), c_str(), empty(), curl_slist_append(), THEMIS_WARN(), prometheus::BuildCounter().

#### `void stop()`
- Source: `include/api/otlp_exporter.h`:148
- Brief: Stop.
- Parameters: none
- Details: Calls: joinable(), lk(), store(), notify_all(), join(), curl_slist_free_all(), curl_easy_cleanup(), THEMIS_INFO().

#### `~OtlpExporter()`
- Source: `include/api/otlp_exporter.h`:133
- Brief: n/a
- Parameters: none

### themis::api::SchemaValidationResult

#### `SchemaValidationResult fail(std::string type_name, std::string field_name, std::string message)`
- Source: `include/api/graphql_schema_builder.h`:130
- Brief: Fail.
- Parameters:
  - `type_name` (std::string): Name of the type.
  - `field_name` (std::string): Name of the field.
  - `message` (std::string): Input parameter.
- Return: Return value.
- Details: type_name Name of the type. field_name Name of the field. message Input parameter. Return value. Calls: std::move().

#### `SchemaValidationResult fail(std::vector< SchemaValidationError > errs)`
- Source: `include/api/graphql_schema_builder.h`:118
- Brief: Fail.
- Parameters:
  - `errs` (std::vector< SchemaValidationError >): Input parameter.
- Return: Return value.
- Details: errs Input parameter. Return value. Calls: std::move().

#### `SchemaValidationResult ok()`
- Source: `include/api/graphql_schema_builder.h`:108
- Brief: Ok.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements ok without additional internal calls.

### themis::api::ThemisDBGrpcService

#### `ThemisDBGrpcService(std::shared_ptr< RocksDBWrapper > db, std::shared_ptr< TransactionManager > txn_mgr)`
- Source: `include/api/themisdb_grpc_service.h`:43
- Brief: n/a
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): n/a
  - `txn_mgr` (std::shared_ptr< TransactionManager >): n/a

#### `ThemisDBGrpcService(std::shared_ptr< RocksDBWrapper > db, std::shared_ptr< TransactionManager > txn_mgr, std::shared_ptr< themis::IQueryEngine > aql_engine, std::shared_ptr< themis::IVectorIndex > vector_index)`
- Source: `include/api/themisdb_grpc_service.h`:48
- Brief: n/a
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): n/a
  - `txn_mgr` (std::shared_ptr< TransactionManager >): n/a
  - `aql_engine` (std::shared_ptr< themis::IQueryEngine >): n/a
  - `vector_index` (std::shared_ptr< themis::IVectorIndex >): n/a

#### `void buildImpl()`
- Source: `include/api/themisdb_grpc_service.h`:91
- Brief: Build Impl.
- Parameters: none
- Details: Calls: THEMIS_WARN(), lock(), fn(), THEMIS_ERROR(), what().

#### `void * service()`
- Source: `include/api/themisdb_grpc_service.h`:61
- Brief: Service.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result. Pointer to the result. Calls: get().

#### `void setServiceFn(ServiceFn fn)`
- Source: `include/api/themisdb_grpc_service.h`:67
- Brief: Set Service Fn.
- Parameters:
  - `fn` (ServiceFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lock(), std::move().

#### `~ThemisDBGrpcService()`
- Source: `include/api/themisdb_grpc_service.h`:55
- Brief: n/a
- Parameters: none

### themis::api::ThemisDBGrpcServiceFactory

#### `ThemisDBGrpcServiceFactory()=default`
- Source: `include/api/themisdb_grpc_service_factory.h`:32
- Brief: n/a
- Parameters: none

#### `std::unique_ptr< ThemisDBGrpcService > build() const`
- Source: `include/api/themisdb_grpc_service_factory.h`:82
- Brief: n/a
- Parameters: none

#### `ThemisDBGrpcServiceFactory & withDb(std::shared_ptr< RocksDBWrapper > db)`
- Source: `include/api/themisdb_grpc_service_factory.h`:40
- Brief: With Db.
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): Input parameter.
- Return: Return value.
- Details: db Input parameter. Return value. Calls: std::move().

#### `ThemisDBGrpcServiceFactory & withQueryEngine(std::shared_ptr< themis::IQueryEngine > engine)`
- Source: `include/api/themisdb_grpc_service_factory.h`:64
- Brief: With Query Engine.
- Parameters:
  - `engine` (std::shared_ptr< themis::IQueryEngine >): Input parameter.
- Return: Return value.
- Details: engine Input parameter. Return value. Calls: std::move().

#### `ThemisDBGrpcServiceFactory & withTxnMgr(std::shared_ptr< TransactionManager > txn_mgr)`
- Source: `include/api/themisdb_grpc_service_factory.h`:52
- Brief: With Txn Mgr.
- Parameters:
  - `txn_mgr` (std::shared_ptr< TransactionManager >): Input parameter.
- Return: Return value.
- Details: txn_mgr Input parameter. Return value. Calls: std::move().

#### `ThemisDBGrpcServiceFactory & withVectorIndex(std::shared_ptr< themis::IVectorIndex > index)`
- Source: `include/api/themisdb_grpc_service_factory.h`:76
- Brief: With Vector Index.
- Parameters:
  - `index` (std::shared_ptr< themis::IVectorIndex >): Input parameter.
- Return: Return value.
- Details: index Input parameter. Return value. Calls: std::move().

### themis::api::TracingMiddleware

#### `TracingMiddleware()=default`
- Source: `include/api/tracing_middleware.h`:60
- Brief: n/a
- Parameters: none

#### `TracingMiddleware(OtlpExporter *exporter)`
- Source: `include/api/tracing_middleware.h`:67
- Brief: Tracing Middleware.
- Parameters:
  - `exporter` (OtlpExporter *): Input/output parameter.
- Return: Return value.
- Details: exporter Input/output parameter. Return value.

#### `TracingMiddleware(TracingMiddleware &&) noexcept=default`
- Source: `include/api/tracing_middleware.h`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (TracingMiddleware &&): n/a

#### `TracingMiddleware(const TracingMiddleware &)=delete`
- Source: `include/api/tracing_middleware.h`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TracingMiddleware &): n/a

#### `void clearContext() noexcept`
- Source: `include/api/tracing_middleware.h`:99
- Brief: Clear Context.
- Parameters: none
- Details: Exception safety: noexcept.

#### `const std::string & currentCorrelationId() noexcept`
- Source: `include/api/tracing_middleware.h`:93
- Brief: Current Correlation Id.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `void finishSpan(std::string_view span_name, int http_status=0) const`
- Source: `include/api/tracing_middleware.h`:86
- Brief: n/a
- Parameters:
  - `span_name` (std::string_view): n/a
  - `http_status` (int): n/a

#### `std::string generateUuidV4()`
- Source: `include/api/tracing_middleware.h`:105
- Brief: Generate Uuid V4.
- Parameters: none
- Return: Return value.
- Details: static Return value. Return value. Calls: boost::uuids::to_string(), gen().

#### `TracingMiddleware & operator=(TracingMiddleware &&) noexcept=default`
- Source: `include/api/tracing_middleware.h`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (TracingMiddleware &&): n/a

#### `TracingMiddleware & operator=(const TracingMiddleware &)=delete`
- Source: `include/api/tracing_middleware.h`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TracingMiddleware &): n/a

#### `std::string processRequest(std::string_view incoming_id) const`
- Source: `include/api/tracing_middleware.h`:84
- Brief: Process Request.
- Parameters:
  - `incoming_id` (std::string_view): Identifier of the incoming.
- Return: Return value.
- Details: incoming_id Identifier of the incoming. Return value.

#### `~TracingMiddleware()=default`
- Source: `include/api/tracing_middleware.h`:69
- Brief: n/a
- Parameters: none

### themis::api::TransportContractValidator

#### `bool isPathLengthValid(std::string_view path) noexcept`
- Source: `include/api/api_transport_contracts.h`:163
- Brief: n/a
- Parameters:
  - `path` (std::string_view): n/a

#### `bool isPayloadWithinLimit(std::size_t payload_bytes) noexcept`
- Source: `include/api/api_transport_contracts.h`:154
- Brief: n/a
- Parameters:
  - `payload_bytes` (std::size_t): n/a

#### `bool isSupportedVersion(std::string_view version) noexcept`
- Source: `include/api/api_transport_contracts.h`:142
- Brief: n/a
- Parameters:
  - `version` (std::string_view): n/a

#### `bool requiresContentType(std::string_view method) noexcept`
- Source: `include/api/api_transport_contracts.h`:159
- Brief: n/a
- Parameters:
  - `method` (std::string_view): n/a

#### `TransportFailureClass validate(std::string_view method, std::string_view path, std::string_view content_type, std::size_t payload_bytes, std::string_view api_version) noexcept`
- Source: `include/api/api_transport_contracts.h`:168
- Brief: n/a
- Parameters:
  - `method` (std::string_view): n/a
  - `path` (std::string_view): n/a
  - `content_type` (std::string_view): n/a
  - `payload_bytes` (std::size_t): n/a
  - `api_version` (std::string_view): n/a

### themis::api::TransportPolicyConfig

#### `TransportPolicyConfig normalized() const noexcept`
- Source: `include/api/api_transport_policy.h`:62
- Brief: n/a
- Parameters: none

### themis::api::TransportPolicyMiddleware

#### `TransportPolicyMiddleware(TransportPolicyMiddleware &&) noexcept=default`
- Source: `include/api/api_transport_policy.h`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportPolicyMiddleware &&): n/a

#### `TransportPolicyMiddleware(const TransportPolicyMiddleware &)=delete`
- Source: `include/api/api_transport_policy.h`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TransportPolicyMiddleware &): n/a

#### `TransportPolicyMiddleware(std::shared_ptr< IHttpHandler > inner)`
- Source: `include/api/api_transport_policy.h`:86
- Brief: Transport Policy Middleware.
- Parameters:
  - `inner` (std::shared_ptr< IHttpHandler >): Input parameter.
- Return: Return value.
- Details: inner Input parameter. Return value.

#### `TransportPolicyMiddleware(std::shared_ptr< IHttpHandler > inner, const TransportPolicyConfig &config)`
- Source: `include/api/api_transport_policy.h`:88
- Brief: n/a
- Parameters:
  - `inner` (std::shared_ptr< IHttpHandler >): n/a
  - `config` (const TransportPolicyConfig &): n/a

#### `std::string_view adapterName() const noexcept override`
- Source: `include/api/api_transport_policy.h`:116
- Brief: n/a
- Parameters: none

#### `TransportFailureClass applyPolicy(const HttpRequest &request) const noexcept`
- Source: `include/api/api_transport_policy.h`:130
- Brief: n/a
- Parameters:
  - `request` (const HttpRequest &): n/a

#### `TransportCapability capabilities() const noexcept override`
- Source: `include/api/api_transport_policy.h`:114
- Brief: n/a
- Parameters: none

#### `const TransportPolicyConfig & config() const noexcept`
- Source: `include/api/api_transport_policy.h`:122
- Brief: n/a
- Parameters: none

#### `themis::Result< HttpResponse > handle(const HttpRequest &request) override`
- Source: `include/api/api_transport_policy.h`:103
- Brief: Handle.
- Parameters:
  - `request` (const HttpRequest &): Input parameter.
- Return: Return value.
- Details: request Input parameter. Return value. Calls: applyPolicy(), ApiErrorTaxonomy::toErrorCode(), ApiErrorTaxonomy::toMessage(), adapterName(), tl::unexpected(), themis::Error().

#### `std::string_view handlerName() const noexcept override`
- Source: `include/api/api_transport_policy.h`:108
- Brief: n/a
- Parameters: none

#### `TransportPolicyMiddleware & operator=(TransportPolicyMiddleware &&) noexcept=default`
- Source: `include/api/api_transport_policy.h`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportPolicyMiddleware &&): n/a

#### `TransportPolicyMiddleware & operator=(const TransportPolicyMiddleware &)=delete`
- Source: `include/api/api_transport_policy.h`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TransportPolicyMiddleware &): n/a

#### `bool requiresAuthentication() const noexcept override`
- Source: `include/api/api_transport_policy.h`:106
- Brief: n/a
- Parameters: none

#### `~TransportPolicyMiddleware() override=default`
- Source: `include/api/api_transport_policy.h`:91
- Brief: n/a
- Parameters: none

### themis::api::VersionDescriptor

#### `VersionDescriptor current(int major, int minor=0, std::string label={})`
- Source: `include/api/api_version_router.h`:46
- Brief: n/a
- Parameters:
  - `major` (int): n/a
  - `minor` (int): n/a
  - `label` (std::string): n/a

#### `VersionDescriptor deprecated(int major, int minor, std::string deprecation_date, std::string sunset_date={}, std::string successor_url={})`
- Source: `include/api/api_version_router.h`:50
- Brief: n/a
- Parameters:
  - `major` (int): n/a
  - `minor` (int): n/a
  - `deprecation_date` (std::string): n/a
  - `sunset_date` (std::string): n/a
  - `successor_url` (std::string): n/a

### themis::api::WebSocketFrame

#### `WebSocketFrame binary(std::string data)`
- Source: `include/api/websocket_handler.h`:107
- Brief: Binary.
- Parameters:
  - `data` (std::string): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: std::move().

#### `WebSocketFrame text(std::string data)`
- Source: `include/api/websocket_handler.h`:97
- Brief: Text.
- Parameters:
  - `data` (std::string): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: std::move().

### themis::api::WebSocketSession

#### `WebSocketSession()=default`
- Source: `include/api/websocket_handler.h`:171
- Brief: n/a
- Parameters: none

#### `WebSocketSession(const WebSocketSession &)=delete`
- Source: `include/api/websocket_handler.h`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WebSocketSession &): n/a

#### `void close(WebSocketCloseCode code, std::string_view reason={}) noexcept=0`
- Source: `include/api/websocket_handler.h`:161
- Brief: n/a
- Parameters:
  - `code` (WebSocketCloseCode): n/a
  - `reason` (std::string_view): n/a

#### `bool isOpen() const noexcept=0`
- Source: `include/api/websocket_handler.h`:168
- Brief: n/a
- Parameters: none

#### `WebSocketSession & operator=(const WebSocketSession &)=delete`
- Source: `include/api/websocket_handler.h`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WebSocketSession &): n/a

#### `std::string_view remoteAddress() const noexcept=0`
- Source: `include/api/websocket_handler.h`:164
- Brief: n/a
- Parameters: none

#### `bool send(WebSocketFrame frame)=0`
- Source: `include/api/websocket_handler.h`:159
- Brief: n/a
- Parameters:
  - `frame` (WebSocketFrame): n/a

#### `std::string_view sessionId() const noexcept=0`
- Source: `include/api/websocket_handler.h`:166
- Brief: n/a
- Parameters: none

#### `~WebSocketSession()=default`
- Source: `include/api/websocket_handler.h`:154
- Brief: Web Socket Session.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::graphql

#### `size_t computeSelectionDepth(const std::vector< Field > &selections, size_t current=1)`
- Source: `src/api/graphql.cpp`:809
- Brief: n/a
- Parameters:
  - `selections` (const std::vector< Field > &): n/a
  - `current` (size_t): n/a

#### `nlohmann::json gqlValueToJson(const std::shared_ptr< Value > &v)`
- Source: `src/api/graphql_aql_resolver.cpp`:208
- Brief: Gql Value To Json.
- Parameters:
  - `v` (const std::shared_ptr< Value > &): Input parameter.
- Return: Return value.
- Details: v Input parameter. Return value. v Input parameter. Return value. Calls: nlohmann::json(), isNull(), isBool(), asBool(), isInt(), asInt(), isFloat(), asFloat().

#### `std::shared_ptr< Value > jsonToGqlValue(const nlohmann::json &j)`
- Source: `src/api/graphql_aql_resolver.cpp`:174
- Brief: Json To Gql Value.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: is_null(), Value::null(), is_boolean(), Value::boolean(), is_number_integer(), Value::integer(), is_number_float(), Value::floating().

#### `std::string makeComplexityErrorMessage(uint32_t actual, uint32_t budget)`
- Source: `src/api/graphql_aql_resolver.cpp`:102
- Brief: Make Complexity Error Message.
- Parameters:
  - `actual` (uint32_t): Input parameter.
  - `budget` (uint32_t): Input parameter.
- Return: Return value.
- Details: actual Input parameter. budget Input parameter. Return value. actual Input parameter. budget Input parameter. Return value. Calls: str().

### themis::graphql::AuditLogBuilder

#### `AuditLogBuilder(AuditLogEntry::EventType type)`
- Source: `include/api/graphql_audit_logger.h`:384
- Brief: n/a
- Parameters:
  - `type` (AuditLogEntry::EventType): n/a

#### `AuditLogEntry build() const`
- Source: `include/api/graphql_audit_logger.h`:511
- Brief: n/a
- Parameters: none

#### `AuditLogBuilder & complexity(size_t complexity)`
- Source: `include/api/graphql_audit_logger.h`:486
- Brief: Complexity.
- Parameters:
  - `complexity` (size_t): Input parameter.
- Return: Return value.
- Details: complexity Input parameter. Return value. Implements complexity without additional internal calls.

#### `AuditLogBuilder & error(const std::string &error_msg)`
- Source: `include/api/graphql_audit_logger.h`:463
- Brief: Error.
- Parameters:
  - `error_msg` (const std::string &): Input parameter.
- Return: Return value.
- Details: error_msg Input parameter. Return value. Implements error without additional internal calls.

#### `AuditLogBuilder & ipAddress(const std::string &ip)`
- Source: `include/api/graphql_audit_logger.h`:441
- Brief: Ip Address.
- Parameters:
  - `ip` (const std::string &): Input parameter.
- Return: Return value.
- Details: ip Input parameter. Return value. Implements ipAddress without additional internal calls.

#### `void log()`
- Source: `include/api/graphql_audit_logger.h`:507
- Brief: Log.
- Parameters: none
- Details: Calls: AuditLogger::instance().

#### `AuditLogBuilder & metadata(const std::string &key, const std::string &value)`
- Source: `include/api/graphql_audit_logger.h`:498
- Brief: Metadata.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. value Input parameter. Return value. Implements metadata without additional internal calls.

#### `AuditLogBuilder & operationName(const std::string &name)`
- Source: `include/api/graphql_audit_logger.h`:397
- Brief: Operation Name.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: Return value.
- Details: name Input parameter. Return value. Implements operationName without additional internal calls.

#### `AuditLogBuilder & operationType(const std::string &type)`
- Source: `include/api/graphql_audit_logger.h`:408
- Brief: Operation Type.
- Parameters:
  - `type` (const std::string &): Input parameter.
- Return: Return value.
- Details: type Input parameter. Return value. Implements operationType without additional internal calls.

#### `AuditLogBuilder & queryHash(const std::string &hash)`
- Source: `include/api/graphql_audit_logger.h`:475
- Brief: Query Hash.
- Parameters:
  - `hash` (const std::string &): Input parameter.
- Return: Return value.
- Details: hash Input parameter. Return value. Implements queryHash without additional internal calls.

#### `AuditLogBuilder & success(bool succeeded)`
- Source: `include/api/graphql_audit_logger.h`:452
- Brief: Success.
- Parameters:
  - `succeeded` (bool): Input parameter.
- Return: Return value.
- Details: succeeded Input parameter. Return value. Implements success without additional internal calls.

#### `AuditLogBuilder & tenant(const std::string &tenant_id)`
- Source: `include/api/graphql_audit_logger.h`:430
- Brief: Tenant.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. Return value. Implements tenant without additional internal calls.

#### `AuditLogBuilder & user(const std::string &user_id)`
- Source: `include/api/graphql_audit_logger.h`:419
- Brief: User.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: user_id Identifier of the user. Return value. Implements user without additional internal calls.

### themis::graphql::AuditLogEntry

#### `std::string eventTypeToString(EventType type)`
- Source: `include/api/graphql_audit_logger.h`:103
- Brief: Event Type To String.
- Parameters:
  - `type` (EventType): Input parameter.
- Return: Return value.
- Details: type Input parameter. Return value. Implements eventTypeToString without additional internal calls.

#### `std::string formatTimestamp() const`
- Source: `include/api/graphql_audit_logger.h`:150
- Brief: n/a
- Parameters: none

#### `std::string toJSON() const`
- Source: `include/api/graphql_audit_logger.h`:117
- Brief: n/a
- Parameters: none

### themis::graphql::AuditLogger

#### `AuditLogger()=default`
- Source: `include/api/graphql_audit_logger.h`:337
- Brief: n/a
- Parameters: none

#### `void addFileHandler(const std::string &path)`
- Source: `include/api/graphql_audit_logger.h`:221
- Brief: Add File Handler.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Details: path Input parameter. Calls: addHandler(), lk(), ofs(), is_open(), toJSON().

#### `void addHandler(const LogHandler &handler)`
- Source: `include/api/graphql_audit_logger.h`:202
- Brief: Add Handler.
- Parameters:
  - `handler` (const LogHandler &): Input parameter.
- Details: handler Input parameter. Calls: lock(), push_back().

#### `void clear()`
- Source: `include/api/graphql_audit_logger.h`:286
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `void clearHandlers()`
- Source: `include/api/graphql_audit_logger.h`:211
- Brief: Clear Handlers.
- Parameters: none
- Details: Calls: lock(), clear().

#### `std::vector< AuditLogEntry > getRecent(size_t count) const`
- Source: `include/api/graphql_audit_logger.h`:234
- Brief: n/a
- Parameters:
  - `count` (size_t): n/a

#### `Stats getStats() const`
- Source: `include/api/graphql_audit_logger.h`:316
- Brief: n/a
- Parameters: none

#### `AuditLogger & instance()`
- Source: `include/api/graphql_audit_logger.h`:331
- Brief: Instance.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements instance without additional internal calls.

#### `void log(const AuditLogEntry &entry)`
- Source: `include/api/graphql_audit_logger.h`:167
- Brief: Log.
- Parameters:
  - `entry` (const AuditLogEntry &): Input parameter.
- Details: entry Input parameter. Calls: lock(), handler(), size(), erase(), begin(), push_back().

#### `std::vector< AuditLogEntry > searchByEventType(AuditLogEntry::EventType type) const`
- Source: `include/api/graphql_audit_logger.h`:264
- Brief: n/a
- Parameters:
  - `type` (AuditLogEntry::EventType): n/a

#### `std::vector< AuditLogEntry > searchByUser(const std::string &user_id) const`
- Source: `include/api/graphql_audit_logger.h`:246
- Brief: n/a
- Parameters:
  - `user_id` (const std::string &): n/a

#### `void setBufferCapacity(size_t capacity)`
- Source: `include/api/graphql_audit_logger.h`:296
- Brief: Set Buffer Capacity.
- Parameters:
  - `capacity` (size_t): Input parameter.
- Details: capacity Input parameter. Calls: lock(), size(), erase(), begin().

### themis::graphql::AuditLogger::Stats

#### `double failureRate() const`
- Source: `include/api/graphql_audit_logger.h`:310
- Brief: n/a
- Parameters: none

### themis::graphql::Cache

#### `Cache(size_t max_size=1000, std::chrono::seconds ttl=std::chrono::seconds(300))`
- Source: `include/api/graphql_cache.h`:87
- Brief: n/a
- Parameters:
  - `max_size` (size_t): n/a
  - `ttl` (std::chrono::seconds): n/a

#### `void clear()`
- Source: `include/api/graphql_cache.h`:191
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `void eraseIf(Predicate pred)`
- Source: `include/api/graphql_cache.h`:175
- Brief: Erase If.
- Parameters:
  - `pred` (Predicate): Input parameter.
- Details: pred Input parameter. Calls: lock(), begin(), end(), pred(), erase().

#### `void evictLRU()`
- Source: `include/api/graphql_cache.h`:233
- Brief: Evict the least recently used entry (back of lru_order_).
- Parameters: none
- Details: O(1). Calls: empty(), back(), erase(), pop_back().

#### `std::shared_ptr< T > get(const std::string &key)`
- Source: `include/api/graphql_cache.h`:96
- Brief: Get.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value. Calls: lock(), find(), end(), isExpired(), erase(), splice(), begin().

#### `CacheStats getStats() const`
- Source: `include/api/graphql_cache.h`:208
- Brief: n/a
- Parameters: none

#### `void invalidate(const std::string &key)`
- Source: `include/api/graphql_cache.h`:160
- Brief: Invalidate.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Details: key Input parameter. Calls: lock(), find(), end(), erase().

#### `void put(const std::string &key, const T &value)`
- Source: `include/api/graphql_cache.h`:127
- Brief: Put.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const T &): Input parameter.
- Details: key Input parameter. value Input parameter. Calls: lock(), find(), end(), std::chrono::steady_clock::now(), splice(), begin(), size(), evictLRU().

#### `size_t size() const`
- Source: `include/api/graphql_cache.h`:218
- Brief: n/a
- Parameters: none

### themis::graphql::Cache::CacheEntry

#### `bool isExpired(std::chrono::seconds ttl) const`
- Source: `include/api/graphql_cache.h`:81
- Brief: n/a
- Parameters:
  - `ttl` (std::chrono::seconds): n/a

### themis::graphql::Cache::CacheStats

#### `double hitRate() const`
- Source: `include/api/graphql_cache.h`:202
- Brief: n/a
- Parameters: none

### themis::graphql::Document

#### `const Operation * getOperation(std::string_view name="") const`
- Source: `include/api/graphql.h`:258
- Brief: n/a
- Parameters:
  - `name` (std::string_view): n/a

### themis::graphql::Executor

#### `Result execute(const Document &document, const ExecutionContext &context, std::string_view operation_name="")`
- Source: `include/api/graphql.h`:569
- Brief: Execute.
- Parameters:
  - `document` (const Document &): Input parameter.
  - `context` (const ExecutionContext &): Input parameter.
  - `operation_name` (std::string_view): Name of the operation.
- Return: Return value.
- Details: document Input parameter. context Input parameter. operation_name Name of the operation. Return value. Calls: getOperation(), addError(), std::string(), size(), empty(), computeSelectionDepth(), timer(), executeOperation().

#### `std::shared_ptr< Value > executeField(const Field &field, const std::shared_ptr< Value > &parent, const ExecutionContext &context)`
- Source: `include/api/graphql.h`:607
- Brief: Execute Field.
- Parameters:
  - `field` (const Field &): Input parameter.
  - `parent` (const std::shared_ptr< Value > &): Input parameter.
  - `context` (const ExecutionContext &): Input parameter.
- Return: Return value.
- Details: field Input parameter. parent Input parameter. context Input parameter. Return value. field Input parameter. parent Input parameter. context Input parameter. Return value. Calls: empty(), isVariableRef(), resolveValue(), find(), end(), second(), isObject(), asObject().

#### `std::shared_ptr< Value > executeOperation(const Operation &operation, const ExecutionContext &context)`
- Source: `include/api/graphql.h`:582
- Brief: Execute Operation.
- Parameters:
  - `operation` (const Operation &): Input parameter.
  - `context` (const ExecutionContext &): Input parameter.
- Return: Return value.
- Details: operation Input parameter. context Input parameter. Return value. operation Input parameter. context Input parameter. Return value. Calls: find(), end(), executeSelections().

#### `std::shared_ptr< Value > executeSelections(const std::vector< Field > &selections, const std::shared_ptr< Value > &parent, const ExecutionContext &context)`
- Source: `include/api/graphql.h`:594
- Brief: Execute Selections.
- Parameters:
  - `selections` (const std::vector< Field > &): Input parameter.
  - `parent` (const std::shared_ptr< Value > &): Input parameter.
  - `context` (const ExecutionContext &): Input parameter.
- Return: Return value.
- Details: selections Input parameter. parent Input parameter. context Input parameter. Return value. selections Input parameter. parent Input parameter. context Input parameter. Return value. Calls: executeField(), responseName(), Value::object(), std::move().

#### `std::shared_ptr< Value > resolveValue(const std::shared_ptr< Value > &value, const ExecutionContext &context)`
- Source: `include/api/graphql.h`:619
- Brief: Resolve Value.
- Parameters:
  - `value` (const std::shared_ptr< Value > &): Input parameter.
  - `context` (const ExecutionContext &): Input parameter.
- Return: Return value.
- Details: value Input parameter. context Input parameter. Return value. value Input parameter. context Input parameter. Return value. Calls: isVariableRef(), asVariableRef(), find(), end(), Value::null().

### themis::graphql::Executor::Result

#### `void addError(const std::string &message, const std::string &code="INTERNAL_ERROR", bool mask=true)`
- Source: `include/api/graphql.h`:562
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a
  - `code` (const std::string &): n/a
  - `mask` (bool): n/a

#### `bool hasErrors() const`
- Source: `include/api/graphql.h`:559
- Brief: n/a
- Parameters: none

### themis::graphql::Field

#### `const std::string & responseName() const`
- Source: `include/api/graphql.h`:228
- Brief: n/a
- Parameters: none

### themis::graphql::FileAuditLogHandler

#### `FileAuditLogHandler(const std::string &path)`
- Source: `include/api/graphql_audit_logger.h`:353
- Brief: File Audit Log Handler.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: path Input parameter. Return value.

#### `void operator()(const AuditLogEntry &entry)`
- Source: `include/api/graphql_audit_logger.h`:356
- Brief: n/a
- Parameters:
  - `entry` (const AuditLogEntry &): n/a

#### `const std::string & path() const`
- Source: `include/api/graphql_audit_logger.h`:375
- Brief: n/a
- Parameters: none

### themis::graphql::GraphQLAqlResolverFactory

#### `GraphQLAqlResolverFactory(::themis::QueryEngine *engine=nullptr)`
- Source: `include/api/graphql_aql_resolver.h`:171
- Brief: n/a
- Parameters:
  - `engine` (::themis::QueryEngine *): n/a

#### `::tl::expected< nlohmann::json, ::themis::query::QueryError > executeAqlWithLimits(const std::string &aql, ::themis::QueryEngine &eng, const ::themis::query::QueryResourceLimits &limits) const`
- Source: `include/api/graphql_aql_resolver.h`:217
- Brief: n/a
- Parameters:
  - `aql` (const std::string &): n/a
  - `eng` (::themis::QueryEngine &): n/a
  - `limits` (const ::themis::query::QueryResourceLimits &): n/a

#### `std::string extractStringArg(const Field &field, const std::string &argName) const`
- Source: `include/api/graphql_aql_resolver.h`:213
- Brief: Extract String Arg.
- Parameters:
  - `field` (const Field &): Input parameter.
  - `argName` (const std::string &): Input parameter.
- Return: Return value.
- Details: field Input parameter. argName Input parameter. Return value.

#### `void injectResolvers(ExecutionContext &ctx, const Document &doc, ::themis::QueryEngine *eng)`
- Source: `include/api/graphql_aql_resolver.h`:200
- Brief: n/a
- Parameters:
  - `ctx` (ExecutionContext &): n/a
  - `doc` (const Document &): n/a
  - `eng` (::themis::QueryEngine *): n/a

#### `ExecutionContext::Resolver makeApiVersionResolver()`
- Source: `include/api/graphql_aql_resolver.h`:192
- Brief: Make Api Version Resolver.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: Value::string().

#### `ExecutionContext::Resolver makeAqlMutationResolver(const Document &doc) const`
- Source: `include/api/graphql_aql_resolver.h`:186
- Brief: Make Aql Mutation Resolver.
- Parameters:
  - `doc` (const Document &): Input parameter.
- Return: Return value.
- Details: doc Input parameter. Return value.

#### `ExecutionContext::Resolver makeAqlQueryResolver(const Document &doc) const`
- Source: `include/api/graphql_aql_resolver.h`:179
- Brief: Make Aql Query Resolver.
- Parameters:
  - `doc` (const Document &): Input parameter.
- Return: Return value.
- Details: doc Input parameter. Return value.

#### `ExecutionContext::Resolver makeSchemaVersionResolver()`
- Source: `include/api/graphql_aql_resolver.h`:198
- Brief: Make Schema Version Resolver.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: Value::string().

### themis::graphql::GraphQLComplexityEstimator

#### `uint32_t estimate(const std::shared_ptr< Document > &doc)`
- Source: `include/api/graphql_aql_resolver.h`:127
- Brief: Estimate.
- Parameters:
  - `doc` (const std::shared_ptr< Document > &): Input parameter.
- Return: Return value.
- Details: doc Input parameter. Return value. doc Input parameter. Return value. Calls: empty(), checkedAdd(), scoreFieldsBounded().

#### `::themis::query::QueryResourceLimits limitsFor(uint32_t complexity)`
- Source: `include/api/graphql_aql_resolver.h`:134
- Brief: Limits For.
- Parameters:
  - `complexity` (uint32_t): Input parameter.
- Return: Return value.
- Details: complexity Input parameter. Return value.

#### `uint32_t scoreSelectionSet(const std::shared_ptr< SelectionSet > &set, uint32_t depth)`
- Source: `include/api/graphql_aql_resolver.h`:143
- Brief: Score Selection Set.
- Parameters:
  - `set` (const std::shared_ptr< SelectionSet > &): Input parameter.
  - `depth` (uint32_t): Input parameter.
- Return: Return value.
- Details: set Input parameter. depth Input parameter. Return value. set Input parameter. depth Input parameter. Return value. Implements scoreSelectionSet without additional internal calls.

### themis::graphql::MaskedError

#### `MaskedError fromInternalError(const std::string &internal_msg, const std::string &error_code="INTERNAL_ERROR", bool mask=true)`
- Source: `include/api/graphql.h`:512
- Brief: n/a
- Parameters:
  - `internal_msg` (const std::string &): n/a
  - `error_code` (const std::string &): n/a
  - `mask` (bool): n/a

#### `std::string toString() const`
- Source: `include/api/graphql.h`:538
- Brief: n/a
- Parameters: none

### themis::graphql::Metrics

#### `Metrics()=default`
- Source: `include/api/graphql_metrics.h`:201
- Brief: n/a
- Parameters: none

#### `std::unordered_map< std::string, QueryMetrics > getAllMetrics() const`
- Source: `include/api/graphql_metrics.h`:171
- Brief: n/a
- Parameters: none

#### `const QueryMetrics & getMetrics(const std::string &operation_type) const`
- Source: `include/api/graphql_metrics.h`:154
- Brief: n/a
- Parameters:
  - `operation_type` (const std::string &): n/a

#### `QueryMetrics & getMetricsForType(const std::string &operation_type)`
- Source: `include/api/graphql_metrics.h`:209
- Brief: Get Metrics For Type.
- Parameters:
  - `operation_type` (const std::string &): Input parameter.
- Return: Return value.
- Details: operation_type Input parameter. Return value. Calls: lock().

#### `Metrics & instance()`
- Source: `include/api/graphql_metrics.h`:195
- Brief: Instance.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements instance without additional internal calls.

#### `void recordQuery(const std::string &operation_type, uint64_t duration_ms, bool success, size_t depth, size_t field_count)`
- Source: `include/api/graphql_metrics.h`:127
- Brief: Record Query.
- Parameters:
  - `operation_type` (const std::string &): Input parameter.
  - `duration_ms` (uint64_t): Input parameter.
  - `success` (bool): Input parameter.
  - `depth` (size_t): Input parameter.
  - `field_count` (size_t): Input parameter.
- Details: operation_type Input parameter. duration_ms Input parameter. success Input parameter. depth Input parameter. field_count Input parameter. Calls: getMetricsForType(), fetch_add(), load(), compare_exchange_weak().

#### `void reset()`
- Source: `include/api/graphql_metrics.h`:185
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Calls: lock(), clear().

### themis::graphql::Metrics::QueryMetrics

#### `QueryMetrics()=default`
- Source: `include/api/graphql_metrics.h`:70
- Brief: n/a
- Parameters: none

#### `QueryMetrics(const QueryMetrics &other)`
- Source: `include/api/graphql_metrics.h`:72
- Brief: n/a
- Parameters:
  - `other` (const QueryMetrics &): n/a

#### `double avgExecutionTimeMs() const`
- Source: `include/api/graphql_metrics.h`:94
- Brief: n/a
- Parameters: none

#### `double avgFieldCount() const`
- Source: `include/api/graphql_metrics.h`:106
- Brief: n/a
- Parameters: none

#### `double avgQueryDepth() const`
- Source: `include/api/graphql_metrics.h`:100
- Brief: n/a
- Parameters: none

#### `double errorRate() const`
- Source: `include/api/graphql_metrics.h`:112
- Brief: n/a
- Parameters: none

#### `QueryMetrics & operator=(const QueryMetrics &other)`
- Source: `include/api/graphql_metrics.h`:81
- Brief: n/a
- Parameters:
  - `other` (const QueryMetrics &): n/a

### themis::graphql::OperationRateLimiter

#### `OperationRateLimiter()=default`
- Source: `include/api/rate_limiter.h`:405
- Brief: n/a
- Parameters: none

#### `bool allow(const std::string &operation_type, const std::string &key, size_t cost=1)`
- Source: `include/api/rate_limiter.h`:327
- Brief: n/a
- Parameters:
  - `operation_type` (const std::string &): n/a
  - `key` (const std::string &): n/a
  - `cost` (size_t): n/a

#### `void clear()`
- Source: `include/api/rate_limiter.h`:389
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `RateLimitHeaders getHeaders(const std::string &operation_type, const std::string &key)`
- Source: `include/api/rate_limiter.h`:369
- Brief: Get Headers.
- Parameters:
  - `operation_type` (const std::string &): Input parameter.
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: operation_type Input parameter. key Input parameter. Return value. Calls: lock(), find(), end(), getConfig(), remaining().

#### `OperationRateLimiter & instance()`
- Source: `include/api/rate_limiter.h`:399
- Brief: Instance.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements instance without additional internal calls.

#### `size_t remaining(const std::string &operation_type, const std::string &key)`
- Source: `include/api/rate_limiter.h`:351
- Brief: Remaining.
- Parameters:
  - `operation_type` (const std::string &): Input parameter.
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: operation_type Input parameter. key Input parameter. Return value. Calls: lock(), find(), end().

#### `void setLimit(const std::string &operation_type, const RateLimiter::Config &config)`
- Source: `include/api/rate_limiter.h`:316
- Brief: Set Limit.
- Parameters:
  - `operation_type` (const std::string &): Input parameter.
  - `config` (const RateLimiter::Config &): Input parameter.
- Details: operation_type Input parameter. config Input parameter. Calls: lock(), find(), end(), setConfig().

### themis::graphql::ParseError

#### `std::string toString() const`
- Source: `include/api/graphql.h`:273
- Brief: n/a
- Parameters: none

### themis::graphql::Parser

#### `Parser(std::string_view query, const QueryLimits &limits)`
- Source: `include/api/graphql.h`:348
- Brief: n/a
- Parameters:
  - `query` (std::string_view): n/a
  - `limits` (const QueryLimits &): n/a

#### `bool checkASTNodeLimit()`
- Source: `include/api/graphql.h`:393
- Brief: Check ASTNode Limit.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Calls: error(), std::to_string().

#### `bool checkDepthLimit(size_t depth)`
- Source: `include/api/graphql.h`:383
- Brief: Check Depth Limit.
- Parameters:
  - `depth` (size_t): Input parameter.
- Return: True when the operation succeeds.
- Details: depth Input parameter. True when the operation succeeds. depth Input parameter. True when the operation succeeds. Calls: error(), std::to_string().

#### `bool checkFieldLimit()`
- Source: `include/api/graphql.h`:388
- Brief: Check Field Limit.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Calls: error(), std::to_string().

#### `bool checkQuerySize()`
- Source: `include/api/graphql.h`:377
- Brief: Check Query Size.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Calls: size(), error(), std::to_string().

#### `ParseError convertToParseError(const themis::Error &error)`
- Source: `include/api/graphql.h`:471
- Brief: Convert To Parse Error.
- Parameters:
  - `error` (const themis::Error &): Input parameter.
- Return: Return value.
- Details: error Input parameter. Return value. error Input parameter. Return value. Calls: message().

#### `void error(std::string message)`
- Source: `include/api/graphql.h`:477
- Brief: Deprecated: Use Result<T> return types instead of error() method.
- Parameters:
  - `message` (std::string): Input parameter.
- Details: Error. message Input parameter. message Input parameter. Calls: std::move(), push_back().

#### `std::string getLocationContext() const`
- Source: `include/api/graphql.h`:465
- Brief: Get Location Context.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void incrementASTNodeCount()`
- Source: `include/api/graphql.h`:410
- Brief: Increment ASTNode Count.
- Parameters: none
- Details: Implements incrementASTNodeCount without additional internal calls.

#### `void incrementFieldCount()`
- Source: `include/api/graphql.h`:405
- Brief: Increment Field Count.
- Parameters: none
- Details: Implements incrementFieldCount without additional internal calls.

#### `bool isIntrospectionFieldName(std::string_view field_name) noexcept`
- Source: `include/api/graphql.h`:400
- Brief: Is Introspection Field Name.
- Parameters:
  - `field_name` (std::string_view): Name of the field.
- Return: True when the operation succeeds.
- Details: field_name Name of the field. True when the operation succeeds. Exception safety: noexcept.

#### `bool match(char c)`
- Source: `include/api/graphql.h`:426
- Brief: Match.
- Parameters:
  - `c` (char): Input parameter.
- Return: True when the operation succeeds.
- Details: c Input parameter. True when the operation succeeds. c Input parameter. True when the operation succeeds. Calls: size().

#### `bool match(std::string_view s)`
- Source: `include/api/graphql.h`:432
- Brief: Match.
- Parameters:
  - `s` (std::string_view): Input parameter.
- Return: True when the operation succeeds.
- Details: s Input parameter. True when the operation succeeds. s Input parameter. True when the operation succeeds. Calls: size(), substr(), std::isalnum().

#### `Result parse(std::string_view query)`
- Source: `include/api/graphql.h`:337
- Brief: Parse.
- Parameters:
  - `query` (std::string_view): Input parameter.
- Return: Return value.
- Details: query Input parameter. Return value. query Input parameter. Return value. Calls: query_str(), QueryPlanCache::instance(), get(), QueryLimits::defaults(), parser(), parseDocument(), put().

#### `Result parse(std::string_view query, const QueryLimits &limits)`
- Source: `include/api/graphql.h`:345
- Brief: Parse.
- Parameters:
  - `query` (std::string_view): Input parameter.
  - `limits` (const QueryLimits &): Input parameter.
- Return: Return value.
- Details: query Input parameter. limits Input parameter. Return value. query Input parameter. limits Input parameter. Return value. Calls: parser(), parseDocument().

#### `Result parseDocument()`
- Source: `include/api/graphql.h`:354
- Brief: Parse Document.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: checkQuerySize(), skipWhitespace(), size(), push_back(), parseOperation(), std::move(), convertToParseError(), error().

#### `themis::Result< Field > parseField(size_t depth=0)`
- Source: `include/api/graphql.h`:360
- Brief: Parse Field.
- Parameters:
  - `depth` (size_t): Input parameter.
- Return: Return value.
- Details: depth Input parameter. Return value. Calls: incrementFieldCount(), incrementASTNodeCount(), checkDepthLimit(), checkFieldLimit(), checkASTNodeLimit(), skipWhitespace(), match(), parseName().

#### `themis::Result< double > parseFloat()`
- Source: `include/api/graphql.h`:458
- Brief: Parse Float.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `themis::Result< int64_t > parseInt()`
- Source: `include/api/graphql.h`:453
- Brief: Parse Int.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `themis::Result< std::string > parseName()`
- Source: `include/api/graphql.h`:443
- Brief: Parse Name.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: size(), std::isalpha(), std::isalnum(), themis::Ok(), std::string(), substr(), getLocationContext().

#### `themis::Result< Operation > parseOperation()`
- Source: `include/api/graphql.h`:359
- Brief: Parse Operation.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: incrementASTNodeCount(), checkASTNodeLimit(), skipWhitespace(), match(), peek(), getLocationContext(), parseName(), size().

#### `themis::Result< std::string > parseString()`
- Source: `include/api/graphql.h`:448
- Brief: Parse String.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: match(), getLocationContext(), size(), themis::Ok(), std::move().

#### `themis::Result< std::shared_ptr< Value > > parseValue()`
- Source: `include/api/graphql.h`:365
- Brief: Parse Value.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: skipWhitespace(), match(), themis::Ok(), Value::null(), Value::boolean(), peek(), parseString(), error().

#### `themis::Result< VariableDefinition > parseVariableDefinition()`
- Source: `include/api/graphql.h`:370
- Brief: Parse Variable Definition.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: match(), getLocationContext(), parseName(), error(), code(), context(), skipWhitespace(), parseValue().

#### `bool peek(char c) const`
- Source: `include/api/graphql.h`:438
- Brief: Peek.
- Parameters:
  - `c` (char): Input parameter.
- Return: True when the operation succeeds.
- Details: c Input parameter. True when the operation succeeds.

#### `void skipComment()`
- Source: `include/api/graphql.h`:420
- Brief: Skip Comment.
- Parameters: none
- Details: Calls: size().

#### `void skipWhitespace()`
- Source: `include/api/graphql.h`:416
- Brief: Skip Whitespace.
- Parameters: none
- Details: Calls: size(), skipComment().

### themis::graphql::PersistedQueryRegistry

#### `PersistedQueryRegistry()=default`
- Source: `include/api/persisted_queries.h`:191
- Brief: n/a
- Parameters: none

#### `void clear()`
- Source: `include/api/persisted_queries.h`:165
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `bool deprecateQuery(const std::string &query_id, const std::string &reason)`
- Source: `include/api/persisted_queries.h`:123
- Brief: Deprecate Query.
- Parameters:
  - `query_id` (const std::string &): Identifier of the query.
  - `reason` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: query_id Identifier of the query. reason Input parameter. True when the operation succeeds. Calls: lock(), find(), end().

#### `std::vector< std::string > getAllQueryIds() const`
- Source: `include/api/persisted_queries.h`:145
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< PersistedQuery > getQuery(const std::string &query_id) const`
- Source: `include/api/persisted_queries.h`:101
- Brief: n/a
- Parameters:
  - `query_id` (const std::string &): n/a

#### `PersistedQueryRegistry & instance()`
- Source: `include/api/persisted_queries.h`:185
- Brief: Instance.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements instance without additional internal calls.

#### `bool isRegistered(const std::string &query_id) const`
- Source: `include/api/persisted_queries.h`:135
- Brief: n/a
- Parameters:
  - `query_id` (const std::string &): n/a

#### `bool registerQuery(const std::string &query_id, const std::string &query_text, const std::string &description="")`
- Source: `include/api/persisted_queries.h`:76
- Brief: n/a
- Parameters:
  - `query_id` (const std::string &): n/a
  - `query_text` (const std::string &): n/a
  - `description` (const std::string &): n/a

#### `size_t size() const`
- Source: `include/api/persisted_queries.h`:170
- Brief: n/a
- Parameters: none

### themis::graphql::QueryAllowList

#### `QueryAllowList()=default`
- Source: `include/api/persisted_queries.h`:279
- Brief: n/a
- Parameters: none

#### `void allow(const std::string &query_hash)`
- Source: `include/api/persisted_queries.h`:204
- Brief: Allow.
- Parameters:
  - `query_hash` (const std::string &): Input parameter.
- Details: query_hash Input parameter. Calls: lock(), insert().

#### `void clear()`
- Source: `include/api/persisted_queries.h`:233
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `QueryAllowList & instance()`
- Source: `include/api/persisted_queries.h`:273
- Brief: Instance.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements instance without additional internal calls.

#### `bool isAllowed(const std::string &query_hash) const`
- Source: `include/api/persisted_queries.h`:209
- Brief: n/a
- Parameters:
  - `query_hash` (const std::string &): n/a

#### `bool isEnabled() const`
- Source: `include/api/persisted_queries.h`:258
- Brief: n/a
- Parameters: none

#### `void remove(const std::string &query_hash)`
- Source: `include/api/persisted_queries.h`:224
- Brief: Remove.
- Parameters:
  - `query_hash` (const std::string &): Input parameter.
- Details: query_hash Input parameter. Calls: lock(), erase().

#### `void setEnabled(bool enabled)`
- Source: `include/api/persisted_queries.h`:253
- Brief: Set Enabled.
- Parameters:
  - `enabled` (bool): Input parameter.
- Details: enabled Input parameter. Calls: lock().

#### `size_t size() const`
- Source: `include/api/persisted_queries.h`:238
- Brief: n/a
- Parameters: none

### themis::graphql::QueryHasher

#### `std::string hash(const std::string &query)`
- Source: `include/api/persisted_queries.h`:294
- Brief: Hash.
- Parameters:
  - `query` (const std::string &): Input parameter.
- Return: Return value.
- Details: query Input parameter. Return value. Calls: std::to_string(), hasher().

#### `std::string normalize(const std::string &query)`
- Source: `include/api/persisted_queries.h`:306
- Brief: Normalize.
- Parameters:
  - `query` (const std::string &): Input parameter.
- Return: Return value.
- Details: query Input parameter. Return value. Calls: reserve(), size(), std::isspace(), empty(), back(), pop_back().

### themis::graphql::QueryLimits

#### `QueryLimits defaults()`
- Source: `include/api/graphql.h`:292
- Brief: Default safe limits (development / trusted context).
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

#### `QueryLimits permissive()`
- Source: `include/api/graphql.h`:301
- Brief: More permissive limits for trusted contexts.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements permissive without additional internal calls.

#### `QueryLimits production()`
- Source: `include/api/graphql.h`:317
- Brief: Production.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements production without additional internal calls.

### themis::graphql::QueryPlanCache

#### `QueryPlanCache()`
- Source: `include/api/graphql_cache.h`:306
- Brief: n/a
- Parameters: none

#### `void clear()`
- Source: `include/api/graphql_cache.h`:301
- Brief: Clear.
- Parameters: none
- Details: Implements clear without additional internal calls.

#### `std::shared_ptr< QueryPlan > get(const std::string &query)`
- Source: `include/api/graphql_cache.h`:279
- Brief: Get.
- Parameters:
  - `query` (const std::string &): Input parameter.
- Return: Return value.
- Details: query Input parameter. Return value. Implements get without additional internal calls.

#### `Cache< QueryPlan >::CacheStats getStats() const`
- Source: `include/api/graphql_cache.h`:293
- Brief: n/a
- Parameters: none

#### `QueryPlanCache & instance()`
- Source: `include/api/graphql_cache.h`:268
- Brief: Instance.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements instance without additional internal calls.

#### `void put(const std::string &query, const QueryPlan &plan)`
- Source: `include/api/graphql_cache.h`:289
- Brief: Put.
- Parameters:
  - `query` (const std::string &): Input parameter.
  - `plan` (const QueryPlan &): Input parameter.
- Details: query Input parameter. plan Input parameter. Implements put without additional internal calls.

### themis::graphql::QueryTimer

#### `QueryTimer(const std::string &operation_type, size_t depth, size_t field_count)`
- Source: `include/api/graphql_metrics.h`:220
- Brief: n/a
- Parameters:
  - `operation_type` (const std::string &): n/a
  - `depth` (size_t): n/a
  - `field_count` (size_t): n/a

#### `void setSuccess(bool success)`
- Source: `include/api/graphql_metrics.h`:246
- Brief: Set Success.
- Parameters:
  - `success` (bool): Input parameter.
- Details: success Input parameter. Implements setSuccess without additional internal calls.

#### `~QueryTimer()`
- Source: `include/api/graphql_metrics.h`:228
- Brief: n/a
- Parameters: none

### themis::graphql::RateLimitHeaders

#### `std::unordered_map< std::string, std::string > toHeaders() const`
- Source: `include/api/rate_limiter.h`:299
- Brief: n/a
- Parameters: none

### themis::graphql::RateLimiter

#### `RateLimiter(const Config &config=Config::defaults())`
- Source: `include/api/rate_limiter.h`:282
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `bool allow(const std::string &key, size_t cost=1)`
- Source: `include/api/rate_limiter.h`:147
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `cost` (size_t): n/a

#### `void clear()`
- Source: `include/api/rate_limiter.h`:223
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `Config getConfig() const`
- Source: `include/api/rate_limiter.h`:272
- Brief: n/a
- Parameters: none

#### `Stats getStats() const`
- Source: `include/api/rate_limiter.h`:258
- Brief: n/a
- Parameters: none

#### `size_t remaining(const std::string &key)`
- Source: `include/api/rate_limiter.h`:197
- Brief: Remaining.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value. Calls: std::chrono::steady_clock::now(), lock(), find(), end(), refill(), available().

#### `void reset(const std::string &key)`
- Source: `include/api/rate_limiter.h`:214
- Brief: Reset the modification detection flag.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Details: key Input parameter. Calls: lock(), erase().

#### `void setConfig(const Config &config)`
- Source: `include/api/rate_limiter.h`:267
- Brief: Set Config.
- Parameters:
  - `config` (const Config &): Input parameter.
- Details: config Input parameter. Calls: lock().

### themis::graphql::RateLimiter::Bucket

#### `Bucket(size_t cap, size_t rate)`
- Source: `include/api/rate_limiter.h`:108
- Brief: n/a
- Parameters:
  - `cap` (size_t): n/a
  - `rate` (size_t): n/a

#### `size_t available() const`
- Source: `include/api/rate_limiter.h`:142
- Brief: n/a
- Parameters: none

#### `bool consume(std::chrono::steady_clock::time_point now, size_t count=1)`
- Source: `include/api/rate_limiter.h`:132
- Brief: n/a
- Parameters:
  - `now` (std::chrono::steady_clock::time_point): n/a
  - `count` (size_t): n/a

#### `void refill(std::chrono::steady_clock::time_point now)`
- Source: `include/api/rate_limiter.h`:120
- Brief: Refill.
- Parameters:
  - `now` (std::chrono::steady_clock::time_point): Input parameter.
- Details: now Input parameter. Calls: count(), std::min().

### themis::graphql::RateLimiter::Config

#### `Config defaults()`
- Source: `include/api/rate_limiter.h`:71
- Brief: Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

#### `Config permissive()`
- Source: `include/api/rate_limiter.h`:93
- Brief: Permissive.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::seconds().

#### `Config strict()`
- Source: `include/api/rate_limiter.h`:80
- Brief: Strict.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::seconds().

### themis::graphql::RateLimiter::Stats

#### `Stats()=default`
- Source: `include/api/rate_limiter.h`:233
- Brief: n/a
- Parameters: none

#### `Stats(const Stats &other)`
- Source: `include/api/rate_limiter.h`:235
- Brief: n/a
- Parameters:
  - `other` (const Stats &): n/a

#### `Stats & operator=(const Stats &other)`
- Source: `include/api/rate_limiter.h`:240
- Brief: n/a
- Parameters:
  - `other` (const Stats &): n/a

#### `double rejectRate() const`
- Source: `include/api/rate_limiter.h`:252
- Brief: n/a
- Parameters: none

#### `uint64_t total() const`
- Source: `include/api/rate_limiter.h`:248
- Brief: n/a
- Parameters: none

### themis::graphql::ResponseCache

#### `ResponseCache()`
- Source: `include/api/graphql_cache.h`:374
- Brief: n/a
- Parameters: none

#### `void clear()`
- Source: `include/api/graphql_cache.h`:369
- Brief: Clear.
- Parameters: none
- Details: Implements clear without additional internal calls.

#### `std::shared_ptr< CachedResponse > get(const std::string &query)`
- Source: `include/api/graphql_cache.h`:336
- Brief: Get.
- Parameters:
  - `query` (const std::string &): Input parameter.
- Return: Return value.
- Details: query Input parameter. Return value. Implements get without additional internal calls.

#### `Cache< CachedResponse >::CacheStats getStats() const`
- Source: `include/api/graphql_cache.h`:361
- Brief: n/a
- Parameters: none

#### `ResponseCache & instance()`
- Source: `include/api/graphql_cache.h`:325
- Brief: Instance.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements instance without additional internal calls.

#### `void invalidatePattern(const std::string &pattern)`
- Source: `include/api/graphql_cache.h`:355
- Brief: Invalidate Pattern.
- Parameters:
  - `pattern` (const std::string &): Input parameter.
- Details: pattern Input parameter. Calls: eraseIf(), count().

#### `void put(const std::string &query, const CachedResponse &response)`
- Source: `include/api/graphql_cache.h`:346
- Brief: Put.
- Parameters:
  - `query` (const std::string &): Input parameter.
  - `response` (const CachedResponse &): Input parameter.
- Details: query Input parameter. response Input parameter. Implements put without additional internal calls.

### themis::graphql::Schema

#### `Schema()`
- Source: `include/api/graphql.h`:659
- Brief: n/a
- Parameters: none

#### `void addType(TypeDefinition type)`
- Source: `include/api/graphql.h`:665
- Brief: Add Type.
- Parameters:
  - `type` (TypeDefinition): Input parameter.
- Details: type Input parameter. type Input parameter. Calls: std::move().

#### `const TypeDefinition * getType(std::string_view name) const`
- Source: `include/api/graphql.h`:671
- Brief: Get Type.
- Parameters:
  - `name` (std::string_view): Input parameter.
- Return: Pointer to the result.
- Details: name Input parameter. Pointer to the result.

#### `std::shared_ptr< Value > introspect(const Field &field) const`
- Source: `include/api/graphql.h`:716
- Brief: Introspection support (respects introspection policy).
- Parameters:
  - `field` (const Field &): Input parameter.
- Return: Return value.
- Details: field Input parameter. Return value.

#### `bool isIntrospectionEnabled() const`
- Source: `include/api/graphql.h`:703
- Brief: n/a
- Parameters: none

#### `const std::string & mutationType() const`
- Source: `include/api/graphql.h`:693
- Brief: n/a
- Parameters: none

#### `const std::string & queryType() const`
- Source: `include/api/graphql.h`:692
- Brief: n/a
- Parameters: none

#### `void setIntrospectionEnabled(bool enabled)`
- Source: `include/api/graphql.h`:702
- Brief: Set Introspection Enabled.
- Parameters:
  - `enabled` (bool): Input parameter.
- Details: enabled Input parameter. Implements setIntrospectionEnabled without additional internal calls.

#### `void setMutationType(std::string_view name)`
- Source: `include/api/graphql.h`:684
- Brief: Set Mutation Type.
- Parameters:
  - `name` (std::string_view): Input parameter.
- Details: name Input parameter. Implements setMutationType without additional internal calls.

#### `void setQueryType(std::string_view name)`
- Source: `include/api/graphql.h`:678
- Brief: Set Query Type.
- Parameters:
  - `name` (std::string_view): Input parameter.
- Details: name Input parameter. Implements setQueryType without additional internal calls.

#### `void setSubscriptionType(std::string_view name)`
- Source: `include/api/graphql.h`:690
- Brief: Set Subscription Type.
- Parameters:
  - `name` (std::string_view): Input parameter.
- Details: name Input parameter. Implements setSubscriptionType without additional internal calls.

#### `const std::string & subscriptionType() const`
- Source: `include/api/graphql.h`:694
- Brief: n/a
- Parameters: none

#### `std::string toSDL() const`
- Source: `include/api/graphql.h`:709
- Brief: Generate SDL (Schema Definition Language).
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::graphql::ThemisSchemaBuilder

#### `void addDocumentTypes(Schema &schema)`
- Source: `include/api/graphql.h`:744
- Brief: Add Document Types.
- Parameters:
  - `schema` (Schema &): Input/output parameter.
- Details: schema Input/output parameter. schema Input/output parameter. Calls: push_back(), addType().

#### `void addGeoScalarTypes(Schema &schema)`
- Source: `include/api/graphql.h`:739
- Brief: Add Geo Scalar Types.
- Parameters:
  - `schema` (Schema &): Input/output parameter.
- Details: schema Input/output parameter. schema Input/output parameter. Calls: addType(), push_back().

#### `void addGraphTypes(Schema &schema)`
- Source: `include/api/graphql.h`:749
- Brief: Add Graph Types.
- Parameters:
  - `schema` (Schema &): Input/output parameter.
- Details: schema Input/output parameter. schema Input/output parameter. Calls: push_back(), addType().

#### `void addMutationType(Schema &schema)`
- Source: `include/api/graphql.h`:769
- Brief: Add Mutation Type.
- Parameters:
  - `schema` (Schema &): Input/output parameter.
- Details: schema Input/output parameter. schema Input/output parameter. Calls: push_back(), addType().

#### `void addQueryType(Schema &schema)`
- Source: `include/api/graphql.h`:764
- Brief: Add Query Type.
- Parameters:
  - `schema` (Schema &): Input/output parameter.
- Details: schema Input/output parameter. schema Input/output parameter. Calls: push_back(), addType().

#### `void addSubscriptionType(Schema &schema)`
- Source: `include/api/graphql.h`:774
- Brief: Add Subscription Type.
- Parameters:
  - `schema` (Schema &): Input/output parameter.
- Details: schema Input/output parameter. schema Input/output parameter. Calls: addType(), push_back(), setSubscriptionType().

#### `void addTimeseriesTypes(Schema &schema)`
- Source: `include/api/graphql.h`:759
- Brief: Add Timeseries Types.
- Parameters:
  - `schema` (Schema &): Input/output parameter.
- Details: schema Input/output parameter. schema Input/output parameter. Calls: push_back(), addType().

#### `void addVectorTypes(Schema &schema)`
- Source: `include/api/graphql.h`:754
- Brief: Add Vector Types.
- Parameters:
  - `schema` (Schema &): Input/output parameter.
- Details: schema Input/output parameter. schema Input/output parameter. Calls: push_back(), addType().

#### `Schema build()`
- Source: `include/api/graphql.h`:732
- Brief: Build.
- Parameters: none
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Return value. Return value. std::runtime_error if an error occurs. Calls: addGeoScalarTypes(), addDocumentTypes(), addGraphTypes(), addVectorTypes(), addTimeseriesTypes(), addQueryType(), addMutationType(), addSubscriptionType().

### themis::graphql::Value

#### `bool asBool() const`
- Source: `include/api/graphql.h`:213
- Brief: n/a
- Parameters: none

#### `double asFloat() const`
- Source: `include/api/graphql.h`:215
- Brief: n/a
- Parameters: none

#### `int64_t asInt() const`
- Source: `include/api/graphql.h`:214
- Brief: n/a
- Parameters: none

#### `const ValueList & asList() const`
- Source: `include/api/graphql.h`:217
- Brief: n/a
- Parameters: none

#### `const ValueMap & asObject() const`
- Source: `include/api/graphql.h`:218
- Brief: n/a
- Parameters: none

#### `const std::string & asString() const`
- Source: `include/api/graphql.h`:216
- Brief: n/a
- Parameters: none

#### `const std::string & asVariableRef() const`
- Source: `include/api/graphql.h`:219
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< Value > boolean(bool v)`
- Source: `include/api/graphql.h`:103
- Brief: Boolean.
- Parameters:
  - `v` (bool): Input parameter.
- Return: Return value.
- Details: v Input parameter. Return value. Implements boolean without additional internal calls.

#### `std::shared_ptr< Value > enumValue(std::string v)`
- Source: `include/api/graphql.h`:155
- Brief: Enum Value.
- Parameters:
  - `v` (std::string): Input parameter.
- Return: Return value.
- Details: v Input parameter. Return value. Calls: std::move().

#### `std::shared_ptr< Value > floating(double v)`
- Source: `include/api/graphql.h`:129
- Brief: Floating.
- Parameters:
  - `v` (double): Input parameter.
- Return: Return value.
- Details: v Input parameter. Return value. Implements floating without additional internal calls.

#### `std::shared_ptr< Value > integer(int64_t v)`
- Source: `include/api/graphql.h`:116
- Brief: Integer.
- Parameters:
  - `v` (int64_t): Input parameter.
- Return: Return value.
- Details: v Input parameter. Return value. Implements integer without additional internal calls.

#### `bool isBool() const`
- Source: `include/api/graphql.h`:203
- Brief: n/a
- Parameters: none

#### `bool isEnum() const`
- Source: `include/api/graphql.h`:207
- Brief: n/a
- Parameters: none

#### `bool isFloat() const`
- Source: `include/api/graphql.h`:205
- Brief: n/a
- Parameters: none

#### `bool isInt() const`
- Source: `include/api/graphql.h`:204
- Brief: n/a
- Parameters: none

#### `bool isList() const`
- Source: `include/api/graphql.h`:208
- Brief: n/a
- Parameters: none

#### `bool isNull() const`
- Source: `include/api/graphql.h`:202
- Brief: n/a
- Parameters: none

#### `bool isObject() const`
- Source: `include/api/graphql.h`:209
- Brief: n/a
- Parameters: none

#### `bool isString() const`
- Source: `include/api/graphql.h`:206
- Brief: n/a
- Parameters: none

#### `bool isVariableRef() const`
- Source: `include/api/graphql.h`:210
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< Value > list(ValueList v)`
- Source: `include/api/graphql.h`:168
- Brief: List.
- Parameters:
  - `v` (ValueList): Input parameter.
- Return: Return value.
- Details: v Input parameter. Return value. Calls: std::move().

#### `std::shared_ptr< Value > null()`
- Source: `include/api/graphql.h`:93
- Brief: Null.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements null without additional internal calls.

#### `std::shared_ptr< Value > object(ValueMap v)`
- Source: `include/api/graphql.h`:181
- Brief: Object.
- Parameters:
  - `v` (ValueMap): Input parameter.
- Return: Return value.
- Details: v Input parameter. Return value. Calls: std::move().

#### `std::shared_ptr< Value > string(std::string v)`
- Source: `include/api/graphql.h`:142
- Brief: String.
- Parameters:
  - `v` (std::string): Input parameter.
- Return: Return value.
- Details: v Input parameter. Return value. Calls: std::move().

#### `std::shared_ptr< Value > variableRef(std::string name)`
- Source: `include/api/graphql.h`:194
- Brief: Variable Ref.
- Parameters:
  - `name` (std::string): Input parameter.
- Return: Return value.
- Details: name Input parameter. Return value. Calls: std::move().

