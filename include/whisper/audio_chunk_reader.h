/**
 * @file audio_chunk_reader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <map>

namespace themis {
namespace whisper {

/**
 * @brief Interface for reading audio files and returning PCM float32 samples.
 */
class IAudioChunkReader {
public:
    virtual ~IAudioChunkReader() = default;

    /**
     * @brief Liest die Chunks der Audiodatei und füllt den Float32-Vektor.
     *
     * Diese Funktion ist der zentrale Mechanismus zur Extraktion von Rohdaten (PCM float32)
     * aus dem angegebene Dateipfad. Sie muss sicherstellen, dass alle zu lesenden Daten
     * erfolgreich in den bereitgestellten Vektor kopiert werden und die Quelle korrekt
     * weiterverarbeitet wird. Die Implementierung sollte fehlerhafte Lesevorgänge abfangen
     * und diese über @throws deklarieren. Beachten Sie, dass unter Umständen der Dateistream
     * durch einen Fehler mitten im Vorgang geschlossen werden kann.
     *
     * @param filePath Der absolute Pfad zur Audiodatei, die gelesen werden soll.
     * @param pcmBuffer Ein Pointer auf den Vektor von Float32-Samples, in dem die gelesenen Samples gespeichert werden. Die Größe des Puffers muss ausreichend sein.
     * @return True, wenn die Datei erfolgreich gelesen und der Puffer korrekt gefüllt wurde; andernfalls False.
     * @throws std::runtime_error Wenn ein kritisches Fehlerereignis beim Lesen (z. B. Dateiphil fehlen oder Codec-Fehler) eintritt.
     * @ownership Die gelesenen Daten im pcmBuffer sind temporär und werden vom Aufrufer des Readers verwaltet.
     * @threading Kann potenziell in multithreaded Umgebungen aufgerufen werden; die Implementierung muss Thread-Safety garantieren.
     */
    [[nodiscard]] virtual std::vector<float> readFile(const std::string& path,
                                        float& out_sample_rate) = 0;

    /**
     * @brief Prüft, ob der Reader mit den Inhalten des angegebenen Dateipfades lesen kann.
     *
     * Diese Funktion analysiert die Metadaten oder versucht einen kleinen Test-Read, um zu entscheiden,
     * ob das Format (z.B. WAV, MP3 etc.) mit dem hinterlegten Reader-Typ kompatibel ist.
     * Sie dient als vorgelagerte Prüfung vor einem teuren `readFile`-Aufruf.
     *
     * @param filePath Der Pfad zur Audiodatei, die überprüft werden soll.
     * @return True, wenn der Reader voraussichtlich geeignete Metadaten findet oder das Format unterstützt wird; andernfalls False.
     * @throws Keine spezifischen Ausnahmen erwartet, da es sich um eine reine Überprüfung handelt. Fehlerhafte Dateipfade sollten in einer Exception (z.B. FileSystemError) abgefangen werden können.
     * @ownership Die Funktion hat nur Lesezugriff und erzeugt keinen dauerhaften Ownership-Zustand.
     */
    [[nodiscard]] virtual bool canRead(const std::string& path) const = 0;

    /**
     * @brief Ruft Metadaten des derzeit verarbeiteten Audio-Chunks ab oder gibt einen Standardwert zurück, falls keine spezifischen Metadaten verfügbar sind.
     *
     * Diese Funktion ist entscheidend für die Validierung der Audioquelle und liefert Informationen wie das Dateiformat, die Bitrate oder die Sample Rate in einem einheitlichen Container.
     * Die Implementierung muss die zugrundeliegenden Spezifika (z.B. WAV-Header-Parsing vs. FFMPEG-Streams) korrekt abstrahieren.
     *
     * @return std::map<std::string, std::string> Ein Kartencontainer, der Schlüssel-Wert-Paare mit den extrahierten Metadaten enthält. Der Schlüssel sollte eine standardisierte Bezeichnung (z. B. "SampleRate", "Format") verwenden. Bei Fehlschlagen des Abrufs wird ein leeres oder notwendiger Standardeintrag zurückgegeben.
     * @details Die Rückgabestruktur dient als universelle Schnittstelle, um unterschiedliche Quelleigenschaften konsolidiert darzustellen.
     * @ownership Die übergebenen Daten im Map-Objekt müssen vom Aufrufer sorgfältig auf Gültigkeit geprüft werden.
     * @threading Der Aufruf ist thread-sicher zu gewährleisten; ggf. muss ein externer Mutex zum Schutz der internen Zustandsvariablen verwendet werden, falls das Laden von Metadaten nicht atomar ist.
     */
    [[nodiscard]] virtual std::map<std::string, std::string> getMetadata(const std::string& path) const = 0;
};

/**
 * @brief Minimal RIFF/WAV reader.  Supports 16-bit PCM and 32-bit float WAV.
 *
 * No external library dependency.  If the file is not a valid WAV,
 * readFile() throws std::runtime_error.
 */
class WavAudioChunkReader : public IAudioChunkReader {
public:
    /**
     * @brief Implementiert die Lesefunktion für WAV-Dateien, wobei der Dateipfad analysiert und kontinuierlich Audio-Chunks extrahiert werden.
     *
     * Diese spezialisierte Methode erweitert die Basisimplementierung von IAudioChunkReader und nutzt ausschließlich das interne Wissen über den WAV-Header
     * zur fehlerfreien Auslesung des PCM Float32-Streams. Der Prozess ist für ein exaktes, deterministisches Format (WAV) optimiert.
     *
     * @param path Der absolute Pfad zur zu verarbeitenden Audiodatei im WAV-Format.
     * @param out_sample_rate Die Sampling Rate des gelesenen Audio-Chunks. Nach dem Aufruf muss diese Variable mit der korrekten Frequenz initialisiert sein (z. B. 16000 Hz).
     * @return std::vector<float> Ein Vektor von Float32-Werten, die zusammen einen oder mehrere kontinuierliche Audio-Datenblöcke darstellen. Der Rückgabewert ist leer, wenn am Ende des Streams angelangt wurde und dies als Erfolg zu werten ist.
     * @throws std::runtime_error Wird ausgelöst, falls der Dateipfad ungültig ist, ein I/O-Fehler auftritt oder die gemessene Bitrate mit den erwarteten WAV-Parametern kollidiert.
     * @ownership Die gelesenen Daten werden als temporärer Output des Readers bereitgestellt und sind vom Aufrufer verantwortlich für deren Speicherfreigabe oder Weiterverarbeitung.
     * @threading Da dieser Reader auf lokale Dateioperationen angewiesen ist, sollte bei gleichzeitigen Zugriffsversuchen extern durch Locking mechanismen abgesichert werden.
     */
    std::vector<float> readFile(const std::string& path,
                                float& out_sample_rate) override;

    bool canRead(const std::string& path) const override;

    std::map<std::string, std::string> getMetadata(const std::string& path) const override;

private:
    std::vector<float> parseWav(const std::vector<uint8_t>& data,
                                float& out_sample_rate);
};

/**
 * @brief Audio reader that decodes MP3, OGG, FLAC and other formats by shelling
 *        out to the `ffmpeg` binary.
 *
 * The binary must be on PATH.  Output is always resampled to 16 kHz mono
 * float32 PCM.
 *
 * Security: the file path is shell-escaped before being passed to the subprocess.
 * Files whose path contains characters that cannot be safely escaped are rejected
 * with std::runtime_error.
 *
 * If `ffmpeg` is not found on PATH, readFile() throws
 * std::runtime_error("ffmpeg not available").
 */
class FfmpegAudioChunkReader : public IAudioChunkReader {
public:
    /// Maximum raw PCM output accepted from ffmpeg (≈ 3.5 h at 16 kHz mono f32).
    static constexpr size_t kMaxOutputBytes = 500UL * 1024UL * 1024UL;

    std::vector<float> readFile(const std::string& path,
                                float& out_sample_rate) override;

    bool canRead(const std::string& path) const override;

    std::map<std::string, std::string> getMetadata(const std::string& path) const override;

private:
    /// Shell-escape a path for use inside single quotes.
    /// Throws std::runtime_error if the path contains a NUL byte.
    static std::string shellEscape(const std::string& path);
};

/**
 * @brief Composite reader that delegates to the first registered
 *        IAudioChunkReader whose canRead() returns true.
 *
 * Readers are tried in registration order.  If none accepts the file,
 * readFile() throws std::runtime_error.
 */
class CompositeAudioChunkReader : public IAudioChunkReader {
public:
    /// Register a reader.  Readers are tried in the order they are added.
    void addReader(std::unique_ptr<IAudioChunkReader> reader);

    std::vector<float> readFile(const std::string& path,
                                float& out_sample_rate) override;

    bool canRead(const std::string& path) const override;

    std::map<std::string, std::string> getMetadata(const std::string& path) const override;

private:
    std::vector<std::unique_ptr<IAudioChunkReader>> readers_;
};

/**
 * @brief Liest kontinuierlich audio-Datenblöcke aus der angegebenen Dateiquelle.
 *
 * Dies ist die zentrale Methode, um Zugriff auf die sequenziellen Audio-Chunks zu erhalten.
 * Sie navigiert durch den gesamten Stream und gibt an jedem Schritt einen dekodierten
 * Chunk vom Typ float32 zurück. Die Implementierung kümmert sich dabei um das
 * korrekte Management des externen FFmpeg-Kontextes zur Auslesung der Daten.
 *
 * @param filePath Der absolute Pfad zur zu verarbeitenden Audiodatei (z.B. MP3, FLAC).
 * @return std::vector<float> Ein Vektor von float-Werten, die einen einzelnen Audio-Chunk darstellen. Bei Erfolg ist dieser leer, wenn das Ende des Streams erreicht ist und entsprechend mit einem Fehler signalisiert wird.
 * @throws std::runtime_error Wird ausgelöst, falls der zugrundeliegende FFmpeg-Kontext fehlschlägt oder die Datei nicht geöffnet werden kann.
 * @ownership Die Ressourcenverwaltung für FFmpeg-Handler und Dateistreams erfolgt intern und muss sichergestellt werden.
 */
} // namespace whisper
} // namespace themis
