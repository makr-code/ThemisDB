/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ai.php                                             ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:49:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     58                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • fa8becd33  2026-02-16  WordPress plugins: Fix SQL injections, broken PHP, and st... ║
    • c6716ede7  2026-02-16  Add ThemisDB Order Request Plugin with shortcodes, AJAX h... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<!-- AI Section -->
<div class="themisdb-tco-section" 
     data-animation="<?php echo esc_attr($atts['animation']); ?>" 
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
