/**
 * ThemisDB Downloads Plugin - Admin JavaScript
 */

(function($) {
    'use strict';
    
    $(document).ready(function() {
        
        // Clear cache button
        $('#themisdb_clear_cache').on('click', function(e) {
            e.preventDefault();
            
            var button = $(this);
            var originalText = button.text();
            
            button.prop('disabled', true).text('Wird geleert...');
            
            $.ajax({
                url: themisdbAdmin.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_clear_cache',
                    nonce: themisdbAdmin.nonce
                },
                success: function(response) {
                    if (response.success) {
                        button.text('✓ Erfolgreich!');
                        
                        // Show success notice
                        $('<div class="notice notice-success is-dismissible"><p>' + response.data + '</p></div>')
                            .insertAfter('.themisdb-admin-header')
                            .delay(3000)
                            .fadeOut();
                        
                        setTimeout(function() {
                            button.prop('disabled', false).text(originalText);
                        }, 2000);
                    } else {
                        button.text('✗ Fehler');
                        
                        // Show error notice
                        $('<div class="notice notice-error is-dismissible"><p>' + response.data + '</p></div>')
                            .insertAfter('.themisdb-admin-header');
                        
                        setTimeout(function() {
                            button.prop('disabled', false).text(originalText);
                        }, 2000);
                    }
                },
                error: function(xhr, status, error) {
                    button.text('✗ Fehler').prop('disabled', false);
                    
                    // Show error notice
                    $('<div class="notice notice-error is-dismissible"><p>AJAX-Fehler: ' + error + '</p></div>')
                        .insertAfter('.themisdb-admin-header');
                    
                    setTimeout(function() {
                        button.text(originalText);
                    }, 2000);
                }
            });
        });
        
        // Toggle GitHub token visibility
        var tokenInput = $('#themisdb_github_token');
        if (tokenInput.length) {
            var toggleButton = $('<button type="button" class="button" style="margin-left: 0.5rem;">👁 Anzeigen</button>');
            tokenInput.after(toggleButton);
            
            toggleButton.on('click', function() {
                if (tokenInput.attr('type') === 'password') {
                    tokenInput.attr('type', 'text');
                    toggleButton.text('🙈 Verstecken');
                } else {
                    tokenInput.attr('type', 'password');
                    toggleButton.text('👁 Anzeigen');
                }
            });
        }
        
    });
    
})(jQuery);
