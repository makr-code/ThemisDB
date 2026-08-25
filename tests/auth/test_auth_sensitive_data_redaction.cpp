/**
 * @file test_auth_sensitive_data_redaction.cpp
 * @brief Wave 2-A / A2: Unit-Tests für auth_redaction.h und sensitive-logging-Schutz.
 *
 * Verifiziert, dass:
 * - redact() keine sensiblen Werte im Klartext zurückgibt
 * - redactPartial() nur das erlaubte Präfix zeigt
 * - redactIfPresent() leere Strings korrekt behandelt
 * - Alle Hilfsfunktionen thread-safe in korrekter Manier aufgerufen werden können
 */

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "auth/auth_redaction.h"

namespace themis::auth::test {

// ---------------------------------------------------------------------------
// Test 1: redact() enthält niemals den Originalwert
// ---------------------------------------------------------------------------

TEST(AuthRedactionTest, RedactDoesNotLeakOriginalValue) {
    const std::string password = "mypassword123";
    const std::string result   = redact(password);

    EXPECT_EQ(result.find(password), std::string::npos)
        << "redact() must not contain the original password";
    EXPECT_NE(result, password);
}

// ---------------------------------------------------------------------------
// Test 2: redact() enthält die korrekte Zeichenanzahl
// ---------------------------------------------------------------------------

TEST(AuthRedactionTest, RedactContainsLengthInfo) {
    const std::string token = "******";  // 20 chars
    const std::string result = redact(token);

    EXPECT_NE(result.find("20"), std::string::npos)
        << "redact() should indicate the length of the redacted value";
    EXPECT_NE(result.find("REDACTED"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Test 3: redactPartial() zeigt nur das erlaubte Präfix
// ---------------------------------------------------------------------------

TEST(AuthRedactionTest, RedactPartialExposesOnlyPrefix) {
    const std::string kid    = "rsa-2048-rotation-key-2026";
    const std::string result = redactPartial(kid, 4);

    // Darf nur die ersten 4 Zeichen sichtbar lassen
    EXPECT_EQ(result.substr(0, 4), "rsa-");
    // Rest darf nicht im Klartext stehen
    EXPECT_EQ(result.find("rotation-key-2026"), std::string::npos);
    EXPECT_NE(result.find("REDACTED"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Test 4: redactIfPresent() behandelt leere Strings korrekt
// ---------------------------------------------------------------------------

TEST(AuthRedactionTest, RedactIfPresentHandlesEmptyString) {
    EXPECT_EQ(redactIfPresent(""), "");
    EXPECT_NE(redactIfPresent("secret"), "");
    EXPECT_EQ(redactIfPresent("secret").find("secret"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Test 5: redact() ist thread-safe (mehrere Threads gleichzeitig)
// ---------------------------------------------------------------------------

TEST(AuthRedactionTest, RedactIsThreadSafe) {
    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    std::vector<std::string> results(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&results, i]() {
            results[i] = redact("sensitive_key_material_" + std::to_string(i));
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    for (int i = 0; i < kThreads; ++i) {
        EXPECT_NE(results[i].find("REDACTED"), std::string::npos)
            << "Thread " << i << " should have produced a redacted string";
        const std::string plain = "sensitive_key_material_" + std::to_string(i);
        EXPECT_EQ(results[i].find(plain), std::string::npos)
            << "Thread " << i << " must not leak plaintext";
    }
}

}  // namespace themis::auth::test
