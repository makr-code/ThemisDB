/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_template_validator.h                        ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-04-06 04:09:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     38                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b80f6d5ecc  2026-02-20  Create PromptTemplateValidator class with pragma guards ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
