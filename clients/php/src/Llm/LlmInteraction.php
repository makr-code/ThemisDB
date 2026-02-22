/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LlmInteraction.php                                 ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php

namespace ThemisDB\Llm;

/**
 * Represents a stored LLM interaction
 */
class LlmInteraction
{
    public string $id;
    public string $createdAt;
    public string $model;
    public array $messages;
    public ?array $reasoningSteps = null;
    public ?array $metadata = null;

    public static function fromArray(array $data): self
    {
        $interaction = new self();
        $interaction->id = $data['id'] ?? '';
        $interaction->createdAt = $data['created_at'] ?? '';
        $interaction->model = $data['model'] ?? '';
        $interaction->messages = $data['messages'] ?? [];
        $interaction->reasoningSteps = $data['reasoning_steps'] ?? null;
        $interaction->metadata = $data['metadata'] ?? null;
        
        return $interaction;
    }
}
