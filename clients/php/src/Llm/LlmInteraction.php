/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LlmInteraction.php                                 ║
  Version:         0.0.28                                             ║
  Last Modified:   2026-02-22 11:29:13                                ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c97d719  2026-02-22  Add parallel multi-source BFS/DFS implementation (graph/p... ║
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
