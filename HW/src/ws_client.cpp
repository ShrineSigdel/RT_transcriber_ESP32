/*
 * ws_client.cpp
 * =============
 * Implements ws_client.h using Links2004/WebSockets (WebSocketsClient.h).
 *
 * API pattern for this library
 * ----------------------------
 *
 *  1. Declare a global WebSocketsClient (not a pointer, not on heap).
 *
 *  2. In setup():
 *       webSocket.begin(host, port, path)   — kick off the TCP connect
 *       webSocket.onEvent(callback)         — register event handler
 *       webSocket.setReconnectInterval(ms)  — auto-retry on drop
 *
 *  3. In loop():
 *       webSocket.loop()  — drives everything: reads socket, fires
 *                           callbacks, runs the reconnect timer
 *
 *  4. The callback signature is FIXED by the library:
 *       void fn(WStype_t type, uint8_t* payload, size_t length)
 *
 *     type is one of:
 *       WStype_DISCONNECTED  — TCP link gone
 *       WStype_CONNECTED     — handshake complete; payload = URL
 *       WStype_TEXT          — text frame; payload = null-terminated string
 *       WStype_BIN           — binary frame (we don't use this)
 *       WStype_PING / PONG   — keep-alive frames (library handles automatically)
 *       WStype_ERROR         — protocol error
 *
 * Why payload is uint8_t* not char*
 * -----------------------------------
 * The WS spec allows binary data in any frame, so the library uses
 * uint8_t* (unsigned byte pointer) as the common type.  For text frames
 * the bytes are valid UTF-8 and null-terminated, so casting to (char*)
 * or wrapping in String() is safe.
 *
 * Auto-reconnect
 * --------------
 * setReconnectInterval(5000) tells the library: "if the socket is closed,
 * attempt begin() again after 5 s."  This happens inside webSocket.loop()
 * with no code from us.  wsc_ensure_connected() only updates the OLED.
 */

#include "ws_client.h"
#include "wifi_config.h"
#include "text_buffer.h"
#include "display.h"
#include <WebSocketsClient.h>

// ── Module-private state ──────────────────────────────────────────────────────

// Global instance — Links2004 library requires this to be global (not local).
static WebSocketsClient _webSocket;

// Track connection state so wsc_ensure_connected() knows when to update OLED.
// The library manages the actual reconnect; we only track it for display.
static bool _connected = false;


// ─────────────────────────────────────────────────────────────────────────────
/*
 * _on_event() — internal event callback
 *
 * Called by _webSocket.loop() whenever something happens on the socket.
 * Handles the three states we care about: connected, disconnected, text.
 * All other event types (BIN, PING, PONG, ERROR, FRAGMENT_*) are silently
 * ignored — we don't need them for this project.
 */
static void _on_event(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {

        case WStype_CONNECTED:
            /*
             * Handshake succeeded.  payload contains the path string.
             * Update our state flag and clear the OLED status message
             * so the transcript buffer is visible again.
             */
            _connected = true;
            Serial.printf("[ws] connected to: %s\n", (char*)payload);
            display_status("Connected", "Listening...");
            delay(600);
            // Trigger a redraw of whatever is currently in the text buffer
            {
                String visible[DISPLAY_LINES_ON_SCREEN];
                int count = 0;
                tbuf_get_visible(visible, count);
                display_render(visible, count);
            }
            break;

        case WStype_DISCONNECTED:
            /*
             * TCP connection closed (server restarted, network dropped, etc.).
             * Mark state; wsc_ensure_connected() will show the OLED message.
             * The library's reconnect timer will call begin() again after
             * the interval set in wsc_begin().
             */
            _connected = false;
            Serial.println("[ws] disconnected");
            break;

        case WStype_TEXT:
            /*
             * A text frame arrived from the server.
             * payload is a null-terminated UTF-8 string — safe to wrap in String().
             * Pass it to the text buffer, which word-wraps and redraws the OLED.
             */
            if (length > 0) {
                String text = String((char*)payload);
                text.trim();
                if (text.length() > 0) {
                    Serial.print("[ws] received: ");
                    Serial.println(text);
                    tbuf_push_text(text);
                }
            }
            break;

        // All other event types ignored
        default:
            break;
    }
}


// ─────────────────────────────────────────────────────────────────────────────
void wsc_begin() {
    /*
     * begin() is non-blocking: it starts the TCP connect sequence but
     * returns immediately.  The WStype_CONNECTED event fires later
     * (typically within ~200ms on a local network) inside the first
     * few calls to loop().
     */
    _webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
    _webSocket.onEvent(_on_event);

    /*
     * Auto-reconnect: if the socket closes for any reason, the library
     * waits 5 s then calls begin() internally.  No code from us needed.
     */
    _webSocket.setReconnectInterval(5000);

    Serial.printf("[ws] begin → ws://%s:%d%s\n", WS_HOST, WS_PORT, WS_PATH);
}


// ─────────────────────────────────────────────────────────────────────────────
void wsc_loop() {
    /*
     * This single call does everything:
     *   - reads available bytes from the TCP socket
     *   - parses WebSocket frames
     *   - fires _on_event() for each complete frame
     *   - checks the reconnect timer and calls begin() if needed
     *
     * Must be called on every loop() iteration with no long delays between.
     * The library is not thread-safe — never call this from an ISR or task.
     */
    _webSocket.loop();
}


// ─────────────────────────────────────────────────────────────────────────────
void wsc_ensure_connected() {
    /*
     * Show an OLED message when disconnected.
     * The reconnect itself is handled by the library's internal timer —
     * we don't call begin() here, just update the display.
     *
     * We avoid spamming the OLED: once the message is shown it stays until
     * WStype_CONNECTED fires and overwrites it in _on_event().
     */
    static bool _status_shown = false;

    if (!_connected && !_status_shown) {
        display_status("WS dropped", "Auto-reconnect...");
        _status_shown = true;
        Serial.println("[ws] waiting for auto-reconnect");
    }

    if (_connected) {
        _status_shown = false;   // reset so next disconnect shows again
    }
}
