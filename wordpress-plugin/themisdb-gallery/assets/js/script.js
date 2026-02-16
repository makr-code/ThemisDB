/**
 * ThemisDB Gallery - Frontend JavaScript
 */

(function($) {
    'use strict';
    
    $(document).ready(function() {
        
        // Frontend image search
        $('.themisdb-search-form').on('submit', function(e) {
            e.preventDefault();
            
            var $form = $(this);
            var $widget = $form.closest('.themisdb-image-search-widget');
            var query = $widget.find('.themisdb-search-input').val();
            var provider = $widget.find('.themisdb-provider-select').val();
            var $results = $widget.find('.themisdb-search-results');
            var $btn = $widget.find('.themisdb-search-button');
            
            if (!query) {
                return;
            }
            
            $btn.prop('disabled', true).text('Suche läuft...');
            $results.html('<div class="themisdb-loading"></div>');
            
            $.ajax({
                url: themisdbGallery.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_gallery_frontend_search',
                    nonce: themisdbGallery.nonce,
                    query: query,
                    provider: provider
                },
                success: function(response) {
                    $btn.prop('disabled', false).text('Suchen');
                    
                    if (response.success && response.data.html) {
                        $results.html(response.data.html);
                    } else {
                        $results.html('<div class="themisdb-error">Keine Bilder gefunden</div>');
                    }
                },
                error: function() {
                    $btn.prop('disabled', false).text('Suchen');
                    $results.html('<div class="themisdb-error">Fehler beim Laden der Bilder</div>');
                }
            });
        });
        
        // Simple lightbox for gallery images
        $('body').on('click', '.themisdb-gallery-item a[data-lightbox]', function(e) {
            e.preventDefault();
            
            var imageUrl = $(this).attr('href');
            var imageTitle = $(this).find('img').attr('alt');
            
            // Create simple lightbox overlay
            var lightbox = $('<div class="themisdb-lightbox">' +
                '<div class="themisdb-lightbox-overlay"></div>' +
                '<div class="themisdb-lightbox-content">' +
                '<img src="' + imageUrl + '" alt="' + imageTitle + '" />' +
                '<button class="themisdb-lightbox-close">&times;</button>' +
                '</div>' +
                '</div>');
            
            $('body').append(lightbox);
            lightbox.fadeIn(200);
            
            // Close lightbox
            lightbox.find('.themisdb-lightbox-close, .themisdb-lightbox-overlay').on('click', function() {
                lightbox.fadeOut(200, function() {
                    lightbox.remove();
                });
            });
            
            // Close on Escape key
            $(document).on('keydown.lightbox', function(e) {
                if (e.keyCode === 27) {
                    lightbox.fadeOut(200, function() {
                        lightbox.remove();
                    });
                    $(document).off('keydown.lightbox');
                }
            });
        });
        
    });
    
})(jQuery);
