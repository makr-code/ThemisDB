<?php
/**
 * Title: CTA – Download Section
 * Slug: themisdb-v2/cta-download
 * Categories: themisdb, themisdb-landing
 * Viewport Width: 1280
 * Description: Three-column download options: Docker, binary releases, and compendium bundle.
 */
?>
<!-- wp:group {"style":{"color":{"background":"#2c3e50"},"spacing":{"padding":{"top":"var:preset|spacing|20","bottom":"var:preset|spacing|20"}}},"layout":{"type":"constrained","contentSize":"1100px"}} -->
<div class="wp-block-group" style="background-color:#2c3e50;padding-top:var(--wp--preset--spacing--20);padding-bottom:var(--wp--preset--spacing--20)">
	<!-- wp:heading {"level":2,"textAlign":"center","style":{"typography":{"fontSize":"clamp(1.75rem,3vw,2.5rem)","fontWeight":"700","letterSpacing":"-0.02em"},"color":{"text":"#ffffff"},"spacing":{"margin":{"bottom":"var:preset|spacing|4"}}}} -->
	<h2 class="wp-block-heading has-text-align-center" style="color:#ffffff;font-size:clamp(1.75rem,3vw,2.5rem);font-weight:700;letter-spacing:-0.02em;margin-bottom:var(--wp--preset--spacing--4)">Get ThemisDB</h2>
	<!-- /wp:heading -->
	<!-- wp:paragraph {"textAlign":"center","style":{"typography":{"fontSize":"1.0625rem"},"color":{"text":"rgba(255,255,255,0.6)"},"spacing":{"margin":{"bottom":"var:preset|spacing|12"}}}} -->
	<p class="has-text-align-center" style="color:rgba(255,255,255,0.6);font-size:1.0625rem;margin-bottom:var(--wp--preset--spacing--12)">Multiple installation options — choose what works for your stack.</p>
	<!-- /wp:paragraph -->
	<!-- wp:columns {"style":{"spacing":{"blockGap":{"left":"var:preset|spacing|6"}}}} -->
	<div class="wp-block-columns">
		<!-- wp:column {"style":{"color":{"background":"rgba(255,255,255,0.06)"},"border":{"radius":"10px","color":"rgba(255,255,255,0.1)","width":"1px"},"spacing":{"padding":{"top":"var:preset|spacing|8","bottom":"var:preset|spacing|8","left":"var:preset|spacing|8","right":"var:preset|spacing|8"}}}} -->
		<div class="wp-block-column" style="background-color:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.1);border-radius:10px;padding:var(--wp--preset--spacing--8)">
			<!-- wp:heading {"level":3,"style":{"typography":{"fontSize":"1.125rem","fontWeight":"700"},"color":{"text":"#ffffff"},"spacing":{"margin":{"bottom":"var:preset|spacing|3"}}}} -->
			<h3 class="wp-block-heading" style="color:#ffffff;font-size:1.125rem;font-weight:700;margin-bottom:var(--wp--preset--spacing--3)">🐳 Docker (Recommended)</h3>
			<!-- /wp:heading -->
			<!-- wp:paragraph {"style":{"typography":{"fontSize":"0.875rem"},"color":{"text":"rgba(255,255,255,0.55)"},"spacing":{"margin":{"bottom":"var:preset|spacing|5"}}}} -->
			<p style="color:rgba(255,255,255,0.55);font-size:0.875rem;margin-bottom:var(--wp--preset--spacing--5)">Zero-config setup. Run ThemisDB in seconds.</p>
			<!-- /wp:paragraph -->
			<!-- wp:shortcode -->[themisdb_docker_latest]<!-- /wp:shortcode -->
			<!-- wp:spacer {"height":"12px"} --><div style="height:12px" aria-hidden="true" class="wp-block-spacer"></div><!-- /wp:spacer -->
			<!-- wp:buttons -->
			<div class="wp-block-buttons">
				<!-- wp:button {"style":{"color":{"background":"#0db7ed","text":"#ffffff"},"border":{"radius":"6px"},"spacing":{"padding":{"top":"0.5rem","bottom":"0.5rem","left":"1.25rem","right":"1.25rem"}}},"fontSize":"sm"} -->
				<div class="wp-block-button"><a href="/docker" class="wp-block-button__link wp-element-button has-sm-font-size" style="background-color:#0db7ed;color:#ffffff;border-radius:6px;padding:0.5rem 1.25rem;font-weight:600">All Docker Tags →</a></div>
				<!-- /wp:button -->
			</div>
			<!-- /wp:buttons -->
		</div><!-- /wp:column -->
		<!-- wp:column {"style":{"color":{"background":"rgba(255,255,255,0.06)"},"border":{"radius":"10px","color":"rgba(255,255,255,0.1)","width":"1px"},"spacing":{"padding":{"top":"var:preset|spacing|8","bottom":"var:preset|spacing|8","left":"var:preset|spacing|8","right":"var:preset|spacing|8"}}}} -->
		<div class="wp-block-column" style="background-color:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.1);border-radius:10px;padding:var(--wp--preset--spacing--8)">
			<!-- wp:heading {"level":3,"style":{"typography":{"fontSize":"1.125rem","fontWeight":"700"},"color":{"text":"#ffffff"},"spacing":{"margin":{"bottom":"var:preset|spacing|3"}}}} -->
			<h3 class="wp-block-heading" style="color:#ffffff;font-size:1.125rem;font-weight:700;margin-bottom:var(--wp--preset--spacing--3)">📦 Binary Releases</h3>
			<!-- /wp:heading -->
			<!-- wp:paragraph {"style":{"typography":{"fontSize":"0.875rem"},"color":{"text":"rgba(255,255,255,0.55)"},"spacing":{"margin":{"bottom":"var:preset|spacing|5"}}}} -->
			<p style="color:rgba(255,255,255,0.55);font-size:0.875rem;margin-bottom:var(--wp--preset--spacing--5)">Linux, macOS, Windows. Latest stable release from GitHub.</p>
			<!-- /wp:paragraph -->
			<!-- wp:shortcode -->[themisdb_latest]<!-- /wp:shortcode -->
			<!-- wp:spacer {"height":"12px"} --><div style="height:12px" aria-hidden="true" class="wp-block-spacer"></div><!-- /wp:spacer -->
			<!-- wp:buttons -->
			<div class="wp-block-buttons">
				<!-- wp:button {"style":{"color":{"background":"#3498db","text":"#ffffff"},"border":{"radius":"6px"},"spacing":{"padding":{"top":"0.5rem","bottom":"0.5rem","left":"1.25rem","right":"1.25rem"}}},"fontSize":"sm"} -->
				<div class="wp-block-button"><a href="/downloads" class="wp-block-button__link wp-element-button has-sm-font-size" style="background-color:#3498db;color:#ffffff;border-radius:6px;padding:0.5rem 1.25rem;font-weight:600">All Releases →</a></div>
				<!-- /wp:button -->
			</div>
			<!-- /wp:buttons -->
		</div><!-- /wp:column -->
		<!-- wp:column {"style":{"color":{"background":"rgba(255,255,255,0.06)"},"border":{"radius":"10px","color":"rgba(255,255,255,0.1)","width":"1px"},"spacing":{"padding":{"top":"var:preset|spacing|8","bottom":"var:preset|spacing|8","left":"var:preset|spacing|8","right":"var:preset|spacing|8"}}}} -->
		<div class="wp-block-column" style="background-color:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.1);border-radius:10px;padding:var(--wp--preset--spacing--8)">
			<!-- wp:heading {"level":3,"style":{"typography":{"fontSize":"1.125rem","fontWeight":"700"},"color":{"text":"#ffffff"},"spacing":{"margin":{"bottom":"var:preset|spacing|3"}}}} -->
			<h3 class="wp-block-heading" style="color:#ffffff;font-size:1.125rem;font-weight:700;margin-bottom:var(--wp--preset--spacing--3)">📘 Compendium Bundle</h3>
			<!-- /wp:heading -->
			<!-- wp:paragraph {"style":{"typography":{"fontSize":"0.875rem"},"color":{"text":"rgba(255,255,255,0.55)"},"spacing":{"margin":{"bottom":"var:preset|spacing|5"}}}} -->
			<p style="color:rgba(255,255,255,0.55);font-size:0.875rem;margin-bottom:var(--wp--preset--spacing--5)">Full docs, examples, and tooling in one download.</p>
			<!-- /wp:paragraph -->
			<!-- wp:shortcode -->[themisdb_compendium_downloads]<!-- /wp:shortcode -->
		</div><!-- /wp:column -->
	</div>
	<!-- /wp:columns -->
</div>
<!-- /wp:group -->
