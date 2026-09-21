# PROJECTS DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\projects\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\projects\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 22
- Compounds: 66
- Classes/Structs: 32
- Namespaces: 3
- File Compounds: 22

## Namespaces
- testing
- themis
- themis::projects

## Types
### Classes
- ProjectsPhase4Test
- ProjectsTest
- themis::projects::CollaborationManager
- themis::projects::DocumentManager
- themis::projects::IProjectAuditLog
- themis::projects::IProjectBundleManager
- themis::projects::InMemoryProjectAuditLog
- themis::projects::ProjectDiff
- themis::projects::ProjectLifecycle
- themis::projects::ProjectMerge
- themis::projects::ProjectMetrics
- themis::projects::ProjectTemplate
- themis::projects::ProjectVersioning

### Structs
- themis::projects::AuditQueryOptions
- themis::projects::BundleExportOptions
- themis::projects::BundleImportResult
- themis::projects::Change
- themis::projects::ChunkMeta
- themis::projects::ChunkingConfig
- themis::projects::DeltaEntry
- themis::projects::DeltaSet
- themis::projects::DocumentMeta
- themis::projects::MergeResult
- themis::projects::ProjectAuditEntry
- themis::projects::ProjectBundleManifest
- themis::projects::ProjectStateTransition
- themis::projects::SnapshotMeta
- themis::projects::Status
- themis::projects::TemplateInstantiationResult
- themis::projects::TemplateOptions
- themis::projects::UploadResult
- themis::projects::User

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 192

### ProjectsPhase4Test

#### `void SetUp() override`
- Source: `tests/projects/test_projects_phase4_hardening_focused.cpp`:27
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/projects/test_projects_phase4_hardening_focused.cpp`:37
- Brief: n/a
- Parameters: none

### ProjectsTest

#### `void SetUp() override`
- Source: `tests/projects/test_projects.cpp`:75
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/projects/test_projects.cpp`:85
- Brief: n/a
- Parameters: none

### bench_projects_release_gates.cpp

#### `BENCHMARK(BM_ProjError_BatchCast) -> Arg(1000) ->Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/projects/bench_projects_release_gates.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ProjError_BatchCast): n/a

#### `BENCHMARK(BM_ProjError_Cast) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/projects/bench_projects_release_gates.cpp`:23
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ProjError_Cast): n/a

#### `BENCHMARK(BM_ProjError_RangeCheck) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/projects/bench_projects_release_gates.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ProjError_RangeCheck): n/a

#### `BENCHMARK(BM_ProjError_SwitchDispatch) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/projects/bench_projects_release_gates.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ProjError_SwitchDispatch): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/projects/bench_projects_release_gates.cpp`:82
- Brief: n/a
- Parameters: none

#### `void BM_ProjError_BatchCast(benchmark::State &state)`
- Source: `benchmarks/projects/bench_projects_release_gates.cpp`:63
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ProjError_Cast(benchmark::State &state)`
- Source: `benchmarks/projects/bench_projects_release_gates.cpp`:10
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ProjError_RangeCheck(benchmark::State &state)`
- Source: `benchmarks/projects/bench_projects_release_gates.cpp`:47
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ProjError_SwitchDispatch(benchmark::State &state)`
- Source: `benchmarks/projects/bench_projects_release_gates.cpp`:25
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### test_projects.cpp

#### `TEST(ProjectMetricsTest, PM01_InitialStateIsEmpty)`
- Source: `tests/projects/test_projects.cpp`:505
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectMetricsTest): n/a
  - `<unnamed>` (PM01_InitialStateIsEmpty): n/a

#### `TEST(ProjectMetricsTest, PM02_RecordChangeIncrementsCounter)`
- Source: `tests/projects/test_projects.cpp`:514
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectMetricsTest): n/a
  - `<unnamed>` (PM02_RecordChangeIncrementsCounter): n/a

#### `TEST(ProjectMetricsTest, PM03_RecordDiffAccumulatesCallsAndDuration)`
- Source: `tests/projects/test_projects.cpp`:523
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectMetricsTest): n/a
  - `<unnamed>` (PM03_RecordDiffAccumulatesCallsAndDuration): n/a

#### `TEST(ProjectMetricsTest, PM04_GetMetricsTextContainsAllMetrics)`
- Source: `tests/projects/test_projects.cpp`:532
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectMetricsTest): n/a
  - `<unnamed>` (PM04_GetMetricsTextContainsAllMetrics): n/a

#### `TEST_F(ProjectsTest, CM01_ShareProjectPersistsPermission)`
- Source: `tests/projects/test_projects.cpp`:407
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (CM01_ShareProjectPersistsPermission): n/a

#### `TEST_F(ProjectsTest, CM02_GetUserPermissionReturnsStored)`
- Source: `tests/projects/test_projects.cpp`:414
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (CM02_GetUserPermissionReturnsStored): n/a

#### `TEST_F(ProjectsTest, CM03_RevokeAccessRemovesPermission)`
- Source: `tests/projects/test_projects.cpp`:423
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (CM03_RevokeAccessRemovesPermission): n/a

#### `TEST_F(ProjectsTest, CM04_LockObjectSucceedsSecondLockFails)`
- Source: `tests/projects/test_projects.cpp`:432
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (CM04_LockObjectSucceedsSecondLockFails): n/a

#### `TEST_F(ProjectsTest, CM05_UnlockByWrongLockerFails)`
- Source: `tests/projects/test_projects.cpp`:442
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (CM05_UnlockByWrongLockerFails): n/a

#### `TEST_F(ProjectsTest, CM06_UnlockByCorrectLockerSucceeds)`
- Source: `tests/projects/test_projects.cpp`:449
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (CM06_UnlockByCorrectLockerSucceeds): n/a

#### `TEST_F(ProjectsTest, CM07_NotifyChangeInvokesSubscribers)`
- Source: `tests/projects/test_projects.cpp`:458
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (CM07_NotifyChangeInvokesSubscribers): n/a

#### `TEST_F(ProjectsTest, CM08_GetChangesFiltersByProjectAndTimestamp)`
- Source: `tests/projects/test_projects.cpp`:478
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (CM08_GetChangesFiltersByProjectAndTimestamp): n/a

#### `TEST_F(ProjectsTest, PD01_DiffDocumentsEmptyForEqualDocuments)`
- Source: `tests/projects/test_projects.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PD01_DiffDocumentsEmptyForEqualDocuments): n/a

#### `TEST_F(ProjectsTest, PD02_DiffDocumentsDetectsAddedField)`
- Source: `tests/projects/test_projects.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PD02_DiffDocumentsDetectsAddedField): n/a

#### `TEST_F(ProjectsTest, PD03_DiffDocumentsDetectsRemovedField)`
- Source: `tests/projects/test_projects.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PD03_DiffDocumentsDetectsRemovedField): n/a

#### `TEST_F(ProjectsTest, PD04_DiffDocumentsDetectsModifiedField)`
- Source: `tests/projects/test_projects.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PD04_DiffDocumentsDetectsModifiedField): n/a

#### `TEST_F(ProjectsTest, PD05_DeltaSetRoundTripJson)`
- Source: `tests/projects/test_projects.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PD05_DeltaSetRoundTripJson): n/a

#### `TEST_F(ProjectsTest, PD06_MergeAppliesNonConflictingChanges)`
- Source: `tests/projects/test_projects.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PD06_MergeAppliesNonConflictingChanges): n/a

#### `TEST_F(ProjectsTest, PD07_MergeReportsConflictsForSameFieldChange)`
- Source: `tests/projects/test_projects.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PD07_MergeReportsConflictsForSameFieldChange): n/a

#### `TEST_F(ProjectsTest, PL01_InitProjectSetsCreatedState)`
- Source: `tests/projects/test_projects.cpp`:281
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PL01_InitProjectSetsCreatedState): n/a

#### `TEST_F(ProjectsTest, PL02_ActivateTransitionsCreatedToActive)`
- Source: `tests/projects/test_projects.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PL02_ActivateTransitionsCreatedToActive): n/a

#### `TEST_F(ProjectsTest, PL03_ArchiveTransitionsActiveToArchived)`
- Source: `tests/projects/test_projects.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PL03_ArchiveTransitionsActiveToArchived): n/a

#### `TEST_F(ProjectsTest, PL04_DeleteProjectTransitionsToDeleted)`
- Source: `tests/projects/test_projects.cpp`:308
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PL04_DeleteProjectTransitionsToDeleted): n/a

#### `TEST_F(ProjectsTest, PL05_DeletedIsTerminalState)`
- Source: `tests/projects/test_projects.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PL05_DeletedIsTerminalState): n/a

#### `TEST_F(ProjectsTest, PL06_InvalidTransitionReturnsError)`
- Source: `tests/projects/test_projects.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PL06_InvalidTransitionReturnsError): n/a

#### `TEST_F(ProjectsTest, PL07_GetAuditTrailReturnsAllTransitions)`
- Source: `tests/projects/test_projects.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PL07_GetAuditTrailReturnsAllTransitions): n/a

#### `TEST_F(ProjectsTest, PM05_CollaborationManagerIncrementsMetricsOnNotify)`
- Source: `tests/projects/test_projects.cpp`:546
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PM05_CollaborationManagerIncrementsMetricsOnNotify): n/a

#### `TEST_F(ProjectsTest, PM06_NoMetricsSinkIsNoop)`
- Source: `tests/projects/test_projects.cpp`:563
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PM06_NoMetricsSinkIsNoop): n/a

#### `TEST_F(ProjectsTest, PT01_ListBuiltinTemplatesReturns7)`
- Source: `tests/projects/test_projects.cpp`:354
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PT01_ListBuiltinTemplatesReturns7): n/a

#### `TEST_F(ProjectsTest, PT02_InstantiateEmptyTemplateCreatesProject)`
- Source: `tests/projects/test_projects.cpp`:363
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PT02_InstantiateEmptyTemplateCreatesProject): n/a

#### `TEST_F(ProjectsTest, PT03_InstantiateWebApplicationCreatesObjects)`
- Source: `tests/projects/test_projects.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PT03_InstantiateWebApplicationCreatesObjects): n/a

#### `TEST_F(ProjectsTest, PT04_ValidateTemplateDefinitionRejectsMissingObjects)`
- Source: `tests/projects/test_projects.cpp`:386
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PT04_ValidateTemplateDefinitionRejectsMissingObjects): n/a

#### `TEST_F(ProjectsTest, PT05_InstantiateFromInvalidDefinitionReturnsError)`
- Source: `tests/projects/test_projects.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PT05_InstantiateFromInvalidDefinitionReturnsError): n/a

#### `TEST_F(ProjectsTest, PV01_CreateSnapshotReturnsValidId)`
- Source: `tests/projects/test_projects.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PV01_CreateSnapshotReturnsValidId): n/a

#### `TEST_F(ProjectsTest, PV02_GetSnapshotReturnsMetadata)`
- Source: `tests/projects/test_projects.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PV02_GetSnapshotReturnsMetadata): n/a

#### `TEST_F(ProjectsTest, PV03_ListSnapshotsReturnsAll)`
- Source: `tests/projects/test_projects.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PV03_ListSnapshotsReturnsAll): n/a

#### `TEST_F(ProjectsTest, PV04_VerifySnapshotReturnsTrueForIntactSnapshot)`
- Source: `tests/projects/test_projects.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PV04_VerifySnapshotReturnsTrueForIntactSnapshot): n/a

#### `TEST_F(ProjectsTest, PV05_DeleteSnapshotRemovesIt)`
- Source: `tests/projects/test_projects.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PV05_DeleteSnapshotRemovesIt): n/a

#### `TEST_F(ProjectsTest, PV06_RestoreSnapshotFailsForMissingId)`
- Source: `tests/projects/test_projects.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsTest): n/a
  - `<unnamed>` (PV06_RestoreSnapshotFailsForMissingId): n/a

### test_projects_contract_hardening_focused.cpp

#### `TEST(ProjectsContractTest, PRJ01_ErrorCodesUnique)`
- Source: `tests/projects/test_projects_contract_hardening_focused.cpp`:16
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsContractTest): n/a
  - `<unnamed>` (PRJ01_ErrorCodesUnique): n/a

#### `TEST(ProjectsContractTest, PRJ02_ErrorCodesInRange)`
- Source: `tests/projects/test_projects_contract_hardening_focused.cpp`:27
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsContractTest): n/a
  - `<unnamed>` (PRJ02_ErrorCodesInRange): n/a

#### `TEST(ProjectsContractTest, PRJ03_MemberNotFoundDistinctFromProject)`
- Source: `tests/projects/test_projects_contract_hardening_focused.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsContractTest): n/a
  - `<unnamed>` (PRJ03_MemberNotFoundDistinctFromProject): n/a

#### `TEST(ProjectsContractTest, PRJ04_QuotaDistinctFromAuditOverflow)`
- Source: `tests/projects/test_projects_contract_hardening_focused.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsContractTest): n/a
  - `<unnamed>` (PRJ04_QuotaDistinctFromAuditOverflow): n/a

#### `TEST(ProjectsContractTest, PRJ05_MemberNotFoundLowestCode)`
- Source: `tests/projects/test_projects_contract_hardening_focused.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsContractTest): n/a
  - `<unnamed>` (PRJ05_MemberNotFoundLowestCode): n/a

#### `TEST(ProjectsContractTest, PRJ06_AuditOverflowHighestCode)`
- Source: `tests/projects/test_projects_contract_hardening_focused.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsContractTest): n/a
  - `<unnamed>` (PRJ06_AuditOverflowHighestCode): n/a

#### `TEST(ProjectsContractTest, PRJ07_ErrorSwitchDispatch)`
- Source: `tests/projects/test_projects_contract_hardening_focused.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsContractTest): n/a
  - `<unnamed>` (PRJ07_ErrorSwitchDispatch): n/a

#### `TEST(ProjectsContractTest, PRJ08_AllCodesGe7700)`
- Source: `tests/projects/test_projects_contract_hardening_focused.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsContractTest): n/a
  - `<unnamed>` (PRJ08_AllCodesGe7700): n/a

### test_projects_phase4_hardening_focused.cpp

#### `TEST_F(ProjectsPhase4Test, PRH01_LifecycleRejectsEmptyProjectId)`
- Source: `tests/projects/test_projects_phase4_hardening_focused.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsPhase4Test): n/a
  - `<unnamed>` (PRH01_LifecycleRejectsEmptyProjectId): n/a

#### `TEST_F(ProjectsPhase4Test, PRH02_LifecycleRejectsEmptyActor)`
- Source: `tests/projects/test_projects_phase4_hardening_focused.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsPhase4Test): n/a
  - `<unnamed>` (PRH02_LifecycleRejectsEmptyActor): n/a

#### `TEST_F(ProjectsPhase4Test, PRH03_SnapshotIntegrityValidation)`
- Source: `tests/projects/test_projects_phase4_hardening_focused.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsPhase4Test): n/a
  - `<unnamed>` (PRH03_SnapshotIntegrityValidation): n/a

#### `TEST_F(ProjectsPhase4Test, PRH04_SnapshotRestoreDetectsCorruption)`
- Source: `tests/projects/test_projects_phase4_hardening_focused.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsPhase4Test): n/a
  - `<unnamed>` (PRH04_SnapshotRestoreDetectsCorruption): n/a

#### `TEST_F(ProjectsPhase4Test, PRH05_LockContentionDiagnostics)`
- Source: `tests/projects/test_projects_phase4_hardening_focused.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsPhase4Test): n/a
  - `<unnamed>` (PRH05_LockContentionDiagnostics): n/a

#### `TEST_F(ProjectsPhase4Test, PRH06_CollaborationRejectsEmptyUserId)`
- Source: `tests/projects/test_projects_phase4_hardening_focused.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsPhase4Test): n/a
  - `<unnamed>` (PRH06_CollaborationRejectsEmptyUserId): n/a

#### `TEST_F(ProjectsPhase4Test, PRH07_UnlockValidateLockerMatch)`
- Source: `tests/projects/test_projects_phase4_hardening_focused.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsPhase4Test): n/a
  - `<unnamed>` (PRH07_UnlockValidateLockerMatch): n/a

#### `TEST_F(ProjectsPhase4Test, PRH08_ErrorMessageConsistency)`
- Source: `tests/projects/test_projects_phase4_hardening_focused.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProjectsPhase4Test): n/a
  - `<unnamed>` (PRH08_ErrorMessageConsistency): n/a

### themis::projects

#### `const char * builtinTemplateToString(BuiltinTemplate tmpl) noexcept`
- Source: `src/projects/project_template.cpp`:25
- Brief: Human-readable name for a BuiltinTemplate value.
- Parameters:
  - `tmpl` (BuiltinTemplate): n/a

#### `std::optional< ProjectState > projectStateFromString(const std::string &s) noexcept`
- Source: `src/projects/project_lifecycle.cpp`:34
- Brief: n/a
- Parameters:
  - `s` (const std::string &): n/a
- Details: Parse a state string produced by projectStateToString(). Returns std::nullopt for unknown strings.

#### `const char * projectStateToString(ProjectState state) noexcept`
- Source: `src/projects/project_lifecycle.cpp`:24
- Brief: Human-readable name for a ProjectState value.
- Parameters:
  - `state` (ProjectState): n/a

#### `json safeJsonParse(const std::string &data, const char *context) noexcept`
- Source: `src/projects/project_versioning.cpp`:30
- Brief: n/a
- Parameters:
  - `data` (const std::string &): n/a
  - `context` (const char *): n/a

### themis::projects::Change

#### `Change fromJson(const json &j)`
- Source: `include/projects/collaboration_manager.h`:67
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: value().

#### `json toJson() const`
- Source: `include/projects/collaboration_manager.h`:66
- Brief: n/a
- Parameters: none

### themis::projects::ChunkMeta

#### `ChunkMeta fromJson(const json &j)`
- Source: `include/projects/DocumentManager/document_manager.h`:70
- Brief: n/a
- Parameters:
  - `j` (const json &): n/a

#### `json toJson() const`
- Source: `include/projects/DocumentManager/document_manager.h`:69
- Brief: n/a
- Parameters: none

### themis::projects::ChunkingConfig

#### `std::string toStrategyString() const`
- Source: `include/projects/DocumentManager/document_manager.h`:81
- Brief: n/a
- Parameters: none

### themis::projects::CollaborationManager

#### `CollaborationManager(std::shared_ptr< RocksDBWrapper > storage)`
- Source: `include/projects/collaboration_manager.h`:106
- Brief: n/a
- Parameters:
  - `storage` (std::shared_ptr< RocksDBWrapper >): n/a

#### `void clearAuditLog()`
- Source: `include/projects/collaboration_manager.h`:223
- Brief: Remove the audit log sink (no-op if not set).
- Parameters: none
- Details: Clear Audit Log.

#### `std::vector< Change > getChanges(const std::string &project_id, int64_t since_timestamp) const`
- Source: `include/projects/collaboration_manager.h`:249
- Brief: Return all changes to a project recorded after since_timestamp.
- Parameters:
  - `project_id` (const std::string &): Project UUID.
  - `since_timestamp` (int64_t): Unix timestamp (seconds); 0 returns all entries.
- Details: Changes are ordered chronologically. The in-memory log is bounded to the most recent 10 000 entries per CollaborationManager instance. project_id Project UUID. since_timestamp Unix timestamp (seconds); 0 returns all entries.

#### `std::optional< Permission > getUserPermission(const std::string &project_id, const std::string &user_id) const`
- Source: `include/projects/collaboration_manager.h`:142
- Brief: Return the permission level of a user for a project.
- Parameters:
  - `project_id` (const std::string &): n/a
  - `user_id` (const std::string &): n/a
- Return: Permission level if the user has access, std::nullopt otherwise.
- Details: Permission level if the user has access, std::nullopt otherwise.

#### `bool isLocked(const std::string &project_id, const std::string &object_name) const`
- Source: `include/projects/collaboration_manager.h`:203
- Brief: Check whether an object is currently locked.
- Parameters:
  - `project_id` (const std::string &): Project UUID.
  - `object_name` (const std::string &): Object key to query.
- Details: project_id Project UUID. object_name Object key to query.

#### `Status lockObject(const std::string &project_id, const std::string &object_name, const std::string &locker_id)`
- Source: `include/projects/collaboration_manager.h`:175
- Brief: Acquire a lock on a named object within a project.
- Parameters:
  - `project_id` (const std::string &): Identifier of the project.
  - `object_name` (const std::string &): Name of the object.
  - `locker_id` (const std::string &): Identifier of the locker.
- Return: Status{true} on success, Status{false, reason} if already locked.
- Details: ── Optimistic locking ──────────────────────────────────────────────────────── project_id Project UUID. object_name Object key to lock. locker_id Unique identifier of the lock holder. Status{true} on success, Status{false, reason} if already locked. project_id Identifier of the project. object_name Name of the object. locker_id Identifier of the locker. Return value.

#### `void notifyChange(const Change &change)`
- Source: `include/projects/collaboration_manager.h`:263
- Brief: Record a change event and invoke all registered subscribers.
- Parameters:
  - `change` (const Change &): Input parameter.
- Details: Notify Change. Called by writers after committing a mutation. Persists the change to RocksDB and appends it to the in-memory log before invoking callbacks. change The change to record and broadcast. change Input parameter. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), std::to_string(), put(), toJson(), dump(), lock().

#### `std::optional< Permission > permissionFromString(const std::string &s) noexcept`
- Source: `include/projects/collaboration_manager.h`:290
- Brief: n/a
- Parameters:
  - `s` (const std::string &): n/a

#### `const char * permissionToString(Permission p) noexcept`
- Source: `include/projects/collaboration_manager.h`:289
- Brief: n/a
- Parameters:
  - `p` (Permission): n/a

#### `Status revokeAccess(const std::string &project_id, const std::string &user_id)`
- Source: `include/projects/collaboration_manager.h`:133
- Brief: Revoke a user's access to a project.
- Parameters:
  - `project_id` (const std::string &): Identifier of the project.
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: Revoke Access. project_id Project UUID. user_id User whose access should be revoked. project_id Identifier of the project. user_id Identifier of the user. Return value.

#### `void setAuditLog(std::shared_ptr< IProjectAuditLog > log)`
- Source: `include/projects/collaboration_manager.h`:218
- Brief: Inject an audit log sink.
- Parameters:
  - `log` (std::shared_ptr< IProjectAuditLog >): Input parameter.
- Details: ── Audit log DI ───────────────────────────────────────────────────────────── When set, notifyChange() records a DOCUMENT_UPDATED entry for every change event. Pass nullptr to disable. Thread-safe. log Input parameter.

#### `void setMetrics(std::shared_ptr< ProjectMetrics > metrics)`
- Source: `include/projects/collaboration_manager.h`:236
- Brief: Inject a metrics sink.
- Parameters:
  - `metrics` (std::shared_ptr< ProjectMetrics >): Input parameter.
- Details: Set Metrics. When set, notifyChange() increments ProjectMetrics::recordChange() for every committed event. Pass nullptr to disable. Thread-safe. metrics Input parameter.

#### `Status shareProject(const std::string &project_id, const std::vector< User > &users, Permission permission)`
- Source: `include/projects/collaboration_manager.h`:121
- Brief: Share a project with one or more users.
- Parameters:
  - `project_id` (const std::string &): Identifier of the project.
  - `users` (const std::vector< User > &): Input parameter.
  - `permission` (Permission): Input parameter.
- Return: Return value.
- Details: ── Sharing ─────────────────────────────────────────────────────────────────── Each user in users must have a non-empty User::id. Sharing with an empty-id user returns Status{false, "permission_denied"}. project_id Project UUID. users Users to grant access to. permission Access level granted to all listed users. project_id Identifier of the project. users Input parameter. permission Input parameter. Return value.

#### `void subscribe(ProjectEventCallback callback)`
- Source: `include/projects/collaboration_manager.h`:158
- Brief: Register a callback invoked for every change event.
- Parameters:
  - `callback` (ProjectEventCallback): Input parameter.
- Details: ── Event subscriptions ─────────────────────────────────────────────────────── Callbacks are invoked synchronously inside notifyChange(). Long-running callbacks will delay subsequent notifications. Callbacks must be non-blocking (hand off to a worker thread if needed). callback Callable(const Change&); must be copyable. callback Input parameter. Calls: lock(), push_back(), std::move().

#### `Status unlockObject(const std::string &project_id, const std::string &object_name, const std::string &locker_id)`
- Source: `include/projects/collaboration_manager.h`:191
- Brief: Release a previously acquired object lock.
- Parameters:
  - `project_id` (const std::string &): Identifier of the project.
  - `object_name` (const std::string &): Name of the object.
  - `locker_id` (const std::string &): Identifier of the locker.
- Return: Status{true} on success.
- Details: Unlock Object. Only the current lock holder (matching locker_id) may release the lock. project_id Project UUID. object_name Object key to unlock. locker_id Must match the current lock holder. Status{true} on success. project_id Identifier of the project. object_name Name of the object. locker_id Identifier of the locker. Return value.

#### `void unsubscribeAll()`
- Source: `include/projects/collaboration_manager.h`:163
- Brief: Remove all registered event callbacks.
- Parameters: none
- Details: Unsubscribe All. Calls: lock(), clear().

#### `~CollaborationManager()=default`
- Source: `include/projects/collaboration_manager.h`:107
- Brief: n/a
- Parameters: none

### themis::projects::DeltaEntry

#### `DeltaEntry fromJson(const json &j)`
- Source: `include/projects/project_diff.h`:52
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: value().

#### `json toJson() const`
- Source: `include/projects/project_diff.h`:51
- Brief: n/a
- Parameters: none

### themis::projects::DeltaSet

#### `bool empty() const noexcept`
- Source: `include/projects/project_diff.h`:69
- Brief: True when no differences were found.
- Parameters: none

#### `DeltaSet fromJson(const json &j)`
- Source: `include/projects/project_diff.h`:72
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: contains(), is_array(), push_back().

#### `json toJson() const`
- Source: `include/projects/project_diff.h`:71
- Brief: n/a
- Parameters: none

#### `size_t totalChanges() const noexcept`
- Source: `include/projects/project_diff.h`:66
- Brief: Number of individual field changes.
- Parameters: none

### themis::projects::DocumentManager

#### `DocumentManager(std::shared_ptr< RocksDBWrapper > storage, std::shared_ptr< VectorIndexManager > vector_index, std::shared_ptr< GraphIndexManager > graph_index)`
- Source: `include/projects/DocumentManager/document_manager.h`:119
- Brief: n/a
- Parameters:
  - `storage` (std::shared_ptr< RocksDBWrapper >): n/a
  - `vector_index` (std::shared_ptr< VectorIndexManager >): n/a
  - `graph_index` (std::shared_ptr< GraphIndexManager >): n/a

#### `std::vector< ChunkMeta > chunkText(const std::string &text, const std::string &doc_id, int embedding_dim)`
- Source: `include/projects/DocumentManager/document_manager.h`:222
- Brief: n/a
- Parameters:
  - `text` (const std::string &): n/a
  - `doc_id` (const std::string &): n/a
  - `embedding_dim` (int): n/a

#### `void createChunkGraph(const std::vector< std::string > &chunk_ids, const std::string &doc_id)`
- Source: `include/projects/DocumentManager/document_manager.h`:228
- Brief: n/a
- Parameters:
  - `chunk_ids` (const std::vector< std::string > &): n/a
  - `doc_id` (const std::string &): n/a

#### `Status deleteDocument(const std::string &doc_id)`
- Source: `include/projects/DocumentManager/document_manager.h`:195
- Brief: Delete document and all chunks (cascade).
- Parameters:
  - `doc_id` (const std::string &): Document UUID
- Return: Status with ok=true if deleted
- Details: doc_id Document UUID Status with ok=true if deleted

#### `std::string extractText(const std::string &blob, const std::string &mime_type)`
- Source: `include/projects/DocumentManager/document_manager.h`:221
- Brief: n/a
- Parameters:
  - `blob` (const std::string &): n/a
  - `mime_type` (const std::string &): n/a

#### `std::vector< float > generateEmbedding(const std::string &text)`
- Source: `include/projects/DocumentManager/document_manager.h`:227
- Brief: n/a
- Parameters:
  - `text` (const std::string &): n/a

#### `std::string generateUuid()`
- Source: `include/projects/DocumentManager/document_manager.h`:219
- Brief: n/a
- Parameters: none

#### `std::optional< ChunkMeta > getChunk(const std::string &chunk_id)`
- Source: `include/projects/DocumentManager/document_manager.h`:187
- Brief: Get chunk metadata.
- Parameters:
  - `chunk_id` (const std::string &): Chunk UUID (with or without "chunk:" prefix)
- Return: ChunkMeta if found, std::nullopt otherwise
- Details: chunk_id Chunk UUID (with or without "chunk:" prefix) ChunkMeta if found, std::nullopt otherwise

#### `ChunkingConfig getChunkingConfig() const`
- Source: `include/projects/DocumentManager/document_manager.h`:207
- Brief: Get current chunking configuration.
- Parameters: none

#### `std::optional< DocumentMeta > getDocument(const std::string &doc_id)`
- Source: `include/projects/DocumentManager/document_manager.h`:163
- Brief: Get document metadata.
- Parameters:
  - `doc_id` (const std::string &): Document UUID (with or without "doc:" prefix)
- Return: DocumentMeta if found, std::nullopt otherwise
- Details: doc_id Document UUID (with or without "doc:" prefix) DocumentMeta if found, std::nullopt otherwise

#### `std::optional< std::string > getDocumentBlob(const std::string &doc_id)`
- Source: `include/projects/DocumentManager/document_manager.h`:171
- Brief: Get document blob (binary content).
- Parameters:
  - `doc_id` (const std::string &): Document UUID
- Return: Blob as string if found, std::nullopt otherwise
- Details: doc_id Document UUID Blob as string if found, std::nullopt otherwise

#### `std::vector< ChunkMeta > getDocumentChunks(const std::string &doc_id)`
- Source: `include/projects/DocumentManager/document_manager.h`:179
- Brief: Get all chunks for a document (ordered by seq_num).
- Parameters:
  - `doc_id` (const std::string &): Document UUID
- Return: Vector of ChunkMeta
- Details: doc_id Document UUID Vector of ChunkMeta

#### `std::string normalizeId(const std::string &id, const std::string &prefix)`
- Source: `include/projects/DocumentManager/document_manager.h`:220
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `prefix` (const std::string &): n/a

#### `void setChunkingConfig(const ChunkingConfig &config)`
- Source: `include/projects/DocumentManager/document_manager.h`:200
- Brief: Set chunking configuration.
- Parameters:
  - `config` (const ChunkingConfig &): n/a

#### `UploadResult uploadDocument(const std::string &blob, const std::string &mime_type, const std::string &filename, const std::optional< std::string > &text=std::nullopt, const json &user_metadata=json::object(), bool store_blob=true)`
- Source: `include/projects/DocumentManager/document_manager.h`:148
- Brief: Upload and process a document.
- Parameters:
  - `blob` (const std::string &): Binary content (can be empty if text is provided directly)
  - `mime_type` (const std::string &): MIME type (e.g., "text/plain", "application/pdf")
  - `filename` (const std::string &): Original filename
  - `text` (const std::optional< std::string > &): Optional: Pre-extracted text (skips step 3)
  - `user_metadata` (const json &): Optional: Additional metadata (JSON object)
  - `store_blob` (bool): If true, stores blob in RocksDB; if false, only processes text
- Return: UploadResult with doc_id and status
- Details: Steps: Generate document UUID Store binary blob in RocksDB (if store_blob=true) Extract text from blob (based on mime_type) Chunk text with overlap Generate embeddings for each chunk (external API or mock) Insert chunks into VectorIndex Create graph edges (parent, next/prev) Store document metadata blob Binary content (can be empty if text is provided directly) mime_type MIME type (e.g., "text/plain", "application/pdf") filename Original filename text Optional: Pre-extracted text (skips step 3) user_metadata Optional: Additional metadata (JSON object) store_blob If true, stores blob in RocksDB; if false, only processes text UploadResult with doc_id and status

#### `~DocumentManager()=default`
- Source: `include/projects/DocumentManager/document_manager.h`:125
- Brief: n/a
- Parameters: none

### themis::projects::DocumentMeta

#### `DocumentMeta fromJson(const json &j)`
- Source: `include/projects/DocumentManager/document_manager.h`:49
- Brief: n/a
- Parameters:
  - `j` (const json &): n/a

#### `json toJson() const`
- Source: `include/projects/DocumentManager/document_manager.h`:48
- Brief: n/a
- Parameters: none

### themis::projects::IProjectAuditLog

#### `size_t count(const AuditQueryOptions &opts) const =0`
- Source: `include/projects/project_audit_log.h`:63
- Brief: n/a
- Parameters:
  - `opts` (const AuditQueryOptions &): n/a

#### `bool purge(const std::string &project_id, std::chrono::system_clock::time_point before)=0`
- Source: `include/projects/project_audit_log.h`:64
- Brief: n/a
- Parameters:
  - `project_id` (const std::string &): n/a
  - `before` (std::chrono::system_clock::time_point): n/a

#### `std::vector< ProjectAuditEntry > query(const AuditQueryOptions &opts) const =0`
- Source: `include/projects/project_audit_log.h`:62
- Brief: n/a
- Parameters:
  - `opts` (const AuditQueryOptions &): n/a

#### `void record(const ProjectAuditEntry &entry)=0`
- Source: `include/projects/project_audit_log.h`:61
- Brief: n/a
- Parameters:
  - `entry` (const ProjectAuditEntry &): n/a

#### `~IProjectAuditLog()=default`
- Source: `include/projects/project_audit_log.h`:60
- Brief: n/a
- Parameters: none

### themis::projects::IProjectBundleManager

#### `bool exportToZip(const std::string &project_id, const std::string &output_path, const BundleExportOptions &options={})=0`
- Source: `include/projects/project_bundle.h`:50
- Brief: n/a
- Parameters:
  - `project_id` (const std::string &): n/a
  - `output_path` (const std::string &): n/a
  - `options` (const BundleExportOptions &): n/a

#### `BundleImportResult importFromZip(const std::string &bundle_path, const std::string &target_project_id="")=0`
- Source: `include/projects/project_bundle.h`:53
- Brief: n/a
- Parameters:
  - `bundle_path` (const std::string &): n/a
  - `target_project_id` (const std::string &): n/a

#### `ProjectBundleManifest readManifest(const std::string &bundle_path)=0`
- Source: `include/projects/project_bundle.h`:55
- Brief: n/a
- Parameters:
  - `bundle_path` (const std::string &): n/a

#### `bool validateBundle(const std::string &bundle_path, std::vector< std::string > &errors)=0`
- Source: `include/projects/project_bundle.h`:56
- Brief: n/a
- Parameters:
  - `bundle_path` (const std::string &): n/a
  - `errors` (std::vector< std::string > &): n/a

#### `~IProjectBundleManager()=default`
- Source: `include/projects/project_bundle.h`:49
- Brief: n/a
- Parameters: none

### themis::projects::InMemoryProjectAuditLog

#### `InMemoryProjectAuditLog(const InMemoryProjectAuditLog &)=delete`
- Source: `include/projects/in_memory_project_audit_log.h`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (const InMemoryProjectAuditLog &): n/a

#### `InMemoryProjectAuditLog(size_t max_entries=kDefaultMaxEntries)`
- Source: `include/projects/in_memory_project_audit_log.h`:44
- Brief: n/a
- Parameters:
  - `max_entries` (size_t): n/a

#### `std::vector< ProjectAuditEntry > applyFilters(const AuditQueryOptions &opts) const`
- Source: `include/projects/in_memory_project_audit_log.h`:103
- Brief: Apply all filters in opts; returns matching entries unsorted.
- Parameters:
  - `opts` (const AuditQueryOptions &): n/a

#### `void clear()`
- Source: `include/projects/in_memory_project_audit_log.h`:95
- Brief: Remove all entries. Intended for test teardown.
- Parameters: none
- Details: Clear.

#### `size_t count(const AuditQueryOptions &opts) const override`
- Source: `include/projects/in_memory_project_audit_log.h`:76
- Brief: Count entries matching the given options (ignores limit/offset).
- Parameters:
  - `opts` (const AuditQueryOptions &): n/a

#### `InMemoryProjectAuditLog & operator=(const InMemoryProjectAuditLog &)=delete`
- Source: `include/projects/in_memory_project_audit_log.h`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (const InMemoryProjectAuditLog &): n/a

#### `bool purge(const std::string &project_id, std::chrono::system_clock::time_point before) override`
- Source: `include/projects/in_memory_project_audit_log.h`:82
- Brief: Remove all entries for project_id that are older than before. Returns the number of entries removed.
- Parameters:
  - `project_id` (const std::string &): Identifier of the project.
  - `before` (std::chrono::system_clock::time_point): Input parameter.
- Return: True when the operation succeeds.
- Details: ── purge ───────────────────────────────────────────────────────────────────── project_id Identifier of the project. before Input parameter. True when the operation succeeds.

#### `std::vector< ProjectAuditEntry > query(const AuditQueryOptions &opts) const override`
- Source: `include/projects/in_memory_project_audit_log.h`:70
- Brief: Query log entries matching the given options.
- Parameters:
  - `opts` (const AuditQueryOptions &): n/a
- Details: Applies filters in this order: project_id match (required) action_filter (optional) actor_id_filter (optional, empty = all actors) time window [start_time, end_time) Result is sorted by timestamp descending (opts.sort_direction == "desc") or ascending (any other value). limit and offset are applied last.

#### `void record(const ProjectAuditEntry &entry) override`
- Source: `include/projects/in_memory_project_audit_log.h`:57
- Brief: Append an audit entry to the log. Thread-safe.
- Parameters:
  - `entry` (const ProjectAuditEntry &): Input parameter.
- Details: ── record ──────────────────────────────────────────────────────────────────── entry Input parameter.

#### `size_t size() const`
- Source: `include/projects/in_memory_project_audit_log.h`:90
- Brief: Total number of entries currently stored.
- Parameters: none

#### `~InMemoryProjectAuditLog() override=default`
- Source: `include/projects/in_memory_project_audit_log.h`:47
- Brief: n/a
- Parameters: none

### themis::projects::ProjectDiff

#### `ProjectDiff(std::shared_ptr< RocksDBWrapper > storage)`
- Source: `include/projects/project_diff.h`:101
- Brief: n/a
- Parameters:
  - `storage` (std::shared_ptr< RocksDBWrapper >): n/a

#### `DeltaSet diff(const SnapshotId &from_snap, const SnapshotId &to_snap) const`
- Source: `include/projects/project_diff.h`:110
- Brief: Compute the delta between two existing snapshots.
- Parameters:
  - `from_snap` (const SnapshotId &): Baseline snapshot identifier.
  - `to_snap` (const SnapshotId &): Target snapshot identifier.
- Return: Structured field-level DeltaSet.
- Details: from_snap Baseline snapshot identifier. to_snap Target snapshot identifier. Structured field-level DeltaSet.

#### `DeltaSet diffDocuments(const json &from, const json &to) const`
- Source: `include/projects/project_diff.h`:122
- Brief: Compute the delta between two arbitrary JSON documents.
- Parameters:
  - `from` (const json &): n/a
  - `to` (const json &): n/a
- Details: Useful for one-off comparisons without creating persistent snapshots. Both from and to are treated as JSON objects; nested fields are compared recursively.

#### `void diffRecursive(const std::string &path, const json &from, const json &to, std::vector< DeltaEntry > &out) const`
- Source: `include/projects/project_diff.h`:139
- Brief: Recursively compare two JSON values, accumulating DeltaEntry records.
- Parameters:
  - `path` (const std::string &): n/a
  - `from` (const json &): n/a
  - `to` (const json &): n/a
  - `out` (std::vector< DeltaEntry > &): n/a

#### `void setMetrics(std::shared_ptr< ProjectMetrics > metrics)`
- Source: `include/projects/project_diff.h`:131
- Brief: Inject a metrics sink.
- Parameters:
  - `metrics` (std::shared_ptr< ProjectMetrics >): Input parameter.
- Details: Set Metrics. When set, every diff() call records its wall-clock latency via ProjectMetrics::recordDiff(). Pass nullptr to disable. Thread-safe. metrics Input parameter. Calls: lock(), std::move().

### themis::projects::ProjectLifecycle

#### `ProjectLifecycle(std::shared_ptr< RocksDBWrapper > storage)`
- Source: `include/projects/project_lifecycle.h`:96
- Brief: n/a
- Parameters:
  - `storage` (std::shared_ptr< RocksDBWrapper >): n/a

#### `Status activate(const std::string &project_id, const std::string &actor={})`
- Source: `include/projects/project_lifecycle.h`:119
- Brief: Transition a project from CREATED or ARCHIVED to ACTIVE.
- Parameters:
  - `project_id` (const std::string &): Identifier of the project.
  - `actor` (const std::string &): Input parameter.
- Return: Return value.
- Details: Activate. project_id Project UUID. actor Identity of the user performing the transition. project_id Identifier of the project. actor Input parameter. Return value.

#### `Status applyTransition(const std::string &project_id, ProjectState to_state, const std::string &actor, const std::string &reason)`
- Source: `include/projects/project_lifecycle.h`:175
- Brief: Low-level helper: write new state + audit entry in one batch.
- Parameters:
  - `project_id` (const std::string &): Identifier of the project.
  - `to_state` (ProjectState): Input parameter.
  - `actor` (const std::string &): Input parameter.
  - `reason` (const std::string &): Input parameter.
- Return: Return value.
- Details: ── Low-level helper ───────────────────────────────────────────────────────── project_id Identifier of the project. to_state Input parameter. actor Input parameter. reason Input parameter. Return value.

#### `Status archive(const std::string &project_id, const std::string &actor={}, const std::string &reason={})`
- Source: `include/projects/project_lifecycle.h`:133
- Brief: Transition a project from ACTIVE to ARCHIVED.
- Parameters:
  - `project_id` (const std::string &): Identifier of the project.
  - `actor` (const std::string &): Input parameter.
  - `reason` (const std::string &): Input parameter.
- Return: Return value.
- Details: Archive. Archived projects become read-only; all data is preserved. project_id Project UUID. actor Identity of the user archiving the project. reason Optional human-readable archival reason. project_id Identifier of the project. actor Input parameter. reason Input parameter. Return value.

#### `Status deleteProject(const std::string &project_id, const std::string &actor={})`
- Source: `include/projects/project_lifecycle.h`:147
- Brief: Transition a project to the terminal DELETED state.
- Parameters:
  - `project_id` (const std::string &): Identifier of the project.
  - `actor` (const std::string &): Input parameter.
- Return: Return value.
- Details: Delete Project. Once deleted a project cannot be recovered through this interface. project_id Project UUID. actor Identity of the user deleting the project. project_id Identifier of the project. actor Input parameter. Return value.

#### `std::vector< ProjectStateTransition > getAuditTrail(const std::string &project_id) const`
- Source: `include/projects/project_lifecycle.h`:163
- Brief: Return the complete append-only audit trail for a project.
- Parameters:
  - `project_id` (const std::string &): n/a
- Details: Entries are ordered chronologically (oldest first).

#### `std::optional< ProjectState > getState(const std::string &project_id) const`
- Source: `include/projects/project_lifecycle.h`:156
- Brief: Return the current state of a project.
- Parameters:
  - `project_id` (const std::string &): n/a
- Return: ProjectState if found, std::nullopt if unknown project.
- Details: ProjectState if found, std::nullopt if unknown project.

#### `Status initProject(const std::string &project_id, const std::string &actor={})`
- Source: `include/projects/project_lifecycle.h`:108
- Brief: Initialise lifecycle tracking for a newly created project.
- Parameters:
  - `project_id` (const std::string &): Identifier of the project.
  - `actor` (const std::string &): Input parameter.
- Return: Return value.
- Details: ── Public API ─────────────────────────────────────────────────────────────── Sets the initial state to CREATED and records the first audit entry. Returns an error if lifecycle state already exists for the project. project_id Project UUID. actor Identity of the creator (optional). project_id Identifier of the project. actor Input parameter. Return value.

#### `bool isValidTransition(ProjectState from, ProjectState to) noexcept`
- Source: `include/projects/project_lifecycle.h`:172
- Brief: Validate whether a transition from → to is permitted.
- Parameters:
  - `from` (ProjectState): n/a
  - `to` (ProjectState): n/a

#### `~ProjectLifecycle()=default`
- Source: `include/projects/project_lifecycle.h`:97
- Brief: n/a
- Parameters: none

### themis::projects::ProjectMerge

#### `ProjectMerge(std::shared_ptr< RocksDBWrapper > storage)`
- Source: `include/projects/project_diff.h`:164
- Brief: n/a
- Parameters:
  - `storage` (std::shared_ptr< RocksDBWrapper >): n/a

#### `MergeResult merge(const SnapshotId &ancestor_snap, const SnapshotId &ours_snap, const SnapshotId &theirs_snap) const`
- Source: `include/projects/project_diff.h`:174
- Brief: Perform a three-way merge.
- Parameters:
  - `ancestor_snap` (const SnapshotId &): Common ancestor snapshot.
  - `ours_snap` (const SnapshotId &): Our current state snapshot.
  - `theirs_snap` (const SnapshotId &): Incoming state snapshot.
- Return: MergeResult with applied deltas and any unresolved conflicts.
- Details: ancestor_snap Common ancestor snapshot. ours_snap Our current state snapshot. theirs_snap Incoming state snapshot. MergeResult with applied deltas and any unresolved conflicts.

### themis::projects::ProjectMetrics

#### `ProjectMetrics() noexcept=default`
- Source: `include/projects/project_metrics.h`:56
- Brief: n/a
- Parameters: none

#### `ProjectMetrics(const ProjectMetrics &)=delete`
- Source: `include/projects/project_metrics.h`:59
- Brief: Not copyable — each owner should hold a shared_ptr<ProjectMetrics>.
- Parameters:
  - `<unnamed>` (const ProjectMetrics &): n/a

#### `uint64_t changesTotal() const noexcept`
- Source: `include/projects/project_metrics.h`:89
- Brief: Return the total number of change events recorded.
- Parameters: none

#### `uint64_t diffCallsTotal() const noexcept`
- Source: `include/projects/project_metrics.h`:94
- Brief: Return the total number of diff calls.
- Parameters: none

#### `uint64_t diffDurationMsTotal() const noexcept`
- Source: `include/projects/project_metrics.h`:99
- Brief: Return the cumulative diff duration in milliseconds.
- Parameters: none

#### `std::string getMetricsText() const`
- Source: `include/projects/project_metrics.h`:111
- Brief: Produce a Prometheus text-format (v0.0.4) metrics payload.
- Parameters: none
- Details: Returns an empty string when no data has been recorded yet (avoids emitting zero-value metrics on a fresh instance).

#### `ProjectMetrics & operator=(const ProjectMetrics &)=delete`
- Source: `include/projects/project_metrics.h`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ProjectMetrics &): n/a

#### `void recordChange() noexcept`
- Source: `include/projects/project_metrics.h`:70
- Brief: Increment the collaboration-change counter by one.
- Parameters: none
- Details: Call once per CollaborationManager::notifyChange() invocation. Thread-safe.

#### `void recordDiff(uint64_t latency_ms) noexcept`
- Source: `include/projects/project_metrics.h`:81
- Brief: Record one completed ProjectDiff::diff() call.
- Parameters:
  - `latency_ms` (uint64_t): Wall-clock duration of the diff in milliseconds.
- Details: latency_ms Wall-clock duration of the diff in milliseconds. Thread-safe.

### themis::projects::ProjectStateTransition

#### `ProjectStateTransition fromJson(const json &j)`
- Source: `include/projects/project_lifecycle.h`:75
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: value(), projectStateFromString(), value_or().

#### `json toJson() const`
- Source: `include/projects/project_lifecycle.h`:74
- Brief: n/a
- Parameters: none

### themis::projects::ProjectTemplate

#### `ProjectTemplate(std::shared_ptr< RocksDBWrapper > storage)`
- Source: `include/projects/project_template.h`:91
- Brief: n/a
- Parameters:
  - `storage` (std::shared_ptr< RocksDBWrapper >): n/a

#### `std::optional< std::string > createObjectFromDefinition(const std::string &project_id, const json &object_def, bool include_sample_data)`
- Source: `include/projects/project_template.h`:146
- Brief: Write a single object entry to storage; return object name on success.
- Parameters:
  - `project_id` (const std::string &): Identifier of the project.
  - `object_def` (const json &): n/a
  - `include_sample_data` (bool): Input parameter.
- Return: Return value.
- Details: ─── Object creation ───────────────────────────────────────────────────────── project_id Identifier of the project. obj_def Input parameter. include_sample_data Input parameter. Return value.

#### `std::string generateUuid() const`
- Source: `include/projects/project_template.h`:143
- Brief: Generate a new UUID string.
- Parameters: none

#### `json getBuiltinTemplateSchema(BuiltinTemplate tmpl) const`
- Source: `include/projects/project_template.h`:140
- Brief: Return the JSON schema for a built-in template (object definitions).
- Parameters:
  - `tmpl` (BuiltinTemplate): n/a

#### `TemplateInstantiationResult instantiate(BuiltinTemplate tmpl, const TemplateOptions &options)`
- Source: `include/projects/project_template.h`:101
- Brief: Instantiate a project from a built-in template.
- Parameters:
  - `tmpl` (BuiltinTemplate): Input parameter.
  - `options` (const TemplateOptions &): Input parameter.
- Return: TemplateInstantiationResult describing success or failure.
- Details: ─── instantiate ───────────────────────────────────────────────────────────── tmpl The built-in template to use. options Instantiation options (must include a project_name). TemplateInstantiationResult describing success or failure. tmpl Input parameter. options Input parameter. Return value.

#### `TemplateInstantiationResult instantiateFromDefinition(const json &template_def, const TemplateOptions &options)`
- Source: `include/projects/project_template.h`:116
- Brief: Instantiate a project from a caller-supplied JSON template definition.
- Parameters:
  - `template_def` (const json &): Input parameter.
  - `options` (const TemplateOptions &): Input parameter.
- Return: TemplateInstantiationResult.
- Details: Instantiate From Definition. The definition is validated against the template schema before any objects are written. template_def JSON template definition (see validateTemplateDefinition). options Instantiation options. TemplateInstantiationResult. template_def Input parameter. options Input parameter. Return value.

#### `std::vector< std::string > listBuiltinTemplates()`
- Source: `include/projects/project_template.h`:124
- Brief: List the names of all available built-in templates.
- Parameters: none
- Return: Return value.
- Details: ─── listBuiltinTemplates ───────────────────────────────────────────────────── Return value. Calls: builtinTemplateToString().

#### `Status validateTemplateDefinition(const json &template_def)`
- Source: `include/projects/project_template.h`:134
- Brief: Validate a custom template definition.
- Parameters:
  - `template_def` (const json &): n/a
- Return: Status{true} if the definition is valid, Status{false, reason} otherwise.
- Details: ─── Validation ─────────────────────────────────────────────────────────────── Required top-level fields: "name" (string), "objects" (array). Each object entry must have: "type" (string), "name" (string). Status{true} if the definition is valid, Status{false, reason} otherwise. def Input parameter. Return value. Calls: is_object(), Status::Error(), contains(), is_string(), is_array(), Status::OK().

#### `~ProjectTemplate()=default`
- Source: `include/projects/project_template.h`:92
- Brief: n/a
- Parameters: none

### themis::projects::ProjectVersioning

#### `ProjectVersioning(std::shared_ptr< RocksDBWrapper > storage)`
- Source: `include/projects/project_versioning.h`:73
- Brief: n/a
- Parameters:
  - `storage` (std::shared_ptr< RocksDBWrapper >): n/a

#### `std::vector< std::string > collectProjectDocKeys(const std::string &project_id) const`
- Source: `include/projects/project_versioning.h`:142
- Brief: Collect all doc keys for a project from storage.
- Parameters:
  - `project_id` (const std::string &): n/a

#### `Sha256Digest computeChecksum(const std::string &data) const`
- Source: `include/projects/project_versioning.h`:139
- Brief: n/a
- Parameters:
  - `data` (const std::string &): n/a

#### `std::variant< SnapshotId, Status > createSnapshot(const std::string &project_id, const std::string &description={}, const json &metadata=json::object())`
- Source: `include/projects/project_versioning.h`:88
- Brief: Create an immutable snapshot of the given project.
- Parameters:
  - `project_id` (const std::string &): Project UUID whose state is to be captured.
  - `description` (const std::string &): Optional human-readable label for the snapshot.
  - `metadata` (const json &): Optional arbitrary metadata stored with the snapshot.
- Return: New SnapshotId on success, or a Status{false, …} on failure.
- Details: Captures the current list of document metadata records stored under the project and computes a SHA-256 content checksum over the serialised payload. project_id Project UUID whose state is to be captured. description Optional human-readable label for the snapshot. metadata Optional arbitrary metadata stored with the snapshot. New SnapshotId on success, or a Status{false, …} on failure.

#### `Status deleteSnapshot(const SnapshotId &snap_id)`
- Source: `include/projects/project_versioning.h`:111
- Brief: Delete a snapshot and all associated data.
- Parameters:
  - `snap_id` (const SnapshotId &): Identifier of the snap.
- Return: Return value.
- Details: Delete Snapshot. This is an irreversible operation. Active snapshots referenced by an in-progress restore cannot be deleted and will return an error. snap_id Identifier of the snap. Return value. Calls: empty(), Status::Error(), starts_with(), lock(), get(), safeJsonParse(), is_null(), is_object().

#### `std::string generateUuid() const`
- Source: `include/projects/project_versioning.h`:138
- Brief: n/a
- Parameters: none

#### `std::optional< SnapshotMeta > getSnapshot(const SnapshotId &snap_id) const`
- Source: `include/projects/project_versioning.h`:98
- Brief: Retrieve snapshot metadata by ID.
- Parameters:
  - `snap_id` (const SnapshotId &): n/a
- Return: SnapshotMeta if found, std::nullopt otherwise.
- Details: SnapshotMeta if found, std::nullopt otherwise.

#### `std::vector< SnapshotMeta > listSnapshots(const std::string &project_id) const`
- Source: `include/projects/project_versioning.h`:103
- Brief: List all snapshots belonging to a project, newest first.
- Parameters:
  - `project_id` (const std::string &): n/a

#### `Status restoreSnapshot(const SnapshotId &snap_id, const std::string &target_project_id)`
- Source: `include/projects/project_versioning.h`:123
- Brief: Restore a project to the state captured in a snapshot.
- Parameters:
  - `snap_id` (const SnapshotId &): Identifier of the snap.
  - `target_project_id` (const std::string &): Identifier of the target project.
- Return: Return value.
- Details: Restore Snapshot. The content checksum is verified before any data is written to target_project_id. On checksum mismatch the restore is aborted and an error Status is returned. snap_id Snapshot to restore from. target_project_id Destination project UUID (may be same as source). snap_id Identifier of the snap. target_project_id Identifier of the target project. Return value.

#### `bool verifySnapshot(const SnapshotId &snap_id) const`
- Source: `include/projects/project_versioning.h`:132
- Brief: Verify snapshot integrity (checksum re-computation).
- Parameters:
  - `snap_id` (const SnapshotId &): n/a
- Return: true if the stored checksum matches the current content.
- Details: true if the stored checksum matches the current content.

#### `~ProjectVersioning()=default`
- Source: `include/projects/project_versioning.h`:74
- Brief: n/a
- Parameters: none

### themis::projects::SnapshotMeta

#### `SnapshotMeta fromJson(const json &j)`
- Source: `include/projects/project_versioning.h`:53
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: value(), json::object().

#### `json toJson() const`
- Source: `include/projects/project_versioning.h`:52
- Brief: n/a
- Parameters: none

### themis::projects::Status

#### `Status Error(std::string msg)`
- Source: `include/projects/DocumentManager/document_manager.h`:108
- Brief: n/a
- Parameters:
  - `msg` (std::string): n/a

#### `Status OK()`
- Source: `include/projects/DocumentManager/document_manager.h`:107
- Brief: n/a
- Parameters: none

#### `Status(bool success, std::string msg={})`
- Source: `include/projects/DocumentManager/document_manager.h`:104
- Brief: n/a
- Parameters:
  - `success` (bool): n/a
  - `msg` (std::string): n/a

