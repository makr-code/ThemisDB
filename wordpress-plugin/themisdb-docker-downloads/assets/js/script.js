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
            // Try modern Clipboard API first
            if (navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText(text)
                    .then(function() {
                        showCopySuccess(button);
                    })
                    .catch(function(err) {
                        console.error('Clipboard API failed:', err);
                        // Fallback to execCommand
                        fallbackCopy(text, button);
                    });
            } else {
                // Fallback for older browsers
                fallbackCopy(text, button);
            }
        }
        
        /**
         * Fallback copy method using execCommand
         * @param {string} text - Text to copy
         * @param {jQuery} button - Button element that was clicked
         */
        function fallbackCopy(text, button) {
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
                showCopySuccess(button);
            } catch (err) {
                console.error('Failed to copy:', err);
                showCopyError(button);
            }
            
            // Remove temporary textarea
            document.body.removeChild(textarea);
        }
        
        /**
         * Show copy success feedback
         * @param {jQuery} button - Button element that was clicked
         */
        function showCopySuccess(button) {
            const originalText = button.text();
            button.text('✓ Copied!');
            button.addClass('copy-success');
            
            // Reset button after 2 seconds
            setTimeout(function() {
                button.text(originalText);
                button.removeClass('copy-success');
            }, 2000);
        }
        
        /**
         * Show copy error feedback
         * @param {jQuery} button - Button element that was clicked
         */
        function showCopyError(button) {
            const originalText = button.text();
            button.text('✗ Failed');
            button.addClass('copy-error');
            
            // Reset button after 2 seconds
            setTimeout(function() {
                button.text(originalText);
                button.removeClass('copy-error');
            }, 2000);
        }
        
        // Show full digest on click
        $('.digest-value').on('click', function() {
            const fullDigest = $(this).attr('title');
            const $digestDisplay = $('<div class="digest-fullscreen">')
                .html('<div class="digest-modal">' +
                      '<h3>Full Image Digest</h3>' +
                      '<code class="full-digest-code">' + fullDigest + '</code>' +
                      '<button class="copy-full-digest" data-digest="' + fullDigest + '">📋 Copy Digest</button>' +
                      '<button class="close-digest-modal">✕ Close</button>' +
                      '</div>')
                .hide()
                .appendTo('body')
                .fadeIn(200);
            
            // Close on button click
            $('.close-digest-modal').on('click', function() {
                $digestDisplay.fadeOut(200, function() {
                    $(this).remove();
                });
            });
            
            // Close on background click
            $('.digest-fullscreen').on('click', function(e) {
                if ($(e.target).hasClass('digest-fullscreen')) {
                    $(this).fadeOut(200, function() {
                        $(this).remove();
                    });
                }
            });
            
            // Copy full digest
            $('.copy-full-digest').on('click', function() {
                const digest = $(this).data('digest');
                copyToClipboard(digest, $(this));
            });
        });
    });
    
})(jQuery);
