/*
 * main.cpp
 * ========
 * Entry point — wires the four modules together.
 *
 * Module responsibilities
 * -----------------------
 *   display.*      SH1106 OLED via U8g2 (I2C GPIO 21/22)
 *   text_buffer.*  word-wrap + 8-line scrolling buffer
 *   ws_client.*    Links2004/WebSockets client
 *   wifi_config.h  credentials — the only file you edit
 *
 * Boot sequence
 * -------------
 *   display_init()   OLED ready, can show status
 *   tbuf_init()      clear line buffer
 *   WiFi connect     blocking; shows progress on OLED
 *   wsc_begin()      non-blocking; connection result arrives as WS event
 *
 * Loop
 * ----
 *   wsc_loop()              process frames + drive reconnect timer
 *   wsc_ensure_connected()  update OLED if link is down
 *
 * Data flow on text arrival
 * -------------------------
 *   wsc_loop() → _on_event(WStype_TEXT) → tbuf_push_text() → display_render()
 */

#include <Arduino.h>
#include <WiFi.h>

#include "wifi_config.h"
#include "display.h"
#include "text_buffer.h"
#include "ws_client.h"

const int LED_PIN = 23;

// ─────────────────────────────────────────────────────────────────────────────
static void connect_wifi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("[wifi] connecting");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("[wifi] connected — IP: ");
    Serial.println(WiFi.localIP());
}

// ─────────────────────────────────────────────────────────────────────────────
void setup()
{

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    Serial.begin(115200);
    Serial.println("\n[boot] LiveVoice OLED");

    display_init();
    display_status("LiveVoice", "Booting...");

    tbuf_init();

    display_status("WiFi", "Connecting...");
    connect_wifi();
    display_status("WiFi OK", WiFi.localIP().toString().c_str());
    delay(800);

    // wsc_begin() is non-blocking — WStype_CONNECTED fires inside wsc_loop()
    display_status("Server", "Connecting...");
    wsc_begin();
}

// ─────────────────────────────────────────────────────────────────────────────
void loop()
{
    wsc_loop();             // process frames; drives reconnect timer
    wsc_ensure_connected(); // update OLED if disconnected
    delay(10);              // small yield to prevent WDT issues
}
