"""
routes.py
=========
Defines every HTTP endpoint and WebSocket handler for the application.

Endpoints
---------
GET  /            → Serve the frontend HTML page
POST /transcribe  → Receive audio, transcribe it, broadcast result via WebSocket
WS   /ws          → Persistent connection used to push text to the browser

How data flows through the app
-------------------------------
1.  Browser opens  WS /ws  and stays connected (receives-only).
2.  Browser POSTs a 2.5s audio blob to  POST /transcribe.
3.  /transcribe calls Groq Whisper via `transcriber.transcribe()`.
4.  If successful, calls `manager.broadcast(text)` → every WS client gets it.
5.  Browser renders the received text in the transcript panel.

This file imports `manager` and `transcriber` from the app package,
keeping it free of business logic.
"""

import asyncio
import os
from pathlib import Path

from fastapi import APIRouter, WebSocket, WebSocketDisconnect, UploadFile, File
from fastapi.responses import HTMLResponse, JSONResponse

from websocket_manager import ConnectionManager
from transcriber import AudioTranscriber

# ── Shared singletons ─────────────────────────────────────────────────────────
# One ConnectionManager for all WebSocket clients.
# One AudioTranscriber that holds the Groq client.
# Both are created once here and used by every request handler below.
manager = ConnectionManager()
transcriber = AudioTranscriber()

# ── Router ────────────────────────────────────────────────────────────────────
# APIRouter lets us keep routes in their own file.
# In server.py we do  app.include_router(router)  to register them all.
router = APIRouter()

# Path to the static folder that sits next to this file
STATIC_DIR = Path(__file__).parent / "static"


# ── Route: Serve the frontend ─────────────────────────────────────────────────
@router.get("/", response_class=HTMLResponse)
async def serve_frontend():
    """
    Serve index.html when a browser visits the root URL.

    FastAPI can serve static files automatically, but reading the file
    manually gives us a plain HTMLResponse without needing to mount a
    StaticFiles handler just for one file.
    """
    html_file = STATIC_DIR / "index.html"
    return HTMLResponse(html_file.read_text(encoding="utf-8"))


# ── Route: Transcribe audio ───────────────────────────────────────────────────
@router.post("/transcribe")
async def transcribe_audio(audio: UploadFile = File(...)):
    """
    Receive an audio blob from the browser, transcribe it, broadcast the text.

    Request
    -------
    Multipart form field `audio`:  audio/webm blob from MediaRecorder

    Response (JSON)
    ---------------
    { "text": "the transcribed words" }          on success
    { "error": "description" }                   on failure  (HTTP 500)

    Side effect
    -----------
    On success, `manager.broadcast(text)` is called so every connected
    WebSocket client (browser + future ESP32) receives the text immediately.
    """
    # Read the raw bytes from the uploaded file
    audio_bytes = await audio.read()

    # Get the MIME type the browser sent (e.g. "audio/webm;codecs=opus")
    mime_type = audio.content_type or "audio/webm"

    try:
        text = transcriber.transcribe(audio_bytes, mime_type)
    except RuntimeError as exc:
        # Log the error server-side and return a clear message to the client
        print(f"[transcribe] error: {exc}")
        return JSONResponse({"error": str(exc)}, status_code=500)

    # Only broadcast if Whisper actually returned something
    if text:
        print(f"[transcribe] → {text!r}")
        await manager.broadcast(text)

    return JSONResponse({"text": text})


# ── Route: WebSocket connection ───────────────────────────────────────────────
@router.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """
    Persistent WebSocket connection for pushing transcript text to clients.

    The browser connects here on page load and stays connected.
    This handler does nothing except keep the connection alive —
    all messages are pushed from /transcribe via manager.broadcast().

    The `asyncio.sleep(30)` loop acts as a heartbeat: it yields control
    back to the event loop regularly so FastAPI can process other requests,
    while keeping our coroutine alive so the connection stays open.

    WebSocketDisconnect is raised by FastAPI when the browser tab closes
    or the network drops — we catch it to cleanly remove the client.
    """
    await manager.connect(websocket)
    try:
        while True:
            # Keep the coroutine alive without busy-waiting.
            # 30 seconds is a safe heartbeat interval.
            await asyncio.sleep(30)
    except WebSocketDisconnect:
        manager.disconnect(websocket)
