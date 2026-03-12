<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            infrastructure.php                                 ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     80                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<!-- Infrastructure Section -->
<div class="themisdb-tco-section" 
     data-animation=" echo esc_attr($atts['animation']); ?>" 
     data-delay="<?php echo esc_attr($atts['delay']); ?>"
     style="transform: scale(<?php echo esc_attr($atts['scale']); ?>); transform-origin: center;">
    <div class="parameter-group">
        <h3 class="group-title">🖥️ Infrastruktur & Hardware</h3>
        <div class="form-grid">
            <div class="form-group slider-group">
                <label for="serverCost">
                    <span class="label-text">Server-Kosten (€/Monat)</span>
                    <span class="label-info" title="Monatliche Kosten pro Server (Cloud oder On-Premise)">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="serverCost" class="slider" value="500" min="100" max="2000" step="50">
                    <output for="serverCost" id="serverCost-value" class="slider-value">€500</output>
                </div>
                <small>€100 bis €2.000 pro Server</small>
            </div>

            <div class="form-group slider-group">
                <label for="storageCostPerGB">
                    <span class="label-text">Speicher-Kosten (€/GB/Monat)</span>
                    <span class="label-info" title="Monatliche Kosten pro GB Speicher">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="storageCostPerGB" class="slider" value="0.10" min="0.01" max="1" step="0.01">
                    <output for="storageCostPerGB" id="storageCostPerGB-value" class="slider-value">€0.10</output>
                </div>
                <small>€0.01 bis €1.00 pro GB</small>
            </div>

            <div class="form-group slider-group">
                <label for="networkCost">
                    <span class="label-text">Netzwerk-Kosten (€/TB)</span>
                    <span class="label-info" title="Kosten für ausgehenden Datenverkehr pro TB">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="networkCost" class="slider" value="50" min="10" max="200" step="10">
                    <output for="networkCost" id="networkCost-value" class="slider-value">€50</output>
                </div>
                <small>€10 bis €200 pro TB</small>
            </div>

            <div class="form-group slider-group">
                <label for="backupCost">
                    <span class="label-text">Backup-Kosten (€/GB/Monat)</span>
                    <span class="label-info" title="Monatliche Kosten für Backup-Speicher">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="backupCost" class="slider" value="0.05" min="0.01" max="0.5" step="0.01">
                    <output for="backupCost" id="backupCost-value" class="slider-value">€0.05</output>
                </div>
                <small>€0.01 bis €0.50 pro GB</small>
            </div>
        </div>
    </div>
</div>
