/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            personnel.php                                      ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:23:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     84                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<!-- Personnel Section -->
<div class="themisdb-tco-section" 
     data-animation="<?php echo esc_attr($atts['animation']); ?>" 
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
