<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ai.php                                             ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<!-- AI Section -->
<div class="themisdb-tco-section" 
     data-animation=" echo esc_attr($atts['animation']); ?>" 
     data-delay="<?php echo esc_attr($atts['delay']); ?>"
     style="transform: scale(<?php echo esc_attr($atts['scale']); ?>); transform-origin: center;">
    <div class="parameter-group">
        <h3 class="group-title">🤖 AI & LLM Features</h3>
        <div class="form-grid">
            <div class="form-group">
                <label for="useAI">
                    <span class="label-text">AI/LLM Features nutzen?</span>
                    <span class="label-info" title="Native LLM-Integration mit llama.cpp">ℹ️</span>
                </label>
                <select id="useAI">
                    <option value="false">Nein</option>
                    <option value="true">Ja (inkl. GPU)</option>
                </select>
            </div>

            <div class="form-group slider-group">
                <label for="aiApiCost">
                    <span class="label-text">Externe AI API Kosten (€/Monat)</span>
                    <span class="label-info" title="Monatliche Kosten für externe AI APIs (OpenAI, etc.)">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="aiApiCost" class="slider" value="5000" min="0" max="20000" step="500">
                    <output for="aiApiCost" id="aiApiCost-value" class="slider-value">€5.000</output>
                </div>
                <small>€0 bis €20.000</small>
            </div>
        </div>
    </div>
</div>
