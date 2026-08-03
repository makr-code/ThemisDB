# Metadata module test registration (extracted from root tests/CMakeLists.txt)

# Metadata - SchemaManager focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_schema_manager.cpp")
    message(STATUS "Adding Metadata SchemaManager focused tests")

    add_executable(test_schema_manager_focused
        test_schema_manager.cpp
    )

    target_include_directories(test_schema_manager_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_schema_manager_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_schema_manager_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME SchemaManagerFocusedTests
        TARGET test_schema_manager_focused
        TIER unit
        TIMEOUT 180
        LABELS schema std-gate
    )

    message(STATUS "  SchemaManager: automatic table discovery, property type detection, adaptive TTL, changefeeds")
else()
    message(STATUS "SchemaManager focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_schema_manager.cpp")
endif()

if(WIN32 AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_schema_manager.cpp")
    set(_themis_schema_smoke_script "${CMAKE_CURRENT_LIST_DIR}/../integration/server_schema_endpoint_smoke.ps1")
    set(_themis_core_contract_script "${CMAKE_CURRENT_LIST_DIR}/../integration/server_core_endpoint_contract.ps1")
    set(_themis_error_contract_script "${CMAKE_CURRENT_LIST_DIR}/../integration/server_error_path_contract.ps1")
    set(_themis_restart_resilience_script "${CMAKE_CURRENT_LIST_DIR}/../integration/server_restart_resilience.ps1")
    set(_themis_schema_soak_script "${CMAKE_CURRENT_LIST_DIR}/../integration/server_schema_endpoint_soak.ps1")
    set(_themis_std_gate_check_script "${CMAKE_CURRENT_LIST_DIR}/../integration/validate_std_gate.ps1")
    if(EXISTS "${_themis_schema_smoke_script}")
        add_test(NAME ServerSchemaEndpointSmoke
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                -File "${_themis_schema_smoke_script}"
                -ServerExe "${CMAKE_BINARY_DIR}/bin/themis_server.exe"
                -CtlExe "${CMAKE_BINARY_DIR}/bin/themisctl.exe"
        )
        set_tests_properties(ServerSchemaEndpointSmoke PROPERTIES
            LABELS "server;endpoint;schema;smoke;process;std-gate"
            TIMEOUT 240
        )
    endif()

    if(EXISTS "${_themis_std_gate_check_script}")
        add_test(NAME StdGateCompositionTests
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                -File "${_themis_std_gate_check_script}"
                -SourceDir "${CMAKE_SOURCE_DIR}"
                -BuildDir "${CMAKE_BINARY_DIR}"
                -Preset "windows-release-gate"
                -RequiredTestsCsv "SchemaManagerFocusedTests,ServerSchemaEndpointSmoke,ServerCoreEndpointContract,ServerErrorPathContract,ServerRestartResilience"
        )
        set_tests_properties(StdGateCompositionTests PROPERTIES
            LABELS "std-gate;gate-contract;meta"
            TIMEOUT 60
        )
    endif()

    if(EXISTS "${_themis_core_contract_script}")
        add_test(NAME ServerCoreEndpointContract
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                -File "${_themis_core_contract_script}"
                -ServerExe "${CMAKE_BINARY_DIR}/bin/themis_server.exe"
                -CtlExe "${CMAKE_BINARY_DIR}/bin/themisctl.exe"
        )
        set_tests_properties(ServerCoreEndpointContract PROPERTIES
            LABELS "server;endpoint;contract;schema;health;version;std-gate"
            TIMEOUT 240
        )
    endif()

    if(EXISTS "${_themis_error_contract_script}")
        add_test(NAME ServerErrorPathContract
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                -File "${_themis_error_contract_script}"
                -ServerExe "${CMAKE_BINARY_DIR}/bin/themis_server.exe"
                -CtlExe "${CMAKE_BINARY_DIR}/bin/themisctl.exe"
        )
        set_tests_properties(ServerErrorPathContract PROPERTIES
            LABELS "server;endpoint;contract;error;negative;std-gate"
            TIMEOUT 180
        )
    endif()

    if(EXISTS "${_themis_restart_resilience_script}")
        add_test(NAME ServerRestartResilience
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                -File "${_themis_restart_resilience_script}"
                -ServerExe "${CMAKE_BINARY_DIR}/bin/themis_server.exe"
                -CtlExe "${CMAKE_BINARY_DIR}/bin/themisctl.exe"
        )
        set_tests_properties(ServerRestartResilience PROPERTIES
            LABELS "server;endpoint;restart;resilience;process;std-gate"
            TIMEOUT 300
        )
    endif()

    if(EXISTS "${_themis_schema_soak_script}")
        add_test(NAME ServerSchemaEndpointSoak
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                -File "${_themis_schema_soak_script}"
                -ServerExe "${CMAKE_BINARY_DIR}/bin/themis_server.exe"
                -CtlExe "${CMAKE_BINARY_DIR}/bin/themisctl.exe"
                -Iterations 30
                -DelayMs 200
        )
        set_tests_properties(ServerSchemaEndpointSoak PROPERTIES
            LABELS "server;endpoint;schema;soak;process;std-gate-soak"
            TIMEOUT 300
        )
    endif()

    if(EXISTS "${_themis_std_gate_check_script}")
        add_test(NAME StdGateSoakCompositionTests
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                -File "${_themis_std_gate_check_script}"
                -SourceDir "${CMAKE_SOURCE_DIR}"
                -BuildDir "${CMAKE_BINARY_DIR}"
                -Preset "windows-release-gate-soak"
                -RequiredTestsCsv "SchemaManagerFocusedTests,ServerSchemaEndpointSmoke,ServerCoreEndpointContract,ServerErrorPathContract,ServerRestartResilience,ServerSchemaEndpointSoak"
        )
        set_tests_properties(StdGateSoakCompositionTests PROPERTIES
            LABELS "std-gate-soak;gate-contract;meta"
            TIMEOUT 60
        )
    endif()
endif()

# Metadata - StatisticsCollector focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_statistics_collector.cpp")
    message(STATUS "Adding Metadata StatisticsCollector focused tests")

    add_executable(test_statistics_collector_focused
        test_statistics_collector.cpp
    )

    target_include_directories(test_statistics_collector_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_statistics_collector_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_statistics_collector_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME StatisticsCollectorFocusedTests
        TARGET test_statistics_collector_focused
        TIER unit
        TIMEOUT 120
        LABELS statistics histograms cardinality selectivity
    )

    message(STATUS "  StatisticsCollector: equi-height histograms, cardinality estimation, index stats, auto-refresh, metrics hook")
endif()

# Metadata - StatisticsCollector auto-refresh focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_statistics_auto_refresh.cpp")
    message(STATUS "Adding Metadata StatisticsCollector auto-refresh focused tests")

    add_executable(test_statistics_auto_refresh_focused
        test_statistics_auto_refresh.cpp
    )

    target_include_directories(test_statistics_auto_refresh_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_statistics_auto_refresh_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_statistics_auto_refresh_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME StatisticsAutoRefreshFocusedTests
        TARGET test_statistics_auto_refresh_focused
        TIER unit
        TIMEOUT 120
        LABELS statistics auto-refresh
    )

    message(STATUS "  StatisticsCollector auto-refresh: background thread, metrics hook, interval control")
endif()

# Metadata - ColumnLineageTracker focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_column_lineage.cpp")
    message(STATUS "Adding Metadata ColumnLineage focused tests")

    add_executable(test_column_lineage_focused
        test_column_lineage.cpp
    )

    target_include_directories(test_column_lineage_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_column_lineage_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_column_lineage_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME ColumnLineageFocusedTests
        TARGET test_column_lineage_focused
        TIER unit
        TIMEOUT 60
        LABELS lineage provenance dag column-derivation
    )

    message(STATUS "  ColumnLineage: derivation DAG, transitive upstream/downstream traversal, provenance JSON export")
endif()

# Metadata - CatalogExporter focused tests (Atlas + DataHub)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_catalog_exporter.cpp")
    message(STATUS "Adding Metadata CatalogExporter focused tests")

    add_executable(test_catalog_exporter_focused
        test_catalog_exporter.cpp
    )

    target_include_directories(test_catalog_exporter_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_catalog_exporter_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_catalog_exporter_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME CatalogExporterFocusedTests
        TARGET test_catalog_exporter_focused
        TIER unit
        TIMEOUT 60
        LABELS catalog atlas datahub external-catalog
    )

    message(STATUS "  CatalogExporter: Apache Atlas entity bulk API, DataHub MCP emission, injected HTTP (no network)")
endif()

# Metadata - SchemaAuditLog focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_schema_audit_log.cpp")
    message(STATUS "Adding Metadata SchemaAuditLog focused tests")

    add_executable(test_schema_audit_log_focused
        test_schema_audit_log.cpp
    )

    target_include_directories(test_schema_audit_log_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_schema_audit_log_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_schema_audit_log_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME SchemaAuditLogFocusedTests
        TARGET test_schema_audit_log_focused
        TIER unit
        TIMEOUT 60
        LABELS audit schema-changes ddl rocksdb
    )

    message(STATUS "  SchemaAuditLog: durable per-table audit trail, JSON export, round-trip persistence")
endif()

# Metadata - SchemaConsistencyChecker focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_schema_consistency_checker.cpp")
    message(STATUS "Adding Metadata SchemaConsistencyChecker focused tests")

    add_executable(test_schema_consistency_checker_focused
        test_schema_consistency_checker.cpp
    )

    target_include_directories(test_schema_consistency_checker_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_schema_consistency_checker_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_schema_consistency_checker_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME SchemaConsistencyCheckerFocusedTests
        TARGET test_schema_consistency_checker_focused
        TIER unit
        TIMEOUT 60
        LABELS consistency background-scan schema-health
    )

    message(STATUS "  SchemaConsistencyChecker: background health scan, inconsistency detection and reporting")
endif()

# Metadata - IndexRecommender focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_index_recommender.cpp")
    message(STATUS "Adding Metadata IndexRecommender focused tests")

    add_executable(test_index_recommender_focused
        test_index_recommender.cpp
        ${THEMIS_ROOT_DIR}/src/metadata/index_recommender.cpp
    )

    target_include_directories(test_index_recommender_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_index_recommender_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_index_recommender_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME IndexRecommenderFocusedTests
        TARGET test_index_recommender_focused
        TIER unit
        TIMEOUT 60
        LABELS index recommendation query-patterns advisor persistence
    )

    message(STATUS "  IndexRecommender: query-pattern analysis, usage tracking, missing index detection, cost-benefit, RocksDB persistence, cost-model scoring, metric emission")
endif()

# Metadata - DistributedMetadataCatalog focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_distributed_catalog.cpp")
    message(STATUS "Adding Metadata DistributedMetadataCatalog focused tests")

    add_executable(test_distributed_catalog_focused
        test_distributed_catalog.cpp
        ${THEMIS_ROOT_DIR}/src/metadata/distributed_catalog.cpp
    )

    target_include_directories(test_distributed_catalog_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_distributed_catalog_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_distributed_catalog_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME DistributedMetadataCatalogFocusedTests
        TARGET test_distributed_catalog_focused
        TIER unit
        TIMEOUT 90
        LABELS distributed catalog shards replication
    )

    message(STATUS "  DistributedMetadataCatalog: cross-shard catalog synchronisation, shard metadata lookup")
endif()

# Metadata - SchemaVersionManager focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_schema_version_manager.cpp")
    message(STATUS "Adding Metadata SchemaVersionManager focused tests")

    add_executable(test_schema_version_manager_focused
        test_schema_version_manager.cpp
    )

    target_include_directories(test_schema_version_manager_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_schema_version_manager_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_schema_version_manager_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME SchemaVersionManagerFocusedTests
        TARGET test_schema_version_manager_focused
        TIER unit
        TIMEOUT 60
        LABELS versioning schema history rollback migration
    )

    message(STATUS "  SchemaVersionManager: schema version tracking, change history, rollback, diff, audit log integration")
endif()

# Metadata - SchemaVersionManager dry-run / validateMigration focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_schema_version_dryrun.cpp")
    message(STATUS "Adding Metadata SchemaVersionManager dry-run focused tests")

    add_executable(test_schema_version_dryrun_focused
        test_schema_version_dryrun.cpp
    )

    target_include_directories(test_schema_version_dryrun_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_schema_version_dryrun_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_schema_version_dryrun_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME SchemaVersionDryRunFocusedTests
        TARGET test_schema_version_dryrun_focused
        TIER unit
        TIMEOUT 60
        LABELS versioning schema dry-run migration-validation
    )

    message(STATUS "  SchemaVersionManager/validateMigration: dry-run validation, duplicate-column detection, no-persist guarantee")
endif()

# Metadata - generateMigrationScript focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_schema_migration_script.cpp")
    message(STATUS "Adding Metadata MigrationScript focused tests")

    add_executable(test_schema_migration_script_focused
        test_schema_migration_script.cpp
    )

    target_include_directories(test_schema_migration_script_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_schema_migration_script_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_schema_migration_script_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME SchemaMigrationScriptFocusedTests
        TARGET test_schema_migration_script_focused
        TIER unit
        TIMEOUT 60
        LABELS versioning migration ddl add-column drop-column alter-column
    )

    message(STATUS "  SchemaMigrationScript: ADD/DROP/ALTER COLUMN DDL generation, type mapping, NOT NULL, reverse migration")
endif()

# Metadata - schema migration regression focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_schema_migration_regression.cpp")
    message(STATUS "Adding Metadata MigrationRegression focused tests")

    add_executable(test_schema_migration_regression_focused
        test_schema_migration_regression.cpp
    )

    target_include_directories(test_schema_migration_regression_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_schema_migration_regression_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_schema_migration_regression_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME SchemaMigrationRegressionFocusedTests
        TARGET test_schema_migration_regression_focused
        TIER unit
        TIMEOUT 90
        LABELS versioning migration regression lifecycle rollback validate
    )

    message(STATUS "  SchemaMigrationRegression: forward/rollback across 5 versions, audit log, validateMigration lifecycle")
endif()

# Metadata - SchemaConstraints focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_schema_constraints.cpp")
    message(STATUS "Adding Metadata SchemaConstraints focused tests")

    add_executable(test_schema_constraints_focused
        test_schema_constraints.cpp
    )

    target_include_directories(test_schema_constraints_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_schema_constraints_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_schema_constraints_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME SchemaConstraintsFocusedTests
        TARGET test_schema_constraints_focused
        TIER unit
        TIMEOUT 60
        LABELS schema constraints not-null unique check default foreign-key
    )

    message(STATUS "  SchemaConstraints: NOT NULL, UNIQUE, CHECK, DEFAULT, foreign key enforcement")
endif()

# Metadata - SchemaConstraints persistence focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_schema_constraints_persistence.cpp")
    message(STATUS "Adding Metadata SchemaConstraints persistence focused tests")

    add_executable(test_schema_constraints_persistence_focused
        test_schema_constraints_persistence.cpp
    )

    target_include_directories(test_schema_constraints_persistence_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_schema_constraints_persistence_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_schema_constraints_persistence_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME SchemaConstraintsPersistenceFocusedTests
        TARGET test_schema_constraints_persistence_focused
        TIER unit
        TIMEOUT 60
        LABELS schema constraints persistence rocksdb
    )

    message(STATUS "  SchemaConstraintsPersistence: RocksDB round-trip for constraint definitions, reload after restart")
endif()

# Metadata - IMetadataSecurityProvider focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_metadata_security_provider.cpp")
    message(STATUS "Adding Metadata SecurityProvider focused tests")

    add_executable(test_metadata_security_provider_focused
        test_metadata_security_provider.cpp
    )

    target_include_directories(test_metadata_security_provider_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_metadata_security_provider_focused PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_metadata_security_provider_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME MetadataSecurityProviderFocusedTests
        TARGET test_metadata_security_provider_focused
        TIER unit
        TIMEOUT 60
        LABELS security rbac access-control information-disclosure
    )

    message(STATUS "  MetadataSecurityProvider: NoOp allows all, RBAC grant/revoke/revokeAll, wildcard resource, ADMIN implies all, assertPermission throws, thread-safety")
endif()

# Metadata - IMetadataChangeListener focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_metadata_change_listener.cpp")
    message(STATUS "Adding Metadata ChangeListener focused tests")

    add_executable(test_metadata_change_listener_focused
        test_metadata_change_listener.cpp
    )

    target_include_directories(test_metadata_change_listener_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_metadata_change_listener_focused PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_metadata_change_listener_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME MetadataChangeListenerFocusedTests
        TARGET test_metadata_change_listener_focused
        TIER unit
        TIMEOUT 60
        LABELS observer change-listener schema-events
    )

    message(STATUS "  MetadataChangeListener: FIFO ordering, lastEvent, clear, callback dispatch, setCallback, thread-safety, toJSON")
endif()

# Metadata - IMetadataExportPolicy focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_metadata_export_policy.cpp")
    message(STATUS "Adding Metadata ExportPolicy focused tests")

    add_executable(test_metadata_export_policy_focused
        test_metadata_export_policy.cpp
    )

    target_include_directories(test_metadata_export_policy_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_metadata_export_policy_focused PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_metadata_export_policy_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME MetadataExportPolicyFocusedTests
        TARGET test_metadata_export_policy_focused
        TIER unit
        TIMEOUT 60
        LABELS export policy atlas datahub external-catalog
    )

    message(STATUS "  MetadataExportPolicy: Always/Never/Filtered, exclusion list, exportDelay, removeExclusion, thread-safety")
endif()

# Metadata - IMetadataEncryptionProvider focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_metadata_encryption_provider.cpp")
    message(STATUS "Adding Metadata EncryptionProvider focused tests")

    add_executable(test_metadata_encryption_provider_focused
        test_metadata_encryption_provider.cpp
    )

    target_include_directories(test_metadata_encryption_provider_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_metadata_encryption_provider_focused PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_metadata_encryption_provider_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME MetadataEncryptionProviderFocusedTests
        TARGET test_metadata_encryption_provider_focused
        TIER unit
        TIMEOUT 60
        LABELS encryption field-level rbac security
    )

    message(STATUS "  MetadataEncryptionProvider: NoOp passthrough, FieldSet XOR cipher, wildcard, addField/removeField, thread-safety, round-trip")
endif()

# Metadata - SchemaDiff focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_schema_diff.cpp")
    message(STATUS "Adding Metadata SchemaDiff focused tests")

    add_executable(test_schema_diff_focused
        test_schema_diff.cpp
    )

    target_include_directories(test_schema_diff_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_schema_diff_focused PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_schema_diff_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME SchemaDiffFocusedTests
        TARGET test_schema_diff_focused
        TIER unit
        TIMEOUT 60
        LABELS schema diff migration column index
    )

    message(STATUS "  SchemaDiff: added/removed/modified columns and indexes, sorted output, isEmpty, toJSON, SchemaDiffEngine")
endif()

# Metadata - MetadataSnapshot focused tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_metadata_snapshot.cpp")
    message(STATUS "Adding Metadata Snapshot focused tests")

    add_executable(test_metadata_snapshot_focused
        test_metadata_snapshot.cpp
    )

    target_sources(test_metadata_snapshot_focused PRIVATE
        ${THEMIS_ROOT_DIR}/src/metadata/schema_manager.cpp
    )

    target_include_directories(test_metadata_snapshot_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_metadata_snapshot_focused PRIVATE
        themis_storage
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_metadata_snapshot_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME MetadataSnapshotFocusedTests
        TARGET test_metadata_snapshot_focused
        TIER unit
        TIMEOUT 60
        LABELS snapshot point-in-time catalog audit
    )

    message(STATUS "  MetadataSnapshot: save/load/remove/list, empty-id guard, sorted list, thread-safety, toJSON, polymorphic store")
endif()

# Metadata - contract hardening focused tests (MET-01..MET-08)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_metadata_contract_hardening_focused.cpp")
    message(STATUS "Adding Metadata contract hardening focused tests")

    add_executable(test_metadata_contract_hardening_focused
        test_metadata_contract_hardening_focused.cpp
    )

    target_include_directories(test_metadata_contract_hardening_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_metadata_contract_hardening_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_metadata_contract_hardening_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME MetadataContractHardeningFocusedTests
        TARGET test_metadata_contract_hardening_focused
        TIER unit
        TIMEOUT 60
        LABELS contract error-codes hardening api
    )

    message(STATUS "  MetadataContractHardening: error code uniqueness, range, dispatch (MET-01..MET-08)")
endif()

# Metadata - Phase 2/3 hardening focused tests (MCH-01..MCH-08)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_metadata_hardening_phase23_focused.cpp")
    message(STATUS "Adding Metadata Phase 2/3 hardening focused tests")

    add_executable(test_metadata_hardening_phase23_focused
        test_metadata_hardening_phase23_focused.cpp
    )

    target_include_directories(test_metadata_hardening_phase23_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_metadata_hardening_phase23_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_metadata_hardening_phase23_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE metadata
        NAME MetadataHardeningPhase23FocusedTests
        TARGET test_metadata_hardening_phase23_focused
        TIER unit
        TIMEOUT 60
        LABELS hardening consistency diagnostics phase23 schema export
    )

    message(STATUS "  MetadataHardeningPhase23: ConsistencyIssue JSON, ColumnRef round-trip, diagnostics (MCH-01..MCH-08)")
endif()

# AUTOGEN PREFIX BLOCK: metadata
file(GLOB METADATA_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_metadata_*.cpp"
)

foreach(_src IN LISTS METADATA_AUTOGEN_PREFIX_TEST_SOURCES)
    # Skip sources already wired by explicit focused targets in this module file.
    set(_src_already_registered FALSE)
    get_property(_metadata_dir_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_metadata_tgt IN LISTS _metadata_dir_targets)
        get_target_property(_metadata_tgt_sources ${_metadata_tgt} SOURCES)
        if(_metadata_tgt_sources)
            list(FIND _metadata_tgt_sources "${_src}" _metadata_src_idx)
            if(NOT _metadata_src_idx EQUAL -1)
                set(_src_already_registered TRUE)
                break()
            endif()
        endif()
    endforeach()
    if(_src_already_registered)
        continue()
    endif()

    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_metadata_${_stem}_autofocused")
    set(_ctest "${_stem}_metadata_AutoFocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/..  # tests/ root — provides utils/test_stability.h
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE metadata
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS metadata autogen
    )
endforeach()