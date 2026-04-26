/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LlmMessage.php                                     ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     56                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php

namespace ThemisDB\Llm;

/**
 * Represents a message in an LLM conversation
 */
class LlmMessage
{
    public string $role;
    public string $content;
    public ?string $imageUrl = null;

    public function __construct(string $role, string $content, ?string $imageUrl = null)
    {
        $this->role = $role;
        $this->content = $content;
        $this->imageUrl = $imageUrl;
    }

    public function toArray(): array
    {
        $data = [
            'role' => $this->role,
            'content' => $this->content,
        ];
        
        if ($this->imageUrl !== null) {
            $data['image_url'] = $this->imageUrl;
        }
        
        return $data;
    }
}
