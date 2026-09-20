# CONFIG DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\config\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\config\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 29
- Compounds: 103
- Classes/Structs: 52
- Namespaces: 14
- File Compounds: 29

## Namespaces
- benchmark
- cms
- prometheus
- std
- testing
- themis
- themis::config
- themis::config::@060356276226016333360373033113373214261337157121
- themis::config::@107240002240116025345143054126217343223246243271
- themis::config::@257020160243035230026110161175257171225153366327
- themis::config::@272024027246117113103111134261002021300327043064
- themis::config::@350033207235101135137275166052364252156151046033
- themis::config::bench
- themis::config::test

## Types
### Classes
- ConfigMigrationScannerTest
- themis::config::ConfigAuditLog
- themis::config::ConfigEncryptedStore
- themis::config::ConfigEncryptionException
- themis::config::ConfigException
- themis::config::ConfigFileWatcher
- themis::config::ConfigKeyNotFoundException
- themis::config::ConfigMetricsExporter
- themis::config::ConfigNotFoundException
- themis::config::ConfigPathResolver
- themis::config::ConfigPathResolver::DeprecationAggregator
- themis::config::ConfigPermissionException
- themis::config::ConfigSchemaValidator
- themis::config::InvalidPathException
- themis::config::LRUCacheWithTTL
- themis::config::MappingNotFoundException
- themis::config::SchemaValidationException
- themis::config::bench::ConfigEncryptedStoreHotPathFixture
- themis::config::bench::ConfigPathResolverBenchFixture
- themis::config::bench::ConfigResolveHotPathFixture
- themis::config::bench::ConfigValidateHotPathFixture
- themis::config::bench::MigrationScannerBenchFixture
- themis::config::test::AuditLogDirectTest
- themis::config::test::CacheEnvConfigTest
- themis::config::test::ConfigAuditLogTest
- themis::config::test::ConfigEncryptedStoreHardeningTest
- themis::config::test::ConfigEncryptedStoreTest
- themis::config::test::ConfigEnvOverlayTest
- themis::config::test::ConfigFileWatcherHardeningTest
- themis::config::test::ConfigFileWatcherTest
- themis::config::test::ConfigMetricsExporterTest
- themis::config::test::ConfigMetricsScrapeTest
- themis::config::test::ConfigPathResolverExtraTest
- themis::config::test::ConfigPathResolverTest
- themis::config::test::ConfigResolverHardeningTest
- themis::config::test::ConfigSchemaValidatorTest
- themis::config::test::ConfigValidatorHardeningTest
- themis::config::test::HotReloadIntegrationTest
- themis::config::test::LRUCacheTest
- themis::config::test::SchemaValidatorExtraTest

### Structs
- cms::ScanMatch
- themis::config::AuditEntry
- themis::config::ConfigEncryptedBlob
- themis::config::ConfigEncryptedStore::KeyMaterial
- themis::config::ConfigPathResolver::CacheConfig
- themis::config::ConfigPathResolver::DeprecationEntry
- themis::config::ConfigPathResolver::Metrics
- themis::config::ConfigSchemaValidator::ValidationResult
- themis::config::LRUCacheWithTTL::ListEntry
- themis::config::LRUCacheWithTTL::MapEntry
- themis::config::LRUCacheWithTTL::Stats
- themis::config::PathMappingMetadata

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 644

### ConfigMigrationScannerTest

#### `void SetUp() override`
- Source: `tests/config/test_config_migration_scanner.cpp`:44
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_migration_scanner.cpp`:50
- Brief: n/a
- Parameters: none

### bench_config_migration_scanner.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:253
- Brief: n/a
- Parameters: none

### bench_config_path_resolver.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:432
- Brief: n/a
- Parameters: none

### cms

#### `bool fixFile(const fs::path &file, const std::vector< ScanMatch > &matches, bool dry_run)`
- Source: `include/config/config_migration_scanner_impl.h`:170
- Brief: Apply fix: rewrite the file replacing legacy strings with new paths.
- Parameters:
  - `file` (const fs::path &): Input parameter.
  - `matches` (const std::vector< ScanMatch > &): Input parameter.
  - `dry_run` (bool): Input parameter.
- Return: True when the operation succeeds.
- Details: file Input parameter. matches Input parameter. dry_run Input parameter. True when the operation succeeds. Creates a .bak backup before modifying. Returns true on success (or when no change is needed), false on I/O error. Calls: std::find_if(), begin(), end(), emplace_back(), empty(), ifs(), is_open(), content().

#### `std::string formatTimePoint(const std::optional< std::chrono::system_clock::time_point > &timestamp)`
- Source: `include/config/config_migration_scanner_impl.h`:99
- Brief: Format Time Point.
- Parameters:
  - `timestamp` (const std::optional< std::chrono::system_clock::time_point > &): Input parameter.
- Return: Return value.
- Details: timestamp Input parameter. Return value. Calls: has_value(), year(), month(), day(), str().

#### `void printCsv(const std::vector< ScanMatch > &matches)`
- Source: `include/config/config_migration_scanner_impl.h`:303
- Brief: Print Csv.
- Parameters:
  - `matches` (const std::vector< ScanMatch > &): Input parameter.
- Details: matches Input parameter. Calls: find(), q(), string().

#### `void printJson(const std::vector< ScanMatch > &matches)`
- Source: `include/config/config_migration_scanner_impl.h`:268
- Brief: Print Json.
- Parameters:
  - `matches` (const std::vector< ScanMatch > &): Input parameter.
- Details: matches Input parameter. Calls: size(), escape(), string().

#### `void printText(const std::vector< ScanMatch > &matches)`
- Source: `include/config/config_migration_scanner_impl.h`:245
- Brief: Print Text.
- Parameters:
  - `matches` (const std::vector< ScanMatch > &): Input parameter.
- Details: matches Input parameter. Calls: string(), empty().

#### `const std::set< std::string > & scanExtensions()`
- Source: `include/config/config_migration_scanner_impl.h`:68
- Brief: Scan Extensions.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements scanExtensions without additional internal calls.

#### `std::vector< ScanMatch > scanFile(const fs::path &file)`
- Source: `include/config/config_migration_scanner_impl.h`:122
- Brief: Scan a single file for any legacy path references.
- Parameters:
  - `file` (const fs::path &): Input parameter.
- Return: Return value.
- Details: file Input parameter. Return value. Calls: ifs(), is_open(), std::getline(), themis::config::ConfigPathResolver::legacyPathMappings(), find(), themis::config::ConfigPathResolver::getMetadata(), isRemovalDue(), formatTimePoint().

#### `bool shouldScanFile(const fs::path &p)`
- Source: `include/config/config_migration_scanner_impl.h`:81
- Brief: Should Scan File.
- Parameters:
  - `p` (const fs::path &): Input parameter.
- Return: True when the operation succeeds.
- Details: p Input parameter. True when the operation succeeds. Calls: filename(), string(), std::transform(), begin(), end(), extension(), scanExtensions(), count().

### test_config_migration_scanner.cpp

#### `TEST(FormatTimePointTest, NulloptReturnsEmptyString)`
- Source: `tests/config/test_config_migration_scanner.cpp`:439
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTimePointTest): n/a
  - `<unnamed>` (NulloptReturnsEmptyString): n/a

#### `TEST(FormatTimePointTest, SingleDigitMonthAndDayArePadded)`
- Source: `tests/config/test_config_migration_scanner.cpp`:456
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTimePointTest): n/a
  - `<unnamed>` (SingleDigitMonthAndDayArePadded): n/a

#### `TEST(FormatTimePointTest, ValidDateFormatsAsYyyyMmDd)`
- Source: `tests/config/test_config_migration_scanner.cpp`:443
- Brief: n/a
- Parameters:
  - `<unnamed>` (FormatTimePointTest): n/a
  - `<unnamed>` (ValidDateFormatsAsYyyyMmDd): n/a

#### `TEST(PrintCsvTest, EmptyMatchesProducesOnlyHeaderRow)`
- Source: `tests/config/test_config_migration_scanner.cpp`:654
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintCsvTest): n/a
  - `<unnamed>` (EmptyMatchesProducesOnlyHeaderRow): n/a

#### `TEST(PrintCsvTest, FieldsWithCommasAreQuoted)`
- Source: `tests/config/test_config_migration_scanner.cpp`:706
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintCsvTest): n/a
  - `<unnamed>` (FieldsWithCommasAreQuoted): n/a

#### `TEST(PrintCsvTest, RemovalOverdueColumn)`
- Source: `tests/config/test_config_migration_scanner.cpp`:686
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintCsvTest): n/a
  - `<unnamed>` (RemovalOverdueColumn): n/a

#### `TEST(PrintCsvTest, SingleMatchHasCorrectNumberOfColumns)`
- Source: `tests/config/test_config_migration_scanner.cpp`:665
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintCsvTest): n/a
  - `<unnamed>` (SingleMatchHasCorrectNumberOfColumns): n/a

#### `TEST(PrintJsonTest, EmptyMatchesProducesEmptyJsonArray)`
- Source: `tests/config/test_config_migration_scanner.cpp`:566
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintJsonTest): n/a
  - `<unnamed>` (EmptyMatchesProducesEmptyJsonArray): n/a

#### `TEST(PrintJsonTest, JsonEscapesDoubleQuotesInPaths)`
- Source: `tests/config/test_config_migration_scanner.cpp`:641
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintJsonTest): n/a
  - `<unnamed>` (JsonEscapesDoubleQuotesInPaths): n/a

#### `TEST(PrintJsonTest, MultipleMatchesNoTrailingCommaOnLastEntry)`
- Source: `tests/config/test_config_migration_scanner.cpp`:622
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintJsonTest): n/a
  - `<unnamed>` (MultipleMatchesNoTrailingCommaOnLastEntry): n/a

#### `TEST(PrintJsonTest, RemovalOverdueFalseWhenNotDue)`
- Source: `tests/config/test_config_migration_scanner.cpp`:612
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintJsonTest): n/a
  - `<unnamed>` (RemovalOverdueFalseWhenNotDue): n/a

#### `TEST(PrintJsonTest, RemovalOverdueTrueWhenDue)`
- Source: `tests/config/test_config_migration_scanner.cpp`:602
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintJsonTest): n/a
  - `<unnamed>` (RemovalOverdueTrueWhenDue): n/a

#### `TEST(PrintJsonTest, SingleMatchContainsRequiredKeys)`
- Source: `tests/config/test_config_migration_scanner.cpp`:574
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintJsonTest): n/a
  - `<unnamed>` (SingleMatchContainsRequiredKeys): n/a

#### `TEST(PrintJsonTest, SingleMatchLineNumberIsNumeric)`
- Source: `tests/config/test_config_migration_scanner.cpp`:592
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintJsonTest): n/a
  - `<unnamed>` (SingleMatchLineNumberIsNumeric): n/a

#### `TEST(PrintTextTest, EmptyMatchesProducesNoOutput)`
- Source: `tests/config/test_config_migration_scanner.cpp`:508
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintTextTest): n/a
  - `<unnamed>` (EmptyMatchesProducesNoOutput): n/a

#### `TEST(PrintTextTest, MultipleMatchesProduceMultipleLines)`
- Source: `tests/config/test_config_migration_scanner.cpp`:550
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintTextTest): n/a
  - `<unnamed>` (MultipleMatchesProduceMultipleLines): n/a

#### `TEST(PrintTextTest, OverdueFlagAbsentWhenNotDue)`
- Source: `tests/config/test_config_migration_scanner.cpp`:540
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintTextTest): n/a
  - `<unnamed>` (OverdueFlagAbsentWhenNotDue): n/a

#### `TEST(PrintTextTest, OverdueFlagAppearsWhenRemovalDue)`
- Source: `tests/config/test_config_migration_scanner.cpp`:529
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintTextTest): n/a
  - `<unnamed>` (OverdueFlagAppearsWhenRemovalDue): n/a

#### `TEST(PrintTextTest, SingleMatchContainsExpectedFields)`
- Source: `tests/config/test_config_migration_scanner.cpp`:513
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrintTextTest): n/a
  - `<unnamed>` (SingleMatchContainsExpectedFields): n/a

#### `TEST_F(ConfigMigrationScannerTest, FixFileCreatesBackup)`
- Source: `tests/config/test_config_migration_scanner.cpp`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (FixFileCreatesBackup): n/a

#### `TEST_F(ConfigMigrationScannerTest, FixFileDryRunDoesNotModifyFile)`
- Source: `tests/config/test_config_migration_scanner.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (FixFileDryRunDoesNotModifyFile): n/a

#### `TEST_F(ConfigMigrationScannerTest, FixFileDryRunPrintsWouldUpdateMessage)`
- Source: `tests/config/test_config_migration_scanner.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (FixFileDryRunPrintsWouldUpdateMessage): n/a

#### `TEST_F(ConfigMigrationScannerTest, FixFileIdempotent)`
- Source: `tests/config/test_config_migration_scanner.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (FixFileIdempotent): n/a

#### `TEST_F(ConfigMigrationScannerTest, FixFileNoMatchesReturnsTrueNoChange)`
- Source: `tests/config/test_config_migration_scanner.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (FixFileNoMatchesReturnsTrueNoChange): n/a

#### `TEST_F(ConfigMigrationScannerTest, FixFileReplacesAllOccurrences)`
- Source: `tests/config/test_config_migration_scanner.cpp`:278
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (FixFileReplacesAllOccurrences): n/a

#### `TEST_F(ConfigMigrationScannerTest, FixFileReplacesLegacyPath)`
- Source: `tests/config/test_config_migration_scanner.cpp`:245
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (FixFileReplacesLegacyPath): n/a

#### `TEST_F(ConfigMigrationScannerTest, FixFileReplacesMultipleDifferentLegacyPaths)`
- Source: `tests/config/test_config_migration_scanner.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (FixFileReplacesMultipleDifferentLegacyPaths): n/a

#### `TEST_F(ConfigMigrationScannerTest, NonScanableFilesAreSkipped)`
- Source: `tests/config/test_config_migration_scanner.cpp`:412
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (NonScanableFilesAreSkipped): n/a

#### `TEST_F(ConfigMigrationScannerTest, RecursiveScanFindsMatchesInSubdirectories)`
- Source: `tests/config/test_config_migration_scanner.cpp`:381
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (RecursiveScanFindsMatchesInSubdirectories): n/a

#### `TEST_F(ConfigMigrationScannerTest, ScanFileFindsKnownLegacyPath)`
- Source: `tests/config/test_config_migration_scanner.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ScanFileFindsKnownLegacyPath): n/a

#### `TEST_F(ConfigMigrationScannerTest, ScanFileMultipleLegacyPathsInOneFile)`
- Source: `tests/config/test_config_migration_scanner.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ScanFileMultipleLegacyPathsInOneFile): n/a

#### `TEST_F(ConfigMigrationScannerTest, ScanFileNoMatches)`
- Source: `tests/config/test_config_migration_scanner.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ScanFileNoMatches): n/a

#### `TEST_F(ConfigMigrationScannerTest, ScanFilePopulatesMigrationGuideUrl)`
- Source: `tests/config/test_config_migration_scanner.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ScanFilePopulatesMigrationGuideUrl): n/a

#### `TEST_F(ConfigMigrationScannerTest, ScanFileReportsCorrectLineNumbers)`
- Source: `tests/config/test_config_migration_scanner.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ScanFileReportsCorrectLineNumbers): n/a

#### `TEST_F(ConfigMigrationScannerTest, ScanFileUnreadableReturnsEmpty)`
- Source: `tests/config/test_config_migration_scanner.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ScanFileUnreadableReturnsEmpty): n/a

#### `TEST_F(ConfigMigrationScannerTest, ScanMatchHasCorrectCategory)`
- Source: `tests/config/test_config_migration_scanner.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ScanMatchHasCorrectCategory): n/a

#### `TEST_F(ConfigMigrationScannerTest, ScanMatchHasFormattedDates)`
- Source: `tests/config/test_config_migration_scanner.cpp`:351
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ScanMatchHasFormattedDates): n/a

#### `TEST_F(ConfigMigrationScannerTest, ShouldNotScanCppSource)`
- Source: `tests/config/test_config_migration_scanner.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ShouldNotScanCppSource): n/a

#### `TEST_F(ConfigMigrationScannerTest, ShouldNotScanHeader)`
- Source: `tests/config/test_config_migration_scanner.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ShouldNotScanHeader): n/a

#### `TEST_F(ConfigMigrationScannerTest, ShouldNotScanMarkdown)`
- Source: `tests/config/test_config_migration_scanner.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ShouldNotScanMarkdown): n/a

#### `TEST_F(ConfigMigrationScannerTest, ShouldScanCaseInsensitiveExtension)`
- Source: `tests/config/test_config_migration_scanner.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ShouldScanCaseInsensitiveExtension): n/a

#### `TEST_F(ConfigMigrationScannerTest, ShouldScanEnv)`
- Source: `tests/config/test_config_migration_scanner.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ShouldScanEnv): n/a

#### `TEST_F(ConfigMigrationScannerTest, ShouldScanIni)`
- Source: `tests/config/test_config_migration_scanner.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ShouldScanIni): n/a

#### `TEST_F(ConfigMigrationScannerTest, ShouldScanJson)`
- Source: `tests/config/test_config_migration_scanner.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ShouldScanJson): n/a

#### `TEST_F(ConfigMigrationScannerTest, ShouldScanToml)`
- Source: `tests/config/test_config_migration_scanner.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ShouldScanToml): n/a

#### `TEST_F(ConfigMigrationScannerTest, ShouldScanYaml)`
- Source: `tests/config/test_config_migration_scanner.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMigrationScannerTest): n/a
  - `<unnamed>` (ShouldScanYaml): n/a

#### `std::string captureStdout(Fn fn)`
- Source: `tests/config/test_config_migration_scanner.cpp`:475
- Brief: n/a
- Parameters:
  - `fn` (Fn): n/a

#### `cms::ScanMatch makeScanMatch(const std::string &file, int line, const std::string &legacy, const std::string &new_p, const std::string &category, bool removal_due=false, const std::string &depr_date="2024-01-01", const std::string &rm_date="2026-06-30", const std::string &guide="docs/config_migration_guide.md")`
- Source: `tests/config/test_config_migration_scanner.cpp`:484
- Brief: n/a
- Parameters:
  - `file` (const std::string &): n/a
  - `line` (int): n/a
  - `legacy` (const std::string &): n/a
  - `new_p` (const std::string &): n/a
  - `category` (const std::string &): n/a
  - `removal_due` (bool): n/a
  - `depr_date` (const std::string &): n/a
  - `rm_date` (const std::string &): n/a
  - `guide` (const std::string &): n/a

#### `std::string readFile(const fs::path &path)`
- Source: `tests/config/test_config_migration_scanner.cpp`:32
- Brief: n/a
- Parameters:
  - `path` (const fs::path &): n/a

#### `void writeFile(const fs::path &path, const std::string &content)`
- Source: `tests/config/test_config_migration_scanner.cpp`:26
- Brief: n/a
- Parameters:
  - `path` (const fs::path &): n/a
  - `content` (const std::string &): n/a

### themis::config

#### `bool isFailClosedClass(ConfigFailureClass fc) noexcept`
- Source: `include/config/config_contract.h`:177
- Brief: Returns true when the given failure class mandates fail-closed denial.
- Parameters:
  - `fc` (ConfigFailureClass): n/a
- Details: Use this predicate in catch blocks to decide whether to use a fallback or cache: try{ returnresolver->resolve(path); }catch(constConfigException&ex){ if(isFailClosedClass(classifyError(ex.error().code()))){ //Harddenial—uselast-known-goodordenyrequest throw; } //Structurally-invalidinput—alsodeny throw; }

#### `std::chrono::system_clock::time_point parseDate(const std::string &iso_date)`
- Source: `src/config/config_path_resolver.cpp`:415
- Brief: Helper to create a date from ISO string (YYYY-MM-DD).
- Parameters:
  - `iso_date` (const std::string &): Input parameter.
- Return: Return value.
- Details: iso_date Input parameter. Return value. Calls: ss(), std::get_time(), std::chrono::system_clock::from_time_t(), std::mktime().

### themis::config::ConfigAuditLog

#### `void clear()`
- Source: `include/config/config_audit_log.h`:86
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `void disable()`
- Source: `include/config/config_audit_log.h`:45
- Brief: Disable.
- Parameters: none
- Details: Calls: store().

#### `void enable()`
- Source: `include/config/config_audit_log.h`:40
- Brief: Enable.
- Parameters: none
- Details: Calls: store().

#### `std::vector< AuditEntry > getEntries() const`
- Source: `include/config/config_audit_log.h`:75
- Brief: Get Entries.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool isEnabled() const`
- Source: `include/config/config_audit_log.h`:51
- Brief: Is Enabled.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `std::size_t maxEntries() const`
- Source: `include/config/config_audit_log.h`:63
- Brief: Max Entries.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void record(AuditEntry entry)`
- Source: `include/config/config_audit_log.h`:69
- Brief: Record.
- Parameters:
  - `entry` (AuditEntry): Input parameter.
- Details: entry Input parameter. entry Input parameter. Calls: load(), lock(), push_back(), std::move(), size(), pop_front().

#### `void setMaxEntries(std::size_t max)`
- Source: `include/config/config_audit_log.h`:57
- Brief: Set Max Entries.
- Parameters:
  - `max` (std::size_t): Input parameter.
- Details: max Input parameter. max Input parameter. Calls: lock(), size(), pop_front().

#### `std::size_t size() const`
- Source: `include/config/config_audit_log.h`:81
- Brief: Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::config::ConfigEncryptedBlob

#### `ConfigEncryptedBlob fromJson(const std::string &json_str)`
- Source: `include/config/config_encrypted_store.h`:67
- Brief: From Json.
- Parameters:
  - `json_str` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - ConfigEncryptionException: if an error occurs.
- Details: json_str Input parameter. Return value. json_str Input parameter. Return value. ConfigEncryptionException if an error occurs. Calls: nlohmann::json::parse(), at(), base64Decode(), std::string(), what().

#### `std::string toJson() const`
- Source: `include/config/config_encrypted_store.h`:60
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::config::ConfigEncryptedStore

#### `ConfigEncryptedStore()`
- Source: `include/config/config_encrypted_store.h`:72
- Brief: n/a
- Parameters: none

#### `ConfigEncryptedStore(ConfigEncryptedStore &&) noexcept=delete`
- Source: `include/config/config_encrypted_store.h`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStore &&): n/a

#### `ConfigEncryptedStore(const ConfigEncryptedStore &)=delete`
- Source: `include/config/config_encrypted_store.h`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ConfigEncryptedStore &): n/a

#### `std::string aesGcmDecrypt(const std::vector< uint8_t > &ciphertext, const std::vector< uint8_t > &key, const std::vector< uint8_t > &iv, const std::vector< uint8_t > &tag)`
- Source: `include/config/config_encrypted_store.h`:213
- Brief: Aes Gcm Decrypt.
- Parameters:
  - `ciphertext` (const std::vector< uint8_t > &): Input parameter.
  - `key` (const std::vector< uint8_t > &): Input parameter.
  - `iv` (const std::vector< uint8_t > &): Input parameter.
  - `tag` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Throws:
  - ConfigEncryptionException: if an error occurs.
- Details: ciphertext Input parameter. key Input parameter. iv Input parameter. tag Input parameter. Return value. ciphertext Input parameter. key Input parameter. iv Input parameter. tag Input parameter. Return value. ConfigEncryptionException if an error occurs. Calls: size(), EVP_CIPHER_CTX_new(), CtxGuard(), EVP_CIPHER_CTX_free(), EVP_DecryptInit_ex(), EVP_aes_256_gcm(), EVP_CIPHER_CTX_ctrl(), data().

#### `std::vector< uint8_t > aesGcmEncrypt(const std::string &plaintext, const std::vector< uint8_t > &key, std::vector< uint8_t > &out_iv, std::vector< uint8_t > &out_tag)`
- Source: `include/config/config_encrypted_store.h`:199
- Brief: Aes Gcm Encrypt.
- Parameters:
  - `plaintext` (const std::string &): Input parameter.
  - `key` (const std::vector< uint8_t > &): Input parameter.
  - `out_iv` (std::vector< uint8_t > &): Input/output parameter.
  - `out_tag` (std::vector< uint8_t > &): Input/output parameter.
- Return: Return value.
- Throws:
  - ConfigEncryptionException: if an error occurs.
- Details: plaintext Input parameter. key Input parameter. out_iv Input/output parameter. out_tag Input/output parameter. Return value. plaintext Input parameter. key Input parameter. out_iv Input/output parameter. out_tag Input/output parameter. Return value. ConfigEncryptionException if an error occurs. Calls: generateIV(), resize(), EVP_CIPHER_CTX_new(), CtxGuard(), EVP_CIPHER_CTX_free(), EVP_EncryptInit_ex(), EVP_aes_256_gcm(), EVP_CIPHER_CTX_ctrl().

#### `void clear()`
- Source: `include/config/config_encrypted_store.h`:136
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `bool contains(const std::string &config_key) const`
- Source: `include/config/config_encrypted_store.h`:119
- Brief: Contains.
- Parameters:
  - `config_key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: config_key Input parameter. True when the operation succeeds.

#### `uint32_t currentKeyVersion() const`
- Source: `include/config/config_encrypted_store.h`:152
- Brief: Current Key Version.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string decryptBlob(const ConfigEncryptedBlob &blob) const`
- Source: `include/config/config_encrypted_store.h`:230
- Brief: Decrypt Blob.
- Parameters:
  - `blob` (const ConfigEncryptedBlob &): Input parameter.
- Return: Return value.
- Details: blob Input parameter. Return value.

#### `void deserialize(const std::string &json_str)`
- Source: `include/config/config_encrypted_store.h`:168
- Brief: Deserialize.
- Parameters:
  - `json_str` (const std::string &): Input parameter.
- Throws:
  - ConfigEncryptionException: if an error occurs.
- Details: json_str Input parameter. json_str Input parameter. ConfigEncryptionException if an error occurs. Calls: nlohmann::json::parse(), at(), base64Decode(), size(), std::to_string(), begin(), end(), key().

#### `ConfigEncryptedBlob encryptValue(const std::string &plaintext) const`
- Source: `include/config/config_encrypted_store.h`:224
- Brief: Encrypt Value.
- Parameters:
  - `plaintext` (const std::string &): Input parameter.
- Return: Return value.
- Details: plaintext Input parameter. Return value.

#### `std::vector< uint8_t > generateIV()`
- Source: `include/config/config_encrypted_store.h`:189
- Brief: Generate IV.
- Parameters: none
- Return: Return value.
- Throws:
  - ConfigEncryptionException: if an error occurs.
- Details: Return value. Return value. ConfigEncryptionException if an error occurs. Calls: iv(), RAND_bytes(), data(), size().

#### `std::vector< uint8_t > generateKey()`
- Source: `include/config/config_encrypted_store.h`:183
- Brief: - helpers -
- Parameters: none
- Return: Return value.
- Throws:
  - ConfigEncryptionException: if an error occurs.
- Details: Generate Key. Return value. Return value. ConfigEncryptionException if an error occurs. Calls: key(), RAND_bytes(), data(), size().

#### `std::string get(const std::string &config_key) const`
- Source: `include/config/config_encrypted_store.h`:98
- Brief: Get.
- Parameters:
  - `config_key` (const std::string &): Input parameter.
- Return: Return value.
- Details: config_key Input parameter. Return value.

#### `std::vector< std::string > keys() const`
- Source: `include/config/config_encrypted_store.h`:125
- Brief: Keys.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `ConfigEncryptedStore & operator=(ConfigEncryptedStore &&) noexcept=delete`
- Source: `include/config/config_encrypted_store.h`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStore &&): n/a

#### `ConfigEncryptedStore & operator=(const ConfigEncryptedStore &)=delete`
- Source: `include/config/config_encrypted_store.h`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ConfigEncryptedStore &): n/a

#### `bool remove(const std::string &config_key)`
- Source: `include/config/config_encrypted_store.h`:112
- Brief: Remove.
- Parameters:
  - `config_key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: config_key Input parameter. True when the operation succeeds. config_key Input parameter. True when the operation succeeds. Calls: lock(), erase().

#### `uint32_t rotateKey()`
- Source: `include/config/config_encrypted_store.h`:146
- Brief: Rotate Key.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: lock(), generateKey(), reserve(), size(), decryptBlob(), aesGcmEncrypt(), std::move(), std::fill().

#### `std::string serialize() const`
- Source: `include/config/config_encrypted_store.h`:162
- Brief: Serialize.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void set(const std::string &config_key, const std::string &plaintext)`
- Source: `include/config/config_encrypted_store.h`:91
- Brief: Set.
- Parameters:
  - `config_key` (const std::string &): Input parameter.
  - `plaintext` (const std::string &): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: config_key Input parameter. plaintext Input parameter. config_key Input parameter. plaintext Input parameter. std::invalid_argument if an error occurs. Calls: empty(), lock(), encryptValue().

#### `std::size_t size() const`
- Source: `include/config/config_encrypted_store.h`:131
- Brief: Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::optional< std::string > tryGet(const std::string &config_key) const`
- Source: `include/config/config_encrypted_store.h`:105
- Brief: Try Get.
- Parameters:
  - `config_key` (const std::string &): Input parameter.
- Return: Return value.
- Details: config_key Input parameter. Return value.

#### `~ConfigEncryptedStore()=default`
- Source: `include/config/config_encrypted_store.h`:74
- Brief: n/a
- Parameters: none

### themis::config::ConfigEncryptionException

#### `ConfigEncryptionException(const std::string &msg)`
- Source: `include/config/config_encrypted_store.h`:35
- Brief: Config Encryption Exception.
- Parameters:
  - `msg` (const std::string &): Input parameter.
- Return: Return value.
- Details: msg Input parameter. Return value.

### themis::config::ConfigException

#### `ConfigException(const std::string &message)`
- Source: `include/config/config_errors.h`:23
- Brief: n/a
- Parameters:
  - `message` (const std::string &): n/a

#### `const char * what() const noexcept override`
- Source: `include/config/config_errors.h`:24
- Brief: n/a
- Parameters: none

### themis::config::ConfigFileWatcher

#### `ConfigFileWatcher(ConfigFileWatcher &&)=delete`
- Source: `include/config/config_file_watcher.h`:36
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigFileWatcher &&): n/a

#### `ConfigFileWatcher(const ConfigFileWatcher &)=delete`
- Source: `include/config/config_file_watcher.h`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ConfigFileWatcher &): n/a

#### `ConfigFileWatcher(std::string watch_path, std::function< void()> callback, std::chrono::milliseconds debounce=std::chrono::milliseconds(200))`
- Source: `include/config/config_file_watcher.h`:26
- Brief: n/a
- Parameters:
  - `watch_path` (std::string): n/a
  - `callback` (std::function< void()>): n/a
  - `debounce` (std::chrono::milliseconds): n/a

#### `std::chrono::milliseconds debounceInterval() const`
- Source: `include/config/config_file_watcher.h`:54
- Brief: n/a
- Parameters: none

#### `bool isRunning() const`
- Source: `include/config/config_file_watcher.h`:50
- Brief: n/a
- Parameters: none

#### `ConfigFileWatcher & operator=(ConfigFileWatcher &&)=delete`
- Source: `include/config/config_file_watcher.h`:37
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigFileWatcher &&): n/a

#### `ConfigFileWatcher & operator=(const ConfigFileWatcher &)=delete`
- Source: `include/config/config_file_watcher.h`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ConfigFileWatcher &): n/a

#### `void scheduleCallback()`
- Source: `include/config/config_file_watcher.h`:83
- Brief: ── Debounce helper ─────────────────────────────────────────────────
- Parameters: none
- Details: ── Debounce helper ─────────────────────────────────────────────────────────── Calls: lk(), std::chrono::steady_clock::now().

#### `bool start()`
- Source: `include/config/config_file_watcher.h`:43
- Brief: Start.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Calls: load(), std::filesystem::exists(), spdlog::warn(), defined(), pipe2(), strerror(), kqueue(), pipe().

#### `void stop()`
- Source: `include/config/config_file_watcher.h`:48
- Brief: Stop.
- Parameters: none
- Details: Calls: exchange(), defined(), write(), spdlog::warn(), strerror(), SetEvent(), joinable(), join().

#### `void watchLoop()`
- Source: `include/config/config_file_watcher.h`:60
- Brief: ── Entry point for the watcher thread ──────────────────────────────
- Parameters: none
- Details: Watch Loop. Calls: defined(), watchLoopInotify(), watchLoopKqueue(), watchLoopReadDirChanges(), spdlog::warn(), store().

#### `const std::string & watchPath() const`
- Source: `include/config/config_file_watcher.h`:52
- Brief: n/a
- Parameters: none

#### `~ConfigFileWatcher()`
- Source: `include/config/config_file_watcher.h`:31
- Brief: n/a
- Parameters: none

### themis::config::ConfigKeyNotFoundException

#### `ConfigKeyNotFoundException(const std::string &key_id)`
- Source: `include/config/config_encrypted_store.h`:46
- Brief: Config Key Not Found Exception.
- Parameters:
  - `key_id` (const std::string &): Identifier of the key.
- Return: Return value.
- Details: key_id Identifier of the key. Return value.

### themis::config::ConfigMetricsExporter

#### `ConfigMetricsExporter()=delete`
- Source: `include/config/config_metrics_exporter.h`:30
- Brief: n/a
- Parameters: none

#### `std::string collect()`
- Source: `include/config/config_metrics_exporter.h`:36
- Brief: Collect.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: ConfigPathResolver::metrics(), load(), ConfigPathResolver::cacheStats(), ConfigPathResolver::legacyFallbacksByCategory(), lock(), counterDelta(), Increment(), Set().

#### `std::mutex & gaugeSinkFnMutex()`
- Source: `include/config/config_metrics_exporter.h`:65
- Brief: Gauge Sink Fn Mutex.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements gaugeSinkFnMutex without additional internal calls.

#### `GaugeSinkFn & gaugeSinkFnStorage()`
- Source: `include/config/config_metrics_exporter.h`:74
- Brief: Gauge Sink Fn Storage.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements gaugeSinkFnStorage without additional internal calls.

#### `void registerWithRegistry(const std::shared_ptr< prometheus::Registry > &registry)`
- Source: `include/config/config_metrics_exporter.h`:47
- Brief: Register With Registry.
- Parameters:
  - `registry` (const std::shared_ptr< prometheus::Registry > &): Input parameter.
- Details: registry Input parameter. registry Input parameter. Calls: lock(), prometheus::BuildCounter(), Name(), Help(), Register(), Add(), ConfigPathResolver::legacyFallbackCategories(), prometheus::BuildGauge().

#### `void setGaugeSinkFn(GaugeSinkFn fn)`
- Source: `include/config/config_metrics_exporter.h`:54
- Brief: Set Gauge Sink Fn.
- Parameters:
  - `fn` (GaugeSinkFn): Input parameter.
- Details: fn Input parameter. Calls: lk(), gaugeSinkFnMutex(), gaugeSinkFnStorage(), std::move().

#### `void updateMetricsCollector()`
- Source: `include/config/config_metrics_exporter.h`:41
- Brief: Update Metrics Collector.
- Parameters: none
- Details: Calls: ConfigPathResolver::metrics(), load(), ConfigPathResolver::cacheStats(), ConfigPathResolver::currentCacheConfig(), lk(), ConfigMetricsExporter::gaugeSinkFnMutex(), ConfigMetricsExporter::gaugeSinkFnStorage(), fn().

### themis::config::ConfigNotFoundException

#### `ConfigNotFoundException(const std::string &path, const std::vector< std::string > &attempted_paths)`
- Source: `include/config/config_errors.h`:32
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a
  - `attempted_paths` (const std::vector< std::string > &): n/a

#### `const std::vector< std::string > & attempted_paths() const`
- Source: `include/config/config_errors.h`:39
- Brief: n/a
- Parameters: none

#### `std::string buildMessage(const std::string &path, const std::vector< std::string > &attempted)`
- Source: `include/config/config_errors.h`:52
- Brief: Build Message.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `attempted` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: path Input parameter. attempted Input parameter. Return value. Calls: empty().

#### `const std::string & requested_path() const`
- Source: `include/config/config_errors.h`:38
- Brief: n/a
- Parameters: none

### themis::config::ConfigPathResolver

#### `std::vector< AuditEntry > auditLog()`
- Source: `include/config/config_path_resolver.h`:215
- Brief: Audit Log.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: getEntries().

#### `auto cacheStats()`
- Source: `include/config/config_path_resolver.h`:122
- Brief: Cache Stats.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: stats().

#### `void checkFallbackRateThreshold()`
- Source: `include/config/config_path_resolver.h`:319
- Brief: Check whether the current fallback rate has crossed the threshold and, if so, emit a rate-limited spdlog::warn.
- Parameters: none
- Details: Check Fallback Rate Threshold. Calls: load(), compare_exchange_strong(), spdlog::warn().

#### `void clearAuditLog()`
- Source: `include/config/config_path_resolver.h`:220
- Brief: Clear Audit Log.
- Parameters: none
- Details: Calls: clear().

#### `void clearCache()`
- Source: `include/config/config_path_resolver.h`:128
- Brief: Clear Cache.
- Parameters: none
- Details: Calls: clear().

#### `CacheConfig currentCacheConfig()`
- Source: `include/config/config_path_resolver.h`:173
- Brief: Current Cache Config.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Implements currentCacheConfig without additional internal calls.

#### `std::vector< DeprecationEntry > deprecationReport()`
- Source: `include/config/config_path_resolver.h`:202
- Brief: Deprecation Report.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: getReport().

#### `ConfigEnvironment envFromEnvironmentVariable()`
- Source: `include/config/config_path_resolver.h`:307
- Brief: Reads and validates THEMIS_CONFIG_ENV at initialisation time.
- Parameters: none
- Return: Return value.
- Details: Env From Environment Variable. Return value. Return value. Calls: std::getenv(), val(), spdlog::warn(), std::transform(), begin(), end(), std::tolower().

#### `std::string envToString(ConfigEnvironment env)`
- Source: `include/config/config_path_resolver.h`:301
- Brief: Converts a ConfigEnvironment to its lowercase string name.
- Parameters:
  - `env` (ConfigEnvironment): Input parameter.
- Return: Return value.
- Details: Env To String. env Input parameter. Return value. env Input parameter. Return value. Implements envToString without additional internal calls.

#### `ConfigEnvironment getEnvironment()`
- Source: `include/config/config_path_resolver.h`:140
- Brief: Get Environment.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: load().

#### `double getLegacyFallbackRateThreshold()`
- Source: `include/config/config_path_resolver.h`:196
- Brief: Get Legacy Fallback Rate Threshold.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: load().

#### `std::optional< PathMappingMetadata > getMetadata(const std::string &legacy_path)`
- Source: `include/config/config_path_resolver.h`:77
- Brief: Get Metadata.
- Parameters:
  - `legacy_path` (const std::string &): Path to the legacy.
- Return: Return value.
- Details: legacy_path Path to the legacy. Return value. legacy_path Path to the legacy. Return value. Calls: normalizePath(), find(), end(), inferCategory().

#### `void handleSighup(int sig)`
- Source: `include/config/config_path_resolver.h`:289
- Brief: Handle Sighup.
- Parameters:
  - `sig` (int): Input parameter.
- Details: Static signal handler – must be async-signal-safe; only sets a flag. sig Input parameter. int Input parameter. Implements handleSighup without additional internal calls.

#### `std::string inferCategory(const std::string &new_path)`
- Source: `include/config/config_path_resolver.h`:273
- Brief: Get category from new path.
- Parameters:
  - `new_path` (const std::string &): Path to the new.
- Return: Return value.
- Details: Infer Category. new_path Path to the new. Return value. new_path Path to the new. Return value. Calls: normalizePath(), starts_with(), substr(), size(), find(), empty().

#### `void initLegacyFallbackCategoryCounters()`
- Source: `include/config/config_path_resolver.h`:252
- Brief: Init Legacy Fallback Category Counters.
- Parameters: none
- Details: Calls: std::call_once(), clear(), emplace(), inferCategory(), try_emplace().

#### `bool isLegacyPath(const std::string &path)`
- Source: `include/config/config_path_resolver.h`:70
- Brief: Is Legacy Path.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: path Input parameter. True when the operation succeeds. path Input parameter. True when the operation succeeds. Calls: normalizePath(), find(), end().

#### `std::vector< std::string > legacyFallbackCategories()`
- Source: `include/config/config_path_resolver.h`:104
- Brief: Legacy Fallback Categories.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: initLegacyFallbackCategoryCounters(), reserve(), size(), push_back().

#### `std::vector< std::pair< std::string, uint64_t > > legacyFallbacksByCategory()`
- Source: `include/config/config_path_resolver.h`:98
- Brief: n/a
- Parameters: none

#### `const std::map< std::string, std::string > & legacyPathMappings()`
- Source: `include/config/config_path_resolver.h`:79
- Brief: n/a
- Parameters: none

#### `std::string mapLegacyToNew(const std::string &legacy_path)`
- Source: `include/config/config_path_resolver.h`:63
- Brief: Map Legacy To New.
- Parameters:
  - `legacy_path` (const std::string &): Path to the legacy.
- Return: Return value.
- Details: legacy_path Path to the legacy. Return value. legacy_path Path to the legacy. Return value. Calls: normalizePath(), find(), end().

#### `const Metrics & metrics()`
- Source: `include/config/config_path_resolver.h`:96
- Brief: Metrics.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements metrics without additional internal calls.

#### `std::string normalizePath(const std::string &path)`
- Source: `include/config/config_path_resolver.h`:259
- Brief: Helper to normalize path separators.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: ═══════════════════════════════════════════════════════════ Private Helper Methods ═══════════════════════════════════════════════════════════ path Input parameter. Return value. path Input parameter. Return value. Calls: std::replace(), begin(), end(), starts_with(), substr(), ends_with(), length(), pop_back().

#### `void registerSighupHandler()`
- Source: `include/config/config_path_resolver.h`:145
- Brief: Register Sighup Handler.
- Parameters: none
- Details: Calls: defined(), spdlog::debug(), sigemptyset(), sigaction(), spdlog::warn(), spdlog::info().

#### `void resetMetrics()`
- Source: `include/config/config_path_resolver.h`:109
- Brief: Reset Metrics.
- Parameters: none
- Details: Calls: reset(), store().

#### `std::string resolve(const std::string &legacy_path)`
- Source: `include/config/config_path_resolver.h`:49
- Brief: Resolve.
- Parameters:
  - `legacy_path` (const std::string &): Path to the legacy.
- Return: Return value.
- Throws:
  - ConfigNotFoundException: if an error occurs.
- Details: ═══════════════════════════════════════════════════════════ Public API Implementation ═══════════════════════════════════════════════════════════ legacy_path Path to the legacy. Return value. legacy_path Path to the legacy. Return value. ConfigNotFoundException if an error occurs. Calls: tryResolve(), normalizePath(), mapLegacyToNew(), load(), empty(), starts_with(), substr(), size().

#### `void setAggregationEnabled(bool enabled, int interval_seconds=300)`
- Source: `include/config/config_path_resolver.h`:184
- Brief: Set Aggregation Enabled.
- Parameters:
  - `enabled` (bool): Input parameter.
  - `interval_seconds` (int): Input parameter.
- Details: enabled Input parameter. interval_seconds Input parameter. Calls: store(), start(), stop().

#### `void setAuditLogEnabled(bool enabled)`
- Source: `include/config/config_path_resolver.h`:209
- Brief: ── Audit log API ────────────────────────────────────────────────────
- Parameters:
  - `enabled` (bool): Input parameter.
- Details: Set Audit Log Enabled. enabled Input parameter. enabled Input parameter. Calls: enable(), disable().

#### `void setAuditLogMaxEntries(std::size_t max)`
- Source: `include/config/config_path_resolver.h`:226
- Brief: Set Audit Log Max Entries.
- Parameters:
  - `max` (std::size_t): Input parameter.
- Details: max Input parameter. max Input parameter. Calls: setMaxEntries().

#### `void setCachingEnabled(bool enabled)`
- Source: `include/config/config_path_resolver.h`:115
- Brief: Set Caching Enabled.
- Parameters:
  - `enabled` (bool): Input parameter.
- Details: enabled Input parameter. enabled Input parameter. Calls: store(), clear().

#### `void setEnvironment(ConfigEnvironment env)`
- Source: `include/config/config_path_resolver.h`:134
- Brief: Set Environment.
- Parameters:
  - `env` (ConfigEnvironment): Input parameter.
- Details: env Input parameter. env Input parameter. Calls: store(), clear(), spdlog::info(), envToString().

#### `void setLegacyFallbackRateThreshold(double threshold)`
- Source: `include/config/config_path_resolver.h`:190
- Brief: Set Legacy Fallback Rate Threshold.
- Parameters:
  - `threshold` (double): Input parameter.
- Details: threshold Input parameter. threshold Input parameter. Calls: store().

#### `bool startHotReload(const std::string &watch_dir="config", std::chrono::milliseconds debounce=std::chrono::milliseconds(200))`
- Source: `include/config/config_path_resolver.h`:147
- Brief: ── inotify/kqueue/ReadDirectoryChangesW hot-reload ─────────────────────────
- Parameters:
  - `watch_dir` (const std::string &): Input parameter.
  - `debounce` (std::chrono::milliseconds): Input parameter.
- Return: True when the operation succeeds.
- Details: watch_dir Input parameter. debounce Input parameter. True when the operation succeeds. Calls: isRunning(), spdlog::debug(), watchPath(), clear(), spdlog::info(), start(), reset(), spdlog::warn().

#### `void stopHotReload()`
- Source: `include/config/config_path_resolver.h`:154
- Brief: Stop Hot Reload.
- Parameters: none
- Details: Calls: stop(), reset().

#### `std::optional< std::string > tryResolve(const std::string &legacy_path)`
- Source: `include/config/config_path_resolver.h`:56
- Brief: Try Resolve.
- Parameters:
  - `legacy_path` (const std::string &): Path to the legacy.
- Return: Return value.
- Details: legacy_path Path to the legacy. Return value. legacy_path Path to the legacy. Return value. Calls: normalizePath(), validatePath(), load(), envToString(), clear(), spdlog::info(), get(), isLegacyPath().

#### `void validatePath(const std::string &path)`
- Source: `include/config/config_path_resolver.h`:266
- Brief: Validate Path.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Throws:
  - InvalidPathException: if an error occurs.
- Details: path Input parameter. path Input parameter. InvalidPathException if an error occurs. Calls: find(), fs_path(), is_absolute(), std::replace(), begin(), end(), size(), compare().

### themis::config::ConfigPathResolver::DeprecationAggregator

#### `std::vector< ConfigPathResolver::DeprecationEntry > getReport() const`
- Source: `src/config/config_path_resolver.cpp`:55
- Brief: n/a
- Parameters: none

#### `void incrementUsage(const std::string &legacy_path)`
- Source: `src/config/config_path_resolver.cpp`:50
- Brief: Increment Usage.
- Parameters:
  - `legacy_path` (const std::string &): Path to the legacy.
- Details: legacy_path Path to the legacy. Calls: lock().

#### `bool isRunning() const`
- Source: `src/config/config_path_resolver.cpp`:132
- Brief: n/a
- Parameters: none

#### `void logReport()`
- Source: `src/config/config_path_resolver.cpp`:153
- Brief: Log Report.
- Parameters: none
- Details: Calls: getReport(), empty(), spdlog::info(), size(), has_value(), year(), month(), day().

#### `void reporterLoop()`
- Source: `src/config/config_path_resolver.cpp`:139
- Brief: Reporter Loop.
- Parameters: none
- Details: Calls: load(), lock(), wait_for(), logReport().

#### `void reset()`
- Source: `src/config/config_path_resolver.cpp`:95
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Calls: lock(), clear().

#### `void start(int interval_seconds=DEFAULT_INTERVAL_SECONDS)`
- Source: `src/config/config_path_resolver.cpp`:100
- Brief: n/a
- Parameters:
  - `interval_seconds` (int): n/a

#### `void stop()`
- Source: `src/config/config_path_resolver.cpp`:119
- Brief: Stop.
- Parameters: none
- Details: Calls: lock(), notify_all(), tlock(), joinable(), join().

#### `~DeprecationAggregator()`
- Source: `src/config/config_path_resolver.cpp`:41
- Brief: n/a
- Parameters: none

### themis::config::ConfigPermissionException

#### `ConfigPermissionException(const std::string &path)`
- Source: `include/config/config_errors.h`:104
- Brief: Config Permission Exception.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: path Input parameter. Return value.

#### `const std::string & config_path() const`
- Source: `include/config/config_errors.h`:108
- Brief: n/a
- Parameters: none

### themis::config::ConfigSchemaValidator

#### `nlohmann::json loadAsJson(const std::string &content, bool is_yaml)`
- Source: `include/config/config_schema_validator.h`:94
- Brief: Load As Json.
- Parameters:
  - `content` (const std::string &): Input parameter.
  - `is_yaml` (bool): Input parameter.
- Return: Return value.
- Throws:
  - SchemaValidationException: if an error occurs.
- Details: content Input parameter. is_yaml Input parameter. Return value. content Input parameter. is_yaml Input parameter. Return value. SchemaValidationException if an error occurs. Calls: YAML::Load(), yamlNodeToJsonImpl(), std::string(), what(), nlohmann::json::parse().

#### `nlohmann::json loadAsJson(const std::string &file_path)`
- Source: `include/config/config_schema_validator.h`:86
- Brief: Load As Json.
- Parameters:
  - `file_path` (const std::string &): Path to the file.
- Return: Return value.
- Throws:
  - SchemaValidationException: if an error occurs.
- Details: ═══════════════════════════════════════════════════════════ loadAsJson ═══════════════════════════════════════════════════════════ file_path Path to the file. Return value. file_path Path to the file. Return value. SchemaValidationException if an error occurs. Calls: size(), substr(), std::tolower(), YAML::LoadFile(), yamlNodeToJsonImpl(), std::string(), what(), ifs().

#### `bool matchesType(const nlohmann::json &value, const std::string &type)`
- Source: `include/config/config_schema_validator.h`:286
- Brief: Check whether a JSON value matches the given JSON Schema type string.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
  - `type` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: ═══════════════════════════════════════════════════════════ matchesType ═══════════════════════════════════════════════════════════ value Input parameter. type Input parameter. True when the operation succeeds. value Input parameter. type Input parameter. True when the operation succeeds. Calls: is_null(), is_boolean(), is_number_integer(), is_number(), is_string(), is_array(), is_object().

#### `const nlohmann::json * resolveRef(const std::string &ref, const nlohmann::json &root_schema)`
- Source: `include/config/config_schema_validator.h`:145
- Brief: Resolve a local $ref string (e.
- Parameters:
  - `ref` (const std::string &): Input parameter.
  - `root_schema` (const nlohmann::json &): Input parameter.
- Return: Pointer to the result.
- Details: ref Input parameter. root_schema Input parameter. Pointer to the result. g. "#/$defs/Foo" or "#/definitions/Bar") against root_schema using a JSON Pointer walk (RFC 6901). Returns a pointer into root_schema, or nullptr on failure. Only document-internal refs starting with '#' are supported.

#### `ValidationResult validate(const std::string &config_path, const nlohmann::json &schema)`
- Source: `include/config/config_schema_validator.h`:69
- Brief: Validate.
- Parameters:
  - `config_path` (const std::string &): Path to the retention policy configuration file.
  - `schema` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: ═══════════════════════════════════════════════════════════ validate ═══════════════════════════════════════════════════════════ config_path Path to the retention policy configuration file. schema Input parameter. Return value. config_path Path to the retention policy configuration file. schema Input parameter. Return value. Calls: loadAsJson(), addError(), what(), std::string(), validateValue().

#### `void validateAllOf(const nlohmann::json &value, const nlohmann::json &schemas, const std::string &json_path, ValidationResult &result, const nlohmann::json &root_schema, std::vector< std::string > &visited_refs)`
- Source: `include/config/config_schema_validator.h`:225
- Brief: Validate All Of.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
  - `schemas` (const nlohmann::json &): Input parameter.
  - `json_path` (const std::string &): Path to the json.
  - `result` (ValidationResult &): Input/output parameter.
  - `root_schema` (const nlohmann::json &): Input parameter.
  - `visited_refs` (std::vector< std::string > &): Input/output parameter.
- Details: ═══════════════════════════════════════════════════════════ validateAllOf ═══════════════════════════════════════════════════════════ value Input parameter. schemas Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. value Input parameter. schemas Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. Calls: is_object(), validateValueImpl().

#### `void validateAnyOf(const nlohmann::json &value, const nlohmann::json &schemas, const std::string &json_path, ValidationResult &result, const nlohmann::json &root_schema, std::vector< std::string > &visited_refs)`
- Source: `include/config/config_schema_validator.h`:241
- Brief: Validate Any Of.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
  - `schemas` (const nlohmann::json &): Input parameter.
  - `json_path` (const std::string &): Path to the json.
  - `result` (ValidationResult &): Input/output parameter.
  - `root_schema` (const nlohmann::json &): Input parameter.
  - `visited_refs` (std::vector< std::string > &): Input/output parameter.
- Details: ═══════════════════════════════════════════════════════════ validateAnyOf ═══════════════════════════════════════════════════════════ value Input parameter. schemas Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. value Input parameter. schemas Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. Calls: is_object(), validateValueImpl(), addError().

#### `void validateArray(const nlohmann::json &value, const nlohmann::json &schema, const std::string &json_path, ValidationResult &result, const nlohmann::json &root_schema, std::vector< std::string > &visited_refs)`
- Source: `include/config/config_schema_validator.h`:185
- Brief: Validate Array.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
  - `schema` (const nlohmann::json &): Input parameter.
  - `json_path` (const std::string &): Path to the json.
  - `result` (ValidationResult &): Input/output parameter.
  - `root_schema` (const nlohmann::json &): Input parameter.
  - `visited_refs` (std::vector< std::string > &): Input/output parameter.
- Details: ═══════════════════════════════════════════════════════════ validateArray ═══════════════════════════════════════════════════════════ value Input parameter. schema Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. value Input parameter. schema Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. Calls: contains(), is_number_integer(), size(), addError(), std::to_string(), is_object(), validateValueImpl(), is_boolean().

#### `ValidationResult validateFromString(const std::string &content, bool is_yaml, const nlohmann::json &schema)`
- Source: `include/config/config_schema_validator.h`:103
- Brief: Validate From String.
- Parameters:
  - `content` (const std::string &): Input parameter.
  - `is_yaml` (bool): Input parameter.
  - `schema` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: content Input parameter. is_yaml Input parameter. schema Input parameter. Return value.

#### `void validateNot(const nlohmann::json &value, const nlohmann::json &not_schema, const std::string &json_path, ValidationResult &result, const nlohmann::json &root_schema, std::vector< std::string > &visited_refs)`
- Source: `include/config/config_schema_validator.h`:273
- Brief: Validate Not.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
  - `not_schema` (const nlohmann::json &): Input parameter.
  - `json_path` (const std::string &): Path to the json.
  - `result` (ValidationResult &): Input/output parameter.
  - `root_schema` (const nlohmann::json &): Input parameter.
  - `visited_refs` (std::vector< std::string > &): Input/output parameter.
- Details: ═══════════════════════════════════════════════════════════ validateNot ═══════════════════════════════════════════════════════════ value Input parameter. not_schema Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. value Input parameter. not_schema Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. Calls: validateValueImpl(), addError().

#### `void validateNumber(const nlohmann::json &value, const nlohmann::json &schema, const std::string &json_path, ValidationResult &result)`
- Source: `include/config/config_schema_validator.h`:211
- Brief: Validate Number.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
  - `schema` (const nlohmann::json &): Input parameter.
  - `json_path` (const std::string &): Path to the json.
  - `result` (ValidationResult &): Input/output parameter.
- Details: ═══════════════════════════════════════════════════════════ validateNumber ═══════════════════════════════════════════════════════════ value Input parameter. schema Input parameter. json_path Path to the json. result Input/output parameter. value Input parameter. schema Input parameter. json_path Path to the json. result Input/output parameter. Calls: contains(), is_number(), addError(), std::to_string().

#### `void validateObject(const nlohmann::json &value, const nlohmann::json &schema, const std::string &json_path, ValidationResult &result, const nlohmann::json &root_schema, std::vector< std::string > &visited_refs)`
- Source: `include/config/config_schema_validator.h`:169
- Brief: Validate Object.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
  - `schema` (const nlohmann::json &): Input parameter.
  - `json_path` (const std::string &): Path to the json.
  - `result` (ValidationResult &): Input/output parameter.
  - `root_schema` (const nlohmann::json &): Input parameter.
  - `visited_refs` (std::vector< std::string > &): Input/output parameter.
- Details: ═══════════════════════════════════════════════════════════ validateObject ═══════════════════════════════════════════════════════════ value Input parameter. schema Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. value Input parameter. schema Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. Calls: contains(), is_array(), is_string(), addError(), is_object(), items(), validateValueImpl(), push_back().

#### `void validateOneOf(const nlohmann::json &value, const nlohmann::json &schemas, const std::string &json_path, ValidationResult &result, const nlohmann::json &root_schema, std::vector< std::string > &visited_refs)`
- Source: `include/config/config_schema_validator.h`:257
- Brief: Validate One Of.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
  - `schemas` (const nlohmann::json &): Input parameter.
  - `json_path` (const std::string &): Path to the json.
  - `result` (ValidationResult &): Input/output parameter.
  - `root_schema` (const nlohmann::json &): Input parameter.
  - `visited_refs` (std::vector< std::string > &): Input/output parameter.
- Details: ═══════════════════════════════════════════════════════════ validateOneOf ═══════════════════════════════════════════════════════════ value Input parameter. schemas Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. value Input parameter. schemas Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. Calls: is_object(), validateValueImpl(), addError(), std::to_string().

#### `void validateString(const nlohmann::json &value, const nlohmann::json &schema, const std::string &json_path, ValidationResult &result)`
- Source: `include/config/config_schema_validator.h`:199
- Brief: Validate String.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
  - `schema` (const nlohmann::json &): Input parameter.
  - `json_path` (const std::string &): Path to the json.
  - `result` (ValidationResult &): Input/output parameter.
- Details: ═══════════════════════════════════════════════════════════ validateString ═══════════════════════════════════════════════════════════ value Input parameter. schema Input parameter. json_path Path to the json. result Input/output parameter. value Input parameter. schema Input parameter. json_path Path to the json. result Input/output parameter. Calls: contains(), is_number_integer(), size(), addError(), std::to_string(), is_string(), re(), std::regex_search().

#### `void validateType(const nlohmann::json &value, const std::string &expected_type, const std::string &json_path, ValidationResult &result)`
- Source: `include/config/config_schema_validator.h`:155
- Brief: Validate Type.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
  - `expected_type` (const std::string &): Input parameter.
  - `json_path` (const std::string &): Path to the json.
  - `result` (ValidationResult &): Input/output parameter.
- Details: ═══════════════════════════════════════════════════════════ validateType ═══════════════════════════════════════════════════════════ value Input parameter. expected_type Input parameter. json_path Path to the json. result Input/output parameter. value Input parameter. expected_type Input parameter. json_path Path to the json. result Input/output parameter. Calls: matchesType(), addError(), std::string(), type_name().

#### `void validateValue(const nlohmann::json &value, const nlohmann::json &schema, const std::string &json_path, ValidationResult &result)`
- Source: `include/config/config_schema_validator.h`:116
- Brief: Entry-point wrapper: uses schema itself as the root schema and an empty visited-refs set.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
  - `schema` (const nlohmann::json &): Input parameter.
  - `json_path` (const std::string &): Path to the json.
  - `result` (ValidationResult &): Input/output parameter.
- Details: ═══════════════════════════════════════════════════════════ validateValue (entry-point wrapper) ═══════════════════════════════════════════════════════════ value Input parameter. schema Input parameter. json_path Path to the json. result Input/output parameter. Called by validate() and validateWithSchemaFile(). value Input parameter. schema Input parameter. json_path Path to the json. result Input/output parameter. Calls: validateValueImpl().

#### `void validateValueImpl(const nlohmann::json &value, const nlohmann::json &schema, const std::string &json_path, ValidationResult &result, const nlohmann::json &root_schema, std::vector< std::string > &visited_refs)`
- Source: `include/config/config_schema_validator.h`:131
- Brief: Internal recursive implementation.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
  - `schema` (const nlohmann::json &): Input parameter.
  - `json_path` (const std::string &): Path to the json.
  - `result` (ValidationResult &): Input/output parameter.
  - `root_schema` (const nlohmann::json &): Input parameter.
  - `visited_refs` (std::vector< std::string > &): Input/output parameter.
- Details: ═══════════════════════════════════════════════════════════ validateValueImpl (dispatcher, carries root schema and visited-refs) ═══════════════════════════════════════════════════════════ value Input parameter. schema Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. root_schema — top-level schema object used for $ref/$defs resolution. visited_refs — current $ref resolution chain for cycle detection. value Input parameter. schema Input parameter. json_path Path to the json. result Input/output parameter. root_schema Input parameter. visited_refs Input/output parameter. Calls: is_object(), contains(), is_string(), addError(), empty(), resolveRef(), push_back(), pop_back().

#### `ValidationResult validateWithSchemaFile(const std::string &config_path, const std::string &schema_path)`
- Source: `include/config/config_schema_validator.h`:78
- Brief: Validate With Schema File.
- Parameters:
  - `config_path` (const std::string &): Path to the retention policy configuration file.
  - `schema_path` (const std::string &): Path to the schema.
- Return: Return value.
- Details: ═══════════════════════════════════════════════════════════ validateWithSchemaFile ═══════════════════════════════════════════════════════════ config_path Path to the retention policy configuration file. schema_path Path to the schema. Return value. config_path Path to the retention policy configuration file. schema_path Path to the schema. Return value. Calls: ConfigPathResolver::tryResolve(), has_value(), loadAsJson(), addError(), std::string(), what(), validateValue().

### themis::config::ConfigSchemaValidator::ValidationResult

#### `void addError(const std::string &error)`
- Source: `include/config/config_schema_validator.h`:37
- Brief: Add Error.
- Parameters:
  - `error` (const std::string &): Input parameter.
- Details: error Input parameter. Calls: push_back().

#### `void addWarning(const std::string &warning)`
- Source: `include/config/config_schema_validator.h`:47
- Brief: Add Warning.
- Parameters:
  - `warning` (const std::string &): Input parameter.
- Details: warning Input parameter. Calls: push_back().

#### `std::string formatErrors() const`
- Source: `include/config/config_schema_validator.h`:51
- Brief: n/a
- Parameters: none

### themis::config::InvalidPathException

#### `InvalidPathException(const std::string &path, const std::string &reason)`
- Source: `include/config/config_errors.h`:84
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a
  - `reason` (const std::string &): n/a

#### `const std::string & invalid_path() const`
- Source: `include/config/config_errors.h`:89
- Brief: n/a
- Parameters: none

#### `const std::string & reason() const`
- Source: `include/config/config_errors.h`:90
- Brief: n/a
- Parameters: none

### themis::config::LRUCacheWithTTL

#### `LRUCacheWithTTL(size_t capacity=1000, int default_ttl_seconds=300)`
- Source: `include/config/lru_cache.h`:29
- Brief: n/a
- Parameters:
  - `capacity` (size_t): n/a
  - `default_ttl_seconds` (int): n/a

#### `void clear()`
- Source: `include/config/lru_cache.h`:123
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `bool empty() const`
- Source: `include/config/lru_cache.h`:139
- Brief: n/a
- Parameters: none

#### `void evictLRU()`
- Source: `include/config/lru_cache.h`:220
- Brief: Evict LRU.
- Parameters: none
- Details: Calls: empty(), back(), erase(), pop_back().

#### `std::optional< Value > get(const Key &key)`
- Source: `include/config/lru_cache.h`:73
- Brief: Get.
- Parameters:
  - `key` (const Key &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value. Calls: lock(), find(), end(), std::chrono::steady_clock::now(), erase(), splice(), begin().

#### `bool invalidate(const Key &key)`
- Source: `include/config/lru_cache.h`:106
- Brief: Invalidate.
- Parameters:
  - `key` (const Key &): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), erase().

#### `void put(const Key &key, const Value &value, std::optional< int > ttl_seconds=std::nullopt)`
- Source: `include/config/lru_cache.h`:34
- Brief: n/a
- Parameters:
  - `key` (const Key &): n/a
  - `value` (const Value &): n/a
  - `ttl_seconds` (std::optional< int >): n/a

#### `void removeExpired()`
- Source: `include/config/lru_cache.h`:184
- Brief: Remove Expired.
- Parameters: none
- Details: Calls: lock(), std::chrono::steady_clock::now(), rbegin(), rend(), erase(), std::next(), base().

#### `size_t size() const`
- Source: `include/config/lru_cache.h`:129
- Brief: n/a
- Parameters: none

#### `Stats stats() const`
- Source: `include/config/lru_cache.h`:159
- Brief: n/a
- Parameters: none

### themis::config::MappingNotFoundException

#### `MappingNotFoundException(const std::string &path)`
- Source: `include/config/config_errors.h`:72
- Brief: Mapping Not Found Exception.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: path Input parameter. Return value.

#### `const std::string & legacy_path() const`
- Source: `include/config/config_errors.h`:76
- Brief: n/a
- Parameters: none

### themis::config::PathMappingMetadata

#### `int daysUntilRemoval() const`
- Source: `include/config/path_mapping_metadata.h`:57
- Brief: n/a
- Parameters: none
- Details: Get days until removal (negative if already past removal date).

#### `std::string getDeprecationMessage() const`
- Source: `include/config/path_mapping_metadata.h`:70
- Brief: n/a
- Parameters: none
- Details: Get formatted deprecation warning message.

#### `bool isDeprecated() const`
- Source: `include/config/path_mapping_metadata.h`:37
- Brief: n/a
- Parameters: none
- Details: Check if this mapping is currently deprecated.

#### `bool isRemovalDue() const`
- Source: `include/config/path_mapping_metadata.h`:47
- Brief: n/a
- Parameters: none
- Details: Check if removal deadline has passed.

### themis::config::SchemaValidationException

#### `SchemaValidationException(const std::string &file_path, const std::string &reason)`
- Source: `include/config/config_errors.h`:116
- Brief: n/a
- Parameters:
  - `file_path` (const std::string &): n/a
  - `reason` (const std::string &): n/a

#### `const std::string & file_path() const`
- Source: `include/config/config_errors.h`:121
- Brief: n/a
- Parameters: none

#### `const std::string & reason() const`
- Source: `include/config/config_errors.h`:122
- Brief: n/a
- Parameters: none

### themis::config::bench

#### `BENCHMARK(BM_MapLegacyToNew) -> Unit(benchmark::kNanosecond) ->MinTime(0.5)`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MapLegacyToNew): n/a

#### `BENCHMARK(BM_MapLegacyToNew_Miss) -> Unit(benchmark::kNanosecond) ->MinTime(0.5)`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MapLegacyToNew_Miss): n/a

#### `BENCHMARK(BM_ScanTree_10K) -> Unit(benchmark::kSecond) ->Iterations(1)`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ScanTree_10K): n/a

#### `BENCHMARK(BM_ScanTree_1K) -> Unit(benchmark::kMillisecond) ->Iterations(1)`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ScanTree_1K): n/a

#### `BENCHMARK(BM_ShouldScanFile) -> Unit(benchmark::kNanosecond) ->MinTime(0.5)`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ShouldScanFile): n/a

#### `BENCHMARK(BenchmarkGateCfg05WatcherPollInterval) -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/config/bench_config_release_gates.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (BenchmarkGateCfg05WatcherPollInterval): n/a

#### `BENCHMARK(BenchmarkGateCfg06MetricsOverhead) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/config/bench_config_release_gates.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (BenchmarkGateCfg06MetricsOverhead): n/a

#### `BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, CacheHitRate_BulkAccess)(benchmark`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverBenchFixture): n/a
  - `<unnamed>` (CacheHitRate_BulkAccess): n/a

#### `BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, CacheHit_MappedPath)(benchmark`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverBenchFixture): n/a
  - `<unnamed>` (CacheHit_MappedPath): n/a

#### `BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, CacheHit_UnmappedPath)(benchmark`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverBenchFixture): n/a
  - `<unnamed>` (CacheHit_UnmappedPath): n/a

#### `BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, CacheMiss_MappedPath)(benchmark`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverBenchFixture): n/a
  - `<unnamed>` (CacheMiss_MappedPath): n/a

#### `BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, CacheMiss_UnmappedPath)(benchmark`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverBenchFixture): n/a
  - `<unnamed>` (CacheMiss_UnmappedPath): n/a

#### `BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, ConcurrentCacheHit)(benchmark`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverBenchFixture): n/a
  - `<unnamed>` (ConcurrentCacheHit): n/a

#### `BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, DeprecationAggregator_Overhead)(benchmark`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverBenchFixture): n/a
  - `<unnamed>` (DeprecationAggregator_Overhead): n/a

#### `BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, DeprecationReport_60Paths)(benchmark`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverBenchFixture): n/a
  - `<unnamed>` (DeprecationReport_60Paths): n/a

#### `BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, LegacyFallback)(benchmark`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverBenchFixture): n/a
  - `<unnamed>` (LegacyFallback): n/a

#### `BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, MetricsScrape)(benchmark`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverBenchFixture): n/a
  - `<unnamed>` (MetricsScrape): n/a

#### `BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, Resolve_CacheHit)(benchmark`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:415
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverBenchFixture): n/a
  - `<unnamed>` (Resolve_CacheHit): n/a

#### `BENCHMARK_DEFINE_F(MigrationScannerBenchFixture, ScanSingleFile_NoMatch)(benchmark`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScannerBenchFixture): n/a
  - `<unnamed>` (ScanSingleFile_NoMatch): n/a

#### `BENCHMARK_DEFINE_F(MigrationScannerBenchFixture, ScanSingleFile_WithMatch)(benchmark`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScannerBenchFixture): n/a
  - `<unnamed>` (ScanSingleFile_WithMatch): n/a

#### `void BM_MapLegacyToNew(benchmark::State &state)`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:202
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MapLegacyToNew_Miss(benchmark::State &state)`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:217
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ScanTree_10K(benchmark::State &state)`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:217
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ScanTree_1K(benchmark::State &state)`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:186
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ShouldScanFile(benchmark::State &state)`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:126
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BenchmarkGateCfg05WatcherPollInterval(benchmark::State &state)`
- Source: `benchmarks/config/bench_config_release_gates.cpp`:195
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BenchmarkGateCfg06MetricsOverhead(benchmark::State &state)`
- Source: `benchmarks/config/bench_config_release_gates.cpp`:224
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `state SetLabel("target: p99 ≤ 1 ms")`
- Source: `benchmarks/config/bench_config_release_gates.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` ("target: p99 ≤ 1 ms"): n/a

#### `Threads(1) -> Threads(2) ->Threads(4) ->Threads(8) ->Unit(benchmark::kNanosecond) ->MinTime(0.5)`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:404
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Unit(benchmark::kMicrosecond) -> MinTime(0.5)`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMicrosecond): n/a

#### `Unit(benchmark::kNanosecond) -> MinTime(0.5)`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kNanosecond): n/a

#### `fs::path buildFileTree(int num_files)`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:91
- Brief: n/a
- Parameters:
  - `num_files` (int): n/a

#### `for(auto _ :state)`
- Source: `benchmarks/config/bench_config_release_gates.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (auto _ :state): n/a

#### `void writeFile(const fs::path &p, const char *content="key: value\n")`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:24
- Brief: n/a
- Parameters:
  - `p` (const fs::path &): n/a
  - `content` (const char *): n/a

#### `void writeFile(const fs::path &p, const std::string &content)`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:39
- Brief: n/a
- Parameters:
  - `p` (const fs::path &): n/a
  - `content` (const std::string &): n/a

### themis::config::bench::ConfigEncryptedStoreHotPathFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/config/bench_config_release_gates.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/config/bench_config_release_gates.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### themis::config::bench::ConfigPathResolverBenchFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/config/bench_config_path_resolver.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### themis::config::bench::ConfigResolveHotPathFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/config/bench_config_release_gates.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/config/bench_config_release_gates.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### themis::config::bench::ConfigValidateHotPathFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/config/bench_config_release_gates.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/config/bench_config_release_gates.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### themis::config::bench::MigrationScannerBenchFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/config/bench_config_migration_scanner.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### themis::config::test

#### `TEST(ConfigErrorsStatelessTest, ConfigNotFoundExceptionAccessors)`
- Source: `tests/config/test_config_coverage.cpp`:319
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigErrorsStatelessTest): n/a
  - `<unnamed>` (ConfigNotFoundExceptionAccessors): n/a

#### `TEST(ConfigErrorsStatelessTest, ConfigNotFoundExceptionMessage)`
- Source: `tests/config/test_config_coverage.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigErrorsStatelessTest): n/a
  - `<unnamed>` (ConfigNotFoundExceptionMessage): n/a

#### `TEST(ConfigErrorsStatelessTest, ConfigPermissionExceptionAccessor)`
- Source: `tests/config/test_config_coverage.cpp`:343
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigErrorsStatelessTest): n/a
  - `<unnamed>` (ConfigPermissionExceptionAccessor): n/a

#### `TEST(ConfigErrorsStatelessTest, ExceptionHierarchy)`
- Source: `tests/config/test_config_coverage.cpp`:350
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigErrorsStatelessTest): n/a
  - `<unnamed>` (ExceptionHierarchy): n/a

#### `TEST(ConfigErrorsStatelessTest, InvalidPathExceptionAccessors)`
- Source: `tests/config/test_config_coverage.cpp`:335
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigErrorsStatelessTest): n/a
  - `<unnamed>` (InvalidPathExceptionAccessors): n/a

#### `TEST(ConfigErrorsStatelessTest, MappingNotFoundExceptionMessage)`
- Source: `tests/config/test_config_coverage.cpp`:328
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigErrorsStatelessTest): n/a
  - `<unnamed>` (MappingNotFoundExceptionMessage): n/a

#### `TEST(PathMappingMetadataNoFixtureTest, IsDeprecatedWhenDateInPast)`
- Source: `tests/config/test_config_coverage.cpp`:365
- Brief: n/a
- Parameters:
  - `<unnamed>` (PathMappingMetadataNoFixtureTest): n/a
  - `<unnamed>` (IsDeprecatedWhenDateInPast): n/a

#### `TEST(PathMappingMetadataNoFixtureTest, IsRemovalDueWhenDateInPast)`
- Source: `tests/config/test_config_coverage.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (PathMappingMetadataNoFixtureTest): n/a
  - `<unnamed>` (IsRemovalDueWhenDateInPast): n/a

#### `TEST(PathMappingMetadataNoFixtureTest, NotDeprecatedWhenNoDate)`
- Source: `tests/config/test_config_coverage.cpp`:371
- Brief: n/a
- Parameters:
  - `<unnamed>` (PathMappingMetadataNoFixtureTest): n/a
  - `<unnamed>` (NotDeprecatedWhenNoDate): n/a

#### `TEST(PathMappingMetadataNoFixtureTest, NotRemovalDueWhenDateInFuture)`
- Source: `tests/config/test_config_coverage.cpp`:387
- Brief: n/a
- Parameters:
  - `<unnamed>` (PathMappingMetadataNoFixtureTest): n/a
  - `<unnamed>` (NotRemovalDueWhenDateInFuture): n/a

#### `TEST(PathMappingMetadataNoFixtureTest, NotRemovalDueWhenNoDate)`
- Source: `tests/config/test_config_coverage.cpp`:382
- Brief: n/a
- Parameters:
  - `<unnamed>` (PathMappingMetadataNoFixtureTest): n/a
  - `<unnamed>` (NotRemovalDueWhenNoDate): n/a

#### `TEST_F(AuditLogDirectTest, BoundedRingBufferEvictsOldest)`
- Source: `tests/config/test_config_coverage.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLogDirectTest): n/a
  - `<unnamed>` (BoundedRingBufferEvictsOldest): n/a

#### `TEST_F(AuditLogDirectTest, ClearEmptiesLog)`
- Source: `tests/config/test_config_coverage.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLogDirectTest): n/a
  - `<unnamed>` (ClearEmptiesLog): n/a

#### `TEST_F(AuditLogDirectTest, DefaultMaxEntriesIsLarge)`
- Source: `tests/config/test_config_coverage.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLogDirectTest): n/a
  - `<unnamed>` (DefaultMaxEntriesIsLarge): n/a

#### `TEST_F(AuditLogDirectTest, DisabledByDefault)`
- Source: `tests/config/test_config_coverage.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLogDirectTest): n/a
  - `<unnamed>` (DisabledByDefault): n/a

#### `TEST_F(AuditLogDirectTest, EnableDisable)`
- Source: `tests/config/test_config_coverage.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLogDirectTest): n/a
  - `<unnamed>` (EnableDisable): n/a

#### `TEST_F(AuditLogDirectTest, RecordWhenDisabledIsNoOp)`
- Source: `tests/config/test_config_coverage.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLogDirectTest): n/a
  - `<unnamed>` (RecordWhenDisabledIsNoOp): n/a

#### `TEST_F(AuditLogDirectTest, RecordWhenEnabledStoresEntry)`
- Source: `tests/config/test_config_coverage.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLogDirectTest): n/a
  - `<unnamed>` (RecordWhenEnabledStoresEntry): n/a

#### `TEST_F(AuditLogDirectTest, SetMaxEntriesChangesLimit)`
- Source: `tests/config/test_config_coverage.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLogDirectTest): n/a
  - `<unnamed>` (SetMaxEntriesChangesLimit): n/a

#### `TEST_F(AuditLogDirectTest, SetMaxEntriesZeroClampsToOne)`
- Source: `tests/config/test_config_coverage.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLogDirectTest): n/a
  - `<unnamed>` (SetMaxEntriesZeroClampsToOne): n/a

#### `TEST_F(AuditLogDirectTest, ShrinkingMaxEvictsOldest)`
- Source: `tests/config/test_config_coverage.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLogDirectTest): n/a
  - `<unnamed>` (ShrinkingMaxEvictsOldest): n/a

#### `TEST_F(AuditLogDirectTest, SizeZeroInitially)`
- Source: `tests/config/test_config_coverage.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLogDirectTest): n/a
  - `<unnamed>` (SizeZeroInitially): n/a

#### `TEST_F(CacheEnvConfigTest, CacheCapacityMatchesEnvOrDefault)`
- Source: `tests/config/test_config_path_resolver.cpp`:1130
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheEnvConfigTest): n/a
  - `<unnamed>` (CacheCapacityMatchesEnvOrDefault): n/a

#### `TEST_F(CacheEnvConfigTest, CurrentCacheConfigMatchesCacheStats)`
- Source: `tests/config/test_config_path_resolver.cpp`:1163
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheEnvConfigTest): n/a
  - `<unnamed>` (CurrentCacheConfigMatchesCacheStats): n/a

#### `TEST_F(CacheEnvConfigTest, CurrentCacheConfigReturnsPositiveCapacityAndTtl)`
- Source: `tests/config/test_config_path_resolver.cpp`:1157
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheEnvConfigTest): n/a
  - `<unnamed>` (CurrentCacheConfigReturnsPositiveCapacityAndTtl): n/a

#### `TEST_F(CacheEnvConfigTest, CurrentCacheConfigUsesDefaultsWhenEnvVarsAbsent)`
- Source: `tests/config/test_config_path_resolver.cpp`:1170
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheEnvConfigTest): n/a
  - `<unnamed>` (CurrentCacheConfigUsesDefaultsWhenEnvVarsAbsent): n/a

#### `TEST_F(CacheEnvConfigTest, DefaultCacheSizeConstantIs1000)`
- Source: `tests/config/test_config_path_resolver.cpp`:1113
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheEnvConfigTest): n/a
  - `<unnamed>` (DefaultCacheSizeConstantIs1000): n/a

#### `TEST_F(CacheEnvConfigTest, DefaultCacheTtlConstantIs300)`
- Source: `tests/config/test_config_path_resolver.cpp`:1109
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheEnvConfigTest): n/a
  - `<unnamed>` (DefaultCacheTtlConstantIs300): n/a

#### `TEST_F(CacheEnvConfigTest, KCacheTtlSecondsIsPositive)`
- Source: `tests/config/test_config_path_resolver.cpp`:1117
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheEnvConfigTest): n/a
  - `<unnamed>` (KCacheTtlSecondsIsPositive): n/a

#### `TEST_F(CacheEnvConfigTest, KCacheTtlSecondsMatchesCacheStats)`
- Source: `tests/config/test_config_path_resolver.cpp`:1121
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheEnvConfigTest): n/a
  - `<unnamed>` (KCacheTtlSecondsMatchesCacheStats): n/a

#### `TEST_F(CacheEnvConfigTest, MetricsExporterReportsCapacityMatchingEnvOrDefault)`
- Source: `tests/config/test_config_path_resolver.cpp`:1148
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheEnvConfigTest): n/a
  - `<unnamed>` (MetricsExporterReportsCapacityMatchingEnvOrDefault): n/a

#### `TEST_F(CacheEnvConfigTest, MetricsExporterReportsTtlMatchingKCacheTtlSeconds)`
- Source: `tests/config/test_config_path_resolver.cpp`:1140
- Brief: n/a
- Parameters:
  - `<unnamed>` (CacheEnvConfigTest): n/a
  - `<unnamed>` (MetricsExporterReportsTtlMatchingKCacheTtlSeconds): n/a

#### `TEST_F(ConfigAuditLogTest, AuditLogBoundedByMaxEntries)`
- Source: `tests/config/test_config_path_resolver.cpp`:1319
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigAuditLogTest): n/a
  - `<unnamed>` (AuditLogBoundedByMaxEntries): n/a

#### `TEST_F(ConfigAuditLogTest, AuditLogClearWorks)`
- Source: `tests/config/test_config_path_resolver.cpp`:1305
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigAuditLogTest): n/a
  - `<unnamed>` (AuditLogClearWorks): n/a

#### `TEST_F(ConfigAuditLogTest, AuditLogDisabledByDefault_NoEntriesRecorded)`
- Source: `tests/config/test_config_path_resolver.cpp`:1217
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigAuditLogTest): n/a
  - `<unnamed>` (AuditLogDisabledByDefault_NoEntriesRecorded): n/a

#### `TEST_F(ConfigAuditLogTest, AuditLogEmptyByDefault)`
- Source: `tests/config/test_config_path_resolver.cpp`:1212
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigAuditLogTest): n/a
  - `<unnamed>` (AuditLogEmptyByDefault): n/a

#### `TEST_F(ConfigAuditLogTest, AuditLogMarksCacheHit)`
- Source: `tests/config/test_config_path_resolver.cpp`:1284
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigAuditLogTest): n/a
  - `<unnamed>` (AuditLogMarksCacheHit): n/a

#### `TEST_F(ConfigAuditLogTest, AuditLogMarksLegacyFallback)`
- Source: `tests/config/test_config_path_resolver.cpp`:1263
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigAuditLogTest): n/a
  - `<unnamed>` (AuditLogMarksLegacyFallback): n/a

#### `TEST_F(ConfigAuditLogTest, AuditLogNotRecordingOnFailedResolution)`
- Source: `tests/config/test_config_path_resolver.cpp`:1338
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigAuditLogTest): n/a
  - `<unnamed>` (AuditLogNotRecordingOnFailedResolution): n/a

#### `TEST_F(ConfigAuditLogTest, AuditLogRecordsEntryWhenEnabled)`
- Source: `tests/config/test_config_path_resolver.cpp`:1229
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigAuditLogTest): n/a
  - `<unnamed>` (AuditLogRecordsEntryWhenEnabled): n/a

#### `TEST_F(ConfigAuditLogTest, AuditLogRecordsTimestamp)`
- Source: `tests/config/test_config_path_resolver.cpp`:1246
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigAuditLogTest): n/a
  - `<unnamed>` (AuditLogRecordsTimestamp): n/a

#### `TEST_F(ConfigAuditLogTest, ConcurrentRecordAndSnapshot_ThreadSafety)`
- Source: `tests/config/test_config_path_resolver.cpp`:1394
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigAuditLogTest): n/a
  - `<unnamed>` (ConcurrentRecordAndSnapshot_ThreadSafety): n/a

#### `TEST_F(ConfigAuditLogTest, ResolveThrowingVariantRecordsAuditEntry)`
- Source: `tests/config/test_config_path_resolver.cpp`:1348
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigAuditLogTest): n/a
  - `<unnamed>` (ResolveThrowingVariantRecordsAuditEntry): n/a

#### `TEST_F(ConfigAuditLogTest, ShrinkingMaxEntriesEvictsOldestFirst)`
- Source: `tests/config/test_config_path_resolver.cpp`:1369
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigAuditLogTest): n/a
  - `<unnamed>` (ShrinkingMaxEntriesEvictsOldestFirst): n/a

#### `TEST_F(ConfigEncryptedStoreHardeningTest, CFG25_EncryptionAlgorithm)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:191
- Brief: CFG-25: Encrypted-store uses AES-256-GCM for all stored values.
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreHardeningTest): n/a
  - `<unnamed>` (CFG25_EncryptionAlgorithm): n/a

#### `TEST_F(ConfigEncryptedStoreHardeningTest, CFG26_GcmAuthTagLength)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:196
- Brief: CFG-26: Encrypted-store enforces GCM authentication tag length (16 bytes).
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreHardeningTest): n/a
  - `<unnamed>` (CFG26_GcmAuthTagLength): n/a

#### `TEST_F(ConfigEncryptedStoreHardeningTest, CFG27_GcmIvLength)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:201
- Brief: CFG-27: Encrypted-store enforces GCM IV length (12 bytes, 96 bits).
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreHardeningTest): n/a
  - `<unnamed>` (CFG27_GcmIvLength): n/a

#### `TEST_F(ConfigEncryptedStoreHardeningTest, CFG28_DecryptionErrorOnAuthFailure)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:206
- Brief: CFG-28: Encrypted-store returns CONFIG_DECRYPTION_ERROR on auth tag verification failure.
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreHardeningTest): n/a
  - `<unnamed>` (CFG28_DecryptionErrorOnAuthFailure): n/a

#### `TEST_F(ConfigEncryptedStoreHardeningTest, CFG29_NonBlockingKeyRotation)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:213
- Brief: CFG-29: Encrypted-store supports non-blocking key rotation.
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreHardeningTest): n/a
  - `<unnamed>` (CFG29_NonBlockingKeyRotation): n/a

#### `TEST_F(ConfigEncryptedStoreHardeningTest, CFG30_MetadataValidation)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:221
- Brief: CFG-30: Encrypted-store validates metadata before decryption (IV, nonce, length checks).
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreHardeningTest): n/a
  - `<unnamed>` (CFG30_MetadataValidation): n/a

#### `TEST_F(ConfigEncryptedStoreHardeningTest, CFG31_SecureKeyZeroing)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:228
- Brief: CFG-31: Encrypted-store secures old keys after rotation (zeroing after kMaxEncryptionKeyRotationHistorySize).
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreHardeningTest): n/a
  - `<unnamed>` (CFG31_SecureKeyZeroing): n/a

#### `TEST_F(ConfigEncryptedStoreHardeningTest, CFG32_AtomicValueReads)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:234
- Brief: CFG-32: Encrypted-store does not support partial reads (entire value or failure).
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreHardeningTest): n/a
  - `<unnamed>` (CFG32_AtomicValueReads): n/a

#### `TEST_F(ConfigEncryptedStoreTest, BinaryLikeValueRoundtrip)`
- Source: `tests/config/test_config_encrypted_store.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (BinaryLikeValueRoundtrip): n/a

#### `TEST_F(ConfigEncryptedStoreTest, ClearRemovesAllEntries)`
- Source: `tests/config/test_config_encrypted_store.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (ClearRemovesAllEntries): n/a

#### `TEST_F(ConfigEncryptedStoreTest, ConcurrentReadersDoNotBlockEachOther)`
- Source: `tests/config/test_config_encrypted_store.cpp`:365
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (ConcurrentReadersDoNotBlockEachOther): n/a

#### `TEST_F(ConfigEncryptedStoreTest, ConcurrentRotationIsThreadSafe)`
- Source: `tests/config/test_config_encrypted_store.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (ConcurrentRotationIsThreadSafe): n/a

#### `TEST_F(ConfigEncryptedStoreTest, ConcurrentSetGetIsThreadSafe)`
- Source: `tests/config/test_config_encrypted_store.cpp`:301
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (ConcurrentSetGetIsThreadSafe): n/a

#### `TEST_F(ConfigEncryptedStoreTest, ContainsReturnsFalseForMissingKey)`
- Source: `tests/config/test_config_encrypted_store.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (ContainsReturnsFalseForMissingKey): n/a

#### `TEST_F(ConfigEncryptedStoreTest, ContainsReturnsTrueAfterSet)`
- Source: `tests/config/test_config_encrypted_store.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (ContainsReturnsTrueAfterSet): n/a

#### `TEST_F(ConfigEncryptedStoreTest, DeserializeEmptyStoreSnapshot)`
- Source: `tests/config/test_config_encrypted_store.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (DeserializeEmptyStoreSnapshot): n/a

#### `TEST_F(ConfigEncryptedStoreTest, DeserializeMalformedJsonThrows)`
- Source: `tests/config/test_config_encrypted_store.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (DeserializeMalformedJsonThrows): n/a

#### `TEST_F(ConfigEncryptedStoreTest, DeserializeRestoresKeyVersion)`
- Source: `tests/config/test_config_encrypted_store.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (DeserializeRestoresKeyVersion): n/a

#### `TEST_F(ConfigEncryptedStoreTest, DeserializeTruncatedJsonThrows)`
- Source: `tests/config/test_config_encrypted_store.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (DeserializeTruncatedJsonThrows): n/a

#### `TEST_F(ConfigEncryptedStoreTest, EmptyValueRoundtrip)`
- Source: `tests/config/test_config_encrypted_store.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (EmptyValueRoundtrip): n/a

#### `TEST_F(ConfigEncryptedStoreTest, GetMissingKeyThrows)`
- Source: `tests/config/test_config_encrypted_store.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (GetMissingKeyThrows): n/a

#### `TEST_F(ConfigEncryptedStoreTest, InitialKeyVersionIsOne)`
- Source: `tests/config/test_config_encrypted_store.cpp`:36
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (InitialKeyVersionIsOne): n/a

#### `TEST_F(ConfigEncryptedStoreTest, KeysReturnsAllStoredKeys)`
- Source: `tests/config/test_config_encrypted_store.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (KeysReturnsAllStoredKeys): n/a

#### `TEST_F(ConfigEncryptedStoreTest, LargeValueRoundtrip)`
- Source: `tests/config/test_config_encrypted_store.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (LargeValueRoundtrip): n/a

#### `TEST_F(ConfigEncryptedStoreTest, MultipleRotationsPreserveValues)`
- Source: `tests/config/test_config_encrypted_store.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (MultipleRotationsPreserveValues): n/a

#### `TEST_F(ConfigEncryptedStoreTest, MultipleValuesRoundtrip)`
- Source: `tests/config/test_config_encrypted_store.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (MultipleValuesRoundtrip): n/a

#### `TEST_F(ConfigEncryptedStoreTest, NewStoreIsEmpty)`
- Source: `tests/config/test_config_encrypted_store.cpp`:31
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (NewStoreIsEmpty): n/a

#### `TEST_F(ConfigEncryptedStoreTest, NewValuesEncryptedWithLatestKeyAfterRotation)`
- Source: `tests/config/test_config_encrypted_store.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (NewValuesEncryptedWithLatestKeyAfterRotation): n/a

#### `TEST_F(ConfigEncryptedStoreTest, OverwriteExistingKey)`
- Source: `tests/config/test_config_encrypted_store.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (OverwriteExistingKey): n/a

#### `TEST_F(ConfigEncryptedStoreTest, RemoveReturnsFalseWhenKeyMissing)`
- Source: `tests/config/test_config_encrypted_store.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (RemoveReturnsFalseWhenKeyMissing): n/a

#### `TEST_F(ConfigEncryptedStoreTest, RemoveReturnsTrueWhenKeyExists)`
- Source: `tests/config/test_config_encrypted_store.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (RemoveReturnsTrueWhenKeyExists): n/a

#### `TEST_F(ConfigEncryptedStoreTest, RotateKeyIncrementsVersion)`
- Source: `tests/config/test_config_encrypted_store.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (RotateKeyIncrementsVersion): n/a

#### `TEST_F(ConfigEncryptedStoreTest, RotateKeyOnEmptyStoreSucceeds)`
- Source: `tests/config/test_config_encrypted_store.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (RotateKeyOnEmptyStoreSucceeds): n/a

#### `TEST_F(ConfigEncryptedStoreTest, SerializeDeserializeRoundtrip)`
- Source: `tests/config/test_config_encrypted_store.cpp`:233
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (SerializeDeserializeRoundtrip): n/a

#### `TEST_F(ConfigEncryptedStoreTest, SetAndGetRoundtrip)`
- Source: `tests/config/test_config_encrypted_store.cpp`:44
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (SetAndGetRoundtrip): n/a

#### `TEST_F(ConfigEncryptedStoreTest, SetEmptyKeyThrows)`
- Source: `tests/config/test_config_encrypted_store.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (SetEmptyKeyThrows): n/a

#### `TEST_F(ConfigEncryptedStoreTest, StoredValueIsNotPlaintext)`
- Source: `tests/config/test_config_encrypted_store.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (StoredValueIsNotPlaintext): n/a

#### `TEST_F(ConfigEncryptedStoreTest, TryGetReturnsNulloptWhenAbsent)`
- Source: `tests/config/test_config_encrypted_store.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (TryGetReturnsNulloptWhenAbsent): n/a

#### `TEST_F(ConfigEncryptedStoreTest, TryGetReturnsValueWhenPresent)`
- Source: `tests/config/test_config_encrypted_store.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (TryGetReturnsValueWhenPresent): n/a

#### `TEST_F(ConfigEncryptedStoreTest, TwoEncryptionsOfSamePlaintextDiffer)`
- Source: `tests/config/test_config_encrypted_store.cpp`:282
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (TwoEncryptionsOfSamePlaintextDiffer): n/a

#### `TEST_F(ConfigEncryptedStoreTest, UnicodeValueRoundtrip)`
- Source: `tests/config/test_config_encrypted_store.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (UnicodeValueRoundtrip): n/a

#### `TEST_F(ConfigEncryptedStoreTest, ValuesAccessibleAfterKeyRotation)`
- Source: `tests/config/test_config_encrypted_store.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEncryptedStoreTest): n/a
  - `<unnamed>` (ValuesAccessibleAfterKeyRotation): n/a

#### `TEST_F(ConfigEnvOverlayTest, CacheKeyIncludesEnvironment)`
- Source: `tests/config/test_config_path_resolver.cpp`:738
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (CacheKeyIncludesEnvironment): n/a

#### `TEST_F(ConfigEnvOverlayTest, DefaultEnvironmentIsProd)`
- Source: `tests/config/test_config_path_resolver.cpp`:655
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (DefaultEnvironmentIsProd): n/a

#### `TEST_F(ConfigEnvOverlayTest, DevFallsBackToLegacyPathWhenBothAbsent)`
- Source: `tests/config/test_config_path_resolver.cpp`:726
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (DevFallsBackToLegacyPathWhenBothAbsent): n/a

#### `TEST_F(ConfigEnvOverlayTest, DevFallsBackToNewPathWhenOverlayAbsent)`
- Source: `tests/config/test_config_path_resolver.cpp`:714
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (DevFallsBackToNewPathWhenOverlayAbsent): n/a

#### `TEST_F(ConfigEnvOverlayTest, DevOverlayTakesPrecedenceOverNewPath)`
- Source: `tests/config/test_config_path_resolver.cpp`:678
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (DevOverlayTakesPrecedenceOverNewPath): n/a

#### `TEST_F(ConfigEnvOverlayTest, ProdEnvironmentDoesNotUseOverlay)`
- Source: `tests/config/test_config_path_resolver.cpp`:702
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (ProdEnvironmentDoesNotUseOverlay): n/a

#### `TEST_F(ConfigEnvOverlayTest, ResolveErrorMessageIncludesOverlayPathInDevEnv)`
- Source: `tests/config/test_config_path_resolver.cpp`:784
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (ResolveErrorMessageIncludesOverlayPathInDevEnv): n/a

#### `TEST_F(ConfigEnvOverlayTest, SetAndGetEnvironmentRoundTrip)`
- Source: `tests/config/test_config_path_resolver.cpp`:759
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (SetAndGetEnvironmentRoundTrip): n/a

#### `TEST_F(ConfigEnvOverlayTest, SetEnvironmentChangesActiveEnv)`
- Source: `tests/config/test_config_path_resolver.cpp`:659
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (SetEnvironmentChangesActiveEnv): n/a

#### `TEST_F(ConfigEnvOverlayTest, SetEnvironmentClearsCache)`
- Source: `tests/config/test_config_path_resolver.cpp`:667
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (SetEnvironmentClearsCache): n/a

#### `TEST_F(ConfigEnvOverlayTest, StagingOverlayTakesPrecedenceOverNewPath)`
- Source: `tests/config/test_config_path_resolver.cpp`:691
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (StagingOverlayTakesPrecedenceOverNewPath): n/a

#### `TEST_F(ConfigEnvOverlayTest, ThemisConfigEnvVariableDevSetsDevEnvironment)`
- Source: `tests/config/test_config_path_resolver.cpp`:807
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (ThemisConfigEnvVariableDevSetsDevEnvironment): n/a

#### `TEST_F(ConfigEnvOverlayTest, ThemisConfigEnvVariableProdDefaultsToNoOverlay)`
- Source: `tests/config/test_config_path_resolver.cpp`:819
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (ThemisConfigEnvVariableProdDefaultsToNoOverlay): n/a

#### `TEST_F(ConfigEnvOverlayTest, ThemisConfigEnvVariableStagingSetsStagingEnvironment)`
- Source: `tests/config/test_config_path_resolver.cpp`:814
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (ThemisConfigEnvVariableStagingSetsStagingEnvironment): n/a

#### `TEST_F(ConfigEnvOverlayTest, UnmappedPathInDevDoesNotProbeOverlay)`
- Source: `tests/config/test_config_path_resolver.cpp`:771
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigEnvOverlayTest): n/a
  - `<unnamed>` (UnmappedPathInDevDoesNotProbeOverlay): n/a

#### `TEST_F(ConfigFileWatcherHardeningTest, CFG17_PollingIntervalBounds)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:42
- Brief: CFG-17: Watcher respects polling interval bounds [kFileWatcherMinPollInterval, kFileWatcherMaxPollInterval].
- Parameters:
  - `<unnamed>` (ConfigFileWatcherHardeningTest): n/a
  - `<unnamed>` (CFG17_PollingIntervalBounds): n/a

#### `TEST_F(ConfigFileWatcherHardeningTest, CFG18_ModificationDetectionLatency)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:52
- Brief: CFG-18: Watcher detects file modifications within bounded latency.
- Parameters:
  - `<unnamed>` (ConfigFileWatcherHardeningTest): n/a
  - `<unnamed>` (CFG18_ModificationDetectionLatency): n/a

#### `TEST_F(ConfigFileWatcherHardeningTest, CFG19_FileDeletionHandling)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:58
- Brief: CFG-19: Watcher handles file deletion gracefully (signals change, no crash).
- Parameters:
  - `<unnamed>` (ConfigFileWatcherHardeningTest): n/a
  - `<unnamed>` (CFG19_FileDeletionHandling): n/a

#### `TEST_F(ConfigFileWatcherHardeningTest, CFG20_MinimumPollInterval)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:85
- Brief: CFG-20: Watcher prevents busy-wait by enforcing minimum poll interval.
- Parameters:
  - `<unnamed>` (ConfigFileWatcherHardeningTest): n/a
  - `<unnamed>` (CFG20_MinimumPollInterval): n/a

#### `TEST_F(ConfigFileWatcherHardeningTest, CFG21_ConcurrentModificationConsistency)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:90
- Brief: CFG-21: Watcher handles concurrent modifications during watch window (eventual consistency).
- Parameters:
  - `<unnamed>` (ConfigFileWatcherHardeningTest): n/a
  - `<unnamed>` (CFG21_ConcurrentModificationConsistency): n/a

#### `TEST_F(ConfigFileWatcherHardeningTest, CFG22_OperationTimeout)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:122
- Brief: CFG-22: Watcher operation timeout prevents indefinite blocking (kFileWatcherOperationTimeout).
- Parameters:
  - `<unnamed>` (ConfigFileWatcherHardeningTest): n/a
  - `<unnamed>` (CFG22_OperationTimeout): n/a

#### `TEST_F(ConfigFileWatcherHardeningTest, CFG23_PermissionDeniedHandling)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:129
- Brief: CFG-23: Watcher handles permission denied errors gracefully (logs, signals error, no crash).
- Parameters:
  - `<unnamed>` (ConfigFileWatcherHardeningTest): n/a
  - `<unnamed>` (CFG23_PermissionDeniedHandling): n/a

#### `TEST_F(ConfigFileWatcherHardeningTest, CFG24_NonBlockingWatcher)`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:158
- Brief: CFG-24: Watcher does not block main application threads (all I/O is async or threaded).
- Parameters:
  - `<unnamed>` (ConfigFileWatcherHardeningTest): n/a
  - `<unnamed>` (CFG24_NonBlockingWatcher): n/a

#### `TEST_F(ConfigFileWatcherTest, ConstructorSetsProperties)`
- Source: `tests/config/test_config_file_watcher.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigFileWatcherTest): n/a
  - `<unnamed>` (ConstructorSetsProperties): n/a

#### `TEST_F(ConfigFileWatcherTest, CustomDebounce)`
- Source: `tests/config/test_config_file_watcher.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigFileWatcherTest): n/a
  - `<unnamed>` (CustomDebounce): n/a

#### `TEST_F(ConfigFileWatcherTest, DestructorStopsThread)`
- Source: `tests/config/test_config_file_watcher.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigFileWatcherTest): n/a
  - `<unnamed>` (DestructorStopsThread): n/a

#### `TEST_F(ConfigFileWatcherTest, StartAndStop)`
- Source: `tests/config/test_config_file_watcher.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigFileWatcherTest): n/a
  - `<unnamed>` (StartAndStop): n/a

#### `TEST_F(ConfigFileWatcherTest, StartFailsOnNonexistentPath)`
- Source: `tests/config/test_config_file_watcher.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigFileWatcherTest): n/a
  - `<unnamed>` (StartFailsOnNonexistentPath): n/a

#### `TEST_F(ConfigFileWatcherTest, StartIdempotent)`
- Source: `tests/config/test_config_file_watcher.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigFileWatcherTest): n/a
  - `<unnamed>` (StartIdempotent): n/a

#### `TEST_F(ConfigFileWatcherTest, StopBeforeStart)`
- Source: `tests/config/test_config_file_watcher.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigFileWatcherTest): n/a
  - `<unnamed>` (StopBeforeStart): n/a

#### `TEST_F(ConfigFileWatcherTest, StopIdempotent)`
- Source: `tests/config/test_config_file_watcher.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigFileWatcherTest): n/a
  - `<unnamed>` (StopIdempotent): n/a

#### `TEST_F(ConfigMetricsExporterTest, CacheCapacityIsPositive)`
- Source: `tests/config/test_config_path_resolver.cpp`:1002
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsExporterTest): n/a
  - `<unnamed>` (CacheCapacityIsPositive): n/a

#### `TEST_F(ConfigMetricsExporterTest, CacheHitRatioIsZeroWithNoActivity)`
- Source: `tests/config/test_config_path_resolver.cpp`:995
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsExporterTest): n/a
  - `<unnamed>` (CacheHitRatioIsZeroWithNoActivity): n/a

#### `TEST_F(ConfigMetricsExporterTest, CacheTtlSecondsIsPositive)`
- Source: `tests/config/test_config_path_resolver.cpp`:1009
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsExporterTest): n/a
  - `<unnamed>` (CacheTtlSecondsIsPositive): n/a

#### `TEST_F(ConfigMetricsExporterTest, CollectContainsHelpAndTypeAnnotations)`
- Source: `tests/config/test_config_path_resolver.cpp`:970
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsExporterTest): n/a
  - `<unnamed>` (CollectContainsHelpAndTypeAnnotations): n/a

#### `TEST_F(ConfigMetricsExporterTest, CollectContainsPerCategoryFallbackMetric)`
- Source: `tests/config/test_config_path_resolver.cpp`:1054
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsExporterTest): n/a
  - `<unnamed>` (CollectContainsPerCategoryFallbackMetric): n/a

#### `TEST_F(ConfigMetricsExporterTest, CollectContainsRequiredMetricNames)`
- Source: `tests/config/test_config_path_resolver.cpp`:958
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsExporterTest): n/a
  - `<unnamed>` (CollectContainsRequiredMetricNames): n/a

#### `TEST_F(ConfigMetricsExporterTest, CollectIsIdempotent)`
- Source: `tests/config/test_config_path_resolver.cpp`:1040
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsExporterTest): n/a
  - `<unnamed>` (CollectIsIdempotent): n/a

#### `TEST_F(ConfigMetricsExporterTest, CollectReflectsResolutionMissCount)`
- Source: `tests/config/test_config_path_resolver.cpp`:981
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsExporterTest): n/a
  - `<unnamed>` (CollectReflectsResolutionMissCount): n/a

#### `TEST_F(ConfigMetricsExporterTest, CollectReturnsNonEmptyString)`
- Source: `tests/config/test_config_path_resolver.cpp`:953
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsExporterTest): n/a
  - `<unnamed>` (CollectReturnsNonEmptyString): n/a

#### `TEST_F(ConfigMetricsExporterTest, UpdateMetricsCollectorDoesNotThrow)`
- Source: `tests/config/test_config_path_resolver.cpp`:1018
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsExporterTest): n/a
  - `<unnamed>` (UpdateMetricsCollectorDoesNotThrow): n/a

#### `TEST_F(ConfigMetricsExporterTest, UpdateMetricsCollectorGaugeSinkBridgeIsUsed)`
- Source: `tests/config/test_config_path_resolver.cpp`:1022
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsExporterTest): n/a
  - `<unnamed>` (UpdateMetricsCollectorGaugeSinkBridgeIsUsed): n/a

#### `TEST_F(ConfigMetricsScrapeTest, CacheHitRatioIsZeroWhenNoLookups)`
- Source: `tests/config/test_config_metrics_scrape.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsScrapeTest): n/a
  - `<unnamed>` (CacheHitRatioIsZeroWhenNoLookups): n/a

#### `TEST_F(ConfigMetricsScrapeTest, OutputContainsAllRequiredMetricNames)`
- Source: `tests/config/test_config_metrics_scrape.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsScrapeTest): n/a
  - `<unnamed>` (OutputContainsAllRequiredMetricNames): n/a

#### `TEST_F(ConfigMetricsScrapeTest, OutputContainsHelpAndTypeAnnotations)`
- Source: `tests/config/test_config_metrics_scrape.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsScrapeTest): n/a
  - `<unnamed>` (OutputContainsHelpAndTypeAnnotations): n/a

#### `TEST_F(ConfigMetricsScrapeTest, OutputEndsWithNewline)`
- Source: `tests/config/test_config_metrics_scrape.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsScrapeTest): n/a
  - `<unnamed>` (OutputEndsWithNewline): n/a

#### `TEST_F(ConfigMetricsScrapeTest, RepeatedScrapesAllBelowOneMillisecond)`
- Source: `tests/config/test_config_metrics_scrape.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsScrapeTest): n/a
  - `<unnamed>` (RepeatedScrapesAllBelowOneMillisecond): n/a

#### `TEST_F(ConfigMetricsScrapeTest, ScrapeLatencyColdBelowOneMillisecond)`
- Source: `tests/config/test_config_metrics_scrape.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsScrapeTest): n/a
  - `<unnamed>` (ScrapeLatencyColdBelowOneMillisecond): n/a

#### `TEST_F(ConfigMetricsScrapeTest, ScrapeLatencyWarmBelowOneMillisecond)`
- Source: `tests/config/test_config_metrics_scrape.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsScrapeTest): n/a
  - `<unnamed>` (ScrapeLatencyWarmBelowOneMillisecond): n/a

#### `TEST_F(ConfigMetricsScrapeTest, ZeroCountersOnFreshReset)`
- Source: `tests/config/test_config_metrics_scrape.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigMetricsScrapeTest): n/a
  - `<unnamed>` (ZeroCountersOnFreshReset): n/a

#### `TEST_F(ConfigPathResolverExtraTest, AbsolutePathWithConfigComponentIsAccepted)`
- Source: `tests/config/test_config_coverage.cpp`:425
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverExtraTest): n/a
  - `<unnamed>` (AbsolutePathWithConfigComponentIsAccepted): n/a

#### `TEST_F(ConfigPathResolverExtraTest, CachingDisabledSkipsCache)`
- Source: `tests/config/test_config_coverage.cpp`:505
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverExtraTest): n/a
  - `<unnamed>` (CachingDisabledSkipsCache): n/a

#### `TEST_F(ConfigPathResolverExtraTest, GetMetadataForKnownPathReturnsMeta)`
- Source: `tests/config/test_config_coverage.cpp`:497
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverExtraTest): n/a
  - `<unnamed>` (GetMetadataForKnownPathReturnsMeta): n/a

#### `TEST_F(ConfigPathResolverExtraTest, GetMetadataReturnsNulloptForUnknownPath)`
- Source: `tests/config/test_config_coverage.cpp`:492
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverExtraTest): n/a
  - `<unnamed>` (GetMetadataReturnsNulloptForUnknownPath): n/a

#### `TEST_F(ConfigPathResolverExtraTest, MetricsTrackNewPathHits)`
- Source: `tests/config/test_config_coverage.cpp`:473
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverExtraTest): n/a
  - `<unnamed>` (MetricsTrackNewPathHits): n/a

#### `TEST_F(ConfigPathResolverExtraTest, RegisterSighupHandlerDoesNotThrow)`
- Source: `tests/config/test_config_coverage.cpp`:421
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverExtraTest): n/a
  - `<unnamed>` (RegisterSighupHandlerDoesNotThrow): n/a

#### `TEST_F(ConfigPathResolverExtraTest, ThresholdDoublingPreventsRepeatWarnings)`
- Source: `tests/config/test_config_coverage.cpp`:441
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverExtraTest): n/a
  - `<unnamed>` (ThresholdDoublingPreventsRepeatWarnings): n/a

#### `TEST_F(ConfigPathResolverTest, AggregationEnabledSuppressesPerCallWarnings)`
- Source: `tests/config/test_config_path_resolver.cpp`:522
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (AggregationEnabledSuppressesPerCallWarnings): n/a

#### `TEST_F(ConfigPathResolverTest, AggregationEnabledTracksUsageWhileSuppressingWarnings)`
- Source: `tests/config/test_config_path_resolver.cpp`:548
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (AggregationEnabledTracksUsageWhileSuppressingWarnings): n/a

#### `TEST_F(ConfigPathResolverTest, AllMappingTableEntriesAreValid)`
- Source: `tests/config/test_config_path_resolver.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (AllMappingTableEntriesAreValid): n/a

#### `TEST_F(ConfigPathResolverTest, CacheClear)`
- Source: `tests/config/test_config_path_resolver.cpp`:243
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (CacheClear): n/a

#### `TEST_F(ConfigPathResolverTest, CachingDisabled)`
- Source: `tests/config/test_config_path_resolver.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (CachingDisabled): n/a

#### `TEST_F(ConfigPathResolverTest, CachingEnabled)`
- Source: `tests/config/test_config_path_resolver.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (CachingEnabled): n/a

#### `TEST_F(ConfigPathResolverTest, CurrentCacheConfigReturnsDefaults)`
- Source: `tests/config/test_config_path_resolver.cpp`:432
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (CurrentCacheConfigReturnsDefaults): n/a

#### `TEST_F(ConfigPathResolverTest, DeprecationReportEmptyInitially)`
- Source: `tests/config/test_config_path_resolver.cpp`:441
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (DeprecationReportEmptyInitially): n/a

#### `TEST_F(ConfigPathResolverTest, DeprecationReportEntriesHaveExpectedFields)`
- Source: `tests/config/test_config_path_resolver.cpp`:482
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (DeprecationReportEntriesHaveExpectedFields): n/a

#### `TEST_F(ConfigPathResolverTest, DeprecationReportResetOnMetricsReset)`
- Source: `tests/config/test_config_path_resolver.cpp`:506
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (DeprecationReportResetOnMetricsReset): n/a

#### `TEST_F(ConfigPathResolverTest, DeprecationReportSortedByUsageCountDescending)`
- Source: `tests/config/test_config_path_resolver.cpp`:580
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (DeprecationReportSortedByUsageCountDescending): n/a

#### `TEST_F(ConfigPathResolverTest, DeprecationReportTracksLegacyUsage)`
- Source: `tests/config/test_config_path_resolver.cpp`:447
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (DeprecationReportTracksLegacyUsage): n/a

#### `TEST_F(ConfigPathResolverTest, IsLegacyPathFalse)`
- Source: `tests/config/test_config_path_resolver.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (IsLegacyPathFalse): n/a

#### `TEST_F(ConfigPathResolverTest, IsLegacyPathTrue)`
- Source: `tests/config/test_config_path_resolver.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (IsLegacyPathTrue): n/a

#### `TEST_F(ConfigPathResolverTest, LegacyPathMappingsReturnsNonEmptyMap)`
- Source: `tests/config/test_config_path_resolver.cpp`:422
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (LegacyPathMappingsReturnsNonEmptyMap): n/a

#### `TEST_F(ConfigPathResolverTest, MapLegacyToNewKnownPath)`
- Source: `tests/config/test_config_path_resolver.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (MapLegacyToNewKnownPath): n/a

#### `TEST_F(ConfigPathResolverTest, MapLegacyToNewPerformancePath)`
- Source: `tests/config/test_config_path_resolver.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (MapLegacyToNewPerformancePath): n/a

#### `TEST_F(ConfigPathResolverTest, MapLegacyToNewSecurityPath)`
- Source: `tests/config/test_config_path_resolver.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (MapLegacyToNewSecurityPath): n/a

#### `TEST_F(ConfigPathResolverTest, MapLegacyToNewUnknownPath)`
- Source: `tests/config/test_config_path_resolver.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (MapLegacyToNewUnknownPath): n/a

#### `TEST_F(ConfigPathResolverTest, MetadataDeprecationMessageContainsBothPaths)`
- Source: `tests/config/test_config_path_resolver.cpp`:414
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (MetadataDeprecationMessageContainsBothPaths): n/a

#### `TEST_F(ConfigPathResolverTest, MetadataTableCoversAllMappedPaths)`
- Source: `tests/config/test_config_path_resolver.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (MetadataTableCoversAllMappedPaths): n/a

#### `TEST_F(ConfigPathResolverTest, MetricsResetWorks)`
- Source: `tests/config/test_config_path_resolver.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (MetricsResetWorks): n/a

#### `TEST_F(ConfigPathResolverTest, MetricsTrackResolutionHits)`
- Source: `tests/config/test_config_path_resolver.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (MetricsTrackResolutionHits): n/a

#### `TEST_F(ConfigPathResolverTest, MetricsTrackResolutionMisses)`
- Source: `tests/config/test_config_path_resolver.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (MetricsTrackResolutionMisses): n/a

#### `TEST_F(ConfigPathResolverTest, MetricsTrackUnmappedRequests)`
- Source: `tests/config/test_config_path_resolver.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (MetricsTrackUnmappedRequests): n/a

#### `TEST_F(ConfigPathResolverTest, NewPathsFollowHierarchicalStructure)`
- Source: `tests/config/test_config_path_resolver.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (NewPathsFollowHierarchicalStructure): n/a

#### `TEST_F(ConfigPathResolverTest, NormalizePathBackslashes)`
- Source: `tests/config/test_config_path_resolver.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (NormalizePathBackslashes): n/a

#### `TEST_F(ConfigPathResolverTest, NormalizePathForwardSlashes)`
- Source: `tests/config/test_config_path_resolver.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (NormalizePathForwardSlashes): n/a

#### `TEST_F(ConfigPathResolverTest, NormalizePathRemovesLeadingDotSlash)`
- Source: `tests/config/test_config_path_resolver.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (NormalizePathRemovesLeadingDotSlash): n/a

#### `TEST_F(ConfigPathResolverTest, NormalizePathRemovesTrailingSlash)`
- Source: `tests/config/test_config_path_resolver.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (NormalizePathRemovesTrailingSlash): n/a

#### `TEST_F(ConfigPathResolverTest, RejectsAbsoluteSymlinkOutsideConfigRoot)`
- Source: `tests/config/test_config_path_resolver.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (RejectsAbsoluteSymlinkOutsideConfigRoot): n/a

#### `TEST_F(ConfigPathResolverTest, RejectsMultiplePathTraversals)`
- Source: `tests/config/test_config_path_resolver.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (RejectsMultiplePathTraversals): n/a

#### `TEST_F(ConfigPathResolverTest, RejectsPathTraversal)`
- Source: `tests/config/test_config_path_resolver.cpp`:305
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (RejectsPathTraversal): n/a

#### `TEST_F(ConfigPathResolverTest, RejectsRelativePathTraversal)`
- Source: `tests/config/test_config_path_resolver.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (RejectsRelativePathTraversal): n/a

#### `TEST_F(ConfigPathResolverTest, RejectsSymlinkOutsideConfigRoot)`
- Source: `tests/config/test_config_path_resolver.cpp`:320
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (RejectsSymlinkOutsideConfigRoot): n/a

#### `TEST_F(ConfigPathResolverTest, ResolveThrowsConfigNotFoundExceptionWithDetails)`
- Source: `tests/config/test_config_path_resolver.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (ResolveThrowsConfigNotFoundExceptionWithDetails): n/a

#### `TEST_F(ConfigPathResolverTest, ResolveThrowsForNonexistentFile)`
- Source: `tests/config/test_config_path_resolver.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (ResolveThrowsForNonexistentFile): n/a

#### `TEST_F(ConfigPathResolverTest, SetAggregationEnabledStartsAndStopsThread)`
- Source: `tests/config/test_config_path_resolver.cpp`:513
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (SetAggregationEnabledStartsAndStopsThread): n/a

#### `TEST_F(ConfigPathResolverTest, ThresholdCanBeSetAndRetrieved)`
- Source: `tests/config/test_config_path_resolver.cpp`:839
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (ThresholdCanBeSetAndRetrieved): n/a

#### `TEST_F(ConfigPathResolverTest, ThresholdClampsAboveOneToOne)`
- Source: `tests/config/test_config_path_resolver.cpp`:852
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (ThresholdClampsAboveOneToOne): n/a

#### `TEST_F(ConfigPathResolverTest, ThresholdClampsNegativeToZero)`
- Source: `tests/config/test_config_path_resolver.cpp`:847
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (ThresholdClampsNegativeToZero): n/a

#### `TEST_F(ConfigPathResolverTest, ThresholdDefaultIsZero)`
- Source: `tests/config/test_config_path_resolver.cpp`:835
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (ThresholdDefaultIsZero): n/a

#### `TEST_F(ConfigPathResolverTest, ThresholdNoWarningWhenDisabled)`
- Source: `tests/config/test_config_path_resolver.cpp`:860
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (ThresholdNoWarningWhenDisabled): n/a

#### `TEST_F(ConfigPathResolverTest, ThresholdNoWarningWhenRateBelowThreshold)`
- Source: `tests/config/test_config_path_resolver.cpp`:905
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (ThresholdNoWarningWhenRateBelowThreshold): n/a

#### `TEST_F(ConfigPathResolverTest, ThresholdWarnCountResetOnMetricsReset)`
- Source: `tests/config/test_config_path_resolver.cpp`:928
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (ThresholdWarnCountResetOnMetricsReset): n/a

#### `TEST_F(ConfigPathResolverTest, ThresholdWarningFiredWhenRateExceeds)`
- Source: `tests/config/test_config_path_resolver.cpp`:879
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (ThresholdWarningFiredWhenRateExceeds): n/a

#### `TEST_F(ConfigPathResolverTest, TryResolveReturnsNulloptForNonexistentFile)`
- Source: `tests/config/test_config_path_resolver.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (TryResolveReturnsNulloptForNonexistentFile): n/a

#### `TEST_F(ConfigPathResolverTest, TryResolveReturnsNulloptForPathTraversal)`
- Source: `tests/config/test_config_path_resolver.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigPathResolverTest): n/a
  - `<unnamed>` (TryResolveReturnsNulloptForPathTraversal): n/a

#### `TEST_F(ConfigResolverHardeningTest, CFG01_RejectOversizedPath)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:39
- Brief: CFG-01: Resolver rejects oversized paths (> kMaxConfigPathBytes).
- Parameters:
  - `<unnamed>` (ConfigResolverHardeningTest): n/a
  - `<unnamed>` (CFG01_RejectOversizedPath): n/a

#### `TEST_F(ConfigResolverHardeningTest, CFG02_MissingPathNoFallback)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:49
- Brief: CFG-02: Resolver handles missing paths gracefully (no exists, no fallback).
- Parameters:
  - `<unnamed>` (ConfigResolverHardeningTest): n/a
  - `<unnamed>` (CFG02_MissingPathNoFallback): n/a

#### `TEST_F(ConfigResolverHardeningTest, CFG03_EmptyPath)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:58
- Brief: CFG-03: Resolver handles empty paths gracefully.
- Parameters:
  - `<unnamed>` (ConfigResolverHardeningTest): n/a
  - `<unnamed>` (CFG03_EmptyPath): n/a

#### `TEST_F(ConfigResolverHardeningTest, CFG04_PreventDirectoryTraversal)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:67
- Brief: CFG-04: Resolver prevents directory-traversal attacks (../, ..\).
- Parameters:
  - `<unnamed>` (ConfigResolverHardeningTest): n/a
  - `<unnamed>` (CFG04_PreventDirectoryTraversal): n/a

#### `TEST_F(ConfigResolverHardeningTest, CFG05_NullBytesInPath)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:80
- Brief: CFG-05: Resolver handles paths with null bytes safely.
- Parameters:
  - `<unnamed>` (ConfigResolverHardeningTest): n/a
  - `<unnamed>` (CFG05_NullBytesInPath): n/a

#### `TEST_F(ConfigResolverHardeningTest, CFG06_AbsoluteVsRelativePaths)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:96
- Brief: CFG-06: Resolver handles absolute vs. relative paths consistently.
- Parameters:
  - `<unnamed>` (ConfigResolverHardeningTest): n/a
  - `<unnamed>` (CFG06_AbsoluteVsRelativePaths): n/a

#### `TEST_F(ConfigResolverHardeningTest, CFG07_SpecialCharactersInPath)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:110
- Brief: CFG-07: Resolver handles special characters in paths safely (spaces, quotes, etc.).
- Parameters:
  - `<unnamed>` (ConfigResolverHardeningTest): n/a
  - `<unnamed>` (CFG07_SpecialCharactersInPath): n/a

#### `TEST_F(ConfigResolverHardeningTest, CFG08_BoundedFallbackAttempts)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:119
- Brief: CFG-08: Resolver enforces bounded fallback resolution attempts.
- Parameters:
  - `<unnamed>` (ConfigResolverHardeningTest): n/a
  - `<unnamed>` (CFG08_BoundedFallbackAttempts): n/a

#### `TEST_F(ConfigSchemaValidatorTest, AdditionalPropertiesFalseFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:386
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (AdditionalPropertiesFalseFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, AdditionalPropertiesFalsePass)`
- Source: `tests/config/test_config_schema_validator.cpp`:379
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (AdditionalPropertiesFalsePass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, AllOfCollectsErrorsFromAllFailingSubschemas)`
- Source: `tests/config/test_config_schema_validator.cpp`:516
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (AllOfCollectsErrorsFromAllFailingSubschemas): n/a

#### `TEST_F(ConfigSchemaValidatorTest, AllOfFailWhenOneSubschemaFails)`
- Source: `tests/config/test_config_schema_validator.cpp`:493
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (AllOfFailWhenOneSubschemaFails): n/a

#### `TEST_F(ConfigSchemaValidatorTest, AllOfNestedTypeConstraintsFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:509
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (AllOfNestedTypeConstraintsFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, AllOfNestedTypeConstraintsPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:502
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (AllOfNestedTypeConstraintsPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, AllOfPassWhenAllSubschemasMatch)`
- Source: `tests/config/test_config_schema_validator.cpp`:486
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (AllOfPassWhenAllSubschemasMatch): n/a

#### `TEST_F(ConfigSchemaValidatorTest, AnyOfFailWhenNoSubschemaMatches)`
- Source: `tests/config/test_config_schema_validator.cpp`:554
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (AnyOfFailWhenNoSubschemaMatches): n/a

#### `TEST_F(ConfigSchemaValidatorTest, AnyOfPassWhenFirstSubschemaMatches)`
- Source: `tests/config/test_config_schema_validator.cpp`:540
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (AnyOfPassWhenFirstSubschemaMatches): n/a

#### `TEST_F(ConfigSchemaValidatorTest, AnyOfPassWhenSecondSubschemaMatches)`
- Source: `tests/config/test_config_schema_validator.cpp`:547
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (AnyOfPassWhenSecondSubschemaMatches): n/a

#### `TEST_F(ConfigSchemaValidatorTest, AnyOfWithPropertyConstraints)`
- Source: `tests/config/test_config_schema_validator.cpp`:563
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (AnyOfWithPropertyConstraints): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ArrayItemSchemaValidation)`
- Source: `tests/config/test_config_schema_validator.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ArrayItemSchemaValidation): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ArrayMinItemsFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:329
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ArrayMinItemsFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ArrayMinItemsPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:322
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ArrayMinItemsPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ConstFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:368
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ConstFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ConstPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:361
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ConstPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, CyclicRefReportsError)`
- Source: `tests/config/test_config_schema_validator.cpp`:693
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (CyclicRefReportsError): n/a

#### `TEST_F(ConfigSchemaValidatorTest, EnumFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:354
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (EnumFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, EnumPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (EnumPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ExclusiveMinimumFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:311
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ExclusiveMinimumFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ExternalRefReportsError)`
- Source: `tests/config/test_config_schema_validator.cpp`:683
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ExternalRefReportsError): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatDateFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:755
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatDateFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatDatePass)`
- Source: `tests/config/test_config_schema_validator.cpp`:748
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatDatePass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatDateTimeFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:769
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatDateTimeFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatDateTimePass)`
- Source: `tests/config/test_config_schema_validator.cpp`:762
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatDateTimePass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatEmailFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:783
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatEmailFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatEmailPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:776
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatEmailPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatIpv4Fail)`
- Source: `tests/config/test_config_schema_validator.cpp`:811
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatIpv4Fail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatIpv4FailNotDottedDecimal)`
- Source: `tests/config/test_config_schema_validator.cpp`:818
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatIpv4FailNotDottedDecimal): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatIpv4Pass)`
- Source: `tests/config/test_config_schema_validator.cpp`:804
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatIpv4Pass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatIpv6Fail)`
- Source: `tests/config/test_config_schema_validator.cpp`:832
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatIpv6Fail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatIpv6Pass)`
- Source: `tests/config/test_config_schema_validator.cpp`:825
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatIpv6Pass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatUnknownIsAccepted)`
- Source: `tests/config/test_config_schema_validator.cpp`:839
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatUnknownIsAccepted): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatUriFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:797
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatUriFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, FormatUriPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:790
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (FormatUriPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, LoadInvalidJsonStringThrows)`
- Source: `tests/config/test_config_schema_validator.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (LoadInvalidJsonStringThrows): n/a

#### `TEST_F(ConfigSchemaValidatorTest, LoadInvalidYamlStringThrows)`
- Source: `tests/config/test_config_schema_validator.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (LoadInvalidYamlStringThrows): n/a

#### `TEST_F(ConfigSchemaValidatorTest, LoadJsonFile)`
- Source: `tests/config/test_config_schema_validator.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (LoadJsonFile): n/a

#### `TEST_F(ConfigSchemaValidatorTest, LoadJsonStringOverload)`
- Source: `tests/config/test_config_schema_validator.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (LoadJsonStringOverload): n/a

#### `TEST_F(ConfigSchemaValidatorTest, LoadMissingFileThrows)`
- Source: `tests/config/test_config_schema_validator.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (LoadMissingFileThrows): n/a

#### `TEST_F(ConfigSchemaValidatorTest, LoadYamlFileAsJson)`
- Source: `tests/config/test_config_schema_validator.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (LoadYamlFileAsJson): n/a

#### `TEST_F(ConfigSchemaValidatorTest, LoadYamlStringBoolAndIntOverload)`
- Source: `tests/config/test_config_schema_validator.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (LoadYamlStringBoolAndIntOverload): n/a

#### `TEST_F(ConfigSchemaValidatorTest, LoadYamlStringOverload)`
- Source: `tests/config/test_config_schema_validator.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (LoadYamlStringOverload): n/a

#### `TEST_F(ConfigSchemaValidatorTest, LoadYmlExtensionAsJson)`
- Source: `tests/config/test_config_schema_validator.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (LoadYmlExtensionAsJson): n/a

#### `TEST_F(ConfigSchemaValidatorTest, MainConfigJsonPassesOfficialSchema)`
- Source: `tests/config/test_config_schema_validator.cpp`:466
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (MainConfigJsonPassesOfficialSchema): n/a

#### `TEST_F(ConfigSchemaValidatorTest, NestedRefResolutionPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:665
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (NestedRefResolutionPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, NotKeyword_FailWhenValueMatchesSubSchema)`
- Source: `tests/config/test_config_schema_validator.cpp`:859
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (NotKeyword_FailWhenValueMatchesSubSchema): n/a

#### `TEST_F(ConfigSchemaValidatorTest, NotKeyword_FailWithEnumSubSchema)`
- Source: `tests/config/test_config_schema_validator.cpp`:876
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (NotKeyword_FailWithEnumSubSchema): n/a

#### `TEST_F(ConfigSchemaValidatorTest, NotKeyword_NotWithComplexSubSchema)`
- Source: `tests/config/test_config_schema_validator.cpp`:901
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (NotKeyword_NotWithComplexSubSchema): n/a

#### `TEST_F(ConfigSchemaValidatorTest, NotKeyword_NotWithComplexSubSchemaPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:910
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (NotKeyword_NotWithComplexSubSchemaPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, NotKeyword_PassWhenValueDoesNotMatchSubSchema)`
- Source: `tests/config/test_config_schema_validator.cpp`:851
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (NotKeyword_PassWhenValueDoesNotMatchSubSchema): n/a

#### `TEST_F(ConfigSchemaValidatorTest, NotKeyword_PassWithEnumSubSchema)`
- Source: `tests/config/test_config_schema_validator.cpp`:868
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (NotKeyword_PassWithEnumSubSchema): n/a

#### `TEST_F(ConfigSchemaValidatorTest, NotKeyword_TopLevelNotFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:893
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (NotKeyword_TopLevelNotFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, NotKeyword_TopLevelNotPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:885
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (NotKeyword_TopLevelNotPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, NumberAboveMaximumFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (NumberAboveMaximumFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, NumberBelowMinimumFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (NumberBelowMinimumFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, NumberMinimumPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (NumberMinimumPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, OneOfFailWhenMultipleSubschemasMatch)`
- Source: `tests/config/test_config_schema_validator.cpp`:590
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (OneOfFailWhenMultipleSubschemasMatch): n/a

#### `TEST_F(ConfigSchemaValidatorTest, OneOfFailWhenNoSubschemaMatches)`
- Source: `tests/config/test_config_schema_validator.cpp`:581
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (OneOfFailWhenNoSubschemaMatches): n/a

#### `TEST_F(ConfigSchemaValidatorTest, OneOfPassWhenExactlyOneSubschemaMatches)`
- Source: `tests/config/test_config_schema_validator.cpp`:574
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (OneOfPassWhenExactlyOneSubschemaMatches): n/a

#### `TEST_F(ConfigSchemaValidatorTest, OneOfWithObjectSchemas)`
- Source: `tests/config/test_config_schema_validator.cpp`:601
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (OneOfWithObjectSchemas): n/a

#### `TEST_F(ConfigSchemaValidatorTest, Performance_100FieldConfig_200RuleSchema_Under5ms)`
- Source: `tests/config/test_config_schema_validator.cpp`:924
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (Performance_100FieldConfig_200RuleSchema_Under5ms): n/a

#### `TEST_F(ConfigSchemaValidatorTest, RefDefinitionsResolutionFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:639
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (RefDefinitionsResolutionFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, RefDefinitionsResolutionPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:631
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (RefDefinitionsResolutionPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, RefDefsResolutionFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:621
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (RefDefsResolutionFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, RefDefsResolutionPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:613
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (RefDefsResolutionPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, RefDetectsMissingRequiredProperty)`
- Source: `tests/config/test_config_schema_validator.cpp`:655
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (RefDetectsMissingRequiredProperty): n/a

#### `TEST_F(ConfigSchemaValidatorTest, RefEnforcesRequiredPropertiesPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:647
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (RefEnforcesRequiredPropertiesPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, RequiredPropertyMissing)`
- Source: `tests/config/test_config_schema_validator.cpp`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (RequiredPropertyMissing): n/a

#### `TEST_F(ConfigSchemaValidatorTest, RequiredPropertyPresent)`
- Source: `tests/config/test_config_schema_validator.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (RequiredPropertyPresent): n/a

#### `TEST_F(ConfigSchemaValidatorTest, SchemaValidationExceptionCarriesDetails)`
- Source: `tests/config/test_config_schema_validator.cpp`:440
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (SchemaValidationExceptionCarriesDetails): n/a

#### `TEST_F(ConfigSchemaValidatorTest, StringMaxLengthFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (StringMaxLengthFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, StringMinLengthFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (StringMinLengthFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, StringMinLengthPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (StringMinLengthPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, StringPatternFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (StringPatternFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, StringPatternPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:272
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (StringPatternPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, UniqueItemsFailWhenDuplicateExists)`
- Source: `tests/config/test_config_schema_validator.cpp`:714
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (UniqueItemsFailWhenDuplicateExists): n/a

#### `TEST_F(ConfigSchemaValidatorTest, UniqueItemsFalseDoesNotEnforceUniqueness)`
- Source: `tests/config/test_config_schema_validator.cpp`:723
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (UniqueItemsFalseDoesNotEnforceUniqueness): n/a

#### `TEST_F(ConfigSchemaValidatorTest, UniqueItemsPassOnEmptyArray)`
- Source: `tests/config/test_config_schema_validator.cpp`:730
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (UniqueItemsPassOnEmptyArray): n/a

#### `TEST_F(ConfigSchemaValidatorTest, UniqueItemsPassWhenAllDistinct)`
- Source: `tests/config/test_config_schema_validator.cpp`:707
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (UniqueItemsPassWhenAllDistinct): n/a

#### `TEST_F(ConfigSchemaValidatorTest, UniqueItemsWithIntegerDuplicates)`
- Source: `tests/config/test_config_schema_validator.cpp`:737
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (UniqueItemsWithIntegerDuplicates): n/a

#### `TEST_F(ConfigSchemaValidatorTest, UnresolvableRefReportsError)`
- Source: `tests/config/test_config_schema_validator.cpp`:673
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (UnresolvableRefReportsError): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateBooleanTypePass)`
- Source: `tests/config/test_config_schema_validator.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateBooleanTypePass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateFromJsonStringFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateFromJsonStringFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateFromJsonStringPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateFromJsonStringPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateFromStringConfigPathIsString)`
- Source: `tests/config/test_config_schema_validator.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateFromStringConfigPathIsString): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateFromStringInvalidJsonReportsError)`
- Source: `tests/config/test_config_schema_validator.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateFromStringInvalidJsonReportsError): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateFromStringInvalidYamlReportsError)`
- Source: `tests/config/test_config_schema_validator.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateFromStringInvalidYamlReportsError): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateFromStringMissingRequiredFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateFromStringMissingRequiredFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateFromStringWithRefDefs)`
- Source: `tests/config/test_config_schema_validator.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateFromStringWithRefDefs): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateFromStringWithSchemaComposition)`
- Source: `tests/config/test_config_schema_validator.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateFromStringWithSchemaComposition): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateFromYamlStringFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateFromYamlStringFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateFromYamlStringPass)`
- Source: `tests/config/test_config_schema_validator.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateFromYamlStringPass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateIntegerTypePass)`
- Source: `tests/config/test_config_schema_validator.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateIntegerTypePass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateNumberAcceptsFloat)`
- Source: `tests/config/test_config_schema_validator.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateNumberAcceptsFloat): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateStringTypeFail)`
- Source: `tests/config/test_config_schema_validator.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateStringTypeFail): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateStringTypePass)`
- Source: `tests/config/test_config_schema_validator.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateStringTypePass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateWithMissingSchemaFileReportsError)`
- Source: `tests/config/test_config_schema_validator.cpp`:427
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateWithMissingSchemaFileReportsError): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateWithSchemaFilePass)`
- Source: `tests/config/test_config_schema_validator.cpp`:419
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateWithSchemaFilePass): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateYamlFileAgainstSchema)`
- Source: `tests/config/test_config_schema_validator.cpp`:399
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateYamlFileAgainstSchema): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidateYamlFileFailsSchemaViolation)`
- Source: `tests/config/test_config_schema_validator.cpp`:407
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidateYamlFileFailsSchemaViolation): n/a

#### `TEST_F(ConfigSchemaValidatorTest, ValidationResultFormatErrors)`
- Source: `tests/config/test_config_schema_validator.cpp`:452
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConfigSchemaValidatorTest): n/a
  - `<unnamed>` (ValidationResultFormatErrors): n/a

#### `TEST_F(ConfigValidatorHardeningTest, CFG09_RejectOversizedConfigFile)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:139
- Brief: CFG-09: Validator rejects oversized config files (> kMaxConfigFileSizeBytes).
- Parameters:
  - `<unnamed>` (ConfigValidatorHardeningTest): n/a
  - `<unnamed>` (CFG09_RejectOversizedConfigFile): n/a

#### `TEST_F(ConfigValidatorHardeningTest, CFG10_RejectOversizedSchema)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:146
- Brief: CFG-10: Validator rejects oversized schemas (> kMaxSchemaSizeBytes).
- Parameters:
  - `<unnamed>` (ConfigValidatorHardeningTest): n/a
  - `<unnamed>` (CFG10_RejectOversizedSchema): n/a

#### `TEST_F(ConfigValidatorHardeningTest, CFG11_CircularSchemaReferences)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:151
- Brief: CFG-11: Validator detects and rejects circular schema references.
- Parameters:
  - `<unnamed>` (ConfigValidatorHardeningTest): n/a
  - `<unnamed>` (CFG11_CircularSchemaReferences): n/a

#### `TEST_F(ConfigValidatorHardeningTest, CFG12_MaxNestingDepth)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:168
- Brief: CFG-12: Validator enforces maximum nesting depth (kMaxConfigNestingDepth).
- Parameters:
  - `<unnamed>` (ConfigValidatorHardeningTest): n/a
  - `<unnamed>` (CFG12_MaxNestingDepth): n/a

#### `TEST_F(ConfigValidatorHardeningTest, CFG13_MaxTopLevelKeys)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:187
- Brief: CFG-13: Validator enforces maximum config key count (kMaxConfigTopLevelKeys).
- Parameters:
  - `<unnamed>` (ConfigValidatorHardeningTest): n/a
  - `<unnamed>` (CFG13_MaxTopLevelKeys): n/a

#### `TEST_F(ConfigValidatorHardeningTest, CFG14_MalformedJsonHandling)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:205
- Brief: CFG-14: Validator rejects malformed JSON/YAML without crashing.
- Parameters:
  - `<unnamed>` (ConfigValidatorHardeningTest): n/a
  - `<unnamed>` (CFG14_MalformedJsonHandling): n/a

#### `TEST_F(ConfigValidatorHardeningTest, CFG15_MaxValueSize)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:218
- Brief: CFG-15: Validator enforces maximum value size (kMaxConfigValueBytes).
- Parameters:
  - `<unnamed>` (ConfigValidatorHardeningTest): n/a
  - `<unnamed>` (CFG15_MaxValueSize): n/a

#### `TEST_F(ConfigValidatorHardeningTest, CFG16_ExternalRefPrevention)`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:234
- Brief: CFG-16: Validator prevents external $ref resolution (SSRF prevention).
- Parameters:
  - `<unnamed>` (ConfigValidatorHardeningTest): n/a
  - `<unnamed>` (CFG16_ExternalRefPrevention): n/a

#### `TEST_F(HotReloadIntegrationTest, StartHotReloadFailsForNonexistentDir)`
- Source: `tests/config/test_config_file_watcher.cpp`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadIntegrationTest): n/a
  - `<unnamed>` (StartHotReloadFailsForNonexistentDir): n/a

#### `TEST_F(HotReloadIntegrationTest, StartHotReloadIdempotent)`
- Source: `tests/config/test_config_file_watcher.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadIntegrationTest): n/a
  - `<unnamed>` (StartHotReloadIdempotent): n/a

#### `TEST_F(HotReloadIntegrationTest, StartHotReloadReturnsTrue)`
- Source: `tests/config/test_config_file_watcher.cpp`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadIntegrationTest): n/a
  - `<unnamed>` (StartHotReloadReturnsTrue): n/a

#### `TEST_F(HotReloadIntegrationTest, StopHotReloadBeforeStart)`
- Source: `tests/config/test_config_file_watcher.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadIntegrationTest): n/a
  - `<unnamed>` (StopHotReloadBeforeStart): n/a

#### `TEST_F(LRUCacheTest, ClearEmptiesCache)`
- Source: `tests/config/test_config_coverage.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (ClearEmptiesCache): n/a

#### `TEST_F(LRUCacheTest, EmptyOnConstruction)`
- Source: `tests/config/test_config_coverage.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (EmptyOnConstruction): n/a

#### `TEST_F(LRUCacheTest, EvictsLRUWhenFull)`
- Source: `tests/config/test_config_coverage.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (EvictsLRUWhenFull): n/a

#### `TEST_F(LRUCacheTest, GetReturnsNulloptForMissingKey)`
- Source: `tests/config/test_config_coverage.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (GetReturnsNulloptForMissingKey): n/a

#### `TEST_F(LRUCacheTest, GetReturnsPutValue)`
- Source: `tests/config/test_config_coverage.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (GetReturnsPutValue): n/a

#### `TEST_F(LRUCacheTest, InvalidateRemovesEntry)`
- Source: `tests/config/test_config_coverage.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (InvalidateRemovesEntry): n/a

#### `TEST_F(LRUCacheTest, InvalidateReturnsFalseForMissingKey)`
- Source: `tests/config/test_config_coverage.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (InvalidateReturnsFalseForMissingKey): n/a

#### `TEST_F(LRUCacheTest, PutUpdateExistingKey)`
- Source: `tests/config/test_config_coverage.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (PutUpdateExistingKey): n/a

#### `TEST_F(LRUCacheTest, PutWithCustomTtlExpiresSeparately)`
- Source: `tests/config/test_config_coverage.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (PutWithCustomTtlExpiresSeparately): n/a

#### `TEST_F(LRUCacheTest, RemoveExpiredClearsExpiredEntries)`
- Source: `tests/config/test_config_coverage.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (RemoveExpiredClearsExpiredEntries): n/a

#### `TEST_F(LRUCacheTest, RemoveExpiredKeepsValidEntries)`
- Source: `tests/config/test_config_coverage.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (RemoveExpiredKeepsValidEntries): n/a

#### `TEST_F(LRUCacheTest, SizeIncreasesAfterPut)`
- Source: `tests/config/test_config_coverage.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (SizeIncreasesAfterPut): n/a

#### `TEST_F(LRUCacheTest, StatsCapacityMatchesConstructor)`
- Source: `tests/config/test_config_coverage.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (StatsCapacityMatchesConstructor): n/a

#### `TEST_F(LRUCacheTest, StatsTrackEvictions)`
- Source: `tests/config/test_config_coverage.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (StatsTrackEvictions): n/a

#### `TEST_F(LRUCacheTest, StatsTrackHitsAndMisses)`
- Source: `tests/config/test_config_coverage.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (StatsTrackHitsAndMisses): n/a

#### `TEST_F(LRUCacheTest, TTLExpiryReturnsNullopt)`
- Source: `tests/config/test_config_coverage.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (LRUCacheTest): n/a
  - `<unnamed>` (TTLExpiryReturnsNullopt): n/a

#### `TEST_F(SchemaValidatorExtraTest, AdditionalPropertiesAsSchemaFailsWrongType)`
- Source: `tests/config/test_config_coverage.cpp`:681
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (AdditionalPropertiesAsSchemaFailsWrongType): n/a

#### `TEST_F(SchemaValidatorExtraTest, AdditionalPropertiesAsSchemaValidatesExtras)`
- Source: `tests/config/test_config_coverage.cpp`:672
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (AdditionalPropertiesAsSchemaValidatesExtras): n/a

#### `TEST_F(SchemaValidatorExtraTest, ArrayMaxItemsFail)`
- Source: `tests/config/test_config_coverage.cpp`:560
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (ArrayMaxItemsFail): n/a

#### `TEST_F(SchemaValidatorExtraTest, ArrayMaxItemsPass)`
- Source: `tests/config/test_config_coverage.cpp`:553
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (ArrayMaxItemsPass): n/a

#### `TEST_F(SchemaValidatorExtraTest, EmptySchemaAllowsAnything)`
- Source: `tests/config/test_config_coverage.cpp`:636
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (EmptySchemaAllowsAnything): n/a

#### `TEST_F(SchemaValidatorExtraTest, ExclusiveMaximumFail)`
- Source: `tests/config/test_config_coverage.cpp`:611
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (ExclusiveMaximumFail): n/a

#### `TEST_F(SchemaValidatorExtraTest, ExclusiveMaximumPass)`
- Source: `tests/config/test_config_coverage.cpp`:604
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (ExclusiveMaximumPass): n/a

#### `TEST_F(SchemaValidatorExtraTest, IntegerTypeRejectsFloat)`
- Source: `tests/config/test_config_coverage.cpp`:583
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (IntegerTypeRejectsFloat): n/a

#### `TEST_F(SchemaValidatorExtraTest, InvalidRegexPatternProducesWarning)`
- Source: `tests/config/test_config_coverage.cpp`:643
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (InvalidRegexPatternProducesWarning): n/a

#### `TEST_F(SchemaValidatorExtraTest, JsonParseErrorProducesValidationError)`
- Source: `tests/config/test_config_coverage.cpp`:664
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (JsonParseErrorProducesValidationError): n/a

#### `TEST_F(SchemaValidatorExtraTest, NestedObjectMissingRequiredProperty)`
- Source: `tests/config/test_config_coverage.cpp`:627
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (NestedObjectMissingRequiredProperty): n/a

#### `TEST_F(SchemaValidatorExtraTest, NestedObjectValidation)`
- Source: `tests/config/test_config_coverage.cpp`:620
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (NestedObjectValidation): n/a

#### `TEST_F(SchemaValidatorExtraTest, NullTypeFail)`
- Source: `tests/config/test_config_coverage.cpp`:576
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (NullTypeFail): n/a

#### `TEST_F(SchemaValidatorExtraTest, NullTypePass)`
- Source: `tests/config/test_config_coverage.cpp`:569
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (NullTypePass): n/a

#### `TEST_F(SchemaValidatorExtraTest, TypeAsArrayAcceptsMatchingType)`
- Source: `tests/config/test_config_coverage.cpp`:590
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (TypeAsArrayAcceptsMatchingType): n/a

#### `TEST_F(SchemaValidatorExtraTest, TypeAsArrayRejectsNonMatchingType)`
- Source: `tests/config/test_config_coverage.cpp`:597
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (TypeAsArrayRejectsNonMatchingType): n/a

#### `TEST_F(SchemaValidatorExtraTest, ValidateResultSchemaPathIsSet)`
- Source: `tests/config/test_config_coverage.cpp`:688
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (ValidateResultSchemaPathIsSet): n/a

#### `TEST_F(SchemaValidatorExtraTest, YamlParseErrorProducesValidationError)`
- Source: `tests/config/test_config_coverage.cpp`:654
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaValidatorExtraTest): n/a
  - `<unnamed>` (YamlParseErrorProducesValidationError): n/a

#### `bool contains(const std::string &text, std::string_view needle)`
- Source: `tests/config/test_config_metrics_scrape.cpp`:20
- Brief: Returns true when text contains needle as a substring.
- Parameters:
  - `text` (const std::string &): n/a
  - `needle` (std::string_view): n/a

#### `void writeFile(const std::filesystem::path &path, const std::string &content="")`
- Source: `tests/config/test_config_file_watcher.cpp`:23
- Brief: Write content to a file, creating directories as needed.
- Parameters:
  - `path` (const std::filesystem::path &): File path to write to
  - `content` (const std::string &): Optional content (default: empty file). Single-arg version creates empty file.
- Details: path File path to write to content Optional content (default: empty file). Single-arg version creates empty file.

### themis::config::test::CacheEnvConfigTest

#### `void SetUp() override`
- Source: `tests/config/test_config_path_resolver.cpp`:1102
- Brief: n/a
- Parameters: none

### themis::config::test::ConfigAuditLogTest

#### `void SetUp() override`
- Source: `tests/config/test_config_path_resolver.cpp`:1186
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_path_resolver.cpp`:1196
- Brief: n/a
- Parameters: none

#### `void createFile(const std::filesystem::path &p)`
- Source: `tests/config/test_config_path_resolver.cpp`:1203
- Brief: n/a
- Parameters:
  - `p` (const std::filesystem::path &): n/a

### themis::config::test::ConfigEncryptedStoreHardeningTest

#### `void SetUp() override`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:186
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:187
- Brief: n/a
- Parameters: none

### themis::config::test::ConfigEncryptedStoreTest

#### `void SetUp() override`
- Source: `tests/config/test_config_encrypted_store.cpp`:20
- Brief: n/a
- Parameters: none

### themis::config::test::ConfigEnvOverlayTest

#### `void SetUp() override`
- Source: `tests/config/test_config_path_resolver.cpp`:622
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_path_resolver.cpp`:636
- Brief: n/a
- Parameters: none

#### `void createFile(const std::filesystem::path &rel_path)`
- Source: `tests/config/test_config_path_resolver.cpp`:644
- Brief: n/a
- Parameters:
  - `rel_path` (const std::filesystem::path &): n/a

### themis::config::test::ConfigFileWatcherHardeningTest

#### `void SetUp() override`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:37
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_hardening_watcher_store.cpp`:38
- Brief: n/a
- Parameters: none

### themis::config::test::ConfigFileWatcherTest

#### `void SetUp() override`
- Source: `tests/config/test_config_file_watcher.cpp`:41
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_file_watcher.cpp`:48
- Brief: n/a
- Parameters: none

### themis::config::test::ConfigMetricsExporterTest

#### `void SetUp() override`
- Source: `tests/config/test_config_path_resolver.cpp`:946
- Brief: n/a
- Parameters: none

### themis::config::test::ConfigMetricsScrapeTest

#### `void SetUp() override`
- Source: `tests/config/test_config_metrics_scrape.cpp`:30
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_metrics_scrape.cpp`:35
- Brief: n/a
- Parameters: none

### themis::config::test::ConfigPathResolverExtraTest

#### `void SetUp() override`
- Source: `tests/config/test_config_coverage.cpp`:399
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_coverage.cpp`:410
- Brief: n/a
- Parameters: none

### themis::config::test::ConfigPathResolverTest

#### `void SetUp() override`
- Source: `tests/config/test_config_path_resolver.cpp`:18
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_path_resolver.cpp`:38
- Brief: n/a
- Parameters: none

#### `void createTestFile(const std::filesystem::path &path)`
- Source: `tests/config/test_config_path_resolver.cpp`:43
- Brief: n/a
- Parameters:
  - `path` (const std::filesystem::path &): n/a

### themis::config::test::ConfigResolverHardeningTest

#### `void SetUp() override`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:34
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:35
- Brief: n/a
- Parameters: none

### themis::config::test::ConfigSchemaValidatorTest

#### `void SetUp() override`
- Source: `tests/config/test_config_schema_validator.cpp`:19
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_schema_validator.cpp`:24
- Brief: n/a
- Parameters: none

#### `std::string writeFile(const std::string &name, const std::string &content)`
- Source: `tests/config/test_config_schema_validator.cpp`:29
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `content` (const std::string &): n/a

### themis::config::test::ConfigValidatorHardeningTest

#### `void SetUp() override`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:134
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_hardening_resolver_validator.cpp`:135
- Brief: n/a
- Parameters: none

### themis::config::test::HotReloadIntegrationTest

#### `void SetUp() override`
- Source: `tests/config/test_config_file_watcher.cpp`:224
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_file_watcher.cpp`:233
- Brief: n/a
- Parameters: none

### themis::config::test::SchemaValidatorExtraTest

#### `void SetUp() override`
- Source: `tests/config/test_config_coverage.cpp`:534
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/config/test_config_coverage.cpp`:539
- Brief: n/a
- Parameters: none

#### `std::string writeFile(const std::string &name, const std::string &content)`
- Source: `tests/config/test_config_coverage.cpp`:543
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `content` (const std::string &): n/a

