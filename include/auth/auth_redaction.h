/**
 * @file auth_redaction.h
 * @brief Hilfsfunktionen zum Schwärzen sensibler Daten in Log-Ausgaben.
 *
 * Stellt sicher, dass Passwörter, Tokens, Schlüssel und andere sensible Werte
 * niemals im Klartext in Log-Einträgen erscheinen. Alle Funktionen sind
 * @c constexpr / @c inline und verursachen keinen Laufzeit-Overhead.
 *
 * @note Diese Datei ist header-only. Keine Bibliotheksabhängigkeiten.
 *
 * @example
 * @code
 * std::string kid = "rsa-2048-2024-03";
 * THEMIS_INFO("Key rotated: {}", themis::auth::redact(kid));
 * // → "Key rotated: [REDACTED:16chars]"
 * @endcode
 */

#pragma once

#include <algorithm>
#include <string>
#include <string_view>

namespace themis {
namespace auth {

/**
 * @brief Ersetzt einen sensiblen Wert durch eine sichere Platzhalter-Darstellung.
 *
 * Die Ausgabe enthält nur die Länge des ursprünglichen Werts, nicht den Inhalt.
 * Dies erlaubt es, bei Debugging-Zwecken die Existenz und Länge eines Werts zu
 * prüfen, ohne den Klartext offenzulegen.
 *
 * @param sensitive Der zu schwärzende sensible Wert.
 * @return Platzhalterstring der Form @c "[REDACTED:Nchars]".
 */
[[nodiscard]] inline std::string redact(std::string_view sensitive) {
    return "[REDACTED:" + std::to_string(sensitive.size()) + "chars]";
}

/**
 * @brief Schwärzt einen Wert nur wenn er nicht leer ist; leere Werte bleiben leer.
 *
 * Nützlich für optionale Felder, bei denen ein leerer String kein Geheimnis enthält.
 *
 * @param sensitive Der zu schwärzende sensible Wert.
 * @return Platzhalterstring oder leerer String.
 */
[[nodiscard]] inline std::string redactIfPresent(std::string_view sensitive) {
    if (sensitive.empty()) {
        return {};
    }
    return redact(sensitive);
}

/**
 * @brief Maskiert einen Wert partiell: Zeigt die ersten @p prefix_len Zeichen,
 *        schwärzt den Rest.
 *
 * Nützlich für Key-IDs (z. B. JWT kid), bei denen ein kurzes Präfix zur
 * Diagnose ausreicht (z. B. Algorithmus-Typ) ohne den vollständigen Namen zu
 * offenbaren.
 *
 * @param sensitive  Der zu maskierende Wert.
 * @param prefix_len Anzahl der sichtbaren Zeichen am Anfang (max. @c sensitive.size()).
 * @return Partiell maskierter String, z. B. @c "rsa-[REDACTED:12chars]".
 */
[[nodiscard]] inline std::string redactPartial(std::string_view sensitive,
                                               std::size_t prefix_len = 4) {
    if (sensitive.empty()) {
        return {};
    }
    const std::size_t visible = std::min(prefix_len, sensitive.size());
    const std::size_t hidden  = sensitive.size() - visible;
    return std::string(sensitive.substr(0, visible)) + "[REDACTED:" + std::to_string(hidden)
           + "chars]";
}

}  // namespace auth
}  // namespace themis
