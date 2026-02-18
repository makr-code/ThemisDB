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
            const item = $('<div>')
                .addClass('ppp-playlist-item')
                .attr('data-index', index);
            
            // Add thumbnail if available
            if (episode.thumbnail && episode.thumbnail.thumbnail) {
                const thumbnail = $('<div>')
                    .addClass('ppp-playlist-thumbnail')
                    .css('background-image', 'url(' + episode.thumbnail.thumbnail + ')');
                item.append(thumbnail);
            } else {
                // Default thumbnail placeholder
                const placeholder = $('<div>')
                    .addClass('ppp-playlist-thumbnail ppp-playlist-thumbnail-placeholder')
                    .html('<span>&#127911;</span>'); // Microphone emoji
                item.append(placeholder);
            }
            
            // Episode info container
            const info = $('<div>').addClass('ppp-playlist-info');
            
            const title = $('<div>')
                .addClass('ppp-playlist-title')
                .text(episode.title);
            
            const desc = $('<div>')
                .addClass('ppp-playlist-desc')
                .text(episode.desc ? episode.desc.substring(0, 80) + '...' : '');
            
            info.append(title);
            if (desc.text()) {
                info.append(desc);
            }
            
            item.append(info);
            
            // Play button overlay
            const playBtn = $('<button>')
                .addClass('ppp-playlist-play-btn')
                .html('&#9654;')
                .attr('title', 'Play');
            
            item.append(playBtn);
            
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
        $('#ppp-playlist-items').on('click', '.ppp-playlist-item, .ppp-playlist-play-btn', function(e) {
            e.stopPropagation();
            const item = $(this).hasClass('ppp-playlist-item') ? $(this) : $(this).closest('.ppp-playlist-item');
            const index = parseInt(item.attr('data-index'));
            selectEpisode(index);
        });
        
        // Progress bar click for seeking
        $('#ppp-progress-bar').on('click', function(e) {
            const bar = $(this);
            const clickX = e.pageX - bar.offset().left;
            const width = bar.width();
            const percentage = clickX / width;
            
            if (audio.duration) {
                audio.currentTime = audio.duration * percentage;
            }
        });
        
        // Audio ended event
        $(audio).on('ended', function() {
            playNext();
        });
        
        // Audio time update (for progress bar and time display)
        $(audio).on('timeupdate', function() {
            updateProgress();
            saveState();
        });
        
        // Audio metadata loaded (for duration)
        $(audio).on('loadedmetadata', function() {
            updateTotalTime();
        });
    }
    
    /**
     * Update progress bar and current time
     */
    function updateProgress() {
        if (!audio.duration) return;
        
        const percentage = (audio.currentTime / audio.duration) * 100;
        $('#ppp-progress-fill').css('width', percentage + '%');
        
        $('#ppp-current-time').text(formatTime(audio.currentTime));
    }
    
    /**
     * Update total time display
     */
    function updateTotalTime() {
        if (audio.duration) {
            $('#ppp-total-time').text(formatTime(audio.duration));
        }
    }
    
    /**
     * Format time in MM:SS or HH:MM:SS
     */
    function formatTime(seconds) {
        if (isNaN(seconds) || seconds === 0) return '0:00';
        
        const hours = Math.floor(seconds / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        const secs = Math.floor(seconds % 60);
        
        if (hours > 0) {
            return hours + ':' + pad(minutes) + ':' + pad(secs);
        }
        return minutes + ':' + pad(secs);
    }
    
    /**
     * Pad number with leading zero
     */
    function pad(num) {
        return num < 10 ? '0' + num : num;
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
