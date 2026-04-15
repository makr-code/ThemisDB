"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            voice_assistant_example.py                         ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     356                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Voice Assistant - Example Usage

This example demonstrates how to use the ThemisDB Voice Assistant API
to record and transcribe phone calls, generate meeting protocols, and
interact with the database using voice commands.

Requirements:
    pip install requests

Usage:
    python voice_assistant_example.py
"""

import requests
import base64
import json
import os
from datetime import datetime
from pathlib import Path

# Configuration
THEMIS_URL = os.getenv("THEMIS_URL", "http://localhost:8080")
THEMIS_TOKEN = os.getenv("THEMIS_TOKEN", "your-jwt-token-here")

class ThemisVoiceAssistant:
    """Client for ThemisDB Voice Assistant API"""
    
    def __init__(self, base_url: str, token: str):
        self.base_url = base_url
        self.headers = {
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json"
        }
    
    def transcribe_audio(self, audio_file: str, language: str = "auto") -> dict:
        """Transcribe audio file to text"""
        with open(audio_file, "rb") as f:
            audio_data = f.read()
            audio_base64 = base64.b64encode(audio_data).decode()
        
        response = requests.post(
            f"{self.base_url}/api/v1/voice/transcribe",
            headers=self.headers,
            json={
                "audio_base64": audio_base64,
                "language": language,
                "timestamps": True,
                "speaker_diarization": False
            }
        )
        response.raise_for_status()
        return response.json()
    
    def synthesize_speech(self, text: str, voice: str = "default") -> bytes:
        """Convert text to speech"""
        response = requests.post(
            f"{self.base_url}/api/v1/voice/synthesize",
            headers=self.headers,
            json={
                "text": text,
                "voice": voice,
                "speed": 1.0,
                "format": "wav",
                "return_base64": True
            }
        )
        response.raise_for_status()
        result = response.json()
        return base64.b64decode(result["audio_base64"])
    
    def process_voice_command(self, text: str, session_id: str = "default") -> dict:
        """Process a voice command"""
        response = requests.post(
            f"{self.base_url}/api/v1/voice/command",
            headers=self.headers,
            json={
                "text": text,
                "session_id": session_id
            }
        )
        response.raise_for_status()
        return response.json()
    
    def record_phone_call(self, audio_file: str, caller: str, callee: str, **kwargs) -> dict:
        """Record and transcribe a phone call"""
        with open(audio_file, "rb") as f:
            audio_data = f.read()
            audio_base64 = base64.b64encode(audio_data).decode()
        
        payload = {
            "audio_base64": audio_base64,
            "call_id": kwargs.get("call_id", f"call-{datetime.now().strftime('%Y%m%d-%H%M%S')}"),
            "caller": caller,
            "callee": callee,
            "start_time": kwargs.get("start_time", int(datetime.now().timestamp() * 1000)),
            "end_time": kwargs.get("end_time", int(datetime.now().timestamp() * 1000)),
            "call_type": kwargs.get("call_type", "inbound"),
            "custom_fields": kwargs.get("custom_fields", {})
        }
        
        response = requests.post(
            f"{self.base_url}/api/v1/voice/call/record",
            headers=self.headers,
            json=payload
        )
        response.raise_for_status()
        return response.json()
    
    def generate_meeting_protocol(self, audio_file: str, title: str, participants: list, **kwargs) -> dict:
        """Generate a meeting protocol from audio"""
        with open(audio_file, "rb") as f:
            audio_data = f.read()
            audio_base64 = base64.b64encode(audio_data).decode()
        
        payload = {
            "audio_base64": audio_base64,
            "meeting_id": kwargs.get("meeting_id", f"meeting-{datetime.now().strftime('%Y%m%d-%H%M%S')}"),
            "title": title,
            "start_time": kwargs.get("start_time", int(datetime.now().timestamp() * 1000)),
            "end_time": kwargs.get("end_time", int(datetime.now().timestamp() * 1000)),
            "organizer": kwargs.get("organizer", ""),
            "participants": participants,
            "custom_fields": kwargs.get("custom_fields", {})
        }
        
        response = requests.post(
            f"{self.base_url}/api/v1/voice/meeting/protocol",
            headers=self.headers,
            json=payload
        )
        response.raise_for_status()
        return response.json()
    
    def get_stats(self) -> dict:
        """Get voice assistant statistics"""
        response = requests.get(
            f"{self.base_url}/api/v1/voice/stats",
            headers=self.headers
        )
        response.raise_for_status()
        return response.json()


def example_1_transcribe_audio():
    """Example 1: Transcribe an audio file"""
    print("\n" + "="*60)
    print("Example 1: Transcribe Audio File")
    print("="*60)
    
    assistant = ThemisVoiceAssistant(THEMIS_URL, THEMIS_TOKEN)
    
    # Note: Replace with actual audio file
    # result = assistant.transcribe_audio("sample_audio.mp3", language="en")
    # print(f"Transcript: {result['text']}")
    # print(f"Language: {result['language']}")
    # print(f"Confidence: {result['confidence']:.2%}")
    
    print("Note: This example requires an actual audio file.")
    print("Usage: result = assistant.transcribe_audio('audio.mp3')")


def example_2_synthesize_speech():
    """Example 2: Convert text to speech"""
    print("\n" + "="*60)
    print("Example 2: Text-to-Speech Synthesis")
    print("="*60)
    
    assistant = ThemisVoiceAssistant(THEMIS_URL, THEMIS_TOKEN)
    
    text = "Hello! Welcome to ThemisDB Voice Assistant."
    print(f"Synthesizing: {text}")
    
    # audio_data = assistant.synthesize_speech(text, voice="default")
    # with open("output.wav", "wb") as f:
    #     f.write(audio_data)
    # print("Audio saved to output.wav")
    
    print("Note: Uncomment the code above to generate audio.")


def example_3_voice_command():
    """Example 3: Process voice command"""
    print("\n" + "="*60)
    print("Example 3: Voice Command Processing")
    print("="*60)
    
    assistant = ThemisVoiceAssistant(THEMIS_URL, THEMIS_TOKEN)
    
    commands = [
        "Show me the total revenue for this month",
        "List the top 10 customers by sales",
        "What was our best selling product last week?"
    ]
    
    session_id = "demo-session"
    
    for command in commands:
        print(f"\nUser: {command}")
        # result = assistant.process_voice_command(command, session_id)
        # print(f"Assistant: {result['response']}")
    
    print("\nNote: Uncomment the code above to process commands.")


def example_4_phone_call_recording():
    """Example 4: Record and transcribe phone call"""
    print("\n" + "="*60)
    print("Example 4: Phone Call Recording")
    print("="*60)
    
    assistant = ThemisVoiceAssistant(THEMIS_URL, THEMIS_TOKEN)
    
    # result = assistant.record_phone_call(
    #     audio_file="call_recording.mp3",
    #     caller="+1234567890",
    #     callee="+0987654321",
    #     call_type="inbound",
    #     custom_fields={
    #         "department": "Customer Support",
    #         "category": "Technical Issue",
    #         "priority": "high"
    #     }
    # )
    # 
    # print(f"Call ID: {result['call_id']}")
    # print(f"Duration: {result['duration_ms'] / 1000 / 60:.1f} minutes")
    # print(f"\nTranscript:\n{result['transcript']}")
    # print(f"\nSummary:\n{result['summary']}")
    # print(f"\nStored as: {result['document_id']}")
    
    print("Note: This example requires an actual call recording.")
    print("The recording will be transcribed, summarized, and stored in ThemisDB")
    print("with full revision control and audit logging.")


def example_5_meeting_protocol():
    """Example 5: Generate meeting protocol"""
    print("\n" + "="*60)
    print("Example 5: Meeting Protocol Generation")
    print("="*60)
    
    assistant = ThemisVoiceAssistant(THEMIS_URL, THEMIS_TOKEN)
    
    # result = assistant.generate_meeting_protocol(
    #     audio_file="meeting_recording.wav",
    #     title="Q4 Planning Meeting",
    #     participants=[
    #         "john.doe@company.com",
    #         "jane.smith@company.com",
    #         "bob.jones@company.com"
    #     ],
    #     organizer="john.doe@company.com",
    #     custom_fields={
    #         "project": "Phoenix",
    #         "location": "Conference Room A"
    #     }
    # )
    # 
    # print(f"Meeting: {result['title']}")
    # print(f"Duration: {result['duration_ms'] / 1000 / 60:.1f} minutes")
    # print(f"\nSummary:\n{result['summary']}")
    # print(f"\nKey Points:")
    # for point in result['key_points']:
    #     print(f"  - {point}")
    # print(f"\nAction Items:")
    # for item in result['action_items']:
    #     print(f"  [ ] {item['description']}")
    # print(f"\nStored as: {result['document_id']}")
    
    print("Note: This example requires an actual meeting recording.")
    print("The protocol will include:")
    print("  - Full transcript with timestamps")
    print("  - AI-generated summary")
    print("  - Key discussion points")
    print("  - Action items with assignments")


def example_6_statistics():
    """Example 6: Get voice assistant statistics"""
    print("\n" + "="*60)
    print("Example 6: Voice Assistant Statistics")
    print("="*60)
    
    assistant = ThemisVoiceAssistant(THEMIS_URL, THEMIS_TOKEN)
    
    # stats = assistant.get_stats()
    # print(json.dumps(stats, indent=2))
    
    print("Note: Statistics include:")
    print("  - STT: transcriptions completed, processing time, real-time factor")
    print("  - TTS: syntheses completed, audio duration")
    print("  - LLM: tokens processed, cache hits, latency")
    print("  - Sessions: active session count")


def main():
    """Run all examples"""
    print("""
    ╔══════════════════════════════════════════════════════════╗
    ║      ThemisDB Voice Assistant - Example Usage           ║
    ╚══════════════════════════════════════════════════════════╝
    
    This script demonstrates the Voice Assistant API capabilities:
    - Speech-to-Text transcription
    - Text-to-Speech synthesis
    - Voice command processing
    - Phone call recording and transcription
    - Meeting protocol generation
    
    Note: Most examples are commented out and require actual audio files.
    Uncomment the code blocks to try them with your own data.
    """)
    
    # Run examples
    example_1_transcribe_audio()
    example_2_synthesize_speech()
    example_3_voice_command()
    example_4_phone_call_recording()
    example_5_meeting_protocol()
    example_6_statistics()
    
    print("\n" + "="*60)
    print("Examples completed!")
    print("="*60)
    print("\nFor more information, see:")
    print("  - docs/en/features/voice_assistant_guide.md")
    print("  - docs/de/features/sprachassistent_anleitung.md")
    print()


if __name__ == "__main__":
    main()
