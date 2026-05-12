#include <iostream>
#include <string>

#include "giuf/core/app_controller.hpp"

namespace {

giuf::core::OperationType parseOperation(const std::string& value) {
    if (value == "install") {
        return giuf::core::OperationType::Install;
    }
    if (value == "update") {
        return giuf::core::OperationType::Update;
    }
    if (value == "uninstall") {
        return giuf::core::OperationType::Uninstall;
    }
    return giuf::core::OperationType::Check;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string op = (argc > 1) ? argv[1] : "check";

    const giuf::core::AppController controller;
    const auto result = controller.run(parseOperation(op));

    std::cout << (result.ok ? "OK: " : "ERROR: ") << result.message << '\n';
    return result.ok ? 0 : 1;
}
