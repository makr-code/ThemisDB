#pragma once

#include <string>

namespace giuf::core {

enum class OperationType {
    Install,
    Update,
    Uninstall,
    Check
};

struct OperationResult {
    bool ok{false};
    std::string message;
};

class AppController {
public:
    OperationResult run(OperationType type) const;
};

}  // namespace giuf::core
