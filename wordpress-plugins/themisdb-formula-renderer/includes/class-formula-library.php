<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-formula-library.php                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     275                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */


/**
 * Formula Library
 * 
 * Common mathematical formulas
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Formula_Library {
    
    /**
     * Get common formulas
     */
    public static function get_formulas() {
        return array(
            'algebra' => array(
                'name' => 'Algebra',
                'formulas' => array(
                    array(
                        'name' => 'Quadratic Formula',
                        'latex' => 'x = \frac{-b \pm \sqrt{b^2 - 4ac}}{2a}',
                        'description' => 'Solution to ax² + bx + c = 0'
                    ),
                    array(
                        'name' => 'Binomial Theorem',
                        'latex' => '(x+y)^n = \sum_{k=0}^{n} \binom{n}{k} x^{n-k} y^k',
                        'description' => 'Expansion of (x+y)ⁿ'
                    ),
                    array(
                        'name' => 'Logarithm Properties',
                        'latex' => '\log_b(xy) = \log_b(x) + \log_b(y)',
                        'description' => 'Product rule for logarithms'
                    )
                )
            ),
            'calculus' => array(
                'name' => 'Calculus',
                'formulas' => array(
                    array(
                        'name' => 'Derivative Power Rule',
                        'latex' => '\frac{d}{dx}(x^n) = nx^{n-1}',
                        'description' => 'Power rule for derivatives'
                    ),
                    array(
                        'name' => 'Integration by Parts',
                        'latex' => '\int u \, dv = uv - \int v \, du',
                        'description' => 'Integration by parts formula'
                    ),
                    array(
                        'name' => 'Fundamental Theorem',
                        'latex' => '\int_a^b f(x) \, dx = F(b) - F(a)',
                        'description' => 'Fundamental theorem of calculus'
                    )
                )
            ),
            'statistics' => array(
                'name' => 'Statistics',
                'formulas' => array(
                    array(
                        'name' => 'Mean',
                        'latex' => '\bar{x} = \frac{1}{n}\sum_{i=1}^{n} x_i',
                        'description' => 'Arithmetic mean'
                    ),
                    array(
                        'name' => 'Standard Deviation',
                        'latex' => '\sigma = \sqrt{\frac{1}{n}\sum_{i=1}^{n} (x_i - \mu)^2}',
                        'description' => 'Population standard deviation'
                    ),
                    array(
                        'name' => 'Normal Distribution',
                        'latex' => 'f(x) = \frac{1}{\sigma\sqrt{2\pi}} e^{-\frac{1}{2}\left(\frac{x-\mu}{\sigma}\right)^2}',
                        'description' => 'Probability density function'
                    )
                )
            ),
            'physics' => array(
                'name' => 'Physics',
                'formulas' => array(
                    array(
                        'name' => 'Einstein Mass-Energy',
                        'latex' => 'E = mc^2',
                        'description' => 'Mass-energy equivalence'
                    ),
                    array(
                        'name' => 'Newton\'s Second Law',
                        'latex' => 'F = ma',
                        'description' => 'Force equals mass times acceleration'
                    ),
                    array(
                        'name' => 'Kinetic Energy',
                        'latex' => 'KE = \frac{1}{2}mv^2',
                        'description' => 'Kinetic energy formula'
                    )
                )
            ),
            'geometry' => array(
                'name' => 'Geometry',
                'formulas' => array(
                    array(
                        'name' => 'Pythagorean Theorem',
                        'latex' => 'a^2 + b^2 = c^2',
                        'description' => 'Right triangle relationship'
                    ),
                    array(
                        'name' => 'Circle Area',
                        'latex' => 'A = \pi r^2',
                        'description' => 'Area of a circle'
                    ),
                    array(
                        'name' => 'Sphere Volume',
                        'latex' => 'V = \frac{4}{3}\pi r^3',
                        'description' => 'Volume of a sphere'
                    )
                )
            )
        );
    }
    
    /**
     * Render admin page
     */
    public function render_admin_page() {
        $formulas = self::get_formulas();
        ?>
        <div class="wrap themisdb-formula-library">
            <h1><?php _e('Formula Library', 'themisdb-formula-renderer'); ?></h1>
            <p><?php _e('Click any formula to copy its shortcode.', 'themisdb-formula-renderer'); ?></p>
            
            <?php foreach ($formulas as $category_key => $category): ?>
            <div class="formula-category">
                <h2><?php echo esc_html($category['name']); ?></h2>
                <div class="formula-grid">
                    <?php foreach ($category['formulas'] as $formula): ?>
                    <div class="formula-card">
                        <h3><?php echo esc_html($formula['name']); ?></h3>
                        <p class="formula-description"><?php echo esc_html($formula['description']); ?></p>
                        <div class="formula-preview">
                            <?php echo do_shortcode('[themisdb_formula]' . $formula['latex'] . '[/themisdb_formula]'); ?>
                        </div>
                        <div class="formula-actions">
                            <button class="button button-primary copy-shortcode" 
                                    data-shortcode='[themisdb_formula]<?php echo esc_attr($formula['latex']); ?>[/themisdb_formula]'>
                                📋 Copy Shortcode
                            </button>
                            <button class="button copy-latex" 
                                    data-latex="<?php echo esc_attr($formula['latex']); ?>">
                                Copy LaTeX
                            </button>
                        </div>
                    </div>
                    <?php endforeach; ?>
                </div>
            </div>
            <?php endforeach; ?>
        </div>
        
        <style>
        .themisdb-formula-library {
            max-width: 1400px;
        }
        .formula-category {
            margin: 2rem 0;
        }
        .formula-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
            gap: 1.5rem;
            margin-top: 1rem;
        }
        .formula-card {
            background: white;
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 1.5rem;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            transition: transform 0.2s;
        }
        .formula-card:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 8px rgba(0,0,0,0.15);
        }
        .formula-card h3 {
            margin: 0 0 0.5rem 0;
            color: #2c3e50;
        }
        .formula-description {
            color: #666;
            font-size: 0.9em;
            margin-bottom: 1rem;
        }
        .formula-preview {
            background: #f5f5f5;
            padding: 1rem;
            border-radius: 4px;
            margin-bottom: 1rem;
            min-height: 60px;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .formula-actions {
            display: flex;
            gap: 0.5rem;
        }
        .formula-actions button {
            flex: 1;
        }
        </style>
        
        <script>
        jQuery(document).ready(function($) {
            $('.copy-shortcode, .copy-latex').on('click', function() {
                const text = $(this).data('shortcode') || $(this).data('latex');
                const $btn = $(this);
                
                if (navigator.clipboard && navigator.clipboard.writeText) {
                    navigator.clipboard.writeText(text).then(() => {
                        const originalText = $btn.text();
                        $btn.text('✅ Copied!');
                        setTimeout(() => $btn.text(originalText), 2000);
                    }).catch(err => {
                        console.error('Clipboard copy failed:', err);
                        $btn.text('❌ Failed');
                        setTimeout(() => $btn.text($btn.data('original-text') || 'Copy'), 2000);
                    });
                } else {
                    // Fallback for older browsers
                    const originalText = $btn.text();
                    try {
                        const textarea = document.createElement('textarea');
                        textarea.value = text;
                        textarea.style.position = 'fixed';
                        textarea.style.opacity = '0';
                        document.body.appendChild(textarea);
                        textarea.select();
                        document.execCommand('copy');
                        document.body.removeChild(textarea);
                        $btn.text('✅ Copied!');
                    } catch (err) {
                        console.error('Fallback copy failed:', err);
                        $btn.text('❌ Failed');
                    }
                    setTimeout(() => $btn.text(originalText), 2000);
                }
            });
        });
        </script>
        <?php
    }
}
