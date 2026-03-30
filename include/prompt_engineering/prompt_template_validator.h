/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_template_validator.h                        ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-30 04:09:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     16                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b80f6d5ec  2026-02-20  Create PromptTemplateValidator class with pragma guards ║
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
