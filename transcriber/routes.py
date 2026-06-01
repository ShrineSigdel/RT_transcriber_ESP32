"""
routes.py
=========
All HTTP and WebSocket endpoints.

Endpoints
---------
GET  /            → Serve index.html
POST /transcribe  → Receive audio blob → Groq Whisper → broadcast via WS
WS   /ws          → Persistent connection; receives broadcast from /transcribe
GET  /status      → JSON health-check (useful to verify server is reachable
                    from ESP32 before the WS connection — curl or browser)

Data flow
---------
  Browser mic  →  POST /transcribe  →  Groq Whisper
                                              ↓
  Browser UI   ←  WS /ws broadcast  ←  manager.broadcast(text)
  ESP32 OLED   ←  WS /ws broadcast  ↗   (same broadcast, both clients)
"""

import asyncio
from pathlib import Path

from fastapi import APIRouter, WebSocket, WebSocketDisconnect, UploadFile, File
from fastapi.responses import HTMLResponse, JSONResponse

from websocket_manager import ConnectionManager
from transcriber import AudioTranscriber

# ── Singletons ────────────────────────────────────────────────────────────────
# Created once at import time, shared by every request.
manager     = ConnectionManager()
transcriber = AudioTranscriber()

router     = APIRouter()
STATIC_DIR = Path(__file__).parent / "static"


# ── GET / ─────────────────────────────────────────────────────────────────────
@router.get("/", response_class=HTMLResponse)
async def serve_frontend():
    """Serve the browser UI."""
    return HTMLResponse((STATIC_DIR / "index.html").read_text(encoding="utf-8"))


# ── GET /status ───────────────────────────────────────────────────────────────
@router.get("/status")
async def status():
    """
    Health-check endpoint.

    Returns the number of currently connected WebSocket clients so you can
    confirm the ESP32 (or browser) has successfully connected.

    Test from terminal:
        curl http://192.168.1.x:8000/status

    Expected response when ESP32 + browser are both connected:
        {"ok": true, "clients": 2}
    """
    return JSONResponse({
        "ok":      True,
        "clients": manager.client_count,
    })


# ── POST /transcribe ──────────────────────────────────────────────────────────
@router.post("/transcribe")
async def transcribe_audio(audio: UploadFile = File(...)):
    """
    Receive audio blob → transcribe → broadcast text to all WS clients.

    Request  : multipart/form-data, field "audio", audio/webm blob
    Response : {"text": "..."} on success | {"error": "..."} HTTP 500
    Side effect: manager.broadcast(text) → browser + ESP32 both receive it
    """
    audio_bytes = await audio.read()
    mime_type   = audio.content_type or "audio/webm"

    try:
        text = transcriber.transcribe(audio_bytes, mime_type)
    except RuntimeError as exc:
        print(f"[transcribe] error: {exc}")
        return JSONResponse({"error": str(exc)}, status_code=500)

    if text:
        print(f"[transcribe] → {text!r}  (→ {manager.client_count} client(s))")
        await manager.broadcast(text)

    return JSONResponse({"text": text})


# ── WS /ws ────────────────────────────────────────────────────────────────────
@router.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """
    Persistent WebSocket connection.

    Both the browser and the ESP32 connect here.
    This handler only keeps the connection alive — actual messages are
    pushed from /transcribe via manager.broadcast().

    asyncio.sleep(30) yields control to the event loop every 30 s so other
    coroutines (POST /transcribe) can run.  Without the yield the handler
    would block and broadcasts would queue up.
    """
    await manager.connect(websocket)
    try:
        while True:
            await asyncio.sleep(30)   # heartbeat yield — not a timer
    except WebSocketDisconnect:
        manager.disconnect(websocket)
