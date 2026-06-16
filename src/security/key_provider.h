#ifndef KEY_PROVIDER_H
#define KEY_PROVIDER_H

// Placeholder stub for key management provider interface.
// This file is required by demo_encryption.cpp to resolve build dependency errors.

namespace myproject {
    class IKeyProvider {
    public:
        virtual ~IKeyProvider() = default;
        virtual std::string fetchKey(const std::string& keyId) const = 0;
    };
}

#endif // KEY_PROVIDER_H