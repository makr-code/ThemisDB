#include <iostream>
#include <nlohmann/json.hpp>

int main() {
    std::string invalid_json = "not a json";
    
    try {
        nlohmann::json parsed = nlohmann::json::parse(invalid_json);
        std::cout << "Parsed successfully (unexpected)" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Caught exception as expected: " << e.what() << std::endl;
    }
    
    std::cout << "Test completed" << std::endl;
    return 0;
}
