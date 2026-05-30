"""
websocket_manager.py
====================
Manages the pool of active WebSocket connections.

Why a separate module?
  The server needs to push transcript text to *every* connected browser tab
  (and later, every ESP32 device).  This class keeps that list and handles
  the case where a client silently disappears mid-stream.

Usage:
    manager = ConnectionManager()

    # In a WebSocket endpoint:
    await manager.connect(websocket)

    # To push a message to all connected clients:
    await manager.broadcast("Hello everyone!")

    # On disconnect:
    manager.disconnect(websocket)
"""

from fastapi import WebSocket


class ConnectionManager:
    """Keeps track of all live WebSocket connections and broadcasts messages."""

    def __init__(self):
        # A plain Python set — fast O(1) add/remove, no duplicates possible.
        self._active: set[WebSocket] = set()

    async def connect(self, websocket: WebSocket) -> None:
        """
        Accept an incoming WebSocket handshake and register the connection.
        Must be called before trying to send any message to this client.
        """
        await websocket.accept()
        self._active.add(websocket)
        print(f"[ws] client connected  — {len(self._active)} total")

    def disconnect(self, websocket: WebSocket) -> None:
        """
        Remove a client from the pool.
        Safe to call even if the socket is not in the set (no-op).
        """
        self._active.discard(websocket)
        print(f"[ws] client disconnected — {len(self._active)} remaining")

    async def broadcast(self, message: str) -> None:
        """
        Send a text message to every connected client.

        Dead connections (network drop, tab close) raise an exception when
        we try to write to them.  We collect those and remove them *after*
        the loop so we never modify the set while iterating over it.
        """
        dead: set[WebSocket] = set()

        for websocket in self._active:
            try:
                await websocket.send_text(message)
            except Exception as exc:
                # Client is gone — mark for removal, don't crash the loop.
                print(f"[ws] dead connection removed: {exc}")
                dead.add(websocket)

        # Prune dead sockets now that iteration is complete.
        self._active -= dead

    @property
    def client_count(self) -> int:
        """How many clients are currently connected."""
        return len(self._active)
