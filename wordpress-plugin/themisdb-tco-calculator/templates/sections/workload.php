/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            workload.php                                       ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:05:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     85                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • fa8becd33  2026-02-16  WordPress plugins: Fix SQL injections, broken PHP, and st... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<!-- Workload Section -->
<div class="themisdb-tco-section" 
     data-animation="<?php echo esc_attr($atts['animation']); ?>" 
     data-delay="<?php echo esc_attr($atts['delay']); ?>"
     style="transform: scale(<?php echo esc_attr($atts['scale']); ?>); transform-origin: center;">
    <div class="parameter-group">
        <h3 class="group-title">📊 Workload & Anforderungen</h3>
        <div class="form-grid">
            <div class="form-group slider-group">
                <label for="requestsPerDay">
                    <span class="label-text">Anfragen pro Tag</span>
                    <span class="label-info" title="Durchschnittliche Anzahl von Datenbank-Anfragen pro Tag">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="requestsPerDay" class="slider" value="1000000" min="1000" max="10000000" step="10000">
                    <output for="requestsPerDay" id="requestsPerDay-value" class="slider-value">1.000.000</output>
                </div>
                <small>1 Tausend bis 10 Millionen Anfragen/Tag</small>
            </div>

            <div class="form-group slider-group">
                <label for="dataSize">
                    <span class="label-text">Datenmenge (GB)</span>
                    <span class="label-info" title="Geschätzte Gesamtgröße Ihrer Daten">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="dataSize" class="slider" value="500" min="10" max="10000" step="10">
                    <output for="dataSize" id="dataSize-value" class="slider-value">500 GB</output>
                </div>
                <small>10 GB bis 10 TB</small>
            </div>

            <div class="form-group slider-group">
                <label for="peakLoad">
                    <span class="label-text">Spitzenlast-Faktor</span>
                    <span class="label-info" title="Verhältnis von Spitzenlast zu Durchschnittslast">ℹ️</span>
                </label>
                <div class="slider-container">
                    <input type="range" id="peakLoad" class="slider" value="3" min="1" max="10" step="0.5">
                    <output for="peakLoad" id="peakLoad-value" class="slider-value">3x</output>
                </div>
                <small>1x bis 10x der Durchschnittslast</small>
            </div>

            <div class="form-group">
                <label for="availability">
                    <span class="label-text">Verfügbarkeitsanforderung</span>
                    <span class="label-info" title="Benötigte Systemverfügbarkeit">ℹ️</span>
                </label>
                <select id="availability">
                    <option value="99">99% (Standard)</option>
                    <option value="99.9">99.9% (High)</option>
                    <option value="99.99">99.99% (Very High)</option>
                    <option value="99.999" selected>99.999% (Mission Critical)</option>
                </select>
            </div>
        </div>
    </div>
</div>
