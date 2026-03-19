#pragma once

#include <string>
#include <json/json.h> // Assuming you use a JSON library like jsoncpp

class PromptTemplateValidator {
public:
    // Constructor
    PromptTemplateValidator(const std::string &schema);

    // Validate the prompt template against JSON schema
    bool validate(const Json::Value &templateJson);

private:
    std::string m_schema;
};
