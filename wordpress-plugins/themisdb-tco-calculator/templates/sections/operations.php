<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            operations.php                                     ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     56                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<!-- Operations Section -->
<div class="themisdb-tco-section" 
     data-animation=" echo esc_attr($atts['animation']); ?>" 
     data-delay="<?php echo esc_attr($atts['delay']); ?>"
     style="transform: scale(<?php echo esc_attr($atts['scale']); ?>); transform-origin: center;">
    <div class="parameter-group">
        <h3 class="group-title">🔧 Betrieb & Support</h3>
        <div class="form-grid">
            <div class="form-group slider-group">
                <label for="trainingCost">
                    <span class="label-text">Schulungskosten (€/Jahr)</span>
                    <span class="label-info" title="Jährliche Kosten für Team-Schulungen">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="trainingCost" class="slider" value="20000" min="0" max="100000" step="1000">
                    <output for="trainingCost" id="trainingCost-value" class="slider-value">€20.000</output>
                </div>
                <small>€0 bis €100.000</small>
            </div>

            <div class="form-group slider-group">
                <label for="supportCost">
                    <span class="label-text">Support-Kosten (€/Jahr)</span>
                    <span class="label-info" title="Jährliche Enterprise-Support-Kosten">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="supportCost" class="slider" value="50000" min="0" max="200000" step="5000">
                    <output for="supportCost" id="supportCost-value" class="slider-value">€50.000</output>
                </div>
                <small>€0 bis €200.000</small>
            </div>
        </div>
    </div>
</div>
