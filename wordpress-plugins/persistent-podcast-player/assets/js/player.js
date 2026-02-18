(function($) {
    'use strict';
    
    let episodes = [];
    let currentIndex = 0;
    let audio = null;
    
    // Initialize player on document ready
    $(document).ready(function() {
        audio = document.getElementById('ppp-audio');
        
        if (!audio) {
            console.error('Audio element not found');
            return;
        }
        
        // Fetch episodes from REST API
        fetchEpisodes();
        
        // Set up event listeners
        setupEventListeners();
    });
    
    /**
     * Fetch episodes from REST API
     */
    function fetchEpisodes() {
        if (!pppData || !pppData.restUrl) {
            console.error('REST URL not available');
            return;
        }
        
        $.ajax({
            url: pppData.restUrl,
            method: 'GET',
            success: function(data) {
                episodes = data;
                
                // Load state from localStorage (optional)
                loadState();
                
                renderPlaylist();
                
                // Load episode based on saved state or first episode if available
                if (episodes.length > 0) {
                    updateEpisodeUI(currentIndex, false);
                }
            },
            error: function(xhr, status, error) {
                console.error('Failed to fetch episodes:', error);
            }
        });
    }
    
    /**
     * Render playlist
     */
    function renderPlaylist() {
        const playlistContainer = $('#ppp-playlist-items');
        playlistContainer.empty();
        
        if (episodes.length === 0) {
            playlistContainer.html('<div class="ppp-playlist-empty">No episodes available</div>');
            return;
        }
        
        episodes.forEach(function(episode, index) {
            const item = $('<button>')
                .addClass('ppp-playlist-item')
                .attr('data-index', index)
                .text(episode.title);
            
            if (index === currentIndex) {
                item.addClass('ppp-active');
            }
            
            playlistContainer.append(item);
        });
    }
    
    /**
     * Set up event listeners
     */
    function setupEventListeners() {
        // Play/Pause button
        $('#ppp-play-pause').on('click', function() {
            if (audio.paused) {
                playAudio();
            } else {
                pauseAudio();
            }
        });
        
        // Previous button
        $('#ppp-prev').on('click', function() {
            playPrevious();
        });
        
        // Next button
        $('#ppp-next').on('click', function() {
            playNext();
        });
        
        // Playlist toggle
        $('#ppp-toggle-playlist').on('click', function() {
            $('#ppp-playlist').toggle();
        });
        
        // Playlist item click (delegated event)
        $('#ppp-playlist-items').on('click', '.ppp-playlist-item', function() {
            const index = parseInt($(this).attr('data-index'));
            selectEpisode(index);
        });
        
        // Audio ended event
        $(audio).on('ended', function() {
            playNext();
        });
        
        // Audio time update (for localStorage persistence)
        $(audio).on('timeupdate', function() {
            saveState();
        });
    }
    
    /**
     * Play audio
     */
    function playAudio() {
        if (!episodes[currentIndex] || !episodes[currentIndex].audio) {
            console.error('No audio URL available');
            return;
        }
        
        audio.play().then(function() {
            $('#ppp-play-pause').removeClass('ppp-btn-play').addClass('ppp-btn-pause');
        }).catch(function(error) {
            console.error('Failed to play audio:', error);
        });
    }
    
    /**
     * Pause audio
     */
    function pauseAudio() {
        audio.pause();
        $('#ppp-play-pause').removeClass('ppp-btn-pause').addClass('ppp-btn-play');
    }
    
    /**
     * Play previous episode
     */
    function playPrevious() {
        if (currentIndex > 0) {
            selectEpisode(currentIndex - 1);
        }
    }
    
    /**
     * Play next episode
     */
    function playNext() {
        if (currentIndex < episodes.length - 1) {
            selectEpisode(currentIndex + 1);
        } else {
            // Reached end of playlist
            pauseAudio();
        }
    }
    
    /**
     * Select and play an episode
     */
    function selectEpisode(index) {
        if (index < 0 || index >= episodes.length) {
            return;
        }
        
        currentIndex = index;
        updateEpisodeUI(index, true);
        renderPlaylist(); // Update active state
    }
    
    /**
     * Update episode UI
     */
    function updateEpisodeUI(index, autoplay) {
        const episode = episodes[index];
        
        if (!episode) {
            return;
        }
        
        // Update title
        $('#ppp-title').text(episode.title);
        
        // Update excerpt
        if (episode.excerpt) {
            $('#ppp-excerpt').text(episode.excerpt).show();
        } else {
            $('#ppp-excerpt').text('').hide();
        }
        
        // Update link
        const link = $('#ppp-link');
        if (episode.permalink) {
            link.attr('href', episode.permalink).show();
        } else {
            link.attr('href', '#').hide();
        }
        
        // Load audio
        if (episode.audio) {
            audio.src = episode.audio;
            audio.load();
            
            // Restore saved time for this episode (only once)
            if (!autoplay && typeof(Storage) !== 'undefined') {
                try {
                    const savedTime = localStorage.getItem('ppp_current_time');
                    if (savedTime !== null && !isNaN(savedTime)) {
                        $(audio).one('loadedmetadata', function() {
                            audio.currentTime = parseFloat(savedTime);
                        });
                    }
                } catch (e) {
                    // localStorage might be disabled
                }
            }
            
            if (autoplay) {
                playAudio();
            }
        }
    }
    
    /**
     * Save state to localStorage
     */
    function saveState() {
        if (typeof(Storage) !== 'undefined') {
            try {
                localStorage.setItem('ppp_current_index', currentIndex);
                localStorage.setItem('ppp_current_time', audio.currentTime);
            } catch (e) {
                // localStorage might be disabled
            }
        }
    }
    
    /**
     * Load state from localStorage
     */
    function loadState() {
        if (typeof(Storage) !== 'undefined') {
            try {
                const savedIndex = localStorage.getItem('ppp_current_index');
                
                if (savedIndex !== null) {
                    const index = parseInt(savedIndex);
                    if (index >= 0 && index < episodes.length) {
                        currentIndex = index;
                    }
                }
            } catch (e) {
                // localStorage might be disabled
            }
        }
    }
    
})(jQuery);
