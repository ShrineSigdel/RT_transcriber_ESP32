/*
 * ws_client.h
 * ===========
 * Public interface for the WebSocket client module.
 * Implementation uses Links2004/WebSockets (WebSocketsClient.h).
 *
 * Key difference from ArduinoWebsockets
 * --------------------------------------
 * Links2004 uses an EVENT-DRIVEN model, not a return-value model:
 *
 *   ArduinoWebsockets:              Links2004/WebSockets:
 *   ─────────────────────────────   ──────────────────────────────────
 *   client.connect() → bool         webSocket.begin()  (void, async)
 *   client.available() → bool       WStype_DISCONNECTED event fires
 *   client.poll()                   webSocket.loop()
 *   callback(WebsocketsMessage)     callback(WStype_t, uint8_t*, size_t)
 *
 * Auto-reconnect is BUILT INTO the library via setReconnectInterval().
 * wsc_ensure_connected() still exists for showing OLED status on drops,
 * but the actual TCP reconnect happens automatically inside webSocket.loop().
 *
 * Connection lifecycle
 * --------------------
 *   setup()  →  wsc_begin()           register callback + begin connection
 *   loop()   →  wsc_loop()            process frames, drive reconnect timer
 *            →  wsc_ensure_connected  update OLED status if link dropped
 */

#pragma once

/*
 * wsc_begin()
 * Register the event callback and initiate the connection to
 * WS_HOST:WS_PORT/WS_PATH (defined in wifi_config.h).
 * Also sets the auto-reconnect interval to 5 s.
 * Does NOT block — connection result arrives as WStype_CONNECTED event.
 */
void wsc_begin();

/*
 * wsc_loop()
 * Must be called every loop().
 * Drives the TCP socket, fires event callbacks for arriving frames,
 * and handles the reconnect timer if the connection dropped.
 * Non-blocking — returns immediately if nothing is pending.
 */
void wsc_loop();

/*
 * wsc_ensure_connected()
 * Show an OLED status message when the connection is known to be down.
 * The actual reconnect is handled automatically inside wsc_loop() via
 * setReconnectInterval — this function only manages the display.
 */
void wsc_ensure_connected();
