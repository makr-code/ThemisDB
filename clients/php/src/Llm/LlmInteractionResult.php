/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LlmInteractionResult.php                           ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     21                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php

namespace ThemisDB\Llm;

/**
 * Result of creating an LLM interaction
 */
class LlmInteractionResult
{
    public string $id;
    public bool $success;

    public static function fromArray(array $data): self
    {
        $result = new self();
        $result->id = $data['id'] ?? '';
        $result->success = $data['success'] ?? false;
        
        return $result;
    }
}
