#include "giuf/core/app_controller.hpp"

namespace giuf::core {

OperationResult AppController::run(OperationType type) const {
    switch (type) {
        case OperationType::Install:
            return {true, "Install workflow placeholder"};
        case OperationType::Update:
            return {true, "Update workflow placeholder"};
        case OperationType::Uninstall:
            return {true, "Uninstall workflow placeholder"};
        case OperationType::Check:
            return {true, "Check workflow placeholder"};
    }
    return {false, "Unknown operation"};
}

}  // namespace giuf::core
