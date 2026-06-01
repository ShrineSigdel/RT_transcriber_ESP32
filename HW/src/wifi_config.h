/*
 * wifi_config.h
 * =============
 * THE ONLY FILE YOU NEED TO EDIT before flashing.
 *
 * Fill in your WiFi credentials and your laptop's LAN IP address.
 * Everything else is auto-configured from these three values.
 *
 * How to find your laptop's LAN IP
 * ----------------------------------
 *   Linux / Mac:   ip route get 1 | awk '{print $7; exit}'
 *   Windows:       ipconfig | findstr "IPv4"
 *
 * The IP must be on the same subnet as your ESP32.
 * Example: laptop = 192.168.1.42  →  WS_HOST = "192.168.1.42"
 */

#pragma once

// ── WiFi ──────────────────────────────────────────────────────────────────────
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ── Server (your laptop running server.py) ────────────────────────────────────
#define WS_HOST "YOUR_LAN_IP" // Change it to your laptop's LAN IP address
#define WS_PORT 8000
#define WS_PATH "/ws"
