/**
 * ThemisDB Downloads Plugin - Frontend JavaScript
 */

(function($) {
    'use strict';
    
    $(document).ready(function() {
        
        // Initialize Mermaid for diagram rendering
        if (typeof mermaid !== 'undefined') {
            mermaid.initialize({
                startOnLoad: true,
                theme: 'default',
                securityLevel: 'strict',
                fontFamily: 'inherit'
            });
        }
        
        // Copy hash to clipboard
        $('.copy-hash-button').on('click', function(e) {
            e.preventDefault();
            
            var button = $(this);
            var hash = button.data('hash');
            
            // Copy to clipboard
            copyToClipboard(hash);
            
            // Visual feedback
            var originalText = button.text();
            button.text('✓ Kopiert!').addClass('copied');
            
            setTimeout(function() {
                button.text(originalText).removeClass('copied');
            }, 2000);
        });
        
        // File verification
        $('#themisdb-verify-button').on('click', function(e) {
            e.preventDefault();
            
            var fileInput = document.getElementById('themisdb-file-upload');
            var expectedHashInput = document.getElementById('themisdb-expected-hash');
            var resultDiv = $('#themisdb-verify-result');
            
            if (!fileInput.files.length) {
                showVerifyResult('error', 'Bitte wählen Sie eine Datei aus.');
                return;
            }
            
            if (!expectedHashInput.value.trim()) {
                showVerifyResult('error', 'Bitte geben Sie den erwarteten SHA256-Hash ein.');
                return;
            }
            
            var file = fileInput.files[0];
            var expectedHash = expectedHashInput.value.trim().toLowerCase();
            
            // Show loading state
            showVerifyResult('warning', '⏳ Berechne SHA256-Hash... Dies kann einen Moment dauern.');
            
            // Calculate hash
            calculateSHA256(file).then(function(calculatedHash) {
                if (calculatedHash === expectedHash) {
                    showVerifyResult('success', 
                        '✅ <strong>Verifizierung erfolgreich!</strong><br>' +
                        'Die Datei ist authentisch und wurde nicht manipuliert.<br>' +
                        'SHA256: ' + calculatedHash
                    );
                } else {
                    showVerifyResult('error', 
                        '❌ <strong>Verifizierung fehlgeschlagen!</strong><br>' +
                        'Die Checksums stimmen nicht überein.<br>' +
                        '<strong>Berechnet:</strong> ' + calculatedHash + '<br>' +
                        '<strong>Erwartet:</strong> ' + expectedHash + '<br>' +
                        '<strong>Warnung:</strong> Die Datei könnte beschädigt oder manipuliert sein. Bitte laden Sie sie erneut herunter.'
                    );
                }
            }).catch(function(error) {
                showVerifyResult('error', 'Fehler beim Berechnen des Hash: ' + error.message);
            });
        });
        
        /**
         * Show verification result
         */
        function showVerifyResult(type, message) {
            var resultDiv = $('#themisdb-verify-result');
            resultDiv.removeClass('success error warning').addClass(type);
            resultDiv.html(message);
        }
        
        /**
         * Calculate SHA256 hash of file
         */
        function calculateSHA256(file) {
            return new Promise(function(resolve, reject) {
                // Check for SubtleCrypto API support
                if (!window.crypto || !window.crypto.subtle) {
                    reject(new Error('SHA256-Berechnung wird in diesem Browser nicht unterstützt. Bitte verwenden Sie die Kommandozeile.'));
                    return;
                }
                
                var reader = new FileReader();
                
                reader.onload = function(e) {
                    var buffer = e.target.result;
                    
                    window.crypto.subtle.digest('SHA-256', buffer).then(function(hashBuffer) {
                        var hashArray = Array.from(new Uint8Array(hashBuffer));
                        var hashHex = hashArray.map(function(b) {
                            // Use slice for better browser compatibility instead of padStart
                            return ('0' + b.toString(16)).slice(-2);
                        }).join('');
                        
                        resolve(hashHex);
                    }).catch(reject);
                };
                
                reader.onerror = function() {
                    reject(new Error('Fehler beim Lesen der Datei'));
                };
                
                reader.readAsArrayBuffer(file);
            });
        }
        
        /**
         * Copy text to clipboard
         */
        function copyToClipboard(text) {
            if (navigator.clipboard && navigator.clipboard.writeText) {
                // Modern async clipboard API
                navigator.clipboard.writeText(text).catch(function(err) {
                    // Fallback to old method
                    fallbackCopy(text);
                });
            } else {
                // Fallback for older browsers
                fallbackCopy(text);
            }
        }
        
        /**
         * Fallback copy method
         */
        function fallbackCopy(text) {
            var textArea = document.createElement('textarea');
            textArea.value = text;
            textArea.style.position = 'fixed';
            textArea.style.left = '-999999px';
            textArea.style.top = '-999999px';
            document.body.appendChild(textArea);
            textArea.focus();
            textArea.select();
            
            try {
                document.execCommand('copy');
            } catch (err) {
                console.error('Fallback copy failed:', err);
            }
            
            document.body.removeChild(textArea);
        }
        
        // Hash click to copy
        $('.sha256-hash').on('click', function() {
            var hash = $(this).data('hash');
            copyToClipboard(hash);
            
            // Visual feedback
            var original = $(this).css('background-color');
            $(this).css('background-color', '#d7f5d7');
            
            setTimeout(function() {
                $(this).css('background-color', original);
            }.bind(this), 500);
        });
        
    });
    
})(jQuery);
