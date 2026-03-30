# CTest Inventory — ThemisDB

> Preset: `msvc-ninja-release` · Stand: 2026-03-30 · Build: `build-msvc-ninja-release`
> Generiert aus: `ctest --preset msvc-ninja-release --show-only`

---

## Übersicht

| Kategorie | Anzahl |
|---|---|
| **Gesamt registrierte CTest-Tests** | **617** |
| ✅ Binary vorhanden / lauffähig | 48 |
| ⬜ Binary fehlt (`EXCLUDE_FROM_ALL`, noch nicht gebaut) | 545 |
| 🚫 Bewusst deaktiviert (`_NOT_BUILT` im Ziel-Namen) | 24 |
| 🔬 Benchmarks (Laufzeit-Benchmarks, kein Pass/Fail) | 1 |

> **Warum 545 „fehlend"?**
> Fast alle Test-Executables sind in `tests/CMakeLists.txt` mit
> `EXCLUDE_FROM_ALL` definiert — sie werden durch einen normalen
> `cmake --build` nicht automatisch gebaut. Sie müssen explizit per
> `cmake --build --preset vscode-windows-release --target <name>`
> oder per Sammel-Build angefordert werden.
>
> **`_NOT_BUILT`-Targets** sind bewusst deaktivierte Platzhalter —
> ihr CMake-Target existiert (per `set_tests_properties ... LABELS`),
> aber das zugehörige `add_executable` wurde entfernt oder steht unter
> einer noch nicht erfüllten Feature-Guard.

---

## ✅ Lauffähige Tests (48)

Binary ist gebaut, Test kann direkt per CTest oder direkt ausgeführt werden.

| ID  | Test-Name |  Status |
|-----|-----------|---------|
|  25 | AllTests | ✅ |
|  26 | ThemisWireProtocolV1Tests | ✅ |
|  27 | BuildInfoTests | ✅ |
|  28 | EditionManagerTests | ✅ |
|  29 | DynamicFeatureFlagTests | ✅ |
|  30 | RuntimeLicenseGateTests | ✅ |
|  31 | ModuleHashVerifierFocusedTests | ✅ |
|  32 | ModuleSignatureVerifierFocusedTests | ✅ |
|  33 | ModuleDependencyResolverFocusedTests | ✅ |
|  34 | ModuleLoaderFocusedTests | ✅ |
|  35 | PluginWatchdogFocusedTests | ✅ |
|  36 | ThemisIntegrationTests | ✅ |
|  37 | RemoteRegistryClientUnifiedTests | ✅ |
|  41 | RaftConfigurationTests | ✅ |
|  42 | PropertyGraphTests | ✅ |
|  43 | GraphQueryOptimizerTests | ✅ |
|  44 | GraphAdvancedFeaturesTests | ✅ |
|  45 | DistributedGraphTests | ✅ |
|  46 | DistributedGraphSharedMutexFocusedTests | ✅ |
|  47 | ParallelGraphTraversalTests | ✅ |
|  48 | GPUGraphTraversalTests | ✅ |
|  49 | GraphAnalyticsTests | ✅ |
|  50 | GraphTypeFilteringTests | ✅ |
|  78 | IndexMaintenanceTests | ✅ |
|  79 | GraphQLParserTests | ✅ |
|  80 | GraphQLPerformanceTests | ✅ |
|  84 | ApiInterfacesTests | ✅ |
|  85 | GraphQLErrorMaskingTests | ✅ |
|  86 | GraphQLP1FeatureTests | ✅ |
|  87 | GraphQLMultiModelTests | ✅ |
| 110 | DistributedSagaTests | ✅ |
| 171 | SelfAwarenessProductionTests | ✅ |
| 196 | ProcessDiscoveryConformanceFocusedTests | ✅ |
| 197 | GraphQueryExplainFocusedTests | ✅ |
| 212 | PostgresImporterFocusedTests | ✅ |
| 224 | PluginHotReloadEnhancedFocusedTests | ✅ |
| 349 | ChimeraAdapterFactoryTests | ✅ |
| 356 | ChimeraWeaviateAdapterTests | ✅ |
| 360 | ChimeraBatchOperationsTests | ✅ |
| 501 | TrainingConvergenceFocusedTests | ✅ |
| 502 | ProvenanceAqlIntegrationTests | ✅ |
| 503 | AutoLabelerDbFetchFocusedTests | ✅ |
| 513 | BinaryDeltaPatchesFocusedTests | ✅ |
| 561 | EpochFencingFocusedTests | ✅ |
| 562 | ProcessModuleFocusedTests | ✅ |
| 571 | AdaptiveCompactionFocusedTests | ✅ |
| 614 | CrossModuleQueryShardingTests | ✅ |
| 617 | CrossModuleAccelerationIndexTests | ✅ |

---

## ⬜ Nicht gebaute Tests — EXCLUDE_FROM_ALL (545)

Binary nicht im Standard-Build enthalten. Build-Aufruf:
`cmake --build --preset vscode-windows-release --target <executable-name>`

| ID  | Test-Name |
|-----|-----------|
|  38 | SessionManagerTests |
|  39 | ConsensusModuleTests |
|  40 | RaftConsensusAdapterTests |
|  51 | GraphEdgeEncryptionTests |
|  52 | HTTPClientPoolTests |
|  53 | GrpcChannelPoolTests |
|  54 | GrpcApiServerTests |
|  55 | ThemisDBGrpcServiceTests |
|  56 | BatchOperationManagerTests |
|  57 | HnswParameterTunerTests |
|  58 | HnswIncrementalReindexTests |
|  59 | EnhancedQueryCacheTests |
|  60 | CacheAdminApiHandlerTests |
|  61 | CacheWarmupTests |
|  62 | CacheReplicationTests |
|  63 | CacheReplicationCoordinatorTests |
|  64 | DistributedCacheCoordinatorTests |
|  65 | MultiRegionActiveActiveTests |
|  66 | GeoReplicationConsistencyFocusedTests |
|  67 | ReplicationHAFocusedTests |
|  68 | MultiTierReplicationFocusedTests |
|  69 | ReplicationNewFeaturesFocusedTests |
|  70 | ReplicationHATests |
|  71 | ReplicationNewFeaturesTests |
|  72 | LogicalReplicationTests |
|  73 | ReplicationTopologyAPITests |
|  74 | ReplicationRaftV2Tests |
|  75 | ReplicationCRDTTypesTests |
|  76 | RPCGeoQueryTests |
|  77 | RPCBatchOperationsTests |
|  81 | GraphQLLimitsTests |
|  82 | OtlpExporterTests |
|  83 | TracingMiddlewareTests |
|  88 | GraphQLCacheSecurityTests |
|  89 | ServerlessFunctionApiHandlerTests |
|  90 | WasmHandlerRegistryTests |
|  91 | GrpcWebProxyHandlerTests |
|  92 | ServiceMeshApiHandlerTests |
|  93 | UdfApiHandlerTests |
|  94 | InvertedIndexTests |
|  95 | FulltextPhraseFuzzyTests |
|  96 | RocksDBWrapperComprehensiveTests |
|  97 | PITRManagerComprehensiveTests |
|  98 | VectorIndexComprehensiveTests |
|  99 | VectorCompressionLosslessTests |
| 100 | TransactionSavepointTests |
| 101 | TransactionManagerComprehensiveTests |
| 102 | TransactionOccTests |
| 103 | TransactionBulkTests |
| 104 | TransactionRetryTests |
| 105 | TenantTransactionNamespaceTests |
| 106 | TransactionTimeoutTests |
| 107 | TransactionIsolationTests |
| 108 | TransactionSSITests |
| 109 | SagaConcurrentExecutionTests |
| 111 | SagaOperationTests |
| 112 | SAGAOrchestratorFocusedTests |
| 113 | TransactionBatcherFocusedTests |
| 114 | TransactionAuditorFocusedTests |
| 115 | TransactionManagerFocusedTests |
| 116 | AdaptiveDeadlockPreventionFocusedTests |
| 117 | TransactionDistributed2PCFocusedTests |
| 118 | TransactionIsolationLevelsFocusedTests |
| 119 | SAGALoggerFocusedTests |
| 120 | SAGACompactorFocusedTests |
| 121 | HashChainAuditFocusedTests |
| 122 | PIIStreamScannerFocusedTests |
| 123 | SampledLoggerFocusedTests |
| 124 | TimestampUtilsFocusedTests |
| 125 | UtilsRateLimiterFocusedTests |
| 126 | UtilsInterfacesFocusedTests |
| 127 | RateLimiterV2FocusedTests |
| 128 | RateLimitingImprovementsFocusedTests |
| 129 | UtilsStandaloneFocusedTests |
| 130 | ShardingTransactionWALFocusedTests |
| 131 | MultiShardTransactionFocusedTests |
| 132 | DistributedTransactionsFocusedTests |
| 133 | PercolatorCoordinatorFocusedTests |
| 134 | PostgresTransactionFocusedTests |
| 135 | AQLMultiStatementTransactionFocusedTests |
| 136 | DbTransactionIsolationFocusedTests |
| 137 | UtilitiesComprehensiveTests |
| 138 | LoggerProductionTests |
| 139 | TracingProductionTests |
| 140 | OtelTracerAdapterTests |
| 141 | JaegerTracerAdapterTests |
| 142 | ZipkinTracerAdapterTests |
| 143 | OtelPropagationTests |
| 144 | StructuredLogCorrelationTests |
| 145 | DistributedTracingTests |
| 146 | OtelApiTracingTests |
| 147 | SamplingStrategyTests |
| 148 | AuditLoggerProductionTests |
| 149 | GovernancePolicyHotReloadTests |
| 150 | GovernanceOpaAdapterFocusedTests |
| 151 | GovernancePolicySimulationFocusedTests |
| 152 | GovernanceComplianceTimeWindowFocusedTests |
| 153 | GovernanceReviewSchedulerFocusedTests |
| 154 | ModelGovernanceFocusedTests |
| 155 | ComplianceSecurityGovernanceFocusedTests |
| 156 | HttpGovernanceFocusedTests |
| 157 | ExportApiHandlerFocusedTests |
| 158 | HttpChangefeedGovernanceFocusedTests |
| 159 | ComplianceReportingFocusedTests |
| 160 | CcpaRulesFocusedTests |
| 161 | CrossTenantPolicyInheritanceFocusedTests |
| 162 | DataLineageFocusedTests |
| 163 | DataMaskerFocusedTests |
| 164 | PciDssRulesFocusedTests |
| 165 | PolicyReviewFocusedTests |
| 166 | PolicyTemplateFocusedTests |
| 167 | PolicyVersioningFocusedTests |
| 168 | Soc2ControlsFocusedTests |
| 169 | TaskSchedulerAuthContextFocusedTests |
| 170 | LEKManagerLifecycleTests |
| 172 | GraphIndexComprehensiveTests |
| 173 | LearnedIndexTests |
| 174 | TieredIndexMigrationTests |
| 175 | HnswRecallIntegrationTests |
| 176 | SpatialCorrectnessIntegrationTests |
| 177 | CloudStorageBackupComprehensiveTests |
| 178 | StreamingWindowFocusedTests |
| 179 | AnomalyDetectionFocusedTests |
| 180 | LLMProcessAnalyzerFocusedTests |
| 181 | AutoMlFocusedTests |
| 182 | DistributedAnalyticsFocusedTests |
| 183 | ProcessPatternMatcherFocusedTests |
| 184 | ForecastingFocusedTests |
| 185 | IncrementalViewFocusedTests |
| 186 | ColumnarExecutionFocusedTests |
| 187 | JitAggregationFocusedTests |
| 188 | MlServingFocusedTests |
| 189 | ModelServingFocusedTests |
| 190 | ArrowFlightFocusedTests |
| 191 | CepEngineFocusedTests |
| 192 | AnalyticsMemoryPoolFocusedTests |
| 193 | ArrowExportFocusedTests |
| 194 | OLAPLRUCacheFocusedTests |
| 195 | ProcessMiningPatternFocusedTests |
| 198 | ScheduledEdgeRefreshFocusedTests |
| 199 | ImporterPluginApiTests |
| 200 | ImporterInterfacesTests |
| 201 | FlatfileImporterFocusedTests |
| 202 | SchemaValidatorImporterFocusedTests |
| 203 | ImporterConflictResolverFocusedTests |
| 204 | ImporterAsyncApiFocusedTests |
| 205 | MySQLImporterFocusedTests |
| 206 | MySQLImporterRegistryTests |
| 207 | MongoImporterFocusedTests |
| 208 | SQLiteImporterFocusedTests |
| 209 | KafkaImporterFocusedTests |
| 210 | OracleImporterFocusedTests |
| 211 | S3ImporterFocusedTests |
| 213 | PostgresImporterV2FocusedTests |
| 214 | ImportWizardFocusedTests |
| 215 | MDMEntityMatchingFocusedTests |
| 216 | MDMEngineFocusedTests |
| 217 | PostgresImporterMDMFocusedTests |
| 218 | PluginCapabilityNegotiationTests |
| 219 | PluginManagerFocusedTests |
| 220 | PluginLifecycleFocusedTests |
| 221 | GenericPluginRegistryFocusedTests |
| 222 | PluginHealthMonitorFocusedTests |
| 223 | PluginHotPlugFocusedTests |
| 225 | PluginDependencyGraphFocusedTests |
| 226 | PluginDependencyResolverFocusedTests |
| 227 | PluginMetricsFocusedTests |
| 228 | PluginMetricsIntegrationFocusedTests |
| 229 | PluginSecurityAuditFocusedTests |
| 230 | PluginSecurityImplementationFocusedTests |
| 231 | PluginSecurityCRLOCSPTests |
| 232 | PluginSecurityPECertExtractionTests |
| 233 | PluginMarketplaceManifestFocusedTests |
| 234 | PluginManagerComprehensiveFocusedTests |
| 235 | LLMTimeoutCancellationTests |
| 236 | PerOperationCircuitBreakersFocusedTests |
| 237 | AccurateTokenCountEstimationTests |
| 238 | GlobalTransactionManagerTests |
| 239 | ParallelScanTests |
| 240 | ParallelExecutorTests |
| 241 | CdnCacheMiddlewareTests |
| 242 | ContentMetricsFocusedTests |
| 243 | ContentSecurityFocusedTests |
| 244 | ContentLanguageDetectorFocusedTests |
| 245 | ContentAudioProcessorFocusedTests |
| 246 | ContentProcessorChainFocusedTests |
| 247 | ContentDeduplicationFocusedTests |
| 248 | ContentPolicyOcrFocusedTests |
| 249 | OcrDpiPreprocessingFocusedTests |
| 250 | OcrDefaultDataDirFocusedTests |
| 251 | AsyncIngestionBackpressureFocusedTests |
| 252 | AsyncIngestionYamlConfigFocusedTests |
| 253 | LegacyOfficeExtractionFocusedTests |
| 254 | LibreOfficeSecurityFocusedTests |
| 255 | CDCAdminFocusedTests |
| 256 | TenantBufferManagerFocusedTests |
| 257 | CDCRetentionFocusedTests |
| 258 | CDCChangefeedSequenceCounterTests |
| 259 | ChangefeedCoreFocusedTests |
| 260 | DiffEngineFocusedTests |
| 261 | CdcWsHandlerFocusedTests |
| 262 | ConsumerGroupFocusedTests |
| 263 | CDCPauseControlFocusedTests |
| 264 | CDCBackpressureSignalFocusedTests |
| 265 | CDCFanInFocusedTests |
| 266 | CDCEventSchemaFocusedTests |
| 267 | CDCDeliveryGuaranteeConfigFocusedTests |
| 268 | ConfigPathResolverFocusedTests |
| 269 | ConfigFileWatcherFocusedTests |
| 270 | ConfigSchemaValidatorFocusedTests |
| 271 | ConfigMigrationScannerFocusedTests |
| 272 | ConfigCoverageFocusedTests |
| 273 | MetricsScrapeFocusedTests |
| 274 | ConfigEncryptedStoreFocusedTests |
| 275 | JWTValidatorTests |
| 276 | JWTEdDSAComprehensiveTests |
| 277 | JWTES256ComprehensiveTests |
| 278 | JWTECCurvesComprehensiveTests |
| 279 | JWTIntegrationTests |
| 280 | JWTKeyRotationComprehensiveTests |
| 281 | JWTManagementComprehensiveTests |
| 282 | JWTRotationUnitTests |
| 283 | JWTTokenRevocationIntegrationTests |
| 284 | JWTValidationHardeningTests |
| 285 | ApiKeyAuthenticatorTests |
| 286 | AuthAnomalyDetectionTests |
| 287 | AuthAuditLoggerTests |
| 288 | AuthErrorTests |
| 289 | AuthInputValidationTests |
| 290 | AuthMetricsTests |
| 291 | AuthRateLimiterTests |
| 292 | AuthRateLimiterDistributedTests |
| 293 | AuthMiddlewareFocusedTests |
| 294 | GSSAPIAuthenticatorTests |
| 295 | LDAPAuthenticatorTests |
| 296 | LDAPConnectionPoolTests |
| 297 | MFAAuthenticatorTests |
| 298 | MtlsAuthenticatorFocusedTests |
| 299 | OAuthDeviceFlowTests |
| 300 | OAuthPKCEFlowTests |
| 301 | SAMLAuthenticatorTests |
| 302 | SAMLAuthProviderTests |
| 303 | OAuth2ProviderTests |
| 304 | SessionManagerFocusedTests |
| 305 | TOTPReplayCacheTests |
| 306 | TOTPSecretEncryptionTests |
| 307 | WebAuthnAuthenticatorTests |
| 308 | ZeroTrustAuthVerifierTests |
| 309 | ZeroTrustPolicyEnforcerFocusedTests |
| 310 | ConcernsContextFocusedTests |
| 311 | FuzzCoreFocusedTests |
| 312 | LockFreeMetricsFocusedTests |
| 313 | ZeroCopyLoggingFocusedTests |
| 314 | ZeroCopyBlobTransferFocusedTests |
| 315 | SecurityEvidenceCollectorFocusedTests |
| 316 | FipsCryptoModeFocusedTests |
| 317 | AccessControlManagerFocusedTests |
| 318 | RowLevelSecurityFocusedTests |
| 319 | ArrowUserRegistrationPluginFocusedTests |
| 320 | CryptoAttackVectorTests |
| 321 | InjectionAttackVectorTests |
| 322 | AuthenticationAttackVectorTests |
| 323 | SecurityNegativeIntegrationFocusedTests |
| 324 | InputValidationSecurityFocusedTests |
| 325 | USBVolumeHardeningFocusedTests |
| 326 | DownsamplingFocusedTests |
| 327 | TSAdaptiveFlushFocusedTests |
| 328 | PrometheusRemoteWriteFocusedTests |
| 329 | TSStoreOutOfOrderFocusedTests |
| 330 | GeoRtreeFocusedTests |
| 331 | GeoClusteringFocusedTests |
| 332 | GeoRasterFocusedTests |
| 333 | TemporalSpatialQueryFocusedTests |
| 334 | GeoTileServerFocusedTests |
| 335 | GeoSpatialJoinFocusedTests |
| 336 | GeoDeviceDetectorFocusedTests |
| 337 | GeoEwkbFocusedTests |
| 338 | GeoPrecisionModeFocusedTests |
| 339 | GeoStBufferFocusedTests |
| 340 | GeoStUnionDifferenceFocusedTests |
| 341 | RtreeCpuIntegrationFocusedTests |
| 342 | SpatialIndexFocusedTests |
| 343 | AqlStFunctionsFocusedTests |
| 344 | AqlStQueryengineFocusedTests |
| 345 | Geo3dFunctionsFocusedTests |
| 346 | GeoWgs84SphericalFocusedTests |
| 347 | GpuBackendProductionFocusedTests |
| 348 | GpuKernelDispatcherFocusedTests |
| 350 | ChimeraThemisDBAdapterTests |
| 351 | ChimeraMongoDBAdapterTests |
| 352 | ChimeraPostgreSQLAdapterTests |
| 353 | ChimeraElasticsearchAdapterTests |
| 354 | ChimeraPineconeAdapterTests |
| 355 | ChimeraQdrantAdapterTests |
| 357 | ChimeraNeo4jAdapterTests |
| 358 | ChimeraCapabilityMatrixTests |
| 359 | ChimeraRetryPolicyTests |
| 361 | ChimeraAdapterConfigValidationTests |
| 362 | ChimeraAsyncAPITests |
| 363 | JsonlLlmExporterFocusedTests |
| 364 | HuggingFaceExporterFocusedTests |
| 365 | ParquetExporterFocusedTests |
| 366 | ArrowIpcExporterFocusedTests |
| 367 | StreamingExporterFocusedTests |
| 368 | IncrementalExporterFocusedTests |
| 369 | AqlPredicateFilterFocusedTests |
| 370 | FormatTemplateFocusedTests |
| 371 | ExportEncryptionFocusedTests |
| 372 | DataAugmentationFocusedTests |
| 373 | ExportFormatRegistryFocusedTests |
| 374 | HuggingFaceHubClientFocusedTests |
| 375 | JoinExporterFocusedTests |
| 376 | CacheInterfacesTests |
| 377 | LlmAiOrchestratorFocusedTests |
| 378 | LlmStreamingHandlerFocusedTests |
| 379 | LlmOpenAICompatAdapterFocusedTests |
| 380 | LlmLoraHotLoadingFocusedTests |
| 381 | LlmModelLoaderAsyncFocusedTests |
| 382 | LlmAuditLoggerFocusedTests |
| 383 | LlmJsonSchemaBindingFocusedTests |
| 384 | LlmGrammarIntegrationFocusedTests |
| 385 | LlmValidatorFocusedTests |
| 386 | LlmDeploymentPluginFocusedTests |
| 387 | LlmLoraAdaptersFocusedTests |
| 388 | LlmLoraAutoBindingFocusedTests |
| 389 | LlmLoraAdapterApplicationFocusedTests |
| 390 | LlmMcpOrchestratorBridgeFocusedTests |
| 391 | LlmExtendedContextFocusedTests |
| 392 | LlmInferencePerformanceFocusedTests |
| 393 | LlmInferenceQualityFocusedTests |
| 394 | LlmKernelFusionCpuFallbackFocusedTests |
| 395 | LlmKernelFusionCudaFocusedTests |
| 396 | LlmLlamaCppTokenizerFocusedTests |
| 397 | LlmLlamaWrapperStateFocusedTests |
| 398 | LlmGpuLoraIntegrationFocusedTests |
| 399 | LlmRealEmbeddingsFocusedTests |
| 400 | LlmModelLoaderErrorHandlingFocusedTests |
| 401 | LlmModelLoadingBestPracticesFocusedTests |
| 402 | LlmModelLoadingFromThemisDbFocusedTests |
| 403 | LlmBenchContinuousBatchScheduler |
| 404 | LlmActiveVRAMAllocatorFocusedTests |
| 405 | InferenceEngineEnhancedFocusedTests |
| 406 | IngestionBuilderFocusedTests |
| 407 | IngestionFeaturesFocusedTests |
| 408 | IngestionErrorsFocusedTests |
| 409 | IngestionCheckpointFocusedTests |
| 410 | IngestionResilienceFocusedTests |
| 411 | IngestionReconfigFocusedTests |
| 412 | IngestionPluginApiFocusedTests |
| 413 | IngestionSchemaValidationFocusedTests |
| 414 | IngestionSecurityFocusedTests |
| 415 | IngestionOauthFocusedTests |
| 416 | IngestionKafkaFocusedTests |
| 417 | IngestionObjectStorageFocusedTests |
| 418 | S3ConnectorFocusedTests |
| 419 | IngestionDatabaseFocusedTests |
| 420 | IngestionWebCrawlerFocusedTests |
| 421 | IngestionCoordinatorFocusedTests |
| 422 | IngestionCdcFocusedTests |
| 423 | IngestionIntegrationFocusedTests |
| 424 | IngestionPipelineFocusedTests |
| 425 | IngestionLineageFocusedTests |
| 426 | LegalExtractionFocusedTests |
| 427 | IngestionLlmAdapterFocusedTests |
| 428 | SchemaManagerFocusedTests |
| 429 | StatisticsCollectorFocusedTests |
| 430 | StatisticsAutoRefreshFocusedTests |
| 431 | ColumnLineageFocusedTests |
| 432 | CatalogExporterFocusedTests |
| 433 | SchemaAuditLogFocusedTests |
| 434 | SchemaConsistencyCheckerFocusedTests |
| 435 | IndexRecommenderFocusedTests |
| 436 | DistributedMetadataCatalogFocusedTests |
| 437 | SchemaVersionManagerFocusedTests |
| 438 | SchemaVersionDryRunFocusedTests |
| 439 | SchemaMigrationScriptFocusedTests |
| 440 | SchemaMigrationRegressionFocusedTests |
| 441 | SchemaConstraintsFocusedTests |
| 442 | SchemaConstraintsPersistenceFocusedTests |
| 443 | MetadataSecurityProviderFocusedTests |
| 444 | MetadataChangeListenerFocusedTests |
| 445 | MetadataExportPolicyFocusedTests |
| 446 | WireProtocolV1HandlersFocusedTests |
| 447 | WireProtocolBackpressureFocusedTests |
| 448 | WireProtocolIPv6FocusedTests |
| 449 | QoSManagerFocusedTests |
| 450 | BandwidthManagementQoSFocusedTests |
| 451 | NetworkTimeoutFocusedTests |
| 452 | NetworkCircuitBreakerFocusedTests |
| 453 | WireProtocolConnectionPoolFocusedTests |
| 454 | WireProtocolPerformanceFocusedTests |
| 455 | WireProtocolOptimizationsFocusedTests |
| 456 | UDPFastPathFocusedTests |
| 457 | UDPServerFocusedTests |
| 458 | GeoTopologyRouterFocusedTests |
| 459 | WireProtocolV2FocusedTests |
| 460 | GrpcTransportFocusedTests |
| 461 | PromptManagerFocusedTests |
| 462 | PromptVersionControlFocusedTests |
| 463 | FeedbackCollectorFocusedTests |
| 464 | PromptOptimizerFocusedTests |
| 465 | MetaPromptGeneratorFocusedTests |
| 466 | AnnIndexFocusedTests |
| 467 | DistributedVectorIndexFocusedTests |
| 468 | GPUMemoryOversubscriptionFocusedTests |
| 469 | IndexCompressionFocusedTests |
| 470 | MatryoshkaTruncationFocusedTests |
| 471 | PromptPerformanceTrackerFocusedTests |
| 472 | PromptEngineeringMetricsFocusedTests |
| 473 | SelfImprovementOrchestratorFocusedTests |
| 474 | PromptInjectionDetectorFocusedTests |
| 475 | ChainOfThoughtFocusedTests |
| 476 | CoTTracerFocusedTests |
| 477 | PromptRegressionRunnerFocusedTests |
| 478 | PromptABExperimentFocusedTests |
| 479 | PromptLibraryIOFocusedTests |
| 480 | RAGPromptBuilderFocusedTests |
| 481 | SystemPromptManagerFocusedTests |
| 482 | PromptEvaluatorFocusedTests |
| 483 | PromptEngineeringIntegrationFocusedTests |
| 484 | ContextWindowBudgetManagerFocusedTests |
| 485 | ReflectionTunerFocusedTests |
| 486 | ReflectionIntegrationFocusedTests |
| 487 | ProTeGiOptimizerFocusedTests |
| 488 | DspyModuleFocusedTests |
| 489 | MetricsCollectorFocusedTests |
| 490 | MetricsExemplarFocusedTests |
| 491 | MetricsAggregationFocusedTests |
| 492 | AlertRulesFocusedTests |
| 493 | AlertingEngineFocusedTests |
| 494 | MetricsStreamServerFocusedTests |
| 495 | ContinuousProfilerFocusedTests |
| 496 | ObservabilityTracerFocusedTests |
| 497 | LogAggregatorFocusedTests |
| 498 | MLAnomalyDetectorFocusedTests |
| 499 | RootCauseAnalyzerFocusedTests |
| 500 | ModalityParserFocusedTests |
| 504 | KgeVectorSearchFocusedTests |
| 505 | LoRAAdapterFocusedTests |
| 506 | AdvancedTrainingFeaturesFocusedTests |
| 507 | AdaLoRAFocusedTests |
| 508 | LoRAMergerFocusedTests |
| 509 | VoiceProductionFocusedTests |
| 510 | BlueGreenDeploymentFocusedTests |
| 511 | VoiceCoverageFocusedTests |
| 512 | CanaryRolloutFocusedTests |
| 514 | AutomaticSchemaMigrationFocusedTests |
| 515 | DistributedClusterUpdatesFocusedTests |
| 516 | ManifestDatabaseFileDeletionFocusedTests |
| 517 | CapGenPersistStateTests |
| 518 | VoiceAssistantFocusedTests |
| 519 | VoiceBrowserStreamingFocusedTests |
| 520 | VoiceTelephonyFocusedTests |
| 521 | NotificationWebhookFocusedTests |
| 522 | PreflightHealthCheckFocusedTests |
| 523 | SchemaMigrationTesterFocusedTests |
| 524 | ParallelFileDownloadsFocusedTests |
| 525 | DependencyResolutionEngineFocusedTests |
| 526 | ContentEmbeddingPipelineFocusedTests |
| 527 | MultiTenantUpdateSchedulingFocusedTests |
| 528 | RaftLoadBalancerFocusedTests |
| 529 | DistributedGatewayFocusedTests |
| 530 | APIGatewayEnhancementsFocusedTests |
| 531 | DatabaseMaintenanceOrchestratorFocusedTests |
| 532 | QueryEngineFocusedTests |
| 533 | QueryFederationShardRoutingTests |
| 534 | QueryPlanVisualizerFocusedTests |
| 535 | QueryPlanCachingFocusedTests |
| 536 | QueryJITCompilationFocusedTests |
| 537 | VectorizedExecutionFocusedTests |
| 538 | MaterializedViewFocusedTests |
| 539 | StorageAuditLoggerFocusedTests |
| 540 | WomTreeFocusedTests |
| 541 | StorageEngineDIFocusedTests |
| 542 | StorageEngineProdFocusedTests |
| 543 | NVMeFocusedTests |
| 544 | ErasureCodingFocusedTests |
| 545 | WireProtocolWebSocketFocusedTests |
| 546 | BloomFilterFocusedTests |
| 547 | CDNCacheMiddlewareFocusedTests |
| 548 | ImportWizardBuilderFocusedTests |
| 549 | CloudBackupFocusedTests |
| 550 | TwoPhaseCommitFocusedTests |
| 551 | ShardingCoreFocusedTests |
| 552 | ConsistentHashDistributionFocusedTests |
| 553 | RSRepairParallelisationFocusedTests |
| 554 | ShardingChaosFocusedTests |
| 555 | ShardingE2EFocusedTests |
| 556 | ShardingGossipFocusedTests |
| 557 | ShardingIntegrationFocusedTests |
| 558 | ShardingInterfacesFocusedTests |
| 559 | ShardingOperationalMetricsFocusedTests |
| 560 | ShardingUncoveredFocusedTests |
| 563 | ProcessGraphVisitTimestampFocusedTests |
| 564 | TemporalConflictResolverFocusedTests |
| 565 | TemporalRetentionManagerFocusedTests |
| 566 | BiTemporalFocusedTests |
| 567 | TemporalQueryEngineFocusedTests |
| 568 | ContinuousAggMaterializationFocusedTests |
| 569 | DistributedCacheIntegrationFocusedTests |
| 570 | OnlineSchemaMigrationFocusedTests |
| 572 | AdaptiveShardRebalancerFocusedTests |
| 573 | AdaptiveJoinStrategiesFocusedTests |
| 574 | DeviceManagerFocusedTests |
| 575 | BackendRegistryStartupFocusedTests |
| 576 | BackendRegistryThreadSafetyFocusedTests |
| 577 | VLLMResourceStatsFocusedTests |
| 578 | BlobRedundancyEventListenerFocusedTests |
| 579 | RemoteRegistryClientFocusedTests |
| 580 | AdaptiveQueryCompilationFocusedTests |
| 581 | HardwareAcceleratorFocusedTests |
| 582 | ThemisctlFocusedTests |
| 583 | LoRACertificateStoreFocusedTests |
| 584 | IntelligentPrefetchingFocusedTests |
| 585 | OrphanDetectorWiredFocusedTests |
| 586 | SecuritySignatureRocksDBIterationFocusedTests |
| 587 | RocksDBSizeCalculationFocusedTests |
| 588 | ShardRpcIntegrationFocusedTests |
| 589 | TSStoreGorillaBufFocusedTests |
| 590 | PredictivePrefetcherMarkovTests |
| 591 | VersionedApiRoutingFocusedTests |
| 592 | CudaHnswLargeKFocusedTests |
| 593 | ComputeInterfacesFocusedTests |
| 594 | QueryFederationRoutingFocusedTests |
| 595 | OZGServiceRegistryFocusedTests |
| 596 | XOEVImporterFocusedTests |
| 597 | XDOMEAConnectorFocusedTests |
| 598 | EIDAuthenticatorFocusedTests |
| 599 | BehoerdenGenehmigungsverfahrenE2EFocusedTests |
| 600 | BImSchVGenehmigungsverfahrenE2EFocusedTests |
| 601 | EGovDataDrivenFocusedTests |
| 602 | ReplugRetrieverFocusedTests |
| 603 | RLAIFTrainerFocusedTests |
| 604 | CypherParserFocusedTests |
| 605 | GremlinParserFocusedTests |
| 606 | MqttClientServiceFocusedTests |
| 607 | CrossModuleTimeseriesForecastingTests |
| 608 | CrossModuleTemporalBiTemporalTests |
| 609 | CrossModuleGermanEGovTests |
| 610 | CrossModuleIndexMatryoshkaTests |
| 611 | CrossModuleSecurityGovernanceTests |
| 612 | CrossModuleCacheAnomalyTests |
| 613 | CrossModuleTrainingGovernanceTests |
| 615 | CrossModuleGraphLineageTests |
| 616 | CrossModuleGeoSpatialTests |

---

## 🚫 Bewusst deaktivierte Tests — _NOT_BUILT (24)

Diese Test-Targets haben absichtlich keinen `add_executable`-Eintrag mehr.
Sie sind in CTest registriert, um den Fehlerstatus sichtbar zu machen.

| ID  | Ziel-Name | Grund |
|-----|-----------|-------|
|   1 | themis_secidx_tests_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|   2 | themis_tests_critical_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|   3 | test_phase1_flash_attention_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|   4 | test_phase1_kv_cache_reuse_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|   5 | test_ann_index_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|   6 | test_erasure_coding_backend_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|   7 | test_search_highlighter_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|   8 | test_distributed_hybrid_search_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|   9 | test_bitemporal_join_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  10 | test_temporal_snapshot_manager_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  11 | test_interval_tree_index_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  12 | test_temporal_compressor_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  13 | test_temporal_cdc_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  14 | test_ts_auto_buffer_adaptive_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  15 | test_chunk_level_encryption_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  16 | test_wasm_runtime_injector_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  17 | WasmSandboxInjectionFocusedTests_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  18 | test_simd_distance_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  19 | test_memory_pressure_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  20 | test_workload_predictor_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  21 | test_cycle_metrics_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  22 | test_numa_topology_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  23 | test_wire_perf_benchmark_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |
|  24 | test_adaptive_batch_tuner_NOT_BUILT | Bewusst deaktiviert (CMakeLists) |

---

## 🔬 Benchmarks (1 aktiv, 1 deaktiviert)

| ID  | Name | Status |
|-----|------|--------|
| 403 | LlmBenchContinuousBatchScheduler | ⬜ Binary fehlt (`EXCLUDE_FROM_ALL`) |
|  23 | test_wire_perf_benchmark_NOT_BUILT | 🚫 Deaktiviert |

---

## Preset-Referenz

| Preset | Enthält | Tests |
|--------|---------|-------|
| `graph-tests-release` | Nur Graph/GraphQL/Distributed/Analytics/Chimera/Process | 29 |
| `msvc-ninja-release` | Alle 617 Tests | 617 |
| `msvc-ninja-debug` | Alle Tests (Debug-Build) | 617 |

---

## Schnellstart: Alle fehlenden Binaries bauen

```powershell
# Alle 545 EXCLUDE_FROM_ALL Targets auf einmal bauen (ca. 45–90 min):
$env:SCCACHE_DISABLED=1
cmake --build --preset vscode-windows-release

# Danach alle Tests ausführen:
$env:PATH = "C:\VCC\themis\build-msvc-ninja-release\cmake\tests_gd_shared_out;" + `
             "C:\VCC\themis\build-msvc-ninja-release\cmake\tests_gd_out;" + `
             "C:\VCC\themis\build-msvc-ninja-release\cmake\tests;" + `
             "C:\VCC\themis\build-msvc-ninja-release\bin;" + $env:PATH
ctest --preset msvc-ninja-release --output-on-failure --parallel 4
```

> Hinweis: `_NOT_BUILT`-Tests bleiben immer `Not Run`. Sie repräsentieren
> Features, die noch nicht vollständig implementiert oder explizit gated sind.