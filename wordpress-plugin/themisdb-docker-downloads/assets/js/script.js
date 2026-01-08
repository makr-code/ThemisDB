/**
 * ThemisDB Docker Downloads - Frontend JavaScript
 */

(function($) {
    'use strict';
    
    $(document).ready(function() {
        // Copy command to clipboard
        $('.copy-command-btn, .copy-btn').on('click', function() {
            const command = $(this).data('command');
            copyToClipboard(command, $(this));
        });
        
        // Copy digest to clipboard
        $('.copy-digest-btn').on('click', function() {
            const digest = $(this).data('digest');
            copyToClipboard(digest, $(this));
        });
        
        /**
         * Copy text to clipboard
         * @param {string} text - Text to copy
         * @param {jQuery} button - Button element that was clicked
         */
        function copyToClipboard(text, button) {
            // Create temporary textarea
            const textarea = document.createElement('textarea');
            textarea.value = text;
            textarea.style.position = 'fixed';
            textarea.style.opacity = '0';
            document.body.appendChild(textarea);
            
            // Select and copy
            textarea.select();
            textarea.setSelectionRange(0, 99999); // For mobile devices
            
            try {
                document.execCommand('copy');
                
                // Show success feedback
                const originalText = button.text();
                button.text('✓ Copied!');
                button.addClass('copy-success');
                
                // Reset button after 2 seconds
                setTimeout(function() {
                    button.text(originalText);
                    button.removeClass('copy-success');
                }, 2000);
            } catch (err) {
                console.error('Failed to copy:', err);
                alert('Failed to copy to clipboard');
            }
            
            // Remove temporary textarea
            document.body.removeChild(textarea);
        }
        
        // Show full digest on click
        $('.digest-value').on('click', function() {
            const fullDigest = $(this).attr('title');
            alert('Full Digest:\n\n' + fullDigest);
        });
    });
    
})(jQuery);
