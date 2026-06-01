"""
config.py
=========
Central configuration for the LiveVoice server.
Change values here — never hunt through other files.

    from config import settings
"""

import os
from dataclasses import dataclass


@dataclass
class Settings:

    # ── Groq API ──────────────────────────────────────────────────────────────
    # Get a free key at https://console.groq.com/keys
    # Set in your shell:  export GROQ_API_KEY="gsk_..."
    groq_api_key: str = os.environ.get("GROQ_API_KEY", "")

    # "whisper-large-v3-turbo" → fastest, free tier
    # "whisper-large-v3"       → more accurate, slower
    groq_model: str = "whisper-large-v3-turbo"

    # BCP-47 language code: "en", "ne", "hi" — or "" for auto-detect
    language: str = "en"

    # ── Server ────────────────────────────────────────────────────────────────
    # "0.0.0.0" = accept from any device on the network (needed for ESP32)
    host: str = "0.0.0.0"
    port: int = 8000
    reload: bool = True   # set False in production

    # ── ESP32 info (not used by server code — just documents what to flash) ──
    # The ESP32 connects to ws://<your-laptop-LAN-IP>:8000/ws
    # It is a plain WebSocket client, identical to the browser tab.
    # No server changes needed — broadcast() already sends to every client.
    #
    # To find your LAN IP (the value to put in wifi_config.h on the ESP32):
    #   Linux/Mac:  ip route get 1 | awk '{print $7; exit}'
    #   Windows:    ipconfig | findstr "IPv4"


# Single shared instance
settings = Settings()
