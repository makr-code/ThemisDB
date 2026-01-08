/**
 * ThemisDB Docker Downloads - Admin JavaScript
 */

(function($) {
    'use strict';
    
    $(document).ready(function() {
        // Test Docker Hub connection
        $('#test-dockerhub-connection').on('click', function() {
            const button = $(this);
            const resultSpan = $('#connection-test-result');
            
            button.prop('disabled', true);
            resultSpan.html('<span class="loading-spinner"></span>');
            
            $.ajax({
                url: themisdbDockerAdmin.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_docker_test_connection',
                    nonce: themisdbDockerAdmin.nonce
                },
                success: function(response) {
                    if (response.success) {
                        resultSpan.html('<span class="success-message">✓ ' + response.data + '</span>');
                    } else {
                        resultSpan.html('<span class="error-message">✗ ' + response.data + '</span>');
                    }
                },
                error: function(xhr, status, error) {
                    resultSpan.html('<span class="error-message">✗ Connection failed: ' + error + '</span>');
                },
                complete: function() {
                    button.prop('disabled', false);
                }
            });
        });
        
        // Clear cache
        $('#clear-cache').on('click', function() {
            const button = $(this);
            const resultSpan = $('#clear-cache-result');
            
            button.prop('disabled', true);
            resultSpan.html('<span class="loading-spinner"></span>');
            
            $.ajax({
                url: themisdbDockerAdmin.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_docker_clear_cache',
                    nonce: themisdbDockerAdmin.nonce
                },
                success: function(response) {
                    if (response.success) {
                        resultSpan.html('<span class="success-message">✓ ' + response.data + '</span>');
                    } else {
                        resultSpan.html('<span class="error-message">✗ ' + response.data + '</span>');
                    }
                    
                    // Clear message after 3 seconds
                    setTimeout(function() {
                        resultSpan.html('');
                    }, 3000);
                },
                error: function(xhr, status, error) {
                    resultSpan.html('<span class="error-message">✗ Failed: ' + error + '</span>');
                },
                complete: function() {
                    button.prop('disabled', false);
                }
            });
        });
    });
    
})(jQuery);
