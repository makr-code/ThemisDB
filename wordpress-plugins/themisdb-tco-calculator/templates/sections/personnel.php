<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            personnel.php                                      ║
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

<!-- Personnel Section -->
<div class="themisdb-tco-section" 
     data-animation=" echo esc_attr($atts['animation']); ?>" 
     data-delay="<?php echo esc_attr($atts['delay']); ?>"
     style="transform: scale(<?php echo esc_attr($atts['scale']); ?>); transform-origin: center;">
    <div class="parameter-group">
        <h3 class="group-title">👥 Personal & Team</h3>
        <div class="form-grid">
            <div class="form-group slider-group">
                <label for="dbaCount">
                    <span class="label-text">Anzahl DBAs</span>
                    <span class="label-info" title="Anzahl Vollzeit-Datenbankadministratoren">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="dbaCount" class="slider" value="2" min="0" max="10" step="0.5">
                    <output for="dbaCount" id="dbaCount-value" class="slider-value">2 FTE</output>
                </div>
                <small>0 bis 10 FTE</small>
            </div>

            <div class="form-group slider-group">
                <label for="dbaSalary">
                    <span class="label-text">DBA Gehalt (€/Jahr)</span>
                    <span class="label-info" title="Durchschnittliches Jahresgehalt pro DBA">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="dbaSalary" class="slider" value="85000" min="40000" max="150000" step="5000">
                    <output for="dbaSalary" id="dbaSalary-value" class="slider-value">€85.000</output>
                </div>
                <small>€40.000 bis €150.000</small>
            </div>

            <div class="form-group slider-group">
                <label for="devCount">
                    <span class="label-text">Anzahl Entwickler</span>
                    <span class="label-info" title="Entwickler für Datenbankintegration">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="devCount" class="slider" value="5" min="0" max="20" step="0.5">
                    <output for="devCount" id="devCount-value" class="slider-value">5 FTE</output>
                </div>
                <small>0 bis 20 FTE</small>
            </div>

            <div class="form-group slider-group">
                <label for="devSalary">
                    <span class="label-text">Dev Gehalt (€/Jahr)</span>
                    <span class="label-info" title="Durchschnittliches Jahresgehalt pro Entwickler">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="devSalary" class="slider" value="75000" min="35000" max="130000" step="5000">
                    <output for="devSalary" id="devSalary-value" class="slider-value">€75.000</output>
                </div>
                <small>€35.000 bis €130.000</small>
            </div>
        </div>
    </div>
</div>
