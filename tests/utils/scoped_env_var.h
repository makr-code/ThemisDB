#pragma once

#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace themis::test {

class ScopedEnvVar {
public:
    ScopedEnvVar(std::string name, std::string value)
        : ScopedEnvVar(std::move(name),
                       std::optional<std::string>{std::move(value)}, InitTag{}) {}

    ScopedEnvVar(std::string name, std::nullopt_t)
        : ScopedEnvVar(std::move(name), std::optional<std::string>{}, InitTag{}) {}

    ~ScopedEnvVar() {
        try {
            apply(previous_);
        } catch (...) {
        }
    }

    ScopedEnvVar(const ScopedEnvVar&) = delete;
    ScopedEnvVar& operator=(const ScopedEnvVar&) = delete;

private:
    struct InitTag {};

    ScopedEnvVar(std::string name, std::optional<std::string> value, InitTag)
        : name_(std::move(name)) {
        if (const char* current = std::getenv(name_.c_str())) {
            previous_ = current;
        }
        apply(value);
    }

    void apply(const std::optional<std::string>& value) const {
#ifdef _WIN32
        if (_putenv_s(name_.c_str(), value ? value->c_str() : "") != 0) {
            throw std::runtime_error("Failed to set test environment variable");
        }
#else
        const int result = value
            ? setenv(name_.c_str(), value->c_str(), 1)
            : unsetenv(name_.c_str());
        if (result != 0) {
            throw std::runtime_error("Failed to update test environment variable");
        }
#endif
    }

    std::string name_;
    std::optional<std::string> previous_;
};

} // namespace themis::test
