/**
 * ThemisDB LLM Integration - Frontend JavaScript
 */

(function($) {
    'use strict';
    
    /**
     * Chat Widget Handler
     */
    function initChatWidget() {
        const $chatSend = $('#themis-chat-send');
        const $chatText = $('#themis-chat-text');
        const $chatMessages = $('#themis-chat-messages');
        
        if (!$chatSend.length) return;
        
        // Send message on button click
        $chatSend.on('click', function() {
            sendChatMessage();
        });
        
        // Send message on Enter key
        $chatText.on('keypress', function(e) {
            if (e.which === 13) {
                sendChatMessage();
            }
        });
        
        function sendChatMessage() {
            const query = $chatText.val().trim();
            if (!query) return;
            
            // Add user message to chat
            $chatMessages.append(
                '<div class="themis-chat-message user">' + 
                escapeHtml(query) + 
                '</div>'
            );
            
            // Clear input
            $chatText.val('');
            
            // Show loading indicator
            const $loading = $('<div class="themis-chat-message ai">' +
                '<span class="themis-loading"></span> Thinking...</div>');
            $chatMessages.append($loading);
            
            // Scroll to bottom
            $chatMessages.scrollTop($chatMessages[0].scrollHeight);
            
            // Send AJAX request
            $.ajax({
                url: themisdb.ajax_url,
                method: 'POST',
                data: {
                    action: 'themis_rag_query',
                    nonce: themisdb.rag_nonce,
                    query: query
                },
                success: function(response) {
                    $loading.remove();
                    
                    if (response.success) {
                        const answer = response.data.answer;
                        $chatMessages.append(
                            '<div class="themis-chat-message ai">' + 
                            escapeHtml(answer) + 
                            '</div>'
                        );
                    } else {
                        $chatMessages.append(
                            '<div class="themis-chat-message ai" style="background:#e74c3c;">' +
                            'Error: ' + escapeHtml(response.data || 'Unknown error') +
                            '</div>'
                        );
                    }
                    
                    // Scroll to bottom
                    $chatMessages.scrollTop($chatMessages[0].scrollHeight);
                },
                error: function() {
                    $loading.remove();
                    $chatMessages.append(
                        '<div class="themis-chat-message ai" style="background:#e74c3c;">' +
                        'Network error. Please try again.' +
                        '</div>'
                    );
                    $chatMessages.scrollTop($chatMessages[0].scrollHeight);
                }
            });
        }
    }
    
    /**
     * Search Widget Handler
     */
    function initSearchWidget() {
        const $searchButton = $('#themis-search-button');
        const $searchInput = $('#themis-search-input');
        const $searchResults = $('#themis-search-results');
        
        if (!$searchButton.length) return;
        
        // Search on button click
        $searchButton.on('click', function() {
            performSearch();
        });
        
        // Search on Enter key
        $searchInput.on('keypress', function(e) {
            if (e.which === 13) {
                performSearch();
            }
        });
        
        function performSearch() {
            const query = $searchInput.val().trim();
            if (!query) return;
            
            // Show loading
            $searchResults.html('<div class="themis-loading"></div> Searching...');
            
            // Send AJAX request
            $.ajax({
                url: themisdb.ajax_url,
                method: 'POST',
                data: {
                    action: 'themis_semantic_search',
                    nonce: themisdb.search_nonce,
                    query: query
                },
                success: function(response) {
                    if (response.success) {
                        displaySearchResults(response.data);
                    } else {
                        $searchResults.html(
                            '<p style="color:#e74c3c;">Error: ' + 
                            escapeHtml(response.data || 'Unknown error') + 
                            '</p>'
                        );
                    }
                },
                error: function() {
                    $searchResults.html(
                        '<p style="color:#e74c3c;">Network error. Please try again.</p>'
                    );
                }
            });
        }
        
        function displaySearchResults(posts) {
            if (!posts || posts.length === 0) {
                $searchResults.html('<p>No results found.</p>');
                return;
            }
            
            let html = '<div class="themis-search-results-list">';
            
            posts.forEach(function(post) {
                const score = (post.score * 100).toFixed(1);
                html += '<div class="themis-search-result">';
                html += '<h3><a href="' + escapeHtml(post.url) + '">' + 
                        escapeHtml(post.title) + '</a>';
                html += '<span class="score">' + score + '% match</span></h3>';
                html += '<div class="excerpt">' + escapeHtml(post.excerpt) + '</div>';
                html += '<div class="meta">Relevance: ' + score + '%</div>';
                html += '</div>';
            });
            
            html += '</div>';
            $searchResults.html(html);
        }
    }
    
    /**
     * Escape HTML to prevent XSS
     */
    function escapeHtml(text) {
        const map = {
            '&': '&amp;',
            '<': '&lt;',
            '>': '&gt;',
            '"': '&quot;',
            "'": '&#039;'
        };
        return text.replace(/[&<>"']/g, function(m) { return map[m]; });
    }
    
    /**
     * Initialize on document ready
     */
    $(document).ready(function() {
        initChatWidget();
        initSearchWidget();
    });
    
})(jQuery);
