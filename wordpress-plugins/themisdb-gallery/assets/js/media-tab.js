/**
 * ThemisDB Gallery - Media Library Tab JavaScript
 */

(function($) {
    'use strict';
    
    $(document).ready(function() {
        
        // Search images
        $('#themisdb-gallery-media-search-btn').on('click', function() {
            var query = $('#themisdb-gallery-media-search-input').val();
            var provider = $('#themisdb-gallery-media-provider').val();
            
            if (!query) {
                alert(themisdbGalleryMediaTab.searchRequired);
                return;
            }
            
            searchImages(query, provider);
        });
        
        // Allow Enter key to trigger search
        $('#themisdb-gallery-media-search-input').on('keypress', function(e) {
            if (e.which === 13) {
                $('#themisdb-gallery-media-search-btn').click();
                return false;
            }
        });
        
        // Generate AI image
        $('#themisdb-gallery-media-ai-btn').on('click', function() {
            var prompt = $('#themisdb-gallery-media-ai-prompt').val();
            
            if (!prompt) {
                alert(themisdbGalleryMediaTab.aiPromptRequired);
                return;
            }
            
            generateAIImage(prompt);
        });
        
        // Allow Enter key to trigger AI generation
        $('#themisdb-gallery-media-ai-prompt').on('keypress', function(e) {
            if (e.which === 13) {
                $('#themisdb-gallery-media-ai-btn').click();
                return false;
            }
        });
        
        /**
         * Search for images
         */
        function searchImages(query, provider) {
            var $results = $('#themisdb-gallery-media-results');
            var $btn = $('#themisdb-gallery-media-search-btn');
            
            $btn.prop('disabled', true).text(themisdbGalleryMediaTab.searching);
            $results.html('<div class="themisdb-media-loading" style="text-align: center; padding: 40px; color: #666;"><span class="spinner is-active" style="float: none; margin: 0 auto;"></span><br>' + themisdbGalleryMediaTab.searching + '</div>');
            
            $.ajax({
                url: themisdbGalleryMediaTab.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_gallery_search',
                    nonce: themisdbGalleryMediaTab.nonce,
                    query: query,
                    provider: provider
                },
                success: function(response) {
                    $btn.prop('disabled', false).text(themisdbGalleryMediaTab.searchBtn);
                    
                    if (response.success && response.data.images) {
                        displayResults(response.data.images);
                    } else {
                        $results.html('<div class="themisdb-media-error" style="text-align: center; padding: 40px; color: #a00;">' + (response.data.message || themisdbGalleryMediaTab.noResults) + '</div>');
                    }
                },
                error: function() {
                    $btn.prop('disabled', false).text(themisdbGalleryMediaTab.searchBtn);
                    $results.html('<div class="themisdb-media-error" style="text-align: center; padding: 40px; color: #a00;">' + themisdbGalleryMediaTab.error + '</div>');
                }
            });
        }
        
        /**
         * Generate AI image
         */
        function generateAIImage(prompt) {
            var $results = $('#themisdb-gallery-media-results');
            var $btn = $('#themisdb-gallery-media-ai-btn');
            
            $btn.prop('disabled', true).text(themisdbGalleryMediaTab.generatingAI);
            $results.html('<div class="themisdb-media-loading" style="text-align: center; padding: 40px; color: #666;"><span class="spinner is-active" style="float: none; margin: 0 auto;"></span><br>' + themisdbGalleryMediaTab.generatingAI + '</div>');
            
            $.ajax({
                url: themisdbGalleryMediaTab.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_gallery_generate_ai',
                    nonce: themisdbGalleryMediaTab.nonce,
                    prompt: prompt
                },
                success: function(response) {
                    $btn.prop('disabled', false).text(themisdbGalleryMediaTab.generateAIBtn);
                    
                    if (response.success && response.data.image) {
                        displayResults([response.data.image]);
                    } else {
                        $results.html('<div class="themisdb-media-error" style="text-align: center; padding: 40px; color: #a00;">' + (response.data.message || themisdbGalleryMediaTab.error) + '</div>');
                    }
                },
                error: function() {
                    $btn.prop('disabled', false).text(themisdbGalleryMediaTab.generateAIBtn);
                    $results.html('<div class="themisdb-media-error" style="text-align: center; padding: 40px; color: #a00;">' + themisdbGalleryMediaTab.error + '</div>');
                }
            });
        }
        
        /**
         * Display search results
         */
        function displayResults(images) {
            var $results = $('#themisdb-gallery-media-results');
            
            if (!images || images.length === 0) {
                $results.html('<div class="themisdb-media-error" style="text-align: center; padding: 40px; color: #a00;">' + themisdbGalleryMediaTab.noResults + '</div>');
                return;
            }
            
            var html = '<div class="themisdb-media-image-grid" style="display: grid; grid-template-columns: repeat(auto-fill, minmax(150px, 1fr)); gap: 15px; padding: 10px;">';
            
            images.forEach(function(image) {
                html += '<div class="themisdb-media-image-item" data-image=\'' + JSON.stringify(image) + '\' style="position: relative; border: 1px solid #ddd; border-radius: 4px; overflow: hidden; cursor: pointer; transition: all 0.2s;">';
                html += '<img src="' + escapeHtml(image.thumb) + '" alt="' + escapeHtml(image.title) + '" style="width: 100%; height: 150px; object-fit: cover; display: block;" />';
                html += '<div class="themisdb-media-image-overlay" style="position: absolute; bottom: 0; left: 0; right: 0; background: rgba(0,0,0,0.8); color: white; padding: 8px; font-size: 11px; opacity: 0; transition: opacity 0.2s;">';
                html += '<div style="font-weight: bold; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;">' + escapeHtml(image.source) + '</div>';
                html += '<div style="white-space: nowrap; overflow: hidden; text-overflow: ellipsis;">' + escapeHtml(image.author) + '</div>';
                html += '</div>';
                html += '<button class="themisdb-insert-media-image-btn button button-primary" style="position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); opacity: 0; transition: opacity 0.2s; z-index: 10;">' + themisdbGalleryMediaTab.insertImage + '</button>';
                html += '</div>';
            });
            
            html += '</div>';
            $results.html(html);
            
            // Add hover effects
            $('.themisdb-media-image-item').hover(
                function() {
                    $(this).css('box-shadow', '0 4px 8px rgba(0,0,0,0.2)');
                    $(this).find('.themisdb-media-image-overlay, .themisdb-insert-media-image-btn').css('opacity', '1');
                },
                function() {
                    $(this).css('box-shadow', 'none');
                    $(this).find('.themisdb-media-image-overlay, .themisdb-insert-media-image-btn').css('opacity', '0');
                }
            );
            
            // Attach click handlers
            $('.themisdb-insert-media-image-btn').on('click', function(e) {
                e.stopPropagation();
                var $item = $(this).closest('.themisdb-media-image-item');
                var imageData = $item.data('image');
                insertImageToMedia(imageData, $(this));
            });
        }
        
        /**
         * Insert image into WordPress media library and then into post
         */
        function insertImageToMedia(imageData, $btn) {
            $btn.prop('disabled', true).text(themisdbGalleryMediaTab.downloading);
            
            $.ajax({
                url: themisdbGalleryMediaTab.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_gallery_import_image',
                    nonce: themisdbGalleryMediaTab.nonce,
                    image_data: JSON.stringify(imageData),
                    post_id: 0 // Import to media library without attaching to specific post
                },
                success: function(response) {
                    if (response.success && response.data.attachment_id) {
                        // Send image data back to parent window (media modal)
                        if (window.parent && window.parent.tb_remove) {
                            // For classic media modal (thickbox)
                            var attachmentId = response.data.attachment_id;
                            var attachmentUrl = response.data.url;
                            var attachmentThumb = response.data.thumb;
                            
                            // Try to trigger the insert into editor
                            if (window.parent.send_to_editor) {
                                var html = '<img src="' + escapeHtml(attachmentUrl) + '" alt="' + escapeHtml(imageData.title) + '" />';
                                if (response.data.attribution) {
                                    // Attribution is already escaped server-side by generate_attribution_text()
                                    html = '<figure>' + html + '<figcaption>' + response.data.attribution + '</figcaption></figure>';
                                }
                                window.parent.send_to_editor(html);
                            }
                            
                            // Close modal
                            window.parent.tb_remove();
                        }
                        
                        $btn.text('✓ ' + themisdbGalleryMediaTab.imported);
                        
                    } else {
                        alert(response.data.message || themisdbGalleryMediaTab.error);
                        $btn.prop('disabled', false).text(themisdbGalleryMediaTab.insertImage);
                    }
                },
                error: function() {
                    alert(themisdbGalleryMediaTab.error);
                    $btn.prop('disabled', false).text(themisdbGalleryMediaTab.insertImage);
                }
            });
        }
        
        /**
         * Escape HTML
         */
        function escapeHtml(text) {
            var map = {
                '&': '&amp;',
                '<': '&lt;',
                '>': '&gt;',
                '"': '&quot;',
                "'": '&#039;'
            };
            return (text || '').replace(/[&<>"']/g, function(m) { return map[m]; });
        }
    });
    
})(jQuery);
