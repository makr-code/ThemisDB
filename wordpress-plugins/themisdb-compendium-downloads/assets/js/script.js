/**
 * ThemisDB Compendium Downloads - JavaScript
 */

(function($) {
    'use strict';
    
    // Debounce helper to prevent rapid-fire requests
    var downloadTrackingQueue = {};
    var trackingDebounceDelay = 1000; // 1 second
    
    /**
     * Track download clicks (debounced)
     */
    function trackDownload(assetName) {
        if (!themisdbCompendium.ajaxUrl || !themisdbCompendium.nonce) {
            return;
        }
        
        // Clear existing timeout for this asset
        if (downloadTrackingQueue[assetName]) {
            clearTimeout(downloadTrackingQueue[assetName]);
        }
        
        // Set new timeout
        downloadTrackingQueue[assetName] = setTimeout(function() {
            $.ajax({
                url: themisdbCompendium.ajaxUrl,
                type: 'POST',
                data: {
                    action: 'themisdb_track_download',
                    nonce: themisdbCompendium.nonce,
                    asset: assetName
                },
                success: function(response) {
                    if (response.success && window.themisdbDebug) {
                        console.log('Download tracked:', assetName, response.data);
                    }
                },
                error: function(xhr, status, error) {
                    if (window.themisdbDebug) {
                        console.error('Failed to track download:', error);
                    }
                }
            });
            
            delete downloadTrackingQueue[assetName];
        }, trackingDebounceDelay);
    }
    
    /**
     * Initialize download tracking
     */
    function initDownloadTracking() {
        $('.themisdb-download-button').on('click', function() {
            var assetName = $(this).data('asset');
            if (assetName) {
                trackDownload(assetName);
            }
        });
    }
    
    /**
     * Add smooth hover effects
     */
    function initHoverEffects() {
        $('.themisdb-compendium-item').hover(
            function() {
                $(this).addClass('is-hovered');
            },
            function() {
                $(this).removeClass('is-hovered');
            }
        );
    }
    
    /**
     * Initialize tooltips (if needed)
     */
    function initTooltips() {
        // Add tooltips for file sizes or other information
        $('.themisdb-item-size').each(function() {
            var $this = $(this);
            var title = $this.text();
            $this.attr('title', 'Dateigröße: ' + title);
        });
    }
    
    /**
     * Handle responsive layout adjustments
     */
    function handleResponsiveLayout() {
        var windowWidth = $(window).width();
        
        // Adjust layout for mobile devices
        if (windowWidth < 768) {
            $('.themisdb-compendium-downloads').addClass('is-mobile');
        } else {
            $('.themisdb-compendium-downloads').removeClass('is-mobile');
        }
    }
    
    /**
     * Initialize on document ready
     */
    $(document).ready(function() {
        initDownloadTracking();
        initHoverEffects();
        initTooltips();
        handleResponsiveLayout();
        
        // Update layout on window resize
        $(window).on('resize', function() {
            handleResponsiveLayout();
        });
        
        // Log initialization (only in debug mode)
        if (window.themisdbDebug) {
            console.log('ThemisDB Compendium Downloads initialized');
        }
    });
    
    /**
     * Re-initialize for dynamic content (AJAX, infinite scroll, etc.)
     */
    $(document).on('contentLoaded ajaxComplete', function() {
        initDownloadTracking();
        initHoverEffects();
        initTooltips();
    });
    
})(jQuery);
