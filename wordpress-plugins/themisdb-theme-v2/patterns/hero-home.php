<?php
/**
 * Title: Hero – Home (Azure Style)
 * Slug: themisdb-v2/hero-home
 * Categories: themisdb, themisdb-landing, featured
 * Block Types: core/cover
 * Post Types: wp_template, page
 * Viewport Width: 1280
 * Description: Full-width dark hero section with headline, subtitle, CTA buttons and Docker command.
 */
?>
<!-- wp:cover {"dimRatio":0,"minHeight":620,"minHeightUnit":"px","isDark":true,"gradient":"themis-hero","className":"themis-hero-section","layout":{"type":"constrained","contentSize":"900px"}} -->
<div class="wp-block-cover themis-hero-section is-light" style="min-height:620px">
	<span aria-hidden="true" class="wp-block-cover__background has-background-dim-0 has-background-dim wp-block-cover__gradient-background has-background-gradient" style="background:linear-gradient(160deg,#1a252f 0%,#2c3e50 50%,#34495e 100%)"></span>
	<div class="wp-block-cover__inner-container">
		<!-- wp:spacer {"height":"48px"} --><div style="height:48px" aria-hidden="true" class="wp-block-spacer"></div><!-- /wp:spacer -->
		<!-- wp:heading {"level":1,"textAlign":"center","style":{"typography":{"fontSize":"clamp(2.25rem,5vw,3.75rem)","fontWeight":"800","lineHeight":"1.12","letterSpacing":"-0.04em"},"color":{"text":"#ffffff"}}} -->
		<h1 class="wp-block-heading has-text-align-center" style="color:#ffffff;font-size:clamp(2.25rem,5vw,3.75rem);font-weight:800;line-height:1.12;letter-spacing:-0.04em;">The Database Built for<br>the AI Era</h1>
		<!-- /wp:heading -->
		<!-- wp:paragraph {"textAlign":"center","style":{"typography":{"fontSize":"clamp(1rem,2vw,1.25rem)","lineHeight":"1.65"},"color":{"text":"rgba(255,255,255,0.72)"},"spacing":{"margin":{"top":"var:preset|spacing|5","bottom":"var:preset|spacing|8"}}}} -->
		<p class="has-text-align-center" style="color:rgba(255,255,255,0.72);font-size:clamp(1rem,2vw,1.25rem);line-height:1.65;margin-top:var(--wp--preset--spacing--5);margin-bottom:var(--wp--preset--spacing--8);">ThemisDB combines multi-model storage, native AI/LLM integration, and extreme performance in one unified, open-source database platform.</p>
		<!-- /wp:paragraph -->
		<!-- wp:buttons {"layout":{"type":"flex","justifyContent":"center","flexWrap":"wrap"},"style":{"spacing":{"blockGap":"var:preset|spacing|4"}}} -->
		<div class="wp-block-buttons">
			<!-- wp:button {"style":{"color":{"background":"#3498db","text":"#ffffff"},"border":{"radius":"8px"},"spacing":{"padding":{"top":"0.875rem","bottom":"0.875rem","left":"2rem","right":"2rem"}}},"fontSize":"md"} -->
			<div class="wp-block-button"><a href="/downloads" class="wp-block-button__link wp-element-button has-md-font-size" style="background-color:#3498db;color:#ffffff;border-radius:8px;padding:0.875rem 2rem;font-weight:700">⬇ Download Free</a></div>
			<!-- /wp:button -->
			<!-- wp:button {"className":"is-style-outline","style":{"color":{"text":"rgba(255,255,255,0.9)"},"border":{"radius":"8px","color":"rgba(255,255,255,0.35)","width":"1.5px"},"spacing":{"padding":{"top":"0.875rem","bottom":"0.875rem","left":"2rem","right":"2rem"}}},"fontSize":"md"} -->
			<div class="wp-block-button is-style-outline"><a href="/docs/getting-started" class="wp-block-button__link wp-element-button has-md-font-size" style="color:rgba(255,255,255,0.9);border:1.5px solid rgba(255,255,255,0.35);border-radius:8px;padding:0.875rem 2rem">View Documentation →</a></div>
			<!-- /wp:button -->
		</div>
		<!-- /wp:buttons -->
		<!-- wp:html -->
		<div style="text-align:center;max-width:520px;margin:2rem auto 0;">
			<div style="display:inline-flex;align-items:center;gap:0.75rem;background:rgba(0,0,0,0.35);border:1px solid rgba(255,255,255,0.12);border-radius:10px;padding:0.625rem 1.25rem;">
				<span style="color:#7c4dff;">$</span>
				<code style="font-family:'SFMono-Regular',Consolas,monospace;font-size:0.875rem;color:#e2e8f0;">docker pull themisdb/themisdb:latest</code>
			</div>
		</div>
		<!-- /wp:html -->
		<!-- wp:spacer {"height":"48px"} --><div style="height:48px" aria-hidden="true" class="wp-block-spacer"></div><!-- /wp:spacer -->
	</div>
</div>
<!-- /wp:cover -->
