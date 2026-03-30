/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LlmMessage.php                                     ║
  Version:         0.0.36                                             ║
  Last Modified:   2026-03-30 04:04:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
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
