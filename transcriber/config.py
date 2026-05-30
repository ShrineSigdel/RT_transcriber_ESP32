"""
config.py
=========
Central configuration file for the LiveVoice transcription server.

All tunable settings live here so you never have to hunt through
multiple files to change a value.  Import this anywhere with:

    from config import settings
"""

import os
from dataclasses import dataclass


@dataclass
class Settings:
    # ── Groq API ──────────────────────────────────────────────────────────────
    # Your Groq API key.  Get one free at https://console.groq.com/keys
    # Set it as an environment variable so it never ends up in source code:
    #   export GROQ_API_KEY="gsk_..."
    groq_api_key: str = os.environ.get("GROQ_API_KEY", "")

    # Groq Whisper model to use.
    # "whisper-large-v3-turbo" is the fastest; good balance of speed & accuracy.
    # Other options: "whisper-large-v3" (slower, more accurate)
    groq_model: str = "whisper-large-v3-turbo"

    # Language hint passed to Whisper.
    # Use a BCP-47 code like "en", "ne" (Nepali), "hi" (Hindi), or "" for auto-detect.
    language: str = "en"

    # ── Server ────────────────────────────────────────────────────────────────
    # Host and port the FastAPI server listens on.
    # "0.0.0.0" means it accepts connections from any device on your network,
    # useful when you want the ESP32 or another device to reach it.
    host: str = "0.0.0.0"
    port: int = 8000

    # When True, uvicorn restarts the server on every file save.
    # Set to False in production.
    reload: bool = True


# Single shared instance — import this everywhere
settings = Settings()
