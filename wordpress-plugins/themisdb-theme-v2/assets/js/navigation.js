/**
 * ThemisDB v2 – Navigation & UI Enhancements
 *
 * Handles:
 * - Mobile menu toggle
 * - Announcement bar dismiss
 * - Scroll-aware header
 * - Code copy buttons
 * - Active nav link highlighting
 *
 * @package ThemisDB_V2
 */
( function () {
	'use strict';

	/* ----------------------------------------------------------
	   Mobile Navigation Toggle
	   ---------------------------------------------------------- */
	document.addEventListener( 'DOMContentLoaded', function () {

		// WP block navigation has its own toggle; we enhance it
		var navToggle = document.querySelector( '.wp-block-navigation__responsive-container-open' );
		var navClose  = document.querySelector( '.wp-block-navigation__responsive-container-close' );

		if ( navToggle ) {
			navToggle.setAttribute( 'aria-label', 'Open navigation menu' );
		}
		if ( navClose ) {
			navClose.setAttribute( 'aria-label', 'Close navigation menu' );
		}

		/* ----------------------------------------------------------
		   Announcement Bar: Dismiss / persist via sessionStorage
		   ---------------------------------------------------------- */
		var announcementBar = document.querySelector( '.themis-announcement-bar' );
		if ( announcementBar ) {
			var dismissed = sessionStorage.getItem( 'themis-announcement-dismissed' );
			if ( dismissed ) {
				announcementBar.style.display = 'none';
			}

			// Add a close button if not already present
			if ( ! announcementBar.querySelector( '.themis-announcement-close' ) ) {
				var closeBtn = document.createElement( 'button' );
				closeBtn.className        = 'themis-announcement-close';
				closeBtn.setAttribute( 'aria-label', 'Dismiss announcement' );
				closeBtn.innerHTML        = '×';
				closeBtn.style.cssText    =
					'background:none;border:none;color:rgba(255,255,255,0.7);font-size:1.25rem;' +
					'cursor:pointer;padding:0 0.5rem;line-height:1;position:absolute;right:1rem;top:50%;transform:translateY(-50%);';
				announcementBar.style.position = 'relative';
				announcementBar.appendChild( closeBtn );
				closeBtn.addEventListener( 'click', function () {
					announcementBar.style.display = 'none';
					sessionStorage.setItem( 'themis-announcement-dismissed', '1' );
				} );
			}
		}

		/* ----------------------------------------------------------
		   Scroll-aware Header: add shadow on scroll
		   ---------------------------------------------------------- */
		var siteHeader = document.querySelector( '.site-header, .themis-header' );
		if ( siteHeader ) {
			var addHeaderShadow = function () {
				if ( window.scrollY > 8 ) {
					siteHeader.classList.add( 'is-scrolled' );
				} else {
					siteHeader.classList.remove( 'is-scrolled' );
				}
			};
			window.addEventListener( 'scroll', addHeaderShadow, { passive: true } );
			addHeaderShadow();
		}

		/* ----------------------------------------------------------
		   Code Copy Buttons
		   ---------------------------------------------------------- */
		var codeBlocks = document.querySelectorAll( 'pre code, .wp-block-code code' );
		codeBlocks.forEach( function ( codeEl ) {
			var pre = codeEl.closest( 'pre' );
			if ( ! pre || pre.querySelector( '.themis-copy-btn' ) ) return;

			var copyBtn = document.createElement( 'button' );
			copyBtn.className  = 'themis-copy-btn';
			copyBtn.textContent = 'Copy';
			copyBtn.setAttribute( 'aria-label', 'Copy code to clipboard' );
			copyBtn.style.cssText =
				'position:absolute;top:0.5rem;right:0.5rem;background:rgba(255,255,255,0.08);' +
				'border:1px solid rgba(255,255,255,0.15);border-radius:4px;color:rgba(255,255,255,0.6);' +
				'font-size:0.6875rem;padding:0.2rem 0.5rem;cursor:pointer;font-family:inherit;' +
				'transition:all 0.15s ease;';
			pre.style.position = 'relative';
			pre.appendChild( copyBtn );

			copyBtn.addEventListener( 'click', function () {
				if ( navigator.clipboard ) {
					navigator.clipboard.writeText( codeEl.textContent ).then( function () {
						copyBtn.textContent = 'Copied!';
						copyBtn.style.color = '#27ae60';
						setTimeout( function () {
							copyBtn.textContent = 'Copy';
							copyBtn.style.color = 'rgba(255,255,255,0.6)';
						}, 2000 );
					} );
				}
			} );
		} );

		/* ----------------------------------------------------------
		   Active Navigation Link Highlight
		   ---------------------------------------------------------- */
		var currentPath = window.location.pathname;
		var navLinks    = document.querySelectorAll( '.site-header .wp-block-navigation-item__content, .site-header .wp-block-navigation-link__content' );
		navLinks.forEach( function ( link ) {
			if ( link.getAttribute( 'href' ) === currentPath ) {
				link.closest( '.wp-block-navigation-item, li' ).classList.add( 'current-nav-item' );
			}
		} );

		/* ----------------------------------------------------------
		   Hover effect for product/docs cards (enhance CSS transitions)
		   ---------------------------------------------------------- */
		var hoverCards = document.querySelectorAll(
			'.themis-product-card, .themis-docs-card, .themis-post-card, .themis-blog-card'
		);
		hoverCards.forEach( function ( card ) {
			card.addEventListener( 'mouseenter', function () {
				this.style.borderColor   = '#3498db';
				this.style.boxShadow     = '0 4px 12px rgba(0,0,0,0.10)';
				this.style.transform     = 'translateY(-2px)';
			} );
			card.addEventListener( 'mouseleave', function () {
				this.style.borderColor   = '';
				this.style.boxShadow     = '';
				this.style.transform     = '';
			} );
		} );

		/* ----------------------------------------------------------
		   Docs "Was this helpful?" feedback
		   ---------------------------------------------------------- */
		var feedbackBtns = document.querySelectorAll( '.themis-docs-feedback button' );
		feedbackBtns.forEach( function ( btn ) {
			btn.addEventListener( 'click', function () {
				var container = this.closest( '.themis-docs-feedback' );
				container.innerHTML =
					'<p style="font-size:0.875rem;color:#27ae60;font-weight:600;">✓ Thanks for your feedback!</p>';
			} );
		} );

	} );

} )();
