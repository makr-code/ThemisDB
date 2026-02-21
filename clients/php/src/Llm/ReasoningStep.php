/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ReasoningStep.php                                  ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     26                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php

namespace ThemisDB\Llm;

/**
 * Represents a reasoning step in LLM interaction
 */
class ReasoningStep
{
    public string $type;
    public array $content;

    public function __construct(string $type, array $content)
    {
        $this->type = $type;
        $this->content = $content;
    }

    public function toArray(): array
    {
        return [
            'type' => $this->type,
            'content' => $this->content,
        ];
    }
}
