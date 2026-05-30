"""
server.py
=========
Application entry point.  Run this file to start the server:

    python server.py

This file is intentionally short — its only job is to:
  1. Create the FastAPI app
  2. Register middleware
  3. Attach the routes from routes.py
  4. Start uvicorn

All business logic lives in the other modules:
  config.py            → settings
  transcriber.py       → Groq Whisper wrapper
  websocket_manager.py → WebSocket connection pool
  routes.py            → HTTP + WebSocket endpoints
  static/index.html    → browser UI
"""

import uvicorn
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from config import settings
from routes import router


# ── Create the FastAPI application ────────────────────────────────────────────
app = FastAPI(
    title="LiveVoice",
    description="Real-time voice transcription via Groq Whisper",
    version="1.0.0",
)

# ── CORS middleware ────────────────────────────────────────────────────────────
# Allows the frontend (even if opened from a different origin) to call our API.
# In production you'd lock allow_origins down to your specific domain.
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# ── Register all routes defined in routes.py ──────────────────────────────────
app.include_router(router)


# ── Startup message ───────────────────────────────────────────────────────────
@app.on_event("startup")
async def on_startup():
    print("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print("  LiveVoice  |  Groq Whisper")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print(f"  Model   : {settings.groq_model}")
    print(f"  Language: {settings.language or 'auto-detect'}")
    print(f"  URL     : http://localhost:{settings.port}")

    if not settings.groq_api_key:
        print("\n  ⚠  GROQ_API_KEY is not set!")
        print("     export GROQ_API_KEY='gsk_...'")
        print("     Free key: https://console.groq.com/keys\n")
    else:
        print("\n  ✓  Groq API key loaded\n")


# ── Run ───────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    uvicorn.run(
        "server:app",          # module:attribute string — required for --reload
        host=settings.host,
        port=settings.port,
        reload=settings.reload,
    )
