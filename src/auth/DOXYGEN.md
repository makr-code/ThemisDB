# AUTH DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\auth\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\auth\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 106
- Compounds: 405
- Classes/Structs: 241
- Namespaces: 49
- File Compounds: 106

## Namespaces
- @002141314075174135046125111024177276357014036342
- @021153151042057377015243071006103325163057207036
- @041200120077277114360340145316123331003101301102
- @066201142231267347101167346074324164147353160303
- @116073144055113244041173007276337163307315375250
- @125132270015312351206046224066326204111160135072
- @130170365326313370013165324213020226073077237222
- @144200325213023323375140154027364034253044007074
- @154205254130271111151253253224055364133346246364
- @157017365177374061262046307216332246066015371314
- @306326155035161223143007270053214006203144047350
- benchmark
- prometheus
- pugi
- rocksdb
- std
- std::chrono_literals
- testing
- themis
- themis::auth
- themis::auth::@010304050367133264100364044050206216046153256052
- themis::auth::@031135357066036177140031066373366354133065232322
- themis::auth::@055075007104313272116202324007125150330170352172
- themis::auth::@065046301177277302167062156363311366232060257125
- themis::auth::@117040274074236205141313205200167104034326104251
- themis::auth::@126046037000030220065044060163102003150047024316
- themis::auth::@127005343165276256246237263244223026161046142306
- themis::auth::@151061360275364150146200004173046220123313016206
- themis::auth::@161253076053324314131231335062312304005102032161
- themis::auth::@165072142357354352032102370131235366143141041270
- themis::auth::@220010012016247270042164364150060035122016353007
- themis::auth::@220223113372155012273071250256001234046042340356
- themis::auth::@240221243344250252125001351106072155151150021315
- themis::auth::@325221233152243272006352002371171006152346207037
- themis::auth::@334045076214172341100331061064227343162302316264
- themis::auth::@347214175241006205372350353233317032313307122306
- themis::auth::@356225072230055013225161236000246114363023116311
- themis::auth::@360261223242114156002271360251367200067054254172
- themis::auth::detail
- themis::auth::test
- themis::auth::tests
- themis::auth::tests::@143267001042144345135276072223216142225113037036
- themis::auth::tests::@252055223377273026237207271070370203315143361260
- themis::auth::tests::@364007262271173265350105002030026033246264216322
- themis::bench
- themis::bench::ahp
- themis::bench::ahp::themis
- themis::bench::ahp::themis::auth
- themis::utils

## Types
### Classes
- ApiKeyMiddlewareTest
- AuditLoggerIntegrationTest
- AuthAuditLoggerTest
- AuthMiddlewareTest
- BruteForceAnomalyTest
- CredentialStuffingEscalationTest
- CredentialStuffingTest
- DistributedTokenBlacklistTest
- FederationHardeningTest
- JWTScopeEnforcementTest
- RevocationHardeningTest
- SessionHardeningTest
- Wave4B2COSEAlgTest
- Wave4B2JWTKeyAuditTest
- Wave4B2MTLSAuditDirectTest
- Wave4B2PasskeyVerifyAuditTest
- Wave4B2RolePermAuditTest
- Wave4BAuditLoggerNewEventsTest
- Wave4BJWTKeyRotationAuditTest
- Wave4BMTLSAuditTest
- Wave4BPasskeyAuditTest
- prometheus::Counter
- prometheus::Family
- prometheus::Gauge
- prometheus::Histogram
- prometheus::Registry
- themis::auth::AccountLockoutManager
- themis::auth::ApiKeyAuthenticator
- themis::auth::AsyncHTTPAuth
- themis::auth::AuthAuditLogger
- themis::auth::AuthDurationTimer
- themis::auth::AuthError
- themis::auth::AuthException
- themis::auth::AuthMetrics
- themis::auth::AuthRateLimiter
- themis::auth::AuthWorkerThreadPool
- themis::auth::CertificateUtils
- themis::auth::ChannelBindingGenerator
- themis::auth::DefaultAuthEventBus
- themis::auth::DistributedTokenBlacklist
- themis::auth::EIDAuthResult::IdentityResult
- themis::auth::FederatedIdentityManager
- themis::auth::GSSAPIAuthenticator
- themis::auth::IAuthEventBus
- themis::auth::IAuthEventSubscriber
- themis::auth::IAuthorizationPolicy
- themis::auth::IEIDAuthenticator
- themis::auth::IPasskeyAuthenticator
- themis::auth::IRateLimiterBackend
- themis::auth::ITokenBlacklist
- themis::auth::InMemoryEIDAuthenticator
- themis::auth::InMemoryRateLimiterBackend
- themis::auth::JWKSSecureFetcher
- themis::auth::JWKSSecurityConfig
- themis::auth::JWKSValidator
- themis::auth::JWTKeyRotationManager
- themis::auth::JWTValidator
- themis::auth::KerberosSecurityValidator
- themis::auth::LDAPAuthenticator
- themis::auth::LDAPConnectionPool
- themis::auth::MFAAuthenticator
- themis::auth::MTLSAuthenticator
- themis::auth::OAuthDeviceFlow
- themis::auth::OAuthPKCEFlow
- themis::auth::OIDCProvider
- themis::auth::PasskeyAuthenticator
- themis::auth::PasswordPolicy
- themis::auth::PooledConnection
- themis::auth::PrincipalValidator
- themis::auth::PrincipalValidatorPresets
- themis::auth::RedisRateLimiterBackend
- themis::auth::RedisTokenBlacklist
- themis::auth::RocksDBTokenBlacklist
- themis::auth::SAMLAuthenticator
- themis::auth::SecureBuffer
- themis::auth::SecureMFAValidator
- themis::auth::SecureString
- themis::auth::SessionManager
- themis::auth::TOTPReplayCache
- themis::auth::TOTPSecretEncryption
- themis::auth::TOTPSecretRotationManager
- themis::auth::TokenBlacklist
- themis::auth::WebAuthnAuthenticator
- themis::auth::ZeroTrustAuthVerifier
- themis::auth::tests::AuthMethodsTest
- themis::auth::tests::FederationProvidersTest
- themis::auth::tests::JWTAccessControlTest
- themis::auth::tests::JWTAsyncValidationTest
- themis::auth::tests::JWTConfigurationTest
- themis::auth::tests::JWTFailureClassificationTest
- themis::auth::tests::JWTKIDRevocationTest
- themis::auth::tests::JWTScopeExtractionTest
- themis::auth::tests::JWTTemporalContractTest
- themis::auth::tests::JWTTokenBlacklistTest
- themis::auth::tests::JWTTokenExpirationTest
- themis::auth::tests::JWTTokenSizeValidationTest
- themis::auth::tests::JWTUserKeyDerivationTest
- themis::auth::tests::MockTokenBlacklist
- themis::auth::tests::RateLimitingTest
- themis::auth::tests::SAMLAttributeExtractionTest
- themis::auth::tests::SAMLAudienceValidationTest
- themis::auth::tests::SAMLAuthnRequestTest
- themis::auth::tests::SAMLConfigurationTest
- themis::auth::tests::SAMLIssuerValidationTest
- themis::auth::tests::SAMLReplayDetectionTest
- themis::auth::tests::SAMLResponseProcessingTest
- themis::auth::tests::SAMLSha1DeprecationTest
- themis::auth::tests::SAMLSignatureVerificationTest
- themis::auth::tests::SAMLTestHelper
- themis::auth::tests::SAMLTimeValidationTest
- themis::auth::tests::TokenLifecycleTest
- themis::auth::tests::WebAuthnAttestationTest
- themis::auth::tests::WebAuthnAuthDataTest
- themis::auth::tests::WebAuthnAuthenticationCeremonyTest
- themis::auth::tests::WebAuthnChallengeLifecycleTest
- themis::auth::tests::WebAuthnClientDataTest
- themis::auth::tests::WebAuthnCoseKeyTest
- themis::auth::tests::WebAuthnInitializationTest
- themis::auth::tests::WebAuthnRandomBytesTest
- themis::auth::tests::WebAuthnRegistrationCeremonyTest
- themis::auth::tests::WebAuthnSignatureVerificationTest
- themis::auth::tests::WebAuthnTestHelper
- themis::bench::ahp::DistributedBlacklistFixture
- themis::bench::ahp::themis::auth::AuthAuditLogger
- themis::bench::ahp::themis::auth::LDAPConnectionPool
- themis::bench::ahp::themis::auth::PooledConnection

### Structs
- CollectedAuthEvents
- themis::auth::ApiKeyAuthenticator::Config
- themis::auth::ApiKeyClaims
- themis::auth::ApiKeyCredential
- themis::auth::AuthAnomalyEvent
- themis::auth::AuthEvent
- themis::auth::AuthMetrics::Config
- themis::auth::AuthRateLimitConfig
- themis::auth::AuthRateLimitConfig::CredentialStuffingRedisConfig
- themis::auth::AuthRateLimiter::CredentialStuffingEntry
- themis::auth::AuthRateLimiter::Statistics
- themis::auth::AuthnRequestParams
- themis::auth::CachedValidation
- themis::auth::CertificateUtils::CertInfo
- themis::auth::ClusterNode
- themis::auth::DistributedBlacklistConfig
- themis::auth::DistributedTokenBlacklist::ReplicationStats
- themis::auth::EIDAttribute
- themis::auth::EIDAuthConfig
- themis::auth::EIDAuthRequest
- themis::auth::EIDAuthResult
- themis::auth::EIDAuthSession
- themis::auth::EIDIdentity
- themis::auth::EnvironmentAttributes
- themis::auth::FailedAttempt
- themis::auth::FederatedValidationResult
- themis::auth::GSSAPIAuthResult
- themis::auth::HTTPAuthConfig
- themis::auth::HTTPAuthResponse
- themis::auth::JWKKeyInfo
- themis::auth::JWKSSecureFetcher::FetchStats
- themis::auth::JWKSSecureFetcher::Impl
- themis::auth::JWKSSecurityConfig::Config
- themis::auth::JWKSValidator::Config
- themis::auth::JWKSValidator::ValidationResult
- themis::auth::JWTClaims
- themis::auth::JWTKeyRotationManager::Config
- themis::auth::JWTKeyRotationManager::Statistics
- themis::auth::JWTValidatorConfig
- themis::auth::KerberosConfig
- themis::auth::KerberosConfig::PrincipalMapping
- themis::auth::KerberosSecurityValidator::ASN1Tag
- themis::auth::KerberosSecurityValidator::Config
- themis::auth::KerberosSecurityValidator::TokenInfo
- themis::auth::LDAPAuthResult
- themis::auth::LDAPConfig
- themis::auth::LDAPConfig::GroupMapping
- themis::auth::LDAPPoolConfig
- themis::auth::LockoutInfo
- themis::auth::MFAAuthenticator::Config
- themis::auth::MFAAuthenticator::EnrollmentData
- themis::auth::MTLSAuthenticator::Config
- themis::auth::MTLSAuthenticator::Impl
- themis::auth::MTLSClaims
- themis::auth::OAuthDeviceFlow::Config
- themis::auth::OAuthDeviceFlow::DeviceCodeResponse
- themis::auth::OAuthDeviceFlow::TokenResponse
- themis::auth::OAuthPKCEFlow::Config
- themis::auth::OAuthPKCEFlow::PKCEChallenge
- themis::auth::OAuthPKCEFlow::TokenResponse
- themis::auth::OIDCDiscoveryDocument
- themis::auth::OIDCProviderConfig
- themis::auth::PasskeyAssertionResponse
- themis::auth::PasskeyChallenge
- themis::auth::PasskeyCredential
- themis::auth::PasswordPolicy::Config
- themis::auth::PasswordPolicy::ValidationResult
- themis::auth::PolicyEvaluationResult
- themis::auth::PrincipalValidator::Config
- themis::auth::PrincipalValidator::MappingRule
- themis::auth::PrincipalValidator::Rule
- themis::auth::PrincipalValidator::Statistics
- themis::auth::PrincipalValidator::ValidationContext
- themis::auth::PrincipalValidator::ValidationResult
- themis::auth::RedisRateLimiterBackend::Config
- themis::auth::RedisTokenBlacklist::Config
- themis::auth::ResourceAttributes
- themis::auth::RocksDBTokenBlacklist::Config
- themis::auth::SAMLClaims
- themis::auth::SAMLConfig
- themis::auth::SecureMFAValidator::Config
- themis::auth::SessionManager::SessionInfo
- themis::auth::SessionManager::SessionLimits
- themis::auth::SessionManager::ValidationResult
- themis::auth::SubjectAttributes
- themis::auth::TOTPReplayCache::Config
- themis::auth::TOTPReplayCache::Statistics
- themis::auth::TOTPReplayCache::UsedCode
- themis::auth::TOTPSecretEncryption::Config
- themis::auth::TOTPSecretEncryption::EncryptedSecret
- themis::auth::TOTPSecretEncryption::Impl
- themis::auth::TOTPSecretRotationManager::RotationConfig
- themis::auth::TOTPSecretRotationManager::SecretVersion
- themis::auth::TokenBlacklist::BloomFilter
- themis::auth::TokenBlacklist::Config
- themis::auth::TokenBlacklist::Entry
- themis::auth::TokenBlacklist::Statistics
- themis::auth::TokenExchangeResult
- themis::auth::WebAuthnAuthenticator::AssertionResult
- themis::auth::WebAuthnAuthenticator::AttestationResult
- themis::auth::WebAuthnAuthenticator::AuthData
- themis::auth::WebAuthnAuthenticator::AuthenticatorSelection
- themis::auth::WebAuthnAuthenticator::ClientData
- themis::auth::WebAuthnAuthenticator::CredentialCreationOptions
- themis::auth::WebAuthnAuthenticator::CredentialRequestOptions
- themis::auth::WebAuthnAuthenticator::PendingEntry
- themis::auth::WebAuthnAuthenticator::RelyingParty
- themis::auth::WebAuthnAuthenticator::User
- themis::auth::ZeroTrustAuthVerifier::Config
- themis::auth::ZeroTrustAuthVerifier::Decision
- themis::auth::ZeroTrustAuthVerifier::MonitorEntry
- themis::auth::ZeroTrustAuthVerifier::MonitoredSession
- themis::auth::ZeroTrustAuthVerifier::Request
- themis::bench::ahp::LatencySummary
- themis::bench::ahp::themis::auth::LDAPPoolConfig

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 1515

### ApiKeyMiddlewareTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_middleware.cpp`:724
- Brief: n/a
- Parameters: none

### AuditLoggerIntegrationTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:412
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:418
- Brief: n/a
- Parameters: none

### AuthAuditLoggerTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_audit_logger.cpp`:64
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/auth/test_auth_audit_logger.cpp`:72
- Brief: n/a
- Parameters: none

### AuthMiddlewareTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_middleware.cpp`:25
- Brief: n/a
- Parameters: none

### BruteForceAnomalyTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:66
- Brief: n/a
- Parameters: none

### CollectedAuthEvents

#### `AuthAnomalyEvent at(size_t i) const`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:42
- Brief: n/a
- Parameters:
  - `i` (size_t): n/a

#### `void clear()`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:32
- Brief: n/a
- Parameters: none

#### `bool hasType(AuthAnomalyEvent::Type t) const`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:47
- Brief: n/a
- Parameters:
  - `t` (AuthAnomalyEvent::Type): n/a

#### `size_t size() const`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:37
- Brief: n/a
- Parameters: none

### CredentialStuffingEscalationTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:520
- Brief: n/a
- Parameters: none

#### `AuthAnomalyEvent triggerStuffingForUser(AuthRateLimiter &rl, const std::string &user_id, const std::string &ip)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:539
- Brief: n/a
- Parameters:
  - `rl` (AuthRateLimiter &): n/a
  - `user_id` (const std::string &): n/a
  - `ip` (const std::string &): n/a

### CredentialStuffingTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:184
- Brief: n/a
- Parameters: none

### DistributedTokenBlacklistTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:48
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:56
- Brief: n/a
- Parameters: none

#### `std::chrono::system_clock::time_point future(int seconds)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:72
- Brief: Time-point helper: now + seconds.
- Parameters:
  - `seconds` (int): n/a

#### `std::chrono::system_clock::time_point past(int seconds)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:78
- Brief: Time-point helper: now - seconds (already expired).
- Parameters:
  - `seconds` (int): n/a

#### `DistributedBlacklistConfig singleNodeConfig() const`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:61
- Brief: Build a minimal single-node config (no cluster sync).
- Parameters: none

### JWTScopeEnforcementTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_middleware.cpp`:964
- Brief: n/a
- Parameters: none

### RevocationHardeningTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:100
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:101
- Brief: n/a
- Parameters: none

### SessionHardeningTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:334
- Brief: n/a
- Parameters: none

### Wave4B2JWTKeyAuditTest

#### `void SetUp() override`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:354
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:359
- Brief: n/a
- Parameters: none

### Wave4B2MTLSAuditDirectTest

#### `void SetUp() override`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:268
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:274
- Brief: n/a
- Parameters: none

### Wave4B2PasskeyVerifyAuditTest

#### `void SetUp() override`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:177
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:184
- Brief: n/a
- Parameters: none

### Wave4B2RolePermAuditTest

#### `void SetUp() override`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:307
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:313
- Brief: n/a
- Parameters: none

### Wave4BAuditLoggerNewEventsTest

#### `void SetUp() override`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:184
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:190
- Brief: n/a
- Parameters: none

### Wave4BJWTKeyRotationAuditTest

#### `void SetUp() override`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:232
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:237
- Brief: n/a
- Parameters: none

### Wave4BMTLSAuditTest

#### `void SetUp() override`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:138
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:144
- Brief: n/a
- Parameters: none

### Wave4BPasskeyAuditTest

#### `void SetUp() override`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:82
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:90
- Brief: n/a
- Parameters: none

### bench_auth_hotpaths.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:588
- Brief: n/a
- Parameters: none

### distributed_token_blacklist.cpp

#### `void sockClose(SockFd f) noexcept`
- Source: `src/auth/distributed_token_blacklist.cpp`:65
- Brief: n/a
- Parameters:
  - `f` (SockFd): n/a

#### `int sockLastErr() noexcept`
- Source: `src/auth/distributed_token_blacklist.cpp`:67
- Brief: n/a
- Parameters: none

#### `bool sockValid(SockFd f) noexcept`
- Source: `src/auth/distributed_token_blacklist.cpp`:66
- Brief: n/a
- Parameters:
  - `f` (SockFd): n/a

### prometheus::Counter

#### `void Increment(double=1.0)`
- Source: `include/auth/auth_metrics.h`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (double): n/a

### prometheus::Gauge

#### `void Decrement(double=1.0)`
- Source: `include/auth/auth_metrics.h`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (double): n/a

#### `void Increment(double=1.0)`
- Source: `include/auth/auth_metrics.h`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (double): n/a

#### `void Set(double)`
- Source: `include/auth/auth_metrics.h`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (double): n/a

### prometheus::Histogram

#### `void Observe(double)`
- Source: `include/auth/auth_metrics.h`:36
- Brief: n/a
- Parameters:
  - `<unnamed>` (double): n/a

### test_auth_anomaly_detection.cpp

#### `TEST(AuthAnomalyThreadSafetyTest, ConcurrentCallbackReplacement)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAnomalyThreadSafetyTest): n/a
  - `<unnamed>` (ConcurrentCallbackReplacement): n/a

#### `TEST(AuthAnomalyThreadSafetyTest, ConcurrentRecordFailedAuthDoesNotCrash)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAnomalyThreadSafetyTest): n/a
  - `<unnamed>` (ConcurrentRecordFailedAuthDoesNotCrash): n/a

#### `TEST(AuthMetricsCredentialStuffingTest, MetricsWiredToRateLimiter)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:670
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMetricsCredentialStuffingTest): n/a
  - `<unnamed>` (MetricsWiredToRateLimiter): n/a

#### `TEST(AuthMetricsCredentialStuffingTest, RecordIncreasesInternalCounter)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:662
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMetricsCredentialStuffingTest): n/a
  - `<unnamed>` (RecordIncreasesInternalCounter): n/a

#### `TEST_F(AuditLoggerIntegrationTest, BruteForceTriggersAuditLog)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:436
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLoggerIntegrationTest): n/a
  - `<unnamed>` (BruteForceTriggersAuditLog): n/a

#### `TEST_F(AuditLoggerIntegrationTest, CredentialStuffingTriggersAuditLog)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:457
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLoggerIntegrationTest): n/a
  - `<unnamed>` (CredentialStuffingTriggersAuditLog): n/a

#### `TEST_F(AuditLoggerIntegrationTest, DetachAuditLoggerStopsLogging)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:479
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLoggerIntegrationTest): n/a
  - `<unnamed>` (DetachAuditLoggerStopsLogging): n/a

#### `TEST_F(AuditLoggerIntegrationTest, SetAuditLoggerDoesNotCrash)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:423
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuditLoggerIntegrationTest): n/a
  - `<unnamed>` (SetAuditLoggerDoesNotCrash): n/a

#### `TEST_F(BruteForceAnomalyTest, BruteForceEventContainsCorrectFields)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (BruteForceAnomalyTest): n/a
  - `<unnamed>` (BruteForceEventContainsCorrectFields): n/a

#### `TEST_F(BruteForceAnomalyTest, CallbackCanBeDeregistered)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (BruteForceAnomalyTest): n/a
  - `<unnamed>` (CallbackCanBeDeregistered): n/a

#### `TEST_F(BruteForceAnomalyTest, CallbackNotFiredBeforeLockoutThreshold)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (BruteForceAnomalyTest): n/a
  - `<unnamed>` (CallbackNotFiredBeforeLockoutThreshold): n/a

#### `TEST_F(BruteForceAnomalyTest, LockoutTriggersAccountLockoutEvent)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (BruteForceAnomalyTest): n/a
  - `<unnamed>` (LockoutTriggersAccountLockoutEvent): n/a

#### `TEST_F(BruteForceAnomalyTest, LockoutTriggersBruteForceEvent)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (BruteForceAnomalyTest): n/a
  - `<unnamed>` (LockoutTriggersBruteForceEvent): n/a

#### `TEST_F(BruteForceAnomalyTest, NoCallbackDoesNotCrash)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (BruteForceAnomalyTest): n/a
  - `<unnamed>` (NoCallbackDoesNotCrash): n/a

#### `TEST_F(CredentialStuffingEscalationTest, AnomalyEventCarriesUserIdOnFailedAuth)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:593
- Brief: n/a
- Parameters:
  - `<unnamed>` (CredentialStuffingEscalationTest): n/a
  - `<unnamed>` (AnomalyEventCarriesUserIdOnFailedAuth): n/a

#### `TEST_F(CredentialStuffingEscalationTest, FirstBreachTriggersCaptcha)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:566
- Brief: n/a
- Parameters:
  - `<unnamed>` (CredentialStuffingEscalationTest): n/a
  - `<unnamed>` (FirstBreachTriggersCaptcha): n/a

#### `TEST_F(CredentialStuffingEscalationTest, InMemoryBreachCountClearedByReset)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:624
- Brief: n/a
- Parameters:
  - `<unnamed>` (CredentialStuffingEscalationTest): n/a
  - `<unnamed>` (InMemoryBreachCountClearedByReset): n/a

#### `TEST_F(CredentialStuffingEscalationTest, SecondBreachTriggersOTP)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:573
- Brief: n/a
- Parameters:
  - `<unnamed>` (CredentialStuffingEscalationTest): n/a
  - `<unnamed>` (SecondBreachTriggersOTP): n/a

#### `TEST_F(CredentialStuffingEscalationTest, ThirdBreachLocksAccount24h)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:580
- Brief: n/a
- Parameters:
  - `<unnamed>` (CredentialStuffingEscalationTest): n/a
  - `<unnamed>` (ThirdBreachLocksAccount24h): n/a

#### `TEST_F(CredentialStuffingTest, AlertAlsoFiredFromFailedAuth)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (CredentialStuffingTest): n/a
  - `<unnamed>` (AlertAlsoFiredFromFailedAuth): n/a

#### `TEST_F(CredentialStuffingTest, AlertContainsCorrectIP)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (CredentialStuffingTest): n/a
  - `<unnamed>` (AlertContainsCorrectIP): n/a

#### `TEST_F(CredentialStuffingTest, AlertFiredOncePerEvent)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (CredentialStuffingTest): n/a
  - `<unnamed>` (AlertFiredOncePerEvent): n/a

#### `TEST_F(CredentialStuffingTest, AlertFiredWhenThresholdReached)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (CredentialStuffingTest): n/a
  - `<unnamed>` (AlertFiredWhenThresholdReached): n/a

#### `TEST_F(CredentialStuffingTest, AlertNotFiredBelowThreshold)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (CredentialStuffingTest): n/a
  - `<unnamed>` (AlertNotFiredBelowThreshold): n/a

#### `TEST_F(CredentialStuffingTest, DetectionDisabledByConfig)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:278
- Brief: n/a
- Parameters:
  - `<unnamed>` (CredentialStuffingTest): n/a
  - `<unnamed>` (DetectionDisabledByConfig): n/a

#### `TEST_F(CredentialStuffingTest, SameUserRepeatedDoesNotTriggerAlert)`
- Source: `tests/auth/test_auth_anomaly_detection.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (CredentialStuffingTest): n/a
  - `<unnamed>` (SameUserRepeatedDoesNotTriggerAlert): n/a

### test_auth_audit_logger.cpp

#### `TEST_F(AuthAuditLoggerTest, ApiKeyAuthenticator_FailureLogged)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:491
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (ApiKeyAuthenticator_FailureLogged): n/a

#### `TEST_F(AuthAuditLoggerTest, ApiKeyAuthenticator_NoAuditLogger_DoesNotCrash)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:520
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (ApiKeyAuthenticator_NoAuditLogger_DoesNotCrash): n/a

#### `TEST_F(AuthAuditLoggerTest, ApiKeyAuthenticator_SecretMismatchLogged)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:504
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (ApiKeyAuthenticator_SecretMismatchLogged): n/a

#### `TEST_F(AuthAuditLoggerTest, ApiKeyAuthenticator_SetAuditLogger_AcceptsNull)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:467
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (ApiKeyAuthenticator_SetAuditLogger_AcceptsNull): n/a

#### `TEST_F(AuthAuditLoggerTest, ApiKeyAuthenticator_SuccessLogged)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:474
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (ApiKeyAuthenticator_SuccessLogged): n/a

#### `TEST_F(AuthAuditLoggerTest, GSSAPIAuthenticator_FailureLogged_WhenNotInitialized)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (GSSAPIAuthenticator_FailureLogged_WhenNotInitialized): n/a

#### `TEST_F(AuthAuditLoggerTest, GSSAPIAuthenticator_SetAuditLogger_AcceptsNull)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:283
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (GSSAPIAuthenticator_SetAuditLogger_AcceptsNull): n/a

#### `TEST_F(AuthAuditLoggerTest, IsEnabled_WhenLoggerAttached)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (IsEnabled_WhenLoggerAttached): n/a

#### `TEST_F(AuthAuditLoggerTest, IsEnabled_WhenLoggerNull)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (IsEnabled_WhenLoggerNull): n/a

#### `TEST_F(AuthAuditLoggerTest, JWTValidator_SetAuditLogger_AcceptsNull)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (JWTValidator_SetAuditLogger_AcceptsNull): n/a

#### `TEST_F(AuthAuditLoggerTest, LogApiKeyFailure_NullLogger_NoOp)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:426
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogApiKeyFailure_NullLogger_NoOp): n/a

#### `TEST_F(AuthAuditLoggerTest, LogApiKeyFailure_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:420
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogApiKeyFailure_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogApiKeySuccess_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:414
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogApiKeySuccess_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogJWTFailure_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogJWTFailure_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogJWTSuccess_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogJWTSuccess_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogKerberosFailure_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogKerberosFailure_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogKerberosSuccess_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogKerberosSuccess_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogMFAEnrolled_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogMFAEnrolled_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogOAuthDeviceDenied_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogOAuthDeviceDenied_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogOAuthDeviceGranted_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogOAuthDeviceGranted_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogRecoveryCodeUsed_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogRecoveryCodeUsed_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogSAMLFailure_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogSAMLFailure_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogSAMLSuccess_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogSAMLSuccess_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogTOTPFailure_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogTOTPFailure_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogTOTPSuccess_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogTOTPSuccess_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogTokenRevoked_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogTokenRevoked_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogZeroTrustAllowed_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogZeroTrustAllowed_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, LogZeroTrustDenied_WritesEntry)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (LogZeroTrustDenied_WritesEntry): n/a

#### `TEST_F(AuthAuditLoggerTest, MFAAuthenticator_EnrollmentLogged)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (MFAAuthenticator_EnrollmentLogged): n/a

#### `TEST_F(AuthAuditLoggerTest, MFAAuthenticator_NoAuditLogger_DoesNotCrash)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (MFAAuthenticator_NoAuditLogger_DoesNotCrash): n/a

#### `TEST_F(AuthAuditLoggerTest, MFAAuthenticator_RecoveryCodeLogged)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (MFAAuthenticator_RecoveryCodeLogged): n/a

#### `TEST_F(AuthAuditLoggerTest, MFAAuthenticator_TOTPSuccessLogged)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (MFAAuthenticator_TOTPSuccessLogged): n/a

#### `TEST_F(AuthAuditLoggerTest, NullLogger_NoOp)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (NullLogger_NoOp): n/a

#### `TEST_F(AuthAuditLoggerTest, OAuthDeviceFlow_SetAuditLogger_AcceptsNull)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:343
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (OAuthDeviceFlow_SetAuditLogger_AcceptsNull): n/a

#### `TEST_F(AuthAuditLoggerTest, PrincipalValidator_AllowedPrincipalLogged)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:366
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (PrincipalValidator_AllowedPrincipalLogged): n/a

#### `TEST_F(AuthAuditLoggerTest, PrincipalValidator_DeniedPrincipalLogged)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (PrincipalValidator_DeniedPrincipalLogged): n/a

#### `TEST_F(AuthAuditLoggerTest, PrincipalValidator_NoAuditLogger_DoesNotCrash)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:400
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (PrincipalValidator_NoAuditLogger_DoesNotCrash): n/a

#### `TEST_F(AuthAuditLoggerTest, PrincipalValidator_SetAuditLogger_AcceptsNull)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:359
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (PrincipalValidator_SetAuditLogger_AcceptsNull): n/a

#### `TEST_F(AuthAuditLoggerTest, SAMLAuthenticator_SetAuditLogger_AcceptsNull)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (SAMLAuthenticator_SetAuditLogger_AcceptsNull): n/a

#### `TEST_F(AuthAuditLoggerTest, SetLogger_DetachReattach)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (SetLogger_DetachReattach): n/a

#### `TEST_F(AuthAuditLoggerTest, TokenBlacklist_NoAuditLogger_DoesNotCrash)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:456
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (TokenBlacklist_NoAuditLogger_DoesNotCrash): n/a

#### `TEST_F(AuthAuditLoggerTest, TokenBlacklist_RevokeLogged)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:443
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (TokenBlacklist_RevokeLogged): n/a

#### `TEST_F(AuthAuditLoggerTest, TokenBlacklist_SetAuditLogger_AcceptsNull)`
- Source: `tests/auth/test_auth_audit_logger.cpp`:436
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthAuditLoggerTest): n/a
  - `<unnamed>` (TokenBlacklist_SetAuditLogger_AcceptsNull): n/a

### test_auth_distributed_blacklist.cpp

#### `TEST_F(DistributedTokenBlacklistTest, DBL01_AddAndIsRevoked)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:91
- Brief: Adding a JTI causes isRevoked() to return true for that JTI.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL01_AddAndIsRevoked): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL02_FutureExpiryRemainsRevoked)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:109
- Brief: A JTI with a future expiry is reported as revoked immediately after add() and remains revoked as long as the expiry has not elapsed.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL02_FutureExpiryRemainsRevoked): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL03_UnknownJtiNotRevoked)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:129
- Brief: An unknown JTI must not be considered revoked.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL03_UnknownJtiNotRevoked): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL04_ExpiredEntryNotRevoked)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:148
- Brief: A JTI whose expiry is in the past should not be reported as revoked.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL04_ExpiredEntryNotRevoked): n/a
- Details: The implementation stores the expiry time and checks it against the current time, so a past expiry causes isRevoked() to return false even if the entry is still physically present in RocksDB.

#### `TEST_F(DistributedTokenBlacklistTest, DBL05_PurgeExpiredRemovesOldEntries)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:169
- Brief: purgeExpired() removes entries with past expiry from RocksDB. After purging, isRevoked() continues to return false (expired entries are already false before purging; purge just frees storage).
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL05_PurgeExpiredRemovesOldEntries): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL06_MultipleJtisAreIndependent)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:198
- Brief: Each JTI has an independent revocation entry; revoking one JTI does not affect others.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL06_MultipleJtisAreIndependent): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL07_ConcurrentAccessIsThreadSafe)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:226
- Brief: Concurrent writers and readers must not produce data races or crashes. After all writers finish, all added JTIs must be reported as revoked.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL07_ConcurrentAccessIsThreadSafe): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL08_ReAddIsIdempotent)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:286
- Brief: add() for an already-revoked JTI must not crash or corrupt state. The entry remains revoked; last-write-wins semantics are acceptable.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL08_ReAddIsIdempotent): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL09_SingleNodeBecomesLeader)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:309
- Brief: A node with no peers must elect itself as leader.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL09_SingleNodeBecomesLeader): n/a
- Details: Leader election uses node-ID ordering: a node becomes leader when no peer has a strictly smaller node_id. With no peers, the node is always leader.

#### `TEST_F(DistributedTokenBlacklistTest, DBL10_LowestNodeIdBecomesLeader)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:333
- Brief: When peers have lower node_ids, the local node must NOT be leader. When the local node_id is the smallest, it MUST be leader.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL10_LowestNodeIdBecomesLeader): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL11_IsLeaderReflectsElectionResult)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:389
- Brief: Before any sync, the default leadership state is false (not yet elected). After syncWithCluster() the state updates based on the election result.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL11_IsLeaderReflectsElectionResult): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL12_SyncWithClusterReturnsFuture)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:411
- Brief: syncWithCluster() must return a std::future<bool> that resolves to true (success) in single-node / no-peer mode.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL12_SyncWithClusterReturnsFuture): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL13_WaitConvergenceImmediateInSingleNode)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:430
- Brief: In single-node mode (enable_cluster_sync=false), there are no peers to converge with, so waitForClusterConvergence() must return true immediately without blocking.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL13_WaitConvergenceImmediateInSingleNode): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL14_WaitConvergenceTimesOut)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:453
- Brief: When cluster sync is enabled and peers are configured but unreachable, waitForClusterConvergence() must return false after the timeout instead of blocking indefinitely.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL14_WaitConvergenceTimesOut): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL15_InitialStatsAreZero)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:490
- Brief: Before any sync activity, all replication counters must be zero.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL15_InitialStatsAreZero): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL16_ConfigAccessorReturnsConstructedConfig)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:508
- Brief: config() must return the exact configuration passed to the constructor.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL16_ConfigAccessorReturnsConstructedConfig): n/a

#### `TEST_F(DistributedTokenBlacklistTest, DBL17_DestructorIsClean)`
- Source: `tests/auth/test_auth_distributed_blacklist.cpp`:536
- Brief: Creating and immediately destroying a DistributedTokenBlacklist must not crash, throw, or leave background threads running.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (DBL17_DestructorIsClean): n/a
- Details: Tests both single-node and cluster-sync-enabled configurations.

### test_auth_error.cpp

#### `TEST(AuthErrorTest, AuthException)`
- Source: `tests/auth/test_auth_error.cpp`:189
- Brief: Test AuthException throwing and catching.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (AuthException): n/a

#### `TEST(AuthErrorTest, BasicCreation)`
- Source: `tests/auth/test_auth_error.cpp`:10
- Brief: Test basic AuthError creation.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (BasicCreation): n/a

#### `TEST(AuthErrorTest, CustomRequestId)`
- Source: `tests/auth/test_auth_error.cpp`:26
- Brief: Test AuthError with custom request ID.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (CustomRequestId): n/a

#### `TEST(AuthErrorTest, ErrorCodeConversion)`
- Source: `tests/auth/test_auth_error.cpp`:209
- Brief: Test error code conversion.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (ErrorCodeConversion): n/a

#### `TEST(AuthErrorTest, ErrorCodeRange)`
- Source: `tests/auth/test_auth_error.cpp`:315
- Brief: Test all auth error codes are in valid range.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (ErrorCodeRange): n/a

#### `TEST(AuthErrorTest, ErrorLogging)`
- Source: `tests/auth/test_auth_error.cpp`:282
- Brief: Test error logging (just verify it doesn't crash).
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (ErrorLogging): n/a

#### `TEST(AuthErrorTest, ErrorRegistration)`
- Source: `tests/auth/test_auth_error.cpp`:232
- Brief: Test error registration.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (ErrorRegistration): n/a

#### `TEST(AuthErrorTest, FromException)`
- Source: `tests/auth/test_auth_error.cpp`:173
- Brief: Test AuthError from exception.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (FromException): n/a

#### `TEST(AuthErrorTest, InternalJSONSerialization)`
- Source: `tests/auth/test_auth_error.cpp`:63
- Brief: Test AuthError JSON serialization (internal).
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (InternalJSONSerialization): n/a

#### `TEST(AuthErrorTest, MaskEmail)`
- Source: `tests/auth/test_auth_error.cpp`:104
- Brief: Test email masking.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (MaskEmail): n/a

#### `TEST(AuthErrorTest, MaskFilePath)`
- Source: `tests/auth/test_auth_error.cpp`:130
- Brief: Test file path masking.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (MaskFilePath): n/a

#### `TEST(AuthErrorTest, MaskIPAddress)`
- Source: `tests/auth/test_auth_error.cpp`:143
- Brief: Test IP address masking.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (MaskIPAddress): n/a

#### `TEST(AuthErrorTest, MaskMultipleSensitiveData)`
- Source: `tests/auth/test_auth_error.cpp`:155
- Brief: Test multiple sensitive data types in one string.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (MaskMultipleSensitiveData): n/a

#### `TEST(AuthErrorTest, MaskPrincipal)`
- Source: `tests/auth/test_auth_error.cpp`:118
- Brief: Test Kerberos principal masking.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (MaskPrincipal): n/a

#### `TEST(AuthErrorTest, PublicJSONSerialization)`
- Source: `tests/auth/test_auth_error.cpp`:40
- Brief: Test AuthError JSON serialization (public).
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (PublicJSONSerialization): n/a

#### `TEST(AuthErrorTest, RequestIdGeneration)`
- Source: `tests/auth/test_auth_error.cpp`:217
- Brief: Test request ID generation.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (RequestIdGeneration): n/a

#### `TEST(AuthErrorTest, RetryAfter)`
- Source: `tests/auth/test_auth_error.cpp`:82
- Brief: Test retry-after functionality.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (RetryAfter): n/a

#### `TEST(AuthErrorTest, ThrowMacro)`
- Source: `tests/auth/test_auth_error.cpp`:247
- Brief: Test macro for throwing auth errors.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (ThrowMacro): n/a

#### `TEST(AuthErrorTest, ThrowMacroWithRequestId)`
- Source: `tests/auth/test_auth_error.cpp`:265
- Brief: Test macro with request ID.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (ThrowMacroWithRequestId): n/a

#### `TEST(AuthErrorTest, Timestamp)`
- Source: `tests/auth/test_auth_error.cpp`:297
- Brief: Test timestamp is set correctly.
- Parameters:
  - `<unnamed>` (AuthErrorTest): n/a
  - `<unnamed>` (Timestamp): n/a

### test_auth_hardening_revocation_federation.cpp

#### `TEST_F(FederationHardeningTest, FED01_UnknownRealmThrowsFederationCode)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:235
- Brief: FED-01: validateToken for a token with an unknown issuer throws FEDERATION_UNKNOWN_REALM (not JWT_ISSUER_MISMATCH).
- Parameters:
  - `<unnamed>` (FederationHardeningTest): n/a
  - `<unnamed>` (FED01_UnknownRealmThrowsFederationCode): n/a
- Details: Phase 2/3 hardened the error classification: callers can now distinguish "no such realm" from "realm exists but token is invalid".

#### `TEST_F(FederationHardeningTest, FED02_DuplicateRealmThrowsConfigError)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:258
- Brief: FED-02: Registering the same issuer URL twice throws AUTH_CONFIG_INVALID.
- Parameters:
  - `<unnamed>` (FederationHardeningTest): n/a
  - `<unnamed>` (FED02_DuplicateRealmThrowsConfigError): n/a

#### `TEST_F(FederationHardeningTest, FED03_MalformedTokenThrowsInvalidFormat)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:276
- Brief: FED-03: A token that is not a valid JWT (no dot separators) throws JWT_INVALID_FORMAT before any realm lookup.
- Parameters:
  - `<unnamed>` (FederationHardeningTest): n/a
  - `<unnamed>` (FED03_MalformedTokenThrowsInvalidFormat): n/a

#### `TEST_F(FederationHardeningTest, FED06_RealmProviderUnknownThrows)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:288
- Brief: FED-06: realmProvider for an unknown issuer throws FEDERATION_UNKNOWN_REALM.
- Parameters:
  - `<unnamed>` (FederationHardeningTest): n/a
  - `<unnamed>` (FED06_RealmProviderUnknownThrows): n/a

#### `TEST_F(FederationHardeningTest, FED07_MultipleRealmsCoexist)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:300
- Brief: FED-07: Multiple realms coexist; realmCount() reflects additions.
- Parameters:
  - `<unnamed>` (FederationHardeningTest): n/a
  - `<unnamed>` (FED07_MultipleRealmsCoexist): n/a

#### `TEST_F(FederationHardeningTest, FED08_RealmCountIncrements)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:318
- Brief: FED-08: realmCount() starts at 0 and increments correctly.
- Parameters:
  - `<unnamed>` (FederationHardeningTest): n/a
  - `<unnamed>` (FED08_RealmCountIncrements): n/a

#### `TEST_F(RevocationHardeningTest, RFP01_AddIsRevokedRoundTrip)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:107
- Brief: RFP-01: Basic add / isRevoked round-trip after Phase 2/3 hardening.
- Parameters:
  - `<unnamed>` (RevocationHardeningTest): n/a
  - `<unnamed>` (RFP01_AddIsRevokedRoundTrip): n/a

#### `TEST_F(RevocationHardeningTest, RFP02_EmptyJtiRejected)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:120
- Brief: RFP-02: Empty JTI is rejected with REVOCATION_ENTRY_INVALID.
- Parameters:
  - `<unnamed>` (RevocationHardeningTest): n/a
  - `<unnamed>` (RFP02_EmptyJtiRejected): n/a
- Details: An empty JTI cannot meaningfully identify a token; accepting it would allow silent revocation of every token that lacks a jti claim.

#### `TEST_F(RevocationHardeningTest, RFP03_OversizedJtiRejected)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:138
- Brief: RFP-03: JTI exceeding kMaxJtiBytes (1024) is rejected with REVOCATION_ENTRY_INVALID.
- Parameters:
  - `<unnamed>` (RevocationHardeningTest): n/a
  - `<unnamed>` (RFP03_OversizedJtiRejected): n/a

#### `TEST_F(RevocationHardeningTest, RFP04_PurgeExpiredSelectiveRemoval)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:152
- Brief: RFP-04: purgeExpired removes expired entries; non-expired entries survive.
- Parameters:
  - `<unnamed>` (RevocationHardeningTest): n/a
  - `<unnamed>` (RFP04_PurgeExpiredSelectiveRemoval): n/a

#### `TEST_F(RevocationHardeningTest, RFP05_ConcurrentAddIsRevoked)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:164
- Brief: RFP-05: Concurrent add() and isRevoked() are thread-safe.
- Parameters:
  - `<unnamed>` (RevocationHardeningTest): n/a
  - `<unnamed>` (RFP05_ConcurrentAddIsRevoked): n/a

#### `TEST_F(RevocationHardeningTest, RFP06_IdempotentReAdd)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:194
- Brief: RFP-06: Re-adding the same JTI is idempotent (no exception, LWW semantics).
- Parameters:
  - `<unnamed>` (RevocationHardeningTest): n/a
  - `<unnamed>` (RFP06_IdempotentReAdd): n/a

#### `TEST_F(RevocationHardeningTest, RFP07_UnknownJtiIsNotRevoked)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:204
- Brief: RFP-07: isRevoked() returns false for a JTI that was never added.
- Parameters:
  - `<unnamed>` (RevocationHardeningTest): n/a
  - `<unnamed>` (RFP07_UnknownJtiIsNotRevoked): n/a

#### `TEST_F(RevocationHardeningTest, RFP08_ExpiredEntryNotRevoked)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:212
- Brief: RFP-08: After an entry expires and purgeExpired runs, isRevoked() returns false.
- Parameters:
  - `<unnamed>` (RevocationHardeningTest): n/a
  - `<unnamed>` (RFP08_ExpiredEntryNotRevoked): n/a

#### `TEST_F(SessionHardeningTest, ASY01_EmptyUserIdThrows)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:347
- Brief: ASY-01: createSession with empty user_id throws std::invalid_argument.
- Parameters:
  - `<unnamed>` (SessionHardeningTest): n/a
  - `<unnamed>` (ASY01_EmptyUserIdThrows): n/a
- Details: The contract requires user_id to be non-empty; an empty subject would produce an anonymous session with no deterministic ownership.

#### `TEST_F(SessionHardeningTest, ASY02_UnknownSessionReturnsFalse)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:359
- Brief: ASY-02: validateSession for a completely unknown session ID returns valid=false without throwing.
- Parameters:
  - `<unnamed>` (SessionHardeningTest): n/a
  - `<unnamed>` (ASY02_UnknownSessionReturnsFalse): n/a

#### `TEST_F(SessionHardeningTest, ASY03_ExpiredSessionReturnsFalse)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:370
- Brief: ASY-03: validateSession for a session with an expired absolute timeout returns valid=false with a populated reason string.
- Parameters:
  - `<unnamed>` (SessionHardeningTest): n/a
  - `<unnamed>` (ASY03_ExpiredSessionReturnsFalse): n/a

#### `TEST_F(SessionHardeningTest, ASY04_TerminateUnknownIsIdempotent)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:388
- Brief: ASY-04: terminateSession for a non-existent session ID is idempotent.
- Parameters:
  - `<unnamed>` (SessionHardeningTest): n/a
  - `<unnamed>` (ASY04_TerminateUnknownIsIdempotent): n/a

#### `TEST_F(SessionHardeningTest, ASY05_TerminateAllOtherSessions)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:398
- Brief: ASY-05: terminateAllOtherSessions removes non-kept sessions and preserves the specified keep session.
- Parameters:
  - `<unnamed>` (SessionHardeningTest): n/a
  - `<unnamed>` (ASY05_TerminateAllOtherSessions): n/a

#### `TEST_F(SessionHardeningTest, ASY06_SessionIdPrefixInvariant)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:422
- Brief: ASY-06: All session IDs created by createSession() start with "sess_".
- Parameters:
  - `<unnamed>` (SessionHardeningTest): n/a
  - `<unnamed>` (ASY06_SessionIdPrefixInvariant): n/a
- Details: The "sess_" prefix is part of the public contract: downstream components that route or filter session tokens rely on this prefix.

#### `TEST_F(SessionHardeningTest, ASY07_PruneExpiredSelectiveRemoval)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:434
- Brief: ASY-07: pruneExpired removes expired sessions; active sessions are unaffected.
- Parameters:
  - `<unnamed>` (SessionHardeningTest): n/a
  - `<unnamed>` (ASY07_PruneExpiredSelectiveRemoval): n/a

#### `TEST_F(SessionHardeningTest, ASY08_PerUserSessionLimitEnforced)`
- Source: `tests/auth/test_auth_hardening_revocation_federation.cpp`:466
- Brief: ASY-08: Per-user session limit is enforced; creating a session beyond the limit evicts the oldest session for that user.
- Parameters:
  - `<unnamed>` (SessionHardeningTest): n/a
  - `<unnamed>` (ASY08_PerUserSessionLimitEnforced): n/a

### test_auth_highcardinality_stress.cpp

#### `TEST(WaveD_AuthStress, ConcurrentProviderDegradedStress)`
- Source: `tests/auth/test_auth_highcardinality_stress.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_AuthStress): n/a
  - `<unnamed>` (ConcurrentProviderDegradedStress): n/a

#### `TEST(WaveD_AuthStress, FederationMatrixStress)`
- Source: `tests/auth/test_auth_highcardinality_stress.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_AuthStress): n/a
  - `<unnamed>` (FederationMatrixStress): n/a

#### `TEST(WaveD_AuthStress, HighCardinalityTokenValidation)`
- Source: `tests/auth/test_auth_highcardinality_stress.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_AuthStress): n/a
  - `<unnamed>` (HighCardinalityTokenValidation): n/a

### test_auth_input_validation.cpp

#### `TEST(AuthInputValidationTest, GSSAPI_ConfigurationValidation)`
- Source: `tests/auth/test_auth_input_validation.cpp`:80
- Brief: Test GSSAPI configuration validation.
- Parameters:
  - `<unnamed>` (AuthInputValidationTest): n/a
  - `<unnamed>` (GSSAPI_ConfigurationValidation): n/a

#### `TEST(AuthInputValidationTest, GSSAPI_Constants)`
- Source: `tests/auth/test_auth_input_validation.cpp`:96
- Brief: Test GSSAPI constants are defined correctly.
- Parameters:
  - `<unnamed>` (AuthInputValidationTest): n/a
  - `<unnamed>` (GSSAPI_Constants): n/a

#### `TEST(AuthInputValidationTest, GSSAPI_ContextTimeout)`
- Source: `tests/auth/test_auth_input_validation.cpp`:106
- Brief: Test GSSAPI context timeout configuration.
- Parameters:
  - `<unnamed>` (AuthInputValidationTest): n/a
  - `<unnamed>` (GSSAPI_ContextTimeout): n/a

#### `TEST(AuthInputValidationTest, JWT_CustomTimeout)`
- Source: `tests/auth/test_auth_input_validation.cpp`:57
- Brief: Test JWT validator configuration with custom timeout.
- Parameters:
  - `<unnamed>` (AuthInputValidationTest): n/a
  - `<unnamed>` (JWT_CustomTimeout): n/a

#### `TEST(AuthInputValidationTest, JWT_EmptyToken)`
- Source: `tests/auth/test_auth_input_validation.cpp`:30
- Brief: Test JWT empty token validation.
- Parameters:
  - `<unnamed>` (AuthInputValidationTest): n/a
  - `<unnamed>` (JWT_EmptyToken): n/a

#### `TEST(AuthInputValidationTest, JWT_PrincipalLengthLimit)`
- Source: `tests/auth/test_auth_input_validation.cpp`:47
- Brief: Test JWT principal/subject length validation.
- Parameters:
  - `<unnamed>` (AuthInputValidationTest): n/a
  - `<unnamed>` (JWT_PrincipalLengthLimit): n/a

#### `TEST(AuthInputValidationTest, JWT_RetryConfiguration)`
- Source: `tests/auth/test_auth_input_validation.cpp`:120
- Brief: Test JWT validator retry configuration.
- Parameters:
  - `<unnamed>` (AuthInputValidationTest): n/a
  - `<unnamed>` (JWT_RetryConfiguration): n/a

#### `TEST(AuthInputValidationTest, JWT_TokenSizeLimit)`
- Source: `tests/auth/test_auth_input_validation.cpp`:11
- Brief: Test JWT token size validation.
- Parameters:
  - `<unnamed>` (AuthInputValidationTest): n/a
  - `<unnamed>` (JWT_TokenSizeLimit): n/a

### test_auth_metrics.cpp

#### `TEST(AuthMetricsTest, AccountLockoutMetrics)`
- Source: `tests/auth/test_auth_metrics.cpp`:112
- Brief: Test account lockout metrics.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (AccountLockoutMetrics): n/a

#### `TEST(AuthMetricsTest, AuthMethodSeparation)`
- Source: `tests/auth/test_auth_metrics.cpp`:286
- Brief: Test different auth methods are tracked separately.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (AuthMethodSeparation): n/a

#### `TEST(AuthMetricsTest, BasicRecording)`
- Source: `tests/auth/test_auth_metrics.cpp`:11
- Brief: Test basic metrics recording without Prometheus.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (BasicRecording): n/a

#### `TEST(AuthMetricsTest, CustomConfig)`
- Source: `tests/auth/test_auth_metrics.cpp`:270
- Brief: Test metrics with custom config.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (CustomConfig): n/a

#### `TEST(AuthMetricsTest, DurationTimer)`
- Source: `tests/auth/test_auth_metrics.cpp`:213
- Brief: Test AuthDurationTimer RAII helper.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (DurationTimer): n/a

#### `TEST(AuthMetricsTest, DurationTimerAutoRecord)`
- Source: `tests/auth/test_auth_metrics.cpp`:237
- Brief: Test AuthDurationTimer automatic recording on destruction.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (DurationTimerAutoRecord): n/a

#### `TEST(AuthMetricsTest, DurationTimerNoDoubleRecord)`
- Source: `tests/auth/test_auth_metrics.cpp`:253
- Brief: Test AuthDurationTimer prevents double recording.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (DurationTimerNoDoubleRecord): n/a

#### `TEST(AuthMetricsTest, ErrorRecording)`
- Source: `tests/auth/test_auth_metrics.cpp`:123
- Brief: Test error recording.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (ErrorRecording): n/a

#### `TEST(AuthMetricsTest, JWKSCacheMetrics)`
- Source: `tests/auth/test_auth_metrics.cpp`:77
- Brief: Test JWKS cache metrics.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (JWKSCacheMetrics): n/a

#### `TEST(AuthMetricsTest, JWKSFetchMetrics)`
- Source: `tests/auth/test_auth_metrics.cpp`:91
- Brief: Test JWKS fetch metrics.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (JWKSFetchMetrics): n/a

#### `TEST(AuthMetricsTest, LargeValues)`
- Source: `tests/auth/test_auth_metrics.cpp`:313
- Brief: Test large values don't overflow.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (LargeValues): n/a

#### `TEST(AuthMetricsTest, MultipleAuthMethods)`
- Source: `tests/auth/test_auth_metrics.cpp`:63
- Brief: Test multiple auth methods.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (MultipleAuthMethods): n/a

#### `TEST(AuthMetricsTest, RateLimitingMetrics)`
- Source: `tests/auth/test_auth_metrics.cpp`:101
- Brief: Test rate limiting metrics.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (RateLimitingMetrics): n/a

#### `TEST(AuthMetricsTest, RecordAuthFailure)`
- Source: `tests/auth/test_auth_metrics.cpp`:49
- Brief: Test auth failure recording.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (RecordAuthFailure): n/a

#### `TEST(AuthMetricsTest, RecordAuthSuccess)`
- Source: `tests/auth/test_auth_metrics.cpp`:35
- Brief: Test auth success recording.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (RecordAuthSuccess): n/a

#### `TEST(AuthMetricsTest, RevokedTokenChecks)`
- Source: `tests/auth/test_auth_metrics.cpp`:145
- Brief: Test revoked token checks.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (RevokedTokenChecks): n/a

#### `TEST(AuthMetricsTest, SuccessRateCalculation)`
- Source: `tests/auth/test_auth_metrics.cpp`:155
- Brief: Test success rate calculation.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (SuccessRateCalculation): n/a

#### `TEST(AuthMetricsTest, ThreadSafety)`
- Source: `tests/auth/test_auth_metrics.cpp`:178
- Brief: Test thread safety of counters.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (ThreadSafety): n/a

#### `TEST(AuthMetricsTest, TimerGetDuration)`
- Source: `tests/auth/test_auth_metrics.cpp`:327
- Brief: Test getDuration in timer.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (TimerGetDuration): n/a

#### `TEST(AuthMetricsTest, TokenValidationMetrics)`
- Source: `tests/auth/test_auth_metrics.cpp`:135
- Brief: Test token validation metrics.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (TokenValidationMetrics): n/a

#### `TEST(AuthMetricsTest, ZeroDuration)`
- Source: `tests/auth/test_auth_metrics.cpp`:303
- Brief: Test zero duration is handled correctly.
- Parameters:
  - `<unnamed>` (AuthMetricsTest): n/a
  - `<unnamed>` (ZeroDuration): n/a

### test_auth_middleware.cpp

#### `TEST(AuthMiddlewareGap013Test, Authorize_InvalidToken_ReturnsDenied)`
- Source: `tests/auth/test_auth_middleware.cpp`:1185
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareGap013Test): n/a
  - `<unnamed>` (Authorize_InvalidToken_ReturnsDenied): n/a

#### `TEST(AuthMiddlewareGap013Test, Authorize_WrongScope_ReturnsDenied)`
- Source: `tests/auth/test_auth_middleware.cpp`:1172
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareGap013Test): n/a
  - `<unnamed>` (Authorize_WrongScope_ReturnsDenied): n/a

#### `TEST(AuthMiddlewareGap013Test, ConcurrentDenyRequests_NoCrossContamination)`
- Source: `tests/auth/test_auth_middleware.cpp`:1250
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareGap013Test): n/a
  - `<unnamed>` (ConcurrentDenyRequests_NoCrossContamination): n/a

#### `TEST(AuthMiddlewareGap013Test, DeniedReason_DoesNotEchoPresentedToken)`
- Source: `tests/auth/test_auth_middleware.cpp`:1197
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareGap013Test): n/a
  - `<unnamed>` (DeniedReason_DoesNotEchoPresentedToken): n/a

#### `TEST(AuthMiddlewareGap013Test, InsufficientScope_ReasonDoesNotEchoToken)`
- Source: `tests/auth/test_auth_middleware.cpp`:1213
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareGap013Test): n/a
  - `<unnamed>` (InsufficientScope_ReasonDoesNotEchoToken): n/a

#### `TEST(AuthMiddlewareGap013Test, ValidateToken_ReasonDoesNotEchoToken)`
- Source: `tests/auth/test_auth_middleware.cpp`:1232
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareGap013Test): n/a
  - `<unnamed>` (ValidateToken_ReasonDoesNotEchoToken): n/a

#### `TEST(AuthMiddlewareGap013Test, ValidateToken_UnknownToken_ReturnsDenied)`
- Source: `tests/auth/test_auth_middleware.cpp`:1157
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareGap013Test): n/a
  - `<unnamed>` (ValidateToken_UnknownToken_ReturnsDenied): n/a

#### `TEST(JWTClaimsScopesTest, ScopesFieldExistsAndDefaultsEmpty)`
- Source: `tests/auth/test_auth_middleware.cpp`:979
- Brief: n/a
- Parameters:
  - `<unnamed>` (JWTClaimsScopesTest): n/a
  - `<unnamed>` (ScopesFieldExistsAndDefaultsEmpty): n/a

#### `TEST(RoleScopeMappingTest, SetRoleScopeMappingConfiguresMapping)`
- Source: `tests/auth/test_auth_middleware.cpp`:1083
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoleScopeMappingTest): n/a
  - `<unnamed>` (SetRoleScopeMappingConfiguresMapping): n/a

#### `TEST(TaskSchedulerRequestContext, ClearResetsToDefault)`
- Source: `tests/auth/test_auth_middleware.cpp`:1122
- Brief: n/a
- Parameters:
  - `<unnamed>` (TaskSchedulerRequestContext): n/a
  - `<unnamed>` (ClearResetsToDefault): n/a

#### `TEST(TaskSchedulerRequestContext, ContextIsPerThread)`
- Source: `tests/auth/test_auth_middleware.cpp`:1129
- Brief: n/a
- Parameters:
  - `<unnamed>` (TaskSchedulerRequestContext): n/a
  - `<unnamed>` (ContextIsPerThread): n/a

#### `TEST(TaskSchedulerRequestContext, DefaultIsSystemUser)`
- Source: `tests/auth/test_auth_middleware.cpp`:1109
- Brief: n/a
- Parameters:
  - `<unnamed>` (TaskSchedulerRequestContext): n/a
  - `<unnamed>` (DefaultIsSystemUser): n/a

#### `TEST(TaskSchedulerRequestContext, FallbackParameterUsed)`
- Source: `tests/auth/test_auth_middleware.cpp`:1144
- Brief: n/a
- Parameters:
  - `<unnamed>` (TaskSchedulerRequestContext): n/a
  - `<unnamed>` (FallbackParameterUsed): n/a

#### `TEST(TaskSchedulerRequestContext, SetContextReturnsCorrectValues)`
- Source: `tests/auth/test_auth_middleware.cpp`:1115
- Brief: n/a
- Parameters:
  - `<unnamed>` (TaskSchedulerRequestContext): n/a
  - `<unnamed>` (SetContextReturnsCorrectValues): n/a

#### `TEST_F(ApiKeyMiddlewareTest, Authorize_MultipleScopes)`
- Source: `tests/auth/test_auth_middleware.cpp`:772
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMiddlewareTest): n/a
  - `<unnamed>` (Authorize_MultipleScopes): n/a

#### `TEST_F(ApiKeyMiddlewareTest, Authorize_ScopeMissing)`
- Source: `tests/auth/test_auth_middleware.cpp`:766
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMiddlewareTest): n/a
  - `<unnamed>` (Authorize_ScopeMissing): n/a

#### `TEST_F(ApiKeyMiddlewareTest, Authorize_ScopePresent)`
- Source: `tests/auth/test_auth_middleware.cpp`:760
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMiddlewareTest): n/a
  - `<unnamed>` (Authorize_ScopePresent): n/a

#### `TEST_F(ApiKeyMiddlewareTest, ExpiredKey_Rejected)`
- Source: `tests/auth/test_auth_middleware.cpp`:795
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMiddlewareTest): n/a
  - `<unnamed>` (ExpiredKey_Rejected): n/a

#### `TEST_F(ApiKeyMiddlewareTest, InactiveKey_Rejected)`
- Source: `tests/auth/test_auth_middleware.cpp`:806
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMiddlewareTest): n/a
  - `<unnamed>` (InactiveKey_Rejected): n/a

#### `TEST_F(ApiKeyMiddlewareTest, IsEnabled_ApiKeyOnly)`
- Source: `tests/auth/test_auth_middleware.cpp`:787
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMiddlewareTest): n/a
  - `<unnamed>` (IsEnabled_ApiKeyOnly): n/a

#### `TEST_F(ApiKeyMiddlewareTest, RemoveCredential_RevokedKeyDenied)`
- Source: `tests/auth/test_auth_middleware.cpp`:781
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMiddlewareTest): n/a
  - `<unnamed>` (RemoveCredential_RevokedKeyDenied): n/a

#### `TEST_F(ApiKeyMiddlewareTest, Roles_PropagatedToAuthResult)`
- Source: `tests/auth/test_auth_middleware.cpp`:831
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMiddlewareTest): n/a
  - `<unnamed>` (Roles_PropagatedToAuthResult): n/a

#### `TEST_F(ApiKeyMiddlewareTest, StaticTokensCoexistWithApiKeyAuth)`
- Source: `tests/auth/test_auth_middleware.cpp`:817
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMiddlewareTest): n/a
  - `<unnamed>` (StaticTokensCoexistWithApiKeyAuth): n/a

#### `TEST_F(ApiKeyMiddlewareTest, ValidateToken_UnknownKeyId)`
- Source: `tests/auth/test_auth_middleware.cpp`:755
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMiddlewareTest): n/a
  - `<unnamed>` (ValidateToken_UnknownKeyId): n/a

#### `TEST_F(ApiKeyMiddlewareTest, ValidateToken_ValidCombinedKey)`
- Source: `tests/auth/test_auth_middleware.cpp`:742
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMiddlewareTest): n/a
  - `<unnamed>` (ValidateToken_ValidCombinedKey): n/a

#### `TEST_F(ApiKeyMiddlewareTest, ValidateToken_WrongSecret)`
- Source: `tests/auth/test_auth_middleware.cpp`:749
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApiKeyMiddlewareTest): n/a
  - `<unnamed>` (ValidateToken_WrongSecret): n/a

#### `TEST_F(AuthMiddlewareTest, AuthMiddlewareInHTTPPipeline)`
- Source: `tests/auth/test_auth_middleware.cpp`:550
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (AuthMiddlewareInHTTPPipeline): n/a

#### `TEST_F(AuthMiddlewareTest, Authorize_AdminHasAllScopes)`
- Source: `tests/auth/test_auth_middleware.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (Authorize_AdminHasAllScopes): n/a

#### `TEST_F(AuthMiddlewareTest, Authorize_InvalidToken)`
- Source: `tests/auth/test_auth_middleware.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (Authorize_InvalidToken): n/a

#### `TEST_F(AuthMiddlewareTest, Authorize_ReadonlyLimitedScopes)`
- Source: `tests/auth/test_auth_middleware.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (Authorize_ReadonlyLimitedScopes): n/a

#### `TEST_F(AuthMiddlewareTest, BearerTokenExtraction)`
- Source: `tests/auth/test_auth_middleware.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (BearerTokenExtraction): n/a

#### `TEST_F(AuthMiddlewareTest, ClearTokens)`
- Source: `tests/auth/test_auth_middleware.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (ClearTokens): n/a

#### `TEST_F(AuthMiddlewareTest, ConcurrentSessions)`
- Source: `tests/auth/test_auth_middleware.cpp`:421
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (ConcurrentSessions): n/a

#### `TEST_F(AuthMiddlewareTest, DeniedAccessResponse)`
- Source: `tests/auth/test_auth_middleware.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (DeniedAccessResponse): n/a

#### `TEST_F(AuthMiddlewareTest, ErrorResponseFormats)`
- Source: `tests/auth/test_auth_middleware.cpp`:644
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (ErrorResponseFormats): n/a

#### `TEST_F(AuthMiddlewareTest, ExpiredTokenRejected)`
- Source: `tests/auth/test_auth_middleware.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (ExpiredTokenRejected): n/a

#### `TEST_F(AuthMiddlewareTest, ExtractBearerToken)`
- Source: `tests/auth/test_auth_middleware.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (ExtractBearerToken): n/a

#### `TEST_F(AuthMiddlewareTest, InvalidSignatureDetected)`
- Source: `tests/auth/test_auth_middleware.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (InvalidSignatureDetected): n/a

#### `TEST_F(AuthMiddlewareTest, MalformedTokenRejected)`
- Source: `tests/auth/test_auth_middleware.cpp`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (MalformedTokenRejected): n/a

#### `TEST_F(AuthMiddlewareTest, Metrics_TrackAuthAttempts)`
- Source: `tests/auth/test_auth_middleware.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (Metrics_TrackAuthAttempts): n/a

#### `TEST_F(AuthMiddlewareTest, MissingAuthHeaderHandling)`
- Source: `tests/auth/test_auth_middleware.cpp`:590
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (MissingAuthHeaderHandling): n/a

#### `TEST_F(AuthMiddlewareTest, MultipleHTTPMethodsAuth)`
- Source: `tests/auth/test_auth_middleware.cpp`:615
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (MultipleHTTPMethodsAuth): n/a

#### `TEST_F(AuthMiddlewareTest, PermissionCheckOnEndpoints)`
- Source: `tests/auth/test_auth_middleware.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (PermissionCheckOnEndpoints): n/a

#### `TEST_F(AuthMiddlewareTest, RBACEnforcement)`
- Source: `tests/auth/test_auth_middleware.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (RBACEnforcement): n/a

#### `TEST_F(AuthMiddlewareTest, RateLimitingOnFailedAuth)`
- Source: `tests/auth/test_auth_middleware.cpp`:501
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (RateLimitingOnFailedAuth): n/a

#### `TEST_F(AuthMiddlewareTest, RemoveToken)`
- Source: `tests/auth/test_auth_middleware.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (RemoveToken): n/a

#### `TEST_F(AuthMiddlewareTest, ResourceLevelAuthorization)`
- Source: `tests/auth/test_auth_middleware.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (ResourceLevelAuthorization): n/a

#### `TEST_F(AuthMiddlewareTest, SQLInjectionPrevention)`
- Source: `tests/auth/test_auth_middleware.cpp`:454
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (SQLInjectionPrevention): n/a

#### `TEST_F(AuthMiddlewareTest, SessionManagement)`
- Source: `tests/auth/test_auth_middleware.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (SessionManagement): n/a

#### `TEST_F(AuthMiddlewareTest, TenantExtraction)`
- Source: `tests/auth/test_auth_middleware.cpp`:675
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (TenantExtraction): n/a

#### `TEST_F(AuthMiddlewareTest, TenantInAuthContext)`
- Source: `tests/auth/test_auth_middleware.cpp`:688
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (TenantInAuthContext): n/a

#### `TEST_F(AuthMiddlewareTest, TokenRefreshMechanism)`
- Source: `tests/auth/test_auth_middleware.cpp`:346
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (TokenRefreshMechanism): n/a

#### `TEST_F(AuthMiddlewareTest, TokenReplayAttackPrevention)`
- Source: `tests/auth/test_auth_middleware.cpp`:478
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (TokenReplayAttackPrevention): n/a

#### `TEST_F(AuthMiddlewareTest, TokenWithoutTenant)`
- Source: `tests/auth/test_auth_middleware.cpp`:699
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (TokenWithoutTenant): n/a

#### `TEST_F(AuthMiddlewareTest, ValidJWTTokenAccepted)`
- Source: `tests/auth/test_auth_middleware.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (ValidJWTTokenAccepted): n/a

#### `TEST_F(AuthMiddlewareTest, ValidateToken_Invalid)`
- Source: `tests/auth/test_auth_middleware.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (ValidateToken_Invalid): n/a

#### `TEST_F(AuthMiddlewareTest, ValidateToken_Valid)`
- Source: `tests/auth/test_auth_middleware.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (ValidateToken_Valid): n/a

#### `TEST_F(AuthMiddlewareTest, mTLSCertificateValidation)`
- Source: `tests/auth/test_auth_middleware.cpp`:522
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthMiddlewareTest): n/a
  - `<unnamed>` (mTLSCertificateValidation): n/a

#### `TEST_F(JWTScopeEnforcementTest, DirectScopeClaimTakesPrecedenceOverRoleMap)`
- Source: `tests/auth/test_auth_middleware.cpp`:1054
- Brief: n/a
- Parameters:
  - `<unnamed>` (JWTScopeEnforcementTest): n/a
  - `<unnamed>` (DirectScopeClaimTakesPrecedenceOverRoleMap): n/a

#### `TEST_F(JWTScopeEnforcementTest, EmptyRequiredScopeAlwaysPasses)`
- Source: `tests/auth/test_auth_middleware.cpp`:1018
- Brief: n/a
- Parameters:
  - `<unnamed>` (JWTScopeEnforcementTest): n/a
  - `<unnamed>` (EmptyRequiredScopeAlwaysPasses): n/a

#### `TEST_F(JWTScopeEnforcementTest, MissingScopeDenied)`
- Source: `tests/auth/test_auth_middleware.cpp`:1002
- Brief: n/a
- Parameters:
  - `<unnamed>` (JWTScopeEnforcementTest): n/a
  - `<unnamed>` (MissingScopeDenied): n/a

#### `TEST_F(JWTScopeEnforcementTest, NoScopeClaimDeniedWhenScopeRequired)`
- Source: `tests/auth/test_auth_middleware.cpp`:1010
- Brief: n/a
- Parameters:
  - `<unnamed>` (JWTScopeEnforcementTest): n/a
  - `<unnamed>` (NoScopeClaimDeniedWhenScopeRequired): n/a

#### `TEST_F(JWTScopeEnforcementTest, RoleMappingDeniesWhenScopeNotInRole)`
- Source: `tests/auth/test_auth_middleware.cpp`:1041
- Brief: n/a
- Parameters:
  - `<unnamed>` (JWTScopeEnforcementTest): n/a
  - `<unnamed>` (RoleMappingDeniesWhenScopeNotInRole): n/a

#### `TEST_F(JWTScopeEnforcementTest, RoleMappingGrantsScope)`
- Source: `tests/auth/test_auth_middleware.cpp`:1027
- Brief: n/a
- Parameters:
  - `<unnamed>` (JWTScopeEnforcementTest): n/a
  - `<unnamed>` (RoleMappingGrantsScope): n/a

#### `TEST_F(JWTScopeEnforcementTest, ScpArrayClaimParsed)`
- Source: `tests/auth/test_auth_middleware.cpp`:994
- Brief: n/a
- Parameters:
  - `<unnamed>` (JWTScopeEnforcementTest): n/a
  - `<unnamed>` (ScpArrayClaimParsed): n/a

#### `TEST_F(JWTScopeEnforcementTest, SetRoleScopeMappingOverridesLoadedConfig)`
- Source: `tests/auth/test_auth_middleware.cpp`:1068
- Brief: n/a
- Parameters:
  - `<unnamed>` (JWTScopeEnforcementTest): n/a
  - `<unnamed>` (SetRoleScopeMappingOverridesLoadedConfig): n/a

#### `TEST_F(JWTScopeEnforcementTest, SpaceSeparatedScopeClaimParsed)`
- Source: `tests/auth/test_auth_middleware.cpp`:986
- Brief: n/a
- Parameters:
  - `<unnamed>` (JWTScopeEnforcementTest): n/a
  - `<unnamed>` (SpaceSeparatedScopeClaimParsed): n/a

### test_auth_protocol_matrix_regression.cpp

#### `TEST(AuthProtocolMatrix, APM01_OIDCmTLS_RejectionDoesNotFallThrough)`
- Source: `tests/auth/test_auth_protocol_matrix_regression.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthProtocolMatrix): n/a
  - `<unnamed>` (APM01_OIDCmTLS_RejectionDoesNotFallThrough): n/a

#### `TEST(AuthProtocolMatrix, APM02_SAMLMFADegradation_AuditDecisionClassDistinct)`
- Source: `tests/auth/test_auth_protocol_matrix_regression.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthProtocolMatrix): n/a
  - `<unnamed>` (APM02_SAMLMFADegradation_AuditDecisionClassDistinct): n/a

#### `TEST(AuthProtocolMatrix, APM03_KerberosRevocation_RevokedJTIIsRejected)`
- Source: `tests/auth/test_auth_protocol_matrix_regression.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthProtocolMatrix): n/a
  - `<unnamed>` (APM03_KerberosRevocation_RevokedJTIIsRejected): n/a

#### `TEST(AuthProtocolMatrix, APM04_LDAPFederation_PoolExhaustionThrowsProviderDegraded)`
- Source: `tests/auth/test_auth_protocol_matrix_regression.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthProtocolMatrix): n/a
  - `<unnamed>` (APM04_LDAPFederation_PoolExhaustionThrowsProviderDegraded): n/a

#### `TEST(AuthProtocolMatrix, APM05_APIKeyRateLimiting_ExceedRateLimitReturnsFalse)`
- Source: `tests/auth/test_auth_protocol_matrix_regression.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthProtocolMatrix): n/a
  - `<unnamed>` (APM05_APIKeyRateLimiting_ExceedRateLimitReturnsFalse): n/a

#### `TEST(AuthProtocolMatrix, APM06_WebAuthnSession_SessionCreateValidateConsistent)`
- Source: `tests/auth/test_auth_protocol_matrix_regression.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthProtocolMatrix): n/a
  - `<unnamed>` (APM06_WebAuthnSession_SessionCreateValidateConsistent): n/a

#### `TEST(AuthProtocolMatrix, APM07_OAuthBlacklist_RevokedTokenIsRejected)`
- Source: `tests/auth/test_auth_protocol_matrix_regression.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthProtocolMatrix): n/a
  - `<unnamed>` (APM07_OAuthBlacklist_RevokedTokenIsRejected): n/a

#### `TEST(AuthProtocolMatrix, APM08_OIDCBlacklist_CacheClearEvictsAll)`
- Source: `tests/auth/test_auth_protocol_matrix_regression.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthProtocolMatrix): n/a
  - `<unnamed>` (APM08_OIDCBlacklist_CacheClearEvictsAll): n/a

#### `TEST(AuthProtocolMatrix, APM09_FederationUnknownRealm_FailClosedCorrectCode)`
- Source: `tests/auth/test_auth_protocol_matrix_regression.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthProtocolMatrix): n/a
  - `<unnamed>` (APM09_FederationUnknownRealm_FailClosedCorrectCode): n/a

#### `TEST(AuthProtocolMatrix, APM10_FederationEmptySub_AuditDecisionClassFederationIsCorrect)`
- Source: `tests/auth/test_auth_protocol_matrix_regression.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthProtocolMatrix): n/a
  - `<unnamed>` (APM10_FederationEmptySub_AuditDecisionClassFederationIsCorrect): n/a

#### `TEST(AuthProtocolMatrix, APM11_LDAPPool_CheckoutTimeoutThrowsProviderDegraded)`
- Source: `tests/auth/test_auth_protocol_matrix_regression.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthProtocolMatrix): n/a
  - `<unnamed>` (APM11_LDAPPool_CheckoutTimeoutThrowsProviderDegraded): n/a

#### `TEST(AuthProtocolMatrix, APM12_MultiRealmTrustIsolation_TrustRegistryEnforced)`
- Source: `tests/auth/test_auth_protocol_matrix_regression.cpp`:295
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthProtocolMatrix): n/a
  - `<unnamed>` (APM12_MultiRealmTrustIsolation_TrustRegistryEnforced): n/a

### test_auth_rate_limiter.cpp

#### `TEST(AccountLockoutManagerTest, BasicFunctionality)`
- Source: `tests/auth/test_auth_rate_limiter.cpp`:303
- Brief: Test account lockout manager directly.
- Parameters:
  - `<unnamed>` (AccountLockoutManagerTest): n/a
  - `<unnamed>` (BasicFunctionality): n/a

#### `TEST(AuthRateLimiterTest, AccountLockout)`
- Source: `tests/auth/test_auth_rate_limiter.cpp`:62
- Brief: Test account lockout after failed attempts.
- Parameters:
  - `<unnamed>` (AuthRateLimiterTest): n/a
  - `<unnamed>` (AccountLockout): n/a

#### `TEST(AuthRateLimiterTest, CombinedLimits)`
- Source: `tests/auth/test_auth_rate_limiter.cpp`:333
- Brief: Test combined IP and user rate limiting.
- Parameters:
  - `<unnamed>` (AuthRateLimiterTest): n/a
  - `<unnamed>` (CombinedLimits): n/a

#### `TEST(AuthRateLimiterTest, IPRateLimiting)`
- Source: `tests/auth/test_auth_rate_limiter.cpp`:11
- Brief: Test basic rate limiting for IP addresses.
- Parameters:
  - `<unnamed>` (AuthRateLimiterTest): n/a
  - `<unnamed>` (IPRateLimiting): n/a

#### `TEST(AuthRateLimiterTest, IPWhitelist)`
- Source: `tests/auth/test_auth_rate_limiter.cpp`:162
- Brief: Test IP whitelist bypasses rate limiting.
- Parameters:
  - `<unnamed>` (AuthRateLimiterTest): n/a
  - `<unnamed>` (IPWhitelist): n/a

#### `TEST(AuthRateLimiterTest, LockoutWindow)`
- Source: `tests/auth/test_auth_rate_limiter.cpp`:223
- Brief: Test lockout window behavior.
- Parameters:
  - `<unnamed>` (AuthRateLimiterTest): n/a
  - `<unnamed>` (LockoutWindow): n/a

#### `TEST(AuthRateLimiterTest, ManualUnlock)`
- Source: `tests/auth/test_auth_rate_limiter.cpp`:99
- Brief: Test manual account unlock.
- Parameters:
  - `<unnamed>` (AuthRateLimiterTest): n/a
  - `<unnamed>` (ManualUnlock): n/a

#### `TEST(AuthRateLimiterTest, Reset)`
- Source: `tests/auth/test_auth_rate_limiter.cpp`:274
- Brief: Test reset functionality.
- Parameters:
  - `<unnamed>` (AuthRateLimiterTest): n/a
  - `<unnamed>` (Reset): n/a

#### `TEST(AuthRateLimiterTest, RetryAfter)`
- Source: `tests/auth/test_auth_rate_limiter.cpp`:252
- Brief: Test retry-after header value.
- Parameters:
  - `<unnamed>` (AuthRateLimiterTest): n/a
  - `<unnamed>` (RetryAfter): n/a

#### `TEST(AuthRateLimiterTest, Statistics)`
- Source: `tests/auth/test_auth_rate_limiter.cpp`:189
- Brief: Test statistics tracking.
- Parameters:
  - `<unnamed>` (AuthRateLimiterTest): n/a
  - `<unnamed>` (Statistics): n/a

#### `TEST(AuthRateLimiterTest, SuccessfulAuthResetFailures)`
- Source: `tests/auth/test_auth_rate_limiter.cpp`:130
- Brief: Test successful auth resets failure count.
- Parameters:
  - `<unnamed>` (AuthRateLimiterTest): n/a
  - `<unnamed>` (SuccessfulAuthResetFailures): n/a

#### `TEST(AuthRateLimiterTest, UserRateLimiting)`
- Source: `tests/auth/test_auth_rate_limiter.cpp`:36
- Brief: Test per-user rate limiting.
- Parameters:
  - `<unnamed>` (AuthRateLimiterTest): n/a
  - `<unnamed>` (UserRateLimiting): n/a

### test_auth_rate_limiter_distributed.cpp

#### `TEST(AuthRateLimiterDistributedTest, DifferentIPsAreIndependent)`
- Source: `tests/auth/test_auth_rate_limiter_distributed.cpp`:212
- Brief: Verify that two limiters sharing a backend independently enforce limits for different IPs.
- Parameters:
  - `<unnamed>` (AuthRateLimiterDistributedTest): n/a
  - `<unnamed>` (DifferentIPsAreIndependent): n/a

#### `TEST(AuthRateLimiterDistributedTest, NoBackendFallsBackToTokenBucket)`
- Source: `tests/auth/test_auth_rate_limiter_distributed.cpp`:243
- Brief: Verify that a limiter with no backend set continues to use the existing in-process token-bucket behaviour (backward compatibility).
- Parameters:
  - `<unnamed>` (AuthRateLimiterDistributedTest): n/a
  - `<unnamed>` (NoBackendFallsBackToTokenBucket): n/a

#### `TEST(AuthRateLimiterDistributedTest, NullBackendRevertsToDefault)`
- Source: `tests/auth/test_auth_rate_limiter_distributed.cpp`:263
- Brief: Verify that setBackend(nullptr) reverts to the default token-bucket.
- Parameters:
  - `<unnamed>` (AuthRateLimiterDistributedTest): n/a
  - `<unnamed>` (NullBackendRevertsToDefault): n/a

#### `TEST(AuthRateLimiterDistributedTest, SharedBackendCombinedIPCount)`
- Source: `tests/auth/test_auth_rate_limiter_distributed.cpp`:150
- Brief: Two AuthRateLimiter instances sharing a backend see the combined IP request count, causing rate-limiting when the joint total exceeds the per-IP limit.
- Parameters:
  - `<unnamed>` (AuthRateLimiterDistributedTest): n/a
  - `<unnamed>` (SharedBackendCombinedIPCount): n/a
- Details: This mirrors the behaviour expected from two nodes sharing a RedisRateLimiterBackend: an attacker cannot bypass the per-IP limit by spreading requests across nodes.

#### `TEST(AuthRateLimiterDistributedTest, SharedBackendCombinedUserCount)`
- Source: `tests/auth/test_auth_rate_limiter_distributed.cpp`:181
- Brief: Two AuthRateLimiter instances sharing a backend see the combined per-user request count.
- Parameters:
  - `<unnamed>` (AuthRateLimiterDistributedTest): n/a
  - `<unnamed>` (SharedBackendCombinedUserCount): n/a

#### `TEST(InMemoryRateLimiterBackendTest, BasicIncrement)`
- Source: `tests/auth/test_auth_rate_limiter_distributed.cpp`:17
- Brief: Verify that increment() counts within the sliding window.
- Parameters:
  - `<unnamed>` (InMemoryRateLimiterBackendTest): n/a
  - `<unnamed>` (BasicIncrement): n/a

#### `TEST(InMemoryRateLimiterBackendTest, ConcurrentIncrements)`
- Source: `tests/auth/test_auth_rate_limiter_distributed.cpp`:84
- Brief: Verify thread-safety: concurrent increments from multiple threads produce the correct total count.
- Parameters:
  - `<unnamed>` (InMemoryRateLimiterBackendTest): n/a
  - `<unnamed>` (ConcurrentIncrements): n/a

#### `TEST(InMemoryRateLimiterBackendTest, GetCountIsReadOnly)`
- Source: `tests/auth/test_auth_rate_limiter_distributed.cpp`:31
- Brief: Verify that getCount() is read-only (does not modify counters).
- Parameters:
  - `<unnamed>` (InMemoryRateLimiterBackendTest): n/a
  - `<unnamed>` (GetCountIsReadOnly): n/a

#### `TEST(InMemoryRateLimiterBackendTest, Reset)`
- Source: `tests/auth/test_auth_rate_limiter_distributed.cpp`:45
- Brief: Verify that reset() clears the counter for the given key.
- Parameters:
  - `<unnamed>` (InMemoryRateLimiterBackendTest): n/a
  - `<unnamed>` (Reset): n/a

#### `TEST(InMemoryRateLimiterBackendTest, SlidingWindowExpiry)`
- Source: `tests/auth/test_auth_rate_limiter_distributed.cpp`:62
- Brief: Verify that entries older than the window are pruned automatically.
- Parameters:
  - `<unnamed>` (InMemoryRateLimiterBackendTest): n/a
  - `<unnamed>` (SlidingWindowExpiry): n/a

#### `TEST(RedisRateLimiterBackendNoRedisTest, UsesInProcessFallbackCounters)`
- Source: `tests/auth/test_auth_rate_limiter_distributed.cpp`:293
- Brief: Verify RedisRateLimiterBackend falls back to process-local in-memory counters when hiredis support is not compiled in.
- Parameters:
  - `<unnamed>` (RedisRateLimiterBackendNoRedisTest): n/a
  - `<unnamed>` (UsesInProcessFallbackCounters): n/a

#### `TEST(RedisRateLimiterBackendStubBridgeTest, BridgeCallbacksOverrideLocalFallback)`
- Source: `tests/auth/test_auth_rate_limiter_distributed.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (RedisRateLimiterBackendStubBridgeTest): n/a
  - `<unnamed>` (BridgeCallbacksOverrideLocalFallback): n/a

### test_auth_soak.cpp

#### `TEST(AuthSoak, SOAK01_TokenCacheInsertEvictCycle)`
- Source: `tests/auth/test_auth_soak.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthSoak): n/a
  - `<unnamed>` (SOAK01_TokenCacheInsertEvictCycle): n/a

#### `TEST(AuthSoak, SOAK02_BlacklistAddIsRevokedCycle)`
- Source: `tests/auth/test_auth_soak.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthSoak): n/a
  - `<unnamed>` (SOAK02_BlacklistAddIsRevokedCycle): n/a

#### `TEST(AuthSoak, SOAK03_SessionCreateInvalidateCycle)`
- Source: `tests/auth/test_auth_soak.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthSoak): n/a
  - `<unnamed>` (SOAK03_SessionCreateInvalidateCycle): n/a

#### `TEST(AuthSoak, SOAK04_ProviderDegradationSimulationAndRecovery)`
- Source: `tests/auth/test_auth_soak.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthSoak): n/a
  - `<unnamed>` (SOAK04_ProviderDegradationSimulationAndRecovery): n/a

### test_wave4b_auth_hardening.cpp

#### `TEST(Wave4BCoseAlg, C1_EC2WithDisallowedAlgRejected)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:435
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BCoseAlg): n/a
  - `<unnamed>` (C1_EC2WithDisallowedAlgRejected): n/a

#### `TEST(Wave4BCoseAlg, C1_RSAWithDisallowedAlgRejected)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:467
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BCoseAlg): n/a
  - `<unnamed>` (C1_RSAWithDisallowedAlgRejected): n/a

#### `TEST(Wave4BCoseAlg, C3_RSAKeyTooShortRejected)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:552
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BCoseAlg): n/a
  - `<unnamed>` (C3_RSAKeyTooShortRejected): n/a

#### `TEST(Wave4BHTTPRetry, B3_PKCEFlowRetriesOn503)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:309
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BHTTPRetry): n/a
  - `<unnamed>` (B3_PKCEFlowRetriesOn503): n/a

#### `TEST(Wave4BHTTPRetry, B4_DeviceFlowRetriesOn503)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:339
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BHTTPRetry): n/a
  - `<unnamed>` (B4_DeviceFlowRetriesOn503): n/a

#### `TEST(Wave4BLDAPRetry, B1_CheckoutReturnsNullWhenNoLDAPSupport)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BLDAPRetry): n/a
  - `<unnamed>` (B1_CheckoutReturnsNullWhenNoLDAPSupport): n/a

#### `TEST(Wave4BMTLSHardening, C2_MissingEKURejected)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:501
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BMTLSHardening): n/a
  - `<unnamed>` (C2_MissingEKURejected): n/a

#### `TEST(Wave4BPasskeyAuditExtra, A7_LogPasskeyRegisteredOnCompleteRegistration)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:520
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BPasskeyAuditExtra): n/a
  - `<unnamed>` (A7_LogPasskeyRegisteredOnCompleteRegistration): n/a

#### `TEST_F(Wave4BAuditLoggerNewEventsTest, A4_LogPermissionChangeGrantedEmitsEvent)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BAuditLoggerNewEventsTest): n/a
  - `<unnamed>` (A4_LogPermissionChangeGrantedEmitsEvent): n/a

#### `TEST_F(Wave4BAuditLoggerNewEventsTest, A4_LogPermissionChangeRevokedEmitsEvent)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BAuditLoggerNewEventsTest): n/a
  - `<unnamed>` (A4_LogPermissionChangeRevokedEmitsEvent): n/a

#### `TEST_F(Wave4BAuditLoggerNewEventsTest, A4_LogRoleChangeEmitsEvent)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BAuditLoggerNewEventsTest): n/a
  - `<unnamed>` (A4_LogRoleChangeEmitsEvent): n/a

#### `TEST_F(Wave4BAuditLoggerNewEventsTest, A4_NoopWhenLoggerIsNull)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BAuditLoggerNewEventsTest): n/a
  - `<unnamed>` (A4_NoopWhenLoggerIsNull): n/a

#### `TEST_F(Wave4BJWTKeyRotationAuditTest, A5_MaxKeysLimitEmitsAuditEvent)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BJWTKeyRotationAuditTest): n/a
  - `<unnamed>` (A5_MaxKeysLimitEmitsAuditEvent): n/a

#### `TEST_F(Wave4BJWTKeyRotationAuditTest, A6_UnknownKidRevokeEmitsAuditEvent)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:263
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BJWTKeyRotationAuditTest): n/a
  - `<unnamed>` (A6_UnknownKidRevokeEmitsAuditEvent): n/a

#### `TEST_F(Wave4BMTLSAuditTest, A2_InvalidCertEmitsNoAuditEvent)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BMTLSAuditTest): n/a
  - `<unnamed>` (A2_InvalidCertEmitsNoAuditEvent): n/a

#### `TEST_F(Wave4BMTLSAuditTest, A2_SetAuditLoggerCompiles)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BMTLSAuditTest): n/a
  - `<unnamed>` (A2_SetAuditLoggerCompiles): n/a

#### `TEST_F(Wave4BPasskeyAuditTest, A1_FailureOnChallengeNotFound)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BPasskeyAuditTest): n/a
  - `<unnamed>` (A1_FailureOnChallengeNotFound): n/a

#### `TEST_F(Wave4BPasskeyAuditTest, A1_SuccessPathLogsPasskeySuccess)`
- Source: `tests/auth/test_wave4b_auth_hardening.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4BPasskeyAuditTest): n/a
  - `<unnamed>` (A1_SuccessPathLogsPasskeySuccess): n/a

### test_wave4b_auth_hardening2.cpp

#### `TEST(Wave4B2FederatedRetry, B2_ExchangeTokenRetriesOn503)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:421
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2FederatedRetry): n/a
  - `<unnamed>` (B2_ExchangeTokenRetriesOn503): n/a

#### `TEST(Wave4B2LDAPRetry, B1_CheckoutHandlesUnreachableServer)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:406
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2LDAPRetry): n/a
  - `<unnamed>` (B1_CheckoutHandlesUnreachableServer): n/a

#### `TEST(Wave4B2MTLSCrypto, C2_InvalidPEMStillThrows)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:538
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2MTLSCrypto): n/a
  - `<unnamed>` (C2_InvalidPEMStillThrows): n/a

#### `TEST(Wave4B2MTLSCrypto, C2_ServerAuthOnlyEKURejected)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:524
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2MTLSCrypto): n/a
  - `<unnamed>` (C2_ServerAuthOnlyEKURejected): n/a

#### `TEST(Wave4B2RSAKeySize, C3_1024BitRSAKeyRejected)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:552
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2RSAKeySize): n/a
  - `<unnamed>` (C3_1024BitRSAKeyRejected): n/a

#### `TEST_F(Wave4B2COSEAlgTest, C1_EC2WithES384AlgRejected)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:463
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2COSEAlgTest): n/a
  - `<unnamed>` (C1_EC2WithES384AlgRejected): n/a

#### `TEST_F(Wave4B2COSEAlgTest, C1_RSAWithPS256AlgRejected)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:487
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2COSEAlgTest): n/a
  - `<unnamed>` (C1_RSAWithPS256AlgRejected): n/a

#### `TEST_F(Wave4B2JWTKeyAuditTest, A5_KeyRotationFailedEventPresentEvenIfLogFlushedLate)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2JWTKeyAuditTest): n/a
  - `<unnamed>` (A5_KeyRotationFailedEventPresentEvenIfLogFlushedLate): n/a

#### `TEST_F(Wave4B2JWTKeyAuditTest, A5_KeyRotationFailedFiresBeforeRethrow)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:364
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2JWTKeyAuditTest): n/a
  - `<unnamed>` (A5_KeyRotationFailedFiresBeforeRethrow): n/a

#### `TEST_F(Wave4B2MTLSAuditDirectTest, A2_LogMTLSFailureEmitsOneEvent)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2MTLSAuditDirectTest): n/a
  - `<unnamed>` (A2_LogMTLSFailureEmitsOneEvent): n/a

#### `TEST_F(Wave4B2MTLSAuditDirectTest, A2_LogMTLSSuccessEmitsOneEvent)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2MTLSAuditDirectTest): n/a
  - `<unnamed>` (A2_LogMTLSSuccessEmitsOneEvent): n/a

#### `TEST_F(Wave4B2MTLSAuditDirectTest, A2_MTLSAuditNullLoggerIsNoop)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2MTLSAuditDirectTest): n/a
  - `<unnamed>` (A2_MTLSAuditNullLoggerIsNoop): n/a

#### `TEST_F(Wave4B2PasskeyVerifyAuditTest, A1_VerifyAuthExceptionPathEmitsAuditFailure)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2PasskeyVerifyAuditTest): n/a
  - `<unnamed>` (A1_VerifyAuthExceptionPathEmitsAuditFailure): n/a

#### `TEST_F(Wave4B2PasskeyVerifyAuditTest, A1_VerifyAuthExpiredChallengeLogsFailure)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2PasskeyVerifyAuditTest): n/a
  - `<unnamed>` (A1_VerifyAuthExpiredChallengeLogsFailure): n/a

#### `TEST_F(Wave4B2PasskeyVerifyAuditTest, A1_VerifyAuthNullLoggerDoesNotCrash)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2PasskeyVerifyAuditTest): n/a
  - `<unnamed>` (A1_VerifyAuthNullLoggerDoesNotCrash): n/a

#### `TEST_F(Wave4B2PasskeyVerifyAuditTest, A1b_CompleteAuthChallengeNotFoundEmitsFailureAudit)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2PasskeyVerifyAuditTest): n/a
  - `<unnamed>` (A1b_CompleteAuthChallengeNotFoundEmitsFailureAudit): n/a

#### `TEST_F(Wave4B2RolePermAuditTest, A4_LogPermissionChangeGrantEmitsEvent)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2RolePermAuditTest): n/a
  - `<unnamed>` (A4_LogPermissionChangeGrantEmitsEvent): n/a

#### `TEST_F(Wave4B2RolePermAuditTest, A4_LogPermissionChangeRevokeEmitsEvent)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2RolePermAuditTest): n/a
  - `<unnamed>` (A4_LogPermissionChangeRevokeEmitsEvent): n/a

#### `TEST_F(Wave4B2RolePermAuditTest, A4_LogRoleChangeEmitsEvent)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:318
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2RolePermAuditTest): n/a
  - `<unnamed>` (A4_LogRoleChangeEmitsEvent): n/a

#### `TEST_F(Wave4B2RolePermAuditTest, A4_MultipleRoleChangesEachEmitEvent)`
- Source: `tests/auth/test_wave4b_auth_hardening2.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4B2RolePermAuditTest): n/a
  - `<unnamed>` (A4_MultipleRoleChangesEachEmitEvent): n/a

### test_wave7_auth_ldap_federated.cpp

#### `TEST(FederatedManager_Wave7, FC01_CacheHit)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:348
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FC01_CacheHit): n/a

#### `TEST(FederatedManager_Wave7, FC02_CacheMiss)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:361
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FC02_CacheMiss): n/a

#### `TEST(FederatedManager_Wave7, FC03_ExpiredEntryReturnsMiss)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:367
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FC03_ExpiredEntryReturnsMiss): n/a

#### `TEST(FederatedManager_Wave7, FC04_EvictExpiredEntries)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:380
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FC04_EvictExpiredEntries): n/a

#### `TEST(FederatedManager_Wave7, FC05_ClearCache)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:399
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FC05_ClearCache): n/a

#### `TEST(FederatedManager_Wave7, FC06_CacheSizeAccuracy)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:413
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FC06_CacheSizeAccuracy): n/a

#### `TEST(FederatedManager_Wave7, FC07_UnknownRealmThrowsFederationError)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:430
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FC07_UnknownRealmThrowsFederationError): n/a

#### `TEST(FederatedManager_Wave7, FR01_AddAndHasRealm)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FR01_AddAndHasRealm): n/a

#### `TEST(FederatedManager_Wave7, FR02_RemoveRealm)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FR02_RemoveRealm): n/a

#### `TEST(FederatedManager_Wave7, FR03_DuplicateRealmThrows)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FR03_DuplicateRealmThrows): n/a

#### `TEST(FederatedManager_Wave7, FT01_AddTrustAndCheck)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:278
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FT01_AddTrustAndCheck): n/a

#### `TEST(FederatedManager_Wave7, FT02_SameIssuerAlwaysTrusted)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FT02_SameIssuerAlwaysTrusted): n/a

#### `TEST(FederatedManager_Wave7, FT03_RemoveTrust)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FT03_RemoveTrust): n/a

#### `TEST(FederatedManager_Wave7, FT04_GetCrossProviderTrusts)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:309
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FT04_GetCrossProviderTrusts): n/a

#### `TEST(FederatedManager_Wave7, FT05_EmptyIssuerThrows)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:319
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedManager_Wave7): n/a
  - `<unnamed>` (FT05_EmptyIssuerThrows): n/a

#### `TEST(LDAPAuthenticator_Wave7, WA01_InjectFnUsedWhenPoolDisabled)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPAuthenticator_Wave7): n/a
  - `<unnamed>` (WA01_InjectFnUsedWhenPoolDisabled): n/a

#### `TEST(LDAPAuthenticator_Wave7, WA02_InjectFnUsedWhenPoolEnabled)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPAuthenticator_Wave7): n/a
  - `<unnamed>` (WA02_InjectFnUsedWhenPoolEnabled): n/a

#### `TEST(LDAPAuthenticator_Wave7, WA03_EmptyUsernameThrows)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPAuthenticator_Wave7): n/a
  - `<unnamed>` (WA03_EmptyUsernameThrows): n/a

#### `TEST(LDAPAuthenticator_Wave7, WA04_EmptyPasswordThrows)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPAuthenticator_Wave7): n/a
  - `<unnamed>` (WA04_EmptyPasswordThrows): n/a

#### `TEST(LDAPPool_Wave7, WP01_ConstructionDoesNotThrow)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPPool_Wave7): n/a
  - `<unnamed>` (WP01_ConstructionDoesNotThrow): n/a

#### `TEST(LDAPPool_Wave7, WP02_CheckoutWithoutLdapReturnsNullptr)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPPool_Wave7): n/a
  - `<unnamed>` (WP02_CheckoutWithoutLdapReturnsNullptr): n/a

#### `TEST(LDAPPool_Wave7, WP03_ConfigIsPreserved)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPPool_Wave7): n/a
  - `<unnamed>` (WP03_ConfigIsPreserved): n/a

#### `TEST(LDAPPool_Wave7, WP04_MetricsOnEmptyPool)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPPool_Wave7): n/a
  - `<unnamed>` (WP04_MetricsOnEmptyPool): n/a

#### `TEST(LDAPPool_Wave7, WP05_PoolExhaustionThrowsProviderDegraded)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPPool_Wave7): n/a
  - `<unnamed>` (WP05_PoolExhaustionThrowsProviderDegraded): n/a

#### `TEST(LDAPPool_Wave7, WP06_StaleConnectionIsEvictedNotReturned)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPPool_Wave7): n/a
  - `<unnamed>` (WP06_StaleConnectionIsEvictedNotReturned): n/a

#### `FederatedValidationResult makeFakeResult(const std::string &realm, const std::string &sub, std::chrono::system_clock::time_point exp)`
- Source: `tests/auth/test_wave7_auth_ldap_federated.cpp`:334
- Brief: Build a FederatedValidationResult for cache injection tests (no real JWT).
- Parameters:
  - `realm` (const std::string &): n/a
  - `sub` (const std::string &): n/a
  - `exp` (std::chrono::system_clock::time_point): n/a

### test_wave8_auth_hardening.cpp

#### `TEST(Wave8LDAPPaginationRetry, SetNullAuditLoggerIsSafe)`
- Source: `tests/auth/test_wave8_auth_hardening.cpp`:66
- Brief: W8-AUTH-01: setAuditLogger(nullptr) on LDAPAuthenticator is safe (no-op).
- Parameters:
  - `<unnamed>` (Wave8LDAPPaginationRetry): n/a
  - `<unnamed>` (SetNullAuditLoggerIsSafe): n/a

#### `TEST(Wave8LDAPPaginationRetry, SetRealAuditLoggerAccepted)`
- Source: `tests/auth/test_wave8_auth_hardening.cpp`:77
- Brief: W8-AUTH-02: setAuditLogger() on LDAPAuthenticator accepts a real logger.
- Parameters:
  - `<unnamed>` (Wave8LDAPPaginationRetry): n/a
  - `<unnamed>` (SetRealAuditLoggerAccepted): n/a

#### `TEST(Wave8PoolExhaustionAudit, SetNullAuditLoggerIsSafe)`
- Source: `tests/auth/test_wave8_auth_hardening.cpp`:203
- Brief: W8-AUTH-11: setAuditLogger(nullptr) on LDAPConnectionPool is safe.
- Parameters:
  - `<unnamed>` (Wave8PoolExhaustionAudit): n/a
  - `<unnamed>` (SetNullAuditLoggerIsSafe): n/a

#### `TEST(Wave8PoolExhaustionAudit, SetRealAuditLoggerAccepted)`
- Source: `tests/auth/test_wave8_auth_hardening.cpp`:213
- Brief: W8-AUTH-12: setAuditLogger() on LDAPConnectionPool accepts a real logger.
- Parameters:
  - `<unnamed>` (Wave8PoolExhaustionAudit): n/a
  - `<unnamed>` (SetRealAuditLoggerAccepted): n/a

#### `TEST(Wave8TokenCacheLRU, CacheHitReturnsResult)`
- Source: `tests/auth/test_wave8_auth_hardening.cpp`:106
- Brief: W8-AUTH-04: Cache hit returns the stored result.
- Parameters:
  - `<unnamed>` (Wave8TokenCacheLRU): n/a
  - `<unnamed>` (CacheHitReturnsResult): n/a

#### `TEST(Wave8TokenCacheLRU, CacheMissReturnsNullopt)`
- Source: `tests/auth/test_wave8_auth_hardening.cpp`:99
- Brief: W8-AUTH-03: Cache miss returns std::nullopt.
- Parameters:
  - `<unnamed>` (Wave8TokenCacheLRU): n/a
  - `<unnamed>` (CacheMissReturnsNullopt): n/a

#### `TEST(Wave8TokenCacheLRU, CacheSizeReflectsLiveCount)`
- Source: `tests/auth/test_wave8_auth_hardening.cpp`:171
- Brief: W8-AUTH-09: tokenCacheSize() reflects the live count.
- Parameters:
  - `<unnamed>` (Wave8TokenCacheLRU): n/a
  - `<unnamed>` (CacheSizeReflectsLiveCount): n/a

#### `TEST(Wave8TokenCacheLRU, CacheSurvivesLargeLoad)`
- Source: `tests/auth/test_wave8_auth_hardening.cpp`:183
- Brief: W8-AUTH-10: Cache survives filling kTokenCacheMaxSize entries (no crash).
- Parameters:
  - `<unnamed>` (Wave8TokenCacheLRU): n/a
  - `<unnamed>` (CacheSurvivesLargeLoad): n/a

#### `TEST(Wave8TokenCacheLRU, ClearTokenCacheRemovesAll)`
- Source: `tests/auth/test_wave8_auth_hardening.cpp`:159
- Brief: W8-AUTH-08: clearTokenCache removes all entries.
- Parameters:
  - `<unnamed>` (Wave8TokenCacheLRU): n/a
  - `<unnamed>` (ClearTokenCacheRemovesAll): n/a

#### `TEST(Wave8TokenCacheLRU, DifferentTokensDoNotCollide)`
- Source: `tests/auth/test_wave8_auth_hardening.cpp`:119
- Brief: W8-AUTH-05: Two different tokens do not collide in the cache.
- Parameters:
  - `<unnamed>` (Wave8TokenCacheLRU): n/a
  - `<unnamed>` (DifferentTokensDoNotCollide): n/a

#### `TEST(Wave8TokenCacheLRU, ExpiredEntriesAreEvicted)`
- Source: `tests/auth/test_wave8_auth_hardening.cpp`:145
- Brief: W8-AUTH-07: Expired entries are evicted by evictExpiredCacheEntries().
- Parameters:
  - `<unnamed>` (Wave8TokenCacheLRU): n/a
  - `<unnamed>` (ExpiredEntriesAreEvicted): n/a

#### `TEST(Wave8TokenCacheLRU, OverwriteTokenUpdatesResult)`
- Source: `tests/auth/test_wave8_auth_hardening.cpp`:133
- Brief: W8-AUTH-06: Overwriting a token updates the cached result.
- Parameters:
  - `<unnamed>` (Wave8TokenCacheLRU): n/a
  - `<unnamed>` (OverwriteTokenUpdatesResult): n/a

### themis::auth

#### `SockFd connectWithTimeout(const std::string &host, int port, int timeout_ms)`
- Source: `src/auth/distributed_token_blacklist.cpp`:197
- Brief: Connect With Timeout.
- Parameters:
  - `host` (const std::string &): Input parameter.
  - `port` (int): Input parameter.
  - `timeout_ms` (int): Input parameter.
- Return: Return value.
- Details: host Input parameter. port Input parameter. timeout_ms Input parameter. Return value. Calls: empty(), std::to_string(), getaddrinfo(), c_str(), sockValid(), socket(), ioctlsocket(), fcntl().

#### `int64_t decodeI64(const uint8_t *in) noexcept`
- Source: `src/auth/distributed_token_blacklist.cpp`:116
- Brief: n/a
- Parameters:
  - `in` (const uint8_t *): n/a

#### `uint16_t decodeU16(const uint8_t *in) noexcept`
- Source: `src/auth/distributed_token_blacklist.cpp`:108
- Brief: n/a
- Parameters:
  - `in` (const uint8_t *): n/a

#### `uint32_t decodeU32(const uint8_t *in) noexcept`
- Source: `src/auth/distributed_token_blacklist.cpp`:98
- Brief: n/a
- Parameters:
  - `in` (const uint8_t *): n/a

#### `void encodeI64(int64_t v, uint8_t *out) noexcept`
- Source: `src/auth/distributed_token_blacklist.cpp`:112
- Brief: n/a
- Parameters:
  - `v` (int64_t): n/a
  - `out` (uint8_t *): n/a

#### `void encodeU16(uint16_t v, uint8_t *out) noexcept`
- Source: `src/auth/distributed_token_blacklist.cpp`:104
- Brief: n/a
- Parameters:
  - `v` (uint16_t): n/a
  - `out` (uint8_t *): n/a

#### `void encodeU32(uint32_t v, uint8_t *out) noexcept`
- Source: `src/auth/distributed_token_blacklist.cpp`:92
- Brief: n/a
- Parameters:
  - `v` (uint32_t): n/a
  - `out` (uint8_t *): n/a

#### `std::string formatDateTime(std::chrono::system_clock::time_point tp)`
- Source: `src/auth/saml_authenticator.cpp`:161
- Brief: Build ISO 8601 UTC timestamp from a time_point.
- Parameters:
  - `tp` (std::chrono::system_clock::time_point): Input parameter.
- Return: Return value.
- Details: tp Input parameter. Return value. Calls: std::chrono::system_clock::to_time_t(), gmtime_s(), gmtime_r(), std::strftime(), std::string().

#### `IAuthEventBus & getAuthEventBus()`
- Source: `src/auth/auth_event_bus.cpp`:107
- Brief: Get Auth Event Bus.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getAuthEventBus without additional internal calls.

#### `bool hasCapability(ProviderCapability set, ProviderCapability flag) noexcept`
- Source: `include/auth/auth_principal_contract.h`:205
- Brief: n/a
- Parameters:
  - `set` (ProviderCapability): n/a
  - `flag` (ProviderCapability): n/a

#### `bool isFailClosedClass(AuthFailureClass fc) noexcept`
- Source: `include/auth/auth_principal_contract.h`:152
- Brief: Returns true when the given failure class mandates fail-closed denial.
- Parameters:
  - `fc` (AuthFailureClass): n/a
- Details: Use this predicate in catch blocks to decide whether to propagate the denial or attempt a fallback: try{ returnprovider->validateToken(token); }catch(constAuthException&ex){ if(isFailClosedClass(classifyError(ex.error().code()))){ throw;//harddenial—nofallback } //structurally-invalidinput—alsodenybutwithadistincterrorcode throw; }

#### `bool operator!=(const char *lhs, const SecureString &rhs) noexcept`
- Source: `include/auth/secure_memory.h`:212
- Brief: n/a
- Parameters:
  - `lhs` (const char *): n/a
  - `rhs` (const SecureString &): n/a

#### `bool operator!=(const std::string &lhs, const SecureString &rhs) noexcept`
- Source: `include/auth/secure_memory.h`:213
- Brief: n/a
- Parameters:
  - `lhs` (const std::string &): n/a
  - `rhs` (const SecureString &): n/a

#### `bool operator!=(const std::vector< T > &lhs, const SecureBuffer< T > &rhs) noexcept`
- Source: `include/auth/secure_memory.h`:334
- Brief: n/a
- Parameters:
  - `lhs` (const std::vector< T > &): n/a
  - `rhs` (const SecureBuffer< T > &): n/a

#### `bool operator==(const char *lhs, const SecureString &rhs) noexcept`
- Source: `include/auth/secure_memory.h`:210
- Brief: n/a
- Parameters:
  - `lhs` (const char *): n/a
  - `rhs` (const SecureString &): n/a

#### `bool operator==(const std::string &lhs, const SecureString &rhs) noexcept`
- Source: `include/auth/secure_memory.h`:211
- Brief: n/a
- Parameters:
  - `lhs` (const std::string &): n/a
  - `rhs` (const SecureString &): n/a

#### `bool operator==(const std::vector< T > &lhs, const SecureBuffer< T > &rhs) noexcept`
- Source: `include/auth/secure_memory.h`:332
- Brief: n/a
- Parameters:
  - `lhs` (const std::vector< T > &): n/a
  - `rhs` (const SecureBuffer< T > &): n/a

#### `ProviderCapability operator\|(ProviderCapability a, ProviderCapability b) noexcept`
- Source: `include/auth/auth_principal_contract.h`:200
- Brief: n/a
- Parameters:
  - `a` (ProviderCapability): n/a
  - `b` (ProviderCapability): n/a

#### `std::string outcomeToString(CredentialStuffingOutcome outcome)`
- Source: `src/auth/auth_rate_limiter.cpp`:645
- Brief: Convert a CredentialStuffingOutcome to the canonical string used as a Prometheus label value and in log messages.
- Parameters:
  - `outcome` (CredentialStuffingOutcome): Input parameter.
- Return: Return value.
- Details: outcome Input parameter. Return value. Implements outcomeToString without additional internal calls.

#### `std::pair< std::string, int > parseAddress(const std::string &addr)`
- Source: `src/auth/distributed_token_blacklist.cpp`:179
- Brief: n/a
- Parameters:
  - `addr` (const std::string &): n/a

#### `bool recvAll(SockFd fd, void *buf, size_t len) noexcept`
- Source: `src/auth/distributed_token_blacklist.cpp`:162
- Brief: n/a
- Parameters:
  - `fd` (SockFd): n/a
  - `buf` (void *): n/a
  - `len` (size_t): n/a

#### `std::string redact(std::string_view sensitive)`
- Source: `include/auth/auth_redaction.h`:38
- Brief: Ersetzt einen sensiblen Wert durch eine sichere Platzhalter-Darstellung.
- Parameters:
  - `sensitive` (std::string_view): Der zu schwärzende sensible Wert.
- Return: Platzhalterstring der Form "[REDACTED:Nchars]".
- Details: Die Ausgabe enthält nur die Länge des ursprünglichen Werts, nicht den Inhalt. Dies erlaubt es, bei Debugging-Zwecken die Existenz und Länge eines Werts zu prüfen, ohne den Klartext offenzulegen. sensitive Der zu schwärzende sensible Wert. Platzhalterstring der Form "[REDACTED:Nchars]".

#### `std::string redactIfPresent(std::string_view sensitive)`
- Source: `include/auth/auth_redaction.h`:50
- Brief: Schwärzt einen Wert nur wenn er nicht leer ist; leere Werte bleiben leer.
- Parameters:
  - `sensitive` (std::string_view): Der zu schwärzende sensible Wert.
- Return: Platzhalterstring oder leerer String.
- Details: Nützlich für optionale Felder, bei denen ein leerer String kein Geheimnis enthält. sensitive Der zu schwärzende sensible Wert. Platzhalterstring oder leerer String.

#### `std::string redactPartial(std::string_view sensitive, std::size_t prefix_len=4)`
- Source: `include/auth/auth_redaction.h`:69
- Brief: Maskiert einen Wert partiell: Zeigt die ersten prefix_len Zeichen, schwärzt den Rest.
- Parameters:
  - `sensitive` (std::string_view): Der zu maskierende Wert.
  - `prefix_len` (std::size_t): Anzahl der sichtbaren Zeichen am Anfang (max. sensitive.size()).
- Return: Partiell maskierter String, z. B. "rsa-[REDACTED:12chars]".
- Details: Nützlich für Key-IDs (z. B. JWT kid), bei denen ein kurzes Präfix zur Diagnose ausreicht (z. B. Algorithmus-Typ) ohne den vollständigen Namen zu offenbaren. sensitive Der zu maskierende Wert. prefix_len Anzahl der sichtbaren Zeichen am Anfang (max. sensitive.size()). Partiell maskierter String, z. B. "rsa-[REDACTED:12chars]".

#### `void registerAuthErrors()`
- Source: `src/auth/auth_error.cpp`:272
- Brief: Register Auth Errors.
- Parameters: none
- Details: Calls: errors::ErrorRegistry::getInstance(), registerError(), toErrorCode(), nlohmann::json::array().

#### `bool sendAll(SockFd fd, const void *buf, size_t len) noexcept`
- Source: `src/auth/distributed_token_blacklist.cpp`:145
- Brief: n/a
- Parameters:
  - `fd` (SockFd): n/a
  - `buf` (const void *): n/a
  - `len` (size_t): n/a

#### `void setAuthEventBus(std::shared_ptr< IAuthEventBus > bus)`
- Source: `src/auth/auth_event_bus.cpp`:116
- Brief: Set Auth Event Bus.
- Parameters:
  - `bus` (std::shared_ptr< IAuthEventBus >): Input parameter.
- Details: bus Input parameter. Implements setAuthEventBus without additional internal calls.

#### `std::string sha256Hex(const std::string &input)`
- Source: `src/auth/federated_identity_manager.cpp`:54
- Brief: Sha256 Hex.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value. Calls: EVP_Digest(), data(), size(), EVP_sha256(), std::string(), std::setfill(), std::setw(), str().

#### `void sockSetTimeout(SockFd fd, int timeout_ms) noexcept`
- Source: `src/auth/distributed_token_blacklist.cpp`:127
- Brief: n/a
- Parameters:
  - `fd` (SockFd): n/a
  - `timeout_ms` (int): n/a

#### `errors::ErrorCode toErrorCode(AuthErrorCode code)`
- Source: `include/auth/auth_error.h`:147
- Brief: To Error Code.
- Parameters:
  - `code` (AuthErrorCode): Input parameter.
- Return: Return value.
- Details: code Input parameter. Return value. Implements toErrorCode without additional internal calls.

#### `size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)`
- Source: `src/auth/jwks_security.cpp`:263
- Brief: Callback for writing data.
- Parameters:
  - `contents` (void *): Input/output parameter.
  - `size` (size_t): Input parameter.
  - `nmemb` (size_t): Input parameter.
  - `userp` (void *): Input/output parameter.
- Return: Return value.
- Details: contents Input/output parameter. size Input parameter. nmemb Input parameter. userp Input/output parameter. Return value. Calls: append().

### themis::auth::AccountLockoutManager

#### `AccountLockoutManager(const AuthRateLimitConfig &config)`
- Source: `include/auth/auth_rate_limiter.h`:132
- Brief: Account Lockout Manager.
- Parameters:
  - `config` (const AuthRateLimitConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `void cleanup()`
- Source: `include/auth/auth_rate_limiter.h`:190
- Brief: Cleanup.
- Parameters: none
- Details: Calls: lock(), std::chrono::system_clock::now(), begin(), end(), erase(), std::chrono::system_clock::from_time_t(), std::chrono::system_clock::to_time_t(), std::chrono::steady_clock::now().

#### `void forceLockAccount(const std::string &user_id, std::chrono::seconds duration)`
- Source: `include/auth/auth_rate_limiter.h`:185
- Brief: Force Lock Account.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `duration` (std::chrono::seconds): Input parameter.
- Details: user_id Identifier of the user. duration Input parameter. user_id Identifier of the user. duration Input parameter. Calls: lock(), std::chrono::system_clock::now(), utils::Logger::warn(), std::to_string(), count().

#### `size_t getLockedAccountCount() const`
- Source: `include/auth/auth_rate_limiter.h`:178
- Brief: Get Locked Account Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::optional< LockoutInfo > getLockoutInfo(const std::string &user_id) const`
- Source: `include/auth/auth_rate_limiter.h`:165
- Brief: Get Lockout Info.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: user_id Identifier of the user. Return value.

#### `bool isAccountLocked(const std::string &user_id) const`
- Source: `include/auth/auth_rate_limiter.h`:158
- Brief: Is Account Locked.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: True when the operation succeeds.
- Details: user_id Identifier of the user. True when the operation succeeds.

#### `void lockAccount(const std::string &user_id, const LockoutInfo &info)`
- Source: `include/auth/auth_rate_limiter.h`:203
- Brief: Lock Account.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `info` (const LockoutInfo &): Input parameter.
- Details: user_id Identifier of the user. info Input parameter. user_id Identifier of the user. info Input parameter. Calls: std::chrono::system_clock::now(), utils::Logger::warn(), std::to_string().

#### `bool recordFailedAttempt(const std::string &user_id, const std::string &ip_address, const std::string &reason)`
- Source: `include/auth/auth_rate_limiter.h`:141
- Brief: Record Failed Attempt.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `ip_address` (const std::string &): Input parameter.
  - `reason` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: user_id Identifier of the user. ip_address Input parameter. reason Input parameter. True when the operation succeeds. user_id Identifier of the user. ip_address Input parameter. reason Input parameter. True when the operation succeeds. Calls: lock(), std::chrono::system_clock::now(), utils::Logger::warn(), clear(), push_back(), erase(), std::remove_if(), begin().

#### `void recordSuccessfulAuth(const std::string &user_id)`
- Source: `include/auth/auth_rate_limiter.h`:151
- Brief: Record Successful Auth.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user. user_id Identifier of the user. Calls: lock(), find(), end(), clear(), erase().

#### `void reset()`
- Source: `include/auth/auth_rate_limiter.h`:195
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Calls: lock(), clear().

#### `bool shouldLockAccount(const LockoutInfo &info) const`
- Source: `include/auth/auth_rate_limiter.h`:209
- Brief: Should Lock Account.
- Parameters:
  - `info` (const LockoutInfo &): Input parameter.
- Return: True when the operation succeeds.
- Details: info Input parameter. True when the operation succeeds.

#### `bool unlockAccount(const std::string &user_id)`
- Source: `include/auth/auth_rate_limiter.h`:172
- Brief: Unlock Account.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: True when the operation succeeds.
- Details: user_id Identifier of the user. True when the operation succeeds. user_id Identifier of the user. True when the operation succeeds. Calls: lock(), find(), end(), utils::Logger::info(), clear(), erase().

### themis::auth::ApiKeyAuthenticator

#### `ApiKeyAuthenticator(const Config &config=Config::defaults())`
- Source: `include/auth/api_key_authenticator.h`:78
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `void addCredential(const ApiKeyCredential &credential)`
- Source: `include/auth/api_key_authenticator.h`:95
- Brief: Add Credential.
- Parameters:
  - `credential` (const ApiKeyCredential &): Input parameter.
- Throws:
  - AuthException: if an error occurs.
- Details: credential Input parameter. credential Input parameter. AuthException if an error occurs. Calls: empty(), AuthError(), size(), lock(), spdlog::debug().

#### `ApiKeyClaims authenticate(const std::string &key_id, const std::string &secret)`
- Source: `include/auth/api_key_authenticator.h`:119
- Brief: Authenticate.
- Parameters:
  - `key_id` (const std::string &): Identifier of the key.
  - `secret` (const std::string &): Input parameter.
- Return: Authentication result.
- Details: key_id Identifier of the key. secret Input parameter. Authentication result.

#### `ApiKeyClaims authenticateCombined(const std::string &combined)`
- Source: `include/auth/api_key_authenticator.h`:127
- Brief: Authenticate Combined.
- Parameters:
  - `combined` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: combined Input parameter. Return value. combined Input parameter. Return value. AuthException if an error occurs. Calls: find(), size(), AuthError(), authenticate(), substr().

#### `ApiKeyClaims claimsFromCredential(const ApiKeyCredential &cred) const`
- Source: `include/auth/api_key_authenticator.h`:177
- Brief: Claims From Credential.
- Parameters:
  - `cred` (const ApiKeyCredential &): Input parameter.
- Return: Return value.
- Details: cred Input parameter. Return value.

#### `bool constantTimeEqual(const std::string &a, const std::string &b)`
- Source: `include/auth/api_key_authenticator.h`:163
- Brief: Constant Time Equal.
- Parameters:
  - `a` (const std::string &): Input parameter.
  - `b` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: a Input parameter. b Input parameter. True when the operation succeeds.

#### `ApiKeyCredential createCredential(const std::string &key_id, const std::string &secret, const std::string &principal, const std::vector< std::string > &scopes={}, const std::vector< std::string > &roles={}, const std::string &tenant_id="", std::chrono::system_clock::time_point expires_at=std::chrono::system_clock::time_point{})`
- Source: `include/auth/api_key_authenticator.h`:140
- Brief: Create Credential.
- Parameters:
  - `key_id` (const std::string &): Identifier of the key.
  - `secret` (const std::string &): Input parameter.
  - `principal` (const std::string &): Input parameter.
  - `scopes` (const std::vector< std::string > &): Input parameter.
  - `roles` (const std::vector< std::string > &): Input parameter.
  - `tenant_id` (const std::string &): Identifier of the tenant.
  - `expires_at` (std::chrono::system_clock::time_point): Input parameter.
- Return: Return value.
- Details: key_id Identifier of the key. secret Input parameter. principal Input parameter. scopes Input parameter. roles Input parameter. tenant_id Identifier of the tenant. expires_at Input parameter. Return value.

#### `size_t credentialCount() const`
- Source: `include/auth/api_key_authenticator.h`:107
- Brief: Credential Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string hashSecret(const std::string &secret)`
- Source: `include/auth/api_key_authenticator.h`:138
- Brief: Hash Secret.
- Parameters:
  - `secret` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: secret Input parameter. Return value. secret Input parameter. Return value. AuthException if an error occurs. Calls: SHA256(), data(), size(), AuthError(), hexEncode().

#### `std::string hexEncode(const unsigned char *data, size_t len)`
- Source: `include/auth/api_key_authenticator.h`:170
- Brief: Hex Encode.
- Parameters:
  - `data` (const unsigned char *): Input parameter.
  - `len` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. len Input parameter. Return value. data Input parameter. len Input parameter. Return value. Calls: std::setfill(), std::setw(), str().

#### `void removeCredential(const std::string &key_id)`
- Source: `include/auth/api_key_authenticator.h`:101
- Brief: Remove Credential.
- Parameters:
  - `key_id` (const std::string &): Identifier of the key.
- Details: key_id Identifier of the key. key_id Identifier of the key. Calls: lock(), erase(), spdlog::debug().

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/api_key_authenticator.h`:85
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

### themis::auth::ApiKeyAuthenticator::Config

#### `Config defaults()`
- Source: `include/auth/api_key_authenticator.h`:75
- Brief: Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

### themis::auth::ApiKeyClaims

#### `bool hasScope(const std::string &scope) const`
- Source: `include/auth/api_key_authenticator.h`:43
- Brief: n/a
- Parameters:
  - `scope` (const std::string &): n/a

#### `bool isExpired() const`
- Source: `include/auth/api_key_authenticator.h`:35
- Brief: n/a
- Parameters: none

### themis::auth::AsyncHTTPAuth

#### `AsyncHTTPAuth(AsyncHTTPAuth &&)=delete`
- Source: `include/auth/http_auth_async.h`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncHTTPAuth &&): n/a

#### `AsyncHTTPAuth(const AsyncHTTPAuth &)=delete`
- Source: `include/auth/http_auth_async.h`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AsyncHTTPAuth &): n/a

#### `AsyncHTTPAuth(const HTTPAuthConfig &config=HTTPAuthConfig())`
- Source: `include/auth/http_auth_async.h`:74
- Brief: n/a
- Parameters:
  - `config` (const HTTPAuthConfig &): n/a

#### `std::future< bool > checkConnectivityAsync(const std::string &url)`
- Source: `include/auth/http_auth_async.h`:98
- Brief: Check Connectivity Async.
- Parameters:
  - `url` (const std::string &): Input parameter.
- Return: Return value.
- Details: url Input parameter. Return value.

#### `const HTTPAuthConfig & config() const`
- Source: `include/auth/http_auth_async.h`:100
- Brief: n/a
- Parameters: none

#### `std::future< HTTPAuthResponse > getAsync(const std::string &url, const std::vector< std::pair< std::string, std::string > > &headers={})`
- Source: `include/auth/http_auth_async.h`:83
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a
  - `headers` (const std::vector< std::pair< std::string, std::string > > &): n/a

#### `AsyncHTTPAuth & operator=(AsyncHTTPAuth &&)=delete`
- Source: `include/auth/http_auth_async.h`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (AsyncHTTPAuth &&): n/a

#### `AsyncHTTPAuth & operator=(const AsyncHTTPAuth &)=delete`
- Source: `include/auth/http_auth_async.h`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AsyncHTTPAuth &): n/a

#### `bool performConnectivityCheck(const std::string &url)`
- Source: `include/auth/http_auth_async.h`:129
- Brief: Perform Connectivity Check.
- Parameters:
  - `url` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: url Input parameter. True when the operation succeeds.

#### `HTTPAuthResponse performGet(const std::string &url, const std::vector< std::pair< std::string, std::string > > &headers)`
- Source: `include/auth/http_auth_async.h`:114
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a
  - `headers` (const std::vector< std::pair< std::string, std::string > > &): n/a

#### `HTTPAuthResponse performPost(const std::string &url, const std::string &body, const std::string &content_type, const std::vector< std::pair< std::string, std::string > > &headers)`
- Source: `include/auth/http_auth_async.h`:118
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a
  - `body` (const std::string &): n/a
  - `content_type` (const std::string &): n/a
  - `headers` (const std::vector< std::pair< std::string, std::string > > &): n/a

#### `std::future< HTTPAuthResponse > postAsync(const std::string &url, const std::string &body, const std::string &content_type="application/json", const std::vector< std::pair< std::string, std::string > > &headers={})`
- Source: `include/auth/http_auth_async.h`:87
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a
  - `body` (const std::string &): n/a
  - `content_type` (const std::string &): n/a
  - `headers` (const std::vector< std::pair< std::string, std::string > > &): n/a

#### `size_t threadCount() const`
- Source: `include/auth/http_auth_async.h`:106
- Brief: Thread Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void validateURL(const std::string &url)`
- Source: `include/auth/http_auth_async.h`:135
- Brief: Validate URL.
- Parameters:
  - `url` (const std::string &): Input parameter.
- Details: =========================================================================== Helper: Validate URL format =========================================================================== url Input parameter.

#### `~AsyncHTTPAuth()`
- Source: `include/auth/http_auth_async.h`:75
- Brief: n/a
- Parameters: none

### themis::auth::AuthAuditLogger

#### `AuthAuditLogger(utils::AuditLogger *logger=nullptr)`
- Source: `include/auth/auth_audit_logger.h`:34
- Brief: n/a
- Parameters:
  - `logger` (utils::AuditLogger *): n/a

#### `void emit(utils::SecurityEventType type, const std::string &user_id, const std::string &resource, const nlohmann::json &details={})`
- Source: `include/auth/auth_audit_logger.h`:334
- Brief: Emit.
- Parameters:
  - `type` (utils::SecurityEventType): Input parameter.
  - `user_id` (const std::string &): Identifier of the user.
  - `resource` (const std::string &): Input parameter.
  - `details` (const nlohmann::json &): Input parameter.
- Details: type Input parameter. user_id Identifier of the user. resource Input parameter. details Input parameter. Calls: logSecurityEvent().

#### `void emitWithDecisionClass(utils::SecurityEventType type, const std::string &user_id, const std::string &resource, DecisionClass dc, const nlohmann::json &details={})`
- Source: `include/auth/auth_audit_logger.h`:50
- Brief: Emit With Decision Class.
- Parameters:
  - `type` (utils::SecurityEventType): Input parameter.
  - `user_id` (const std::string &): Identifier of the user.
  - `resource` (const std::string &): Input parameter.
  - `dc` (DecisionClass): Input parameter.
  - `details` (const nlohmann::json &): Input parameter.
- Details: type Input parameter. user_id Identifier of the user. resource Input parameter. dc Input parameter. details Input parameter. Calls: logSecurityEvent().

#### `bool isEnabled() const`
- Source: `include/auth/auth_audit_logger.h`:44
- Brief: n/a
- Parameters: none

#### `void logAccountLockoutTriggered(const std::string &user_id, const std::string &ip)`
- Source: `include/auth/auth_audit_logger.h`:328
- Brief: Log Account Lockout Triggered.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `ip` (const std::string &): Input parameter.
- Details: user_id Identifier of the user. ip Input parameter. user_id Identifier of the user. ip Input parameter. Calls: emit().

#### `void logApiKeyFailure(const std::string &key_id, const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:154
- Brief: Log Api Key Failure.
- Parameters:
  - `key_id` (const std::string &): Identifier of the key.
  - `reason` (const std::string &): Input parameter.
- Details: key_id Identifier of the key. reason Input parameter. key_id Identifier of the key. reason Input parameter. Calls: emit().

#### `void logApiKeySuccess(const std::string &key_id, const std::string &principal)`
- Source: `include/auth/auth_audit_logger.h`:146
- Brief: Log Api Key Success.
- Parameters:
  - `key_id` (const std::string &): Identifier of the key.
  - `principal` (const std::string &): Input parameter.
- Details: key_id Identifier of the key. principal Input parameter. key_id Identifier of the key. principal Input parameter. Calls: emit().

#### `void logBruteForceDetected(const std::string &user_id, const std::string &ip, size_t failed_attempts)`
- Source: `include/auth/auth_audit_logger.h`:311
- Brief: -------------------------------------------------------------------- Anomaly detection events (brute-force, credential stuffing) --------------------------------------------------------------------
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `ip` (const std::string &): Input parameter.
  - `failed_attempts` (size_t): Input parameter.
- Details: ------------------------------------------------------------------------ Anomaly detection events (brute-force, credential stuffing) ------------------------------------------------------------------------ user_id Identifier of the user. ip Input parameter. failed_attempts Input parameter. user_id Identifier of the user. ip Input parameter. failed_attempts Input parameter. Calls: emit().

#### `void logCredentialStuffingSuspected(const std::string &ip, size_t distinct_users)`
- Source: `include/auth/auth_audit_logger.h`:320
- Brief: Log Credential Stuffing Suspected.
- Parameters:
  - `ip` (const std::string &): Input parameter.
  - `distinct_users` (size_t): Input parameter.
- Details: ip Input parameter. distinct_users Input parameter. ip Input parameter. distinct_users Input parameter. Calls: emit().

#### `void logJWTFailure(const std::string &reason, const std::string &kid="")`
- Source: `include/auth/auth_audit_logger.h`:72
- Brief: Log JWTFailure.
- Parameters:
  - `reason` (const std::string &): Input parameter.
  - `kid` (const std::string &): Input parameter.
- Details: reason Input parameter. kid Input parameter. Calls: empty(), emit().

#### `void logJWTSuccess(const std::string &sub, const std::string &jti, const std::string &issuer, const std::string &kid)`
- Source: `include/auth/auth_audit_logger.h`:67
- Brief: Log JWTSuccess.
- Parameters:
  - `sub` (const std::string &): Input parameter.
  - `jti` (const std::string &): Input parameter.
  - `issuer` (const std::string &): Input parameter.
  - `kid` (const std::string &): Input parameter.
- Details: sub Input parameter. jti Input parameter. issuer Input parameter. kid Input parameter. sub Input parameter. jti Input parameter. issuer Input parameter. kid Input parameter. Calls: emit().

#### `void logKerberosFailure(const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:97
- Brief: Log Kerberos Failure.
- Parameters:
  - `reason` (const std::string &): Input parameter.
- Details: reason Input parameter. reason Input parameter. Calls: emit().

#### `void logKerberosSuccess(const std::string &principal)`
- Source: `include/auth/auth_audit_logger.h`:91
- Brief: Log Kerberos Success.
- Parameters:
  - `principal` (const std::string &): Input parameter.
- Details: principal Input parameter. principal Input parameter. Calls: emit().

#### `void logLDAPFailure(const std::string &username, const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:277
- Brief: Log LDAPFailure.
- Parameters:
  - `username` (const std::string &): Input parameter.
  - `reason` (const std::string &): Input parameter.
- Details: username Input parameter. reason Input parameter. username Input parameter. reason Input parameter. Calls: emit().

#### `void logLDAPSuccess(const std::string &username, const std::string &dn)`
- Source: `include/auth/auth_audit_logger.h`:269
- Brief: Log LDAPSuccess.
- Parameters:
  - `username` (const std::string &): Input parameter.
  - `dn` (const std::string &): Input parameter.
- Details: username Input parameter. dn Input parameter. username Input parameter. dn Input parameter. Calls: emit().

#### `void logMFAEnrolled(const std::string &user_id)`
- Source: `include/auth/auth_audit_logger.h`:135
- Brief: Log MFAEnrolled.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user. user_id Identifier of the user. Calls: emit().

#### `void logMTLSFailure(const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:234
- Brief: Log MTLSFailure.
- Parameters:
  - `reason` (const std::string &): Input parameter.
- Details: reason Input parameter. reason Input parameter. Calls: emit().

#### `void logMTLSSuccess(const std::string &principal, const std::string &serial)`
- Source: `include/auth/auth_audit_logger.h`:228
- Brief: Log MTLSSuccess.
- Parameters:
  - `principal` (const std::string &): Input parameter.
  - `serial` (const std::string &): Input parameter.
- Details: principal Input parameter. serial Input parameter. principal Input parameter. serial Input parameter. Calls: emit().

#### `void logOAuthDeviceDenied(const std::string &client_id, const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:174
- Brief: Log OAuth Device Denied.
- Parameters:
  - `client_id` (const std::string &): Identifier of the client.
  - `reason` (const std::string &): Input parameter.
- Details: client_id Identifier of the client. reason Input parameter. client_id Identifier of the client. reason Input parameter. Calls: emit().

#### `void logOAuthDeviceGranted(const std::string &client_id, const std::string &sub)`
- Source: `include/auth/auth_audit_logger.h`:166
- Brief: Log OAuth Device Granted.
- Parameters:
  - `client_id` (const std::string &): Identifier of the client.
  - `sub` (const std::string &): Input parameter.
- Details: client_id Identifier of the client. sub Input parameter. client_id Identifier of the client. sub Input parameter. Calls: emit().

#### `void logPasskeyFailure(const std::string &user_id, const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:207
- Brief: Log Passkey Failure.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `reason` (const std::string &): Input parameter.
- Details: user_id Identifier of the user. reason Input parameter. user_id Identifier of the user. reason Input parameter. Calls: emit().

#### `void logPasskeyRegistered(const std::string &user_id, const std::string &credential_id, const std::string &rp_id)`
- Source: `include/auth/auth_audit_logger.h`:215
- Brief: Log Passkey Registered.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `credential_id` (const std::string &): Identifier of the credential.
  - `rp_id` (const std::string &): Identifier of the rp.
- Details: user_id Identifier of the user. credential_id Identifier of the credential. rp_id Identifier of the rp. user_id Identifier of the user. credential_id Identifier of the credential. rp_id Identifier of the rp. Calls: emit().

#### `void logPasskeySuccess(const std::string &user_id, const std::string &credential_id)`
- Source: `include/auth/auth_audit_logger.h`:200
- Brief: Log Passkey Success.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `credential_id` (const std::string &): Identifier of the credential.
- Details: user_id Identifier of the user. credential_id Identifier of the credential. user_id Identifier of the user. credential_id Identifier of the credential. Calls: emit().

#### `void logPermissionChange(const std::string &user_id, const std::string &permission, bool granted)`
- Source: `include/auth/auth_audit_logger.h`:256
- Brief: Log Permission Change.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `permission` (const std::string &): Input parameter.
  - `granted` (bool): Input parameter.
- Details: user_id Identifier of the user. permission Input parameter. granted Input parameter. user_id Identifier of the user. permission Input parameter. granted Input parameter. Calls: emit().

#### `void logRecoveryCodeUsed(const std::string &user_id)`
- Source: `include/auth/auth_audit_logger.h`:129
- Brief: Log Recovery Code Used.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user. user_id Identifier of the user. Calls: emit().

#### `void logRoleChange(const std::string &user_id, const std::string &old_role, const std::string &new_role)`
- Source: `include/auth/auth_audit_logger.h`:246
- Brief: Log Role Change.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `old_role` (const std::string &): Input parameter.
  - `new_role` (const std::string &): Input parameter.
- Details: user_id Identifier of the user. old_role Input parameter. new_role Input parameter. user_id Identifier of the user. old_role Input parameter. new_role Input parameter. Calls: emit().

#### `void logSAMLFailure(const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:189
- Brief: Log SAMLFailure.
- Parameters:
  - `reason` (const std::string &): Input parameter.
- Details: reason Input parameter. reason Input parameter. Calls: emit().

#### `void logSAMLSuccess(const std::string &subject, const std::string &issuer)`
- Source: `include/auth/auth_audit_logger.h`:182
- Brief: Log SAMLSuccess.
- Parameters:
  - `subject` (const std::string &): Input parameter.
  - `issuer` (const std::string &): Input parameter.
- Details: subject Input parameter. issuer Input parameter. subject Input parameter. issuer Input parameter. Calls: emit().

#### `void logTOTPDrift(const std::string &user_id, int step_offset, std::chrono::system_clock::time_point timestamp)`
- Source: `include/auth/auth_audit_logger.h`:121
- Brief: Log TOTPDrift.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `step_offset` (int): Input parameter.
  - `timestamp` (std::chrono::system_clock::time_point): Input parameter.
- Details: user_id Identifier of the user. step_offset Input parameter. timestamp Input parameter. user_id Identifier of the user. step_offset Input parameter. timestamp Input parameter. Calls: time_since_epoch(), count(), emit().

#### `void logTOTPFailure(const std::string &user_id)`
- Source: `include/auth/auth_audit_logger.h`:113
- Brief: Log TOTPFailure.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user. user_id Identifier of the user. Calls: emit().

#### `void logTOTPSuccess(const std::string &user_id)`
- Source: `include/auth/auth_audit_logger.h`:107
- Brief: Log TOTPSuccess.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user. user_id Identifier of the user. Calls: emit().

#### `void logTokenRevoked(const std::string &jti, const std::string &sub)`
- Source: `include/auth/auth_audit_logger.h`:80
- Brief: Log Token Revoked.
- Parameters:
  - `jti` (const std::string &): Input parameter.
  - `sub` (const std::string &): Input parameter.
- Details: jti Input parameter. sub Input parameter. jti Input parameter. sub Input parameter. Calls: emit().

#### `void logZeroTrustAllowed(const std::string &user_id, const std::string &resource, double trust_score, const std::string &request_id="")`
- Source: `include/auth/auth_audit_logger.h`:284
- Brief: ------------------------------------------------------------------------ Zero-trust continuous verification events ------------------------------------------------------------------------
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `resource` (const std::string &): Input parameter.
  - `trust_score` (double): Input parameter.
  - `request_id` (const std::string &): Identifier of the request.
- Details: user_id Identifier of the user. resource Input parameter. trust_score Input parameter. request_id Identifier of the request. Calls: empty(), emit().

#### `void logZeroTrustDenied(const std::string &user_id, const std::string &resource, const std::string &reason, const std::string &request_id="")`
- Source: `include/auth/auth_audit_logger.h`:289
- Brief: Log Zero Trust Denied.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `resource` (const std::string &): Input parameter.
  - `reason` (const std::string &): Input parameter.
  - `request_id` (const std::string &): Identifier of the request.
- Details: user_id Identifier of the user. resource Input parameter. reason Input parameter. request_id Identifier of the request. Calls: empty(), emit().

#### `void logZeroTrustReEvaluationFailed(const std::string &user_id, const std::string &session_id, const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:300
- Brief: Log Zero Trust Re Evaluation Failed.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `session_id` (const std::string &): Identifier of the session.
  - `reason` (const std::string &): Input parameter.
- Details: user_id Identifier of the user. session_id Identifier of the session. reason Input parameter. user_id Identifier of the user. session_id Identifier of the session. reason Input parameter. Calls: emit().

#### `void setLogger(utils::AuditLogger *logger)`
- Source: `include/auth/auth_audit_logger.h`:42
- Brief: Set Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setLogger without additional internal calls.

### themis::auth::AuthDurationTimer

#### `AuthDurationTimer(AuthMetrics &metrics, AuthMethod method)`
- Source: `include/auth/auth_metrics.h`:367
- Brief: n/a
- Parameters:
  - `metrics` (AuthMetrics &): n/a
  - `method` (AuthMethod): n/a

#### `double getDuration() const`
- Source: `include/auth/auth_metrics.h`:407
- Brief: n/a
- Parameters: none

#### `void recordFailure(int error_code)`
- Source: `include/auth/auth_metrics.h`:399
- Brief: Record Failure.
- Parameters:
  - `error_code` (int): Input parameter.
- Details: error_code Input parameter. Calls: getDuration(), recordAuthFailure().

#### `void recordSuccess()`
- Source: `include/auth/auth_metrics.h`:386
- Brief: Record Success.
- Parameters: none
- Details: Calls: getDuration(), recordAuthSuccess().

#### `~AuthDurationTimer()`
- Source: `include/auth/auth_metrics.h`:374
- Brief: n/a
- Parameters: none

### themis::auth::AuthError

#### `AuthError(AuthErrorCode code, std::string public_message, std::string internal_message="", std::string request_id="")`
- Source: `include/auth/auth_error.h`:153
- Brief: n/a
- Parameters:
  - `code` (AuthErrorCode): n/a
  - `public_message` (std::string): n/a
  - `internal_message` (std::string): n/a
  - `request_id` (std::string): n/a

#### `AuthErrorCode code() const`
- Source: `include/auth/auth_error.h`:160
- Brief: n/a
- Parameters: none

#### `AuthError fromException(const std::exception &e, const std::string &request_id="")`
- Source: `include/auth/auth_error.h`:200
- Brief: From Exception.
- Parameters:
  - `e` (const std::exception &): Input parameter.
  - `request_id` (const std::string &): Identifier of the request.
- Return: Return value.
- Details: e Input parameter. request_id Identifier of the request. Return value.

#### `std::string generateRequestId()`
- Source: `include/auth/auth_error.h`:224
- Brief: Generate Request Id.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: local_gen(), local_rd(), dis(), str().

#### `const std::string & internalMessage() const`
- Source: `include/auth/auth_error.h`:164
- Brief: n/a
- Parameters: none

#### `void logError() const`
- Source: `include/auth/auth_error.h`:198
- Brief: Log Error.
- Parameters: none

#### `std::string maskEmail(const std::string &email)`
- Source: `include/auth/auth_error.h`:230
- Brief: Mask Email.
- Parameters:
  - `email` (const std::string &): Input parameter.
- Return: Return value.
- Details: email Input parameter. Return value. email Input parameter. Return value. Calls: find(), substr(), length().

#### `std::string maskFilePath(const std::string &path)`
- Source: `include/auth/auth_error.h`:242
- Brief: Mask File Path.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: path Input parameter. Return value. path Input parameter. Return value. Calls: find_last_of(), substr().

#### `std::string maskIPAddress(const std::string &ip)`
- Source: `include/auth/auth_error.h`:248
- Brief: Mask IPAddress.
- Parameters:
  - `ip` (const std::string &): Input parameter.
- Return: Return value.
- Details: ip Input parameter. Return value. ip Input parameter. Return value. Calls: find(), substr().

#### `std::string maskPrincipal(const std::string &principal)`
- Source: `include/auth/auth_error.h`:236
- Brief: Mask Principal.
- Parameters:
  - `principal` (const std::string &): Input parameter.
- Return: Return value.
- Details: principal Input parameter. Return value. principal Input parameter. Return value. Calls: find(), substr().

#### `std::string maskSensitiveData(const std::string &input)`
- Source: `include/auth/auth_error.h`:210
- Brief: Mask Sensitive Data.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value. input Input parameter. Return value. Calls: reserve(), size(), it(), begin(), end(), append(), position(), masker().

#### `std::string maskToken(const std::string &token)`
- Source: `include/auth/auth_error.h`:254
- Brief: Mask Token.
- Parameters:
  - `token` (const std::string &): Input parameter.
- Return: Return value.
- Details: token Input parameter. Return value. token Input parameter. Return value. Calls: length(), substr().

#### `const std::string & publicMessage() const`
- Source: `include/auth/auth_error.h`:162
- Brief: n/a
- Parameters: none

#### `const std::string & requestId() const`
- Source: `include/auth/auth_error.h`:166
- Brief: n/a
- Parameters: none

#### `std::optional< std::chrono::seconds > retryAfter() const`
- Source: `include/auth/auth_error.h`:179
- Brief: n/a
- Parameters: none

#### `void setRetryAfter(std::chrono::seconds duration)`
- Source: `include/auth/auth_error.h`:175
- Brief: Set Retry After.
- Parameters:
  - `duration` (std::chrono::seconds): Input parameter.
- Details: duration Input parameter. Implements setRetryAfter without additional internal calls.

#### `std::chrono::system_clock::time_point timestamp() const`
- Source: `include/auth/auth_error.h`:168
- Brief: n/a
- Parameters: none

#### `nlohmann::json toInternalJSON() const`
- Source: `include/auth/auth_error.h`:193
- Brief: To Internal JSON.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `nlohmann::json toPublicJSON() const`
- Source: `include/auth/auth_error.h`:187
- Brief: To Public JSON.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::auth::AuthException

#### `AuthException(const AuthError &error)`
- Source: `include/auth/auth_error.h`:264
- Brief: Auth Exception.
- Parameters:
  - `error` (const AuthError &): Input parameter.
- Return: Return value.
- Details: error Input parameter. Return value.

#### `const AuthError & error() const`
- Source: `include/auth/auth_error.h`:269
- Brief: n/a
- Parameters: none

### themis::auth::AuthMetrics

#### `AuthMetrics(const Config &config=Config::defaults())`
- Source: `include/auth/auth_metrics.h`:81
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `std::string authMethodToString(AuthMethod method)`
- Source: `include/auth/auth_metrics.h`:362
- Brief: Auth Method To String.
- Parameters:
  - `method` (AuthMethod): Input parameter.
- Return: Return value.
- Details: method Input parameter. Return value. method Input parameter. Return value. Implements authMethodToString without additional internal calls.

#### `uint64_t getCredentialStuffingTotal() const`
- Source: `include/auth/auth_metrics.h`:290
- Brief: Get Credential Stuffing Total.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `uint64_t getFailedAuths() const`
- Source: `include/auth/auth_metrics.h`:278
- Brief: Get Failed Auths.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `int getLDAPActiveConnections() const`
- Source: `include/auth/auth_metrics.h`:308
- Brief: Get LDAPActive Connections.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `int getLDAPIdleConnections() const`
- Source: `include/auth/auth_metrics.h`:302
- Brief: Get LDAPIdle Connections.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `int getLDAPPoolSize() const`
- Source: `include/auth/auth_metrics.h`:296
- Brief: Get LDAPPool Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `double getSuccessRate() const`
- Source: `include/auth/auth_metrics.h`:284
- Brief: Get Success Rate.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `uint64_t getSuccessfulAuths() const`
- Source: `include/auth/auth_metrics.h`:272
- Brief: Get Successful Auths.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `uint64_t getTOTPDriftCount() const`
- Source: `include/auth/auth_metrics.h`:234
- Brief: Get TOTPDrift Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `uint64_t getTotalAttempts() const`
- Source: `include/auth/auth_metrics.h`:266
- Brief: Get Total Attempts.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void recordAccountLockout(const std::string &user_id)`
- Source: `include/auth/auth_metrics.h`:159
- Brief: Record Account Lockout.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user. user_id Identifier of the user. Calls: Add(), Increment(), utils::Logger::warn().

#### `void recordAccountUnlock(const std::string &user_id)`
- Source: `include/auth/auth_metrics.h`:165
- Brief: Record Account Unlock.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user. user_id Identifier of the user. Calls: Add(), Increment(), utils::Logger::info().

#### `void recordAuthAttempt(AuthMethod method, bool success, double duration_ms=0.0)`
- Source: `include/auth/auth_metrics.h`:90
- Brief: Record Auth Attempt.
- Parameters:
  - `method` (AuthMethod): Input parameter.
  - `success` (bool): Input parameter.
  - `duration_ms` (double): Input parameter.
- Details: method Input parameter. success Input parameter. duration_ms Input parameter. Calls: fetch_add(), authMethodToString(), Add(), Increment(), Observe().

#### `void recordAuthFailure(AuthMethod method, int error_code, double duration_ms)`
- Source: `include/auth/auth_metrics.h`:105
- Brief: Record Auth Failure.
- Parameters:
  - `method` (AuthMethod): Input parameter.
  - `error_code` (int): Input parameter.
  - `duration_ms` (double): Input parameter.
- Details: method Input parameter. error_code Input parameter. duration_ms Input parameter. method Input parameter. error_code Input parameter. duration_ms Input parameter. Calls: recordAuthAttempt(), recordError().

#### `void recordAuthSuccess(AuthMethod method, double duration_ms)`
- Source: `include/auth/auth_metrics.h`:97
- Brief: Record Auth Success.
- Parameters:
  - `method` (AuthMethod): Input parameter.
  - `duration_ms` (double): Input parameter.
- Details: method Input parameter. duration_ms Input parameter. method Input parameter. duration_ms Input parameter. Calls: recordAuthAttempt().

#### `void recordCredentialStuffingAttempt(const std::string &user_id, const std::string &ip, const std::string &outcome)`
- Source: `include/auth/auth_metrics.h`:216
- Brief: Record Credential Stuffing Attempt.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `ip` (const std::string &): Input parameter.
  - `outcome` (const std::string &): Input parameter.
- Details: user_id Identifier of the user. ip Input parameter. outcome Input parameter. user_id Identifier of the user. ip Input parameter. outcome Input parameter. Calls: fetch_add(), Add(), Increment().

#### `void recordError(int error_code)`
- Source: `include/auth/auth_metrics.h`:181
- Brief: Record Error.
- Parameters:
  - `error_code` (int): Input parameter.
- Details: error_code Input parameter.

#### `void recordErrorByCategory(const std::string &category)`
- Source: `include/auth/auth_metrics.h`:187
- Brief: Record Error By Category.
- Parameters:
  - `category` (const std::string &): Input parameter.
- Details: category Input parameter. category Input parameter. Calls: Add(), Increment().

#### `void recordJWKSCacheHit()`
- Source: `include/auth/auth_metrics.h`:114
- Brief: Record JWKSCache Hit.
- Parameters: none
- Details: Calls: Add(), Increment().

#### `void recordJWKSCacheMiss()`
- Source: `include/auth/auth_metrics.h`:119
- Brief: Record JWKSCache Miss.
- Parameters: none
- Details: Calls: Add(), Increment().

#### `void recordJWKSFetch(double duration_ms, bool success)`
- Source: `include/auth/auth_metrics.h`:126
- Brief: Record JWKSFetch.
- Parameters:
  - `duration_ms` (double): Input parameter.
  - `success` (bool): Input parameter.
- Details: duration_ms Input parameter. success Input parameter. duration_ms Input parameter. success Input parameter. Calls: Add(), Increment(), Observe().

#### `void recordRateLimitExceeded(const std::string &type)`
- Source: `include/auth/auth_metrics.h`:142
- Brief: Record Rate Limit Exceeded.
- Parameters:
  - `type` (const std::string &): Input parameter.
- Details: type Input parameter. type Input parameter. Calls: Add(), Increment().

#### `void recordRevokedTokenCheck(bool was_revoked)`
- Source: `include/auth/auth_metrics.h`:204
- Brief: Record Revoked Token Check.
- Parameters:
  - `was_revoked` (bool): Input parameter.
- Details: was_revoked Input parameter.

#### `void recordTOTPDrift(int step_offset)`
- Source: `include/auth/auth_metrics.h`:228
- Brief: Record TOTPDrift.
- Parameters:
  - `step_offset` (int): Input parameter.
- Details: step_offset Input parameter.

#### `void recordTokenValidation(AuthMethod method, double duration_ms)`
- Source: `include/auth/auth_metrics.h`:198
- Brief: Record Token Validation.
- Parameters:
  - `method` (AuthMethod): Input parameter.
  - `duration_ms` (double): Input parameter.
- Details: method Input parameter. duration_ms Input parameter. method Input parameter. duration_ms Input parameter. Calls: authMethodToString(), Add(), Observe().

#### `void setJWKSCacheSize(int num_keys)`
- Source: `include/auth/auth_metrics.h`:132
- Brief: Set JWKSCache Size.
- Parameters:
  - `num_keys` (int): Input parameter.
- Details: num_keys Input parameter.

#### `void setLDAPActiveConnections(int count)`
- Source: `include/auth/auth_metrics.h`:256
- Brief: Set LDAPActive Connections.
- Parameters:
  - `count` (int): Input parameter.
- Details: count Input parameter.

#### `void setLDAPIdleConnections(int count)`
- Source: `include/auth/auth_metrics.h`:250
- Brief: Set LDAPIdle Connections.
- Parameters:
  - `count` (int): Input parameter.
- Details: count Input parameter.

#### `void setLDAPPoolSize(int count)`
- Source: `include/auth/auth_metrics.h`:244
- Brief: Set LDAPPool Size.
- Parameters:
  - `count` (int): Input parameter.
- Details: count Input parameter.

#### `void setLockedAccountCount(int count)`
- Source: `include/auth/auth_metrics.h`:171
- Brief: Set Locked Account Count.
- Parameters:
  - `count` (int): Input parameter.
- Details: count Input parameter.

#### `void setRateLimitTokens(const std::string &identifier, double tokens)`
- Source: `include/auth/auth_metrics.h`:149
- Brief: Set Rate Limit Tokens.
- Parameters:
  - `identifier` (const std::string &): Input parameter.
  - `tokens` (double): Input parameter.
- Details: identifier Input parameter. tokens Input parameter. param Input parameter. double Input parameter. Implements setRateLimitTokens without additional internal calls.

#### `~AuthMetrics()=default`
- Source: `include/auth/auth_metrics.h`:84
- Brief: n/a
- Parameters: none

### themis::auth::AuthMetrics::Config

#### `Config defaults()`
- Source: `include/auth/auth_metrics.h`:67
- Brief: Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

### themis::auth::AuthRateLimiter

#### `AuthRateLimiter(const AuthRateLimitConfig &config=AuthRateLimitConfig())`
- Source: `include/auth/auth_rate_limiter.h`:225
- Brief: n/a
- Parameters:
  - `config` (const AuthRateLimitConfig &): n/a

#### `bool allowAuthAttempt(const std::string &ip_address, const std::string &user_id="")`
- Source: `include/auth/auth_rate_limiter.h`:227
- Brief: Allow Auth Attempt.
- Parameters:
  - `ip_address` (const std::string &): Input parameter.
  - `user_id` (const std::string &): Identifier of the user.
- Return: True when the operation succeeds.
- Details: ip_address Input parameter. user_id Identifier of the user. True when the operation succeeds. Calls: fetch_add(), lock(), isWhitelisted(), empty(), isAccountLocked(), utils::Logger::warn(), increment(), allowRequest().

#### `void cleanup()`
- Source: `include/auth/auth_rate_limiter.h`:343
- Brief: Cleanup.
- Parameters: none
- Details: Calls: lock(), std::chrono::steady_clock::now(), std::chrono::seconds(), slock(), begin(), end(), std::lower_bound(), erase().

#### `std::string csBreachKey(const std::string &user_id)`
- Source: `include/auth/auth_rate_limiter.h`:401
- Brief: ── Per-user persistent breach-count tracking ──────────────────────── Build the Redis/in-memory key for a user on the current UTC day.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: user_id Identifier of the user. Return value. Format: "cs:{user_id}:{YYYYMMDD}"

#### `CredentialStuffingOutcome escalateCredentialStuffing(const std::string &user_id, const std::string &ip)`
- Source: `include/auth/auth_rate_limiter.h`:425
- Brief: Called after the IP-level stuffing threshold fires.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `ip` (const std::string &): Input parameter.
- Return: Return value.
- Details: Escalate Credential Stuffing. user_id Identifier of the user. ip Input parameter. Return value. Increments the per-user daily breach counter and fires the appropriate escalation response (CAPTCHA / OTP / 24h lock). Returns the outcome. Must NOT be called with stats_mutex_ held. user_id Identifier of the user. ip Input parameter. Return value. Calls: empty(), incrementAndGetBreachCount(), outcomeFromBreachCount(), forceLockAccount(), utils::Logger::warn(), std::to_string(), store(), getLockedAccountCount().

#### `void fireAuthAnomaly(AuthAnomalyEvent::Type type, const std::string &ip, const std::string &user_id, const std::string &detail, CredentialStuffingOutcome cs_outcome=CredentialStuffingOutcome::ALLOWED) const`
- Source: `include/auth/auth_rate_limiter.h`:377
- Brief: n/a
- Parameters:
  - `type` (AuthAnomalyEvent::Type): n/a
  - `ip` (const std::string &): n/a
  - `user_id` (const std::string &): n/a
  - `detail` (const std::string &): n/a
  - `cs_outcome` (CredentialStuffingOutcome): n/a

#### `std::optional< LockoutInfo > getLockoutInfo(const std::string &user_id) const`
- Source: `include/auth/auth_rate_limiter.h`:266
- Brief: Get Lockout Info.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: user_id Identifier of the user. Return value.

#### `uint32_t getRetryAfter(const std::string &ip_address) const`
- Source: `include/auth/auth_rate_limiter.h`:280
- Brief: Get Retry After.
- Parameters:
  - `ip_address` (const std::string &): Input parameter.
- Return: Return value.
- Details: ip_address Input parameter. Return value.

#### `Statistics getStatistics() const`
- Source: `include/auth/auth_rate_limiter.h`:333
- Brief: Return access control statistics.
- Parameters: none
- Return: Access control statistics.
- Details: Access control statistics.

#### `uint32_t incrementAndGetBreachCount(const std::string &user_id)`
- Source: `include/auth/auth_rate_limiter.h`:409
- Brief: Atomically increment the daily breach counter for user_id and return the new count.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: Increment And Get Breach Count. user_id Identifier of the user. Return value. Uses Redis when available; otherwise falls back to the in-process map. Must NOT be called with stats_mutex_ held (may block on network I/O). user_id Identifier of the user. Return value. Calls: csBreachKey(), rlock(), connectCsRedis(), redisCommand(), c_str(), freeReplyObject(), redisFree(), utils::Logger::error().

#### `bool isAccountLocked(const std::string &user_id) const`
- Source: `include/auth/auth_rate_limiter.h`:259
- Brief: Is Account Locked.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: True when the operation succeeds.
- Details: user_id Identifier of the user. True when the operation succeeds.

#### `bool isWhitelisted(const std::string &ip_address) const`
- Source: `include/auth/auth_rate_limiter.h`:287
- Brief: Is Whitelisted.
- Parameters:
  - `ip_address` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: ip_address Input parameter. True when the operation succeeds.

#### `CredentialStuffingOutcome outcomeFromBreachCount(uint32_t count)`
- Source: `include/auth/auth_rate_limiter.h`:416
- Brief: Determine the escalation outcome from a raw breach count.
- Parameters:
  - `count` (uint32_t): Input parameter.
- Return: Return value.
- Details: count Input parameter. Return value.

#### `void recordFailedAuth(const std::string &user_id, const std::string &ip_address, const std::string &reason)`
- Source: `include/auth/auth_rate_limiter.h`:238
- Brief: Record Failed Auth.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `ip_address` (const std::string &): Input parameter.
  - `reason` (const std::string &): Input parameter.
- Details: user_id Identifier of the user. ip_address Input parameter. reason Input parameter. user_id Identifier of the user. ip_address Input parameter. reason Input parameter. Calls: fetch_add(), lock(), empty(), slock(), trackCredentialStuffing(), recordFailedAttempt(), store(), getLockedAccountCount().

#### `void recordSuccessfulAuth(const std::string &user_id, const std::string &ip_address)`
- Source: `include/auth/auth_rate_limiter.h`:249
- Brief: Record Successful Auth.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `ip_address` (const std::string &): Input parameter.
- Details: user_id Identifier of the user. ip_address Input parameter. user_id Identifier of the user. ip_address Input parameter. Calls: fetch_add(), empty().

#### `void reset()`
- Source: `include/auth/auth_rate_limiter.h`:338
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Calls: slock(), clear(), store(), mlock().

#### `void setAnomalyCallback(AuthAnomalyCallback callback)`
- Source: `include/auth/auth_rate_limiter.h`:293
- Brief: Set Anomaly Callback.
- Parameters:
  - `callback` (AuthAnomalyCallback): Input parameter.
- Details: callback Input parameter. callback Input parameter. Calls: lock(), std::move().

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/auth_rate_limiter.h`:299
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. logger Input/output parameter. Calls: lock().

#### `void setBackend(std::shared_ptr< IRateLimiterBackend > backend)`
- Source: `include/auth/auth_rate_limiter.h`:305
- Brief: Set Backend.
- Parameters:
  - `backend` (std::shared_ptr< IRateLimiterBackend >): Input parameter.
- Details: backend Input parameter. backend Input parameter. Calls: lock(), std::move().

#### `void setMetrics(AuthMetrics *metrics)`
- Source: `include/auth/auth_rate_limiter.h`:311
- Brief: Set Metrics.
- Parameters:
  - `metrics` (AuthMetrics *): Input/output parameter.
- Details: metrics Input/output parameter. metrics Input/output parameter. Calls: lock().

#### `bool trackCredentialStuffing(const std::string &ip, const std::string &user_id, const AuthRateLimitConfig &cfg)`
- Source: `include/auth/auth_rate_limiter.h`:392
- Brief: Track credential-stuffing for a given (ip, user_id) pair.
- Parameters:
  - `ip` (const std::string &): Input parameter.
  - `user_id` (const std::string &): Identifier of the user.
  - `cfg` (const AuthRateLimitConfig &): Input parameter.
- Return: True when the operation succeeds.
- Details: Track Credential Stuffing. ip Input parameter. user_id Identifier of the user. cfg Input parameter. True when the operation succeeds. Returns true if the credential-stuffing alert threshold was just crossed. Must be called with stuffing_mutex_ held. ip Input parameter. user_id Identifier of the user. cfg Input parameter. True when the operation succeeds. Calls: empty(), std::chrono::steady_clock::now(), std::chrono::seconds(), std::lower_bound(), begin(), end(), erase(), clear().

#### `bool unlockAccount(const std::string &user_id)`
- Source: `include/auth/auth_rate_limiter.h`:273
- Brief: Unlock Account.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: True when the operation succeeds.
- Details: user_id Identifier of the user. True when the operation succeeds. user_id Identifier of the user. True when the operation succeeds. Calls: store(), getLockedAccountCount().

#### `void updateConfig(const AuthRateLimitConfig &config)`
- Source: `include/auth/auth_rate_limiter.h`:317
- Brief: Update the access control configuration.
- Parameters:
  - `config` (const AuthRateLimitConfig &): New access control configuration.
- Details: config New access control configuration. config New access control configuration. Calls: lock(), slock(), clear().

### themis::auth::AuthWorkerThreadPool

#### `AuthWorkerThreadPool(AuthWorkerThreadPool &&)=delete`
- Source: `include/auth/auth_worker_thread_pool.h`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthWorkerThreadPool &&): n/a

#### `AuthWorkerThreadPool(const AuthWorkerThreadPool &)=delete`
- Source: `include/auth/auth_worker_thread_pool.h`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AuthWorkerThreadPool &): n/a

#### `AuthWorkerThreadPool(size_t min_threads=kMinThreads, size_t max_threads=kMaxThreads)`
- Source: `include/auth/auth_worker_thread_pool.h`:36
- Brief: n/a
- Parameters:
  - `min_threads` (size_t): n/a
  - `max_threads` (size_t): n/a

#### `AuthWorkerThreadPool & operator=(AuthWorkerThreadPool &&)=delete`
- Source: `include/auth/auth_worker_thread_pool.h`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthWorkerThreadPool &&): n/a

#### `AuthWorkerThreadPool & operator=(const AuthWorkerThreadPool &)=delete`
- Source: `include/auth/auth_worker_thread_pool.h`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AuthWorkerThreadPool &): n/a

#### `void shutdown() noexcept`
- Source: `include/auth/auth_worker_thread_pool.h`:98
- Brief: n/a
- Parameters: none

#### `void spawnWorker()`
- Source: `include/auth/auth_worker_thread_pool.h`:143
- Brief: Spawn one new worker thread.
- Parameters: none
- Details: Must be called with queue_mutex_ held. Calls: emplace_back(), void(), lock(), wait(), empty(), std::move(), front(), pop().

#### `std::future< std::invoke_result_t< Func, Args... > > submit(Func &&f, Args &&... args)`
- Source: `include/auth/auth_worker_thread_pool.h`:60
- Brief: n/a
- Parameters:
  - `f` (Func &&): n/a
  - `args` (Args &&...): n/a

#### `size_t threadCount() const noexcept`
- Source: `include/auth/auth_worker_thread_pool.h`:120
- Brief: n/a
- Parameters: none

#### `void tryGrow()`
- Source: `include/auth/auth_worker_thread_pool.h`:167
- Brief: Grow the pool by one thread if every current worker is busy and we haven't hit max_threads_.
- Parameters: none
- Details: Must be called with queue_mutex_ held. Calls: size(), spawnWorker().

#### `~AuthWorkerThreadPool() noexcept`
- Source: `include/auth/auth_worker_thread_pool.h`:55
- Brief: n/a
- Parameters: none

### themis::auth::CertificateUtils

#### `std::string computeSPKIHashFromFile(const std::string &cert_path)`
- Source: `include/auth/jwks_security.h`:179
- Brief: Compute SPKIHash From File.
- Parameters:
  - `cert_path` (const std::string &): Path to the cert.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: cert_path Path to the cert. Return value. cert_path Path to the cert. Return value. std::runtime_error if an error occurs. Calls: openFileRead(), PEM_read_X509(), fclose(), cert(), i2d_X509_PUBKEY(), X509_get_X509_PUBKEY(), get(), spki().

#### `std::string computeSPKIHashFromPEM(const std::string &cert_pem)`
- Source: `include/auth/jwks_security.h`:186
- Brief: Compute SPKIHash From PEM.
- Parameters:
  - `cert_pem` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: cert_pem Input parameter. Return value. cert_pem Input parameter. Return value. std::runtime_error if an error occurs. Calls: BIO_new_mem_buf(), c_str(), size(), PEM_read_bio_X509(), BIO_free(), cert(), i2d_X509_PUBKEY(), X509_get_X509_PUBKEY().

#### `CertInfo getCertificateInfo(const std::string &cert_path)`
- Source: `include/auth/jwks_security.h`:211
- Brief: Get Certificate Info.
- Parameters:
  - `cert_path` (const std::string &): Path to the cert.
- Return: Return value.
- Details: cert_path Path to the cert. Return value.

#### `bool verifyCertificate(const std::string &cert_path)`
- Source: `include/auth/jwks_security.h`:193
- Brief: Verify Certificate.
- Parameters:
  - `cert_path` (const std::string &): Path to the cert.
- Return: True when the operation succeeds.
- Details: cert_path Path to the cert. True when the operation succeeds. cert_path Path to the cert. True when the operation succeeds. Calls: openFileRead(), PEM_read_X509(), fclose(), X509_get0_notAfter(), ASN1_TIME_diff(), X509_free().

### themis::auth::ChannelBindingGenerator

#### `std::vector< uint8_t > formatChannelBinding(const std::vector< uint8_t > &initiator_address, const std::vector< uint8_t > &acceptor_address, const std::vector< uint8_t > &application_data)`
- Source: `include/auth/kerberos_security.h`:229
- Brief: Format Channel Binding.
- Parameters:
  - `initiator_address` (const std::vector< uint8_t > &): Input parameter.
  - `acceptor_address` (const std::vector< uint8_t > &): Input parameter.
  - `application_data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: initiator_address Input parameter. acceptor_address Input parameter. application_data Input parameter. Return value. initiator_address Input parameter. acceptor_address Input parameter. application_data Input parameter. Return value. Calls: insert(), end(), size(), begin().

#### `std::vector< uint8_t > generateFromTLSCertificate(const std::vector< uint8_t > &server_cert)`
- Source: `include/auth/kerberos_security.h`:200
- Brief: Generate From TLSCertificate.
- Parameters:
  - `server_cert` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: server_cert Input parameter. Return value. server_cert Input parameter. Return value. Calls: hash(), SHA256(), data(), size(), utils::Logger::info().

#### `std::vector< uint8_t > generateFromTLSExporter(const std::vector< uint8_t > &exporter_value)`
- Source: `include/auth/kerberos_security.h`:218
- Brief: Generate From TLSExporter.
- Parameters:
  - `exporter_value` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: exporter_value Input parameter. Return value. exporter_value Input parameter. Return value. Calls: utils::Logger::info().

#### `std::vector< uint8_t > generateFromTLSFinished(const std::vector< uint8_t > &finished_message)`
- Source: `include/auth/kerberos_security.h`:209
- Brief: Generate From TLSFinished.
- Parameters:
  - `finished_message` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: finished_message Input parameter. Return value. finished_message Input parameter. Return value. Calls: utils::Logger::info().

### themis::auth::DefaultAuthEventBus

#### `DefaultAuthEventBus()=default`
- Source: `src/auth/auth_event_bus.cpp`:29
- Brief: n/a
- Parameters: none

#### `void publish(const AuthEvent &event) override`
- Source: `src/auth/auth_event_bus.cpp`:31
- Brief: Publish.
- Parameters:
  - `event` (const AuthEvent &): Input parameter.
- Details: event Input parameter.

#### `bool subscribe(std::shared_ptr< IAuthEventSubscriber > subscriber) override`
- Source: `src/auth/auth_event_bus.cpp`:48
- Brief: n/a
- Parameters:
  - `subscriber` (std::shared_ptr< IAuthEventSubscriber >): n/a

#### `size_t subscriberCount() const override`
- Source: `src/auth/auth_event_bus.cpp`:87
- Brief: n/a
- Parameters: none

#### `bool unsubscribe(const std::string &subscriber_id) override`
- Source: `src/auth/auth_event_bus.cpp`:69
- Brief: n/a
- Parameters:
  - `subscriber_id` (const std::string &): n/a

### themis::auth::DistributedTokenBlacklist

#### `DistributedTokenBlacklist(const DistributedBlacklistConfig &config)`
- Source: `include/auth/distributed_token_blacklist.h`:71
- Brief: Distributed Token Blacklist.
- Parameters:
  - `config` (const DistributedBlacklistConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `DistributedTokenBlacklist(const DistributedTokenBlacklist &)=delete`
- Source: `include/auth/distributed_token_blacklist.h`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedTokenBlacklist &): n/a

#### `void add(const std::string &jti, std::chrono::system_clock::time_point expiry) override`
- Source: `include/auth/distributed_token_blacklist.h`:82
- Brief: Add.
- Parameters:
  - `jti` (const std::string &): Input parameter.
  - `expiry` (std::chrono::system_clock::time_point): Input parameter.
- Details: jti Input parameter. expiry Input parameter.

#### `void applyEntries(const std::vector< std::pair< std::string, int64_t > > &entries)`
- Source: `include/auth/distributed_token_blacklist.h`:164
- Brief: n/a
- Parameters:
  - `entries` (const std::vector< std::pair< std::string, int64_t > > &): n/a

#### `const DistributedBlacklistConfig & config() const`
- Source: `include/auth/distributed_token_blacklist.h`:105
- Brief: n/a
- Parameters: none

#### `std::chrono::system_clock::time_point decodeExpiry(const std::string &val)`
- Source: `include/auth/distributed_token_blacklist.h`:201
- Brief: Decode Expiry.
- Parameters:
  - `val` (const std::string &): Input parameter.
- Return: Return value.
- Details: val Input parameter. Return value.

#### `std::string encodeExpiry(std::chrono::system_clock::time_point tp)`
- Source: `include/auth/distributed_token_blacklist.h`:195
- Brief: Encode Expiry.
- Parameters:
  - `tp` (std::chrono::system_clock::time_point): Input parameter.
- Return: Return value.
- Details: =========================================================================== Helper: Expiry encoding/decoding =========================================================================== tp Input parameter. Return value.

#### `std::vector< std::pair< std::string, std::chrono::system_clock::time_point > > getAllEntries() const`
- Source: `include/auth/distributed_token_blacklist.h`:162
- Brief: n/a
- Parameters: none

#### `ReplicationStats getReplicationStats() const`
- Source: `include/auth/distributed_token_blacklist.h`:103
- Brief: Get Replication Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void handlePeerConnection(std::uintptr_t client_fd)`
- Source: `include/auth/distributed_token_blacklist.h`:159
- Brief: Handle Peer Connection.
- Parameters:
  - `client_fd` (std::uintptr_t): Input parameter.
- Details: client_fd Input parameter.

#### `bool isLeader() const`
- Source: `include/auth/distributed_token_blacklist.h`:107
- Brief: n/a
- Parameters: none

#### `bool isRevoked(const std::string &jti) const override`
- Source: `include/auth/distributed_token_blacklist.h`:85
- Brief: n/a
- Parameters:
  - `jti` (const std::string &): n/a

#### `DistributedTokenBlacklist & operator=(const DistributedTokenBlacklist &)=delete`
- Source: `include/auth/distributed_token_blacklist.h`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedTokenBlacklist &): n/a

#### `bool performClusterSync()`
- Source: `include/auth/distributed_token_blacklist.h`:170
- Brief: RPC handlers (for peer-to-peer communication).
- Parameters: none
- Return: True when the operation succeeds.
- Details: Perform Cluster Sync. True when the operation succeeds.

#### `bool performLeaderElection()`
- Source: `include/auth/distributed_token_blacklist.h`:175
- Brief: Perform Leader Election.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `bool pullRevisionsFromLeader(const std::string &leader_address)`
- Source: `include/auth/distributed_token_blacklist.h`:187
- Brief: Pull Revisions From Leader.
- Parameters:
  - `leader_address` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: leader_address Input parameter. True when the operation succeeds.

#### `void purgeExpired() override`
- Source: `include/auth/distributed_token_blacklist.h`:87
- Brief: Purge Expired.
- Parameters: none

#### `void purgeLoop()`
- Source: `include/auth/distributed_token_blacklist.h`:144
- Brief: Purge Loop.
- Parameters: none

#### `bool pushRevisionsToFollower(const std::string &peer_address)`
- Source: `include/auth/distributed_token_blacklist.h`:181
- Brief: Push Revisions To Follower.
- Parameters:
  - `peer_address` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: peer_address Input parameter. True when the operation succeeds.

#### `void replicationLoop()`
- Source: `include/auth/distributed_token_blacklist.h`:148
- Brief: Replication Loop.
- Parameters: none

#### `void serveIncomingConnections()`
- Source: `include/auth/distributed_token_blacklist.h`:153
- Brief: TCP server listener — accepts PUSH and PULL_REQ connections from cluster peers.
- Parameters: none
- Details: =========================================================================== TCP server listener — accepts inbound connections from cluster peers ===========================================================================

#### `std::future< bool > syncWithCluster()`
- Source: `include/auth/distributed_token_blacklist.h`:97
- Brief: Sync With Cluster.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool waitForClusterConvergence(std::chrono::milliseconds timeout=std::chrono::milliseconds(0))`
- Source: `include/auth/distributed_token_blacklist.h`:109
- Brief: Wait For Cluster Convergence.
- Parameters:
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Return: True when the operation succeeds.
- Details: timeout Input parameter. True when the operation succeeds.

#### `~DistributedTokenBlacklist() override`
- Source: `include/auth/distributed_token_blacklist.h`:73
- Brief: n/a
- Parameters: none

### themis::auth::EIDAuthResult

#### `EIDAuthResult Failure(EIDAuthErrorCode code, std::string msg)`
- Source: `include/auth/eid_authenticator.h`:277
- Brief: Failure.
- Parameters:
  - `code` (EIDAuthErrorCode): Input parameter.
  - `msg` (std::string): Input parameter.
- Return: Return value.
- Details: code Input parameter. msg Input parameter. Return value. Calls: std::move().

#### `EIDAuthResult Success(EIDIdentity id)`
- Source: `include/auth/eid_authenticator.h`:263
- Brief: Success.
- Parameters:
  - `id` (EIDIdentity): Input parameter.
- Return: Return value.
- Details: id Input parameter. Return value. Calls: std::move().

#### `bool has_value() const`
- Source: `include/auth/eid_authenticator.h`:245
- Brief: n/a
- Parameters: none

#### `EIDIdentity * operator->()`
- Source: `include/auth/eid_authenticator.h`:253
- Brief: n/a
- Parameters: none

#### `const EIDIdentity * operator->() const`
- Source: `include/auth/eid_authenticator.h`:249
- Brief: n/a
- Parameters: none

### themis::auth::EIDAuthResult::IdentityResult

#### `IdentityResult()=default`
- Source: `include/auth/eid_authenticator.h`:194
- Brief: n/a
- Parameters: none

#### `IdentityResult(EIDIdentity id)`
- Source: `include/auth/eid_authenticator.h`:195
- Brief: n/a
- Parameters:
  - `id` (EIDIdentity): n/a

#### `std::string fullName() const`
- Source: `include/auth/eid_authenticator.h`:227
- Brief: n/a
- Parameters: none

#### `std::optional< std::string > getAttribute(EIDAttributeType type) const`
- Source: `include/auth/eid_authenticator.h`:231
- Brief: n/a
- Parameters:
  - `type` (EIDAttributeType): n/a

#### `bool has_value() const`
- Source: `include/auth/eid_authenticator.h`:208
- Brief: n/a
- Parameters: none

#### `operator bool() const`
- Source: `include/auth/eid_authenticator.h`:209
- Brief: n/a
- Parameters: none

#### `EIDIdentity * operator->()`
- Source: `include/auth/eid_authenticator.h`:219
- Brief: n/a
- Parameters: none

#### `const EIDIdentity * operator->() const`
- Source: `include/auth/eid_authenticator.h`:223
- Brief: n/a
- Parameters: none

#### `IdentityResult & operator=(EIDIdentity id)`
- Source: `include/auth/eid_authenticator.h`:198
- Brief: n/a
- Parameters:
  - `id` (EIDIdentity): n/a

#### `IdentityResult & operator=(std::optional< EIDIdentity > id)`
- Source: `include/auth/eid_authenticator.h`:203
- Brief: n/a
- Parameters:
  - `id` (std::optional< EIDIdentity >): n/a

#### `EIDIdentity & value()`
- Source: `include/auth/eid_authenticator.h`:216
- Brief: Value.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements value without additional internal calls.

#### `const EIDIdentity & value() const`
- Source: `include/auth/eid_authenticator.h`:217
- Brief: n/a
- Parameters: none

### themis::auth::EIDIdentity

#### `std::string fullName() const`
- Source: `include/auth/eid_authenticator.h`:131
- Brief: n/a
- Parameters: none

#### `std::optional< std::string > getAttribute(EIDAttributeType type) const`
- Source: `include/auth/eid_authenticator.h`:122
- Brief: n/a
- Parameters:
  - `type` (EIDAttributeType): n/a

### themis::auth::FederatedIdentityManager

#### `FederatedIdentityManager()=default`
- Source: `include/auth/federated_identity_manager.h`:56
- Brief: n/a
- Parameters: none

#### `FederatedIdentityManager(FederatedIdentityManager &&) noexcept=default`
- Source: `include/auth/federated_identity_manager.h`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedIdentityManager &&): n/a

#### `FederatedIdentityManager(const FederatedIdentityManager &)=delete`
- Source: `include/auth/federated_identity_manager.h`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (const FederatedIdentityManager &): n/a

#### `void addCrossProviderTrust(const std::string &subject_issuer, const std::string &trusting_issuer)`
- Source: `include/auth/federated_identity_manager.h`:155
- Brief: -------------------------------------------------------------------- Cross-provider trust registry Records which issuers are trusted by which realms.
- Parameters:
  - `subject_issuer` (const std::string &): Input parameter.
  - `trusting_issuer` (const std::string &): Input parameter.
- Throws:
  - AuthException: if an error occurs.
- Details: Add Cross Provider Trust. subject_issuer Input parameter. trusting_issuer Input parameter. Used internally by exchangeToken() to guard cross-realm token exchange. All methods are thread-safe. -------------------------------------------------------------------- subject_issuer Input parameter. trusting_issuer Input parameter. AuthException if an error occurs. Calls: normalize(), empty(), AuthError(), lock(), insert(), spdlog::info().

#### `void addRealm(const OIDCProviderConfig &config)`
- Source: `include/auth/federated_identity_manager.h`:74
- Brief: Add Realm.
- Parameters:
  - `config` (const OIDCProviderConfig &): Input parameter.
- Throws:
  - AuthException: if an error occurs.
- Details: config Input parameter. config Input parameter. AuthException if an error occurs. Calls: normalize(), empty(), AuthError(), setHttpGetForTesting(), lock(), count(), emplace(), std::move().

#### `std::string buildFormBody(const std::vector< std::pair< std::string, std::string > > &params)`
- Source: `include/auth/federated_identity_manager.h`:244
- Brief: n/a
- Parameters:
  - `params` (const std::vector< std::pair< std::string, std::string > > &): n/a

#### `void cacheValidationResult(const std::string &token, const FederatedValidationResult &result)`
- Source: `include/auth/federated_identity_manager.h`:201
- Brief: -------------------------------------------------------------------- In-memory token validation cache validateToken() populates the cache automatically after each successful validation.
- Parameters:
  - `token` (const std::string &): Input parameter.
  - `result` (const FederatedValidationResult &): Input parameter.
- Details: ------------------------------------------------------------------------ In-memory token validation cache ------------------------------------------------------------------------ token Input parameter. result Input parameter. Callers may also query and manage the cache directly. All entries are keyed by the raw bearer token string. -------------------------------------------------------------------- token Input parameter. result Input parameter. Calls: sha256Hex(), lock(), size(), count(), empty(), erase(), back(), pop_back().

#### `void clearTokenCache()`
- Source: `include/auth/federated_identity_manager.h`:221
- Brief: Clear Token Cache.
- Parameters: none
- Details: Calls: lock(), clear().

#### `size_t evictExpiredCacheEntries()`
- Source: `include/auth/federated_identity_manager.h`:216
- Brief: Evict Expired Cache Entries.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: std::chrono::system_clock::now(), lock(), begin(), end(), remove(), erase(), spdlog::debug().

#### `TokenExchangeResult exchangeToken(const std::string &subject_token, const std::string &subject_token_type, const std::string &requested_token_type, const std::vector< std::string > &target_scopes={})`
- Source: `include/auth/federated_identity_manager.h`:117
- Brief: Exchange Token.
- Parameters:
  - `subject_token` (const std::string &): Input parameter.
  - `subject_token_type` (const std::string &): Input parameter.
  - `requested_token_type` (const std::string &): Input parameter.
  - `target_scopes` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: subject_token Input parameter. subject_token_type Input parameter. requested_token_type Input parameter. target_scopes Input parameter. Return value. AuthException if an error occurs. Calls: starts_with(), substr(), extractIssuer(), normalize(), lock(), find(), end(), spdlog::warn().

#### `std::string extractIssuer(const std::string &token)`
- Source: `include/auth/federated_identity_manager.h`:242
- Brief: Extract Issuer.
- Parameters:
  - `token` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: static token Input parameter. Return value. raw_token Input parameter. Return value. AuthException if an error occurs. Calls: size(), substr(), AuthError(), std::to_string(), find(), reserve(), push_back(), nlohmann::json::parse().

#### `std::optional< FederatedValidationResult > getCachedResult(const std::string &token) const`
- Source: `include/auth/federated_identity_manager.h`:209
- Brief: Get Cached Result.
- Parameters:
  - `token` (const std::string &): Input parameter.
- Return: Return value.
- Details: token Input parameter. Return value.

#### `std::vector< std::string > getCrossProviderTrusts(const std::string &trusting_issuer) const`
- Source: `include/auth/federated_identity_manager.h`:181
- Brief: Get Cross Provider Trusts.
- Parameters:
  - `trusting_issuer` (const std::string &): Input parameter.
- Return: Return value.
- Details: trusting_issuer Input parameter. Return value.

#### `bool hasRealm(const std::string &issuer_url) const`
- Source: `include/auth/federated_identity_manager.h`:88
- Brief: Has Realm.
- Parameters:
  - `issuer_url` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: issuer_url Input parameter. True when the operation succeeds.

#### `std::string httpPost(const std::string &url, const std::string &body) const`
- Source: `include/auth/federated_identity_manager.h`:253
- Brief: Http Post.
- Parameters:
  - `url` (const std::string &): Input parameter.
  - `body` (const std::string &): Input parameter.
- Return: Return value.
- Details: url Input parameter. body Input parameter. Return value.

#### `bool isTrustedBy(const std::string &subject_issuer, const std::string &trusting_issuer) const`
- Source: `include/auth/federated_identity_manager.h`:173
- Brief: Is Trusted By.
- Parameters:
  - `subject_issuer` (const std::string &): Input parameter.
  - `trusting_issuer` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: subject_issuer Input parameter. trusting_issuer Input parameter. True when the operation succeeds.

#### `std::string normalize(const std::string &url)`
- Source: `include/auth/federated_identity_manager.h`:235
- Brief: Normalize.
- Parameters:
  - `url` (const std::string &): Input parameter.
- Return: Return value.
- Details: static url Input parameter. Return value. url Input parameter. Return value. Calls: empty(), back(), pop_back().

#### `FederatedIdentityManager & operator=(FederatedIdentityManager &&) noexcept=default`
- Source: `include/auth/federated_identity_manager.h`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedIdentityManager &&): n/a

#### `FederatedIdentityManager & operator=(const FederatedIdentityManager &)=delete`
- Source: `include/auth/federated_identity_manager.h`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (const FederatedIdentityManager &): n/a

#### `size_t realmCount() const`
- Source: `include/auth/federated_identity_manager.h`:100
- Brief: Realm Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< std::string > realmIssuers() const`
- Source: `include/auth/federated_identity_manager.h`:94
- Brief: Realm Issuers.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `OIDCProvider & realmProvider(const std::string &issuer_url)`
- Source: `include/auth/federated_identity_manager.h`:128
- Brief: Realm Provider.
- Parameters:
  - `issuer_url` (const std::string &): Input parameter.
- Return: Return value.
- Details: issuer_url Input parameter. Return value.

#### `bool removeCrossProviderTrust(const std::string &subject_issuer, const std::string &trusting_issuer)`
- Source: `include/auth/federated_identity_manager.h`:164
- Brief: Remove Cross Provider Trust.
- Parameters:
  - `subject_issuer` (const std::string &): Input parameter.
  - `trusting_issuer` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: subject_issuer Input parameter. trusting_issuer Input parameter. True when the operation succeeds. subject_issuer Input parameter. trusting_issuer Input parameter. True when the operation succeeds. Calls: normalize(), lock(), find(), end(), erase(), empty().

#### `bool removeRealm(const std::string &issuer_url)`
- Source: `include/auth/federated_identity_manager.h`:81
- Brief: Remove Realm.
- Parameters:
  - `issuer_url` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: issuer_url Input parameter. True when the operation succeeds. issuer_url Input parameter. True when the operation succeeds. Calls: normalize(), lock(), find(), end(), erase(), spdlog::info().

#### `void setAuditLogger(AuthAuditLogger *logger)`
- Source: `include/auth/federated_identity_manager.h`:135
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (AuthAuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `void setHttpGetForTesting(std::function< std::string(const std::string &url)> fn)`
- Source: `include/auth/federated_identity_manager.h`:141
- Brief: n/a
- Parameters:
  - `fn` (std::function< std::string(const std::string &url)>): n/a

#### `void setHttpPostForTesting(std::function< std::string(const std::string &url, const std::string &body)> fn)`
- Source: `include/auth/federated_identity_manager.h`:144
- Brief: n/a
- Parameters:
  - `fn` (std::function< std::string(const std::string &url, const std::string &body)>): n/a

#### `void syncTrustState(const std::string &peer_node_id, const std::string &peer_rpc_endpoint)`
- Source: `include/auth/federated_identity_manager.h`:191
- Brief: -------------------------------------------------------------------- Multi-realm distributed trust-state synchronization (ROADMAP §3c) Propagates the local trust registry to a peer node via a simple TCP JSON payload using the existing TBLK/v1 retry pattern.
- Parameters:
  - `peer_node_id` (const std::string &): Identifier of the peer node.
  - `peer_rpc_endpoint` (const std::string &): Input parameter.
- Throws:
  - AuthException: if an error occurs.
  - std::runtime_error: if an error occurs.
- Details: ------------------------------------------------------------------------ [3c] Multi-realm distributed trust-state synchronization ------------------------------------------------------------------------ peer_node_id Identifier of the peer node. peer_rpc_endpoint Input parameter. The peer node must expose a JSON-over-TCP listener on peer_rpc_endpoint. Wire format: {"op":"sync_trust","entries":[{"subject":"<issuer>","trusting":"<issuer>"},…]} This push throws AuthException(AUTH_INTERNAL_ERROR) if the connect or send fails after all retry attempts; individual attempt failures within the retry budget are logged but swallowed. -------------------------------------------------------------------- peer_node_id Identifier of the peer node. peer_rpc_endpoint Input parameter. AuthException if an error occurs. std::runtime_error if an error occurs. Calls: nlohmann::json::array(), lock(), push_back(), dump(), rfind(), AuthError(), substr(), std::stoi().

#### `size_t tokenCacheSize() const`
- Source: `include/auth/federated_identity_manager.h`:227
- Brief: Token Cache Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `FederatedValidationResult validateToken(const std::string &token)`
- Source: `include/auth/federated_identity_manager.h`:111
- Brief: Validate Token.
- Parameters:
  - `token` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: token Input parameter. Return value. token Input parameter. Return value. AuthException if an error occurs. Calls: std::chrono::system_clock::now(), sha256Hex(), c_lock(), find(), end(), remove(), push_front(), spdlog::debug().

### themis::auth::GSSAPIAuthResult

#### `GSSAPIAuthResult Failed(const std::string &error)`
- Source: `include/auth/gssapi_authenticator.h`:95
- Brief: Failed.
- Parameters:
  - `error` (const std::string &): Input parameter.
- Return: Return value.
- Details: error Input parameter. Return value. Implements Failed without additional internal calls.

#### `GSSAPIAuthResult Success(const std::string &principal, const std::vector< std::string > &roles)`
- Source: `include/auth/gssapi_authenticator.h`:85
- Brief: Success.
- Parameters:
  - `principal` (const std::string &): Input parameter.
  - `roles` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: principal Input parameter. roles Input parameter. Return value. Implements Success without additional internal calls.

### themis::auth::GSSAPIAuthenticator

#### `GSSAPIAuthenticator()`
- Source: `include/auth/gssapi_authenticator.h`:102
- Brief: n/a
- Parameters: none

#### `GSSAPIAuthenticator(GSSAPIAuthenticator &&)=delete`
- Source: `include/auth/gssapi_authenticator.h`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (GSSAPIAuthenticator &&): n/a

#### `GSSAPIAuthenticator(const GSSAPIAuthenticator &)=delete`
- Source: `include/auth/gssapi_authenticator.h`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GSSAPIAuthenticator &): n/a

#### `bool acceptSecurityContext(const std::vector< uint8_t > &input_token, std::string &principal_name)`
- Source: `include/auth/gssapi_authenticator.h`:176
- Brief: Accept Security Context.
- Parameters:
  - `input_token` (const std::vector< uint8_t > &): Input parameter.
  - `principal_name` (std::string &): Name of the principal.
- Return: True when the operation succeeds.
- Details: input_token Input parameter. principal_name Name of the principal. True when the operation succeeds. input_token Input parameter. principal_name Name of the principal. True when the operation succeeds. Calls: size(), data(), output_buffer(), AcceptSecurityContext(), THEMIS_ERROR(), QueryContextAttributes(), FreeContextBuffer(), DeleteSecurityContext().

#### `GSSAPIAuthResult authenticateToken(const std::string &token)`
- Source: `include/auth/gssapi_authenticator.h`:133
- Brief: Authenticate Token.
- Parameters:
  - `token` (const std::string &): Input parameter.
- Return: Return value.
- Details: token Input parameter. Return value. token Input parameter. Return value. Calls: logSecurityEvent(), GSSAPIAuthResult::Failed(), empty(), token_bytes(), begin(), end(), acceptSecurityContext(), mapPrincipalToRoles().

#### `void cleanup()`
- Source: `include/auth/gssapi_authenticator.h`:182
- Brief: Cleanup.
- Parameters: none
- Details: Calls: FreeCredentialsHandle(), gss_delete_sec_context(), gss_release_cred(), gss_release_name().

#### `const KerberosConfig & getConfig() const`
- Source: `include/auth/gssapi_authenticator.h`:144
- Brief: n/a
- Parameters: none

#### `std::string getGSSAPIError(uint32_t major_status, uint32_t minor_status) const`
- Source: `include/auth/gssapi_authenticator.h`:199
- Brief: Get GSSAPIError.
- Parameters:
  - `major_status` (uint32_t): Input parameter.
  - `minor_status` (uint32_t): Input parameter.
- Return: Return value.
- Details: major_status Input parameter. minor_status Input parameter. Return value.

#### `std::string getServicePrincipal() const`
- Source: `include/auth/gssapi_authenticator.h`:135
- Brief: n/a
- Parameters: none

#### `bool initialize(const KerberosConfig &config)`
- Source: `include/auth/gssapi_authenticator.h`:124
- Brief: Initialize.
- Parameters:
  - `config` (const KerberosConfig &): Input parameter.
- Return: True when the operation succeeds.
- Details: config Input parameter. True when the operation succeeds. config Input parameter. True when the operation succeeds. Calls: THEMIS_WARN(), empty(), THEMIS_ERROR(), _putenv_s(), c_str(), setenv(), THEMIS_INFO(), initializeServerCredentials().

#### `bool initializeServerCredentials()`
- Source: `include/auth/gssapi_authenticator.h`:168
- Brief: Initialize Server Credentials.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Calls: AcquireCredentialsHandle(), c_str(), THEMIS_ERROR(), length(), gss_import_name(), GSS_ERROR(), getGSSAPIError(), gss_acquire_cred().

#### `bool isInitialized() const`
- Source: `include/auth/gssapi_authenticator.h`:126
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > mapPrincipalToRoles(const std::string &principal) const`
- Source: `include/auth/gssapi_authenticator.h`:142
- Brief: Map Principal To Roles.
- Parameters:
  - `principal` (const std::string &): Input parameter.
- Return: Return value.
- Details: principal Input parameter. Return value.

#### `GSSAPIAuthenticator & operator=(GSSAPIAuthenticator &&)=delete`
- Source: `include/auth/gssapi_authenticator.h`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (GSSAPIAuthenticator &&): n/a

#### `GSSAPIAuthenticator & operator=(const GSSAPIAuthenticator &)=delete`
- Source: `include/auth/gssapi_authenticator.h`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GSSAPIAuthenticator &): n/a

#### `bool principalMatchesPattern(const std::string &principal, const std::string &pattern) const`
- Source: `include/auth/gssapi_authenticator.h`:190
- Brief: Principal Matches Pattern.
- Parameters:
  - `principal` (const std::string &): Input parameter.
  - `pattern` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: principal Input parameter. pattern Input parameter. True when the operation succeeds.

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/gssapi_authenticator.h`:117
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `~GSSAPIAuthenticator()`
- Source: `include/auth/gssapi_authenticator.h`:104
- Brief: n/a
- Parameters: none

### themis::auth::HTTPAuthResponse

#### `HTTPAuthResponse Failed(const std::string &error)`
- Source: `include/auth/http_auth_async.h`:51
- Brief: Failed.
- Parameters:
  - `error` (const std::string &): Input parameter.
- Return: Return value.
- Details: error Input parameter. Return value.

#### `HTTPAuthResponse Success(int code, const std::string &body_content)`
- Source: `include/auth/http_auth_async.h`:37
- Brief: Success.
- Parameters:
  - `code` (int): Input parameter.
  - `body_content` (const std::string &): Input parameter.
- Return: Return value.
- Details: code Input parameter. body_content Input parameter. Return value.

### themis::auth::IAuthEventBus

#### `void publish(const AuthEvent &event)=0`
- Source: `include/auth/auth_event_bus.h`:99
- Brief: Publish.
- Parameters:
  - `event` (const AuthEvent &): Input parameter.
- Details: event Input parameter.

#### `bool subscribe(std::shared_ptr< IAuthEventSubscriber > subscriber)=0`
- Source: `include/auth/auth_event_bus.h`:101
- Brief: n/a
- Parameters:
  - `subscriber` (std::shared_ptr< IAuthEventSubscriber >): n/a

#### `size_t subscriberCount() const =0`
- Source: `include/auth/auth_event_bus.h`:105
- Brief: n/a
- Parameters: none

#### `bool unsubscribe(const std::string &subscriber_id)=0`
- Source: `include/auth/auth_event_bus.h`:103
- Brief: n/a
- Parameters:
  - `subscriber_id` (const std::string &): n/a

#### `~IAuthEventBus()=default`
- Source: `include/auth/auth_event_bus.h`:93
- Brief: IAuth Event Bus.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::auth::IAuthEventSubscriber

#### `void onAuthEvent(const AuthEvent &event)=0`
- Source: `include/auth/auth_event_bus.h`:78
- Brief: On Auth Event.
- Parameters:
  - `event` (const AuthEvent &): Input parameter.
- Details: event Input parameter.

#### `std::string subscriberId() const =0`
- Source: `include/auth/auth_event_bus.h`:80
- Brief: n/a
- Parameters: none

#### `~IAuthEventSubscriber()=default`
- Source: `include/auth/auth_event_bus.h`:72
- Brief: IAuth Event Subscriber.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::auth::IAuthorizationPolicy

#### `PolicyEvaluationResult evaluate(const SubjectAttributes &subject, const ResourceAttributes &resource, const std::string &action, const EnvironmentAttributes &environment={}) const =0`
- Source: `include/auth/authorization_policy.h`:91
- Brief: n/a
- Parameters:
  - `subject` (const SubjectAttributes &): n/a
  - `resource` (const ResourceAttributes &): n/a
  - `action` (const std::string &): n/a
  - `environment` (const EnvironmentAttributes &): n/a

#### `std::string policyId() const =0`
- Source: `include/auth/authorization_policy.h`:98
- Brief: n/a
- Parameters: none

#### `std::string policyVersion() const =0`
- Source: `include/auth/authorization_policy.h`:100
- Brief: n/a
- Parameters: none

#### `bool reload()=0`
- Source: `include/auth/authorization_policy.h`:102
- Brief: n/a
- Parameters: none

#### `~IAuthorizationPolicy()=default`
- Source: `include/auth/authorization_policy.h`:89
- Brief: IAuthorization Policy.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::auth::IEIDAuthenticator

#### `std::vector< std::string > activeSessions() const =0`
- Source: `include/auth/eid_authenticator.h`:331
- Brief: n/a
- Parameters: none

#### `std::string beginAuthSession(std::string_view session_id)=0`
- Source: `include/auth/eid_authenticator.h`:320
- Brief: n/a
- Parameters:
  - `session_id` (std::string_view): n/a

#### `EIDAuthResult completeAuthSession(std::string_view session_id, std::string_view saml_response)=0`
- Source: `include/auth/eid_authenticator.h`:322
- Brief: n/a
- Parameters:
  - `session_id` (std::string_view): n/a
  - `saml_response` (std::string_view): n/a

#### `EIDAuthConfig config() const =0`
- Source: `include/auth/eid_authenticator.h`:333
- Brief: n/a
- Parameters: none

#### `bool initialize(const EIDAuthConfig &config)=0`
- Source: `include/auth/eid_authenticator.h`:316
- Brief: n/a
- Parameters:
  - `config` (const EIDAuthConfig &): n/a

#### `bool isInitialized() const =0`
- Source: `include/auth/eid_authenticator.h`:318
- Brief: n/a
- Parameters: none

#### `void revokeSession(std::string_view session_id)=0`
- Source: `include/auth/eid_authenticator.h`:329
- Brief: Revoke Session.
- Parameters:
  - `session_id` (std::string_view): Identifier of the session.
- Details: session_id Identifier of the session.

#### `~IEIDAuthenticator()=default`
- Source: `include/auth/eid_authenticator.h`:314
- Brief: IEIDAuthenticator.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::auth::IPasskeyAuthenticator

#### `PasskeyChallenge beginAuthentication(const std::string &user_id="")=0`
- Source: `include/auth/passkey_authenticator.h`:105
- Brief: n/a
- Parameters:
  - `user_id` (const std::string &): n/a

#### `PasskeyChallenge beginRegistration(const std::string &user_id)=0`
- Source: `include/auth/passkey_authenticator.h`:94
- Brief: n/a
- Parameters:
  - `user_id` (const std::string &): n/a

#### `PasskeyVerifyResult completeAuthentication(const std::string &challenge_id, const PasskeyAssertionResponse &response, std::string &out_user_id)=0`
- Source: `include/auth/passkey_authenticator.h`:109
- Brief: n/a
- Parameters:
  - `challenge_id` (const std::string &): n/a
  - `response` (const PasskeyAssertionResponse &): n/a
  - `out_user_id` (std::string &): n/a

#### `bool completeRegistration(const std::string &challenge_id, const PasskeyCredential &credential)=0`
- Source: `include/auth/passkey_authenticator.h`:96
- Brief: n/a
- Parameters:
  - `challenge_id` (const std::string &): n/a
  - `credential` (const PasskeyCredential &): n/a

#### `std::vector< PasskeyCredential > listCredentials(const std::string &user_id) const =0`
- Source: `include/auth/passkey_authenticator.h`:119
- Brief: n/a
- Parameters:
  - `user_id` (const std::string &): n/a

#### `bool revokeCredential(const std::string &credential_id)=0`
- Source: `include/auth/passkey_authenticator.h`:123
- Brief: n/a
- Parameters:
  - `credential_id` (const std::string &): n/a

#### `~IPasskeyAuthenticator()=default`
- Source: `include/auth/passkey_authenticator.h`:88
- Brief: IPasskey Authenticator.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::auth::IRateLimiterBackend

#### `int64_t getCount(const std::string &key, uint32_t window_seconds) const =0`
- Source: `include/auth/rate_limiter_backend.h`:45
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `window_seconds` (uint32_t): n/a

#### `int64_t increment(const std::string &key, uint32_t window_seconds)=0`
- Source: `include/auth/rate_limiter_backend.h`:43
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `window_seconds` (uint32_t): n/a

#### `void reset(const std::string &key)=0`
- Source: `include/auth/rate_limiter_backend.h`:51
- Brief: Reset the modification detection flag.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Details: key Input parameter.

#### `~IRateLimiterBackend()=default`
- Source: `include/auth/rate_limiter_backend.h`:39
- Brief: IRate Limiter Backend.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::auth::ITokenBlacklist

#### `void add(const std::string &jti, std::chrono::system_clock::time_point expiry)=0`
- Source: `include/auth/token_blacklist.h`:50
- Brief: Add.
- Parameters:
  - `jti` (const std::string &): Input parameter.
  - `expiry` (std::chrono::system_clock::time_point): Input parameter.
- Details: jti Input parameter. expiry Input parameter.

#### `bool isRevoked(const std::string &jti) const =0`
- Source: `include/auth/token_blacklist.h`:53
- Brief: n/a
- Parameters:
  - `jti` (const std::string &): n/a

#### `void purgeExpired()=0`
- Source: `include/auth/token_blacklist.h`:58
- Brief: Purge Expired.
- Parameters: none

#### `~ITokenBlacklist()=default`
- Source: `include/auth/token_blacklist.h`:43
- Brief: IToken Blacklist.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::auth::InMemoryEIDAuthenticator

#### `std::vector< std::string > activeSessions() const override`
- Source: `include/auth/eid_authenticator.h`:504
- Brief: n/a
- Parameters: none

#### `EIDAuthSession beginAuthSession(const EIDAuthRequest &request)`
- Source: `include/auth/eid_authenticator.h`:435
- Brief: Legacy overload used by older tests.
- Parameters:
  - `request` (const EIDAuthRequest &): Input parameter.
- Return: Return value.
- Details: request Input parameter. Return value. Calls: empty().

#### `std::string beginAuthSession(std::string_view session_id) override`
- Source: `include/auth/eid_authenticator.h`:409
- Brief: n/a
- Parameters:
  - `session_id` (std::string_view): n/a

#### `EIDAuthResult completeAuthSession(std::string_view session_id, std::string_view saml_response) override`
- Source: `include/auth/eid_authenticator.h`:445
- Brief: n/a
- Parameters:
  - `session_id` (std::string_view): n/a
  - `saml_response` (std::string_view): n/a

#### `EIDAuthConfig config() const override`
- Source: `include/auth/eid_authenticator.h`:514
- Brief: n/a
- Parameters: none

#### `bool initialize(const EIDAuthConfig &config) override`
- Source: `include/auth/eid_authenticator.h`:378
- Brief: n/a
- Parameters:
  - `config` (const EIDAuthConfig &): n/a

#### `bool isInitialized() const override`
- Source: `include/auth/eid_authenticator.h`:399
- Brief: n/a
- Parameters: none

#### `void registerTestFailure(std::string_view session_id, EIDAuthErrorCode code, std::string message)`
- Source: `include/auth/eid_authenticator.h`:369
- Brief: Register Test Failure.
- Parameters:
  - `session_id` (std::string_view): Identifier of the session.
  - `code` (EIDAuthErrorCode): Input parameter.
  - `message` (std::string): Input parameter.
- Details: session_id Identifier of the session. code Input parameter. message Input parameter. Calls: lk(), std::string(), std::move().

#### `void registerTestIdentity(std::string_view session_id, const EIDIdentity &identity)`
- Source: `include/auth/eid_authenticator.h`:347
- Brief: ── Test helper ───────────────────────────────────────────────────────────
- Parameters:
  - `session_id` (std::string_view): Identifier of the session.
  - `identity` (const EIDIdentity &): Input parameter.
- Details: session_id Identifier of the session. identity Input parameter. Calls: lk(), std::string().

#### `void revokeSession(std::string_view session_id) override`
- Source: `include/auth/eid_authenticator.h`:494
- Brief: Revoke Session.
- Parameters:
  - `session_id` (std::string_view): Identifier of the session.
- Details: session_id Identifier of the session.

#### `void storeIdentity(const EIDIdentity &identity)`
- Source: `include/auth/eid_authenticator.h`:358
- Brief: Legacy helper name kept for compatibility with older tests.
- Parameters:
  - `identity` (const EIDIdentity &): Input parameter.
- Details: identity Input parameter. Calls: registerTestIdentity().

### themis::auth::InMemoryRateLimiterBackend

#### `InMemoryRateLimiterBackend()=default`
- Source: `include/auth/rate_limiter_backend.h`:60
- Brief: n/a
- Parameters: none

#### `InMemoryRateLimiterBackend(const InMemoryRateLimiterBackend &)=delete`
- Source: `include/auth/rate_limiter_backend.h`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (const InMemoryRateLimiterBackend &): n/a

#### `int64_t getCount(const std::string &key, uint32_t window_seconds) const override`
- Source: `include/auth/rate_limiter_backend.h`:67
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `window_seconds` (uint32_t): n/a

#### `int64_t increment(const std::string &key, uint32_t window_seconds) override`
- Source: `include/auth/rate_limiter_backend.h`:66
- Brief: Increment.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `window_seconds` (uint32_t): Input parameter.
- Return: Return value.
- Details: key Input parameter. window_seconds Input parameter. Return value.

#### `InMemoryRateLimiterBackend & operator=(const InMemoryRateLimiterBackend &)=delete`
- Source: `include/auth/rate_limiter_backend.h`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (const InMemoryRateLimiterBackend &): n/a

#### `void reset(const std::string &key) override`
- Source: `include/auth/rate_limiter_backend.h`:68
- Brief: Reset the modification detection flag.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Details: key Input parameter.

#### `~InMemoryRateLimiterBackend() override=default`
- Source: `include/auth/rate_limiter_backend.h`:61
- Brief: n/a
- Parameters: none

### themis::auth::JWKKeyInfo

#### `bool isExpired() const`
- Source: `include/auth/jwt_key_rotation_manager.h`:49
- Brief: n/a
- Parameters: none

### themis::auth::JWKSSecureFetcher

#### `JWKSSecureFetcher(JWKSSecureFetcher &&) noexcept`
- Source: `include/auth/jwks_security.h`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (JWKSSecureFetcher &&): n/a

#### `JWKSSecureFetcher(const JWKSSecureFetcher &)=delete`
- Source: `include/auth/jwks_security.h`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (const JWKSSecureFetcher &): n/a

#### `JWKSSecureFetcher(const JWKSSecurityConfig::Config &config)`
- Source: `include/auth/jwks_security.h`:116
- Brief: JWKSSecure Fetcher.
- Parameters:
  - `config` (const JWKSSecurityConfig::Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `std::string computeSPKIHash(const std::string &cert_data)`
- Source: `include/auth/jwks_security.h`:164
- Brief: Compute SPKI hash from certificate.
- Parameters:
  - `cert_data` (const std::string &): Input parameter.
- Return: Return value.
- Details: Compute SPKIHash. cert_data Input parameter. Return value. cert_data Input parameter. Return value. Calls: SHA256(), c_str(), size(), base64Encode().

#### `std::string fetch(const std::string &url)`
- Source: `include/auth/jwks_security.h`:130
- Brief: Fetch.
- Parameters:
  - `url` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: url Input parameter. Return value. url Input parameter. Return value. std::runtime_error if an error occurs. Calls: substr().

#### `FetchStats getLastFetchStats() const`
- Source: `include/auth/jwks_security.h`:153
- Brief: Get Last Fetch Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `JWKSSecureFetcher & operator=(JWKSSecureFetcher &&) noexcept`
- Source: `include/auth/jwks_security.h`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (JWKSSecureFetcher &&): n/a

#### `JWKSSecureFetcher & operator=(const JWKSSecureFetcher &)=delete`
- Source: `include/auth/jwks_security.h`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (const JWKSSecureFetcher &): n/a

#### `void setupTLSContext()`
- Source: `include/auth/jwks_security.h`:169
- Brief: Setup TLS context with config.
- Parameters: none
- Details: Setup TLSContext. Implements setupTLSContext without additional internal calls.

#### `bool verifyPinning(const std::vector< std::string > &cert_chain)`
- Source: `include/auth/jwks_security.h`:137
- Brief: Verify Pinning.
- Parameters:
  - `cert_chain` (const std::vector< std::string > &): Input parameter.
- Return: True when the operation succeeds.
- Details: cert_chain Input parameter. True when the operation succeeds. cert_chain Input parameter. True when the operation succeeds. Calls: computeSPKIHash().

#### `~JWKSSecureFetcher()`
- Source: `include/auth/jwks_security.h`:117
- Brief: n/a
- Parameters: none

### themis::auth::JWKSSecureFetcher::Impl

#### `Impl(const JWKSSecurityConfig::Config &cfg)`
- Source: `src/auth/jwks_security.cpp`:229
- Brief: n/a
- Parameters:
  - `cfg` (const JWKSSecurityConfig::Config &): n/a

#### `~Impl()`
- Source: `src/auth/jwks_security.cpp`:237
- Brief: n/a
- Parameters: none

### themis::auth::JWKSSecurityConfig

#### `JWKSSecurityConfig(const Config &config)`
- Source: `include/auth/jwks_security.h`:70
- Brief: JWKSSecurity Config.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `const Config & getConfig() const`
- Source: `include/auth/jwks_security.h`:72
- Brief: n/a
- Parameters: none

#### `Config secureDefaults()`
- Source: `include/auth/jwks_security.h`:103
- Brief: Secure Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: Config().

#### `void validate() const`
- Source: `include/auth/jwks_security.h`:77
- Brief: Validate.
- Parameters: none

#### `Config withCertificatePinning(const std::string &cert_path)`
- Source: `include/auth/jwks_security.h`:91
- Brief: With Certificate Pinning.
- Parameters:
  - `cert_path` (const std::string &): Path to the cert.
- Return: Return value.
- Details: cert_path Path to the cert. Return value.

#### `Config withMTLS(const std::string &client_cert_path, const std::string &client_key_path, const std::string &key_password="")`
- Source: `include/auth/jwks_security.h`:93
- Brief: n/a
- Parameters:
  - `client_cert_path` (const std::string &): n/a
  - `client_key_path` (const std::string &): n/a
  - `key_password` (const std::string &): n/a

#### `Config withPublicKeyPinning(const std::vector< std::string > &spki_hashes)`
- Source: `include/auth/jwks_security.h`:84
- Brief: With Public Key Pinning.
- Parameters:
  - `spki_hashes` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: spki_hashes Input parameter. Return value.

### themis::auth::JWKSValidator

#### `JWKSValidator(const Config &config=Config::defaults())`
- Source: `include/auth/jwks_validator.h`:75
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `bool checkDuplicateKids(const nlohmann::json &jwks, ValidationResult &result) const`
- Source: `include/auth/jwks_validator.h`:148
- Brief: Check for duplicate kids.
- Parameters:
  - `jwks` (const nlohmann::json &): Input parameter.
  - `result` (ValidationResult &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: jwks Input parameter. result Input/output parameter. True when the operation succeeds.

#### `ValidationResult validate(const nlohmann::json &jwks) const`
- Source: `include/auth/jwks_validator.h`:82
- Brief: Validate.
- Parameters:
  - `jwks` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: jwks Input parameter. Return value.

#### `bool validateECKey(const nlohmann::json &jwk, size_t index, ValidationResult &result) const`
- Source: `include/auth/jwks_validator.h`:130
- Brief: Validate ECKey.
- Parameters:
  - `jwk` (const nlohmann::json &): Input parameter.
  - `index` (size_t): Input parameter.
  - `result` (ValidationResult &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: jwk Input parameter. index Input parameter. result Input/output parameter. True when the operation succeeds.

#### `bool validateKey(const nlohmann::json &jwk, size_t index, ValidationResult &result) const`
- Source: `include/auth/jwks_validator.h`:110
- Brief: Validate Key.
- Parameters:
  - `jwk` (const nlohmann::json &): Input parameter.
  - `index` (size_t): Input parameter.
  - `result` (ValidationResult &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: jwk Input parameter. index Input parameter. result Input/output parameter. True when the operation succeeds.

#### `void validateOrThrow(const nlohmann::json &jwks) const`
- Source: `include/auth/jwks_validator.h`:88
- Brief: Validate Or Throw.
- Parameters:
  - `jwks` (const nlohmann::json &): Input parameter.
- Details: jwks Input parameter.

#### `bool validateRSAKey(const nlohmann::json &jwk, size_t index, ValidationResult &result) const`
- Source: `include/auth/jwks_validator.h`:120
- Brief: Validate RSAKey.
- Parameters:
  - `jwk` (const nlohmann::json &): Input parameter.
  - `index` (size_t): Input parameter.
  - `result` (ValidationResult &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: jwk Input parameter. index Input parameter. result Input/output parameter. True when the operation succeeds.

#### `bool validateStructure(const nlohmann::json &jwks, ValidationResult &result) const`
- Source: `include/auth/jwks_validator.h`:100
- Brief: Validate Structure.
- Parameters:
  - `jwks` (const nlohmann::json &): Input parameter.
  - `result` (ValidationResult &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: jwks Input parameter. result Input/output parameter. True when the operation succeeds.

#### `bool validateSymmetricKey(const nlohmann::json &jwk, size_t index, ValidationResult &result) const`
- Source: `include/auth/jwks_validator.h`:140
- Brief: Validate Symmetric Key.
- Parameters:
  - `jwk` (const nlohmann::json &): Input parameter.
  - `index` (size_t): Input parameter.
  - `result` (ValidationResult &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: jwk Input parameter. index Input parameter. result Input/output parameter. True when the operation succeeds.

### themis::auth::JWKSValidator::Config

#### `Config defaults()`
- Source: `include/auth/jwks_validator.h`:72
- Brief: Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

### themis::auth::JWKSValidator::ValidationResult

#### `std::string getErrorSummary() const`
- Source: `include/auth/jwks_validator.h`:29
- Brief: n/a
- Parameters: none

### themis::auth::JWTClaims

#### `bool isExpired() const`
- Source: `include/auth/jwt_validator.h`:48
- Brief: n/a
- Parameters: none

### themis::auth::JWTKeyRotationManager

#### `JWTKeyRotationManager(JWTValidator &validator, TokenBlacklist *blacklist, const Config &config)`
- Source: `include/auth/jwt_key_rotation_manager.h`:73
- Brief: n/a
- Parameters:
  - `validator` (JWTValidator &): n/a
  - `blacklist` (TokenBlacklist *): n/a
  - `config` (const Config &): n/a

#### `JWTKeyRotationManager(JWTValidator &validator, TokenBlacklist *blacklist=nullptr)`
- Source: `include/auth/jwt_key_rotation_manager.h`:70
- Brief: n/a
- Parameters:
  - `validator` (JWTValidator &): n/a
  - `blacklist` (TokenBlacklist *): n/a

#### `JWTKeyRotationManager(const JWTKeyRotationManager &)=delete`
- Source: `include/auth/jwt_key_rotation_manager.h`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (const JWTKeyRotationManager &): n/a

#### `std::string activeKeyId() const`
- Source: `include/auth/jwt_key_rotation_manager.h`:128
- Brief: Active Key Id.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void checkAndRotate()`
- Source: `include/auth/jwt_key_rotation_manager.h`:118
- Brief: Check And Rotate.
- Parameters: none
- Details: Calls: lock(), std::chrono::system_clock::now(), push_back(), revokeKid(), THEMIS_WARN(), redact().

#### `std::optional< JWKKeyInfo > getKeyInfo(const std::string &kid) const`
- Source: `include/auth/jwt_key_rotation_manager.h`:147
- Brief: Get Key Info.
- Parameters:
  - `kid` (const std::string &): Input parameter.
- Return: Return value.
- Details: kid Input parameter. Return value.

#### `Statistics getStatistics() const`
- Source: `include/auth/jwt_key_rotation_manager.h`:166
- Brief: Return access control statistics.
- Parameters: none
- Return: Access control statistics.
- Details: Access control statistics.

#### `bool isRotationDue() const`
- Source: `include/auth/jwt_key_rotation_manager.h`:113
- Brief: Is Rotation Due.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `JWTKeyRotationManager & operator=(const JWTKeyRotationManager &)=delete`
- Source: `include/auth/jwt_key_rotation_manager.h`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (const JWTKeyRotationManager &): n/a

#### `std::vector< std::string > passiveKeyIds() const`
- Source: `include/auth/jwt_key_rotation_manager.h`:134
- Brief: Passive Key Ids.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool reactivateKey(const std::string &kid)`
- Source: `include/auth/jwt_key_rotation_manager.h`:103
- Brief: Reactivate Key.
- Parameters:
  - `kid` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: kid Input parameter. True when the operation succeeds. kid Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), THEMIS_WARN(), redact(), std::chrono::system_clock::now(), THEMIS_INFO().

#### `bool revokeKey(const std::string &kid)`
- Source: `include/auth/jwt_key_rotation_manager.h`:96
- Brief: Revoke Key.
- Parameters:
  - `kid` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: kid Input parameter. True when the operation succeeds. kid Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), THEMIS_WARN(), redact(), logSecurityEvent(), revokeKid().

#### `std::vector< std::string > revokedKeyIds() const`
- Source: `include/auth/jwt_key_rotation_manager.h`:140
- Brief: Revoked Key Ids.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void rotateActiveKey(const std::string &new_kid, std::optional< std::chrono::seconds > max_age=std::nullopt)`
- Source: `include/auth/jwt_key_rotation_manager.h`:88
- Brief: Rotate Active Key.
- Parameters:
  - `new_kid` (const std::string &): Input parameter.
  - `max_age` (std::optional< std::chrono::seconds >): Input parameter.
- Throws:
  - std::length_error: if an error occurs.
- Details: new_kid Input parameter. max_age Input parameter. std::length_error if an error occurs. Calls: lock(), size(), find(), end(), logSecurityEvent(), std::to_string(), std::chrono::system_clock::now(), THEMIS_INFO().

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/jwt_key_rotation_manager.h`:173
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `~JWTKeyRotationManager()`
- Source: `include/auth/jwt_key_rotation_manager.h`:82
- Brief: n/a
- Parameters: none

### themis::auth::JWTValidator

#### `JWTValidator(const JWTValidatorConfig &cfg)`
- Source: `include/auth/jwt_validator.h`:89
- Brief: JWTValidator.
- Parameters:
  - `cfg` (const JWTValidatorConfig &): Input parameter.
- Return: Return value.
- Details: cfg Input parameter. Return value.

#### `JWTValidator(const std::string &jwks_url)`
- Source: `include/auth/jwt_validator.h`:82
- Brief: JWTValidator.
- Parameters:
  - `jwks_url` (const std::string &): Input parameter.
- Return: Return value.
- Details: jwks_url Input parameter. Return value.

#### `bool checkAudience(const nlohmann::json &payload) const`
- Source: `include/auth/jwt_validator.h`:241
- Brief: Check Audience.
- Parameters:
  - `payload` (const nlohmann::json &): Input parameter.
- Return: True when the operation succeeds.
- Details: payload Input parameter. True when the operation succeeds.

#### `std::vector< uint8_t > decodeBase64Url(const std::string &input)`
- Source: `include/auth/jwt_validator.h`:158
- Brief: Decode Base64 Url.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value. input Input parameter. Return value. Calls: std::replace(), begin(), end(), size(), BIO_new_mem_buf(), data(), BIO_new(), BIO_f_base64().

#### `std::string decodeBase64UrlToString(const std::string &input)`
- Source: `include/auth/jwt_validator.h`:165
- Brief: Decode Base64 Url To String.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value. input Input parameter. Return value. Calls: decodeBase64Url(), std::string(), data(), size().

#### `std::vector< uint8_t > deriveUserKey(const std::vector< uint8_t > &dek, const JWTClaims &claims, const std::string &field_name)`
- Source: `include/auth/jwt_validator.h`:112
- Brief: Derive User Key.
- Parameters:
  - `dek` (const std::vector< uint8_t > &): Input parameter.
  - `claims` (const JWTClaims &): Input parameter.
  - `field_name` (const std::string &): Name of the field.
- Return: Return value.
- Details: dek Input parameter. claims Input parameter. field_name Name of the field. Return value. dek Input parameter. claims Input parameter. field_name Name of the field. Return value. Calls: salt(), begin(), end(), themis::utils::HKDFHelper::derive().

#### `nlohmann::json fetchJWKS()`
- Source: `include/auth/jwt_validator.h`:171
- Brief: Fetch JWKS.
- Parameters: none
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Return value. Return value. std::runtime_error if an error occurs. Calls: std::chrono::system_clock::now(), read_lock(), empty(), refresh_lock(), wait_for(), THEMIS_WARN(), count(), ScopedRefreshReset().

#### `const nlohmann::json * findJwkForKid(const nlohmann::json &jwks, const std::string &kid) const`
- Source: `include/auth/jwt_validator.h`:179
- Brief: Find Jwk For Kid.
- Parameters:
  - `jwks` (const nlohmann::json &): Input parameter.
  - `kid` (const std::string &): Input parameter.
- Return: Pointer to the result.
- Details: jwks Input parameter. kid Input parameter. Pointer to the result.

#### `bool hasAccess(const JWTClaims &claims, const std::string &encryption_context)`
- Source: `include/auth/jwt_validator.h`:124
- Brief: Has Access.
- Parameters:
  - `claims` (const JWTClaims &): Input parameter.
  - `encryption_context` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: claims Input parameter. encryption_context Input parameter. True when the operation succeeds. claims Input parameter. encryption_context Input parameter. True when the operation succeeds. Implements hasAccess without additional internal calls.

#### `bool isKidRevoked(const std::string &kid) const`
- Source: `include/auth/jwt_validator.h`:150
- Brief: Is Kid Revoked.
- Parameters:
  - `kid` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: kid Input parameter. True when the operation succeeds.

#### `JWTClaims parseAndValidate(const std::string &token)`
- Source: `include/auth/jwt_validator.h`:96
- Brief: Parse And Validate.
- Parameters:
  - `token` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: token Input parameter. Return value. token Input parameter. Return value. std::runtime_error if an error occurs. Calls: rfind(), substr(), size(), utils::Logger::warn(), logSecurityEvent(), empty(), ss(), std::getline().

#### `void revokeKid(const std::string &kid)`
- Source: `include/auth/jwt_validator.h`:143
- Brief: Revoke Kid.
- Parameters:
  - `kid` (const std::string &): Input parameter.
- Details: kid Input parameter. kid Input parameter. Calls: push_back(), utils::Logger::info().

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/jwt_validator.h`:137
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `void setJWKSForTesting(const nlohmann::json &jwks, std::chrono::system_clock::time_point t=std::chrono::system_clock::now())`
- Source: `include/auth/jwt_validator.h`:245
- Brief: Set JWKSFor Testing.
- Parameters:
  - `jwks` (const nlohmann::json &): Input parameter.
  - `t` (std::chrono::system_clock::time_point): Input parameter.
- Details: jwks Input parameter. t Input parameter. Calls: lock().

#### `void setTokenBlacklist(TokenBlacklist *bl)`
- Source: `include/auth/jwt_validator.h`:130
- Brief: Set Token Blacklist.
- Parameters:
  - `bl` (TokenBlacklist *): Input/output parameter.
- Details: bl Input/output parameter. bl Input/output parameter. Implements setTokenBlacklist without additional internal calls.

#### `std::future< JWTClaims > validateAsync(const std::string &token)`
- Source: `include/auth/jwt_validator.h`:103
- Brief: Validate Async.
- Parameters:
  - `token` (const std::string &): Input parameter.
- Return: Return value.
- Details: token Input parameter. Return value. token Input parameter. Return value. Calls: submit(), parseAndValidate().

#### `bool verifySignatureEC(const std::string &header_payload, const std::vector< uint8_t > &signature, const nlohmann::json &jwk, const std::string &alg)`
- Source: `include/auth/jwt_validator.h`:221
- Brief: Verify Signature EC.
- Parameters:
  - `header_payload` (const std::string &): Input parameter.
  - `signature` (const std::vector< uint8_t > &): Input parameter.
  - `jwk` (const nlohmann::json &): Input parameter.
  - `alg` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: header_payload Input parameter. signature Input parameter. jwk Input parameter. alg Input parameter. True when the operation succeeds. header_payload Input parameter. signature Input parameter. jwk Input parameter. alg Input parameter. True when the operation succeeds. Calls: value(), EVP_sha256(), EVP_sha384(), EVP_sha512(), empty(), decodeBase64Url(), size(), group().

#### `bool verifySignatureES256(const std::string &header_payload, const std::vector< uint8_t > &signature, const nlohmann::json &jwk)`
- Source: `include/auth/jwt_validator.h`:210
- Brief: Verify Signature ES256.
- Parameters:
  - `header_payload` (const std::string &): Input parameter.
  - `signature` (const std::vector< uint8_t > &): Input parameter.
  - `jwk` (const nlohmann::json &): Input parameter.
- Return: True when the operation succeeds.
- Details: header_payload Input parameter. signature Input parameter. jwk Input parameter. True when the operation succeeds. header_payload Input parameter. signature Input parameter. jwk Input parameter. True when the operation succeeds. Calls: verifySignatureEC().

#### `bool verifySignatureEdDSA(const std::string &header_payload, const std::vector< uint8_t > &signature, const nlohmann::json &jwk)`
- Source: `include/auth/jwt_validator.h`:232
- Brief: Verify Signature Ed DSA.
- Parameters:
  - `header_payload` (const std::string &): Input parameter.
  - `signature` (const std::vector< uint8_t > &): Input parameter.
  - `jwk` (const nlohmann::json &): Input parameter.
- Return: True when the operation succeeds.
- Details: header_payload Input parameter. signature Input parameter. jwk Input parameter. True when the operation succeeds. header_payload Input parameter. signature Input parameter. jwk Input parameter. True when the operation succeeds. Calls: find(), end(), decodeBase64Url(), size(), EVP_PKEY_new_raw_public_key(), data(), pkey(), EVP_MD_CTX_new().

#### `bool verifySignatureRS256(const std::string &header_payload, const std::vector< uint8_t > &signature, const nlohmann::json &jwk)`
- Source: `include/auth/jwt_validator.h`:188
- Brief: Verify Signature RS256.
- Parameters:
  - `header_payload` (const std::string &): Input parameter.
  - `signature` (const std::vector< uint8_t > &): Input parameter.
  - `jwk` (const nlohmann::json &): Input parameter.
- Return: True when the operation succeeds.
- Details: header_payload Input parameter. signature Input parameter. jwk Input parameter. True when the operation succeeds. header_payload Input parameter. signature Input parameter. jwk Input parameter. True when the operation succeeds. Calls: verifySignatureRSA().

#### `bool verifySignatureRSA(const std::string &header_payload, const std::vector< uint8_t > &signature, const nlohmann::json &jwk, const std::string &alg)`
- Source: `include/auth/jwt_validator.h`:199
- Brief: Verify Signature RSA.
- Parameters:
  - `header_payload` (const std::string &): Input parameter.
  - `signature` (const std::vector< uint8_t > &): Input parameter.
  - `jwk` (const nlohmann::json &): Input parameter.
  - `alg` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: header_payload Input parameter. signature Input parameter. jwk Input parameter. alg Input parameter. True when the operation succeeds. header_payload Input parameter. signature Input parameter. jwk Input parameter. alg Input parameter. True when the operation succeeds. Calls: value(), empty(), decodeBase64Url(), utils::BIGNUMPtr(), BN_bin2bn(), data(), size(), utils::make_evp_key().

### themis::auth::KerberosSecurityValidator

#### `KerberosSecurityValidator(const Config &config)`
- Source: `include/auth/kerberos_security.h`:62
- Brief: Kerberos Security Validator.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `std::vector< uint8_t > computeTLSServerEndpoint(const std::vector< uint8_t > &cert_data)`
- Source: `include/auth/kerberos_security.h`:190
- Brief: Compute TLSServer Endpoint.
- Parameters:
  - `cert_data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: cert_data Input parameter. Return value.

#### `std::string extractServicePrincipal(const std::vector< uint8_t > &token_data)`
- Source: `include/auth/kerberos_security.h`:105
- Brief: Extract Service Principal.
- Parameters:
  - `token_data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: token_data Input parameter. Return value. token_data Input parameter. Return value. Calls: extractKrb5Fields(), empty(), utils::Logger::info(), utils::Logger::warn(), size().

#### `Config forService(const std::string &service_principal)`
- Source: `include/auth/kerberos_security.h`:153
- Brief: For Service.
- Parameters:
  - `service_principal` (const std::string &): Input parameter.
- Return: Return value.
- Details: service_principal Input parameter. Return value. service_principal Input parameter. Return value. Implements forService without additional internal calls.

#### `const Config & getConfig() const`
- Source: `include/auth/kerberos_security.h`:64
- Brief: n/a
- Parameters: none

#### `TokenInfo getTokenInfo(const std::vector< uint8_t > &token_data)`
- Source: `include/auth/kerberos_security.h`:133
- Brief: Get Token Info.
- Parameters:
  - `token_data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: token_data Input parameter. Return value. token_data Input parameter. Return value. Calls: extractKrb5Fields(), empty(), std::chrono::system_clock::now(), std::chrono::system_clock::to_time_t().

#### `bool isTicketExpired(const std::vector< uint8_t > &token_data)`
- Source: `include/auth/kerberos_security.h`:112
- Brief: Is Ticket Expired.
- Parameters:
  - `token_data` (const std::vector< uint8_t > &): Input parameter.
- Return: True when the operation succeeds.
- Details: token_data Input parameter. True when the operation succeeds. token_data Input parameter. True when the operation succeeds. Calls: getTokenInfo(), std::chrono::system_clock::now(), std::chrono::system_clock::to_time_t(), std::abs(), utils::Logger::warn().

#### `bool parseASN1Tag(const uint8_t *data, size_t size, ASN1Tag &tag)`
- Source: `include/auth/kerberos_security.h`:174
- Brief: Parse ASN1 Tag.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
  - `tag` (ASN1Tag &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: data Input parameter. size Input parameter. tag Input/output parameter. True when the operation succeeds. data Input parameter. size Input parameter. tag Input/output parameter. True when the operation succeeds. Implements parseASN1Tag without additional internal calls.

#### `Config strictValidation()`
- Source: `include/auth/kerberos_security.h`:146
- Brief: Strict Validation.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Implements strictValidation without additional internal calls.

#### `bool validateASN1Depth(const uint8_t *data, size_t size, size_t current_depth)`
- Source: `include/auth/kerberos_security.h`:182
- Brief: Validate ASN1 Depth.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
  - `current_depth` (size_t): Input parameter.
- Return: True when the operation succeeds.
- Details: data Input parameter. size Input parameter. current_depth Input parameter. True when the operation succeeds. data Input parameter. size Input parameter. current_depth Input parameter. True when the operation succeeds. Calls: utils::Logger::warn(), parseASN1Tag().

#### `bool validateASN1Structure(const std::vector< uint8_t > &data)`
- Source: `include/auth/kerberos_security.h`:76
- Brief: Validate ASN1 Structure.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: True when the operation succeeds.
- Details: data Input parameter. True when the operation succeeds. data Input parameter. True when the operation succeeds. Calls: empty(), validateASN1Depth(), data(), size().

#### `bool validateToken(const std::vector< uint8_t > &token_data, const std::vector< uint8_t > &channel_binding={})`
- Source: `include/auth/kerberos_security.h`:66
- Brief: Validate Token.
- Parameters:
  - `token_data` (const std::vector< uint8_t > &): Input parameter.
  - `channel_binding` (const std::vector< uint8_t > &): Input parameter.
- Return: True when the operation succeeds.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: token_data Input parameter. channel_binding Input parameter. True when the operation succeeds. std::runtime_error if an error occurs. Calls: validateASN1Structure(), isTicketExpired(), empty(), verifyServicePrincipal(), verifyChannelBinding(), getTokenInfo(), utils::Logger::info().

#### `bool verifyChannelBinding(const std::vector< uint8_t > &token_data, const std::vector< uint8_t > &channel_binding)`
- Source: `include/auth/kerberos_security.h`:95
- Brief: Verify Channel Binding.
- Parameters:
  - `token_data` (const std::vector< uint8_t > &): Input parameter.
  - `channel_binding` (const std::vector< uint8_t > &): Input parameter.
- Return: True when the operation succeeds.
- Details: token_data Input parameter. channel_binding Input parameter. True when the operation succeeds. token_data Input parameter. channel_binding Input parameter. True when the operation succeeds. Calls: empty(), parseGssapiKrb5Header(), data(), size(), utils::Logger::warn(), utils::Logger::info().

#### `bool verifyServicePrincipal(const std::vector< uint8_t > &token_data, const std::string &expected_principal)`
- Source: `include/auth/kerberos_security.h`:84
- Brief: Verify Service Principal.
- Parameters:
  - `token_data` (const std::vector< uint8_t > &): Input parameter.
  - `expected_principal` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: token_data Input parameter. expected_principal Input parameter. True when the operation succeeds. token_data Input parameter. expected_principal Input parameter. True when the operation succeeds. Calls: extractServicePrincipal(), empty(), utils::Logger::warn().

#### `Config withChannelBindings(ChannelBindingType type)`
- Source: `include/auth/kerberos_security.h`:140
- Brief: With Channel Bindings.
- Parameters:
  - `type` (ChannelBindingType): Input parameter.
- Return: Return value.
- Details: type Input parameter. Return value. type Input parameter. Return value. Implements withChannelBindings without additional internal calls.

### themis::auth::LDAPAuthResult

#### `LDAPAuthResult Failed(const std::string &error)`
- Source: `include/auth/ldap_authenticator.h`:114
- Brief: Failed.
- Parameters:
  - `error` (const std::string &): Input parameter.
- Return: Return value.
- Details: error Input parameter. Return value.

#### `LDAPAuthResult Success(const std::string &user, const std::string &distinguished_name, const std::vector< std::string > &mapped_roles, const std::vector< std::string > &ldap_groups={})`
- Source: `include/auth/ldap_authenticator.h`:95
- Brief: n/a
- Parameters:
  - `user` (const std::string &): n/a
  - `distinguished_name` (const std::string &): n/a
  - `mapped_roles` (const std::vector< std::string > &): n/a
  - `ldap_groups` (const std::vector< std::string > &): n/a

### themis::auth::LDAPAuthenticator

#### `LDAPAuthenticator()`
- Source: `include/auth/ldap_authenticator.h`:135
- Brief: n/a
- Parameters: none

#### `LDAPAuthenticator(LDAPAuthenticator &&)=delete`
- Source: `include/auth/ldap_authenticator.h`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPAuthenticator &&): n/a

#### `LDAPAuthenticator(const LDAPAuthenticator &)=delete`
- Source: `include/auth/ldap_authenticator.h`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LDAPAuthenticator &): n/a

#### `LDAPAuthenticator(const LDAPConfig &config)`
- Source: `include/auth/ldap_authenticator.h`:137
- Brief: n/a
- Parameters:
  - `config` (const LDAPConfig &): n/a

#### `LDAPAuthResult authenticate(const std::string &username, const std::string &password)`
- Source: `include/auth/ldap_authenticator.h`:171
- Brief: Authenticate.
- Parameters:
  - `username` (const std::string &): Input parameter.
  - `password` (const std::string &): Input parameter.
- Return: Authentication result.
- Details: username Input parameter. password Input parameter. Authentication result.

#### `std::future< LDAPAuthResult > authenticateAsync(const std::string &username, const std::string &password)`
- Source: `include/auth/ldap_authenticator.h`:180
- Brief: Authenticate Async.
- Parameters:
  - `username` (const std::string &): Input parameter.
  - `password` (const std::string &): Input parameter.
- Return: Return value.
- Details: username Input parameter. password Input parameter. Return value.

#### `std::string buildGroupSearchFilter(const std::string &dn, const std::string &username="") const`
- Source: `include/auth/ldap_authenticator.h`:194
- Brief: n/a
- Parameters:
  - `dn` (const std::string &): n/a
  - `username` (const std::string &): n/a

#### `std::string buildUserDN(const std::string &username) const`
- Source: `include/auth/ldap_authenticator.h`:192
- Brief: Build User DN.
- Parameters:
  - `username` (const std::string &): Input parameter.
- Return: Return value.
- Details: username Input parameter. Return value.

#### `const LDAPConnectionPool * connectionPool() const noexcept`
- Source: `include/auth/ldap_authenticator.h`:185
- Brief: n/a
- Parameters: none

#### `const LDAPConfig & getConfig() const`
- Source: `include/auth/ldap_authenticator.h`:183
- Brief: n/a
- Parameters: none

#### `bool initialize(const LDAPConfig &config)`
- Source: `include/auth/ldap_authenticator.h`:161
- Brief: Initialize.
- Parameters:
  - `config` (const LDAPConfig &): Input parameter.
- Return: True when the operation succeeds.
- Details: config Input parameter. True when the operation succeeds.

#### `bool isInitialized() const`
- Source: `include/auth/ldap_authenticator.h`:163
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > mapGroupsToRoles(const std::vector< std::string > &groups) const`
- Source: `include/auth/ldap_authenticator.h`:202
- Brief: Map Groups To Roles.
- Parameters:
  - `groups` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: groups Input parameter. Return value.

#### `LDAPAuthenticator & operator=(LDAPAuthenticator &&)=delete`
- Source: `include/auth/ldap_authenticator.h`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPAuthenticator &&): n/a

#### `LDAPAuthenticator & operator=(const LDAPAuthenticator &)=delete`
- Source: `include/auth/ldap_authenticator.h`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LDAPAuthenticator &): n/a

#### `LDAPAuthResult performBind(const std::string &username, const std::string &dn, const std::string &password)`
- Source: `include/auth/ldap_authenticator.h`:220
- Brief: Perform Bind.
- Parameters:
  - `username` (const std::string &): Input parameter.
  - `dn` (const std::string &): Input parameter.
  - `password` (const std::string &): Input parameter.
- Return: Return value.
- Details: ------------------------------------------------------------------------ PERMANENT FALLBACK NOTE: Purpose: Link-compatible LDAP fallback for builds without libldap. username Input parameter. dn Input parameter. password Input parameter. Return value. username Input parameter. dn Input parameter. password Input parameter. Return value. Returns a hard-failure from performBind() so any LDAP-gated authentication request is explicitly rejected rather than accidentally allowed. This fallback is permanent for builds that intentionally omit libldap. Activation: Compiled when THEMIS_HAS_LDAP is NOT defined. Set via -DTHEMIS_ENABLE_LDAP=ON in CMake to enable the real implementation. Production Delta: All LDAP-based logins will fail with an explicit error message. No silent pass-through; the rejection is logged and audited. Real implementation: Install libldap and build with -DTHEMIS_ENABLE_LDAP=ON. The #if THEMIS_HAS_LDAP branch above handles TLS, paging, group membership, and attribute mapping. Roadmap ref: src/auth/FUTURE_ENHANCEMENTS.md § "LDAP Group Membership (v1.6.0)" ------------------------------------------------------------------------

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/ldap_authenticator.h`:154
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `void setLdapBindFn(LdapBindFn fn)`
- Source: `include/auth/ldap_authenticator.h`:133
- Brief: Set Ldap Bind Fn.
- Parameters:
  - `fn` (LdapBindFn): Input parameter.
- Details: fn Input parameter.

#### `~LDAPAuthenticator()`
- Source: `include/auth/ldap_authenticator.h`:141
- Brief: n/a
- Parameters: none

### themis::auth::LDAPConnectionPool

#### `LDAPConnectionPool(LDAPConnectionPool &&)=delete`
- Source: `include/auth/ldap_connection_pool.h`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPConnectionPool &&): n/a

#### `LDAPConnectionPool(const LDAPConnectionPool &)=delete`
- Source: `include/auth/ldap_connection_pool.h`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LDAPConnectionPool &): n/a

#### `LDAPConnectionPool(const LDAPPoolConfig &config)`
- Source: `include/auth/ldap_connection_pool.h`:88
- Brief: LDAPConnection Pool.
- Parameters:
  - `config` (const LDAPPoolConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `int activeConnections() const noexcept`
- Source: `include/auth/ldap_connection_pool.h`:127
- Brief: Active Connections.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `std::unique_ptr< PooledConnection > checkout()`
- Source: `include/auth/ldap_connection_pool.h`:101
- Brief: Checkout.
- Parameters: none
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: Return value. Return value. AuthException if an error occurs. Calls: spdlog::warn(), std::chrono::steady_clock::now(), std::chrono::milliseconds(), lock(), empty(), front(), pop_front(), unlock().

#### `const LDAPPoolConfig & config() const noexcept`
- Source: `include/auth/ldap_connection_pool.h`:103
- Brief: n/a
- Parameters: none

#### `LDAP * createConnection()`
- Source: `include/auth/ldap_connection_pool.h`:143
- Brief: Create Connection.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result.

#### `void destroyHandle(LDAP *handle) noexcept`
- Source: `include/auth/ldap_connection_pool.h`:157
- Brief: Destroy Handle.
- Parameters:
  - `handle` (LDAP *): Input/output parameter.
- Details: handle Input/output parameter. Exception safety: noexcept.

#### `int idleConnections() const noexcept`
- Source: `include/auth/ldap_connection_pool.h`:120
- Brief: Idle Connections.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `bool isHealthy(LDAP *handle) const`
- Source: `include/auth/ldap_connection_pool.h`:150
- Brief: Is Healthy.
- Parameters:
  - `handle` (LDAP *): Input/output parameter.
- Return: True when the operation succeeds.
- Details: handle Input/output parameter. True when the operation succeeds.

#### `LDAPConnectionPool & operator=(LDAPConnectionPool &&)=delete`
- Source: `include/auth/ldap_connection_pool.h`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPConnectionPool &&): n/a

#### `LDAPConnectionPool & operator=(const LDAPConnectionPool &)=delete`
- Source: `include/auth/ldap_connection_pool.h`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LDAPConnectionPool &): n/a

#### `int poolSize() const noexcept`
- Source: `include/auth/ldap_connection_pool.h`:113
- Brief: -------------------------------------------------------------------- Metrics accessors (used by auth_metrics) --------------------------------------------------------------------
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `void returnConnection(LDAP *handle, bool stale)`
- Source: `include/auth/ldap_connection_pool.h`:137
- Brief: Return Connection.
- Parameters:
  - `handle` (LDAP *): Input/output parameter.
  - `stale` (bool): Input parameter.
- Details: handle Input/output parameter. stale Input parameter. handle Input/output parameter. stale Input parameter. Calls: lock(), size(), push_back(), notify_one(), destroyHandle(), notify_all().

#### `void setAuditLogger(utils::AuditLogger *logger) noexcept`
- Source: `include/auth/ldap_connection_pool.h`:105
- Brief: n/a
- Parameters:
  - `logger` (utils::AuditLogger *): n/a

#### `~LDAPConnectionPool()`
- Source: `include/auth/ldap_connection_pool.h`:89
- Brief: n/a
- Parameters: none

### themis::auth::MFAAuthenticator

#### `MFAAuthenticator()`
- Source: `include/auth/mfa_authenticator.h`:71
- Brief: n/a
- Parameters: none

#### `MFAAuthenticator(const Config &config)`
- Source: `include/auth/mfa_authenticator.h`:77
- Brief: MFAAuthenticator.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `std::vector< uint8_t > base32Decode(const std::string &input) const`
- Source: `include/auth/mfa_authenticator.h`:179
- Brief: Convert Base32 string to binary.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value.

#### `std::string base32Encode(const std::vector< uint8_t > &input) const`
- Source: `include/auth/mfa_authenticator.h`:186
- Brief: Convert binary to Base32 string.
- Parameters:
  - `input` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value.

#### `std::string computeTOTP(const std::vector< uint8_t > &secret, uint64_t time_counter) const`
- Source: `include/auth/mfa_authenticator.h`:169
- Brief: Compute TOTP value for given time counter.
- Parameters:
  - `secret` (const std::vector< uint8_t > &): Input parameter.
  - `time_counter` (uint64_t): Input parameter.
- Return: Return value.
- Details: secret Input parameter. time_counter Input parameter. Return value.

#### `EnrollmentData generateEnrollment(const std::string &user_id)`
- Source: `include/auth/mfa_authenticator.h`:106
- Brief: Generate Enrollment.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: user_id Identifier of the user. Return value. user_id Identifier of the user. Return value. Calls: generateSecret(), generateRecoveryCodes(), std::chrono::system_clock::now(), spdlog::info(), logSecurityEvent().

#### `std::string generateProvisioningURI(const EnrollmentData &enrollment) const`
- Source: `include/auth/mfa_authenticator.h`:113
- Brief: Generate Provisioning URI.
- Parameters:
  - `enrollment` (const EnrollmentData &): Input parameter.
- Return: Return value.
- Details: enrollment Input parameter. Return value.

#### `std::string generateRecoveryCode() const`
- Source: `include/auth/mfa_authenticator.h`:161
- Brief: Generate single recovery code.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< std::string > generateRecoveryCodes(const std::string &user_id)`
- Source: `include/auth/mfa_authenticator.h`:138
- Brief: Generate Recovery Codes.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: user_id Identifier of the user. Return value. user_id Identifier of the user. Return value. Calls: reserve(), push_back(), generateRecoveryCode(), spdlog::debug().

#### `std::string generateSecret() const`
- Source: `include/auth/mfa_authenticator.h`:155
- Brief: Generate random secret for TOTP (20 bytes = 160 bits).
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string getCurrentTOTP(const std::string &secret_base32, std::optional< std::chrono::system_clock::time_point > timestamp=std::nullopt) const`
- Source: `include/auth/mfa_authenticator.h`:140
- Brief: n/a
- Parameters:
  - `secret_base32` (const std::string &): n/a
  - `timestamp` (std::optional< std::chrono::system_clock::time_point >): n/a

#### `uint64_t getTimeCounter(std::chrono::system_clock::time_point timestamp) const`
- Source: `include/auth/mfa_authenticator.h`:205
- Brief: Get time counter from timestamp.
- Parameters:
  - `timestamp` (std::chrono::system_clock::time_point): Input parameter.
- Return: Return value.
- Details: timestamp Input parameter. Return value.

#### `std::vector< uint8_t > hmacSHA1(const std::vector< uint8_t > &key, const std::vector< uint8_t > &message) const`
- Source: `include/auth/mfa_authenticator.h`:195
- Brief: Hmac SHA1.
- Parameters:
  - `key` (const std::vector< uint8_t > &): Input parameter.
  - `message` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: key Input parameter. message Input parameter. Return value.

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/mfa_authenticator.h`:85
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `void setAuthAuditLogger(AuthAuditLogger *logger)`
- Source: `include/auth/mfa_authenticator.h`:92
- Brief: Set Auth Audit Logger.
- Parameters:
  - `logger` (AuthAuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuthAuditLogger without additional internal calls.

#### `void setMetrics(AuthMetrics *metrics)`
- Source: `include/auth/mfa_authenticator.h`:99
- Brief: Set Metrics.
- Parameters:
  - `metrics` (AuthMetrics *): Input/output parameter.
- Details: metrics Input/output parameter. Implements setMetrics without additional internal calls.

#### `bool validateRecoveryCode(EnrollmentData &enrollment, const std::string &recovery_code)`
- Source: `include/auth/mfa_authenticator.h`:128
- Brief: Validate Recovery Code.
- Parameters:
  - `enrollment` (EnrollmentData &): Input/output parameter.
  - `recovery_code` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: enrollment Input/output parameter. recovery_code Input parameter. True when the operation succeeds. enrollment Input/output parameter. recovery_code Input parameter. True when the operation succeeds. Calls: size(), CRYPTO_memcmp(), data(), erase(), begin(), spdlog::info(), logSecurityEvent(), spdlog::debug().

#### `bool validateTOTP(const std::string &secret_base32, const std::string &code, std::optional< std::chrono::system_clock::time_point > timestamp=std::nullopt, const std::string &subject="") const`
- Source: `include/auth/mfa_authenticator.h`:115
- Brief: n/a
- Parameters:
  - `secret_base32` (const std::string &): n/a
  - `code` (const std::string &): n/a
  - `timestamp` (std::optional< std::chrono::system_clock::time_point >): n/a
  - `subject` (const std::string &): n/a

#### `~MFAAuthenticator()=default`
- Source: `include/auth/mfa_authenticator.h`:78
- Brief: n/a
- Parameters: none

### themis::auth::MFAAuthenticator::EnrollmentData

#### `EnrollmentData from_json(const nlohmann::json &j)`
- Source: `include/auth/mfa_authenticator.h`:68
- Brief: From json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: at(), std::chrono::system_clock::from_time_t().

#### `nlohmann::json to_json() const`
- Source: `include/auth/mfa_authenticator.h`:62
- Brief: To json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::auth::MTLSAuthenticator

#### `MTLSAuthenticator(const Config &config)`
- Source: `include/auth/mtls_authenticator.h`:65
- Brief: MTLSAuthenticator.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `MTLSClaims authenticate(const std::string &cert_pem)`
- Source: `include/auth/mtls_authenticator.h`:78
- Brief: Authenticate.
- Parameters:
  - `cert_pem` (const std::string &): Input parameter.
- Return: Authentication result.
- Throws:
  - AuthException: if an error occurs.
- Details: cert_pem Input parameter. Authentication result. cert_pem Input parameter. Authentication result. AuthException if an error occurs. Calls: lock(), parsePEM(), AuthError(), opensslError(), ctx(), X509_STORE_CTX_new(), X509_STORE_CTX_init(), get().

#### `MTLSClaims authenticateDER(const std::vector< uint8_t > &cert_der)`
- Source: `include/auth/mtls_authenticator.h`:85
- Brief: Authenticate DER.
- Parameters:
  - `cert_der` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: cert_der Input parameter. Return value. cert_der Input parameter. Return value. AuthException if an error occurs. Calls: der_bio(), BIO_new_mem_buf(), data(), size(), AuthError(), cert(), d2i_X509_bio(), get().

#### `std::string certFingerprint(const std::string &cert_pem)`
- Source: `include/auth/mtls_authenticator.h`:125
- Brief: Cert Fingerprint.
- Parameters:
  - `cert_pem` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: cert_pem Input parameter. Return value. cert_pem Input parameter. Return value. AuthException if an error occurs. Calls: parsePEM(), AuthError(), opensslError(), computeFingerprint(), get().

#### `std::string computeFingerprint(void *x509)`
- Source: `include/auth/mtls_authenticator.h`:179
- Brief: Compute Fingerprint.
- Parameters:
  - `x509` (void *): Input/output parameter.
- Return: Return value.
- Details: x509 Input/output parameter. Return value. x509_ptr Input/output parameter. Return value. Calls: X509_digest(), EVP_sha256(), std::setfill(), std::setw(), str().

#### `std::vector< std::string > extractSANs(void *x509, int san_type)`
- Source: `include/auth/mtls_authenticator.h`:186
- Brief: Extract SANs.
- Parameters:
  - `x509` (void *): Input/output parameter.
  - `san_type` (int): Input parameter.
- Return: Return value.
- Details: x509 Input/output parameter. san_type Input parameter. Return value. x509_ptr Input/output parameter. san_type Input parameter. Return value. Calls: X509_get_ext_d2i(), sk_GENERAL_NAME_num(), sk_GENERAL_NAME_value(), ASN1_STRING_get0_data(), ASN1_STRING_length(), emplace_back(), push_back(), str().

#### `std::string extractSubjectCN(const std::string &cert_pem)`
- Source: `include/auth/mtls_authenticator.h`:132
- Brief: Extract Subject CN.
- Parameters:
  - `cert_pem` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: cert_pem Input parameter. Return value. cert_pem Input parameter. Return value. AuthException if an error occurs. Calls: parsePEM(), AuthError(), opensslError(), X509_get_subject_name(), get(), X509_NAME_get_index_by_NID(), X509_NAME_get_entry(), X509_NAME_ENTRY_get_data().

#### `bool initCAStore()`
- Source: `include/auth/mtls_authenticator.h`:155
- Brief: Init CAStore.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Calls: reset(), X509_STORE_new(), spdlog::error(), bio(), BIO_new_mem_buf(), data(), size(), ca().

#### `bool initCRL()`
- Source: `include/auth/mtls_authenticator.h`:160
- Brief: Init CRL.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Calls: bio(), BIO_new_mem_buf(), data(), size(), reset(), PEM_read_bio_X509_CRL(), get(), spdlog::error().

#### `bool isRevoked(const std::string &serial_hex) const`
- Source: `include/auth/mtls_authenticator.h`:108
- Brief: Is Revoked.
- Parameters:
  - `serial_hex` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: serial_hex Input parameter. True when the operation succeeds.

#### `void revokeCertificate(const std::string &serial_hex)`
- Source: `include/auth/mtls_authenticator.h`:95
- Brief: Revoke Certificate.
- Parameters:
  - `serial_hex` (const std::string &): Input parameter.
- Throws:
  - AuthException: if an error occurs.
- Details: serial_hex Input parameter. serial_hex Input parameter. AuthException if an error occurs. Calls: empty(), AuthError(), lock(), insert().

#### `size_t revokedCount() const`
- Source: `include/auth/mtls_authenticator.h`:114
- Brief: Revoked Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string serialToHex(void *serial)`
- Source: `include/auth/mtls_authenticator.h`:173
- Brief: Serial To Hex.
- Parameters:
  - `serial` (void *): Input/output parameter.
- Return: Return value.
- Details: serial Input/output parameter. Return value. serial_ptr Input/output parameter. Return value. Calls: bn(), ASN1_INTEGER_to_BN(), BN_bn2hex(), get(), result(), OPENSSL_free(), tolower().

#### `void setAuditLogger(AuthAuditLogger *logger)`
- Source: `include/auth/mtls_authenticator.h`:139
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (AuthAuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `void unrevokeCertificate(const std::string &serial_hex)`
- Source: `include/auth/mtls_authenticator.h`:101
- Brief: Unrevoke Certificate.
- Parameters:
  - `serial_hex` (const std::string &): Input parameter.
- Details: serial_hex Input parameter. serial_hex Input parameter. Calls: lock(), erase().

#### `std::string x509NameToString(void *name)`
- Source: `include/auth/mtls_authenticator.h`:167
- Brief: X509 Name To String.
- Parameters:
  - `name` (void *): Input/output parameter.
- Return: Return value.
- Details: name Input/output parameter. Return value. name_ptr Input/output parameter. Return value. Calls: bio(), BIO_new(), BIO_s_mem(), X509_NAME_print_ex(), get(), BIO_get_mem_ptr(), std::string().

#### `~MTLSAuthenticator()`
- Source: `include/auth/mtls_authenticator.h`:67
- Brief: n/a
- Parameters: none

### themis::auth::MTLSClaims

#### `bool isExpired() const`
- Source: `include/auth/mtls_authenticator.h`:41
- Brief: n/a
- Parameters: none

### themis::auth::OAuthDeviceFlow

#### `OAuthDeviceFlow(const Config &config)`
- Source: `include/auth/oauth_device_flow.h`:72
- Brief: OAuth Device Flow.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `JWTClaims authenticate(std::function< void(const DeviceCodeResponse &)> progress_cb=nullptr)`
- Source: `include/auth/oauth_device_flow.h`:95
- Brief: n/a
- Parameters:
  - `progress_cb` (std::function< void(const DeviceCodeResponse &)>): n/a

#### `std::string buildFormBody(const std::vector< std::pair< std::string, std::string > > &params)`
- Source: `include/auth/oauth_device_flow.h`:133
- Brief: n/a
- Parameters:
  - `params` (const std::vector< std::pair< std::string, std::string > > &): n/a

#### `std::string httpPost(const std::string &url, const std::string &body)`
- Source: `include/auth/oauth_device_flow.h`:125
- Brief: Http Post.
- Parameters:
  - `url` (const std::string &): Input parameter.
  - `body` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: url Input parameter. body Input parameter. Return value. url Input parameter. body Input parameter. Return value. std::runtime_error if an error occurs. Calls: http_post_fn_(), curl_easy_init(), curl_easy_setopt(), c_str(), size(), curl_slist_append(), curl_multi_init(), curl_slist_free_all().

#### `TokenResponse pollForToken(const std::string &device_code, PollStatus &status_out)`
- Source: `include/auth/oauth_device_flow.h`:86
- Brief: Poll For Token.
- Parameters:
  - `device_code` (const std::string &): Input parameter.
  - `status_out` (PollStatus &): Input/output parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: ============================================================================ RFC 8628 §3. device_code Input parameter. status_out Input/output parameter. Return value. device_code Input parameter. status_out Input/output parameter. Return value. AuthException if an error occurs. 4 – Device Access Token Request ============================================================================ Calls: empty(), emplace_back(), buildFormBody(), spdlog::debug(), httpPost(), what(), find(), spdlog::error().

#### `DeviceCodeResponse requestDeviceCode()`
- Source: `include/auth/oauth_device_flow.h`:78
- Brief: Request Device Code.
- Parameters: none
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: ============================================================================ RFC 8628 §3. Return value. Return value. AuthException if an error occurs. 1 – Device Authorization Request ============================================================================ Calls: empty(), size(), emplace_back(), buildFormBody(), spdlog::debug(), httpPost(), spdlog::error(), what().

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/oauth_device_flow.h`:112
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `void setHttpPostForTesting(std::function< std::string(const std::string &url, const std::string &body)> fn)`
- Source: `include/auth/oauth_device_flow.h`:103
- Brief: n/a
- Parameters:
  - `fn` (std::function< std::string(const std::string &url, const std::string &body)>): n/a

#### `std::string urlEncode(const std::string &value)`
- Source: `include/auth/oauth_device_flow.h`:132
- Brief: Url Encode.
- Parameters:
  - `value` (const std::string &): Input parameter.
- Return: Return value.
- Details: value Input parameter. Return value. value Input parameter. Return value. Calls: curl_easy_init(), curl_easy_escape(), c_str(), size(), curl_free(), curl_easy_cleanup().

#### `JWTClaims validateIdToken(const TokenResponse &token_response)`
- Source: `include/auth/oauth_device_flow.h`:93
- Brief: Validate Id Token.
- Parameters:
  - `token_response` (const TokenResponse &): Input parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: token_response Input parameter. Return value. token_response Input parameter. Return value. AuthException if an error occurs. Calls: empty(), AuthError(), validator(), parseAndValidate().

### themis::auth::OAuthPKCEFlow

#### `OAuthPKCEFlow(const Config &config)`
- Source: `include/auth/oauth_pkce_flow.h`:57
- Brief: OAuth PKCEFlow.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `std::string base64UrlEncode(const unsigned char *data, std::size_t len)`
- Source: `include/auth/oauth_pkce_flow.h`:121
- Brief: Base64 Url Encode.
- Parameters:
  - `data` (const unsigned char *): Input parameter.
  - `len` (std::size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. len Input parameter. Return value. data Input parameter. len Input parameter. Return value. Calls: reserve(), empty(), back(), pop_back().

#### `std::string buildAuthorizationUrl(const PKCEChallenge &challenge, const std::string &state="") const`
- Source: `include/auth/oauth_pkce_flow.h`:65
- Brief: n/a
- Parameters:
  - `challenge` (const PKCEChallenge &): n/a
  - `state` (const std::string &): n/a

#### `std::string buildFormBody(const std::vector< std::pair< std::string, std::string > > &params)`
- Source: `include/auth/oauth_pkce_flow.h`:134
- Brief: n/a
- Parameters:
  - `params` (const std::vector< std::pair< std::string, std::string > > &): n/a

#### `TokenResponse exchangeCode(const std::string &authorization_code, const std::string &code_verifier)`
- Source: `include/auth/oauth_pkce_flow.h`:74
- Brief: Exchange Code.
- Parameters:
  - `authorization_code` (const std::string &): Input parameter.
  - `code_verifier` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: ============================================================================ RFC 7636 §4. authorization_code Input parameter. code_verifier Input parameter. Return value. authorization_code Input parameter. code_verifier Input parameter. Return value. AuthException if an error occurs. 5 – Token Exchange ============================================================================ Calls: empty(), AuthError(), buildFormBody(), spdlog::debug(), httpPost(), std::current_exception(), what(), find().

#### `void fillRandomBytes(unsigned char *buf, std::size_t len)`
- Source: `include/auth/oauth_pkce_flow.h`:113
- Brief: Fill Random Bytes.
- Parameters:
  - `buf` (unsigned char *): Input/output parameter.
  - `len` (std::size_t): Input parameter.
- Throws:
  - AuthException: if an error occurs.
- Details: buf Input/output parameter. len Input parameter. buf Input/output parameter. len Input parameter. AuthException if an error occurs. Calls: rand_bytes_fn_(), RAND_bytes(), AuthError().

#### `PKCEChallenge generateChallenge()`
- Source: `include/auth/oauth_pkce_flow.h`:63
- Brief: Generate Challenge.
- Parameters: none
- Return: Return value.
- Details: ============================================================================ RFC 7636 §4. Return value. Return value. 1 – Generate code_verifier and code_challenge ============================================================================ Calls: fillRandomBytes(), data(), size(), base64UrlEncode(), sha256(), spdlog::debug().

#### `std::string httpPost(const std::string &url, const std::string &body)`
- Source: `include/auth/oauth_pkce_flow.h`:107
- Brief: Http Post.
- Parameters:
  - `url` (const std::string &): Input parameter.
  - `body` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: url Input parameter. body Input parameter. Return value. url Input parameter. body Input parameter. Return value. std::runtime_error if an error occurs. Calls: http_post_fn_(), curl_easy_init(), curl_easy_setopt(), c_str(), size(), curl_slist_append(), curl_multi_init(), curl_slist_free_all().

#### `void setHttpPostForTesting(std::function< std::string(const std::string &url, const std::string &body)> fn)`
- Source: `include/auth/oauth_pkce_flow.h`:88
- Brief: n/a
- Parameters:
  - `fn` (std::function< std::string(const std::string &url, const std::string &body)>): n/a

#### `void setRandBytesForTesting(std::function< void(unsigned char *buf, std::size_t len)> fn)`
- Source: `include/auth/oauth_pkce_flow.h`:92
- Brief: n/a
- Parameters:
  - `fn` (std::function< void(unsigned char *buf, std::size_t len)>): n/a

#### `std::string sha256(const std::string &input)`
- Source: `include/auth/oauth_pkce_flow.h`:127
- Brief: Sha256.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: input Input parameter. Return value. input Input parameter. Return value. AuthException if an error occurs. Calls: EVP_MD_CTX_new(), AuthError(), EVP_DigestInit_ex(), EVP_sha256(), EVP_DigestUpdate(), data(), size(), EVP_DigestFinal_ex().

#### `std::string urlEncode(const std::string &value)`
- Source: `include/auth/oauth_pkce_flow.h`:133
- Brief: Url Encode.
- Parameters:
  - `value` (const std::string &): Input parameter.
- Return: Return value.
- Details: value Input parameter. Return value. value Input parameter. Return value. Calls: curl_easy_init(), curl_easy_escape(), c_str(), size(), curl_free(), curl_easy_cleanup().

#### `JWTClaims validateIdToken(const TokenResponse &token_response)`
- Source: `include/auth/oauth_pkce_flow.h`:82
- Brief: Validate Id Token.
- Parameters:
  - `token_response` (const TokenResponse &): Input parameter.
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: token_response Input parameter. Return value. token_response Input parameter. Return value. AuthException if an error occurs. Calls: empty(), AuthError(), validator(), parseAndValidate().

### themis::auth::OIDCProvider

#### `OIDCProvider(OIDCProvider &&) noexcept=default`
- Source: `include/auth/oidc_provider.h`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (OIDCProvider &&): n/a

#### `OIDCProvider(const OIDCProvider &)=delete`
- Source: `include/auth/oidc_provider.h`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (const OIDCProvider &): n/a

#### `OIDCProvider(const OIDCProviderConfig &config)`
- Source: `include/auth/oidc_provider.h`:62
- Brief: OIDCProvider.
- Parameters:
  - `config` (const OIDCProviderConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `JWTValidatorConfig buildValidatorConfig() const`
- Source: `include/auth/oidc_provider.h`:166
- Brief: Build JWTValidatorConfig from the discovery document and provider config.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `const std::string & clientId() const`
- Source: `include/auth/oidc_provider.h`:118
- Brief: n/a
- Parameters: none

#### `const std::string & clientSecret() const`
- Source: `include/auth/oidc_provider.h`:120
- Brief: n/a
- Parameters: none

#### `OAuthDeviceFlow createDeviceFlow()`
- Source: `include/auth/oidc_provider.h`:112
- Brief: Create Device Flow.
- Parameters: none
- Return: Return value.
- Throws:
  - AuthException: if an error occurs.
- Details: Return value. Return value. AuthException if an error occurs. Calls: has_value(), discover(), empty(), AuthError(), OAuthDeviceFlow().

#### `void discover()`
- Source: `include/auth/oidc_provider.h`:79
- Brief: Discover.
- Parameters: none
- Throws:
  - AuthException: if an error occurs.
- Details: AuthException if an error occurs. Calls: has_value(), empty(), back(), pop_back(), spdlog::debug(), httpGet(), spdlog::error(), what().

#### `const OIDCDiscoveryDocument & discoveryDocument()`
- Source: `include/auth/oidc_provider.h`:85
- Brief: Discovery Document.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: has_value(), discover().

#### `std::string httpGet(const std::string &url) const`
- Source: `include/auth/oidc_provider.h`:153
- Brief: Fetch a URL and return the raw body (uses libcurl unless overridden).
- Parameters:
  - `url` (const std::string &): Input parameter.
- Return: Return value.
- Details: url Input parameter. Return value.

#### `OIDCProvider & operator=(OIDCProvider &&) noexcept=default`
- Source: `include/auth/oidc_provider.h`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (OIDCProvider &&): n/a

#### `OIDCProvider & operator=(const OIDCProvider &)=delete`
- Source: `include/auth/oidc_provider.h`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (const OIDCProvider &): n/a

#### `OIDCDiscoveryDocument parseDiscovery(const std::string &json_body)`
- Source: `include/auth/oidc_provider.h`:160
- Brief: Parse raw JSON into OIDCDiscoveryDocument.
- Parameters:
  - `json_body` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: static json_body Input parameter. Return value. json_body Input parameter. Return value. std::runtime_error if an error occurs. Calls: nlohmann::json::parse(), is_object(), at(), contains(), is_string(), is_array().

#### `void setDiscoveryDocumentForTesting(const OIDCDiscoveryDocument &doc)`
- Source: `include/auth/oidc_provider.h`:130
- Brief: Set Discovery Document For Testing.
- Parameters:
  - `doc` (const OIDCDiscoveryDocument &): Input parameter.
- Details: doc Input parameter. doc Input parameter. Calls: buildValidatorConfig().

#### `void setHttpGetForTesting(std::function< std::string(const std::string &url)> fn)`
- Source: `include/auth/oidc_provider.h`:132
- Brief: n/a
- Parameters:
  - `fn` (std::function< std::string(const std::string &url)>): n/a

#### `JWTClaims validateToken(const std::string &token)`
- Source: `include/auth/oidc_provider.h`:96
- Brief: Validate Token.
- Parameters:
  - `token` (const std::string &): Input parameter.
- Return: Return value.
- Details: token Input parameter. Return value. token Input parameter. Return value. Calls: discover(), parseAndValidate().

#### `JWTValidator & validator()`
- Source: `include/auth/oidc_provider.h`:102
- Brief: Validator.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: discover().

### themis::auth::PasskeyAuthenticator

#### `PasskeyAuthenticator(std::string relying_party_id, std::string expected_origin)`
- Source: `include/auth/passkey_authenticator.h`:138
- Brief: Passkey Authenticator.
- Parameters:
  - `relying_party_id` (std::string): Identifier of the relying party.
  - `expected_origin` (std::string): Input parameter.
- Return: Return value.
- Details: relying_party_id Identifier of the relying party. expected_origin Input parameter. Return value.

#### `PasskeyChallenge beginAuthentication(const std::string &user_id="") override`
- Source: `include/auth/passkey_authenticator.h`:149
- Brief: Begin Authentication.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: user_id Identifier of the user. Return value. Calls: generateSecureChallenge(), std::chrono::system_clock::now(), std::chrono::minutes(), lock(), spdlog::debug().

#### `PasskeyChallenge beginRegistration(const std::string &user_id) override`
- Source: `include/auth/passkey_authenticator.h`:144
- Brief: Begin Registration.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: user_id Identifier of the user. Return value. Calls: generateSecureChallenge(), std::chrono::system_clock::now(), std::chrono::minutes(), lock(), spdlog::debug().

#### `bool cloneDetectionFailed(uint32_t stored_sign_count, uint32_t assertion_sign_count) noexcept`
- Source: `include/auth/passkey_authenticator.h`:183
- Brief: n/a
- Parameters:
  - `stored_sign_count` (uint32_t): n/a
  - `assertion_sign_count` (uint32_t): n/a

#### `PasskeyVerifyResult completeAuthentication(const std::string &challenge_id, const PasskeyAssertionResponse &response, std::string &out_user_id) override`
- Source: `include/auth/passkey_authenticator.h`:151
- Brief: Complete Authentication.
- Parameters:
  - `challenge_id` (const std::string &): Identifier of the challenge.
  - `response` (const PasskeyAssertionResponse &): Input parameter.
  - `out_user_id` (std::string &): Identifier of the out user.
- Return: Return value.
- Details: challenge_id Identifier of the challenge. response Input parameter. out_user_id Identifier of the out user. Return value.

#### `bool completeRegistration(const std::string &challenge_id, const PasskeyCredential &credential) override`
- Source: `include/auth/passkey_authenticator.h`:146
- Brief: Complete Registration.
- Parameters:
  - `challenge_id` (const std::string &): Identifier of the challenge.
  - `credential` (const PasskeyCredential &): Input parameter.
- Return: True when the operation succeeds.
- Details: challenge_id Identifier of the challenge. credential Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), spdlog::warn(), erase(), std::chrono::system_clock::now(), logPasskeyRegistered(), spdlog::info().

#### `std::string generateSecureChallenge(size_t bytes=32) const`
- Source: `include/auth/passkey_authenticator.h`:198
- Brief: n/a
- Parameters:
  - `bytes` (size_t): n/a

#### `std::vector< PasskeyCredential > listCredentials(const std::string &user_id) const override`
- Source: `include/auth/passkey_authenticator.h`:156
- Brief: n/a
- Parameters:
  - `user_id` (const std::string &): n/a

#### `bool revokeCredential(const std::string &credential_id) override`
- Source: `include/auth/passkey_authenticator.h`:159
- Brief: Revoke Credential.
- Parameters:
  - `credential_id` (const std::string &): Identifier of the credential.
- Return: True when the operation succeeds.
- Details: credential_id Identifier of the credential. True when the operation succeeds. Calls: lock(), find(), end(), spdlog::warn(), erase(), spdlog::info().

#### `void setAuditLogger(AuthAuditLogger *logger)`
- Source: `include/auth/passkey_authenticator.h`:170
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (AuthAuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `bool verifyAuthentication(const PasskeyChallenge &challenge, const PasskeyCredential &credential, const std::string &assertion_response_b64)`
- Source: `include/auth/passkey_authenticator.h`:179
- Brief: Verify Authentication.
- Parameters:
  - `challenge` (const PasskeyChallenge &): Input parameter.
  - `credential` (const PasskeyCredential &): Input parameter.
  - `assertion_response_b64` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: challenge Input parameter. credential Input parameter. assertion_response_b64 Input parameter. True when the operation succeeds.

#### `bool verifyRegistration(const PasskeyChallenge &challenge, const std::string &attestation_response_b64)`
- Source: `include/auth/passkey_authenticator.h`:176
- Brief: Verify Registration.
- Parameters:
  - `challenge` (const PasskeyChallenge &): Input parameter.
  - `attestation_response_b64` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: challenge Input parameter. attestation_response_b64 Input parameter. True when the operation succeeds.

### themis::auth::PasswordPolicy

#### `PasswordPolicy()`
- Source: `include/auth/password_policy.h`:54
- Brief: n/a
- Parameters: none

#### `PasswordPolicy(const Config &config)`
- Source: `include/auth/password_policy.h`:61
- Brief: Password Policy.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `PasswordPolicy basic()`
- Source: `include/auth/password_policy.h`:110
- Brief: Basic.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: Config(), PasswordPolicy().

#### `double computeEntropy(const std::string &password)`
- Source: `include/auth/password_policy.h`:91
- Brief: Compute Entropy.
- Parameters:
  - `password` (const std::string &): Input parameter.
- Return: Return value.
- Details: password Input parameter. Return value. password Input parameter. Return value. Calls: empty(), size(), std::log2().

#### `const Config & getConfig() const`
- Source: `include/auth/password_policy.h`:77
- Brief: n/a
- Parameters: none

#### `bool isCompliant(const std::string &password) const`
- Source: `include/auth/password_policy.h`:75
- Brief: Is Compliant.
- Parameters:
  - `password` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: password Input parameter. True when the operation succeeds.

#### `PasswordPolicy nistGuidelines()`
- Source: `include/auth/password_policy.h`:98
- Brief: - Preset factories ---------------------------------------------
- Parameters: none
- Return: Return value.
- Details: Nist Guidelines. Return value. Return value. Calls: Config(), PasswordPolicy().

#### `void setConfig(const Config &config)`
- Source: `include/auth/password_policy.h`:84
- Brief: Set Config.
- Parameters:
  - `config` (const Config &): Input parameter.
- Details: config Input parameter. Implements setConfig without additional internal calls.

#### `PasswordPolicy strict()`
- Source: `include/auth/password_policy.h`:104
- Brief: Strict.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: Config(), PasswordPolicy().

#### `ValidationResult validate(const std::string &password) const`
- Source: `include/auth/password_policy.h`:68
- Brief: Validate.
- Parameters:
  - `password` (const std::string &): Input parameter.
- Return: Return value.
- Details: password Input parameter. Return value.

### themis::auth::PasswordPolicy::ValidationResult

#### `operator bool() const`
- Source: `include/auth/password_policy.h`:51
- Brief: n/a
- Parameters: none

### themis::auth::PooledConnection

#### `PooledConnection(LDAPConnectionPool &pool, LDAP *handle)`
- Source: `include/auth/ldap_connection_pool.h`:74
- Brief: n/a
- Parameters:
  - `pool` (LDAPConnectionPool &): n/a
  - `handle` (LDAP *): n/a

#### `PooledConnection(PooledConnection &&)`
- Source: `include/auth/ldap_connection_pool.h`:60
- Brief: n/a
- Parameters:
  - `other` (PooledConnection &&): n/a

#### `PooledConnection(const PooledConnection &)=delete`
- Source: `include/auth/ldap_connection_pool.h`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PooledConnection &): n/a

#### `bool isStale() const noexcept`
- Source: `include/auth/ldap_connection_pool.h`:69
- Brief: n/a
- Parameters: none

#### `void markStale() noexcept`
- Source: `include/auth/ldap_connection_pool.h`:67
- Brief: n/a
- Parameters: none

#### `PooledConnection & operator=(PooledConnection &&)`
- Source: `include/auth/ldap_connection_pool.h`:61
- Brief: n/a
- Parameters:
  - `other` (PooledConnection &&): n/a

#### `PooledConnection & operator=(const PooledConnection &)=delete`
- Source: `include/auth/ldap_connection_pool.h`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PooledConnection &): n/a

#### `LDAP * rawHandle() const noexcept`
- Source: `include/auth/ldap_connection_pool.h`:65
- Brief: n/a
- Parameters: none

#### `~PooledConnection()`
- Source: `include/auth/ldap_connection_pool.h`:63
- Brief: n/a
- Parameters: none

### themis::auth::PrincipalValidator

#### `PrincipalValidator(const Config &config=Config::defaults())`
- Source: `include/auth/principal_validator.h`:98
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `void addMappingRule(const MappingRule &rule)`
- Source: `include/auth/principal_validator.h`:129
- Brief: Add Mapping Rule.
- Parameters:
  - `rule` (const MappingRule &): Input parameter.
- Details: rule Input parameter. rule Input parameter. Calls: push_back(), std::sort(), begin(), end().

#### `void addRule(const Rule &rule)`
- Source: `include/auth/principal_validator.h`:123
- Brief: Add Rule.
- Parameters:
  - `rule` (const Rule &): Input parameter.
- Details: rule Input parameter. rule Input parameter. Calls: push_back(), std::sort(), begin(), end().

#### `std::vector< std::string > applyMappingRules(const std::string &principal) const`
- Source: `include/auth/principal_validator.h`:182
- Brief: Apply mapping rules to get roles.
- Parameters:
  - `principal` (const std::string &): Input parameter.
- Return: Return value.
- Details: principal Input parameter. Return value.

#### `void clearRules(RuleType type)`
- Source: `include/auth/principal_validator.h`:135
- Brief: Clear Rules.
- Parameters:
  - `type` (RuleType): Input parameter.
- Details: type Input parameter. type Input parameter. Calls: erase(), std::remove_if(), begin(), end().

#### `void compileRegex(const MappingRule &rule) const`
- Source: `include/auth/principal_validator.h`:200
- Brief: Compile Regex.
- Parameters:
  - `rule` (const MappingRule &): Input parameter.
- Details: rule Input parameter.

#### `void compileRegex(const Rule &rule) const`
- Source: `include/auth/principal_validator.h`:195
- Brief: Compile regex for a rule.
- Parameters:
  - `rule` (const Rule &): Input parameter.
- Details: rule Input parameter.

#### `const PolicyEngine * getAbacEngine() const`
- Source: `include/auth/principal_validator.h`:114
- Brief: n/a
- Parameters: none

#### `const Config & getConfig() const`
- Source: `include/auth/principal_validator.h`:137
- Brief: n/a
- Parameters: none

#### `Statistics getStatistics() const`
- Source: `include/auth/principal_validator.h`:153
- Brief: Return access control statistics.
- Parameters: none
- Return: Access control statistics.
- Details: Access control statistics.

#### `void logAudit(const ValidationResult &result) const`
- Source: `include/auth/principal_validator.h`:189
- Brief: Log Audit.
- Parameters:
  - `result` (const ValidationResult &): Input parameter.
- Details: result Input parameter.

#### `bool matchesMappingRule(const std::string &principal, const MappingRule &rule) const`
- Source: `include/auth/principal_validator.h`:175
- Brief: Check if principal matches a mapping rule.
- Parameters:
  - `principal` (const std::string &): Input parameter.
  - `rule` (const MappingRule &): Input parameter.
- Return: True when the operation succeeds.
- Details: principal Input parameter. rule Input parameter. True when the operation succeeds.

#### `bool matchesRule(const std::string &principal, const Rule &rule) const`
- Source: `include/auth/principal_validator.h`:167
- Brief: Check if principal matches a rule.
- Parameters:
  - `principal` (const std::string &): Input parameter.
  - `rule` (const Rule &): Input parameter.
- Return: True when the operation succeeds.
- Details: principal Input parameter. rule Input parameter. True when the operation succeeds.

#### `void setAbacEngine(PolicyEngine *engine)`
- Source: `include/auth/principal_validator.h`:112
- Brief: Set Abac Engine.
- Parameters:
  - `engine` (PolicyEngine *): Input/output parameter.
- Details: engine Input/output parameter. Implements setAbacEngine without additional internal calls.

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/principal_validator.h`:105
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `ValidationResult validate(const std::string &principal, const ValidationContext &ctx={})`
- Source: `include/auth/principal_validator.h`:116
- Brief: Validate.
- Parameters:
  - `principal` (const std::string &): Input parameter.
  - `ctx` (const ValidationContext &): Input parameter.
- Return: Return value.
- Details: principal Input parameter. ctx Input parameter. Return value. Calls: matchesRule(), empty(), logAudit(), applyMappingRules(), value_or(), authorize().

### themis::auth::PrincipalValidator::Config

#### `Config defaults()`
- Source: `include/auth/principal_validator.h`:95
- Brief: Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

### themis::auth::PrincipalValidatorPresets

#### `PrincipalValidator enterpriseStandard(const std::string &realm)`
- Source: `include/auth/principal_validator.h`:231
- Brief: Enterprise Standard.
- Parameters:
  - `realm` (const std::string &): Input parameter.
- Return: Return value.
- Details: realm Input parameter. Return value. realm Input parameter. Return value. Calls: push_back(), PrincipalValidator().

#### `PrincipalValidator realmRestricted(const std::string &realm)`
- Source: `include/auth/principal_validator.h`:210
- Brief: Realm Restricted.
- Parameters:
  - `realm` (const std::string &): Input parameter.
- Return: Return value.
- Details: realm Input parameter. Return value. realm Input parameter. Return value. Calls: push_back(), PrincipalValidator().

#### `PrincipalValidator withBlacklist(const std::vector< std::string > &blocked_principals)`
- Source: `include/auth/principal_validator.h`:217
- Brief: With Blacklist.
- Parameters:
  - `blocked_principals` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: blocked_principals Input parameter. Return value. blocked_principals Input parameter. Return value. Calls: push_back(), PrincipalValidator().

#### `PrincipalValidator withWhitelist(const std::vector< std::string > &allowed_principals)`
- Source: `include/auth/principal_validator.h`:224
- Brief: With Whitelist.
- Parameters:
  - `allowed_principals` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: allowed_principals Input parameter. Return value. allowed_principals Input parameter. Return value. Calls: push_back(), PrincipalValidator().

### themis::auth::RedisRateLimiterBackend

#### `RedisRateLimiterBackend(const Config &config=Config::defaults())`
- Source: `include/auth/rate_limiter_backend.h`:104
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `RedisRateLimiterBackend(const RedisRateLimiterBackend &)=delete`
- Source: `include/auth/rate_limiter_backend.h`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RedisRateLimiterBackend &): n/a

#### `int64_t getCount(const std::string &key, uint32_t window_seconds) const override`
- Source: `include/auth/rate_limiter_backend.h`:116
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `window_seconds` (uint32_t): n/a

#### `int64_t increment(const std::string &key, uint32_t window_seconds) override`
- Source: `include/auth/rate_limiter_backend.h`:114
- Brief: Increment.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `window_seconds` (uint32_t): Input parameter.
- Return: Return value.
- Details: key Input parameter. window_seconds Input parameter. Return value.

#### `bool isConnected() const`
- Source: `include/auth/rate_limiter_backend.h`:128
- Brief: Is Connected.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `RedisRateLimiterBackend & operator=(const RedisRateLimiterBackend &)=delete`
- Source: `include/auth/rate_limiter_backend.h`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RedisRateLimiterBackend &): n/a

#### `bool reconnect()`
- Source: `include/auth/rate_limiter_backend.h`:134
- Brief: Reconnect.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `void reset(const std::string &key) override`
- Source: `include/auth/rate_limiter_backend.h`:118
- Brief: Reset the modification detection flag.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Details: key Input parameter.

#### `void setGetCountFn(GetCountFn fn)`
- Source: `include/auth/rate_limiter_backend.h`:145
- Brief: Set Get Count Fn.
- Parameters:
  - `fn` (GetCountFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lk(), std::move().

#### `void setIncrementFn(IncrementFn fn)`
- Source: `include/auth/rate_limiter_backend.h`:140
- Brief: Set Increment Fn.
- Parameters:
  - `fn` (IncrementFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lk(), std::move().

#### `void setIsConnectedFn(IsConnectedFn fn)`
- Source: `include/auth/rate_limiter_backend.h`:155
- Brief: Set Is Connected Fn.
- Parameters:
  - `fn` (IsConnectedFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lk(), std::move().

#### `void setReconnectFn(ReconnectFn fn)`
- Source: `include/auth/rate_limiter_backend.h`:160
- Brief: Set Reconnect Fn.
- Parameters:
  - `fn` (ReconnectFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lk(), std::move().

#### `void setResetFn(ResetFn fn)`
- Source: `include/auth/rate_limiter_backend.h`:150
- Brief: Set Reset Fn.
- Parameters:
  - `fn` (ResetFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lk(), std::move().

#### `~RedisRateLimiterBackend() override`
- Source: `include/auth/rate_limiter_backend.h`:105
- Brief: n/a
- Parameters: none

### themis::auth::RedisRateLimiterBackend::Config

#### `Config defaults()`
- Source: `include/auth/rate_limiter_backend.h`:101
- Brief: Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

### themis::auth::RedisTokenBlacklist

#### `RedisTokenBlacklist()`
- Source: `include/auth/redis_token_blacklist.h`:39
- Brief: n/a
- Parameters: none

#### `RedisTokenBlacklist(const Config &config)`
- Source: `include/auth/redis_token_blacklist.h`:45
- Brief: Redis Token Blacklist.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `RedisTokenBlacklist(const RedisTokenBlacklist &)=delete`
- Source: `include/auth/redis_token_blacklist.h`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RedisTokenBlacklist &): n/a

#### `void add(const std::string &jti, std::chrono::system_clock::time_point expiry) override`
- Source: `include/auth/redis_token_blacklist.h`:55
- Brief: Add.
- Parameters:
  - `jti` (const std::string &): Input parameter.
  - `expiry` (std::chrono::system_clock::time_point): Input parameter.
- Details: jti Input parameter. expiry Input parameter. Calls: lock().

#### `bool isConnected() const`
- Source: `include/auth/redis_token_blacklist.h`:70
- Brief: Is Connected.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `bool isRevoked(const std::string &jti) const override`
- Source: `include/auth/redis_token_blacklist.h`:58
- Brief: n/a
- Parameters:
  - `jti` (const std::string &): n/a

#### `RedisTokenBlacklist & operator=(const RedisTokenBlacklist &)=delete`
- Source: `include/auth/redis_token_blacklist.h`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RedisTokenBlacklist &): n/a

#### `void purgeExpired() override`
- Source: `include/auth/redis_token_blacklist.h`:60
- Brief: Purge Expired.
- Parameters: none
- Details: Calls: std::chrono::system_clock::now(), lock(), begin(), end(), erase().

#### `bool reconnect()`
- Source: `include/auth/redis_token_blacklist.h`:76
- Brief: Reconnect.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. True when the operation succeeds. Implements reconnect without additional internal calls.

#### `~RedisTokenBlacklist() override`
- Source: `include/auth/redis_token_blacklist.h`:46
- Brief: n/a
- Parameters: none

### themis::auth::RocksDBTokenBlacklist

#### `RocksDBTokenBlacklist(const Config &config)`
- Source: `include/auth/rocksdb_token_blacklist.h`:50
- Brief: Rocks DBToken Blacklist.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `RocksDBTokenBlacklist(const RocksDBTokenBlacklist &)=delete`
- Source: `include/auth/rocksdb_token_blacklist.h`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RocksDBTokenBlacklist &): n/a

#### `void add(const std::string &jti, std::chrono::system_clock::time_point expiry) override`
- Source: `include/auth/rocksdb_token_blacklist.h`:61
- Brief: Add.
- Parameters:
  - `jti` (const std::string &): Input parameter.
  - `expiry` (std::chrono::system_clock::time_point): Input parameter.
- Details: jti Input parameter. expiry Input parameter. Calls: empty(), Put(), rocksdb::Slice(), encodeExpiry(), ok(), THEMIS_WARN(), ToString(), THEMIS_DEBUG().

#### `std::chrono::system_clock::time_point decodeExpiry(const std::string &val)`
- Source: `include/auth/rocksdb_token_blacklist.h`:101
- Brief: Decode Expiry.
- Parameters:
  - `val` (const std::string &): Input parameter.
- Return: Return value.
- Details: val Input parameter. Return value. val Input parameter. Return value. Calls: size().

#### `std::string encodeExpiry(std::chrono::system_clock::time_point tp)`
- Source: `include/auth/rocksdb_token_blacklist.h`:94
- Brief: Encode Expiry.
- Parameters:
  - `tp` (std::chrono::system_clock::time_point): Input parameter.
- Return: Return value.
- Details: tp Input parameter. Return value. tp Input parameter. Return value. Calls: time_since_epoch(), count(), buf().

#### `bool isRevoked(const std::string &jti) const override`
- Source: `include/auth/rocksdb_token_blacklist.h`:64
- Brief: n/a
- Parameters:
  - `jti` (const std::string &): n/a

#### `RocksDBTokenBlacklist & operator=(const RocksDBTokenBlacklist &)=delete`
- Source: `include/auth/rocksdb_token_blacklist.h`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RocksDBTokenBlacklist &): n/a

#### `void purgeExpired() override`
- Source: `include/auth/rocksdb_token_blacklist.h`:66
- Brief: Purge Expired.
- Parameters: none
- Details: Calls: std::chrono::system_clock::now(), it(), NewIterator(), SeekToFirst(), Valid(), Next(), decodeExpiry(), value().

#### `void purgeLoop()`
- Source: `include/auth/rocksdb_token_blacklist.h`:87
- Brief: Purge Loop.
- Parameters: none
- Details: Calls: load(), lk(), wait_for(), std::chrono::seconds(), unlock(), THEMIS_DEBUG(), purgeExpired().

#### `~RocksDBTokenBlacklist() override`
- Source: `include/auth/rocksdb_token_blacklist.h`:52
- Brief: n/a
- Parameters: none

### themis::auth::SAMLAuthenticator

#### `SAMLAuthenticator(const SAMLConfig &config)`
- Source: `include/auth/saml_authenticator.h`:132
- Brief: SAMLAuthenticator.
- Parameters:
  - `config` (const SAMLConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `std::vector< uint8_t > base64Decode(const std::string &input)`
- Source: `include/auth/saml_authenticator.h`:205
- Brief: Base64 Decode.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: ============================================================================ Base64 decode (for SAMLResponse POST body) ============================================================================ input Input parameter. Return value. input Input parameter. Return value. Calls: BIO_new(), BIO_f_base64(), BIO_new_mem_buf(), data(), size(), BIO_free(), BIO_push(), BIO_set_flags().

#### `AuthnRequestParams buildAuthnRequest(const std::string &relay_state="") const`
- Source: `include/auth/saml_authenticator.h`:140
- Brief: n/a
- Parameters:
  - `relay_state` (const std::string &): n/a

#### `std::string buildAuthnRequestUrl(const std::string &relay_state="") const`
- Source: `include/auth/saml_authenticator.h`:142
- Brief: n/a
- Parameters:
  - `relay_state` (const std::string &): n/a

#### `std::string buildAuthnRequestXml(const std::string &request_id, const std::string &issue_instant) const`
- Source: `include/auth/saml_authenticator.h`:190
- Brief: Build Authn Request Xml.
- Parameters:
  - `request_id` (const std::string &): Identifier of the request.
  - `issue_instant` (const std::string &): Input parameter.
- Return: Return value.
- Details: request_id Identifier of the request. issue_instant Input parameter. Return value.

#### `std::string decryptAssertion(const pugi::xml_node &encrypted_assertion_node) const`
- Source: `include/auth/saml_authenticator.h`:258
- Brief: Decrypt Assertion.
- Parameters:
  - `encrypted_assertion_node` (const pugi::xml_node &): Input parameter.
- Return: Return value.
- Details: encrypted_assertion_node Input parameter. Return value.

#### `std::string deflateAndBase64Encode(const std::string &input)`
- Source: `include/auth/saml_authenticator.h`:198
- Brief: Deflate And Base64 Encode.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: input Input parameter. Return value. input Input parameter. Return value. std::runtime_error if an error occurs. Calls: compressBound(), size(), compressed(), deflateInit2(), data(), deflate(), deflateEnd(), resize().

#### `std::string generateRequestId()`
- Source: `include/auth/saml_authenticator.h`:235
- Brief: Generate Request Id.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: local_gen(), local_rd(), std::setfill(), std::setw(), dist(), str().

#### `void loadIdPCertificate()`
- Source: `include/auth/saml_authenticator.h`:182
- Brief: private helpers
- Parameters: none
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Load Id PCertificate. std::runtime_error if an error occurs. Calls: BIO_new_mem_buf(), data(), size(), PEM_read_bio_X509(), BIO_free(), X509_get_pubkey(), X509_free().

#### `std::chrono::system_clock::time_point parseDateTime(const std::string &s)`
- Source: `include/auth/saml_authenticator.h`:229
- Brief: Parse Date Time.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: s Input parameter. Return value. s Input parameter. Return value. std::runtime_error if an error occurs. Calls: size(), std::sscanf(), c_str(), _mkgmtime(), timegm(), std::chrono::system_clock::from_time_t().

#### `SAMLClaims processResponse(const std::string &saml_response_b64, const std::string &in_response_to="") const`
- Source: `include/auth/saml_authenticator.h`:148
- Brief: n/a
- Parameters:
  - `saml_response_b64` (const std::string &): n/a
  - `in_response_to` (const std::string &): n/a

#### `SAMLClaims processResponseImpl(const std::string &saml_response_b64, const std::string &in_response_to) const`
- Source: `include/auth/saml_authenticator.h`:250
- Brief: Process Response Impl.
- Parameters:
  - `saml_response_b64` (const std::string &): Input parameter.
  - `in_response_to` (const std::string &): Input parameter.
- Return: Return value.
- Details: saml_response_b64 Input parameter. in_response_to Input parameter. Return value.

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/saml_authenticator.h`:163
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `void setClockForTesting(std::function< std::chrono::system_clock::time_point()> clock)`
- Source: `include/auth/saml_authenticator.h`:156
- Brief: n/a
- Parameters:
  - `clock` (std::function< std::chrono::system_clock::time_point()>): n/a

#### `std::string urlEncode(const std::string &input)`
- Source: `include/auth/saml_authenticator.h`:242
- Brief: Url Encode.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value. input Input parameter. Return value. Calls: std::isalnum(), std::setw(), std::setfill(), str().

#### `bool verifyXmlSignature(const std::string &reference_xml, const std::string &signature_value_b64, const std::string &signed_info_c14n, const std::string &digest_value_b64, const std::string &digest_algorithm_uri, const std::string &sig_algorithm_uri) const`
- Source: `include/auth/saml_authenticator.h`:217
- Brief: Verify Xml Signature.
- Parameters:
  - `reference_xml` (const std::string &): Input parameter.
  - `signature_value_b64` (const std::string &): Input parameter.
  - `signed_info_c14n` (const std::string &): Input parameter.
  - `digest_value_b64` (const std::string &): Input parameter.
  - `digest_algorithm_uri` (const std::string &): Input parameter.
  - `sig_algorithm_uri` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: reference_xml Input parameter. signature_value_b64 Input parameter. signed_info_c14n Input parameter. digest_value_b64 Input parameter. digest_algorithm_uri Input parameter. sig_algorithm_uri Input parameter. True when the operation succeeds.

#### `~SAMLAuthenticator()`
- Source: `include/auth/saml_authenticator.h`:134
- Brief: n/a
- Parameters: none

### themis::auth::SecureBuffer

#### `SecureBuffer() noexcept=default`
- Source: `include/auth/secure_memory.h`:222
- Brief: n/a
- Parameters: none

#### `SecureBuffer(SecureBuffer &&o) noexcept`
- Source: `include/auth/secure_memory.h`:254
- Brief: n/a
- Parameters:
  - `o` (SecureBuffer &&): n/a

#### `SecureBuffer(const SecureBuffer &o)`
- Source: `include/auth/secure_memory.h`:240
- Brief: n/a
- Parameters:
  - `o` (const SecureBuffer &): n/a

#### `SecureBuffer(const std::vector< T > &v)`
- Source: `include/auth/secure_memory.h`:235
- Brief: n/a
- Parameters:
  - `v` (const std::vector< T > &): n/a

#### `SecureBuffer(std::size_t n, T val=T{})`
- Source: `include/auth/secure_memory.h`:224
- Brief: n/a
- Parameters:
  - `n` (std::size_t): n/a
  - `val` (T): n/a

#### `void assign(const T *src, std::size_t n)`
- Source: `include/auth/secure_memory.h`:309
- Brief: Assign.
- Parameters:
  - `src` (const T *): Input parameter.
  - `n` (std::size_t): Input parameter.
- Details: src Input parameter. n Input parameter. Calls: std::memcpy(), detail::secure_mlock().

#### `const T * data() const noexcept`
- Source: `include/auth/secure_memory.h`:271
- Brief: n/a
- Parameters: none

#### `T * data() noexcept`
- Source: `include/auth/secure_memory.h`:270
- Brief: n/a
- Parameters: none

#### `bool empty() const noexcept`
- Source: `include/auth/secure_memory.h`:273
- Brief: n/a
- Parameters: none

#### `bool operator!=(const SecureBuffer &o) const noexcept`
- Source: `include/auth/secure_memory.h`:296
- Brief: n/a
- Parameters:
  - `o` (const SecureBuffer &): n/a

#### `bool operator!=(const std::vector< T > &v) const noexcept`
- Source: `include/auth/secure_memory.h`:297
- Brief: n/a
- Parameters:
  - `v` (const std::vector< T > &): n/a

#### `SecureBuffer & operator=(SecureBuffer &&o) noexcept`
- Source: `include/auth/secure_memory.h`:259
- Brief: n/a
- Parameters:
  - `o` (SecureBuffer &&): n/a

#### `SecureBuffer & operator=(const SecureBuffer &o)`
- Source: `include/auth/secure_memory.h`:244
- Brief: n/a
- Parameters:
  - `o` (const SecureBuffer &): n/a

#### `SecureBuffer & operator=(const std::vector< T > &v)`
- Source: `include/auth/secure_memory.h`:249
- Brief: n/a
- Parameters:
  - `v` (const std::vector< T > &): n/a

#### `bool operator==(const SecureBuffer &o) const noexcept`
- Source: `include/auth/secure_memory.h`:278
- Brief: n/a
- Parameters:
  - `o` (const SecureBuffer &): n/a

#### `bool operator==(const std::vector< T > &v) const noexcept`
- Source: `include/auth/secure_memory.h`:287
- Brief: n/a
- Parameters:
  - `v` (const std::vector< T > &): n/a

#### `const T & operator[](std::size_t i) const noexcept`
- Source: `include/auth/secure_memory.h`:276
- Brief: n/a
- Parameters:
  - `i` (std::size_t): n/a

#### `T & operator[](std::size_t i) noexcept`
- Source: `include/auth/secure_memory.h`:275
- Brief: n/a
- Parameters:
  - `i` (std::size_t): n/a

#### `void release() noexcept`
- Source: `include/auth/secure_memory.h`:321
- Brief: n/a
- Parameters: none

#### `std::size_t size() const noexcept`
- Source: `include/auth/secure_memory.h`:272
- Brief: n/a
- Parameters: none

#### `~SecureBuffer()`
- Source: `include/auth/secure_memory.h`:268
- Brief: n/a
- Parameters: none

### themis::auth::SecureMFAValidator

#### `SecureMFAValidator(const Config &config=Config::defaults())`
- Source: `include/auth/totp_replay_cache.h`:150
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `void clearUserCache(const std::string &user_id)`
- Source: `include/auth/totp_replay_cache.h`:169
- Brief: Clear User Cache.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user. user_id Identifier of the user. Calls: clearUser().

#### `TOTPReplayCache::Statistics getReplayStatistics() const`
- Source: `include/auth/totp_replay_cache.h`:175
- Brief: Get Replay Statistics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool validateTOTP(const std::string &user_id, const std::string &secret_base32, const std::string &code)`
- Source: `include/auth/totp_replay_cache.h`:159
- Brief: Validate TOTP.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `secret_base32` (const std::string &): Input parameter.
  - `code` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: user_id Identifier of the user. secret_base32 Input parameter. code Input parameter. True when the operation succeeds.

### themis::auth::SecureMFAValidator::Config

#### `Config defaults()`
- Source: `include/auth/totp_replay_cache.h`:147
- Brief: Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

### themis::auth::SecureString

#### `SecureString() noexcept=default`
- Source: `include/auth/secure_memory.h`:76
- Brief: n/a
- Parameters: none

#### `SecureString(SecureString &&o) noexcept`
- Source: `include/auth/secure_memory.h`:128
- Brief: n/a
- Parameters:
  - `o` (SecureString &&): n/a

#### `SecureString(const SecureString &o)`
- Source: `include/auth/secure_memory.h`:101
- Brief: n/a
- Parameters:
  - `o` (const SecureString &): n/a

#### `SecureString(const char *s)`
- Source: `include/auth/secure_memory.h`:84
- Brief: Secure String.
- Parameters:
  - `s` (const char *): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: assign(), std::strlen().

#### `SecureString(const std::string &s)`
- Source: `include/auth/secure_memory.h`:96
- Brief: Secure String.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: assign(), data(), size().

#### `void assign(const char *src, std::size_t len)`
- Source: `include/auth/secure_memory.h`:189
- Brief: Assign.
- Parameters:
  - `src` (const char *): Input parameter.
  - `len` (std::size_t): Input parameter.
- Details: src Input parameter. len Input parameter. Calls: std::memcpy(), detail::secure_mlock().

#### `const char * c_str() const noexcept`
- Source: `include/auth/secure_memory.h`:147
- Brief: n/a
- Parameters: none

#### `char * data() noexcept`
- Source: `include/auth/secure_memory.h`:148
- Brief: Writable pointer for in-place cleansing.
- Parameters: none

#### `bool empty() const noexcept`
- Source: `include/auth/secure_memory.h`:150
- Brief: n/a
- Parameters: none

#### `bool operator!=(const SecureString &rhs) const noexcept`
- Source: `include/auth/secure_memory.h`:177
- Brief: n/a
- Parameters:
  - `rhs` (const SecureString &): n/a

#### `bool operator!=(const char *rhs) const noexcept`
- Source: `include/auth/secure_memory.h`:175
- Brief: n/a
- Parameters:
  - `rhs` (const char *): n/a

#### `bool operator!=(const std::string &rhs) const noexcept`
- Source: `include/auth/secure_memory.h`:176
- Brief: n/a
- Parameters:
  - `rhs` (const std::string &): n/a

#### `SecureString & operator=(SecureString &&o) noexcept`
- Source: `include/auth/secure_memory.h`:134
- Brief: n/a
- Parameters:
  - `o` (SecureString &&): n/a

#### `SecureString & operator=(const SecureString &o)`
- Source: `include/auth/secure_memory.h`:105
- Brief: n/a
- Parameters:
  - `o` (const SecureString &): n/a

#### `SecureString & operator=(const char *s)`
- Source: `include/auth/secure_memory.h`:113
- Brief: n/a
- Parameters:
  - `s` (const char *): n/a

#### `SecureString & operator=(const std::string &s)`
- Source: `include/auth/secure_memory.h`:121
- Brief: n/a
- Parameters:
  - `s` (const std::string &): n/a

#### `bool operator==(const SecureString &rhs) const noexcept`
- Source: `include/auth/secure_memory.h`:168
- Brief: n/a
- Parameters:
  - `rhs` (const SecureString &): n/a

#### `bool operator==(const char *rhs) const noexcept`
- Source: `include/auth/secure_memory.h`:152
- Brief: n/a
- Parameters:
  - `rhs` (const char *): n/a

#### `bool operator==(const std::string &rhs) const noexcept`
- Source: `include/auth/secure_memory.h`:162
- Brief: n/a
- Parameters:
  - `rhs` (const std::string &): n/a

#### `void release() noexcept`
- Source: `include/auth/secure_memory.h`:200
- Brief: n/a
- Parameters: none

#### `std::size_t size() const noexcept`
- Source: `include/auth/secure_memory.h`:149
- Brief: n/a
- Parameters: none

#### `~SecureString()`
- Source: `include/auth/secure_memory.h`:145
- Brief: n/a
- Parameters: none

### themis::auth::SessionManager

#### `SessionManager()`
- Source: `include/auth/session_manager.h`:70
- Brief: n/a
- Parameters: none

#### `SessionManager(SessionManager &&) noexcept=default`
- Source: `include/auth/session_manager.h`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (SessionManager &&): n/a

#### `SessionManager(const SessionLimits &limits)`
- Source: `include/auth/session_manager.h`:76
- Brief: Session Manager.
- Parameters:
  - `limits` (const SessionLimits &): Input parameter.
- Return: Return value.
- Details: limits Input parameter. Return value.

#### `SessionManager(const SessionManager &)=delete`
- Source: `include/auth/session_manager.h`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SessionManager &): n/a

#### `std::string createSession(const std::string &user_id, const std::string &device_fingerprint={}, const std::string &ip_address={}, const std::string &user_agent={})`
- Source: `include/auth/session_manager.h`:89
- Brief: Create a session for an authenticated user.
- Parameters:
  - `user_id` (const std::string &): User identifier.
  - `device_fingerprint` (const std::string &): Input parameter.
  - `ip_address` (const std::string &): Input parameter.
  - `user_agent` (const std::string &): Input parameter.
- Return: Session token.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: user_id User identifier. device_fingerprint Input parameter. ip_address Input parameter. user_agent Input parameter. Session token. std::invalid_argument if an error occurs. Calls: empty(), lock(), pruneExpiredLocked(), enforceSessionLimits(), std::chrono::system_clock::now(), count(), std::chrono::system_clock::time_point::max(), generateSessionId().

#### `void enforceSessionLimits(const std::string &user_id)`
- Source: `include/auth/session_manager.h`:170
- Brief: Enforce Session Limits.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user. user_id Identifier of the user. Calls: emplace_back(), size(), std::sort(), begin(), end(), THEMIS_INFO(), erase().

#### `std::string generateSessionId()`
- Source: `include/auth/session_manager.h`:152
- Brief: Generate Session Id.
- Parameters: none
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Return value. Return value. std::runtime_error if an error occurs. Calls: RAND_bytes(), std::setw(), std::setfill(), str().

#### `void invalidateSession(const std::string &session_id)`
- Source: `include/auth/session_manager.h`:112
- Brief: Invalidate a session token.
- Parameters:
  - `session_id` (const std::string &): Identifier of the session.
- Details: session_id Identifier of the session. Calls: terminateSession().

#### `bool isExpired(const SessionInfo &s) const`
- Source: `include/auth/session_manager.h`:164
- Brief: Is Expired.
- Parameters:
  - `s` (const SessionInfo &): Input parameter.
- Return: True when the operation succeeds.
- Details: s Input parameter. True when the operation succeeds.

#### `std::vector< SessionInfo > listSessions(const std::string &user_id)`
- Source: `include/auth/session_manager.h`:130
- Brief: List Sessions.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: user_id Identifier of the user. Return value. user_id Identifier of the user. Return value. Calls: lock(), isExpired(), push_back(), erase(), std::sort(), begin(), end().

#### `SessionManager & operator=(SessionManager &&) noexcept=default`
- Source: `include/auth/session_manager.h`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (SessionManager &&): n/a

#### `SessionManager & operator=(const SessionManager &)=delete`
- Source: `include/auth/session_manager.h`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SessionManager &): n/a

#### `size_t pruneExpired()`
- Source: `include/auth/session_manager.h`:142
- Brief: Prune Expired.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: lock(), pruneExpiredLocked().

#### `size_t pruneExpiredLocked()`
- Source: `include/auth/session_manager.h`:176
- Brief: Prune Expired Locked.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: isExpired(), push_back(), erase(), size().

#### `size_t size() const`
- Source: `include/auth/session_manager.h`:136
- Brief: Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `int terminateAllOtherSessions(const std::string &user_id, const std::string &keep_session_id={})`
- Source: `include/auth/session_manager.h`:116
- Brief: Terminate All Other Sessions.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `keep_session_id` (const std::string &): Identifier of the keep session.
- Return: Return value.
- Details: user_id Identifier of the user. keep_session_id Identifier of the keep session. Return value. Calls: lock(), constantTimeSessionIdEquals(), push_back(), erase(), THEMIS_INFO(), size().

#### `void terminateSession(const std::string &session_id)`
- Source: `include/auth/session_manager.h`:105
- Brief: Terminate Session.
- Parameters:
  - `session_id` (const std::string &): Identifier of the session.
- Details: session_id Identifier of the session. session_id Identifier of the session. Calls: lock(), find(), hashSessionId(), end(), THEMIS_INFO(), erase().

#### `ValidationResult validateSession(const std::string &session_id, const std::string &current_ip={})`
- Source: `include/auth/session_manager.h`:96
- Brief: Validate a session token.
- Parameters:
  - `session_id` (const std::string &): Identifier of the session.
  - `current_ip` (const std::string &): n/a
- Return: Validated session on success.
- Details: session_id Identifier of the session. param Input parameter. Validated session on success. Calls: empty(), lock(), find(), hashSessionId(), end(), isExpired(), erase(), std::chrono::system_clock::now().

#### `~SessionManager()=default`
- Source: `include/auth/session_manager.h`:77
- Brief: n/a
- Parameters: none

### themis::auth::SessionManager::ValidationResult

#### `operator bool() const noexcept`
- Source: `include/auth/session_manager.h`:61
- Brief: n/a
- Parameters: none

### themis::auth::TOTPReplayCache

#### `TOTPReplayCache(TOTPReplayCache &&) noexcept=default`
- Source: `include/auth/totp_replay_cache.h`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (TOTPReplayCache &&): n/a

#### `TOTPReplayCache(const Config &config=Config::defaults())`
- Source: `include/auth/totp_replay_cache.h`:43
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `TOTPReplayCache(const TOTPReplayCache &)=delete`
- Source: `include/auth/totp_replay_cache.h`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TOTPReplayCache &): n/a

#### `bool checkAndMarkUsed(const std::string &user_id, const std::string &code)`
- Source: `include/auth/totp_replay_cache.h`:58
- Brief: Check And Mark Used.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `code` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: user_id Identifier of the user. code Input parameter. True when the operation succeeds. user_id Identifier of the user. code Input parameter. True when the operation succeeds. Calls: lock(), needsCleanup(), cleanup(), std::chrono::system_clock::now(), utils::Logger::warn(), push_back(), size(), std::sort().

#### `void cleanup()`
- Source: `include/auth/totp_replay_cache.h`:82
- Brief: Cleanup.
- Parameters: none
- Details: Calls: std::chrono::system_clock::now(), begin(), end(), size(), erase(), std::remove_if(), empty(), std::chrono::steady_clock::now().

#### `void cleanupUser(const std::string &user_id)`
- Source: `include/auth/totp_replay_cache.h`:121
- Brief: Helper: Remove expired codes for a user.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: Cleanup User. user_id Identifier of the user. user_id Identifier of the user. Calls: find(), end(), std::chrono::system_clock::now(), size(), erase(), std::remove_if(), begin(), empty().

#### `void clear()`
- Source: `include/auth/totp_replay_cache.h`:77
- Brief: Clear.
- Parameters: none
- Details: Calls: lock(), utils::Logger::info().

#### `void clearUser(const std::string &user_id)`
- Source: `include/auth/totp_replay_cache.h`:72
- Brief: Clear User.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user. user_id Identifier of the user. Calls: lock(), find(), end(), size(), erase().

#### `Statistics getStatistics() const`
- Source: `include/auth/totp_replay_cache.h`:95
- Brief: Return access control statistics.
- Parameters: none
- Return: Access control statistics.
- Details: Access control statistics.

#### `bool isUsed(const std::string &user_id, const std::string &code) const`
- Source: `include/auth/totp_replay_cache.h`:66
- Brief: Is Used.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `code` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: user_id Identifier of the user. code Input parameter. True when the operation succeeds.

#### `bool needsCleanup() const`
- Source: `include/auth/totp_replay_cache.h`:127
- Brief: Helper: Check if cleanup is needed.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `TOTPReplayCache & operator=(TOTPReplayCache &&) noexcept=default`
- Source: `include/auth/totp_replay_cache.h`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (TOTPReplayCache &&): n/a

#### `TOTPReplayCache & operator=(const TOTPReplayCache &)=delete`
- Source: `include/auth/totp_replay_cache.h`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TOTPReplayCache &): n/a

#### `~TOTPReplayCache()=default`
- Source: `include/auth/totp_replay_cache.h`:44
- Brief: n/a
- Parameters: none

### themis::auth::TOTPReplayCache::Config

#### `Config defaults()`
- Source: `include/auth/totp_replay_cache.h`:40
- Brief: Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

### themis::auth::TOTPSecretEncryption

#### `TOTPSecretEncryption(TOTPSecretEncryption &&) noexcept`
- Source: `include/auth/totp_secret_encryption.h`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (TOTPSecretEncryption &&): n/a

#### `TOTPSecretEncryption(const Config &config)`
- Source: `include/auth/totp_secret_encryption.h`:78
- Brief: TOTPSecret Encryption.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `TOTPSecretEncryption(const TOTPSecretEncryption &)=delete`
- Source: `include/auth/totp_secret_encryption.h`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TOTPSecretEncryption &): n/a

#### `std::string decrypt(const EncryptedSecret &encrypted)`
- Source: `include/auth/totp_secret_encryption.h`:99
- Brief: Decrypt.
- Parameters:
  - `encrypted` (const EncryptedSecret &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: encrypted Input parameter. Return value. encrypted Input parameter. Return value. std::runtime_error if an error occurs. Calls: deriveKey(), EVP_CIPHER_CTX_new(), EVP_DecryptInit_ex(), EVP_aes_256_gcm(), data(), plaintext(), size(), EVP_CIPHER_block_size().

#### `SecureBuffer< uint8_t > deriveKey(const std::vector< uint8_t > &salt)`
- Source: `include/auth/totp_secret_encryption.h`:146
- Brief: Derive encryption key from master key and salt.
- Parameters:
  - `salt` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Derive Key. salt Input parameter. Return value. Returns a SecureBuffer so the derived key is zeroed when it goes out of scope. salt Input parameter. Return value. std::runtime_error if an error occurs. Calls: derived_key(), PKCS5_PBKDF2_HMAC(), data(), size(), EVP_sha256().

#### `std::string deserializeAndDecrypt(const std::string &serialized)`
- Source: `include/auth/totp_secret_encryption.h`:113
- Brief: Deserialize And Decrypt.
- Parameters:
  - `serialized` (const std::string &): Input parameter.
- Return: Return value.
- Details: serialized Input parameter. Return value. serialized Input parameter. Return value. Calls: EncryptedSecret::deserialize(), decrypt().

#### `EncryptedSecret encrypt(const std::string &plaintext_secret)`
- Source: `include/auth/totp_secret_encryption.h`:92
- Brief: Encrypt.
- Parameters:
  - `plaintext_secret` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: plaintext_secret Input parameter. Return value. plaintext_secret Input parameter. Return value. std::runtime_error if an error occurs. Calls: generateRandomBytes(), deriveKey(), EVP_CIPHER_CTX_new(), EVP_EncryptInit_ex(), EVP_aes_256_gcm(), data(), plaintext(), begin().

#### `std::string encryptAndSerialize(const std::string &plaintext_secret)`
- Source: `include/auth/totp_secret_encryption.h`:106
- Brief: Encrypt And Serialize.
- Parameters:
  - `plaintext_secret` (const std::string &): Input parameter.
- Return: Return value.
- Details: plaintext_secret Input parameter. Return value. plaintext_secret Input parameter. Return value. Calls: encrypt(), serialize().

#### `std::vector< uint8_t > generateRandomBytes(size_t size)`
- Source: `include/auth/totp_secret_encryption.h`:154
- Brief: Generate Random Bytes.
- Parameters:
  - `size` (size_t): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: size Input parameter. Return value. size Input parameter. Return value. std::runtime_error if an error occurs. Calls: bytes(), RAND_bytes(), data().

#### `bool needsReencryption(const EncryptedSecret &encrypted) const`
- Source: `include/auth/totp_secret_encryption.h`:127
- Brief: Needs Reencryption.
- Parameters:
  - `encrypted` (const EncryptedSecret &): Input parameter.
- Return: True when the operation succeeds.
- Details: encrypted Input parameter. True when the operation succeeds.

#### `TOTPSecretEncryption & operator=(TOTPSecretEncryption &&) noexcept`
- Source: `include/auth/totp_secret_encryption.h`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (TOTPSecretEncryption &&): n/a

#### `TOTPSecretEncryption & operator=(const TOTPSecretEncryption &)=delete`
- Source: `include/auth/totp_secret_encryption.h`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TOTPSecretEncryption &): n/a

#### `EncryptedSecret reencrypt(const EncryptedSecret &old_encrypted)`
- Source: `include/auth/totp_secret_encryption.h`:134
- Brief: Reencrypt.
- Parameters:
  - `old_encrypted` (const EncryptedSecret &): Input parameter.
- Return: Return value.
- Details: old_encrypted Input parameter. Return value. old_encrypted Input parameter. Return value. Calls: decrypt(), encrypt().

#### `void rotateKey(const SecureBuffer< uint8_t > &new_master_key, int new_version)`
- Source: `include/auth/totp_secret_encryption.h`:120
- Brief: Rotate Key.
- Parameters:
  - `new_master_key` (const SecureBuffer< uint8_t > &): Input parameter.
  - `new_version` (int): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: new_master_key Input parameter. new_version Input parameter. new_master_key Input parameter. new_version Input parameter. std::invalid_argument if an error occurs. Calls: size(), utils::Logger::info().

#### `~TOTPSecretEncryption()`
- Source: `include/auth/totp_secret_encryption.h`:79
- Brief: n/a
- Parameters: none

### themis::auth::TOTPSecretEncryption::EncryptedSecret

#### `EncryptedSecret deserialize(const std::string &data)`
- Source: `include/auth/totp_secret_encryption.h`:70
- Brief: Deserialize.
- Parameters:
  - `data` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: data Input parameter. Return value. data Input parameter. Return value. std::runtime_error if an error occurs. Calls: iss(), std::getline(), push_back(), size(), std::stoi(), base64Decode().

#### `std::string serialize() const`
- Source: `include/auth/totp_secret_encryption.h`:62
- Brief: Serialize to string for storage.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::auth::TOTPSecretEncryption::Impl

#### `Impl(const Config &cfg)`
- Source: `src/auth/totp_secret_encryption.cpp`:37
- Brief: n/a
- Parameters:
  - `cfg` (const Config &): n/a

#### `~Impl()`
- Source: `src/auth/totp_secret_encryption.cpp`:43
- Brief: n/a
- Parameters: none

### themis::auth::TOTPSecretRotationManager

#### `TOTPSecretRotationManager()`
- Source: `include/auth/totp_secret_encryption.h`:174
- Brief: n/a
- Parameters: none

#### `TOTPSecretRotationManager(const RotationConfig &config)`
- Source: `include/auth/totp_secret_encryption.h`:180
- Brief: TOTPSecret Rotation Manager.
- Parameters:
  - `config` (const RotationConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `size_t cleanupExpiredSecrets()`
- Source: `include/auth/totp_secret_encryption.h`:213
- Brief: Cleanup Expired Secrets.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: size(), erase(), std::remove_if(), begin(), end(), isSecretValid(), utils::Logger::info().

#### `std::vector< SecretVersion > getActiveSecrets(const std::string &user_id)`
- Source: `include/auth/totp_secret_encryption.h`:200
- Brief: Get Active Secrets.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Return: Return value.
- Details: user_id Identifier of the user. Return value.

#### `bool isSecretValid(const SecretVersion &secret_version) const`
- Source: `include/auth/totp_secret_encryption.h`:207
- Brief: Is Secret Valid.
- Parameters:
  - `secret_version` (const SecretVersion &): Input parameter.
- Return: True when the operation succeeds.
- Details: secret_version Input parameter. True when the operation succeeds.

#### `SecretVersion rotateSecret(const std::string &user_id, const std::string &old_secret, const std::string &new_secret)`
- Source: `include/auth/totp_secret_encryption.h`:189
- Brief: Rotate Secret.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `old_secret` (const std::string &): Input parameter.
  - `new_secret` (const std::string &): Input parameter.
- Return: Return value.
- Details: user_id Identifier of the user. old_secret Input parameter. new_secret Input parameter. Return value. user_id Identifier of the user. param Input parameter. new_secret Input parameter. Return value. Calls: std::chrono::system_clock::now(), size(), push_back(), utils::Logger::info().

### themis::auth::TokenBlacklist

#### `TokenBlacklist(TokenBlacklist &&) noexcept=delete`
- Source: `include/auth/token_blacklist.h`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (TokenBlacklist &&): n/a

#### `TokenBlacklist(const Config &config=Config::defaults())`
- Source: `include/auth/token_blacklist.h`:78
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `TokenBlacklist(const TokenBlacklist &)=delete`
- Source: `include/auth/token_blacklist.h`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TokenBlacklist &): n/a

#### `void add(const std::string &jti, std::chrono::system_clock::time_point expiry) override`
- Source: `include/auth/token_blacklist.h`:111
- Brief: Add.
- Parameters:
  - `jti` (const std::string &): Input parameter.
  - `expiry` (std::chrono::system_clock::time_point): Input parameter.
- Details: jti Input parameter. expiry Input parameter. Calls: revoke().

#### `void clear()`
- Source: `include/auth/token_blacklist.h`:145
- Brief: Clear.
- Parameters: none
- Details: Calls: lock(), reset(), THEMIS_INFO().

#### `void clearOnRevokeCallback()`
- Source: `include/auth/token_blacklist.h`:105
- Brief: Clear On Revoke Callback.
- Parameters: none
- Details: Calls: lock().

#### `Statistics getStatistics() const`
- Source: `include/auth/token_blacklist.h`:166
- Brief: Return access control statistics.
- Parameters: none
- Return: Access control statistics.
- Details: Access control statistics.

#### `bool isRevoked(const std::string &jti) const override`
- Source: `include/auth/token_blacklist.h`:114
- Brief: n/a
- Parameters:
  - `jti` (const std::string &): n/a

#### `bool needsCleanup() const`
- Source: `include/auth/token_blacklist.h`:250
- Brief: Needs Cleanup.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `TokenBlacklist & operator=(TokenBlacklist &&) noexcept=delete`
- Source: `include/auth/token_blacklist.h`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (TokenBlacklist &&): n/a

#### `TokenBlacklist & operator=(const TokenBlacklist &)=delete`
- Source: `include/auth/token_blacklist.h`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TokenBlacklist &): n/a

#### `void pruneExpired()`
- Source: `include/auth/token_blacklist.h`:140
- Brief: Prune Expired.
- Parameters: none
- Details: Calls: lock(), pruneExpiredLocked().

#### `void pruneExpiredLocked()`
- Source: `include/auth/token_blacklist.h`:254
- Brief: Prune Expired Locked.
- Parameters: none
- Details: Calls: std::chrono::system_clock::now(), reset(), begin(), end(), erase(), add(), std::chrono::steady_clock::now().

#### `void purgeExpired() override`
- Source: `include/auth/token_blacklist.h`:116
- Brief: Purge Expired.
- Parameters: none
- Details: Calls: pruneExpired().

#### `void revoke(const std::string &jti, std::chrono::system_clock::time_point expires_at)`
- Source: `include/auth/token_blacklist.h`:127
- Brief: Revoke.
- Parameters:
  - `jti` (const std::string &): Input parameter.
  - `expires_at` (std::chrono::system_clock::time_point): Input parameter.
- Details: jti Input parameter. expires_at Input parameter. jti Input parameter. expires_at Input parameter. Calls: empty(), THEMIS_WARN(), lock(), needsCleanup(), pruneExpiredLocked(), size(), begin(), end().

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/token_blacklist.h`:92
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `void setOnRevokeCallback(RevocationCallback cb)`
- Source: `include/auth/token_blacklist.h`:100
- Brief: Set On Revoke Callback.
- Parameters:
  - `cb` (RevocationCallback): Input parameter.
- Details: cb Input parameter. cb Input parameter. Calls: lock(), std::move().

#### `size_t size() const`
- Source: `include/auth/token_blacklist.h`:151
- Brief: Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool unrevoke(const std::string &jti)`
- Source: `include/auth/token_blacklist.h`:135
- Brief: Unrevoke.
- Parameters:
  - `jti` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: jti Input parameter. True when the operation succeeds. jti Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), erase(), THEMIS_INFO().

#### `~TokenBlacklist()=default`
- Source: `include/auth/token_blacklist.h`:79
- Brief: n/a
- Parameters: none

### themis::auth::TokenBlacklist::BloomFilter

#### `BloomFilter(size_t capacity)`
- Source: `include/auth/token_blacklist.h`:192
- Brief: Bloom Filter.
- Parameters:
  - `capacity` (size_t): Input parameter.
- Return: Return value.
- Details: capacity Input parameter. Return value.

#### `void add(const std::string &key) noexcept`
- Source: `include/auth/token_blacklist.h`:197
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::pair< size_t, size_t > hashes(const std::string &key) noexcept`
- Source: `include/auth/token_blacklist.h`:221
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `bool mayContain(const std::string &key) const noexcept`
- Source: `include/auth/token_blacklist.h`:205
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `void reset() noexcept`
- Source: `include/auth/token_blacklist.h`:216
- Brief: n/a
- Parameters: none

### themis::auth::TokenBlacklist::Config

#### `Config defaults()`
- Source: `include/auth/token_blacklist.h`:75
- Brief: Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

### themis::auth::WebAuthnAuthenticator

#### `WebAuthnAuthenticator(const RelyingParty &rp)`
- Source: `include/auth/webauthn_authenticator.h`:108
- Brief: Web Authn Authenticator.
- Parameters:
  - `rp` (const RelyingParty &): Input parameter.
- Return: Return value.
- Details: rp Input parameter. Return value.

#### `std::vector< uint8_t > base64UrlDecode(const std::string &input)`
- Source: `include/auth/webauthn_authenticator.h`:237
- Brief: Base64 Url Decode.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value. input Input parameter. Return value. Calls: base64UrlDecodeImpl(), THROW_AUTH_ERROR(), what().

#### `std::string base64UrlEncode(const std::vector< uint8_t > &data)`
- Source: `include/auth/webauthn_authenticator.h`:231
- Brief: Base64URL codec (RFC 4648 §5, no padding).
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: Base64 Url Encode. data Input parameter. Return value. data Input parameter. Return value. Calls: base64UrlEncodeImpl(), data(), size().

#### `AssertionResult completeAuthentication(const nlohmann::json &credential_response, const std::vector< uint8_t > &stored_public_key, uint32_t stored_sign_count)`
- Source: `include/auth/webauthn_authenticator.h`:149
- Brief: Complete Authentication.
- Parameters:
  - `credential_response` (const nlohmann::json &): Input parameter.
  - `stored_public_key` (const std::vector< uint8_t > &): Input parameter.
  - `stored_sign_count` (uint32_t): Input parameter.
- Return: Return value.
- Details: credential_response Input parameter. stored_public_key Input parameter. stored_sign_count Input parameter. Return value.

#### `AttestationResult completeRegistration(const nlohmann::json &credential_response)`
- Source: `include/auth/webauthn_authenticator.h`:132
- Brief: Complete Registration.
- Parameters:
  - `credential_response` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: credential_response Input parameter. Return value. cred Input parameter. Return value. Calls: value(), THROW_AUTH_ERROR(), at(), base64UrlDecode(), parseClientDataJSON(), verifyAndConsumeChallenge(), cborParseAttestationObject(), std::string().

#### `std::pair< std::vector< uint8_t >, std::string > coseKeyToSpki(const std::vector< uint8_t > &cose_key_bytes)`
- Source: `include/auth/webauthn_authenticator.h`:283
- Brief: n/a
- Parameters:
  - `cose_key_bytes` (const std::vector< uint8_t > &): n/a

#### `void fillRandomBytes(unsigned char *buf, std::size_t len)`
- Source: `include/auth/webauthn_authenticator.h`:210
- Brief: Fill Random Bytes.
- Parameters:
  - `buf` (unsigned char *): Input/output parameter.
  - `len` (std::size_t): Input parameter.
- Details: buf Input/output parameter. len Input parameter. buf Input/output parameter. len Input parameter. Calls: rand_bytes_fn_(), RAND_bytes(), THROW_AUTH_ERROR().

#### `std::string generateChallenge()`
- Source: `include/auth/webauthn_authenticator.h`:192
- Brief: Generate Challenge.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: fillRandomBytes(), data(), size(), base64UrlEncode(), begin(), end(), lock(), purgeExpiredChallenges().

#### `void parseAttestationObject(const std::vector< uint8_t > &cbor_bytes, std::string &fmt, std::vector< uint8_t > &auth_data)`
- Source: `include/auth/webauthn_authenticator.h`:277
- Brief: Parse Attestation Object.
- Parameters:
  - `cbor_bytes` (const std::vector< uint8_t > &): Input parameter.
  - `fmt` (std::string &): Input/output parameter.
  - `auth_data` (std::vector< uint8_t > &): Input/output parameter.
- Details: cbor_bytes Input parameter. fmt Input/output parameter. auth_data Input/output parameter. cbor_bytes Input parameter. fmt Input/output parameter. auth_data Input/output parameter. Calls: cborParseAttestationObject().

#### `AuthData parseAuthData(const std::vector< uint8_t > &auth_data_bytes)`
- Source: `include/auth/webauthn_authenticator.h`:269
- Brief: Parse Auth Data.
- Parameters:
  - `auth_data_bytes` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: auth_data_bytes Input parameter. Return value. auth_data_bytes Input parameter. Return value. std::runtime_error if an error occurs. Calls: size(), std::to_string(), std::copy(), begin(), assign(), cred_id_bytes(), base64UrlEncodeImpl(), data().

#### `ClientData parseClientDataJSON(const std::vector< uint8_t > &client_data_json)`
- Source: `include/auth/webauthn_authenticator.h`:250
- Brief: Parse Client Data JSON.
- Parameters:
  - `client_data_json` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: client_data_json Input parameter. Return value.

#### `void purgeExpiredChallenges()`
- Source: `include/auth/webauthn_authenticator.h`:203
- Brief: Purge Expired Challenges.
- Parameters: none
- Details: Calls: std::chrono::system_clock::now(), begin(), end(), erase().

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/webauthn_authenticator.h`:116
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `void setExpectedOrigin(const std::string &origin)`
- Source: `include/auth/webauthn_authenticator.h`:167
- Brief: Set Expected Origin.
- Parameters:
  - `origin` (const std::string &): Input parameter.
- Details: origin Input parameter. origin Input parameter. Implements setExpectedOrigin without additional internal calls.

#### `void setRandBytesForTesting(std::function< void(unsigned char *buf, std::size_t len)> fn)`
- Source: `include/auth/webauthn_authenticator.h`:159
- Brief: n/a
- Parameters:
  - `fn` (std::function< void(unsigned char *buf, std::size_t len)>): n/a

#### `std::vector< uint8_t > sha256(const std::string &data)`
- Source: `include/auth/webauthn_authenticator.h`:224
- Brief: Sha256.
- Parameters:
  - `data` (const std::string &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. data Input parameter. Return value. Calls: SHA256(), data(), size(), begin(), end().

#### `std::vector< uint8_t > sha256(const std::vector< uint8_t > &data)`
- Source: `include/auth/webauthn_authenticator.h`:218
- Brief: Sha256.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. data Input parameter. Return value. Calls: SHA256(), data(), size(), begin(), end().

#### `CredentialRequestOptions startAuthentication(const std::optional< std::string > &user_id=std::nullopt)`
- Source: `include/auth/webauthn_authenticator.h`:138
- Brief: n/a
- Parameters:
  - `user_id` (const std::optional< std::string > &): n/a

#### `CredentialCreationOptions startRegistration(const User &user, bool resident_key=false)`
- Source: `include/auth/webauthn_authenticator.h`:122
- Brief: Start Registration.
- Parameters:
  - `user` (const User &): Input parameter.
  - `resident_key` (bool): Input parameter.
- Return: Return value.
- Details: user Input parameter. resident_key Input parameter. Return value. Calls: generateChallenge(), spdlog::info().

#### `void verifyAndConsumeChallenge(const std::string &challenge_b64url)`
- Source: `include/auth/webauthn_authenticator.h`:198
- Brief: Verify And Consume Challenge.
- Parameters:
  - `challenge_b64url` (const std::string &): Input parameter.
- Details: challenge_b64url Input parameter. challenge_b64 Input parameter. Calls: lock(), purgeExpiredChallenges(), find(), end(), THROW_AUTH_ERROR(), erase().

#### `void verifySignature(const std::vector< uint8_t > &auth_data_bytes, const std::vector< uint8_t > &client_data_hash, const std::vector< uint8_t > &signature_bytes, const std::vector< uint8_t > &spki_bytes)`
- Source: `include/auth/webauthn_authenticator.h`:294
- Brief: Verify Signature.
- Parameters:
  - `auth_data_bytes` (const std::vector< uint8_t > &): Input parameter.
  - `client_data_hash` (const std::vector< uint8_t > &): Input parameter.
  - `signature_bytes` (const std::vector< uint8_t > &): Input parameter.
  - `spki_bytes` (const std::vector< uint8_t > &): Input parameter.
- Details: auth_data_bytes Input parameter. client_data_hash Input parameter. signature_bytes Input parameter. spki_bytes Input parameter. auth_data_bytes Input parameter. client_data_hash Input parameter. signature_bytes Input parameter. spki_bytes Input parameter. Calls: data(), d2i_PUBKEY(), size(), THROW_AUTH_ERROR(), reserve(), insert(), end(), begin().

#### `~WebAuthnAuthenticator()=default`
- Source: `include/auth/webauthn_authenticator.h`:109
- Brief: n/a
- Parameters: none

### themis::auth::WebAuthnAuthenticator::CredentialCreationOptions

#### `nlohmann::json to_json() const`
- Source: `include/auth/webauthn_authenticator.h`:68
- Brief: To json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::auth::WebAuthnAuthenticator::CredentialRequestOptions

#### `nlohmann::json to_json() const`
- Source: `include/auth/webauthn_authenticator.h`:82
- Brief: To json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::auth::ZeroTrustAuthVerifier

#### `ZeroTrustAuthVerifier()`
- Source: `include/auth/zero_trust_auth_verifier.h`:80
- Brief: n/a
- Parameters: none

#### `ZeroTrustAuthVerifier(TokenVerifier token_verifier)`
- Source: `include/auth/zero_trust_auth_verifier.h`:86
- Brief: Zero Trust Auth Verifier.
- Parameters:
  - `token_verifier` (TokenVerifier): Input parameter.
- Return: Return value.
- Details: token_verifier Input parameter. Return value.

#### `ZeroTrustAuthVerifier(const Config &config, TokenVerifier token_verifier=nullptr)`
- Source: `include/auth/zero_trust_auth_verifier.h`:87
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a
  - `token_verifier` (TokenVerifier): n/a

#### `void addNetworkPolicy(const security::NetworkPolicy &policy)`
- Source: `include/auth/zero_trust_auth_verifier.h`:109
- Brief: ======================================================================== Network policy management (delegated to the underlying enforcer) ========================================================================
- Parameters:
  - `policy` (const security::NetworkPolicy &): Network policy to add.
- Details: Register a network policy. policy Network policy to add. policy Network policy to add. Implements addNetworkPolicy without additional internal calls.

#### `const security::ZeroTrustPolicyEnforcer::Metrics & getMetrics() const`
- Source: `include/auth/zero_trust_auth_verifier.h`:157
- Brief: n/a
- Parameters: none

#### `std::vector< security::NetworkPolicy > getNetworkPolicies() const`
- Source: `include/auth/zero_trust_auth_verifier.h`:122
- Brief: Return all currently registered network policies.
- Parameters: none
- Return: Snapshot of network policies.
- Details: Snapshot of network policies.

#### `void monitorLoop()`
- Source: `include/auth/zero_trust_auth_verifier.h`:187
- Brief: Monitor Loop.
- Parameters: none
- Details: ------------------------------------------------------------------------ Private: background monitoring loop ------------------------------------------------------------------------ Calls: lock(), load(), empty(), std::chrono::steady_clock::now(), wait_until(), push_back(), submit(), reEvaluateSession().

#### `size_t monitoredSessionCount() const`
- Source: `include/auth/zero_trust_auth_verifier.h`:151
- Brief: Monitored Session Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void reEvaluateSession(const MonitorEntry &entry)`
- Source: `include/auth/zero_trust_auth_verifier.h`:193
- Brief: Re Evaluate Session.
- Parameters:
  - `entry` (const MonitorEntry &): Input parameter.
- Details: ------------------------------------------------------------------------ Private: single-session re-evaluation (runs on worker thread) ------------------------------------------------------------------------ entry Input parameter. entry Input parameter. Calls: lock(), find(), end(), verify(), THEMIS_WARN(), al(), logZeroTrustReEvaluationFailed(), terminateSession().

#### `bool removeNetworkPolicy(const std::string &policy_id)`
- Source: `include/auth/zero_trust_auth_verifier.h`:116
- Brief: Remove a network policy by id.
- Parameters:
  - `policy_id` (const std::string &): Identifier of the policy to remove.
- Return: True when a policy was removed.
- Details: policy_id Identifier of the policy to remove. True when a policy was removed. policy_id Identifier of the policy to remove. True when a policy was removed. Implements removeNetworkPolicy without additional internal calls.

#### `void setAuditLogger(utils::AuditLogger *logger)`
- Source: `include/auth/zero_trust_auth_verifier.h`:102
- Brief: Set Audit Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setAuditLogger without additional internal calls.

#### `void startSessionMonitoring(const MonitoredSession &session, SessionManager *session_manager)`
- Source: `include/auth/zero_trust_auth_verifier.h`:138
- Brief: ======================================================================== Background session monitoring (async policy re-evaluation) ========================================================================
- Parameters:
  - `session` (const MonitoredSession &): Input parameter.
  - `session_manager` (SessionManager *): Input/output parameter.
- Details: ============================================================================ Background session monitoring (async policy re-evaluation) ============================================================================ session Input parameter. session_manager Input/output parameter.

#### `void stopSessionMonitoring(const std::string &session_id)`
- Source: `include/auth/zero_trust_auth_verifier.h`:145
- Brief: Stop Session Monitoring.
- Parameters:
  - `session_id` (const std::string &): Identifier of the session.
- Details: session_id Identifier of the session. session_id Identifier of the session. Calls: lock(), erase(), empty(), store(), load(), notify_all().

#### `Decision verify(const Request &req)`
- Source: `include/auth/zero_trust_auth_verifier.h`:130
- Brief: ======================================================================== Core: continuous per-request verification ========================================================================
- Parameters:
  - `req` (const Request &): Input parameter.
- Return: Verification result.
- Details: ============================================================================ Core: continuous per-request verification ============================================================================ req Input parameter. Verification result. req Input parameter. Verification result. Calls: std::chrono::system_clock::now(), THEMIS_WARN(), al(), logZeroTrustDenied(), std::to_string(), THEMIS_DEBUG(), logZeroTrustAllowed().

#### `~ZeroTrustAuthVerifier()`
- Source: `include/auth/zero_trust_auth_verifier.h`:91
- Brief: n/a
- Parameters: none

### themis::auth::detail

#### `void secure_mlock(void *ptr, std::size_t len) noexcept`
- Source: `include/auth/secure_memory.h`:35
- Brief: n/a
- Parameters:
  - `ptr` (void *): n/a
  - `len` (std::size_t): n/a

#### `void secure_munlock(void *ptr, std::size_t len) noexcept`
- Source: `include/auth/secure_memory.h`:46
- Brief: n/a
- Parameters:
  - `ptr` (void *): n/a
  - `len` (std::size_t): n/a

#### `void secure_release(T *ptr, std::size_t count) noexcept`
- Source: `include/auth/secure_memory.h`:58
- Brief: n/a
- Parameters:
  - `ptr` (T *): n/a
  - `count` (std::size_t): n/a

### themis::auth::test

#### `TEST(AuthRedactionTest, RedactContainsLengthInfo)`
- Source: `tests/auth/test_auth_sensitive_data_redaction.cpp`:40
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthRedactionTest): n/a
  - `<unnamed>` (RedactContainsLengthInfo): n/a

#### `TEST(AuthRedactionTest, RedactDoesNotLeakOriginalValue)`
- Source: `tests/auth/test_auth_sensitive_data_redaction.cpp`:27
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthRedactionTest): n/a
  - `<unnamed>` (RedactDoesNotLeakOriginalValue): n/a

#### `TEST(AuthRedactionTest, RedactIfPresentHandlesEmptyString)`
- Source: `tests/auth/test_auth_sensitive_data_redaction.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthRedactionTest): n/a
  - `<unnamed>` (RedactIfPresentHandlesEmptyString): n/a

#### `TEST(AuthRedactionTest, RedactIsThreadSafe)`
- Source: `tests/auth/test_auth_sensitive_data_redaction.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthRedactionTest): n/a
  - `<unnamed>` (RedactIsThreadSafe): n/a

#### `TEST(AuthRedactionTest, RedactPartialExposesOnlyPrefix)`
- Source: `tests/auth/test_auth_sensitive_data_redaction.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (AuthRedactionTest): n/a
  - `<unnamed>` (RedactPartialExposesOnlyPrefix): n/a

### themis::auth::tests

#### `TEST(AuthorizationPolicyTest, AUTH_AuthZ_01_PolicyDecisionDefaultIsNotApplicable)`
- Source: `tests/auth/test_auth_wavec_authorization.cpp`:33
- Brief: AUTH-AuthZ-01 — A default-constructed PolicyEvaluationResult must carry PolicyDecision::NOT_APPLICABLE so that a policy with no opinion does not accidentally ALLOW or DENY.
- Parameters:
  - `<unnamed>` (AuthorizationPolicyTest): n/a
  - `<unnamed>` (AUTH_AuthZ_01_PolicyDecisionDefaultIsNotApplicable): n/a

#### `TEST(AuthorizationPolicyTest, AUTH_AuthZ_02_PolicyDecisionAllowCarriesReason)`
- Source: `tests/auth/test_auth_wavec_authorization.cpp`:48
- Brief: AUTH-AuthZ-02 — A PolicyEvaluationResult can be constructed with ALLOW and an accompanying reason string; both must round-trip.
- Parameters:
  - `<unnamed>` (AuthorizationPolicyTest): n/a
  - `<unnamed>` (AUTH_AuthZ_02_PolicyDecisionAllowCarriesReason): n/a

#### `TEST(AuthorizationPolicyTest, AUTH_AuthZ_03_SubjectAttributesDefaultRoleIsEmpty)`
- Source: `tests/auth/test_auth_wavec_authorization.cpp`:68
- Brief: AUTH-AuthZ-03 — A default-constructed SubjectAttributes must have an empty role field. Policy rules that depend on role membership must not accidentally match against an uninitialised value.
- Parameters:
  - `<unnamed>` (AuthorizationPolicyTest): n/a
  - `<unnamed>` (AUTH_AuthZ_03_SubjectAttributesDefaultRoleIsEmpty): n/a

#### `TEST(AuthorizationPolicyTest, AUTH_AuthZ_04_ResourceAttributesResourceTypeIsSettable)`
- Source: `tests/auth/test_auth_wavec_authorization.cpp`:83
- Brief: AUTH-AuthZ-04 — resource_type in ResourceAttributes must accept arbitrary string values and preserve them without truncation.
- Parameters:
  - `<unnamed>` (AuthorizationPolicyTest): n/a
  - `<unnamed>` (AUTH_AuthZ_04_ResourceAttributesResourceTypeIsSettable): n/a

#### `TEST(AuthorizationPolicyTest, AUTH_AuthZ_05_EnvironmentAttributesMfaDefaultFalse)`
- Source: `tests/auth/test_auth_wavec_authorization.cpp`:102
- Brief: AUTH-AuthZ-05 — is_mfa_verified in EnvironmentAttributes must default to false so that policies requiring MFA deny by default when no environment context is supplied.
- Parameters:
  - `<unnamed>` (AuthorizationPolicyTest): n/a
  - `<unnamed>` (AUTH_AuthZ_05_EnvironmentAttributesMfaDefaultFalse): n/a

#### `TEST(AuthorizationPolicyTest, AUTH_AuthZ_06_SubjectAttributesGroupsStartEmpty)`
- Source: `tests/auth/test_auth_wavec_authorization.cpp`:118
- Brief: AUTH-AuthZ-06 — A default-constructed SubjectAttributes must have an empty groups vector. Group-membership policies must not fire against uninitialised data.
- Parameters:
  - `<unnamed>` (AuthorizationPolicyTest): n/a
  - `<unnamed>` (AUTH_AuthZ_06_SubjectAttributesGroupsStartEmpty): n/a

#### `TEST(AuthorizationPolicyTest, AUTH_AuthZ_07_PolicyDecisionDenyHasReason)`
- Source: `tests/auth/test_auth_wavec_authorization.cpp`:133
- Brief: AUTH-AuthZ-07 — A PolicyEvaluationResult set to DENY must preserve both the decision and the associated reason string for audit logging.
- Parameters:
  - `<unnamed>` (AuthorizationPolicyTest): n/a
  - `<unnamed>` (AUTH_AuthZ_07_PolicyDecisionDenyHasReason): n/a

#### `TEST(AuthorizationPolicyTest, AUTH_AuthZ_08_ResourceAttributesClassificationIsSettable)`
- Source: `tests/auth/test_auth_wavec_authorization.cpp`:151
- Brief: AUTH-AuthZ-08 — The classification field in ResourceAttributes must accept standard data sensitivity labels and preserve them correctly.
- Parameters:
  - `<unnamed>` (AuthorizationPolicyTest): n/a
  - `<unnamed>` (AUTH_AuthZ_08_ResourceAttributesClassificationIsSettable): n/a

#### `TEST(AuthorizationPolicyTest, PolicyEvaluationResultApplicablePoliciesAreSettable)`
- Source: `tests/auth/test_auth_wavec_authorization.cpp`:184
- Brief: Verifies PolicyEvaluationResult applicable_policies list is settable.
- Parameters:
  - `<unnamed>` (AuthorizationPolicyTest): n/a
  - `<unnamed>` (PolicyEvaluationResultApplicablePoliciesAreSettable): n/a

#### `TEST(AuthorizationPolicyTest, SubjectAttributesTenantAndClearanceAreSettable)`
- Source: `tests/auth/test_auth_wavec_authorization.cpp`:172
- Brief: Verifies SubjectAttributes tenant_id and clearance_level are settable.
- Parameters:
  - `<unnamed>` (AuthorizationPolicyTest): n/a
  - `<unnamed>` (SubjectAttributesTenantAndClearanceAreSettable): n/a

#### `TEST(DistributedTokenBlacklistTest, AUTH_Token_06_AddThenIsRevokedReturnsTrue)`
- Source: `tests/auth/test_auth_wavec_token_lifecycle.cpp`:184
- Brief: AUTH-Token-06 — After add(jti, future_expiry) the blacklist must return isRevoked(jti) == true for that JTI.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (AUTH_Token_06_AddThenIsRevokedReturnsTrue): n/a

#### `TEST(DistributedTokenBlacklistTest, AUTH_Token_07_IsRevokedReturnsFalseForUnknownJti)`
- Source: `tests/auth/test_auth_wavec_token_lifecycle.cpp`:205
- Brief: AUTH-Token-07 — isRevoked() must return false for a JTI that was never added to the blacklist.
- Parameters:
  - `<unnamed>` (DistributedTokenBlacklistTest): n/a
  - `<unnamed>` (AUTH_Token_07_IsRevokedReturnsFalseForUnknownJti): n/a

#### `TEST(JWTConfigurationTest, JWKSTimeoutBounds)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:446
- Brief: Edge Case: JWKS timeout configuration limits Expected: Timeout bounds are enforced.
- Parameters:
  - `<unnamed>` (JWTConfigurationTest): n/a
  - `<unnamed>` (JWKSTimeoutBounds): n/a

#### `TEST(JWTConfigurationTest, MaxRetriesBounded)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:458
- Brief: Edge Case: Max retries configuration Expected: Retry count is bounded.
- Parameters:
  - `<unnamed>` (JWTConfigurationTest): n/a
  - `<unnamed>` (MaxRetriesBounded): n/a

#### `TEST(JWTConfigurationTest, MissingAudienceWithValidationRequired)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:434
- Brief: Edge Case: Missing required_audience_validation but no audience set Expected: Throw at construction.
- Parameters:
  - `<unnamed>` (JWTConfigurationTest): n/a
  - `<unnamed>` (MissingAudienceWithValidationRequired): n/a

#### `TEST(JWTConfigurationTest, MissingIssuerWithValidationRequired)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:422
- Brief: Edge Case: Missing required_issuer_validation but no issuer set Expected: Throw at construction.
- Parameters:
  - `<unnamed>` (JWTConfigurationTest): n/a
  - `<unnamed>` (MissingIssuerWithValidationRequired): n/a

#### `TEST_F(AuthMethodsTest, AUTH_Auth_01_RejectsMalformedTokenEmptySignature)`
- Source: `tests/auth/test_auth_wavec_authentication_methods.cpp`:94
- Brief: AUTH-Auth-01 — A JWT whose third segment (signature) is empty is structurally malformed and must be rejected with JWT_INVALID_FORMAT.
- Parameters:
  - `<unnamed>` (AuthMethodsTest): n/a
  - `<unnamed>` (AUTH_Auth_01_RejectsMalformedTokenEmptySignature): n/a

#### `TEST_F(AuthMethodsTest, AUTH_Auth_02_RejectsExpiredToken)`
- Source: `tests/auth/test_auth_wavec_authentication_methods.cpp`:128
- Brief: AUTH-Auth-02 — A token whose exp claim is in the past must be rejected. The validator should raise an error rather than returning valid claims.
- Parameters:
  - `<unnamed>` (AuthMethodsTest): n/a
  - `<unnamed>` (AUTH_Auth_02_RejectsExpiredToken): n/a
- Details: Strategy: craft a three-part "JWT" with exp=1 (Unix epoch + 1 second) so it is always expired. The signature verification will fail first or the expiry check will fire — either way the call must not succeed.

#### `TEST_F(AuthMethodsTest, AUTH_Auth_03_RejectsWrongIssuer)`
- Source: `tests/auth/test_auth_wavec_authentication_methods.cpp`:149
- Brief: AUTH-Auth-03 — When require_issuer_validation=true the validator must reject a token whose iss claim does not match expected_issuer.
- Parameters:
  - `<unnamed>` (AuthMethodsTest): n/a
  - `<unnamed>` (AUTH_Auth_03_RejectsWrongIssuer): n/a

#### `TEST_F(AuthMethodsTest, AUTH_Auth_04_RejectsWrongAudience)`
- Source: `tests/auth/test_auth_wavec_authentication_methods.cpp`:178
- Brief: AUTH-Auth-04 — When require_audience_validation=true the validator must reject a token whose aud claim does not match expected_audience.
- Parameters:
  - `<unnamed>` (AuthMethodsTest): n/a
  - `<unnamed>` (AUTH_Auth_04_RejectsWrongAudience): n/a

#### `TEST_F(AuthMethodsTest, AUTH_Auth_05_SAMLRejectsEmptyAssertion)`
- Source: `tests/auth/test_auth_wavec_authentication_methods.cpp`:207
- Brief: AUTH-Auth-05 — SAMLAuthenticator::processResponse() must reject an empty Base64 SAML response string without crashing.
- Parameters:
  - `<unnamed>` (AuthMethodsTest): n/a
  - `<unnamed>` (AUTH_Auth_05_SAMLRejectsEmptyAssertion): n/a

#### `TEST_F(AuthMethodsTest, AUTH_Auth_06_MTLSRejectsEmptyCertificate)`
- Source: `tests/auth/test_auth_wavec_authentication_methods.cpp`:229
- Brief: AUTH-Auth-06 — MTLSAuthenticator::authenticate() must reject an empty PEM string and throw rather than returning bogus claims.
- Parameters:
  - `<unnamed>` (AuthMethodsTest): n/a
  - `<unnamed>` (AUTH_Auth_06_MTLSRejectsEmptyCertificate): n/a

#### `TEST_F(AuthMethodsTest, AUTH_Auth_07_ConstructorThrowsOnMissingIssuerConfig)`
- Source: `tests/auth/test_auth_wavec_authentication_methods.cpp`:250
- Brief: AUTH-Auth-07 — Constructing JWTValidator with a config that has require_issuer_validation=true but no expected_issuer set must throw AUTH_CONFIG_INVALID at construction time.
- Parameters:
  - `<unnamed>` (AuthMethodsTest): n/a
  - `<unnamed>` (AUTH_Auth_07_ConstructorThrowsOnMissingIssuerConfig): n/a

#### `TEST_F(AuthMethodsTest, AUTH_Auth_08_RejectsMissingJtiWhenRequired)`
- Source: `tests/auth/test_auth_wavec_authentication_methods.cpp`:279
- Brief: AUTH-Auth-08 — When require_jti=true the validator must reject a token that does not contain a "jti" claim.
- Parameters:
  - `<unnamed>` (AuthMethodsTest): n/a
  - `<unnamed>` (AUTH_Auth_08_RejectsMissingJtiWhenRequired): n/a
- Details: The token payload intentionally omits "jti"; expiry is set to the far future to ensure the rejection is about the missing claim, not expiry.

#### `TEST_F(FederationProvidersTest, AUTH_Provider_01_ValidateTokenThrowsForUnknownIssuer)`
- Source: `tests/auth/test_auth_wavec_federation_providers.cpp`:91
- Brief: AUTH-Provider-01 — Validating a token whose issuer is not registered must throw AuthException with FEDERATION_UNKNOWN_REALM.
- Parameters:
  - `<unnamed>` (FederationProvidersTest): n/a
  - `<unnamed>` (AUTH_Provider_01_ValidateTokenThrowsForUnknownIssuer): n/a
- Details: Strategy: register no realms, then call validateToken() with a token carrying a recognisable iss claim. The manager must fail-closed. Token payload (Base64url): {"sub":"u1","iss":"https://rogue.example.com","exp":9999999999}

#### `TEST_F(FederationProvidersTest, AUTH_Provider_02_AddRealmIncrementsRealmCount)`
- Source: `tests/auth/test_auth_wavec_federation_providers.cpp`:122
- Brief: AUTH-Provider-02 — After a successful addRealm() the manager must report the new realm in realmCount() and hasRealm().
- Parameters:
  - `<unnamed>` (FederationProvidersTest): n/a
  - `<unnamed>` (AUTH_Provider_02_AddRealmIncrementsRealmCount): n/a

#### `TEST_F(FederationProvidersTest, AUTH_Provider_03_AddRealmWithHttpEndpointThrows)`
- Source: `tests/auth/test_auth_wavec_federation_providers.cpp`:150
- Brief: AUTH-Provider-03 — An OIDC realm whose issuer_url uses plain HTTP rather than HTTPS must be rejected. The expected error is AUTH_CONFIG_INVALID (configuration contract violation) or PROVIDER_CAPABILITY_MISMATCH (TLS not configured).
- Parameters:
  - `<unnamed>` (FederationProvidersTest): n/a
  - `<unnamed>` (AUTH_Provider_03_AddRealmWithHttpEndpointThrows): n/a
- Details: Per auth_principal_contract.h §6: "Provider configuration declares a capability that the runtime environment cannot satisfy."

#### `TEST_F(FederationProvidersTest, AUTH_Provider_04_ValidateTokenWrapsDegradedProvider)`
- Source: `tests/auth/test_auth_wavec_federation_providers.cpp`:184
- Brief: AUTH-Provider-04 — When the OIDCProvider backend throws a generic network or discovery error, FederatedIdentityManager must wrap it as PROVIDER_DEGRADED (fail-closed per auth_principal_contract.h §6).
- Parameters:
  - `<unnamed>` (FederationProvidersTest): n/a
  - `<unnamed>` (AUTH_Provider_04_ValidateTokenWrapsDegradedProvider): n/a
- Details: Strategy: register a realm backed by an HTTP GET function that always throws std::runtime_error (simulating network failure), then call validateToken() with a token from that issuer.

#### `TEST_F(FederationProvidersTest, AUTH_Provider_05_RemoveRealmDecreasesRealmCount)`
- Source: `tests/auth/test_auth_wavec_federation_providers.cpp`:231
- Brief: AUTH-Provider-05 — removeRealm() must remove the realm from the registry and return true; a subsequent hasRealm() must return false.
- Parameters:
  - `<unnamed>` (FederationProvidersTest): n/a
  - `<unnamed>` (AUTH_Provider_05_RemoveRealmDecreasesRealmCount): n/a

#### `TEST_F(FederationProvidersTest, AUTH_Provider_06_ExchangeTokenThrowsForUnknownRealm)`
- Source: `tests/auth/test_auth_wavec_federation_providers.cpp`:259
- Brief: AUTH-Provider-06 — exchangeToken() with a subject_token whose issuer is not registered must throw FEDERATION_UNKNOWN_REALM before any token exchange network call is made.
- Parameters:
  - `<unnamed>` (FederationProvidersTest): n/a
  - `<unnamed>` (AUTH_Provider_06_ExchangeTokenThrowsForUnknownRealm): n/a
- Details: Token payload (iss = "https://unknown-realm.example.com"): Base64url({"alg":"RS256"}).Base64url({"sub":"u1","iss":"https://unknown-realm.example.com","exp":9999999999}).fakesig

#### `TEST_F(JWTAccessControlTest, AccessWithEmptyClaims)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:560
- Brief: Edge Case: Empty claims Expected: hasAccess returns false.
- Parameters:
  - `<unnamed>` (JWTAccessControlTest): n/a
  - `<unnamed>` (AccessWithEmptyClaims): n/a

#### `TEST_F(JWTAccessControlTest, AccessWithMatchingGroup)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:522
- Brief: Edge Case: User has matching group in encryption context Expected: hasAccess returns true.
- Parameters:
  - `<unnamed>` (JWTAccessControlTest): n/a
  - `<unnamed>` (AccessWithMatchingGroup): n/a

#### `TEST_F(JWTAccessControlTest, AccessWithUserContextMatch)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:548
- Brief: Edge Case: Encryption context is user subject Expected: hasAccess returns true.
- Parameters:
  - `<unnamed>` (JWTAccessControlTest): n/a
  - `<unnamed>` (AccessWithUserContextMatch): n/a

#### `TEST_F(JWTAccessControlTest, AccessWithoutMatchingGroup)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:535
- Brief: Edge Case: User lacks group in encryption context Expected: hasAccess returns false.
- Parameters:
  - `<unnamed>` (JWTAccessControlTest): n/a
  - `<unnamed>` (AccessWithoutMatchingGroup): n/a

#### `TEST_F(JWTAsyncValidationTest, AsyncValidationEmptyToken)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:399
- Brief: Edge Case: validateAsync with null/empty token Expected: std::future holds MalformedArtifact exception.
- Parameters:
  - `<unnamed>` (JWTAsyncValidationTest): n/a
  - `<unnamed>` (AsyncValidationEmptyToken): n/a

#### `TEST_F(JWTAsyncValidationTest, AsyncValidationThreadPoolNotReady)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:380
- Brief: Edge Case: validateAsync on uninitialized thread pool Expected: std::future holds InternalError exception.
- Parameters:
  - `<unnamed>` (JWTAsyncValidationTest): n/a
  - `<unnamed>` (AsyncValidationThreadPoolNotReady): n/a

#### `TEST_F(JWTFailureClassificationTest, ExpiredToken)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:250
- Brief: Edge Case: Expired token Expected: AuthFailureClass::ExpiredCredential.
- Parameters:
  - `<unnamed>` (JWTFailureClassificationTest): n/a
  - `<unnamed>` (ExpiredToken): n/a

#### `TEST_F(JWTFailureClassificationTest, InvalidBase64UrlEncoding)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:238
- Brief: Edge Case: Invalid Base64URL encoding in token Expected: AuthFailureClass::MalformedArtifact.
- Parameters:
  - `<unnamed>` (JWTFailureClassificationTest): n/a
  - `<unnamed>` (InvalidBase64UrlEncoding): n/a

#### `TEST_F(JWTFailureClassificationTest, MalformedTokenFormat)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:226
- Brief: Edge Case: Malformed token format (not 3 dot-separated parts) Expected: AuthFailureClass::MalformedArtifact.
- Parameters:
  - `<unnamed>` (JWTFailureClassificationTest): n/a
  - `<unnamed>` (MalformedTokenFormat): n/a

#### `TEST_F(JWTKIDRevocationTest, EmptyKIDRevocation)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:311
- Brief: Edge Case: Empty KID revocation Expected: Empty string handled gracefully.
- Parameters:
  - `<unnamed>` (JWTKIDRevocationTest): n/a
  - `<unnamed>` (EmptyKIDRevocation): n/a

#### `TEST_F(JWTKIDRevocationTest, MultipleKIDRevocations)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:292
- Brief: Edge Case: Multiple KID revocations Expected: All revoked KIDs are tracked.
- Parameters:
  - `<unnamed>` (JWTKIDRevocationTest): n/a
  - `<unnamed>` (MultipleKIDRevocations): n/a

#### `TEST_F(JWTKIDRevocationTest, RevokeAndCheckKID)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:275
- Brief: Edge Case: Revoke and check KID status Expected: isKidRevoked returns true after revocation.
- Parameters:
  - `<unnamed>` (JWTKIDRevocationTest): n/a
  - `<unnamed>` (RevokeAndCheckKID): n/a

#### `TEST_F(JWTScopeExtractionTest, EmptyScopes)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:357
- Brief: Edge Case: Empty scopes Expected: Empty vector.
- Parameters:
  - `<unnamed>` (JWTScopeExtractionTest): n/a
  - `<unnamed>` (EmptyScopes): n/a

#### `TEST_F(JWTScopeExtractionTest, ParseArrayScopes)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:345
- Brief: Edge Case: scope claim as JSON array Expected: Preserved as-is in scopes vector.
- Parameters:
  - `<unnamed>` (JWTScopeExtractionTest): n/a
  - `<unnamed>` (ParseArrayScopes): n/a

#### `TEST_F(JWTScopeExtractionTest, ParseSpaceSeparatedScopes)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:331
- Brief: Edge Case: scope claim as space-separated string Expected: Parsed into individual scope vectors.
- Parameters:
  - `<unnamed>` (JWTScopeExtractionTest): n/a
  - `<unnamed>` (ParseSpaceSeparatedScopes): n/a

#### `TEST_F(JWTTemporalContractTest, ClockSkewToleranceBoundary)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:131
- Brief: Edge Case: Clock skew tolerance boundary (±60 seconds) Expected: Token with exp at now + 30s should be accepted within skew window.
- Parameters:
  - `<unnamed>` (JWTTemporalContractTest): n/a
  - `<unnamed>` (ClockSkewToleranceBoundary): n/a

#### `TEST_F(JWTTemporalContractTest, MaxSessionLifetime)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:153
- Brief: Edge Case: Session absolute timeout (30 days max) Expected: Enforced per auth_principal_contract.h § 2.
- Parameters:
  - `<unnamed>` (JWTTemporalContractTest): n/a
  - `<unnamed>` (MaxSessionLifetime): n/a

#### `TEST_F(JWTTokenBlacklistTest, MissingJTIWithBlacklistAttached)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:203
- Brief: Edge Case: Token missing JTI when blacklist is required Expected: May warn once (lazy), but continue validation.
- Parameters:
  - `<unnamed>` (JWTTokenBlacklistTest): n/a
  - `<unnamed>` (MissingJTIWithBlacklistAttached): n/a

#### `TEST_F(JWTTokenBlacklistTest, ProceedWithoutBlacklist)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:194
- Brief: Edge Case: JTI present but blacklist not configured Expected: Token proceeds without JTI revocation check (no rejection).
- Parameters:
  - `<unnamed>` (JWTTokenBlacklistTest): n/a
  - `<unnamed>` (ProceedWithoutBlacklist): n/a

#### `TEST_F(JWTTokenBlacklistTest, RejectRevokedJTI)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:178
- Brief: Edge Case: JTI present and blacklist reports revoked Expected: Token rejected with InvalidCredential failure class.
- Parameters:
  - `<unnamed>` (JWTTokenBlacklistTest): n/a
  - `<unnamed>` (RejectRevokedJTI): n/a

#### `TEST_F(JWTTokenExpirationTest, ExactExpirationTime)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:576
- Brief: Edge Case: Token isExpired check at exact expiration time Expected: isExpired returns true when now > exp.
- Parameters:
  - `<unnamed>` (JWTTokenExpirationTest): n/a
  - `<unnamed>` (ExactExpirationTime): n/a

#### `TEST_F(JWTTokenExpirationTest, FutureToken)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:593
- Brief: Edge Case: Future token not expired Expected: isExpired returns false.
- Parameters:
  - `<unnamed>` (JWTTokenExpirationTest): n/a
  - `<unnamed>` (FutureToken): n/a

#### `TEST_F(JWTTokenExpirationTest, PastToken)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:607
- Brief: Edge Case: Past token expired Expected: isExpired returns true.
- Parameters:
  - `<unnamed>` (JWTTokenExpirationTest): n/a
  - `<unnamed>` (PastToken): n/a

#### `TEST_F(JWTTokenSizeValidationTest, AcceptTokenAtBoundary)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:92
- Brief: Edge Case: Token at exact size boundary (16KB) Expected: Should be accepted for further validation.
- Parameters:
  - `<unnamed>` (JWTTokenSizeValidationTest): n/a
  - `<unnamed>` (AcceptTokenAtBoundary): n/a

#### `TEST_F(JWTTokenSizeValidationTest, RejectBearerPrefixOnly)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:84
- Brief: Edge Case: Token with only "Bearer " prefix Expected: Rejection (malformed artifact).
- Parameters:
  - `<unnamed>` (JWTTokenSizeValidationTest): n/a
  - `<unnamed>` (RejectBearerPrefixOnly): n/a

#### `TEST_F(JWTTokenSizeValidationTest, RejectEmptyToken)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:76
- Brief: Edge Case: Empty token string Expected: Immediate rejection.
- Parameters:
  - `<unnamed>` (JWTTokenSizeValidationTest): n/a
  - `<unnamed>` (RejectEmptyToken): n/a

#### `TEST_F(JWTTokenSizeValidationTest, RejectOversizedToken)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:55
- Brief: Edge Case: Token size exceeds MAX_JWT_TOKEN_SIZE (16KB) Expected: Rejection before any cryptographic processing.
- Parameters:
  - `<unnamed>` (JWTTokenSizeValidationTest): n/a
  - `<unnamed>` (RejectOversizedToken): n/a

#### `TEST_F(JWTUserKeyDerivationTest, DeriveKeyValidInputs)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:487
- Brief: Edge Case: Derive key with valid inputs Expected: Returns non-empty key bytes.
- Parameters:
  - `<unnamed>` (JWTUserKeyDerivationTest): n/a
  - `<unnamed>` (DeriveKeyValidInputs): n/a

#### `TEST_F(JWTUserKeyDerivationTest, DeriveKeyWithEmptyDEK)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:475
- Brief: Edge Case: Derive key with empty DEK Expected: Should handle gracefully or throw.
- Parameters:
  - `<unnamed>` (JWTUserKeyDerivationTest): n/a
  - `<unnamed>` (DeriveKeyWithEmptyDEK): n/a

#### `TEST_F(JWTUserKeyDerivationTest, DeriveKeyWithLongFieldName)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:502
- Brief: Edge Case: Derive key with very long field name Expected: Handled without truncation errors.
- Parameters:
  - `<unnamed>` (JWTUserKeyDerivationTest): n/a
  - `<unnamed>` (DeriveKeyWithLongFieldName): n/a

#### `TEST_F(RateLimitingTest, AUTH_RateLimit_01_FirstRequestIsAllowed)`
- Source: `tests/auth/test_auth_wavec_rate_limiting.cpp`:69
- Brief: AUTH-RateLimit-01 — The very first authentication attempt from a user must be allowed regardless of the configured threshold.
- Parameters:
  - `<unnamed>` (RateLimitingTest): n/a
  - `<unnamed>` (AUTH_RateLimit_01_FirstRequestIsAllowed): n/a

#### `TEST_F(RateLimitingTest, AUTH_RateLimit_02_BlocksAfterThresholdExceededForSameUser)`
- Source: `tests/auth/test_auth_wavec_rate_limiting.cpp`:87
- Brief: AUTH-RateLimit-02 — Once the per-user rate-limit threshold is crossed, subsequent attempts from the same user must be blocked.
- Parameters:
  - `<unnamed>` (RateLimitingTest): n/a
  - `<unnamed>` (AUTH_RateLimit_02_BlocksAfterThresholdExceededForSameUser): n/a
- Details: Strategy: exhaust the threshold with allowAuthAttempt() calls and assert that at least one is eventually denied.

#### `TEST_F(RateLimitingTest, AUTH_RateLimit_03_AllowsAgainAfterWindowExpires)`
- Source: `tests/auth/test_auth_wavec_rate_limiting.cpp`:123
- Brief: AUTH-RateLimit-03 — After the rate-limit window expires the user must be allowed to authenticate again.
- Parameters:
  - `<unnamed>` (RateLimitingTest): n/a
  - `<unnamed>` (AUTH_RateLimit_03_AllowsAgainAfterWindowExpires): n/a
- Details: Strategy: use a limiter with max_attempts_per_user_per_minute=1 and record a failed attempt so the first window is consumed. After sleeping past the window duration (≈ 1/60 of a minute ≈ very short for tests) we call reset() to simulate window expiry and confirm the attempt is allowed. Note: The internal token-bucket window granularity may be coarser than milliseconds. We therefore use reset() to deterministically clear state rather than sleeping for 60 seconds.

#### `TEST_F(RateLimitingTest, AUTH_RateLimit_04_DifferentUsersHaveIndependentCounters)`
- Source: `tests/auth/test_auth_wavec_rate_limiting.cpp`:146
- Brief: AUTH-RateLimit-04 — Exhausting the rate limit for user A must not affect user B's ability to authenticate.
- Parameters:
  - `<unnamed>` (RateLimitingTest): n/a
  - `<unnamed>` (AUTH_RateLimit_04_DifferentUsersHaveIndependentCounters): n/a

#### `TEST_F(RateLimitingTest, AUTH_RateLimit_05_ResetClearsCounterForUser)`
- Source: `tests/auth/test_auth_wavec_rate_limiting.cpp`:170
- Brief: AUTH-RateLimit-05 — After reset() the limiter must behave as if no prior attempts were recorded; the first attempt is allowed again.
- Parameters:
  - `<unnamed>` (RateLimitingTest): n/a
  - `<unnamed>` (AUTH_RateLimit_05_ResetClearsCounterForUser): n/a

#### `TEST_F(RateLimitingTest, AUTH_RateLimit_06_ThreadSafetyConcurrentRequestsStayBounded)`
- Source: `tests/auth/test_auth_wavec_rate_limiting.cpp`:213
- Brief: AUTH-RateLimit-06 — Under concurrent load from multiple threads for the same user, the total number of ALLOWED attempts must not far exceed the configured threshold. The limiter must remain free of data races (verified by TSAN when enabled).
- Parameters:
  - `<unnamed>` (RateLimitingTest): n/a
  - `<unnamed>` (AUTH_RateLimit_06_ThreadSafetyConcurrentRequestsStayBounded): n/a
- Details: We check a soft bound: allowed ≤ threshold * kSlackFactor. A small slack factor accounts for racing window resets in token-bucket implementations.

#### `TEST_F(SAMLAttributeExtractionTest, EmailAttributeCustomName)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:584
- Brief: Edge Case: Email attribute extraction with custom name Expected: Extracted correctly from configured attribute name.
- Parameters:
  - `<unnamed>` (SAMLAttributeExtractionTest): n/a
  - `<unnamed>` (EmailAttributeCustomName): n/a

#### `TEST_F(SAMLAttributeExtractionTest, GroupsAndRolesExtraction)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:613
- Brief: Edge Case: Groups and roles attributes Expected: Extracted into attributes_groups and attributes_roles vectors.
- Parameters:
  - `<unnamed>` (SAMLAttributeExtractionTest): n/a
  - `<unnamed>` (GroupsAndRolesExtraction): n/a

#### `TEST_F(SAMLAttributeExtractionTest, MissingEmailAttribute)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:595
- Brief: Edge Case: Missing email attribute Expected: Empty email in claims.
- Parameters:
  - `<unnamed>` (SAMLAttributeExtractionTest): n/a
  - `<unnamed>` (MissingEmailAttribute): n/a

#### `TEST_F(SAMLAttributeExtractionTest, MultipleValuesForAttribute)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:604
- Brief: Edge Case: Multiple values for single-valued attribute Expected: First value used or error.
- Parameters:
  - `<unnamed>` (SAMLAttributeExtractionTest): n/a
  - `<unnamed>` (MultipleValuesForAttribute): n/a

#### `TEST_F(SAMLAudienceValidationTest, AudienceMatches)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:696
- Brief: Edge Case: Audience includes SP Entity ID Expected: Accepted.
- Parameters:
  - `<unnamed>` (SAMLAudienceValidationTest): n/a
  - `<unnamed>` (AudienceMatches): n/a

#### `TEST_F(SAMLAudienceValidationTest, AudienceMismatch)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:708
- Brief: Edge Case: Audience does not include SP Entity ID Expected: Rejected.
- Parameters:
  - `<unnamed>` (SAMLAudienceValidationTest): n/a
  - `<unnamed>` (AudienceMismatch): n/a

#### `TEST_F(SAMLAudienceValidationTest, NoAudienceRestriction)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:720
- Brief: Edge Case: No AudienceRestriction in assertion Expected: Accepted (no restriction enforced).
- Parameters:
  - `<unnamed>` (SAMLAudienceValidationTest): n/a
  - `<unnamed>` (NoAudienceRestriction): n/a

#### `TEST_F(SAMLAuthnRequestTest, BuildAuthnRequestEmptyRelayState)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:179
- Brief: Edge Case: Build AuthnRequest with empty relay state Expected: Returns valid URL with SAMLRequest parameter.
- Parameters:
  - `<unnamed>` (SAMLAuthnRequestTest): n/a
  - `<unnamed>` (BuildAuthnRequestEmptyRelayState): n/a

#### `TEST_F(SAMLAuthnRequestTest, BuildAuthnRequestLongRelayState)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:211
- Brief: Edge Case: Build AuthnRequest with very long relay state (>80 chars) Expected: Handled gracefully (truncation or encoding).
- Parameters:
  - `<unnamed>` (SAMLAuthnRequestTest): n/a
  - `<unnamed>` (BuildAuthnRequestLongRelayState): n/a

#### `TEST_F(SAMLAuthnRequestTest, BuildAuthnRequestWithID)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:226
- Brief: Edge Case: Retrieve both URL and request ID Expected: buildAuthnRequest returns AuthnRequestParams with request_id.
- Parameters:
  - `<unnamed>` (SAMLAuthnRequestTest): n/a
  - `<unnamed>` (BuildAuthnRequestWithID): n/a

#### `TEST_F(SAMLAuthnRequestTest, BuildAuthnRequestWithRelayState)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:194
- Brief: Edge Case: Build AuthnRequest with non-empty relay state Expected: URL includes both SAMLRequest and RelayState.
- Parameters:
  - `<unnamed>` (SAMLAuthnRequestTest): n/a
  - `<unnamed>` (BuildAuthnRequestWithRelayState): n/a

#### `TEST_F(SAMLConfigurationTest, EmptyIdPEntityID)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:113
- Brief: Edge Case: Empty IdP Entity ID Expected: Throw at construction.
- Parameters:
  - `<unnamed>` (SAMLConfigurationTest): n/a
  - `<unnamed>` (EmptyIdPEntityID): n/a

#### `TEST_F(SAMLConfigurationTest, EmptyIdPSSOURL)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:102
- Brief: Edge Case: Empty IdP SSO URL Expected: Throw at construction.
- Parameters:
  - `<unnamed>` (SAMLConfigurationTest): n/a
  - `<unnamed>` (EmptyIdPSSOURL): n/a

#### `TEST_F(SAMLConfigurationTest, EmptySPAcsURL)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:91
- Brief: Edge Case: Empty SP ACS URL Expected: Throw at construction.
- Parameters:
  - `<unnamed>` (SAMLConfigurationTest): n/a
  - `<unnamed>` (EmptySPAcsURL): n/a

#### `TEST_F(SAMLConfigurationTest, EmptySPEntityID)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:80
- Brief: Edge Case: Empty SP Entity ID Expected: Throw at construction.
- Parameters:
  - `<unnamed>` (SAMLConfigurationTest): n/a
  - `<unnamed>` (EmptySPEntityID): n/a

#### `TEST_F(SAMLConfigurationTest, InvalidCertificatePEM)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:124
- Brief: Edge Case: Invalid/malformed certificate PEM Expected: Throw at construction.
- Parameters:
  - `<unnamed>` (SAMLConfigurationTest): n/a
  - `<unnamed>` (InvalidCertificatePEM): n/a

#### `TEST_F(SAMLConfigurationTest, NegativeClockSkew)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:135
- Brief: Edge Case: Clock skew set to negative value Expected: Accepted (implementation-defined behavior).
- Parameters:
  - `<unnamed>` (SAMLConfigurationTest): n/a
  - `<unnamed>` (NegativeClockSkew): n/a

#### `TEST_F(SAMLConfigurationTest, ZeroReplayCacheSize)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:148
- Brief: Edge Case: Replay cache size set to zero Expected: Accepted but disables replay detection.
- Parameters:
  - `<unnamed>` (SAMLConfigurationTest): n/a
  - `<unnamed>` (ZeroReplayCacheSize): n/a

#### `TEST_F(SAMLIssuerValidationTest, EmptyIssuer)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:666
- Brief: Edge Case: Empty Issuer in response Expected: Rejected.
- Parameters:
  - `<unnamed>` (SAMLIssuerValidationTest): n/a
  - `<unnamed>` (EmptyIssuer): n/a

#### `TEST_F(SAMLIssuerValidationTest, IssuerMatches)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:642
- Brief: Edge Case: Issuer matches configured IdP Entity ID Expected: Accepted.
- Parameters:
  - `<unnamed>` (SAMLIssuerValidationTest): n/a
  - `<unnamed>` (IssuerMatches): n/a

#### `TEST_F(SAMLIssuerValidationTest, IssuerMismatch)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:654
- Brief: Edge Case: Issuer does not match Expected: Rejected with SAML_INVALID_RESPONSE.
- Parameters:
  - `<unnamed>` (SAMLIssuerValidationTest): n/a
  - `<unnamed>` (IssuerMismatch): n/a

#### `TEST_F(SAMLReplayDetectionTest, DuplicateAssertionID)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:343
- Brief: Edge Case: Same AssertionID within cache TTL Expected: Second occurrence rejected as replay.
- Parameters:
  - `<unnamed>` (SAMLReplayDetectionTest): n/a
  - `<unnamed>` (DuplicateAssertionID): n/a

#### `TEST_F(SAMLReplayDetectionTest, ExpiredAssertionIDReuse)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:371
- Brief: Edge Case: AssertionID expires after NotOnOrAfter Expected: Duplicate ID allowed after expiry.
- Parameters:
  - `<unnamed>` (SAMLReplayDetectionTest): n/a
  - `<unnamed>` (ExpiredAssertionIDReuse): n/a

#### `TEST_F(SAMLReplayDetectionTest, ReplayCacheEviction)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:358
- Brief: Edge Case: AssertionID cache exceeds max_replay_cache_size Expected: Old entries evicted, new ones added.
- Parameters:
  - `<unnamed>` (SAMLReplayDetectionTest): n/a
  - `<unnamed>` (ReplayCacheEviction): n/a

#### `TEST_F(SAMLResponseProcessingTest, EmptySAMLResponse)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:259
- Brief: Edge Case: Empty SAMLResponse Expected: Throw AuthException with SAML_INVALID_RESPONSE.
- Parameters:
  - `<unnamed>` (SAMLResponseProcessingTest): n/a
  - `<unnamed>` (EmptySAMLResponse): n/a

#### `TEST_F(SAMLResponseProcessingTest, FailureStatus)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:296
- Brief: Edge Case: SAMLResponse with Status != Success Expected: Throw AuthException with SAML_INVALID_RESPONSE.
- Parameters:
  - `<unnamed>` (SAMLResponseProcessingTest): n/a
  - `<unnamed>` (FailureStatus): n/a

#### `TEST_F(SAMLResponseProcessingTest, InResponseToValidation)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:308
- Brief: Edge Case: Process response with InResponseTo validation Expected: Validates request_id against response InResponseTo.
- Parameters:
  - `<unnamed>` (SAMLResponseProcessingTest): n/a
  - `<unnamed>` (InResponseToValidation): n/a

#### `TEST_F(SAMLResponseProcessingTest, InvalidBase64Encoding)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:270
- Brief: Edge Case: Invalid Base64 encoding Expected: Throw AuthException with SAML_INVALID_RESPONSE.
- Parameters:
  - `<unnamed>` (SAMLResponseProcessingTest): n/a
  - `<unnamed>` (InvalidBase64Encoding): n/a

#### `TEST_F(SAMLResponseProcessingTest, MalformedXML)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:282
- Brief: Edge Case: Malformed XML after Base64 decode Expected: Throw AuthException with SAML_INVALID_RESPONSE.
- Parameters:
  - `<unnamed>` (SAMLResponseProcessingTest): n/a
  - `<unnamed>` (MalformedXML): n/a

#### `TEST_F(SAMLSha1DeprecationTest, Sha1AllowedWithWarning)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:548
- Brief: Edge Case: SHA-1 signature when allow_sha1_deprecated=true Expected: Accepted but logged warning.
- Parameters:
  - `<unnamed>` (SAMLSha1DeprecationTest): n/a
  - `<unnamed>` (Sha1AllowedWithWarning): n/a

#### `TEST_F(SAMLSha1DeprecationTest, Sha1RejectedWhenDisallowed)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:560
- Brief: Edge Case: SHA-1 signature when allow_sha1_deprecated=false Expected: Rejected.
- Parameters:
  - `<unnamed>` (SAMLSha1DeprecationTest): n/a
  - `<unnamed>` (Sha1RejectedWhenDisallowed): n/a

#### `TEST_F(SAMLSignatureVerificationTest, SignatureAlgorithmMismatch)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:518
- Brief: Edge Case: Signature algorithm mismatch Expected: May be rejected depending on config.
- Parameters:
  - `<unnamed>` (SAMLSignatureVerificationTest): n/a
  - `<unnamed>` (SignatureAlgorithmMismatch): n/a

#### `TEST_F(SAMLSignatureVerificationTest, SignatureVerificationFailure)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:506
- Brief: Edge Case: Signature verification with wrong key Expected: Throw with SAML_INVALID_RESPONSE.
- Parameters:
  - `<unnamed>` (SAMLSignatureVerificationTest): n/a
  - `<unnamed>` (SignatureVerificationFailure): n/a

#### `TEST_F(SAMLSignatureVerificationTest, UnsignedAssertionWhenRequired)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:494
- Brief: Edge Case: Assertion not signed when required Expected: Throw with SAML_INVALID_RESPONSE.
- Parameters:
  - `<unnamed>` (SAMLSignatureVerificationTest): n/a
  - `<unnamed>` (UnsignedAssertionWhenRequired): n/a

#### `TEST_F(SAMLSignatureVerificationTest, UnsignedResponseWhenRequired)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:482
- Brief: Edge Case: SAMLResponse not signed when required Expected: Throw with SAML_INVALID_RESPONSE.
- Parameters:
  - `<unnamed>` (SAMLSignatureVerificationTest): n/a
  - `<unnamed>` (UnsignedResponseWhenRequired): n/a

#### `TEST_F(SAMLTimeValidationTest, NotBeforeTooFar)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:420
- Brief: Edge Case: Assertion NotBefore too far in future Expected: Rejected.
- Parameters:
  - `<unnamed>` (SAMLTimeValidationTest): n/a
  - `<unnamed>` (NotBeforeTooFar): n/a

#### `TEST_F(SAMLTimeValidationTest, NotBeforeWithinClockSkew)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:405
- Brief: Edge Case: Assertion NotBefore in future within clock skew Expected: Accepted.
- Parameters:
  - `<unnamed>` (SAMLTimeValidationTest): n/a
  - `<unnamed>` (NotBeforeWithinClockSkew): n/a

#### `TEST_F(SAMLTimeValidationTest, NotOnOrAfterTooFar)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:448
- Brief: Edge Case: Assertion NotOnOrAfter too far in past Expected: Rejected (expired).
- Parameters:
  - `<unnamed>` (SAMLTimeValidationTest): n/a
  - `<unnamed>` (NotOnOrAfterTooFar): n/a

#### `TEST_F(SAMLTimeValidationTest, NotOnOrAfterWithinClockSkew)`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:434
- Brief: Edge Case: Assertion NotOnOrAfter in past within clock skew Expected: Accepted.
- Parameters:
  - `<unnamed>` (SAMLTimeValidationTest): n/a
  - `<unnamed>` (NotOnOrAfterWithinClockSkew): n/a

#### `TEST_F(TokenLifecycleTest, AUTH_Token_01_CreateSessionReturnsNonEmptyId)`
- Source: `tests/auth/test_auth_wavec_token_lifecycle.cpp`:72
- Brief: AUTH-Token-01 — createSession() must return a non-empty, "sess_"- prefixed session identifier for a valid user.
- Parameters:
  - `<unnamed>` (TokenLifecycleTest): n/a
  - `<unnamed>` (AUTH_Token_01_CreateSessionReturnsNonEmptyId): n/a

#### `TEST_F(TokenLifecycleTest, AUTH_Token_02_ValidateSessionReturnsTrueForFreshSession)`
- Source: `tests/auth/test_auth_wavec_token_lifecycle.cpp`:88
- Brief: AUTH-Token-02 — A freshly created session must validate as valid and carry the correct user_id.
- Parameters:
  - `<unnamed>` (TokenLifecycleTest): n/a
  - `<unnamed>` (AUTH_Token_02_ValidateSessionReturnsTrueForFreshSession): n/a

#### `TEST_F(TokenLifecycleTest, AUTH_Token_03_ValidateSessionReturnsFalseForExpired)`
- Source: `tests/auth/test_auth_wavec_token_lifecycle.cpp`:109
- Brief: AUTH-Token-03 — A session whose absolute_timeout is exceeded must be treated as expired (valid=false) on subsequent validation.
- Parameters:
  - `<unnamed>` (TokenLifecycleTest): n/a
  - `<unnamed>` (AUTH_Token_03_ValidateSessionReturnsFalseForExpired): n/a
- Details: Strategy: construct a SessionManager with a 1 ms absolute_timeout, create a session, sleep briefly, then validate — it must be expired.

#### `TEST_F(TokenLifecycleTest, AUTH_Token_04_TerminateSessionMakesItInvalid)`
- Source: `tests/auth/test_auth_wavec_token_lifecycle.cpp`:135
- Brief: AUTH-Token-04 — After terminateSession() the session must no longer validate as valid.
- Parameters:
  - `<unnamed>` (TokenLifecycleTest): n/a
  - `<unnamed>` (AUTH_Token_04_TerminateSessionMakesItInvalid): n/a

#### `TEST_F(TokenLifecycleTest, AUTH_Token_05_PruneExpiredRemovesDeadSessions)`
- Source: `tests/auth/test_auth_wavec_token_lifecycle.cpp`:155
- Brief: AUTH-Token-05 — pruneExpired() must remove sessions whose absolute lifetime has elapsed and reduce size() accordingly.
- Parameters:
  - `<unnamed>` (TokenLifecycleTest): n/a
  - `<unnamed>` (AUTH_Token_05_PruneExpiredRemovesDeadSessions): n/a

#### `TEST_F(TokenLifecycleTest, AUTH_Token_08_EnforcesMaxSessionsPerUser)`
- Source: `tests/auth/test_auth_wavec_token_lifecycle.cpp`:222
- Brief: AUTH-Token-08 — When max_sessions_per_user is exceeded the oldest session must be evicted so the total for that user stays at the limit; the new session is always created successfully.
- Parameters:
  - `<unnamed>` (TokenLifecycleTest): n/a
  - `<unnamed>` (AUTH_Token_08_EnforcesMaxSessionsPerUser): n/a

#### `TEST_F(WebAuthnAttestationTest, InvalidCBOR)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:529
- Brief: Edge Case: Invalid CBOR attestation object Expected: Throw AuthException(AUTH_INTERNAL_ERROR).
- Parameters:
  - `<unnamed>` (WebAuthnAttestationTest): n/a
  - `<unnamed>` (InvalidCBOR): n/a

#### `TEST_F(WebAuthnAttestationTest, NoneAttestationFormat)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:513
- Brief: Edge Case: Parse "none" attestation format Expected: fmt = "none", auth_data extracted.
- Parameters:
  - `<unnamed>` (WebAuthnAttestationTest): n/a
  - `<unnamed>` (NoneAttestationFormat): n/a

#### `TEST_F(WebAuthnAttestationTest, PackedAttestationFormat)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:521
- Brief: Edge Case: Parse "packed" attestation format Expected: fmt = "packed", signature verified.
- Parameters:
  - `<unnamed>` (WebAuthnAttestationTest): n/a
  - `<unnamed>` (PackedAttestationFormat): n/a

#### `TEST_F(WebAuthnAuthDataTest, AttestedCredentialPresent)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:592
- Brief: Edge Case: Parse authenticator data with attested credential Expected: has_attested_credential = true, aaguid populated.
- Parameters:
  - `<unnamed>` (WebAuthnAuthDataTest): n/a
  - `<unnamed>` (AttestedCredentialPresent): n/a

#### `TEST_F(WebAuthnAuthDataTest, NoAttestedCredential)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:601
- Brief: Edge Case: Parse authenticator data without attested credential Expected: has_attested_credential = false, aaguid empty.
- Parameters:
  - `<unnamed>` (WebAuthnAuthDataTest): n/a
  - `<unnamed>` (NoAttestedCredential): n/a

#### `TEST_F(WebAuthnAuthDataTest, SignatureCounter)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:618
- Brief: Edge Case: Parse sign counter Expected: Counter value extracted and used for rollback detection.
- Parameters:
  - `<unnamed>` (WebAuthnAuthDataTest): n/a
  - `<unnamed>` (SignatureCounter): n/a

#### `TEST_F(WebAuthnAuthDataTest, UserPresentFlag)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:610
- Brief: Edge Case: Parse UP flag (User Presence) Expected: UP flag (bit 0) always set.
- Parameters:
  - `<unnamed>` (WebAuthnAuthDataTest): n/a
  - `<unnamed>` (UserPresentFlag): n/a

#### `TEST_F(WebAuthnAuthenticationCeremonyTest, BadSignature)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:409
- Brief: Edge Case: Complete authentication with bad signature Expected: Throw AuthException(AUTH_INVALID_CREDENTIALS).
- Parameters:
  - `<unnamed>` (WebAuthnAuthenticationCeremonyTest): n/a
  - `<unnamed>` (BadSignature): n/a

#### `TEST_F(WebAuthnAuthenticationCeremonyTest, CompleteAuthenticationInvalidChallenge)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:335
- Brief: Edge Case: Complete authentication with invalid challenge Expected: Throw AuthException(AUTH_TOKEN_INVALID).
- Parameters:
  - `<unnamed>` (WebAuthnAuthenticationCeremonyTest): n/a
  - `<unnamed>` (CompleteAuthenticationInvalidChallenge): n/a

#### `TEST_F(WebAuthnAuthenticationCeremonyTest, MissingUserPresenceFlag)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:420
- Brief: Edge Case: Complete authentication missing UP flag Expected: Throw AuthException(AUTH_INVALID_CREDENTIALS).
- Parameters:
  - `<unnamed>` (WebAuthnAuthenticationCeremonyTest): n/a
  - `<unnamed>` (MissingUserPresenceFlag): n/a

#### `TEST_F(WebAuthnAuthenticationCeremonyTest, SignatureCounterIncrement)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:377
- Brief: Edge Case: Complete authentication with counter increment Expected: Accepted and counter updated.
- Parameters:
  - `<unnamed>` (WebAuthnAuthenticationCeremonyTest): n/a
  - `<unnamed>` (SignatureCounterIncrement): n/a

#### `TEST_F(WebAuthnAuthenticationCeremonyTest, SignatureCounterNoIncrement)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:367
- Brief: Edge Case: Complete authentication with same counter (invalid) Expected: May be rejected (counter must increment).
- Parameters:
  - `<unnamed>` (WebAuthnAuthenticationCeremonyTest): n/a
  - `<unnamed>` (SignatureCounterNoIncrement): n/a

#### `TEST_F(WebAuthnAuthenticationCeremonyTest, SignatureCounterRollback)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:352
- Brief: Edge Case: Complete authentication with signature counter rollback Expected: Throw AuthException(AUTH_TOKEN_INVALID) – cloned token detected.
- Parameters:
  - `<unnamed>` (WebAuthnAuthenticationCeremonyTest): n/a
  - `<unnamed>` (SignatureCounterRollback): n/a

#### `TEST_F(WebAuthnAuthenticationCeremonyTest, StartAuthenticationDiscoverable)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:314
- Brief: Edge Case: Start authentication without specifying user_id Expected: Returns CredentialRequestOptions with empty allow_credentials.
- Parameters:
  - `<unnamed>` (WebAuthnAuthenticationCeremonyTest): n/a
  - `<unnamed>` (StartAuthenticationDiscoverable): n/a

#### `TEST_F(WebAuthnAuthenticationCeremonyTest, StartAuthenticationWithUserID)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:324
- Brief: Edge Case: Start authentication with user_id specified Expected: Returns CredentialRequestOptions (caller populates allow_credentials).
- Parameters:
  - `<unnamed>` (WebAuthnAuthenticationCeremonyTest): n/a
  - `<unnamed>` (StartAuthenticationWithUserID): n/a

#### `TEST_F(WebAuthnAuthenticationCeremonyTest, WrongOrigin)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:387
- Brief: Edge Case: Complete authentication with wrong origin Expected: Throw AuthException(AUTH_TOKEN_INVALID).
- Parameters:
  - `<unnamed>` (WebAuthnAuthenticationCeremonyTest): n/a
  - `<unnamed>` (WrongOrigin): n/a

#### `TEST_F(WebAuthnAuthenticationCeremonyTest, WrongRPID)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:398
- Brief: Edge Case: Complete authentication with wrong RP ID Expected: Throw AuthException(AUTH_TOKEN_INVALID).
- Parameters:
  - `<unnamed>` (WebAuthnAuthenticationCeremonyTest): n/a
  - `<unnamed>` (WrongRPID): n/a

#### `TEST_F(WebAuthnChallengeLifecycleTest, ChallengeExpiry)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:164
- Brief: Edge Case: Challenge expires after TTL (300 seconds) Expected: verifyAndConsumeChallenge throws after expiry.
- Parameters:
  - `<unnamed>` (WebAuthnChallengeLifecycleTest): n/a
  - `<unnamed>` (ChallengeExpiry): n/a

#### `TEST_F(WebAuthnChallengeLifecycleTest, ChallengeGeneration)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:135
- Brief: Edge Case: Challenge generation returns 32 random bytes base64url Expected: Challenge string is non-empty, valid base64url, decodable to 32 bytes.
- Parameters:
  - `<unnamed>` (WebAuthnChallengeLifecycleTest): n/a
  - `<unnamed>` (ChallengeGeneration): n/a

#### `TEST_F(WebAuthnChallengeLifecycleTest, ChallengePurging)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:178
- Brief: Edge Case: Challenge purging removes all expired entries Expected: After purgeExpiredChallenges(), only valid challenges remain.
- Parameters:
  - `<unnamed>` (WebAuthnChallengeLifecycleTest): n/a
  - `<unnamed>` (ChallengePurging): n/a

#### `TEST_F(WebAuthnChallengeLifecycleTest, ChallengeRandomness)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:151
- Brief: Edge Case: Consecutive challenges are different (randomness) Expected: Two consecutive calls produce different challenges.
- Parameters:
  - `<unnamed>` (WebAuthnChallengeLifecycleTest): n/a
  - `<unnamed>` (ChallengeRandomness): n/a

#### `TEST_F(WebAuthnClientDataTest, ClientDataTypeCreate)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:552
- Brief: Edge Case: ClientDataJSON with type="webauthn.create" Expected: Accepted for registration ceremony.
- Parameters:
  - `<unnamed>` (WebAuthnClientDataTest): n/a
  - `<unnamed>` (ClientDataTypeCreate): n/a

#### `TEST_F(WebAuthnClientDataTest, ClientDataTypeGet)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:560
- Brief: Edge Case: ClientDataJSON with type="webauthn.get" Expected: Accepted for authentication ceremony.
- Parameters:
  - `<unnamed>` (WebAuthnClientDataTest): n/a
  - `<unnamed>` (ClientDataTypeGet): n/a

#### `TEST_F(WebAuthnClientDataTest, MissingClientDataFields)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:577
- Brief: Edge Case: ClientDataJSON missing required fields Expected: Throw AuthException(AUTH_INTERNAL_ERROR).
- Parameters:
  - `<unnamed>` (WebAuthnClientDataTest): n/a
  - `<unnamed>` (MissingClientDataFields): n/a

#### `TEST_F(WebAuthnClientDataTest, ValidClientData)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:544
- Brief: Edge Case: Parse valid clientDataJSON Expected: type, challenge, origin extracted correctly.
- Parameters:
  - `<unnamed>` (WebAuthnClientDataTest): n/a
  - `<unnamed>` (ValidClientData): n/a

#### `TEST_F(WebAuthnClientDataTest, WrongClientDataType)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:568
- Brief: Edge Case: ClientDataJSON with wrong type Expected: Throw AuthException(AUTH_TOKEN_INVALID).
- Parameters:
  - `<unnamed>` (WebAuthnClientDataTest): n/a
  - `<unnamed>` (WrongClientDataType): n/a

#### `TEST_F(WebAuthnCoseKeyTest, CoseKeyES256Parsing)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:482
- Brief: Edge Case: Parse COSE key to DER SPKI for ES256 Expected: Returns DER-encoded SPKI + "ES256" algorithm.
- Parameters:
  - `<unnamed>` (WebAuthnCoseKeyTest): n/a
  - `<unnamed>` (CoseKeyES256Parsing): n/a

#### `TEST_F(WebAuthnCoseKeyTest, CoseKeyRS256Parsing)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:490
- Brief: Edge Case: Parse COSE key to DER SPKI for RS256 Expected: Returns DER-encoded SPKI + "RS256" algorithm.
- Parameters:
  - `<unnamed>` (WebAuthnCoseKeyTest): n/a
  - `<unnamed>` (CoseKeyRS256Parsing): n/a

#### `TEST_F(WebAuthnCoseKeyTest, UnsupportedCoseAlgorithm)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:498
- Brief: Edge Case: Parse unsupported COSE key algorithm Expected: Throw AuthException(AUTH_NOT_IMPLEMENTED).
- Parameters:
  - `<unnamed>` (WebAuthnCoseKeyTest): n/a
  - `<unnamed>` (UnsupportedCoseAlgorithm): n/a

#### `TEST_F(WebAuthnInitializationTest, AttachAuditLogger)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:109
- Brief: Edge Case: Attach audit logger Expected: Non-owning pointer stored.
- Parameters:
  - `<unnamed>` (WebAuthnInitializationTest): n/a
  - `<unnamed>` (AttachAuditLogger): n/a

#### `TEST_F(WebAuthnInitializationTest, EmptyRelyingPartyID)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:61
- Brief: Edge Case: Empty RP ID Expected: Throw AuthException(AUTH_CONFIG_INVALID).
- Parameters:
  - `<unnamed>` (WebAuthnInitializationTest): n/a
  - `<unnamed>` (EmptyRelyingPartyID): n/a

#### `TEST_F(WebAuthnInitializationTest, EmptyRelyingPartyName)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:73
- Brief: Edge Case: Empty RP name (optional) Expected: Should be accepted.
- Parameters:
  - `<unnamed>` (WebAuthnInitializationTest): n/a
  - `<unnamed>` (EmptyRelyingPartyName): n/a

#### `TEST_F(WebAuthnInitializationTest, SetExpectedOrigin)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:97
- Brief: Edge Case: Set expected origin explicitly Expected: Overrides default HTTPS://{rp.id} origin.
- Parameters:
  - `<unnamed>` (WebAuthnInitializationTest): n/a
  - `<unnamed>` (SetExpectedOrigin): n/a

#### `TEST_F(WebAuthnInitializationTest, ValidRelyingPartyInit)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:51
- Brief: Edge Case: Valid RP with all fields populated Expected: Authenticator initializes successfully.
- Parameters:
  - `<unnamed>` (WebAuthnInitializationTest): n/a
  - `<unnamed>` (ValidRelyingPartyInit): n/a

#### `TEST_F(WebAuthnInitializationTest, VeryLongRelyingPartyName)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:85
- Brief: Edge Case: Very long RP name Expected: Accepted (no truncation).
- Parameters:
  - `<unnamed>` (WebAuthnInitializationTest): n/a
  - `<unnamed>` (VeryLongRelyingPartyName): n/a

#### `TEST_F(WebAuthnRandomBytesTest, DeterministicChallenges)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:661
- Brief: Edge Case: Deterministic challenges with fixed RNG Expected: Same user/seed produces same challenge.
- Parameters:
  - `<unnamed>` (WebAuthnRandomBytesTest): n/a
  - `<unnamed>` (DeterministicChallenges): n/a

#### `TEST_F(WebAuthnRandomBytesTest, InjectedRandomBytes)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:640
- Brief: Edge Case: setRandBytesForTesting with valid function Expected: Challenge generation uses injected randomness.
- Parameters:
  - `<unnamed>` (WebAuthnRandomBytesTest): n/a
  - `<unnamed>` (InjectedRandomBytes): n/a

#### `TEST_F(WebAuthnRegistrationCeremonyTest, CompleteRegistrationInvalidChallenge)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:255
- Brief: Edge Case: Complete registration with invalid challenge Expected: Throw AuthException(AUTH_TOKEN_INVALID).
- Parameters:
  - `<unnamed>` (WebAuthnRegistrationCeremonyTest): n/a
  - `<unnamed>` (CompleteRegistrationInvalidChallenge): n/a

#### `TEST_F(WebAuthnRegistrationCeremonyTest, CompleteRegistrationWrongOrigin)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:269
- Brief: Edge Case: Complete registration with wrong origin Expected: Throw AuthException(AUTH_TOKEN_INVALID).
- Parameters:
  - `<unnamed>` (WebAuthnRegistrationCeremonyTest): n/a
  - `<unnamed>` (CompleteRegistrationWrongOrigin): n/a

#### `TEST_F(WebAuthnRegistrationCeremonyTest, CompleteRegistrationWrongRPID)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:281
- Brief: Edge Case: Complete registration with wrong RP ID Expected: Throw AuthException(AUTH_TOKEN_INVALID).
- Parameters:
  - `<unnamed>` (WebAuthnRegistrationCeremonyTest): n/a
  - `<unnamed>` (CompleteRegistrationWrongRPID): n/a

#### `TEST_F(WebAuthnRegistrationCeremonyTest, StartRegistrationNonResident)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:210
- Brief: Edge Case: Start registration with resident key disabled Expected: CredentialCreationOptions.authenticator_selection.require_resident_key = false.
- Parameters:
  - `<unnamed>` (WebAuthnRegistrationCeremonyTest): n/a
  - `<unnamed>` (StartRegistrationNonResident): n/a

#### `TEST_F(WebAuthnRegistrationCeremonyTest, StartRegistrationResident)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:221
- Brief: Edge Case: Start registration with resident key enabled Expected: CredentialCreationOptions.authenticator_selection.require_resident_key = true.
- Parameters:
  - `<unnamed>` (WebAuthnRegistrationCeremonyTest): n/a
  - `<unnamed>` (StartRegistrationResident): n/a

#### `TEST_F(WebAuthnRegistrationCeremonyTest, SupportedAlgorithms)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:232
- Brief: Edge Case: CredentialCreationOptions contains supported algorithms Expected: pub_key_cred_params includes ["ES256", "RS256"].
- Parameters:
  - `<unnamed>` (WebAuthnRegistrationCeremonyTest): n/a
  - `<unnamed>` (SupportedAlgorithms): n/a

#### `TEST_F(WebAuthnRegistrationCeremonyTest, UnsupportedKeyAlgorithm)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:290
- Brief: Edge Case: Complete registration with unsupported key algorithm Expected: Throw AuthException(AUTH_NOT_IMPLEMENTED).
- Parameters:
  - `<unnamed>` (WebAuthnRegistrationCeremonyTest): n/a
  - `<unnamed>` (UnsupportedKeyAlgorithm): n/a

#### `TEST_F(WebAuthnRegistrationCeremonyTest, UserPresenceFlagRequired)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:247
- Brief: Edge Case: User presence (UP) flag mandatory Expected: completeRegistration throws if UP flag not set in attestation.
- Parameters:
  - `<unnamed>` (WebAuthnRegistrationCeremonyTest): n/a
  - `<unnamed>` (UserPresenceFlagRequired): n/a

#### `TEST_F(WebAuthnSignatureVerificationTest, ES256InvalidSignature)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:443
- Brief: Edge Case: ES256 signature verification with invalid signature Expected: Throw AuthException(AUTH_INVALID_CREDENTIALS).
- Parameters:
  - `<unnamed>` (WebAuthnSignatureVerificationTest): n/a
  - `<unnamed>` (ES256InvalidSignature): n/a

#### `TEST_F(WebAuthnSignatureVerificationTest, ES256ValidSignature)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:435
- Brief: Edge Case: ES256 signature verification with valid signature Expected: verifySignature completes without exception.
- Parameters:
  - `<unnamed>` (WebAuthnSignatureVerificationTest): n/a
  - `<unnamed>` (ES256ValidSignature): n/a

#### `TEST_F(WebAuthnSignatureVerificationTest, RS256InvalidSignature)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:459
- Brief: Edge Case: RS256 signature verification with invalid signature Expected: Throw AuthException(AUTH_INVALID_CREDENTIALS).
- Parameters:
  - `<unnamed>` (WebAuthnSignatureVerificationTest): n/a
  - `<unnamed>` (RS256InvalidSignature): n/a

#### `TEST_F(WebAuthnSignatureVerificationTest, RS256ValidSignature)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:451
- Brief: Edge Case: RS256 signature verification with valid signature Expected: verifySignature completes without exception.
- Parameters:
  - `<unnamed>` (WebAuthnSignatureVerificationTest): n/a
  - `<unnamed>` (RS256ValidSignature): n/a

#### `TEST_F(WebAuthnSignatureVerificationTest, TruncatedSignature)`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:467
- Brief: Edge Case: ECDSA signature with short (truncated) signature Expected: Throw AuthException(AUTH_INVALID_CREDENTIALS).
- Parameters:
  - `<unnamed>` (WebAuthnSignatureVerificationTest): n/a
  - `<unnamed>` (TruncatedSignature): n/a

### themis::auth::tests::AuthMethodsTest

#### `JWTValidatorConfig makePermissiveConfig(const std::string &jwks_url="http://localhost:9999/jwks")`
- Source: `tests/auth/test_auth_wavec_authentication_methods.cpp`:56
- Brief: Build a minimal JWTValidatorConfig that skips issuer/audience enforcement (so construction always succeeds) and points to a fake JWKS URL. Validation of a malformed token fails BEFORE any network request is made.
- Parameters:
  - `jwks_url` (const std::string &): n/a

#### `JWTValidatorConfig makeStrictAudienceConfig(const std::string &expected_audience)`
- Source: `tests/auth/test_auth_wavec_authentication_methods.cpp`:78
- Brief: Build a config with strict audience validation enabled.
- Parameters:
  - `expected_audience` (const std::string &): n/a

#### `JWTValidatorConfig makeStrictIssuerConfig(const std::string &expected_issuer)`
- Source: `tests/auth/test_auth_wavec_authentication_methods.cpp`:68
- Brief: Build a config with strict issuer validation enabled.
- Parameters:
  - `expected_issuer` (const std::string &): n/a

### themis::auth::tests::JWTAsyncValidationTest

#### `JWTValidatorConfig getBaseConfig() const`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:370
- Brief: n/a
- Parameters: none

### themis::auth::tests::JWTFailureClassificationTest

#### `JWTValidatorConfig getBaseConfig() const`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:216
- Brief: n/a
- Parameters: none

### themis::auth::tests::JWTTemporalContractTest

#### `JWTValidatorConfig getBaseConfig() const`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:117
- Brief: n/a
- Parameters: none

### themis::auth::tests::JWTTokenBlacklistTest

#### `void SetUp() override`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:169
- Brief: n/a
- Parameters: none

### themis::auth::tests::MockTokenBlacklist

#### `bool isRevoked(const std::string &jti) const override`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:26
- Brief: n/a
- Parameters:
  - `jti` (const std::string &): n/a

#### `void setRevoked(const std::string &jti, bool revoked)`
- Source: `tests/auth/test_jwt_validator_focused.cpp`:30
- Brief: n/a
- Parameters:
  - `jti` (const std::string &): n/a
  - `revoked` (bool): n/a

### themis::auth::tests::RateLimitingTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_wavec_rate_limiting.cpp`:50
- Brief: n/a
- Parameters: none

#### `AuthRateLimitConfig makeTightConfig(size_t max_per_user=3, uint32_t=200)`
- Source: `tests/auth/test_auth_wavec_rate_limiting.cpp`:39
- Brief: Build an AuthRateLimitConfig with tight limits suitable for deterministic unit tests.
- Parameters:
  - `max_per_user` (size_t): Maximum auth attempts per user per window.
  - `<unnamed>` (uint32_t): n/a
- Details: max_per_user Maximum auth attempts per user per window. window_ms Rate-limit window in milliseconds.

### themis::auth::tests::SAMLAudienceValidationTest

#### `void SetUp() override`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:682
- Brief: n/a
- Parameters: none

### themis::auth::tests::SAMLAuthnRequestTest

#### `void SetUp() override`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:163
- Brief: n/a
- Parameters: none

### themis::auth::tests::SAMLIssuerValidationTest

#### `void SetUp() override`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:627
- Brief: n/a
- Parameters: none

### themis::auth::tests::SAMLReplayDetectionTest

#### `void SetUp() override`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:328
- Brief: n/a
- Parameters: none

### themis::auth::tests::SAMLResponseProcessingTest

#### `void SetUp() override`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:245
- Brief: n/a
- Parameters: none

### themis::auth::tests::SAMLSha1DeprecationTest

#### `void SetUp() override`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:533
- Brief: n/a
- Parameters: none

### themis::auth::tests::SAMLSignatureVerificationTest

#### `void SetUp() override`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:466
- Brief: n/a
- Parameters: none

### themis::auth::tests::SAMLTestHelper

#### `SAMLConfig getValidConfig()`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:47
- Brief: n/a
- Parameters: none

#### `std::string loadRepoTestCertificatePem()`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:33
- Brief: n/a
- Parameters: none

### themis::auth::tests::SAMLTimeValidationTest

#### `void SetUp() override`
- Source: `tests/auth/test_saml_authenticator_focused.cpp`:390
- Brief: n/a
- Parameters: none

### themis::auth::tests::TokenLifecycleTest

#### `void SetUp() override`
- Source: `tests/auth/test_auth_wavec_token_lifecycle.cpp`:56
- Brief: n/a
- Parameters: none

### themis::auth::tests::WebAuthnAuthenticationCeremonyTest

#### `void SetUp() override`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:303
- Brief: n/a
- Parameters: none

### themis::auth::tests::WebAuthnChallengeLifecycleTest

#### `void SetUp() override`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:125
- Brief: n/a
- Parameters: none

### themis::auth::tests::WebAuthnRandomBytesTest

#### `void SetUp() override`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:630
- Brief: n/a
- Parameters: none

### themis::auth::tests::WebAuthnRegistrationCeremonyTest

#### `void SetUp() override`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:199
- Brief: n/a
- Parameters: none

### themis::auth::tests::WebAuthnTestHelper

#### `WebAuthnAuthenticator::RelyingParty getValidRP()`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:32
- Brief: n/a
- Parameters: none

#### `WebAuthnAuthenticator::User getValidUser()`
- Source: `tests/auth/test_webauthn_authenticator_focused.cpp`:36
- Brief: n/a
- Parameters: none

### themis::bench::ahp

#### `BENCHMARK(BM_AHP08_FederationRealmLookup) -> Arg(1) ->Arg(5) ->Arg(20) ->Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:450
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_AHP08_FederationRealmLookup): n/a

#### `BENCHMARK(BM_AHP11_FederationRealmTrustLookup) -> Arg(1) ->Arg(5) ->Arg(20) ->Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:581
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_AHP11_FederationRealmTrustLookup): n/a

#### `BENCHMARK_DEFINE_F(DistributedBlacklistFixture, AHP06_DistributedAdd)(benchmark`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:357
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedBlacklistFixture): n/a
  - `<unnamed>` (AHP06_DistributedAdd): n/a

#### `BENCHMARK_DEFINE_F(DistributedBlacklistFixture, AHP07_DistributedIsRevoked)(benchmark`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:392
- Brief: AHP-07: DistributedTokenBlacklist::isRevoked() — O(1) RocksDB point-read.
- Parameters:
  - `<unnamed>` (DistributedBlacklistFixture): n/a
  - `<unnamed>` (AHP07_DistributedIsRevoked): n/a
- Details: Pre-warms the RocksDB block-cache before measurement. GATE-AHP-06: p99 ≤ 1 µs (warm cache).

#### `void BM_AHP02_BlacklistIsRevoked_Hit(benchmark::State &state)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:183
- Brief: AHP-02: isRevoked() for a JTI known to be in the blacklist.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Measures the hot O(1) lookup path. GATE-AHP-01: p99 ≤ 1 µs.

#### `void BM_AHP03_BlacklistIsRevoked_Miss(benchmark::State &state)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:219
- Brief: AHP-03: isRevoked() for a JTI that is NOT in the blacklist.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-AHP-02: p99 ≤ 1 µs.

#### `void BM_AHP04_SessionCreate(benchmark::State &state)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:255
- Brief: AHP-04: SessionManager::createSession() latency.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Measures cryptographic random ID generation + map insert. GATE-AHP-03: p99 ≤ 5 ms.

#### `void BM_AHP05_SessionValidate(benchmark::State &state)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:293
- Brief: AHP-05: SessionManager::validateSession() latency for an active session.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Measures map lookup + expiry check against a pre-created session. GATE-AHP-04: p99 ≤ 1 ms.

#### `void BM_AHP08_FederationRealmLookup(benchmark::State &state)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:432
- Brief: AHP-08: FederatedIdentityManager::realmCount() — mutex + hash-map overhead.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Tests the synchronisation cost of the realm-map fast path with N registered realms. No network I/O; purely in-process cost.

#### `void BM_AHP09_LDAPBindSimulation(benchmark::State &state)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:471
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_AHP10_OIDCTokenParse(benchmark::State &state)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:500
- Brief: AHP-10: OIDC token issuer extraction overhead (pre-crypto path).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Measures the cost of extracting the iss claim from a JWT payload without performing any cryptographic verification. Represents the fast-path cost in FederatedIdentityManager::validateToken() before realm dispatch. Tagged with bench_auth_protocol_micro.

#### `void BM_AHP11_FederationRealmTrustLookup(benchmark::State &state)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:553
- Brief: AHP-11: FederatedIdentityManager::hasRealm() lookup with N realms.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Extends AHP-08 to also measure hasRealm() and getCrossProviderTrusts() cost with a populated trust registry. Represents the fast-path cost in FederatedIdentityManager::validateToken() after realm dispatch. Tagged with bench_auth_protocol_micro.

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:484
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

#### `UseManualTime() -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:206
- Brief: n/a
- Parameters: none

#### `std::chrono::system_clock::time_point futureExpiry()`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:156
- Brief: Time-point helper: far future expiry.
- Parameters: none

#### `std::string makeJti(int index)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:151
- Brief: Generate a deterministic JTI string from an integer index.
- Parameters:
  - `index` (int): n/a

#### `std::unique_ptr< TokenBlacklist > makePreloadedBlacklist()`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:166
- Brief: n/a
- Parameters: none
- Details: SetUp helper: returns an in-memory TokenBlacklist with kBlacklistPreloadSize entries pre-inserted so that the "hit" benchmark path is warm.

#### `double percentile(std::vector< double > samples, double fraction)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:103
- Brief: n/a
- Parameters:
  - `samples` (std::vector< double >): n/a
  - `fraction` (double): n/a

#### `void publishLatencyCounters(benchmark::State &state, const std::vector< double > &samples_us, double gate_target_us)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:134
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
  - `samples_us` (const std::vector< double > &): n/a
  - `gate_target_us` (double): n/a

#### `LatencySummary summarizeLatencies(const std::vector< double > &samples_us)`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:120
- Brief: n/a
- Parameters:
  - `samples_us` (const std::vector< double > &): n/a

### themis::bench::ahp::DistributedBlacklistFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:337
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/auth/bench_auth_hotpaths.cpp`:351
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

### themis::bench::ahp::themis::auth::AuthAuditLogger

#### `AuthAuditLogger(utils::AuditLogger *logger=nullptr)`
- Source: `include/auth/auth_audit_logger.h`:35
- Brief: n/a
- Parameters:
  - `logger` (utils::AuditLogger *): n/a

#### `void emit(utils::SecurityEventType type, const std::string &user_id, const std::string &resource, const nlohmann::json &details={})`
- Source: `include/auth/auth_audit_logger.h`:335
- Brief: n/a
- Parameters:
  - `type` (utils::SecurityEventType): n/a
  - `user_id` (const std::string &): n/a
  - `resource` (const std::string &): n/a
  - `details` (const nlohmann::json &): n/a

#### `void emitWithDecisionClass(utils::SecurityEventType type, const std::string &user_id, const std::string &resource, DecisionClass dc, const nlohmann::json &details={})`
- Source: `include/auth/auth_audit_logger.h`:51
- Brief: n/a
- Parameters:
  - `type` (utils::SecurityEventType): n/a
  - `user_id` (const std::string &): n/a
  - `resource` (const std::string &): n/a
  - `dc` (DecisionClass): n/a
  - `details` (const nlohmann::json &): n/a

#### `bool isEnabled() const`
- Source: `include/auth/auth_audit_logger.h`:45
- Brief: n/a
- Parameters: none

#### `void logAccountLockoutTriggered(const std::string &user_id, const std::string &ip)`
- Source: `include/auth/auth_audit_logger.h`:329
- Brief: Log Account Lockout Triggered.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `ip` (const std::string &): Input parameter.
- Details: user_id Identifier of the user. ip Input parameter.

#### `void logApiKeyFailure(const std::string &key_id, const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:155
- Brief: Log Api Key Failure.
- Parameters:
  - `key_id` (const std::string &): Identifier of the key.
  - `reason` (const std::string &): Input parameter.
- Details: key_id Identifier of the key. reason Input parameter.

#### `void logApiKeySuccess(const std::string &key_id, const std::string &principal)`
- Source: `include/auth/auth_audit_logger.h`:147
- Brief: Log Api Key Success.
- Parameters:
  - `key_id` (const std::string &): Identifier of the key.
  - `principal` (const std::string &): Input parameter.
- Details: key_id Identifier of the key. principal Input parameter.

#### `void logBruteForceDetected(const std::string &user_id, const std::string &ip, size_t failed_attempts)`
- Source: `include/auth/auth_audit_logger.h`:312
- Brief: -------------------------------------------------------------------- Anomaly detection events (brute-force, credential stuffing) --------------------------------------------------------------------
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `ip` (const std::string &): Input parameter.
  - `failed_attempts` (size_t): Input parameter.
- Details: user_id Identifier of the user. ip Input parameter. failed_attempts Input parameter.

#### `void logCredentialStuffingSuspected(const std::string &ip, size_t distinct_users)`
- Source: `include/auth/auth_audit_logger.h`:321
- Brief: Log Credential Stuffing Suspected.
- Parameters:
  - `ip` (const std::string &): Input parameter.
  - `distinct_users` (size_t): Input parameter.
- Details: ip Input parameter. distinct_users Input parameter.

#### `void logJWTFailure(const std::string &reason, const std::string &kid="")`
- Source: `include/auth/auth_audit_logger.h`:73
- Brief: n/a
- Parameters:
  - `reason` (const std::string &): n/a
  - `kid` (const std::string &): n/a

#### `void logJWTSuccess(const std::string &sub, const std::string &jti, const std::string &issuer, const std::string &kid)`
- Source: `include/auth/auth_audit_logger.h`:68
- Brief: Log JWTSuccess.
- Parameters:
  - `sub` (const std::string &): Input parameter.
  - `jti` (const std::string &): Input parameter.
  - `issuer` (const std::string &): Input parameter.
  - `kid` (const std::string &): Input parameter.
- Details: sub Input parameter. jti Input parameter. issuer Input parameter. kid Input parameter.

#### `void logKerberosFailure(const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:98
- Brief: Log Kerberos Failure.
- Parameters:
  - `reason` (const std::string &): Input parameter.
- Details: reason Input parameter.

#### `void logKerberosSuccess(const std::string &principal)`
- Source: `include/auth/auth_audit_logger.h`:92
- Brief: Log Kerberos Success.
- Parameters:
  - `principal` (const std::string &): Input parameter.
- Details: principal Input parameter.

#### `void logLDAPFailure(const std::string &username, const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:278
- Brief: Log LDAPFailure.
- Parameters:
  - `username` (const std::string &): Input parameter.
  - `reason` (const std::string &): Input parameter.
- Details: username Input parameter. reason Input parameter.

#### `void logLDAPSuccess(const std::string &username, const std::string &dn)`
- Source: `include/auth/auth_audit_logger.h`:270
- Brief: Log LDAPSuccess.
- Parameters:
  - `username` (const std::string &): Input parameter.
  - `dn` (const std::string &): Input parameter.
- Details: username Input parameter. dn Input parameter.

#### `void logMFAEnrolled(const std::string &user_id)`
- Source: `include/auth/auth_audit_logger.h`:136
- Brief: Log MFAEnrolled.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user.

#### `void logMTLSFailure(const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:235
- Brief: Log MTLSFailure.
- Parameters:
  - `reason` (const std::string &): Input parameter.
- Details: reason Input parameter.

#### `void logMTLSSuccess(const std::string &principal, const std::string &serial)`
- Source: `include/auth/auth_audit_logger.h`:229
- Brief: Log MTLSSuccess.
- Parameters:
  - `principal` (const std::string &): Input parameter.
  - `serial` (const std::string &): Input parameter.
- Details: principal Input parameter. serial Input parameter.

#### `void logOAuthDeviceDenied(const std::string &client_id, const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:175
- Brief: Log OAuth Device Denied.
- Parameters:
  - `client_id` (const std::string &): Identifier of the client.
  - `reason` (const std::string &): Input parameter.
- Details: client_id Identifier of the client. reason Input parameter.

#### `void logOAuthDeviceGranted(const std::string &client_id, const std::string &sub)`
- Source: `include/auth/auth_audit_logger.h`:167
- Brief: Log OAuth Device Granted.
- Parameters:
  - `client_id` (const std::string &): Identifier of the client.
  - `sub` (const std::string &): Input parameter.
- Details: client_id Identifier of the client. sub Input parameter.

#### `void logPasskeyFailure(const std::string &user_id, const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:208
- Brief: Log Passkey Failure.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `reason` (const std::string &): Input parameter.
- Details: user_id Identifier of the user. reason Input parameter.

#### `void logPasskeyRegistered(const std::string &user_id, const std::string &credential_id, const std::string &rp_id)`
- Source: `include/auth/auth_audit_logger.h`:216
- Brief: Log Passkey Registered.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `credential_id` (const std::string &): Identifier of the credential.
  - `rp_id` (const std::string &): Identifier of the rp.
- Details: user_id Identifier of the user. credential_id Identifier of the credential. rp_id Identifier of the rp.

#### `void logPasskeySuccess(const std::string &user_id, const std::string &credential_id)`
- Source: `include/auth/auth_audit_logger.h`:201
- Brief: Log Passkey Success.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `credential_id` (const std::string &): Identifier of the credential.
- Details: user_id Identifier of the user. credential_id Identifier of the credential.

#### `void logPermissionChange(const std::string &user_id, const std::string &permission, bool granted)`
- Source: `include/auth/auth_audit_logger.h`:257
- Brief: Log Permission Change.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `permission` (const std::string &): Input parameter.
  - `granted` (bool): Input parameter.
- Details: user_id Identifier of the user. permission Input parameter. granted Input parameter.

#### `void logRecoveryCodeUsed(const std::string &user_id)`
- Source: `include/auth/auth_audit_logger.h`:130
- Brief: Log Recovery Code Used.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user.

#### `void logRoleChange(const std::string &user_id, const std::string &old_role, const std::string &new_role)`
- Source: `include/auth/auth_audit_logger.h`:247
- Brief: Log Role Change.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `old_role` (const std::string &): Input parameter.
  - `new_role` (const std::string &): Input parameter.
- Details: user_id Identifier of the user. old_role Input parameter. new_role Input parameter.

#### `void logSAMLFailure(const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:190
- Brief: Log SAMLFailure.
- Parameters:
  - `reason` (const std::string &): Input parameter.
- Details: reason Input parameter.

#### `void logSAMLSuccess(const std::string &subject, const std::string &issuer)`
- Source: `include/auth/auth_audit_logger.h`:183
- Brief: Log SAMLSuccess.
- Parameters:
  - `subject` (const std::string &): Input parameter.
  - `issuer` (const std::string &): Input parameter.
- Details: subject Input parameter. issuer Input parameter.

#### `void logTOTPDrift(const std::string &user_id, int step_offset, std::chrono::system_clock::time_point timestamp)`
- Source: `include/auth/auth_audit_logger.h`:122
- Brief: Log TOTPDrift.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `step_offset` (int): Input parameter.
  - `timestamp` (std::chrono::system_clock::time_point): Input parameter.
- Details: user_id Identifier of the user. step_offset Input parameter. timestamp Input parameter.

#### `void logTOTPFailure(const std::string &user_id)`
- Source: `include/auth/auth_audit_logger.h`:114
- Brief: Log TOTPFailure.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user.

#### `void logTOTPSuccess(const std::string &user_id)`
- Source: `include/auth/auth_audit_logger.h`:108
- Brief: Log TOTPSuccess.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: user_id Identifier of the user.

#### `void logTokenRevoked(const std::string &jti, const std::string &sub)`
- Source: `include/auth/auth_audit_logger.h`:81
- Brief: Log Token Revoked.
- Parameters:
  - `jti` (const std::string &): Input parameter.
  - `sub` (const std::string &): Input parameter.
- Details: jti Input parameter. sub Input parameter.

#### `void logZeroTrustAllowed(const std::string &user_id, const std::string &resource, double trust_score, const std::string &request_id="")`
- Source: `include/auth/auth_audit_logger.h`:285
- Brief: n/a
- Parameters:
  - `user_id` (const std::string &): n/a
  - `resource` (const std::string &): n/a
  - `trust_score` (double): n/a
  - `request_id` (const std::string &): n/a

#### `void logZeroTrustDenied(const std::string &user_id, const std::string &resource, const std::string &reason, const std::string &request_id="")`
- Source: `include/auth/auth_audit_logger.h`:290
- Brief: n/a
- Parameters:
  - `user_id` (const std::string &): n/a
  - `resource` (const std::string &): n/a
  - `reason` (const std::string &): n/a
  - `request_id` (const std::string &): n/a

#### `void logZeroTrustReEvaluationFailed(const std::string &user_id, const std::string &session_id, const std::string &reason)`
- Source: `include/auth/auth_audit_logger.h`:301
- Brief: Log Zero Trust Re Evaluation Failed.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
  - `session_id` (const std::string &): Identifier of the session.
  - `reason` (const std::string &): Input parameter.
- Details: user_id Identifier of the user. session_id Identifier of the session. reason Input parameter.

#### `void setLogger(utils::AuditLogger *logger)`
- Source: `include/auth/auth_audit_logger.h`:43
- Brief: Set Logger.
- Parameters:
  - `logger` (utils::AuditLogger *): Input/output parameter.
- Details: logger Input/output parameter. Implements setLogger without additional internal calls.

### themis::bench::ahp::themis::auth::LDAPConnectionPool

#### `LDAPConnectionPool(LDAPConnectionPool &&)=delete`
- Source: `include/auth/ldap_connection_pool.h`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPConnectionPool &&): n/a

#### `LDAPConnectionPool(const LDAPConnectionPool &)=delete`
- Source: `include/auth/ldap_connection_pool.h`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LDAPConnectionPool &): n/a

#### `LDAPConnectionPool(const LDAPPoolConfig &config)`
- Source: `include/auth/ldap_connection_pool.h`:88
- Brief: LDAPConnection Pool.
- Parameters:
  - `config` (const LDAPPoolConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `int activeConnections() const noexcept`
- Source: `include/auth/ldap_connection_pool.h`:127
- Brief: Active Connections.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `std::unique_ptr< PooledConnection > checkout()`
- Source: `include/auth/ldap_connection_pool.h`:101
- Brief: Checkout.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `const LDAPPoolConfig & config() const noexcept`
- Source: `include/auth/ldap_connection_pool.h`:103
- Brief: n/a
- Parameters: none

#### `LDAP * createConnection()`
- Source: `include/auth/ldap_connection_pool.h`:143
- Brief: Create Connection.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result.

#### `void destroyHandle(LDAP *handle) noexcept`
- Source: `include/auth/ldap_connection_pool.h`:157
- Brief: Destroy Handle.
- Parameters:
  - `handle` (LDAP *): Input/output parameter.
- Details: handle Input/output parameter. Exception safety: noexcept.

#### `int idleConnections() const noexcept`
- Source: `include/auth/ldap_connection_pool.h`:120
- Brief: Idle Connections.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `bool isHealthy(LDAP *handle) const`
- Source: `include/auth/ldap_connection_pool.h`:150
- Brief: Is Healthy.
- Parameters:
  - `handle` (LDAP *): Input/output parameter.
- Return: True when the operation succeeds.
- Details: handle Input/output parameter. True when the operation succeeds.

#### `LDAPConnectionPool & operator=(LDAPConnectionPool &&)=delete`
- Source: `include/auth/ldap_connection_pool.h`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDAPConnectionPool &&): n/a

#### `LDAPConnectionPool & operator=(const LDAPConnectionPool &)=delete`
- Source: `include/auth/ldap_connection_pool.h`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LDAPConnectionPool &): n/a

#### `int poolSize() const noexcept`
- Source: `include/auth/ldap_connection_pool.h`:113
- Brief: -------------------------------------------------------------------- Metrics accessors (used by auth_metrics) --------------------------------------------------------------------
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `void returnConnection(LDAP *handle, bool stale)`
- Source: `include/auth/ldap_connection_pool.h`:137
- Brief: Return Connection.
- Parameters:
  - `handle` (LDAP *): Input/output parameter.
  - `stale` (bool): Input parameter.
- Details: handle Input/output parameter. stale Input parameter.

#### `void setAuditLogger(utils::AuditLogger *logger) noexcept`
- Source: `include/auth/ldap_connection_pool.h`:105
- Brief: n/a
- Parameters:
  - `logger` (utils::AuditLogger *): n/a

#### `~LDAPConnectionPool()`
- Source: `include/auth/ldap_connection_pool.h`:89
- Brief: n/a
- Parameters: none

### themis::bench::ahp::themis::auth::PooledConnection

#### `PooledConnection(LDAPConnectionPool &pool, LDAP *handle)`
- Source: `include/auth/ldap_connection_pool.h`:74
- Brief: n/a
- Parameters:
  - `pool` (LDAPConnectionPool &): n/a
  - `handle` (LDAP *): n/a

#### `PooledConnection(PooledConnection &&)`
- Source: `include/auth/ldap_connection_pool.h`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (PooledConnection &&): n/a

#### `PooledConnection(const PooledConnection &)=delete`
- Source: `include/auth/ldap_connection_pool.h`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PooledConnection &): n/a

#### `bool isStale() const noexcept`
- Source: `include/auth/ldap_connection_pool.h`:69
- Brief: n/a
- Parameters: none

#### `void markStale() noexcept`
- Source: `include/auth/ldap_connection_pool.h`:67
- Brief: n/a
- Parameters: none

#### `PooledConnection & operator=(PooledConnection &&)`
- Source: `include/auth/ldap_connection_pool.h`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (PooledConnection &&): n/a

#### `PooledConnection & operator=(const PooledConnection &)=delete`
- Source: `include/auth/ldap_connection_pool.h`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PooledConnection &): n/a

#### `LDAP * rawHandle() const noexcept`
- Source: `include/auth/ldap_connection_pool.h`:65
- Brief: n/a
- Parameters: none

#### `~PooledConnection()`
- Source: `include/auth/ldap_connection_pool.h`:63
- Brief: n/a
- Parameters: none

