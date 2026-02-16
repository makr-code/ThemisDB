/**
 * ThemisDB Gallery - Admin JavaScript
 */

(function($) {
    'use strict';
    
    $(document).ready(function() {
        
        // Search images
        $('#themisdb-gallery-search-btn').on('click', function() {
            var query = $('#themisdb-gallery-search-input').val();
            var provider = $('#themisdb-gallery-provider').val();
            
            if (!query) {
                alert(themisdbGalleryAdmin.searchPlaceholder);
                return;
            }
            
            searchImages(query, provider);
        });
        
        // Allow Enter key to trigger search
        $('#themisdb-gallery-search-input').on('keypress', function(e) {
            if (e.which === 13) {
                $('#themisdb-gallery-search-btn').click();
                return false;
            }
        });
        
        // Generate AI image
        $('#themisdb-gallery-ai-btn').on('click', function() {
            var prompt = $('#themisdb-gallery-ai-prompt').val();
            
            if (!prompt) {
                alert('Bitte geben Sie eine Bildbeschreibung ein');
                return;
            }
            
            generateAIImage(prompt);
        });
        
        // Allow Enter key to trigger AI generation
        $('#themisdb-gallery-ai-prompt').on('keypress', function(e) {
            if (e.which === 13) {
                $('#themisdb-gallery-ai-btn').click();
                return false;
            }
        });
        
        /**
         * Search for images
         */
        function searchImages(query, provider) {
            var $results = $('#themisdb-gallery-results');
            var $btn = $('#themisdb-gallery-search-btn');
            
            $btn.prop('disabled', true).text(themisdbGalleryAdmin.searching);
            $results.html('<div class="themisdb-admin-loading">' + themisdbGalleryAdmin.searching + '</div>');
            
            $.ajax({
                url: themisdbGalleryAdmin.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_gallery_search',
                    nonce: themisdbGalleryAdmin.nonce,
                    query: query,
                    provider: provider
                },
                success: function(response) {
                    $btn.prop('disabled', false).text('Suchen');
                    
                    if (response.success && response.data.images) {
                        displayResults(response.data.images);
                    } else {
                        $results.html('<div class="themisdb-admin-error">' + (response.data.message || themisdbGalleryAdmin.noResults) + '</div>');
                    }
                },
                error: function() {
                    $btn.prop('disabled', false).text('Suchen');
                    $results.html('<div class="themisdb-admin-error">' + themisdbGalleryAdmin.error + '</div>');
                }
            });
        }
        
        /**
         * Generate AI image
         */
        function generateAIImage(prompt) {
            var $results = $('#themisdb-gallery-results');
            var $btn = $('#themisdb-gallery-ai-btn');
            
            $btn.prop('disabled', true).text('Generiere...');
            $results.html('<div class="themisdb-admin-loading">Bild wird generiert, bitte warten...</div>');
            
            $.ajax({
                url: themisdbGalleryAdmin.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_gallery_generate_ai',
                    nonce: themisdbGalleryAdmin.nonce,
                    prompt: prompt
                },
                success: function(response) {
                    $btn.prop('disabled', false).text('AI Generieren');
                    
                    if (response.success && response.data.image) {
                        displayResults([response.data.image]);
                    } else {
                        $results.html('<div class="themisdb-admin-error">' + (response.data.message || themisdbGalleryAdmin.error) + '</div>');
                    }
                },
                error: function() {
                    $btn.prop('disabled', false).text('AI Generieren');
                    $results.html('<div class="themisdb-admin-error">' + themisdbGalleryAdmin.error + '</div>');
                }
            });
        }
        
        /**
         * Display search results
         */
        function displayResults(images) {
            var $results = $('#themisdb-gallery-results');
            
            if (!images || images.length === 0) {
                $results.html('<div class="themisdb-admin-error">' + themisdbGalleryAdmin.noResults + '</div>');
                return;
            }
            
            var html = '<div class="themisdb-admin-image-grid">';
            
            images.forEach(function(image) {
                html += '<div class="themisdb-admin-image-item" data-image=\'' + JSON.stringify(image) + '\'>';
                html += '<img src="' + escapeHtml(image.thumb) + '" alt="' + escapeHtml(image.title) + '" />';
                html += '<button class="themisdb-insert-image-btn">' + themisdbGalleryAdmin.insertImage + '</button>';
                html += '<div class="themisdb-admin-image-overlay">';
                html += '<div class="themisdb-admin-image-source">' + escapeHtml(image.source) + '</div>';
                html += '<div>' + escapeHtml(image.author) + '</div>';
                html += '</div>';
                html += '</div>';
            });
            
            html += '</div>';
            $results.html(html);
            
            // Attach click handlers
            $('.themisdb-insert-image-btn').on('click', function(e) {
                e.stopPropagation();
                var $item = $(this).closest('.themisdb-admin-image-item');
                var imageData = $item.data('image');
                insertImage(imageData, $(this));
            });
        }
        
        /**
         * Insert image into post
         */
        function insertImage(imageData, $btn) {
            var postId = $('#post_ID').val() || 0;
            
            $btn.prop('disabled', true).addClass('loading').text(themisdbGalleryAdmin.downloading);
            
            $.ajax({
                url: themisdbGalleryAdmin.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_gallery_import_image',
                    nonce: themisdbGalleryAdmin.nonce,
                    image_data: JSON.stringify(imageData),
                    post_id: postId
                },
                success: function(response) {
                    if (response.success) {
                        // Insert into editor
                        var html = '<figure class="wp-block-image">';
                        html += '<img src="' + response.data.url + '" alt="' + imageData.title + '" />';
                        if (response.data.attribution) {
                            html += '<figcaption>' + response.data.attribution + '</figcaption>';
                        }
                        html += '</figure>';
                        
                        // Try to insert into Gutenberg or Classic Editor
                        if (wp.data && wp.data.dispatch('core/editor')) {
                            // Gutenberg
                            var blocks = wp.blocks.parse(html);
                            wp.data.dispatch('core/editor').insertBlocks(blocks);
                        } else if (typeof tinymce !== 'undefined' && tinymce.activeEditor) {
                            // Classic Editor
                            tinymce.activeEditor.insertContent(html);
                        }
                        
                        $btn.removeClass('loading').text('✓ Eingefügt');
                        setTimeout(function() {
                            $btn.prop('disabled', false).text(themisdbGalleryAdmin.insertImage);
                        }, 2000);
                        
                    } else {
                        alert(response.data.message || themisdbGalleryAdmin.error);
                        $btn.prop('disabled', false).removeClass('loading').text(themisdbGalleryAdmin.insertImage);
                    }
                },
                error: function() {
                    alert(themisdbGalleryAdmin.error);
                    $btn.prop('disabled', false).removeClass('loading').text(themisdbGalleryAdmin.insertImage);
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
