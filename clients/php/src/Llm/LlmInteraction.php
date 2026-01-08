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
