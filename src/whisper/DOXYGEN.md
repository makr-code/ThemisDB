# WHISPER DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\whisper\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\whisper\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 17
- Compounds: 59
- Classes/Structs: 23
- Namespaces: 9
- File Compounds: 17

## Namespaces
- @260171210011301133255203365112043144120054305322
- audio
- benchmark
- plugins
- themis
- themis::audio
- themis::plugins
- themis::whisper
- themis::whisper::@326041076141103153274003167121323146322377321353

## Types
### Classes
- PresetReader
- StubModelRegistry
- StubTranscriber
- WhisperPluginFixture
- WhisperStubTranscriber
- themis::whisper::CompositeAudioChunkReader
- themis::whisper::EnergyThresholdVad
- themis::whisper::FfmpegAudioChunkReader
- themis::whisper::IAudioChunkReader
- themis::whisper::IWhisperTranscriber
- themis::whisper::InMemoryWhisperTranscriber
- themis::whisper::WavAudioChunkReader
- themis::whisper::WhisperPlugin
- themis::whisper::WhisperPluginAdapter
- themis::whisper::WhisperPluginRegistrar
- themis::whisper::WhisperStubTranscriber

### Structs
- TranscriptRecord
- themis::whisper::DiarisationConfig
- themis::whisper::DiarisationResult
- themis::whisper::DiarisationSegment
- themis::whisper::SpeechSegment
- themis::whisper::VadConfig
- themis::whisper::WhisperConfig

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 235

### PresetReader

#### `PresetReader(std::vector< float > s={0.1f}, float sr=16000.f, bool throws=false)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:97
- Brief: n/a
- Parameters:
  - `s` (std::vector< float >): n/a
  - `sr` (float): n/a
  - `throws` (bool): n/a

#### `bool canRead(const std::string &p) const override`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:107
- Brief: Prüft, ob der Reader mit den Inhalten des angegebenen Dateipfades lesen kann.
- Parameters:
  - `path` (const std::string &): n/a
- Return: True, wenn der Reader voraussichtlich geeignete Metadaten findet oder das Format unterstützt wird; andernfalls False.
- Throws:
  - Keine: spezifischen Ausnahmen erwartet, da es sich um eine reine Überprüfung handelt. Fehlerhafte Dateipfade sollten in einer Exception (z.B. FileSystemError) abgefangen werden können. @ownership Die Funktion hat nur Lesezugriff und erzeugt keinen dauerhaften Ownership-Zustand.
- Details: Diese Funktion analysiert die Metadaten oder versucht einen kleinen Test-Read, um zu entscheiden, ob das Format (z.B. WAV, MP3 etc.) mit dem hinterlegten Reader-Typ kompatibel ist. Sie dient als vorgelagerte Prüfung vor einem teuren readFile-Aufruf. filePath Der Pfad zur Audiodatei, die überprüft werden soll. True, wenn der Reader voraussichtlich geeignete Metadaten findet oder das Format unterstützt wird; andernfalls False. Keine spezifischen Ausnahmen erwartet, da es sich um eine reine Überprüfung handelt. Fehlerhafte Dateipfade sollten in einer Exception (z.B. FileSystemError) abgefangen werden können. @ownership Die Funktion hat nur Lesezugriff und erzeugt keinen dauerhaften Ownership-Zustand.

#### `std::map< std::string, std::string > getMetadata(const std::string &) const override`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:110
- Brief: Ruft Metadaten des derzeit verarbeiteten Audio-Chunks ab oder gibt einen Standardwert zurück, falls keine spezifischen Metadaten verfügbar sind.
- Parameters:
  - `path` (const std::string &): n/a
- Return: std::map<std::string, std::string> Ein Kartencontainer, der Schlüssel-Wert-Paare mit den extrahierten Metadaten enthält. Der Schlüssel sollte eine standardisierte Bezeichnung (z. B. "SampleRate", "Format") verwenden. Bei Fehlschlagen des Abrufs wird ein leeres oder notwendiger Standardeintrag zurückgegeben.
- Details: Diese Funktion ist entscheidend für die Validierung der Audioquelle und liefert Informationen wie das Dateiformat, die Bitrate oder die Sample Rate in einem einheitlichen Container. Die Implementierung muss die zugrundeliegenden Spezifika (z.B. WAV-Header-Parsing vs. FFMPEG-Streams) korrekt abstrahieren. std::map<std::string, std::string> Ein Kartencontainer, der Schlüssel-Wert-Paare mit den extrahierten Metadaten enthält. Der Schlüssel sollte eine standardisierte Bezeichnung (z. B. "SampleRate", "Format") verwenden. Bei Fehlschlagen des Abrufs wird ein leeres oder notwendiger Standardeintrag zurückgegeben. Die Rückgabestruktur dient als universelle Schnittstelle, um unterschiedliche Quelleigenschaften konsolidiert darzustellen. @ownership Die übergebenen Daten im Map-Objekt müssen vom Aufrufer sorgfältig auf Gültigkeit geprüft werden. @threading Der Aufruf ist thread-sicher zu gewährleisten; ggf. muss ein externer Mutex zum Schutz der internen Zustandsvariablen verwendet werden, falls das Laden von Metadaten nicht atomar ist.

#### `std::vector< float > readFile(const std::string &, float &out_sr) override`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:100
- Brief: Liest die Chunks der Audiodatei und füllt den Float32-Vektor.
- Parameters:
  - `path` (const std::string &): n/a
  - `out_sample_rate` (float &): n/a
- Return: True, wenn die Datei erfolgreich gelesen und der Puffer korrekt gefüllt wurde; andernfalls False.
- Throws:
  - deklarieren.: Beachten Sie, dass unter Umständen der Dateistream durch einen Fehler mitten im Vorgang geschlossen werden kann.
  - std::runtime_error: Wenn ein kritisches Fehlerereignis beim Lesen (z. B. Dateiphil fehlen oder Codec-Fehler) eintritt. @ownership Die gelesenen Daten im pcmBuffer sind temporär und werden vom Aufrufer des Readers verwaltet. @threading Kann potenziell in multithreaded Umgebungen aufgerufen werden; die Implementierung muss Thread-Safety garantieren.
- Details: Diese Funktion ist der zentrale Mechanismus zur Extraktion von Rohdaten (PCM float32) aus dem angegebene Dateipfad. Sie muss sicherstellen, dass alle zu lesenden Daten erfolgreich in den bereitgestellten Vektor kopiert werden und die Quelle korrekt weiterverarbeitet wird. Die Implementierung sollte fehlerhafte Lesevorgänge abfangen und diese über deklarieren. Beachten Sie, dass unter Umständen der Dateistream durch einen Fehler mitten im Vorgang geschlossen werden kann. filePath Der absolute Pfad zur Audiodatei, die gelesen werden soll. pcmBuffer Ein Pointer auf den Vektor von Float32-Samples, in dem die gelesenen Samples gespeichert werden. Die Größe des Puffers muss ausreichend sein. True, wenn die Datei erfolgreich gelesen und der Puffer korrekt gefüllt wurde; andernfalls False. std::runtime_error Wenn ein kritisches Fehlerereignis beim Lesen (z. B. Dateiphil fehlen oder Codec-Fehler) eintritt. @ownership Die gelesenen Daten im pcmBuffer sind temporär und werden vom Aufrufer des Readers verwaltet. @threading Kann potenziell in multithreaded Umgebungen aufgerufen werden; die Implementierung muss Thread-Safety garantieren.

### StubModelRegistry

#### `bool loadModel(const std::string &path) noexcept`
- Source: `tests/whisper/test_whisper_highcardinality_stress.cpp`:62
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

#### `uint64_t loads() const noexcept`
- Source: `tests/whisper/test_whisper_highcardinality_stress.cpp`:70
- Brief: n/a
- Parameters: none

#### `void unload() noexcept`
- Source: `tests/whisper/test_whisper_highcardinality_stress.cpp`:67
- Brief: n/a
- Parameters: none

#### `uint64_t unloads() const noexcept`
- Source: `tests/whisper/test_whisper_highcardinality_stress.cpp`:71
- Brief: n/a
- Parameters: none

### StubTranscriber

#### `uint64_t totalOps() const noexcept`
- Source: `tests/whisper/test_whisper_highcardinality_stress.cpp`:55
- Brief: n/a
- Parameters: none

#### `TranscriptRecord transcribe(const std::string &audio_id) noexcept`
- Source: `tests/whisper/test_whisper_highcardinality_stress.cpp`:51
- Brief: n/a
- Parameters:
  - `audio_id` (const std::string &): n/a

### WhisperPluginFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### WhisperStubTranscriber

#### `WhisperStubTranscriber()=default`
- Source: `include/whisper/whisper_transcriber.cpp`:148
- Brief: n/a
- Parameters: none

#### `bool deserialize(std::istream &stream) override`
- Source: `include/whisper/whisper_transcriber.cpp`:216
- Brief: n/a
- Parameters:
  - `stream` (std::istream &): n/a

#### `audio::LanguageDetectionResult detectLanguage(const std::vector< float > &pcm, float sample_rate) override`
- Source: `include/whisper/whisper_transcriber.cpp`:171
- Brief: n/a
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a

#### `std::string getModelId() const override`
- Source: `include/whisper/whisper_transcriber.cpp`:195
- Brief: n/a
- Parameters: none

#### `bool initialize(const WhisperConfig &cfg) override`
- Source: `include/whisper/whisper_transcriber.cpp`:151
- Brief: n/a
- Parameters:
  - `cfg` (const WhisperConfig &): n/a

#### `bool isInitialized() const override`
- Source: `include/whisper/whisper_transcriber.cpp`:157
- Brief: n/a
- Parameters: none

#### `bool isModelVersionSupported(const std::string &version_id) const override`
- Source: `include/whisper/whisper_transcriber.cpp`:231
- Brief: n/a
- Parameters:
  - `version_id` (const std::string &): n/a

#### `bool isVersionSupported(const std::string &version_id) const override`
- Source: `include/whisper/whisper_transcriber.cpp`:225
- Brief: n/a
- Parameters:
  - `version_id` (const std::string &): n/a

#### `void loadState(const std::vector< char > &stateData) override`
- Source: `include/whisper/whisper_transcriber.cpp`:206
- Brief: n/a
- Parameters:
  - `stateData` (const std::vector< char > &): n/a

#### `std::vector< char > serialize() const override`
- Source: `include/whisper/whisper_transcriber.cpp`:201
- Brief: n/a
- Parameters: none

#### `bool serialize(std::ostream &stream) const override`
- Source: `include/whisper/whisper_transcriber.cpp`:210
- Brief: n/a
- Parameters:
  - `stream` (std::ostream &): n/a

#### `audio::TranscriptionResult transcribe(const std::vector< float > &pcm, float sample_rate) override`
- Source: `include/whisper/whisper_transcriber.cpp`:160
- Brief: n/a
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a

#### `audio::TranscriptionResult transcribeStream(const std::vector< float > &pcm, float sample_rate, audio::StreamCallback callback)`
- Source: `include/whisper/whisper_transcriber.cpp`:181
- Brief: n/a
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a
  - `callback` (audio::StreamCallback): n/a

#### `~WhisperStubTranscriber() override=default`
- Source: `include/whisper/whisper_transcriber.cpp`:149
- Brief: n/a
- Parameters: none

### bench_whisper_transcription.cpp

#### `Arg(100) -> Arg(500) ->Arg(1000) ->Arg(5000) ->Arg(30000) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `BENCHMARK(BM_WhisperStub_Direct)`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_WhisperStub_Direct): n/a

#### `BENCHMARK_F(WhisperPluginFixture, DetectLanguage)(benchmark`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFixture): n/a
  - `<unnamed>` (DetectLanguage): n/a

#### `BENCHMARK_F(WhisperPluginFixture, StatsQuery)(benchmark`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFixture): n/a
  - `<unnamed>` (StatsQuery): n/a

#### `BENCHMARK_F(WhisperPluginFixture, TranscribeFile_NonExistent)(benchmark`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFixture): n/a
  - `<unnamed>` (TranscribeFile_NonExistent): n/a

#### `BENCHMARK_F(WhisperPluginFixture, Transcribe_1s)(benchmark`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFixture): n/a
  - `<unnamed>` (Transcribe_1s): n/a

#### `BENCHMARK_F(WhisperPluginFixture, Transcribe_30s_CLIParity)(benchmark`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFixture): n/a
  - `<unnamed>` (Transcribe_30s_CLIParity): n/a

#### `BENCHMARK_F(WhisperPluginFixture, Transcribe_5s)(benchmark`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFixture): n/a
  - `<unnamed>` (Transcribe_5s): n/a

#### `BENCHMARK_F(WhisperPluginFixture, Transcribe_8kHz_1s)(benchmark`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFixture): n/a
  - `<unnamed>` (Transcribe_8kHz_1s): n/a

#### `void BM_Transcribe_BufferSize(benchmark::State &state)`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:223
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_WhisperStub_Direct(benchmark::State &state)`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:204
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `std::vector< float > makePCM(int duration_ms, float sample_rate=16000.0f)`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:67
- Brief: Generate synthetic PCM float samples (silence + low-energy noise).
- Parameters:
  - `duration_ms` (int): n/a
  - `sample_rate` (float): n/a

#### `std::string resolveCompileTimeWhisperModelPath()`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:42
- Brief: n/a
- Parameters: none

#### `std::string resolveWhisperModelPath()`
- Source: `benchmarks/whisper/bench_whisper_transcription.cpp`:58
- Brief: n/a
- Parameters: none

### test_whisper_highcardinality_stress.cpp

#### `TEST(WhisperHighCardinalityStress, WSTR01_HighCardinalityAudioBatch)`
- Source: `tests/whisper/test_whisper_highcardinality_stress.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperHighCardinalityStress): n/a
  - `<unnamed>` (WSTR01_HighCardinalityAudioBatch): n/a

#### `TEST(WhisperHighCardinalityStress, WSTR02_ConcurrentTranscriptionStress)`
- Source: `tests/whisper/test_whisper_highcardinality_stress.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperHighCardinalityStress): n/a
  - `<unnamed>` (WSTR02_ConcurrentTranscriptionStress): n/a

#### `TEST(WhisperHighCardinalityStress, WSTR03_ModelReloadStress)`
- Source: `tests/whisper/test_whisper_highcardinality_stress.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperHighCardinalityStress): n/a
  - `<unnamed>` (WSTR03_ModelReloadStress): n/a

### test_whisper_plugin.cpp

#### `TEST(WhisperPluginFocusedTests, A1_FromJsonEmptyUsesDefaults)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (A1_FromJsonEmptyUsesDefaults): n/a

#### `TEST(WhisperPluginFocusedTests, A2_FromJsonCustomValues)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (A2_FromJsonCustomValues): n/a

#### `TEST(WhisperPluginFocusedTests, A3_FromJsonClampsZeroThreads)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (A3_FromJsonClampsZeroThreads): n/a

#### `TEST(WhisperPluginFocusedTests, B1_ToJsonRoundTrip)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (B1_ToJsonRoundTrip): n/a

#### `TEST(WhisperPluginFocusedTests, B2_ToJsonContainsAllKeys)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (B2_ToJsonContainsAllKeys): n/a

#### `TEST(WhisperPluginFocusedTests, B3_QualityThresholdRoundTrip)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (B3_QualityThresholdRoundTrip): n/a

#### `TEST(WhisperPluginFocusedTests, C1_CanReadAcceptsWav)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (C1_CanReadAcceptsWav): n/a

#### `TEST(WhisperPluginFocusedTests, C2_CanReadRejectsMp3AndFlac)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (C2_CanReadRejectsMp3AndFlac): n/a

#### `TEST(WhisperPluginFocusedTests, C3_ReadFileThrowsOnNonWavData)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (C3_ReadFileThrowsOnNonWavData): n/a

#### `TEST(WhisperPluginFocusedTests, D1_InitializeReturnsTrue)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (D1_InitializeReturnsTrue): n/a

#### `TEST(WhisperPluginFocusedTests, D2_TranscribeReturnsPresetText)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (D2_TranscribeReturnsPresetText): n/a

#### `TEST(WhisperPluginFocusedTests, D3_DetectLanguageReturnsPreset)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:222
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (D3_DetectLanguageReturnsPreset): n/a

#### `TEST(WhisperPluginFocusedTests, E1_InitializeViaDI)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (E1_InitializeViaDI): n/a

#### `TEST(WhisperPluginFocusedTests, E2_TranscribeAfterInit)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:243
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (E2_TranscribeAfterInit): n/a

#### `TEST(WhisperPluginFocusedTests, E2b_DefaultCtorUsesInjectedStubFactoryWhenWhisperDisabled)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (E2b_DefaultCtorUsesInjectedStubFactoryWhenWhisperDisabled): n/a

#### `TEST(WhisperPluginFocusedTests, E3_DetectLanguageAfterInit)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (E3_DetectLanguageAfterInit): n/a

#### `TEST(WhisperPluginFocusedTests, F1_TranscribeFileDelegatesToReader)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (F1_TranscribeFileDelegatesToReader): n/a

#### `TEST(WhisperPluginFocusedTests, F2_TranscribeFileSuccessCountsTranscription)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (F2_TranscribeFileSuccessCountsTranscription): n/a

#### `TEST(WhisperPluginFocusedTests, F3_TranscribeFileReaderThrowsReturnsError)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:309
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (F3_TranscribeFileReaderThrowsReturnsError): n/a

#### `TEST(WhisperPluginFocusedTests, G1_IngestionSourceTypeAlwaysWHISPER)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:322
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (G1_IngestionSourceTypeAlwaysWHISPER): n/a

#### `TEST(WhisperPluginFocusedTests, G2_PluginVersionAlways2_0_0)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (G2_PluginVersionAlways2_0_0): n/a

#### `TEST(WhisperPluginFocusedTests, G3_GenerationTimestampPositive)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (G3_GenerationTimestampPositive): n/a

#### `TEST(WhisperPluginFocusedTests, H1_StatisticsContainsRequiredKeys)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:350
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (H1_StatisticsContainsRequiredKeys): n/a

#### `TEST(WhisperPluginFocusedTests, H2_StatisticsPluginName)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:360
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (H2_StatisticsPluginName): n/a

#### `TEST(WhisperPluginFocusedTests, H3_StatisticsVersionIs2_0_0)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:366
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (H3_StatisticsVersionIs2_0_0): n/a

#### `TEST(WhisperPluginFocusedTests, I1_TranscribeUninitializedReturnsError)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (I1_TranscribeUninitializedReturnsError): n/a

#### `TEST(WhisperPluginFocusedTests, I2_TranscribeFileUninitializedReturnsError)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:385
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (I2_TranscribeFileUninitializedReturnsError): n/a

#### `TEST(WhisperPluginFocusedTests, I3_TranscribeEmptyPcmDoesNotCrash)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (I3_TranscribeEmptyPcmDoesNotCrash): n/a

#### `TEST(WhisperPluginFocusedTests, J1_DoubleInitIsSafe)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:405
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (J1_DoubleInitIsSafe): n/a

#### `TEST(WhisperPluginFocusedTests, J2_GetModelIdMatchesTranscriber)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:413
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (J2_GetModelIdMatchesTranscriber): n/a

#### `TEST(WhisperPluginFocusedTests, J3_ErrorCountIncrements)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:420
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (J3_ErrorCountIncrements): n/a

#### `TEST(WhisperPluginFocusedTests, K1_ConcurrentTranscribeDoesNotCrash)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:433
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (K1_ConcurrentTranscribeDoesNotCrash): n/a

#### `TEST(WhisperPluginFocusedTests, K2_AtomicCountersUnderConcurrentErrors)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:467
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (K2_AtomicCountersUnderConcurrentErrors): n/a

#### `TEST(WhisperPluginFocusedTests, K3_ConcurrentDetectLanguageDoesNotCrash)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:499
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (K3_ConcurrentDetectLanguageDoesNotCrash): n/a

#### `TEST(WhisperPluginFocusedTests, L1_FfmpegCanReadMp3OggFlac)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:532
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (L1_FfmpegCanReadMp3OggFlac): n/a

#### `TEST(WhisperPluginFocusedTests, L2_FfmpegThrowsOnMissingFileOrNoFfmpeg)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:543
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (L2_FfmpegThrowsOnMissingFileOrNoFfmpeg): n/a

#### `TEST(WhisperPluginFocusedTests, L3_CompositeRoutesByExtension)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:550
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (L3_CompositeRoutesByExtension): n/a

#### `TEST(WhisperPluginFocusedTests, M1_LanguageConfidenceThresholdDefaultZeroDisabled)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:572
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (M1_LanguageConfidenceThresholdDefaultZeroDisabled): n/a

#### `TEST(WhisperPluginFocusedTests, M2_LanguageDetectionPassesWhenAboveThreshold)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:588
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (M2_LanguageDetectionPassesWhenAboveThreshold): n/a

#### `TEST(WhisperPluginFocusedTests, M3_LanguageDetectionUnknownWhenBelowThreshold)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:601
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (M3_LanguageDetectionUnknownWhenBelowThreshold): n/a

#### `TEST(WhisperPluginFocusedTests, N1_BeamSizeClampedToOne)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:618
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (N1_BeamSizeClampedToOne): n/a

#### `TEST(WhisperPluginFocusedTests, N2_LanguageConfidenceThresholdRoundTrip)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:623
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (N2_LanguageConfidenceThresholdRoundTrip): n/a

#### `TEST(WhisperPluginFocusedTests, N3_LanguageConfidenceThresholdClamped)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:630
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (N3_LanguageConfidenceThresholdClamped): n/a

#### `TEST(WhisperPluginFocusedTests, N4_WavReaderParsesStereo16BitPcm)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:638
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (N4_WavReaderParsesStereo16BitPcm): n/a

#### `TEST(WhisperPluginFocusedTests, N5_ToJsonContainsLanguageConfidenceThresholdKey)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:670
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (N5_ToJsonContainsLanguageConfidenceThresholdKey): n/a

#### `TEST(WhisperPluginFocusedTests, O1_StreamSingleTokenFallback)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:681
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (O1_StreamSingleTokenFallback): n/a

#### `TEST(WhisperPluginFocusedTests, O2_StreamMultipleTokens)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:706
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (O2_StreamMultipleTokens): n/a

#### `TEST(WhisperPluginFocusedTests, O3_StreamCallbackExceptionYieldsFailure)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:738
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (O3_StreamCallbackExceptionYieldsFailure): n/a

#### `TEST(WhisperPluginFocusedTests, O4_StreamUninitGuard)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:757
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (O4_StreamUninitGuard): n/a

#### `TEST(WhisperPluginFocusedTests, O5_StreamProvenanceAlwaysSet)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:766
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (O5_StreamProvenanceAlwaysSet): n/a

#### `TEST(WhisperPluginFocusedTests, P1_VadAllSilenceYieldsNoSegments)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:788
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (P1_VadAllSilenceYieldsNoSegments): n/a

#### `TEST(WhisperPluginFocusedTests, P2_VadAllSpeechYieldsOneSegment)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:800
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (P2_VadAllSpeechYieldsOneSegment): n/a

#### `TEST(WhisperPluginFocusedTests, P3_VadMixedYieldsSpeechSegments)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:814
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (P3_VadMixedYieldsSpeechSegments): n/a

#### `TEST(WhisperPluginFocusedTests, Q1_VadSkipsSilentInput)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:839
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (Q1_VadSkipsSilentInput): n/a

#### `TEST(WhisperPluginFocusedTests, Q2_VadPassesSpeechThrough)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:863
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (Q2_VadPassesSpeechThrough): n/a

#### `TEST(WhisperPluginFocusedTests, Q3_NullVadIsNoOp)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:886
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (Q3_NullVadIsNoOp): n/a

#### `TEST(WhisperPluginFocusedTests, R1_ConcurrentVadSetAndTranscribeStream)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:909
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (R1_ConcurrentVadSetAndTranscribeStream): n/a

#### `TEST(WhisperPluginFocusedTests, R2_SetVadAfterTranscribeIsSafe)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:959
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (R2_SetVadAfterTranscribeIsSafe): n/a

#### `TEST(WhisperPluginFocusedTests, S1_ParseWavRejectsZeroChannels)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:1027
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (S1_ParseWavRejectsZeroChannels): n/a

#### `TEST(WhisperPluginFocusedTests, S2_ParseWavRejectsExcessiveChannelCount)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:1038
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (S2_ParseWavRejectsExcessiveChannelCount): n/a

#### `TEST(WhisperPluginFocusedTests, S3_ParseWavAcceptsMaxValidChannelCount)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:1049
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (S3_ParseWavAcceptsMaxValidChannelCount): n/a

#### `TEST(WhisperPluginFocusedTests, T1_DiarisationFixtureResultFromTranscriber)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:1079
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (T1_DiarisationFixtureResultFromTranscriber): n/a

#### `TEST(WhisperPluginFocusedTests, T2_DiarisationEmptyConfigIsAccepted)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:1094
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (T2_DiarisationEmptyConfigIsAccepted): n/a

#### `TEST(WhisperPluginFocusedTests, T3_DiarisationMissingModelFallbackKeepsTranscriptionWorking)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:1103
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (T3_DiarisationMissingModelFallbackKeepsTranscriptionWorking): n/a

#### `TEST(WhisperPluginFocusedTests, T4_DiarisationSpeakerCountClamp)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:1126
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (T4_DiarisationSpeakerCountClamp): n/a

#### `TEST(WhisperPluginFocusedTests, T5_DiarisationResultHasProvenanceStamp)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:1141
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (T5_DiarisationResultHasProvenanceStamp): n/a

#### `TEST(WhisperPluginFocusedTests, U1_ModelSha256RoundTripInConfig)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:1161
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (U1_ModelSha256RoundTripInConfig): n/a

#### `TEST(WhisperPluginFocusedTests, U2_EmptyModelSha256AcceptedInStubMode)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:1169
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginFocusedTests): n/a
  - `<unnamed>` (U2_EmptyModelSha256AcceptedInStubMode): n/a

#### `bool joinThreadWithTimeout(std::thread &th, std::atomic< bool > &done_flag, std::chrono::milliseconds timeout=std::chrono::seconds(5))`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:46
- Brief: n/a
- Parameters:
  - `th` (std::thread &): n/a
  - `done_flag` (std::atomic< bool > &): n/a
  - `timeout` (std::chrono::milliseconds): n/a

#### `std::vector< uint8_t > minimalWav()`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:70
- Brief: Minimal RIFF/WAV (16-bit PCM mono 16kHz, 4 zero samples).
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: b().

#### `std::string writeTmpFile(const std::string &name, const std::vector< uint8_t > &bytes)`
- Source: `src/whisper/tests/test_whisper_plugin.cpp`:37
- Brief: ── helpers ───────────────────────────────────────────────────────────────────
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `bytes` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: name Input parameter. bytes Input parameter. Return value. Calls: f(), write(), data(), size().

### test_whisper_plugin_registrar.cpp

#### `TEST(WhisperPluginRegistrarTests, A1_CreatePluginStubMode)`
- Source: `src/whisper/tests/test_whisper_plugin_registrar.cpp`:24
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginRegistrarTests): n/a
  - `<unnamed>` (A1_CreatePluginStubMode): n/a

#### `TEST(WhisperPluginRegistrarTests, A2_CreatePluginEmptyModelPath)`
- Source: `src/whisper/tests/test_whisper_plugin_registrar.cpp`:31
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginRegistrarTests): n/a
  - `<unnamed>` (A2_CreatePluginEmptyModelPath): n/a

#### `TEST(WhisperPluginRegistrarTests, A3_CreatePluginWithModelPath)`
- Source: `src/whisper/tests/test_whisper_plugin_registrar.cpp`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginRegistrarTests): n/a
  - `<unnamed>` (A3_CreatePluginWithModelPath): n/a

#### `TEST(WhisperPluginRegistrarTests, B1_CreateAdapterNotNull)`
- Source: `src/whisper/tests/test_whisper_plugin_registrar.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginRegistrarTests): n/a
  - `<unnamed>` (B1_CreateAdapterNotNull): n/a

#### `TEST(WhisperPluginRegistrarTests, B2_AdapterTypeIsAudioProcessing)`
- Source: `src/whisper/tests/test_whisper_plugin_registrar.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginRegistrarTests): n/a
  - `<unnamed>` (B2_AdapterTypeIsAudioProcessing): n/a

#### `TEST(WhisperPluginRegistrarTests, B3_AdapterGetInstanceReturnsWhisperPlugin)`
- Source: `src/whisper/tests/test_whisper_plugin_registrar.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginRegistrarTests): n/a
  - `<unnamed>` (B3_AdapterGetInstanceReturnsWhisperPlugin): n/a

#### `TEST(WhisperPluginRegistrarTests, C1_AdapterNameAndVersion)`
- Source: `src/whisper/tests/test_whisper_plugin_registrar.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginRegistrarTests): n/a
  - `<unnamed>` (C1_AdapterNameAndVersion): n/a

#### `TEST(WhisperPluginRegistrarTests, C2_AdapterCapabilitiesThreadSafe)`
- Source: `src/whisper/tests/test_whisper_plugin_registrar.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginRegistrarTests): n/a
  - `<unnamed>` (C2_AdapterCapabilitiesThreadSafe): n/a

#### `TEST(WhisperPluginRegistrarTests, C3_AdapterInitializeAndShutdownCycle)`
- Source: `src/whisper/tests/test_whisper_plugin_registrar.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginRegistrarTests): n/a
  - `<unnamed>` (C3_AdapterInitializeAndShutdownCycle): n/a

#### `TEST(WhisperPluginRegistrarTests, D1_DefaultReloadCallbackStubMode)`
- Source: `src/whisper/tests/test_whisper_plugin_registrar.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginRegistrarTests): n/a
  - `<unnamed>` (D1_DefaultReloadCallbackStubMode): n/a

#### `TEST(WhisperPluginRegistrarTests, D2_DefaultReloadCallbackWithPath)`
- Source: `src/whisper/tests/test_whisper_plugin_registrar.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginRegistrarTests): n/a
  - `<unnamed>` (D2_DefaultReloadCallbackWithPath): n/a

#### `TEST(WhisperPluginRegistrarTests, D3_DefaultReloadCallbackEmptyPath)`
- Source: `src/whisper/tests/test_whisper_plugin_registrar.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperPluginRegistrarTests): n/a
  - `<unnamed>` (D3_DefaultReloadCallbackEmptyPath): n/a

### test_whisper_stub_transcribe_bridge.cpp

#### `TEST(WhisperStubTranscribeBridge, WST_01_InjectedFnCalledInsteadOfEmpty)`
- Source: `tests/whisper/test_whisper_stub_transcribe_bridge.cpp`:29
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperStubTranscribeBridge): n/a
  - `<unnamed>` (WST_01_InjectedFnCalledInsteadOfEmpty): n/a

#### `TEST(WhisperStubTranscribeBridge, WST_02_NullFnRestoresEmptyResult)`
- Source: `tests/whisper/test_whisper_stub_transcribe_bridge.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperStubTranscribeBridge): n/a
  - `<unnamed>` (WST_02_NullFnRestoresEmptyResult): n/a

#### `TEST(WhisperStubTranscribeBridge, WST_03_ResultPropagatedThroughPlugin)`
- Source: `tests/whisper/test_whisper_stub_transcribe_bridge.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (WhisperStubTranscribeBridge): n/a
  - `<unnamed>` (WST_03_ResultPropagatedThroughPlugin): n/a

### themis::whisper

#### `uint16_t readU16LE(const uint8_t *p)`
- Source: `src/whisper/audio_chunk_reader.cpp`:50
- Brief: Read U16 LE.
- Parameters:
  - `p` (const uint8_t *): Input parameter.
- Return: Return value.
- Details: p Input parameter. Return value. Implements readU16LE without additional internal calls.

#### `uint32_t readU32LE(const uint8_t *p)`
- Source: `src/whisper/audio_chunk_reader.cpp`:60
- Brief: Read U32 LE.
- Parameters:
  - `p` (const uint8_t *): Input parameter.
- Return: Return value.
- Details: p Input parameter. Return value. Implements readU32LE without additional internal calls.

#### `std::string toLower(std::string s)`
- Source: `src/whisper/audio_chunk_reader.cpp`:38
- Brief: ── helpers ─────────────────────────────────────────────────────────────────
- Parameters:
  - `s` (std::string): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: std::transform(), begin(), end(), std::tolower().

### themis::whisper::CompositeAudioChunkReader

#### `void addReader(std::unique_ptr< IAudioChunkReader > reader)`
- Source: `include/whisper/audio_chunk_reader.h`:152
- Brief: Register a reader. Readers are tried in the order they are added.
- Parameters:
  - `reader` (std::unique_ptr< IAudioChunkReader >): Input parameter.
- Details: ── CompositeAudioChunkReader ──────────────────────────────────────────────── reader Input parameter. Calls: push_back(), std::move().

#### `bool canRead(const std::string &path) const override`
- Source: `include/whisper/audio_chunk_reader.h`:157
- Brief: Prüft, ob der Reader mit den Inhalten des angegebenen Dateipfades lesen kann.
- Parameters:
  - `path` (const std::string &): n/a
- Return: True, wenn der Reader voraussichtlich geeignete Metadaten findet oder das Format unterstützt wird; andernfalls False.
- Throws:
  - Keine: spezifischen Ausnahmen erwartet, da es sich um eine reine Überprüfung handelt. Fehlerhafte Dateipfade sollten in einer Exception (z.B. FileSystemError) abgefangen werden können. @ownership Die Funktion hat nur Lesezugriff und erzeugt keinen dauerhaften Ownership-Zustand.
- Details: Diese Funktion analysiert die Metadaten oder versucht einen kleinen Test-Read, um zu entscheiden, ob das Format (z.B. WAV, MP3 etc.) mit dem hinterlegten Reader-Typ kompatibel ist. Sie dient als vorgelagerte Prüfung vor einem teuren readFile-Aufruf. filePath Der Pfad zur Audiodatei, die überprüft werden soll. True, wenn der Reader voraussichtlich geeignete Metadaten findet oder das Format unterstützt wird; andernfalls False. Keine spezifischen Ausnahmen erwartet, da es sich um eine reine Überprüfung handelt. Fehlerhafte Dateipfade sollten in einer Exception (z.B. FileSystemError) abgefangen werden können. @ownership Die Funktion hat nur Lesezugriff und erzeugt keinen dauerhaften Ownership-Zustand.

#### `std::map< std::string, std::string > getMetadata(const std::string &path) const override`
- Source: `include/whisper/audio_chunk_reader.h`:159
- Brief: Ruft Metadaten des derzeit verarbeiteten Audio-Chunks ab oder gibt einen Standardwert zurück, falls keine spezifischen Metadaten verfügbar sind.
- Parameters:
  - `path` (const std::string &): n/a
- Return: std::map<std::string, std::string> Ein Kartencontainer, der Schlüssel-Wert-Paare mit den extrahierten Metadaten enthält. Der Schlüssel sollte eine standardisierte Bezeichnung (z. B. "SampleRate", "Format") verwenden. Bei Fehlschlagen des Abrufs wird ein leeres oder notwendiger Standardeintrag zurückgegeben.
- Details: Diese Funktion ist entscheidend für die Validierung der Audioquelle und liefert Informationen wie das Dateiformat, die Bitrate oder die Sample Rate in einem einheitlichen Container. Die Implementierung muss die zugrundeliegenden Spezifika (z.B. WAV-Header-Parsing vs. FFMPEG-Streams) korrekt abstrahieren. std::map<std::string, std::string> Ein Kartencontainer, der Schlüssel-Wert-Paare mit den extrahierten Metadaten enthält. Der Schlüssel sollte eine standardisierte Bezeichnung (z. B. "SampleRate", "Format") verwenden. Bei Fehlschlagen des Abrufs wird ein leeres oder notwendiger Standardeintrag zurückgegeben. Die Rückgabestruktur dient als universelle Schnittstelle, um unterschiedliche Quelleigenschaften konsolidiert darzustellen. @ownership Die übergebenen Daten im Map-Objekt müssen vom Aufrufer sorgfältig auf Gültigkeit geprüft werden. @threading Der Aufruf ist thread-sicher zu gewährleisten; ggf. muss ein externer Mutex zum Schutz der internen Zustandsvariablen verwendet werden, falls das Laden von Metadaten nicht atomar ist.

#### `std::vector< float > readFile(const std::string &path, float &out_sample_rate) override`
- Source: `include/whisper/audio_chunk_reader.h`:154
- Brief: Read File.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `out_sample_rate` (float &): Input/output parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: path Input parameter. out_sample_rate Input/output parameter. Return value. std::runtime_error if an error occurs. Calls: canRead().

### themis::whisper::EnergyThresholdVad

#### `std::vector< SpeechSegment > detect(const std::vector< float > &pcm, float sample_rate, const VadConfig &cfg) const override`
- Source: `include/whisper/voice_activity_detector.h`:106
- Brief: Detect speech segments in a PCM buffer.
- Parameters:
  - `pcm` (const std::vector< float > &): Mono float32 samples, normalised to [-1, 1].
  - `sample_rate` (float): Sampling rate of pcm (e.g. 16000.0f).
  - `cfg` (const VadConfig &): VAD parameters.
- Return: List of speech segments ordered by start_sample.
- Details: pcm Mono float32 samples, normalised to [-1, 1]. sample_rate Sampling rate of pcm (e.g. 16000.0f). cfg VAD parameters. List of speech segments ordered by start_sample.

#### `float frameRms(const std::vector< float > &pcm, std::size_t start, std::size_t end) noexcept`
- Source: `include/whisper/voice_activity_detector.h`:143
- Brief: n/a
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `start` (std::size_t): n/a
  - `end` (std::size_t): n/a

### themis::whisper::FfmpegAudioChunkReader

#### `bool canRead(const std::string &path) const override`
- Source: `include/whisper/audio_chunk_reader.h`:132
- Brief: Prüft, ob der Reader mit den Inhalten des angegebenen Dateipfades lesen kann.
- Parameters:
  - `path` (const std::string &): n/a
- Return: True, wenn der Reader voraussichtlich geeignete Metadaten findet oder das Format unterstützt wird; andernfalls False.
- Throws:
  - Keine: spezifischen Ausnahmen erwartet, da es sich um eine reine Überprüfung handelt. Fehlerhafte Dateipfade sollten in einer Exception (z.B. FileSystemError) abgefangen werden können. @ownership Die Funktion hat nur Lesezugriff und erzeugt keinen dauerhaften Ownership-Zustand.
- Details: Diese Funktion analysiert die Metadaten oder versucht einen kleinen Test-Read, um zu entscheiden, ob das Format (z.B. WAV, MP3 etc.) mit dem hinterlegten Reader-Typ kompatibel ist. Sie dient als vorgelagerte Prüfung vor einem teuren readFile-Aufruf. filePath Der Pfad zur Audiodatei, die überprüft werden soll. True, wenn der Reader voraussichtlich geeignete Metadaten findet oder das Format unterstützt wird; andernfalls False. Keine spezifischen Ausnahmen erwartet, da es sich um eine reine Überprüfung handelt. Fehlerhafte Dateipfade sollten in einer Exception (z.B. FileSystemError) abgefangen werden können. @ownership Die Funktion hat nur Lesezugriff und erzeugt keinen dauerhaften Ownership-Zustand.

#### `std::map< std::string, std::string > getMetadata(const std::string &path) const override`
- Source: `include/whisper/audio_chunk_reader.h`:134
- Brief: Ruft Metadaten des derzeit verarbeiteten Audio-Chunks ab oder gibt einen Standardwert zurück, falls keine spezifischen Metadaten verfügbar sind.
- Parameters:
  - `path` (const std::string &): n/a
- Return: std::map<std::string, std::string> Ein Kartencontainer, der Schlüssel-Wert-Paare mit den extrahierten Metadaten enthält. Der Schlüssel sollte eine standardisierte Bezeichnung (z. B. "SampleRate", "Format") verwenden. Bei Fehlschlagen des Abrufs wird ein leeres oder notwendiger Standardeintrag zurückgegeben.
- Details: Diese Funktion ist entscheidend für die Validierung der Audioquelle und liefert Informationen wie das Dateiformat, die Bitrate oder die Sample Rate in einem einheitlichen Container. Die Implementierung muss die zugrundeliegenden Spezifika (z.B. WAV-Header-Parsing vs. FFMPEG-Streams) korrekt abstrahieren. std::map<std::string, std::string> Ein Kartencontainer, der Schlüssel-Wert-Paare mit den extrahierten Metadaten enthält. Der Schlüssel sollte eine standardisierte Bezeichnung (z. B. "SampleRate", "Format") verwenden. Bei Fehlschlagen des Abrufs wird ein leeres oder notwendiger Standardeintrag zurückgegeben. Die Rückgabestruktur dient als universelle Schnittstelle, um unterschiedliche Quelleigenschaften konsolidiert darzustellen. @ownership Die übergebenen Daten im Map-Objekt müssen vom Aufrufer sorgfältig auf Gültigkeit geprüft werden. @threading Der Aufruf ist thread-sicher zu gewährleisten; ggf. muss ein externer Mutex zum Schutz der internen Zustandsvariablen verwendet werden, falls das Laden von Metadaten nicht atomar ist.

#### `std::vector< float > readFile(const std::string &path, float &out_sample_rate) override`
- Source: `include/whisper/audio_chunk_reader.h`:129
- Brief: Read File.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `out_sample_rate` (float &): Input/output parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: path Input parameter. out_sample_rate Input/output parameter. Return value. std::runtime_error if an error occurs. Calls: shellEscape(), THEMIS_PCLOSE(), int(), probe(), THEMIS_POPEN(), std::fread(), get(), operator().

#### `std::string shellEscape(const std::string &path)`
- Source: `include/whisper/audio_chunk_reader.h`:139
- Brief: Shell Escape.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Shell-escape a path for use inside single quotes. Throws std::runtime_error if the path contains a NUL byte. path Input parameter. Return value. std::runtime_error if an error occurs. Calls: find(), str().

### themis::whisper::IAudioChunkReader

#### `bool canRead(const std::string &path) const =0`
- Source: `include/whisper/audio_chunk_reader.h`:61
- Brief: Prüft, ob der Reader mit den Inhalten des angegebenen Dateipfades lesen kann.
- Parameters:
  - `path` (const std::string &): n/a
- Return: True, wenn der Reader voraussichtlich geeignete Metadaten findet oder das Format unterstützt wird; andernfalls False.
- Throws:
  - Keine: spezifischen Ausnahmen erwartet, da es sich um eine reine Überprüfung handelt. Fehlerhafte Dateipfade sollten in einer Exception (z.B. FileSystemError) abgefangen werden können. @ownership Die Funktion hat nur Lesezugriff und erzeugt keinen dauerhaften Ownership-Zustand.
- Details: Diese Funktion analysiert die Metadaten oder versucht einen kleinen Test-Read, um zu entscheiden, ob das Format (z.B. WAV, MP3 etc.) mit dem hinterlegten Reader-Typ kompatibel ist. Sie dient als vorgelagerte Prüfung vor einem teuren readFile-Aufruf. filePath Der Pfad zur Audiodatei, die überprüft werden soll. True, wenn der Reader voraussichtlich geeignete Metadaten findet oder das Format unterstützt wird; andernfalls False. Keine spezifischen Ausnahmen erwartet, da es sich um eine reine Überprüfung handelt. Fehlerhafte Dateipfade sollten in einer Exception (z.B. FileSystemError) abgefangen werden können. @ownership Die Funktion hat nur Lesezugriff und erzeugt keinen dauerhaften Ownership-Zustand.

#### `std::map< std::string, std::string > getMetadata(const std::string &path) const =0`
- Source: `include/whisper/audio_chunk_reader.h`:74
- Brief: Ruft Metadaten des derzeit verarbeiteten Audio-Chunks ab oder gibt einen Standardwert zurück, falls keine spezifischen Metadaten verfügbar sind.
- Parameters:
  - `path` (const std::string &): n/a
- Return: std::map<std::string, std::string> Ein Kartencontainer, der Schlüssel-Wert-Paare mit den extrahierten Metadaten enthält. Der Schlüssel sollte eine standardisierte Bezeichnung (z. B. "SampleRate", "Format") verwenden. Bei Fehlschlagen des Abrufs wird ein leeres oder notwendiger Standardeintrag zurückgegeben.
- Details: Diese Funktion ist entscheidend für die Validierung der Audioquelle und liefert Informationen wie das Dateiformat, die Bitrate oder die Sample Rate in einem einheitlichen Container. Die Implementierung muss die zugrundeliegenden Spezifika (z.B. WAV-Header-Parsing vs. FFMPEG-Streams) korrekt abstrahieren. std::map<std::string, std::string> Ein Kartencontainer, der Schlüssel-Wert-Paare mit den extrahierten Metadaten enthält. Der Schlüssel sollte eine standardisierte Bezeichnung (z. B. "SampleRate", "Format") verwenden. Bei Fehlschlagen des Abrufs wird ein leeres oder notwendiger Standardeintrag zurückgegeben. Die Rückgabestruktur dient als universelle Schnittstelle, um unterschiedliche Quelleigenschaften konsolidiert darzustellen. @ownership Die übergebenen Daten im Map-Objekt müssen vom Aufrufer sorgfältig auf Gültigkeit geprüft werden. @threading Der Aufruf ist thread-sicher zu gewährleisten; ggf. muss ein externer Mutex zum Schutz der internen Zustandsvariablen verwendet werden, falls das Laden von Metadaten nicht atomar ist.

#### `std::vector< float > readFile(const std::string &path, float &out_sample_rate)=0`
- Source: `include/whisper/audio_chunk_reader.h`:46
- Brief: Liest die Chunks der Audiodatei und füllt den Float32-Vektor.
- Parameters:
  - `path` (const std::string &): n/a
  - `out_sample_rate` (float &): n/a
- Return: True, wenn die Datei erfolgreich gelesen und der Puffer korrekt gefüllt wurde; andernfalls False.
- Throws:
  - deklarieren.: Beachten Sie, dass unter Umständen der Dateistream durch einen Fehler mitten im Vorgang geschlossen werden kann.
  - std::runtime_error: Wenn ein kritisches Fehlerereignis beim Lesen (z. B. Dateiphil fehlen oder Codec-Fehler) eintritt. @ownership Die gelesenen Daten im pcmBuffer sind temporär und werden vom Aufrufer des Readers verwaltet. @threading Kann potenziell in multithreaded Umgebungen aufgerufen werden; die Implementierung muss Thread-Safety garantieren.
- Details: Diese Funktion ist der zentrale Mechanismus zur Extraktion von Rohdaten (PCM float32) aus dem angegebene Dateipfad. Sie muss sicherstellen, dass alle zu lesenden Daten erfolgreich in den bereitgestellten Vektor kopiert werden und die Quelle korrekt weiterverarbeitet wird. Die Implementierung sollte fehlerhafte Lesevorgänge abfangen und diese über deklarieren. Beachten Sie, dass unter Umständen der Dateistream durch einen Fehler mitten im Vorgang geschlossen werden kann. filePath Der absolute Pfad zur Audiodatei, die gelesen werden soll. pcmBuffer Ein Pointer auf den Vektor von Float32-Samples, in dem die gelesenen Samples gespeichert werden. Die Größe des Puffers muss ausreichend sein. True, wenn die Datei erfolgreich gelesen und der Puffer korrekt gefüllt wurde; andernfalls False. std::runtime_error Wenn ein kritisches Fehlerereignis beim Lesen (z. B. Dateiphil fehlen oder Codec-Fehler) eintritt. @ownership Die gelesenen Daten im pcmBuffer sind temporär und werden vom Aufrufer des Readers verwaltet. @threading Kann potenziell in multithreaded Umgebungen aufgerufen werden; die Implementierung muss Thread-Safety garantieren.

#### `~IAudioChunkReader()=default`
- Source: `include/whisper/audio_chunk_reader.h`:27
- Brief: n/a
- Parameters: none

### themis::whisper::IVoiceActivityDetector

#### `std::vector< SpeechSegment > detect(const std::vector< float > &pcm, float sample_rate, const VadConfig &cfg) const =0`
- Source: `include/whisper/voice_activity_detector.h`:81
- Brief: Detect speech segments in a PCM buffer.
- Parameters:
  - `pcm` (const std::vector< float > &): Mono float32 samples, normalised to [-1, 1].
  - `sample_rate` (float): Sampling rate of pcm (e.g. 16000.0f).
  - `cfg` (const VadConfig &): VAD parameters.
- Return: List of speech segments ordered by start_sample.
- Details: pcm Mono float32 samples, normalised to [-1, 1]. sample_rate Sampling rate of pcm (e.g. 16000.0f). cfg VAD parameters. List of speech segments ordered by start_sample.

#### `~IVoiceActivityDetector()=default`
- Source: `include/whisper/voice_activity_detector.h`:70
- Brief: n/a
- Parameters: none

### themis::whisper::IWhisperTranscriber

#### `bool deserialize(std::istream &stream)=0`
- Source: `include/whisper/whisper_transcriber.h`:201
- Brief: Deserializes the transcriber object's state from an input stream.
- Parameters:
  - `stream` (std::istream &): A constant reference to an input stream (std::istream&). The deserialization logic must read and reconstruct all critical state information from this provided stream.
- Return: bool Returns true if the state was successfully reconstructed from the stream; otherwise, it returns false, indicating that the stream data was corrupted or incomplete.
- Details: This method is responsible for reading and restoring all necessary internal parameters, model configurations (e.g., whisper version ID), and any transient state data previously saved by serialize(). It must ensure the object's methods are updated to reflect the loaded state. stream A constant reference to an input stream (std::istream&). The deserialization logic must read and reconstruct all critical state information from this provided stream. bool Returns true if the state was successfully reconstructed from the stream; otherwise, it returns false, indicating that the stream data was corrupted or incomplete.

#### `audio::LanguageDetectionResult detectLanguage(const std::vector< float > &pcm, float sample_rate)=0`
- Source: `include/whisper/whisper_transcriber.h`:102
- Brief: Detects the language spoken within the audio input bytes before transcription.
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a
- Return: std::true_type If a dominant language was successfully identified and configured. Returns DetectionError upon failure or ambiguity.
- Details: This preparatory method analyzes the raw audio data stream to determine the primary natural language, optimizing subsequent transcription calls for accuracy and localization. The detected language code (e.g., 'en', 'de') can be crucial for selecting the appropriate Whisper model variant or pre-processing configuration. audioInput Raw byte array stream containing the audio data to analyze. std::true_type If a dominant language was successfully identified and configured. Returns DetectionError upon failure or ambiguity.

#### `DiarisationResult diarize(const std::vector< float > &, float, const DiarisationConfig &)`
- Source: `include/whisper/whisper_transcriber.h`:111
- Brief: Optional speaker diarisation API.
- Parameters:
  - `<unnamed>` (const std::vector< float > &): n/a
  - `<unnamed>` (float): n/a
  - `<unnamed>` (const DiarisationConfig &): n/a
- Details: Default implementation returns an empty successful result so existing implementations remain source-compatible.

#### `std::string getLastError() const`
- Source: `include/whisper/whisper_transcriber.h`:122
- Brief: Return the most recent initialization or runtime error.
- Parameters: none
- Details: Implementations should return an empty string when no error is available.

#### `std::string getModelId() const =0`
- Source: `include/whisper/whisper_transcriber.h`:152
- Brief: Retrieves the unique identifier string for the loaded Whisper model.
- Parameters: none
- Return: std::string The unique identifier string for the initialized Whisper audio model (e.g., "small", "medium").
- Details: This model ID determines which specific pre-trained weight set was used for transcription. This information is crucial for external debugging, reproducibility checks, and logging purposes when linking a transcript to its source model version. std::string The unique identifier string for the initialized Whisper audio model (e.g., "small", "medium").

#### `bool initialize(const WhisperConfig &cfg)=0`
- Source: `include/whisper/whisper_transcriber.h`:66
- Brief: n/a
- Parameters:
  - `cfg` (const WhisperConfig &): n/a

#### `bool isInitialized() const =0`
- Source: `include/whisper/whisper_transcriber.h`:76
- Brief: Checks if the transcriber instance has been successfully initialized.
- Parameters: none
- Return: std::true_type true if the transciver is in a usable state; otherwise, false.
- Details: This method provides a quick way to verify whether model loading and configuration steps, like calling initialize(), have completed successfully without runtime errors. It is crucial for robust application flow control within ThemisDB components that rely on basic writability. std::true_type true if the transciver is in a usable state; otherwise, false.

#### `bool isVersionSupported(const std::string &version_id) const =0`
- Source: `include/whisper/whisper_transcriber.h`:212
- Brief: Checks if a given state identifier is known and loadable by the current instance.
- Parameters:
  - `version_id` (const std::string &): The unique string identifier representing the model/state version (e.g., "V2\_LSTM\_2024").
- Return: bool True if the system recognizes and supports initializing with this specific version ID; otherwise, false.
- Details: This is used primarily for version checking before attempting deserialization, ensuring that incompatible schema changes do not cause runtime failures. It should return true only for versions explicitly supported by the implementation class derived from this interface. version_id The unique string identifier representing the model/state version (e.g., "V2\_LSTM\_2024"). bool True if the system recognizes and supports initializing with this specific version ID; otherwise, false.

#### `void loadState(const std::vector< char > &stateData)=0`
- Source: `include/whisper/whisper_transcriber.h`:173
- Brief: Loads and restores the internal state of the transcriber from a serialized stream.
- Parameters:
  - `stateData` (const std::vector< char > &): The byte buffer containing the serialized transcriber state. Must not be empty.
- Details: This function takes raw bytes representing a saved state and reconstructs all necessary internal variables, such as accumulated text segments or chunk metadata required for continued transcription. Failure to provide valid data will result in an exception or corrupted state. stateData The byte buffer containing the serialized transcriber state. Must not be empty.

#### `std::vector< char > serialize() const =0`
- Source: `include/whisper/whisper_transcriber.h`:163
- Brief: Serializes the current state of the transcriber.
- Parameters: none
- Return: A byte buffer containing the entire persistent state of the transcriber.
- Details: This method generates a compact, self-contained representation of the transcriber's internal state (e.g., accumulated phrases, metadata). These serialized data structures can be persisted to disk or transmitted over a network and later restored by calling loadState(). The format must be strictly defined to ensure reproducibility across different application runs. A byte buffer containing the entire persistent state of the transcriber.

#### `bool serialize(std::ostream &stream) const =0`
- Source: `include/whisper/whisper_transcriber.h`:192
- Brief: Serializes the entire state of the transcriber to a data stream or file.
- Parameters:
  - `stream` (std::ostream &): A constant reference to an output stream (std::ostream&), such as std::ofstream. The state data will be written directly to this stream.
- Return: true if the serialization was successful for all components; false otherwise, indicating a critical failure during write operations.
- Details: This function is crucial for persisting the active session state (including transcript segments, internal buffers, and configuration) so that it can be reliably restored later. The format used must adhere strictly to ThemisDB's defined serialization protocol v2.0. stream A reference to the output stream (e.g., std::ostream&) where the serialized data will be written. This stream must be open and writable. true if the serialization was successful for all components; false otherwise, indicating a critical failure during write operations. Serializes the current state of the transcriber object to a given output stream. This function is essential for saving the runtime state (e.g., model configuration, last recognized features) so that the transcriber can be re-initialized exactly at a later point in the application lifecycle (persistence). stream A constant reference to an output stream (std::ostream&), such as std::ofstream. The state data will be written directly to this stream. bool Returns true if all necessary components were successfully written to the stream; otherwise, false. Must check for streaming errors.

#### `audio::TranscriptionResult transcribe(const std::vector< float > &pcm, float sample_rate)=0`
- Source: `include/whisper/whisper_transcriber.h`:89
- Brief: Transcribes the provided audio input into structured transcription data.
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a
- Return: std::true_type The successful transcription result object containing all necessary details. Throws an exception upon failure.
- Details: This core method uses the underlying Whisper model to process raw audio bytes and generate a comprehensive audio::TranscriptionResult object containing transcribed text, timestamps, and confidence scores for synchronization purposes. The audioInput must be correctly formed and contain valid audio data recognized by the system's backend. audioInput Reference to the audio data source; must not be empty or improperly formatted. std::true_type The successful transcription result object containing all necessary details. Throws an exception upon failure.

#### `audio::TranscriptionResult transcribeStream(const std::vector< float > &pcm, float sample_rate, audio::StreamCallback callback)`
- Source: `include/whisper/whisper_transcriber.h`:131
- Brief: Transcribe with incremental token streaming.
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a
  - `callback` (audio::StreamCallback): n/a
- Details: Default implementation calls transcribe() and emits the full text as one token. Implementations backed by a real model should call the callback for every word or segment.

#### `~IWhisperTranscriber()=default`
- Source: `include/whisper/whisper_transcriber.h`:64
- Brief: Virtual destructor for IWhisperTranscriber.
- Parameters: none
- Details: Properly cleans up all resources associated with any concrete implementations derived from this interface, ensuring safe polymorphic destruction.

### themis::whisper::InMemoryWhisperTranscriber

#### `bool deserialize(std::istream &stream) override`
- Source: `include/whisper/whisper_transcriber.h`:521
- Brief: Deserializes the transcriber object's state from an input stream.
- Parameters:
  - `stream` (std::istream &): A constant reference to an input stream (std::istream&). The deserialization logic must read and reconstruct all critical state information from this provided stream.
- Return: bool Returns true if the state was successfully reconstructed from the stream; otherwise, it returns false, indicating that the stream data was corrupted or incomplete.
- Details: This method is responsible for reading and restoring all necessary internal parameters, model configurations (e.g., whisper version ID), and any transient state data previously saved by serialize(). It must ensure the object's methods are updated to reflect the loaded state. stream A constant reference to an input stream (std::istream&). The deserialization logic must read and reconstruct all critical state information from this provided stream. bool Returns true if the state was successfully reconstructed from the stream; otherwise, it returns false, indicating that the stream data was corrupted or incomplete.

#### `audio::LanguageDetectionResult detectLanguage(const std::vector< float > &, float) override`
- Source: `include/whisper/whisper_transcriber.h`:459
- Brief: Detects the language spoken within the audio input bytes before transcription.
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a
- Return: std::true_type If a dominant language was successfully identified and configured. Returns DetectionError upon failure or ambiguity.
- Details: This preparatory method analyzes the raw audio data stream to determine the primary natural language, optimizing subsequent transcription calls for accuracy and localization. The detected language code (e.g., 'en', 'de') can be crucial for selecting the appropriate Whisper model variant or pre-processing configuration. audioInput Raw byte array stream containing the audio data to analyze. std::true_type If a dominant language was successfully identified and configured. Returns DetectionError upon failure or ambiguity.

#### `DiarisationResult diarize(const std::vector< float > &, float sample_rate, const DiarisationConfig &) override`
- Source: `include/whisper/whisper_transcriber.h`:462
- Brief: Optional speaker diarisation API.
- Parameters:
  - `<unnamed>` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a
  - `<unnamed>` (const DiarisationConfig &): n/a
- Details: Default implementation returns an empty successful result so existing implementations remain source-compatible.

#### `std::string getModelId() const override`
- Source: `include/whisper/whisper_transcriber.h`:496
- Brief: Retrieves the unique identifier string for the loaded Whisper model.
- Parameters: none
- Return: std::string The unique identifier string for the initialized Whisper audio model (e.g., "small", "medium").
- Details: This model ID determines which specific pre-trained weight set was used for transcription. This information is crucial for external debugging, reproducibility checks, and logging purposes when linking a transcript to its source model version. std::string The unique identifier string for the initialized Whisper audio model (e.g., "small", "medium").

#### `bool initialize(const WhisperConfig &cfg) override`
- Source: `include/whisper/whisper_transcriber.h`:446
- Brief: n/a
- Parameters:
  - `cfg` (const WhisperConfig &): n/a

#### `bool isInitialized() const override`
- Source: `include/whisper/whisper_transcriber.h`:451
- Brief: Checks if the transcriber instance has been successfully initialized.
- Parameters: none
- Return: std::true_type true if the transciver is in a usable state; otherwise, false.
- Details: This method provides a quick way to verify whether model loading and configuration steps, like calling initialize(), have completed successfully without runtime errors. It is crucial for robust application flow control within ThemisDB components that rely on basic writability. std::true_type true if the transciver is in a usable state; otherwise, false.

#### `bool isVersionSupported(const std::string &version_id) const override`
- Source: `include/whisper/whisper_transcriber.h`:533
- Brief: Checks if a given state identifier is known and loadable by the current instance.
- Parameters:
  - `version_id` (const std::string &): The unique string identifier representing the model/state version (e.g., "V2\_LSTM\_2024").
- Return: bool True if the system recognizes and supports initializing with this specific version ID; otherwise, false.
- Details: This is used primarily for version checking before attempting deserialization, ensuring that incompatible schema changes do not cause runtime failures. It should return true only for versions explicitly supported by the implementation class derived from this interface. version_id The unique string identifier representing the model/state version (e.g., "V2\_LSTM\_2024"). bool True if the system recognizes and supports initializing with this specific version ID; otherwise, false.

#### `void loadState(const std::vector< char > &stateData) override`
- Source: `include/whisper/whisper_transcriber.h`:505
- Brief: Loads and restores the internal state of the transcriber from a serialized stream.
- Parameters:
  - `stateData` (const std::vector< char > &): The byte buffer containing the serialized transcriber state. Must not be empty.
- Details: This function takes raw bytes representing a saved state and reconstructs all necessary internal variables, such as accumulated text segments or chunk metadata required for continued transcription. Failure to provide valid data will result in an exception or corrupted state. stateData The byte buffer containing the serialized transcriber state. Must not be empty.

#### `std::vector< char > serialize() const override`
- Source: `include/whisper/whisper_transcriber.h`:497
- Brief: Serializes the current state of the transcriber.
- Parameters: none
- Return: A byte buffer containing the entire persistent state of the transcriber.
- Details: This method generates a compact, self-contained representation of the transcriber's internal state (e.g., accumulated phrases, metadata). These serialized data structures can be persisted to disk or transmitted over a network and later restored by calling loadState(). The format must be strictly defined to ensure reproducibility across different application runs. A byte buffer containing the entire persistent state of the transcriber.

#### `bool serialize(std::ostream &stream) const override`
- Source: `include/whisper/whisper_transcriber.h`:514
- Brief: Serializes the entire state of the transcriber to a data stream or file.
- Parameters:
  - `stream` (std::ostream &): A constant reference to an output stream (std::ostream&), such as std::ofstream. The state data will be written directly to this stream.
- Return: true if the serialization was successful for all components; false otherwise, indicating a critical failure during write operations.
- Details: This function is crucial for persisting the active session state (including transcript segments, internal buffers, and configuration) so that it can be reliably restored later. The format used must adhere strictly to ThemisDB's defined serialization protocol v2.0. stream A reference to the output stream (e.g., std::ostream&) where the serialized data will be written. This stream must be open and writable. true if the serialization was successful for all components; false otherwise, indicating a critical failure during write operations. Serializes the current state of the transcriber object to a given output stream. This function is essential for saving the runtime state (e.g., model configuration, last recognized features) so that the transcriber can be re-initialized exactly at a later point in the application lifecycle (persistence). stream A constant reference to an output stream (std::ostream&), such as std::ofstream. The state data will be written directly to this stream. bool Returns true if all necessary components were successfully written to the stream; otherwise, false. Must check for streaming errors.

#### `void setNextDiarisationResult(DiarisationResult r)`
- Source: `include/whisper/whisper_transcriber.h`:476
- Brief: n/a
- Parameters:
  - `r` (DiarisationResult): n/a

#### `void setNextLanguage(audio::LanguageDetectionResult r)`
- Source: `include/whisper/whisper_transcriber.h`:438
- Brief: n/a
- Parameters:
  - `r` (audio::LanguageDetectionResult): n/a

#### `void setNextResult(audio::TranscriptionResult r)`
- Source: `include/whisper/whisper_transcriber.h`:434
- Brief: n/a
- Parameters:
  - `r` (audio::TranscriptionResult): n/a

#### `void setStreamTokens(std::vector< audio::TranscriptionToken > tokens)`
- Source: `include/whisper/whisper_transcriber.h`:442
- Brief: n/a
- Parameters:
  - `tokens` (std::vector< audio::TranscriptionToken >): n/a
- Details: Pre-set tokens to emit during transcribeStream() instead of one bulk token.

#### `audio::TranscriptionResult transcribe(const std::vector< float > &, float) override`
- Source: `include/whisper/whisper_transcriber.h`:453
- Brief: Transcribes the provided audio input into structured transcription data.
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a
- Return: std::true_type The successful transcription result object containing all necessary details. Throws an exception upon failure.
- Details: This core method uses the underlying Whisper model to process raw audio bytes and generate a comprehensive audio::TranscriptionResult object containing transcribed text, timestamps, and confidence scores for synchronization purposes. The audioInput must be correctly formed and contain valid audio data recognized by the system's backend. audioInput Reference to the audio data source; must not be empty or improperly formatted. std::true_type The successful transcription result object containing all necessary details. Throws an exception upon failure.

#### `audio::TranscriptionResult transcribeStream(const std::vector< float > &pcm, float sample_rate, audio::StreamCallback callback) override`
- Source: `include/whisper/whisper_transcriber.h`:480
- Brief: Transcribe with incremental token streaming.
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a
  - `callback` (audio::StreamCallback): n/a
- Details: Default implementation calls transcribe() and emits the full text as one token. Implementations backed by a real model should call the callback for every word or segment.

### themis::whisper::WavAudioChunkReader

#### `bool canRead(const std::string &path) const override`
- Source: `include/whisper/audio_chunk_reader.h`:101
- Brief: Prüft, ob der Reader mit den Inhalten des angegebenen Dateipfades lesen kann.
- Parameters:
  - `path` (const std::string &): n/a
- Return: True, wenn der Reader voraussichtlich geeignete Metadaten findet oder das Format unterstützt wird; andernfalls False.
- Throws:
  - Keine: spezifischen Ausnahmen erwartet, da es sich um eine reine Überprüfung handelt. Fehlerhafte Dateipfade sollten in einer Exception (z.B. FileSystemError) abgefangen werden können. @ownership Die Funktion hat nur Lesezugriff und erzeugt keinen dauerhaften Ownership-Zustand.
- Details: Diese Funktion analysiert die Metadaten oder versucht einen kleinen Test-Read, um zu entscheiden, ob das Format (z.B. WAV, MP3 etc.) mit dem hinterlegten Reader-Typ kompatibel ist. Sie dient als vorgelagerte Prüfung vor einem teuren readFile-Aufruf. filePath Der Pfad zur Audiodatei, die überprüft werden soll. True, wenn der Reader voraussichtlich geeignete Metadaten findet oder das Format unterstützt wird; andernfalls False. Keine spezifischen Ausnahmen erwartet, da es sich um eine reine Überprüfung handelt. Fehlerhafte Dateipfade sollten in einer Exception (z.B. FileSystemError) abgefangen werden können. @ownership Die Funktion hat nur Lesezugriff und erzeugt keinen dauerhaften Ownership-Zustand.

#### `std::map< std::string, std::string > getMetadata(const std::string &path) const override`
- Source: `include/whisper/audio_chunk_reader.h`:103
- Brief: Ruft Metadaten des derzeit verarbeiteten Audio-Chunks ab oder gibt einen Standardwert zurück, falls keine spezifischen Metadaten verfügbar sind.
- Parameters:
  - `path` (const std::string &): n/a
- Return: std::map<std::string, std::string> Ein Kartencontainer, der Schlüssel-Wert-Paare mit den extrahierten Metadaten enthält. Der Schlüssel sollte eine standardisierte Bezeichnung (z. B. "SampleRate", "Format") verwenden. Bei Fehlschlagen des Abrufs wird ein leeres oder notwendiger Standardeintrag zurückgegeben.
- Details: Diese Funktion ist entscheidend für die Validierung der Audioquelle und liefert Informationen wie das Dateiformat, die Bitrate oder die Sample Rate in einem einheitlichen Container. Die Implementierung muss die zugrundeliegenden Spezifika (z.B. WAV-Header-Parsing vs. FFMPEG-Streams) korrekt abstrahieren. std::map<std::string, std::string> Ein Kartencontainer, der Schlüssel-Wert-Paare mit den extrahierten Metadaten enthält. Der Schlüssel sollte eine standardisierte Bezeichnung (z. B. "SampleRate", "Format") verwenden. Bei Fehlschlagen des Abrufs wird ein leeres oder notwendiger Standardeintrag zurückgegeben. Die Rückgabestruktur dient als universelle Schnittstelle, um unterschiedliche Quelleigenschaften konsolidiert darzustellen. @ownership Die übergebenen Daten im Map-Objekt müssen vom Aufrufer sorgfältig auf Gültigkeit geprüft werden. @threading Der Aufruf ist thread-sicher zu gewährleisten; ggf. muss ein externer Mutex zum Schutz der internen Zustandsvariablen verwendet werden, falls das Laden von Metadaten nicht atomar ist.

#### `std::vector< float > parseWav(const std::vector< uint8_t > &data, float &out_sample_rate)`
- Source: `include/whisper/audio_chunk_reader.h`:106
- Brief: ── WavAudioChunkReader::parseWav ────────────────────────────────────────────
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
  - `out_sample_rate` (float &): Input/output parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: data Input parameter. out_sample_rate Input/output parameter. Return value. std::runtime_error if an error occurs. Calls: size(), readU32LE(), readU16LE(), std::to_string(), std::min(), reserve(), std::memcpy(), push_back().

#### `std::vector< float > readFile(const std::string &path, float &out_sample_rate) override`
- Source: `include/whisper/audio_chunk_reader.h`:98
- Brief: Implementiert die Lesefunktion für WAV-Dateien, wobei der Dateipfad analysiert und kontinuierlich Audio-Chunks extrahiert werden.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `out_sample_rate` (float &): Input/output parameter.
- Return: std::vector<float> Ein Vektor von Float32-Werten, die zusammen einen oder mehrere kontinuierliche Audio-Datenblöcke darstellen. Der Rückgabewert ist leer, wenn am Ende des Streams angelangt wurde und dies als Erfolg zu werten ist.
- Throws:
  - std::runtime_error: Wird ausgelöst, falls der Dateipfad ungültig ist, ein I/O-Fehler auftritt oder die gemessene Bitrate mit den erwarteten WAV-Parametern kollidiert. @ownership Die gelesenen Daten werden als temporärer Output des Readers bereitgestellt und sind vom Aufrufer verantwortlich für deren Speicherfreigabe oder Weiterverarbeitung. @threading Da dieser Reader auf lokale Dateioperationen angewiesen ist, sollte bei gleichzeitigen Zugriffsversuchen extern durch Locking mechanismen abgesichert werden.
  - std::runtime_error: if an error occurs.
- Details: ── WavAudioChunkReader::readFile ──────────────────────────────────────────── Diese spezialisierte Methode erweitert die Basisimplementierung von IAudioChunkReader und nutzt ausschließlich das interne Wissen über den WAV-Header zur fehlerfreien Auslesung des PCM Float32-Streams. Der Prozess ist für ein exaktes, deterministisches Format (WAV) optimiert. path Der absolute Pfad zur zu verarbeitenden Audiodatei im WAV-Format. out_sample_rate Die Sampling Rate des gelesenen Audio-Chunks. Nach dem Aufruf muss diese Variable mit der korrekten Frequenz initialisiert sein (z. B. 16000 Hz). std::vector<float> Ein Vektor von Float32-Werten, die zusammen einen oder mehrere kontinuierliche Audio-Datenblöcke darstellen. Der Rückgabewert ist leer, wenn am Ende des Streams angelangt wurde und dies als Erfolg zu werten ist. std::runtime_error Wird ausgelöst, falls der Dateipfad ungültig ist, ein I/O-Fehler auftritt oder die gemessene Bitrate mit den erwarteten WAV-Parametern kollidiert. @ownership Die gelesenen Daten werden als temporärer Output des Readers bereitgestellt und sind vom Aufrufer verantwortlich für deren Speicherfreigabe oder Weiterverarbeitung. @threading Da dieser Reader auf lokale Dateioperationen angewiesen ist, sollte bei gleichzeitigen Zugriffsversuchen extern durch Locking mechanismen abgesichert werden. path Input parameter. out_sample_rate Input/output parameter. Return value. std::runtime_error if an error occurs. Calls: f(), is_open(), tellg(), seekg(), data(), read(), parseWav().

### themis::whisper::WhisperConfig

#### `WhisperConfig fromJson(const json &j)`
- Source: `include/whisper/whisper_config.h`:33
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: contains().

#### `json toJson() const`
- Source: `include/whisper/whisper_config.h`:34
- Brief: n/a
- Parameters: none

### themis::whisper::WhisperPlugin

#### `WhisperPlugin()`
- Source: `include/whisper/whisper_plugin.h`:46
- Brief: n/a
- Parameters: none
- Details: Default constructor – builds production or stub backend automatically.

#### `WhisperPlugin(std::unique_ptr< IWhisperTranscriber > transcriber, std::unique_ptr< IAudioChunkReader > reader)`
- Source: `include/whisper/whisper_plugin.h`:49
- Brief: n/a
- Parameters:
  - `transcriber` (std::unique_ptr< IWhisperTranscriber >): n/a
  - `reader` (std::unique_ptr< IAudioChunkReader >): n/a
- Details: Injection constructor for tests.

#### `std::vector< float > applyVad(const std::vector< float > &pcm, float sample_rate) const`
- Source: `include/whisper/whisper_plugin.h`:111
- Brief: n/a
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a

#### `audio::LanguageDetectionResult detectLanguage(const std::vector< float > &pcm_samples, float sample_rate) override`
- Source: `include/whisper/whisper_plugin.h`:79
- Brief: ── detectLanguage ───────────────────────────────────────────────────────────
- Parameters:
  - `pcm_samples` (const std::vector< float > &): n/a
  - `sample_rate` (float): Input parameter.
- Return: Return value.
- Details: pcm Input parameter. sample_rate Input parameter. Return value. Calls: load(), lock().

#### `std::string getModelId() const override`
- Source: `include/whisper/whisper_plugin.h`:92
- Brief: n/a
- Parameters: none

#### `std::string getPluginVersion() const override`
- Source: `include/whisper/whisper_plugin.h`:93
- Brief: n/a
- Parameters: none

#### `nlohmann::json getStatistics() const override`
- Source: `include/whisper/whisper_plugin.h`:94
- Brief: n/a
- Parameters: none

#### `bool initialize(const std::string &model_path, const nlohmann::json &config) override`
- Source: `include/whisper/whisper_plugin.h`:55
- Brief: ── initialize ───────────────────────────────────────────────────────────────
- Parameters:
  - `model_path` (const std::string &): Path to the model.
  - `config` (const nlohmann::json &): Input parameter.
- Return: True when the operation succeeds.
- Details: model_path Path to the model. config Input parameter. True when the operation succeeds. Calls: WhisperConfig::fromJson(), getLastError(), empty(), lk(), std::move(), clear(), store().

#### `bool isInitialized() const override`
- Source: `include/whisper/whisper_plugin.h`:58
- Brief: n/a
- Parameters: none

#### `void setStubTranscriberFactoryFn(StubTranscriberFactoryFn fn)`
- Source: `include/whisper/whisper_plugin.h`:96
- Brief: Set Stub Transcriber Factory Fn.
- Parameters:
  - `fn` (StubTranscriberFactoryFn): Input parameter.
- Details: fn Input parameter. Calls: lk(), std::move().

#### `void setVoiceActivityDetector(std::unique_ptr< IVoiceActivityDetector > vad, const VadConfig &cfg={})`
- Source: `include/whisper/whisper_plugin.h`:106
- Brief: Inject a custom Voice Activity Detector.
- Parameters:
  - `vad` (std::unique_ptr< IVoiceActivityDetector >): Input parameter.
  - `cfg` (const VadConfig &): Input parameter.
- Details: ── VAD ────────────────────────────────────────────────────────────────────── If set, transcribeStream() (and transcribe() when vad_config is non- default) uses the VAD to skip silent segments before inference. Passing nullptr disables VAD. vad Input parameter. cfg Input parameter. Calls: lk(), std::move().

#### `audio::TranscriptionResult transcribe(const std::vector< float > &pcm_samples, float sample_rate) override`
- Source: `include/whisper/whisper_plugin.h`:60
- Brief: ── transcribe ───────────────────────────────────────────────────────────────
- Parameters:
  - `pcm_samples` (const std::vector< float > &): n/a
  - `sample_rate` (float): Input parameter.
- Return: Return value.
- Details: pcm Input parameter. sample_rate Input parameter. Return value. Calls: load(), fetch_add(), lk(), empty(), getPluginVersion(), lock(), getModelId(), std::chrono::system_clock::now().

#### `audio::TranscriptionResult transcribeFile(const std::string &path) override`
- Source: `include/whisper/whisper_plugin.h`:63
- Brief: ── transcribeFile ───────────────────────────────────────────────────────────
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: path Input parameter. Return value. Calls: load(), fetch_add(), lk(), empty(), getPluginVersion(), readFile(), transcribe(), what().

#### `audio::TranscriptionResult transcribeStream(const std::vector< float > &pcm_samples, float sample_rate, audio::StreamCallback callback) override`
- Source: `include/whisper/whisper_plugin.h`:74
- Brief: Transcribe with incremental token streaming.
- Parameters:
  - `pcm_samples` (const std::vector< float > &): Input parameter.
  - `sample_rate` (float): Input parameter.
  - `callback` (audio::StreamCallback): Input parameter.
- Return: Return value.
- Details: ── transcribeStream ───────────────────────────────────────────────────────── If a VAD is installed, silent segments are skipped before the PCM is forwarded to the transcriber. The callback is invoked once per token emitted by the underlying transcriber. Any exception thrown by the callback aborts the stream and the returned result has success=false. pcm_samples Input parameter. sample_rate Input parameter. callback Input parameter. Return value. Calls: load(), fetch_add(), lk(), empty(), getPluginVersion(), applyVad(), lock(), std::move().

#### `DiarisationResult transcribeWithDiarisation(const std::vector< float > &pcm_samples, float sample_rate, const DiarisationConfig &cfg)`
- Source: `include/whisper/whisper_plugin.h`:88
- Brief: Transcribe and optionally attach speaker diarisation segments.
- Parameters:
  - `pcm_samples` (const std::vector< float > &): Input parameter.
  - `sample_rate` (float): Input parameter.
  - `cfg` (const DiarisationConfig &): Input parameter.
- Return: Return value.
- Details: Transcribe With Diarisation. Uses the transcriber's optional diarize() capability and always applies plugin-side provenance fields on the returned result. pcm_samples Input parameter. sample_rate Input parameter. cfg Input parameter. Return value. Calls: getPluginVersion(), getModelId(), std::chrono::system_clock::now(), time_since_epoch(), count(), load(), fetch_add(), lk().

#### `~WhisperPlugin() override=default`
- Source: `include/whisper/whisper_plugin.h`:52
- Brief: n/a
- Parameters: none

### themis::whisper::WhisperPluginAdapter

#### `WhisperPluginAdapter(std::unique_ptr< WhisperPlugin > plugin)`
- Source: `include/whisper/whisper_plugin_registrar.h`:55
- Brief: Construct from an existing WhisperPlugin instance.
- Parameters:
  - `plugin` (std::unique_ptr< WhisperPlugin >): Heap-allocated WhisperPlugin; adapter takes ownership.
- Details: plugin Heap-allocated WhisperPlugin; adapter takes ownership.

#### `plugins::PluginCapabilities getCapabilities() const override`
- Source: `include/whisper/whisper_plugin_registrar.h`:65
- Brief: n/a
- Parameters: none

#### `void * getInstance() override`
- Source: `include/whisper/whisper_plugin_registrar.h`:88
- Brief: Return a pointer to the underlying WhisperPlugin.
- Parameters: none
- Details: Callers should cast the return value to whisper::WhisperPlugin*.

#### `const char * getName() const override`
- Source: `include/whisper/whisper_plugin_registrar.h`:58
- Brief: n/a
- Parameters: none

#### `plugins::PluginType getType() const override`
- Source: `include/whisper/whisper_plugin_registrar.h`:61
- Brief: n/a
- Parameters: none

#### `const char * getVersion() const override`
- Source: `include/whisper/whisper_plugin_registrar.h`:59
- Brief: n/a
- Parameters: none

#### `WhisperPlugin * getWhisperPlugin()`
- Source: `include/whisper/whisper_plugin_registrar.h`:93
- Brief: n/a
- Parameters: none
- Return: Non-owning pointer to the wrapped WhisperPlugin.
- Details: Non-owning pointer to the wrapped WhisperPlugin.

#### `const WhisperPlugin * getWhisperPlugin() const`
- Source: `include/whisper/whisper_plugin_registrar.h`:94
- Brief: n/a
- Parameters: none

#### `bool initialize(const char *config_json) override`
- Source: `include/whisper/whisper_plugin_registrar.h`:76
- Brief: Initialize the underlying WhisperPlugin.
- Parameters:
  - `config_json` (const char *): Input parameter.
- Return: true on success; false when config is missing a non-empty model_path.
- Details: Initialize. Parses config_json and calls WhisperPlugin::initialize(). Expected JSON keys: "model_path" (string, optional). config_json JSON configuration string. true on success; false when config is missing a non-empty model_path. config_json Input parameter. True when the operation succeeds. Calls: nlohmann::json::parse(), contains(), is_string(), empty().

#### `void shutdown() override`
- Source: `include/whisper/whisper_plugin_registrar.h`:81
- Brief: Shutdown: reset the inner plugin to its default stub state.
- Parameters: none
- Details: Shutdown. Calls: clear().

### themis::whisper::WhisperPluginRegistrar

#### `WhisperPluginRegistrar()=delete`
- Source: `include/whisper/whisper_plugin_registrar.h`:198
- Brief: n/a
- Parameters: none

#### `std::unique_ptr< WhisperPluginAdapter > createAdapter(const json &config={})`
- Source: `include/whisper/whisper_plugin_registrar.h`:153
- Brief: Create a WhisperPluginAdapter wrapping a new WhisperPlugin.
- Parameters:
  - `config` (const json &): Input parameter.
- Return: Heap-allocated WhisperPluginAdapter; caller owns.
- Details: Create Adapter. The adapter implements IThemisPlugin and can be handed directly to plugins::PluginManager. config Optional JSON configuration. Heap-allocated WhisperPluginAdapter; caller owns. config Input parameter. Return value. Calls: createPlugin(), std::move().

#### `std::unique_ptr< WhisperPlugin > createPlugin(const json &config={})`
- Source: `include/whisper/whisper_plugin_registrar.h`:142
- Brief: Create a standalone WhisperPlugin instance.
- Parameters:
  - `config` (const json &): Input parameter.
- Return: Heap-allocated WhisperPlugin; caller owns.
- Details: ── WhisperPluginRegistrar — factory methods ────────────────────────────────── config Optional JSON configuration. Key: "model_path" (string) — if present, calls WhisperPlugin::initialize(model_path, config). Heap-allocated WhisperPlugin; caller owns. config Input parameter. Return value. Calls: contains(), is_string(), empty(), initialize().

#### `ReloadCallback defaultReloadCallback()`
- Source: `include/whisper/whisper_plugin_registrar.h`:174
- Brief: Default hot-plug reload callback.
- Parameters: none
- Details: Calls WhisperPlugin::initialize(model_path, config) when "model_path" is present; otherwise returns false.

#### `void disableHotPlug(plugins::PluginManager &manager)`
- Source: `include/whisper/whisper_plugin_registrar.h`:195
- Brief: Disable PluginManager hot-plug monitoring.
- Parameters:
  - `manager` (plugins::PluginManager &): Input/output parameter.
- Details: Disable Hot Plug. manager PluginManager instance. manager Input/output parameter. Implements disableHotPlug without additional internal calls.

#### `bool enableHotPlug(plugins::PluginManager &manager, const std::string &directory)`
- Source: `include/whisper/whisper_plugin_registrar.h`:186
- Brief: Enable PluginManager hot-plug monitoring for a directory.
- Parameters:
  - `manager` (plugins::PluginManager &): Input/output parameter.
  - `directory` (const std::string &): Input parameter.
- Return: true on success.
- Details: Enable Hot Plug. Calls manager.enableHotPlug(directory, config) with default options (auto_load=true, auto_reload=true, auto_unload=true). manager PluginManager instance. directory Directory to watch. true on success. manager Input/output parameter. directory Input parameter. True when the operation succeeds. Implements enableHotPlug without additional internal calls.

### themis::whisper::WhisperStubTranscriber

#### `bool deserialize(std::istream &stream) override`
- Source: `include/whisper/whisper_transcriber.h`:399
- Brief: Deserializes the transcriber object's state from an input stream.
- Parameters:
  - `stream` (std::istream &): A constant reference to an input stream (std::istream&). The deserialization logic must read and reconstruct all critical state information from this provided stream.
- Return: bool Returns true if the state was successfully reconstructed from the stream; otherwise, it returns false, indicating that the stream data was corrupted or incomplete.
- Details: This method is responsible for reading and restoring all necessary internal parameters, model configurations (e.g., whisper version ID), and any transient state data previously saved by serialize(). It must ensure the object's methods are updated to reflect the loaded state. stream A constant reference to an input stream (std::istream&). The deserialization logic must read and reconstruct all critical state information from this provided stream. bool Returns true if the state was successfully reconstructed from the stream; otherwise, it returns false, indicating that the stream data was corrupted or incomplete.

#### `audio::LanguageDetectionResult detectLanguage(const std::vector< float > &, float) override`
- Source: `include/whisper/whisper_transcriber.h`:354
- Brief: Detects the language spoken within the audio input bytes before transcription.
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a
- Return: std::true_type If a dominant language was successfully identified and configured. Returns DetectionError upon failure or ambiguity.
- Details: This preparatory method analyzes the raw audio data stream to determine the primary natural language, optimizing subsequent transcription calls for accuracy and localization. The detected language code (e.g., 'en', 'de') can be crucial for selecting the appropriate Whisper model variant or pre-processing configuration. audioInput Raw byte array stream containing the audio data to analyze. std::true_type If a dominant language was successfully identified and configured. Returns DetectionError upon failure or ambiguity.

#### `DiarisationResult diarize(const std::vector< float > &, float sample_rate, const DiarisationConfig &) override`
- Source: `include/whisper/whisper_transcriber.h`:357
- Brief: Optional speaker diarisation API.
- Parameters:
  - `<unnamed>` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a
  - `<unnamed>` (const DiarisationConfig &): n/a
- Details: Default implementation returns an empty successful result so existing implementations remain source-compatible.

#### `std::string getModelId() const override`
- Source: `include/whisper/whisper_transcriber.h`:374
- Brief: Retrieves the unique identifier string for the loaded Whisper model.
- Parameters: none
- Return: std::string The unique identifier string for the initialized Whisper audio model (e.g., "small", "medium").
- Details: This model ID determines which specific pre-trained weight set was used for transcription. This information is crucial for external debugging, reproducibility checks, and logging purposes when linking a transcript to its source model version. std::string The unique identifier string for the initialized Whisper audio model (e.g., "small", "medium").

#### `bool initialize(const WhisperConfig &cfg) override`
- Source: `include/whisper/whisper_transcriber.h`:317
- Brief: n/a
- Parameters:
  - `cfg` (const WhisperConfig &): n/a

#### `bool isInitialized() const override`
- Source: `include/whisper/whisper_transcriber.h`:322
- Brief: Checks if the transcriber instance has been successfully initialized.
- Parameters: none
- Return: std::true_type true if the transciver is in a usable state; otherwise, false.
- Details: This method provides a quick way to verify whether model loading and configuration steps, like calling initialize(), have completed successfully without runtime errors. It is crucial for robust application flow control within ThemisDB components that rely on basic writability. std::true_type true if the transciver is in a usable state; otherwise, false.

#### `bool isVersionSupported(const std::string &version_id) const override`
- Source: `include/whisper/whisper_transcriber.h`:411
- Brief: Checks if a given state identifier is known and loadable by the current instance.
- Parameters:
  - `version_id` (const std::string &): The unique string identifier representing the model/state version (e.g., "V2\_LSTM\_2024").
- Return: bool True if the system recognizes and supports initializing with this specific version ID; otherwise, false.
- Details: This is used primarily for version checking before attempting deserialization, ensuring that incompatible schema changes do not cause runtime failures. It should return true only for versions explicitly supported by the implementation class derived from this interface. version_id The unique string identifier representing the model/state version (e.g., "V2\_LSTM\_2024"). bool True if the system recognizes and supports initializing with this specific version ID; otherwise, false.

#### `void loadState(const std::vector< char > &stateData) override`
- Source: `include/whisper/whisper_transcriber.h`:383
- Brief: Loads and restores the internal state of the transcriber from a serialized stream.
- Parameters:
  - `stateData` (const std::vector< char > &): The byte buffer containing the serialized transcriber state. Must not be empty.
- Details: This function takes raw bytes representing a saved state and reconstructs all necessary internal variables, such as accumulated text segments or chunk metadata required for continued transcription. Failure to provide valid data will result in an exception or corrupted state. stateData The byte buffer containing the serialized transcriber state. Must not be empty.

#### `std::vector< char > serialize() const override`
- Source: `include/whisper/whisper_transcriber.h`:375
- Brief: Serializes the current state of the transcriber.
- Parameters: none
- Return: A byte buffer containing the entire persistent state of the transcriber.
- Details: This method generates a compact, self-contained representation of the transcriber's internal state (e.g., accumulated phrases, metadata). These serialized data structures can be persisted to disk or transmitted over a network and later restored by calling loadState(). The format must be strictly defined to ensure reproducibility across different application runs. A byte buffer containing the entire persistent state of the transcriber.

#### `bool serialize(std::ostream &stream) const override`
- Source: `include/whisper/whisper_transcriber.h`:392
- Brief: Serializes the entire state of the transcriber to a data stream or file.
- Parameters:
  - `stream` (std::ostream &): A constant reference to an output stream (std::ostream&), such as std::ofstream. The state data will be written directly to this stream.
- Return: true if the serialization was successful for all components; false otherwise, indicating a critical failure during write operations.
- Details: This function is crucial for persisting the active session state (including transcript segments, internal buffers, and configuration) so that it can be reliably restored later. The format used must adhere strictly to ThemisDB's defined serialization protocol v2.0. stream A reference to the output stream (e.g., std::ostream&) where the serialized data will be written. This stream must be open and writable. true if the serialization was successful for all components; false otherwise, indicating a critical failure during write operations. Serializes the current state of the transcriber object to a given output stream. This function is essential for saving the runtime state (e.g., model configuration, last recognized features) so that the transcriber can be re-initialized exactly at a later point in the application lifecycle (persistence). stream A constant reference to an output stream (std::ostream&), such as std::ofstream. The state data will be written directly to this stream. bool Returns true if all necessary components were successfully written to the stream; otherwise, false. Must check for streaming errors.

#### `void setNextDiarisationResult(DiarisationResult r)`
- Source: `include/whisper/whisper_transcriber.h`:371
- Brief: n/a
- Parameters:
  - `r` (DiarisationResult): n/a

#### `void setTranscribeFn(TranscribeFn fn)`
- Source: `include/whisper/whisper_transcriber.h`:326
- Brief: n/a
- Parameters:
  - `fn` (TranscribeFn): n/a
- Details: Inject (or remove) a real transcription fn. Pass nullptr to restore the empty-result stub. Thread-safe with concurrent transcribe() calls.

#### `audio::TranscriptionResult transcribe(const std::vector< float > &pcm, float sample_rate) override`
- Source: `include/whisper/whisper_transcriber.h`:331
- Brief: Transcribes the provided audio input into structured transcription data.
- Parameters:
  - `pcm` (const std::vector< float > &): n/a
  - `sample_rate` (float): n/a
- Return: std::true_type The successful transcription result object containing all necessary details. Throws an exception upon failure.
- Details: This core method uses the underlying Whisper model to process raw audio bytes and generate a comprehensive audio::TranscriptionResult object containing transcribed text, timestamps, and confidence scores for synchronization purposes. The audioInput must be correctly formed and contain valid audio data recognized by the system's backend. audioInput Reference to the audio data source; must not be empty or improperly formatted. std::true_type The successful transcription result object containing all necessary details. Throws an exception upon failure.

### whisper_plugin.h

#### `THEMIS_AUDIO_PLUGIN()`
- Source: `include/whisper/whisper_plugin.h`:132
- Brief: n/a
- Parameters: none

### whisper_transcriber.cpp

#### `bool deserialize(std::istream &stream)=0`
- Source: `include/whisper/whisper_transcriber.cpp`:114
- Brief: Deserializes the transcriber object's state from a given output stream.
- Parameters:
  - `stream` (std::istream &): A constant reference to an input stream (std::istream&). The deserialization logic must read and reconstruct all critical state information from this provided stream.
- Return: bool Returns true if the state was successfully reconstructed from the stream; otherwise, it returns false, indicating that the stream data was corrupted or incomplete.
- Details: stream A constant reference to an input stream (std::istream&). The deserialization logic must read and reconstruct all critical state information from this provided stream. bool Returns true if the state was successfully reconstructed from the stream; otherwise, it returns false, indicating that the stream data was corrupted or incomplete.

#### `bool isModelVersionSupported(const std::string &version_id) const =0`
- Source: `include/whisper/whisper_transcriber.cpp`:133
- Brief: Checks if a specific whisper model version ID is supported by this transcriber implementation.
- Parameters:
  - `version_id` (const std::string &): A constant reference to a std::string. This must match one of the supported model identifiers for this implementation of $IWhisperTranscriber$.
- Return: bool Returns true if $version_id corresponds to a supported model; otherwise, it returns false, signaling an unsupported version attempt.
- Details: This contract method allows client code to verify compatibility before attempting complex operations like initialization or transcription. The returned boolean value determines whether the underlying Whisper engine (or its required dependencies) recognizes and supports the provided version string. version_id A constant reference to a std::string. This must match one of the supported model identifiers for this implementation of $IWhisperTranscriber$. bool Returns true if $version_id corresponds to a supported model; otherwise, it returns false, signaling an unsupported version attempt.

#### `bool isVersionSupported(const std::string &version_id) const =0`
- Source: `include/whisper/whisper_transcriber.cpp`:124
- Brief: Checks if a given state identifier is known and loadable by the current instance.
- Parameters:
  - `version_id` (const std::string &): The unique string identifier representing the model/state version (e.g., "V2_LSTM_2024").
- Return: bool True if the system recognizes and supports initializing with this specific version ID; otherwise, false.
- Details: This is used primarily for version checking before attempting deserialization, ensuring that incompatible schema changes do not cause runtime failures. It should return true only for versions explicitly supported by the implementation class derived from this interface. version_id The unique string identifier representing the model/state version (e.g., "V2_LSTM_2024"). bool True if the system recognizes and supports initializing with this specific version ID; otherwise, false.

#### `void run_transcription_demo()`
- Source: `include/whisper/whisper_transcriber.cpp`:247
- Brief: Simulates the main execution flow to demonstrate transcription life-cycle.
- Parameters: none

#### `T_STUB bool serialize(std::ostream &stream) const =0`
- Source: `include/whisper/whisper_transcriber.cpp`:107
- Brief: n/a
- Parameters:
  - `stream` (std::ostream &): n/a

