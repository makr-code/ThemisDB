# Voice Assistant Framework Sources
# Speech-to-Text, Text-to-Speech, and LLM-based voice interaction

if(THEMIS_ENABLE_VOICE_ASSISTANT)
    list(APPEND THEMIS_CORE_SOURCES
        # Voice Assistant Core
        ../src/voice/voice_assistant.cpp
        ../src/voice/voice_assistant_llm.cpp
        ../src/voice/audio_preprocessing.cpp
        ../src/voice/voice_intent_detector.cpp
        ../src/voice/voice_session_manager.cpp
        ../src/voice/voice_security.cpp
        ../src/voice/voice_error_handler.cpp
        ../src/voice/voice_tts_customizer.cpp
        ../src/voice/voice_meeting_support.cpp
        ../src/voice/voice_audio_storage.cpp
    )
endif()
